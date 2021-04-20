//
//	File:	ceParse.cpp
//
//	Legal Notice: This file is part of the HadronZoo::Autodoc program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	HadronZoo::Autodoc is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
//	Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//	as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//	HadronZoo::Autodoc is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with HadronZoo::Autodoc. If not, see http://www.gnu.org/licenses.
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
#include "hzDirectory.h"
#include "hzDocument.h"
#include "hzProcess.h"

#include "enforcer.h"

using namespace std ;

/*
**	Variables
*/

extern	hzProcess		proc ;			//	Process instance (all HadronZoo apps need this)
extern	hzLogger		slog ;			//	Logfile
extern	ceProject		g_Project ;		//	Project (current)

extern	hzSet<hzString>	g_allStrings ;	//	All strings

extern	bool			g_bDebug ;		//	Extra debugging (-b option)

/*
**	Functions
*/

ceFile*	ceProject::LocateFile	(const hzString& name)
{
	//	Locate a file by name only. This is a support function to Preproc and needed because to #include directives do not generally specify the full path.

	_hzfunc("ceProject::LocateFile") ;

	uint32_t	nLo ;
	uint32_t	nHi ;

	nLo = m_HeadersByName.First(name) ;
	if (nLo >= 0)
	{
		nHi = m_HeadersByName.Last(name) ;
		if (nLo == nHi)
			return m_HeadersByName.GetObj(nLo) ;

		//	Ambiguous - try to resolve?
		return 0 ;
	}

	nLo = m_SourcesByName.First(name) ;
	if (nLo < 0)
		return 0 ;

	nHi = m_SourcesByName.Last(name) ;
	if (nLo == nHi)
		return m_SourcesByName.GetObj(nLo) ;

	return 0 ;
}

hzEcode	ceFile::_incorporate	(ceFile* pFile)
{
	//	Incorporate include file (from #include directive) into the m_Includes member

	_hzfunc("ceFile::_incorporate") ;

	ceFile*		pf ;
	hzString	pf_name ;		//	Test file name
	uint32_t	nF = 0 ;

	if (!pFile)
		Fatal("%s. No include file supplied\n", __func__) ;

	directInc.Add(pFile) ;

	if (m_Includes.Exists(pFile->StrName()))
		slog.Out("%s. NOTE Already have %s in %s\n", *_fn, *pFile->StrName(), *m_Name) ;
	else
		m_Includes.Insert(pFile->StrName(), pFile) ;

	for (nF = 0 ; nF < pFile->m_Includes.Count() ; nF++)
	{
		pf = pFile->m_Includes.GetObj(nF) ;

		if (m_Includes.Exists(pf->StrName()))
			continue ;

		m_Includes.Insert(pf->StrName(), pf) ;
	}

	return E_OK ;
}

static	hzMapS<hzString,ceDefine*>	s_all_defines ;		//	All #defines in the original source code
static	hzMapS<hzString,ceLiteral*>	s_all_literals ;	//	All #define literals in the original source code
static	hzMapS<hzString,ceMacro*>	s_all_macros ;		//	All #define macros in the original source code

