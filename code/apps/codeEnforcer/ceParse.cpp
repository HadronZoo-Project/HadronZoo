//
//	File:	ceParse.cpp
//
//	Legal Notice: This file is part of the HadronZoo::CodeEnforcer program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	HadronZoo::CodeEnforcer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
//	Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//	as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//	HadronZoo::CodeEnforcer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with HadronZoo::CodeEnforcer. If not, see http://www.gnu.org/licenses.
//

#include <iostream>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <execinfo.h>

#include "hzBasedefs.h"
#include "hzChars.h"
#include "hzTextproc.h"
#include "hzCtmpls.h"
#include "hzDirectory.h"
#include "hzDocument.h"
#include "hzProcess.h"

#include "enforcer.h"

using namespace std ;

/*
**	Variables
*/

extern	hzProcess	proc ;			//	Process instance (all HadronZoo apps need this)
extern	hzLogger	slog ;			//	Logfile
extern	ceProject	g_Project ;		//	Project (current)

extern	hzString	g_sysDfltDesc ;	//	Systemm entity default desciption
extern	bool		g_debug ;		//	Extra debugging (-b option)

Scope	s_opRange ;					//	Currently applicable operating scope

hzVect	<ceFunc*>	g_FuncDelay ;	//	Functions defined in class definition bodies for later processing
hzVect	<ceTarg*>	g_CurTmplArgs ;	//	Template arguments if any

/*
**	ceTyplex members
*/

/*
**	ceFile Functions
*/

hzEcode	ceFile::Init	(ceComp* pComp, const hzString& Path)
{
	//	Read in file, setup buffer and line pointers
	//
	//	Arguments:	1)	The project component to which the file is deemed in the config file to belong
	//				2)	The pathname of the file

	_hzfunc("ceFile::Init") ;

	static uint32_t	s_seq_ceFile = 0 ;

	hzChain::Iter	t ;

	FSTAT		fs ;			//	File status
	ifstream	is ;			//	Input stream
	hzString	S ;				//	Temp string
	const char*	i ;				//	Pathname iterator
	const char*	j ;				//	Pathname placeholder
	char		buf	[20] ;		//	Sequence number buffer
	hzEcode		rc = E_OK ;		//	Return code

	//	Check, open and load the file
	if (!pComp)
		hzexit(_fn, E_ARGUMENT, "No component") ;

	if (stat(*Path, &fs) == -1)
		hzexit(_fn, E_NOTFOUND, "%s ERROR File %s does not exist", *_fn, *Path) ;

	is.open(*Path) ;
	if (is.fail())
		{ slog.Out("%s ERROR Cannot open file (%s)\n", *_fn, *Path) ; return E_OPENFAIL ; }

	m_Content += is ;
	if (m_Content.Size() != fs.st_size)
	{
		slog.Out("%s ERROR File %s: Only %d of %d bytes read in\n", *_fn, *Path, m_Content.Size(), fs.st_size) ;
		is.close() ;
		return E_NODATA ;
	}
	is.close() ;

	//	Determine the type of file
	m_Path = Path ;
	i = strrchr(*Path, CHAR_FWSLASH) ;
	if (i)
		{ i++ ; S = i ; SetName(S) ; }
	else
		SetName(Path) ;

	j = strrchr(i, CHAR_PERIOD) ;
	if (j)
	{
		if		(strstr(j, ".h"))	m_Filetype = FILE_HEADER ;
		else if (strstr(j, ".cpp"))	m_Filetype = FILE_SOURCE ;
		else if (strstr(j, ".txt"))	m_Filetype = FILE_DOCUMENT ;
		else if (strstr(j, ".sys"))	m_Filetype = FILE_SYSINC ;
		else
			m_Filetype = FILE_UNKNOWN ;
	}

	sprintf(buf, "fi%04d", ++s_seq_ceFile) ;
	m_Docname = buf ;

	if (m_Filetype == FILE_UNKNOWN)
		{ slog.Out("%s ERROR File %s is neither a header or a source file\n", *_fn, *Path) ; return E_TYPE ; }

	m_pParComp = pComp ;
	slog.Out("%s. Component %s. Initialized file %s (%s)\n", *_fn, *m_pParComp->StrName(), *m_Path, *StrName()) ;

	return rc ;
}

hzEcode	ceFile::Activate	(void)
{
	//	File activation is simply to open the file and populate the array of original tokens. The pre-proceesor will later reduce these tokens to only
	//	those to be acted upon.

	_hzfunc("ceFile::Activate") ;

	ceToken		T ;				//	Individual token
	uint32_t	nIndex ;		//	Token iterator
	hzEcode		rc = E_OK ;		//	Return code

	if (P.Count())
		{ slog.Out("Activate file %s - already active\n", *m_Name) ; return E_DUPLICATE ; }

	rc = Tokenize(P, m_Content, m_Name) ;
	if (rc != E_OK)
		{ slog.Out("Activate File %s - Could not tokenize\n", *m_Name) ; return rc ; }

	rc = DeTab(m_Content) ;
	if (rc != E_OK)
		{ slog.Out("Could not de-tab file %s\n", *m_Name, P.Count()) ; return rc ; }

	slog.Out("%s. File %s activated with %d tokens\n", *_fn, *m_Name, P.Count()) ;

	//	Set token's orig posn value
	for (nIndex = 0 ; nIndex < P.Count() ; nIndex++)
		P[nIndex].orig = nIndex ;

	return rc ;
}

/*
**	Section X:	Entity Processing
*/

