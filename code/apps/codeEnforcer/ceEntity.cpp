//
//	File:	ceEntity.cpp
//
//  Legal Notice: This file is part of the HadronZoo::CodeEnforcer program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
//
//  Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//  HadronZoo::CodeEnforcer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or any later version.
//
//  The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//  as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//  HadronZoo::CodeEnforcer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with HadronZoo::CodeEnforcer. If not, see http://www.gnu.org/licenses.
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

extern	hzProcess	proc ;					//	Process instance (all HadronZoo apps need this)
extern	hzLogger	slog ;					//	Logfile
extern	ceProject	g_Project ;				//	Project (current)
extern	hzString	g_sysDfltDesc ;			//	Default description for system entities

static	uint32_t	s_seq_ceClass = 0 ;		//	Sequence for classes
static	uint32_t	s_seq_ceUnion = 0 ;		//	Sequence for unions
static	uint32_t	s_seq_ceFnset = 0 ;		//	Sequence for function sets
static	uint32_t	s_seq_ceFunc = 0 ;		//	Sequence for functions
static	uint32_t	s_seq_ceEnum = 0 ;		//	Sequence for enumerations
static	uint32_t	s_seq_ceEnt = 0 ;		//	Sequence for all entities

/*
**	Misc Non member functions
*/

const char*	Entity2Txt	(ceEntType e)
{
	static	hzString	s_null		= "Invalid entity" ;
	static	hzString	s_cuNull	= "CE_UNIT_UNKNOWN" ;
	static	hzString	s_cuProj	= "CE_UNIT_PROJECT" ;
	static	hzString	s_cuComp	= "CE_UNIT_COMPONENT" ;
	static	hzString	s_cuFile	= "CE_UNIT_FILE" ;
	static	hzString	s_cuSynop	= "CE_UNIT_SYNOPSIS" ;
	static	hzString	s_cuCateg	= "CE_UNIT_CATEGORY" ;
	static	hzString	s_cuClGrp	= "CE_UNIT_CLASS_GRP" ;
	static	hzString	s_cuFnGrp	= "CE_UNIT_FUNC_GRP" ;

	static	hzString	s_enNamesp	= "ENTITY_NAMESPACE" ;
	static	hzString	s_enClass	= "ENTITY_CLASS" ;
	static	hzString	s_enUnion	= "ENTITY_UNION" ;
	static	hzString	s_enEnum	= "ENTITY_ENUM" ;
	static	hzString	s_enEnval	= "ENTITY_ENUM_VALUE" ;
	static	hzString	s_enCStd	= "ENTITY_CSTD" ;

	static	hzString	s_enVarib	= "ENTITY_VARIABLE" ;
	static	hzString	s_enFunc	= "ENTITY_FUNCTION" ;
	static	hzString	s_enFnOvld	= "ENTITY_FN_OVLD" ;
	static	hzString	s_enFnptr	= "ENTITY_FNPTR" ;
	static	hzString	s_enCFunc	= "ENTITY_CFUNC" ;

	static	hzString	s_enTmplarg	= "ENTITY_TMPLARG" ;
	static	hzString	s_enMacro	= "ENTITY_MACRO" ;
	static	hzString	s_enHashdef	= "ENTITY_DEFINE" ;
	static	hzString	s_enCLit	= "ENTITY_LITERAL" ;
	static	hzString	s_enTypdef	= "ENTITY_TYPEDEF" ;

	const char*	s ;

	switch	(e)
	{
	case CE_UNIT_UNKNOWN:	s = *s_cuNull ;		break ;
	case CE_UNIT_PROJECT:	s = *s_cuProj ;		break ;
	case CE_UNIT_COMPONENT:	s = *s_cuComp ;		break ;
	case CE_UNIT_FILE:		s = *s_cuFile ;		break ;
	case CE_UNIT_SYNOPSIS:	s = *s_cuSynop ;	break ;
	case CE_UNIT_CATEGORY:	s = *s_cuCateg ;	break ;
	case CE_UNIT_CLASS_GRP:	s = *s_cuClGrp ;	break ;
	case CE_UNIT_FUNC_GRP:	s = *s_cuFnGrp ;	break ;

	case ENTITY_NAMSP:		s = *s_enNamesp ;	break ;
	case ENTITY_CLASS:		s = *s_enClass ;	break ;
	case ENTITY_UNION:		s = *s_enUnion ;	break ;
	case ENTITY_ENUM:		s = *s_enEnum ;		break ;
	case ENTITY_ENVAL:		s = *s_enEnval ;	break ;
	case ENTITY_CSTD:		s = *s_enCStd ;		break ;

	case ENTITY_VARIABLE:	s = *s_enVarib ;	break ;
	case ENTITY_FUNCTION:	s = *s_enFunc ;		break ;
	case ENTITY_FN_OVLD:	s = *s_enFnOvld ;	break ;
	case ENTITY_FNPTR:		s = *s_enFnptr ;	break ;
	case ENTITY_CFUNC:		s = *s_enCFunc ;	break ;

	case ENTITY_TMPLARG:	s = *s_enTmplarg ;	break ;
	case ENTITY_MACRO:		s = *s_enMacro ;	break ;
	case ENTITY_DEFINE:		s = *s_enHashdef ;	break ;
	case ENTITY_LITERAL:	s = *s_enCLit ;		break ;
	case ENTITY_TYPEDEF:	s = *s_enTypdef ;	break ;
	default:
		s = s_null ;
	}

	return s ;
}

const char*	Filetype2Txt	(Filetype e)
{
	static	const char*	s_null = "invalid file type" ;
	static	const char*	_x[] =
	{
	"FILE_UNKNOWN",
	"FILE_HEADER",
	"FILE_SOURCE",
	""
	} ;

	if (e < FILE_UNKNOWN || e > FILE_SOURCE)
		return s_null ;
	return _x[e] ;
}

const char*	Scope2Txt	(Scope e)
{
	static	const char*	s_null = "invalid scope" ;

	static	const char*	_x[] =
	{
		"SCOPE_UNKNOWN",
		"SCOPE_GLOBAL",
		"SCOPE_STATIC_FILE",
		"SCOPE_FUNC",
		"SCOPE_PRIVATE",
		"SCOPE_PROTECTED",
		"SCOPE_PUBLIC",
		""
	} ;

	if (e < SCOPE_UNKNOWN || e > SCOPE_PUBLIC)
		return s_null ;
	return _x[e] ;
}

const char*	ceFrame::EntDesc	(void) const
{
	return Entity2Txt(Whatami()) ;
}

/*
**	Entity Init functions
*/