ceMacro*	ceFile::TryMacro	(const hzString& name, uint32_t& end, uint32_t start, uint32_t stm_limit)
{
	//	Support function to Preporoc to verify that what appears to have the format of a macro is actually a viable macro - then add it as an entity.

	_hzfunc("ceFile::TryMacro") ;

	hzArray	<ceToken>	macArgs ;	//	Macro call arguments

	ceToken		tok ;				//	Token for copying
	ceMacro*	pMacro = 0 ;		//	Entity as macro
	ceMacro*	pMacx = 0 ;			//	Entity as previously defined macro
	ceDefine*	pDefine = 0 ;		//	Entity as previously defined #define
	uint32_t	ct ;				//	Token iterator
	uint32_t	nE ;				//	Ersatz iterator
	uint32_t	nA ;				//	Macro args iterator
	uint32_t	col ;				//	Artificial columnizer
	bool		bMacro ;			//	Macro test
	hzEcode		rc = E_OK ;			//	Return code

	ct = end = start ;

	if (P[ct].type != TOK_ROUND_OPEN)
	{
		slog.Out("%s (%d) %s Line %d col %d: ERROR: Expected macro arg block\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;
		return 0 ;
	}

	//	Not a macro if no arguments are defined
	if (P[ct+1].type == TOK_ROUND_CLOSE)
		return 0 ;

	g_EC[_hzlevel()].Clear() ;

	//	Preemptively declare macro instance
	pMacro = new ceMacro() ;
	//pMacro->InitMacro(this, name) ;
	bMacro = true ;
	g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Investingting macro args until from %d to %d\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col, ct, stm_limit) ;
	ct++ ;

	for (;;)
	{
		if (P[ct].type != TOK_WORD)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d: Not a macro definition (token is %s)\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].Text()) ;
			bMacro = false ;
			break ;
		}

		pMacro->m_Defargs.Insert(P[ct].value, pMacro->m_Defargs.Count() + 1) ;
		ct++ ;

		if (P[ct].type == TOK_ROUND_CLOSE)
			break ;

		if (P[ct].type != TOK_SEP)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d: Corrupt macro definition (expect coma or closing ')' after arg)\n", __func__, __LINE__, *StrName(), P[ct].line) ;
			bMacro = false ;
		}
		ct++ ;
	}

	g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Have now %d macro args\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col, pMacro->m_Defargs.Count()) ;

	if (!pMacro->m_Defargs.Count())
	{
		bMacro = false ;
		delete pMacro ;
		return 0 ;
	}

	if (bMacro)
	{
		//	Add the ersatz sequence. Note that where the whole macro is placed in matching curly braces, we need a step to remove them
		//	because they later screw up processing in cases of conditional operators.
		if (pMacro->m_Defargs.Count())
			bMacro = false ;

		col = 1 ;
		for (ct++ ; ct <= stm_limit ; ct++)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Case 1\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;

			if (pMacro->m_Defargs.Exists(P[ct].value))
			{
				P[ct].t_argno = pMacro->m_Defargs[P[ct].value] ;
				bMacro = true ;
			}
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Case 2\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;

			//	If the token is an already defined #define, expand out the tokens
			pDefine = s_all_defines[P[ct].value] ;
			if (pDefine)
			{
				for (nE = 0 ; nE < pDefine->m_Ersatz.Count() ; nE++)
				{
					tok = pDefine->m_Ersatz[nE] ;
					tok.col = col++ ;
					macArgs.Add(tok) ;
				}
				continue ;
			}
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Case 3\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;

			//	If the token is an already defined macro, expand out the tokens
			pMacx = s_all_macros[P[ct].value] ;
			if (pMacx)
			{
				for (nE = 0 ; nE < pMacx->m_Ersatz.Count() ; nE++)
				{
					tok = pMacx->m_Ersatz[nE] ;
					tok.col = col++ ;
					macArgs.Add(tok) ;	//pMacx->m_Ersatz[nE]) ;
				}
				ct += pMacx->OrigLen() - 1 ;
				continue ;
			}
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Case 4\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;

			tok = P[ct] ;
			tok.col = col++ ;
			macArgs.Add(tok) ;
			//macArgs.Add(P[ct]) ;
		}

		//	Now copy macArgs to ersazt minus the ()
		if (bMacro)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Case 5\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;
			for (nA = 0 ; nA < macArgs.Count() ; nA++)
			{
				macArgs[nA].orig = 999999 ;
				pMacro->m_Ersatz.Add(macArgs[nA]) ;
			}
		}
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Case 2. No mac args %d\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col, macArgs.Count()) ;
			macArgs.Clear() ;
			g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Case 2\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;
	}

	if (!bMacro)
	{
		//slog.Out("%s (%d) %s Line %d col %d: Macro eliminated\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;
		delete pMacro ;
		return 0 ;
	}
	g_EC[_hzlevel()].Printf("%s (%d) %s Line %d col %d: Macro accepted\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;

	//	Add the macro
	s_all_macros.Insert(name, pMacro) ;
	pMacro->InitMacro(this, name, start, ct) ;
	rc = g_Project.m_RootET.AddEntity(this, pMacro, __func__) ;
	if (rc != E_OK)
	{
		g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		g_EC[_hzlevel()].Printf("Could not add macro of %s\n", *pMacro->StrName()) ;
		slog.Out(g_EC[_hzlevel()]) ;
	}
	else
	{
		g_EC[_hzlevel()].Printf("Added macro of %s with %d args and an eratz of %d tokens [ ", *pMacro->StrName(), pMacro->m_Defargs.Count(), pMacro->m_Ersatz.Count()) ;

		for (nE = 0 ; nE < pMacro->m_Ersatz.Count() ; nE++)
			g_EC[_hzlevel()].Printf("%s ", *pMacro->m_Ersatz[nE].value) ;
		g_EC[_hzlevel()].Printf("]\n") ;
		//slog.Out(g_EC[_hzlevel()]) ;
	}

	ct = stm_limit + 1 ;

	if (P[ct].type == TOK_COMMENT)
	{
		pMacro->SetDesc(P[ct].value) ;
		ct++ ;
	}

	end = ct ;
	return pMacro ;
}