uint32_t	ceFile::ProcTmplBase	(uint32_t& ultimate, uint32_t start)
{
	//	Process a set of template arguments in a <> block.
	//
	//	If the vector g_CurTmplArgs is empty, this function will populated it. If g_CurTmplArgs is populated, any template arguments found within the template argument blocks, must
	//	match one that already exists.
	//
	//	Arguments:	1)	end		Reference to post process token position, set to one place beyond the closing > of the block.
	//				3)	start	The starting token which must be the '<' char.
	//
	//	Returns:	1)	-1 	If the '<' is not the start of a valid <...> block
	//				2)	The number of args if the operation was successful

	_hzfunc("ceFile::ProcTmplBase") ;

	ceTarg*		pTarg ;			//	Template argument
	uint32_t	ct ;			//	File token iterator
	uint32_t	count ;			//	Used to iterate existing args
	uint32_t	level ;			//	Used to match <>
	bool		bEmpty ;		//	True if g_CurTmplArgs is empty

	ct = ultimate = start ;

	if (X[ct].type != TOK_TEMPLATE)
		{ slog.Out("%s (%d) %s line %d col %d: Wrong call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ; return -1 ; }

	if (X[ct+1].type != TOK_OP_LESS)
		{ slog.Out("%s (%d) %s line %d col %d: Expected a <...> after 'template'\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ; return -1 ; }

	//	Find closing > char
	ct += 2 ;
	for (level = 1 ; level ; ct++)
	{
		if (X[ct].type == TOK_OP_LESS)	level++ ;
		if (X[ct].type == TOK_OP_MORE)	level-- ;
	}

	if (level)
	{
		slog.Out("%s (%d) %s line %d col %d: Template arg block malformed. <> not matched on line\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return -1 ;
	}
	ultimate = ct + 1 ;

	bEmpty = g_CurTmplArgs.Count() ? false : true ;

	for (ct = start + 2 ; ct < ultimate ; ct++)
	{
		if (X[ct].type == TOK_WORD || X[ct].type == TOK_CLASS)
		{
			if (X[ct].type == TOK_CLASS)
			{
				ct++ ;
				if (X[ct].type != TOK_WORD)
				{
					slog.Out("%s (%d) %s line %d col %d: Expected a generic class name in tmplate arg %s\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
					return -1 ;
				}
			}

			if (bEmpty)
			{
				//	Add the template arg
				pTarg = new ceTarg() ;
				pTarg->SetName(X[ct].value) ;
				slog.Out("%s (%d) %s line %d col %d: Added a template arg %s to arglist position %d\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pTarg->StrName(), g_CurTmplArgs.Count()) ;
				g_CurTmplArgs.Add(pTarg) ;
			}
			else
			{
				//	Check the template arg exists
				for (count = 0 ; count < g_CurTmplArgs.Count() ; count++)
				{
					pTarg = g_CurTmplArgs[count] ;
					if (pTarg->StrName() == X[ct].value)
						break ;
				}

				if (count == g_CurTmplArgs.Count())
				{
					slog.Out("%s (%d) %s line %d col %d: Ttemplate arg %s not found in existing arglist\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
					return 0 ;
				}
			}

			continue ;
		}

		if (X[ct].type == TOK_OP_MORE)
			break ;

		if (X[ct].type == TOK_SEP)
			continue ;

		slog.Out("%s (%d) %s line %d col %d: Unexpected token (%s) found while scanning template block\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[ct].Show()) ;
		return -1 ;
	}

	if (X[ct].type != TOK_OP_MORE)
	{
		slog.Out("%s (%d) %s line %d col %d: Templates argument block did not terminate at the '>' char\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return -1 ;
	}

	ultimate = ct + 1 ;
	return g_CurTmplArgs.Count() ;
}

/*
**	Main Parsing Section
*/

hzString	CTX_PARSE = "CTX_PARSE" ;	//	ProcStatement called from the file level
hzString	CTX_CLASS = "CTX_CLASS" ;	//	ProcStatement called from within ProcClass
hzString	CTX_CFUNC = "CTX_CFUNC" ;	//	ProcStatement called from within ProcFuncDef

hzEcode	ceFile::ProcStmtTypedef	(ceStmt& Stmt, ceKlass* pKlass, ceEntbl* pFET, uint32_t& ultimate, uint32_t start)
{
	//	Process a typedef statement.
	//
	//	A typedef statement is used to create a new data type that is based on an existing type OR to define a new type and then rename it. The latter use is not recommended under
	//	HadronZoo coding standards. Under any circumstances, the propsed new type name must not exist as an entity of any description. 
	//
	//	The purpose RE CodeEnforcer is mainly for presentation. Where a typedef has defined a new data type which is then used to define variables, function returns and arguments,
	//	then it should be the new rather than the established data type that appears in the output HTML.

	_hzfunc("ceFile::ProcStmtTypedef") ;

	ceTyplex	p_tpx ;					//	Primary typlex (in statments begining with [keywords] typlex ...
	hzString	obj_name ;				//	Name of entity being defined or declared.
	ceEntbl*	tgt_ET ;				//	Entity table of object
	ceEntity*	pEnt = 0 ;				//	Returned by Lookup in the current scope
	ceTypedef*	pTypedef = 0 ;			//	Entity is a typedef
	uint32_t	ct ;					//	Iterator, current token
	uint32_t	t_end ;					//	Set by various interim statement processors
	hzEcode		rc = E_OK ;				//	Return code

	ct = start ;

	if (pKlass)
	{
		//	Operating within a class/ctmpl definition
		tgt_ET = &(pKlass->m_ET) ;
		if (pFET)
			tgt_ET = pFET ;
	}
	else
	{
		//	Operating outside class/ctmpl definition. 
		tgt_ET = g_Project.m_pCurrNamsp ? &(g_Project.m_pCurrNamsp->m_ET) : &g_Project.m_RootET ;
	}

	if (X[ct].type != TOK_TYPEDEF)
		return E_SYNTAX ;

	ct++ ;
	if (X[ct].type == TOK_CLASS || X[ct].type == TOK_STRUCT)
		ct++ ;
	rc = GetTyplex(p_tpx, pKlass, pFET, t_end, ct) ;
	if (rc != E_OK)
	{
		slog.Out(g_EC[_hzlevel()+1]) ;
		slog.Out("%s (%d) %s line %d col %d: Cannot establish type for typedef conversion\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}
	ct = t_end ;

	if (X[ct].type != TOK_WORD)
	{
		slog.Out("%s (%d) %s line %d col %d: Expected ersatz type for typedef conversion. Got %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}
	if (X[ct+1].type != TOK_END)
	{
		slog.Out("%s (%d) %s line %d col %d: Expected ; after ersatz type to end typedef statement\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}

	obj_name = X[ct].value ;

	//	We have the real and the ersatz data type. It is essential the ersatz is not already defined in the entity table of the current scope as a data type
	//	(even it happens to be already set to the real type)
	pEnt = LookupToken(X, pKlass, pFET, t_end, ct, false, __func__) ;
	if (pEnt)
	{
		if (pEnt->Whatami() != ENTITY_TYPEDEF)
		{
			slog.Out("%s (%d) %s line %d col %d: ERROR: (TYPEDEF) Entity %s is already in use as a %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name, pEnt->EntDesc()) ;
			return E_SYNTAX ;
		}
		else
		{
			pTypedef = (ceTypedef*) pEnt ;
			slog.Out("%s (%d) %s line %d col %d: WARNING: (TYPEDEF) Entity %s is already defined in file %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name, pTypedef->DefFname()) ;
		}
	}
	ultimate = ct = t_end ;

	if (!pTypedef)
	{
		pTypedef = new ceTypedef() ;
		pTypedef->InitTypedef(this, p_tpx, obj_name) ;

		rc = tgt_ET->AddEntity(this, pTypedef, __func__) ;
		if (rc == E_OK)
			slog.Out("%s (%d) %s line %d col %d: Added TYPEDEF (%s)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
		else
		{
			slog.Out(g_EC[_hzlevel()+1]) ;
			return rc ;
		}

		Stmt.m_eSType = STMT_TYPEDEF ;
		Stmt.m_Object = obj_name ;
		Stmt.m_nLine = X[ct].line ;
		Stmt.m_End = ultimate ;
	}

	return rc ;
}

hzEcode	ceFile::ProcStatement	(hzVect<ceStmt>& statements, ceKlass* thisKlass, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, hzString ctx, uint32_t codeLevel)
{
	//	Process a single C++ statement.
	//
	//	This function is called repeatedly to process whole C++ statements found in one of four contexts as follows:-
	//
	//		1) CTX_PARSE.	That of a source or header file having statements laying outside any function, class or class template definition.
	//		2) CTX_CLASS.	That of a class (or struct)
	//		3) CTX_CTMPL.	That of a class/struct template
	//		4) CTX_CFUNC.	That of a function body appearing within a class/ctmpl definition
	//
	//	Note that in (1) the incidence of a function definition body is processed directly by a call to ProcFuncDef, while in (2) and (3) ProcFuncHead is called instead. The latter
	//	function only determines if the function is being declared or defined, thereby defering processing of the function body until later. The reason for this being that within a
	//	class or class template definition, statements within a member function body may legally refer to as yet undefined class members. Because of this, ProcClass processes these
	//	function bodies as a separate later step, once all members have been declared.
	//
	//	Arguments:	1)	statements	Vector of statement (either for the file, the class or the function), this function will add to
	//				2)	thisKlass	When called by ProcClass() or ProcCtmpl() this is the class or class template whose definition is being processed.
	//				3)	pFET		Function entity table (functions only)
	//				4)	ultimate	The postion of the last token found in the statement (usually a ;)
	//				5)	start		Starting token assumed to be at the begining of the statement
	//				6)	ctx			Called from Parse(), ProcClass() or ProcFuncDef()
	//				7)	codeLevel	Level of {} indent
	//
	//	Returns:	1) E_SYNTAX		In the event of a malformed or otherwise invalid statement
	//				2) E_NOTFOUND	In the event no valid statement form was established
	//				3) E_OK			If the statement was successfully processed

	_hzfunc("ceFile::ProcStatement") ;

	hzList	<ceVar*>::Iter	aI1 ;			//	Function arg iterator
	hzList	<ceVar*>::Iter	aI2 ;			//	Function arg iterator

	hzList	<ceVar*>	f_args ;			//	Function arguments
	hzList	<ceTyplex>	f_vals ;			//	Function arguments
	hzVect	<ceFunc*>	funcs ;				//	List of possible functions

	ceTyplex	theTpx ;					//	Evluation result
	hzAtom		theVal ;					//	Value set by expression assesment if any
	hzChain		Z ;							//	For reporting a series of connected tokens
	hzString	S ;							//	Temp string
	ceStmt		Stmt ;						//	Statement populated here
	hzString	obj_name ;					//	Name of entity being defined or declared.
	hzString	obj_fqname ;				//	For when we have a host class
	ceToken		tok ;						//	Current of reference token (start of statement)
	ceTyplex	p_tpx ;						//	Primary typlex (in statments begining with [keywords] typlex ...
	ceTyplex	s_tpx ;						//	Seconday typlex where statements involve further typlex
	ceEntbl*	tgt_ET ;					//	Entity table of object
	ceEntity*	pEnt = 0 ;					//	Returned by Lookup in the current scope
	ceNamsp*	pNamsp ;					//	Namespace identified by 'using' statement
	ceKlass*	tmpHost = 0 ;				//	Temp host (class/ctmpl)
	ceClass*	pClass = 0 ;				//	Class pointed to by current token if any
	ceVar*		pVar = 0 ;					//	Current variable
	ceFunc*		pFunc = 0 ;					//	Current function
	uint32_t	ct ;						//	Iterator, current token
	uint32_t	pt ;						//	Offset into file array P for comments
	uint32_t	t_end ;						//	Set by various interim statement processors
	uint32_t	match ;						//	For matching braces
	uint32_t	stm_limit ;					//	End of statement (either ; or end of line)
	bool		bRegister = false ;			//	posn of word 'register' if present
	bool		bExtern = false ;			//	posn of word 'extern' if present
	bool		bFriend = false ;			//	posn of word 'friend' if present
	bool		bVirtual = false ;			//	posn of word 'virtual' if present
	bool		bInline = false ;			//	posn of word 'inline' if present
	bool		bStatic = false ;			//	posn of word 'static' if present
	bool		bFunction = false ;			//	Set if keywords or other mean statement is function dcl or def
	bool		bVariable = false ;			//	Set if keywords or other mean statement is variable dcl
	bool		bTemplate = false ;			//	position of the word template if encountered
	Scope		opRange = SCOPE_GLOBAL ;	//	Applied to class members
	hzEcode		rc = E_OK ;					//	Return code

	/*
	**	Set up target entity table for new objects to be added to
	*/

	if (thisKlass)
	{
		//	Operating within a class/ctmpl definition

		if (thisKlass->Whatami() == ENTITY_CLASS)
			{ pClass = (ceClass*) thisKlass ; tgt_ET = &(pClass->m_ET) ; }
		else
			Fatal("%s. Supplied host class is not a class or a class template. It is a %s\n", __func__, __LINE__, Entity2Txt(thisKlass->Whatami())) ;

		if (pFET)
			tgt_ET = pFET ;
	}
	else
	{
		//	Operating outside class/ctmpl definition. 
		tgt_ET = g_Project.m_pCurrNamsp ? &(g_Project.m_pCurrNamsp->m_ET) : &g_Project.m_RootET ;

		if (pFET)
			tgt_ET = pFET ;
	}

	//	Check context
	if (ctx == CTX_PARSE && thisKlass)
		{ g_EC[_hzlevel()].Printf("%s %s line %d col %d: Context parse but klass supplied\n", __func__, *StrName(), X[ct].line, X[ct].col) ; return E_SYNTAX ; }
	if (ctx == CTX_CLASS && !pClass)
		{ g_EC[_hzlevel()].Printf("%s %s line %d col %d: Context class but none supplied\n", __func__, *StrName(), X[ct].line, X[ct].col) ; return E_SYNTAX ; }
	if (ctx == CTX_CFUNC && !pFET)
		{ g_EC[_hzlevel()].Printf("%s %s line %d col %d: Context cfunc but none supplied\n", __func__, *StrName(), X[ct].line, X[ct].col) ; return E_SYNTAX ; }

	/*
	**	Parse statement
	*/

	for (ct = start ; ct < X.Count() ; ct++)
	{
		Z << X[ct].value ;
		if (X[ct].type == TOK_CURLY_OPEN)
			{ Z.AddByte(CHAR_CURCLOSE) ; break ; }
		if (X[ct].type == TOK_END)
			break ;
	}
	S = Z ;

	stm_limit = ct ;
	ct = start ;

	g_EC[_hzlevel()].Printf("\n%s (%d ctx %s) %s line %d col %d: toks %d to %d: klass %s func %s STATEMENT %s\n",
		__func__, __LINE__, *ctx, *StrName(), X[ct].line, X[ct].col, start, stm_limit, thisKlass ? *thisKlass->StrName() : "none", pFET ? *pFET->StrName() : "null", *S) ;

	pFunc = 0 ;
	obj_name = (char*) 0 ;

	f_args.Clear() ;
	funcs.Clear() ;
	p_tpx.Clear() ;
	bTemplate = false ;
	opRange = SCOPE_GLOBAL ;

	Stmt.Clear() ;
	Stmt.m_Start = ct ;
	Stmt.m_nLine = X[ct].line ;

	//	Deal with compiler directives first as these determine if interleaving code is active and if so, how it should be interpreted.
	if (X[ct].IsDirective())
	{
		//	Compiler directives should not appear after pre-processing.

		if (X[ct].type == TOK_DIR_INCLUDE)
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: UNEXPECTED INCLUDE (%s)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct+1].value) ;
		else
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Directive of %s found - illegal after pre-proc\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	//	Scenario 1) Typdef statements (typedef new_type typlex;)
	if (X[ct].type == TOK_TYPEDEF)
	{
		rc = ProcStmtTypedef(Stmt, thisKlass, pFET, ultimate, ct) ;
		if (rc == E_OK)
			statements.Add(Stmt) ;
		else
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Typedef failed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return rc ;
	}

	//	Deal with using and namespace directives
	if (X[ct].type == TOK_KW_USING)
	{
		if (!(X[ct+1].type == TOK_KW_NAMESPACE && X[ct+2].type == TOK_WORD && X[ct+3].type == TOK_END))
			{ g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Required format is 'using namespace namespace_name ;'\n", __func__, __LINE__, *StrName(), X[ct].line) ; return E_SYNTAX ; }

		pEnt = LookupToken(X, thisKlass, pFET, t_end, ct + 2, false, __func__) ;
		if (!pEnt)
			{ g_EC[_hzlevel()].Printf("%s (%d) %s line %d: No namespace %s found\n", __func__, __LINE__, *StrName(), X[ct+2].line, *X[ct+2].value) ; return E_SYNTAX ; }
		if (pEnt->Whatami() != ENTITY_NAMSP)
			{ g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Entity %s is not a namespace\n", __func__, __LINE__, *StrName(), X[ct+2].line, *X[ct+2].value) ; return E_SYNTAX ; }

		pNamsp = (ceNamsp*) pEnt ;
		g_Project.m_Using.Insert(pNamsp) ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Now using namespace %s (count = %d)\n", __func__, __LINE__, *StrName(), X[ct+2].line, *pNamsp->StrName(), g_Project.m_Using.Count()) ;

		//	Apply namespace
		Stmt.m_eSType = STMT_USING ;
		Stmt.m_Object = X[ct + 2].value ;
		Stmt.m_End = ultimate = ct + 3 ;
		statements.Add(Stmt) ;
		return E_OK ;
	}

	/*
	**	Deal with declarations and related matter
	*/

	if (X[ct].type == TOK_KW_NAMESPACE)
	{
		//	We have a namespace statement of the form namespace namespace_name { code_body }. First step is to create the namespace if it does not yet
		//	exist and set the global current namespace to it. Then just call ProcNamespace to process the namespace body.

		//	return TestNamespace(statements, thisKlass, pFET, ultimate, start, ctx, codeLevel) ;

		if (X[ct+1].type != TOK_WORD)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR namespace must be followed by a namespace name\n", __func__, __LINE__, *StrName(), X[ct+1].line) ;
			return E_SYNTAX ;
		}
		obj_name = X[ct+1].value ;

		if (X[ct+2].type != TOK_CURLY_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR expected namespace body\n", __func__, __LINE__, *StrName(), X[ct+2].line) ;
			return E_SYNTAX ;
		}
		ultimate = X[ct+2].match ;

		//	Chek if namespace name exists
		pEnt = LookupToken(X, thisKlass, pFET, t_end, ct+1, true, __func__) ;
		if (pEnt)
		{
			if (pEnt->Whatami() != ENTITY_NAMSP)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR %s is already defined as a %s\n", __func__, __LINE__, *StrName(), X[ct+2].line, *obj_name, pEnt->EntDesc()) ;
				return E_SYNTAX ;
			}
			pNamsp = (ceNamsp*) pEnt ;
		}
		else
		{
			//	Create the namespace and add it to the current target
			pNamsp = new ceNamsp() ;
			pNamsp->Init(obj_name, S) ;

			if (tgt_ET->AddEntity(this, pNamsp, obj_name) != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR Failed to add namespace %s\n", __func__, __LINE__, *StrName(), X[ct+1].line, *obj_name) ;
				return E_SYNTAX ;
			}
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		}

		//	Apply namespace
		Stmt.m_eSType = STMT_NAMESPACE ;
		Stmt.m_Object = obj_name ;
		Stmt.m_Start = ct ;
		Stmt.m_End = ultimate ;
		statements.Add(Stmt) ;

		//	Now process namespace body
		rc = ProcNamespace(statements, thisKlass, pNamsp, t_end, ct+2, codeLevel+1) ;
		if (rc != E_OK)
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR Failed to process namespace %s\n", __func__, __LINE__, *StrName(), X[ct+1].line, *obj_name) ;
		return rc ;
	}

	//	If in a class or class template definition, we need to cope with directives that affect the scope of member variables
	if (thisKlass && ctx == CTX_CLASS)
	{
		if (X[ct+1].type == TOK_OP_SEP)
		{
			if (X[ct].type == TOK_KW_PUBLIC)	{ ct += 2 ; opRange = SCOPE_PUBLIC ; }
			if (X[ct].type == TOK_KW_PRIVATE)	{ ct += 2 ; opRange = SCOPE_PRIVATE ; }
			if (X[ct].type == TOK_KW_PROTECTED)	{ ct += 2 ; opRange = SCOPE_PROTECTED ; }
		}
	}

	//	Check for a tempate<args> construct
	if (X[ct].type == TOK_TEMPLATE)
	{
		//	The word 'template' has been encountered. There must now follow a set of template args (virtaul types) in a <> block

		if (ctx != CTX_PARSE)
			{ g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Cannot have 'template' in context %s\n",  __func__, __LINE__, *StrName(), X[ct].line, *ctx) ; return E_SYNTAX ; }

		if (!ProcTmplBase(t_end, ct))
			return E_SYNTAX ;

		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: TMPL ARGS ON: Count = %d\n", __func__, __LINE__, *StrName(), X[ct].line, g_CurTmplArgs.Count()) ;
		bTemplate = true ;
		ct = t_end ;
	}

	//	Deal with keywords that can only preceed a function declaration or definitionm
	if (X[ct].type == TOK_KW_FRIEND)
		{ bFriend = bFunction = true ; ct++ ; }

	if (X[ct].type == TOK_KW_INLINE)
		{ bInline = bFunction = true ; ct++ ; }

	if (X[ct].type == TOK_KW_VIRTUAL)
		{ bVirtual = bFunction = true ; ct++ ; }

	//	Deal with keywords that can only preceed a variable declaration
	if (X[ct].type == TOK_KW_EXTERN)
		{ bExtern = true ; ct++ ; }

	//	The static keyword could apply to functions or variables
	if (X[ct].type == TOK_KW_STATIC)
		{ bStatic = true ; ct++ ; }

	if (X[ct].type == TOK_KW_REGISTER)
		{ bRegister = bVariable = true ; ct++ ; }

	//	Check keyword combinations
	if (bStatic && bExtern)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Objects may not be both extern and static\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}

	if (bTemplate)
	{
		if (X[ct].type != TOK_CLASS && X[ct].type != TOK_STRUCT)
		{
			//	Function Templates (either the template is a function template or the statement is invalid)

			rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected a typlex as part of a template function def/decl. Got %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
				return E_SYNTAX ;
			}
			ct = t_end ;
			obj_name = X[ct].value ;

			if (X[ct].type == TOK_WORD && X[ct+1].type == TOK_ROUND_OPEN)
			{
				rc = ProcFuncDef(Stmt, thisKlass, p_tpx, obj_name, (EAttr) (FN_ATTR_STDFUNC | FN_ATTR_TEMPLATE), SCOPE_PUBLIC, t_end, ct + 1, ctx) ;
				if (rc != E_OK)
					return rc ;

				if (t_end == ct)
					{ g_EC[_hzlevel()].Printf("%s. function template uptake failure!\n", __func__) ; return E_SYNTAX ; }

				ultimate = ct = t_end ;
				statements.Add(Stmt) ;

				if (bTemplate)
				{
					g_CurTmplArgs.Clear() ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: TMPL ARGS OFF: Count = %d\n", __func__, __LINE__, *StrName(), X[ct].line, g_CurTmplArgs.Count()) ;
				}
				return E_OK ;
			}
		}
	}

	/*
	**	Handle class/struct/union/enum definitions including forward declarations - these may occur within any context.
	*/

	//	Handle class/struct definitions and forward declarations (ProcClass decides which of these has been encountered so it updates the file statement list)
	if ((X[ct].type == TOK_CLASS || X[ct].type == TOK_STRUCT)
		&& X[ct+1].type == TOK_WORD
		&& (X[ct+2].type == TOK_END || X[ct+2].type == TOK_OP_SEP || X[ct+2].type == TOK_CURLY_OPEN))
	{
		rc = ProcClass(Stmt, thisKlass, t_end, ct, codeLevel+1) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; g_EC[_hzlevel()].Printf("%s (%d): ProcClass failed\n", __func__, __LINE__) ; return rc ; }

		if (t_end == ct)
			{ g_EC[_hzlevel()].Printf("%s. class uptake failure!\n", __func__) ; return E_SYNTAX ; }

		ct = t_end ;
		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Class definitions or forward declarations must be terminated with ;\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		Stmt.m_eSType = STMT_TYPEDEF ;
		Stmt.m_Object = X[ct + 2].value ;
		Stmt.m_End = ultimate = ct ;
		ultimate = ct ;
		statements.Add(Stmt) ;

		if (bTemplate)
		{
			g_CurTmplArgs.Clear() ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: TMPL ARGS OFF: Count = %d\n", __func__, __LINE__, *StrName(), X[ct].line, g_CurTmplArgs.Count()) ;
		}
		return E_OK ;
	}

	//	Handle union definitions and forward declarations
	if (X[ct].type == TOK_UNION)
	{
		rc = ProcUnion(Stmt, thisKlass, t_end, ct, codeLevel+1) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; g_EC[_hzlevel()].Printf("%s. Proc Union failed!\n", __func__) ; return rc ; }

		statements.Add(Stmt) ;
		ct = t_end ;
		ct++ ;
		ultimate = ct ;
		g_EC[_hzlevel()].Printf("%s. Proc Union OK!\n", __func__) ;
		return E_OK ;
	}

	//	Handle enum definitions and forward declarations.
	if (X[ct].type == TOK_ENUM)
	{
		rc = ProcEnum(Stmt, thisKlass, t_end, ct) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

		if (t_end == ct)
			{ g_EC[_hzlevel()].Printf("%s. Enum uptake failure!\n", __func__) ; return E_SYNTAX ; }

		ct = t_end ;
		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Enum definitions or forward declarations must be terminated with ;\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		statements.Add(Stmt) ;
		ct++ ;
		ultimate = ct ;
		return E_OK ;
	}

	//	If in a class or class template, look for constructors/destructors etc
	if (ctx == CTX_CLASS)
	{
		//	Within a class/ctmpl definition, every possible statement must either be:-
		//
		//		a)	Declaration of variable or function and start with a DATATYPE
		//		b)	Definition of a sub-class and start with the word 'class' or 'struct'
		//		c)	Definition of a sub-union and start with the word 'union'
		//		d)	Declaration of a casting operator and start with the word 'operator'
		//
		//	Here we deal only with case (d) and a subset of (a), namely where the statement defines a constructor/destructor

		if (X[ct].value == "operator" && !X[ct+1].IsOperator())
		{
			//	Case of casting operator decl/def

			rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct+1) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s. Casting operators are of the form 'operator type (args);' Type not establihed\n", __func__) ;
				return rc ;
			}

			Z.Clear() ;
			Z << X[ct].value ;
			for (ct++ ; ct < t_end ; ct++)
			{
				Z.AddByte(CHAR_SPACE) ;
				Z << X[ct].value ;
			}
			obj_name = Z ;

			g_EC[_hzlevel()].Printf("%s (%d) Adding a casting operator to class %s\n", __func__, __LINE__, *thisKlass->StrName()) ;
			rc = ProcFuncDef(Stmt, thisKlass, p_tpx, obj_name, FN_ATTR_OPERATOR, opRange, t_end, ct, ctx) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to add a casting (%s) to class %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name, *thisKlass->StrName()) ;
				return rc ;
			}
			ultimate = ct = t_end ;
			statements.Add(Stmt) ;
			return E_OK ;
		}

		if (X[ct].value != "class" && X[ct].value != "struct" && X[ct].value != "union")
		{
			//	Handle the class destructors
			if (X[ct].value == "~" && X[ct+1].value == thisKlass->StrName() && X[ct+2].type == TOK_ROUND_OPEN)
			{
			
				obj_name = X[ct].value + X[ct+1].value ;
				p_tpx.Clear() ;

				g_EC[_hzlevel()].Printf("%s (%d) Adding a destructor to class %s\n", __func__, __LINE__, *thisKlass->StrName()) ;
				rc = ProcFuncDef(Stmt, thisKlass, p_tpx, obj_name, FN_ATTR_DESTRUCTOR, opRange, t_end, ct + 2, ctx) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to add a destructor to klass %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *thisKlass->StrName()) ;
					return rc ;
				}
				ultimate = ct = t_end ;
				statements.Add(Stmt) ;
				return E_OK ;
			}

			//	Handle constructors
			if (X[ct].value == thisKlass->StrName())
			{
				rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: At this point, only a DATATYPE is allowed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					return rc ;
				}

				//if (X[ct].value == thisKlass->StrName() && X[t_end+1].type == TOK_ROUND_OPEN)
				if (X[t_end].type == TOK_ROUND_OPEN)
				{
					ct = t_end ;
					g_EC[_hzlevel()].Printf("%s (%d) Adding a constructor to class %s\n", __func__, __LINE__, *thisKlass->StrName()) ;

					p_tpx.Clear() ;
					p_tpx.SetType(thisKlass) ;
					p_tpx.m_Indir = 1 ;

					rc = ProcFuncDef(Stmt, thisKlass, p_tpx, thisKlass->StrName(), FN_ATTR_CONSTRUCTOR, opRange, t_end, ct, ctx) ;
					if (rc != E_OK)
					{
						g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
						g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to add a constructor to klass %s\n",
							__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *thisKlass->StrName()) ;
						return rc ;
					}
					ultimate = ct = t_end ;
					statements.Add(Stmt) ;
					return E_OK ;
				}
			}
		}
	}

	//	Consider the case of class constructors/destrutors and casting operators defined outside the class definition body
	if (ctx == CTX_PARSE && X[ct].type == TOK_WORD && X[ct+1].type == TOK_OP_SCOPE)
	{
		//	current token has to be confirmed as a class

		pClass = GetClass(t_end, ct) ;

		if (pClass)
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: incumbant class is %s. Now %s is a class (ct=%d, t_end=%d)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, thisKlass ? *thisKlass->StrName() : "NULL", *X[ct].value, ct, t_end) ;
		else
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: incumbant class is %s. Now %s is not a class\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, thisKlass ? *thisKlass->StrName() : "NULL", *X[ct].value) ;

		if (pClass)
		{
			ct = t_end ;

			if (X[ct].type == TOK_WORD && X[ct+1].type == TOK_OP_SCOPE && X[ct].value == X[ct+2].value && X[ct+3].type == TOK_ROUND_OPEN)
			{
				//	Class constructors defined outside class definition
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing extrnally defined Constructor to class %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, *pClass->StrName()) ;

				obj_name = pClass->StrName() ;

				rc = ProcFuncDef(Stmt, pClass, p_tpx, obj_name, FN_ATTR_CONSTRUCTOR, SCOPE_PUBLIC, t_end, ct + 3, ctx) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Aborting after failed fn process call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					return E_SYNTAX ;
				}
				ultimate = ct = t_end ;
				statements.Add(Stmt) ;
				return E_OK ;
			}

			if (X[ct].type == TOK_WORD && X[ct+1].type == TOK_OP_SCOPE && X[ct+2].type == TOK_OP_INVERT && X[ct].value == X[ct+3].value && X[ct+4].type == TOK_ROUND_OPEN)
			{
				//	Class destructors defined outside class definition
				obj_name = "~" + pClass->StrName() ;

				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing extrnally defined destructor to class %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, *pClass->StrName()) ;
				rc = ProcFuncDef(Stmt, pClass, p_tpx, obj_name, FN_ATTR_DESTRUCTOR, SCOPE_PUBLIC, t_end, ct + 4, ctx) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Aborting after failed fn process call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					return E_SYNTAX ;
				}
				ultimate = ct = t_end ;
				statements.Add(Stmt) ;
				return E_OK ;
			}

			if (X[ct].type == TOK_WORD && X[ct+1].type == TOK_OP_SCOPE && X[ct+2].value == "operator")
			{
				//	Class operators defined outside class definition
				rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct + 3) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Casting operators are of the form 'operator type (args);' Type not establihed\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					return E_SYNTAX ;
				}

				Z.Clear() ;
				Z << X[ct].value ;
				for (ct++ ; ct < t_end ; ct++)
				{
					Z.AddByte(CHAR_SPACE) ;
					Z << X[ct].value ;
				}
				obj_name = Z ;

				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing extrnally defined operator to class %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, *pClass->StrName()) ;
				rc = ProcFuncDef(Stmt, pClass, p_tpx, obj_name, FN_ATTR_OPERATOR, SCOPE_PUBLIC, t_end, ct, ctx) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Aborting after failed fn process call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					return E_SYNTAX ;
				}
				ultimate = ct = t_end ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Scenario 12 met\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				statements.Add(Stmt) ;
				return E_OK ;
			}
		}
	}

 	//	At this juncture, all remaining legal statement forms would (after any optional keywords already processed above) begin with a typlex and
	//	then a word that must either amount to a variable or function name.

	rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct) ;
	if (rc != E_OK)
	{
		g_EC[_hzlevel()].Printf("%s (%d ctx %s) %s line %d col %d: Expected a typlex as part of a variable or function def/decl. At tok %d Got %s Stmt=%s\n",
			__func__, __LINE__, *ctx, *StrName(), X[ct].line, X[ct].col, ct, *X[ct].value, *S) ;
		return E_SYNTAX ;
	}

	ct = t_end ;

	if (!p_tpx.Type())
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected a typlex to preceed object %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	/*
	**	After the typlex the next item in the statement must be the [fully qualified] name of either a function or variable. The first step is to
	**	see if the current token is the start of a construct of the form '[namespace::][class::]member'. If it is it should be already declared in
	**	the current scope - as we are here processing code that lies outside of any class or class template or function definition body.
	*/

	pClass = GetClass(t_end, ct) ;

	if (pClass)
		ct = t_end + 2 ;

	//	If a class is involved the current token should be on the class memeber
	if (X[ct].type == TOK_ROUND_OPEN && X[ct+1].type == TOK_OP_MULT && X[ct+2].type == TOK_WORD && X[ct+3].type == TOK_ROUND_CLOSE
		&& X[ct+4].type == TOK_ROUND_OPEN)
	{
		//	Investigate function pointer
		obj_name = X[ct+2].value ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Adding a func_ptr of name %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;

		ceTyplex	a_tpx ;		//	Argument typlex

		//	Declare ceVar instance
		pVar = new ceVar() ;

		//	Read arguments. Note these must not be named.
		for (ct += 5 ; rc == E_OK ;)
		{
			rc = GetTyplex(a_tpx, thisKlass, 0, t_end, ct) ;
			if (rc != E_OK)
				break ;

			ct = t_end ;
			if (X[ct].type == TOK_WORD)
				ct++ ;

			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Tok %s Adding a fnptr arg of %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, *a_tpx.Str()) ;

			p_tpx.m_Args.Add(a_tpx) ;
			if (X[ct].type == TOK_SEP)
				{ ct++ ; continue ; }

			if (X[ct].type == TOK_ROUND_CLOSE)
				{ ct++ ; break ; }

			rc = E_SYNTAX ;
			break ;
		}

		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: FAILED to add func_ptr of name %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
			return rc ;
		}

		p_tpx.SetAttr(DT_ATTR_FNPTR) ;
		rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;

		rc = tgt_ET->AddEntity(this, pVar, __func__) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: FAILED to add entity func_ptr of name %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
			return rc ;
		}
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: DONE add entity func_ptr of name %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;

		if (X[ct].type == TOK_OP_EQ)
			ct += 2 ;

		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed func_ptr (expected a ; after (*func_name)(arg-set) form. Got %s)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		Stmt.m_eSType = STMT_VARDCL_FNPTR ;
		Stmt.m_Object = obj_name ;
		Stmt.m_End = ultimate = ct ;
		statements.Add(Stmt) ;

		pt = X[ct].comPost ;
		if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
		{
			if (!g_Project.m_bSystemMask)
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the function pointer\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		}
		else
		{
			pVar->SetDesc(P[pt].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		}

		return E_OK ;
	}

	//	Deal with special case of operator functions
	if (X[ct].value == "operator")
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing externally defined non-casting operator to class %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, thisKlass ? *thisKlass->StrName() : "null") ;

		//	A non-casting operator function decl/def
		if (X[ct+1].value == "new" || X[ct+1].value == "delete")
		{
			if (X[ct+2].type == TOK_SQ_OPEN && X[ct+3].type == TOK_SQ_CLOSE)
			{
				obj_name = X[ct].value + X[ct+1].value + "[]" ;
				ct += 4 ;
			}
			else
			{
				obj_name = X[ct].value + X[ct+1].value ;
				ct += 2 ;
			}
		}
		else if (X[ct+1].type == TOK_SQ_OPEN && X[ct+2].type == TOK_SQ_CLOSE)
		{
			obj_name = X[ct].value + "[]" ;
			ct += 3 ;
		}
		else
		{
			if (!X[ct+1].IsOperator())
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: After 'operator' a C++ operator is expected - Got %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct+1].value) ;
				return E_SYNTAX ;
			}

			obj_name = X[ct].value + X[ct+1].value ;
			ct += 2 ;
		}

		if (ctx == CTX_PARSE)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing externally defined non-casting operator to class %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, pClass ? *pClass->StrName() : "null") ;
			rc = ProcFuncDef(Stmt, pClass, p_tpx, obj_name, FN_ATTR_OPERATOR, SCOPE_PUBLIC, t_end, ct, ctx) ;
		}
		if (ctx == CTX_CLASS)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing externally defined non-casting operator to class %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, *thisKlass->StrName()) ;
			rc = ProcFuncDef(Stmt, thisKlass, p_tpx, obj_name, FN_ATTR_OPERATOR, SCOPE_PUBLIC, t_end, ct, ctx) ;
		}
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Aborting after failed fn process call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		ultimate = ct = t_end ;

		if (pClass)
			g_EC[_hzlevel()].Printf("%s (%d) Added non-casting operator (%s) to class %s\n", __func__, __LINE__, *obj_name, *pClass->StrName()) ;
		else
			g_EC[_hzlevel()].Printf("%s (%d) Added a non-member non-casting operator (%s)\n", __func__, __LINE__, *obj_name) ;

		statements.Add(Stmt) ;
		return E_OK ;
	}

	/*
	**	Now confining investigations to:-
	**
	**	Single variable declaration		of the form [keywords] class/type name ;
	**	Variable array declarations		of the form [keywords] class/type name[noElements] ;
	**	Single variable declarations	of the form [keywords] typlex var_name(value_args) ;
	**	Function declarations			of the form [keywords] typlex func_name (type_args) [const] ;
	**	Function definitions			of the form [keywords] typlex func_name (type_args) [const] {body} ;
	*/

	//	Now expect variable or function name
	if (X[ct].type != TOK_WORD)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected variable or function name. Got %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	//	Obtain the object name
	Stmt.m_Object = obj_name = X[ct].value ;
	ct++ ;

	if (!pClass)
		obj_fqname = obj_name ;
	else
	{
		obj_fqname = pClass->Fqname() ;
		obj_fqname += "::" ;
		obj_fqname += obj_name ;
	}

	/*
	**	Investigate variable declaration.
	*/

	//	Investigate variable array declaration
	if (X[ct].type == TOK_SQ_OPEN)
	{
		//	For an opening square bracket to appear after the object name means the statement must be a declaration of an array of variables. There are now two
		//	possible forms. Either there is no assignment in which case the number of elements must be specified within the [] block OR the array is assigned a
		//	value, supplied in the form {values}. In the latter case the number of elements can be left unspecified within the [] since this is derived from the
		//	supplied initialization array. If the number of elements is specified within the [] block and an assignement applies, then the number of elements in
		//	the supplied initialization array must agree with this number. There is a third case allowed at the file level in which arrays declared as extern do
		//	not need to specify the number of elements and must not be assigned an array of values.

		match = X[ct].match ;

		ct++ ;
		if (p_tpx.m_Indir >= 0)
			p_tpx.m_Indir++ ;

		g_EC[_hzlevel()].Printf("%s (%d) S15 Adding a variable array of name %s\n", __func__, __LINE__, *obj_name) ;

		//	Special case of an extern array 
		if (X[ct].type == TOK_SQ_CLOSE && X[ct+1].type == TOK_END)
		{
			if (!bExtern)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Empty arrays may only be extern\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}

			//	Check if the object exists in the entity table
			pEnt = tgt_ET->GetEntity(obj_name) ;

			if (pEnt)
			{
				//	If the entity is already declared, check the entity is a variable and of the correct typlex
				if (pEnt->Whatami() != ENTITY_VARIABLE)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Variable %s already declared as a %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, pEnt->EntDesc()) ;
					return E_SYNTAX ;
				}
				pVar = (ceVar*) pEnt ;
				if (pVar->Typlex() != p_tpx)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: WARNING variable %s already has type %s, not %s\n",
						__func__, __LINE__, *StrName(), X[ct].line, *obj_name, *pVar->Typlex().Str(), *p_tpx.Str()) ;
			}
			else
			{
				//	Declare the entity for the first time
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding extern variable %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
				pVar = new ceVar() ;
				rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding extern variable %s as %s Failed\n",
						__func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
					return rc ;
				}

				rc = tgt_ET->AddEntity(this, pVar, __func__) ;
				if (rc != E_OK)
					{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			}

			ct++ ;
			Stmt.m_eSType = STMT_VARDCL_ARRAY ;
			Stmt.m_End = ultimate = ct ;
			statements.Add(Stmt) ;

			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Added extern variable\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;

			pt = X[ct].comPost ;
			if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
			{
				if (!g_Project.m_bSystemMask)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the extern array\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			}
			else
			{
				pVar->SetDesc(P[pt].value) ;
				P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
			}

			return E_OK ;
		}

		if (ctx == CTX_PARSE && bStatic)
		{
			//	Set target entity table to the file
			if (!m_pET)
				{ m_pET = new ceEntbl() ; m_pET->InitET(this) ; g_Project.m_EntityTables.Insert(m_pET->StrName(), m_pET) ; }
			tgt_ET = m_pET ;
		}

		//	Get the array size if it is available
		if (X[ct].type != TOK_SQ_CLOSE)
		{
			rc = AssessExpr(statements, theTpx, theVal, thisKlass, pFET, t_end, ct, match + 1, 0) ;
			if (!theTpx.Type())
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d: Expression assumed to be array size (%s) failed to evaluate\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
				return E_SYNTAX ;
			}
			ct = match ;
			g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d: Array sise now defined. Tok now %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

			if (theTpx.Basis() != ADT_ENUM && !(theTpx.Basis() & (CPP_DT_INT | CPP_DT_UNT)))
			{
				g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d: Expression assumed to be array size does not evaluate to an integer\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}
		}

		//	Variable array of defined or undefined number of elements with assignment
		if (X[ct].type == TOK_SQ_CLOSE && X[ct+1].type == TOK_OP_EQ)
		{
			ct += 2 ;
			if (X[ct].type != TOK_CURLY_OPEN)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed variable array declaration - expected opening '{' of initializing array\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}

			//	Check for comment after the opening curly brace
			pt = X[ct].comPost ;
			ct = X[ct].match + 1 ;

			//	Check for termination
			if (X[ct].type != TOK_END)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed variable initializer (expected a ; after multiple value)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}

			//	Check if the object exists in the entity table
			pEnt = tgt_ET->GetEntity(obj_name) ;

			if (pEnt)
			{
				//	If the entity is already declared, check the entity is a variable and of the correct typlex
				if (pEnt->Whatami() != ENTITY_VARIABLE)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Variable %s already declared as a %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, pEnt->EntDesc()) ;
					return E_SYNTAX ;
				}
				pVar = (ceVar*) pEnt ;
				if (pVar->Typlex() != p_tpx)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: WARNING variable %s already has type %s, not %s\n",
						__func__, __LINE__, *StrName(), X[ct].line, *obj_name, *pVar->Typlex().Str(), *p_tpx.Str()) ;
			}
			else
			{
				//	Declare the entity for the first time
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding assign array %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
				pVar = new ceVar() ;
				rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
				if (rc == E_OK)
					rc = tgt_ET->AddEntity(this, pVar, __func__) ;

				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Failed to add assigned array %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;
					return rc ;
				}
			}

			if (rc == E_OK)
			{
				if (pt != 0xffffffff)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Added a post= comment\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;
					pVar->SetDesc(P[pt].value) ;
				}

				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Added an assigned array\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;
				Stmt.m_eSType = STMT_VARDCL_ASSIG ;
				Stmt.m_End = ct ;
				statements.Add(Stmt) ;
				ultimate = ct ;
			}
			return rc ;
		}

		if (X[ct].type == TOK_SQ_CLOSE && X[ct+1].type == TOK_END)
		{
			//	Variable array of defined size without assignment.

			pVar = new ceVar() ;

			rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding assign array %s as %s Failed\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
				return rc ;
			}

			ct++ ;
			rc = tgt_ET->AddEntity(this, pVar, __func__) ;
			if (rc != E_OK)
				{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

			Stmt.m_eSType = STMT_VARDCL_ARRAY ;
			Stmt.m_End = ultimate = ct ;
			statements.Add(Stmt) ;

			pt = X[ct].comPost ;
			if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
			{
				if (!g_Project.m_bSystemMask)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the array\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			}
			else
			{
				pVar->SetDesc(P[pt].value) ;
				P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
			}

			return E_OK ;
		}

		//	Error
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed variable array declalation: Either ar_name[noElements] ; OR initializing array expected\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}

	//	Not an array so statement may be a single variable
	if (X[ct].type == TOK_OP_EQ)
	{
		//	Single variable assigned a single value. Check for comment after the = sign
		pt = X[ct].comPost ;
		ct++ ;

		g_EC[_hzlevel()].Printf("%s (%d) S15 Adding and assigning variable of name %s\n", __func__, __LINE__, *obj_name) ;
		rc = AssessExpr(statements, theTpx, theVal, thisKlass, pFET, t_end, ct, stm_limit, 0) ;
		if (!theTpx.Type())
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d: The RHS (%s) of an assignment must evaluate correctly\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}
		ct = t_end ;

		pVar = new ceVar() ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding assign array %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
		rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) Failed to init variable %s\n", __func__, __LINE__, *obj_name) ;
			return rc ;
		}
		if (pt != 0xffffffff)
			pVar->SetDesc(P[pt].value) ;

		if (ctx == CTX_PARSE && bStatic)
		{
			if (!m_pET)
				{ m_pET = new ceEntbl() ; m_pET->InitET(this) ; g_Project.m_EntityTables.Insert(m_pET->StrName(), m_pET) ; }
			tgt_ET = m_pET ;
		}

		rc = tgt_ET->AddEntity(this, pVar, __func__) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) Failed to add variable %s\n", __func__, __LINE__, *obj_name) ;
			return rc ;
		}
		g_EC[_hzlevel()].Printf("%s (%d) DONE add variable %s\n", __func__, __LINE__, *obj_name) ;

		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed variable initializer (expected a ; after single value. Got %s)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		//	Check for comment
		pt = X[ct].comPost ;
		if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
		{
			if (!pVar->StrDesc() && !g_Project.m_bSystemMask && X[ct].col > 0)
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the variable decl and assign (%s)\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
		}
		else
		{
			pVar->SetDesc(P[pt].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		}

		Stmt.m_eSType = STMT_VARDCL_ASSIG ;
		Stmt.m_End = ultimate = ct ;
		statements.Add(Stmt) ;
		return E_OK ;
	}

	if (X[ct].type == TOK_END)
	{
		//	Statement terminated after object name meaning a variable declaration of the form [keywords] class/type name ;

		pVar = new ceVar() ;

		rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) Failed to init variable %s\n", __func__, __LINE__, *obj_name) ;
			return rc ;
		}

		if (ctx == CTX_PARSE && bStatic)
		{
			if (!m_pET)
				{ m_pET = new ceEntbl() ; m_pET->InitET(this) ; g_Project.m_EntityTables.Insert(m_pET->StrName(), m_pET) ; }
			rc = m_pET->AddEntity(this, pVar, __func__) ;
		}
		else
			rc = tgt_ET->AddEntity(this, pVar, __func__) ;

		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) Failed to add variable %s\n", __func__, __LINE__, *obj_name) ;
			return rc ;
		}
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Added variable %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;

		Stmt.m_eSType = STMT_VARDCL_STD ;
		Stmt.m_End = ultimate = ct ;
		statements.Add(Stmt) ;

		//	Check for comment
		pt = X[ct].comPost ;
		if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
		{
			if (!g_Project.m_bSystemMask && X[ct].col > 0)
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the variable (%s)\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
		}
		else
		{
			pVar->SetDesc(P[pt].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		}

		return E_OK ;
	}

	/*
	**	While the opening '[' at this juncture signals an array of variable, the only remaining legal token is the opening '(' which confines the
	**	analysis to one of the following:-
	**
	**	Variable declarations -> [keywords] typlex var_name(constructor_args values) ;
	**	Function declarations -> [keywords] typlex func_name (type_args) [const] ;
	**	Function definitions  -> [keywords] typlex func_name (type_args) [const] {body} ;
	*/

	if (X[ct].type != TOK_ROUND_OPEN)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected function argument block. Got %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	//	Will be a function if the next token amounts to a type
	if (X[ct+1].type != TOK_ROUND_CLOSE)
	{
		rc = GetTyplex(s_tpx, thisKlass, 0, t_end, ct + 1) ;
		if (rc != E_OK)
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
	}

	if (rc == E_OK)
	{
		//	Function declarations -> [keywords] typlex func_name (args) [const] ;
		//	Function definitions  -> [keywords] typlex func_name (args) [const] {body} ;

		if (!pClass)
			tmpHost = thisKlass ;
		else
			tmpHost = pClass ;

		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing ordinary function to class %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, tmpHost ? *tmpHost->StrName() : "null") ;
		rc = ProcFuncDef(Stmt, tmpHost, p_tpx, obj_name, FN_ATTR_STDFUNC, SCOPE_PUBLIC, t_end, ct, ctx) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: case 4 - Aborting after failed fn process call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		ultimate = ct = t_end ;
		statements.Add(Stmt) ;
		return E_OK ;
	}

	match = X[ct].match ;
	if (X[match+1].type == TOK_END)
	{
		//	Test S17, that of a variable declaration in which a class/ctmpl is instantiated and initialized by a contructor with argument(s). For
		//	this to be the case, the class/ctmpl must have a constructor that can be called in this way.

		if (p_tpx.Type() && p_tpx.Whatami() == ENTITY_CLASS)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Examining scenario 17 (var inst by constructor args)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;

			//	The type is a class or class template so we look for it's constructors and test if the statement is calling a constructor with
			//	valid arguments

			hzVect<ceStmt>			sx ;
			hzList<ceFunc*>::Iter	fi ;
			hzList<ceVar*>::Iter	ai ;

			hzArray<ceTyplex>	valList ;			//	Supplied argument values

			ceTyplex	tmpTpx ;
			ceVar*		theArg ;
			ceClass*	clsX ;
			hzString	tmp_name ;
			uint32_t	c_tmp ;
			uint32_t	nVals ;

			if (p_tpx.Whatami() == ENTITY_CLASS)
				{ clsX = (ceClass*) p_tpx.Type() ; fi = clsX->m_Funcs ; tmp_name = clsX->StrName() ; }
			tmpHost = (ceKlass*) p_tpx.Type() ;

			for (; fi.Valid() ; fi++)
			{
				pFunc = fi.Element() ;
				valList.Clear() ;

				g_EC[_hzlevel()].Printf("%s (%d) Case 1 Testing func %s\n", __func__, __LINE__, *pFunc->StrName()) ;
				if (pFunc->StrName() != tmp_name)
					{ pFunc = 0 ; continue ; }

				//	We have a constructor to test
				for (c_tmp = ct + 1 ; c_tmp < match ; c_tmp++)
				{
					g_EC[_hzlevel()].Printf("%s (%d) S17 testing token %d %s\n", __func__, __LINE__, c_tmp, *X[c_tmp].value) ;
					rc = AssessExpr(sx, tmpTpx, theVal, tmpHost, 0, t_end, c_tmp, match + 1, 0) ;
					if (!tmpTpx.Type())
						{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; pFunc = 0 ; break ; }

					valList.Add(tmpTpx) ;
					c_tmp = t_end ;

					if (X[c_tmp].type != TOK_SEP)
						break ;
				}

				if (!pFunc)
					{ pFunc = 0 ; continue ; }

				//	Match function to args
				g_EC[_hzlevel()].Printf("%s (%d) Case 2 Testing func %s\n", __func__, __LINE__, *pFunc->StrName()) ;
				if (pFunc->m_Args.Count() != valList.Count())
					{ pFunc = 0 ; continue ; }

				for (nVals = 0, ai = pFunc->m_Args ; ai.Valid() ; nVals++, ai++)
				{
					theArg = ai.Element() ;
					tmpTpx = valList[nVals] ;

					if (theArg->Typlex().Testset(tmpTpx) != E_OK)
						{ pFunc = 0 ; break ; }
				}

				if (pFunc)
					break ;
			}

			if (pFunc)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: S17 Case 1 Have var decl of the form {[keywords] class_type var_name(constructor_args);}\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;

				pVar = new ceVar() ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding assign array %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
				rc = pVar->InitDeclare(this, tmpHost, p_tpx, SCOPE_GLOBAL, obj_name) ;	//, obj_fqname) ;
				if (rc != E_OK)
					return rc ;

				rc = tgt_ET->AddEntity(this, pVar, __func__) ;
				if (rc != E_OK)
					{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

				ct = match + 2 ;
				Stmt.m_eSType = STMT_VARDCL_CONS ;
				Stmt.m_End = ultimate = ct ;
				Stmt.m_Object = obj_name ;
				statements.Add(Stmt) ;
				return E_OK ;
			}
		}
	}

	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed Statement\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
	return E_NOTFOUND ;
}