void	ceFrame::SetDesc	(const hzString& S)
{
	//	Replace  < with &lt;

	_hzfunc("ceFrame::SetDesc") ;

	hzChain		Z ;		//	Chain to process input string
	hzChain		F ;		//	Final chain
	chIter		zi ;	//	Chain iterator

	Z = S ;
	for (zi = Z ; *zi && !zi.eof() ;)
	{
		if (*zi == CHAR_LESS)
			F << "&lt;" ;
		else if (*zi == CHAR_MORE)
			F << "&gt;" ;
		else
			F.AddByte(*zi) ;
		zi++ ;
	}

	m_Desc = F ;
}

ceEntity::ceEntity	(void)
{
	//	Entity constructor. If declared in system file, suppress description.

	m_pNamsp = 0 ;

	if (g_Project.m_bSystemMask)
		m_Desc = g_sysDfltDesc ;

	m_Entattr = 0 ;
	m_Scope = SCOPE_UNKNOWN ;
	m_UEID = ++s_seq_ceEnt ;
}

ceNamsp::ceNamsp	(void)
{
	m_Scope = SCOPE_GLOBAL ;
	m_ET.InitET(this) ;
}

ceNamsp::~ceNamsp	(void)	{}

ceFngrp::ceFngrp	(void)
{
}

ceDefine::ceDefine    (void)
{
    m_pFileDef = 0 ;
    m_DefTokStart = 0 ;
    m_DefTokEnd = 0 ;
    m_bEval = false ;
    m_Scope = SCOPE_GLOBAL ;
}

ceMacro::ceMacro (void)
{
    m_pFileDef = 0 ;
    m_DefTokStart = 0 ;
    m_DefTokEnd = 0 ;
}

ceUnion::ceUnion	(void)
{
    m_pHost = 0 ;
    m_pFileDef = 0 ;
    m_DefTokStart = 0 ;
    m_DefTokEnd = 0 ;
    m_Scope = SCOPE_GLOBAL ;
    m_ET.InitET(this) ;
}

ceClass::ceClass (void)
{
    m_pBase = 0 ;
    m_ParKlass = 0 ;
    m_pFileDef = 0 ;
    m_DefTokStart = 0 ;
    m_DefTokEnd = 0 ;
    m_Scope = SCOPE_GLOBAL ;
    m_ET.InitET(this) ;
    m_Basis = ADT_CLASS ;
}


//	Entity destructors
ceFngrp::~ceFngrp	(void)	{}

hzEcode		ceDefine::InitHashdef	(ceFile* pFile, const hzString& name, uint32_t tokStart, uint32_t tokEnd)
{
	//	Initializes a #define instance. Note as this is only called in parse (with a file) we use only the otok array.

	_hzfunc("ceDefine::Init") ;

	hzChain		diag ;		//	Diagnostics chain
	ceToken		tok ;		//	Temp token
	uint32_t	pt ;		//	Current token in array P

	m_pFileDef = pFile ;
	m_DefTokStart = tokStart ;
	m_DefTokEnd = tokEnd ;

	SetName(name) ;
	m_fqname = name ;
	m_Scope = SCOPE_GLOBAL ;

	if (!pFile)
		return E_OK ;

	for (pt = tokStart + 2 ; pt <= tokEnd ; pt++)
	{
		tok = pFile->P[pt] ;
		tok.orig = 888888 ;
		m_Ersatz.Add(tok) ;
	}

	return E_OK ;
}

hzEcode		ceLiteral::InitLiteral	(ceFile* pFile, const hzString& name, const hzString& value)
{
	//	Initializes a #define instance. Note as this is only called in parse (with a file) we use only the otok array.

	_hzfunc("ceDefine::Init") ;

	hzChain		diag ;		//	Diagnostics chain
	ceToken		tok ;		//	Temp token

	m_pFileDef = pFile ;

	SetName(name) ;
	m_Str = value ;
	m_fqname = name ;
	m_Scope = SCOPE_GLOBAL ;

	return E_OK ;
}

hzEcode	ceMacro::InitMacro	(ceFile* pFile, const hzString& name, uint32_t tokStart, uint32_t tokEnd)
{
	_hzfunc("ceMacro::InitMacro") ;

	m_pFileDef = pFile ;
	m_DefTokStart = tokStart ;
	m_DefTokEnd = tokEnd ;
	if (pFile)
		slog.Out("%s - Init macro %s (file %s toks %d to %d)\n", __func__, *name, *pFile->StrName(), tokStart, tokEnd) ;
	else
		slog.Out("%s - Init macro %s (file NULL toks %d to %d)\n", __func__, *name, tokStart, tokEnd) ;
	SetName(name) ;
	m_Scope = SCOPE_GLOBAL ;
	return E_OK ;
}

hzEcode	ceClass::InitClass	(ceFile* pFile, ceClass* pBase, ceKlass* pParent, const hzString& name, Scope scope, bool bStruct)
{
	//	Initialize the ceClass instance and allocate a sequential pathname
	//
	//	Arguments:	1)	pFile	The occurent file.
	//				2)	pBase	The base class (if applicable).
	//				3)	pParent	Parent class (or class template). Only applies if this class is the subclass of another
	//				4)	name	The name of this class.
	//				5)	scope	Scope
	//				6)	bStruct	A boolean indicator set if this class is a struct.

	_hzfunc("ceClass::InitDefine") ;

	char	buf	[16] ;

	if (m_Name)
		Fatal("Class %s is already initialized\n", *m_Name) ;

	if (m_Docname)
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE) ;

	m_pFileDef = pFile ;
	m_pBase = pBase ;
	m_ParKlass = pParent ;
	m_Name = name ;
	m_Scope = scope ;

	if (pBase && m_ParKlass)
	{
		if (m_ParKlass->Whatami() == ENTITY_CLASS)
			Fatal("%s. Sub-class %s of class %s, cannot be a derivative\n", *_fn, *name, *m_ParKlass->StrName()) ;
	}

	if (bStruct)
		SetAttr(CL_ATTR_STRUCT) ;
	if (name[0] == CHAR_USCORE)
		SetAttr(EATTR_INTERNAL) ;

	if (m_ParKlass)
		m_fqname = m_ParKlass->Fqname() + "::" + name ;
	else
		m_fqname = name ;

	sprintf(buf, "cl%03d", ++s_seq_ceClass) ;
	m_Docname = buf ;
   	return E_OK ;
}

hzEcode ceFnset::InitFnset	(ceEntity* parent, const hzString& title)
{
	_hzfunc("ceFnset::InitFnset") ;

	char	buf	[16] ;

	sprintf(buf, "fs%04d", ++s_seq_ceFnset) ;
	m_Docname = buf ;
	m_Title = title ;

	slog.Out("NEW FUNC SET %s\n", *m_Title) ;
	return E_OK ;
}