hzEcode	ceFile::Preproc		(uint32_t nRecurse)
{
	//	Pre-process tokens in a file.
	//
	//	This process applies compiler directives to the source or header file content. Once this is done, parsing operates on pure code in which all #defines are fully expanded and
	//	there are no remaining code exclusion clauses. Pre-processing focuses only on what is or is not defined by a #define. Other definitions are not processed at this point.

	_hzfunc("ceFile::Preproc") ;

	hzArray	<ceToken>	macArgs ;	//	Macro call arguments

	hzList	<ceToken>::Iter	mI ;	//	Single macro call argument iterator

	ceToken		token ;				//	Token
	hzChain		Z ;					//	For reporting a series of connected tokens
	hzString	directive ;			//	Value of first token of a compiler directive
	hzString	name ;				//	Name of entity being defined or declared.
	ceFile*		pIncFile = 0 ;		//	For recursive includes
	ceEntity*	pEnt = 0 ;			//	Returned by Lookup in the current scope
	ceLiteral*	pLiteral = 0 ;		//	Entity as macro
	ceMacro*	pMacro = 0 ;		//	Entity as macro
	ceDefine*	pDefine = 0 ;		//	Entity as #define
	uint32_t	loop_stop ;			//	Loop stopper
	uint32_t	ct ;				//	Iterator, current token
	uint32_t	xt ;				//	Iterator, auxillary token
	uint32_t	stm_start ;			//	First token in statement
	uint32_t	stm_limit ;			//	Last token in statement
	uint32_t	t_end ;				//	Token after Lookup returns
	uint32_t	curLine ;			//	Line number of token (useful for directives processing)
	uint32_t	col	;				//	Artifical column number
	uint32_t	nE ;				//	Ersatz iterator
	uint32_t	nA ;				//	Macro args iterator
	uint32_t	cdLevel = 0 ;		//	Compiler directive nesting level
	uint32_t	ma_nest ;			//	Nesting level for macro arg
	hzEcode		rc = E_OK ;			//	Return code
	char		cdCtrl	[24] ;		//	If this is set for current cdLevel then code is excluded.

	/*
	**	Check args and set scope
	*/

	if (m_bStage1)
		return E_OK ;
	m_bStage1 = true ;

	if (nRecurse >= 10)
		Fatal("%s. Recursion limit reached\n", __func__) ;

	slog.SetIndent(nRecurse+1) ;
	slog.Out("%s: PREPROCESSING FILE (level %d, file %s)\n", *_fn, nRecurse, *StrName()) ;

	cdLevel = 0 ;
	memset(cdCtrl, 0, 20) ;

	//	Put tokens into string table
	//	if (g_pStrtbl)
	//	{
	//		for (ct = 0 ; ct < P.Count() ; ct++)
	//		{
	//			P[ct].strNo = g_pStrtbl->Insert(P[ct].value) ;
	//			g_allStrings.Insert(P[ct].value) ;
	//		}
	//	}

	/*
	**	Parse tokens into statements
	*/

	loop_stop = -1 ;
	ct = 0 ;

	for (; rc == E_OK && ct < P.Count() ;)
	{
		//	Bypass comments
		if (P[ct].type == TOK_COMMENT)
			{ ct++ ; continue ; }

		//	if (rc != E_OK)
		//	{
		//		slog.Out("%s (%d) %s line %d: rc found to be %s with token=%s\n", __func__, __LINE__, *StrName(), *P[ct].value, Err2Txt(rc), P[ct].line) ;
		//		break ;
		//	}

		//	Trap looping
		if (ct == loop_stop)
			hzexit(_fn, 0, E_CORRUPT, "Loop_stop condition at line %d token %d (%s)", X[ct].line, loop_stop, *X[ct].value) ;
		loop_stop = ct ;

		//	Deal with compiler directives first as these determine if code is active and if so, how it should be interpreted.
		if (P[ct].IsDirective())
		{
			//	Compiler directives do not always have well defined terminations. The trick is to run to the end of the line
			//	taking account of the \ line extension char when applicable.

			directive = P[ct].value ;
			curLine = P[ct].line ;
			stm_start = ct ;
			Z.Clear() ;
			Z << P[ct].value ;
			for (stm_limit = ct + 1 ; stm_limit < P.Count() && P[stm_limit].line == curLine ; stm_limit++)
			{
				if (P[stm_limit].type == TOK_COMMENT)
					break ;

				if (P[stm_limit].type == TOK_ESCAPE)
				{
					Z.AddByte(CHAR_NL) ;
					curLine++ ;
				}
				else
				{
					Z.AddByte(CHAR_SPACE) ;
					Z << P[stm_limit].value ;
				}
			}
			stm_limit-- ;
			name = Z ;

			/*
			**	Are we in excluded code and does the present directive undo this?
			*/

			if (cdCtrl[cdLevel])
			{
				//	Mark the token as excluded
				P[ct].t_excl = cdLevel+1 ;

				slog.Out("%s line=%d: (excl %d) %s\n", *StrName(), P[ct].line, cdLevel, *name) ;

				if (directive == "#if" || directive == "#ifdef" || directive == "#ifndef")
					{ cdLevel++ ; cdCtrl[cdLevel] = 1 ; ct++ ; continue ; }

				if (directive == "#endif" || directive == "#else")
					{ cdCtrl[cdLevel] = 0 ; cdLevel-- ; ct++ ; continue ; }

				if (directive == "#elseif" || directive == "#elif")
				{
					cdCtrl[cdLevel] = 0 ;
					cdLevel-- ;
					ct++ ;
					if (cdCtrl[cdLevel])
						continue ;
				}
				else
				{
					//	Other directives have no bearing in excluded code
					ct++ ;
					continue ;
				}
			}
			else
			{
				P[ct].t_excl = 0 ;
				//	slog.Out("%s line=%d: (incl %d) %s\n", *StrName(), P[ct].line, cdLevel, *name) ;
			}

			if (P[ct].type == TOK_DIR_INCLUDE)
			{
				//	Handle includes. All that needs to happen here is to check if the include file is one in the arg list, and if so is the m_bParsed
				//	flag set (to indicate it has been parsed). If we need to parse it then call this function on that header file.

				//	We are in a header file so check if we have visited this file. Note we only want included
				//	files enclosed in double quotes. We don't want to parse the system includes enclosed in
				//	angle brackets

				if (P[ct+1].type == TOK_QUOTE)
				{
					name = P[ct+1].value ;
					if (!name)
					{
						slog.Out("%s (%d) %s line %d: #include but no filename\n", __func__, __LINE__, *StrName(), P[ct].line) ;
						rc = E_SYNTAX ;
						goto preproc_done ;
					}

					pIncFile = g_Project.LocateFile(name) ;
					if (!pIncFile)
					{
						slog.Out("%s (%d) %s: No such file as '%s'\n", __func__, __LINE__, *name) ;
						rc = E_SYNTAX ;
						goto preproc_done ;
					}

					rc = pIncFile->Preproc(nRecurse + 1) ;
					if (rc != E_OK)
					{
						slog.Out("%s (%d) %s: Failed to parse %s - aborting\n", __func__, __LINE__, *name) ;
						rc = E_SYNTAX ;
						goto preproc_done ;
					}

					_incorporate(pIncFile) ;
				}

				ct = stm_limit + 1 ;
				continue ;
			}

			if (P[ct].type == TOK_DIR_IFDEF)
			{
				//	This is followed by a single word which will be an entity that is may or may not be defined (it does not matter what as). If
				//	it has been defined (the test passes) the condition is placed in a stack (to be be removed later upon the matching #endif).
				//	Until then the tokens are to be processed as though no condition applied. If the test fails (the entity is not defined) then
				//	all tokens until and including the corresponding #endif are bypassed.

				if (P[ct+1].type != TOK_WORD)
				{
					slog.Out("%s (%d) %s line %d col %d: Directive #ifdef must be followed by a word (next tok is [%s])\n",
						__func__, __LINE__, *StrName(), P[ct].line, P[ct].col, *P[ct+1].value) ;
					rc = E_SYNTAX ;
					continue ;
				}
				name = P[ct+1].value ;

				cdLevel++ ;
				//pEnt = _lookup(0, 0, 0, P[ct+1].value) ;
				pEnt = LookupToken(P, 0, 0, t_end, ct+1, false, __func__) ;
				if (pEnt)
					cdCtrl[cdLevel] = 0 ;
				else
					cdCtrl[cdLevel] = 1 ;
				if (g_bDebug)
				{
					if (pEnt)
						slog.Out("cnd met - %s is defined as %s\n", *name, pEnt->EntDesc()) ;
					else
						slog.Out("not met - %s not defined\n", *name) ;
				}

				ct = stm_limit + 1 ;
				continue ;
			}

			if (P[ct].type == TOK_DIR_IFNDEF)
			{
				//	Exactly as for #ifdef but with the logic reversed.

				if (P[ct+1].type != TOK_WORD)
				{
					slog.Out("%s (%d) %s line %d col %d: Directive #ifndef must be followed by a word (next tok is [%s])\n",
						__func__, __LINE__, *StrName(), P[ct].line, P[ct].col, *P[ct+1].value) ;
					rc = E_SYNTAX ;
					continue ;
				}
				name = P[ct+1].value ;

				cdLevel++ ;
				//pEnt = _lookup(0, 0, 0, P[ct+1].value) ;
				pEnt = LookupToken(P, 0, 0, t_end, ct+1, false, __func__) ;
				if (pEnt)
					cdCtrl[cdLevel] = 1 ;
				else
					cdCtrl[cdLevel] = 0 ;
				if (g_bDebug)
				{
					if (pEnt)
						slog.Out("not met - %s is defined as %s\n", *name, pEnt->EntDesc()) ;
					else
						slog.Out("cnd met - %s not defined\n", *name) ;
				}

				ct = stm_limit + 1 ;
				continue ;
			}

			if (P[ct].type == TOK_DIR_ELSE)
			{
				//	Simply reverse the preceeding condition but remain at the same level
				cdCtrl[cdLevel] = cdCtrl[cdLevel] ? 0 : 1 ;
				ct++ ;
				continue ;
			}

			if (P[ct].type == TOK_DIR_ENDIF)
			{
				//	End the current condition and reduce the level
				cdLevel-- ;
				ct++ ;
				continue ;
			}

			/*
			**	Directive is other than #include or #ifdef/#ifndef which must nessesarily be followed by a single entity on the same line. Trick to dealing with #define and other
			**	directives is to run to the end of the line taking account of the \ line extension char when applicable.
			*/

			if (P[ct].type == TOK_DIR_DEFINE)
			{
				//	#define directives can put a name to a literal value, define a macro function or create a shorthand notation in which a single token becomes the replacement for
				//	a sequence of tokens. A #define will have one of the following forms:-
				//
				//		(1) #define identifier:	This only defines the identifier as an entity which exists. The existance can then be tested by an #ifdef directive or similar. This
				//		approach is used to control header files inclusion by the compiler.
				//
				//		(2) #define identifier literal_value: This makes code more readable because names can be used instead of numbers. If we have "#define BLKSIZE 4096", we have
				//		an entity called 'BLKSIZE' with a literal value of 4096. We can then use BLKSIZE instead of 4096 in the code to make clear we are counting out to the end of
				//		a disk block.
				//
				//		(3) #define macro_name(args) macro_code: E.g. "#define max(a,b) a>b?a:b" which will translate the statement "hi = max(hi,curr);" into "hi=hi>curr?hi:curr;".
				//
				//		(4) #define identifier token_sequence: This is general purpose text substitution.
				//
				//	Note that in the above, the identifier and the macro name are always a single token word. These are required to be unique. Note also that (1) and (4) produce an
				//	instance of ceDefine while (2) produces an instance of ceLiteral and (3) an instance of ceMacro.

				ct++ ;
				if (!(P[ct].type == TOK_WORD || P[ct].IsKeyword() || P[ct].IsCommand()))
				{
					slog.Out("%s (%d) %s Line %d: A #define statement must be followed by at least one alphanumeric sequence\n",
						__func__, __LINE__, *StrName(), P[stm_start].line) ;
					rc = E_SYNTAX ;
					goto preproc_done ;
				}
				name = P[ct].value ;

				//	Check this is not already defined.
				pEnt = LookupToken(P, 0, 0, t_end, ct, false, __func__) ;
				if (pEnt)
				{
					if (pEnt->Whatami() != ENTITY_DEFINE)
						slog.Out("%s (%d) %s line %d: We already have %s defined as a %s\n", __func__, __LINE__, *StrName(), P[ct].line, *name, pEnt->EntDesc()) ;
					else
					{
						pDefine = (ceDefine*) pEnt ;
						slog.Out("%s (%d) %s line %d: We already have %s defined in %s\n", __func__, __LINE__, *StrName(), P[ct].line, *name, pDefine->DefFname()) ;
					}
				}

				//	Deal with case where the entity being defined has no ersatz
				if (ct == stm_limit)
				{
					pDefine = new ceDefine() ;
					pDefine->InitHashdef(this, name, stm_start, stm_limit) ;

					s_all_defines.Insert(pDefine->StrName(), pDefine) ;
					Z.Clear() ;
					rc = g_Project.m_RootET.AddEntity(this, pDefine, __func__) ;
					if (rc == E_OK)
						slog.Out("Added #define of %s with no ersatz\n", *pDefine->StrName()) ;
					else
					{
						slog.Out(Z) ;
						slog.Out("%s (%d) ERROR: Could not add #define of %s (no ersatz)\n", __func__, __LINE__, *pDefine->StrName()) ;
					}

					ct = stm_limit + 1 ;

					if (P[ct].type == TOK_COMMENT)
					{
						pDefine->SetDesc(P[ct].value) ;
						ct++ ;
					}
					continue ;
				}
				ct++ ;

				//	Deal with the lieral case

				//	Is the entity being defined a macro?
				if (P[ct].type == TOK_ROUND_OPEN && P[ct+1].type != TOK_ROUND_CLOSE)
				{
					pMacro = TryMacro(name, t_end, ct, stm_limit) ;
					if (pMacro)
						{ ct = t_end ; continue ; }
				}

				//	Not a macro
				if (ct == stm_limit && P[ct].IsLiteral())
				{
					slog.Out("Added LITERAL %s", *name) ;
					//slog.Out("of %s with value of %s\n", *pDefine->StrName(), *P[ct].value) ;
					pLiteral = new ceLiteral() ;
					pLiteral->InitLiteral(this, name, P[ct].value) ;
					s_all_literals.Insert(name, pLiteral) ;

					rc = g_Project.m_RootET.AddEntity(this, pLiteral, __func__) ;
					if (rc == E_OK)
						slog.Out("Added LITERAL of %s with value of %s\n", *pLiteral->StrName(), *pLiteral->m_Str) ;
					else
					{
						slog.Out(Z) ;
						slog.Out("%s (%d) ERROR: Could not add LITERAL of %s with value of %s\n", __func__, __LINE__, *pLiteral->StrName(), *pLiteral->m_Str) ;
					}
					ct = stm_limit + 1 ;

					if (P[ct].type == TOK_COMMENT)
					{
						pLiteral->SetDesc(P[ct].value) ;
						ct++ ;
					}
					continue ;
				}

				pDefine = new ceDefine() ;
				pDefine->InitHashdef(this, name, stm_start, stm_limit) ;

				s_all_defines.Insert(pDefine->StrName(), pDefine) ;
				Z.Clear() ;
				rc = g_Project.m_RootET.AddEntity(this, pDefine, __func__) ;
				if (rc == E_OK)
					slog.Out("Added #define of %s with an eratz of %d tokens\n", *pDefine->StrName(), pDefine->m_Ersatz.Count()) ;
				else
				{
					slog.Out(Z) ;
					slog.Out("%s (%d) ERROR: Could not add #define of %s with an eratz of %d tokens\n", __func__, __LINE__, *pDefine->StrName(), pDefine->m_Ersatz.Count()) ;
				}
				ct = stm_limit + 1 ;

				if (P[ct].type == TOK_COMMENT)
				{
					pDefine->SetDesc(P[ct].value) ;
					ct++ ;
				}
				continue ;
			}

			if (P[ct].type == TOK_DIR_IF)
			{
				if (P[ct+1].type != TOK_WORD)
				{
					slog.Out("%s (%d) File %s Line %d: Directive #if must be followed by an expression\n", __func__, __LINE__, *StrName(), P[ct+1].line) ;
					rc = E_SYNTAX ;
					continue ;
				}

				cdLevel++ ;
				if (P[ct+1].value == "0")
				{
					//	Bypass all tokens until the matching #endif or #else/#elif/#elseif
					cdCtrl[cdLevel] = 1 ;
					continue ;
				}

				cdCtrl[cdLevel] = 0 ;
				ct = stm_limit + 1 ;
				continue ;
			}

			if (P[ct].type == TOK_DIR_ELSEIF)
			{
				if (P[ct+1].type != TOK_WORD)
				{
					slog.Out("%s (%d) %s Line %d: Directive #elseif must be followed by an expression\n", __func__, __LINE__, *StrName(), P[ct+1].line) ;
					rc = E_SYNTAX ;
					continue ;
				}

				ct = stm_limit + 1 ;
				continue ;
			}

			slog.Out("%s (%d) %s, line %d: Unknown compiler directive %s\n", __func__, __LINE__, *StrName(), P[ct].line, *P[ct].value) ;
			ct = stm_limit + 1 ;
			continue ;
		}

		/*
		**	Not in a compiler directive but are we in code excluded by one?
		*/

		if (cdCtrl[cdLevel])
		{
			ct++ ;
			continue ;
		}

		/*
		**	Every thing below here is included code but not a compiler directive. All we do here is test if a token is a
		**	#define entity or not. If it is we expand it out.
		*/

		curLine = P[ct].line ;
		pEnt = LookupToken(P, 0, 0, t_end, ct, false, __func__) ;
		/*
		if (!pEnt)
		{
			if (s_all_macros.Exists(P[ct].value))
				slog.Out("xxx-entity %s %d tok %s\n", *StrName(), P[ct].line, *P[ct].value) ;
			else
				slog.Out("non-entity %s %d tok %s\n", *StrName(), P[ct].line, *P[ct].value) ;

			pEnt = g_Project.m_RootET.GetEntity(P[ct].value) ;
		}
		*/

		if (pEnt)
		{
			if (pEnt->Whatami() == ENTITY_DEFINE)
			{
				pDefine = (ceDefine*) pEnt ;

				//	Do a direct and literal replacement (add nothing if no tail)
				//	Z.Clear() ;
				//	Z.Printf("%s (%d) %s line %d col %d: Encountered #define %s {", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col, *pDefine->StrName()) ;
				for (nE = 0 ; nE < pDefine->m_Ersatz.Count() ; nE++)
				{
					//	Z << pDefine->m_Ersatz[nE].value ;
					token = pDefine->m_Ersatz[nE] ;
					token.line = curLine ;
					token.col = 0 ;
					X.Add(token) ;
				}
				//	Z << "}\n" ;
				//	slog.Out(Z) ;
				ct++ ;
				continue ;
			}

			if (pEnt->Whatami() == ENTITY_MACRO)
			{
				//	Macro call. Take the supplied arguments and generate a series of tokens identical in form to those in the macro ersatz but where
				//	the ersatz tokens indicate an argument number, use the arguments actually supplied.

				pMacro = (ceMacro*) pEnt ;
				Z.Clear() ;
				//Z.Printf("%s (%d) %s line %d col %d: MACRO CALL %s(", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col, *pMacro->StrName()) ;
				for (nA = 0 ; nA < pMacro->m_Defargs.Count() ; nA++)
				{
					if (nA)
						Z.AddByte(CHAR_COMMA) ;
					Z << pMacro->m_Defargs.GetKey(nA) ;
				}
				Z << ") = {" ;
				for (nA = 0 ; nA < pMacro->m_Ersatz.Count() ; nA++)
				{
					if (pMacro->m_Ersatz[nA].t_argno)
						Z.Printf("@%d=", pMacro->m_Ersatz[nA].t_argno) ;
					Z << pMacro->m_Ersatz[nA].value ;
				}
				Z << "}\n" ;
				//slog.Out(Z) ;

				//	gather the args in the exected (...) block
				ct++ ;
				if (P[ct].type != TOK_ROUND_OPEN)
				{
					slog.Out("%s (%d) %s line %d col %d: Expected start of (arg) block\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col) ;
					return E_SYNTAX ;
				}

				macArgs.Clear() ;
				ma_nest = nA = 1 ;

				for (ct++ ;; ct++)
				{
					if (P[ct].type == TOK_ROUND_OPEN)
						ma_nest++ ;
					if (P[ct].type == TOK_ROUND_CLOSE)
						ma_nest-- ;

					if (ma_nest == 0 || P[ct].type == TOK_SEP)
					{
						//	store token sequence of supplied argument
						nA++ ;
						if (!ma_nest)
							break ;
						continue ;
					}

					P[ct].t_argno = nA ;
					macArgs.Add(P[ct]) ;
					//	slog.Out("%s (%d) %s line %d col %d: macro call arg %d is [%s]\n", __func__, __LINE__, *StrName(), P[ct].line, P[ct].col, nA, *P[ct].value) ;
				}
				if (!macArgs.Count())
				{
					slog.Out("%s (%d) %s Line %d: Empty macro argument\n", __func__, __LINE__, *StrName(), P[ct].line) ;
					return E_SYNTAX ;
				}

				//	inject tokens into the pre-processed buffer.
				Z.Clear() ;
				//	Z.Printf("%s (%d) %s Line %d: Macro=", __func__, __LINE__, *StrName(), P[ct].line) ;
				col = 0 ;
				for (nE = 0 ; nE < pMacro->m_Ersatz.Count() ; nE++)
				{
					token = pMacro->m_Ersatz[nE] ;
					token.line = curLine ;

					if (token.t_argno == 0)
					{
						Z.Printf("%s", *token.value) ;
						token.col = col ;	//++ ;
						X.Add(token) ;
					}
					else
					{
						Z.Printf("{arg%d=", token.t_argno) ;
						for (xt = 0 ; xt < macArgs.Count() ; xt++)
						{
							if (macArgs[xt].t_argno == token.t_argno)
							{
								token = macArgs[xt] ;
								token.col = col ;	//++ ;
								X.Add(token) ;
								Z.Printf("%s", *token.value) ;
							}
						}
						Z << "}" ;
					}
				}
				Z.AddByte(CHAR_NL) ;
				//	slog.Out(Z) ;
				Z.Clear() ;

				ct++ ;
				macArgs.Clear() ;
				continue ;
			}

			//slog.Out("yes-entity %s %d tok %s\n", *StrName(), P[ct].line, *P[ct].value) ;
		}

		//	Add include token
		X.Add(P[ct]) ;
		ct++ ;
	}

	/*
	**	Diagnostics - Output all tokens to be processed in the file
	*/

preproc_done:
	if (rc != E_OK)
		slog.Out("%s. Failed file %s:\n", *_fn, *StrName()) ;
	else
	{
		rc = MatchTokens(X) ;

		nA = nE = 0 ;

		for (ct = 0 ; ct < X.Count() ; ct++)
		{
			if (X[ct].type == TOK_COMMENT)
				slog.Out("WARNING: Comment found file %s line %d col %d\n", *StrName(), X[ct].line, X[ct].col) ;

			//	Set comPost for active tokens followed by comment
			if (X[ct].col == 0)
				continue ;

			xt = X[ct].orig ;
			if ((xt+1) < P.Count() && P[xt+1].type == TOK_COMMENT)
			{
				X[ct].comPost = xt+1 ;
				P[xt+1].comPost = xt ;
				nE++ ;
			}

			//	Set comPre for active token preceeded by an unassigned comment
			if (xt > 0 && P[xt-1].type == TOK_COMMENT && P[xt-1].comPost == 0xffffffff)
			{
				X[ct].comPre = xt-1 ;
				P[xt-1].comPre = xt ;
				nA++ ;
			}
		}

		slog.Out("%s. Completed file %s: had %d tokens, now have %d tokens (%d pre and %d post comments)\n", *_fn, *StrName(), P.Count(), X.Count(), nA, nE) ;
	}

	slog.SetIndent(nRecurse) ;
	return rc ;
}
