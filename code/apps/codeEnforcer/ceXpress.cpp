//
//	File:	ceXpress.cpp
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
#include "hzCtmpls.h"
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
//extern	ceEntbl			g_RootET ;		//	All system defined entities (eg standard C++ types)
extern	hzVect<ceTarg*>	g_CurTmplArgs ;	//	Current template arguments

extern	bool	g_debug ;				//	Extra debugging (-b option)

/*
**	ceTyplex members
*/

static	int32_t	fn_mon = 0 ;

hzString	ceTyplex::Str	(void) const
{
	//	Print out the typlex in full

	_hzfunc("ceTyplex::Str") ;

	hzList<ceTyplex>::Iter	ti ;	//	Arguments iterator

	ceTyplex	arg ;		//	Argument
	hzChain		Z ;			//	Chain to build output string
	hzString	S ;			//	Output string
	ceClass*	pKlass ;	//	Data type class
	int32_t		n ;			//	Indirection counter

	fn_mon++ ;

	if (Basis() == ADT_NULL)
		{ S = "NULL-BASIS" ; return S ; }

	if (!m_pType)
		{ S = "NULL-TYPE" ; return S ; }

	//	If base type is a class, set this here
	pKlass = dynamic_cast<ceClass*> (m_pType) ;

	//	If the typlex is a const, the const keyword is output first
	if (IsConst())
		Z << "const " ;

	//	Write out the base type
	if (pKlass && pKlass->ParKlass())
	{
		//	Write out class and parent class
		if (pKlass->ParKlass()->StrName())
			Z << pKlass->ParKlass()->StrName() ;
		else
			Z << "no-class" ;
	}
	else
	{
		if (m_pType->StrName())
			Z << m_pType->StrName() ;
		else
			Z << "untitled" ;
	}

	if (m_dt_attrs & DT_ATTR_FNPTR)
	{
		for (n = 0 ; n < m_Indir ; n++)
			Z.AddByte(CHAR_ASTERISK) ;

		Z << "(*)(" ;
		for (ti = m_Args ; ti.Valid() ;)
		{
			arg = ti.Element() ;
			//printf("recurse case 1\n") ;
			Z << arg.Str() ;
			ti++ ;
			if (ti.Valid())
				Z.AddByte(CHAR_COMMA) ;
		}
		Z.AddByte(')') ;
	}
	else
	{
		if (m_Args.Count())
		{
			Z.AddByte(CHAR_LESS) ;
			for (ti = m_Args ; ti.Valid() ;)
			{
				arg = ti.Element() ;
			//printf("recurse case 2\n") ;
				Z << arg.Str() ;
				ti++ ;
				if (ti.Valid())
					Z.AddByte(CHAR_COMMA) ;
			}
			Z.AddByte(CHAR_MORE) ;
		}

		if (pKlass && pKlass->ParKlass())
			Z << "::" << m_pType->StrName() ;

		if (m_dt_attrs & DT_ATTR_REFERENCE)
			Z.AddByte(CHAR_AMPSAND) ;

		for (n = 0 ; n < m_Indir ; n++)
			Z.AddByte(CHAR_ASTERISK) ;
	}

	S = Z ;
	Z.Clear() ;
	return S ;
}

bool	ceTyplex::Same	(const ceTyplex& supp) const
{
	//	Determines if the current ceTyplex is the same type as the supplied.
	//
	//	For this to be the case, both the data type and indirection must match. And where the data type has further qualifying arguments (class templates and function pointers),
	//	these must match also.

	if (m_pType != supp.m_pType || m_Indir != supp.m_Indir || m_Args.Count() != supp.m_Args.Count())
		return false ;

	if ((m_dt_attrs & DT_ATTR_FNPTR) != (supp.m_dt_attrs & DT_ATTR_FNPTR))
		return false ;

	if (!m_Args.Count())
		return true ;

	hzList<ceTyplex>::Iter	ai_this ;	//	Arguments iterator (this)
	hzList<ceTyplex>::Iter	ai_supp ;	//	Arguments iterator (supplied)

	ceTyplex	arg_this ;	//	Argument this
	ceTyplex	arg_supp ;	//	Argument supplied

	ai_this = m_Args ;
	ai_supp = supp.m_Args ;

	for (; ai_this.Valid() && ai_supp.Valid() ;)
	{
		arg_this = ai_this.Element() ;
		arg_supp = ai_supp.Element() ;

		if (!arg_this.Same(arg_supp))
			break ;
		ai_this++ ;
		ai_supp++ ;
	}

	return ai_this.Valid() ? false : true ;
}

ceTyplex&	ceTyplex::operator=	(const ceTyplex& supp)
{
	m_Args = supp.m_Args ;
	m_pType = supp.m_pType ;
	m_nElements = supp.m_nElements ;
	m_Indir = supp.m_Indir ;
	m_dt_attrs = supp.m_dt_attrs ;

	return *this ;
}

const char*	Dta2Txt	(ceBasis eB)
{
	static const hzString	_adtNULL	= "ADT_NULL" ;
	static const hzString	_adtVOID	= "ADT_VOID" ;
	static const hzString	_adtENUM	= "ADT_ENUM" ;
	static const hzString	_adtBOOL	= "ADT_BOOL" ;
	static const hzString	_adtDOUBLE	= "ADT_DOUBLE" ;
	static const hzString	_adtINT64	= "ADT_INT64" ;
	static const hzString	_adtINT32	= "ADT_INT32" ;
	static const hzString	_adtINT16	= "ADT_INT16" ;
	static const hzString	_adtBYTE	= "ADT_BYTE" ;
	static const hzString	_adtUNT64	= "ADT_UNT64" ;
	static const hzString	_adtUNT32	= "ADT_UNT32" ;
	static const hzString	_adtUNT16	= "ADT_UNT16" ;
	static const hzString	_adtUBYTE	= "ADT_UBYTE" ;
	static const hzString	_adtSTRING	= "ADT_STRING" ;
	static const hzString	_adtTEXT	= "ADT_TEXT" ;
	static const hzString	_adtCLASS	= "ADT_CLASS" ;
	static const hzString	_adtVARG	= "ADT_VARG" ;

	switch	(eB)
	{
	case ADT_NULL:		return *_adtNULL ;
	case ADT_VOID:		return *_adtVOID ;
	case ADT_ENUM:		return *_adtENUM ;
	case ADT_BOOL:		return *_adtBOOL ;
	case ADT_DOUBLE:	return *_adtDOUBLE ;
	case ADT_INT64:		return *_adtINT64 ;
	case ADT_INT32:		return *_adtINT32 ;
	case ADT_INT16:		return *_adtINT16 ;
	case ADT_BYTE:		return *_adtBYTE ;
	case ADT_UNT64:		return *_adtUNT64 ;
	case ADT_UNT32:		return *_adtUNT32 ;
	case ADT_UNT16:		return *_adtUNT16 ;
	case ADT_UBYTE:		return *_adtUBYTE ;
	case ADT_STRING:	return *_adtSTRING ;
	case ADT_TEXT:		return *_adtTEXT ;
	case ADT_CLASS:		return *_adtCLASS ;
	case ADT_VARG:		return *_adtVARG ;
	}

	return *_adtNULL ;
}

bool	_test_type_assign	(ceBasis tgt, ceBasis src)
{
	//	Support function to Testset

	if (tgt == src)
		return true ;

	switch	(tgt)
	{
	case ADT_UNT64:		return (src==ADT_UNT32 || src==ADT_UNT16 || src==ADT_UBYTE) ? true : false ;
	case ADT_UNT32:		return (src==ADT_UNT16 || src==ADT_UBYTE) ? true : false ;
	case ADT_UNT16:		return (src==ADT_UBYTE) ? true : false ;
	case ADT_INT64:		return (src==ADT_UNT64 || src==ADT_UNT32 || src==ADT_UNT16 || src==ADT_UBYTE || src==ADT_INT32 || src==ADT_INT16 || src==ADT_BYTE) ? true : false ;
	case ADT_INT32:		return (src==ADT_UNT32 || src==ADT_UNT16 || src==ADT_UBYTE || src==ADT_INT16 || src==ADT_BYTE) ? true : false ;
	case ADT_INT16:		return (src==ADT_UNT16 || src==ADT_UBYTE || src==ADT_BYTE) ? true : false ;
	case ADT_BYTE:		return (src==ADT_UBYTE) ? true : false ;
	}

	return false ;
}

ceFunc*	LocateOperator	(const ceTyplex& tpxA, const ceTyplex& tpxB, const char* opname)
{
	//	Establish if the ceDatatype has an operator overload matching the supplied argument and operation

	_hzfunc(__func__) ;

	hzList<ceFunc*>::Iter	opI ;	//	Operations iterator
	hzList<ceVar*>::Iter	ai ;	//	Arguments iterator

	ceDatatype*	pDT ;				//	Base data types (needed for list of operations - question this)
	ceFunc*		pOper = 0 ;			//	Operation under test
	ceVar*		pVar ;				//	Argument

	g_EC[_hzlevel()].Clear() ;
	g_EC[_hzlevel()].Printf("%s. (%d) Locating operator for typlex A %s and B %s\n", *_fn, __LINE__, *tpxA.Str(), *tpxB.Str()) ;

	pDT = tpxA.Type() ;
	if (!pDT->m_Ops.Count())
		return 0 ;

	for (opI = pDT->m_Ops ; opI.Valid() ; opI++)
	{
		pOper = opI.Element() ;

		//	Ignore casting operator
		if (tpxB.Type() && pOper->m_Args.Count() == 0)
			continue ;

		if (pOper->StrName() != opname)
			continue ;

		pVar = 0 ;
		if (pOper->m_Args.Count())
		{
			ai = pOper->m_Args ;
			pVar = ai.Element() ;
		}

		if (!pVar)
			return pOper ;

		if (pVar->Typlex().Same(tpxB))
			return pOper ;
		
	}
	g_EC[_hzlevel()].Printf("%s: No operator %s found for typlex A %s and B %s\n", __func__, opname, *tpxA.Str(), *tpxB.Str()) ;

	for (opI = pDT->m_Ops ; opI.Valid() ; opI++)
	{
		pOper = opI.Element() ;

		//	Ignore casting operator
		if (tpxB.Type() && pOper->m_Args.Count() == 0)
			continue ;

		//if (pOper->m_Name != opname)
		if (pOper->StrName() != opname)
			continue ;

		pVar = 0 ;
		if (pOper->m_Args.Count())
		{
			ai = pOper->m_Args ;
			pVar = ai.Element() ;
		}

		if (pVar)
			g_EC[_hzlevel()].Printf("%s: using %s found for datatype %s with arg of %s\n", __func__, opname, *pDT->StrName(), *pVar->Typlex().Str()) ;
		else
			g_EC[_hzlevel()].Printf("%s: using %s found for datatype %s with arg of NULLs\n", __func__, opname, *pDT->StrName()) ;
		return pOper ;
	}

	return 0 ;
}

hzEcode	ceTyplex::Testset	(const ceTyplex& op) const
{
	//	Tests if a variable of this ceTyplex could legally be set by a value of the supplied ceTyplex. This will be true if any of the following are true:-
	//
	//		1) The data types and level of direction is the same and there is no violation of const
	//		2) The variable requies an unsigned integer and the supplied value is an unsigned integer of the same size or smaller
	//		3) The variable requies an signed integer and the supplied value is a signed or unsigned integer of the same size or smaller
	//		4) The variable is of a class that has an assignment operator that will accept the supplied ceTyplex
	//		5) The supplied ceTyplex is a class that has a casting operator to the ceTyplex required by the variable.
	//		6) The variable is a template argument of a class template that has been set to an ceTyplex matching on any of the above.
	//
	//	Note that an object (variable) defined as const may be set equal to either a const or a non const object of a suitable data type. A pointer to a const object or objects can
	//	be changed but not the object(s) pointed to. A reference to a const object is liekwise.
	//
	//	In the operating context of this function, we are testing if a function argument as it is defined, can be set by the argument supplied in a call. If the defined argument is
	//	const the supplied argument can be either const or not const, as long as it has a compatible data type. However if the defined argument is not const, the supplied argument
	//	can only be const as long as it is not subsequently altered by the function. Under HadronZoo coding standards, if the defined argument is not const, the supplied argument
	//	must not be const.
	//
	//	Arguments:	1)	The supplied ceTyplex
	//
	//	Returns:	1)	E_TYPE	If the supplied ceTyplex cannot legally set the variable
	//				2)	E_OK	If the supplied ceTyplex can legally set the variable

	_hzfunc("ceTyplex::Testset") ;

	static hzString	opstr = "operator=" ;

	ceTyplex	resTpx ;
	hzAtom		resVal ;
	hzEcode		rc ;

	rc = ApplyAssignment(resTpx, resVal, *this, op, opstr, TOK_OP_EQ, 0) ;
	if (rc == E_SYNTAX)
		slog.Out(g_EC[_hzlevel()+1]) ;
	return rc ;
}