hzEcode	ceFile::ProcStructStmt	(hzVect<ceStmt>& Stmts, ceKlass* thisKlass, ceEntbl* pFET, Scope opRange, uint32_t& ultimate, uint32_t start, uint32_t codeLevel)
{
	//	Process a single C++ statement, within and only within a class or class template definition body.
	//
	//	Arguments:	1)	Stmts		Vector of statement (either for the file, the class or the function), this function will add to
	//				2)	ss_Err		Error report
	//				3)	thisKlass	When called by ProcClass() or ProcCtmpl() this is the class or class template whose definition is being processed.
	//				4)	pFET		Function entity table (functions only)
	//				5)	ultimate	The postion of the last token found in the statement (usually a ;)
	//				6)	start		Starting token assumed to be at the begining of the statement
	//				7)	codeLevel	Level of {} indent
	//
	//	Returns:	1) E_SYNTAX		In the event of a malformed or otherwise invalid statement
	//				2) E_NOTFOUND	In the event no valid statement form was established
	//				3) E_OK			If the statement was successfully processed

	_hzfunc("ceFile::ProcStructStmt") ;

	hzList	<ceVar*>::Iter	aI1 ;			//	Function arg iterator
	hzList	<ceVar*>::Iter	aI2 ;			//	Function arg iterator

	hzList	<ceVar*>	f_args ;			//	Function arguments
	hzList	<ceTyplex>	f_vals ;			//	Function arguments
	hzVect	<ceFunc*>	funcs ;				//	List of possible functions

	ceTyplex	theTpx ;					//	Evluation result
	hzAtom		theVal ;					//	Value set by expression assesment if any
	hzChain		tmpErr ;					//	Auxillary error chain
	hzChain		Z ;							//	For reporting a series of connected tokens
	hzString	S ;							//	Temp string
	ceStmt		Stmt ;						//	Statement populated here
	hzString	obj_name ;					//	Name of entity being defined or declared.
	hzString	obj_fqname ;				//	For when we have a host class
	ceToken		tok ;						//	Current of reference token (start of statement)
	ceTyplex	p_tpx ;						//	Primary typlex (in statments begining with [keywords] typlex ...
	ceTyplex	s_tpx ;						//	Seconday typlex where statements involve further typlex
	ceEntbl*	tgt_ET ;					//	Entity table of object
	ceEntity*	pEnt = 0 ;					//	Returned by Lookup in the current scope
	ceKlass*	tmpHost = 0 ;				//	Temp host (class/ctmpl)
	ceClass*	pClass = 0 ;				//	Class pointed to by current token if any
	ceVar*		pVar = 0 ;					//	Current variable
	ceFunc*		pFunc = 0 ;					//	Current function
	uint32_t	ct ;						//	Iterator, current token
	uint32_t	pt ;						//	Offset into file array P for comments
	uint32_t	t_end ;						//	Set by various interim statement processors
	uint32_t	match ;						//	For matching braces
	uint32_t	stm_limit ;					//	End of statement (either ; or end of line)
	bool		bRegister = false ;			//	posn of word 'register' if present
	bool		bExtern = false ;			//	posn of word 'extern' if present
	bool		bFriend = false ;			//	posn of word 'friend' if present
	bool		bVirtual = false ;			//	posn of word 'virtual' if present
	bool		bInline = false ;			//	posn of word 'inline' if present
	bool		bStatic = false ;			//	posn of word 'static' if present
	bool		bFunction = false ;			//	Set if keywords or other mean statement is function dcl or def
	bool		bVariable = false ;			//	Set if keywords or other mean statement is variable dcl
	bool		bTemplate = false ;			//	position of the word template if encountered
	hzEcode		rc = E_OK ;					//	Return code

	/*
	**	Set up target entity table for new objects to be added to
	*/

	if (thisKlass)
	{
		//	Operating within a class/ctmpl definition

		if (thisKlass->Whatami() == ENTITY_CLASS)
			{ pClass = (ceClass*) thisKlass ; tgt_ET = &(pClass->m_ET) ; }
		else
			Fatal("%s. Supplied host class is not a class or a class template. It is a %s\n", __func__, __LINE__, Entity2Txt(thisKlass->Whatami())) ;

		if (pFET)
			tgt_ET = pFET ;
	}
	else
	{
		//	Operating outside class/ctmpl definition. 
		tgt_ET = g_Project.m_pCurrNamsp ? &(g_Project.m_pCurrNamsp->m_ET) : &g_Project.m_RootET ;

		if (pFET)
			tgt_ET = pFET ;
	}

	//	Check context
	if (!pClass)
		{ g_EC[_hzlevel()].Printf("%s %s line %d col %d: Context class but none supplied\n", __func__, *StrName(), X[ct].line, X[ct].col) ; return E_SYNTAX ; }

	/*
	**	Parse statement
	*/

	for (ct = start ; ct < X.Count() ; ct++)
	{
		Z << X[ct].value ;
		if (X[ct].type == TOK_CURLY_OPEN)
			{ Z.AddByte(CHAR_CURCLOSE) ; break ; }
		if (X[ct].type == TOK_END)
			break ;
	}
	S = Z ;

	stm_limit = ct ;
	ct = start ;

	g_EC[_hzlevel()].Printf("\n%s (%d) %s line %d col %d: toks %d to %d: klass %s func %s STATEMENT %s\n",
		__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, start, stm_limit, thisKlass ? *thisKlass->StrName() : "none", pFET ? *pFET->StrName() : "null", *S) ;

	pFunc = 0 ;
	obj_name = (char*) 0 ;

	f_args.Clear() ;
	funcs.Clear() ;
	p_tpx.Clear() ;
	bTemplate = false ;
	//opRange = SCOPE_GLOBAL ;

	Stmt.Clear() ;
	Stmt.m_Start = ct ;
	Stmt.m_nLine = X[ct].line ;

	//	Deal with compiler directives first as these determine if interleaving code is active and if so, how it should be interpreted.
	if (X[ct].IsDirective())
	{
		//	Compiler directives should not appear after pre-processing.

		if (X[ct].type == TOK_DIR_INCLUDE)
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: UNEXPECTED INCLUDE (%s)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct+1].value) ;
		else
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Directive of %s found - illegal after pre-proc\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	//	If in a class or class template definition, we need to cope with directives that affect the scope of member variables
	if (thisKlass)
	{
		if (X[ct+1].type == TOK_OP_SEP)
		{
			if (X[ct].type == TOK_KW_PUBLIC)	{ ct += 2 ; opRange = SCOPE_PUBLIC ; }
			if (X[ct].type == TOK_KW_PRIVATE)	{ ct += 2 ; opRange = SCOPE_PRIVATE ; }
			if (X[ct].type == TOK_KW_PROTECTED)	{ ct += 2 ; opRange = SCOPE_PROTECTED ; }
		}
	}

	//	Deal with keywords that can only preceed a function declaration or definitionm
	if (X[ct].type == TOK_KW_FRIEND)
		{ bFriend = bFunction = true ; ct++ ; }

	if (X[ct].type == TOK_KW_INLINE)
		{ bInline = bFunction = true ; ct++ ; }

	if (X[ct].type == TOK_KW_VIRTUAL)
		{ bVirtual = bFunction = true ; ct++ ; }

	//	Deal with keywords that can only preceed a variable declaration
	if (X[ct].type == TOK_KW_EXTERN)
		{ bExtern = true ; ct++ ; }

	//	The static keyword could apply to functions or variables
	if (X[ct].type == TOK_KW_STATIC)
		{ bStatic = true ; ct++ ; }

	if (X[ct].type == TOK_KW_REGISTER)
		{ bRegister = bVariable = true ; ct++ ; }

	//	Check keyword combinations
	if (bStatic && bExtern)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Objects may not be both extern and static\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}

	if (bTemplate)
	{
		if (X[ct].type != TOK_CLASS && X[ct].type != TOK_STRUCT)
		{
			//	Function Templates (either the template is a function template or the statement is invalid)

			rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected a typlex as part of a template function def/decl. Got %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
				return E_SYNTAX ;
			}
			ct = t_end ;
			obj_name = X[ct].value ;

			if (X[ct].type == TOK_WORD && X[ct+1].type == TOK_ROUND_OPEN)
			{
				Z.Clear() ;
				rc = ProcFuncDef(Stmt, thisKlass, p_tpx, obj_name, (EAttr) (FN_ATTR_STDFUNC | FN_ATTR_TEMPLATE), SCOPE_PUBLIC, t_end, ct + 1, CTX_CLASS) ;
				if (rc != E_OK)
					{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

				if (t_end == ct)
					{ g_EC[_hzlevel()].Printf("%s. function template uptake failure!\n", __func__) ; return E_SYNTAX ; }

				ultimate = ct = t_end ;
				Stmts.Add(Stmt) ;

				if (bTemplate)
				{
					g_CurTmplArgs.Clear() ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: TMPL ARGS OFF: Count = %d\n", __func__, __LINE__, *StrName(), X[ct].line, g_CurTmplArgs.Count()) ;
				}
				return E_OK ;
			}
		}
	}

	/*
	**	Handle class/struct/union/enum definitions including forward declarations - these may occur within any context.
	*/

	//	Handle class/struct definitions and forward declarations (ProcClass decides which of these has been encountered so it updates the file statement list)
	if ((X[ct].type == TOK_CLASS || X[ct].type == TOK_STRUCT)
		&& X[ct+1].type == TOK_WORD
		&& (X[ct+2].type == TOK_END || X[ct+2].type == TOK_OP_SEP || X[ct+2].type == TOK_CURLY_OPEN))
	{
		tmpErr.Clear() ;
		rc = ProcClass(Stmt, thisKlass, t_end, ct, codeLevel+1) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; g_EC[_hzlevel()].Printf("%s (%d): ProcClass failed\n", __func__, __LINE__) ; return rc ; }

		if (t_end == ct)
			{ g_EC[_hzlevel()].Printf("%s. class uptake failure!\n", __func__) ; return E_SYNTAX ; }

		ct = t_end ;
		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Class definitions or forward declarations must be terminated with ;\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		Stmt.m_eSType = STMT_TYPEDEF ;
		Stmt.m_Object = X[ct + 2].value ;
		Stmt.m_End = ultimate = ct ;
		ultimate = ct ;
		Stmts.Add(Stmt) ;

		if (bTemplate)
		{
			g_CurTmplArgs.Clear() ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: TMPL ARGS OFF: Count = %d\n", __func__, __LINE__, *StrName(), X[ct].line, g_CurTmplArgs.Count()) ;
		}
		return E_OK ;
	}

	//	Handle union definitions and forward declarations
	if (X[ct].type == TOK_UNION)
	{
		tmpErr.Clear() ;
		rc = ProcUnion(Stmt, thisKlass, t_end, ct, codeLevel+1) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; g_EC[_hzlevel()].Printf("%s. Proc Union failed!\n", __func__) ; return rc ; }

		Stmts.Add(Stmt) ;
		ct = t_end ;
		ct++ ;
		ultimate = ct ;
		g_EC[_hzlevel()].Printf("%s. Proc Union OK!\n", __func__) ;
		return E_OK ;
	}

	//	Handle enum definitions and forward declarations.
	if (X[ct].type == TOK_ENUM)
	{
		tmpErr.Clear() ;
		rc = ProcEnum(Stmt, thisKlass, t_end, ct) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

		if (t_end == ct)
			{ g_EC[_hzlevel()].Printf("%s. Enum uptake failure!\n", __func__) ; return E_SYNTAX ; }

		ct = t_end ;
		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Enum definitions or forward declarations must be terminated with ;\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		Stmts.Add(Stmt) ;
		ct++ ;
		ultimate = ct ;
		return E_OK ;
	}

	//	Within a class/ctmpl definition, every possible statement must either be:-
	//
	//		a)	Declaration of variable or function and start with a DATATYPE
	//		b)	Definition of a sub-class and start with the word 'class' or 'struct'
	//		c)	Definition of a sub-union and start with the word 'union'
	//		d)	Declaration of a casting operator and start with the word 'operator'
	//
	//	Here we deal only with case (d) and a subset of (a), namely where the statement defines a constructor/destructor

	if (X[ct].value == "operator" && !X[ct+1].IsOperator())
	{
		//	Case of casting operator decl/def

		Z.Clear() ;
		rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct+1) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s. Casting operators are of the form 'operator type (args);' Type not establihed\n", __func__) ;
			return rc ;
		}

		Z.Clear() ;
		Z << X[ct].value ;
		for (ct++ ; ct < t_end ; ct++)
		{
			Z.AddByte(CHAR_SPACE) ;
			Z << X[ct].value ;
		}
		obj_name = Z ;

		g_EC[_hzlevel()].Printf("%s (%d) Adding a casting operator to class %s\n", __func__, __LINE__, *thisKlass->StrName()) ;
		Z.Clear() ;
		rc = ProcFuncDef(Stmt, thisKlass, p_tpx, obj_name, FN_ATTR_OPERATOR, opRange, t_end, ct, CTX_CLASS) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to add a casting (%s) to class %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name, *thisKlass->StrName()) ;
			return rc ;
		}
		ultimate = ct = t_end ;
		Stmts.Add(Stmt) ;
		return E_OK ;
	}

	if (X[ct].value != "class" && X[ct].value != "struct" && X[ct].value != "union")
	{
		//	Handle the class destructors
		if (X[ct].value == "~" && X[ct+1].value == thisKlass->StrName() && X[ct+2].type == TOK_ROUND_OPEN)
		{
			obj_name = X[ct].value + X[ct+1].value ;
			p_tpx.Clear() ;

			g_EC[_hzlevel()].Printf("%s (%d) Adding a destructor to class %s\n", __func__, __LINE__, *thisKlass->StrName()) ;
			Z.Clear() ;
			rc = ProcFuncDef(Stmt, thisKlass, p_tpx, obj_name, FN_ATTR_DESTRUCTOR, opRange, t_end, ct + 2, CTX_CLASS) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to add a destructor to klass %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *thisKlass->StrName()) ;
				return rc ;
			}
			ultimate = ct = t_end ;
			Stmts.Add(Stmt) ;
			return E_OK ;
		}

		//	Handle constructors
		if (X[ct].value == thisKlass->StrName())
		{
			Z.Clear() ;
			rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: At this point, only a DATATYPE is allowed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return rc ;
			}

			//if (X[ct].value == thisKlass->StrName() && X[t_end+1].type == TOK_ROUND_OPEN)
			if (X[t_end].type == TOK_ROUND_OPEN)
			{
				ct = t_end ;
				g_EC[_hzlevel()].Printf("%s (%d) Adding a constructor to class %s\n", __func__, __LINE__, *thisKlass->StrName()) ;

				p_tpx.Clear() ;
				p_tpx.SetType(thisKlass) ;
				p_tpx.m_Indir = 1 ;

				Z.Clear() ;
				rc = ProcFuncDef(Stmt, thisKlass, p_tpx, thisKlass->StrName(), FN_ATTR_CONSTRUCTOR, opRange, t_end, ct, CTX_CLASS) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to add a constructor to klass %s\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *thisKlass->StrName()) ;
					return rc ;
				}
				ultimate = ct = t_end ;
				Stmts.Add(Stmt) ;
				return E_OK ;
			}
		}
	}

 	//	At this juncture, all remaining legal statement forms would (after any optional keywords already processed above) begin with a typlex and
	//	then a word that must either amount to a variable or function name.

	Z.Clear() ;
	rc = GetTyplex(p_tpx, thisKlass, 0, t_end, ct) ;
	if (rc != E_OK)
	{
		g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected a typlex as part of a variable or function def/decl. At tok %d Got %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, ct, *X[ct].value) ;
		return E_SYNTAX ;
	}

	ct = t_end ;

	if (!p_tpx.Type())
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected a typlex to preceed object %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	/*
	**	After the typlex the next item in the statement must be the [fully qualified] name of either a function or variable. The first step is to
	**	see if the current token is the start of a construct of the form '[namespace::][class::]member'. If it is it should be already declared in
	**	the current scope - as we are here processing code that lies outside of any class or class template or function definition body.
	*/

	pClass = GetClass(t_end, ct) ;

	if (pClass)
		ct = t_end + 2 ;

	//	If a class is involved the current token should be on the class memeber
	if (X[ct].type == TOK_ROUND_OPEN && X[ct+1].type == TOK_OP_MULT && X[ct+2].type == TOK_WORD && X[ct+3].type == TOK_ROUND_CLOSE
		&& X[ct+4].type == TOK_ROUND_OPEN)
	{
		//	Investigate function pointer
		obj_name = X[ct+2].value ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Adding a func_ptr of name %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;

		ceTyplex	a_tpx ;		//	Argument typlex

		//	Declare ceVar instance
		pVar = new ceVar() ;

		//	Read arguments. Note these must not be named.
		for (ct += 5 ; rc == E_OK ;)
		{
			Z.Clear() ;
			rc = GetTyplex(a_tpx, thisKlass, 0, t_end, ct) ;
			if (rc != E_OK)
				{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; break ; }

			ct = t_end ;
			if (X[ct].type == TOK_WORD)
				ct++ ;

			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Tok %s Adding a fnptr arg of %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, *a_tpx.Str()) ;

			p_tpx.m_Args.Add(a_tpx) ;
			if (X[ct].type == TOK_SEP)
				{ ct++ ; continue ; }

			if (X[ct].type == TOK_ROUND_CLOSE)
				{ ct++ ; break ; }

			rc = E_SYNTAX ;
			break ;
		}

		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: FAILED to add func_ptr of name %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
			return rc ;
		}

		p_tpx.SetAttr(DT_ATTR_FNPTR) ;
		rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;

		rc = tgt_ET->AddEntity(this, pVar, __func__) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: FAILED to add entity func_ptr of name %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
			return rc ;
		}
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: DONE add entity func_ptr of name %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;

		if (X[ct].type == TOK_OP_EQ)
			ct += 2 ;

		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed func_ptr (expected a ; after (*func_name)(arg-set) form. Got %s)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		Stmt.m_eSType = STMT_VARDCL_FNPTR ;
		Stmt.m_Object = obj_name ;
		Stmt.m_End = ultimate = ct ;
		Stmts.Add(Stmt) ;

		pt = X[ct].comPost ;
		if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
		{
			if (!g_Project.m_bSystemMask)
				slog.Out("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the function pointer\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		}
		else
		{
			pVar->SetDesc(P[pt].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		}

		return E_OK ;
	}

	//	Deal with special case of operator functions
	if (X[ct].value == "operator")
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing externally defined non-casting operator to class %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, thisKlass ? *thisKlass->StrName() : "null") ;

		//	A non-casting operator function decl/def
		if (X[ct+1].value == "new" || X[ct+1].value == "delete")
		{
			if (X[ct+2].type == TOK_SQ_OPEN && X[ct+3].type == TOK_SQ_CLOSE)
			{
				obj_name = X[ct].value + X[ct+1].value + "[]" ;
				ct += 4 ;
			}
			else
			{
				obj_name = X[ct].value + X[ct+1].value ;
				ct += 2 ;
			}
		}
		else if (X[ct+1].type == TOK_SQ_OPEN && X[ct+2].type == TOK_SQ_CLOSE)
		{
			obj_name = X[ct].value + "[]" ;
			ct += 3 ;
		}
		else
		{
			if (!X[ct+1].IsOperator())
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: After 'operator' a C++ operator is expected - Got %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct+1].value) ;
				return E_SYNTAX ;
			}

			obj_name = X[ct].value + X[ct+1].value ;
			ct += 2 ;
		}

		Z.Clear() ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing externally defined non-casting operator to class %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, *thisKlass->StrName()) ;
		rc = ProcFuncDef(Stmt, thisKlass, p_tpx, obj_name, FN_ATTR_OPERATOR, SCOPE_PUBLIC, t_end, ct, CTX_CLASS) ;

		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Aborting after failed fn process call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		ultimate = ct = t_end ;

		if (pClass)
			g_EC[_hzlevel()].Printf("%s (%d) Added non-casting operator (%s) to class %s\n", __func__, __LINE__, *obj_name, *pClass->StrName()) ;
		else
			g_EC[_hzlevel()].Printf("%s (%d) Added a non-member non-casting operator (%s)\n", __func__, __LINE__, *obj_name) ;

		Stmts.Add(Stmt) ;
		return E_OK ;
	}

	/*
	**	Now confining investigations to:-
	**
	**	Single variable declaration		of the form [keywords] class/type name ;
	**	Variable array declarations		of the form [keywords] class/type name[noElements] ;
	**	Single variable declarations	of the form [keywords] typlex var_name(value_args) ;
	**	Function declarations			of the form [keywords] typlex func_name (type_args) [const] ;
	**	Function definitions			of the form [keywords] typlex func_name (type_args) [const] {body} ;
	*/

	//	Now expect variable or function name
	if (X[ct].type != TOK_WORD)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected variable or function name. Got %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	//	Obtain the object name
	Stmt.m_Object = obj_name = X[ct].value ;
	ct++ ;

	if (!pClass)
		obj_fqname = obj_name ;
	else
	{
		obj_fqname = pClass->Fqname() ;
		obj_fqname += "::" ;
		obj_fqname += obj_name ;
	}

	/*
	**	Investigate variable declaration.
	*/

	//	Investigate variable array declaration
	if (X[ct].type == TOK_SQ_OPEN)
	{
		//	For an opening square bracket to appear after the object name means the statement must be a declaration of an array of variables. There are now two
		//	possible forms. Either there is no assignment in which case the number of elements must be specified within the [] block OR the array is assigned a
		//	value, supplied in the form {values}. In the latter case the number of elements can be left unspecified within the [] since this is derived from the
		//	supplied initialization array. If the number of elements is specified within the [] block and an assignement applies, then the number of elements in
		//	the supplied initialization array must agree with this number. There is a third case allowed at the file level in which arrays declared as extern do
		//	not need to specify the number of elements and must not be assigned an array of values.

		match = X[ct].match ;

		ct++ ;
		if (p_tpx.m_Indir >= 0)
			p_tpx.m_Indir++ ;

		g_EC[_hzlevel()].Printf("%s (%d) S15 Adding a variable array of name %s\n", __func__, __LINE__, *obj_name) ;

		//	Special case of an extern array 
		if (X[ct].type == TOK_SQ_CLOSE && X[ct+1].type == TOK_END)
		{
			if (!bExtern)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Empty arrays may only be extern\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}

			//	Check if the object exists in the entity table
			pEnt = tgt_ET->GetEntity(obj_name) ;

			if (pEnt)
			{
				//	If the entity is already declared, check the entity is a variable and of the correct typlex
				if (pEnt->Whatami() != ENTITY_VARIABLE)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Variable %s already declared as a %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, pEnt->EntDesc()) ;
					return E_SYNTAX ;
				}
				pVar = (ceVar*) pEnt ;
				if (pVar->Typlex() != p_tpx)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: WARNING variable %s already has type %s, not %s\n",
						__func__, __LINE__, *StrName(), X[ct].line, *obj_name, *pVar->Typlex().Str(), *p_tpx.Str()) ;
			}
			else
			{
				//	Declare the entity for the first time
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding extern variable %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
				pVar = new ceVar() ;
				rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding extern variable %s as %s Failed\n",
						__func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
					return rc ;
				}

				rc = tgt_ET->AddEntity(this, pVar, __func__) ;
				if (rc != E_OK)
					{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }
			}

			ct++ ;
			Stmt.m_eSType = STMT_VARDCL_ARRAY ;
			Stmt.m_End = ultimate = ct ;
			Stmts.Add(Stmt) ;

			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Added extern variable\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;

			pt = X[ct].comPost ;
			if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
			{
				if (!g_Project.m_bSystemMask)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards m_Error. Expected a comment for the extern array\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			}
			else
			{
				pVar->SetDesc(P[pt].value) ;
				P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
			}

			return E_OK ;
		}

		//	Get the array size if it is available
		if (X[ct].type != TOK_SQ_CLOSE)
		{
			Z.Clear() ;
			rc = AssessExpr(Stmts, theTpx, theVal, thisKlass, pFET, t_end, ct, match + 1, 0) ;
			if (!theTpx.Type())
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d: Expression assumed to be array size (%s) failed to evaluate\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
				return E_SYNTAX ;
			}
			ct = match ;
			g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d: Array sise now defined. Tok now %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

			if (theTpx.Basis() != ADT_ENUM && !(theTpx.Basis() & (CPP_DT_INT | CPP_DT_UNT)))
			{
				g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d: Expression assumed to be array size does not evaluate to an integer\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}
		}

		//	Variable array of defined or undefined number of elements with assignment
		if (X[ct].type == TOK_SQ_CLOSE && X[ct+1].type == TOK_OP_EQ)
		{
			ct += 2 ;
			if (X[ct].type != TOK_CURLY_OPEN)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed variable array declaration - expected opening '{' of initializing array\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}

			//	Check for comment after the opening curly brace
			pt = X[ct].comPost ;
			ct = X[ct].match + 1 ;

			//	Check for termination
			if (X[ct].type != TOK_END)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed variable initializer (expected a ; after multiple value)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}

			//	Check if the object exists in the entity table
			pEnt = tgt_ET->GetEntity(obj_name) ;

			if (pEnt)
			{
				//	If the entity is already declared, check the entity is a variable and of the correct typlex
				if (pEnt->Whatami() != ENTITY_VARIABLE)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Variable %s already declared as a %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, pEnt->EntDesc()) ;
					return E_SYNTAX ;
				}
				pVar = (ceVar*) pEnt ;
				if (pVar->Typlex() != p_tpx)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: WARNING variable %s already has type %s, not %s\n",
						__func__, __LINE__, *StrName(), X[ct].line, *obj_name, *pVar->Typlex().Str(), *p_tpx.Str()) ;
			}
			else
			{
				//	Declare the entity for the first time
				pVar = new ceVar() ;
				rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
				if (rc == E_OK)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Assign array %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
				else
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Failed to assigned array %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;
					return rc ;
				}

				rc = tgt_ET->AddEntity(this, pVar, __func__) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Failed to add assigned array %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;
					return rc ;
				}
			}

			if (rc == E_OK)
			{
				if (pt != 0xffffffff)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Added a post= comment\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;
					pVar->SetDesc(P[pt].value) ;
				}

				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Added an assigned array\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name) ;
				Stmt.m_eSType = STMT_VARDCL_ASSIG ;
				Stmt.m_End = ct ;
				Stmts.Add(Stmt) ;
				ultimate = ct ;
			}
			return rc ;
		}

		if (X[ct].type == TOK_SQ_CLOSE && X[ct+1].type == TOK_END)
		{
			//	Variable array of defined size without assignment.

			pVar = new ceVar() ;

			rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding assign array %s as %s Failed\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
				return rc ;
			}

			ct++ ;
			rc = tgt_ET->AddEntity(this, pVar, __func__) ;
			if (rc != E_OK)
				{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

			Stmt.m_eSType = STMT_VARDCL_ARRAY ;
			Stmt.m_End = ultimate = ct ;
			Stmts.Add(Stmt) ;

			pt = X[ct].comPost ;
			if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
			{
				if (!g_Project.m_bSystemMask)
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the array\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			}
			else
			{
				pVar->SetDesc(P[pt].value) ;
				P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
			}

			return E_OK ;
		}

		//	Error
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed variable array declalation: Either ar_name[noElements] ; OR initializing array expected\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}

	//	Not an array so statement may be a single variable
	if (X[ct].type == TOK_OP_EQ)
	{
		//	Single variable assigned a single value. Check for comment after the = sign
		pt = X[ct].comPost ;
		ct++ ;

		g_EC[_hzlevel()].Printf("%s (%d) S15 Adding and assigning variable of name %s\n", __func__, __LINE__, *obj_name) ;
		Z.Clear() ;
		rc = AssessExpr(Stmts, theTpx, theVal, thisKlass, pFET, t_end, ct, stm_limit, 0) ;
		if (!theTpx.Type())
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d: The RHS (%s) of an assignment must evaluate correctly\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}
		ct = t_end ;

		pVar = new ceVar() ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding assign array %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
		rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) Failed to init variable %s\n", __func__, __LINE__, *obj_name) ;
			return rc ;
		}
		if (pt != 0xffffffff)
			pVar->SetDesc(P[pt].value) ;

		rc = tgt_ET->AddEntity(this, pVar, __func__) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; g_EC[_hzlevel()].Printf("%s (%d) Failed to add variable %s\n", __func__, __LINE__, *obj_name) ; return rc ; }
		g_EC[_hzlevel()].Printf("%s (%d) DONE add variable %s\n", __func__, __LINE__, *obj_name) ;

		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Malformed variable initializer (expected a ; after single value. Got %s)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		//	Check for comment
		pt = X[ct].comPost ;
		if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
		{
			if (!pVar->StrDesc() && !g_Project.m_bSystemMask && X[ct].col > 0)
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the variable decl and assign (%s)\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
		}
		else
		{
			pVar->SetDesc(P[pt].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		}

		Stmt.m_eSType = STMT_VARDCL_ASSIG ;
		Stmt.m_End = ultimate = ct ;
		Stmts.Add(Stmt) ;
		return E_OK ;
	}

	if (X[ct].type == TOK_END)
	{
		//	Statement terminated after object name meaning a variable declaration of the form [keywords] class/type name ;
		pVar = new ceVar() ;

		rc = pVar->InitDeclare(this, pClass, p_tpx, opRange, obj_name) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) Failed to init variable %s\n", __func__, __LINE__, *obj_name) ;
			return rc ;
		}

		rc = tgt_ET->AddEntity(this, pVar, __func__) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; g_EC[_hzlevel()].Printf("%s (%d) Failed to add variable %s\n", __func__, __LINE__, *obj_name) ; return rc ; }
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Added variable %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;

		Stmt.m_eSType = STMT_VARDCL_STD ;
		Stmt.m_End = ultimate = ct ;
		Stmts.Add(Stmt) ;

		//	Check for comment
		pt = X[ct].comPost ;
		if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
		{
			if (!g_Project.m_bSystemMask && X[ct].col > 0)
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Coding Standards error. Expected a comment for the variable (%s)\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *obj_name) ;
		}
		else
		{
			pVar->SetDesc(P[pt].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		}

		return E_OK ;
	}

	/*
	**	While the opening '[' at this juncture signals an array of variable, the only remaining legal token is the opening '(' which confines the
	**	analysis to one of the following:-
	**
	**	Variable declarations -> [keywords] typlex var_name(constructor_args values) ;
	**	Function declarations -> [keywords] typlex func_name (type_args) [const] ;
	**	Function definitions  -> [keywords] typlex func_name (type_args) [const] {body} ;
	*/

	if (X[ct].type != TOK_ROUND_OPEN)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected function argument block. Got %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	//	Will be a function if the next token amounts to a type
	if (X[ct+1].type != TOK_ROUND_CLOSE)
		rc = GetTyplex(s_tpx, thisKlass, 0, t_end, ct + 1) ;

	if (rc == E_OK)
	{
		//	Function declarations -> [keywords] typlex func_name (args) [const] ;
		//	Function definitions  -> [keywords] typlex func_name (args) [const] {body} ;

		if (!pClass)
			tmpHost = thisKlass ;
		else
			tmpHost = pClass ;

		g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Processing ordinary function to class %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, tmpHost ? *tmpHost->StrName() : "null") ;

		Z.Clear() ;
		rc = ProcFuncDef(Stmt, tmpHost, p_tpx, obj_name, FN_ATTR_STDFUNC, SCOPE_PUBLIC, t_end, ct, CTX_CLASS) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: case 4 - Aborting after failed fn process call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		ultimate = ct = t_end ;
		Stmts.Add(Stmt) ;
		return E_OK ;
	}

	match = X[ct].match ;
	if (X[match+1].type == TOK_END)
	{
		//	Test S17, that of a variable declaration in which a class/ctmpl is instantiated and initialized by a contructor with argument(s). For
		//	this to be the case, the class/ctmpl must have a constructor that can be called in this way.

		if (p_tpx.Type() && p_tpx.Whatami() == ENTITY_CLASS)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Examining scenario 17 (var inst by constructor args)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;

			//	The type is a class or class template so we look for it's constructors and test if the statement is calling a constructor with
			//	valid arguments

			hzVect<ceStmt>			sx ;
			hzList<ceFunc*>::Iter	fi ;
			hzList<ceVar*>::Iter	ai ;

			hzArray<ceTyplex>	valList ;			//	Supplied argument values

			ceTyplex	tmpTpx ;
			ceVar*		theArg ;
			ceClass*	clsX ;
			hzString	tmp_name ;
			uint32_t	c_tmp ;
			uint32_t	nVals ;

			if (p_tpx.Whatami() == ENTITY_CLASS)
				{ clsX = (ceClass*) p_tpx.Type() ; fi = clsX->m_Funcs ; tmp_name = clsX->StrName() ; }
			tmpHost = (ceKlass*) p_tpx.Type() ;

			for (; fi.Valid() ; fi++)
			{
				pFunc = fi.Element() ;
				valList.Clear() ;

				g_EC[_hzlevel()].Printf("%s (%d) Case 1 Testing func %s\n", __func__, __LINE__, *pFunc->StrName()) ;
				if (pFunc->StrName() != tmp_name)
					{ pFunc = 0 ; continue ; }

				//	We have a constructor to test
				for (c_tmp = ct + 1 ; c_tmp < match ; c_tmp++)
				{
					g_EC[_hzlevel()].Printf("%s (%d) S17 testing token %d %s\n", __func__, __LINE__, c_tmp, *X[c_tmp].value) ;
					Z.Clear() ;
					rc = AssessExpr(sx, tmpTpx, theVal, tmpHost, 0, t_end, c_tmp, match + 1, 0) ;
					if (!tmpTpx.Type())
						{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; pFunc = 0 ; break ; } 

					valList.Add(tmpTpx) ;
					c_tmp = t_end ;

					if (X[c_tmp].type != TOK_SEP)
						break ;
				}

				if (!pFunc)
					{ pFunc = 0 ; continue ; }

				//	Match function to args
				g_EC[_hzlevel()].Printf("%s (%d) Case 2 Testing func %s\n", __func__, __LINE__, *pFunc->StrName()) ;
				if (pFunc->m_Args.Count() != valList.Count())
					{ pFunc = 0 ; continue ; }

				for (nVals = 0, ai = pFunc->m_Args ; ai.Valid() ; nVals++, ai++)
				{
					theArg = ai.Element() ;
					tmpTpx = valList[nVals] ;

					if (theArg->Typlex().Testset(tmpTpx) != E_OK)
						{ pFunc = 0 ; break ; }
				}

				if (pFunc)
					break ;
			}

			if (pFunc)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: S17 Case 1 Have var decl of the form {[keywords] class_type var_name(constructor_args);}\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;

				pVar = new ceVar() ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: Adding assign array %s as %s\n", __func__, __LINE__, *StrName(), X[ct].line, *obj_name, *p_tpx.Str()) ;
				rc = pVar->InitDeclare(this, tmpHost, p_tpx, SCOPE_GLOBAL, obj_name) ;	//, obj_fqname) ;
				if (rc != E_OK)
					return rc ;

				rc = tgt_ET->AddEntity(this, pVar, __func__) ;
				if (rc != E_OK)
					{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

				ct = match + 2 ;
				Stmt.m_eSType = STMT_VARDCL_CONS ;
				Stmt.m_End = ultimate = ct ;
				Stmt.m_Object = obj_name ;
				Stmts.Add(Stmt) ;
				return E_OK ;
			}
		}
	}

	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed Statement\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
	return E_NOTFOUND ;
}