hzEcode	ceUnion::InitUnion	(ceFile* pFile, ceEntity* pHost, const hzString& name)
{
	_hzfunc("ceUnion::InitDefine") ;

	char	buf	[16] ;

	m_pFileDef = pFile ;
	m_pHost = pHost ;
	SetName(name) ;

	sprintf(buf, "un%04d", ++s_seq_ceUnion) ;
	m_Docname = buf ;

	return E_OK ;
}

hzEcode	ceTypedef::InitTypedef	(ceFile* pFile, ceTyplex& tpx, const hzString& name)
{
	_hzfunc("ceTypedef::InitDefine") ;

	m_pFileDef = pFile ;
	m_Tpx = tpx ;
	//m_FdType = tpx.m_Datatype ;
	SetName(name) ;

	return E_OK ;
}

hzEcode	ceVar::InitAsArg	(ceTyplex& tpx, Scope range, const hzString& name)
{
	_hzfunc("ceVar::InitAsArg") ;

	m_pFileDcl = 0 ;
	m_ParKlass = 0 ;
	m_Tpx = tpx ;
	m_Scope = range ;
	m_Tpx.SetAttr(DT_ATTR_LVALUE) ;

	if (!m_Tpx.Type())
		{ slog.Out("%s. Invalid type for variable %s\n", *_fn, *name) ; return E_SYNTAX ; }

	if (m_Tpx.Basis() == ADT_NULL)
		{ slog.Out("%s. Variable %s has no hadronzoo type\n", *_fn, *name) ; return E_ARGUMENT ; }

	m_fqname = m_Name = name ;
	return E_OK ;
}

hzEcode	ceVar::InitDeclare	(ceFile* pFile, ceKlass* parent, ceTyplex& tpx, Scope range, const hzString& name)
{
	_hzfunc("ceVar::InitDeclare") ;

	if (!name)		Fatal("%s. Cannot initialize variable without a name\n", *_fn) ;
	if (!pFile)		Fatal("%s. Cannot initialize variable %s without a file\n", *_fn, *name) ;

	m_pFileDcl = pFile ;
	m_ParKlass = parent ;
	m_Tpx = tpx ;
	m_Scope = range ;
	m_Tpx.SetAttr(DT_ATTR_LVALUE) ;

	if (!m_Tpx.Type())
		{ slog.Out("%s. Invalid type for variable %s\n", *_fn, *name) ; return E_SYNTAX ; }

	if (m_Tpx.Basis() == ADT_NULL)
		{ slog.Out("%s. Variable %s has no hadronzoo type\n", *_fn, *name) ; return E_ARGUMENT ; }

	m_Name = name ;
	m_fqname = m_ParKlass ? m_ParKlass->Fqname() + "::" + m_Name : m_Name ;

	return E_OK ;
}

hzEcode	ceFunc::InitFuncDcl	(ceFile* pFile, ceKlass* parent, ceTyplex& retTpx, hzList<ceVar*>& funcArgs, Scope scope, const hzString& fnName)
{
	//	InitDeclare sets the 'declared in file' for the function as the supplied file pointer
	//
	//	Arguments:	1)	pFile	The applicable source file (in which the function is declared)
	//				2)	parent	The parent entity (this may be a class or a class template)
	//				3)	retTpx	The return datatype-complex
	//				4)	args	The argument list
	//				5)	scope	Applicable scope
	//				6)	fnName	Function basename (part after any :: but before (arglist))
	//
	//	Note this function set the function fullname which is nessesary for insertation into the entity table.

	_hzfunc("ceFunc::InitDeclare") ;

	hzList	<ceVar*>::Iter	ai ;	//	Argument iterator

	hzChain		Z ;					//	For building full name (including ret-type & args)
	ceVar*		pVar ;				//	Individual argument
	hzString	est_name ;			//	Calculated function name
	char		buf [16] ;			//	For article name

	if (!fnName)	Fatal("%s. Cannot initialize a function without a name\n", *_fn) ;
	if (!pFile)		Fatal("%s. Cannot initialize function %s without a file\n", *_fn, *fnName) ;

	if (m_pFileDcl)
		Fatal("%s. Cannot re-init (func %s)\n", *_fn, *fnName) ;

	if (parent)
	{
		if (parent->Whatami() != ENTITY_CLASS && parent->Whatami() != ENTITY_UNION)
		{
			slog.Out("%s. ERROR The parent of function (%s) must be either a class or class template\n", *_fn, *fnName) ;
			return E_SYNTAX ;
		}
	}

	m_pFileDcl = pFile ;
	m_ParKlass = parent ;

	m_Name = fnName ;
	m_fqname = m_ParKlass ? m_ParKlass->Fqname() + "::" + m_Name : m_Name ;

	if (fnName[0] == CHAR_USCORE)
		SetAttr(EATTR_INTERNAL) ;

	//	Copy the arguments to m_Args
	for (ai = funcArgs ; ai.Valid() ; ai++)
	{
		pVar = ai.Element() ;
		m_Args.Add(pVar) ;
	}

	//	Compute the function name to be name(arg1,...,argN)
	Z << fnName ;

	if (!funcArgs.Count())
		Z << "(void)" ;
	else
	{
		Z.AddByte(CHAR_PAROPEN) ;
		ai = funcArgs ;
		pVar = ai.Element() ;
		Z << pVar->Typlex().Str() ;

		for (ai++ ; ai.Valid() ; ai++)
		{
			Z.AddByte(CHAR_COMMA) ;
			pVar = ai.Element() ;
			Z << pVar->Typlex().Str() ;
		}
		Z.AddByte(CHAR_PARCLOSE) ;
	}
	m_exname = Z ;
	Z.Clear() ;

	m_Tpx = retTpx ;
	m_Scope = scope ;

	if (!m_Docname)
	{
		sprintf(buf, "fn%03d", ++s_seq_ceFunc) ;
		m_Docname = buf ;
	}

	return E_OK ;
}

hzEcode	ceFunc::InitFuncDef	(ceFile* pFile, uint32_t start, uint32_t end)
{
	//	Set the function's definition file and the start and end tokens for the body definition

	_hzfunc("ceFunc::InitDefine") ;

	if (m_pFileDef)
		Fatal("%s. Cannot re-init (func %s)\n", *_fn, *m_fqname) ;

	m_pFileDef = pFile ;
	m_DefTokStart = start ;
	m_DefTokEnd = end ;

	return E_OK ;
}

hzEcode	ceFunc::SetCategory	(const hzString& category)
{
	//	Set function category. This will fail if the group or set the function belongs to already has a conflicting category

	_hzfunc("ceFunc::SetCategory") ;

	if (m_pGrp)
		if (m_pGrp->m_Category && category != m_pGrp->m_Category)
			return E_CONFLICT ;

	if (m_pSet)
		if (m_pSet->m_Category && category != m_pSet->m_Category)
			return E_CONFLICT ;

	m_Category = category ;
	return E_OK ;
}