bool	ceTyplex::operator==	(const ceTyplex& op) const
{
	//	Determine if the operand typlex is the same as this. This will be true only if both are unset OR both are set AND
	//
	//		1) The Whatami() matches
	//		2) The type names match
	//		3) The indirections match
	//		4) The set of arguments (themselves typlexes of course) match

	_hzfunc("ceTyplex::operator==") ;

	if (m_pType && !op.m_pType) return false ;
	if (!m_pType && op.m_pType) return false ;

	if (m_pType && op.m_pType)
	{
		if (m_pType->Whatami() != op.m_pType->Whatami())
			return false ;

		if (m_pType->StrName() != op.m_pType->StrName())
			return false ;
	}

	if (m_Indir != op.m_Indir)
		return false ;

	if ((m_dt_attrs & DT_ATTR_CONST) != (op.m_dt_attrs & DT_ATTR_CONST))
	//if (m_dt_attrs != op.m_dt_attrs)
		return false ;

	if (m_Args.Count() == 0 && op.m_Args.Count() == 0)
		return true ;
	if (m_Args.Count() != op.m_Args.Count())
		return false ;

	//	Now compare the args

	hzList<ceTyplex>::Iter	a ;
	hzList<ceTyplex>::Iter	b ;

	for (a = m_Args, b = op.m_Args ; a.Valid() && b.Valid() ; a++, b++)
	{
		if (a.Element() != b.Element())
			return false ;
	}

	return true ;
}

/*
**	ceFile Functions
*/

hzEcode	ceFile::GetTyplex	(ceTyplex& resultTpx, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start)
{
	//	Determines if the current token amounts to a typlex or is the start of a series of tokens that amount to typlex.
	//
	//	Arguments:	1) tpx		The ceTyplex first cleared then populated by this function. It is this that should be tested by the caller.
	//				2) pNamsp	Applicable namespace for lookup purposes
	//				2) pHost	Host class for lookup purposes
	//				4) end		Reference to post process token position, set by lookup to one place beyond the last token of the typlex.
	//				5) start	The starting token
	//
	//	Returns:	1) E_NOTFOUND	In the event of a lookup not finding a WORD token
	//				2) E_SYNTAX		In the event of a sytax error
	//				3) E_TYPE		In the event the token(s) examined do not amount to a data type
	//				4) E_OK			If the token(s) examined do amount to a data type

	_hzfunc("ceFile::GetTyplex") ;

	ceTyplex	tpx_t ;				//	Typlex for template arg
	ceEntity*	pEnt = 0 ;			//	Entity found
	ceEntity*	pXnt = 0 ;			//	Sub-entity
	ceTypedef*	pTypedef ;			//	If entity is a typedef
	ceTarg*		pTarg ;				//	Template argument
	uint32_t	ct ;				//	Token iterator
	uint32_t	t_end ;				//	Set by Lookup
	bool		bConst = false ;	//	Set if const
	hzEcode		rc = E_OK ;			//	Return code

	resultTpx.Clear() ;
	g_EC[_hzlevel()].Clear() ;

	//	Ignore mutable keyword
	ct = start ;
	if (X[ct].value == "mutable")
		ct++ ;

	//	Handle const (only allowed keyword related to type)
	if (X[ct].type == TOK_VTYPE_CONST)
		{ bConst = true ; ct++ ; }

	//	Ignore leading class/struct token. If we are looking at an already defined type this token is allowed but superflous
	if (X[ct].type == TOK_CLASS || X[ct].type == TOK_STRUCT)
		ct++ ;

	//	Lookup on the current token to see if a type can be identified. Take account of any template arguments first
	if (g_CurTmplArgs.Count())
	{
		for (uint32_t n = 0 ; n < g_CurTmplArgs.Count() ; n++)
		{
			pTarg = g_CurTmplArgs[n] ;
			g_EC[_hzlevel()].Printf("%s %s line %d col %d: Compare token %s to targ %s\n", __func__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, *pTarg->StrName()) ;

			if (pTarg->StrName() == X[ct].value)
			{
				pEnt = pTarg ;
				g_EC[_hzlevel()].Printf("%s %s line %d col %d: Match token %s to targ %s\n", __func__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, *pTarg->StrName()) ;
				ct++ ;
				break ;
			}
		}
	}

	if (!pEnt)
	{
		pEnt = LookupToken(X, pHost, pFET, t_end, ct, true, __func__) ;

		if (!pEnt)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Sequence at %s not a type or other entity\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return E_NOTFOUND ;
		}
		ct = t_end ;
	}

	//	Have entity but if not a type then return empty handed
	if (!pEnt->IsType())
	{
		g_EC[_hzlevel()].Printf("%s (%d): %s line %d col %d: Entity %s not a type\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pEnt->StrName()) ;
		return E_TYPE ;
	}

	//	Handle typedefs - these will always be one-token except for the indirection or address-of operators.
	if (pEnt->Whatami() == ENTITY_TYPEDEF)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Entity %s is a typedef\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pEnt->StrName()) ;
		pTypedef = (ceTypedef*) pEnt ;
		resultTpx = pTypedef->Typlex() ;
	}
	else
	{
		resultTpx.SetType((ceDatatype*) pEnt) ;
		if (resultTpx.Basis() == ADT_NULL)
		{
			g_EC[_hzlevel()].Printf("%s (%d): %s line %d col %d: Entity %s has no type\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pEnt->StrName()) ;
			return E_TYPE ;
		}
	}

	for (; pEnt->Whatami() == ENTITY_CLASS ;)
	//for (; pEnt->Whatami() == ENTITY_CTMPL ;)
	{
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Entity %s is %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pEnt->StrName(), Entity2Txt(pEnt->Whatami())) ;

		if (X[ct].type == TOK_OP_LESS)
		{
			for (ct++ ; ct < X.Count() ; ct++)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Template argument test\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				rc = GetTyplex(tpx_t, pHost, 0, t_end, ct) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Template argument %d is not a type in this scope\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, resultTpx.m_Args.Count()+1) ;
					break ;
				}
				ct = t_end ;

				resultTpx.m_Args.Add(tpx_t) ;

				if (X[ct].type == TOK_SEP)
					continue ;

				if (X[ct].type == TOK_OP_MORE)
					break ;

				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Unexpected token (%s) found while scanning template args\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, X[ct].Show()) ;
				return E_SYNTAX ;
			}

			ct++ ;
		}

		if (X[ct].type == TOK_OP_SCOPE)
		{
			//	Now this must be followed by a subclass
			pHost = (ceKlass*) pEnt ;

			pXnt = LookupToken(X, pHost, pFET, t_end, ct+1, false, __func__) ;
			if (!pXnt)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expected Subclass. Sequence at %s not a type or other entity\n",
					__func__, __LINE__, *StrName(), X[ct+1].line, X[ct+1].col, *X[ct+1].value) ;
				return E_NOTFOUND ;
			}
			ct = t_end ;

			if (!pXnt->IsType())
			{
				g_EC[_hzlevel()].Printf("%s (%d): %s line %d col %d: Expected Subclass. Entity %s not a type\n",
					__func__, __LINE__, *StrName(), X[ct+1].line, X[ct+1].col, *pXnt->StrName()) ;
				return E_TYPE ;
			}

			pEnt = pXnt ;
			resultTpx.SetType((ceDatatype*) pEnt) ;
			continue ;
		}

		break ;
	}

	//	Calculate indirection
	if		(X[ct].value == "***")	{ ct++ ; resultTpx.m_Indir = 3 ; }
	else if	(X[ct].value == "**")	{ ct++ ; resultTpx.m_Indir = 2 ; }
	else if	(X[ct].value == "*")	{ ct++ ; resultTpx.m_Indir = 1 ; }
	else if	(X[ct].value == "&")
		{ ct++ ; resultTpx.m_Indir = 0 ; resultTpx.m_dt_attrs |= DT_ATTR_REFERENCE ; }
	else
		resultTpx.m_Indir = 0 ;

	if (bConst)
		resultTpx.SetAttr(DT_ATTR_CONST) ;

	ultimate = ct ;
	return E_OK ;
}

bool	ceFile::IsCast	(ceTyplex& c_tpx, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start)
{
	//	Determine if the tokens within a matching round brace block amounts to a cast. It is a cast if it is of the form (typlex)

	_hzfunc("ceFile::IsCast") ;

	hzChain		Z ;				//	For building cast from tokens
	ceTyplex	a_tpx ;			//	Argument typlex (if applicable)
	hzString	S ;				//	Temp string
	uint32_t	ct ;			//	Token iterator
	uint32_t	t_end ;			//	Position of current token after aux function call
	hzEcode		rc = E_OK ;		//	Return code

	if (X[start].type != TOK_ROUND_OPEN)
		return false ;

	t_end = X[start].match ;
	for (ct = start ; ct <= t_end ; ct++)
		Z << X[ct].value ;
	S = Z ;

	//	Do not investigate a cast if first token in the () block is a literal value as this cannot form part of a type
	ct = start+1 ;
	if (!(X[ct].type & TOKTYPE_MASK_LITERAL))
	{
		//	We have a possible cast. There must not be any uary operators in force and next token must be some value of a compatible data type
		rc = GetTyplex(c_tpx, pHost, 0, t_end, ct) ;
		if (rc == E_OK)
		{
			ct = t_end ;
			if (X[ct].type == TOK_ROUND_CLOSE)
			{
				//	Cast confirmed
				ct = ct+1 ;
				ultimate = X[start].match ;
				return true ;
			}

			if (X[ct].type == TOK_ROUND_OPEN && X[ct+1].type == TOK_OP_MULT && X[ct+2].type == TOK_ROUND_CLOSE && X[ct+3].type == TOK_ROUND_OPEN)
			{
				//	Potential cast to function pointer
				for (ct += 4 ; rc == E_OK ; ct = t_end+1)
				{
					rc = GetTyplex(a_tpx, pHost, 0, t_end, ct) ;
					if (rc != E_OK)
						break ;

					c_tpx.m_Args.Add(a_tpx) ;
					if (X[t_end].type == TOK_SEP)
						continue ;

					if (X[t_end].type == TOK_ROUND_CLOSE)
						break ;
					rc = E_SYNTAX ;
				}

				if (rc == E_OK)
				{
					ultimate = X[start].match ;
					return true ;
				}
			}
		}
	}

	return false ;
}

/*
**	Expression parsing
*/

