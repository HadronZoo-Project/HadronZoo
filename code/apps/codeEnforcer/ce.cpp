//
//	File:	ce.cpp
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

//	CodeEnforcer main.

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
//#include "hzDictionary.h"
#include "hzProcess.h"
#include "hzDissemino.h"

#include "enforcer.h"

using namespace std ;

/*
**	Variables
*/

bool	_hzGlobal_XM = false ;		//	Switch on global new/delete overload
bool	_hzGlobal_MT = false ;		//	Run as single threaded

hzProcess			proc ;			//	Process instance (all HadronZoo apps need this)
hzLogger			slog ;			//	Logfile
ceProject			g_Project ;		//	Project (current)

hzSet<hzString>		g_allStrings ;

global	hzIpServer*	g_pTheServer ;	//	The server instance for HTTP
global	hdsApp*		theApp ;		//	Dissemino HTML interface
global	hdsTree		g_treeNav ;		//	Nav-tree of Items of Interest
global	hdsTree		g_treeEnts ;	//	Nav-tree of Items of Interest
global	hzChain		g_EC[50] ;		//	Function call level error reporting

//	Standard types
ceCStd*		g_cpp_void ;			//	void
ceCStd* 	g_cpp_bool ;			//	bool
ceCStd* 	g_cpp_char ;			//	char
ceCStd* 	g_cpp_uchar ;			//	uchar
ceCStd* 	g_cpp_short ;			//	int16_t
ceCStd* 	g_cpp_ushort ;			//	uint16_t
ceCStd* 	g_cpp_int ;				//	int32_t
ceCStd* 	g_cpp_uint ;			//	uint32_t
ceCStd* 	g_cpp_long ;			//	int32_t/64
ceCStd* 	g_cpp_ulong ;			//	uint32_t/64
ceCStd* 	g_cpp_longlong ;		//	int64_t
ceCStd* 	g_cpp_ulonglong ;		//	uint64_t
ceCStd* 	g_cpp_float ;			//	float
ceCStd* 	g_cpp_double ;			//	double

bool		g_bDebug ;				//	Extra debugging (-b option)
bool		g_bLive ;				//	Present findings in live webpage (-l option)

hzString	g_CSS ;					//	CSS filename
hzString	g_sysDfltDesc = "System entity: No description available" ;

extern	hzString	_dsmScript_tog ;
extern	hzString	_dsmScript_loadArticle ;
extern	hzString	_dsmScript_navtree ;

/*
**	Prototypes
*/

hzEcode	PreLoadTypes	(void)
{
	//	Pre-load Standard C++ types.

	_hzfunc(__func__) ;

	hzQue<hzString>	strings_q ;
	hzStack<hzString>	strings_s ;

	ceTyplex	tpx ;			//	Typlex of variables
	ceCStd*		pCStd ;			//	Pointer to C standard instance
	ceCpFn*		pCpFn ;			//	Pointer to C standard instance
	ceDefine*	pDef ;			//	Special #define (eg UNIX)
	ceEntity*	pEnt ;			//	Entity pointer
	hzString	v ;				//	Name of system entity
	uint32_t	n ;				//	Entity iterator (diagnostics)
	hzEcode		rc = E_OK ;		//	Return code

	slog.Out("PRELOADING TYPES ...\n") ;

	//	The global scope
	g_Project.m_RootET.InitET((ceFrame*) &g_Project) ;

	//	Fundamental types
	v = "void" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_VOID) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_void = pCStd ;
	strings_q.Push(v) ;
	strings_s.Push(v) ;
	v = "bool" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_BOOL) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_bool = pCStd ;
	v = "char" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_BYTE) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_char = pCStd ;
	v = "unsigned char" ;	pCStd = new ceCStd() ; pCStd->Init(v, ADT_UBYTE) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_uchar = pCStd ;
	v = "short" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_INT16) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_short = pCStd ;
	v = "unsigned short" ;	pCStd = new ceCStd() ; pCStd->Init(v, ADT_UNT16) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_ushort = pCStd ;
	v = "int" ;				pCStd = new ceCStd() ; pCStd->Init(v, ADT_INT32) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_int = pCStd ;
	v = "unsigned int" ;	pCStd = new ceCStd() ; pCStd->Init(v, ADT_UNT32) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_uint = pCStd ;
	v = "long" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_INT32) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_long = pCStd ;
	v = "unsigned long" ;	pCStd = new ceCStd() ; pCStd->Init(v, ADT_UNT32) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_ulong = pCStd ;
	v = "long long int" ;	pCStd = new ceCStd() ; pCStd->Init(v, ADT_INT64) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_longlong = pCStd ;
	v = "unsigned long long int" ;
							pCStd = new ceCStd() ; pCStd->Init(v, ADT_UNT64) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_ulonglong = pCStd ;
	v = "float" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_DOUBLE) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_float = pCStd ;
	v = "double" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_DOUBLE) ; g_Project.m_RootET.AddEntity(0, pCStd) ; g_cpp_double = pCStd ;

	//	Common types
	//	v = "SSL" ;				pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	//	v = "SSL_CTX" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	//	v = "SSL_METHOD" ;		pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	//	v = "X509" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	//	v = "X509_STORE_CTX" ;	pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;

	//v = "FILE" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	//v = "stat" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	v = "pthread_mutex_t" ;	pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	v = "pthread_cond_t" ;	pCStd = new ceCStd() ; pCStd->Init(v, ADT_CLASS) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	v = "va_list" ;			pCStd = new ceCStd() ; pCStd->Init(v, ADT_VARG) ; g_Project.m_RootET.AddEntity(0, pCStd) ;
	//v = "va_arg" ;			pCStd = new cceStd() ; pCStd->Init(v, ADT_VARG) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "va_start" ;		pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "va_arg" ;			pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "va_end" ;			pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "va_copy" ;			pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "_vainto" ;			pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;

	//	Zip stuff
	v = "deflateInit2" ;	pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "deflate" ;			pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "deflateEnd" ;		pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "inflateInit2" ;	pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "inflate" ;			pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;
	v = "inflateEnd" ;		pCpFn = new ceCpFn() ; pCpFn->InitCpFn(tpx, v) ; g_Project.m_RootET.AddEntity(0, pCpFn) ;

	//	Compile for which operating system
	v = "UNIX" ;	pDef = new ceDefine() ; pDef->InitHashdef(0, v, 0, 0) ; g_Project.m_RootET.AddEntity(0, pDef) ;

	slog.Out("PRELOADED TYPES ...\n") ;
	for (n = 0 ; n < g_Project.m_RootET.Count() ; n++)
	{
		pEnt = g_Project.m_RootET.GetEntity(n) ;
		slog.Out("\t%d\t%s\t%s\n", n, Entity2Txt(pEnt->Whatami()), *pEnt->StrName()) ;
	}
	slog.Out("PRELOADED TYPES ...\n\n") ;

	return rc ;
}