hzString	ceFunc::GetCategory	(void) const
{
	if (m_Category)
		return m_Category ;

	if (m_pGrp)
		if (m_pGrp->m_Category)
			return m_pGrp->m_Category ;
	if (m_pSet)
		if (m_pSet->m_Category)
			return m_pSet->m_Category ;

	return m_Category ;
}

void	ceSynp::SynopInit	(ceFile* pFile, const hzString& order, const hzString& name)	//, const hzString& desc)
{
	_hzfunc("ceSynp::Init") ;

	//	char	buf	[16] ;

	if (pFile)
		m_pParComp = pFile->ParComp() ;
	SetName(name) ;
	m_Docname = "sy" + order ;

	if (order.Contains(CHAR_PERIOD))
	{
		m_Parent = m_Docname ;
		m_Parent.Truncate(CstrLast(*m_Docname, CHAR_PERIOD)) ;
	}
	//	m_Id = ++s_seq_ceSynp ;
	//	sprintf(buf, "sy%03d", m_Id) ;
   	//	m_Docname = buf ;
}

hzEcode	ceEnumval::InitEnval	(ceEnum* parent, const hzString& name)
{
	_hzfunc("ceEnumval::InitEnval") ;

	m_ParEnum = parent ;
	m_Tpx.SetType(parent) ;
	SetName(name) ;

	return E_OK ;
}


hzEcode	ceEnum::InitEnum	(ceFile* pFile, const hzString& name, uint32_t start, uint32_t end, Scope scope)
{
	_hzfunc("ceEnum::InitDefine") ;

	char	buf	[16] ;

	m_pFileDef = pFile ;
	//m_Tpx.SetType(this) ;
	SetName(name) ;
	m_fqname = name ;
	m_DefTokStart = start ;
	m_DefTokEnd = end ;
	m_Scope = scope ;

	m_Id = ++s_seq_ceEnum ;
	sprintf(buf, "en%03d", m_Id) ;
   	m_Docname = buf ;

	return E_OK ;
}

/*
**	Entity table (ceEntbl) Functions
*/

hzEcode	ceEntbl::AddEntity	(ceFile* pFile, ceEntity* pE, const char* caller)
{
	//	Add an entity to the scope

	_hzfunc("ceEntbl::AddEntity") ;

	ceEntity*	pX ;		//	Existig entity
	ceKlass*	pKlass ;	//	Entity as class or union
	ceFunc*		pFunc ;		//	Entity as function
	ceReal*		pReal ;		//	Only if real

	//	Check operational basics - entity table is initialized and an entity has been supplied
	if (!applied)	hzexit(_fn, 0, E_NOINIT, "Attempt to operate on an unassigned scope (called by %s)\n", caller) ;
	if (!pE)		hzexit(_fn, 0, E_ARGUMENT, "No entity supplied (called by %s)", caller) ;

	if (pFile)
	{
		if (!pFile->ParComp())
			Fatal("%s. (%s) REJECT File %s has no parent component - entity %s\n", *_fn, caller, *pFile->StrName(), *pE->StrName()) ;
		//else
		//	pE->SetParComp(pFile->ParComp()) ;
	}

	//	Assume error chain is auxillary in the caller, so OK to clear before use
	g_EC[_hzlevel()].Clear() ;

	if (!pE->StrName())
	{
		g_EC[_hzlevel()].Printf("%s (%s). Supplied entity of type %s is unamed\n", *_fn, caller, pE->EntDesc()) ;
		return E_SYNTAX ;
	}
		g_EC[_hzlevel()].Printf("%s (%s). Supplied entity of type %s\n", *_fn, caller, pE->EntDesc()) ;
		g_EC[_hzlevel()].Printf("%s (%s). Supplied entity %s of type\n", *_fn, caller, *pE->StrName()) ;//, pE->EntDesc()) ;

	if (pE->Whatami() != CE_UNIT_FUNC_GRP)
	{
		//	As function sets are just placeholders and not real C++ entities, they don't have scope. Everything else must have scope.
		if (pE->GetScope() == SCOPE_UNKNOWN)
		{
			g_EC[_hzlevel()].Printf("%s (%s). Cannot add %s %s without an applied scope (range)\n", *_fn, caller, pE->EntDesc(), *pE->StrName()) ;
			return E_SYNTAX ;
		}
	}

	if (g_Project.m_bSystemMask)
		pE->SetAttr(EATTR_INTERNAL) ;

	pKlass = dynamic_cast<ceKlass*>(pE) ;
	if (pKlass)
		pKlass->m_ET.m_parent = this ;

	if (pE->Whatami() == ENTITY_FUNCTION)
	{
		pFunc = (ceFunc*) pE ;

		if (!pFunc->StrName())	{ g_EC[_hzlevel()].Printf("%s (%s). Cannot add an undeclared function (unnamed)\n", *_fn, caller) ; return E_SYNTAX ; }
		if (!pFunc->Extname())	{ g_EC[_hzlevel()].Printf("%s (%s). Cannot add an unprocessed function (no mangle)\n", *_fn, caller) ; return E_SYNTAX ; }

		//	Add the function under its full name
		pX = m_ents[pFunc->Extname()] ;
		if (pX)
		{
			if (pX->Whatami() == pE->Whatami())
			{
				g_EC[_hzlevel()].Printf("%s (%s). URGENT_WARNING: %s already defined (scope=%s)\n", *_fn, caller, *pFunc->StrName(), *applied->StrName()) ;
				slog.Out(g_EC[_hzlevel()]) ;
				return E_OK ;
			}

			//	Even more disasterous, type mis-match
			g_EC[_hzlevel()].Printf("%s (%s). CONFLICT: Case 1 %s already exists in scope (%s) as %s, cannot be reassigned to %s\n",
				*_fn, caller, *pFunc->StrName(), *applied->StrName(), pX->EntDesc(), pE->EntDesc()) ;
			return E_DUPLICATE ;
		}

		if (pFile)
			pE->SetParComp(pFile->ParComp()) ;
		m_ents.Insert(pFunc->Extname(), pE) ;
	}
	else
	{
		pX = m_ents[pE->StrName()] ;
		if (pX)
		{
			if (pX->Whatami() == pE->Whatami())
			{
				g_EC[_hzlevel()].Printf("%s (%s). URGENT_WARNING: %s already defined (scope=%s)\n", *_fn, caller, *pE->StrName(), *applied->StrName()) ;
				slog.Out(g_EC[_hzlevel()]) ;
				return E_OK ;
			}

			//	Even more disasterous, type mis-match
			g_EC[_hzlevel()].Printf("%s (%s). CONFLICT: case 2 %s already exists in scope (%s) as %s, cannot be reassigned to %s\n",
				*_fn, caller, *pE->StrName(), *applied->StrName(), pX->EntDesc(), pE->EntDesc()) ;
			return E_DUPLICATE ;
		}

		if (pFile)
			pE->SetParComp(pFile->ParComp()) ;
		m_ents.Insert(pE->StrName(), pE) ;
	}

	if (!pE->ParComp())
		slog.Out("%s. (%s) REJECT No parent component - entity %s\n", *_fn, caller, *pE->StrName()) ;

	pReal = dynamic_cast<ceReal*>(pE) ;

	if (pReal)
	{
		//g_EC[_hzlevel()].Printf(" - - real -- \n") ;
		g_EC[_hzlevel()].Printf("%s (%s) Ent-table %s accepts %s %s type %s (id %u)\n",
			*_fn, caller, *applied->StrName(), pE->EntDesc(), *pE->StrName(), *pReal->Typlex().Str(), pE->GetUEID()) ;
	}
	else
	{
		//g_EC[_hzlevel()].Printf(" - - not real -- \n") ;
		g_EC[_hzlevel()].Printf("%s (%s) Ent-table %s accepts %s %s (id %u)\n", *_fn, caller, *applied->StrName(), pE->EntDesc(), *pE->StrName(), pE->GetUEID()) ;
	}

	slog.Out(g_EC[_hzlevel()]) ;
	return E_OK ;
}