hzEcode	ceFile::ReadLiteral	(ceTyplex& tpx, hzAtom& Val, uint32_t tokno, bool neg)
{
	//	Read a single token which is required to be a literal value

	_hzfunc("ceFile::ReadLiteral") ;

	hzString	numval ;		//	Includes numeric token and any preceeding plus/minus sign
	hzEcode	rc = E_OK ;		//	return value

	//	Init
	Val.Clear() ;

	//	Token must be a literal value
	if (!(X[tokno].type & TOKTYPE_MASK_LITERAL))
	{
		slog.Out("%s %s line %d col %d: Token %s not a literal value\n", __func__, *StrName(), X[tokno].line, X[tokno].col, *X[tokno].value) ;
		return E_SYNTAX ;
	}

	//	Check use of neg flag is limited to numbers
	if (neg)
	{
		if (X[tokno].type != TOK_NUMBER && X[tokno].type != TOK_STDNUM)
		{
			slog.Out("%s %s line %d col %d: Token %s Illegal negation\n", __func__, *StrName(), X[tokno].line, X[tokno].col, *X[tokno].value) ;
			return E_SYNTAX ;
		}
	}

	//	Note the term is litteral
	tpx.m_dt_attrs = DT_ATTR_LITERAL ;

	if (X[tokno].type == TOK_QUOTE)
	{
		tpx.SetType(g_cpp_char) ;
		tpx.m_Indir = 1 ;
		Val = *X[tokno].value ;
	}
	else if (X[tokno].type == TOK_SCHAR)
	{
		tpx.SetType(g_cpp_char) ;
		Val = X[tokno].value[0] ;
	}
	else if (X[tokno].type == TOK_HEXNUM)
	{
		tpx.SetType(g_cpp_uint) ;
		rc = Val.SetNumber(X[tokno].value) ;
		if (rc != E_OK)
			slog.Out("%s %s line %d col %d: Token %s Hexnum Failed\n", __func__, *StrName(), X[tokno].line, X[tokno].col, *X[tokno].value) ;
	}
	else if (X[tokno].type == TOK_STDNUM)
	{
		tpx.SetType(g_cpp_uint) ;
		if (Val.SetNumber(X[tokno].value) != E_OK)
		if (rc != E_OK)
			slog.Out("%s %s line %d col %d: Token %s Stdnum Failed\n", __func__, *StrName(), X[tokno].line, X[tokno].col, *X[tokno].value) ;
	}
	else if (X[tokno].type == TOK_NUMBER)
	{
		if (neg == 1)
			{ numval = "-" ; numval += X[tokno].value ; }
		else
		{
			numval = X[tokno].value ;
			if (numval == "0")
				tpx.m_dt_attrs |= DT_ATTR_ZERO ;
		}

		rc = Val.SetNumber(numval) ;
		if (rc != E_OK)
			slog.Out("%s %s line %d col %d: Token %s Integer Failed\n", __func__, *StrName(), X[tokno].line, X[tokno].col, *X[tokno].value) ;
	}
	else if (X[tokno].type == TOK_BOOLEAN)
	{
		if (X[tokno].value != "true" && X[tokno].value != "false")
		{
			slog.Out("%s %s line %d col %d: Token %s Boolean Failed\n", __func__, *StrName(), X[tokno].line, X[tokno].col, *X[tokno].value) ;
			rc = E_SYNTAX ;
		}
		else
		{
			tpx.SetType(g_cpp_bool) ;
			Val = X[tokno].value == "true" ? true : false ;
		}
	}

	if (rc == E_OK)
	{
		switch (Val.Type())
		{
		case BASETYPE_INT64:	tpx.SetType(g_cpp_longlong) ;	break ;
		case BASETYPE_INT32:	tpx.SetType(g_cpp_int) ;		break ;
		case BASETYPE_INT16:	tpx.SetType(g_cpp_short) ;		break ;
		case BASETYPE_BYTE:		tpx.SetType(g_cpp_char) ;		break ;
		case BASETYPE_UINT64:	tpx.SetType(g_cpp_ulonglong) ;	break ;
		case BASETYPE_UINT32:	tpx.SetType(g_cpp_uint) ;		break ;
		case BASETYPE_UINT16:	tpx.SetType(g_cpp_ushort) ;		break ;
		case BASETYPE_UBYTE:	tpx.SetType(g_cpp_uchar) ;		break ;
		}
	}

	return rc ;
}

ceEntity*	ceFile::GetObject	(hzVect<ceStmt>& Stmts, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, uint32_t codeLevel)
{
	//	Locate a variable assumed to start at the current token. This function calls LookupToken on the current token but then follows any member-of operators
	//	as far as possible. The entity resulting from this examination, must be a declared variable. Otherwise this function ???
	//
	//	Arguments:	1) pHost	The class being processed if applicanle.
	//				2) pFET		The entity table of the function being processed if applicable
	//				3) final	The end token of the expression if one is identified. Set to the token after the last token in the last term.
	//				4) start	The current token
	//
	//	Returns:	1) A pointer to the entity if the term identifies one
	//				2) A null pointer if the term does not anount to an entity (eg is a literal value)
	//
	//	Note that if the current token does not form the start of a valid term, the supplied typlex will not aquire a valid ceDatatype. It is by this
	//	means a syntax error is detected.

	_hzfunc("ceFile::GetObject") ;

	hzList<ceFunc*>::Iter	fi ;	//	Function iterator (for getting functon from function group)
	hzList<ceTyplex>::Iter	ta ;	//	Operator iterator
	hzList<ceTarg>::Iter	tai ;	//	Operator iterator

	hzString	S ;					//	Temp string
	hzString	numval ;			//	Includes numeric token and any preceeding plus/minus sign
	ceTyplex	tpxA ;				//	For recursive calls
	ceTyplex	tpxB ;				//	Assessed by a recursive call on 'next part'
	ceTyplex	c_tpx ;				//	For assessing casts
	ceTyplex	t_tpx ;				//	For assessing casts
	hzAtom		valA ;				//	Value found for A (if any)
	hzAtom		valB ;				//	Value found for B (if any)
	ceEntity*	pEnt ;				//	Entity pointer from Lookup()
	ceEntity*	pX ;				//	Entity pointer from direct class lookup
	ceFngrp*	pFgrp ;				//	Actual function called
	ceFunc*		pFunc ;				//	Actual function called
	ceVar*		pVar = 0 ;			//	Actual variable refered to
	ceKlass*	pTmpKlass ;			//	Used where member selection is used
	ceKlass*	pCurKlass ;			//	When variable is a class
	ceFunc*		pOper ;				//	For data type operations
	ceTarg*		pTarg ;				//	For template arg to data type conversion
	uint32_t	ct ;				//	Token iterator
	uint32_t	xt ;				//	Auxilliary token iterator
	uint32_t	t_end ;				//	Limit set by recursed call
	uint32_t	loop_stop = 0 ;		//	Loop stop condition
	int32_t		nDeref = 0 ;		//	Number of * prior to expression term indicating extent to which term is to be dereferenced
	bool		bErr = false ;		//	Set if error
	hzEcode		rc = E_OK ;			//	Return code

	//	Initialize
	ct = start ;

	//	Assess the term which is not in parentheses and will directly amount to a value. Check for literals first
	if (X[ct].type & TOKTYPE_MASK_LITERAL)
		return 0 ;

	//	Term is not a literal value and so must either be or ultimately amount to, a variable or a function call. The examination is done in a
	//	is done in a loop because the imeadiate entity found by lookup on the current token can be followed by member selection operators.

	pTmpKlass = pHost ;

	for (;;)
	{
		//	Bail out of infinite loop
		if (ct == loop_stop)
		{
			slog.Out("%s %s line %d col %d: Loop_stop condition at tok (%s)\n", __func__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			rc = E_SYNTAX ;
			break ;
		}
		loop_stop = ct ;
		
		pEnt = LookupToken(X, pTmpKlass, pFET, t_end, ct, true, __func__) ;
		if (!pEnt)
		{
			slog.Out("%s (%d) %s line %d col %d: Token %s does not equate to an entity\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			return 0 ;
		}
		ct = ultimate = t_end ;

		//	We have a 'final' entity
		if (pEnt->Whatami() == ENTITY_ENVAL)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Token %s -> ENTITY_ENVAL\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_DEFINE)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Token %s -> ENTITY_DEFINE\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_MACRO)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Token %s -> ENTITY_MACRO\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_CFUNC)
		{
			//	For now these cannot be followed by member selection operators
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Token %s -> ENTITY_CFUNC\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			tpxA.SetType(g_cpp_int) ;
			if (X[ct].type != TOK_ROUND_OPEN)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: System func call must be followed by (args..)\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
				return 0 ;
			}
			ct = ultimate = X[ct].match+1 ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_FN_OVLD)
		{
			//	The entity is a function call so ProcFuncCall is called to determine the exact function invoked. The call may be followed by
			//	member selection operators if its return data type is a class, ctmpl or union or a reference or pointer to such an instance.

			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Token %s -> ENTITY_FN_OVLD\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			pFunc = ProcFuncCall(Stmts, (ceFngrp*) pEnt, pHost, pFET, t_end, ct, codeLevel) ;
			if (!pFunc)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Function call %s fails to evaluate\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
				return 0 ;
			}
			ct = ultimate = t_end+1 ;
			//slog.Out("%s (%d) %s line %d col %d: Token %s -> ENTITY_FUNC %s Returning %s\n",
			//	__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, *pFunc->Fqname(), *pFunc->Return().Str()) ;

			if (pFunc->Return().Whatami() == ENTITY_TMPLARG)
			{
				//slog.Out("%s looking for ctmpl xlate\n", __func__) ;

				//	For a template arg to be returned the function must be a member of a class or be itself templated. Look for ctmpl here.
				//	for (pCurKlass = pFunc->ParKlass() ; pCurKlass->ParKlass() && pCurKlass.Whatami() == ENTITY_CLASS ; pCurKlass = pCurKlass->ParKlass()) ;

				pTarg = (ceTarg*) pFunc->Return().Type() ;

				bErr = 1 ;
				xt = 0 ;
				for (ta = t_tpx.m_Args ; ta.Valid() ; ta++)
				{
					if (pTarg->m_Order == xt)
					{
						//slog.Out("%s found ctmpl xlate\n", __func__) ;
						tpxA = ta.Element() ;
						bErr = 0 ;
					}
					xt++ ;
				}

				if (bErr)
				{
					g_EC[_hzlevel()].Printf("%s (%d): Could not translate %s\n", __func__, __LINE__, *pFunc->Return().Str()) ;
					return 0 ;
				}

			}
			else
				tpxA = pFunc->Return() ;
			//break ;
		}

		else if (pEnt->Whatami() == ENTITY_VARIABLE)
		{
			//	If the data type of the variable is a class, ctmpl or union, member selection operators can follow
			pVar = (ceVar*) pEnt ;

			tpxA = pVar->Typlex() ;
			tpxA.SetAttr(DT_ATTR_LVALUE) ;

			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Token %s is ENTITY_VARIABLE of datatype %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value, *pVar->Typlex().Str()) ;

			if (pVar->Typlex().Whatami() == ENTITY_TMPLARG)
			{
				pTarg = (ceTarg*) pVar->Typlex().Type() ;

				bErr = 1 ;
				xt = 0 ;
				for (ta = tpxA.m_Args ; ta.Valid() ; ta++)
				{
					if (pTarg->m_Order == xt)
					{
						//slog.Out("%s found ctmpl xlate\n", __func__) ;
						tpxA = ta.Element() ;
						bErr = 0 ;
					}
					xt++ ;
				}

				if (bErr)
				{
					g_EC[_hzlevel()].Printf("%s (%d): Could not translate %s\n", __func__, __LINE__, *pFunc->Return().Str()) ;
					return 0 ;
				}
			}

			for (; X[ct].type == TOK_SQ_OPEN ;)
			{
				//	Apply the [] operator if present. In most cases the [] block will evaluate to a (preferably unsigned) integer, but there are some
				//	exceptions which have to be coped with. We call AssessExpr on the expression within the [] block to determine its exact ceTyplex.
				//	Note we do this in a loop as it is possible the variable could be a multi-dimensional array.

				rc = AssessExpr(Stmts, tpxB, valB, pHost, pFET, t_end, ct+1, X[ct].match, codeLevel) ;
				if (!tpxB.Type())
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Syntax Error in []\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col) ;
					return 0 ;
				}

				//	The variable data type may have declared [] operators but if not, then the expression within the [] block must amount to a signed
				//	or unsigned integer. If [] operators are specified for the variable data type, it will only be applicable if its argument matches
				//	exactly the ceTyplex of the expression within the supplied [] block. We call LocateOperator to find the operator if it exists. It
				//	is not possible for there to be two [] operators with the same argument.

				pOper = 0 ;
				if (tpxA.Basis() == ADT_CLASS && tpxA.m_Indir == 0)
					pOper = LocateOperator(tpxA, tpxB, "operator[]") ;

				if (pOper)
				{
					//	[] Operator found.
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: operator[] found %d\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, tpxA.m_Indir) ;

					if (pOper->Return().Whatami() != ENTITY_TMPLARG)
					{
						g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: %d Operator returns normal arg %s\n",
							__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pOper->Return().Str()) ;
						tpxA = pOper->Return() ;
					}
					else
					{
						g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Operator returns template arg %d (%s)\n",
							__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, pOper->Return().Type()->m_Order, *pOper->Return().Str()) ;
					}
				}
				else
				{
					//	No [] operator found. Assume integer.
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					if (tpxA.m_Indir < 1)
					{
						g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Error. [] cannot reduce indirection from %d\n",
							__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, tpxA.m_Indir) ;
						return 0 ;
					}
					tpxA.m_Indir-- ;
					//nDeref++ ;
				}

				ct = ultimate = X[ct].match+1 ;
			}

			//	Check for variable being a class instance and followed by member selection
			if (tpxA.Basis() == ADT_CLASS)
			{
				if (X[ct].type == TOK_OP_PERIOD || X[ct].type == TOK_OP_MEMB_PTR)
				{
					pTmpKlass = (ceKlass*) tpxA.Type() ;
					//slog.Out("%s (%d) %s line %d col %d: Member class of %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pTmpKlass->StrName()) ;
					ct++ ;
					continue ;
				}
			}

			//	Check for special case of operator* (as opposed to the standard dereference * appearing before a variable)
			if (nDeref == 1 && tpxA.Basis() == ADT_CLASS)
			{
				if (tpxA.Whatami() == ENTITY_CLASS)
				{
					pCurKlass = (ceKlass*) tpxA.Type() ;
					S = "operator*" ;
					pX = pCurKlass->m_ET.GetEntity(S) ;
					if (pX && pX->Whatami() == ENTITY_FN_OVLD)
					{
						pFgrp = (ceFngrp*) pX ;
						fi = pFgrp->m_functions ;
						pFunc = fi.Element() ;
						tpxA = pFunc->Return() ;
						nDeref = 0 ;
					}

				}
			}
		}
		else
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Syntax Error. %s not handled here\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pEnt->StrName()) ;
			return 0 ;
		}

		if (tpxA.Whatami() != ENTITY_CLASS && tpxA.Whatami() != ENTITY_UNION)
			break ;

		if (X[ct].type != TOK_OP_PERIOD && X[ct].type != TOK_OP_MEMB_PTR)
			break ;

		//slog.Out("%s (%d) %s line %d col %d: Member class of %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *pTmpKlass->StrName()) ;

		pTmpKlass = (ceKlass*) tpxA.Type() ;
		//slog.Out("%s (%d) %s line %d col %d: Have a typlex of %s. Set class to %s\n",
		//	__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *tpxA.Str(), *pTmpKlass->StrName()) ;
		ct++ ;
	}

	//slog.Out("%s (%d) %s line %d col %d: Settled type %s for term %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *tpxA.Str(), *S) ;

	//	Check for ++ and -- coming directly after the term
	if (X[ct].type == TOK_OP_INCR || X[ct].type == TOK_OP_DECR)
		ct++ ;

	if (X[ct].type == TOK_END || X[ct].type == TOK_SEP || X[ct].type == TOK_OP_QUERY || X[ct].type == TOK_OP_SEP || X[ct].type == TOK_ROUND_CLOSE
			|| X[ct].type == TOK_SQ_CLOSE || X[ct].IsOperator())
		return pEnt ;

	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: ERROR type %s with tok now at %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *tpxA.Str(), *X[ct].value) ;
	return 0 ;
}