/*
**	ceComp members
*/

hzEcode	ceComp::ReadView	(hzXmlNode* pN)
{
	//	Reads the <view> tag and subtags

	_hzfunc("ceComp::ReadView") ;

	hzXmlNode*	pN1 ;			//	Level nodes
	hzAttrset	ai ;			//	Attribute iterator
	hzString	name1 ;			//	Node name
	uint32_t	nIndex ;		//	General ierator
	hzEcode		rc = E_OK ;

	if (!pN)
		Fatal("%s. No XML node supplied\n", *_fn) ;
	if (!pN->NameEQ("view"))
		Fatal("%s. Wrong call on <%s>\n", *_fn, pN->TxtName()) ;

	nIndex = 0 ;

	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		name1 = pN1->TxtName() ;

		ai = pN1 ;
		if (!ai.Valid())
			{ rc = E_SYNTAX ; slog.Out("%s. Line %d - All sub-tags of <view> require an attribute of name\n", *_fn, pN1->Line()) ; break ; }

		slog.Out("%s. item %s value %s posn %d\n", *_fn, *name1, ai.Value(), nIndex) ;

		if		(name1 == "item_header")	{ m_View.item_header = ai.Value() ; m_View.m_Items[nIndex++] = 1 ; }
		else if (name1 == "item_source")	{ m_View.item_source = ai.Value() ; m_View.m_Items[nIndex++] = 2 ; }
		else if (name1 == "item_ctmpl")		{ m_View.item_ctmpl  = ai.Value() ; m_View.m_Items[nIndex++] = 3 ; }
		else if (name1 == "item_class")		{ m_View.item_class  = ai.Value() ; m_View.m_Items[nIndex++] = 4 ; }
		else if (name1 == "item_enums")		{ m_View.item_enums  = ai.Value() ; m_View.m_Items[nIndex++] = 5 ; }
		else if (name1 == "item_funcs")		{ m_View.item_funcs  = ai.Value() ; m_View.m_Items[nIndex++] = 6 ; }
		else if (name1 == "item_progs")		{ m_View.item_progs  = ai.Value() ; m_View.m_Items[nIndex++] = 7 ; }
		else if (name1 == "item_notes")		{ m_View.item_notes  = ai.Value() ; m_View.m_Items[nIndex++] = 8 ; }
		else
		{
			slog.Out("%s. Line %d Illegal subtag <%s> of <view>. Only <item_header|source|ctmpl|class|enums|funcs|notes> allowed\n",
				*_fn, pN1->Line(), pN1->TxtName()) ;
			rc = E_SYNTAX ;
			break ;
		}

		if (nIndex > 8)
			Fatal("%s. More than 8 items in component's view\n", *_fn) ;
	}

	return rc ;
}