void	ceEntbl::EntityExport	(hzChain& Z, ceComp* pComp, uint32_t nRecurse)
{
	_hzfunc("ceEntbl::EntityExport") ;

	hzList	<ceVar*>::Iter	ai ;	//	Iterator for fuction args
	hzList	<ceFunc*>::Iter	fi ;	//	Iterator for functions

	ceEntity*		pE ;			//	Entity pointer
	ceVar*			pVar ;			//	Variable pointer
	ceEntbl*		pS_klas ;		//	Class entity table
	ceClass*		pClass ;		//	Class pointer
	ceFunc*			pFunc ;			//	Function pointer
	ceDefine*		pDef ;			//	Hash Define
	ceEnum*			pEnum ;			//	Enum pointer
	ceEnumval*		pEnumV ;		//	Enum value pointer
	const char*		i ;				//	Description iterator
	hzString		tmpStr ;		//	Temp string
	uint32_t		nE ;			//	Entity iterator
	uint32_t		nA ;			//	SUb entity/Func arg iterator
	uint32_t		nInd ;			//	Indentation counter
	uint32_t		indent ;		//	Indentation level
	uint32_t		inner ;			//	Inner indentation level
	uint32_t		nDefines ;		//	Number of hash defines
	uint32_t		nMacros ;		//	Number of macros
	uint32_t		nTypedefs ;		//	Number of typedefs
	uint32_t		nVars ;			//	Number of variables
	uint32_t		nEnums ;		//	Number of enums
	uint32_t		nUnions ;		//	Number of unions
	uint32_t		nClasses ;		//	Number of classes
	uint32_t		nFuncs ;		//	Number of functions

	if (!applied)
		Fatal("%s. Attempt to operate on unassigned scope\n", *_fn) ;
	
	indent = nRecurse + 1 ;
	inner = nRecurse + 2 ;

	/*
	**	Count numbers of each entity
	*/

	nDefines = nMacros = nTypedefs = nVars = nEnums = nUnions = nClasses = nFuncs = 0 ;

	for (nE = 0 ; nE < m_ents.Count() ; nE++)
	{
		pE = m_ents.GetObj(nE) ;

		switch	(pE->Whatami())
		{
		case ENTITY_DEFINE:		nDefines++ ;	break ;
		case ENTITY_MACRO:		nMacros++ ;		break ;
		case ENTITY_TYPEDEF:	nTypedefs++ ;	break ;
		case ENTITY_VARIABLE:	nVars++ ;		break ;
		case ENTITY_ENUM:		nEnums++ ;		break ;
		case ENTITY_UNION:		nUnions++ ;		break ;
		case ENTITY_CLASS:		nClasses++ ;	break ;
		case ENTITY_FUNCTION:	nFuncs++ ;		break ;
		}
	}

	for (nInd = 0 ; nInd < nRecurse ; nInd++)
		Z.AddByte(CHAR_TAB) ;
	if (!m_ents.Count())
	{
		Z.Printf("<EntityTable name=\"%s\">0 Items</EntityTable>\n", *applied->StrName()) ;
		return ;
	}

	Z.Printf("<EntityTable name=\"%s\">\n", *applied->StrName()) ;

	/*
	**	Do the #define set
	*/

	if (nDefines)
	{
		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "<HashDefines>\n" ;

		for (nE = 0 ; nE < m_ents.Count() ; nE++)
		{
			pE = m_ents.GetObj(nE) ;
			if (pE->Whatami() != ENTITY_DEFINE)
				continue ;

			pDef = (ceDefine*) pE ;
			for (nInd = 0 ; nInd < inner ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z.Printf("<hashdef name=\"%s\">", *pE->StrName()) ;
			for (nA = 0 ; nA < pDef->m_Ersatz.Count() ; nA++)
			{
				tmpStr = pDef->m_Ersatz[nA].value ;
				Z.Printf("%s ", *tmpStr) ;
			}
			Z << "</hashdef>\n" ;
		}

		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "</HashDefines>\n" ;
	}

	/*
	**	Do the macro set
	*/

	if (nMacros)
	{
		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "<Macros>\n" ;

		for (nE = 0 ; nE < m_ents.Count() ; nE++)
		{
			pE = m_ents.GetObj(nE) ;
			if (pE->Whatami() != ENTITY_MACRO)
				continue ;

			for (nInd = 0 ; nInd < inner ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z.Printf("<macro name=\"%s\"/>\n", *pE->StrName()) ;
		}

		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "</Macros>\n" ;
	}

	/*
	**	Do the typedefs
	*/

	if (nTypedefs)
	{
		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "<Typedefs>\n" ;

		for (nE = 0 ; nE < m_ents.Count() ; nE++)
		{
			pE = m_ents.GetObj(nE) ;
			if (pE->Whatami() != ENTITY_TYPEDEF)
				continue ;

			for (nInd = 0 ; nInd < inner ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z.Printf("<typedef name=\"%s\"/>\n", *pE->StrName()) ;
		}

		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "</Typedefs>\n" ;
	}

	/*
 	**	Do the variables
	*/

	if (nVars)
	{
		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "<Variables>\n" ;

		for (nE = 0 ; nE < m_ents.Count() ; nE++)
		{
			pE = m_ents.GetObj(nE) ;
			if (pE->Whatami() != ENTITY_VARIABLE)
				continue ;

			pVar = (ceVar*) pE ;
			for (nInd = 0 ; nInd < inner ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z.Printf("<var name=\"%s\" type=\"%s\"/>\n", *pE->StrName(), *pVar->Typlex().Str()) ;
		}

		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "</Variables>\n" ;
	}

	/*
 	**	Do the enums
	*/

	if (nEnums)
	{
		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "<Enums>\n" ;

		for (nE = 0 ; nE < m_ents.Count() ; nE++)
		{
			pE = m_ents.GetObj(nE) ;
			if (pE->Whatami() != ENTITY_ENUM)
				continue ;

			pEnum = (ceEnum*) pE ;

			for (nInd = 0 ; nInd < indent ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z.Printf("<enum name=\"%s\">\n", *pE->StrName()) ;

			indent++ ;
			for (nA = 0 ; nA < pEnum->m_ValuesByNum.Count() ; nA++)
			{
				pEnumV = pEnum->m_ValuesByNum.GetObj(nA) ;

				for (nInd = 0 ; nInd < indent ; nInd++)
					Z.AddByte(CHAR_TAB) ;
				Z.Printf("<eVal name=\"%s\" val=\"%d\" desc=\"%s\"/>\n", *pEnumV->StrName(), *pEnumV->m_txtVal, *pEnumV->StrDesc()) ;
			}
			indent-- ;

			for (nInd = 0 ; nInd < indent ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z << "</enum>\n" ;
		}

		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "</Enums>\n" ;
	}

	/*
	**	Do the unions
	*/

	if (nUnions)
	{
		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "<Unions>\n" ;

		for (nE = 0 ; nE < m_ents.Count() ; nE++)
		{
			pE = m_ents.GetObj(nE) ;
			if (pE->Whatami() != ENTITY_UNION)
				continue ;

			for (nInd = 0 ; nInd < inner ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z.Printf("<union name=\"%s\">\n", *pE->StrName()) ;
			Z << "</union>\n" ;
		}

		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "</Unions>\n" ;
	}

	/*
	**	Do the classes
	*/

	if (nClasses)
	{
		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "<Classes>\n" ;

		for (nE = 0 ; nE < m_ents.Count() ; nE++)
		{
			pE = m_ents.GetObj(nE) ;
			if (pE->Whatami() != ENTITY_CLASS)
				continue ;

			pClass = (ceClass*) pE ;
			pS_klas = &(pClass->m_ET) ;

			for (nInd = 0 ; nInd < inner ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			if (!pS_klas->m_ents.Count())
				Z.Printf("<class name=\"%s\" fqname=\"%s\"/>\n", *pClass->StrName(), *pClass->Fqname()) ;
			else
			{
				inner++ ;
				Z.Printf("<class name=\"%s\" fqname=\"%s\">\n", *pClass->StrName(), *pClass->Fqname()) ;

				if (pS_klas->m_ents.Count())
					pS_klas->EntityExport(Z, pComp, nRecurse+1) ;
				for (nInd = 0 ; nInd < inner ; nInd++)
					Z.AddByte(CHAR_TAB) ;
				Z << "</class>\n" ;
				inner-- ;
			}
		}

		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "</Classes>\n" ;
	}

	/*
	**	Do the functions
	*/

	if (nFuncs)
	{
		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "<Functions>\n" ;

		for (nE = 0 ; nE < m_ents.Count() ; nE++)
		{
			pE = m_ents.GetObj(nE) ;
			if (pE->Whatami() != ENTITY_FUNCTION)
				continue ;
			/*
			if (pE->ParComp() != pComp)
				continue ;
			*/

			pFunc = (ceFunc*) pE ;
			for (nInd = 0 ; nInd < indent ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z.Printf("<func name=\"%s\" fqname=\"%s\" page=\"%s\">\n", *pFunc->StrName(), *pFunc->Fqname(), *pFunc->Artname()) ;

			indent++ ;
			for (nInd = 0 ; nInd < indent ; nInd++)
				Z.AddByte(CHAR_TAB) ;

			if (!pFunc->StrDesc())
				Z << "<desc>No fngrp description</desc>\n" ;
			else if (pFunc->StrDesc().Length() < 120)
				Z.Printf("<desc>%s</desc>\n", *pFunc->StrDesc()) ;
			else
			{
				Z << "<fng:desc>\n" ;
				indent++ ;
				for (i = *pFunc->StrDesc() ; *i ; i++)
				{
					Z.AddByte(*i) ;
					if (*i != CHAR_NL)
						continue ;
					if (i[1])
						for (nInd = 0 ; nInd < indent ; nInd++)
							Z.AddByte(CHAR_TAB) ;
				}
				indent-- ;
				for (nInd = 0 ; nInd < indent ; nInd++)
					Z.AddByte(CHAR_TAB) ;
				Z << "</fng:desc>\n" ;
			}

			//	Do the functions
			for (nInd = 0 ; nInd < indent ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z.Printf("<func name=\"%s\" fqname=\"%s\">\n", *pFunc->Fqname(), *pFunc->Fqname()) ;

			if (pFunc->m_Args.Count())
			{
				for (ai = pFunc->m_Args ; ai.Valid() ; ai++)
				{
					pVar = ai.Element() ;

					for (nInd = 0 ; nInd < indent ; nInd++)
						Z.AddByte(CHAR_TAB) ;
					Z.Printf("\t<arg type=\"%s\" name=\"%s\" desc=\"%s\"/>\n", *pVar->Typlex().Str(), *pVar->StrName(), *pVar->StrDesc()) ;
				}
			}

			indent++ ;
			for (nInd = 0 ; nInd < indent ; nInd++)
				Z.AddByte(CHAR_TAB) ;

			if (!pFunc->StrDesc())
				Z << "<f:desc>No function description</f:desc>\n" ;
			else if (pFunc->StrDesc().Length() < 120)
				Z.Printf("<f:desc>%s</f:desc>\n", *pFunc->StrDesc()) ;
			else
			{
				Z << "<f:desc>\n" ;
				for (nInd = 0 ; nInd < indent ; nInd++)
					Z.AddByte(CHAR_TAB) ;
				indent++ ;
				for (i = *pFunc->StrDesc() ; *i ; i++)
				{
					Z.AddByte(*i) ;
					if (*i != CHAR_NL)
					continue ;
					if (i[1])
						for (nInd = 0 ; nInd < indent ; nInd++)
							Z.AddByte(CHAR_TAB) ;
				}
				indent-- ;
				for (nInd = 0 ; nInd < indent ; nInd++)
					Z.AddByte(CHAR_TAB) ;
				Z << "</f:desc>\n" ;
			}
			indent-- ;
			for (nInd = 0 ; nInd < indent ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z << "</func>\n" ;

			indent-- ;
			for (nInd = 0 ; nInd < indent ; nInd++)
				Z.AddByte(CHAR_TAB) ;
			Z << "</fngrp>\n" ;
			break ;
		}

		for (nInd = 0 ; nInd < indent ; nInd++)
			Z.AddByte(CHAR_TAB) ;
		Z << "</Functions>\n" ;
	}

	for (nInd = 0 ; nInd < nRecurse ; nInd++)
		Z.AddByte(CHAR_TAB) ;

	Z.Printf("</EntityTable>\n") ;
}

/*
**	Project Functions
*/

hzEcode	ceProject::Export	(void)
{
	//	Starting with the global scope, list all entities. Where entities are themselves scopes (Classes, functions and
	//	files), list these with indentation.

	_hzfunc("ceProject::Export") ;

	ofstream	os ;
	hzChain		Z ;

	os.open(*m_ExpFile) ;
	if (os.fail())
		return E_OPENFAIL ;

	if (!m_RootET.Count())
		slog.Out("Sorry - No entities in the global scope!\n") ;
	else
	{
		Z.Printf("<autodoc project=\"%s\">\n", *m_Name) ;

		slog.SetIndent(0) ;
		slog.Out("Listing Entity Table\n") ;
		m_RootET.EntityExport(Z, 0, 0) ;
		slog.Out("End of entity tables\n") ;

		Z.Printf("</autodoc>\n") ;
	}

	os << Z ;
	os.close() ;
	return E_OK ;
}

/*
**	Lookup functions
*/

ceEntity*	ceFile::LookupString	(ceKlass* pHost, const hzString& entname, const char* caller)
{
	//	This looks up an entity specified as a string rather than a series of tokens. The string will either be singular (refer directly to an
	//	entity) or composite (split by scoping operators). Only the scoping operator is allowed however. Other divisors such as . and -> are
	//	not applicable.
	//
	//	This is only used by the comment processors and is nessesary because comments, however large, give rise to a single token.

	_hzfunc("ceFile::LookupString") ;

	hzArray<hzString>	s_ar ;	//	Array of strings from spliting by ::
	hzVect<ceToken>		t_ar ;	//	Split string (by ::)

	ceToken		tok ;			//	For conversion to tokens
	ceEntity*	pE ;			//	Entity pointer, set if found
	hzString	S ;				//	Temp string
	uint32_t	x ;				//	Used to split string
	uint32_t	t_end ;			//	Needed for LookupToken

	if (!entname)
		Fatal("%s (%s). No search string supplied\n", *_fn, caller) ;

	//	Split up criteria
	s_ar.Clear() ;

	SplitCstrOnCstr(s_ar, *entname, "::") ;
	if (!s_ar.Count())
	{
		slog.Out("%s (%s). Split error on search string supplied\n", *_fn, caller) ;
		return 0 ;
	}

	//	Convert array of strings to array of tokens
	for (x = 0 ;;)
	{
		tok.value = s_ar[x] ;
		t_ar.Add(tok) ;

		tok.Clear() ;
		x++ ;
		if (x == s_ar.Count())
			break ;

		tok.type = TOK_OP_SCOPE ;
		t_ar.Add(tok) ;
	}

	//	Conduct search
	pE = LookupToken(t_ar, pHost, 0, t_end, 0, true, __func__) ;
	if (!pE)
		slog.Out("%s (%s). case 1: Host=%p, %s not found\n", *_fn, caller, pHost, *entname) ;
	return pE ;
}

ceEntity*	ceFile::LookupToken	(hzVect<ceToken>& T, ceKlass* pHost, ceEntbl* pFET, uint32_t& finish, uint32_t start, bool bSeries, const char* Caller)
{
	//	Locates the first available entity assumed to be refered to by the token(s) at the supplied starting point.
	//
	//	Special cases of multi-word C++ data types are investigated first, then special cases of operator functions. All other entities are a single word token. The multi-word C++
	//	data types are global const so no entity table is used (although these would be in the root entity table). The method for all other entities begins with the current entity
	//	table if supplied or the root otherwise. The method works back to the root entity table and returns the first match.
	//
	//	More sophisticated lookup functions call this function recursively to handle constructs that use the scoping and selection operators.
	//
	//	Arguments:	1) T		The token array.
	//				2) pHost	The starting point for the lookup
	//				3) pFET		This is the function entity table if applicable
	//				4) end		This will be placed one place beyond the token series constituting the discovered entity
	//				5) start	Start position of entity
	//				6) bSeries	Controls depth of search. 0 meand find first entity, 1 find topmost class, 2 find topmost entity
	//
	//	Returns:	Pointer to entity if found
	//				NULL if no entity

	_hzfunc("ceFile::LookupToken") ;

	ceEntbl*	pET = 0 ;			//	Entity table other than root
	ceEntity*	pE = 0 ;			//	Entity pointer, set if found
	ceEntity*	pFound = 0 ;		//	Entity pointer, set if found
	ceNamsp*	pNamsp = 0 ;		//	This would initially be set to cycle thrugh all currently applicable namespaces
	ceKlass*	pKlass = 0 ;		//	From host
	ceClass*	pClass = 0 ;		//	If host is a class
	hzString	entname ;			//	Token value, possible entity name
	uint32_t	ct ;				//	Current token
	uint32_t	n ;					//	Namespace iterator
	bool		bUnsigned = false ;	//	Only for std C++ entities

	/*
	**	Prepare for lookup in the standard C++ entity table. Because some of the types span multiple words these are prepared here
	*/

	ct = start ;
	finish = 0 ;

	//	Deal with the special case of the funndamental numeric C++ types
	if (T[ct].type & TOKTYPE_MASK_VTYPE)
	{
		if (T[ct].type == TOK_VTYPE_VOID)
			{ finish = ct + 1 ; return g_cpp_void ; }

		if (T[ct].type == TOK_VTYPE_UNSIGNED)
			{ bUnsigned = true ; ct++ ; }

		if		(T[ct].type == TOK_VTYPE_CHAR)	{ ct++ ; pFound = bUnsigned ? g_cpp_uchar : g_cpp_char ; }
		else if (T[ct].type == TOK_VTYPE_SHORT)	{ ct++ ; pFound = bUnsigned ? g_cpp_ushort : g_cpp_short ; }
		else if (T[ct].type == TOK_VTYPE_INT)	{ ct++ ; pFound = bUnsigned ? g_cpp_uint : g_cpp_int ; }

		else if (T[ct].type == TOK_VTYPE_LONG)
		{
			//	Could be long-long-int, long-int or just long
			if (T[ct+1].type == TOK_VTYPE_LONG && T[ct+2].type == TOK_VTYPE_INT)
               	{ ct += 3 ; pFound = bUnsigned ? g_cpp_ulonglong : g_cpp_longlong ; }
			else if (T[ct+1].type == TOK_VTYPE_INT)
               	{ ct += 2 ; pFound = bUnsigned ? g_cpp_ulong : g_cpp_long ; }
			else
               	{ ct++ ; pFound = bUnsigned ? g_cpp_ulong : g_cpp_long ; }
		}
		else
		{
			if (bUnsigned)
           		slog.Out("%s %s Line %d (from %s): WARNING Keyword 'unsigned' can only preceed char, short, int or long\n", *_fn, *StrName(), T[ct].line, Caller) ;
           	return 0 ;
		}

		finish = ct ;
		return pFound ;
	}

	//	Everything below here should be a word - establish entity name
	if (T[ct].value == "operator")	// && T[ct+1].IsOperator())
	{
		//	Deal with the special case of functions whose name begins with 'operator'
		if (T[ct+1].type == TOK_SQ_OPEN || T[ct+2].type == TOK_SQ_CLOSE)
			{ entname = "operator[]" ; ct += 3 ; }
		else
		{
			if (T[ct+1].IsOperator())
			{
				entname = T[ct].value + T[ct+1].value ;
				ct += 2 ;
			}
		}
	}

	if (!entname)
	{
		entname = T[ct].value ;
		ct++ ;
	}

	if (!entname)
		{ slog.Out("%s %s Line %d col %d (from %s): Null token\n", *_fn, *StrName(), T[ct].line, T[ct].col, Caller) ; return 0 ; }
	finish = ct ;

	//	If a guest ET supplied (that of a function), this takes precedence
	if (pFET)
	{
		pFound = pFET->GetEntity(entname) ;
		for (; !pFound && pFET->Parent() ;)
		{
			pFET = pFET->Parent() ;
			pFound = pFET->GetEntity(entname) ;
		}
	}

	//	If there are entries in the source file entity table
	if (!pFound && m_pET)
		pFound = m_pET->GetEntity(entname) ;

	//	Now look in the entity table of the host class or class template if supplied. If the entity is found here return it as the supplied host class
	//	or class template takes precedence.
	if (!pFound && pHost)
	{
		pKlass = pHost ;
		pET = &(pKlass->m_ET) ;
		for (; pET ;)
		{
			//	slog.Out("%s (%d) %s Line %d col %d (from %s): lookupA %s with host as %s\n",
			//		__func__, __LINE__, *StrName(), T[ct].line, T[ct].col, Caller, *entname, pET->GetName()) ;

			if (entname == pET->StrName())
				pFound = (ceEntity*) pET->GetApplied() ;
			else
				pFound = pET->GetEntity(entname) ;

			if (pFound)
				break ;

			//	Look for parent class or class template or if none, then base
			pClass = dynamic_cast<ceClass*>(pKlass) ;

			if (pClass)
			{
				if (pClass->ParKlass())
					pKlass = pClass->ParKlass() ;
				else if (pClass->Base())
					pKlass = pClass->Base() ;
				else
					pKlass = 0 ;
			}
					
			if (pKlass)
				pET = &(pKlass->m_ET) ;
			else
				pET = pET->Parent() ;
		}
	}

	//	Then look in the namespace if supplied
	if (!pFound && g_Project.m_Using.Count())
	{
		for (n = 0 ; !pFound && n < g_Project.m_Using.Count() ; n++)
		{
			if (!pFound)
			{
				pNamsp = g_Project.m_Using.GetObj(n) ;
				//slog.Out("%s %s Line %d col %d: lookupC %s with namsp as %s and host as %s\n",
				//	__func__, *StrName(), T[ct].line, T[ct].col, *entname, *pNamsp->StrName(), pHost ? *pHost->StrName() : "null") ;
				pE = pFound = pNamsp->m_ET.GetEntity(entname) ;
			}
		}
	}

	//	Then look in the root
	if (!pFound)
		pFound = g_Project.m_RootET.GetEntity(entname) ;

	if (pFound && bSeries)
	{
		pE = pFound ;

		for (; T[ct].type == TOK_OP_SCOPE ;)
		{
			if		(pE->Whatami() == ENTITY_NAMSP)	{ pNamsp = (ceNamsp*) pE ; pET = &(pNamsp->m_ET) ; }
			else if (pE->Whatami() == ENTITY_CLASS)	{ pClass = (ceClass*) pE ; pET = &(pClass->m_ET) ; }
			else
				break ;

			entname = T[ct+1].value ;
			if (!entname)
				{ slog.Out("%s (%d) %s Line %d col %d (from %s): Null token\n", __func__, __LINE__, *StrName(), T[ct].line, T[ct].col, Caller) ; return 0 ; }

			pE = pET->GetEntity(entname) ;
			if (!pE)
				break ;

			ct += 2 ;
			finish = ct ;
			pFound = pE ;
		}
	}

	return pFound ;
}

ceClass*	ceFile::GetClass	(uint32_t& ultimate, uint32_t start)
{
	//	Determine if the current token is a class or is part of a construct that is a class, e.g. namespace::class.
	//	draws a blank, this function gives up and returns NULL.
	//
	//	Arguments:	1)	ultimate	Reference to final token position
	//				2)	start		Position of current token
	//
	//	Returns:	Pointer to class if found
	//				NULL otherwise

	_hzfunc("ceFile::GetClass") ;

	ceEntity*	pE = 0 ;		//	Test Entity
	ceClass*	pFound = 0 ;	//	Last entity that was a class
	ceNamsp*	pNamsp = 0 ;	//	Namespace
	uint32_t	ct ;			//	Current token

	ct = ultimate = start ;

	//	The first word token is sought in the root entity table
	pE = g_Project.m_RootET.GetEntity(X[ct].value) ;
	if (!pE)
		return 0 ;

	//	If an entity is found and it is a namespace and there is a :: operator, lookup the next word in the namespace ET
	for (; pE && pE->Whatami() == ENTITY_NAMSP && X[ct+1].type == TOK_OP_SCOPE ;)
	{
		pNamsp = (ceNamsp*) pE ;
		ct += 2 ;
		pE = pNamsp->m_ET.GetEntity(X[ct].value) ;
	}

	//	If an entity is found and it is a class and there is an :: operator, mark the class and lookup the next word in the class ET
	for (; pE && pE->Whatami() == ENTITY_CLASS && X[ct+1].type == TOK_OP_SCOPE ;)
	{
		pFound = (ceClass*) pE ;
		ultimate = ct ;
		ct += 2 ;
		pE = pFound->m_ET.GetEntity(X[ct].value) ;
	}

	//	Return the marked class
	return pFound ;
}