hzEcode	ceFile::AssessTerm
	(hzVect<ceStmt>& Stmts, hzString& diag, ceTyplex& resultTpx, hzAtom& resultVal, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, uint32_t limit, uint32_t codeLevel)
{
	//	Performs an assessment of a single term for the purposes of expression parsing. The focus is on establishing the typlex of the term and any value found
	//	is important only where the typlex is an enum or in cases such as the literal number 0 when setting pointers etc.
	//
	//	A term may be EITHER of the following literal values:-
	//
	//		1.1) A string literal (string in double quotes)
	//		1.2) A char literal (single char in single quotes)
	//		1.3) A decimal integer literal (unquoted string of decimal digits)
	//		1.4) An octal integer literal (unquoted string of decimal digits with leading 0)
	//		1.5) A hexadecimal integer literal (unquoted string of hexadecimal digits with leading 0x)
	//		1.6) A floating point literal or standard form number
	//
	//	OR a term may be one of the following entities:-
	//
	//		2.1) An enumerated value
	//		2.2) A declared variable or collection of variables or an element within the collection.
	//		2.3) A pointer to (2.2)
	//		2.4) A direct function call
	//		2.5) An indirect function call (using a function pointer)
	//
	//	Note that of these, only variables (2.2), can be an lvalue (something that has a location) and so can be set. This will be the case if the variable is
	//	instansiated as global, static to a file or declared within a function. Class members that are variables, only become lvalues where an instance of the
	//	class has been created.
	//
	//	In the case of class templates and template arguments, these are not always qualified. When expressions are processed during the analysis of a function
	//	that is a member of a class template, the data type are left unqualified in the resuting typlex. It is only where the function is called on an instance
	//	of the class template, will qualified data types be set in the resulting typlex.
	//
	//	Note that terms may be combined with a unary operator so really the general form of a term is: [unary_operator] term.
	//
	//	Arguments:	1) Stmts		The vector of statements this functon will aggregate to
	//				2) resultTpx	The overal typlex of the expression
	//				3) resultVal	The actual value determined if any
	//				4) pHost		The class being processed if applicanle.
	//				5) pFET			The entity table of the function being processed if applicable
	//				6) end			The end token of the expression if one is identified. Set to the token after the last token in the last term.
	//				7) start		The current token
	//				8) limit		The last valid token of the term, if known by the caller - or the end of the statement
	//				9) codeLevel	Code level
	//
	//	Returns:	1) A pointer to the entity if the term identifies one
	//				2) A null pointer if the term does not anount to an entity (eg is a literal value)

	_hzfunc("ceFile::AssessTerm") ;

	hzList<ceFunc*>::Iter	fi ;	//	Function iterator (for getting functon from function group)
	hzList<ceTyplex>::Iter	ta ;	//	Operator iterator
	hzList<ceTarg>::Iter	tai ;	//	Operator iterator

	hzChain		Z ;					//	For adding tokens to get exp for diagnostics
	hzString	S ;					//	Temp string
	hzString	numval ;			//	Includes numeric token and any preceeding plus/minus sign
	ceTyplex	tpxA ;				//	For recursive calls
	ceTyplex	tpxB ;				//	Assessed by a recursive call on 'next part'
	ceTyplex	c_tpx ;				//	For assessing casts
	ceTyplex	t_tpx ;				//	For assessing casts
	hzAtom		valA ;				//	Value found for A (if any)
	hzAtom		valB ;				//	Value found for B (if any)
	ceStmt		Stmt ;				//	Statement to be added if any
	ceEntity*	pEnt ;				//	Entity pointer from Lookup()
	ceEntity*	pX ;				//	Entity pointer from direct class lookup
	ceFngrp*	pFgrp ;				//	Actual function called
	ceFunc*		pFunc ;				//	Actual function called
	ceVar*		pVar = 0 ;			//	Actual variable refered to
	ceKlass*	pTmpKlass = 0 ;		//	Used where member selection is used
	ceKlass*	pCurKlass = 0 ;		//	When variable is a class
	ceEnumval*	pEmv ;				//	For enum values
	ceLiteral*	pLiteral ;			//	For literal values
	ceFunc*		pOper ;				//	For data type operations
	ceTarg*		pTarg ;				//	For template arg to data type conversion
	uint32_t	ct ;				//	Token iterator
	uint32_t	xt ;				//	Auxilliary token iterator
	uint32_t	t_end ;				//	Limit set by recursed call
	uint32_t	bUopFlags = 0 ;		//	Set to control sign (value A)
	uint32_t	loop_stop = 0 ;		//	Loop stop condition
	int32_t		nDeref = 0 ;		//	Number of * prior to expression term indicating extent to which term is to be dereferenced
	bool		bLiteral = false ;	//	Set if the term in hand is a literal value
	bool		bAddr = false ;		//	Set true if term begins with a & (address-of operator). Following token(s) must amount to a declared variable or function
	bool		bErr = false ;		//	Set if error
	hzEcode		rc = E_OK ;			//	Return code

	//	Initialize
	resultTpx.Clear() ;
	for (xt = start ; xt < limit ; xt++)
		Z << X[xt].value ;
	diag = Z ;
	ct = start ;
	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d TERM [%s]\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *diag) ;

	//	Special case of dynamic_cast which must stand as a term on its own e.g. pSub = dynamic_cast<const hdbClass*>(pType) ;
	if (X[ct].type == TOK_OP_DYN_CAST)
	{
		if (X[ct+1].type == TOK_OP_LESS)
		{
			rc = GetTyplex(resultTpx, pHost, 0, t_end, ct+2) ;
			if (rc != E_OK)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d: Parsing Error. dynamic_cast<type>(var) requires type\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
				return E_SYNTAX ;
			}

			g_EC[_hzlevel()].Printf("Checking TOK_OP_DYN_CAST. Have t_end at tok %d %s\n", t_end, *X[t_end].value) ;

			if (X[t_end].type == TOK_OP_MORE && X[t_end+1].type == TOK_ROUND_OPEN && X[t_end+2].type == TOK_WORD && X[X[t_end+1].match].type == TOK_ROUND_CLOSE)
			{
				ultimate = X[t_end+1].match + 1 ;
				return E_OK ;
			}
		}

		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Syntax Error. Malformed dynamic_cast\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
		return E_SYNTAX ;
	}

	//	Check first for a cast
	if (X[ct].type == TOK_ROUND_OPEN)
	{
		if (IsCast(c_tpx, pHost, pFET, t_end, ct))
			ct = X[ct].match+1 ;
	}

	//	Check for dereferencing
	if (X[ct].type == TOK_OP_MULT)
		{ nDeref = 1 ; ct++ ; }
	if (X[ct].type == TOK_INDIR)
		{ nDeref = X[ct].value.Length() ; ct++ ; }

	//	Check for unary operators
	if (X[ct].type == TOK_OP_INCR || X[ct].type == TOK_OP_DECR)
		ct++ ;
	if (X[ct].type == TOK_OP_LOGIC_AND)	{ bAddr = true ; ct++ ; }
	if (X[ct].type == TOK_OP_PLUS)		{ bUopFlags |= 2 ; ct++ ; }
	if (X[ct].type == TOK_OP_MINUS)		{ bUopFlags |= 1 ; ct++ ; }
	if (X[ct].type == TOK_OP_NOT)		{ bUopFlags |= 4 ; ct++ ; }
	if (X[ct].type == TOK_OP_INVERT)	{ bUopFlags |= 8 ; ct++ ; }

	//	Check for the special case of 'this'
	if (X[ct].type == TOK_OP_THIS)
	{
		if (pHost && pHost->Whatami() == ENTITY_CLASS)
		{
			tpxA.SetType(pHost) ;
			tpxA.m_Indir = 1 ;
		}
		else
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Syntax Error. 'this' can only be applied with a host class or ctmpl\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return E_SYNTAX ;
		}
		ct++ ;
		ultimate = ct ;
		tpxA.SetAttr(DT_ATTR_LVALUE) ;
		goto conclude ;
	}

	//	Check for the special case of sizeof
	if (X[ct].type == TOK_OP_SIZEOF)
	{
		//	Not really interested in this. Just confirm there is a () block.
		if (X[ct+1].type != TOK_ROUND_OPEN)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Syntax Error. Malformed sizeof\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return E_SYNTAX ;
		}
		ct = X[ct+1].match+1 ;
		
		tpxA.SetType(g_cpp_int) ;
		tpxA.m_Indir = 0 ;
		ultimate = ct ;
		goto conclude ;
	}

	//	Check for special case of operator new
	if (X[ct].type == TOK_CMD_NEW)
	{
		rc = GetTyplex(tpxA, pHost, 0, t_end, ct+1) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d: Parsing Error. Operator NEW requires type\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return E_SYNTAX ;
		}
		ct = t_end ;

		tpxA.m_Indir++ ;
		if (X[ct].type == TOK_ROUND_OPEN || X[ct].type == TOK_SQ_OPEN)
			ct = X[ct].match+1 ;

		if (X[ct].type != TOK_END)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d: Parsing Error. Operator NEW should be final part of expression\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return E_SYNTAX ;
		}

		ultimate = ct ;
		tpxA.SetAttr(DT_ATTR_LVALUE) ;
		goto conclude ;
	}

	//	After the unary operator the term could be in parenthesis

	//	Since a cast does not eliminate the possibility the value could be in parenthesis we check this here
	if (X[ct].type == TOK_ROUND_OPEN)
	{
		//	We have an expression in parenthesis so recurse to obtain first term
		if (X[ct].match > limit)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d: Parsing Error. () block exceeds limit (ct %d, lim %d)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, ct, limit) ;
			return E_SYNTAX ;
		}

		rc = AssessExpr(Stmts, tpxA, valA, pHost, pFET, t_end, ct+1, X[ct].match, codeLevel+1) ;
		if (!tpxA.Type())
			return rc ;
		ct = ultimate = t_end+1 ;
		goto conclude ;
	}

	//	Assess the term which is not in parentheses and will directly amount to a value. Check for literals first
	if (X[ct].type & TOKTYPE_MASK_LITERAL)
	{
		bLiteral = true ;
		tpxA.m_dt_attrs = DT_ATTR_LITERAL ;

		if (X[ct].type == TOK_QUOTE)
		{
			tpxA.SetType(g_cpp_char) ;
			tpxA.m_Indir = 1 ;
			resultVal = *X[ct].value ;
		}
		else if (X[ct].type == TOK_SCHAR)
		{
			tpxA.SetType(g_cpp_char) ;
			resultVal = X[ct].value[0] ;
		}
		else if (X[ct].type == TOK_HEXNUM)
		{
			tpxA.SetType(g_cpp_uint) ;
			if (resultVal.SetNumber(X[ct].value) != E_OK)
				g_EC[_hzlevel()].Printf("Hexnum failed\n") ;
		}
		else if (X[ct].type == TOK_STDNUM)
		{
			tpxA.SetType(g_cpp_uint) ;
			if (resultVal.SetNumber(X[ct].value) != E_OK)
				g_EC[_hzlevel()].Printf("Stdnum failed\n") ;
		}
		else if (X[ct].type == TOK_NUMBER)
		{
			if (bUopFlags == 1)
				{ numval = "-" ; numval += X[ct].value ; }
			else
			{
				numval = X[ct].value ;
				if (numval == "0")
					tpxA.m_dt_attrs |= DT_ATTR_ZERO ;
			}

			if (resultVal.SetNumber(numval) != E_OK)
				g_EC[_hzlevel()].Printf("Number failed\n") ;
			else
			{
				switch (resultVal.Type())
				{
				case BASETYPE_INT64:	tpxA.SetType(g_cpp_longlong) ;	break ;
				case BASETYPE_INT32:	tpxA.SetType(g_cpp_int) ;		break ;
				case BASETYPE_INT16:	tpxA.SetType(g_cpp_short) ;		break ;
				case BASETYPE_BYTE:		tpxA.SetType(g_cpp_char) ;		break ;
				case BASETYPE_UINT64:	tpxA.SetType(g_cpp_ulonglong) ;	break ;
				case BASETYPE_UINT32:	tpxA.SetType(g_cpp_uint) ;		break ;
				case BASETYPE_UINT16:	tpxA.SetType(g_cpp_ushort) ;	break ;
				case BASETYPE_UBYTE:	tpxA.SetType(g_cpp_uchar );		break ;
				}
			}
		}
		else if (X[ct].type == TOK_BOOLEAN)
		{
			if (X[ct].value != "true" && X[ct].value != "false")
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Token is of type boolean but with a value of %s\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
				return E_SYNTAX ;
			}

			tpxA.SetType(g_cpp_bool) ;
			resultVal = X[ct].value == "true" ? true : false ;
		}

		ct++ ;
		ultimate = ct ;
		goto conclude ;
	}

	//	Term is not a literal value and so must either be or ultimately amount to, a variable or a function call. The examination is done in a
	//	is done in a loop because the imeadiate entity found by lookup on the current token can be followed by member selection operators.

	pTmpKlass = pHost ;

	for (;;)
	{
		//	Bail out of infinite loop
		if (ct == loop_stop)
		{
			g_EC[_hzlevel()].Printf("%s %s line %d col %d: Loop_stop condition at tok (%s)\n", __func__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
			rc = E_SYNTAX ;
			break ;
		}
		loop_stop = ct ;
		
		pEnt = LookupToken(X, pTmpKlass, pFET, t_end, ct, true, __func__) ;
		//pEnt = GetObject(Stmts, pTmpKlass, pFET, t_end, ct, codeLevel+1) ;
		if (!pEnt)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Syntax Error. %s does not equate to an entity\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *X[ct].value) ;
			return E_SYNTAX ;
		}
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Looked up token %s (%d tokens)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *X[ct].value, t_end - ct) ;
		ct = ultimate = t_end ;

		//	We have a 'final' entity
		if (pEnt->Whatami() == ENTITY_ENVAL)
		{
			pEmv = (ceEnumval*) pEnt ;
			tpxA.SetType((ceDatatype*) pEmv->ParEnum()) ;
			resultVal = (uint32_t)pEmv->m_numVal ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_DEFINE)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: lev %d Token %s -> ENTITY_DEFINE\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *X[ct].value) ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_LITERAL)
		{
			pLiteral = (ceLiteral*) pEnt ;
			tpxA.SetType(g_cpp_uint) ;//pLiteral->m_Type) ;
			resultVal = (uint32_t)pLiteral->m_Value.m_uInt32 ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_MACRO)
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: lev %d Token %s -> ENTITY_MACRO\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *X[ct].value) ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_CFUNC)
		{
			//	For now these cannot be followed by member selection operators
			tpxA.SetType(g_cpp_int) ;
			if (X[ct].type != TOK_ROUND_OPEN)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d System func call must be followed by (args..)\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
				return E_SYNTAX ;
			}
			ct = ultimate = X[ct].match+1 ;
			break ;
		}

		if (pEnt->Whatami() == ENTITY_FN_OVLD)
		{
			//	The entity is a function call so ProcFuncCall is called to determine the exact function invoked. The call may be followed by
			//	member selection operators if its return data type is a class, ctmpl or union or a reference or pointer to such an instance.

			if (bAddr)
			{
				//	As we have the address-of operator, the term should be a pointer to function

			}

			if (X[ct].type == TOK_END || X[ct].type == TOK_SEP || X[ct].type == TOK_ROUND_CLOSE)
			{
				//	We have a named function but with no argument block. This means it is only the address of the function we are after. For this to work, there
				//	must only be one function in the group.

				pFgrp = (ceFngrp*) pEnt ;
				if (pFgrp->m_functions.Count() != 1)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d lev %d: Have a function address but with %d members in the group\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, pFgrp->m_functions.Count(), *pFgrp->StrName()) ;
					return E_SYNTAX ;
				}
				fi = pFgrp->m_functions ;
				pFunc = fi.Element() ;
				tpxA = pFunc->Return() ;
				break ;
			}

			pFunc = ProcFuncCall(Stmts, (ceFngrp*) pEnt, pHost, pFET, t_end, ct, codeLevel) ;
			if (!pFunc)
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Function call %s fails to evaluate\n",
					__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *X[ct].value) ;
				return E_SYNTAX ;
			}
			ct = ultimate = t_end+1 ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: lev %d Token %s -> ENTITY_FUNC %s retTpx %s\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *X[ct].value, *pFunc->Fqname(), *pFunc->Return().Str()) ;

			Stmt.m_eSType = STMT_FUNC_CALL ;
			Stmt.m_Object = pFunc->Fqname() ;
			Stmt.m_nLine = X[ct].line ;
			Stmt.m_nLevel = codeLevel ;
			Stmt.m_Start = Stmt.m_End = ct ;
			Stmts.Add(Stmt) ;

			if (pVar && pFunc->Return().Whatami() == ENTITY_TMPLARG)
			{
				pTarg = (ceTarg*) pFunc->Return().Type() ;
				g_EC[_hzlevel()].Printf("%s (%d) looking for ctmpl xlate with var %s and targ order of %d\n", __func__, __LINE__, *pVar->StrName(), pTarg->m_Order) ;

				bErr = 1 ;
				xt = 0 ;
				for (ta = pVar->Typlex().m_Args ; ta.Valid() ; ta++)
				{
					g_EC[_hzlevel()].Printf("%s (%d) trying ctmpl xlate %d on type %s\n", __func__, __LINE__, xt, *ta.Element().Str()) ;
					if (pTarg->m_Order == xt)
					{
						g_EC[_hzlevel()].Printf("%s (%d) found ctmpl xlate\n", __func__, __LINE__) ;
						tpxA = ta.Element() ;
						tpxA.m_Indir += pFunc->Return().m_Indir ;
						bErr = 0 ;
					}
					xt++ ;
				}

				if (bErr)
				{
					g_EC[_hzlevel()].Printf("%s (%d): Could not translate %s\n", __func__, __LINE__, *pFunc->Return().Str()) ;
					return E_SYNTAX ;
				}

			}
			else
				tpxA = pFunc->Return() ;
			//break ;
		}

		else if (pEnt->Whatami() == ENTITY_VARIABLE)
		{
			//	If the data type of the variable is a class, ctmpl or union, member selection operators can follow
			pVar = (ceVar*) pEnt ;

			if (pVar->Typlex().IsFnptr())
			{
				//	We have a function call by virtual of a function pointer. Need to process as such
				if (X[ct].type == TOK_ROUND_OPEN)
				{
					ct = X[ct].match+1 ;

					tpxA.SetType(pVar->Typlex().Type()) ;
					tpxA.m_Indir = pVar->Typlex().m_Indir ;
				}
				else
				{
					tpxA = pVar->Typlex() ;
				}

				ultimate = ct ;
				break ;
			}

			tpxA = pVar->Typlex() ;
			tpxA.SetAttr(DT_ATTR_LVALUE) ;

			if (pVar->Typlex().Whatami() == ENTITY_TMPLARG)
			{
				pTarg = (ceTarg*) pVar->Typlex().Type() ;

				if (tpxA.m_Args.Count())
				{
					bErr = 1 ;
					xt = 0 ;
					for (ta = tpxA.m_Args ; ta.Valid() ; ta++)
					{
						if (pTarg->m_Order == xt)
						{
							tpxA = ta.Element() ;
							bErr = 0 ;
						}
						xt++ ;
					}

					if (bErr)
					{
						g_EC[_hzlevel()].Printf("%s (%d): Could not translate %s\n", __func__, __LINE__, *pFunc->Return().Str()) ;
						return E_SYNTAX ;
					}
				}
			}

			for (; X[ct].type == TOK_SQ_OPEN ;)
			{
				//	Apply the [] operator if present. In most cases the [] block will evaluate to a (preferably unsigned) integer, but there are some
				//	exceptions which have to be coped with. We call AssessExpr on the expression within the [] block to determine its exact ceTyplex.
				//	Note we do this in a loop as it is possible the variable could be a multi-dimensional array.

				if (X[ct].match > limit)
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d: Parsing Error. [] block exceeds limit (ct %d, lim %d)\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, ct, limit) ;
					return E_SYNTAX ;
				}

				rc = AssessExpr(Stmts, tpxB, valB, pHost, pFET, t_end, ct+1, X[ct].match, codeLevel+1) ;
				if (!tpxB.Type())
				{
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Syntax Error in []\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
					return E_SYNTAX ;
				}

				//	The variable data type may have declared [] operators but if not, then the expression within the [] block must amount to a signed
				//	or unsigned integer. If [] operators are specified for the variable data type, it will only be applicable if its argument matches
				//	exactly the ceTyplex of the expression within the supplied [] block. We call LocateOperator to find the operator if it exists. It
				//	is not possible for there to be two [] operators with the same argument.

				pOper = 0 ;
				if (tpxA.Basis() == ADT_CLASS && tpxA.m_Indir == 0)
					pOper = LocateOperator(tpxA, tpxB, "operator[]") ;

				if (pOper)
				{
					//	[] Operator found.
					if (pOper->Return().Whatami() != ENTITY_TMPLARG)
						tpxA = pOper->Return() ;
					else
					{
						if (tpxA.Whatami() == ENTITY_CLASS)
						{
							xt = 0 ;
							for (ta = tpxA.m_Args ; ta.Valid() ; ta++)
							{
								if (pOper->Return().Type()->m_Order == xt)
								{
									tpxA = ta.Element() ;
									break ;
								}
								xt++ ;
							}
						}
					}
				}
				else
				{
					//	No [] operator found. Assume integer.
					g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
					if (tpxA.m_Indir < 1)
					{
						g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Error. [] cannot reduce indirection from %d\n",
							__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, tpxA.m_Indir) ;
						return E_SYNTAX ;
					}
					tpxA.m_Indir-- ;
					//nDeref++ ;
				}

				ct = ultimate = X[ct].match+1 ;
			}

			//	Check for variable being a class instance and followed by member selection
			if (tpxA.Basis() == ADT_CLASS)
			{
				if (X[ct].type == TOK_OP_PERIOD || X[ct].type == TOK_OP_MEMB_PTR)
				{
					pTmpKlass = (ceKlass*) tpxA.Type() ;
					ct++ ;
					continue ;
				}
			}

			//	Check for special case of operator* (as opposed to the standard dereference * appearing before a variable)
			if (nDeref == 1 && tpxA.Basis() == ADT_CLASS)
			{
				if (tpxA.Whatami() == ENTITY_CLASS)
				{
					pCurKlass = (ceKlass*) tpxA.Type() ;
					S = "operator*" ;
					pX = pCurKlass->m_ET.GetEntity(S) ;
					if (pX && pX->Whatami() == ENTITY_FN_OVLD)
					{
						pFgrp = (ceFngrp*) pX ;
						fi = pFgrp->m_functions ;
						pFunc = fi.Element() ;
						tpxA = pFunc->Return() ;
						nDeref = 0 ;
					}

				}
			}
		}
		else
		{
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Syntax Error. %s does not equate to an entity\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *pEnt->StrName()) ;
			return E_SYNTAX ;
		}

		if (tpxA.Whatami() != ENTITY_CLASS && tpxA.Whatami() != ENTITY_UNION)
			break ;

		if (X[ct].type != TOK_OP_PERIOD && X[ct].type != TOK_OP_MEMB_PTR)
			break ;

		pTmpKlass = (ceKlass*) tpxA.Type() ;
		ct++ ;
	}