/*
**	Section X:	Function processing
*/

hzEcode	ceFile::ProcFuncArg	(hzList<ceVar*>& arglist, hzChain& cname, ceKlass* pHost, uint32_t& ultimate, uint32_t start)
{
	//	Process function arguments on behalf of ProcFuncDef, which processes fuction declarations and definitions.
	//
	//	This function moves from the opening to the closing round brace of the set of function arguments, gathering the list of arguments (variables). The arguments may not always
	//	have a name but will have a typlex.
	//
	//	Args:	1) args		The list of arguments (as typlexes) the function has.
	//			2) cname	The compiled name of the function. As this function is recursive, cname must be cleared beforehand by the original caller.
	//			2) pHost	Parent entity if applicable (host class/class template).
	//			3) ultimate	Reference to post process token position, set to the closing round bracket of the function arguments.
	//			4) start	The starting token
	//
	//	Ret:	1)	E_SYNTAX	If there is a syntax or format error
	//			2)	E_OK		If operation successful

	_hzfunc("ceFile::ProcFuncArg") ;

	hzChain		fa_Err ;			//	For reporting GetTyplex() errors
	hzChain		Z ;					//	For compiling full function name
	hzString	S ;					//	Temporary type name for lookup
	hzAtom		atom ;				//	For default argument values
	ceEntity*	pEnt ;				//	For token lookups
	ceVar*		ardwark ;			//	To be set and added to args vector
	ceTyplex	a_tpx ;				//	Typlex for the new argument
	ceTyplex	t_tpx ;				//	Test typlex for the new argument default
	hzString	argname ;			//	Optional argument name
	uint32_t	ct ;				//	Token iterator
	uint32_t	t_end ;				//	Jump ahead iterator
	uint32_t	bVararg = false ;	//	Is arg a variable ...
	bool		bNeg = false ;		//	Minus indicator
	hzEcode		rc ;				//	Return code

	arglist.Clear() ;
	ct = start ;

	//	Expect an argument block to begin with an opening round bracket
	if (X[ct].type != TOK_ROUND_OPEN)
		Fatal("%s. %s line %d col %d: Expected an opening '(' to start argument list\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
	ultimate = X[ct].match ;

	for (; ct <= ultimate ; ct++)
		Z << X[ct].value ;
	S = Z ;
	Z.Clear() ;
	fa_Err.Printf("%s Processing Func Args %s\n", __func__, *S) ;
	ct = start ;

	//	In the case of an empty argument block, do nothing
	if (X[ct+1].type == TOK_ROUND_CLOSE)
		{ cname += "(void)" ; return E_OK ; }
	if (X[ct+1].value == "void" && X[ct+2].type == TOK_ROUND_CLOSE)
		{ cname += "(void)" ; return E_OK ; }
	ct++ ;
	cname.AddByte(CHAR_PAROPEN) ;

	//	Process the arguments
	for (; ct != ultimate ;)
	{
		//	Establish argument type and allocate an ceVar instance for it

		a_tpx.Clear() ;
		argname.Clear() ;

		if ((rc = GetTyplex(a_tpx, pHost, 0, t_end, ct)) != E_OK)
		{
			fa_Err.Printf("%s (%d) %s line %d col %d: Could not establish argument type for %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			slog.Out(fa_Err) ;
			return E_SYNTAX ;
		}
		ct = t_end ;

		if (!a_tpx.Type())
		{
			fa_Err.Printf("%s (%d) %s line %d col %d: Could not establish argument type for %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			slog.Out(fa_Err) ;
			return E_SYNTAX ;
		}

		S = a_tpx.Str() ;
		cname << S ;

		//	Get the argument name if supplied.
		if (X[ct].type == TOK_WORD)
		{
			argname = X[ct].value ;
			ct++ ;
		}

		//	Handle the case where the arg is a function pointer
		if (X[ct].type == TOK_ROUND_OPEN && X[ct+1].type == TOK_OP_MULT)
		{
			ct += 2 ;

			if (X[ct].type == TOK_ROUND_CLOSE && X[ct+1].type == TOK_ROUND_OPEN)
				ct += 2 ;
			else if (X[ct].type == TOK_WORD && X[ct+1].type == TOK_ROUND_CLOSE && X[ct+2].type == TOK_ROUND_OPEN)
				{ argname = X[ct].value ; ct += 3 ; }
			else
			{
				fa_Err.Printf("%s (%d) %s line %d col %d: Unacceptable function pointer construct (%s)\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[ct].Show()) ;
				slog.Out(fa_Err) ;
				return E_SYNTAX ;
			}

			for (; rc == E_OK ;)
			{
				rc = GetTyplex(t_tpx, pHost, 0, t_end, ct) ;
				if (rc != E_OK)
				{
					fa_Err.Printf("%s (%d) %s line %d col %d: Unacceptable function pointer argument (%s)\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[ct].Show()) ;
					slog.Out(fa_Err) ;
					return E_SYNTAX ;
				}
				ct = t_end ;

				a_tpx.m_Args.Add(t_tpx) ;
				if (X[ct].type == TOK_SEP)
					{ ct++ ; continue ; }

				if (X[ct].type == TOK_ROUND_CLOSE)
					{ ct++ ; break ; }
				rc = E_SYNTAX ;
			}

			if (rc != E_OK)
			{
				fa_Err.Printf("%s (%d) %s line %d col %d: Unacceptable function pointer construct (%s)\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[ct].Show()) ;
				slog.Out(fa_Err) ;
				return E_SYNTAX ;
			}

			a_tpx.SetAttr(DT_ATTR_FNPTR) ;
		}

		//	Handle the case where argument name is postpended with an elipsis (...)
		if (X[ct].type == TOK_ELIPSIS)
		{
			cname += "..." ;
			bVararg = true ;
			ct++ ;
		}

		//	Handle the case where the variable is actually an array
		if (X[ct].type == TOK_SQ_OPEN)
		{
			if (bVararg)
			{
				fa_Err.Printf("%s (%d) %s line %d col %d: A variable cannot be an array if it is a varargs of a function\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				slog.Out(fa_Err) ;
				return E_SYNTAX ;
			}

			cname += "[]" ;
			for (ct++ ; X[ct].type != TOK_SQ_CLOSE ; ct++) ;
			ct++ ;
		}

		//	Handle the case where the name (or name[]) is assigned a default value
		if (X[ct].type == TOK_OP_EQ)
		{
			if (bVararg)
			{
				//	Default values cannot apply to varags
				fa_Err.Printf("%s (%d) %s line %d col %d: A variable cannot be assigned a value if it is a varargs of a function\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				slog.Out(fa_Err) ;
				return E_SYNTAX ;
			}
			ct++ ;
			//attrs |= FN_ATTR_DEFAULT ;
			//pFunc->SetAttr(FN_ATTR_DEFAULT) ;

			//	Obtain the default value - Can only be a single literal value?
			bNeg = false ;
			if (X[ct].type == TOK_OP_MINUS)
				{ bNeg = true ; ct++ ; }
			if (X[ct].type & TOKTYPE_MASK_LITERAL)
			{
				rc = ReadLiteral(t_tpx, atom, ct, bNeg) ;
				if (rc != E_OK || !t_tpx.Type())
				{
					fa_Err.Printf("%s (%d) %s line %d col %d: Could not dechiper argument default value\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					slog.Out(fa_Err) ;
					return E_SYNTAX ;
				}
				ct++ ;
			}
			else
			{
				//	Check if token is a number or a literal or a string
				if (bNeg)
				{
					fa_Err.Printf("%s (%d) %s line %d col %d: Illegal negation of a default term\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					slog.Out(fa_Err) ;
					return E_SYNTAX ;
				}
				pEnt = LookupToken(X, pHost, 0, t_end, ct, true, __func__) ;
				if (!pEnt || !pEnt->IsReal())
				{
					fa_Err.Printf("%s (%d) %s line %d col %d: A argument default value must be a literal\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					slog.Out(fa_Err) ;
					return E_SYNTAX ;
				}
				ct = t_end ;
			}

			fa_Err.Printf("%s (%d) %s line %d col %d: Default Arg set to %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *atom.Str()) ;
		}

		//	Create, init and add the new argument
		ardwark = new ceVar() ;
		if (bVararg)
			a_tpx.m_dt_attrs |= DT_ATTR_VARARG ;
		rc = ardwark->InitAsArg(a_tpx, SCOPE_FUNC, argname) ;
		if (rc != E_OK)
		{
		printf("ProcFuncArg case 3 calling Str()\n") ;
			S = a_tpx.Str() ;
			fa_Err.Printf("%s (%d). Failed to init argument %s of typlex %s\n", __func__, __LINE__, *argname, *S) ;
			slog.Out(fa_Err) ;
			return rc ;
		}
		ardwark->m_atom = atom ;
		arglist.Add(ardwark) ;

		//	At this point we would normally have either a comma separator or the closing round bracket to end the argument block. However a comment is
		//	permitted after a comma or in the case of the last argument, before the closing round bracket.
		if (X[ct].type == TOK_COMMENT)
		{
			if (X[ct+1].type != TOK_ROUND_CLOSE)
			{
				fa_Err.Printf("%s (%d) %s line %d col %d: Comments must not directly follow the argument unless it is the last one\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				slog.Out(fa_Err) ;
				return E_SYNTAX ;
			}
			ct++ ;
		}

		if (X[ct].type == TOK_SEP)
		{
			cname.AddByte(CHAR_COMMA) ;
			ct++ ;
			if (X[ct].type == TOK_COMMENT)
				ct++ ;
			continue ;
		}

		if (X[ct].type != TOK_ROUND_CLOSE)
		{
			fa_Err.Printf("%s (%d) %s line %d col %d: Expected round close to end argument block. Got (%s)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			slog.Out(fa_Err) ;
			return E_SYNTAX ;
		}
	}

	cname.AddByte(CHAR_PARCLOSE) ;
	return E_OK ;
}

uint32_t	MatchArg	(ceTyplex& arg, ceTyplex& supp)
{
	//	Determine if the supplied typlex (supp) matches that required by a function argument (arg). Return a rating as to the suitability.

	_hzfunc(__func__) ;

	hzList<ceFunc*>::Iter	opI ;	//	Operations iterator

	ceFunc*		pOper = 0 ;			//	Casting operator of B if present
	ceDatatype*	pDT ;				//	Datatype of B
	hzEcode		rc ;				//	Return code

	if (arg.Type() == supp.Type() && arg.m_Indir == supp.m_Indir)
		return 5 ;

	if (arg.Basis() != ADT_CLASS && supp.Basis() == ADT_CLASS)
	{
		//	Look for casting operators of B that return the typlex of A
		pDT = supp.Type() ;

		for (opI = pDT->m_Ops ; opI.Valid() ; opI++)
		{
			pOper = opI.Element() ;

			if (pOper->m_Args.Count())
				continue ;

			if (pOper->Return().Type() == arg.Type() && pOper->Return().m_Indir == arg.m_Indir)
				return 4 ;
		}
	}

	//	Failing either of the above ...
	rc = arg.Testset(supp) ;
	if (rc == E_OK)
		return 1 ;
	return 0 ;
}

ceFunc*	ceFile::ProcFuncCall	(hzVect<ceStmt>& statements, ceFngrp* pGrp, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, uint32_t codeLevel)
{
	//	Process a function call.
	//
	//	This function expects the current token to be the opening round brace of the argument block. The function group is supplied so has already been identified. The objective is
	//	to determine if there is a function within the group, that matches the set of arguments supplied in the call. If there is,this function is returned. To be a possible match,
	//	all the defined arguments of the function must either:-
	//
	//		1) Be supplied with an expression which evaluates to a value that is of, or could be taken as, the expected data type.
	//		2) Assume a default value (if one has been specified)
	//
	//	Note that ambiguity frequently arises. With literal values for example, the value 0 can fit a very wide range of data types. A 16-bit integer can be supplied where a 32-bit
	//	integer is expected. A null terminated string can be passed where a hzString is expected because hzString has a constructor that takes a null terminated string. These extra
	//	possibilities can give rise to ambiguities where the function group has multiple over-rides, meaning a particular argument cannot itself uniqely identify the function. This
	//	however, can be assumed to be resolvable when more arguments are tested.
	//
	//	Arguments:	1)	statements	Vector of statements
	//				2)	pGrp		The current funtion group
	//				3)	pHost		The function class if any
	//				4)	pFET		The calling function's entity table
	//				5)	ultimate	This will be set to one place after the closing ')' of the argument set
	//				6)	start		The current token (assumed to be on the functon name)
	//				7)	codeLevel	The current code level (for statements)
	//
	//	Returns:	1) A pointer to an ceFunc if the supplied arguments matches exactly one of the functions in the group.
	//				2) null	otherwise.

	_hzfunc("ceFile::ProcFuncCall") ;

	hzList<ceFunc*>::Iter	fI ;	//	Function iterator
	hzList<ceVar*>::Iter	vI ;	//	Variable iterator

	hzArray<ceTyplex>	supplied ;	//	Supplied argument values
	hzList<ceFunc*>		found ;		//	Fuctions matching

	ceTyplex	theTpx ;			//	Current value being considered
	hzAtom		theVal ;			//	Needed here only for the expression assesment
	hzChain		tmpErr ;			//	Function call error
	ceVar*		theArg ;			//	Argument being examined
	ceFunc*		pFunc ;				//	Function to test
	ceFunc*		pCall = 0 ;			//	Singular matching function if found
	uint32_t	t_end ;				//	End of current expression
	uint32_t	nVals ;				//	Number of values
	uint32_t	ct ;				//	Current token iterator
	uint32_t	xt ;				//	Forward token iterator
	uint32_t	rating ;			//	Used to determine if function is best fit
	hzEcode		rc = E_OK ;			//	Return code

	ct = start ;
	if (X[ct].type != TOK_ROUND_OPEN)
	{
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Bad call. Expected to be at start of argument block. Instead at [%s]\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		//slog << err ;
		//Fatal("Terminating process\n") ;
		return 0 ;
	}
	ultimate = X[ct].match ;

	//	Assemble the supplied arguments
	for (ct++ ; ct < ultimate ;)
	{
		//	Find the end of the supplied argument expression. This cannot simply be a case of finding either a comma or the closing round brace since
		//	the argument expression could include a function call which itself has arguments and the closing round brace.

		for (xt = ct ; xt < ultimate && X[xt].type != TOK_SEP ; xt++)
		{
			if (X[xt].type == TOK_ROUND_OPEN)
				{ xt = X[xt].match ; continue ; }
			if (X[xt].type == TOK_SEP || X[xt].type == TOK_ROUND_CLOSE)
				break ;
		}

		//	Treat supplied argumant as expression in its own right
		tmpErr.Clear() ;
		rc = AssessExpr(statements, theTpx, theVal, pHost, pFET, t_end, ct, xt, codeLevel) ;
		if (!theTpx.Type() || rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Function %s Arg %s fails to evaluate\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pGrp->StrName(), *X[ct].value) ;
			break ;
		}

		supplied.Add(theTpx) ;
		ct = t_end ;
		if (X[ct].type == TOK_SEP)
			ct++ ;
	}

	if (rc != E_OK)
		return 0 ;

	//	Kludge for now - don't test where group has only one function
	if (pGrp->m_functions.Count() == 1)
	{
		fI = pGrp->m_functions ;
		pFunc = fI.Element() ;
		return pFunc ;
	}

	//	Test each function for correct call
	for (fI = pGrp->m_functions ; fI.Valid() ; fI++)
	{
		pCall = 0 ;
		pFunc = fI.Element() ;
		pFunc->m_rating = 0 ;

		g_EC[_hzlevel()].Printf("%s (%d) Testing function %s with %d args %d min (%d supplied)\n",
			__func__, __LINE__, *pFunc->Extname(), pFunc->m_Args.Count(), pFunc->m_minArgs, supplied.Count()) ;

		//	Go for easy knock-outs
		if (supplied.Count() < pFunc->m_minArgs)
			//continue ;
			{ g_EC[_hzlevel()].Printf(" - too few args\n") ; continue ; }

		if (pFunc->m_bVariadic == false && supplied.Count() > pFunc->m_Args.Count())
			//continue ;
			{ g_EC[_hzlevel()].Printf(" - too many args\n") ; continue ; }

		//	Case where function is variadic
		if (pFunc->m_bVariadic)
		{
			//	All the defined arguments must be matched by supplied
			nVals = 0 ;
			pCall = pFunc ;

			for (vI = pFunc->m_Args ; pCall && vI.Valid() ; vI++)
			{
				theArg = vI.Element() ;
				theTpx = theArg->Typlex() ;

				rating = MatchArg(theTpx, supplied[nVals]) ;
				if (rating)
					pFunc->m_rating += rating ;
				else
					{ pCall = 0 ; pFunc->m_rating = 0 ; break ; }
				nVals++ ;
			}

			if (pCall)
				{ found.Add(pFunc) ; continue ; }
		}

		//	Case where all args are supplied
		if (supplied.Count() == pFunc->m_Args.Count())
		{
			nVals = 0 ;
			pCall = pFunc ;

			for (vI = pFunc->m_Args ; pCall && vI.Valid() ; vI++)
			{
				theArg = vI.Element() ;
				theTpx = theArg->Typlex() ;

				rating = MatchArg(theTpx, supplied[nVals]) ;
				if (rating)
					pFunc->m_rating += rating ;
				else
					{ pCall = 0 ; pFunc->m_rating = 0 ; break ; }
				nVals++ ;
			}

			if (pCall)
				{ found.Add(pFunc) ; continue ; }
		}

		//	Case where only the minimum args are supplied
		if (supplied.Count() == pFunc->m_minArgs)
		{
			nVals = 0 ;
			pCall = pFunc ;
			for (vI = pFunc->m_Args ; pCall && vI.Valid() ; vI++)
			{
				theArg = vI.Element() ;

				if (theArg->m_atom.IsNull())
				{
					//	The function argument does not have a default
					theTpx = theArg->Typlex() ;

					rating = MatchArg(theTpx, supplied[nVals]) ;
					if (rating)
						pFunc->m_rating += rating ;
					else
						{ pCall = 0 ; pFunc->m_rating = 0 ; break ; }
					nVals++ ;
				}
			}

			if (pCall)
				{ found.Add(pFunc) ; continue ; }
		}
			

		//	Case where some arguments have defaults and the supplied argument list is less then the defined
		if  (supplied.Count() > pFunc->m_minArgs && supplied.Count() < pFunc->m_Args.Count())
		{
			nVals = 0 ;
			for (vI = pFunc->m_Args ; pCall && vI.Valid() ; vI++)
			{
				theArg = vI.Element() ;

				if (theArg->m_atom.IsNull())
				{
					//	The function argument does not have a default
					theTpx = theArg->Typlex() ;

					rating = MatchArg(theTpx, supplied[nVals]) ;
					if (rating)
						pFunc->m_rating += rating ;
					else
						{ pCall = 0 ; pFunc->m_rating = 0 ; break ; }
					nVals++ ;
				}
			}

			if (pCall)
				{ found.Add(pFunc) ; continue ; }
		}
	}

	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Function call %s has %d candidates\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pGrp->StrName(), found.Count()) ;
	if (!found.Count())
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Function call %s has 0 candidates\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pGrp->StrName()) ;
		return 0 ;
	}

	fI = found ;
	pCall = fI.Element() ;

	if (found.Count() == 1)
		return pCall ;

	//	List all that match
	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Function call %s has multiple candidates\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pGrp->StrName()) ;
	xt = 0 ;
	for (fI = found ; fI.Valid() ; fI++)
	{
		pFunc = fI.Element() ;
		g_EC[_hzlevel()].Printf("\t%s, rating %d\n", *pFunc->Extname(), pFunc->m_rating) ;

		if (pFunc->m_rating > xt)
		{
			xt = pFunc->m_rating ;
			pCall = pFunc ;
		}
	}

	//	return 0 ;
	return pCall ;
}

hzEcode	ceFile::ProcCondition	(hzString& summary, uint32_t& ultimate, uint32_t start)
{
	//	Process a conditional expression occurring as part of IF, ELSE_IF, FOR, WHILE and DO_WHILE statements.
	//
	//	The current means of storing the condition is as start and end token numbers.
	//
	//	Arguments:	1)	cond	Vector of strings comprising the condition
	//				2) summary

	_hzfunc("ceFile::ProcCondition") ;

	hzChain		Z ;			//	For summizing expression
	uint32_t	ct ;		//	Current token

	ct = start ;
	if (X[ct].type != TOK_ROUND_OPEN)
	{
		slog.Out("%s (%d) %s line %d cold %d: Expect an opening round brace\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}
	ultimate = X[ct].match ;

	for (; ct <= ultimate ; ct++)
		Z << X[ct].value ;
	summary = Z ;

	return E_OK ;
}

hzEcode	ceFile::ProcCodeStmt	(ceFunc* pFunc, ceKlass* pHost, ceEntbl* pFET, uint32_t& xend, uint32_t start, uint32_t codeLevel)
{
	//	Process a single code body statement.
	//
	//	Note that statements end on the terminating colon. This is of standalone statements but also of conditional statements in which the {} code block is omited because there is
	//	only a single action statement. The end token position in such cases will always be set one place beyond the terminating colon. However in the event that a {} code block is
	//	involved, such as after an if or a for, then the end token position is set to one place beyond the cosing curly brace.
	//
	//	Arguments:	1) pFunc		The current function
	//				2) pHost		The class to which the function belongs if applicable
	//				3) pFET			The function's temporary entity table
	//				4) xend			End token position, set by this function to be one place beyong the last token in the statement (see note)
	//				5) start		Position of first token in the statement
	//				6) codeLevel	The code level

	_hzfunc("ceFile::ProcCodeStmt") ;

	hzChain		Z ;					//	For statement diagnostics
	ceTyplex	theTpx ;			//	Evaluation typlex
	ceTyplex	tmpTpx ;			//	Evaluation typlex
	hzAtom		theVal ;			//	Evaluation value
	hzAtom		tmpVal ;			//	Evaluation value
	hzString	S ;					//	Temp string
	hzString	categ ;				//	Category
	hzString	desc ;				//	Description
	hzString	preceed ;			//	Preceeding comment
	ceEntity*	pEnt = 0 ;			//	Returned by Lookup in the current scope
	ceStmt		Stmt ;				//	Instruction (for in-function code)
	uint32_t	t_end ;				//	Jump ahead iterator
	uint32_t	ct ;				//	Current token
	uint32_t	xt ;				//	Auxilliary token
	uint32_t	match ;				//	Use to store match
	uint32_t	save_ct ;			//	Saved current token
	uint32_t	stm_limit ;			//	End of statement
	uint32_t	nReturns = 0 ;		//	used to seek to end of parenthesis block
	hzEcode		rc = E_OK ;			//	Return code

	if (!pFunc)
		Fatal("%s. No functiom supplied\n", __func__) ;

	//	Examine statement ?
	ct = save_ct = start ;
	Z.Clear() ;

	for (xt = ct ; X[xt].type != TOK_END ; xt++)
	{
		Z << X[xt].value ;
	}
	stm_limit = xt ;
	Z << X[xt].value ;
	S = Z ;
	Z.Clear() ;

	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d to line %d col %d Level %d STATEMENT: %s\n",
		__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[xt].line, X[xt].col, codeLevel, *S) ;

	//	Any statements starting with keywords or data types need to be processed by ProcStatement
	if (X[ct].type == TOK_KW_STATIC || X[ct].type == TOK_VTYPE_CONST)
		goto procstat ;

	//	Special case of lable for goto
	if (X[ct].type == TOK_WORD && X[ct+1].type == TOK_OP_SEP)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Lable %s found\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		ct += 2 ;
	}

	//	Check the standard C++ commands
	if (X[ct].type == TOK_CMD_BREAK)
	{
		//	The 'break' statement may either be used to break out of a for/while loop or to break out of or terminate code in a switch statement
		//	case. The parent statement must therefore be a for/while or a case. All we do here is add the break to the vector of statements

		if (X[ct+1].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: 'break' must be followed by a ;\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		Stmt.m_eSType = STMT_BREAK ;
		Stmt.m_nLine = X[ct].line ;
		Stmt.m_nLevel = codeLevel ;
		Stmt.m_Start = Stmt.m_End = ct ;
		pFunc->m_Execs.Add(Stmt) ;

		xend = ct+1 ;
		goto finish ;
	}

	if (X[ct].type == TOK_CMD_CONTINUE)
	{
		//	The 'continue' statement is always used to return to the top of for/while loop so the parent statement must be a for/while loop. All we do
		//	here is add the continue to the vector of statments

		if (X[ct+1].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: 'continue' must be followed by a ;\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		Stmt.m_eSType = STMT_CONTINUE ;
		Stmt.m_nLine = X[ct].line ;
		Stmt.m_nLevel = codeLevel ;
		Stmt.m_Start = Stmt.m_End = ct ;
		pFunc->m_Execs.Add(Stmt) ;

		xend = ct+1 ;
		goto finish ;
	}

	if (X[ct].type == TOK_CMD_GOTO)
	{
		//	This is a 'goto' followed by a labal name and a terminator

		Stmt.m_nLevel = codeLevel ;
		Stmt.m_nLine = X[ct].line ;
		Stmt.m_Start = ct ;

		ct++ ;
		if (X[ct].type != TOK_WORD)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Goto expects a label\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		Stmt.m_Object = X[ct].value ;
		Stmt.m_End = xend = ct ;

		ct++ ;
		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Goto expects a label followed by a ;\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		Stmt.m_eSType = STMT_GOTO ;
		pFunc->m_Execs.Add(Stmt) ;

		xend = ct ;
		goto finish ;
	}

	if (X[ct].type == TOK_CMD_RETURN)
	{
		//	The return statement is added to the list of statements but also added to the list of returns. Possible return values for the function can then be calculated. In a void
		//	function, the return is followed by a terminator. In all other cases, it is followed by an expression that should be of the same data type as the function.

		nReturns++ ;
		Stmt.m_eSType = STMT_RETURN ;
		Stmt.m_nLevel = codeLevel ;
		Stmt.m_nLine = X[ct].line ;
		Stmt.m_Start = ct ;

		//	Kludge
		ct = stm_limit ;

		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Return expects a label followed by a terminaing ; We have [%s]\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		Stmt.m_End = xend = ct ;
		pFunc->m_Execs.Add(Stmt) ;
		goto finish ;
	}

	if (X[ct].type == TOK_CMD_DELETE)
	{
		//	The delete statement is added to the list of steps. The expression following the delete is process by a call to AssessExpr().

		Stmt.m_eSType = STMT_DELETE ;
		Stmt.m_nLevel = codeLevel ;
		Stmt.m_nLine = X[ct].line ;
		Stmt.m_Start = ct ;

		//	Kludge
		ct = stm_limit ;

		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Delete expects a label followed by a terminaing ; We have [%s]\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}
		Stmt.m_End = xend = ct ;
		pFunc->m_Execs.Add(Stmt) ;
		goto finish ;
	}

	/*
	**	Contemplate branches
	*/

	if (X[ct].type == TOK_CMD_IF)
	{
		//	An IF statement has the general form: 'if (condition) dependent_code' - Where the term dependent_code may be a code block encased in curly
		//	braces or a single line statement. The later may comprise a nested IF statement of course. In the case of a single line, this function is
		//	called again to handle it. In the case of a code block, ProcCodeBody (which calls this function once for each staement in the block) is
		//	called instead.
		//
		//	Note that the IF statement can optionally be followed by a series of one or more ELSE-IF statements and/or an ELSE statement. The ELSE-IF
		//	has the same form as the IF namely 'else if (condition) dependent_code' while the ELSE has the form 'else dependent_code'. The ELSE-IF and
		//	ELSE statments are considered part of the IF statement that directly precedes them. The ELSE-IF and ELSE statements cannot exist except as
		//	extensions to an IF. Consequently, we only look for an ELSE-IF or ELSE once we find an IF.

		Stmt.m_eSType = STMT_BRANCH_IF ;
		Stmt.m_nLevel = codeLevel ;
		Stmt.m_Start = ct ;
		Stmt.m_nLine = X[ct].line ;

		ct++ ;
		if (X[ct].type != TOK_ROUND_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: An 'if' must be followed by a '('\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		//rc = ProcCondition(condExp, Stmt.m_Object, t_end, ct) ;
		rc = ProcCondition(Stmt.m_Object, t_end, ct) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: ProcCondition failed\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		ct = t_end + 1 ;
		Stmt.m_End = xend = ct ;
		pFunc->m_Execs.Add(Stmt) ;

		//	If (condition) constructs must be followed by either a single statement OR a statement block with zero or more statements
		if (X[ct].type == TOK_CURLY_OPEN)
		{
			Z.Clear() ;
			rc = ProcCodeBody(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to proc IF code block\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
			}

			if (t_end != X[ct].match)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to end IF code block on a closing curly brace\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
				rc = E_SYNTAX ;
			}
		}
		else
		{
			Z.Clear() ;
			rc = ProcCodeStmt(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Level %d Failed to proc IF single line\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col, codeLevel) ;
			}

			if (X[t_end].type != TOK_END && X[t_end].type != TOK_CURLY_CLOSE)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Level %d Failed to end IF single line on a semi-colon\n",
					__func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col, codeLevel) ;
				rc = E_SYNTAX ;
			}
		}

		if (rc != E_OK)
			return rc ;

		xend = ct = t_end ;

		if (X[ct+1].type == TOK_COMMENT && (X[ct+2].type == TOK_CMD_ELSE || X[ct+2].type == TOK_CMD_IF))
			ct++ ;

		//	Now go on to process ELSE-IF chain and an ELSE if these are present

		for (; X[ct+1].type == TOK_CMD_ELSE && X[ct+2].type == TOK_CMD_IF ;)
		{
			//	This must be followed by a condition and then in a separate child step, either a single statement or a statement block. Note that the
			//	'if' can be chained by the 'else' command but no account of that is needed here. When encountered the else command is simply added to
			//	the exec list of the function.

			Stmt.Clear() ;
			Stmt.m_eSType = STMT_BRANCH_ELSEIF ;
			Stmt.m_nLevel = codeLevel ;
			Stmt.m_Start = ct ;
			Stmt.m_nLine = X[ct].line ;

			ct += 3 ;
			if (X[ct].type != TOK_ROUND_OPEN)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: An 'if' must be followed by a '('\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return E_SYNTAX ;
			}

			//rc = ProcCondition(condExp, Stmt.m_Object, t_end, ct) ;
			rc = ProcCondition(Stmt.m_Object, t_end, ct) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: ProcCondition failed\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
				return E_SYNTAX ;
			}

			ct = t_end + 1 ;
			Stmt.m_End = ct ;
			pFunc->m_Execs.Add(Stmt) ;

			//	The ELSE-IF (condition) construct must be followed by either a single statement OR a statement block with zero or more statements
			if (X[ct].type == TOK_CURLY_OPEN)
			{
				Z.Clear() ;
				rc = ProcCodeBody(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to proc ELSE-IF code block\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
				}
				if (t_end != X[ct].match)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to end ELSE-IF code block on a closing curly brace\n", *_fn, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					rc = E_SYNTAX ;
				}
			}
			else
			{
				Z.Clear() ;
				rc = ProcCodeStmt(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to proc ELSE-IF single line\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
				}
				if (X[t_end].type != TOK_END)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to end ELSE-IF single line on a semi-colon\n", *_fn, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
					rc = E_SYNTAX ;
				}
			}

			if (rc != E_OK)
				return rc ;
			xend = ct = t_end ;
		}

		if (X[ct+1].type == TOK_COMMENT && X[ct+2].type == TOK_CMD_ELSE)
			ct++ ;

		if (X[ct+1].type == TOK_CMD_ELSE)
		{
			//	Must occur as a follow-on from an if command (be at the same level) and then in a separate child step, either an 'if' or a statement
			//	or statement block. The else could have a code block if it is not followed directly by an if.

			Stmt.Clear() ;
			Stmt.m_eSType = STMT_BRANCH_ELSE ;
			Stmt.m_nLevel = codeLevel ;
			Stmt.m_Start = ct ;
			Stmt.m_nLine = X[ct].line ;
			pFunc->m_Execs.Add(Stmt) ;

			ct += 2 ;
			Z.Clear() ;
			if (X[ct].type == TOK_CURLY_OPEN)
			{
				rc = ProcCodeBody(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Level %d Failed to proc ELSE code block\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col, codeLevel) ;
				}
				if (t_end != X[ct].match)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Level %d Failed to end ELSE code block on a closing curly brace\n",
						__func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col, codeLevel) ;
					rc = E_SYNTAX ;
				}
			}
			else
			{
				rc = ProcCodeStmt(pFunc, pHost, pFET, t_end, ct, codeLevel + 1) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Level %d Failed to proc ELSE single line\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col, codeLevel) ;
				}
				if (X[t_end].type != TOK_END && X[t_end].type != TOK_CURLY_CLOSE)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Level %d Failed to end ELSE single line on a semi-colon or curly brace\n",
						__func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col, codeLevel) ;
					rc = E_SYNTAX ;
				}
			}

			if (rc != E_OK)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to proc ELSE-IF statement\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
				return rc ;
			}
			xend = ct = t_end ;
		}

		goto finish ;
	}

	if (X[ct].type == TOK_CMD_SWITCH)
	{
		//	Switch commands comprise a set of case command blocks which are not normally encased in curly braces and are anyway terminated by a
		//	break statement. The case commands (and the default case command) are all children of the switch command and the switch command may
		//	only have case commands as children.

		Stmt.m_nLevel = codeLevel ;
		Stmt.m_Start = ct ;
		Stmt.m_nLine = X[ct].line ;

		if (X[ct+1].type != TOK_ROUND_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: A switch must be followed by a (condition) block\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		//rc = ProcCondition(condExp, Stmt.m_Object, t_end, ct + 1) ;
		rc = ProcCondition(Stmt.m_Object, t_end, ct + 1) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: ProcCondition failed\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		ct = t_end + 1 ;

		if (X[ct].type != TOK_CURLY_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: A switch statement must have a body\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		xend = X[ct].match ;

		Stmt.m_eSType = STMT_BRANCH_SWITCH ;
		Stmt.m_End = xend ;
		pFunc->m_Execs.Add(Stmt) ;
		ct++ ;

		//	The blocks of code that follow a case (or default) within a switch statement are terminated by a break.
		for (; X[ct].type == TOK_CMD_CASE ;)
		{
			Stmt.m_eSType = STMT_BRANCH_CASE ;
			Stmt.m_nLine = X[ct].line ;
			pFunc->m_Execs.Add(Stmt) ;
			for (ct++ ; X[ct].type != TOK_OP_SEP ; ct++) ;
			ct++ ;

			//	Could be comment here
			if (X[ct].type == TOK_COMMENT)
				ct++ ;

			//	The case command may have no statements and be followed dircetly by a case or a default
			if (X[ct].type == TOK_CMD_CASE)
				continue ;
			if (X[ct].type == TOK_CMD_DEFAULT)
				break ;

			for (; rc == E_OK && X[ct].type != TOK_CMD_BREAK ;)
			{
				Z.Clear() ;
				rc = ProcCodeStmt(pFunc, pHost, pFET, t_end, ct, codeLevel + 1) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Switch-Case statement failed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					return E_SYNTAX ;
				}
				ct = t_end + 1 ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Switch-Case statement passed tok=%s @\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

				if (X[ct].type == TOK_COMMENT)
					ct++ ;

				//	Bear in mind there may not be a break. Could be a return or a drop through
				if (X[ct].type == TOK_CMD_CASE || X[ct].type == TOK_CMD_DEFAULT || X[ct].type == TOK_CURLY_CLOSE)
					break ;
			}
		}

		if (X[ct].type == TOK_CMD_DEFAULT)
		{
			Stmt.m_eSType = STMT_BRANCH_CASE ;
			Stmt.m_nLine = X[ct].line ;
			pFunc->m_Execs.Add(Stmt) ;
			for (ct++ ; X[ct].type != TOK_OP_SEP ; ct++) ;
			ct++ ;

			//	Could be comment here
			if (X[ct].type == TOK_COMMENT)
				ct++ ;

			//	The default command may have no statements
			if (X[ct].type == TOK_END)
				goto finish ;

			for (; rc == E_OK && X[ct].type != TOK_CMD_BREAK ;)
			{
				Z.Clear() ;
				rc = ProcCodeStmt(pFunc, pHost, pFET, t_end, ct, codeLevel + 1) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Switch-Default statement failed\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
					return E_SYNTAX ;
				}
				ct = t_end + 1 ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Switch-Default statement passed tok=%s @\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col, *X[ct].value) ;
				if (X[ct].type == TOK_CURLY_CLOSE)
					break ;
			}
		}

		goto finish ;
	}

	//	Handle for loop
	if (X[ct].type == TOK_CMD_FOR)
	{
		//	A for loop comprises four components as follows:-
		//
		//	1) a set of zero or more statements to be executed at the outset. These are delimited by commas and terminated by a semicolon
		//	2) A condition that will be tested before the main body is executed. This condition is followed by a semicolon
		//	3) A set of zero or more statements to be executed after each iteration of the main body
		//	4) The main body which, like the if statement, can comprise a single line without a {} code body.
		//
		//	Note that any variables declared in part (1) are within scope for parts 2, 3 and 4.

		Stmt.m_nLevel = codeLevel ;
		Stmt.m_Start = ct ;
		Stmt.m_nLine = X[ct].line ;

		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Encountered for loop\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
		ct++ ;
		if (X[ct].type != TOK_ROUND_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: A for command must be followed by a (condition) block\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		ct = X[ct].match ;

		Stmt.m_eSType = STMT_BRANCH_FOR ;
		Stmt.m_End = ct ;
		pFunc->m_Execs.Add(Stmt) ;

		//	If (condition) constructs must be followed by either a single statement OR a statement block with zero or more statements
		ct++ ;
		if (X[ct].type == TOK_END)
		{
			xend = ct ;
			goto finish ;
		}

		Z.Clear() ;
		if (X[ct].type == TOK_CURLY_OPEN)
			rc = ProcCodeBody(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
		else
			rc = ProcCodeStmt(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;

		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to proc FOR statement\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
			return rc ;
		}

		xend = t_end ;
		goto finish ;
	}

	if (X[ct].type == TOK_CMD_DO)
	{
		//	Look for {code_body}while(condition) construct
		if (X[ct+1].type != TOK_CURLY_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Do command must be followed by {code_body}\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		xt = X[ct+1].match ;
		if (X[xt+1].type != TOK_CMD_WHILE)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Do loop not followed by while condition on line %d\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[xt+1].line) ;
			return E_SYNTAX ;
		}

		if (X[xt+2].type != TOK_ROUND_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Do while construct missing a while condition on line %d\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[xt+2].line) ;
			return E_SYNTAX ;
		}
		match = X[xt+2].match ;

		if (X[match+1].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Do while construct expecting ; on line line %d\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[match+1].line) ;
			return E_SYNTAX ;
		}
		xend = match + 1 ;

		Stmt.m_nLevel = codeLevel ;
		Stmt.m_Start = ct ;
		Stmt.m_nLine = X[ct].line ;
		Stmt.m_eSType = STMT_BRANCH_DOWHILE ;
		Stmt.m_End = xend ;
		pFunc->m_Execs.Add(Stmt) ;

		ct++ ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Processing DO-WHILE statement body\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		Z.Clear() ;
		rc = ProcCodeBody(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to proc DO-WHILE statement body\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return rc ;
		}
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Processed DO-WHILE statement body\n", __func__, __LINE__, *StrName(), X[t_end].line, X[t_end].col) ;
		goto finish ;
	}

	//	Handle the while loop on its own
	if (X[ct].type == TOK_CMD_WHILE)
	{
		Stmt.m_nLevel = codeLevel ;
		Stmt.m_Start = ct ;
		Stmt.m_nLine = X[ct].line ;

		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Encountered solo while loop\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
		ct++ ;
		if (X[ct].type != TOK_ROUND_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: A for command must be followed by a (condition) block\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		match = X[ct].match ;

		Stmt.m_eSType = STMT_BRANCH_WHILE ;
		Stmt.m_End = match ;
		pFunc->m_Execs.Add(Stmt) ;

		//	If (condition) constructs must be followed by either a single statement OR a statement block with zero or more statements
		ct = match + 1 ;
		if (X[ct].type != TOK_END)
		{
			Z.Clear() ;
			if (X[ct].type == TOK_CURLY_OPEN)
				rc = ProcCodeBody(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
			else
				rc = ProcCodeStmt(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;

			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to proc FOR statement\n", __func__, __LINE__, *StrName(), X[ct+1].line, X[ct].col) ;
				return rc ;
			}
			ct = t_end ;
		}
		xend = ct ;
		goto finish ;
	}

	/*
 	**	Handle other function specific, non branhing, non declaritave statements
	*/

	//	Case where a delete, ++ or -- operator appears befor a variable
	if (X[ct].type == TOK_CMD_DELETE || X[ct].type == TOK_OP_INCR || X[ct].type == TOK_OP_DECR)
	{
		//	Expect the next token(s) to amount to a declared variable and be followed by a terminator

		Stmt.m_nLevel = codeLevel ;
		Stmt.m_Start = ct ;
		Stmt.m_nLine = X[ct].line ;

		if (X[ct].type == TOK_CMD_DELETE && X[ct+1].type == TOK_SQ_OPEN && X[ct+2].type == TOK_SQ_CLOSE)
			ct += 2 ;

		//pEnt = LookupToken(X, pHost, pFET, t_end, ct + 1, true, __func__) ;
		Z.Clear() ;
		pEnt = GetObject(pFunc->m_Execs, pHost, pFET, t_end, ct+1, codeLevel) ;
		if (!pEnt)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d (tok %s): Not an entity\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		if (pEnt->Whatami() != ENTITY_VARIABLE)
		{
			g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d (tok %s): Not a variable\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct+1].value) ;
			return E_SYNTAX ;
		}

		ct = t_end ;
		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d (tok %s): Expected , or ;\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		//	Add the inc/dec statement to the array of statements
		Stmt.m_eSType = X[ct].type == TOK_CMD_DELETE ? STMT_DELETE : X[ct].type == TOK_OP_INCR ? STMT_VAR_INCB : STMT_VAR_DECB ;
		Stmt.m_End = xend = ct ;
		Stmt.m_Object = pEnt->StrName() ;
		pFunc->m_Execs.Add(Stmt) ;
		goto finish ;
	}

	//	At this point we eliminate statements begining with a literal value - as there is no such thing!
	if (X[ct].type & TOKTYPE_MASK_LITERAL)
	{
		g_EC[_hzlevel()].Printf("%s (%d). %s line %d col %d (tok %s): Illegal literal at start of statement\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
		return E_SYNTAX ;
	}

	//	Moved from above
	if (X[ct].type != TOK_OP_MULT && X[ct].type != TOK_INDIR)
	{
		Z.Clear() ;
		if (GetTyplex(theTpx, pHost, 0, t_end, ct) != E_OK)
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		else
		{
			if (theTpx.Type())
				goto procstat ;
		}
	}

	//	EXPERIMENT
	Z.Clear() ;
	rc = AssessExpr(pFunc->m_Execs, theTpx, theVal, pHost, pFET, t_end, ct, stm_limit, codeLevel + 1) ;
	if (rc == E_OK)
	{
		xend = t_end ;
		goto finish ;
	}
	if (rc != E_OK)
	{
		g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: EXPERIMENT failed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		goto procstat ;
	}

procstat:
	Z.Clear() ;
	rc = ProcStatement(pFunc->m_Execs, pHost, pFET, t_end, save_ct, CTX_CFUNC, codeLevel) ;
	if (rc != E_OK)
	{
		g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: PROC STATEMENT failed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return rc ;
	}
	xend = ct = t_end ;
	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: PROC STATEMENT completed tok=%s @\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

	//	Special case of an assignment to a variable or a dereferenced variable which is then incremented or decremented
	if (X[ct].type == TOK_OP_INCR || X[ct].type == TOK_OP_DECR)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: ODD case 1\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		ct++ ;
	}

	if (X[ct].type == TOK_ROUND_CLOSE || X[ct].type == TOK_END)
	{
		//cs_Err.Printf("%s (%d) %s line %d col %d: ODD case 2\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		ct++ ;
	}

	//	For certain statements, e.g variable declarations, comments are expected directly afterwards.
	//	for (; X[ct].type == TOK_COMMENT ;)
	//	{
	//		cs_Err.Printf("%s (%d) %s line %d col %d: ODD case 3\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
	//		rc = ProcExtComment(ct) ;
	//		if (rc != E_OK)
	//			break ;
	//		preceed = X[ct].value ;
	//		ct++ ;
	//	}

	//	if (X[ct].type == TOK_CURLY_CLOSE)
	//		break ;

	if (rc == E_OK && X[ct].type == TOK_CURLY_OPEN)
	{
		//cs_Err.Printf("%s (%d) %s line %d col %d: ODD case 4\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		Z.Clear() ;
		rc = ProcCodeBody(pFunc, pHost, pFET, t_end, ct, codeLevel+1) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: PROC CodeBody failed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return rc ;
		}
		xend = ct = t_end ;
	}