hzEcode	ceComp::ReadFileLists	(hzXmlNode* pN)
{
	//	Add a source, header or document file to the project. The project indexes files by name and by path.

	_hzfunc("ceComp::ReadFileLists") ;

	hzVect<hzDirent>	files ;		//	Files found in directory (of header or source files)

	hzDirent	de ;				//	Directory entry
	FSTAT		fs ;				//	For testing files
	hzXmlNode*	pN1 ;				//	Level nodes
	ceFile*		pFile ;				//	Header or source file
	hzAttrset	ai ;				//	Attribute iterator
	hzString	anam ;				//	Attribute name
	hzString	aval ;				//	Attribute value
	hzString	name1 ;				//	Node name
	hzString	currPath ;			//	Directory specified by tag parameter 'dir='
	hzString	criteria ;			//	Criteria for selecting files from currPath
	hzString	filepath ;			//	Filepath of file to be added to project
	uint32_t	nIndex ;			//	General ierator
	uint32_t	filetype ;			//	True if node name is 'headers', flase otherwise
	hzEcode		rc = E_OK ;

	if (!pN)
		Fatal("%s. No XML node supplied\n", *_fn) ;

	if		(pN->NameEQ("hdrs"))	filetype = 1 ;
	else if (pN->NameEQ("srcs"))	filetype = 2 ;
	else if (pN->NameEQ("docs"))	filetype = 3 ;
	else
	{
		slog.Out("%s. Called with tag <%s>, only <hdrs|srcs|docs> acceptable here\n", *_fn, pN->TxtName()) ;
		return E_SYNTAX ;
	}

	ai = pN ;
	if (ai.Valid())
	{
		anam = ai.Name() ;
		aval = ai.Value() ;

		if (!aval)
			{ slog.Out("%s. Line %d <file> has param %s of null value - not meaningful\n", *_fn, pN->Line(), *anam) ; return E_SYNTAX ; }

		if (anam != "dir")
			{ slog.Out("%s. The first param of <%s> must be 'dir'. %s not accepted\n", *_fn, pN->TxtName(), *anam) ; return E_SYNTAX ; }

		GetAbsPath(currPath, aval) ;

		ai.Advance() ;
		if (ai.Valid())
		{
			anam = ai.Name() ;
			aval = ai.Value() ;

			if (!aval)
				{ slog.Out("%s. Line %d <file> has param %s of null value - not meaningful\n", *_fn, pN->Line(), *anam) ; return E_SYNTAX ; }

			if (anam != "files")
				{ slog.Out("%s. The 2nd param of <%s> must be 'files'. %s not accepted\n", *_fn, pN->TxtName(), *anam) ; return E_SYNTAX ; }

			criteria = aval ;
		}
	}

	if (currPath && criteria)
	{
		//	Read currPath to find files matching criteria and add these files to the headers or sources for the component. Note that components
		//	can share header files and document files but not source files.

		rc = ReadDir(files, currPath, criteria) ;
		if (rc != E_OK)
		{
			slog.Out("%s. Failure while reading Dir %s with criteria %s (err=%s)\n", *_fn, *currPath, *criteria, Err2Txt(rc)) ;
			return rc ;
		}

		slog.Out("%s. Reading Dir %s with crit %s finds %d files\n", *_fn, *currPath, *criteria, files.Count()) ;

		for (nIndex = 0 ; nIndex < files.Count() ; nIndex++)
		{
			de = files[nIndex] ;
			filepath = currPath + "/" + de.Name() ;

			//	Get or create ceFile instance
			if (filetype == 1)	pFile = g_Project.m_HeadersByPath[filepath] ;
			if (filetype == 2)	pFile = g_Project.m_SourcesByPath[filepath] ;
			if (filetype == 3)	pFile = g_Project.m_Documents[filepath] ;

			if (pFile)
				slog.Out("%s. Using FILE of %s with component=%s\n", *_fn, *pFile->StrName(), *pFile->ParComp()->m_Name) ;
			else
			{
				pFile = new ceFile() ;

				rc = pFile->Init(this, filepath) ;
				if (rc != E_OK)
					{ slog.Out("%s. Could not init file to %s\n", *_fn, *filepath) ; return E_SYNTAX ; }

				if (filetype == 1)
				{
					if (!g_Project.m_HeadersByPath.Exists(pFile->Path()))
					{
						g_Project.m_HeadersByPath.Insert(pFile->Path(), pFile) ;
						g_Project.m_HeadersByName.Insert(pFile->StrName(), pFile) ;
					}

					m_Headers.Insert(pFile->StrName(), pFile) ;
				}

				if (filetype == 2)
				{
					if (!g_Project.m_SourcesByPath.Exists(pFile->Path()))
					{
						g_Project.m_SourcesByPath.Insert(pFile->Path(), pFile) ;
						g_Project.m_SourcesByName.Insert(pFile->StrName(), pFile) ;
					}

					m_Sources.Insert(pFile->StrName(), pFile) ;
				}

				if (filetype == 3)
				{
					if (!g_Project.m_Documents.Exists(pFile->Path()))
						g_Project.m_Documents.Insert(pFile->Path(), pFile) ;
					m_Documents.Insert(pFile->Path(), pFile) ;
				}
			}
		}
	}

	//	Add the system definitions file $HADRONZOO/data/CodeEnforcer.sys
	slog.Out("Adding path to %s\n", *_hzGlobal_HADRONZOO) ;
	filepath = _hzGlobal_HADRONZOO + "/data/CodeEnforcer.sys" ;
	slog.Out("Now have filepath of %s\n", *filepath) ;

	pFile = new ceFile() ;
	rc = pFile->Init(this, filepath) ;
	if (!g_Project.m_SysInclByPath.Exists(pFile->Path()))
	{
		g_Project.m_SysInclByPath.Insert(pFile->Path(), pFile) ;
		g_Project.m_SysInclByName.Insert(pFile->StrName(), pFile) ;
	}
	m_SysIncl.Insert(pFile->StrName(), pFile) ;

	//	If other files are needed get them here
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("main"))
		{
			//	For this to be valid the componet must be type=program, the filetype flag must be unset and the component must not yet
			//	have m_pMain set

			if (m_cfgType != "Program")
				Fatal("%s. 'main' is not an allowed tag except in <program>\n", *_fn) ;
			if (m_pMain)
				Fatal("%s. The Program component %s already has a main file of %s\n", *_fn, *m_Name, *m_pMain->StrName()) ;
		}
		else
		{
			if (pN1->NameEQ("file"))
				Fatal("%s. Line %d tag <%s> is misplaced. Only <file> tags allowed\n", *_fn, pN1->Line(), pN1->TxtName()) ;
		}

		ai = pN1 ;
		anam = ai.Name() ;
		aval = ai.Value() ;

		if (!aval)
			Fatal("%s. Line %d tag <%s> has param %s of null value - not meaningful\n", *_fn, pN1->Line(), pN1->TxtName(), ai.Name()) ;

		//	A param of path is used where the directory has not been specified in the <header> or <sources> tag or where
		//	it differs from that specified. If we are using the specified directory the param name should be 'name'.

		if (anam == "path")
			filepath = aval ;
		else if (anam == "name")
			filepath = currPath + "/" + aval ;
		else
		{
			slog.Out("%s. Line %d The <%s> tags must have a param of 'path' or 'name'\n", *_fn, pN1->Line(), pN1->TxtName()) ;
			return E_SYNTAX ;
		}

		if (stat(*filepath, &fs) == -1)
		{
			slog.Out("%s. Cannot stat %s\n", *_fn, *filepath) ;
			return E_NOTFOUND ;
		}

		//	Check if file exists
		if (filetype == 1)
			pFile = g_Project.m_HeadersByPath[filepath] ;
		if (filetype == 2)
			pFile = g_Project.m_SourcesByPath[filepath] ;

		if (pFile)
			slog.Out("%s. case 2 Using FILE of %s with component=%s\n", *_fn, *pFile->StrName(), *pFile->ParComp()->m_Name) ;
		else
		{
			pFile = new ceFile() ;

			rc = pFile->Init(this, filepath) ;
			if (rc != E_OK)
				{ slog.Out("%s. Could not init file to %s\n", *_fn, *filepath) ; return E_SYNTAX ; }

			if (filetype == 1)
			{
				if (!g_Project.m_HeadersByPath.Exists(pFile->Path()))
				{
					g_Project.m_HeadersByPath.Insert(pFile->Path(), pFile) ;
					g_Project.m_HeadersByName.Insert(pFile->StrName(), pFile) ;
				}
				m_Headers.Insert(pFile->StrName(), pFile) ;
			}

			if (filetype == 2)
			{
				if (!g_Project.m_SourcesByPath.Exists(pFile->Path()))
				{
					g_Project.m_SourcesByPath.Insert(pFile->Path(), pFile) ;
					g_Project.m_SourcesByName.Insert(pFile->StrName(), pFile) ;
				}
				m_Sources.Insert(pFile->StrName(), pFile) ;
			}

			if (filetype == 3)
			{
				if (!g_Project.m_Documents.Exists(pFile->Path()))
					g_Project.m_Documents.Insert(pFile->Path(), pFile) ;
				m_Documents.Insert(pFile->StrName(), pFile) ;
			}

			if (filetype == 4)
			{
				if (!g_Project.m_SysInclByPath.Exists(pFile->Path()))
				{
					g_Project.m_SysInclByPath.Insert(pFile->Path(), pFile) ;
					g_Project.m_SysInclByName.Insert(pFile->StrName(), pFile) ;
				}
				m_SysIncl.Insert(pFile->StrName(), pFile) ;
			}
		}
	}

	if (m_cfgType == "Program")
		m_pMain = pFile ;

	return E_OK ;
}