conclude:
	//	Check type
	if (tpxA.Basis() == ADT_NULL)
		{ bErr = 1 ; g_EC[_hzlevel()].Printf("%s %s line %d col %d lev %d: Reject No data type\n", __func__, *StrName(), X[ct].line, X[ct].col, codeLevel) ; }

	//	Check sign
	if (bUopFlags & 1 && bUopFlags & 2)
		{ bErr = 1 ; g_EC[_hzlevel()].Printf("%s %s line %d col %d lev %d: Reject both + and -\n", __func__, *StrName(), X[ct].line, X[ct].col, codeLevel) ; }

	//	Check indirection
	if (bLiteral && nDeref)
		{ bErr = 1 ; g_EC[_hzlevel()].Printf("%s %s line %d col %d lev %d: Illegal dereference of a literal\n", __func__, *StrName(), X[ct].line, X[ct].col, codeLevel) ; }

	if (bAddr && nDeref)
		{ bErr = 1 ; g_EC[_hzlevel()].Printf("%s %s line %d col %d lev %d: Have both address-of and dereference\n", __func__, *StrName(), X[ct].line, X[ct].col, codeLevel) ; }

	if (bErr)
		return E_SYNTAX ;

	if (bAddr)
		tpxA.m_Indir++ ;

	if (tpxA.m_Indir >= 0 && nDeref > tpxA.m_Indir)
	{
		//	We have a dereference operator in front of the term which otherwise amounts to an instance of an object without indirection. If the object
		//	is of some class, we need to check if the dereference operator belongs to the class

		pOper = 0 ;
		if (tpxA.Basis() == ADT_CLASS)
		{
			tpxB.Clear() ;
			pOper = LocateOperator(tpxA, tpxB, "operator*") ;
			if (pOper)
			{
				tpxA = pOper->Return() ;
				nDeref-- ;
			}
		}

		if (!pOper)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Execisve dereference (%d > %d)\n",
				__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, nDeref, tpxA.m_Indir) ;
			return E_SYNTAX ;
		}
	}

	if (tpxA.m_Indir >= 0)
		tpxA.m_Indir -= nDeref ;

	if (c_tpx.Type())
		tpxA = c_tpx ;

	//	Check for ++ and -- coming directly after the term
	if (X[ct].type == TOK_OP_INCR || X[ct].type == TOK_OP_DECR)
	{
		ct++ ;
		ultimate = ct ;
	}

	if (X[ct].type == TOK_END || X[ct].type == TOK_SEP || X[ct].type == TOK_OP_QUERY || X[ct].type == TOK_OP_SEP || X[ct].type == TOK_ROUND_CLOSE
		|| X[ct].type == TOK_SQ_CLOSE || X[ct].IsOperator())
	{
		resultTpx = tpxA ;
		return E_OK ;
	}

	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Term $$ %s $$ ERROR type %s with tok now at %s\n",
		__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *S, *tpxA.Str(), *X[ct].value) ;
	return E_SYNTAX ;
}