finish:
	if (rc != E_OK)
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed on level %d\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
	return rc ;
}

hzEcode	ceFile::ProcCodeBody	(ceFunc* pFunc, ceKlass* pHost, ceEntbl* pFET, uint32_t& xend, uint32_t start, uint32_t codeLevel)
{
	//	Process statements that lay within a pair of matching curly braces, be this an entire function definition or a code block within a function. The expected starting position
	//	is the opening brace, the closing position asssuming the statements are successfully processed, will be the closing brace.
	//
	//	The main part of this function is a loop which processes each statement within the code body in turn, by calling ProcCodeStmt. Recursion occurs when statements incorporate
	//	a code body e.g. if (condition) then { ... } OR for (; condition ;) { ... }
	//
	//	Arguments:	1)	pFunc		Current funtion to which the code body belongs
	//				2)	pHost		Class of which the function is a member (if any)
	//				3)	pFET		Function's entity table
	//				4)	xend		Final position token
	//				5)	start		Starting token (must be on the opening curly brace)
	//				6)	codeLevel	The code level
	//				7)	caller		Which function called this
	//
	//	Returns:	1)	E_SYNTAX	In the event of any program error
	//				2)	E_OK		If the code body encountered no errors

	_hzfunc("ceFile::ProcCodeBody") ;

	hzChain		Z ;					//	For statement diagnostics
	ceTyplex	theTpx ;			//	Evaluation typlex
	ceTyplex	tmpTpx ;			//	Evaluation typlex
	hzAtom		theVal ;			//	Evaluation value
	hzAtom		tmpVal ;			//	Evaluation value
	hzString	preceed ;			//	Preceeding comment
	ceCbody*	pBody ;				//	Code body
	ceStmt		Stmt ;				//	Instruction (for in-function code)
	uint32_t	t_end ;				//	Jump ahead iterator
	uint32_t	loop_stop = 0 ;		//	Loop stop condition
	uint32_t	ct ;				//	Current token
	uint32_t	pt ;				//	Offset into file array P for comments
	hzEcode		rc = E_OK ;			//	Return code

	if (!pFunc)
		Fatal("%s. No functiom supplied\n", __func__) ;

	ct = xend = start ;

	//	expect and deal with function body
	if (X[ct].type != TOK_CURLY_OPEN)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expect to be at opening curly brace\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}
	xend = X[ct].match ;

	//	Grab the first comment inside the body's curly braces. This will (should) be the main comment for the statement block.
	pt = X[ct].comPost ;
	if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
	{
		//	Exclude one-liner code blocks from the need for a comment
		if (!pFunc->StrDesc() && X[xend].line > X[ct].line && !(pFunc->GetAttr() & (FN_ATTR_CONSTRUCTOR | FN_ATTR_DESTRUCTOR | FN_ATTR_OPERATOR)))
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Coding Standards error. Expected a leading comment for the code block\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
	}
	else
	{
		if (!pFunc->StrDesc())
		{
			rc = ProcCommentFunc(pFunc, pt) ;
			if (rc != E_OK)
				return rc ;
		}
		P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
	}

	//	Set up code entities and statements
	pBody = new ceCbody() ;
	if (!pFunc->m_pBody)
		pFunc->m_pBody = pBody ;

	ct++ ;
	for (; rc == E_OK && ct < xend ;)
	{
		//	Bail out of infinite loop
		if (ct == loop_stop)
		{
			g_EC[_hzlevel()].Printf("%s %s line %d col %d: Loop_stop condition at tok (%s)\n", __func__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			rc = E_SYNTAX ;
			break ;
		}
		loop_stop = ct ;

		Z.Clear() ;
		rc = ProcCodeStmt(pFunc, pHost, pFET, t_end, ct, codeLevel) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Call to ProcCodeStmt failed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			break ;
		}
		ct = t_end ;

		if (X[ct].type != TOK_CURLY_CLOSE && X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Warn: Call to ProcCodeStmt did not end with a } or a ; Got %d of %d -> %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, ct, X.Count(), *X[ct].value) ;
			rc = E_SYNTAX ;
			break ;
		}
		ct++ ;

		//	Note that ProcCodeStmt may leave the token iterator on the closing semi-colon but not in all cases. If it does we need the iterator to be
		//	incremented to the start of the next statement or the closing curly brace marking the end of the code block
		if (X[ct].type == TOK_END)
			ct++ ;

		if (X[ct].type == TOK_COMMENT)
			ct++ ;
	}

	if (rc != E_OK)
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed on level %d\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
	return rc ;
}