hzEcode	ceComp::ReadConfig	(hzXmlNode* pN)
{
	//	Read the CodeEnforcer project configs.

	_hzfunc("ceComp::ReadConfig") ;

	hzXmlNode*	pN1 ;			//	Level nodes
	hzAttrset	ai ;			//	First node attribute
	hzString	anam ;			//	Attribute name
	hzString	aval ;			//	Attribute value
	hzString	name1 ;			//	Node name
	hzString	name2 ;			//	Node name
	hzString	cont1 ;			//	Node content
	hzString	currPath ;		//	Directory specified by tag parameter 'dir='
	hzString	criteria ;		//	Criteria for selecting files from currPath
	hzString	filepath ;		//	Filepath of file to be added to project
	hzEcode		rc = E_OK ;

	if (!pN)
		Fatal("%s. No XML node supplied\n", *_fn) ;

	if		(pN->NameEQ("library"))	m_cfgType = "Library" ;
	else if (pN->NameEQ("program"))	m_cfgType = "Program" ;
	else
		Fatal("%s. Line %d Called with tag <%s>, only <library|program> acceptable here\n", *_fn, pN->Line(), pN->TxtName()) ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		anam = ai.Name() ;
		aval = ai.Value() ;

		slog.Out("%s. param name=%s, value=%s\n", *_fn, *anam, *aval) ;

		if		(anam == "name")		m_Name = aval ;
		else if (anam == "title")		m_Title = aval ;
		else if (anam == "copyright")	m_Copyright = aval ;
		else
		{
			slog.Out("%s. Line %d Illegal parameter of <%s> (%s=%s) Only type|name|title allowed here\n", *_fn, pN->Line(), *m_cfgType, *anam, *aval) ;
			rc = E_SYNTAX ;
			break ;
		}
	}

	if (!m_Name)	{ slog.Out("%s. Line %d Component must have a name\n", *_fn, pN->Line()) ; rc = E_SYNTAX ; }
	if (!m_Title)	{ slog.Out("%s. Line %d Component must have a title\n", *_fn, pN->Line()) ; rc = E_SYNTAX ; }

	if (g_Project.m_AllComponents.Exists(m_Name))
		Fatal("%s. Line %d Already have a component called %s - sorry\n", *_fn, *m_Name, pN->Line()) ;
	g_Project.m_AllComponents.Insert(m_Name, this) ;

	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		//	Deal with <header> and <source> tags
		name1 = pN1->TxtName() ;
		cont1 = pN1->m_fixContent ;

		slog.Out("%s. Reading Level 1 Tag %s, content=%s\n", *_fn, *name1, *cont1) ;

		if		(name1 == "copyright")	m_Copyright = cont1 ;
		else if (name1 == "desc")		m_Desc = cont1 ;
		else if (name1 == "docs")		rc = ReadFileLists(pN1) ;
		else if (name1 == "hdrs")		rc = ReadFileLists(pN1) ;
		else if (name1 == "srcs")		rc = ReadFileLists(pN1) ;
		else if (name1 == "view")		rc = ReadView(pN1) ;

		else
		{
			slog.Out("File %s line %d. Illegal tag <%s>. Only <copyright|desc|docs|hdrs|srcs|view> allowed\n",
				*g_Project.m_CfgFile, pN1->Line(), pN1->TxtName()) ;
			rc = E_SYNTAX ;
		}
	}

	if (rc == E_OK && m_Desc)
	{
		//	m_Docname = "lib." + m_Name + ".html" ;
		m_Docname = m_Name ;
		//m_Docname.FnameEncode() ;
	}

	return rc ;
}