hzEcode	ApplyAssignment	(ceTyplex& resultTpx, hzAtom& resultVal, const ceTyplex& tpxA, const ceTyplex& tpxB, const hzString& opstr, CppLex oper, int32_t codeLevel)
{
	//	Determines the viability of assigmnent operation. Only objects (variables) may be assigned values as only objects are lvalues (location values) meaning
	//	that a memory location exists for the value. Objects may be instances of a fundamental data type or instances of a defined class. The object value can
	//	be set by either an lvalue (another object) or an rvalue (just a value).
	//
	//	At the point where this function is called, the typlex of both the target object and the source value are known.
	//
	//	Arguments:	1) resultTpx	Result typlex (which may differ from the target lvalue
	//				2) resultVal	Result value
	//				3) tpxA			The target typlex. This must be an lvalue
	//				4) tpxB			The source typlex.
	//				5) oper
	//
	//	Returns:	1) E_NOTFOUND	If the target typlex does not support the assigment operator for the source typlex
	//				2) E_OK			If the assignment is vable.
	//
	//	Note that if the target is of a fundamental data type, the source must also be. If the target is of a defined class, it can be set by a soure value of
	//	any typlex as long as the class of the target has an defined operator function which takes the source typlex as its argument.
	//
	//	Note also that because we are only testing assignment operators, the target must support the operator directly. No attempt is made to find operatore od
	//	the source that could /

	_hzfunc("ceFile::ApplyAssignment") ;

	hzList<ceTyplex>::Iter	ta ;	//	Tamplate args iterator
	hzList<ceFunc*>::Iter	opI ;	//	Operations iterator

	ceFunc*		pOper ;				//	Type specific operator if present
	ceDatatype*	pDT_a = 0 ;			//	Secondary Datatype
	ceDatatype*	pDT_b = 0 ;			//	Secondary Datatype

	resultTpx.Clear() ;
	if (!tpxA.Type() || !tpxB.Type())
		return E_SYNTAX ;

	g_EC[_hzlevel()].Clear() ;
	g_EC[_hzlevel()].Printf("%s (%d) level %d: Aplying %s %s %s\n", __func__, __LINE__, codeLevel, *tpxA.Str(), *opstr, *tpxB.Str()) ;

	if (!tpxA.IsLval())
	{
		g_EC[_hzlevel()].Printf("%s (%d) level %d: Aplying %s %s %s\n", __func__, __LINE__, codeLevel, *tpxA.Str(), *opstr, *tpxB.Str()) ;
		g_EC[_hzlevel()].Printf("Target typlex %s is not an L-value\n", __func__, *tpxA.Str()) ;
		//return E_SYNTAX ;
	}

	//	if both target and source are fundamental
	if (tpxA.m_Indir == tpxB.m_Indir && tpxA.Basis() & CPP_DT_INT|CPP_DT_UNT && tpxB.Basis() & CPP_DT_INT|CPP_DT_UNT)
		{ resultTpx = tpxA ; return E_OK ; }

	//	The special case of 0 (whose data type is initially set to UBTYE), can be used to set any pointer 
	if (tpxB.m_dt_attrs & DT_ATTR_ZERO)
	{
		if (tpxA.m_Indir > 0)
			{ resultTpx = tpxA ; return E_OK ; }
		if (tpxA.Basis() & (CPP_DT_INT | CPP_DT_UNT))
			{ resultTpx = tpxA ; return E_OK ; }
	}

	//	Pointers can be set more flexibly
	if (tpxA.m_Indir > 0)
	{
		if (tpxB.Basis() & CPP_DT_INT|CPP_DT_UNT && (oper == TOK_OP_PLUSEQ || oper == TOK_OP_MINUSEQ))
			{ resultTpx = tpxA ; return E_OK ; }
		if (tpxA.Type() == g_cpp_void && tpxB.m_Indir == tpxA.m_Indir)
			{ resultTpx = tpxA ; return E_OK ; }
	}

	//	If target is a class
	if (tpxA.m_Indir == tpxB.m_Indir && tpxA.Type() == tpxB.Type())
		{ resultTpx = tpxA ; return E_OK ; }

	pDT_a = tpxA.m_Indir == 0 && tpxA.Basis() == ADT_CLASS ? tpxA.Type() : 0 ;
	pDT_b = tpxB.m_Indir == 0 && tpxB.Basis() == ADT_CLASS ? tpxB.Type() : 0 ;

	if (tpxA.Basis() == ADT_CLASS)
	{
		//	Check first for assignment operators of tpxA data type that match the supplied operator and take tpxB as the argument
		if (tpxA.m_Indir == 0)
		{
			pOper = LocateOperator(tpxA, tpxB, *opstr) ;
			if (pOper)
			{
				resultTpx = tpxA ;
				return E_OK ;
			}
			if (pOper)		//	???
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) level %d: Applying %s %s %s for result of %s\n",
					__func__, __LINE__, codeLevel, *tpxA.Str(), *opstr, *tpxB.Str(), *pOper->Return().Str()) ;

				if (pOper->Return().Whatami() != ENTITY_TMPLARG)
				{
					g_EC[_hzlevel()].Printf("%s (%d) level %d Operator returns normal arg %s\n", __func__, __LINE__, codeLevel, *pOper->Return().Str()) ;
					resultTpx = pOper->Return() ;
					return E_OK ;
				}

				g_EC[_hzlevel()].Printf("%s (%d) level %d Operator returns template arg %d (%s)\n",
					__func__, __LINE__, codeLevel, pOper->Return().Type()->m_Order, *pOper->Return().Str()) ;
			}
		}
	}

	if (pDT_a == 0 && pDT_b && oper == TOK_OP_EQ)
	{
		for (opI = pDT_b->m_Ops ; opI.Valid() ; opI++)
		{
			pOper = opI.Element() ;

			if (pOper->m_Args.Count())
				continue ;
			if (pOper->Return().Type() == tpxA.Type() && pOper->Return().m_Indir == tpxA.m_Indir)
			{
				//g_EC[_hzlevel()].Printf("%s (%d) level %d: Using casting operator\n", __func__, __LINE__, codeLevel) ;
				return E_OK ;
			}
		}
	}

	g_EC[_hzlevel()].Printf("%s. No %s found for typlex A %s and B %s\n", __func__, *opstr, *tpxA.Str(), *tpxB.Str()) ;
	return E_NOTFOUND ;
}