hzEcode	ceFile::ProcFuncDef	(ceStmt& stmt, ceKlass* pHost, ceTyplex& rtype, const hzString& fnName, EAttr attr, Scope scope, uint32_t& xend, uint32_t start, hzString ctx)
{
	//	Process a function declaration or definition.
	//
	//	This function is called when processing statements from a source file or a union, class/struct or class/struct template definition - anywhere that function declarations or
	//	definitions can legally occur. These can be any of the following:-
	//
	//		1) Declaration anywhere:		[keywords]type funcname (args);
	//		2) Definition anywhere:			[keywords]type funcname (args) {body}
	//		3) Constructor in class def:	classname(args); or classname(args){body}
	//		4) Constructor in ctmpl def:	ctmpl_name(args); or ctmpl_name<ctmpl_args>(args){body}
	//		5) Constructor in file:			[namespace::][parentclass::]classname::classname(args){body}
	//		6) Destructor in class def:		~classname(args); or classname(args){body};
	//		7) Destructor in fie:			[namespace::][parentclass::]classname::~classname(args){body}
	//
	//	This function is called with the current token on the opening curly brace of the argument block. The function return typlex and name are already known at this juncture. The
	//	end token will be set to one place beyond the terminating ; or in the case of a body, the closing curly bracket
	//
	//	Args:	1)	pStmt	The opening statement maintained by the calling function (contains the real start of the statement)
	//			2)	pHost	The class or class template to which the function is a member
	//			3)	rtype	The return type of the function
	//			4)	fnName	function name (not fully qualified)
	//			5)	attr	The accessability (private/protected/public)
	//			6)	scope	The scope
	//			7)	end		Reference to post process token position, set to either the closing ; or the closing curly bracket of the function body.
	//			8)	start	The starting token must be the function name (just before the open round bracket of arg block)
	//			9)	ctx		If a host is supplied this is tru if we are in the host definition body

	_hzfunc("ceFile::ProcFuncDef") ;

	hzList	<ceFunc*>::Iter	fi ;	//	Function iterator (within function group)
	hzList	<ceVar*>::Iter	an ;	//	Function arg iterator for args of new function
	hzList	<ceVar*>::Iter	af ;	//	Function arg iterator for args of existing function

	hzList	<ceVar*>	funcArgs ;	//	The argument of function being processed
	hzList	<hzString>	argdesc ;	//	Argument descriptions
	//hzVect	<hzString>	condExp ;	//	Condition block

	ceEntbl		fet ;				//	Function entity table
	hzChain		Z ;					//	For computing fullname
	hzString	_test ;				//	Temporary type name for lookup
	hzString	_name ;				//	Variable name (not currently used)
	hzString	_fqname ;			//	Variable name (not currently used)
	hzString	est_name ;			//	Estimated full name of function
	hzString	categ ;				//	Category
	hzString	desc ;				//	Description
	hzString	preceed ;			//	Preceeding comment
	ceEntity*	pEnt ;				//	Entity from lookups
	ceVar*		pVar ;				//	Current variable
	ceClass*	pFriendClass = 0 ;	//	Conversion of host if a class
	ceTarg*		pTarg ;				//	Template argument (if applicable)
	ceEntbl*	pET = 0 ;			//	Entity table of host (default's to that of file)
	ceFngrp*	pFgrp = 0 ;			//	New function group
	ceFunc*		pFunc = 0 ;			//	New function
	uint32_t	t_end ;				//	Jump ahead iterator
	uint32_t	ct ;				//	Current token
	uint32_t	n ;					//	Tmplarg iterator
	bool		bSpecial = false ;	//	Special case of operator++/--(int)
	hzEcode		rc = E_OK ;			//	Return code

	if (!pHost)
	{
		//	Non-member function.
		pET = &g_Project.m_RootET ;
	}
	else
	{
		//	Check if host is actually a class/ctmpl as only these can host a function
		if		(pHost->Whatami() == ENTITY_CLASS)	pET = (ceEntbl*) &(pHost->m_ET) ;
		else if (pHost->Whatami() == ENTITY_UNION)	pET = (ceEntbl*) &(pHost->m_ET) ;
		else
			Fatal("%s. Supplied host class is not a class or a class template. It is a %s\n", __func__, Entity2Txt(pHost->Whatami())) ;

		if (attr & FN_ATTR_FRIEND)
		{
			//	The function will be a friend to the host class rather than a member. This means the function goes in the project entity table
			//	and has the class added as a friend. And the class has to add the function as a friend.

			if (pHost->Whatami() == ENTITY_CLASS)
			{
				pET = &g_Project.m_RootET ;
				//pFriendClass = pHostClass ;
				pFriendClass = (ceClass*) pHost ;
				pHost = 0 ;
			}
		}
	}

	ct = xend = start ;

	//if (pHost)
	//	slog.Out("%s (%d) %s line %d col %d: Called with ctx %d and host %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, ctx, *pHost->StrName()) ;
	//else
	//	slog.Out("%s (%d) %s line %d col %d: Called with ctx %d and host null\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, ctx) ;

	//	Process arguments
	Z << fnName ;

	//	Handle the special case of class::operator++/--(int)
	if (pHost && pHost->Whatami() == ENTITY_CLASS && (fnName == "operator++" || fnName == "operator--")
			&& X[ct].type == TOK_ROUND_OPEN && X[ct+1].value == "int" && X[ct+2].type == TOK_ROUND_CLOSE)
		bSpecial = true ;

	rc = ProcFuncArg(funcArgs, Z, pHost, t_end, ct) ;
	if (rc != E_OK)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: case 1 - Aborting after failed ProcFuncArg call\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return rc ;
	}

	est_name = Z ;
	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: ProcFuncArg -> line %d col %d [%s]\n",
		__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[t_end].line, X[t_end].col, *est_name) ;

	stmt.m_nLevel = m_codeLevel ;
	stmt.m_nLine = X[ct].line ;
	stmt.m_Object = est_name ;

	//	Match the function base name to the function group (if it yet exists)
	pEnt = pET->GetEntity(fnName) ;
	if (pEnt && pEnt->Whatami() != ENTITY_FN_OVLD)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Entity %s is already declared as a %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, pEnt->EntDesc()) ;
		return E_SYNTAX ;
	}
	pFgrp = (ceFngrp*) pEnt ;

	if (!pFgrp)
	{
		pFgrp = new ceFngrp() ;
		pFgrp->SetName(fnName) ;
		pFgrp->SetScope(scope) ;
		rc = pET->AddEntity(this, pFgrp, __func__) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: FAILED to add entity func group %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pFunc->Extname()) ;
			return E_SYNTAX ;
		}
	}

	//	Match the function extended name (with args) to the function (if it yet exists)
	pEnt = pET->GetEntity(est_name) ;
	if (pEnt && pEnt->Whatami() != ENTITY_FUNCTION)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Entity %s is already declared as a %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, pEnt->EntDesc()) ;
		return E_SYNTAX ;
	}
	pFunc = (ceFunc*) pEnt ;

	if (!pFunc)
	{
		//	Function is of a name not yet in the entity table
		pFunc = new ceFunc() ;
		pFunc->m_pGrp = pFgrp ;
		pFgrp->m_functions.Add(pFunc) ;
		pFunc->SetAttr(attr) ;
		if (fnName[0] == CHAR_USCORE)
			pFunc->SetAttr(EATTR_INTERNAL) ;

		if (pFunc->InitFuncDcl(this, pHost, rtype, funcArgs, scope, fnName) != E_OK)
			Fatal("%s Could not initialize function %s\n", __func__, __LINE__, *fnName) ;

		for (af = funcArgs ; af.Valid() ; af++)
		{
			pVar = af.Element() ;
			if (pVar->m_atom.IsNull())
				pFunc->m_minArgs++ ;
			if (pVar->Typlex().IsVararg())
				pFunc->m_bVariadic = true ;
		}

		if (pHost && attr & FN_ATTR_OPERATOR)
		{
			pHost->m_Ops.Add(pFunc) ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Function %s added to list of class operators\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pFunc->Extname()) ;
		}

		if (pFriendClass)
		{
			pFunc->m_Friends.Add(pFriendClass) ;
			pFriendClass->m_Friends.Add(pFunc) ;
		}

		//	if (pHostClass)
		//		pHostClass->m_Funcs.Add(pFunc) ;
		if (pHost)
			pHost->m_Funcs.Add(pFunc) ;

		rc = pET->AddEntity(this, pFunc, __func__) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: FAILED to add entity Function %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pFunc->Extname()) ;
			return E_SYNTAX ;
		}
	}

	//	Set the function's group and add the function to the group
	//pFunc->m_pGrp = pFgrp ;
	//pFgrp->m_functions.Add(pFunc) ;

	//	Add template arguments to the function
	for (n = 0 ; n < g_CurTmplArgs.Count() ; n++)
	{
		pTarg = g_CurTmplArgs[n] ;
		pFunc->m_Targs.Add(pTarg) ;
	}

	//	Now resume token processing
	ct = t_end + 1 ;

	if (X[ct].type == TOK_VTYPE_CONST)
	{
		pFunc->SetAttr(FN_ATTR_CONST) ;
		ct++ ;
	}

	if (X[ct].type == TOK_COMMENT)
		ct++ ;

	if (X[ct].type == TOK_OP_EQ && X[ct+1].value == "0" && X[ct+2].type == TOK_END)
	{
		//	Null instance of a virtual function

		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Virtualfunction, null body\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		stmt.m_eSType = STMT_FUNC_DCL ;
		stmt.m_End = xend = ct + 2 ;
		return E_OK ;
	}

	//	Consider case of function prototype only
	if (X[ct].type != TOK_CURLY_OPEN)
	{
		if (pHost && ctx == CTX_PARSE)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Case 1 Expect function body. Class function prototypes cannot exist outside class def body\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		if (X[ct].type == TOK_END)
		{
			//	Just a function declaration
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Function prototype, null body\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			stmt.m_eSType = STMT_FUNC_DCL ;
			stmt.m_End = xend = ct ;
			return E_OK ;
		}

		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Case 2 Expect function body. Class function prototypes cannot exist outside class def body\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}

	//cf_end = xend = X[ct].match ;
	xend = X[ct].match ;

	if (!pFunc->DefFile())
		pFunc->InitFuncDef(this, ct, xend) ;

	stmt.m_eSType = STMT_FUNC_DEF ;
	stmt.m_End = xend ;

	/*
	**	Now expect and deal with function body
	*/

	g_Project.m_Funclist.Insert(pFunc) ;

	/*
	**	Check args and set scope
	*/

	if (ctx != CTX_CLASS)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Prcessing function %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pFunc->StrName()) ;

		s_opRange = pFunc->GetScope() ;
		if (s_opRange == SCOPE_UNKNOWN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Function %s has no scope\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pFunc->StrName()) ;
			s_opRange = SCOPE_GLOBAL ;
		}

		fet.InitET(pFunc) ;

		af = funcArgs ;
		for (an = pFunc->m_Args ; rc == E_OK && an.Valid() ; af++, an++)
		{
			pVar = an.Element() ;

			if (!af.Valid())
				g_EC[_hzlevel()].Printf("%s: Arg %s not matched\n", __func__, *pVar->StrName()) ;
			if (af.Element()->StrName() != pVar->StrName())
			{
				g_EC[_hzlevel()].Printf("%s: Arg %s was called %s\n", __func__, *pVar->StrName(), *af.Element()->StrName()) ;
				pVar->SetName(af.Element()->StrName()) ;
			}

			if (bSpecial)
				pVar->SetParComp(ParComp()) ;
			else
			{
				rc = fet.AddEntity(this, pVar, __func__) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Giving up! (%p)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, pVar) ;
					return E_SYNTAX ;
				}
			}
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Added arg ", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			g_EC[_hzlevel()].Printf("%s as %s\n", *pVar->StrName(), *pVar->Typlex().Str()) ;

			slog.Out("%s (%d) %s line %d col %d: Added arg ", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			slog.Out("%s as %s\n", *pVar->StrName(), *pVar->Typlex().Str()) ;
		}

		Z.Clear() ;
		rc = ProcCodeBody(pFunc, pHost, &fet, t_end, ct, 0) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to process body (but at tok %s) def=%p (%s)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, pFunc->DefFile(), pFunc->DefFname()) ;
			return E_SYNTAX ;
		}
		ct = t_end ;

		if (X[ct].type != TOK_CURLY_CLOSE)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Body processed OK (but at tok %s) def=%p (%s)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, pFunc->DefFile(), pFunc->DefFname()) ;
			return E_SYNTAX ;
		}
	}
	else
	{
		//	Store the function for later processing
		g_FuncDelay.Add(pFunc) ;
	}

	stmt.m_eSType = STMT_FUNC_DEF ;
	stmt.m_End = xend ;

	if (X[ct+1].type == TOK_END)
	{
		g_EC[_hzlevel()].Printf("WARNING: %s line %d col %d: HadronZoo Coding standards forbid the ; after the function body\n", *StrName(), X[ct].line, X[ct].col) ;
		xend++ ;
	}

	return rc ;
}

hzEcode	ceFile::PostProcFunc	(ceFunc* pFunc, ceKlass* pThisClass, uint32_t start)
{
	//	PostProcFunc is called by ceComp::Process to mop up functions that remain undefined.

	_hzfunc("ceFile::PostProcFunc") ;

	hzList	<ceVar*>::Iter	an ;	//	Function arg iterator for args of new function

	hzChain		tmpErr ;			//	Temp chain
	ceEntbl		fet ;				//	Function entity table
	ceVar*		pVar = 0 ;			//	Current variable
	uint32_t	ct ;				//	Current token
	uint32_t	t_end ;				//	Jump ahead iterator (for call to ProcCodeBody only)
	hzEcode		rc = E_OK ;			//	Return code

	ct = pFunc->m_DefTokStart ;
	t_end = pFunc->m_DefTokEnd ;
	g_EC[_hzlevel()].Printf("\n%s Processing class %s function %s starting at line %d col %d and ending on line %d col %d\n",
		__func__, pThisClass ? *pThisClass->StrName() : "No-Class", *pFunc->Fqname(), X[ct].line, X[ct].col, X[t_end].line, X[t_end].col) ;

	for (ct = pFunc->m_DefTokStart ; ct < pFunc->m_DefTokEnd ; ct++)
	{
		if (X[ct].type == TOK_CURLY_OPEN)
			break ;
	}

	if (X[ct].type != TOK_CURLY_OPEN)
		g_EC[_hzlevel()].Printf("%s: Warning. %s line %d col %d Function %s has no code block\n", __func__, *StrName(), X[ct].line, X[ct].col, *pFunc->Fqname()) ;
	else
	{
		//	Only if the function has a body within the class definition body, do we process it here.

		fet.InitET(pFunc) ;

		for (an = pFunc->m_Args ; rc == E_OK && an.Valid() ; an++)
		{
			pVar = an.Element() ;
			if (pVar->StrName())
			{
				rc = fet.AddEntity(this, pVar, __func__) ;
				if (rc != E_OK)
					{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; g_EC[_hzlevel()].Printf("%s: Function %s failed\n", *_fn, *pFunc->Fqname()) ; return E_SYNTAX ; }
			}
		}

		if (!pFunc->m_pBody)
			pFunc->m_pBody = new ceCbody() ;

		tmpErr.Clear() ;
		rc = ProcCodeBody(pFunc, pThisClass, &fet, t_end, ct, 0) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to process body (but at tok %s) def=%p (%s)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, pFunc->DefFile(), pFunc->DefFname()) ;
		}

		if (rc == E_OK && !pFunc->DefFile())
			pFunc->InitFuncDef(this, ct, t_end) ;
	}

	return rc ;
}

/*
**	Namespace functions
*/