hzEcode	ceProject::Init	(hzString cfgfile)
{
	//	Initiialize Project - Read config files.

	_hzfunc("ceProject::Init") ;

	hzDocXml	X ;				//	XML document of config file
	hzXmlNode*	pRoot ;			//	Root node (<project> tag)
	hzXmlNode*	pN1 ;			//	Level 1 nodes
	hzAttrset	ai ;			//	Attribute iterator
	ceComp*		pComp ;			//	Project component
	hzChain		desc ;			//	For building project description
	hzString	val ;			//	Gen purpose string
	hzEcode		rc = E_OK ;		//	Return code

	slog.Out("%s: INITIALIZING PROJECT using file %s\n", *_fn, *cfgfile) ;

	//	Set up name and paths
	if (!cfgfile)
		{ slog.Out("%s. Cannot init un-named project\n", *_fn) ; return E_INITFAIL ; }

	slog.Out("%s. Loding Configs\n", *_fn) ;
	rc = X.Load(cfgfile) ;
	if (rc != E_OK)
		{ slog.Out("%s. ERROR Could not load project file %s\n", *_fn, *cfgfile) ; return rc ; }

	pRoot = X.GetRoot() ;
	if (!pRoot)
	{
		slog.Out("%s: ERROR No data in project file\n", *_fn) ;
		return E_NODATA ;
	}

	if (!pRoot->NameEQ("autodoc"))
	{
		slog.Out("%s. Unexpected root node of <%s> - Only <autodoc> allowed here\n", *_fn, pRoot->TxtName()) ;
		return E_SYNTAX ;
	}

	for (ai = pRoot ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))			m_Name = ai.Value() ;
		else if (ai.NameEQ("hostpage"))		m_Page = ai.Value() ;
		else if (ai.NameEQ("copyright"))	m_Copyright = ai.Value() ;
		else
			{ rc=E_SYNTAX; slog.Out("%s. <autodoc> Bad param (%s=%s)\n", *_fn, ai.Name(), ai.Value()) ; }
	}

	if (!m_Name)	{ rc=E_SYNTAX; slog.Out("%s. The <autodoc> tag is missing a param of 'name'.\n", *_fn) ; }
	if (!m_Page)	{ rc=E_SYNTAX; slog.Out("%s. The <autodoc> tag is missing a param of 'page'.\n", *_fn) ; }

	for (pN1 = pRoot->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		slog.Out("%s. Reading Level 1 Tag %s\n", *_fn, pN1->TxtName()) ;
		ai = pN1 ;

		//	All the usual!
		if (pN1->NameEQ("website"))
		{
			if (!ai.Valid())		{ rc = E_SYNTAX ; slog.Out("%s. <website> addr not supplied\n", *_fn) ; break ; }
			if (!ai.NameEQ("addr"))	{ rc = E_SYNTAX ; slog.Out("%s. The only legal param to <website> is 'addr'. %s not allowed\n", *_fn, ai.Name()) ; break ; }

			m_Website = ai.Value() ;
		}
		else if (pN1->NameEQ("cssfile"))
		{
			if (!ai.Valid())		{ rc = E_SYNTAX ; slog.Out("%s. <cssfile> file not supplied\n", *_fn) ; break ; }
			if (!ai.NameEQ("file"))	{ rc = E_SYNTAX ; slog.Out("%s. <cssfile> only legal param is 'file'. %s not allowed\n", *_fn, ai.Name()) ; break ; }

			g_CSS = ai.Value() ;
		}
		else if (pN1->NameEQ("htmldir"))
		{
			if (!ai.Valid())		{ rc = E_SYNTAX ; slog.Out("%s. <htmldir> not supplied\n", *_fn) ; break ; }
			if (!ai.NameEQ("path"))	{ rc = E_SYNTAX ; slog.Out("%s. <htmldir> only legal param is 'path'. %s not allowed\n", *_fn, ai.Name()) ; break ; }

			m_HtmlDir = ai.Value() ;
		}
		else if (pN1->NameEQ("desc"))
		{
			m_Desc = pN1->m_fixContent ;
		}
		else if (pN1->NameEQ("copyright"))
		{
			if (!ai.Valid())			{ rc = E_SYNTAX ; slog.Out("%s. <copyright> not supplied\n", *_fn) ; break ; }
			if (!ai.NameEQ("notice"))	{ rc = E_SYNTAX ; slog.Out("%s. <copyright> only legal param is 'notice'. %s not allowed\n", *_fn, ai.Name()) ; break ; }

			m_Copyright = ai.Value() ;
		}
		else if (pN1->NameEQ("library"))
		{
			pComp = new ceComp() ;
			m_Libraries.Add(pComp) ;
			rc = pComp->ReadConfig(pN1) ;
		}
		else if (pN1->NameEQ("program"))
		{
			pComp = new ceComp() ;
			m_Programs.Add(pComp) ;
			rc = pComp->ReadConfig(pN1) ;
		}
		else
		{
			slog.Out("%s. Invalid tag <%s> Only <website|cssfile|htmldir|desc|copyright|library|program> allowed here\n", *_fn, pN1->TxtName()) ;
			rc = E_SYNTAX ;
		}
	}

	if (rc == E_OK && m_Desc)
	{
		m_Docname = "proj." + m_Name + ".html" ;
		//m_Docname.FnameEncode() ;
	}

	if (!m_HtmlDir)
		{ rc = E_SYNTAX ; slog.Out("%s. No website set for component\n", *_fn) ; }

	if (rc != E_OK)
		return rc ;

	m_ExpFile = m_HtmlDir + "/" + m_Name + ".adp" ;

	if (rc == E_OK)
	{
		rc = AssertDir(m_HtmlDir, 0755) ;
		if (rc != E_OK)
			{ slog.Out("%s. Could not assert document root directory %s\n", *_fn, *m_HtmlDir) ; return rc ; }
	}

	if (rc == E_OK)
	{
		rc = AssertDir(m_HtmlDir, 0755) ;
		if (rc != E_OK)
			{ slog.Out("%s. Could not assert project directory %s\n", *_fn, *m_HtmlDir) ; return rc ; }
	}

	if (!m_Libraries.Count() && !m_Programs.Count())
	{
		slog.Out("%s. ERROR No componetnts found\n", *_fn) ;
		return E_NODATA ;
	}

	slog.Out("HTML DIR %s\n", *m_HtmlDir) ;

	return rc ;
}