hzEcode	ApplyOperator	(ceTyplex& resultTpx, hzAtom& resultVal, const ceTyplex& tpxA, const ceTyplex& tpxB, const hzString& opstr, CppLex oper, int32_t codeLevel)
{
	//	Performs A.operator(B)
	//
	//	Note we do not handle assignment operators in this function

	_hzfunc("ApplyOperator") ;

	hzList<ceTyplex>::Iter	ta ;	//	Operator iterator
	hzList<ceFunc*>::Iter	opI ;	//	Operators iterator
	hzList<ceVar*>::Iter	ai ;	//	Arguments iterator

	ceDatatype*	pDT ;				//	Datatype of typlex
	ceDatatype*	pDT_a = 0 ;			//	Secondary Datatype
	ceDatatype*	pDT_b = 0 ;			//	Secondary Datatype
	ceFunc*		pXop ;				//	Operator pointer
	ceFunc*		pOper ;				//	Type specific operator if present
	ceVar*		pArg ;				//	Argument
	hzEcode		rc = E_OK ;			//	Return code

	resultTpx.Clear() ;
	g_EC[_hzlevel()].Clear() ;

	/*
	**	Handle Enum operations.
	*/

	pDT_a = tpxA.Basis() == ADT_ENUM ? tpxA.Type() : 0 ;
	pDT_b = tpxB.Basis() == ADT_ENUM ? tpxB.Type() : 0 ;

	if (pDT_a || pDT_b)
	{
		//	Enum operations are limited. They can be bitwise so it is essential that bitwise opertors are catered for. They are really integers but may not take
		//	part in math operations (expo/root, mult/div or add/sub).

		if ((pDT_a && !(pDT_b || tpxB.Basis() & (CPP_DT_INT|CPP_DT_UNT))) || (pDT_b && !(pDT_a || tpxA.Basis() & (CPP_DT_INT|CPP_DT_UNT))))
		{
			g_EC[_hzlevel()].Printf("%s level %d: Illegal ENUM participants\n", __func__, codeLevel) ;
			return E_SYNTAX ;
		}

		if (oper != TOK_OP_LOGIC_AND && oper != TOK_OP_LOGIC_OR)
		{
			g_EC[_hzlevel()].Printf("%s level %d: Illegal ENUM operation\n", __func__, codeLevel) ;
			return E_SYNTAX ;
		}

		if (tpxA.Basis() == ADT_ENUM && tpxB.Basis() == ADT_ENUM && tpxB.Type() != tpxB.Type())
		{
			g_EC[_hzlevel()].Printf("%s level %d: Illegal ENUM mix %s and %s\n", __func__, codeLevel, *pDT_a->StrName(), *pDT_b->StrName()) ;
			return E_SYNTAX ;
		}

		resultTpx.SetType(pDT_a ? pDT_a : pDT_b) ;
		return E_OK ;
	}

	/*
	**	Handle operations on doubles.
	*/

	pDT_a = tpxA.Basis() == ADT_DOUBLE ? tpxA.Type() : 0 ;
	pDT_b = tpxB.Basis() == ADT_DOUBLE ? tpxB.Type() : 0 ;

	if (pDT_a || pDT_b)
	{
		//	If either A or B is a double, the result if legal, will be a double. Doubles may be operated on by other doubles or integers of any form and on all
		//	such cases, the result will be a double. Certain operations are excluded. Doubles may not be shifted.

		resultTpx.SetType(pDT_a ? pDT_a : pDT_b) ;
		return E_OK ;
	}

	/*
	**	Handle operations on classes.
	*/

	pDT_a = tpxA.m_Indir == 0 && tpxA.Basis() == ADT_CLASS ? tpxA.Type() : 0 ;
	pDT_b = tpxB.m_Indir == 0 && tpxB.Basis() == ADT_CLASS ? tpxB.Type() : 0 ;

	if (pDT_a || pDT_b)
	{
		//	Classes cannot undergo operations without having operators specified that will have the right return typlex and accept the right typlex as argument.
		//	An operation is possible between two values if A is a class and has an operator that accepts the typlex of B as an argument OR if B is a class and
		//	an a operator has been defined as a friend to B, such that will directly facilitate A op B. For example the non-member function:-
		//
		//		hzString	operator+	(char*, const hzString&) ;

		pOper = LocateOperator(tpxA, tpxB, *opstr) ;
		if (pOper)
		{
			//aoErr.Printf("%s (%d) level %d: Case 1 Applying %s %s %s for result of %s\n",
			//	__func__, __LINE__, codeLevel, *tpxA.Str(), *opstr, *tpxB.Str(), *pOper->Return().Str()) ;

			if (pOper->Return().Whatami() != ENTITY_TMPLARG)
			{
				//aoErr.Printf("%s (%d) level %d Operator returns normal arg %s\n", __func__, __LINE__, codeLevel, *pOper->Return().Str()) ;
				resultTpx = pOper->Return() ;
				goto conclude ;
			}

			//aoErr.Printf("%s (%d) level %d Operator returns template arg %d (%s)\n",
			//	__func__, __LINE__, codeLevel, pOper->Return().Type()->m_Order, *pOper->Return().Str()) ;
			goto conclude ;
		}

		//	A is not a class so look for operators linking A and B
		pOper = LocateOperator(tpxB, tpxA, *opstr) ;
		if (pOper)
		{
			//aoErr.Printf("%s (%d) level %d: Case 2 Applying %s %s %s for result of %s\n",
			//	__func__, __LINE__, codeLevel, *tpxA.Str(), *opstr, *tpxB.Str(), *pOper->Return().Str()) ;
			resultTpx = pOper->Return() ;
			goto conclude ;
		}
	}

	//	Now check for the case where the supplied argument is an instance or a reference to an instance of a class with a casting operator to the required type 
	if (tpxA.m_Indir == 0 && tpxA.Basis() == ADT_CLASS)
	{
		//	Check if this class has any constructors or assigmnent operators that take the supplied typlex
		pDT = tpxA.Type() ;
		//aoErr.Printf("%s checking %d operators for this\n", __func__, pDT->m_Ops.Count()) ;
		for (opI = pDT->m_Ops ; opI.Valid() ; opI++)
		{
			pXop = opI.Element() ;

			if (!pXop->m_Args.Count())
				continue ;

			ai = pXop->m_Args ;
			pArg = ai.Element() ;

			if (pArg->Typlex().Type() == tpxB.Type())
			{
				resultTpx = pXop->Return() ;
				goto conclude ;
			}
		}
	}

	if (tpxB.m_Indir == 0 && tpxB.Basis() == ADT_CLASS)
	{
		//	Check if this class has any constructors or assigmnent operators that take the supplied typlex
		pDT = tpxB.Type() ;
		//aoErr.Printf("%s checking %d operators for suplied\n", __func__, pDT->m_Ops.Count()) ;
		for (opI = pDT->m_Ops ; opI.Valid() ; opI++)
		{
			pXop = opI.Element() ;
			if (pXop->Return().Type() == tpxA.Type())
			{
				resultTpx = pXop->Return() ;
				goto conclude ;
			}
		}
	}

	if (tpxA.m_Indir > 0 && tpxB.m_Indir == 0 && tpxB.Basis() & (CPP_DT_INT | CPP_DT_UNT))
	{
		//	tpxA is a pointer and so may participate in pointer arithmetic even if unwise. All other arithmetic operations are meaningless
		if (oper == TOK_OP_PLUSEQ || oper == TOK_OP_PLUS)
		{
			resultTpx = tpxA ;
			goto conclude ;
		}

		if (oper & TOKTYPE_MASK_CONDITION)
		{
			resultTpx.SetType(g_cpp_bool) ;
			goto conclude ;
		}
	}

	if (tpxA.m_Indir >= 0 && tpxB.m_Indir >= 0 && tpxA.m_Indir != tpxB.m_Indir)
	{
		g_EC[_hzlevel()].Printf("%s (%d): A and B have incompatible indirection (%d and %d)\n", __func__, __LINE__, tpxA.m_Indir, tpxB.m_Indir) ;
		return E_SYNTAX ;
	}

	if (tpxA.Basis() & CPP_DT_INT)
	{
		//	Any arithmetic operation between a signed integer and either a singed or unsigned integer, will result in a signed integer. The size will
		//	be largest of the two operand data types.

		if (tpxB.Basis() & CPP_DT_INT || tpxB.Basis() & CPP_DT_UNT)
		{
			if (oper & TOKTYPE_MASK_CONDITION)
				{ resultTpx.SetType(g_cpp_bool) ; goto conclude ; }

			if (oper & TOKTYPE_MASK_BINARY)
			{
				//aoErr.Printf("%s (%d): Op merge case A\n", __func__, __LINE__) ;
				resultTpx.SetType(tpxA.Basis() & 0xff > tpxB.Basis() & 0xff ? tpxB.Type() : tpxA.Type()) ;
				goto conclude ;
			}
		}
	}

	if (tpxA.Basis() & CPP_DT_UNT)
	{
		if (tpxB.Basis() & CPP_DT_UNT)
		{
			if (oper & TOKTYPE_MASK_CONDITION)
				{ resultTpx.SetType(g_cpp_bool) ; goto conclude ; }

			if (oper & TOKTYPE_MASK_BINARY)
			{
				//aoErr.Printf("%s (%d): Op merge case B\n", __func__, __LINE__) ;
				resultTpx.SetType(tpxA.Basis() & 0xff > tpxB.Basis() & 0xff ? tpxB.Type() : tpxA.Type()) ;
				goto conclude ;
			}
		}
		if (tpxB.Basis() & CPP_DT_INT)
		{
			if (oper & TOKTYPE_MASK_CONDITION)
				{ resultTpx.SetType(g_cpp_bool) ; goto conclude ; }

			if (oper & TOKTYPE_MASK_BINARY)
			{
				//aoErr.Printf("%s (%d): Op merge case C\n", __func__, __LINE__) ;
			resultTpx.SetType(tpxA.Basis() & 0xff > tpxB.Basis() & 0xff ? tpxB.Type() : tpxA.Type()) ;
			goto conclude ;
			}
		}
	}

	//	Numbers may participate in arithmetic operations with other numbers and may be tested for <, <=, >, >= and ==
	if (oper & TOKTYPE_MASK_CONDITION)
	{
		resultTpx.SetType(g_cpp_bool) ;
		goto conclude ;
	}

	g_EC[_hzlevel()].Printf("%s (%d): Op merge case D with A as %s (%d) and B as %s (%d) and op %s\n",
		__func__, __LINE__, *tpxA.Str(), tpxA.Basis(), *tpxB.Str(), tpxB.Basis(), *opstr) ;

conclude:
	return rc ;
}

/*
**	Special classes for handling expression terms and operators
*/