hzEcode	ceFile::TestNamespace	(hzVect<ceStmt>& statements, ceKlass* thisKlass, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, hzString ctx, uint32_t codeLevel)
{
	_hzfunc("ceFile::TestNamespace") ;

	ceStmt		Stmt ;			//	Statement populated here
	ceNamsp*	pNamsp ;		//	Namespace identified by 'using' statement
	ceEntbl*	tgt_ET ;		//	Entity table of object
	ceEntity*	pEnt = 0 ;		//	Returned by Lookup in the current scope
	hzString	obj_name ;		//	Name of entity being defined or declared.
	hzString	S ;				//	Temp string
	uint32_t	ct ;			//	Iterator, current token
	uint32_t	t_end ;			//	Set by various interim statement processors
	hzEcode		rc = E_OK ;		//	Return code

	ct = start ;
	if (X[ct].type == TOK_KW_NAMESPACE)
	{
		//	We have a namespace statement of the form namespace namespace_name { code_body }. First step is to create the namespace if it does not yet
		//	exist and set the global current namespace to it. Then just call ProcNamespace to process the namespace body.

		if (X[ct+1].type != TOK_WORD)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR namespace must be followed by a namespace name\n", __func__, __LINE__, *StrName(), X[ct+1].line) ;
			return E_SYNTAX ;
		}
		obj_name = X[ct+1].value ;

		if (X[ct+2].type != TOK_CURLY_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR expected namespace body\n", __func__, __LINE__, *StrName(), X[ct+2].line) ;
			return E_SYNTAX ;
		}
		ultimate = X[ct+2].match ;

		//	Chek if namespace name exists
		pEnt = LookupToken(X, thisKlass, pFET, t_end, ct+1, true, __func__) ;
		if (pEnt)
		{
			if (pEnt->Whatami() != ENTITY_NAMSP)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR %s is already defined as a %s\n", __func__, __LINE__, *StrName(), X[ct+2].line, *obj_name, pEnt->EntDesc()) ;
				return E_SYNTAX ;
			}
			pNamsp = (ceNamsp*) pEnt ;
		}
		else
		{
			//	Create the namespace and add it to the current target
			pNamsp = new ceNamsp() ;
			pNamsp->Init(obj_name, S) ;

			if (tgt_ET->AddEntity(this, pNamsp, obj_name) != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR Failed to add namespace %s\n", __func__, __LINE__, *StrName(), X[ct+1].line, *obj_name) ;
				return E_SYNTAX ;
			}
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		}

		//	Apply namespace
		Stmt.m_eSType = STMT_NAMESPACE ;
		Stmt.m_Object = obj_name ;
		Stmt.m_Start = ct ;
		Stmt.m_End = ultimate ;
		statements.Add(Stmt) ;

		//	Now process namespace body
		rc = ProcNamespace(statements, thisKlass, pNamsp, t_end, ct+2, codeLevel+1) ;
		if (rc != E_OK)
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d: ERROR Failed to process namespace %s\n", __func__, __LINE__, *StrName(), X[ct+1].line, *obj_name) ;
		return rc ;
	}
}

hzEcode	ceFile::ProcNamespace	(hzVect<ceStmt>& statements, ceKlass* parKlass, ceNamsp* pNamsp, uint32_t& ultimate, uint32_t start, uint32_t codeLevel)
{
	//	Process a namespace.
	//
	//	Arguments:	1)	statements	The vector of statements to add to
	//				2)	class		The currently applicable class?
	//				3)	name		Name of namespace
	//				4)	ultimate	The ultimate token (should be closing curly brace of the namespace code body)
	//				5)	start		The starting token (expected to be 'namespace'
	//				6)	codeLevel	Code level indent
	//
	//	Returns:	1) E_SYNTAX	If the processing encounters syntax errors
	//				2) E_OK		If the code body processed OK

	_hzfunc("ceFile::ProcNamespace") ;

	hzChain		Z ;				//	Error reporting
	ceNamsp*	oldNS ;			//	Any namespace currently in force
	hzString	preceed ;		//	Preceeding comment
	uint32_t	loop_stop = 0 ;	//	Loop stopper
	uint32_t	ct ;			//	Iterator, current token
	uint32_t	match ;			//	Iterator, current token
	uint32_t	t_end ;			//	Token (series) end
	hzEcode		rc = E_OK ;		//	Return code

	ct = start ;

	if (X[ct].type != TOK_CURLY_OPEN)
	{
		slog.Out("%s (%d) %s line %d: ERROR expected namespace body\n", __func__, __LINE__, *StrName(), X[ct].line) ;
		return E_SYNTAX ;
	}
	match = X[ct].match ;

	oldNS = g_Project.m_pCurrNamsp ;
	g_Project.m_pCurrNamsp = pNamsp ;
	g_Project.m_Using.Insert(pNamsp) ;

	for (ct++ ; rc == E_OK && ct < match ;)
	{
		if (rc != E_OK)
		{
			slog.Out("%s. %s line %d col %d: rc found to be %s with token=%s\n", *_fn, *StrName(), X[ct].line, X[ct].col, Err2Txt(rc), *X[ct].value) ;
			break ;
		}

		//	Bail out of infinite loop
		if (ct == loop_stop)
		{
			slog.Out("%s. %s line %d col %d: Loop_stop condition at token %d (%s)", *_fn, *StrName(), X[ct].line, X[ct].col, loop_stop, *X[ct].value) ;
			rc = E_SYNTAX ;
			break ;
		}
		loop_stop = ct ;

		//	Create and process the statement
		Z.Clear() ;
		rc = ProcStatement(m_Statements, 0, 0, t_end, ct, CTX_PARSE, codeLevel) ;
		if (rc != E_OK)
		{
			slog.Out(Z) ;
			slog.Out("%s: ProcStatement failed\n", __func__) ;
			break ;
		}
		ct = t_end ;

		if (X[ct].type == TOK_CURLY_CLOSE || X[ct].type == TOK_END)
			ct++ ;
	}

	loop_stop = g_Project.m_Using.Count() ;
	g_Project.m_Using.Delete(pNamsp) ;
	if (g_Project.m_Using.Count() == loop_stop)
		slog.Out("%s: DELETE FAILED\n", __func__) ;
	else
		slog.Out("%s: DELETE OK, now have %d active namespaces\n", __func__, g_Project.m_Using.Count()) ;
	g_Project.m_pCurrNamsp = oldNS ;
	return rc ;
}

/*
**	Section X:	Class and class template definitions
*/

hzEcode	ceFile::ProcClass	(ceStmt& callStmt, ceKlass* parKlass, uint32_t& ultimate, uint32_t start, uint32_t codeLevel, Scope clRange)
{
	//	Processes a class or struct definition.
	//
	//	This function is called whenever either a forward class/struct declaration or a class/struct definition is encountered. It can be called recursively to handle sub-classes.
	//	The current token is expected to be on the word 'class' or 'struct'.
	//
	//	Arguments:	1)	callStmt	The statement this function will populate on behalf of the caller. The class this function will process will have its own set of statements.
	//				2)	parKlass	In the event of a sub-class, the host class/class template. This brings into scope, members of parent class or class template.
	//				3)	ultimate	The end token (set by this function). In all cases this should be the terminating colon.
	//				4)	start		The current token in the current file (the word 'class' or 'struct')
	//				5)	codeLevel	This should be one higher than in the caller
	//				6)	clRange		Default operating range of class 
	//
	//	Returns:	1) E_SYNTAX		Where a syntax error is encountered
	//				2) E_DUPLICATE	Where a class definition is using a name already in use by another entity
	//				3) E_OK			Operation successful.

	_hzfunc("ceFile::ProcClass") ;

	hzList<ceVar>	f_args ;		//	Function arguments
	hzList<hzPair>	argdesc ;		//	Argument descriptions

	ceTyplex	tpx ;				//	For class member variables
	hzChain		tmpErr ;			//	For building names from multiple tokens
	hzChain		Z ;					//	For building names from multiple tokens
	ceStmt		Stmt ;				//	Instruction (for in-function code)
	ceEntity*	pEnt = 0 ;			//	Returned by Lookup in the current scope
	ceClass*	pClass = 0 ;		//	new class when encountered
	ceClass*	pThisClass = 0 ;	//	new class when encountered
	ceClass*	pBase = 0 ;			//	Base class where applicable
	ceEntbl*	tgt_ET ;			//	Entity table in which to place this new class.
	hzString	class_name ;		//	Name of class/struct being defined or declared.
	hzString	member_name ;		//	Name of member of class/struct being defined
	hzString	fqname ;			//	For when we have a host class
	hzString	categ ;				//	Class category
	hzString	desc ;				//	Class description
	hzString	preceed ;			//	Preceeding comment
	uint32_t	ct ;				//	Iterator, current token
	uint32_t	pt ;				//	Offset into file array P for comments
	uint32_t	t_end ;				//	Token (series) end
	uint32_t	cf_end ;			//	Class end
	uint32_t	stm_start ;			//	First token in statement
	uint32_t	loop_stop ;			//	Loop stopper
	bool		bStruct ;			//	Set if class is a struct
	Scope		opRange ;			//	Applied to class members
	hzEcode		rc = E_OK ;			//	Return code

	/*
	**	Check initials
	*/

	if (parKlass)
	{
		if (parKlass->Whatami() == ENTITY_CLASS)
			{ pClass = (ceClass*) parKlass ; tgt_ET = &(pClass->m_ET) ; }
		else
			Fatal("%s. Supplied host class is not a class or a class template. It is a %s\n", *_fn, Entity2Txt(parKlass->Whatami())) ;
	}
	else
	{
		//	Operating outside class/ctmpl definition. 
		tgt_ET = g_Project.m_pCurrNamsp ? &(g_Project.m_pCurrNamsp->m_ET) : &g_Project.m_RootET ;
	}

	ct = stm_start = ultimate = start ;

	if (X[ct].type == TOK_CLASS)
		{ bStruct = false ; opRange = SCOPE_PRIVATE ; }
	else if (X[ct].type == TOK_STRUCT)
		{ bStruct = true ; opRange = SCOPE_PUBLIC ; }
	else
		Fatal("%s. Expected 'class' or 'struct'. %s not expected\n", *_fn, *X[ct].value) ;
	ct++ ;

	/*
	**	Obtain class name
	*/

	if (X[ct].type != TOK_WORD)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: the 'class/struct' must be followed by an alphanumeric class/struct name\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}
	class_name = X[ct].value ;
	callStmt.m_Object = class_name ;
	pEnt = LookupToken(X, parKlass, 0, t_end, ct, false, __func__) ;
	ct++ ;

	//	Reject where 'class' is a supperflous part of a declaration
	if (X[ct].type == TOK_WORD)
		return E_OK ;

	//	Check if class_name is already a defined entity within the current scope and if not create. If there is no host class then the new class is added to the
	//	scope of the file. However if there is a host class or class template then the new class is a subclass of the host class and should only be added to the
	//	scope of host class.

	if (!pEnt)
	{
		//	We don't have an entity in scope that has the same name as the new class

		pThisClass = new ceClass() ;
		pThisClass->InitClass(this, pBase, parKlass, class_name, opRange, bStruct) ;
		pThisClass->SetScope(clRange) ;
	}
	else
	{
		//	We do have an entity in scope that has the same name as the new class. If this is a class then this will be the result of a previously
		//	encountered forward declaration and will be OK to populate with the following code. If it is not a class this is an entity name usage
		//	violation.

		if (pEnt->Whatami() != ENTITY_CLASS)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Cannot use %s as class name as this is already defined as a %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *class_name, pEnt->EntDesc()) ;
			return E_DUPLICATE ;
		}

		pThisClass = (ceClass*) pEnt ;
	}

	//	Is the class a template?
	if (g_CurTmplArgs.Count())
		pThisClass->SetAttr(CL_ATTR_TEMPLATE) ;

	rc = tgt_ET->AddEntity(this, pThisClass, __func__) ;
	if (rc != E_OK)
		{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

	//	Check if this is only a forward declaration (has a premature terminator). If it is we exit here but we must first check if we
	//	have a host class. This is because forward declaration of a class are illegal within a class definition.

	if (X[ct].type == TOK_OP_EQ && X[ct+1].value == "0" && X[ct+2].type == TOK_END)
	{
		ct += 2 ;
		ultimate = ct ;
		callStmt.m_eSType = STMT_CLASS_DCL ;
		callStmt.m_End = ultimate ;
		return E_OK ;
	}

	if (X[ct].type == TOK_END)
	{
		if (parKlass)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Forward decl of class only permitted at the file level\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		ultimate = ct ;
		return E_OK ;
	}

	//	Full class definition. Handle case where class is a derivative
	pBase = 0 ;
	if (X[ct].type == TOK_OP_SEP)
	{
		ct++ ;
		if (X[ct].value != "public")
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected 'public' to follow 'class/struct :' (%s) unexpected\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}
		ct++ ;
		if (X[ct].type != TOK_WORD)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected a base class name: (%s) unexpected\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		pEnt = LookupToken(X, parKlass, 0, t_end, ct, true, __func__) ;
		if (!pEnt)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: %s not defined in this scope\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		if (pEnt->Whatami() != ENTITY_CLASS)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: %s is not defined as a class or struct in this scope\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_SYNTAX ;
		}

		pBase = (ceClass*) pEnt ;
		pThisClass->SetBase(pBase) ;
		ct = t_end ;
	}

	/*
	**	Load the template arguments into the template's own scope
	*/

	ceTarg*	pTarg ;		//	Template argument pointer

	for (uint32_t nVal = 0 ; nVal < g_CurTmplArgs.Count() ; nVal++)
	{
		pTarg = g_CurTmplArgs[nVal] ;
		pTarg->m_Order = nVal ;
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Adding template arg %d of [%s] to class template %s\n",
			*_fn, *StrName(), X[ct].line, X[ct].col, nVal, *pTarg->StrName(), *class_name) ;
		rc = tgt_ET->AddEntity(this, pTarg, __func__) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }
		pThisClass->m_ArgsTmpl.Add(pTarg) ;
	}

	//	Process statements that are allowed within a class definition body. The set of allowed statements is as
	//	follows:-
	//
	//	1)	Sub-class/sub-struct defs	-> class/struct name {body} ;	(will recurse)
	//	2)	Constructor declarations	-> class_name (args) ;
	//	3)	Constructor definitions		-> class_name (args) {} ;
	//	4)	Destructor declarations		-> ~class_name () ;
	//	5)	Destructor definitions		-> ~class_name () {} ;
	//	6)	Casting operator decls		-> operator typlex (args) ;
	//	7)	Casting operator defs		-> operator typlex (args) {} ;
	//	8)	Non casting operator decls	-> [keywords] typlex operator_op (args) ;
	//	9)	Non casting operator defs	-> [keywords] typlex operator_op (args) {} ;
	//	10)	Variable declarations		-> [keywords] typlex var_name [no instances] ;
	//	11)	Function declarations		-> [keywords] typlex func_name (args) [const] [=0] ;
	//	12)	Function definitions		-> [keywords] typlex func_name (args) [const] {body} ;

	if (X[ct].type != TOK_CURLY_OPEN)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Syntax error. Expected a class definition body\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}

	//	Check for class description
	pt = X[ct].comPost ;
	if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
	{
		if (!pThisClass->StrDesc() && !g_Project.m_bSystemMask)
			slog.Out("%s. %s line %d col %d: Coding Standards error. Expected a leading comment for the class\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
	}
	else
	{
		rc = ProcCommentClas(pThisClass, pt) ;
		//pThisClass->SetCateg(categ) ;
		//pThisClass->SetDesc(desc) ;
		P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
	}

	//	Prepare to process
	loop_stop = -1 ;
	cf_end = X[ct].match ;
	ct++ ;

	callStmt.m_eSType = STMT_CLASS_DEF ;
	callStmt.m_End = ultimate ;

	m_codeLevel++ ;

	/*
	**	Process statements
	*/

	for (; rc == E_OK && ct < cf_end ;)
	{
		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: rc found to be %s with token=%s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, Err2Txt(rc), *X[ct].value) ;
			break ;
		}

		//	Bail out of infinite loop
		if (ct == loop_stop)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Loop_stop condition at token %d (%s)",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, loop_stop, *X[ct].value) ;
			rc = E_SYNTAX ;
			break ;
		}
		loop_stop = ct ;

		//	Call ProcExtComment to allow object descriptions that preceed the objects
		if (X[ct].type == TOK_COMMENT)
		{
			rc = ProcExtComment(ct) ;
			if (rc != E_OK)
				break ;

			preceed = X[ct].value ;
			ct++ ;
			continue ;
		}

		//	Create and process the statement
		if (X[ct].value == "public" && X[ct+1].type == TOK_OP_SEP)		{ ct += 2 ; opRange = SCOPE_PUBLIC ; }
		if (X[ct].value == "private" && X[ct+1].type == TOK_OP_SEP)		{ ct += 2 ; opRange = SCOPE_PRIVATE ; }
		if (X[ct].value == "protected" && X[ct+1].type == TOK_OP_SEP)	{ ct += 2 ; opRange = SCOPE_PROTECTED ; }

		tmpErr.Clear() ;
		rc = ProcStructStmt(pThisClass->m_Statements, pThisClass, 0, opRange, t_end, ct, codeLevel) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: PROC STATEMENT failed\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
			return rc ;
		}
		ct = t_end ;

		if (X[ct].type == TOK_CURLY_CLOSE || X[ct].type == TOK_END)
			ct++ ;

		//	For certain statements, e.g variable declarations, comments are expected directly afterwards.
		for (; X[ct].type == TOK_COMMENT ;)
		{
			rc = ProcExtComment(ct) ;
			if (rc != E_OK)
				break ;

			preceed = X[ct].value ;
			ct++ ;
		}
	}

	m_codeLevel-- ;
	if (rc == E_OK)
		ultimate = cf_end + 1 ;

	if (rc == E_OK)
		g_EC[_hzlevel()].Printf("%s (%d) completed class %s\n", __func__, __LINE__, *pThisClass->StrName()) ;
	else
		g_EC[_hzlevel()].Printf("%s (%d) FAILED class %s\n", __func__, __LINE__, *pThisClass->StrName()) ;
	return rc ;
}

hzEcode	ceFile::ProcUnion	(ceStmt& callStmt, ceKlass* pHost, uint32_t& ultimate, uint32_t start, uint32_t codeLevel)
{
	//	Processes a union definition.
	//
	//	This function is called whenever either a forward union declaration or a union definition is encountered. The current token is expected to be on the word 'union'. The end
	//	should fall on the terminating semi-colon. The union token can be part of three possible statement forms as follows:-
	//
	//		1)	An union definition of the form 'union union_name {body} ;
	//		2)	A forward declaration of the form 'union union_name ;'
	//		3)	A variable declaration of in which the word union is supperflous. This has the form 'union union_type varname'
	//
	//

	_hzfunc("ceFile::ProcUnion") ;

	ceTyplex	tpx ;				//	For union member variables
	ceTyplex	p_tpx ;				//	Primary typlex (in statments begining with [keywords] typlex ...
	hzChain		tmpErr ;			//	For reporting GetTyplex() errors
	hzString	nameUnion ;			//	Name of union being defined or declared.
	hzString	nameVar ;			//	Name of union member (variable) being defined or declared.
	ceVar*		pVar ;				//	Union member
	ceNamsp*	pNamsp = 0 ;		//	If host is Class
	ceClass*	pClass = 0 ;		//	If host is Class
	ceEntbl*	pET ;				//	Scope to be applied
	ceEntity*	pEnt = 0 ;			//	Returned by Lookup in the current scope
	ceUnion*	pUnion = 0 ;		//	new union when encountered
	hzString	preceed ;			//	Preceeding comment
	ceStmt		theStmt ;			//	Instruction (for in-function code)
	uint32_t	ct ;				//	Iterator, current token
	uint32_t	pt ;				//	Offset into file array P for comments
	uint32_t	t_end ;				//	Token (series) end
	uint32_t	loop_stop ;			//	Loop stopper
	hzEcode		rc = E_OK ;			//	Return code

	//	Initial checks
	if (!pHost)
		pET = &g_Project.m_RootET ;
	else
	{
		if		(pHost->Whatami() == ENTITY_NAMSP)	{ pNamsp = (ceNamsp*) pHost ; pET = &(pNamsp->m_ET) ; }
		else if (pHost->Whatami() == ENTITY_CLASS)	{ pClass = (ceClass*) pHost ; pET = &(pClass->m_ET) ; }
		else
			Fatal("%s. Invalid host type %s\n", *_fn, Entity2Txt(pHost->Whatami())) ;
	}

	ct = ultimate = start ;
	theStmt.m_Start = ct ;
	theStmt.m_nLine = X[ct].line ;

	if (X[ct].type != TOK_UNION)
		Fatal("ProcUnion - Token not union\n") ;

	//	Obtain union name
	ct++ ;
	if (X[ct].type != TOK_WORD)
	{
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: the 'union' must be followed by an alphanumeric name\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
		return E_FORMAT ;
	}
	nameUnion = X[ct].value ;
	theStmt.m_nLevel = m_codeLevel ;
	theStmt.m_Object = nameUnion ;

	//	Check is union_name is already a defined entity within the current scope
	pEnt = LookupToken(X, pHost, 0, t_end, ct, false, __func__) ;
	if (!pEnt)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Creating new union %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *nameUnion) ;
		pUnion = new ceUnion() ;
		pUnion->InitUnion(this, pHost, nameUnion) ;
		rc = pET->AddEntity(this, pUnion, __func__) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }
	}
	else
	{
		if (pEnt->Whatami() == ENTITY_ENUM)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Enum to union %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *nameUnion) ;
			pUnion = (ceUnion*) pEnt ;
		}
		else
		{
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Cannot use %s as union name as this is already defined as a %s\n",
				*_fn, *StrName(), X[ct].line, X[ct].col, *nameUnion, pEnt->EntDesc()) ;
			return E_DUPLICATE ;
		}
	}
	ct++ ;

	//	Reject where 'union' is supperflous
	if (X[ct].type == TOK_WORD)
	{
		ultimate = start ;
		return E_OK ;
	}

	//	If this is a forward declaration only just add the unitialized ceClass to the current scope
	if (X[ct].type == TOK_END)
	{
		if (pET->GetApplied())
		{
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Forward decl of union only permitted at the file level\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}

		theStmt.m_eSType = STMT_UNION_DCL ;
		theStmt.m_End = ct ;
		//m_statements.Add(theStmt) ;
		ultimate = ct ;
		return E_OK ;
	}

	//	Full union definition. Handle case where class is a derivative
	g_EC[_hzlevel()].Printf("PROCESSING definition of union %s (tgt scope %s)\n", *nameUnion, *pET->StrName()) ;

	if (X[ct].type != TOK_CURLY_OPEN)
	{
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Syntax error. Expected a class definition body\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
		return E_SYNTAX ;
	}

	//	Need a check here if we have not already defined the union!

	//	Obtain limit of union definition (position of the closing curly brace)
	theStmt.m_eSType = STMT_UNION_DEF ;
	ultimate = X[ct].match ;

	//	Expect opening comment
	pt = X[ct].comPost ;
	if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
	{
		if (!g_Project.m_bSystemMask)
			slog.Out("%s. %s line %d col %d: Coding Standards error. Expected a leading comment for the union\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
	}
	else
	{
		pUnion->SetDesc(P[pt].value) ;
		P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
	}

	/*
 	**	Process statements
	*/

	for (ct++ ; ct < ultimate ; ct++)
	{
		if (rc != E_OK)
		{
			g_EC[_hzlevel()].Printf("%s (%d): %s line %d col %d: error=%s with token=%s\n", *_fn, __LINE__, *StrName(), X[ct].line, X[ct].col, Err2Txt(rc), *X[ct].value) ;
			break ;
		}

		if (ct == loop_stop)
		{
			//	Bail out of infinite loop
			g_EC[_hzlevel()].Printf("%s (%d): %s line %d col %d: Loop_stop at token %d (%s)", *_fn, __LINE__, *StrName(), X[ct].line, X[ct].col, loop_stop, *X[ct].value) ;
			rc = E_SYNTAX ;
			break ;
		}
		loop_stop = ct ;
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Considering token %s\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

		//	Test if the statement is the union constructor. (If the token value is same as union name)
		//if (X[ct].type == TOK_WORD && X[ct].value == nameUnion)
		if (X[ct].value == nameUnion)
		{
			//	Statement is union constructor
			if (X[ct+1].type == TOK_ROUND_OPEN)
			{
				ct++ ;
				g_EC[_hzlevel()].Printf("%s (%d) Adding a constructor to union %s\n", __func__, __LINE__, *pUnion->StrName()) ;

				p_tpx.Clear() ;
				p_tpx.SetType(pUnion) ;
				p_tpx.m_Indir = 1 ;

				tmpErr.Clear() ;
				rc = ProcFuncDef(theStmt, pUnion, p_tpx, pUnion->StrName(), FN_ATTR_CONSTRUCTOR, SCOPE_PUBLIC, t_end, ct, CTX_CLASS) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Failed to add a constructor to union %s\n",
						*_fn, __LINE__, *StrName(), X[ct].line, X[ct].col, *pUnion->StrName()) ;
					return rc ;
				}
				ct = t_end ;
				//statements.Add(theStmt) ;
				continue ;
			}
		}

		//	Assume statement is a union member
		tmpErr.Clear() ;
		rc = GetTyplex(tpx, pHost, 0, t_end, ct) ;
		if (rc != E_OK)
		{
			slog << tmpErr ;
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Cannot establish type for union member\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		ct = t_end ;

		if (X[ct].type != TOK_WORD)
		{
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Cannot establish name for union member\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
			return E_SYNTAX ;
		}
		nameVar = X[ct].value ;
		ct++ ;

		if (X[ct].type == TOK_SQ_OPEN)
			ct = X[ct].match + 1 ;

		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Expected colon terminator after decl of union member %s\n",
				*_fn, *StrName(), X[ct].line, X[ct].col, *nameVar) ;
			return E_SYNTAX ;
		}

		pVar = new ceVar() ;
		rc = pVar->InitDeclare(this, pUnion, tpx, SCOPE_PUBLIC, nameVar) ;
		if (rc != E_OK)
			break ;

		rc = pUnion->m_ET.AddEntity(this, pVar, __func__) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; break ; }

		pt = X[ct].comPost ;
		if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
		{
			if (!g_Project.m_bSystemMask)
				g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Coding Standards error. Expected a comment for the union member\n",
					*_fn, *StrName(), X[ct].line, X[ct].col) ;
		}
		else
		{
			pVar->SetDesc(P[X[ct].comPost].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		}
	}

	ultimate++ ;
	theStmt.m_End = ultimate ;
	return rc ;
}