hzEcode	ceProject::Process	(void)
{
	_hzfunc("ceProject::Process") ;

	hzList<ceComp*>::Iter	ci ;

	ceComp*	pComp ;
	hzEcode	rc = E_OK ;

	//	Process
	for (ci = m_Libraries ; rc == E_OK && ci.Valid() ; ci++)
	{
		pComp = ci.Element() ;
		rc = pComp->Process() ;
	}

	for (ci = m_Programs ; rc == E_OK && ci.Valid() ; ci++)
	{
		pComp = ci.Element() ;
		rc = pComp->Process() ;
	}

	//	Post-process
	for (ci = m_Libraries ; rc == E_OK && ci.Valid() ; ci++)
	{
		pComp = ci.Element() ;
		rc = pComp->PostProc() ;
	}

	for (ci = m_Programs ; rc == E_OK && ci.Valid() ; ci++)
	{
		pComp = ci.Element() ;
		rc = pComp->PostProc() ;
	}

	return rc ;
}

hzEcode	showtree	(hzHttpEvent* pE)
{
	//	Display the currently applicable data model as a tree of classes, class members and repositories.
	//
	//	The display comprises a tree on the LHS and a form on the RHS. The root node is automatically named after the application and is displayed as open. This
	//	will have a visible subnode of 'Classes' and if any classes have been defined, another of "Repositories". These will display the '+' expansion symbol if
	//	there are clases defined and/or repositories declared. Clinking on the subnode name will produce on the RHS, a blank form to create a new class or add a
	//	repostory.
	//
	//	Under 'Classes' the tree uses the folder symbol to represent classes and the document symbol to represent class members. The class entries link to a form which
	//	Under "Repositories" the folder 
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc(__func__) ;

	hzChain		Z ;				//	XML Content of key resource (page/navbar/stylesheet etc)
	hzChain		C ;				//	Page output
	hzDirent	de ;			//	Config file info
	hzXDate		date ;			//	File date
	hzString	editorCls ;		//	Class editor base URL
	hzString	editorMbr ;		//	Class member editor base URL
	hzString	S ;				//	Temp string
	hzEcode		rc ;			//	Return code

	//	BUILD HTML
	C <<
	"<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n"
	"<style>\n"
	".stdbody		{ margin:0px; background-color:#FFFFFF; }\n"
	".main			{ text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000; }\n"
	".title			{ text-decoration:none; font-family:verdana; font-size:15px; font-weight:normal; color:#000000; }\n"
	".objpage		{ height:calc(100% - 120px); border:1px; margin-left:5px; background-color:#B8B8FF; overflow-x:auto; overflow-y:auto; }\n"
	".navtree		{ height:calc(100% - 40px); border:0px; margin:0px; list-style-type:none; white-space:nowrap; background-color:#E0E0E0; overflow-x:auto; overflow-y:scroll; }\n"
	".directory		{ list-style-type:none; font-size:11px; margin:0px; white-space:nowrap; overflow-x:auto; overflow-y:auto; font-weight:bold; }\n"
	".navtree img	{ vertical-align:-30%; }\n"
	"pre			{ tab-size:4; white-space: pre-wrap; white-space: -moz-pre-wrap; white-space: -pre-wrap; white-space: -o-pre-wrap; word-wrap: break-word; }\n"
	"</style>\n"
	"<script>\n"
	"var	screenDim_W;\n"
	"var	screenDim_H;\n"
	"function gwp()\n"
	"{\n"
	"\tscreenDim_W = window.innerWidth;\n"
	"\tscreenDim_H = window.innerHeight;\n"
	"\tvar txt;\n"
	"\tvar h;\n"
	"\th=screenDim_H-40;\n"
	"\ttxt=h+\"px\";\n"
	"\tdocument.getElementById(\"navlhs\").style.height=txt;\n"
	"\th-=80;\n"
	"\ttxt=h+\"px\";\n"
	"\tdocument.getElementById(\"articlecontent\").style.height=txt;\n"
	"}\n" ;

	C << _dsmScript_tog ;
	C << _dsmScript_loadArticle ;
	C << _dsmScript_navtree ;

	g_treeNav.ExportDataScript(C) ;
	C <<
	"</script>\n"
	"</head>\n\n" ;

	//	Create HTML table for listing
	C << "<body class=\"stdbody\" onpageshow=\"gwp();\" onresize=\"gwp();\">\n\n" ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"\t<tr height=\"40\"><td align=\"center\" class=\"title\">HadronZoo::CodeEnforcer</td></tr>\n"
	"</table>\n" ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
	"<tr>\n"
	"	<td width=\"20%\" valign=\"top\" class=\"main\">\n"
	"		<div id=\"navlhs\" class=\"navtree\">\n"
	"			<script type=\"text/javascript\">makeTree('hzlib');</script>\n"
	"		</div>\n"
	"	</td>\n"
	"	<td width=\"80%\" valign=\"top\" class=\"main\">\n"
	"		<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
	"		<tr height=\"40\"><td align=\"center\" class=\"title\"><div id=\"articletitle\"></div></td></tr>\n"
	"		<tr>\n"
	"			<td valign=\"top\" class=\"main\"><div id=\"articlecontent\" class=\"objpage\"></div></td>\n"
	"		</tr>\n"
	"		<tr height=\"40\"><td align=\"center\"><input type=\"button\" onclick=\"location.href='/terminate';\" value=\"Exit Program\"/></td></tr>\n"
	"		</table>\n"
	"	</td>\n"
	"</tr>\n"
	"</table>\n" ;

	//	Show buttons
	C <<
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzTcpCode	ProcHTTP	(hzHttpEvent* pE)
{
	//  This is needed as a function to pass to the AddPort() function of the HTTP server instance as it is not possible to pass it a non static
	//  class member function directly. The HTTP handler of the hdsApp class is what does all the work.
	//
	//  Argument:	pE	The HTTP event
	//
	//  Returns:	Enum TCP return code

	_hzfunc(__func__) ;

	if (!theApp)
		return TCP_TERMINATE ;
	return theApp->ProcHTTP(pE) ;
}

int32_t	main	(int32_t argc, char ** argv)
{
	_hzfunc("ce::main") ;

	hzString	projname ;		//	Project name, names logfile and XML config file
	hzString	cfgfile ;		//	Current config file
	int32_t		nArg ;			//	Argument iterator
	hzEcode		rc ;			//	Return code

	//	Set up signals
	signal(SIGHUP,  CatchCtrlC) ;
	signal(SIGINT,  CatchCtrlC) ;
	signal(SIGQUIT, CatchCtrlC) ;
	signal(SIGILL,  CatchSegVio) ;
	signal(SIGTRAP, CatchCtrlC) ;
	signal(SIGABRT, CatchCtrlC) ;
	signal(SIGBUS,  CatchSegVio) ;
	signal(SIGFPE,  CatchSegVio) ;
	signal(SIGKILL, CatchSegVio) ;
	signal(SIGUSR1, CatchCtrlC) ;
	signal(SIGSEGV, CatchSegVio) ;
	signal(SIGUSR2, CatchCtrlC) ;
	signal(SIGPIPE, SIG_IGN) ;
	signal(SIGALRM, CatchCtrlC) ;
	signal(SIGTERM, CatchCtrlC) ;
	signal(SIGCHLD, CatchCtrlC) ;
	signal(SIGCONT, CatchCtrlC) ;
	signal(SIGSTOP, CatchCtrlC) ;

	//	Check args and obtain progect name
	for (nArg = 1 ; nArg < argc ; nArg++)
	{
		if (!strcmp(argv[nArg], "-b"))	{ g_bDebug = true ; continue ; }
		if (!strcmp(argv[nArg], "-l"))	{ g_bLive = true ; continue ; }

		if (projname)
		{
			cout << "Usage: ce [-b] project_name\n" ;
			return -1 ;
		}
		projname = argv[nArg] ;
	}

	if (!projname)
	{
		cout << "Usage: ce [-b] project_name\n" ;
		return -1 ;
	}

	//	Check args and $HADRONZOO
	rc = HadronZooInitEnv() ;
	if (rc != E_OK)
	{
		if (!_hzGlobal_HOME)		cout << "Sorry - No path set for $HOME\n" ;
		if (!_hzGlobal_HADRONZOO)	cout << "Sorry - No path set for $HADRONZOO\n" ;

		cout << "Code Enforcer cannot proceed\n" ;
		return -2 ;
	}

	//	Open logs
	slog.OpenFile(*projname, LOGROTATE_NEVER) ;
	slog.Verbose(true) ;
	slog.Out("HADRONZOO = %s\n", *_hzGlobal_HADRONZOO) ;

	//	Set up string table
	hzFsTbl::StartStrings("words.dat") ;

	//	Init for standard C++ types and the std namespace
	rc = PreLoadTypes() ;
	if (rc != E_OK)
		return -1 ;

	//	Derive pathname of config file
	cfgfile = projname ;
	if (!cfgfile.Contains(".xml"))
		cfgfile += ".xml" ;

	//	Init project with config file
	rc = g_Project.Init(cfgfile) ;
	if (rc != E_OK)
		return -1 ;

	//	Process project components
	slog.Out("\nCode Enforcer: Commencing Processing\n\n") ;
	rc = g_Project.Process() ;
	if (rc != E_OK)
		return -1 ;

	//	Export entities from project components
	slog.Out("\nCode Enforcer: Exporting project\n") ;
	rc = g_Project.Export() ;
	if (rc != E_OK)
		return -1 ;

	//	Issue report (generate HTML files)
	slog.Out("\nCode Enforcer: Reporting project\n") ;
	if (rc == E_OK)
		g_Project.Report() ;

	slog.Out("\nCode Enforcer: Exporting String table\n") ;
	cfgfile = "allwords" ;
	_hzGlobal_StringTable->Export(cfgfile) ;
	slog.Out("Total string expected %d\n", g_allStrings.Count()) ;

	RecordMemoryUsage() ;

	if (g_bLive)
	{
		//	Set up viewing server
		//hzDictionary::GetInstance() ;
		if (!_hzGlobal_StringTable)
			{ slog.Out("Could not create string table\n") ; return -14 ; }

		if (SetupHost() != E_OK)
			{ slog.Log(_fn, "Could not setup hostname\n") ; return 0 ; }

		rc = InitCountryCodes() ;
		if (rc != E_OK)
			{ slog.Out("No Country Codes\n") ; return 105 ; }

		rc = InitIpBasic() ;
		if (rc != E_OK)
			slog.Out("No Basc level IP Location table\n") ;

		Dissemino::GetInstance(slog) ;

		theApp = hdsApp::GetInstance(slog) ;
		theApp->m_Docroot = "." ;
		theApp->m_Images = "/home/rballard/hzraw/img" ;
		theApp->m_Images = _hzGlobal_HADRONZOO + "/img" ;
		if (!theApp->m_DefaultLang)
		theApp->m_DefaultLang = "en-US" ;

		theApp->m_pDfltLang = new hdsLang() ;
		theApp->m_pDfltLang->m_code = theApp->m_DefaultLang ;
		theApp->m_Languages.Insert(theApp->m_pDfltLang->m_code, theApp->m_pDfltLang) ;

		theApp->m_ArticleGroups.Insert("hzlib", &g_treeNav) ; 

		rc = theApp->AddCIFunc(&showtree, "/", ACCESS_PUBLIC, HTTP_GET) ;

		theApp->SetupScripts() ;
		if (rc != E_OK)
			Fatal("%s. Standard Script Generation Failure\n", *_fn) ;
		slog.Out("Scripts Formulated\n") ;


		g_pTheServer = hzIpServer::GetInstance(&slog) ;
		if (!g_pTheServer)
			Fatal("Failed to allocate server instance\n") ;

		rc = g_pTheServer->AddPortHTTP(&ProcHTTP, 900, 18600, 5, false) ;
		if (rc != E_OK)
			{ slog.Log(_fn, "Could not add HTTP port (error=%s)\n", Err2Txt(rc)) ; return 0 ; }

		if (g_pTheServer->Activate() != E_OK)
		{
			slog.Log(_fn, "HTTP Server cannot run. Please check existing TCP connections with netstat -a\n") ;
			return 0 ;
		}

		slog.Log(_fn, "#\n#\tStarting Server\n#\n") ;
		g_pTheServer->ServeEpollST() ;
	}

	return 0 ;
}