hzEcode	ceFile::AssessExpr
	(hzVect<ceStmt>& Stmts, ceTyplex& theTpx, hzAtom& theVal, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, uint32_t limit, uint32_t codeLevel)
{
	//	Evaluate an expression. If the evaluation is successful, the expression is translated into a set of one or more statements and a single overall value of a (fully qualified)
	//	data type is established. Expressions can have three possible forms as follows:-
	//
	//		(a) A single term
	//		(b) First-term THEN binary_operator THEN remainder_of_expression.
	//		(c) Condition ? expressionA : expressionB
	//
	//	Note that all three components of (c) are expressions of either form (a) or (b). Standard infix rules apply to these expressions.
	//
	//	Arithmetic operators have an order of precedence: Exponents and roots are processed first, then multiplications and divisions, then additions and subtractions. The order of
	//	precedece is implimented by multiple parses. In the first parse, all sub-expressions that amount to a pair of terms linked by an exponent or root operator, are evaluated to
	//	produce a single result term, which then replaces the pair. In the second parse, all sub-expressions that amount to a pair of terms linked by a multiply or divide operator,
	//	are likewise evaluated into single terms. In the third pass, all terms are simply added and subracted.
	//
	//	Note also that in the above forms, only binary operators are mentioned. Unary operators are treated as though they are part of a term to which they apply.
	//
	//	Note that for an expression to be valid, all unary operators applied to terms and all binary operators applied to pairs of terms, must be viable. In all cases there must be
	//	either a defined function (witin the body of code under examination), whose argument(s) exactly matches the data types of the term or pair of terms OR an in-built rule that
	//	makes good the absence of such a function. For example, numeric data types such as int or double, will not need functions within the code body to cover the +/- operators as
	//	these are in-built.
	//
	//	Unary operators are interpreted as such where they preceed the first term, come after the last term, or are adjacent to a single binary operator inbetween terms - and where
	//	they are valid for the data type of the term to which they apply. For example in the expression a=b*-5; the minus is unary and applies to the literal term '5'. The asterisk
	//	can be a unary dereference operator but ONLY if it comes before a term that can actually be dereferenced! Since it comes after b and cannot dereference a literal number, it
	//	has to be a multiply operator which must always be a binary. It must therefore be the binary operator between b and 5. Some operators are always unary such as '!' and '~'.
	//	These wto operators apply to integer and boolean terms only. The cast can be applied to pointers, references to class instances but also to class instances where the class
	//	in question has the appropriate casting operators.
	//
	//	Note that integer literals are treated as having the smallest possible integer type needed to accomodate the value. And they will be assumed unsigned unless they are stated
	//	as negative by the application of the minus unary operator. In functions such as ProcFuncCall, where supplied arguments are tested against those expected by the function or
	//	varients of it, this is taken into account. Integers can set larger integers but not smaller integers. Unsigned integers can set signed integers but signed integers can not
	//	set unsigned integers.
	//
	//	The AssessExpr function works by dividing up the expression into a series of one or more terms, which are evaluated by a call to AssessTerm. The AssessTerm function expects
	//	terms to be supplied together with any unary operators that apply. Terms which include expressions in either round braces (parenthesis) or square braces (array offsets) are
	//	also passed to AssessTerm which will then call AssessExpr. Note there is no confusion between an expression in parenthesis and function call arguments.
	//
	//	Note that function calls amount to a term if there is a return code.
	//
	//	Note that ...
	//
	//	Arguments:	1) Stmts		The vector of statements this functon will aggregate to
	//				2) resultTpx	The overal typlex of the expression
	//				3) resultVal	The actual value determined if any
	//				4) pHost		The class being processed if applicanle.
	//				5) pFET			The entity table of the function being processed if applicable
	//				6) end			The end token of the expression if one is identified. Set to the token after the last token in the last term.
	//				7) start		The current token
	//				8) limit		The last valid token of the term, if known by the caller - or the end of the statement
	//				9) codeLevel	Code level
	//
	//	Returns:	The entity type. This will be ENTITY_UNKNOWN if the current token does not amount to an entity in the current scope
	//				and will be the entity type otherwise. Note that if the entity is a function the supplied ceValue instance will not
	//				be set as a function cannot be fully evaluated here. Nor will it be set if a variable is name. If it is a number or
	//				string it will be set to this value.

	_hzfunc("ceFile::AssessExpr") ;

	hzList<ceFunc*>::Iter	fi ;	//	For getting functon from function group
	hzList<ceTyplex>::Iter	ta ;	//	Operator iterator

	hzVect<int32_t>		conds ;		//	Positions in the expression where conditional operators appear.
	hzVect<int32_t>		points ;	//	Points in the expression where execution operators appear.
	hzVect<ceTyplex>	terms ;		//	Assessed terms

	hzChain		D ;					//	Diagnostics (for printing expression)
	hzString	S ;					//	Diagnostics (for printing expression)
	hzString	numval ;			//	Includes numeric token and any preceeding plus/minus sign
	hzString	opstr ;				//	Temp string
	hzString	term ;				//	Current term
	ceTyplex	tpxA ;				//	For recursive calls
	ceTyplex	tpxB ;				//	Assessed by a recursive call on 'next part'
	ceTyplex	tpxC ;				//	Assessed by a recursive call on 'next part'
	ceTyplex	c_tpx ;				//	For assessing casts
	hzAtom		valA ;				//	Value found for A (if any)
	hzAtom		valB ;				//	Value found for B (if any)
	hzAtom		valC ;				//	Value found for B (if any)
	uint32_t	ct ;				//	Token iterator
	uint32_t	t_end ;				//	Limit set by recursed call
	uint32_t	match ;				//	Match for opening braces
	uint32_t	apos = 0 ;			//	Position of the first assignment operator (if present)
	uint32_t	qpos = 0 ;			//	Position of the first ? in the even of a conditional expression
	uint32_t	spos = 0 ;			//	Position of the matching : to the ?
	uint32_t	xpos = 0 ;			//	Position of the first binary operator
	uint32_t	n ;					//	Operations/results iterator
	uint32_t	opmask ;			//	Filter exponent/root, mult/div and add/sub operations
	CppLex		oper ;				//	Operator
	bool		bExpectOp ;			//	An binary operator can be expected anytime after a term has begun
	hzEcode		rc = E_OK ;			//	Return code

	/*
	**	Pre-process the expression to look for conditional expression operators
	*/

	theTpx.Clear() ;
	bExpectOp = false ;

	for (ct = start ; ct < limit ; ct++)
		D << X[ct].value ;
	S = D ;

	//	Because of statements of the form "object = new object_type();" are assignment expressions, they get handed as is to this function. These are 

	//	Build chain of complete expression and record the first assignment operator if present
	for (ct = start ; ct < limit ; ct++)
	{
		//	bypass [] blocks
		if (X[ct].type == TOK_SQ_OPEN)
		{
			match = X[ct].match ;
			ct = match ;
			bExpectOp = true ;
			continue ;
		}

		//	bypass () blocks
		if (X[ct].type == TOK_ROUND_OPEN)
		{
			if (IsCast(c_tpx, pHost, pFET, t_end, ct))
				bExpectOp = false ;
			else
				bExpectOp = true ;

			match = X[ct].match ;
			ct = match ;
			continue ;
		}

		//	Deal with sizeof so as to not confuse the (type) part with a cast
		if (X[ct].type == TOK_OP_SIZEOF)
		{
			if (X[ct+1].type != TOK_ROUND_OPEN)
			{
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Syntax Error. Malformed sizeof\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
				return E_SYNTAX ;
			}
			ct = X[ct+1].match ;
			bExpectOp = true ;
			continue ;
		}

		//	Deal with the dynamic cast construct
		if (X[ct].type == TOK_OP_DYN_CAST)
		{
			if (X[ct+1].type == TOK_OP_LESS)
			{
				rc = GetTyplex(c_tpx, pHost, 0, t_end, ct+2) ;
				if (rc != E_OK)
				{
					g_EC[_hzlevel()] < g_EC[_hzlevel()+1] ;
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d: Parsing Error. dynamic_cast<type>(var) requires type\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
					return E_SYNTAX ;
				}

				g_EC[_hzlevel()].Printf("Checking TOK_OP_DYN_CAST. Have t_end at tok %d %s\n", t_end, *X[t_end].value) ;

				if (X[t_end].type == TOK_OP_MORE && X[t_end+1].type == TOK_ROUND_OPEN && X[t_end+2].type == TOK_WORD && X[X[t_end+1].match].type == TOK_ROUND_CLOSE)
				{
					ultimate = X[t_end+1].match + 1 ;
					return E_OK ;
				}
			}

			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Syntax Error. Malformed dynamic_cast\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return E_SYNTAX ;
		}

		//	Deal with the dynamic_cast construct
		if (X[ct].type == TOK_KW_OPERATOR)
		{
			bExpectOp = true ;
			if (X[ct+1].type == TOK_SQ_OPEN && X[ct+2].type == TOK_SQ_CLOSE)
				{ ct += 2 ; continue ; }
			if (X[ct+1].IsOperator())
				ct++ ;
			continue ;
		}

		if (bExpectOp && X[ct].IsOpAssign())
		{
			if (!apos)
			{
				if (points.Count())
				{
					g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d: Assignment operator must be the first operator if present\n",
						__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
					return E_SYNTAX ;
				}
				apos = ct ;
			}
			points.Add(ct) ;
			bExpectOp = false ;
			continue ;
		}

		if (bExpectOp && X[ct].IsOpCond())
		{
			if (!xpos)
				xpos = ct ;
			conds.Add(ct) ;
		}

		if (bExpectOp && X[ct].IsOpBinary() && (!(X[ct].type == TOK_OP_MULT && X[ct+1].type == TOK_SQ_OPEN)))
		{
			//	Note the bizarre test for * operator not followed by a [ is because of the confusion caused by constructs such as new class*[] ;
			if (!xpos)
				xpos = ct ;
			points.Add(ct) ;
			bExpectOp = false ;
			continue ;
		}

		if (!qpos && X[ct].type == TOK_OP_QUERY)
		{
			qpos = ct ;
			spos = X[qpos].match ;
			bExpectOp = false ;
			continue ;
		}

		bExpectOp = true ;
	}
	ct = start ;
	g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d EXPRESSION %s\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *S) ;

	//	Check if the expression is valid. It may either be:-
	//
	//		1) A single term. This can be a function call or an object with a value.
	//		2) An executive expression in which terms are executed or evaluated. Of the form term OR term-op-remainder_of_expression.
	//		3) A conditional expression comprising only conditional operators.
	//		4) A conditional expression of the form "(condition) ? exec1 : exec2;"
	//		5) An assignment expression of the form "object=expression;" where the expression on the RHS is either of type 1, 2 or 4.
	//
	//	Note that (1) can be a function call that returns no value. If this is the case then it must be the only term in the expression.
	//
	//	Note also that expressions of type 4 are broken into their three separate expressions and processed by three separate recursive calls to this function.
	//	Note also that the condition part is not the same thing as a conditional expression. It can be anything that evalutes to a value in other words any of
	//	the above - as long as braces are applied where nessesary.

	/*
	**	If the expression is an assignment
	*/

	if (apos)
	{
		//	Read the first term
		rc = AssessTerm(Stmts, term, tpxA, valA, pHost, pFET, t_end, start, apos, codeLevel) ;
		if (!tpxA.Type())
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d 1st term FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return rc ;
		}

		if (X[apos+1].type == TOK_CMD_NEW)
		{
			//	In the special case of an allocation by new, we call AssessTerm
			rc = AssessTerm(Stmts, term, tpxB, valB, pHost, pFET, t_end, apos+1, limit, codeLevel) ;
			if (!tpxA.Type())
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d alloc term FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
				return rc ;
			}
		}
		else
		{
			//	Parse the rest of expression
			rc = AssessExpr(Stmts, tpxB, valB, pHost, pFET, t_end, apos+1, limit, codeLevel+1) ;
			if (!tpxB.Type())
			{
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
				g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Exp1 FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
				return E_SYNTAX ;
			}
		}

		//	Apply operator
		oper = X[apos].type ;
		opstr = "operator" ;
		opstr += X[apos].value ;

		g_EC[_hzlevel()].Printf("%s. (%d) %s line %d col %d: level %d Applying assign between %s and %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *tpxA.Str(), *tpxB.Str()) ;

		rc = ApplyAssignment(theTpx, valA, tpxA, tpxB, opstr, oper, codeLevel) ;
		if (rc != E_OK)
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d level %d: Assigment FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return rc ;
		}

		ultimate = t_end ;
		return E_OK ;
	}

	/*
	**	If the expression is conditional
	*/

	if (qpos)
	{
		//	Parse the cond
		AssessExpr(Stmts, tpxA, valA, pHost, pFET, t_end, start, qpos, codeLevel) ;
		if (!tpxA.Type())
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Cond FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return E_SYNTAX ;
		}

		//	Parse exp1
		rc = AssessExpr(Stmts, tpxB, valB, pHost, pFET, t_end, qpos+1, spos, codeLevel+1) ;
		if (!tpxB.Type())
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Exp1 FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return E_SYNTAX ;
		}

		//	Parse exp2
		rc = AssessExpr(Stmts, tpxC, valC, pHost, pFET, t_end, spos+1, limit, codeLevel+1) ;

		if (!tpxC.Type())
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d Exp2 FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return E_SYNTAX ;
		}

		theTpx = tpxC ;
		ultimate = t_end ;
		return E_OK ;
	}

	/*
	**	The Single term case
	*/

	if (!xpos)
	{
		rc = AssessTerm(Stmts, term, tpxA, valA, pHost, pFET, t_end, ct, limit, codeLevel+1) ;
		if (!tpxA.Type())
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d 1st term FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return rc ;
		}
		ct = t_end ;

		theTpx = tpxA ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d EXP is a single term. Returning type %s with tok now at %s\n",
			__func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel, *theTpx.Str(), *X[ct].value) ;	//Dta2Txt(theTpx.Basis()), *X[ct].value) ;
		ultimate = ct ;
		return rc ;
	}

	/*
	**	All other expressions.
	*/

	//	Go through vector of operators four times. Once for exponent/root operators, once for mult/div, once for add/sub and again for all others. Note that at
	//	this point in the code, there can be no assignment operators.

	if (conds.Count() == points.Count())
	{
		//g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d level %d: ALL CONDITION EXP\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
		theTpx.SetType(g_cpp_bool) ;
		ultimate = limit ;
		return E_OK ;
	}

	rc = AssessTerm(Stmts, term, tpxA, valA, pHost, pFET, t_end, start, points[0], codeLevel+1) ;
	if (rc != E_OK || !tpxA.Type())
	{
		g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d 1st term FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
		return rc ;
	}
	terms.Add(tpxA) ;

	for (n = 1 ; n < points.Count() ; n++)
	{
		rc = AssessTerm(Stmts, term, tpxA, valA, pHost, pFET, t_end, points[n-1]+1, points[n], codeLevel+1) ;
		if (rc != E_OK || !tpxA.Type())
		{
			g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d 1st term FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
			return rc ;
		}
		terms.Add(tpxA) ;
	}

	rc = AssessTerm(Stmts, term, tpxA, valA, pHost, pFET, t_end, points[n-1]+1, limit, codeLevel+1) ;
	if (rc != E_OK || !tpxA.Type())
	{
		g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: level %d 1st term FAILED\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, codeLevel) ;
		return rc ;
	}
	terms.Add(tpxA) ;

	for (opmask = TOKTYPE_MASK_LOGIC ; rc == E_OK && points.Count() >= 1 ;)
	{
		for (n = 0 ; rc == E_OK && points.Count() && n < points.Count() ;)	// n++)
		{
			xpos = points[n] ;
			if (!(X[xpos].type & opmask))
				{ n++ ; continue ; }

			oper = X[xpos].type ;
			opstr = "operator" ;
			opstr += X[xpos].value ;

			//	Apply operator
			rc = ApplyOperator(tpxA, valA, terms[n], terms[n+1], opstr, oper, codeLevel) ;
			if (rc != E_OK)
				g_EC[_hzlevel()] << g_EC[_hzlevel()+1] ;
			else
			{
				terms[n] = tpxA ;
				terms.Delete(n+1) ;
				points.Delete(n) ;
			}
		}

		if (opmask == TOKTYPE_MASK_LOGIC)		{ opmask = TOKTYPE_MASK_MATH_MLT ; continue ; }
		if (opmask == TOKTYPE_MASK_MATH_MLT)	{ opmask = TOKTYPE_MASK_MATH_ADD ; continue ; }
		if (opmask == TOKTYPE_MASK_MATH_ADD)	{ opmask = TOKTYPE_MASK_BINARY ; continue ; }

		break ;
	}

	if (rc == E_OK)
		theTpx = terms[0] ;

	//	Conclude
	ultimate = limit ;
	if (rc != E_OK)
		g_EC[_hzlevel()].Printf("%s (%d) %s line %d col %d: Expression invalid (at tok %s)\n", __func__, __LINE__, *StrName(), X[ct].line, X[ct].col, *X[ct].value) ;
	return rc ;
}