hzEcode	ceFile::Evalnum		(ceKlass* pHost, uint32_t& atom, uint32_t& ultimate, uint32_t start)
{
	//	Evaluate the current token to a number.
	//
	//	This function treats expressions as either comprising a single term or having the general form of 'term operator expression'. The allowed token types within the expression
	//	are numbers, numeric operators, and parenthesis.

	_hzfunc("ceFile::Evalnum") ;

	hzChain		tmpErr ;		//	Aux error chain
	ceEntity*	pE ;			//	Entity from lookup
	ceEnumval*	pEV ;			//	Applicable where entity is an enum value
	uint32_t	atomA ;			//	1st term
	uint32_t	atomB ;			//	Evaluated expression
	uint32_t	ct ;			//	Iterator, current token
	uint32_t	t_end ;			//	Iterator as set by recursion to this function
	uint32_t	nVal ;			//	Enum value
	bool		bMinus ;		//	Set if value is negative
	bool		bInvert ;		//	Set if value is inverted
	hzEcode		rc ;			//	Return code

	ct = start ;

	/*
	**	Derive value for 1st term
	*/

	if (X[ct].type == TOK_ROUND_OPEN)
	{
		tmpErr.Clear() ;
		rc = Evalnum(pHost, atomA, t_end, ct + 1) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s. Could not eval parenthesis. (err=%s)\n", *_fn, Err2Txt(rc)) ;
			return rc ;
		}
		ct = t_end ;
	}

	//	Handle unary operators such as minus and one's compliment
	bMinus = false ;
	if (X[ct].type == TOK_OP_MINUS)
		{ bMinus = true ; ct++ ; }
	bInvert = false ;
	if (X[ct].type == TOK_OP_INVERT)
		{ bInvert = true ; ct++ ; }

	//	Get number part
	if (X[ct].type != TOK_NUMBER)
	{
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Enum Value Token %s is not numeric!\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

		//	If token is not a number then see if it equates to a number (eg an enum value)
		if (X[ct].type == TOK_WORD)
		{
			//pE = _lookup(pNamsp, pHost, 0, X[ct].value) ;
			pE = LookupToken(X, pHost, 0, t_end, ct, false, __func__) ;

			if (!pE)
				{ g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Expected an numeric value after the '='\n", *_fn, *StrName(), X[ct].line, X[ct].col) ; return E_FORMAT ; }

			if (pE->Whatami() != ENTITY_ENVAL)
				{ g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Entity %s is not an enum value\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ; return E_FORMAT ; }

			pEV = (ceEnumval*) pE ;
			nVal = pEV->m_numVal ;
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Accepted numeric value of %d after the '='\n", *_fn, *StrName(), X[ct].line, X[ct].col, nVal) ;
		}
	}
	else
	{
		if (X[ct].type == TOK_HEXNUM)
		{
			if (!IsHexnum(nVal, *X[ct].value))
				g_EC[_hzlevel()].Printf("%s. %s line %d col %d: not a hex so testing for int\n", *_fn, *X[ct].value, X[ct].line, X[ct].col) ;
		}
		else
		{
			if (!IsPosint(nVal, *X[ct].value))
			{
				//	We presumably have an expression to evaluate to a number
				g_EC[_hzlevel()].Printf("%s. %s line %d col %d: The word after the '=' must be numeric\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
				return E_FORMAT ;
			}
		}
	}

	//	Apply unaries to number
	ct++ ;
	if (bMinus)
		nVal *= -1 ;
	if (bInvert)
		atomA = ~nVal ;
	else
		atomA = nVal ;

	/*
	**	We should now have either an operator followed by a legal expression or a terminating 
	*/

	if (X[ct].type == TOK_ROUND_CLOSE || X[ct].type == TOK_SQ_CLOSE || X[ct].type == TOK_SEP || X[ct].type == TOK_END || X[ct].type == TOK_COMMENT)
	{
		ultimate = ct ;
		return rc ;
	}

	tmpErr.Clear() ;
	if (X[ct + 1].type == TOK_ROUND_OPEN)
	{
		rc = Evalnum(pHost, atomB, t_end, ct + 2) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }
		ultimate = t_end ;
	}
	else
	{
		rc = Evalnum(pHost, atomB, t_end, ct + 1) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }
		ultimate = t_end ;
	}

	atom = atomA ;
	if		(X[ct].type == TOK_OP_PLUS)			atom += atomB ;
	else if (X[ct].type == TOK_OP_MINUS)		atom -= atomB ;
	else if (X[ct].type == TOK_OP_MULT)			atom *= atomB ;
	else if (X[ct].type == TOK_OP_DIV)			atom /= atomB ;
	else if (X[ct].type == TOK_OP_REM)			atom %= atomB ;
	else if (X[ct].type == TOK_OP_LSHIFT)		atom <<= atomB ;
	else if (X[ct].type == TOK_OP_RSHIFT)		atom >>= atomB ;
	else if (X[ct].type == TOK_OP_LOGIC_EXOR)	atom ^= atomB ;
	else if (X[ct].type == TOK_OP_LOGIC_AND)	atom &= atomB ;
	else if (X[ct].type == TOK_OP_LOGIC_OR)		atom |= atomB ;
	else
		{ g_EC[_hzlevel()].Printf("%s. %s line %d col %d: No such operation allowed\n", *_fn, *StrName(), X[ct].line, X[ct].col) ; return E_FORMAT ; }

	return E_OK ;
} 

hzEcode	ceFile::ProcEnum	(ceStmt& callStmt, ceKlass* pHost, uint32_t& ultimate, uint32_t start)
{
	//	Process an enum definition.
	//
	//	The word 'enum' preceeds three possible statement forms as follows:-
	//
	//		1)	An enum definition of the form 'enum enuName() {body} ;
	//		2)	A forward declaration of the form 'enum enum_name ;'
	//		3)	A typlex in which the word enum is supperflous. This has the form 'enum enum_name word'
	//
	//	Only in the first two cases an enum entity must be inserted into the entity table of the current scope. This function expects the current token to be on the word 'enum' and
	//	will set the current token to the terminating semi-colon.

	_hzfunc("ceFile::ProcEnum") ;

	hzChain		tmpErr ;			//	Aux error chain
	ceEntbl*	pS ;				//	Scope of host or this file
	ceClass*	pClass = 0 ;		//	Entity is Class
	ceEntity*	pEnt = 0 ;			//	Returned by Lookup in the current scope
	ceEnum*		pEnum = 0 ;			//	new enum when encountered
	ceEnumval*	pEV = 0 ;			//	Enum value
	hzString	name ;				//	Name of entity being defined or declared.
	hzString	categ ;				//	Category
	hzString	desc ;				//	Description
	ceStmt		theStmt ;			//	Instruction (for in-function code)
	uint32_t	nVal ;				//	Enum value
	uint32_t	ct ;				//	Iterator, current token
	uint32_t	pt ;				//	Iterator, original token in array P
	uint32_t	seq ;				//	Used to give emun values ascending value
	uint32_t	t_end ;				//	Token (series) end
	bool		bEnd = false ;		//	Set if an enum value is not followed by a comma
	hzEcode		rc = E_OK ;			//	Return code

	if (!pHost)
		pS = &g_Project.m_RootET ;
	else
	{
		if (pHost->Whatami() == ENTITY_CLASS)
			{ pClass = (ceClass*) pHost ; pS = &(pClass->m_ET) ; }
	}

	ct = ultimate = start ;

	if (X[ct].type != TOK_ENUM)
		Fatal("ProcEnum - Token not enum\n") ;

	//	Obtain enum_name
	theStmt.m_Start = ct ;
	theStmt.m_nLine = X[ct].line ;
	ct++ ;

	if (X[ct].type != TOK_WORD)
	{
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: the 'enum' must be followed by an alphanumeric name\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
		return E_FORMAT ;
	}
	name = X[ct].value ;
	theStmt.m_nLevel = m_codeLevel ;
	theStmt.m_Object = name ;

	//	Check is enum_name is already a defined entity within the current scope
	pEnt = LookupToken(X, pHost, 0, t_end, ct, false, __func__) ;
	if (!pEnt)
		pEnum = new ceEnum() ;
	else
	{
		if (pEnt->Whatami() == ENTITY_ENUM)
			pEnum = (ceEnum*) pEnt ;
		else
		{
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Cannot use %s as enum name as this is already defined as a %s\n",
				*_fn, *StrName(), X[ct].line, X[ct].col, *name, pEnt->EntDesc()) ;
			return E_DUPLICATE ;
		}
	}
	ct++ ;

	//	Reject where 'enum' is supperflous
	if (X[ct].type == TOK_WORD)
		return E_OK ;

	//	If a forward decl, return
	if (X[ct].type == TOK_END)
	{
		theStmt.m_eSType = STMT_ENUM_DCL ;
		theStmt.m_End = ct ;
		m_Statements.Add(theStmt) ;
		ultimate = ct ;
		return E_OK ;
	}

	//	Not a foward declaration - assume we are into a legal enum definition - check for body open
	if (X[ct].type != TOK_CURLY_OPEN)
	{
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Expected the opening of an enum definition body\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
		return E_FORMAT ;
	}

	//	Check for leading comment
	pt = X[ct].comPost ;
	if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
	{
		if (!g_Project.m_bSystemMask)
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Coding Standards error. Expected a leading comment for enum %s\n", *_fn, *StrName(), X[ct].line, X[ct].col, *name) ;
	}
	else
	{
		rc = ProcCommentEnum(pEnum, pt) ;	//categ, desc, P[pt].value) ;
		pEnum->SetCateg(categ) ;
		pEnum->SetDesc(desc) ;
		P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
	}

	ultimate = X[ct].match ;
	ct++ ;

	theStmt.m_eSType = STMT_ENUM_DEF ;
	theStmt.m_End = ct ;
	//m_statements.Add(theStmt) ;
	pEnum->InitEnum(this, name, ultimate, start) ;


	//	Process enum values
	for (seq = 0 ; ct < ultimate ; seq++)
	{
		//	Comments before the enum word are possible, we ignore these
		//	Can put something here to catch category of enum entries
		if (X[ct].type == TOK_COMMENT)
			ct++ ;

		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: At value [%s]\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

		//	Expect a WORD which will be the enum value
		if (X[ct].type != TOK_WORD)
		{
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Expected an enum value. Instead found a token %s\n",
				*_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_FORMAT ;
		}

		pEV = new ceEnumval() ;
		pEV->InitEnval(pEnum, X[ct].value) ;
		pEV->m_numVal = seq ;
		ct++ ;
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: At value [%s]\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

		//	Handle case where enum values are assigned
		if (X[ct].IsOperator())
		{
			if (X[ct].type != TOK_OP_EQ)
			{
				g_EC[_hzlevel()].Printf("%s. %s line %d col %d: The only acceptable operator for an enum value is '='\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
				return E_FORMAT ;
			}
			ct++ ;
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: At value [%s]\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;

			//	Now expect a number or an expression that evaluates to a number
			if (X[ct].type == TOK_HEXNUM)
			{
				if (!IsHexnum(nVal, *X[ct].value + 2))
				{
					g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Token %s not a hex number\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
					return E_FORMAT ;
				}
				ct++ ;
				g_EC[_hzlevel()].Printf("%s. %s line %d col %d: At value [%s]\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			}
			else if (X[ct].type == TOK_NUMBER)
			{
				if (!IsPosint(nVal, *X[ct].value))
				{
					g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Token %s not a decimal number\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
					return E_FORMAT ;
				}
				ct++ ;
				g_EC[_hzlevel()].Printf("%s. %s line %d col %d: At value [%s]\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			}
			else
			{
				//	Evaluate expression (must be to a number)
				//rc = AssessExpr(statements, theTpx, theVal, thisKlass, pFET, t_end, ct, match + 1, 0) ;
				tmpErr.Clear() ;
				rc = Evalnum(pHost, nVal, t_end, ct) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Failed to determine numeric value of enum entry\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
					return rc ;
				}
				pEV->m_txtVal = X[ct].value ;
				g_EC[_hzlevel()].Printf("%s. %s line %d col %d: At value [%s]\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
				ct = t_end ;
				g_EC[_hzlevel()].Printf("%s. %s line %d col %d: At value [%s]\n", *_fn, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			}

			pEV->m_numVal = nVal ;
		}

		//	Move on from the comma
		if (X[ct].type == TOK_SEP)
			{ pt = X[ct].comPost ; ct++ ; }
		else
			{ pt = X[ct-1].comPost ; bEnd = true ; }

		//	Grab the line comment for the value and then mark comment as used
		if (pt == 0xffffffff || P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
		{
			if (!g_Project.m_bSystemMask)
				slog.Out("%s. %s line %d col %d: Coding Standards error. Expected comment for enum value\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
		}
		else
		{
			pEV->SetDesc(P[pt].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		}

		//	If last one
		if (bEnd && X[ct].type != TOK_CURLY_CLOSE)
		{
			g_EC[_hzlevel()].Printf("%s. %s line %d col %d: Missing ',' after enum value %s\n", *_fn, *StrName(), X[ct].line, X[ct].col, *pEV->StrName()) ;
			return E_FORMAT ;
		}

		if (pEV->m_numVal == -1)
			pEV->m_numVal = pEnum->m_ValuesByLex.Count() ;

		pEnum->m_ValuesByLex.Insert(pEV->StrName(), pEV) ;
		pEnum->m_ValuesByNum.Insert(pEV->m_numVal, pEV) ;
		rc = pS->AddEntity(this, pEV, __func__) ;
		if (rc != E_OK)
			{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }
	}

	if (ct != ultimate)
	{
		g_EC[_hzlevel()].Printf("%s. %s line %d col %d: the enum definition did not meet expected expanse\n", *_fn, *StrName(), X[ct].line, X[ct].col) ;
		return E_FORMAT ;
	}

	/*
	**	Initialize the enum
	*/

	pEnum->InitEnum(this, name, ultimate, start) ;
	rc = pS->AddEntity(this, pEnum, __func__) ;
	if (rc != E_OK)
		{ g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ; return rc ; }

	ultimate++ ;
	return rc ;
}

hzEcode	ceFile::Parse	(ceFile* pRoot, uint32_t nRecurse)
{
	//	Process statements at the file level, ie outside either a class, class template or function definition body. The allowed statement forms are as follows:-
	//
	//		Typdef statements:				Of the form -> typedef new_type typlex ;
	//		Class/struct definitions:		Of the form -> class/struct name {body} ;
	//		Class/struct forward decls:		Of the form -> class/struct name ;
	//		Union definitions: 				Of the form -> Union name {body} ;
	//		Union forward decls:			Of the form -> Union name ;
	//		Enum definitions:				Of the form -> Enum name {body} ;
	//		Enum forward decls:				Of the form -> Enum name ;
	//
	//		Class Templates:				Of the form <template class X> class classname {} ;
	//		Function Templates:				Of the fomr <template class X> type funcname () {}
	//		External Class constructors:	Of the form classname::classname () {}
	//		External Class destructors:		Of the form classname::~classname () {}
	//		External Class operators:		Of the form ..
	//
	//		Function ptr declare:			Of the form [keywords] typlex (*var_name)(type_args) ;
	//		Non-cast operator dcl:			Of the form [keywords] typlex operator op(type_args) ;
	//		Variable declarations:			Of the form [keywords] typlex var_name ;
	//		Variable declarations:			Of the form [keywords] typlex var_name[noElements] ;
	//		Variable declarations:			Of the form [keywords] class/typlex var_name(value_args) ;
	//		Function declarations:			Of the form [keywords] typlex func_name (type_args) [const] ;
	//		Function definitions:			Of the form	[keywords] typlex [class::]func_name (type_args) [const] {body} ;

	_hzfunc("ceFile::Parse") ;

	static hzString	stmtFunc = "Parse" ;

	//hzChain		err ;				//	Error reporting
	ceFile*		pIncFile = 0 ;		//	For recursive includes
	hzString	preceed ;			//	Preceeding comment
	uint32_t	loop_stop ;			//	Loop stopper
	uint32_t	ct ;				//	Iterator, current token
	uint32_t	pt ;				//	Iterator, original token in array P
	uint32_t	t_end ;				//	Set by various interim processors such as establishment of type(lex)
	uint32_t	line ;				//	Line number of token (useful for directives processing)
	uint32_t	nStat ;				//	Line number of token (useful for directives processing)
	hzEcode		rc = E_OK ;			//	Return code

	/*
	**	Check args and set scope
	*/

	g_EC[_hzlevel()].Clear() ;

	if (m_bStage2)
		{ slog.Out("Parse %s - already done\n", *StrName()) ; return E_OK ; }
	m_bStage2 = true ;

	if (!P.Count())
		{ slog.Out("%s: Cannot process file %s. No tokens\n", __func__, *m_Name) ; return E_OK ; }

	slog.Out("Parse file (%s, tokens X %u P %u)\n", *StrName(), X.Count(), P.Count()) ;
	slog.Out("{\n") ;

	if (nRecurse >= 10)
		Fatal("%s. Recursion limit reached\n", __func__) ;

	if (pRoot)
		m_pRoot = pRoot ;
	else
		m_pRoot = this ;

	slog.SetIndent(nRecurse+1) ;

	/*
	**	Parse any directly inclued files first
	*/

	for (line = 0 ; line < directInc.Count() ; line++)
	{
		pIncFile = directInc[line] ;
		rc = pIncFile->Parse(m_pRoot, nRecurse+1) ;
		if (rc != E_OK)
		{
			slog.Out("%s: FAILED in file %s, a direct include of %s\n", *_fn, *pIncFile->StrName(), *StrName()) ;
			goto parse_done ;
		}
	}

	/*
	**	Assess comments upto the first token
	*/

	if (X.Count())
		t_end = X[0].orig ;
	else
		t_end = P.Count() ;

	for (pt = 0 ; pt < t_end ; pt++)
	{
		//	Ignore previously processed comments
		if (P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
			continue ;

		if (!StrDesc())
		{
			SetDesc(P[pt].value) ;
			P[pt].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
			continue ;
		}

		ProcExtComment(pt) ;
	}

	/*
	**	Parse tokens into statements
	*/

	loop_stop = -1 ;
	ct = nStat = 0 ;

	for (; rc == E_OK && ct < X.Count() ;)
	{
		if (rc != E_OK)
		{
			slog.Out("%s. %s line %d col %d: rc found to be %s with token=%s\n", *_fn, *StrName(), X[ct].line, X[ct].col, Err2Txt(rc), *X[ct].value) ;
			break ;
		}

		//	Bail out of infinite loop
		if (ct == loop_stop)
		{
			slog.Out("%s. %s line %d col %d: Loop_stop condition at token %d (%s)", *_fn, *StrName(), X[ct].line, X[ct].col, loop_stop, *X[ct].value) ;
			rc = E_SYNTAX ;
			break ;
		}
		loop_stop = ct ;

		//	Create and process the statement
		nStat++ ;
		rc = ProcStatement(m_Statements, 0, 0, t_end, ct, CTX_PARSE, nRecurse) ;
		if (rc != E_OK)
			{ slog.Out(g_EC[_hzlevel()+1]) ; break ; }
		ct = t_end ;

		if (X[ct].type == TOK_CURLY_CLOSE || X[ct].type == TOK_END)
			ct++ ;
	}

	slog.Out("Processed %d statements\n", nStat) ;


	/*
	**	Assess all other unused level 0 comments
	*/

	if (rc == E_OK)
	{
		for (; pt < P.Count() ; pt++)
		{
			if (P[pt].type != TOK_COMMENT)
				continue ;
			if (P[pt].codeLevel > 0)
				continue ;
			if (P[pt].t_cmtf & TOK_FLAG_COMMENT_PROC)
				continue ;

			ProcExtComment(pt) ;
		}
	}

parse_done:
	if (rc == E_OK)
		slog.Out("%s. Completed file %s:\n", *_fn, *StrName()) ;
	else
		slog.Out("%s. FAILED FILE %s:\n", *_fn, *StrName()) ;

	slog.SetIndent(nRecurse) ;
	slog.Out("}\n") ;
	return rc ;
}

hzEcode	ceComp::Process	(void)
{
	_hzfunc("ceComp::Process") ;

	//hzChain		tmpErr ;		//	Auxillary error chain
	ceFile*		pFile ;
	ceFunc*		pFunc ;
	uint32_t	n ;
	hzEcode		rc = E_OK ;

	if (!m_Name)
		Fatal("%s. Supplied component (type=%s) has no name\n", *_fn, Entity2Txt(Whatami())) ;
	slog.Out("%s. PROCESSING %s named %s\n", *_fn, Entity2Txt(Whatami()), *m_Name) ;

	/*
	**	Activate files (load contents)
	*/

	if (rc == E_OK)
	{
		slog.Out("%s. Activating sys incls\n", *_fn) ;
		for (n = 0 ; rc == E_OK && n < m_SysIncl.Count() ; n++)
			{ pFile = m_SysIncl.GetObj(n) ; rc = pFile->Activate() ; }
	}

	if (rc == E_OK)
	{
		slog.Out("%s. Activating headers\n", *_fn) ;
		for (n = 0 ; rc == E_OK && n < m_Headers.Count() ; n++)
			{ pFile = m_Headers.GetObj(n) ; rc = pFile->Activate() ; }
	}

	if (rc == E_OK)
	{
		slog.Out("%s. Activating sources\n", *_fn) ;
		for (n = 0 ; rc == E_OK && n < m_Sources.Count() ; n++)
			{ pFile = m_Sources.GetObj(n) ; rc = pFile->Activate() ; }
	}

	if (rc == E_OK)
	{
		slog.Out("%s. Activating documents\n", *_fn) ;
		for (n = 0 ; rc == E_OK && n < m_Documents.Count() ; n++)
			{ pFile = m_Documents.GetObj(n) ; rc = pFile->Activate() ; }
	}

	/*
	**	Only process source files as in so doing, the headers will be processed
	*/

	if (rc == E_OK)
		slog.Out("\nPRE-PROCESS FILES:\n") ;

	for (n = 0 ; rc == E_OK && n < m_SysIncl.Count() ; n++)
	{
		pFile = m_SysIncl.GetObj(n) ;

		rc = pFile->Preproc(0) ;
		if (rc != E_OK)
			{ slog.Out("Terminating CodeEnforcer case 1 (err=%s)\n", Err2Txt(rc)) ; break ; }

		slog.Out("%s. Component %s: Pre-processed %s\n", *_fn, *m_Name, *pFile->StrName()) ;
	}

	for (n = 0 ; rc == E_OK && n < m_Sources.Count() ; n++)
	{
		pFile = m_Sources.GetObj(n) ;

		rc = pFile->Preproc(0) ;
		if (rc != E_OK)
			{ slog.Out("Terminating CodeEnforcer case 2 (err=%s)\n", Err2Txt(rc)) ; break ; }

		slog.Out("%s. Component %s: Pre-processed %s\n", *_fn, *m_Name, *pFile->StrName()) ;
	}

	for (n = 0 ; rc == E_OK && n < m_Documents.Count() ; n++)
	{
		pFile = m_Documents.GetObj(n) ;

		rc = pFile->Preproc(0) ;
		if (rc != E_OK)
			{ slog.Out("Terminating CodeEnforcer case 3 (err=%s)\n", Err2Txt(rc)) ; break ; }

		slog.Out("%s. Component %s: Pre-processed %s\n", *_fn, *m_Name, *pFile->StrName()) ;
	}
	slog.Out("%s. Component %s: PRE-PROCESS FILES: Done\n\n", *_fn, *m_Name) ;

	/*
	**	Likewise, only parse source files as in so doing, the headers will be parsed
	*/

	if (rc == E_OK)
		slog.Out("PARSING FILES:\n") ;

	for (n = 0 ; rc == E_OK && n < m_SysIncl.Count() ; n++)
	{
		pFile = m_SysIncl.GetObj(n) ;

		g_Project.m_bSystemMask = true ;
		rc = pFile->Parse(0, 0) ;
		g_Project.m_bSystemMask = false ;

		if (rc != E_OK)
			{ slog.Out("Terminating CodeEnforcer case 4 (err=%s)\n", Err2Txt(rc)) ; break ; }

		slog.Out("%s. Component %s Parsed File %s\n", *_fn, *m_Name, *pFile->StrName()) ;
	}

	for (n = 0 ; rc == E_OK && n < m_Sources.Count() ; n++)
	{
		pFile = m_Sources.GetObj(n) ;

		rc = pFile->Parse(0, 0) ;
		if (rc != E_OK)
			{ slog.Out("Terminating CodeEnforcer case 5 (err=%s)\n", Err2Txt(rc)) ; break ; }

		slog.Out("%s. Component %s Parsed File %s\n", *_fn, *m_Name, *pFile->StrName()) ;
	}

	//	Now process the delayed function bodies
   	for (n = 0 ; rc == E_OK && n < g_FuncDelay.Count() ; n++)
	{
		pFunc = g_FuncDelay[n] ;
		pFile = pFunc->DefFile() ;

		rc = pFile->PostProcFunc(pFunc, pFunc->ParKlass(), pFunc->m_DefTokStart) ;
		if (rc != E_OK)
			{ slog.Out(g_EC[_hzlevel()+1]) ; slog.Out("%s (%d): Call to PostProcFunc failed on function %s\n", __func__, __LINE__, *pFunc->StrName()) ; return rc ; }
	}

	//	Parse documents
	for (n = 0 ; rc == E_OK && n < m_Documents.Count() ; n++)
	{
		pFile = m_Documents.GetObj(n) ;

		rc = pFile->Parse(0, 0) ;
		if (rc != E_OK)
			{ slog.Out("Terminating CodeEnforcer case 6 (err=%s)\n", Err2Txt(rc)) ; break ; }

		slog.Out("%s. Component %s Parsed File %s\n", *_fn, *m_Name, *pFile->StrName()) ;
	}

	if (rc == E_OK)
		slog.Out("%s. Component %s: PARSING FILES: Done\n\n", *_fn, *m_Name) ;
	else
		slog.Out("%s. Component %s: PARSING FILES: Failed\n\n", *_fn, *m_Name) ;

	return rc ;
}

hzEcode	ceComp::PostProc	(void)
{
	//	Rationalize the function groups

	_hzfunc("ceComp::PostProc") ;

	hzList<ceFunc*>::Iter	fi ;	//	Functions iterator

	ceEntity*	pE ;				//	Entity pointer
	ceFnset*	pFs ;				//	Function set
	ceFunc*		pFn ;				//	Function
	hzString	categ ;				//	Category
	uint32_t	nE ;				//	Entity iterator
	bool		bError = false ;	//	Function mismatch

	for (nE = 0 ; nE < g_Project.m_RootET.Count() ; nE++)
	{
		pE = g_Project.m_RootET.GetEntity(nE) ;

		if (pE->ParComp() != this)
			continue ;

		if (pE->Whatami() == ENTITY_FUNCTION)
		{
			pFn = (ceFunc*) pE ;

			/*
			if (!pFn->Category())
				pFn->SetCateg(pFg->Category()) ;

			if (pFn->Category() != pFg->Category())
			{
				slog.Out("%s. Error group %s has category of %s, function %s has %s\n",
					*_fn, *pFg->StrName(), *pFg->Category(), *pFn->m_fullname, *pFn->Category()) ;
				bError = true ;
			}
			*/

			if (pFn->GetAttr() & EATTR_INTERNAL || pFn->GetAttr() & FN_ATTR_FRIEND || pFn->m_pSet)
				continue ;
		}

		if (pE->Whatami() == CE_UNIT_FUNC_GRP)
		{
			pFs = (ceFnset*) pE ;

			for (fi = pFs->m_functions ; fi.Valid() ; fi++)
			{
				pFn = fi.Element() ;

				if (!pFs->Category())
					pFs->SetCateg(pFn->GetCategory()) ;
				else
				{
					if (!pFn->GetCategory())
						pFn->SetCategory(pFs->Category()) ;

					if (pFn->GetCategory() != pFs->Category())
					{
						slog.Out("%s. Error group %s has category of %s, function %s has %s\n",
							*_fn, *pFs->m_Title, *pFs->Category(), *pFn->Fqname(), *pFn->GetCategory()) ;
						bError = true ;
					}
				}
			}

			//	Run round again in case funtions were missed
			for (fi = pFs->m_functions ; fi.Valid() ; fi++)
			{
				pFn = fi.Element() ;

				if (!pFn->GetCategory())
					pFn->SetCategory(pFs->Category()) ;
			}
		}
	}

	return bError ? E_SYNTAX : E_OK ;
}
