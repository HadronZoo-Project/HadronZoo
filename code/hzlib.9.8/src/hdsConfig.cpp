//
//	File:	hdsConfig.cpp
//
//	Legal Notice: This file is part of the HadronZoo C++ Class Library.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	The HadronZoo C++ Class Library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//	as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
//	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License along with the HadronZoo C++ Class Library. If not, see
//	http://www.gnu.org/licenses.

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>

#include "hzErrcode.h"
#include "hzDissemino.h"

using namespace std ;


global	hzMapS<hzString,uint32_t>	_hzGlobal_JS_Events ;

void	InitJS_Events	(void)
{
	if (_hzGlobal_JS_Events.Count())
		return ;

	hzString	S ;		//	For storing JS event
	uint32_t	n = 0 ;	//	Event number

	S = "onpageshow" ;	_hzGlobal_JS_Events.Insert(S, n++) ;	//	Page has completed loading

	S = "onchange" ;	_hzGlobal_JS_Events.Insert(S, n++) ;	// 	An HTML element has been changed
	S = "onclick" ;		_hzGlobal_JS_Events.Insert(S, n++) ;	//	The user clicks an HTML element
	S = "onmouseover" ;	_hzGlobal_JS_Events.Insert(S, n++) ;	//	The user moves the mouse over an HTML element
	S = "onmouseout" ;	_hzGlobal_JS_Events.Insert(S, n++) ;	// 	The user moves the mouse away from an HTML element
	S = "onkeydown" ;	_hzGlobal_JS_Events.Insert(S, n++) ;	// 	The user pushes a keyboard key
	S = "onload" ;		_hzGlobal_JS_Events.Insert(S, n++) ;	// 	The browser has finished loading the page
}

/*
**	SECTION 1:	Minor Config Read Functions
*/

hzEcode	hdsApp::_readRgxType	(hzXmlNode* pN)
{
	//	Category:	Minor Config Read Functions
	//
	//	Process a <rgxtype> tag to define an instance of hdbRgxtype (a special text regex controlled datatype), and add it to the available data types. As these
	//	are always text fields, it is not necessary to supply any type infomation. These types are always of short fixed or short maximum size and are validated
	//	by means of eiher a formal regex or a less formal expression using Aa/N where A is [A-Z], a is [a-z] and N is a digit.
	//
	//	The attributes name the datatype, set the maximum length and the regex.
	//
	//	Arguments:	1)	pN	Pointer to XML node <rgxType>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the field type parameters are valid

	_hzfunc("hdsApp::_readRgxType") ;
	
	hzAttrset		ai ;			//	Attribute iterator
	hdbRgxtype*		pType ;			//	Fldtype
	hzString		name ;			//	Name of new data type
	hzString		regex ;			//	Controlling expression
	hzString		limit ;			//	Size limit
	uint32_t		max ;			//	Size limit
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)					Fatal("%s: No node supplied\n", *_fn) ;
	if (!pN->NameEQ("rgxtype"))	Fatal("%s. Wrong call", *_fn) ;

	//	Type parameters (name)
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("typename"))	name = ai.Value() ;
		else if (ai.NameEQ("regex"))	regex = ai.Value() ;
		else if (ai.NameEQ("limit"))	limit = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <rgxtype> tag: Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!name)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <rgxtype>: No data type name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!regex)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <rgxtype>: No regex supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (!limit)
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <rgxtype>: No size limit supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	else
	{
		max = atol(*limit) ;
		if (!max)
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <rgxtype>: No size limit could be established\n", *_fn, pN->Fname(), pN->Line()) ; }
	}

	if (m_ADP.GetDatatype(name))
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <rgxtype>: Already have data type %s\n", *_fn, pN->Fname(), pN->Line(), *name) ; }

	//	Type subtags
	if (rc == E_OK)
	{
		pType = new hdbRgxtype() ;
		pType->SetTypename(name) ;

		rc = m_ADP.RegisterRegexType(pType) ;
		m_pLog->Out("%s. File %s Line %d: Added %s\n", *_fn, pN->Fname(), pN->Line(), pType->TxtTypename()) ;
	}

	return rc ;
}

hzEcode	hdsApp::_readInitstate	(hzXmlNode* pN)
{
	//	Category:	Minor Config Read Functions
	//
	//	Read the <initstate> tag. This is only applied in the event of Dissemino being called with the -newData command line argument. The tag has subtags of
	//	<newdataCSV> which name repositories and files to be loaded into them. The upload will be incremental rather than a truncation
	//
	//	Arguments:	1)	pN	Pointer to XML node <initstate>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <initstate> parameters are valid

	_hzfunc("hdsApp::_readInitstate") ;

	const hdbClass*		pClass_host ;	//	Host (repository) class
	const hdbMember*	pMbr_csv ;		//	Data class member of CSV class
	const hdbMember*	pMbr_host ;		//	Data class member of Repository class

	hdsLoad			ld ;			//	Load command
	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Class member subnodes
	hzString		rep ;			//	Named repository
	hzString		cls ;			//	Load data command
	uint32_t		memNo ;			//	Member number
	uint32_t		nMatches ;		//	Number of member matches
	hzEcode			rc = E_OK ;		//	Return code

	//	Check arguments
	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;

	//	Process <initstate> tag
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		ld.Clear() ;

		if (pN1->NameEQ("newdataCSV"))
		{
			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("repos"))	rep = ai.Value() ;
				else if (ai.NameEQ("class"))	cls = ai.Value() ;
				else if (ai.NameEQ("load"))		ld.m_Filepath = ai.Value() ;
				else
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d <newdataCSV> Illegal attribute (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ;
					break ;
				}
			}

			//	Check that filepath, repos and if supplied the class exists and if so add to app's initstate stack
			if (!ld.m_Filepath)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <newdataCSV> No source data file named\n", *_fn, pN->Fname(), pN1->Line()) ; break ; }
			if (!rep)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <newdataCSV> No repository named\n", *_fn, pN->Fname(), pN1->Line()) ; break ; }

			ld.m_pRepos = m_ADP.GetObjRepos(rep) ;
			if (!ld.m_pRepos)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <newdataCSV> Named repository %s not declared\n", *_fn, pN->Fname(), pN->Line(), *rep) ; break ; }
			pClass_host = ld.m_pRepos->Class() ;

			if (!cls)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <newdataCSV> No source data class named\n", *_fn, pN->Fname(), pN1->Line()) ; break ; }
			ld.m_pClass = m_ADP.GetPureClass(cls) ;
			if (!ld.m_pClass)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <newdataCSV> Named class %s not defined\n", *_fn, pN->Fname(), pN->Line(), *cls) ; break ; }

			/*
			**	Now check for commonality between members. There must be at least member in the guest class which matches (on both name and data type) to a member of the host class
			**	and of the matched members, at least one must serve as a unique index into the host repository.
			*/

			for (memNo = 0 ; memNo < ld.m_pClass->MbrCount() ; memNo++)
			{
				pMbr_csv = ld.m_pClass->GetMember(memNo) ;

				//	Check the CSV class member is not a subclasses
				if (pMbr_csv->Basetype() == BASETYPE_CLASS)
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d <newdataCSV> Named CSV class %s has subclass member %s\n",
						*_fn, pN->Fname(), pN->Line(), *cls, pMbr_csv->TxtName()) ;
					break ;
				}

				//	Look the CSV member up in the host class
				pMbr_host = pClass_host->GetMember(pMbr_csv->StrName()) ;
				if (pMbr_host)
				{
					if (pMbr_host->Basetype() != pMbr_csv->Basetype())
					{
						rc = E_SYNTAX ;
						m_pLog->Out("%s. File %s Line %d <newdataCSV> CSV class member %s conflicts on type\n",
							*_fn, pN->Fname(), pN->Line(), pMbr_csv->TxtName()) ;
						break ;
					}
					nMatches++ ;
				}
			}

			if (!nMatches)
			{
				rc = E_SYNTAX ;
				m_pLog->Out("%s. File %s Line %d <newdataCSV> Named CSV class %s has no member matches in repository %s\n",
					*_fn, pN->Fname(), pN1->Line(), *cls, pClass_host->TxtTypename()) ;
			}

			if (rc == E_OK)
				m_InitstateLoads.Add(ld) ;
			continue ;
		}

		rc = E_SYNTAX ;
		m_pLog->Out("%s. File %s Line %d <initstate> Illegal subtag %s\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
	}

	m_pLog->Out("%s. Completed Initstate Directive\n", *_fn) ;
	return rc ;
}

hzEcode	hdsApp::_readScript	(hzXmlNode* pN)
{
	//	Category:	Minor Config Read Functions
	//
	//	Read in a JaveScript from an <xscript> tag and place the script in m_rawScripts (if it does not already exist)
	//
	//	Arguments:	1)	pN	Current XML node expected to be <xscript> tag
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readScript") ;

	ifstream		is ;			//	For reading in script file (if applicable)
	hzChain			Z ;				//	Script content
	hzChain			X ;				//	Script content (zipped)
	hzAttrset		ai ;			//	Attribute iterator
	hzString		name ;			//	Name of script
	hzString		fname ;			//	Filename of script (if applicable)
	hzString		S ;				//	Temp string
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)					Fatal("%s: No node supplied\n", *_fn) ;
	if (!pN->NameEQ("xscript"))	Fatal("%s. Wrong call\n", *_fn) ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))	name = ai.Value() ;
		else if (ai.NameEQ("file"))	fname = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xscript>: Invalid param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!name)
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xscript>: No name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (m_rawScripts.Exists(name))
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xscript>: Name %s in use\n", *_fn, pN->Fname(), pN->Line(), *name) ; }

	if (!fname)
		Z = pN->m_fixContent ;
	else
	{
		is.open(*fname) ;
		if (is.fail())
			{ rc = E_OPENFAIL ; m_pLog->Out("%s. File %s Line %d <xcript> File %s not opened\n", *_fn, pN->Fname(), pN->Line(), *fname) ; }
		else
			{ Z << is ; is.close() ; }
	}

	if (!Z.Size())
	{
		m_pLog->Out("%s. File %s Line %d <xcript> No content\n",  *_fn, pN->Fname(), pN->Line()) ;
		return E_NODATA ;
	}

	S = Z ;
	m_rawScripts.Insert(name, S) ;
	Gzip(X, Z) ;
	S = X ;
	m_zipScripts.Insert(name, S) ;

	m_pLog->Out("%s. Added script %s of %d bytes (%d zipped)\n", *_fn, *name, Z.Size(), X.Size()) ;

	return rc ;
}

hzEcode	hdsApp::_readSiteLangs	(hzXmlNode* pN)
{
	//	Category:	Minor Config Read Functions
	//
	//	Read in a site language directive. This comprises the whole set of languages the site will support. The directive starts with a <siteLangusges> tag and
	//	names each language with a <language> tag. These in turn comprise the language code, the image for the display flag and the language name
	//
	//	Arguments:	1)	pN	Current XML node expected to be <siteLangauges> tag
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readSiteLangs") ;

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	For <subject> tags
	hdsLang*		pLang ;			//	Supported language
	hzString		code ;			//	For language code
	hzString		name ;			//	For language name (in default language e.g. 'German')
	hzString		natv ;			//	For language name (in target language e.g. 'Duetsch')
	hzString		imgf ;			//	For language flag image
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("siteLanguages"))
		Fatal("%s. File %s Line %d Expected <siteLangauges> tag. Got <%s>\n", *_fn, pN->Fname(), pN->Line()) ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if (ai.NameEQ("default"))
			m_DefaultLang = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d. Illegal <siteLanguages> attribute (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!m_DefaultLang)
		{ rc=E_NOINIT; m_pLog->Out("%s. File %s Line %d. <siteLanguages> No default language specified\n", *_fn, pN->Fname(), pN1->Line()) ; }

	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (!pN1->NameEQ("language"))
		{
			m_pLog->Out("%s. File %s Line %d. <%s> Illegal. Only <language> can be a subtag of <siteLanguages>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
			rc = E_SYNTAX ;
			continue ;
		}

		//	Page attributes
		name = code = imgf = (char*)0 ;

		for (ai = pN1 ; ai.Valid() ; ai.Advance())
		{
			if		(ai.NameEQ("code"))		code = ai.Value() ;
			else if (ai.NameEQ("name"))		name = ai.Value() ;
			else if (ai.NameEQ("native"))	natv = ai.Value() ;
			else if (ai.NameEQ("flag"))		imgf = ai.Value() ;
			else
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d. Illegal <language> attribute (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
		}

		if (!name)	{ rc=E_NOINIT; m_pLog->Out("%s. File %s Line %d. <language> No language name supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		if (!natv)	{ rc=E_NOINIT; m_pLog->Out("%s. File %s Line %d. <language> No native language name supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		if (!code)	{ rc=E_NOINIT; m_pLog->Out("%s. File %s Line %d. <language> No language code supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		if (!imgf)	{ rc=E_NOINIT; m_pLog->Out("%s. File %s Line %d. <language> No language image file supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }

		if (rc != E_OK)
			continue ;

		pLang = m_Languages[code] ;

		if (!pLang)
		{
			pLang = new hdsLang() ;
			pLang->m_code = code ;
			pLang->m_flag = imgf ;
			pLang->m_name = name ;
			pLang->m_natv = natv ;

			m_Languages.Insert(pLang->m_code, pLang) ;
			m_pLog->Out("%s. File %s Line %d. <language> INSERTED New Language\n", *_fn, pN->Fname(), pN1->Line(), *pLang->m_code) ;
		}
		else
		{
			pLang->m_code = code ;
			pLang->m_flag = imgf ;
			pLang->m_name = name ;
			pLang->m_natv = natv ;
		}

		if (pLang->m_name == m_DefaultLang)
			m_pDfltLang = pLang ;
	}

	if (!m_pDfltLang)
		{ rc = E_NOINIT; m_pLog->Out("%s. File %s Line %d. <siteLanguages> No Default language\n", *_fn, pN->Fname(), pN->Line()) ; }

	return rc ;
}

hzEcode	hdsApp::_readNav		(hzXmlNode* pN)
{
	//	Category:	Minor Config Read Functions
	//
	//	Read in <navigation> tag and its set of <subject> tags. These set the order for the subject headings for a navbar pull down menu
	//
	//	Arguments:	1)	pN	Current XML node expected to be <navigation> tag
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readNav") ;

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	For <subject> tags
	hdsSubject*		pSubj ;			//	Page subject
	hzString		subj ;			//	For subject name
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;

	ai = pN ;
	if (ai.Valid())
	{
		m_pLog->Out("%s. File %s Line %d. <navigation> does not require attributes.\n", *_fn, pN->Fname(), pN->Line()) ;
		return E_SYNTAX ;
	}

	m_pLog->Out("%s. File %s Line %d. <navigation> directive\n", *_fn, pN->Fname(), pN->Line()) ;

	for (pN1 = pN->GetFirstChild() ; pN1 ; pN1 = pN1->Sibling())
	{
		if (!pN1->NameEQ("subject"))
		{
			m_pLog->Out("%s. File %s Line %d. <%s> Illegal. Only <subject> is allowed as a subtag of <navigation>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
			rc = E_SYNTAX ;
			continue ;
		}

		//	Page attributes
		for (ai = pN1 ; ai.Valid() ; ai.Advance())
		{
			if (ai.NameEQ("name"))
				subj = ai.Value() ;
			else
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d. Illegal <subject> attribute (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
		}

		if (rc != E_OK)
			continue ;

		pSubj = m_setPgSubjects[subj] ;

		if (pSubj)
			m_pLog->Out("%s. File %s Line %d. WARNING Duplicate subject [%s] ignored\n", *_fn, pN->Fname(), pN->Line(), *subj) ;
		else
		{
			pSubj = new hdsSubject() ;
			pSubj->subject = subj ;

			m_setPgSubjects.Insert(pSubj->subject, pSubj) ;
			m_vecPgSubjects.Add(pSubj) ;
			pSubj->m_USL.SetSubj(m_setPgSubjects.Count()) ;
			m_pLog->Out("%s. File %s Line %d. Added subject %s\n", *_fn, pN->Fname(), pN->Line(), *subj) ;
		}
	}

	return rc ;
}


hzEcode	hdsApp::_readCSS	(hzXmlNode* pN)
{
	//	Category:	Minor Config Read Functions
	//
	//	Process the singleton <xstyle> tag. This tag is the same as a HTML style tag except that the style definitions can all be in the XML config file(s) instead of a .css file.
	//	The original plan was twofold; To ensure there was only one CSS resource for the website instead of many, thereby reducing demand on the server AND at some future point to
	//	manage the styles so that warnings could be generated if a style was redundant or did not exist. This latter objective has yet to be implimented, largely because CSS has
	//	undergone significant evolution since the <xstyle> tag was concocted and style management does not have a high priority.
	//
	//	The tag has been considered for deprecation but this is unlikely. It has become an established Dissemino 'habit'.
	//
	//	Arguments:	1)	pN	Pointer to XML node <xstyle>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <xstyle> parameters are valid

	_hzfunc("hdsApp::_readCSS") ;

	hzAttrset	ai ;			//	Attribute iterator

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;

	if (m_namCSS)
		{ m_pLog->Out("%s. File %s Line %d Only one stylesheet allowed\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }

	if (!pN->NameEQ("xstyle"))
		{ m_pLog->Out("%s. File %s Line %d Expected <xstyle> got <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return E_SYNTAX ; }

	if (pN->GetFirstChild())
		{ m_pLog->Out("%s. File %s Line %d <xstyle> tag should not have sub-tags\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }

	//	The <style> tag should define a stylesheet that can be stored as a page in it's own right. It has a single attribute of name which must not
	//	conflict with any of the pages. The page produced (the stylesheet) must nessesarily be public (accesseble to all)
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if (ai.NameEQ("name"))
			m_namCSS = ai.Value() ;
		else
			{ m_pLog->Out("%s File %s Line %d: <xstyle> Illegal attr (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; return E_SYNTAX ; }
	}

	if (!m_namCSS)
	{
		m_pLog->Out("%s File %s Line %d: <xstyle> requires a name sttribute\n", *_fn, pN->Fname(), pN->Line()) ;
		return E_SYNTAX ;
	}

	m_txtCSS << "<style>\n<!--\n" ;

	if (!pN->GetFirstChild())
		m_txtCSS << pN->m_fixContent ;

	m_txtCSS << "-->\n</style>\n" ;
	Gzip(m_zipCSS, m_txtCSS) ;

	return E_OK ;
}

hzEcode	hdsApp::_readInclude	(hzXmlNode* pN, hdsVE* parent, uint32_t nLevel)
{
	//	Category:	Minor Config Read Functions
	//
	//	The <xinclude> tags describe a block of HTML which can then be included in pages and responses. The effect is similar to the HTML include file
	//	statement <!--#include file="filename"-->. The <xinclude> tag is only permitted as a subtag to <project>, it cannot appear with a <page> tag.
	//	Within a <page>, the tag <xblock name="blockname"/> names the block of tags to be included.
	//
	//	Note that virtually all tags that are legal within a page are also legal within a block, which means this function is very similar to the
	//	readPage() function.
	//
	//	Arguments:	1)	pN		Pointer to XML node <xinclude>
	//				2)	parent	Parent visible entity
	//				3)	nLevel	Node level
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <xinclude> parameters are valid

	_hzfunc("hdsApp::_readInclude") ;

	_tagArg			tga ;			//	Tag argument for _readTag()
	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Current XML node
	hdsVE*			thisVE ;		//	Current visual entity
	hdsVE*			newVE ;			//	Subordinate visual entity
	hdsBlock*		pIncl ;			//	This include block
	hzString		name ;			//	Name of block
	hdsUSL			usl ;			//	Temp string for USL
	hzEcode			rc = E_OK ;		//	Return code
	hzRecep32		r32 ;			//	For USL text value

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;
	if (!pN->NameEQ("xinclude"))
		{ m_pLog->Out("%s. File %s Line %d Expected <xinclude> got <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return E_SYNTAX ; }

	m_pLog->Out("%s. Level %d Node %s File %s Line %d\n", *_fn, nLevel, pN->TxtName(), pN->Fname(), pN->Line()) ;

	//	The attr will be the name of the form to which the handler applies
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if (ai.NameEQ("name"))
			name = ai.Value() ;
		else
		{
			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d <xinclude> Illegal attr %s=%s\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
		}
	}

	if (!name)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d Reading <xinclude> tag: No name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (m_Includes.Exists(name))
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d Reading <xinclude> tag: %s already exists\n", *_fn, pN->Fname(), pN->Line(), *name) ; }

	if (rc != E_OK)
		return rc ;

	thisVE = pIncl = new hdsBlock(this) ;
	thisVE->m_Line = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;
	thisVE->m_Tag = pN->TxtName() ;
	pIncl->m_Refname = name ;
	pIncl->m_USL.SetBlock(m_Includes.Count()) ;
	m_pLog->Out("%s. File %s Line %d Inserting xinclude %s\n", *_fn, pN->Fname(), pN->Line(), *pIncl->m_Refname) ;
	m_Includes.Insert(pIncl->m_Refname, pIncl) ;

	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		tga.m_pCaller = *_fn ;
		newVE = _readTag(&tga, pN1, pIncl->m_bScriptFlags, 0, 0) ;

		if (!newVE)
			rc = E_SYNTAX ;
		else
		{
			m_pLog->Out("%s. added child %s to xinclude %s\n", *_fn, *newVE->m_Tag, *pIncl->m_Refname) ;
			pIncl->AddVisent(newVE) ;
		}
	}

	if (rc != E_OK)
		return rc ;

	usl = pIncl->m_USL ;
	AssignVisentIDs(pIncl->m_VEs, pIncl->m_flagVE, usl) ;

	if (pIncl->m_flagVE & VE_ACTIVE)
		m_pLog->Out("%s. Assigned VE ids. Include Block %s deemed ACTIVE\n", *_fn, usl.Txt(r32)) ;
	else
		m_pLog->Out("%s. Assigned VE ids. Include Block %s deemed INACTIVE\n", *_fn, usl.Txt(r32)) ;

	pIncl->Complete() ;

	return rc ;
}

hdsVE*	hdsApp::_readTag	(hdsApp::_tagArg* tga, hzXmlNode* pN, uint32_t& bScrFlags, hdsVE* parent, uint32_t level)
{
	//	Category:	Minor Config Read Functions
	//
	//	Support function to the following:-
	//
	//		1) _readInclude()	which reads the <xinclude> tag to create blocks of tags that can be included in pages and articles.
	//		2) _readDirlist()	which reads the <xdirlist> tag to define how a listing of directory entries should be presented.
	//		3) _readTable()		which reads the <xtable> tag to define how aselection of objects in a repository should be presented.
	//		5) _readFormDef()	which reads the <xformDef> tag to create a form definition.
	//		5) _readPage()		which defines a page
	//		6) _readArticle()	which defines an article
	//
	//	All the above functions expect to process a mix of Dissemino and supportive HTML tags. Certain Dissemino tags are of particular importance to particular functions but since
	//	the tags are mixed, these are commonly parented by ordinary HTML tags. This means a recursive support function of some form is required to process the mix of tags on behalf
	//	of the above functions. Furthermore, many of the Dissemino tags are also only supportive and their use is widely permitted. This means the range of tags a recursive support
	//	functions would have to handle, would be extensive. This would mean the recursive support functions would themselves be extensive.
	//
	//	In some respects, there would be greater clarity if each of the six functions in the above list had their own recursive support function. If this were the approach however,
	//	all the recursive support functions would be extensive and similar to each other, thereby undermining clarity. This function was written to merge recursive tag processing
	//	in order to avoid such verbosity.
	//
	//	Note the recusion is limited in the case of particular tags. This is where the function called to process the tag, is expected to process the entire set
	//	of sub-tags and calls _readTag() in order to acheive this. For example, an <xformDef> tag results in a call to _readFormDef() but before this, a flag is
	//	set to block recursion of <xformDef> sub-tags within this invokation of this function. The _readFormDef() function will separately invoke this function
	//	to process all the sub-tags and return having completed a form definition. It would not make sense for the same sub-tags to be processed again.
	//
	//	Arguments:	1)	pN			The current XML node
	//				2)	bScrFlags	Script flags - set if the presence of a tag in a page implies the page must have a script
	//				3)	pPage		The page in which the tag resides
	//				4)	pFormdef	If the tag falls within a <xformDef> this will be the form definition in progress.
	//				6)	parent		The parent visible entity
	//
	//	Returns:	Pointer to diagram visual entity

	_hzfunc("hdsApp::_readTag") ;

	hzAttrset		ai ;				//	Attribute iterator
	hzXmlNode*		pN1 ;				//	Subtag probe
	hdsVE*			thisVE = 0 ;		//	Visible entity
	hdsVE*			newVE ;				//	Subtag visible entity
	hdsRecap*		pRecap ;			//	Google recaptcha
	hdsButton*		pButt ;				//	Dissemino form button
	hdsXdiv*		pXdiv ;				//	Dissemino user-status dependent tag set
	hdsCond*		pCond ;				//	Dissemino value dependent tag set
	hdsNavbar*		pNavbar ;			//	Dissemino Navigation pull-down menu
	hdsHtag*		pHtag ;				//	Standard HTML tag
	hdsXtag*		pXtag ;				//	Language support tag
	hdsBlock*		pIncl ;				//	Included block of tags
	hdsPage*		pPage ;				//	Set if resource is a page
	hdsFldspec		vd ;				//	Variable prototype
	hzPair			P ;					//	Attr/value pair
	const char*		i ;					//	Iterator for active tag tests
	hzString		cnam ;				//	Class name
	hzString		vnam ;				//	Variable name
	hzString		vdef ;				//	Variable definition (from which name can be used)
	hzString		iname ;				//	Index name
	hzString		name ;				//	Tag/visible entity name
	hzString		title ;				//	Text visible
	hzString		page ;				//	Expected page (to go to)
	hzString		whom ;				//	Allowed users
	hzString		css ;				//	Allowed CSS style
	hzString		hname ;				//	Form action (handler name)
	hzString		faUrl ;				//	Form action URL
	hzString		pcntEnt ;			//	Percent entity
	uint32_t		aflags ;			//	Access flags
	uint32_t		bErr = 0 ;			//	Error condition
	bool			bBlock = false ;	//	This is set for tags where the function called to process the tag, itself calls _readTag (see above note)
	hzEcode			rc = E_OK ;			//	Return code

	if (!this)
		Fatal("%s. No instance\n", *_fn) ;
	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;

	pPage = dynamic_cast<hdsPage*>(tga->m_pLR) ;

	//	m_pLog->Out("%s. File %s Line %d:%d caller %s level %d: Doing tag %s\n", *_fn, pN->Fname(), pN->Line(), pN->Level(), caller, level, pN->TxtName()) ;

	//	Tags that apply to pages and articles generally
	if		(pN->NameEQ("xtable"))		{ bBlock = true ; thisVE = _readTable(pN, pPage) ; }
	else if (pN->NameEQ("xdirlist"))	{ bBlock = true ; thisVE = _readDirlist(pN, pPage) ; }
	else if (pN->NameEQ("xpiechart"))	{ bBlock = true ; thisVE = _readPieChart(pN) ; }
	else if (pN->NameEQ("xbarchart"))	{ bBlock = true ; thisVE = _readBarChart(pN) ; }
	else if (pN->NameEQ("xstdchart"))	{ bBlock = true ; thisVE = _readStdChart(pN) ; }
	else if (pN->NameEQ("xdiagram"))	{ bBlock = true ; thisVE = _readDiagram(pN) ; }

	else if (pN->NameEQ("xtreeCtl"))
	{
		bBlock = true ;
		if (pPage)
			pPage->m_bScriptFlags |= INC_SCRIPT_NAVTREE ;
		thisVE = _readXtreeCtl(pN) ;
	}

	else if (pN->NameEQ("xblock"))
	{
		for (ai = pN ; ai.Valid() ; ai.Advance())
		{
			if (ai.NameEQ("name"))
				name = ai.Value() ;
			else
				{ m_pLog->Out("%s. File %s Line %d <xblock> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; return 0 ; }
		}

		if (!name)
			{ m_pLog->Out("%s. File %s Line %d <xblock> no name attr supplied\n", *_fn, pN->Fname(), pN->Line()) ; return 0 ; }

		if (!m_Includes.Exists(name))
			{ m_pLog->Out("%s. File %s Line %d xblock %s non-existant xinclude\n", *_fn, pN->Fname(), pN->Line(), *name) ; return 0 ; }

		thisVE = pIncl = m_Includes[name] ;
		thisVE->m_Tag = pN->TxtName() ;

		if (pPage)
			pPage->m_bScriptFlags |= pIncl->m_bScriptFlags ;
		bScrFlags |= pIncl->m_bScriptFlags ;

		m_pLog->Out("%s. Added block (%s)(%s)\n", *_fn, *pIncl->m_Refname, *name) ;
		bBlock = true ;
	}

	else if (pN->NameEQ("xdiv"))
	{
		//	Read in an <xdiv> tag. This has a single attribute of user whose value must either be public (no user logged on) or one of the declared
		//	user types. The effect of an <xdiv> is to make the displaying of its subtags conditional on who is logged on.

		whom.Clear() ;
		for (ai = pN ; ai.Valid() ; ai.Advance())
		{
			if (ai.NameEQ("user"))
				whom = ai.Value() ;
			else
				{ m_pLog->Out("%s. File %s Line %d: Invalid attribute for <xdiv> (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; return 0 ; }
		}

		if (!whom)
			{ m_pLog->Out("%s. File %s Line %d: <xdiv> No user specified\n", *_fn, pN->Fname(), pN->Line()) ; return 0 ; }

		aflags = _calcAccessFlgs(whom) ;
		if (aflags == 0xffffffff)
			{ m_pLog->Out("%s. File %s Line %d: Bad access specification (%s)\n", *_fn, pN->Fname(), pN->Line(), *whom) ; return 0 ; }

		//	All is well insert the <xdiv> into the page
		thisVE = pXdiv = new hdsXdiv(this) ;
		thisVE->m_strPretext = pN->TxtPtxt() ;
		thisVE->m_Line = pN->Line() ;
		thisVE->m_Indent = pN->Level() ;
		pXdiv->m_Tag = pN->TxtName() ;
		pXdiv->m_Access = aflags ;
		m_pLog->Out("%s. Added xdiv (%s, %08x)\n", *_fn, *whom, pXdiv->m_Access) ;
	}

	else if (pN->NameEQ("xcond"))
	{
		//	Read in an <xcond> tag. This has a single attribute of either 'exists', 'isnull' or 'equal' whose value must name a variable. The effect
		//	of <xcond> is to conditionally display it's subtags.

		thisVE = pCond = new hdsCond(this) ;
		thisVE->m_strPretext = pN->TxtPtxt() ;
		thisVE->m_Line = pN->Line() ;
		thisVE->m_Indent = pN->Level() ;

		pCond->m_cflags = 0 ;
		for (ai = pN ; ai.Valid() ; ai.Advance())
		{
			rc = thisVE->AddAttr(ai.Name(), ai.Value()) ;
			if (rc == E_SYNTAX)
			{
				m_pLog->Out("%s. File %s Line %d (%d) Malformed percent entity in tag attribute\n", *_fn, pN->Fname(), pN->Line(), thisVE->m_Line) ;
				delete pCond ;
				return 0 ;
			}

			if		(ai.NameEQ("exists"))	{ pCond->m_cflags |= XCOND_EXISTS ; cnam = ai.Value() ; }
			else if (ai.NameEQ("isnull"))	{ pCond->m_cflags |= XCOND_ISNULL ; cnam = ai.Value() ; }
			else if (ai.NameEQ("action"))	{ pCond->m_cflags |= XCOND_ACTION ; cnam = ai.Value() ; }
			else
			{
				m_pLog->Out("%s. File %s Line %d: <xcond> Invalid attribute (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
				delete pCond ;
				return 0 ;
			}
		}

		if (!pCond->m_cflags)
			{ m_pLog->Out("%s. File %s Line %d: <xcond> No condition specified\n", *_fn, pN->Fname(), pN->Line()) ; return 0 ; }
		if (CountBits(&pCond->m_cflags, sizeof(uint32_t)) > 1)
			{ m_pLog->Out("%s. File %s Line %d: <xcond> ambiguous condition specified\n", *_fn, pN->Fname(), pN->Line()) ; return 0 ; }

		//	All is well insert the <xcond> into the page
		pCond->m_Tag = pN->TxtName() ;
	}

	else if (pN->NameEQ("navbar"))
	{
		thisVE = pNavbar = new hdsNavbar(this) ;
		thisVE->m_Line = pN->Line() ;
		thisVE->m_Indent = pN->Level() ;
		thisVE->m_Tag = pN->TxtName() ;

		if (pPage)
			pPage->m_bScriptFlags |= INC_SCRIPT_NAVBAR ;
		else
		{
			//	Run back to hdsBlock (as this is the only other possible root)
			bScrFlags |= INC_SCRIPT_NAVBAR ;
		}
	}

	else if (pN->NameEQ("x"))
	{
		//	Language support tag

		thisVE = pXtag = new hdsXtag(this) ;
		pXtag->m_strPretext = pN->TxtPtxt() ;
		pXtag->m_Tag = pN->TxtName() ;
		pXtag->m_strContent = pN->m_fixContent ;
		pXtag->m_Line = pN->Line() ;
		pXtag->m_Indent = pN->Level() ;
	}

	//	Tags mostly applicable to forms

	else if	(pN->NameEQ("xformDef"))
	{
		m_pLog->Out("%s. File %s Line %d: invoked _readFormDef by %s\n", *_fn, pN->Fname(), pN->Line(), tga->m_pCaller) ;
		bBlock = true ;
		thisVE = _readFormDef(pN, tga->m_pLR) ;
	}
	else if	(pN->NameEQ("xformRef"))
	{
		bBlock = true ;
		thisVE = _readFormRef(pN, tga->m_pLR) ;
	}

	else if (pN->NameEQ("xfield"))
	{
		if (!tga->m_pFormdef)
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: Error: <xfield> not within <xformDef> (caller %s)\n", *_fn, pN->Fname(), pN->Line(), tga->m_pCaller) ; }
		thisVE = _readField(pN, tga->m_pFormdef) ;
	}

	else if (pN->NameEQ("xhide"))
	{
		if (!tga->m_pFormdef)
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: Error: <xhide> not within <xformDef> (caller %s)\n", *_fn, pN->Fname(), pN->Line(), tga->m_pCaller) ; }
		thisVE = _readXhide(pN, tga->m_pFormdef) ;
	}

	else if (pN->NameEQ("recaptcha"))
	{
		if (!tga->m_pFormdef)
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: Error: <recaptcha> not within <xformDef> (caller %s)\n", *_fn, pN->Fname(), pN->Line(), tga->m_pCaller) ; }
		else
		{
			thisVE = pRecap = new hdsRecap(this) ;
			thisVE->m_Line = pN->Line() ;
			thisVE->m_Indent = pN->Level() ;
			thisVE->m_Tag = pN->TxtName() ;

			pPage->m_bScriptFlags |= INC_SCRIPT_RECAPTCHA ;
			tga->m_pFormdef->m_bScriptFlags |= INC_SCRIPT_RECAPTCHA ;
		}
		m_pLog->Out("%s. Added recapture placement\n", *_fn) ;
		//	bBlock = true ;
	}

	else if (pN->NameEQ("xlinkBut"))
	{
		//	The link button manifests as a button but is purely a link. Common in pages but in forms is usually limited to triggering help popups and tha abort
		//	button for abandoning an edit. <xlinkBut> in a form is thus considered as part of the supporting HTML of the form. The <xlinkBut> tag does not add
		//	a form action and so it is not necessary for there to be a form definition in progress.

		for (ai = pN ; ai.Valid() ; ai.Advance())
		{
			if		(ai.NameEQ("title"))	title = ai.Value() ;
			else if (ai.NameEQ("goto"))		page = ai.Value() ;
			else if (ai.NameEQ("css"))		css = ai.Value() ;
			else
			{
				bErr=1 ;
				m_pLog->Out("%s. File %s Line %d Adding <%s> tag: Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName(), ai.Name(), ai.Value()) ;
			}
		}

		if (!title)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xlinkBut> No title supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
		if (!page)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xlinkBut> No destination supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

		thisVE = pButt = new hdsButton(this) ;
		thisVE->m_strPretext = pN->TxtPtxt() ;
		thisVE->m_Line = pN->Line() ;
		thisVE->m_Indent = pN->Level() ;
		thisVE->m_Tag = pN->TxtName() ;
		pButt->m_strContent = title ;
		pButt->m_Linkto = page ;
		pButt->m_CSS = css ;
		m_pLog->Out("%s. Added link button (%s)\n", *_fn, *pButt->m_strContent) ;
	}

	else if (pN->NameEQ("xformBut"))
	{
		thisVE = _readFormBut(pN, tga->m_pFormdef, tga->m_pFormref) ;

		//	The <xformBut> adds an action to a form definition and thus a form definition must be supplied

		/*
		if (!tga->m_pFormdef)
			{ m_pLog->Out("%s. File %s Line %d <xformBut> No form definition applies\n", *_fn, pN->Fname(), pN->Line()) ; return 0 ; }

		for (ai = pN ; ai.Valid() ; ai.Advance())
		{
			if		(ai.NameEQ("title"))	title = ai.Value() ;
			else if (ai.NameEQ("handler"))	hname = ai.Value() ;
			else if (ai.NameEQ("url"))		faUrl = ai.Value() ;
			else if (ai.NameEQ("css"))		css = ai.Value() ;
			else
			{
				bErr=1 ;
				m_pLog->Out("%s. File %s Line %d Adding <%s> tag: Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName(), ai.Name(), ai.Value()) ;
			}
		}

		if (!title)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xformBut> No title supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

		if (hname || faUrl)
		{
			//	If either action or URL are supplied, both must be
			if (!hname)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> URL but no action supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
			if (!faUrl)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> Action but no URL supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
		}

		if (hname && faUrl)
		{
			if (!m_FormHdls.Exists(hname))
			{
				m_FormHdls.Insert(hname,0) ;
				tga->m_pFormdef->m_nActions++ ;
			}
			m_FormUrl2Hdl.Insert(faUrl,hname) ;
			m_FormUrl2Ref.Insert(faUrl, tga->m_pFormref) ;
			m_FormRef2Url.Insert(tga->m_pFormref, faUrl) ;
		}

		if (!hname && !faUrl)
		{
			//	OK for the button to have no action or submission URL as long as the form definition has these and the form is single action.
			if (!tga->m_pFormdef->m_DfltAct)
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xformBut> No action supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
			else
			{
				hname = tga->m_pFormdef->m_DfltAct ;
				faUrl = tga->m_pFormdef->m_DfltURL ;
				tga->m_pFormdef->m_DfltAct = 0 ;
				tga->m_pFormdef->m_DfltURL = 0 ;
			}
		}

		if (bErr)
			return 0 ;

		//	if (page)
		//	{
			//	Lookup the page and check that it exists and is not the same as the page in which the button appears
			//	if (page != "%x_referer;" && !m_PagesName.Exists(page))
				//	m_pLog->Out("%s. Warning: File %s Line %d Adding <button> tag: Page %s not found\n", *_fn, pN->Fname(), pN->Line(), *page) ;
		//	}

		thisVE = pButt = new hdsButton(this) ;
		thisVE->m_strPretext = pN->TxtPtxt() ;
		thisVE->m_Line = pN->Line() ;
		thisVE->m_Indent = pN->Level() ;
		thisVE->m_Tag = pN->TxtName() ;
		thisVE->m_Resv = tga->m_pFormdef->m_nActions ;
		pButt->m_strContent = title ;
		pButt->m_CSS = css ;
		pButt->m_Linkto = hname ;
		pButt->m_Formname = tga->m_pFormdef->m_Formname ;
		m_pLog->Out("%s. Added form button (%s)\n", *_fn, *pButt->m_strContent) ;
		*/
	}

	else
	{
		//	Tag must be a valid HTML tag

		name = pN->TxtName() ;
		if (Txt2Tagtype(name) == HTAG_NULL)
		{
			m_pLog->Out("%s. File %s Line %d: Illegal tag <%s> (caller %s)\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName(), tga->m_pCaller) ;
			return 0 ;
		}

		//m_pLog->Out("%s. File %s Line %d: Doing Tag <%s> (caller %s)\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName(), tga->m_pCaller) ;

		//	Deal with the special case of the <input> tag as this will add to the host form's fields ???
		thisVE = pHtag = new hdsHtag(this) ;
		pHtag->m_strPretext = pN->TxtPtxt() ;
		pHtag->m_Tag = pN->TxtName() ;
		pHtag->m_strContent = pN->m_fixContent ;
		pHtag->m_Line = pN->Line() ;
		pHtag->m_Indent = pN->Level() ;

		for (ai = pN ; ai.Valid() ; ai.Advance())
		{
			//	Check first for JavaScript events
			if (_hzGlobal_JS_Events.Exists(ai.Name()))
			{
				//	Add JS function to list for page. This will later be checked against standard Dissemino scripts and scripts appearing within the <xscript> tag
				if (pPage)
					pPage->m_Scripts.Add(ai.Value()) ;
			}

			rc = pHtag->AddAttr(ai.Name(), ai.Value()) ;
			if (rc == E_SYNTAX)
			{
				m_pLog->Out("%s. File %s Line %d (%d) Malformed percent entity in tag attribute\n", *_fn, pN->Fname(), pN->Line(), thisVE->m_Line) ;
				delete pHtag ;
				return 0 ;
			}

			if (ai.NameEQ("href"))
				m_Links.Insert(ai.Value()) ;
		}
	}

	if (!thisVE)
	{
		m_pLog->Out("%s. File %s Line %d: Failed to proces <%s> tag\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ;
		return 0 ;
	}

	if (!thisVE->m_Line)	{ m_pLog->Out("%s. File %s Line %d. Visible entity %s has no line number\n", *_fn, pN->Fname(), pN->Line(), *thisVE->m_Tag) ; return 0 ; }
	if (!thisVE->m_Indent)	{ m_pLog->Out("%s. File %s Line %d. Visible entity %s has no indentation\n", *_fn, pN->Fname(), pN->Line(), *thisVE->m_Tag) ; return 0 ; }

	//	if (bBlock)
	//		return thisVE ;

	//	If a parent tag supplied, add this entity to the parent list of childen
	if (parent)
		parent->AddChild(thisVE) ;

	//	If current tag has sub-tags, recurse to process them
	if (!bBlock)
	{
		for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
		{
			newVE = _readTag(tga, pN1, bScrFlags, thisVE, level+1) ;
			if (!newVE)
				return 0 ;
		}
	}

	//	If the visible entity has content, we strip leading tabs and insert remainder into m_Strings (the universal collection of strings). Then the content is tested for percent
	//	entities. If any are found the VE is deemed active.

	if (thisVE->m_strContent)
	{
		//	Case where visible entity content is in the form of a string

		for (i = thisVE->m_strContent ; *i && *i < CHAR_SPACE ; i++) ;
		if (!*i)
			thisVE->m_strContent.Clear() ;
		else
		{
			for (i = thisVE->m_strContent ; *i ; i++)
			{
				if (*i == CHAR_PERCENT)
				{
					if (i[1] == CHAR_PERCENT)
						{ i++ ; continue ; }
					if (IsAlpha(i[1]) && i[2] == CHAR_COLON)
					{
						if (IsPcEnt(pcntEnt, i))
							thisVE->m_flagVE |= VE_CT_ACTIVE ;
						else
							{ bErr=1 ; m_pLog->Out("%s. File %s Line %d (%d) Malformed percent entity in tag content\n", *_fn, pN->Fname(), pN->Line(), thisVE->m_Line) ; }
						i += 2 ;
					}
				}
			}
		}
	}

	if (thisVE->m_strPretext)
	{
		for (i = thisVE->m_strPretext ; *i && *i < CHAR_SPACE ; i++) ;
		if (!*i)
			thisVE->m_strPretext.Clear() ;
		else
		{
			for (i = thisVE->m_strPretext ; *i ; i++)
			{
				if (*i == CHAR_PERCENT)
				{
					if (i[1] == CHAR_PERCENT)
						{ i++ ; continue ; }
					if (IsAlpha(i[1]) && i[2] == CHAR_COLON)
					{
						if (IsPcEnt(pcntEnt, i))
							thisVE->m_flagVE |= VE_PT_ACTIVE ;
						else
							{ bErr=1 ; m_pLog->Out("%s. File %s Line %d (%d) Malformed percent entity in tag pretext\n", *_fn, pN->Fname(), pN->Line(), thisVE->m_Line) ; }
						i += 2 ;
					}
				}
			}
		}
	}

	thisVE->Complete() ;

	return bErr ? 0 : thisVE ;
}

/*
**	SECTION 2:	Database and Data Class Config Read Functions
*/

hzEcode	hdsApp::_readDataEnum	(hzXmlNode* pN)
{
	//	Database and Data Class Config Read Functions
	//
	//	Process the <selector> tag to create a hdbSelector instance. The tag has attributes of name and mult (to indicate if multiple items can be selected).
	//
	//	As stated in the hdbSelector class description, instances comprise a one-to-one mapping between enumerated values to be stored in the database and their
	//	associated 'names' strings. A hdbSelector IS a data type in its own right and this means it can be the data type of a member of a data class. Of course
	//	it also means the name given to the hdbSelector in the <selector> tag, must be unique not just among selectors but all data types.
	//
	//	For the purpose of clarity, neither the <selector> tag nor the hbdSelector instance it specifies, are the same thing as a HTML <select> tag. The latter
	//	can appear in HTML output in respect of the hdbSlector, only when the HTML for a form having a field whose data type is the hdbSelctor, is produced. It
	//	should be noted, that the hdbSelector may not necessarily manifest as a HTML <select> tag at all. It could manifest as a set of check boxes or as a set
	//	of radio buttons.
	//
	//	Because in processing the <selector> tag, we are only defining the set of strings, we cannot say at this stage, what the default should be. That has to
	//	be done in the configs for the field within the form.
	//
	//	Likewise the option for multiple selection is not vested with the hdbEnum. That is a matter for any data class member that uses the hdbEnum as it type.
	//	items may be selected from it when it is manifest in a form.
	//
	//	The only thing that is determined here is whether the enum is fixed once loaded with values - or can have values added during runtime.
	//
	//	Arguments:	1)	pN	Pointer to XML node <selector>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <selector> parameters are valid

	_hzfunc("hdsApp::_readDataEnum") ;

	hzArray<hzString>	ar ;		//	Vect of items found so far
	hzSet<hzString>		items ;		//	Set of items found so far to ensure no repeats

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	For iteraton of options
	hdbEnum*		pSlct ;			//	The selector to be added
	hzString		S ;				//	Temp string
	uint32_t		strNo ;			//	Dictionary string number
	uint32_t		n ;				//	Iterator
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;

	pSlct = new hdbEnum() ;

	//	Type parameters (name)
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if (ai.NameEQ("name"))
			pSlct->SetTypename(ai.Value()) ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d Reading <selector> tag: Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	//	Check all attributes have been supplied
	if (!pSlct->StrTypename())
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d Reading <selector> tag: No name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (m_ADP.GetDatatype(pSlct->StrTypename()))
		{ rc = E_DUPLICATE ; m_pLog->Out("%s. File %s Line %d: Name %s already data-type\n", *_fn, pN->Fname(), pN->Line(), pSlct->TxtTypename()) ; }

	//	Selector Items
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("option"))
		{
			ai = pN1 ;
			if (!ai.Valid())
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d Tag <option> requires an attr of 'show' or 'dflt'\n", *_fn, pN->Fname(), pN1->Line()) ; break ; }

			if (ai.NameEQ("dflt") || ai.NameEQ("show"))
			{
				if (ai.NameEQ("dflt"))
					pSlct->m_Default = pSlct->m_Items.Count() ;

				if (items.Exists(ai.Value()))
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: Duplicate option of %s\n", *_fn, pN->Fname(), pN->Line(), ai.Value()) ; break ; }
				items.Insert(ai.Value()) ;

				S = ai.Value() ;
				if (S.Length() > pSlct->m_nMax)
					pSlct->m_nMax = S.Length() ;

				strNo = _hzGlobal_StringTable->Locate(ai.Value()) ;
				if (!strNo)
					strNo = _hzGlobal_StringTable->Insert(ai.Value()) ;

				pSlct->m_Items.Add(strNo) ;
				continue ;
			}

			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d Invalid attr (%s) for <option> tag\n", *_fn, pN->Fname(), pN->Line(), ai.Name()) ;
		}
	}

	if (rc == E_OK && !items.Count())
	{
		//	The enum should be comma-separated in the <enum> tag value
		SplitCSV(ar, pN->m_fixContent) ;

		for (n = 0 ; n < ar.Count() ; n++)
		{
			S = ar[n] ;

			if (items.Exists(S))
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: Duplicate option of %s\n", *_fn, pN->Fname(), pN->Line(), *S) ; break ; }
			items.Insert(S) ;

			if (S.Length() > pSlct->m_nMax)
				pSlct->m_nMax = S.Length() ;

			strNo = _hzGlobal_StringTable->Locate(*S) ;
			if (!strNo)
				strNo = _hzGlobal_StringTable->Insert(*S) ;

			pSlct->m_Items.Add(strNo) ;
		}
	}

	if (rc != E_OK)
		{ delete pSlct ; return rc ; }

	//	Add the selector
	if (rc == E_OK)
	{
		m_ADP.RegisterDataEnum(pSlct) ;

		m_pLog->Out("Added data enums %s\n", pSlct->TxtTypename()) ;
	}

	return rc ;
}

hzEcode	hdsApp::_readUser	(hzXmlNode* pN)
{
	//	Database and Data Class Config Read Functions
	//
	//	A user class is a special case of data class which defines data class members for a particular type of user for the application. Normally, creation of a
	//	data class is a separate act from creating a repository. All classes declared should be used to create repositories otherwise there is no point defining
	//	them, but a class can be used to form a member of another class. This is not the case with a user class. For each <user> tag in the config files, there
	//	will be exactly one user class and one user repository and the repository will bear the same name as the class. It is not possible to use a user class
	//	as a member of another class.
	//
	//	This function adds the user class to m_ADP.mapClasses and adds the user repository to m_ADP.mapRepositories. The user repository must be
	//	cached so storage method is not an attribute of the <user> tag as it is with <repos>. The user class must also name exactly one member to serve as the
	//	primary unique identifier (username). This is specified as an attribute of <user> namely 'primeKey'.
	//
	//	Arguments:	1)	pN	Pointer to XML node <user>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_NOTFOUND	If there is not a setting of project param m_UserBase
	//				E_OK		If the <user> parameters are valid

	_hzfunc("hdsApp::_readUser") ;

	const hdbMember*	pMbr ;	//	Class member

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Class member subnodes
	hdbClass*		pClass ;		//	The class
	hdbObjRepos*	pRepos ;		//	The object cache/store
	hzString		cnam ;			//	Class or user class name
	hzString		prime ;			//	Name of member to be used as primary key
	hzString		memname ;		//	User class member name
	hzString		S ;				//	Intermeadiate string
	hzEcode			rc = E_OK ;		//	Return code

	//	Must be a node
	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;
	if (!pN->NameEQ("user"))
		{ m_pLog->Out("%s. File %s Line %d. Expected <user>. Tag <%s> unexpected\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return E_SYNTAX ; }

	if (!(m_OpFlags & DS_APP_SUBSCRIBERS))
	{
		m_pLog->Out("%s. File %s Line %d. <user> tag without a prior declaration of 'userbase' as a project param. Assuming value of 'subscriber'\n",
			*_fn, pN->Fname(), pN->Line()) ;
		return E_NOTFOUND ;
	}

	//	Read class or user class params from attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("type"))		cnam = ai.Value() ;
		else if (ai.NameEQ("prime"))	prime = ai.Value() ;
		else
		{
			m_pLog->Out("%s. File %s Line %d <user> Only <type|prime> attributes allowed. Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
			return E_SYNTAX ;
		}
	}

	//	Check all atributes are supplied
	if (!cnam)	{ m_pLog->Out("%s. File %s Line %d <class> No user type supplied\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }
	if (!prime)	{ m_pLog->Out("%s. File %s Line %d <class> No primary key supplied\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }

	//	Check user class is unique among classes
	if (m_ADP.GetPureClass(cnam))
		{ m_pLog->Out("%s. File %s Line %d <user> Already have class of %s\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; return E_SYNTAX ; }
	if (m_ADP.GetObjRepos(cnam))
		{ m_pLog->Out("%s. File %s Line %d <user> Already have repository of %s\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; return E_SYNTAX ; }

	//	Add the user type and set access code
	AddUserType(cnam) ;
	if (rc != E_OK)
		{ m_pLog->Out("%s. File %s Line %d <user> Already have user type of %s\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; return E_SYNTAX ; }

	//	Read class members from subnodes
	pClass = new hdbClass(m_ADP, HDB_CLASS_DESIG_USR) ;
	pClass->InitStart(cnam) ;

	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("member"))
		{
			rc = _readMember(pClass, pN1) ;
			if (rc != E_OK)
				m_pLog->Out("%s. File %s Line %d <member> error\n", *_fn, pN->Fname(), pN1->Line()) ;
			continue ;
		}

		rc = E_SYNTAX ;
		m_pLog->Out("%s. File %s Line %d Reading <user> tag: Subnode <%s> invalid\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
	}

	if (rc == E_OK)
	{
		if (!pClass->MbrCount())
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: Class %s has no members\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; }
	}

	if (rc == E_OK)
	{
		pClass->InitDone() ;
		pMbr = pClass->GetMember(prime) ;

		if (!pMbr)
		{
			m_pLog->Out("%s. File %s Line %d: No index. Member of %s not found in user class %s\n", *_fn, pN->Fname(), pN->Line(), *prime, pClass->TxtTypename()) ;
			rc = E_SYNTAX ;
		}
	}

	if (rc == E_OK)
		rc = m_ADP.RegisterDataClass(pClass) ;

	if (rc == E_OK)
	{
		//	Set up repository
		pRepos = new hdbObjCache(m_ADP) ;
		pRepos->InitStart(pClass, pClass->StrTypename(), m_Datadir) ;

		m_ADP.RegisterObjRepos(pRepos) ;

		rc = pRepos->InitMbrIndex(prime, true) ;
		if (rc != E_OK)
			m_pLog->Out("%s. File %s Line %d: ERROR Cannot add %s indexable member\n", *_fn, pN->Fname(), pN->Line(), *S) ;
		else
			m_pLog->Out("%s. Added index %s to cache\n", *_fn, *S) ;

		if (rc == E_OK)
		{
			rc = pRepos->InitDone() ;
			if (rc != E_OK)
				m_pLog->Out("%s. Could not finalize class %s. Err=%s\n", *_fn, *cnam, Err2Txt(rc)) ;
		}
	}

	if (rc == E_OK)
		m_pLog->Out("%s. Completed User Class %p %s\n", *_fn, pClass, *cnam) ;
	else
		m_pLog->Out("%s. Failed User Class %s\n", *_fn, *cnam) ;
	return rc ;
}

hzEcode	hdsApp::_readClass	(hzXmlNode* pN)
{
	//	Database and Data Class Config Read Functions
	//
	//	Process a <class> tag to define a data class. The class may then be used to initialize a repository, or used as a member of another class which may then
	//	be used to initialize a repository.
	//
	//	Arguments:	1)	pN	Pointer to XML node <class>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <class> parameters are valid

	_hzfunc("hdsApp::_readClass") ;

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Class member subnodes
	hdbClass*		pClass ;		//	The class
	//hdsUsertype		utype ;			//	User type
	hzString		cnam ;			//	Class or user class name
	hzString		cat ;			//	Class category (optional, used to drive page/form formation)
	hzString		repos ;			//	Used to direct which form of repository (if any) should be created (hdbObjCache/hdbObjStore/none)
	hzString		memname ;		//	User class member name
	hzString		S ;				//	Intermeadiate string
	hzEcode			rc = E_OK ;		//	Return code

	//	Must be a node
	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;
	if (!pN->NameEQ("class"))
		{ m_pLog->Out("%s. File %s Line %d. Expected <class>. Tag of <%s> unexpected\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return E_SYNTAX ; }

	//	Read class or user class params from attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("cat"))	cat = ai.Value() ;
		else if (ai.NameEQ("name"))	cnam = ai.Value() ;
		else
			{ m_pLog->Out("%s. File %s Line %d <class> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; return E_SYNTAX ; }
	}

	if (!cnam)
		{ m_pLog->Out("%s. File %s Line %d <class> No class name supplied\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }
	if (m_ADP.GetPureClass(cnam))
		{ m_pLog->Out("%s. File %s Line %d <class> Already have class of %s\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; return E_SYNTAX ; }
	if (m_ADP.GetDatatype(cnam))
		{ m_pLog->Out("%s. File %s Line %d <class> Already have datatype of %s\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; return E_SYNTAX ; }

	//	Read class members from subnodes
	pClass = new hdbClass(m_ADP, HDB_CLASS_DESIG_CFG) ;
	pClass->InitStart(cnam) ;
	pClass->m_Category = cat ;

	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("member"))
		{
			rc = _readMember(pClass, pN1) ;
			if (rc != E_OK)
				m_pLog->Out("%s. File %s Line %d <member> error\n", *_fn, pN->Fname(), pN1->Line()) ;
			continue ;
		}

		rc = E_SYNTAX ;
		m_pLog->Out("%s. File %s Line %d Reading <class> tag: Subnode <%s> invalid\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
	}

	if (rc == E_OK)
	{
		if (!pClass->MbrCount())
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: Class %s has no members\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; }
		m_pLog->Out("%s. Class %s has %d members\n", *_fn, *cnam, pClass->MbrCount()) ;
	}

	if (rc == E_OK)
	{
		pClass->InitDone() ;
	
		rc = m_ADP.RegisterDataClass(pClass) ;
	}

	if (rc == E_OK)
		m_pLog->Out("%s. Completed Class %p %s\n", *_fn, pClass, *cnam) ;
	else
		m_pLog->Out("%s. Failed Class %s\n", *_fn, *cnam) ;
	return rc ;
}

hzEcode	hdsApp::_readRepos	(hzXmlNode* pN)
{
	//	Database and Data Class Config Read Functions
	//
	//	Process the <repos> tag which to construct a repository.
	//
	//	Arguments:	1)	pN	Pointer to XML node <repos>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <repos> parameters are valid

	_hzfunc("hdsApp::_readRepos") ;

	const hdbClass*		pClass ;		//	The class
	hzAttrset			ai ;			//	Attribute iterator
	hzXmlNode*			pN1 ;			//	Class member subnodes
	hdbObjRepos*		pRepos = 0 ;	//	The object cache/store
	//hdsUsertype			utype ;			//	User type
	hzString			rnam ;			//	Repository name
	hzString			cnam ;			//	Class name
	hzString			method ;		//	Used to direct which form of repository should be created (hdbObjCache/hdbObjStore)
	hzString			memName ;		//	User class member name
	hzString			S ;				//	Intermeadiate string
	bool				bUnique ;		//	Index is unique
	hzEcode				rc = E_OK ;		//	Return code

	//	Must be a node
	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;

	if (!pN->NameEQ("repos"))
	{
		m_pLog->Out("%s. File %s Line %d. Expected <repos>. Tag of <%s> unexpected\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ;
		return E_SYNTAX ;
	}

	//	Read class or user class params from attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))		rnam = ai.Value() ;
		else if (ai.NameEQ("class"))	cnam = ai.Value() ;
		else if (ai.NameEQ("method"))	method = ai.Value() ;
		else
			{ m_pLog->Out("%s. File %s Line %d <repos> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; return E_SYNTAX ; }
	}

	if (!rnam)
		{ m_pLog->Out("%s. File %s Line %d <repos> No repository name supplied\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }
	if (!cnam)
		{ m_pLog->Out("%s. File %s Line %d <repos> No class name supplied\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }
	if (!method)
		{ m_pLog->Out("%s. File %s Line %d <repos> No store method supplied\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }

	if (m_ADP.GetObjRepos(rnam))
		{ m_pLog->Out("%s. File %s Line %d <repos> Repository %s already exists\n", *_fn, pN->Fname(), pN->Line(), *rnam) ; return E_SYNTAX ; }

	pClass = m_ADP.GetPureClass(cnam) ;
	if (!pClass)
		{ m_pLog->Out("%s. File %s Line %d <repos> No such class as %s\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; return E_SYNTAX ; }

	//	Create repository

	if (method == "cache")
	{
		pRepos = new hdbObjCache(m_ADP) ;
		pRepos->InitStart(pClass, pClass->StrTypename(), m_Datadir) ;
	}

	//	Add indexes
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("index"))
		{
			//	Note that a name member is to be indexed
			memName = (char*) 0 ;
			bUnique = false ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if (ai.NameEQ("member"))
				{
					if (!memName)
						memName = ai.Value() ;
					else
						{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <idx> tag, attr member must be set only once", *_fn, pN->Fname(), pN1->Line()) ; }
				}
				else if (ai.NameEQ("unique"))
				{
					if (ai.ValEQ("true"))
						bUnique = true ;
					else if (ai.ValEQ("false"))
						bUnique = false ;
					else
						{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <idx> tag, attr unique must be either true or false", *_fn, pN->Fname(), pN1->Line()) ; }
				}
				else
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d <index>: member|unique - Bad param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ;
				}
			}

			if (rc != E_OK)
				break ;

			rc = pRepos->InitMbrIndex(memName, bUnique) ;
			if (rc != E_OK)
				{ m_pLog->Out("%s. File %s Line %d: ERROR Cannot add %s indexable member\n", *_fn, pN->Fname(), pN->Line(), *S) ; break ; }
			continue ;
		}

		rc = E_SYNTAX ;
		m_pLog->Out("%s. File %s Line %d Reading <class> tag: Subnode <%s> invalid\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
	}

	if (rc == E_OK)
	{
		rc = pRepos->InitDone() ;
		if (rc != E_OK)
			m_pLog->Out("%s. Could not finalize repository %s. Err=%s\n", *_fn, *cnam, Err2Txt(rc)) ;
		else
		{
			m_ADP.RegisterObjRepos(pRepos) ;
			m_pLog->Out("%s. Completed Repository %s\n", *_fn, *rnam) ;
		}
	}

	return rc ;
}

hzEcode	hdsFldspec::Validate	(hzLogger* pLog, const hzString& cfgFname, const char* caller, uint32_t ln)
{
	//	Database and Data Class Config Read Functions
	//
	//	Validate a field specification
	//
	//	Arguments:	1) pSpec		The field specification to test
	//				2) cfgFname		The source config filename
	//				3) ln			The source config line number
	//
	//	Returns:	E_ARGUMENT	If no field specification, config file or caller is supplied.
	//				E_SYNTAX	If there are any omissions or inconsistancies.
	//				E_OK		If the field specification is validated.

	_hzfunc("hdsApp::_testFldspec") ;

	hzEcode	rc = E_OK ;		//	Return code

	if (!cfgFname)	{ rc = E_ARGUMENT ; pLog->Out("%s. No config file supplied\n", *_fn) ; }
	if (!caller)	{ rc = E_ARGUMENT ; pLog->Out("%s. No caller func supplied\n", *_fn) ; }

	if (rc != E_OK)
		return rc ;

	if (!m_pType)
		{ pLog->Out("%s. File %s Line %d <fldspec> No data type established\n", caller, *cfgFname, ln) ; return E_SYNTAX ; }

	if (htype == HTMLTYPE_NULL)
		{ pLog->Out("%s. File %s Line %d <fldspec> No HTML type established\n", caller, *cfgFname, ln) ; return E_SYNTAX ; }

	if (!m_Refname)
		{ pLog->Out("%s. File %s Line %d <fldspec> No refname supplied\n", caller, *cfgFname, ln) ; return E_SYNTAX ; }

	switch	(htype)
	{
	case HTMLTYPE_TEXT:			//	Full range of printable chars
	case HTMLTYPE_PASSWORD:		//	As text but char's won't print
		if (nRows)
			pLog->Out("%s. File %s Line %d: Note. Rows for TEXT/PASSWORD are always 1 so not required\n", caller, *cfgFname, ln) ;
		if (!nCols)
				nCols = 1 ;
		if (!nSize)
			{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No max size supplied\n", caller, *cfgFname, ln) ; }
		nRows = 1 ;
		break ;

	case HTMLTYPE_TEXTAREA:		//	As text but a text area is described
		if (!nRows)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No row size supplied\n", caller, *cfgFname, ln) ; }
		if (!nCols)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No col size supplied\n", caller, *cfgFname, ln) ; }
		if (!nSize)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No max size supplied\n", caller, *cfgFname, ln) ; }
		break ;

	case HTMLTYPE_SELECT:		//	A HTML selector

		if (m_pType->Basetype() == BASETYPE_ENUM || m_pType->Basetype() == BASETYPE_STRING)
		{
			if (nRows || nCols || nSize)
				pLog->Out("%s. File %s Line %d: Note. For Select field dimensions are not required\n", caller, *cfgFname, ln) ;
			break ;
		}

		rc = E_SYNTAX ;
		pLog->Out("%s. File %s Line %d: <fldspec>: Data type of %s is incompatible to HTMLTYPE_SELECT\n",
			caller, *cfgFname, ln, m_pType->TxtTypename()) ;
		break ;

	case HTMLTYPE_CHECKBOX:		//	A HTML checkbox. Could be a boolean data type with just one checkbox or it could be a data enumeration meaning there will be
								//	one checkbox per item in the enumeration.
		if (nSize)
			pLog->Out("%s. File %s Line %d: Note. For Check-box field size is not required\n", caller, *cfgFname, ln) ;

		if (m_pType->Basetype() == BASETYPE_BOOL)
		{
			if (!nRows)	nRows = 1 ;
			if (!nCols)	nCols = 1 ;
			break ;
		}
		if (m_pType->Basetype() == BASETYPE_ENUM)
		{
			if (!nRows)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No row size supplied\n", caller, *cfgFname, ln) ; }
			if (!nCols)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No col size supplied\n", caller, *cfgFname, ln) ; }
			break ;
		}

		rc = E_SYNTAX ;
		pLog->Out("%s. File %s Line %d: <fldspec>: Data type of %s is incompatible to HTMLTYPE_CHECKBOX\n", m_pType->TxtTypename()) ;
		break ;

	case HTMLTYPE_RADIO:		//	A HTML radio button set must necessarily represent a data enumeration and have min/max population of 1
		if (nSize)
			pLog->Out("%s. File %s Line %d: Note. For Check-box/Radio field size is not required\n", caller, *cfgFname, ln) ;

		if (m_pType->Basetype() == BASETYPE_ENUM)
		{
			if (!nRows)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No row size supplied\n", caller, *cfgFname, ln) ; }
			if (!nCols)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No col size supplied\n", caller, *cfgFname, ln) ; }
			break ;
		}

		rc = E_SYNTAX ;
		pLog->Out("%s. File %s Line %d: <fldspec>: Data type of %s is incompatible to HTMLTYPE_RADIO\n", m_pType->TxtTypename()) ;
		break ;

	case HTMLTYPE_FILE:			//	File uploaded (as for text)
		if (nRows)
			pLog->Out("%s. File %s Line %d: Note. Rows for FILE are always 1 so not required\n", caller, *cfgFname, ln) ;
		if (!nCols)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No col size supplied\n", caller, *cfgFname, ln) ; }
		if (!nSize)	{ rc = E_SYNTAX ; pLog->Out("%s. File %s Line %d: No max size supplied\n", caller, *cfgFname, ln) ; }
		nRows = 1 ;
		break ;

	case HTMLTYPE_HIDDEN:		//	Hidden field
		if (nRows || nCols || nSize)
			pLog->Out("%s. File %s Line %d: Note. For Hidden field dimensions are not required\n", caller, *cfgFname, ln) ;
		break ;
	}

	return rc ;
}

hzEcode	hdsApp::_readFldspec	(hzXmlNode* pN)
{
	//	Database and Data Class Config Read Functions
	//
	//	Create a new field specification in accordance with the <fldspec> tag. The field specification defines what is needed to present the field as part of a
	//	HTML form.
	//
	//	Note that a field specification can be supplied as an attribute to the <member> tag used to define data class members within a <class> tag, exept where
	//	the intended member is itself a class. This is possible because the field specification contains where applicable, the fundamental data type of the data
	//	enum (hdbEnum instance).
	//
	//	This practice is recommended as a convienient short form and because it facilitates the generation of default forms and form-handlers when Dissemino is
	//	run with the -forms option.
	//
	//	Arguments:	1)	pN	Pointer to XML node <fldspec>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <fldspec> parameters are valid

	_hzfunc("hdsApp::_readFldspec") ;

	hzAttrset		ai ;			//	Attribute iterator
	hdsFldspec		fs ;			//	Field spec
	hzEcode			rc = E_OK ;		//	Return code

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))	fs.m_Refname = ai.Value() ;
		else if (ai.NameEQ("cols"))	fs.nCols = atoi(ai.Value()) ;
		else if (ai.NameEQ("rows"))	fs.nRows = atoi(ai.Value()) ;
		else if (ai.NameEQ("size"))	fs.nSize = atoi(ai.Value()) ;

		else if (ai.NameEQ("html"))
		{
			if		(ai.ValEQ("TEXT"))		fs.htype = HTMLTYPE_TEXT ;
			else if (ai.ValEQ("PASSWORD"))	fs.htype = HTMLTYPE_PASSWORD ;
			else if (ai.ValEQ("TEXTAREA"))	fs.htype = HTMLTYPE_TEXTAREA ;
			else if (ai.ValEQ("CHECKBOX"))	fs.htype = HTMLTYPE_CHECKBOX ;
			else if (ai.ValEQ("RADIO"))		fs.htype = HTMLTYPE_RADIO ;
			else if (ai.ValEQ("SELECT"))	fs.htype = HTMLTYPE_SELECT ;
			else if (ai.ValEQ("HIDDEN"))	fs.htype = HTMLTYPE_HIDDEN ;
			else if (ai.ValEQ("FILE"))		fs.htype = HTMLTYPE_FILE ;
			else
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <fldtype> tag: Invalid HTML type (%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Value()) ; }
		}
		else if (ai.NameEQ("enum"))
		{
			//	The attribute value will name the enum (note, not strictly necessary to use 'enum' one can use 'type' or 'fldtype')

			fs.m_pType = m_ADP.GetDataEnum(ai.Value()) ;
			if (!fs.m_pType)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <fldspec>: Invalid ENUM type (%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Value()) ; }
		}
		else if (ai.NameEQ("type") || ai.NameEQ("fldtype"))
		{
			//	The attribute value will name the datatype

			fs.m_pType = m_ADP.GetDatatype(ai.Value()) ;
			if (!fs.m_pType)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d Reading <fldspec> tag: Invalid DATA type (%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Value()) ; }
		}
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d Reading <fldspec> param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (rc == E_OK)
		rc = fs.Validate(m_pLog, pN->Fname(), *_fn, pN->Line()) ;

	if (rc != E_OK)
		m_pLog->Out("Failed to add fldspec %s dataType %s htype %s rows=%03d cols=%03d size=%04d\n",
			*fs.m_Refname, fs.m_pType->TxtTypename(), Htmltype2Txt(fs.htype), fs.nRows, fs.nCols, fs.nSize) ;
	else
	{
		//	Add Field Specification to collection
		m_Fldspecs.Insert(fs.m_Refname, fs) ;
		m_pLog->Out("Added fldspec %s dataType %s htype %s rows=%03d cols=%03d size=%04d\n",
			*fs.m_Refname, fs.m_pType->TxtTypename(), Htmltype2Txt(fs.htype), fs.nRows, fs.nCols, fs.nSize) ;
	}

	return rc ;
}

hzEcode	hdsApp::_readMember	(hdbClass* pClass, hzXmlNode* pN)
{
	//	Database and Data Class Config Read Functions
	//
	//	Process a <member> tag to read in details of a class member. This will be as part of reading in a data or user class definition under either a <class> or <user> tag so this
	//	function must only ever be called by _readClass() or _readUser().
	//
	//	The <member> tag contains database parameters necessary to define the data class member, namely member name, data type and population constraints. In addition, the <member>
	//	tag can supply parameters that direct how the member is to be displayed in HTML - This is for the purposes of enhancing default forms for the data class.
	//
	//	NOTE that if a member has a 'class' attribute, this must name a predefined data class. The member is then defined as having the named class as its data type.
	//
	//	Arguments:	1)	pClass	The data class for which a member is being specified
	//				1)	pN		Pointer to XML node <member>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <member> parameters are valid

	_hzfunc("hdsApp::_readMember") ;

	const hdbDatatype*	pType = 0 ;			//	Data type
	const hdbClass*		pComposite = 0 ;	//	Variable class (if applicable)
	const hdbMember*	pMbr ;				//	Class member
	//const hdbObjRepos*	pRepos = 0 ;		//	Predefined repository (if applicable)

	hdsFldspec		fs ;					//	Field specification that will be applied to member (will copy params from any supplied)
	hzAttrset		ai ;					//	Attribute iterator
	hdsFldspec*		pThisSpec = 0 ;			//	Pre-existing Field specification (if supplied)
	hzXmlNode*		pN1 ;					//	Child nodes
	hzString		str_Html ;				//	Html type
	hzString		str_Vnam ;				//	Variable name
	hzString		str_Title ;				//	Member title (As shown on scree before entry field)
	hzString		str_Desc ;				//	Member description
	hzString		str_Tab ;				//	Tab heading
	hzString		str_Spec ;				//	Field specification if supplied
	hzString		str_Src ;				//	Foreign key source - Specifies a pre-defined search to derive a HTML selector or other form of menu to aid filling in of the field.
	hzString		str_Lim ;				//	Min and Max number of values
	hzString		str_minPop ;			//	Min and Max number of values
	hzString		str_maxPop ;			//	Min and Max number of values
	hzString		str_Num_lo ;			//	Min value if supplied
	hzString		str_Num_hi ;			//	Max value if supplied
	hzString		str_FldSeq ;			//	Display order if supplied
	hzString		str_ExpSeq ;			//	Export order if supplied
	hzString		str_Dtype ;				//	Names a data type (expected alongside HTML display info)
	hzString		str_Class ;				//	Names a pre-defined class (must not have HTML display info)
	//hzString		str_Repos ;				//	Names a pre-defined class (must not have HTML display info)
	hzString		str_Context ;			//	Name of this class and member - used for assigning class delta ids
	uint32_t		minPop = 0 ;			//	Minimum number of values the member can have
	uint32_t		maxPop = 1 ;			//	Maximum number of values the member can have
	uint32_t		nRows = 0 ;				//	Display info: Number of rows
	uint32_t		nCols = 0 ;				//	Display info: Number of columns
	uint32_t		nSize = 0 ;				//	Display info: Max size (e.g of textarea)
	uint32_t		pCount = 0 ;			//	Count of parameters
	hzHtmltype		htype ;					//	HTML type
	bool			bHtml = false ;			//	Set if any HTML display params are supplied
	hzEcode			rc = E_OK ;				//	Return code

	if (!pClass)	Fatal("%s: No hdbClass supplied\n", *_fn) ;
	if (!pN)		Fatal("%s: No XML node supplied\n", *_fn) ;

	if (!pN->NameEQ("member"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <member>\n", *_fn, pN->TxtName()) ;

	htype = HTMLTYPE_NULL ;

	//	Read the parameters
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		//	Member name or class
		if		(ai.NameEQ("name"))		str_Vnam = ai.Value() ;
		else if (ai.NameEQ("class"))	str_Class = ai.Value() ;
		//else if (ai.NameEQ("repos"))	str_Repos = ai.Value() ;

		//	Population
		else if (ai.NameEQ("lim"))		str_Lim = ai.Value() ;
		else if (ai.NameEQ("minPop"))	str_minPop = ai.Value() ;
		else if (ai.NameEQ("maxPop"))	str_maxPop = ai.Value() ;

		//	Ranges
		else if (ai.NameEQ("numLo"))	str_Num_lo = ai.Value() ;
		else if (ai.NameEQ("numHi"))	str_Num_hi = ai.Value() ;

		//	Display and export order
		else if (ai.NameEQ("seq"))		str_FldSeq = ai.Value() ;
		else if (ai.NameEQ("exp"))		str_ExpSeq = ai.Value() ;

		//	Field spec referal
		else if (ai.NameEQ("spec"))		str_Spec = ai.Value() ;
		else if (ai.NameEQ("fldspec"))	str_Spec = ai.Value() ;

		//	OR field spec and data type explicit
		else if (ai.NameEQ("cols"))		{ bHtml = true ; nCols = ai.Value() ? atoi(ai.Value()) : 0 ; }
		else if (ai.NameEQ("rows"))		{ bHtml = true ; nRows = ai.Value() ? atoi(ai.Value()) : 0 ; }
		else if (ai.NameEQ("size"))		{ bHtml = true ; nSize = ai.Value() ? atoi(ai.Value()) : 0 ; }
		else if (ai.NameEQ("html"))		{ bHtml = true ; str_Html = ai.Value() ; }
		else if (ai.NameEQ("type"))		{ bHtml = true ; str_Dtype = ai.Value() ; }

		//	OR foreign entities (No fldspec if class specified)
		else if (ai.NameEQ("fldclass"))	str_Class = ai.Value() ;	//	Pre-defined class. No display data needed so no fldspec allowed

		//	Desc is optional but rejected if a fldspec is supplied
		else if (ai.NameEQ("desc"))		str_Desc = ai.Value() ;

		//	Tab is optional and wil be placed in the fldspec 
		else if (ai.NameEQ("tab"))		str_Tab = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }

		pCount++ ;
	}

	//	If parameters are defined in sub-nodes
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if		(pN1->NameEQ("tab"))	str_Tab = pN1->m_fixContent ;
		else if (pN1->NameEQ("title"))	str_Title = pN1->m_fixContent ;
		else if (pN1->NameEQ("desc"))	str_Desc = pN1->m_fixContent ;
		else if (pN1->NameEQ("source"))	str_Src = pN1->m_fixContent ;
		else if (pN1->NameEQ("range"))
		{
			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("numLo"))	str_Num_lo = ai.Value() ;
				else if (ai.NameEQ("numHi"))	str_Num_hi = ai.Value() ;
				else
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <range> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}
		}
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> Illegal subtag (%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
	}

	//	Name is compulsory unless a class or repos attribute has been supplied in which case name is not allowed
	//if (str_Vnam && (str_Class || str_Repos))
	if (str_Vnam && str_Class)
	{
		rc = E_SYNTAX ;
		m_pLog->Out("%s. File %s Line %d <member> If a member is named it cannot have an attribute of either 'class' or 'repos'\n", *_fn, pN->Fname(), pN->Line()) ;
	}

	if (!str_Vnam)
	{
		//	if (str_Class && str_Repos)
		//		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> Either 'class' or 'repos' is allowed, not both\n", *_fn, pN->Fname(), pN->Line()) ; }

		//	if (!str_Class && !str_Repos)
		if (!str_Class)
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member>, no name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	}

	//	Must be population data. Either 'lim' shorthand or both minPop and maxPop
	if (str_Lim)
	{
		if (str_minPop || str_maxPop)
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> 'lim' must not co-exist with min/max pop\n", *_fn, pN->Fname(), pN->Line()) ; }

		if		(str_Lim == "0,1")	{ minPop = 0 ; maxPop = 1 ; }
		else if (str_Lim == "0,m")	{ minPop = 0 ; maxPop = 0 ; }
		else if (str_Lim == "1,1")	{ minPop = 1 ; maxPop = 1 ; }
		else if (str_Lim == "1,m")	{ minPop = 1 ; maxPop = 0 ; }
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member>, Wrong Limit spec\n", *_fn, pN->Fname(), pN->Line()) ; }
	}
	else
	{
		if		(str_minPop && str_maxPop)		{ minPop = atoi(*str_minPop) ; maxPop = atoi(*str_maxPop) ; }
		else if (!str_minPop && !str_maxPop)	{ minPop = 0 ; maxPop = 1 ; }
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> without 'lim' both min/max pop are needed\n", *_fn, pN->Fname(), pN->Line()) ; }
	}

	//	Do we have a field spec?
	if (str_Spec)
	{
		//	Field spec supplied so we just copy this.
		if (m_Fldspecs.Exists(str_Spec))
			{ fs = m_Fldspecs[str_Spec] ; pType = fs.m_pType ; }
		else
			{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s fldspec %s not defined\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam, *str_Spec) ; }

		if (bHtml)
			{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s fldspec cannot co-exist with HTML info or data type\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam) ; }
		if (str_Src)
			{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s is a foreign class. No fldspec permitted\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam) ; }

		//	Init member
		rc = pClass->InitMember(str_Vnam, pType, minPop, maxPop) ;
		if (rc != E_OK)
			m_pLog->Out("%s. File %s Line %d: <member> Could not add atomic member %s to class %s\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam, pClass->TxtTypename()) ;
		else
			m_pLog->Out("%s. Added atomic member %s of type %s to class %s\n", *_fn, *str_Vnam, pType->TxtTypename(), pClass->TxtTypename()) ;

		if (rc == E_OK)
		{
			pThisSpec = new hdsFldspec() ;
			*pThisSpec = fs ;

			pMbr = pClass->GetMember(str_Vnam) ;
			pMbr->SetSpec(pThisSpec) ;
			pMbr->m_dsmTabSubject = str_Tab ;
		}
	}

	//	OR a class?
	//	else if (str_Class || str_Repos)
	else if (str_Class)
	{
		//	No field spec wanted and the member will in fact not use one.

		if (str_Class)
		{
			pType = pComposite = m_ADP.GetPureClass(str_Class) ;
			if (!pComposite)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s class %s not defined\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam, *str_Class) ; }
			str_Vnam = str_Class ;
		}

		/*
		if (str_Repos)
		{
			pRepos = m_ADP.GetObjRepos(str_Repos) ;
			if (pRepos)
			{
				pComposite = pRepos->Class() ;
				pType = pComposite ;
			}
			else
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s class %s not defined\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam, *str_Class) ; }
			str_Vnam = str_Repos ;
		}
		*/

		if (str_Spec)	{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s is a foreign class. No fldspec permitted\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam) ; }
		if (str_Src)	{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s is a foreign class. No source permitted\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam) ; }
		if (bHtml)		{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s is a foreign class. No HTML info permitted\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam) ; }

		if (rc == E_OK && pComposite)
		{
			rc = pClass->InitMember(str_Vnam, pComposite, minPop, maxPop) ;
			if (rc != E_OK)
				m_pLog->Out("%s. File %s Line %d: <member> Could not add composite variable %s to class %s\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam, pClass->TxtTypename()) ;
			else
				m_pLog->Out("%s. Added composite member %s of class %s to class %s\n", *_fn, *str_Vnam, pComposite->TxtTypename(), pClass->TxtTypename()) ;

			str_Context = pClass->TxtTypename() ;
			str_Context += "." ;
			str_Context += str_Vnam ;

			m_ADP.RegisterComposite(str_Context, pComposite) ;
		}
	}

	//	OR an explicit setting of data type and HTML info?
	else if (bHtml)
	{
		//	Must be a type and all the HTML info. Must not be a class or a supplied field spec (as that is created here)

		if (str_Spec)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> Cannot have fldspec with data type and HTML info\n", *_fn, pN->Fname(), pN->Line()) ; }
		if (str_Class)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> Cannot have class with data type and HTML info\n", *_fn, pN->Fname(), pN->Line()) ; }

		if (!str_Dtype)
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s No type supplied\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam) ; }
		else
		{
			pType = m_ADP.GetDatatype(str_Dtype) ;
			if (!pType)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s Type %s not defined\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam, *str_Dtype) ; }
		}

		if (!str_Html)
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member> %s No HTML type supplied\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam) ; }
		else
		{
			if		(str_Html == "TEXT")		htype = HTMLTYPE_TEXT ;
			else if (str_Html == "PASSWORD")	htype = HTMLTYPE_PASSWORD ;
			else if (str_Html == "TEXTAREA")	htype = HTMLTYPE_TEXTAREA ;
			else if (str_Html == "CHECKBOX")	htype = HTMLTYPE_CHECKBOX ;
			else if (str_Html == "RADIO")		htype = HTMLTYPE_RADIO ;
			else if (str_Html == "SELECT")		htype = HTMLTYPE_SELECT ;
			else if (str_Html == "HIDDEN")		htype = HTMLTYPE_HIDDEN ;
			else if (str_Html == "FILE")		htype = HTMLTYPE_FILE ;
			else
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <member>: Invalid HTML type (%s)\n", *_fn, pN->Fname(), pN->Line(), *str_Html) ; }
		}

		//	Validate HTML info
		switch	(htype)
		{
		case HTMLTYPE_TEXT:			//	Full range of printable chars
		case HTMLTYPE_PASSWORD:		//	As text but char's won't print

			if (nRows)
				m_pLog->Out("%s. File %s Line %d: Note. Rows for TEXT/PASSWORD are always 1 so not required\n", *_fn, pN->Fname(), pN->Line()) ;
			if (!nCols)
					nCols = 1 ;
			if (!nSize)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No max size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
			nRows = 1 ;
			break ;

		case HTMLTYPE_TEXTAREA:		//	As text but a text area is described

			if (!nRows)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No row size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
			if (!nCols)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No col size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
			if (!nSize)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No max size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
			break ;

		case HTMLTYPE_SELECT:		//	A HTML selector

			if (pType->Basetype() == BASETYPE_ENUM || pType->Basetype() == BASETYPE_STRING)
			{
				if (nRows || nCols || nSize)
					m_pLog->Out("%s. File %s Line %d: Note. For Select field dimensions are not required\n", *_fn, pN->Fname(), pN->Line()) ;
				break ;
			}

			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d: <fldspec>: Data type of %s is incompatible to HTMLTYPE_SELECT\n",
				*_fn, pN->Fname(), pN->Line(), pType->TxtTypename()) ;
			break ;

			case HTMLTYPE_CHECKBOX:		//	A HTML checkbox. Could be a boolean data type with just one checkbox or it could be a data enumeration meaning there will be
										//	one checkbox per item in the enumeration.
			if (nSize)
				m_pLog->Out("%s. File %s Line %d: Note. For Check-box field size is not required\n", *_fn, pN->Fname(), pN->Line()) ;

			if (pType->Basetype() == BASETYPE_BOOL)
			{
				if (!nRows)	nRows = 1 ;
				if (!nCols)	nCols = 1 ;
				break ;
			}
			if (pType->Basetype() == BASETYPE_ENUM)
			{
				if (!nRows)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No row size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
				if (!nCols)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No col size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
				break ;
			}

			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d: <fldspec>: Data type of %s is incompatible to HTMLTYPE_CHECKBOX\n", pType->TxtTypename()) ;
			break ;

		case HTMLTYPE_RADIO:		//	A HTML radio button set must necessarily represent a data enumeration and have min/max population of 1
			if (nSize)
				m_pLog->Out("%s. File %s Line %d: Note. For Check-box/Radio field size is not required\n", *_fn, pN->Fname(), pN->Line()) ;

			if (pType->Basetype() == BASETYPE_ENUM)
			{
				if (!nRows)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No row size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
				if (!nCols)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No col size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
				break ;
			}

			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d: <fldspec>: Data type of %s is incompatible to HTMLTYPE_RADIO\n", pType->TxtTypename()) ;
			break ;

		case HTMLTYPE_FILE:			//	File uploaded (as for text)

			if (nRows)
				m_pLog->Out("%s. File %s Line %d: Note. Rows for FILE are always 1 so not required\n", *_fn, pN->Fname(), pN->Line()) ;
			if (!nCols)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No col size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
			if (!nSize)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: No max size supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
			nRows = 1 ;
			break ;

		case HTMLTYPE_HIDDEN:		//	Hidden field

			if (nRows || nCols || nSize)
				m_pLog->Out("%s. File %s Line %d: Note. For Hidden field dimensions are not required\n", *_fn, pN->Fname(), pN->Line()) ;
			break ;
		}

		//	Init member
		rc = pClass->InitMember(str_Vnam, pType, minPop, maxPop) ;
		if (rc != E_OK)
			m_pLog->Out("%s. File %s Line %d: <member> Could not add atomic variable %s to class %s\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam, pClass->TxtTypename()) ;
		else
			m_pLog->Out("%s. Added atomic variable %s of type %s to class %s\n", *_fn, *str_Vnam, pType->TxtTypename(), pClass->TxtTypename()) ;

		//	Create the field spec.
		pThisSpec = new hdsFldspec() ;
		pThisSpec->m_Refname = pClass->StrTypename() + "." + str_Vnam ;

		if (nCols)		pThisSpec->nCols = nCols ;
		if (nRows)		pThisSpec->nRows = nRows ;
		if (nSize)		pThisSpec->nSize = nSize ;
		if (htype)		pThisSpec->htype = htype ;
		if (pType)		pThisSpec->m_pType = pType ;
		if (str_Title)	pThisSpec->m_Desc = str_Title ;
		if (str_Desc)	pThisSpec->m_Desc = str_Desc ;
		if (str_Tab)	pThisSpec->m_Tab = str_Desc ;
		if (str_Src)	pThisSpec->m_Source = str_Src ;

		pMbr = pClass->GetMember(str_Vnam) ;
		pMbr->SetSpec(pThisSpec) ;
		pMbr->m_dsmTabSubject = str_Tab ;
	}

	//	If none of the above then invalid
	else
	{
		rc = E_SYNTAX ;
		m_pLog->Out("%s. File %s Line %d Member %s Must have either fldspec, class, source OR data type plus HTML info\n", *_fn, pN->Fname(), pN->Line(), *str_Vnam) ;
	}

	return rc ;
}

/*
**	SECTION 3:	Tables and Dirlist Config Read Functions
*/

hzEcode	hdsApp::_readColumn	(hdsCol& col, hzXmlNode* pN)
{
	//	Category:	Tables and Dirlist Config Read Functions
	//
	//	Obtain the table columns

	_hzfunc("hdsApp::_readColumn") ;

	hzAttrset		ai ;			//	Attribute iterator
	hzString		css ;			//	Default css
	hzString		bgcol_head ;	//	Background color for column header
	hzString		bgcol_data ;	//	Background color for column data (even)
	hzString		bgcol_alt ;		//	Background color for column data (odd)
	hzString		bgcol ;			//	Default Background color
	hzEcode			rc = E_OK ;		//	Return code

	col.Clear() ;

	if (!pN->NameEQ("xcol"))
		{ m_pLog->Out("%s. File %s Line %d <xcol> Illegal subtag <%s> Only <xcol> allowed\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return E_SYNTAX ; }

	css = bgcol_head = bgcol_data = bgcol_alt = bgcol = 0 ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("title"))		col.m_Title = ai.Value() ;
		else if (ai.NameEQ("member"))		col.m_Member = ai.Value() ;
		else if (ai.NameEQ("css_head"))		col.m_CSS_head = ai.Value() ;
		else if (ai.NameEQ("css_data"))		col.m_CSS_data = ai.Value() ;
		else if (ai.NameEQ("css"))			css = ai.Value() ;
		else if (ai.NameEQ("bgcol_head"))	bgcol_head = ai.Value() ;
		else if (ai.NameEQ("bgcol_data"))	bgcol_data = ai.Value() ;
		else if (ai.NameEQ("bgcol_alt"))	bgcol_alt = ai.Value() ;
		else if (ai.NameEQ("bgcolor"))		bgcol = ai.Value() ;
		else if (ai.NameEQ("size"))			col.m_nSize = ai.Value() ? atoi(ai.Value()) : 0 ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xcol> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	//	Check column title
	if (!col.m_Title)
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xcol> No title suplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!col.m_Member)
		col.m_Member = col.m_Title ;

	//	Assert/check CSS settings
	if (css)
	{
		if (!col.m_CSS_head)	col.m_CSS_head = css ;
		if (!col.m_CSS_data)	col.m_CSS_data = css ;
	}
	if (!col.m_CSS_head)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xcol> No title suplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!col.m_CSS_data)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xcol> No title suplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	//	Assert/check background colors
	if (bgcol)
	{
		if (!bgcol_head)	bgcol_head = bgcol ;
		if (!bgcol_data)	bgcol_data = bgcol ;
		if (!bgcol_alt)		bgcol_alt = bgcol ;
	}
	if (!bgcol_head)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xcol> No bgcolor for head suplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!bgcol_data)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xcol> No bgcolor for data suplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!bgcol_alt)		bgcol_alt = bgcol_data ;

	col.m_bgcol_head = bgcol_head ? atoi(*bgcol_head) : 0 ;
	col.m_bgcol_data = bgcol_data ? atoi(*bgcol_data) : 0 ;
	col.m_bgcol_alt = bgcol_alt ? atoi(*bgcol_alt) : 0 ;

	return rc ;
}

hdsVE*	hdsApp::_readDirlist	(hzXmlNode* pN, hdsResource* pPage)
{
	//	Category:	Tables and Dirlist Config Read Functions
	//
	//	Read the <xdirlist> tag.
	//
	//	The <xdirlist> tag is specifically aimed at listing directory entries and compiling them into a HTML table. The <xdirlist> tags may be used in pages and
	//	articles although less recomended in the latter. <xdirlist> is an active tag and so any page or article containing an <xdirlist> is automatically deemed
	//	active and will always be generated.
	//
	//	The <xdirlist> tag has three compulsory attributes to specify the directory path, the filtering criteria and the sort order. It has other non-compulsory
	//	attributes to specify CSS class, the number of rows, row height and overall width.
	//
	//	The HTML table produced will automatically display a scroll bar on the right hand side if there are too many directory entries to fit the stated display
	//	area. The colunms to be displayed are all standard directory entry properties and are specified with the <xcol> sub-tag. The <ifnone> sub-tag specifies
	//	what HTML is be produced in the event of no entries found. There can also be <header> and <footer> sub-tags to define what HTML goes above and below the
	//	directory entry table.
	//
	//	Argument:	pN	The current XML node expected to be a <xdirlist> tag
	//
	//	Returns:	Pointer to diagram visual entity

	_hzfunc("hdsApp::_readDirlist") ;

	_tagArg			tga ;			//	Tag argument for _readTag()
	hdsCol			col ;			//	Column
	hzAttrset		ai ;			//	Attribute iterator
	hdsDirlist*		pDirlist ;		//	Table
	hzXmlNode*		pN1 ;			//	Subtag probe
	hzXmlNode*		pN2 ;			//	Subtag probe
	hdsVE*			thisVE ;		//	Visible entity/active entity
	uint32_t		flags ;			//	Needed for read tag function
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pPage)
		Fatal("%s. No page supplied\n", *_fn) ;

	thisVE = pDirlist = new hdsDirlist(this) ;
	thisVE->m_strPretext = pN->TxtPtxt() ;
	thisVE->m_Line = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;
	thisVE->m_flagVE |= VE_CT_ACTIVE ;

	//	Clear the column
	col.Clear() ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("directory"))	pDirlist->m_Directory = ai.Value() ;
		else if	(ai.NameEQ("criteria"))		pDirlist->m_Criteria = ai.Value() ;
		else if	(ai.NameEQ("css"))			pDirlist->m_CSS = ai.Value() ;
		else if (ai.NameEQ("rows"))			pDirlist->m_nRows = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("height"))		pDirlist->m_Height = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("width"))		pDirlist->m_Width = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if	(ai.NameEQ("order"))
		{
			if		(ai.ValEQ("name/asc"))	pDirlist->m_Order = 1 ;
			else if (ai.ValEQ("name/dec"))	pDirlist->m_Order = 2 ;
			else if (ai.ValEQ("date/asc"))	pDirlist->m_Order = 3 ;
			else if (ai.ValEQ("date/dec"))	pDirlist->m_Order = 4 ;
			else
			{
				rc = E_SYNTAX ;
				m_pLog->Out("%s. File %s Line %d: <xdirlist> Bad sort param. Only date/asc|date/dec|name/asc|name/dec allowed\n", *_fn, pN->Fname(), pN->Line()) ;
			}
		}
		else
		{
			m_pLog->Out("%s. File %s Line %d: <xdirlist> Only attrs allowed are method|source|criteria|rows|edit. Bad param (%s=%s)\n",
				*_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
			rc = E_SYNTAX ;
		}
	}

	if (!pDirlist->m_Directory)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Directory not suplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!pDirlist->m_Criteria)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Criteria not suplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!pDirlist->m_Order)		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Sort order not suplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (!pDirlist->m_Height)
		pDirlist->m_Height = 500 ;
	if (!pDirlist->m_Width)
		pDirlist->m_Width = 90 ;

	//	Obtain the table columns
	tga.m_pLR = pPage ;
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		pN2 = pN1->GetFirstChild() ;

		if (pN1->NameEQ("xcol"))
		{
			rc = _readColumn(col, pN1) ;

			if (rc == E_OK)
				pDirlist->m_Cols.Add(col) ;
		}
		else if (pN1->NameEQ("ifnone"))
		{
			//	Gather subtags to be rendered in the event of no files/directories found
			if (pDirlist->m_pNone)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Non-singular <ifnone>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }

			pDirlist->m_pNone = _readTag(&tga, pN2, flags, 0, 0) ;
			if (!pDirlist->m_pNone)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Empty <ifnone>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }
		}
		else if (pN1->NameEQ("header"))
		{
			//	Gather subtags to be rendered before the list in the event of files/directories found
			if (pDirlist->m_pHead)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Non-singular <header>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }

			pDirlist->m_pHead = _readTag(&tga, pN1, flags, 0, 0) ;
			if (!pDirlist->m_pHead)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Empty <header>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }
		}
		else if (pN1->NameEQ("footer"))
		{
			//	Gather subtags to be rendered after the list in the event of files/directories found
			if (pDirlist->m_pFoot)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Non-singular <footer>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }

			pDirlist->m_pFoot = _readTag(&tga, pN1, flags, 0, 0) ;
			if (!pDirlist->m_pFoot)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xdirlist> Empty <footer>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }
		}
		else
		{
			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d <xdirlist> Illegal subtag <%s> Only <xcol> allowed\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
		}
	}

	//	Check the colums for the correct file directives

	if (rc != E_OK)
		{ delete pDirlist ; return 0 ; }

	return thisVE ;
}

hdsVE*	hdsApp::_readTable	(hzXmlNode* pN, hdsResource* pPage)
{
	//	Category:	Tables and Dirlist Config Read Functions
	//
	//	Read the <xtable> tag.
	//
	//	The <xtable> tag defines how a list of items is to be compiled and presented as a HTML table. Currently the items can only be data objects selected from
	//	a repository. However the intention is to extend the remit of <xtable> to include other items. Directory entries will not be included however, as these
	//	would be handled by the <xdirlist> tag.
	//
	//	In its present form, the <xtable> tag has two compulsory attributes to specify the respository and the filtering criteria. It has other non-compulsory
	//	attributes to specify CSS class, the number of rows, row height and overall width.
	//
	//	The HTML table produced will automatically display a scroll bar on the right hand side if there are too many directory entries to fit the stated display
	//	area. The colunms to be displayed are all standard directory entry properties and are specified with the <xcol> sub-tag. The <ifnone> sub-tag specifies
	//	what HTML is be produced in the event of no entries found. There can also be <header> and <footer> sub-tags to define what HTML goes above and below the
	//	directory entry table.
	//
	//	Argument:	pN	The current XML node in the configs, expected to be a <xtable> tag
	//
	//	Returns:	Pointer to diagram visual entity

	_hzfunc("hdsApp::_readTable") ;

	const hdbMember*	pMbr ;	//	Data class member

	_tagArg			tga ;			//	Tag argument for _readTag()
	hdsCol			col ;			//	Column
	hzAttrset		ai ;			//	Attribute iterator
	const hdbClass*	pClass ;		//	Data class
	hdsTable*		pTable ;		//	Table
	hzXmlNode*		pN1 ;			//	Subtag probe
	hdsVE*			thisVE ;		//	Visible entity/active entity
	hzString		css ;			//	Default css
	hzString		bgcol_head ;	//	Background color for column header
	hzString		bgcol_data ;	//	Background color for column data (even)
	hzString		bgcol_alt ;		//	Background color for column data (odd)
	hzString		bgcol ;			//	Default Background color
	uint32_t		flags ;			//	Needed for read tag function
	uint32_t		bErr = 0 ;		//	Error condition
	hzEcode			rc = E_OK ;		//	Return code

	thisVE = pTable = new hdsTable(this) ;
	thisVE->m_strPretext = pN->TxtPtxt() ;
	thisVE->m_Line = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;
	thisVE->m_flagVE |= VE_CT_ACTIVE ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("repos"))	pTable->m_Repos = ai.Value() ;
		else if	(ai.NameEQ("criteria"))	pTable->m_Criteria = ai.Value() ;
		else if	(ai.NameEQ("css"))		pTable->m_CSS = ai.Value() ;
		else if (ai.NameEQ("rows"))		pTable->m_nRows = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("width"))	pTable->m_nWidth = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("height"))	pTable->m_nHeight = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("edit"))		pTable->m_bEdit = ai.ValEQ("true") ;
		else
		{
			bErr=1 ;
			m_pLog->Out("%s. File %s Line %d: <xtable> Only attrs allowed are method|source|criteria|rows|edit. Bad param (%s=%s)\n",
				*_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
		}
	}

	if (!pTable->m_Repos)		{ bErr=1; m_pLog->Out("%s. File %s Line %d <xtable> Source not suplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!pTable->m_Criteria)	{ bErr=1; m_pLog->Out("%s. File %s Line %d <xtable> Criteria not suplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (pTable->m_nWidth < 1 || pTable->m_nWidth > 100)
		{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xtable> Width is a percentage and must be between 1 and 100\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (pTable->m_nHeight < 100 || pTable->m_nHeight > 900)
		{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xtable> Hight (in pixels) must be between 100 and 900\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (bErr)
		{ delete pTable ; return 0 ; }

	pClass = m_ADP.GetPureClass(pTable->m_Repos) ;
	if (!pClass)
		{ bErr=1; m_pLog->Out("%s. File %s Line %d <xtable> Class not established for repos %s\n", *_fn, pN->Fname(), pN->Line(), *pTable->m_Repos) ; }
	else
	{
		pTable->m_pCache = (hdbObjCache*) m_ADP.GetObjRepos(pTable->m_Repos) ;
		if (!pTable->m_pCache)
		{
			bErr=1 ;
			m_pLog->Out("%s. File %s Line %d <xtable> Repository %s not defined\n", *_fn, pN->Fname(), pN->Line(), *pTable->m_Repos) ;
		}
	}

	if (bErr)
		{ delete pTable ; return 0 ; }

	//	Obtain the table columns
	tga.m_pLR = pPage ;
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("xcol"))
		{
			rc = _readColumn(col, pN1) ;

			if (rc == E_OK)
			{
				pMbr = pClass->GetMember(col.m_Member) ;
				if (!pMbr)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xcol> No such member as %s\n", *_fn, pN->Fname(), pN1->Line(), *col.m_Member) ; break ; }
				col.m_mbrNo = pMbr->Posn() ;
				pTable->m_Cols.Add(col) ;
			}
		}
		else if (pN1->NameEQ("ifnone"))
		{
			//	Gather subtags to be rendered in the event of no files/directories found
			if (pTable->m_pNone)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtable> Non-singular <ifnone>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }

			if (pN1->GetFirstChild())
				pTable->m_pNone = _readTag(&tga, pN1->GetFirstChild(), flags, 0, 0) ;

			if (!pTable->m_pNone)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtable> Empty <ifnone>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }
		}
		else if (pN1->NameEQ("header"))
		{
			//	Gather subtags to be rendered before the list in the event of files/directories found
			if (pTable->m_pHead)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtable> Non-singular <header>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }

			pTable->m_pHead = _readTag(&tga, pN1, flags, 0, 0) ;
			if (!pTable->m_pHead)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtable> Empty <header>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }
		}
		else if (pN1->NameEQ("footer"))
		{
			//	Gather subtags to be rendered after the list in the event of files/directories found
			if (pTable->m_pFoot)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtable> Non-singular <footer>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }

			pTable->m_pFoot = _readTag(&tga, pN1, flags, 0, 0) ;
			if (!pTable->m_pFoot)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtable> Empty <footer>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; break ; }
		}
		else
		{
			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d <xtable> Illegal subtag <%s> Only <xcol> allowed\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
		}
	}

	return rc == E_OK ? thisVE : 0 ;
}

hzEcode	hdsApp::_readFixDir	(hzXmlNode* pN)
{
	//	Category:	Tables and Dirlist Config Read Functions
	//
	//	Read in an <xfixdir> tag and place the content as a fixed file in the app's m_Fixed map of URLs to fixed HTML or text content
	//
	//	Arguments:	1)	pN	Current XML node
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readFixDir") ;

	hzVect<hzDirent>	files ;		//	All files found in the directory matching the criteria

	ifstream		is ;			//	Input stream
	hzDirent		de ;			//	Individual directory entry
	hzAttrset		ai ;			//	Attribute iterator
	const char*		j ;				//	For file endings and MIME type
	hdsFile*		pFix ;			//	Passive file
	uint64_t		now ;			//	Start of zip
	uint64_t		then ;			//	End of zip
	hzString		absPath ;		//	Fixed file source directory (absolute, incl doc root)
	hzString		directory ;		//	Fixed file source directory (relative, not incl doc root)
	hzString		criteria ;		//	File search criteria
	uint32_t		ln ;			//	Line number
	uint32_t		n ;				//	File iterator
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;

	ln = pN->Line() ;

	if (!pN->NameEQ("xfixdir"))
		{ m_pLog->Out("%s. File %s Line %d Expected <xfixdir> got <%s>\n", *_fn, pN->Fname(), ln, pN->TxtName()) ; return E_SYNTAX ; }

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("dir"))		directory = ai.Value() ;
		else if (ai.NameEQ("criteria"))	criteria = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xfixdir> Bad params (%s=%s)\n", *_fn, pN->Fname(), ln, ai.Name(), ai.Value()) ; }
	}

	//	Obtain the directory listing
	absPath = m_Docroot + "/" + directory ;

	rc = ReadDir(files, absPath, criteria) ;
	if (rc != E_OK)
	{
		m_pLog->Out("%s. File %s Line %d <xfixdir> Could not read directory %s. err=%s\n", *_fn, pN->Fname(), ln, *absPath, Err2Txt(rc)) ;
		return rc ;
	}

	//	Read in content - Either by loading file or from the node content
	for (n = 0 ; n < files.Count() ; n++)
	{
		de = files[n] ;

		if (!de.Size())
			{ m_pLog->Out("%s. File %s Line %d <xfixdir> Igoring empty file %s\n", *_fn, pN->Fname(), ln, *de.Name()) ; continue ; }

		//	Load each file
		pFix = new hdsFile() ;

		pFix->m_Url = "/" + directory + "/" + de.Name() ;
		pFix->m_filepath = absPath + "/" + de.Name() ;
		pFix->m_Title = de.Name() ;

		j = strrchr(*de.Name(), CHAR_PERIOD) ;
		if (j)
			pFix->m_Mimetype = Str2Mimetype(j) ;

		is.open(*pFix->m_filepath) ;
		if (is.fail())
		{
			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d <xfixdir> Content file %s cannot be opened\n", *_fn, pN->Fname(), ln, *pFix->m_filepath) ;
		}
		else
		{
			pFix->m_rawValue << is ;
			is.close() ;
			m_pLog->Out("%s. File %s Line %d <xfixdir> Content file %s opened and has %d bytes\n", *_fn, pN->Fname(), ln, *pFix->m_filepath, pFix->m_rawValue.Size()) ;
		}
		is.clear() ;

		if (!pFix->m_rawValue.Size())
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xfixdir> Content file %s empty\n", *_fn, pN->Fname(), ln, *pFix->m_filepath) ; }

		if (rc == E_OK)
		{
			now = RealtimeNano() ;
			Gzip(pFix->m_zipValue, pFix->m_rawValue) ;
			then = RealtimeNano() ;
			//m_Fixed.Insert(pFix->m_Url, pFix) ;
			m_ResourcesPath.Insert(pFix->m_Url, pFix) ;
			m_pLog->Out("%s: Added fixed page %s (%d,%d) %u nS\n", *_fn, *pFix->m_Url, pFix->m_rawValue.Size(), pFix->m_zipValue.Size(), then - now) ;
		}
	}

	return rc ;
}

hzEcode	hdsApp::_readMiscDir	(hzXmlNode* pN)
{
	//	Category:	Tables and Dirlist Config Read Functions
	//
	//	Read in an <xfixdir> tag and place the content as a fixed file in the app's m_Fixed map of URLs to fixed HTML or text content
	//
	//	Arguments:	1)	pN	Current XML node
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readMiscDir") ;

	hzVect<hzDirent>	files ;		//	All files found in the directory matching the criteria (if any)

	ifstream		is ;			//	Input stream
	hzDirent		de ;			//	Individual directory entry
	hzAttrset		ai ;			//	Attribute iterator
	hzString		directory ;		//	Fixed file source directory (relative, not incl doc root)
	hzString		criteria ;		//	File search criteria
	hzString		fpath ;			//	File path
	uint32_t		ln ;			//	Line number
	uint32_t		n ;				//	File iterator
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;

	ln = pN->Line() ;

	if (!pN->NameEQ("xmiscdir"))
		{ m_pLog->Out("%s. File %s Line %d Expected <xmiscdir> got <%s>\n", *_fn, pN->Fname(), ln, pN->TxtName()) ; return E_SYNTAX ; }

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("dir"))		directory = ai.Value() ;
		else if (ai.NameEQ("criteria"))	criteria = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xmiscdir> Bad params (%s=%s)\n", *_fn, pN->Fname(), ln, ai.Name(), ai.Value()) ; }
	}

	//	Obtain the directory listing
	rc = ReadDir(files, directory, criteria) ;
	if (rc != E_OK)
	{
		m_pLog->Out("%s. File %s Line %d <xmiscdir> Could not read directory %s. err=%s\n", *_fn, pN->Fname(), ln, *directory, Err2Txt(rc)) ;
		return E_SYNTAX ;
	}

	//	Read in content - Either by loading file or from the node content
	for (n = 0 ; n < files.Count() ; n++)
	{
		de = files[n] ;

		if (!de.Size())
			{ m_pLog->Out("%s. File %s Line %d <xmiscdir> Igoring empty file %s\n", *_fn, pN->Fname(), ln, *de.Name()) ; continue ; }

		//	Load each file
		fpath = de.Path() + "/" + de.Name() ;
		m_Misc.Insert(fpath, de) ;
	}

	return rc ;
}

/*
**	SECTION 4:	hdsTree Config Read Functions
*/

hzEcode	hdsApp::_readXtreeItem	(hzXmlNode* pN, hdsTree* pAG)
{
	//	Category:	Config Read Functions (Nav-tree)
	//
	//	Process an <xtreeItem> tag to add a tree item (article) to a public (nav) tree.
	//
	//	Each tree item must have a title (what the user will see), a URL for an AJAX GET, a node identifier (unique within the tree) and a parent node identifier so that hierarchy
	//	can be effected. Article content is supplied as the <xtreeItem> content and the parameters given in attributes as follows:-
	//
	//		1)	parent	Parent node id. This will be null if the item(s) to be added are to be added directly to the root.
	//		2)	id		This is name that will also be used as URL. It has to be unique.
	//		3)	link	True/false. If true the added item will have a link in the resultant navigation tree.
	//		4)	hdln	The title that will appear as the selectable item name in the navigation tree.
	//
	//	Arguments:	1)	pN		Current node, expected to be <xtreeItem> tag
	//				2)	pAG		Current article group
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readXtreeItem") ;

	hzList<hdsVE*>::Iter	ih ;	//  Html entity iterator

	hzHttpEvent		httpEv ;		//	Artificial HTTP event (for language)
	_tagArg			tga ;			//	Tag argument for _readTag()
	hzAttrset		ai ;			//	Attribute iterator
	hdsArticleStd*	pArt ;			//	Article pointer
	hzXmlNode*		pN1 ;			//	Subtag probe
	hdsVE*			pVE ;			//	Set if a HTML entity is found
	hzChain			Z ;				//	For generating article HTML
	hzChain			X ;				//	For generating article HTML
	uint64_t		now ;			//	Start of zip
	uint64_t		then ;			//	Start of zip
	hzString		parent ;		//	Article parent name (if null, parent is the article group)
	hzString		refnam ;		//	Article ref name (compulsory)
	hzString		hdln ;			//	Article title (compulsory as without, no entry in tree)
	hzString		strVal ;		//	Temp string for text value
	uint32_t		bScripts ;		//	Needed for read tag
	bool			bHead = false ;	//	Add item/head
	bool			bDisp = false ;	//	Display open/close by default
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)	Fatal("%s: No node supplied\n", *_fn) ;
	if (!pAG)	Fatal("%s: No article group supplied\n", *_fn) ;

	if (pN->NameEQ("xtreeItem"))
		bHead = false ;
	else if (pN->NameEQ("xtreeHead"))
		bHead = true ;
	else
		{ m_pLog->Out("%s. File %s Line %d Expected <xtreeItem> or <xtreeHead> got <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return E_SYNTAX ; }

	//	The attr will be the name of the form to which the handler applies
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("parent"))	parent = ai.Value() ;
		else if (ai.NameEQ("id"))		refnam = ai.Value() ;
		else if (ai.NameEQ("hdln"))		hdln = ai.Value() ;
		else if (ai.NameEQ("display"))	bDisp = ai.ValEQ("open") ;
		else
		{
			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d <xtreeItem> tag: Illegal attribute %s=%s\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
		}
	}

	if (!refnam)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <%s> tag: No name supplied\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; }
	if (!hdln)		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <%s> tag: No title supplied\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; }

	if (pAG->Item(refnam))
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtreeItem> tag: %s already exists\n", *_fn, pN->Fname(), pN->Line(), *refnam) ; }
	if (parent)
	{
		if (!pAG->Item(parent))
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <%s> tag: parent tag %s not found\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName(), *parent) ; } 
	}

	if (rc != E_OK)
		return rc ;

	if (bHead)
		pArt = (hdsArticleStd*) pAG->AddHead(parent, refnam, hdln, bDisp) ;
	else
		pArt = (hdsArticleStd*) pAG->AddItem(parent, refnam, hdln, bDisp) ;

	rc = pArt->m_USL.SetArticle(pAG->m_USL, pAG->Count()) ;

	//	Get article content from subtags
	tga.m_pLR = pArt ;
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN->NameEQ("xobject"))
		{
			//	Handle <xobject> tag
			for (ai = pN ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("repos"))	{}
				else if (ai.NameEQ("class"))	{}
				else if (ai.NameEQ("objId"))	{}
				else
					{ rc = E_SYNTAX ; m_cfgErr.Printf("%s. File %s Line %d. <xobject> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
			}
			continue ;
		}

		pVE = _readTag(&tga, pN1, bScripts, 0, 0) ;
		if (!pVE)
			rc = E_SYNTAX ;
		else
			pArt->AddVisent(pVE) ;
	}

	if (rc != E_OK)
		return rc ;

	//	Assign language uids to the article's page elements
	AssignVisentIDs(pArt->m_VEs, pArt->m_flagVE, pArt->m_USL) ;

	httpEv.m_pContextLang = m_pDfltLang ;
	if (!(pArt->m_flagVE & VE_ACTIVE))
	{
		pArt->Generate(Z, &httpEv) ;

		if (Z.Size())
		{
			//	Assign article content to fixed buffers, one for unzipped and the other as zipped.

			strVal = Z ;
			m_pDfltLang->m_rawItems.Insert(pArt->m_USL, strVal) ;

			now = RealtimeNano() ;
			Gzip(X, Z) ;
			then = RealtimeNano() ;

			strVal = X ;
			m_pDfltLang->m_zipItems.Insert(pArt->m_USL, strVal) ;
		}
	}

	m_pLog->Out("%s. Added article ref-%s (%s) of %d %d bytes %u nS\n", *_fn, pArt->TxtName(), *pArt->m_Title, X.Size(), Z.Size(), now - then) ;
	return rc ;
}

hzEcode	hdsApp::_readXtreeDcl	(hzXmlNode* pN, hdsPage* pPage)
{
	//	Category:	Config Read Functions (Nav-tree)
	//
	//	Process an <xtreeDcl> tag to declare a instance of a hdsTree (in effect a group of articles). As per the synopsis on "Dissemino trees", an <xtreeDcl> tag appearing OUTSIDE
	//	the scope of an <xpage> tag declares a public tree (that can be referenced by multiple pages), while an <xtreeDcl> tag WITHIN an <xpage> declares a private tree (may only
	//	be used within the page).
	//
	//	Arguments:	1)	pN		Current config node, expected to be <xtree> tag
	//				2)	pPage	The Host page, if applicable
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readXtreeDcl") ;

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Subtag probe
	hdsTree*		pAG ;			//	Article group pointer
	hzString		name ;			//	Article group name
	hzString		page ;			//	Article group hostpage)
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;


	if (!pN->NameEQ("xtreeDcl"))
		{ m_pLog->Out("%s. File %s Line %d Expected <xtreeDcl> got <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return E_SYNTAX ; }

	//	The attr will be the name of the article group
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))	name = ai.Value() ;
		else if (ai.NameEQ("page"))	page = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtreeDcl> tag: Illegal attr %s=%s\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!name)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtreeDcl> tag: No name attr\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!page)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtreeDcl> tag: No host page\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (rc != E_OK)
		return rc ;

	pAG = new hdsTree() ;
	pAG->m_Groupname = name ;
	rc = pAG->m_USL.SetGroup(m_ArticleGroups.Count()) ;
	pAG->m_Hostpage = page ;
	m_ArticleGroups.Insert(pAG->m_Groupname, pAG) ;

	//	Read in list of related articles
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if		(pN1->NameEQ("xtreeItem"))	rc = _readXtreeItem(pN1, pAG) ;
		else if (pN1->NameEQ("xtreeHead"))	rc = _readXtreeItem(pN1, pAG) ;
		else if (pN1->NameEQ("xpage"))		rc = _readPage(pN) ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xtreeDcl> Illegal subtag <%s>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; }
	}

	return rc ;
}

hdsVE*	hdsApp::_readXtreeCtl	(hzXmlNode* pN)
{
	//	Category:	Config Read Functions (Nav-tree)
	//
	//	Process an <xtreeCtl> tag. These should appear only within the page that hosts the nav-tree. xtreeCtl tags control the HTML manifestation of the nav-tree itself and that of
	//	selected articles.
	//
	//	Argument:	pN	The current node expected to be a <xtreectl> tag
	//	Returns:	Pointer to visible entity

	_hzfunc("hdsApp::_readXtreeCtl") ;

	hzAttrset		ai ;			//	Attribute iterator
	hdsTree*		pAG ;			//	Article group pointer
	hdsArtref*		pAR ;			//	Article pointer
	hdsVE*			thisVE ;		//	Visible entity
	hzString		grpName ;		//	Article group name
	hzString		artName ;		//	Article name
	hzString		vname ;			//	Article name
	uint32_t		nShow = 0 ;		//	Show title or content
	uint32_t		bErr = 0 ;		//	Error

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;

	if (!pN->NameEQ("xtreeCtl"))
		{ m_pLog->Out("%s. File %s Line %d Expected <xtreeCtl> got <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return 0 ; }

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("group"))	grpName = ai.Value() ;
		else if (ai.NameEQ("article"))	artName = ai.Value() ;
		else if (ai.NameEQ("show"))
		{
			if		(ai.ValEQ("title"))		nShow = 100 ;
			else if (ai.ValEQ("content"))	nShow = 200 ;
			else if (ai.ValEQ("navtree"))	nShow = 300 ;
			else
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xtreeCtl> Bad show value (must be title/content) not %s\n", *_fn, pN->Fname(), pN->Line(), ai.Value()) ; }
		}
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xtreeCtl> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!grpName)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xtreeCtl> no group name attr supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!nShow)		{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xtreeCtl> no show directive supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (nShow < 300)
	//{
		if (!artName)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xtreeCtl> no article name attr supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	//}

	if (bErr)
		return 0 ;

	pAG = m_ArticleGroups[grpName] ;
	if (!pAG)
		//{ bErr=1 ; m_pLog->Out("%s. File %s Line %d article group %s non-existant\n", *_fn, pN->Fname(), pN->Line(), *grpName) ; }
		{ m_pLog->Out("%s. File %s Line %d article group %s non-existant\n", *_fn, pN->Fname(), pN->Line(), *grpName) ; }
	else
	{
		if (nShow < 300)
		{
			if (!memcmp(*artName, "@resarg;/", 9))
			{
				vname = *artName + 9 ;
				if (!pAG->Item(vname))
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xtreeCtl> Default article %s non-existant\n", *_fn, pN->Fname(), pN->Line(), *vname) ; }
			}
			else
			{
				if (!pAG->Item(artName))
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xtreeCtl> Article %s non-existant\n", *_fn, pN->Fname(), pN->Line(), *artName) ; }
			}
		}
	}

	if (bErr)
		return 0 ;

	thisVE = pAR = new hdsArtref(this) ;
	thisVE->m_Line = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;
	thisVE->m_flagVE |= VE_CT_ACTIVE ;
	pAR->m_Group = grpName ;
	pAR->m_Article = artName ;
	pAR->m_Show = nShow ;

	return pAR ;
}

/*
**	SECTION 5:	Forms Config Read Functions
*/

hzEcode	hdsApp::_readExec	(hzXmlNode* pN, hzList<hdsExec*>& execList, hdsPage* pPage, hdsFormhdl* pFhdl)
{
	//	Read in a set of one or more Dissemino exec commands in a <procdata> tag.
	//
	//	Arguments:	1)	pN			Current XML node being a <procdata> tag
	//				2)	execList	The list of executive commands (in the form handler, page or article)
	//				3)	pPage		Current page (must be 0 if there is a form handler)
	//				4)	pFhdl		Current form handler (must be 0 if there is a page)
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readExec") ;

	const hdbClass*		pClass ;		//	Class named in command
	const hdbClass*		pEstClass ;		//	Class established in the m_ObjKeys map.
	const hdbClass*		pFormClass ;	//	Class of Commit/Fetch/Select
	const hdbDatatype*	pDT ;			//	Data type
	const hdbMember*	pMbr ;			//	Member of Commit/Fetch/Select

	hzAttrset			ai ;			//	Attribute iterator
	hzXmlNode*			pN1 ;			//	Level 1 nodes
	hzXmlNode*			pN2 ;			//	Level 2 nodes
	hdsExec*			pExec ;			//	Instruction
	hzPair				pair ;			//	For use in adduser
	hzString			err ;			//	Error report from PcEntTest/Scan
	hzString			strA ;			//	Primary member name
	hzString			strB ;			//	Secondary member name
	hzString			pcntEnt ;		//	Percent entity
	hzString			from ;			//	Email originator
	hzString			addr ;			//	Recipient address
	hzString			subj ;			//	Email subject
	hzString			name ;			//	Email subject
	hzString			input ;			//	Email subject
	hzString			typeStr ;		//	Email subject
	hzString			param1 ;		//	Array of parameters
	hzString			param2 ;		//	Array of parameters
	hzString			param3 ;		//	Array of parameters
	hzString			param4 ;		//	Array of parameters
	hzString			param5 ;		//	Array of parameters
	hzString			param6 ;		//	Array of parameters
	hzString			param7 ;		//	Array of parameters
	uint32_t			cmd ;			//	Command instruction
	uint32_t			bErr = 0 ;		//	Error condition
	uint32_t			nParams = 0 ;	//	Error condition
	//int32_t				val_Lo ;		//	First index into ADP m_mapSubs
	//int32_t				val_Hi ;		//	Last index into ADP m_mapSubs
	hdbBasetype			peType ;		//	Percent entity type
	hdbBasetype			peType_b ;		//	Percent entity type (second value)
	hzEcode				rc = E_OK ;		//	Return code

	//	Check call validity
	if (!pN)							Fatal("%s: No node supplied\n", *_fn) ;
	if (!pN->NameEQ("procdata"))		Fatal("%s: Expected <procdata> got <%s>\n", *_fn, pN->TxtName()) ;
	if (!pFhdl && !pPage)				Fatal("%s. No form handler or page supplied\n", *_fn) ;
	if (pFhdl && !pFhdl->m_pFormdef)	Fatal("%s. No form supplied\n", *_fn) ;

	pFormClass = pFhdl ? pFhdl->m_pFormdef->m_pClass : 0 ;

	//	If the form to which this is the formhandler is the 'root', clear vars in scope. These will be populated by <setvar>
	//
	//	All form handlers are associated with a form which in turn is associated with a host page. In the case where the host page is not defined
	//	as a page in it's own right but is instead is a response/error page defined as part of a form handler for another form, then the page's
	//	m_pParentForm member will point to this other form. This function returns true in this instance. False otherwise.

	//	Get instructions in the order they are to be executed
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		bErr = nParams = 0 ;

		m_pLog->Out("%s. File %s line %d: doing %s (form hdl %p)\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName(), pFhdl) ;

		if (pN1->NameEQ("sendemail"))
		{
			if (!pFhdl)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s line %d: <sendemail> can only appear in a form handler\n", *_fn, pN->Fname(), pN1->Line()) ; break ; }

			pExec = new hdsExec(this, EXEC_SENDEMAIL) ;
			nParams = 4 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("from"))		param1 = ai.Value() ;
				else if (ai.NameEQ("to"))		param2 = ai.Value() ;
				else if (ai.NameEQ("subject"))	param3 = ai.Value() ;
				else
				{
					m_pLog->Out("%s. File %s line %d: <sendemail> has attr of %s=%s. Only <from|to|subject> allowed\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ;
					rc = E_SYNTAX ;
				}
			}

			param4 = pN1->m_fixContent ;

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> No sender specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> No recipient specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param3)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> No subject specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param4)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> No message specified\n", *_fn, pN->Fname(), pN1->Line()) ; }

			//	Email FROM address
			if (param1)
			{
				if (IsPcEnt(pcntEnt, param1))
				{
					peType = PcEntTest(err, pFhdl->m_pFormdef, 0, param1) ;
					if (peType == BASETYPE_UNDEF)
						{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> Sender %s %s\n", *_fn, pN->Fname(), pN1->Line(), *param1, *err) ; }

					if (peType != BASETYPE_EMADDR)
					{
						m_pLog->Out("%s. File %s line %d: <sendemail> Sender %s is type %s instead of TYPE_EMAIL\n",
							*_fn, pN->Fname(), pN1->Line(), *param1, Basetype2Txt(peType)) ;
						bErr = 1 ;
					}
				}
				else
				{
					if (!IsEmaddr(param1))
						{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> Sender %s invalid email address\n", *_fn, pN->Fname(), pN1->Line(), *param1) ; }
				}
			}

			//	Email TO address
			if (param2)
			{
				if (IsPcEnt(pcntEnt, param2))
				{
					peType = PcEntTest(err, pFhdl->m_pFormdef, 0, param2) ;
					if (peType == BASETYPE_UNDEF)
						{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> Recipient %s %s\n", *_fn, pN->Fname(), pN1->Line(), *param2, *err) ; }

					if (peType != BASETYPE_EMADDR)
					{
						m_pLog->Out("%s. File %s line %d: <sendemail> Recipient %s is type %s instead of TYPE_EMAIL\n",
							*_fn, pN->Fname(), pN1->Line(), *param2, Basetype2Txt(peType)) ;
						bErr = 1 ;
					}
				}
				else
				{
					if (!IsEmaddr(param2))
						{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> Recipient %s invalid email address\n", *_fn, pN->Fname(), pN1->Line(), *param2) ; }
				}
			}

			if (param3)
			{
				rc = PcEntScanStr(err, pFhdl->m_pFormdef, 0, param3) ;
				if (rc != E_OK)
					{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> Subject: %s\n", *_fn, pN->Fname(), pN1->Line(), *err) ; }
			}

			if (param4)
			{
				rc = PcEntScanStr(err, pFhdl->m_pFormdef, 0, param4) ;
				if (rc != E_OK)
					{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <sendemail> Content: %s\n", *_fn, pN->Fname(), pN1->Line(), *err) ; }
			}

			pN2 = pN1->GetFirstChild() ;
			if (pN2)
			{
				//	Only allowed child is 'error' to specify error response
				if (!pN2->NameEQ("error"))
				{
					m_pLog->Out("%s. File %s Line %d: <sendemail> Only allowed subtag is <error>, tag of <%s> unexpected\n",
						*_fn, pN->Fname(), pN1->Line(), pN2->TxtName()) ;
					rc = E_SYNTAX ;
				}

				m_pLog->Out("%s. File %s Line %d: CALL _readResponse case 1\n", *_fn, pN->Fname(), pN1->Line()) ;
				rc = _readResponse(pN2, pFhdl, pExec->m_FailGoto, &pExec->m_pFailResponse) ;
			}
		}

		else if (pN1->NameEQ("setvar"))
		{
			pExec = new hdsExec(this, EXEC_SETVAR) ;
			nParams = 3 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("name"))		param1 = ai.Value() ;
				else if (ai.NameEQ("value"))	param2 = ai.Value() ;
				else if (ai.NameEQ("type"))		param3 = ai.Value() ;
				else
				{
					bErr = 1 ;
					m_pLog->Out("%s. File %s Line %d: <setvar> Illegal attr %s=%s. Only <name|type|value> allowed\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ;
				}
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <setvar> No var name supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <setvar> No value supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param3)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <setvar> No var type supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }

			if (bErr)
				rc = E_SYNTAX ;
			else
			{
				pDT = m_ADP.GetDatatype(typeStr) ;
				if (pDT)
					pExec->m_type = pDT->Basetype() ;
				else
				{
					pExec->m_type = Str2Basetype(param3) ;
					if (!pExec->m_type)
						{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <setvar> Unknown var type %s\n", *_fn, pN->Fname(), pN1->Line(), *param3) ; }
				}

				m_tmpVarsSess.Insert(param1, pExec->m_type) ;
			}

			if (!bErr)
			{
				if (IsPcEnt(pcntEnt, param2))
				{
					peType = PcEntTest(err, pFhdl->m_pFormdef, pFhdl->m_pFormdef->m_pClass, param2) ;
					if (peType == BASETYPE_UNDEF)
						{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <setvar> Value (%s) error %s\n", *_fn, pN->Fname(), pN1->Line(), *param2, *err) ; }

					if (peType != pExec->m_type)
					{
						m_pLog->Out("%s. File %s line %d: <setvar> Value %s is type %s instead of %s\n",
							*_fn, pN->Fname(), pN1->Line(), *param2, Basetype2Txt(peType), Basetype2Txt(pExec->m_type)) ;
						bErr = 1 ;
					}
				}
			}
		}

		else if (pN1->NameEQ("addSubscriber"))
		{
			//	This instructs the system to add a new user. The <addSubscriber> tag has attribute of 'class' to name the user-class and thus the user class repository (if the user
			//	class has its own members). There will be a set of subtags to assign values to the members of the user-class if these exist.

			pExec = new hdsExec(this, EXEC_ADDUSER) ;
			nParams = 4 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("class"))		param1 = ai.Value() ;
				else if (ai.NameEQ("username"))		param2 = ai.Value() ;
				else if (ai.NameEQ("userpass"))		param3 = ai.Value() ;
				else if (ai.NameEQ("email"))		param4 = ai.Value() ;
				else
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <addSubscriber> Illegal attr %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <setusr> No user class named\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <setusr> No user name source\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param3)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <setusr> No password source\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param4)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <setusr> No email source\n", *_fn, pN->Fname(), pN1->Line()) ; }

			if (rc != E_OK)
				break ;

			//	Check user-class exists
			pClass = m_ADP.GetPureClass(param1) ;
			if (!pClass)
			{
				rc = E_SYNTAX ;
				m_pLog->Out("%s. File %s Line %d: <addSubscriber class=\"%s\"> Invalid class\n", *_fn, pN->Fname(), pN1->Line(), *param1) ;
				break ;
			}

			if (bErr)
				rc = E_SYNTAX ;
			else
			{
				pFhdl->m_Exec.Add(pExec) ;
				m_ExecParams.Add(param1) ;
				m_ExecParams.Add(param2) ;
				m_ExecParams.Add(param3) ;
			}

			//	Obtain the user-class member assignments
			for (pN2 = pN1->GetFirstChild() ; rc == E_OK && pN2 ; pN2 = pN2->Sibling())
			{
				//	Tag 'error' to specify error response
				if (pN2->NameEQ("error"))
				{
					m_pLog->Out("%s. File %s Line %d: CALL _readResponse case 2\n", *_fn, pN->Fname(), pN2->Line()) ;
					rc = _readResponse(pN2, pFhdl, pExec->m_FailGoto, &pExec->m_pFailResponse) ;
					continue ;
				}

				if (!pN2->NameEQ("seteq"))
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d: Expected seteq instruction. Got <%s>\n", *_fn, pN->Fname(), pN2->Line(), pN2->TxtName()) ;
					break ;
				}

				pMbr = 0 ;
				for (ai = pN2 ; ai.Valid() ; ai.Advance())
				{
					if		(ai.NameEQ("member"))	pair.name = ai.Value() ;
					else if (ai.NameEQ("input"))	pair.value = ai.Value() ;
					else
					{
						m_pLog->Out("%s. File %s Line %d: <seteq> has attr of %s=%s. Only <member|input> allowed\n", *_fn, pN->Fname(), pN2->Line(), ai.Name(), ai.Value()) ;
						rc = E_SYNTAX ;
					}
				}

				if (!pair.name)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <seteq> No var name supplied\n", *_fn, pN->Fname(), pN2->Line()) ; break ; }

				//	Check member exists
				pMbr = pClass->GetMember(pair.name) ;
				if (!pMbr)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <seteq> No class member identified\n", *_fn, pN->Fname(), pN2->Line()) ; break ; }

				if (pMbr && pMbr->MaxPop() > 1 && cmd == COMMIT_SETEQ)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <seteq> is illegal for list member %s\n", *_fn, pN->Fname(), pN2->Line(), *pair.name) ; break ; }

				if (!pair.value)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <seteq> No input supplied\n", *_fn, pN->Fname(), pN2->Line()) ; break ; }

				if (pair.value[0] == CHAR_PERCENT)
				{
					peType = PcEntTest(err, pFhdl->m_pFormdef, pClass, pair.value) ;
					if (peType == BASETYPE_UNDEF)
						{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <seteq> %s=%s: %s\n", *_fn, pN->Fname(), pN2->Line(), *pair.name, *pair.value, *err) ; }

					if (peType != pMbr->Basetype())
					{
						m_pLog->Out("%s. File %s Line %d: <seteq> %s is of type %s (member requires %s) case 1\n",
							*_fn, pN->Fname(), pN2->Line(), *pair.value, Basetype2Txt(peType), Basetype2Txt(pMbr->Basetype())) ;
						rc = E_TYPE ;
					}
				}
			}
		}

		else if (pN1->NameEQ("logon"))
		{
			pExec = new hdsExec(this, EXEC_LOGON) ;
			nParams = 1 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("user"))	param1 = ai.Value() ;
				else if (ai.NameEQ("pass"))	param2 = ai.Value() ;
				else
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <logon> Illegal attr of %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <logon> No username source\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <logon> No userpass source\n", *_fn, pN->Fname(), pN1->Line()) ; }

			if (bErr)
				rc = E_SYNTAX ;
			else
			{
				pFhdl->m_Exec.Add(pExec) ;
				m_ExecParams.Add(param1) ;
				m_ExecParams.Add(param2) ;
			}
		}

		else if (pN1->NameEQ("testeq"))
		{
			pExec = new hdsExec(this, EXEC_TEST) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("param1"))	param1 = ai.Value() ;
				else if (ai.NameEQ("param2"))	param2 = ai.Value() ;
				else
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d: <testeq> Illegal attr of %s=%s. Only param1/param2 allowed\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ;
				}
			}

			if (!param1 || !param2)
			{
				rc = E_SYNTAX ;
				m_pLog->Out("%s. File %s Line %d: <testeq> must have both param1 and param2 set\n", *_fn, pN->Fname(), pN->Line()) ;
			}

			peType = PcEntTest(err, pFhdl->m_pFormdef, pFormClass, param1) ;
			if (peType == BASETYPE_UNDEF)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <testeq> param1 %s: %s (rc=%s)\n", *_fn, pN->Fname(), pN1->Line(), *name, *err, Err2Txt(rc)) ; }

			peType_b = PcEntTest(err, pFhdl->m_pFormdef, pFormClass, param2) ;
			if (peType_b == BASETYPE_UNDEF)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <testeq> param2 %s: %s (rc=%s)\n", *_fn, pN->Fname(), pN1->Line(), *name, *err, Err2Txt(rc)) ; }

			if (peType != peType_b)
			{
				m_pLog->Out("%s. File %s Line %d: <testeq> param 1 is %s and param 2 is %s\n", *_fn, pN->Fname(), pN1->Line(), Basetype2Txt(peType), Basetype2Txt(peType_b)) ;
				rc = E_SYNTAX ;
			}

			pN2 = pN1->GetFirstChild() ;
			if (pN2)
			{
				//	Only allowed child is 'error' to specify error response
				if (!pN2->NameEQ("error"))
				{
					m_pLog->Out("%s. File %s Line %d: <testeq> Only allowed subtag is <error>, tag of <%s> unexpected\n",
						*_fn, pN->Fname(), pN1->Line(), pN2->TxtName()) ;
					rc = E_SYNTAX ;
				}

				m_pLog->Out("%s. File %s Line %d: CALL _readResponse case 3\n", *_fn, pN->Fname(), pN2->Line()) ;
				rc = _readResponse(pN2, pFhdl, pExec->m_FailGoto, &pExec->m_pFailResponse) ;
			}

			if (rc == E_OK)
			{
				pFhdl->m_Exec.Add(pExec) ;
				m_ExecParams.Add(param1) ;
				m_ExecParams.Add(param2) ;
			}
		}

		else if (pN1->NameEQ("extract"))
		{
			//	Extract text from a src and place in a tgt

			pExec = new hdsExec(this, EXEC_EXTRACT) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("src"))	param1 = ai.Value() ;
				else if (ai.NameEQ("tgt"))	param2 = ai.Value() ;
				else
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d: <extract> Invalid parameter %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ;
				}
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <extract> No source input specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <extract> No target specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}

		else if (pN1->NameEQ("xobjTemp"))
		{
			pExec = new hdsExec(this, EXEC_OBJ_TEMP) ;
			nParams = 3 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("objkey"))	param1 = ai.Value() ;
				else if	(ai.NameEQ("class"))	param2 = ai.Value() ;
				else if	(ai.NameEQ("repos"))	param3 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjTemp> Invalid parameter %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjTemp> No identifier specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			else
			{
				if (m_SObj2Class.Exists(param1))
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjTemp> Illegal reuse of object key %s\n", *_fn, pN->Fname(), pN1->Line(), *param1) ; break ; }
			}

			if (!param2)
				pClass = pFormClass ;
			else
			{
				pClass = m_ADP.GetPureClass(param2) ;
				if (!pClass)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xobjTemp> class %s not found\n", *_fn, pN->Fname(), pN1->Line(), *param2) ; break ; }
			}

			pExec->m_ClassId = pClass->ClassId() ;

			//	Place the refname in the project's map of object specifiers
			m_SObj2Class.Insert(param1, param2) ;
		}

		else if (pN1->NameEQ("xobjStart"))
		{
			pExec = new hdsExec(this, EXEC_OBJ_START) ;
			nParams = 3 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("objkey"))	param1 = ai.Value() ;
				else if	(ai.NameEQ("class"))	param2 = ai.Value() ;
				else if	(ai.NameEQ("repos"))	param3 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjStart> Invalid parameter %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjStart> No identifier specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			else
			{
				if (m_SObj2Class.Exists(param1))
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjStart> Illegal reuse of object key %s\n", *_fn, pN->Fname(), pN1->Line(), *param1) ; break ; }
			}

			if (!param2)
			{
				if (pFhdl)
					pClass = pFormClass ;
				else
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xobjStart> No form and no class\n", *_fn, pN->Fname(), pN1->Line()) ; break ; }
			}
			else
			{
				pClass = m_ADP.GetPureClass(param2) ;
				if (!pClass)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xobjTemp> class %s not found\n", *_fn, pN->Fname(), pN1->Line(), *param2) ; break ; }
			}

			pExec->m_ClassId = pClass->ClassId() ;

			//	Place the refname in the project's map of object specifiers
			m_SObj2Class.Insert(param1, param2) ;
		}

		else if (pN1->NameEQ("xobjImport"))
		{
			//	An import is specifically where the source is a file and not a repository. The file must be named and will commonly use a percent entity
			pExec = new hdsExec(this, EXEC_OBJ_IMPORT) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("objkey"))	param1 = ai.Value() ;
				else if (ai.NameEQ("source"))	param2 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjImport> Invalid parameter %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjImport> No object name specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjImport> No repository specified\n", *_fn, pN->Fname(), pN1->Line()) ; }

			if (!m_SObj2Class.Exists(param1))
				m_pLog->Out("%s. File %s Line %d: <xobjImport> No such object key as %s\n", *_fn, pN->Fname(), pN1->Line(), *param1) ;
		}

		else if (pN1->NameEQ("xobjExport"))
		{
			//	An import is specifically where the source is a file and not a repository. The file must be named and will commonly use a percent entity
			pExec = new hdsExec(this, EXEC_OBJ_EXPORT) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("objkey"))	param1 = ai.Value() ;
				else if (ai.NameEQ("source"))	param2 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjExport> Invalid parameter %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjExport> No object name specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjExport> No repository specified\n", *_fn, pN->Fname(), pN1->Line()) ; }

			if (!m_SObj2Class.Exists(param1))
				m_pLog->Out("%s. File %s Line %d: <xobjExport> No such object key as %s\n", *_fn, pN->Fname(), pN1->Line(), *param1) ;
		}

		else if (pN1->NameEQ("xobjSetMbr"))
		{
			//	For the object named in param 1, set the member named in param 3 to the value of or from the source specified in param 4. The applicable data class will be the form
			//	native unless otherwise specified in param 2. When setting a member of a sub-class instance
			//
			//	The current object (hdsObect) is a one to many map of hzDelta instances to values. hzDelta identifies an object, either of the hdbObject's host
			//	class or a sub-class), by naming the class and stating the object id. hzDelta also states the applicable class member. When writing form handlers
			//	the class and member can be known but the object id cannot be known as this will depend on the object being viewed.
			//
			//	Here then we only specify class and member and the variable that the object member is to be set to - so 3 parameters. The class and member must
			//	exist. The class name can be blank if the class is the current object host class. The variable must necessarily be a percent entity.

			pExec = new hdsExec(this, EXEC_OBJ_SETMBR) ;
			nParams = 4 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("objkey"))	param1 = ai.Value() ;
				else if (ai.NameEQ("class"))	param2 = ai.Value() ;
				else if (ai.NameEQ("member"))	param3 = ai.Value() ;
				else if (ai.NameEQ("input"))	param4 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> Invalid parameter %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> No object key specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param3)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> No member name specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param4)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> No input specified\n", *_fn, pN->Fname(), pN1->Line()) ; }

			if (!m_SObj2Class.Exists(param1))
				//{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> No such object key as %s\n", *_fn, pN->Fname(), pN1->Line(), *param1) ; }
				m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> No such object key as %s\n", *_fn, pN->Fname(), pN1->Line(), *param1) ;

			//	Get class
			if (!param2)
				pClass = pFormClass ;
			else
			{
				pClass = m_ADP.GetPureClass(param2) ;
				if (!pClass)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> class %s not found\n", *_fn, pN->Fname(), pN1->Line(), *param2) ; break ; }
			}

			if (param2 != m_SObj2Class[param1])
			{
				//	Class only matches if it is the same as the established object class or a sub-class of it.
				pEstClass = m_ADP.GetPureClass(m_SObj2Class[param1]) ;

				pClass = 0 ;

				/*
 * 				FIX
 * 				val_Lo = m_ADP.m_mapSubs.First(pEstClass->StrTypename()) ;
				if (val_Lo >= 0)
				{
					val_Hi = m_ADP.m_mapSubs.Last(pEstClass->StrTypename()) ;

					for (; val_Lo <= val_Hi ; val_Lo++)
					{
						pClass = m_ADP.m_mapSubs.GetObj(val_Lo) ;
						if (pClass->StrTypename() == param2)
							break ;
						pClass = 0 ;
					}
				}
				*/

				if (pClass)
					pClass = pEstClass ;
				else
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> wrong class %s\n", *_fn, pN->Fname(), pN1->Line(), *param2) ; break ; }
			}

			if (!param4)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjSetMbr> No input specified\n", *_fn, pN->Fname(), pN1->Line()) ; }

			//	Now we have the object specifier name we can get the repos and class from the 

			//	Obtain the member assignments
			if (param3.Contains("->"))
			{
				strA = strB = param3 ;
				strA.TruncateUpto("->") ;
				strB.TruncateBeyond("->") ;

				pMbr = pClass->GetMember(strA) ;
				if (!pMbr)
				{
					m_pLog->Out("%s. File %s Line %d: <%s> %s not a member of class %s\n",
						*_fn, pN->Fname(), pN1->Line(), pN1->TxtName(), *strA, pClass->TxtTypename()) ;
					rc = E_NOTFOUND ;
				}
				else
				{
					if (pMbr->IsClass())
					{
						m_pLog->Out("%s. File %s Line %d: <%s> Composite %s is not a class\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName(), *strA) ;
						rc = E_NOTFOUND ;
					}
					else
					{
						pMbr = pClass->GetMember(strB) ;
						if (!pMbr)
						{
							m_pLog->Out("%s. File %s Line %d: <%s> Member %s does not exists in class %s\n",
								*_fn, pN->Fname(), pN1->Line(), pN1->TxtName(), *strB, pClass->TxtTypename()) ;
							rc = E_NOTFOUND ;
						}
					}
				}
			}
			else
			{
				pMbr = pClass->GetMember(param3) ;
				if (!pMbr)
				{
					m_pLog->Out("%s. File %s Line %d: <%s> Member %s does not exists in class %s\n",
						*_fn, pN->Fname(), pN1->Line(), pN1->TxtName(), ai.Value(), pClass->TxtTypename()) ;
					rc = E_NOTFOUND ;
				}
				name = ai.Value() ;
			}

			if (pMbr && pMbr->MaxPop() > 1 && cmd == COMMIT_SETEQ)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <%s> is illegal for list member %s\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName(), *param3) ; break ; }

			if (input[0] == CHAR_PERCENT)
			{
				peType = PcEntTest(err, pFhdl->m_pFormdef, pClass, input) ;
				if (peType == BASETYPE_UNDEF)
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <%s> %s: %s\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName(), *param3, *err) ; }

				if (peType != pMbr->Basetype())
				{
					m_pLog->Out("%s. File %s Line %d: <%s> [%s] is of type %x->%s (member requires %x->%s) case 2\n",
						*_fn, pN->Fname(), pN1->Line(), pN1->TxtName(), *input, peType, Basetype2Txt(peType), pMbr->Basetype(), Basetype2Txt(pMbr->Basetype())) ;
					rc = E_TYPE ;
				}
			}
		}

		else if (pN1->NameEQ("xobjCommit"))
		{
			//	This commits the named object to its repository.

			pExec = new hdsExec(this, EXEC_OBJ_COMMIT) ;
			nParams = 1 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if (ai.NameEQ("objkey"))
					param1 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjCommit> Invalid parameter %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!m_SObj2Class.Exists(param1))
				m_pLog->Out("%s. File %s Line %d: <xobjCommit> No such object key as %s\n", *_fn, pN->Fname(), pN1->Line(), *param1) ;
		}

		else if (pN1->NameEQ("xobjClose"))
		{
			//	This commits the named object to its repository.

			pExec = new hdsExec(this, EXEC_OBJ_CLOSE) ;
			nParams = 1 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if (ai.NameEQ("objkey"))
					param1 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xobjClose> Invalid parameter %s=%s\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!m_SObj2Class.Exists(param1))
				m_pLog->Out("%s. File %s Line %d: <xobjClose> No such object key as %s\n", *_fn, pN->Fname(), pN1->Line(), *param1) ;
		}

		else if (pN1->NameEQ("xtreeDcl"))
		{
			pExec = new hdsExec(this, EXEC_TREE_DCL) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("treeid"))	param1 = ai.Value() ;
				else if (ai.NameEQ("copy"))		param2 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeDcl> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeDcl> No tree id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			m_ArticleGroups.Insert(param1, (hdsTree*) 0) ;
		}
		else if (pN1->NameEQ("xtreeHead"))
		{
			//	Adds a tree header (no active link)
			pExec = new hdsExec(this, EXEC_TREE_HEAD) ;
			nParams = 4 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("treeid"))	param1 = ai.Value() ;
				else if (ai.NameEQ("parent"))	param2 = ai.Value() ;
				else if (ai.NameEQ("refname"))	param3 = ai.Value() ;
				else if (ai.NameEQ("title"))	param4 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeHdr> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeHdr> No tree id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param3)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeHdr> No refname supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param4)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeHdr> No title supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}
		else if (pN1->NameEQ("xtreeItem"))
		{
			//	Adds a tree article (refname acts as URL)
			pExec = new hdsExec(this, EXEC_TREE_ITEM) ;
			nParams = 4 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("treeid"))	param1 = ai.Value() ;
				else if (ai.NameEQ("parent"))	param2 = ai.Value() ;
				else if (ai.NameEQ("refname"))	param3 = ai.Value() ;
				else if (ai.NameEQ("title"))	param4 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No tree id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param3)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No refname supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param4)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No title supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}
		else if (pN1->NameEQ("xtreeForm"))
		{
			pExec = new hdsExec(this, EXEC_TREE_FORM) ;
			nParams = 6 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("treeid"))	param1 = ai.Value() ;
				else if (ai.NameEQ("parent"))	param2 = ai.Value() ;
				else if (ai.NameEQ("refname"))	param3 = ai.Value() ;
				else if (ai.NameEQ("title"))	param4 = ai.Value() ;
				else if (ai.NameEQ("form"))		param5 = ai.Value() ;
				else if (ai.NameEQ("class"))	param6 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No tree id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param3)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No refname supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param4)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No title supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param5)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No form supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param6)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No class supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }

			//	Need to check form exists - and that to stated form class is compatible
		}
		else if (pN1->NameEQ("xtreeSync"))
		{
			pExec = new hdsExec(this, EXEC_TREE_SYNC) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("treeid"))	param1 = ai.Value() ;
				else if (ai.NameEQ("objkey"))	param2 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No tree id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeAdd> No object key supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}
		else if (pN1->NameEQ("xtreeDel"))
		{
			pExec = new hdsExec(this, EXEC_TREE_DEL) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("treeid"))	param1 = ai.Value() ;
				else if (ai.NameEQ("nodeid"))	param2 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeDel> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeDel> No tree id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeDel> No node id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}
		else if (pN1->NameEQ("xtreeExp"))
		{
			pExec = new hdsExec(this, EXEC_TREE_EXP) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("treeid"))	param1 = ai.Value() ;
				else if (ai.NameEQ("nodeid"))	param2 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeExp> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeExp> No tree id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeExp> No node id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}
		else if (pN1->NameEQ("xtreeClr"))
		{
			pExec = new hdsExec(this, EXEC_TREE_CLR) ;
			nParams = 2 ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("treeid"))	param1 = ai.Value() ;
				else if (ai.NameEQ("nodeid"))	param2 = ai.Value() ;
				else
					{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeClr> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeClr> No tree id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <treeClr> No node id supplied\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}

		else if (pN1->NameEQ("filesys"))
		{
			//	A file operation allows one to create and delete files and directories, list directories and load files

			pExec = new hdsExec(this, EXEC_FILESYS) ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if (ai.NameEQ("action"))
				{
					if	(ai.ValEQ("mkdir") || ai.ValEQ("rmdir") || ai.ValEQ("save") || ai.ValEQ("delete"))
						param1 = ai.Value() ;
					else
						{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <fileop> Illegal operation (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
				}
				else if (ai.NameEQ("resource"))	param2 = ai.Value() ;
				else if (ai.NameEQ("content"))	param3 = ai.Value() ;
				else
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <fileop> Illegal param (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <filesys> No action specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <filesys> No resource specified\n", *_fn, pN->Fname(), pN1->Line()) ; }

			if (!param3 && (param1 == "save" || param1 == "delete"))
				{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <filesys> No content specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}
		else if (pN1->NameEQ("srchPages"))
		{
			pExec = new hdsExec(this, EXEC_SRCH_PAGES) ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if (ai.NameEQ("limit"))
					param1 = ai.Value() ;
				else
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <srchPages> Illegal attr (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)
				param1 = "0" ;
		}
		else if (pN1->NameEQ("srchRepos"))
		{
			pExec = new hdsExec(this, EXEC_SRCH_REPOS) ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("items"))	param1 = ai.Value() ;
				else if (ai.NameEQ("action"))	param2 = ai.Value() ;
				else if (ai.NameEQ("criteria"))	param3 = ai.Value() ;
				else if (ai.NameEQ("count"))	param4 = ai.Value() ;
				else if (ai.NameEQ("found"))	param5 = ai.Value() ;
				else
					{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <search> Illegal attr (%s=%s)\n", *_fn, pN->Fname(), pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!param1)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <search> No source specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param2)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <search> No action specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
			if (!param3)	{ bErr = 1 ; m_pLog->Out("%s. File %s line %d: <search> No criteria specified\n", *_fn, pN->Fname(), pN1->Line()) ; }
		}
		else
		{
			m_pLog->Out("%s. File %s Line %d: Unrecognized command <%s>\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ;
			rc = E_SYNTAX ;
		}

		//	If all OK, add the exec command to the form handler
		if (rc != E_OK || bErr)
			rc = E_SYNTAX ;
		else
		{
			execList.Add(pExec) ;
			pExec->m_FstParam = m_ExecParams.Count() ;

			if (nParams > 0)	{ m_pLog->Out("ExecParam %d %s\n", m_ExecParams.Count(), *param1) ; m_ExecParams.Add(param1) ; param1 = 0 ; }
			if (nParams > 1)	{ m_pLog->Out("ExecParam %d %s\n", m_ExecParams.Count(), *param2) ; m_ExecParams.Add(param2) ; param2 = 0 ; }
			if (nParams > 2)	{ m_pLog->Out("ExecParam %d %s\n", m_ExecParams.Count(), *param3) ; m_ExecParams.Add(param3) ; param3 = 0 ; }
			if (nParams > 3)	{ m_pLog->Out("ExecParam %d %s\n", m_ExecParams.Count(), *param4) ; m_ExecParams.Add(param4) ; param4 = 0 ; }
			if (nParams > 4)	{ m_pLog->Out("ExecParam %d %s\n", m_ExecParams.Count(), *param5) ; m_ExecParams.Add(param5) ; param5 = 0 ; }
			if (nParams > 5)	{ m_pLog->Out("ExecParam %d %s\n", m_ExecParams.Count(), *param6) ; m_ExecParams.Add(param6) ; param6 = 0 ; }
		}
	}

	return rc ;
}

hzEcode	hdsApp::_readFormHdl	(hzXmlNode* pN)
{
	//	Process a <xformHdl> tag to create a hdsFormhdl (form handler) instance.
	//
	//	Form handlers are always invoked upon form submissions to process the submitted data and formulate a HTML response. The configs for the form handler set
	//	out a series of processing instructions. Each of these may contain a response directive in the event of failure. There is also a response directive for
	//	failure of the series as a whole and another for success. To see the latter, every instruction in the series must succeed.
	//
	//	Argument:	pN	Pointer to XML node <formhdl>
	//
	//	Returns:	E_SYNTAX	In the event of incorrect parameters
	//				E_OK		If the <formhdl> parameters are valid

	_hzfunc("hdsApp::_readFormHdl") ;

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Current XML node
	hdsFormdef*		pForm ;			//	Named form (which must per-exist)
	hdsFormhdl*		pFhdl ;			//	Form handler
	hzString		hname ;			//	Name of form handler
	hzString		fname ;			//	Name of form
	hzString		fops ;			//	Operational flags
	uint32_t		state = 0 ;		//	Instruction context
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;

	m_pLog->Out("%s. Node %s line %d\n", *_fn, pN->TxtName(), pN->Line()) ;

	//	The attr will be the name of the form to which the handler applies
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))	hname = ai.Value() ;
		else if (ai.NameEQ("form"))	fname = ai.Value() ;
		else if (ai.NameEQ("ops"))	fops = ai.Value() ;
		else
		{
			m_pLog->Out("%s. File %s Line %d Reading <formhdl> tag: Bad attribute (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
			return E_SYNTAX ;
		}
	}

	if (!hname)	{ m_pLog->Out("%s. File %s Line %d <formhdl> No form handler name supplied\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }
	if (!fname)	{ m_pLog->Out("%s. File %s Line %d <formhdl> No form name supplied\n", *_fn, pN->Fname(), pN->Line()) ; return E_SYNTAX ; }

	pForm = m_FormDefs[fname] ;
	if (!pForm)
		{ m_pLog->Out("%s. File %s Line %d <formhdl> Form %s not found\n", *_fn, pN->Fname(), pN->Line(), *fname) ; return E_SYNTAX ; }

	if (!m_FormHdls.Exists(hname))
	{
		m_pLog->Out("%s. File %s Line %d. No formhandler of %s expected\n", *_fn, pN->Fname(), pN->Line(), *hname) ;
		return E_NOTFOUND ;
	}

	pFhdl =  m_FormHdls[hname] ;
	if (pFhdl)
	{
		m_pLog->Out("%s. File %s Line %d. Already have a formhandler named %s\n", *_fn, pN->Fname(), pN->Line(), *hname) ;
		return E_DUPLICATE ;
	}

	pFhdl = new hdsFormhdl() ;
	pFhdl->m_Refname = hname ;
	pFhdl->m_pFormdef = pForm ;
	m_FormHdls.Insert(hname, pFhdl) ;

	if (fops)
	{
		if (fops != "cookie")
		{
			m_pLog->Out("%s. File %s Line %d. Only allowed value for ops attr is cookie. %s illegal\n", *_fn, pN->Fname(), pN->Line(), *fops) ;
			return E_SYNTAX ;
		}

		pFhdl->m_flgFH |= VE_COOKIES ;
	}

	//	Get the <procdata> and <response> directives
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("procdata"))
		{
			if (state & 1)
			{
				rc = E_SYNTAX ;
				m_pLog->Out("%s. File %s Line %d Reading <formhdl> tag: Only one <procdata> allowed\n", *_fn, pN->Fname(), pN1->Line()) ;
				break ;
			}
			state |= 1 ;
			rc = _readExec(pN1, pFhdl->m_Exec, 0, pFhdl) ;
			continue ;
		}

		if (pN1->NameEQ("response"))
		{
			if (state & 2)
			{
				rc = E_SYNTAX ;
				m_pLog->Out("%s. File %s Line %d Reading <formhdl> tag: Only one <response> or <showpage> allowed\n", *_fn, pN->Fname(), pN1->Line()) ;
				break ;
			}
			state |= 2 ;
			m_pLog->Out("%s. File %s Line %d: CALL _readResponse case 4\n", *_fn, pN->Fname(), pN1->Line()) ;
			rc = _readResponse(pN1, pFhdl, pFhdl->m_CompleteGoto, &pFhdl->m_pCompletePage) ;
			continue ;
		}

		if (pN1->NameEQ("error"))
		{
			if (state & 4)
			{
				rc = E_SYNTAX ;
				m_pLog->Out("%s. File %s Line %d Reading <formhdl> tag: Only one <error> allowed\n", *_fn, pN->Fname(), pN1->Line()) ;
				break ;
			}
			state |= 4 ;
			m_pLog->Out("%s. File %s Line %d: CALL _readResponse case 5\n", *_fn, pN->Fname(), pN1->Line()) ;
			rc = _readResponse(pN1, pFhdl, pFhdl->m_FailDfltGoto, &pFhdl->m_pFailDfltPage) ;
			continue ;
		}
	}

	if (rc != E_OK)
		return rc ;

	if (!pFhdl->m_pCompletePage && !pFhdl->m_CompleteGoto)
	{
		m_pLog->Out("%s. Form handler for form %s starting line %d has no success state response page or destintion\n", *_fn, *pFhdl->m_Refname, pN->Line()) ;
		rc = E_NODATA ;
	}

	if (!pFhdl->m_pFailDfltPage && !pFhdl->m_FailDfltGoto)
	{
		m_pLog->Out("%s. Form handler for form %s starting line %d has no failue state error page or destination\n",
			*_fn, *pFhdl->m_Refname, pN->Line(), *pFhdl->m_FailDfltGoto) ;
		rc = E_NODATA ;
	}

	m_pLog->Out("%s. Form handler form %s has success response/destination (%p, %s) and failure response/destination (%p, %s)\n",
		*_fn, *pFhdl->m_Refname, pFhdl->m_pCompletePage, *pFhdl->m_CompleteGoto, pFhdl->m_pFailDfltPage, *pFhdl->m_FailDfltGoto) ;

	m_pLog->Out("%s. Returning er=%s\n", *_fn, Err2Txt(rc)) ;

	return rc ;
}

hdsVE*	hdsApp::_readFormBut	(hzXmlNode* pN, hdsFormdef* pFormdef, hdsFormref* pFormref)
{
	//	Support function to hdsApp::_readFormDef called via hdsApp::_readTag, to add an action to the supplied form definition. 

	_hzfunc("hdsApp::_readFormBut") ;

	hzAttrset		ai ;				//	Attribute iterator
	hdsVE*			thisVE = 0 ;		//	Visible entity
	hdsButton*		pButt ;				//	Dissemino form button
	hzString		title ;				//	Text visible
	hzString		page ;				//	Expected page (to go to)
	hzString		whom ;				//	Allowed users
	hzString		css ;				//	Allowed CSS style
	hzString		hname ;				//	Form action (handler name)
	hzString		faUrl ;				//	Form action URL
	uint32_t		bErr = 0 ;			//	Error condition

	//hzXmlNode*		pN1 ;				//	Subtag probe
	//hdsVE*			newVE ;				//	Subtag visible entity
	//hdsRecap*		pRecap ;			//	Google recaptcha

	if (!pN->NameEQ("xformBut"))
		Fatal("%s. Wrong call\n", *_fn) ;

	if (!pFormdef)
		{ m_pLog->Out("%s. File %s Line %d <xformBut> No form definition applies\n", *_fn, pN->Fname(), pN->Line()) ; return 0 ; }

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("title"))	title = ai.Value() ;
		else if (ai.NameEQ("handler"))	hname = ai.Value() ;
		else if (ai.NameEQ("url"))		faUrl = ai.Value() ;
		else if (ai.NameEQ("css"))		css = ai.Value() ;
		else
		{
			bErr=1 ;
			m_pLog->Out("%s. File %s Line %d Adding <%s> tag: Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName(), ai.Name(), ai.Value()) ;
		}
	}

	if (!title)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xformBut> No title supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (hname || faUrl)
	{
		//	If either action or URL are supplied, both must be
		if (!hname)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> URL but no action supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
		if (!faUrl)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> Action but no URL supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	}

	if (hname && faUrl)
	{
		if (!m_FormHdls.Exists(hname))
		{
			m_FormHdls.Insert(hname,0) ;
			pFormdef->m_nActions++ ;
		}
		m_FormUrl2Hdl.Insert(faUrl,hname) ;
		m_FormUrl2Ref.Insert(faUrl, pFormref) ;
		m_FormRef2Url.Insert(pFormref, faUrl) ;
	}

	if (!hname && !faUrl)
	{
		//	OK for the button to have no action or submission URL as long as the form definition has these and the form is single action.
		if (!pFormdef->m_DfltAct)
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xformBut> No action supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
		else
		{
			hname = pFormdef->m_DfltAct ;
			faUrl = pFormdef->m_DfltURL ;
			pFormdef->m_DfltAct = 0 ;
			pFormdef->m_DfltURL = 0 ;
		}
	}

	if (bErr)
		return 0 ;

		//	if (page)
		//	{
			//	Lookup the page and check that it exists and is not the same as the page in which the button appears
			//	if (page != "%x_referer;" && !m_PagesName.Exists(page))
				//	m_pLog->Out("%s. Warning: File %s Line %d Adding <button> tag: Page %s not found\n", *_fn, pN->Fname(), pN->Line(), *page) ;
		//	}

	thisVE = pButt = new hdsButton(this) ;
	thisVE->m_strPretext = pN->TxtPtxt() ;
	thisVE->m_Line = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;
	thisVE->m_Tag = pN->TxtName() ;
	thisVE->m_Resv = pFormdef->m_nActions ;
	pButt->m_strContent = title ;
	pButt->m_CSS = css ;
	pButt->m_Linkto = hname ;
	pButt->m_Formname = pFormdef->m_Formname ;
	m_pLog->Out("%s. Added form button (%s)\n", *_fn, *pButt->m_strContent) ;

	return thisVE ;
}

hzEcode	hdsApp::_readFormDef	(hzXmlNode* pN)
{
	//	Process an <xformDef> tag to define a form (create a hdsFormdef instance), in the INDEPENDENT case where the <xformDef> tag lays outside any page or article definition. The
	//	expectation is that the defined form will later be referred to by means of an <xformRef> tag within one or more pages or articles.
	//
	//	In the independent case <xformDef> has three attributes of 'name' which names the form, 'class' which sets the host data class, and 'action' which names a yet to be defined
	//	form handler. The first two are compulsory but if the action is not set in the <xformDef> tag itself, it must be set for each active button in the form. Both the form name
	//	and form handler name(s) are required to be unique across the entire configs.
	//
	//	The following occurs within this function:
	//
	//		1)	A new form definition is created placed in hdsApp::m_FormDefs. This map is purely to ensure forms are given unique names.
	//
	//		2)	For each form action, a form handler is named and a URL supplied. The form handler name is added to hdsApp::m_FormHdl, ensuring it is unique. No actual form handler
	//			is created however. This must await processing of a later <xformHdl> tag. In the meantime the form handler pointer in the entry in hdsApp::m_FormHdl, is NULL.
	//
	//	If <xformDef> processes successfully, a form definition is created and entered into the hdsApp map m_FormDefs, thereby facilitation form referencing.
	//
	//	Argument:	pN		Current XML node
	//
	//	Returns:	E_SYNTAX	If supplied attributes or sub-tags are in error.
	//				E_OK		If the <xformDef> tag is processed successfully.

	_hzfunc("hdsApp::_readFormDef:1") ;

	_tagArg			tga ;				//	Tag argument for _readTag()
	hzAttrset		ai ;				//	Attribute iterator
	hzXmlNode*		pN1 ;				//	For processing child nodes
	hdsVE*			pVE ;				//	Generic visible entity
	hdsFormdef*		pFormdef ;			//	This form
	const hdbClass*	pClass = 0 ;		//	Data class refered to in xform declaration
	hzString		fname ;				//	Name (of this form)
	hzString		cname ;				//	Class name
	hzString		hname ;				//	Form handler name
	hzString		faUrl ;				//	Form action URL specified in <xformBut> sub-tag
	hzString		title ;				//	Button title
	hzString		page ;				//	Expected page (to go to)
	hzString		whom ;				//	Allowed users
	hzString		css ;				//	Allowed CSS style
	uint32_t		flags ;				//	For _readTag() call
	bool			bRecap = false ;	//	Google Recaptcha Indicator
	hzEcode			rc = E_OK ;			//	Return code

	if (!this)	Fatal("%s. No instance\n", *_fn) ;
	if (!pN)	Fatal("%s. No node supplied\n", *_fn) ;

	if (!pN->NameEQ("xformDef"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <xformDef>\n", *_fn, pN->TxtName()) ;

	m_pLog->Out("%s. File %s Line %d: XFORMDEF tag\n", *_fn, pN->Fname(), pN->Line()) ;

	//	Form parameters
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))			fname = ai.Value() ;
		else if (ai.NameEQ("class"))		cname = ai.Value() ;
		else if (ai.NameEQ("handler"))		hname = ai.Value() ;
		else if (ai.NameEQ("recaptcha"))	bRecap = ai.ValEQ("true") ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <form> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!fname)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xformDef> No name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!cname)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xformDef> No default class\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!hname)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xformDef> No action supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (m_FormDefs.Exists(fname))
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: Form %s already exists\n", *_fn, pN->Fname(), pN->Line(), *fname) ; }

	if (rc != E_OK)
		return rc ;

	//	Deal with the form's default class
	pClass = m_ADP.GetPureClass(cname) ;
	if (!pClass)
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <xformDef> Class %s does not exist\n", *_fn, pN->Fname(), pN->Line(), *cname) ; }
	m_pLog->Out("%s. File %s Line %d: XFORMDEF tag\n", *_fn, pN->Fname(), pN->Line()) ;

	if (rc != E_OK)
		return rc ;

	//	Create new form
	pFormdef = new hdsFormdef() ;
	pFormdef->m_Formname = fname ;
	pFormdef->m_DfltAct = hname ;
	pFormdef->m_pClass = pClass ;

	if (bRecap)
		pFormdef->m_bScriptFlags |= INC_SCRIPT_RECAPTCHA ;

	m_FormDefs.Insert(pFormdef->m_Formname, pFormdef) ;

	//	Check and add the form handler name
	if (!m_FormHdls.Exists(hname))
		m_FormHdls.Insert(hname, (hdsFormhdl*) 0) ;

	m_pLog->Out("%s. Added <xform name=%s>\n", *_fn, *pFormdef->m_Formname) ;

	//	Now process sub-nodes
	tga.m_pFormdef = pFormdef ;
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		pVE = _readTag(&tga, pN1, flags, 0, 0) ;

		if (!pVE)
			{ rc = E_SYNTAX ; break ; }

		pFormdef->m_VEs.Add(pVE) ;
		m_pLog->Out("%s. Added VE of %s\n", *_fn, *pVE->m_Tag) ;
	}

	if (rc != E_OK)
		delete pFormdef ;
	return rc ;
}

hdsFormref*	hdsApp::_readFormDef	(hzXmlNode* pN, hdsResource* pLR)
{
	//	Process an <xformDef> tag occuring WITHIN a page or article definition. This produces both a form definition (hdsFormdef) and a form reference (hdsFormref), as described in
	//	the section on Forms in the Library Overview.
	//
	//	Accordingly, in addition to the three attributes necessary for the form definition, the submission URL needed for the form reference 
	//
	//	For the form definition there are three attributes of 'name' which names the form, 'class' which sets the host data class, and 'action' which names a yet to be defined form
	//	handler. The first two are compulsory but if the action is not set in the <xformDef> tag itself, it must be set for each active button in the form. Both the form definition
	//	name and form handler name(s) are required to be unique across the entire configs.
	//
	//	For the form reference an additional attribute of 'url' is needed to supply the submission URL.
	//
	//	The following occurs within this function:
	//
	//		1)	A new form definition is created placed in hdsApp::m_FormDefs. This map is purely to ensure forms are given unique names.
	//
	//		2)	A new form reference is created and will point to the form definition. A pointer to this is returned if no errors occur.
	//
	//		3)	For each form action, a form handler is named and a URL supplied. The form handler name is added to hdsApp::m_FormHdl, ensuring it is unique. No actual form handler
	//			is created however. This must await processing of a <xformHdl> tag. In the meantime the form handler pointer in the entry in hdsApp::m_FormHdl, is NULL.
	//
	//		4)	The URL and the form handler name are added to hdsApp::m_FormUrl2Hdl
	//
	//		5)	Each form action URL, together with the form reference, is added to both hdsApp::m_FormUrl2Ref and hdsApp::m_FormRef2Url
	//
	//	Arguments:	1)	pN		Current XML node
	//				2)	pLR		Current locatable resoure (page or article)
	//
	//	Returns:	Pointer to the form reference
	//				NULL if a syntax error occurs 

	_hzfunc("hdsApp::_readFormDef:2") ;

	hzArray	<hzString>	hnames ;			//	Proposed form handler names

	_tagArg				tga ;				//	Tag argument for _readTag()
	hzAttrset			ai ;				//	Attribute iterator
	hzXmlNode*			pN1 ;				//	For processing child nodes
	hdsVE*				thisVE ;			//	Generic visible entity
	hdsVE*				pVE ;				//	Generic visible entity
	hdsFormdef*			pFormdef ;			//	This form definition
	hdsFormref*			pFormref ;			//	Implicit form reference
	const hdbClass*		pClass = 0 ;		//	Data class refered to in xform declaration
	hdsPage*			pPage ;				//	If locatable resource is a page
	hzString			fname ;				//	Name (of this form)
	hzString			cname ;				//	Class name
	hzString			hname ;				//	Form handler name
	hzString			faUrl ;				//	Form action URL
	hzString			title ;				//	Button title
	hzString			page ;				//	Expected page (to go to)
	hzString			whom ;				//	Allowed users
	hzString			css ;				//	Allowed CSS style
	uint32_t			flags ;				//	For _readTag() call
	uint32_t			bErr = 0 ;			//	Error condition
	bool				bRecap = false ;	//	Google Recaptcha Indicator

	//	Check params
	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;

	if (!pN->NameEQ("xformDef"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <xformDef>\n", *_fn, pN->TxtName()) ;

	if (!pLR)
		Fatal("%s. No locatable resource supplied\n", *_fn, pN->TxtName()) ;

	pPage = dynamic_cast<hdsPage*>(pLR) ;

	//	Create new form definition and a form reference
	pFormdef = new hdsFormdef() ;
	pFormref = new hdsFormref(this) ;
	m_pLog->Out("%s. File %s Line %d: XFORMDEF tag (%p in-page ref at %p)\n", *_fn, pN->Fname(), pN->Line(), pFormdef, pFormref) ;
	thisVE = pFormref ;
	thisVE->m_strPretext = pN->TxtPtxt() ;
	pFormref->m_Line = pN->Line() ;
	pFormref->m_Indent = pN->Level() ;
	pFormref->m_Tag = pN->TxtName() ;
	pFormref->m_Formname = pFormdef->m_Formname ;

	//	Process attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))			fname = ai.Value() ;
		else if (ai.NameEQ("class"))		cname = ai.Value() ;
		else if (ai.NameEQ("handler"))		hname = ai.Value() ;
		else if (ai.NameEQ("url"))			faUrl = ai.Value() ;
		else if (ai.NameEQ("recaptcha"))	bRecap = ai.ValEQ("true") ;
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <form> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!fname)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> No form name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!cname)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> No native class supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (hname || faUrl)
	{
		//	If either action or URL are supplied, both must be
		if (!hname)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> URL supplied but no form handler\n", *_fn, pN->Fname(), pN->Line()) ; }
		if (!faUrl)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> Form handler supplied but no URL\n", *_fn, pN->Fname(), pN->Line()) ; }
	}

	if (m_FormDefs.Exists(fname))
		{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: Form %s already exists\n", *_fn, pN->Fname(), pN->Line(), *fname) ; }

	//	Deal with the form's default class
	pClass = m_ADP.GetPureClass(cname) ;
	if (!pClass)
		{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformDef> Class %s does not exist\n", *_fn, pN->Fname(), pN->Line(), *cname) ; }

	//	Check form handler name and URL are either both present or both absent
	if (hname)
	{
		if (!faUrl)
			{ faUrl = hname ; m_pLog->Out("%s. File %s Line %d: NOTE: URL assuming Form handler name of %s\n", *_fn, pN->Fname(), pN->Line(), *hname) ; }

		if (faUrl && m_FormUrl2Hdl.Exists(faUrl))
		{
			//	Note that it is not illegal to reuse a submission URL as long as it points to the same form handler
			if (hname != m_FormUrl2Hdl[faUrl])
			{
				bErr=1 ;
				m_pLog->Out("%s. File %s Line %d: Form action URL %s already points to another form handler (%s)\n",
					*_fn, pN->Fname(), pN->Line(), *faUrl, *m_FormUrl2Hdl[faUrl]) ;
			}
		}

		//	Reserve form handler name and associate URL and form handler.
		if (!bErr)
		{
			pFormdef->m_DfltAct = hname ;
			pFormdef->m_DfltURL = faUrl ;

			if (!m_FormHdls.Exists(hname))
			{
				m_FormHdls.Insert(hname,0) ;
				pFormdef->m_nActions++ ;
			}
			m_FormUrl2Hdl.Insert(faUrl,hname) ;
			m_FormUrl2Ref.Insert(faUrl, pFormref) ;
			m_FormRef2Url.Insert(pFormref, faUrl) ;
		}
	}
	else
	{
		if (faUrl)
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: Form action URL %s supplied but no form handler named\n", *_fn, pN->Fname(), pN->Line(), *hname) ; }
	}

	if (bErr)
	{
		delete pFormdef ;
		delete pFormref ;
		return 0 ;
	}

	//	Name the new form definition and copy the form definition name to the form reference
	pFormdef->m_Formname = fname ;
	pFormref->m_Formname = fname ;

	//	Set the form definition class
	pFormdef->m_pClass = pClass ;

	if (bRecap)
	{
		if (pPage)
			pPage->m_bScriptFlags |= INC_SCRIPT_RECAPTCHA ;
		pFormdef->m_bScriptFlags |= INC_SCRIPT_RECAPTCHA ;
	}
	m_FormDefs.Insert(pFormdef->m_Formname, pFormdef) ;

	if (pPage)
		pPage->m_xForms.Add(pFormref) ;

	if (pLR)
		pLR->m_flagVE |= VE_CT_ACTIVE ;

	if (pPage)
		m_pLog->Out("%s. Added <xform name=%s to page %s>\n", *_fn, *pFormdef->m_Formname, *pPage->m_Title) ;
	else
		m_pLog->Out("%s. Added <xform name=%s>\n", *_fn, *pFormdef->m_Formname) ;

	//	Now process sub-nodes (form substance)
	tga.m_pLR = pLR ;
	tga.m_pFormdef = pFormdef ;
	tga.m_pFormref = pFormref ;
	for (pN1 = pN->GetFirstChild() ; !bErr && pN1 ; pN1 = pN1->Sibling())
	{
		m_pLog->Out("%s. Processing a %s tag\n", *_fn, pN1->TxtName()) ;

		pVE = _readTag(&tga, pN1, flags, 0, 0) ;	//pLR, pFormdef, pFormref, 0, *_fn, 0) ;

		if (!pVE)
			{ bErr=1 ; break ; }

		pFormdef->m_VEs.Add(pVE) ;
		m_pLog->Out("%s. Added VE of %s\n", *_fn, *pVE->m_Tag) ;
	}
	m_pLog->Out("%s. File %s Line %d: XFORMDEF tag COMPLETE (in-page ref at %p)\n", *_fn, pN->Fname(), pN->Line(), pFormref) ;

	if (bErr)
		{ delete pFormdef ; delete pFormref ; return 0 ; }

	return pFormref ;
}

hdsFormref*	hdsApp::_readFormRef	(hzXmlNode* pN, hdsResource* pLR)
{
	//	Category:	Dissemino configs
	//
	//	Process a <xformRef> tag.
	//
	//	The <xformRef> tag imports a form as a complete, self contained visual entity into a page or article. Accordingly, the tag is legal only within the definitions of a page or
	//	article or within the definition of an entity that can only be a part of a page or article such as an include block. As the form is self contained, <xformRef> has no legal
	//	sub-tags. The form being imported must have been defined previously and in the project space, outside the definitions of any page or article.
	//
	//	The following occurs within this function:
	//
	//		1)	A new form reference is created and will point to the named form definition. A pointer to this is returned if no errors occur.
	//
	//		2)	For each form action listed in the form definition, a URL supplied. The URL and the form handler name are added to hdsApp::m_FormUrl2Hdl
	//
	//		3)	Each form action URL, together with the form reference, is added to both hdsApp::m_FormUrl2Ref and hdsApp::m_FormRef2Url
	//
	//	Arguments:	1)	pN		Current XML node
	//				2)	pLR		The host resource (page or article)
	//
	//	Returns:	Pointer to the form
	//				NULL if a syntax error occurs 

	_hzfunc("hdsApp::_readFormRef") ;

	hzArray<hzString>	urls ;		//	Form action URLs (form may be multiple action)

	hzAttrset		ai ;			//	Attribute iterator
	hdsFormref*		pFormref ;		//	Form reference
	hzString		fname ;			//	Name (of pre-defined form)
	hzString		faUrl ;			//	Form action URL
	hzString		faCtx ;			//	Form context
	hzString		hnam ;			//	Name of form reference (multiple button forms only)
	uint32_t		bErr = 0 ;		//	Error condition
	uint16_t		clsDtId ;		//	Data class delta id

	//	Check parameters
	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;

	if (!pN->NameEQ("xformRef"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <xformRef>\n", *_fn, pN->TxtName()) ;

	//	Allocate the form references and place them in hdsApp::m_FormRefs
	pFormref = new hdsFormref(this) ;
	pFormref->m_Line = pN->Line() ;
	pFormref->m_Indent = pN->Level() ;
	pFormref->m_Tag = pN->TxtName() ;

	//	Process <xformRef> attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("form"))		fname = ai.Value() ;
		else if (ai.NameEQ("context"))	faCtx = ai.Value() ;
		else if (ai.NameEQ("url"))
		{
			faUrl = ai.Value() ;
			if (m_FormUrl2Ref.Exists(faUrl))
				{ m_pLog->Out("%s. File %s Line %d: <xformRef> Form action URL %s already in use\n", *_fn, pN->Fname(), pN->Line(), *faUrl) ; return 0 ; }
			else
				m_FormUrl2Ref.Insert(faUrl, pFormref) ;
		}
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformRef> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!fname)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformRef> No form definition name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!faCtx)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformRef> No form context supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!faUrl)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xformRef> No submission URLs supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (bErr)
	{
		delete pFormref ;
		return 0 ;
	}

	//	Check form exists
	if (!m_FormDefs.Exists(fname))
		{ m_pLog->Out("%s. File %s Line %d: <xformRef> Form %s not yet defined\n", *_fn, pN->Fname(), pN->Line(), *fname) ; return 0 ; }

	pFormref->m_Formname = fname ;
	m_FormRef2Url.Insert(pFormref, faUrl) ;

	if (pLR)
		pLR->m_flagVE |= VE_CT_ACTIVE ;

	//	Check context exists
	pFormref->m_ClsId = clsDtId = m_ADP.GetDataClassID(faCtx) ;
	if (!pFormref->m_ClsId)
		{ m_pLog->Out("%s. File %s Line %d: <xformRef> Context %s not found\n", *_fn, pN->Fname(), pN->Line(), *faCtx) ; return 0 ; }

	m_pLog->Out("%s. File %s Line %d: <xformRef> Added OK to resource\n", *_fn, pN->Fname(), pN->Line()) ;
	return pFormref ;
}

hdsVE*	hdsApp::_readField	(hzXmlNode* pN, hdsFormdef* pFormdef)
{
	//	Process an <xfield> tag within a <xformDef> tag, thereby adding a HTML field (hdsField) to a HTML form (hdsFormdef).
	//
	//	A field within a HTML form must necessarily relate to a variable. This could be an independent previously declared variable or more usually, a member of
	//	a data class. Firstly an <xfield> tag must through its attributes, identify the variable concerned. Then it must supply all the necessary parameters for
	//	a viable HTML <input> tag. This set of parameters is known as the field specification.
	//
	//	For the avoidance of verbosity and to standardize field dimensions across the application, field specifictaions can be predefined using a <fldspec> tag.
	//	These can then be used in an <xfield> tag by setting the 'fldspec' attrubute to the field specification name. This though, is only one approach. One can
	//	always embrace verbosity and use the attributes of htype to specify HTML type and where applicable, attributes of rows, cols and size. In the case of a
	//	HTML selector, there may be an attribute of 'multiple'. Or in the common case where the variable concerned is a data class member, one can elect to use
	//	the member's default field specification. The information in this of course, has to come from somewhere. It is generally supplied as part of the member
	//	definition in the configs.
	//
	//	Two approaches to <xfield> have evolved. You may choose either the 'var' or the 'member' attribute to identify the variable. The former allows selection
	//	of a predefined field specification while the latter forces use of the member's default. Clearly, if the variable is a not a data class member, var must
	//	be used as there is no other way of identifying it.
	//
	//	Note that use of either a predefined or the member default field specification does not mean the parameters within it are immutable. HTML parameters can
	//	still be separately specified. Each hdsField instance contains a hdsFldspec instance, which this function populates. The selected field specifictaion is
	//	is copied into that of the hdsField. It is this copy the supply of further parameters overrides.
	//
	//	Arguments:	1)	pN		The current node
	//				2)	pForm	Current hdsFormdef in progress
	//
	//	Returns:	Pointer to the visible entity
	//				NULL if a syntax error occurs 

	_hzfunc("hdsApp::_readField") ;

	const hdbMember*	pMbr = 0 ;		//	Class member
	const hdbClass*		pClass = 0 ;	//	Data class refered to in xform declaration

	hdsField*	pFld = 0 ;		//	Mkapp form field
	hzAttrset	ai ;			//	Attribute iterator
	hzString	vnam ;			//	Variable name (from attr var) which can be standalone or class member
	hzString	spec ;			//	fldspec name (from attr spec)
	hzString	cnam ;			//	Variable name (from attr var)
	hzString	memb ;			//	Member name (if not to be infered from 'var' attribute)
	hzString	S ;				//	Temp string
	hzString	dtS ;			//	Data type string
	hzString	htS ;			//	Html type string
	uint32_t	nRows = 0 ;		//	No of rows (only if fldspec not supplied)
	uint32_t	nCols = 0 ;		//	No of cols (only if fldspec not supplied)
	uint32_t	nSize = 0 ;		//	Max size (only if fldspec not supplied)
	uint32_t	flags = 0 ;		//	Additional field flags (eg compulsory)
	uint32_t	bErr = 0 ;		//	Error condition

	//	Check node
	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;

	//	Check form (an <xfield> must be a subtag of <xformDef>)
	if (!pFormdef)
		{ m_pLog->Out("%s. Line %d: No form supplied\n", *_fn, pN->Line()) ; return 0 ; }

	//	Create hdsField instance
	pFld = new hdsField(this) ;
	pFld->m_strPretext = pN->TxtPtxt() ;
	pFld->m_Line = pN->Line() ;
	pFld->m_Indent = pN->Level() ;
	pFld->m_Tag = pN->TxtName() ;

	//	Obtain the settings. Note that in the 
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("var"))		vnam = ai.Value() ;
		else if (ai.NameEQ("fldspec"))	spec = ai.Value() ;
		else if (ai.NameEQ("class"))	cnam = ai.Value() ;
		else if (ai.NameEQ("member"))	memb = ai.Value() ;
		else if (ai.NameEQ("type"))		dtS = ai.Value() ;
		else if (ai.NameEQ("rows"))		nRows = atoi(ai.Value()) ;
		else if (ai.NameEQ("cols"))		nCols = atoi(ai.Value()) ;
		else if (ai.NameEQ("size"))		nSize = atoi(ai.Value()) ;
		else if (ai.NameEQ("data"))
		{
			pFld->m_Source = ai.Value() ;
			flags |= VE_CT_ACTIVE ;
		}
		else if (ai.NameEQ("flags"))
		{
			if		(ai.ValEQ("req"))			flags |= VE_COMPULSORY ;
			else if (ai.ValEQ("opt"))			flags &= ~VE_COMPULSORY ;
			else if (ai.ValEQ("multiple"))		flags |= VE_MULTIPLE ;
			else if (ai.ValEQ("unique"))		flags |= VE_UNIQUE ;
			else if (ai.ValEQ("unique/req"))	flags |= (VE_UNIQUE + VE_COMPULSORY) ;
			else
				{ m_pLog->Out("%s. File %s Line %d: <xfield> tag: Bad flag (%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Value()) ; bErr=1 ; }
		}
		else if (ai.NameEQ("newin"))
		{
			//	Expect arg to name a class.member
			S = ai.Value() ;
			if (!S.Contains(CHAR_PERIOD))
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xfield> newin attr must name class.member\n", *_fn, pN->Fname(), pN->Line()) ; }
			S.TruncateUpto(".") ;
			pClass = m_ADP.GetPureClass(S) ;
			if (!pClass)
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xfield> newin attr class %s not found\n", *_fn, pN->Fname(), pN->Line(), *S) ; }
		}
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xfield> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (bErr)
		goto failed ;

	//	Resolve/Validate parameters. ...
	if (memb)
	{
		//	The member attribute is supplied so no vnam or spec allowed. The member must be of the class in the form and the spec will be the member default
		if (vnam)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xfield> tag has 'member' attr. 'vnam' not permitted\n", *_fn, pN->Fname(), pN->Line()) ; }
		if (spec)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d <xfield> tag has 'member' attr. 'spec' not permitted\n", *_fn, pN->Fname(), pN->Line()) ; }

		//	Check the meber exists
		if (cnam)
		{
			//	Guest class
			pClass = m_ADP.GetPureClass(cnam) ;
			if (!pClass)
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xfield> class %s not found\n", *_fn, pN->Fname(), pN->Line(), *cnam) ; }
		}
		else
		{
			pClass = pFormdef->m_pClass ;
			if (!pClass)
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xfield> class not set in form\n", *_fn, pN->Fname(), pN->Line()) ; }
		}

		if (pClass)
		{
			pFld->m_pClass = pClass ;
			pMbr = pFld->m_pClass->GetMember(memb) ;
			if (!pMbr)
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xfield> NOTE no such class member as %s\n", *_fn, pN->Fname(), pN->Line(), *memb) ; }
			pFld->m_pMem = pMbr ;
		}

		if (pMbr)
		{
			pFld->m_Fldspec = *pMbr->GetSpec() ;
			pFld->m_Varname = pMbr->StrName() ;
		}

		if (bErr)
			goto failed ;

		m_pLog->Out("%s. Using member spec %s Set field %s rows=%d cols=%d size=%d flags=%08x dtype=%s htype=%d\n",
			*_fn, *pFld->m_Fldspec.m_Refname, *pFld->m_Varname, pFld->m_Fldspec.nRows, pFld->m_Fldspec.nCols,
			pFld->m_Fldspec.nSize, pFld->m_flagVE, pFld->m_Fldspec.m_pType->TxtTypename(), pFld->m_Fldspec.htype) ;
	}
	else if (spec)
	{
		//	A spec is supplied so vnam is needed, either to name the member or just be an independent variable
		if (!vnam)
			{ m_pLog->Out("%s. File %s Line %d: <xfield> tag has 'spec' attr but no 'vnam'\n", *_fn, pN->Fname(), pN->Line()) ; goto failed ; }

		if (htS || dtS)
			{ m_pLog->Out("%s. File %s Line %d: <xfield> spec (%s) not allowed with dtype or htype\n", *_fn, pN->Fname(), pN->Line(), *spec) ; goto failed ; }

		if (!m_Fldspecs.Exists(spec))
			{ m_pLog->Out("%s. File %s Line %d field fldspec %s not previously declared\n", *_fn, pN->Fname(), pN->Line(), *spec) ; goto failed ; }

		pFld->m_Fldspec = m_Fldspecs[spec] ;
		pFld->m_Varname = vnam ;
		pFld->m_flagVE |= flags ;

		m_pLog->Out("%s. Using pre-def spec %s Set field %s rows=%d cols=%d size=%d flags=%08x dtype=%s htype=%d\n",
			*_fn, *pFld->m_Fldspec.m_Refname, *pFld->m_Varname, pFld->m_Fldspec.nRows, pFld->m_Fldspec.nCols,
			pFld->m_Fldspec.nSize, pFld->m_flagVE, pFld->m_Fldspec.m_pType->TxtTypename(), pFld->m_Fldspec.htype) ;
	}
	else
	{
		//	No member or pre-defined field specification supplied, must have the HTML params
		if (!nRows || !nCols || !nSize || !dtS)
		{
			if (!vnam)	m_pLog->Out("%s. File %s Line %d: xfield is missing variable name\n", *_fn, pN->Fname(), pN->Line()) ;
			if (!nRows)	m_pLog->Out("%s. File %s Line %d: xfield is missing rows attribte\n", *_fn, pN->Fname(), pN->Line()) ;
			if (!nCols)	m_pLog->Out("%s. File %s Line %d: xfield is missing cols attribte\n", *_fn, pN->Fname(), pN->Line()) ;
			if (!nSize)	m_pLog->Out("%s. File %s Line %d: xfield is missing size attribte\n", *_fn, pN->Fname(), pN->Line()) ;
			if (!dtS)	m_pLog->Out("%s. File %s Line %d: xfield is missing type attribte\n", *_fn, pN->Fname(), pN->Line()) ;

			goto failed ;
		}

		pFld->m_Fldspec.m_pType = m_ADP.GetDatatype(dtS) ;
		if (!pFld->m_Fldspec.m_pType)
			{ m_pLog->Out("%s. File %s Line %d: xfield %s: Illegal fld type %s\n", *_fn, pN->Fname(), pN->Line(), *vnam, *dtS) ; goto failed ; }

		pFld->m_Fldspec.m_Refname = pFormdef->m_Formname + "->" + vnam ;
		pFld->m_Varname = vnam ;
		pFld->m_flagVE |= flags ;
		pFld->m_Fldspec.nRows = nRows ;
		pFld->m_Fldspec.nCols = nCols ;
		pFld->m_Fldspec.nSize = nSize ;
	}

	if (bErr)
		goto failed ;

	if (vnam)
	{
		if (vnam.Contains("."))
		{
			if (!pFormdef->m_pClass)
			{
				m_pLog->Out("%s. File %s Line %d: <xfield> Note the form var=\"composite.member\" is not allowed where the parent <xformDef> lacks a class\n",
					*_fn, pN->Fname(), pN->Line()) ;
				goto failed ;
			}

			cnam = vnam ;
			cnam.TruncateUpto(".") ;
			memb = vnam ;
			memb.TruncateBeyond(".") ;

			pMbr = pFormdef->m_pClass->GetMember(cnam) ;
			if (!pMbr)
			{
				m_pLog->Out("%s. File %s Line %d: <xfield> %s is not a member of class %s\n", *_fn, pN->Fname(), pN->Line(), *cnam, pFormdef->m_pClass->TxtTypename()) ;
				goto failed ;
			}

			//	Now set the class of this <xfield> to that of the composite member
			if (pMbr->Basetype() != BASETYPE_CLASS)
			{
				m_pLog->Out("%s. File %s Line %d: <xfield> %s has an atomic datatype of %s and so is non-coposite\n", *_fn, pN->Fname(), pN->Line(), *cnam) ;
				goto failed ;
			}
			pFld->m_pClass = (hdbClass*) pMbr->Datatype() ;

			//	Now set the member of this <xfield> to that of the composite's member (RHS)
			pFld->m_pMem = pFld->m_pClass->GetMember(memb) ;
		}
		else
		{
			if (pFormdef->m_pClass)
			{
				pFld->m_pClass = pFormdef->m_pClass ;
				pMbr = pFld->m_pClass->GetMember(vnam) ;
				if (!pMbr)
					m_pLog->Out("%s. File %s Line %d: <xfield> NOTE no such class member as %s\n", *_fn, pN->Fname(), pN->Line(), *vnam) ;
				pFld->m_pMem = pMbr ;
			}
		}
	}

	if (bErr)
		goto failed ;

	if (!pFormdef->m_mapFlds.Exists(pFld->m_Varname))
		pFormdef->m_mapFlds.Insert(pFld->m_Varname, pFld) ;
	pFormdef->m_vecFlds.Add(pFld) ;

	m_pLog->Out("%s. Added field %s fldspec %s rows=%d cols=%d size=%d flags=%08x dtype=%s htype=%d to form %s\n",
		*_fn, *pFld->m_Varname, *pFld->m_Fldspec.m_Refname, pFld->m_Fldspec.nRows, pFld->m_Fldspec.nCols, pFld->m_Fldspec.nSize, pFld->m_flagVE,
		pFld->m_Fldspec.m_pType->TxtTypename(), pFld->m_Fldspec.htype, *pFormdef->m_Formname) ;
	return pFld ;

failed:
	delete pFld ;
	return 0 ;
}

hdsVE*	hdsApp::_readXhide	(hzXmlNode* pN, hdsFormdef* pForm)
{
	//	Process an <xhide> tag.
	//
	//	This tag may only exist within an <xformDef> tag and has two attributes, 'name' and 'value'. Its effect is to insert a hidden feild into the output HTML
	//	of the form. The latter attribute is commonly a percent entity.
	//
	//	The <xhide> tag is sometimes used in Dissemino applications to give a path back to some starting point. This could be after the user either completes or
	//	aborts, a series of one or more forms, or it could be to 'rescue' the user after a timeout.
	//
	//	One example would be where a user is on a page they wish to comment on but to submit a comment they need to be logged in. They have either forgotton to
	//	log in or have been timed out. The login is available from the pull-down menu. By using such as <xhide name="lastpage" value="%x:referer;"> as a sub-tag
	//	of <xformDef> in the login page, the login form's 'Log-In' button can take the user back to the page of interest using the percent entity %e:lastpage; 
	//
	//	Hidden fields are notable because they have null field specifications. Only the m_Varname and m_Source members have values.
	//
	//	Arguments:	1)	pN		The current node
	//				2)	pForm	Current hdsFormdef in progress
	//
	//	Returns:	Pointer to the visible entity
	//				NULL if a syntax error occurs 

	_hzfunc("hdsApp::_readXhide") ;

	hzAttrset		ai ;		//	Attribute iterator
	hdsField*		pFld ;		//	Mkapp form field
	hzString		dtS ;		//	Data type/HTML type identifier (for hidden type)
	uint32_t		bErr = 0 ;	//	Error condition

	if (!this)	Fatal("%s. No instance\n", *_fn) ;
	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pForm)
		{ m_pLog->Out("%s. Line %d: No form supplied\n", *_fn, pN->Line()) ; return 0 ; }

	//	Create the field
	pFld = new hdsField(this) ;

	//	Create or fetch the field specification
	dtS = "string" ;
	pFld->m_Fldspec.m_pType = m_ADP.GetDatatype(dtS) ;
	pFld->m_Fldspec.m_Refname = "No refname (form _readXhide)" ;
	pFld->m_Fldspec.htype = HTMLTYPE_HIDDEN ;

	pFld->m_strPretext = pN->TxtPtxt() ;
	pFld->m_Line = pN->Line() ;
	pFld->m_Indent = pN->Level() ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))		pFld->m_Varname = ai.Value() ;
		else if (ai.NameEQ("value"))	pFld->m_Source = ai.Value() ;
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xhide> Bad param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!pFld->m_Varname)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xhide> No name supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!pFld->m_Source)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xhide> No source supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (bErr)
		{ delete pFld ; return 0 ; }

	if (!pForm->m_mapFlds.Exists(pFld->m_Varname))
		pForm->m_mapFlds.Insert(pFld->m_Varname, pFld) ;
	pForm->m_vecFlds.Add(pFld) ;

	m_pLog->Out("%s. Added hidden field %s src=%s to form %s\n", *_fn, *pFld->m_Varname, *pFld->m_Source, *pForm->m_Formname) ;
	return pFld ;
}

hzEcode	hdsApp::_readResponse	(hzXmlNode* pN, hdsFormhdl* pFhdl, hzString& pageGoto, hdsResource** pPageGoto)
{
	//	
	//	Read in all tags for a page (with or without forms). All tags begining with an 'x' are mkapp active tags (eg xform and xfield) and these
	//	tags produce the associated mkapp classes. All other tags must be valid HTML tags and are there only for presentation. Since the config
	//	files are XML a successful load means all the tags are balenced. This means we only need to mirror the structure of XML tags to visual
	//	entities.
	//
	//	Arguments:	1)	pN		Current XML node expected to be <response> tag
	//				2)	pFhdl	Applicable form handler
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readResponse") ;

	_tagArg			tga ;			//	Tag argument for _readTag()
	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Subtag probe
	hdsResource*	pRes = 0 ;		//	Resource
	hdsPage*		pPage = 0 ;		//	Page in progress
	hdsVE*			pVE ;			//	Set if a HTML entity is found
	hzString		name ;			//	Page name from params
	hzString		path ;			//	Page URL from params
	hzString		color ;			//	Background color
	hzString		gopag ;			//	Goto (page name)
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;

	if (!pN->NameEQ("response") && !pN->NameEQ("error"))
		Fatal("%s. Wrong Call - Expected <response> or <error>, got <%s>\n", *_fn, pN->TxtName()) ;

	if (!pFhdl)
		Fatal("%s. No form supplied for <response>\n", *_fn) ;

	m_pLog->Out("%s. Node %s line %d\n", *_fn, pN->TxtName(), pN->Line()) ;

	//	Page attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("name"))		name = ai.Value() ;
		else if (ai.NameEQ("path"))		path = ai.Value() ;
		else if (ai.NameEQ("bgcolor"))	color = ai.Value() ;
		else if (ai.NameEQ("goto"))		gopag = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <response> Invalid param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (rc != E_OK)
		return rc ;

	//	Validate attribute combinations. If there is a goto (page) set, we need to check if its value is either 'hostpage' (meaning the page hosting
	//	the current form) or '%lastpage' (meaning the referer to the hostpage) or that the page already exists as a defined page. If none of these
	//	conditions are met, this is an error. Also if there is a goto set then there should not be any other attribute since we are not defining a
	//	response or error page!

	if (gopag)
	{
		pageGoto = gopag ;

		//	if (gopag[0] == CHAR_PERCENT)
		//		pageGoto = gopag ;
		//	else

		if (gopag[0] != CHAR_PERCENT)
		{
			//pRes = m_PagesPath[gopag] ;
			pRes = m_ResourcesPath[gopag] ;
			if (!pRes)
				m_pLog->Out("%s. File %s Line %d: WARNING Response page goto directive (%s) does not exist\n", *_fn, pN->Fname(), pN->Line(), *gopag) ;
			else
				*pPageGoto = pRes ;
		}

		if (name || path || color)
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: Response page goto directive excludes other attributes\n", *_fn, pN->Fname(), pN->Line()) ; }

		if (pN->GetFirstChild())
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: Response page goto directive excludes other subtags\n", *_fn, pN->Fname(), pN->Line()) ; }

		return rc ;
	}

	//	There is no goto set so we are defining a response or an error page
	if (!name)
	{
		rc = E_SYNTAX ;
		m_pLog->Out("%s. File %s Line %d: Response has no name\n", *_fn, pN->Fname(), pN->Line()) ;
	}
	else
	{
		//	A response page must have a name but not that of an existing page (the opposite of the goto scenario).
		//if (m_PagesName.Exists(name))
		if (m_ResourcesName.Exists(name))
			{ rc = E_DUPLICATE ; m_pLog->Out("%s. File %s Line %d: Response page name (%s) already used by a formal page\n", *_fn, pN->Fname(), pN->Line(), *name) ; }

		if (m_Responses.Exists(name))
			{ rc = E_DUPLICATE ; m_pLog->Out("%s. File %s Line %d: Response page name (%s) already used as a response page\n", *_fn, pN->Fname(), pN->Line(), *name) ; }
	}

	if (rc != E_OK)
		return rc ;

	pPage = new hdsPage(this) ;
	pN->Export(pPage->m_XML) ;
	pPage->m_MD5.CalcMD5(pPage->m_XML) ;
	pPage->m_Title = name ;
	pPage->m_Url = path ;
	pPage->m_Access = pFhdl->m_Access ;
	if (color)
		IsHexnum(pPage->m_BgColor, *color) ; 
	pPage->m_Line = pN->Line() ;
	m_Responses.Insert(name, pPage) ;

	*pPageGoto = pPage ;
	
	//	Page forms and other objects
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		m_pLog->Out("%s. File %s Line %d:%d Tag is %s\n", *_fn, pN->Fname(), pN1->Line(), pN1->Level(), pN1->TxtName()) ;
	}

	tga.m_pLR = pPage ;
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("desc"))
			{ pPage->m_Desc = pN1->m_fixContent ; continue ; }

		pVE = _readTag(&tga, pN1, pPage->m_bScriptFlags, 0, 0) ;	//pPage, 0, 0, 0, *_fn, 0) ;

		if (!pVE)
			rc = E_SYNTAX ;
		else
			pPage->AddVisent(pVE) ;
	}

	return rc ;
}

/*
**	SECTION 6:	Page and Article Config Read Functions
*/

hzEcode	hdsApp::_readPageBody	(hdsPage* pPage, hzXmlNode* pN)
{
	//	Category:	Page and Article Config Read Functions
	//
	//	Process an <xbody> tag to read a page body. The page body can contain a mixture of Dissemino and HTML tags.
	//
	//	Arguments:	1)	pN	Current XML node expected to be <xbody> tag
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readPageBody") ;

	hzVect<hzXmlNode*>	result ;	//	Used by FindSubnodes to extract bodytext

	_tagArg			tga ;			//	Tag argument for _readTag()
	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Subtag probe
	//hzXmlNode*		pN2 ;			//	Subtag probe
	//hzXmlNode*		pN3 ;			//	Subtag probe
	hdsVE*			pVE ;			//	Set if a HTML entity is found
	hzString		color ;			//	Background color
	//uint32_t		x ;				//	Tag iterator (for bodytext garnering)
	hzEcode			rc = E_OK ;		//	Return code
	//hzRecep32		r32 ;			//	For USL text value

	if (!pPage)	Fatal("%s. No page supplied\n", *_fn) ;
	if (!pN)	Fatal("%s. No node supplied\n", *_fn) ;

	if (!pN->NameEQ("xbody"))
		Fatal("%s. Wrong Call. Expected <xbody> got <%s>\n", *_fn, pN->TxtName()) ;

	m_cfgErr.Clear() ;

	//	Body attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("bgcolor"))		color = ai.Value() ;
		else if (ai.NameEQ("css"))			pPage->m_CSS = ai.Value() ;
		else if (ai.NameEQ("onpageshow"))	pPage->m_Onpage = ai.Value() ;
		else if (ai.NameEQ("onload"))		pPage->m_Onload = ai.Value() ;
		else if (ai.NameEQ("onresize"))		pPage->m_Resize = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_cfgErr.Printf("%s. File %s Line %d. Invalid <xbody> param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (rc != E_OK)
		return rc ;

	if (color)
		IsHexnum(pPage->m_BgColor, *color) ; 

	//	Page forms and other objects
	tga.m_pLR = pPage ;
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if		(pN1->NameEQ("xformDef"))	pVE = _readFormDef(pN1, pPage) ;
		else if	(pN1->NameEQ("xformRef"))	pVE = _readFormRef(pN1, pPage) ;
		else
			pVE = _readTag(&tga, pN1, pPage->m_bScriptFlags, 0, 0) ;

		if (!pVE)
			rc = E_SYNTAX ;
		else
			pPage->AddVisent(pVE) ;
	}

	return rc ;
}

hzEcode	hdsApp::_readPage	(hzXmlNode* pN)
{
	//	Category:	Page and Article Config Read Functions
	//
	//	Process an <xpage> tag (a web page definition).  (with or without forms). All tags begining with an 'x' are Dissemino active tags (eg xform and xfield) and these
	//	tags produce the associated Dissemino classes. All other tags must be valid HTML tags and are there only for presentation. Since the config
	//	files are XML a successful load means all the tags are balanced. This means we only need to mirror the structure of XML tags to visual
	//	entities.
	//
	//	Arguments:	1)	pN	Current XML node expected to be <xpage> tag
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readPage") ;

	hzVect<hzXmlNode*>	result ;	//	Used by FindSubnodes to extract bodytext

	_tagArg			tga ;			//	Tag argument for _readTag()
	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN1 ;			//	Subtag probe
	hzXmlNode*		pN2 ;			//	Subtag probe
	hzXmlNode*		pN3 ;			//	Subtag probe
	hdsPage*		pPage ;			//	Page in progress
	hdsVE*			pVE ;			//	Set if a HTML entity is found
	hzString		whom ;			//	Page access criteria from params
	hzString		color ;			//	Background color
	uint32_t		x ;				//	Tag iterator (for bodytext garnering)
	bool			bIndex = true ;	//	Index page by default
	bool			bLang = true ;	//	Language support by default
	hzEcode			rc = E_OK ;		//	Return code
	hzRecep32		r32 ;			//	For USL text value

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("xpage"))
		Fatal("%s. Wrong Call. Expected <xpage> got <%s>\n", *_fn, pN->TxtName()) ;

	m_cfgErr.Clear() ;
	pPage = new hdsPage(this) ;

	//	Page attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if (ai.NameEQ("ops"))
		{
			if (ai.ValEQ("noindex"))
				bIndex = false ;
		}
		else if (ai.NameEQ("title"))		pPage->m_Title = ai.Value() ;
		else if (ai.NameEQ("path"))			pPage->m_Url = ai.Value() ;
		else if (ai.NameEQ("subject"))		pPage->m_Subj = ai.Value() ;
		else if (ai.NameEQ("access"))		whom = ai.Value() ;
		else if (ai.NameEQ("lang"))			bLang = ai.ValEQ("on") ;
		else
		{
			rc = E_SYNTAX ;
			m_cfgErr.Printf("%s. File %s Line %d. Invalid <xpage> param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
		}
	}

	if (!pPage->m_Title)	{ rc = E_SYNTAX ; m_cfgErr.Printf("%s. File %s Line %d. No title supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!pPage->m_Url)		{ rc = E_SYNTAX ; m_cfgErr.Printf("%s. File %s Line %d. No URL supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (!whom)
		{ rc = E_SYNTAX ; m_cfgErr.Printf("%s. File %s Line %d. No access criteria supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	else
	{
		pPage->m_Access = _calcAccessFlgs(whom) ;
		if (pPage->m_Access == 0xffffffff)
			{ rc = E_SYNTAX ; m_cfgErr.Printf("%s. File %s Line %d: Bad access specification (%s)\n", *_fn, pN->Fname(), pN->Line(), *whom) ; }
	}

	if (rc != E_OK)
		{ delete pPage ; return rc ; }

	//	Check against existing pages. This function could be reloading the page as a result of an edit config in which case the page may exist. If not reloading
	//	then the page must not exist by name or path or be a response page. If reloading the page can exist by name, path or both but if by both, then the page
	//	by name must be the same as that by path.

	if (!m_nLoadComplete)
	{
		if (m_ResourcesPath.Exists(pPage->m_Url))
		{
			m_cfgErr.Printf("%s File %s Line %d: Illegal duplicate URL (%s) of an earlier page\n", *_fn, pN->Fname(), pN->Line(), *pPage->m_Url) ;
			rc = E_DUPLICATE ;
		}

		if (m_ResourcesName.Exists(pPage->m_Title))
		{
			m_cfgErr.Printf("%s File %s Line %d: Illegal duplicate title (%s) of an earlier page\n", *_fn, pN->Fname(), pN->Line(), *pPage->m_Title) ;
			rc = E_DUPLICATE ;
		}
	}

	if (rc != E_OK)
		{ delete pPage ; return rc ; }

	pN->Export(pPage->m_XML) ;
	pPage->m_MD5.CalcMD5(pPage->m_XML) ;
	if (color)
		IsHexnum(pPage->m_BgColor, *color) ; 
	pPage->m_Line = pN->Line() ;
	if (bLang)
		pPage->m_flagVE |= VE_LANG ;

	pPage->m_USL.SetPage(pPage->m_EID) ;

	//	Page forms and other objects
	tga.m_pLR = pPage ;
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("desc"))		{ pPage->m_Desc = pN1->m_fixContent ; continue ; }
		if (pN1->NameEQ("keys"))		{ pPage->m_Keys = pN1->m_fixContent ; continue ; }
		if (pN1->NameEQ("xbody"))		{ rc = _readPageBody(pPage, pN1) ; continue ; }
		if (pN1->NameEQ("procdata"))	{ rc = _readExec(pN1, pPage->m_Exec, pPage, 0) ; continue ; }
		if (pN1->NameEQ("xtreeDcl"))	{ rc = _readXtreeDcl(pN1, pPage) ; continue ; }

		if		(pN1->NameEQ("xformDef"))	pVE = _readFormDef(pN1, pPage) ;
		else if	(pN1->NameEQ("xformRef"))	pVE = _readFormRef(pN1, pPage) ;
		else
			pVE = _readTag(&tga, pN1, pPage->m_bScriptFlags, 0, 0) ;

		if (!pVE)
			rc = E_SYNTAX ;
		else
			pPage->AddVisent(pVE) ;
	}

	if (rc != E_OK)
		return rc ;

	//	Get the body text (everything within <p> tags)
	if (bIndex)
	{
		for (pN1 = pN->GetFirstChild() ; pN1 ; pN1 = pN1->Sibling())
		{
			pN1->FindSubnodes(result, "p") ;

			for (x = 0 ; x < result.Count() ; x++)
			{
				pN2 = result[x] ;
				for (pN3 = pN2->GetFirstChild() ; pN3 ; pN3 = pN3->Sibling())
				{
					pPage->m_Bodytext << pN3->TxtPtxt() ;
					pPage->m_Bodytext.AddByte(CHAR_NL) ;
				}
				pPage->m_Bodytext << pN2->m_fixContent ;
			}
		}
	}

	rc = m_vecPages.Add(pPage) ;
	if (rc == E_OK)
		rc = _insertPage(pPage, *_fn) ;

	if (rc == E_OK)
		m_pLog->Out("%s. Added page name %s %s (%s) with XML of %d bytes. Now have (%d names %d urls)\n",
			*_fn, pPage->m_USL.Txt(r32), *pPage->m_Url, *pPage->m_Title, pPage->m_XML.Size(), m_ResourcesName.Count(), m_ResourcesPath.Count()) ;
			//*_fn, pPage->m_USL.Txt(r32), *pPage->m_Url, *pPage->m_Title, pPage->m_XML.Size(), m_PagesName.Count(), m_PagesPath.Count()) ;

	return E_OK ;
}

hzEcode	hdsApp::_readStdLogin	(hzXmlNode* pN)
{
	//	Category:	Dissemino Configuration
	//
	//	Read in the standard login declaration which defines the URLs for the following:-
	//
	//		- the URL for login form submissions
	//		- the URL to go to in the event of authentication failure
	//		- the URL to go to in the event of authentication success
	//		- the URL to go to upon session resumption
	//
	//	Argument:	pN	Current XML node expected to be <loginpage> tag
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readStdLogin") ;

	hzAttrset	ai ;			//	Attribute iterator
	hzString	urlForm ;		//	Form submission URL
	hzString	urlFail ;		//	Form submission URL
	hzString	urlAuth ;		//	Form submission URL
	hzString	urlResume ;		//	Form submission URL
	hzEcode		rc = E_OK ;		//	Return code

	if (!pN)					Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("login"))	Fatal("%s. Wrong Call. Expected <loginpage> got <%s>\n", *_fn, pN->TxtName()) ;

	if (m_LoginPost)
	{
		m_pLog->Out("%s. File %s Line %d <login> tag: Illegal Duplicate\n", *_fn, *pN->Filename(), pN->Line()) ;
		return E_DUPLICATE ;
	}

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("formURL"))		urlForm = ai.Value() ;
		else if (ai.NameEQ("failURL"))		urlFail = ai.Value() ;
		else if (ai.NameEQ("authURL"))		urlAuth = ai.Value() ;
		else if (ai.NameEQ("resumeURL"))	urlResume = ai.Value() ;
		else
		{
			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d <login> tag: Invalid param (%s=%s)\n", *_fn, *pN->Filename(), pN->Line(), ai.Name(), ai.Value()) ;
		}
	}

	//	Test attributes
	if (!urlForm)	{ rc = E_NODATA ; m_pLog->Out("%s File %s Line %d: No login form URL supplied\n", *_fn, *pN->Filename(), pN->Line()) ; }
	if (!urlFail)	{ rc = E_NODATA ; m_pLog->Out("%s File %s Line %d: No login failure URL supplied\n", *_fn, *pN->Filename(), pN->Line()) ; }
	if (!urlAuth)	{ rc = E_NODATA ; m_pLog->Out("%s File %s Line %d: No login success URL supplied\n", *_fn, *pN->Filename(), pN->Line()) ; }
	if (!urlResume)	{ rc = E_NODATA ; m_pLog->Out("%s File %s Line %d: No login resume URL supplied\n", *_fn, *pN->Filename(), pN->Line()) ; }

	if (rc != E_OK)
		return rc ;

	return SetLoginPost(urlForm, urlFail, urlAuth, urlResume) ;
}

hzEcode	hdsApp::_readLogout	(hzXmlNode* pN)
{
	//	Category:	Page and Article Config Read Functions
	//
	//	When a user logs out, they cannot remain on a page that requires a login to view. It is essential the user lands on a publicly available page, such as the home page or the
	//	login page.
	//
	//	Arguments:	1)	pN	Current XML node expected to be <logout> tag
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readLogout") ;

	hzAttrset		ai ;			//	Attribute iterator
	hzString		path ;			//	Page URL from params
	hzString		subj ;			//	Page subject (for classification of material)
	hzString		dest ;			//	Destination page on logout (must already exist as a defined page)
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN)					Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("logout"))	Fatal("%s. Wrong Call. Expected <logout> got <%s>\n", *_fn, pN->TxtName()) ;

	//	Page attributes
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("path"))		path = ai.Value() ;
		else if (ai.NameEQ("subject"))	subj = ai.Value() ;
		else if (ai.NameEQ("goto"))		dest = ai.Value() ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <logout> Invalid param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!path)	{ rc = E_NODATA ; m_pLog->Out("%s File %s Line %d: No logout submission URL supplied\n", *_fn, pN->Fname(), pN->Line()) ; }
	if (!dest)	{ rc = E_NODATA ; m_pLog->Out("%s File %s Line %d: No logout destination URL supplied\n", *_fn, pN->Fname(), pN->Line()) ; }

	if (rc != E_OK)
		return rc ;

	//	Test for duplicate names or URLs
	//if (m_PagesPath.Exists(path))
	if (m_ResourcesPath.Exists(path))
		{ m_pLog->Out("%s File %s Line %d: Illegal duplicate URL (%s) of an earlier page\n", *_fn, pN->Fname(), pN->Line(), *path) ; rc = E_DUPLICATE ; }
	//if (!m_PagesPath.Exists(dest))
	if (m_ResourcesPath.Exists(dest))
		{ m_pLog->Out("%s File %s Line %d: Stated destination page (%s) does not exist\n", *_fn, pN->Fname(), pN->Line(), *dest) ; rc = E_DUPLICATE ; }

	//	Set logout path
	if (rc == E_OK)
	{
		m_LogoutURL = path ;
		m_LogoutDest = dest ;
	}

	return rc ;
}

hzEcode	hdsApp::LoadPassives	(void)
{
	//	Category:	Page and Article Config Read Functions
	//
	//	Load all passive HTML files found in the document root (recurses to sub-dirs)
	//	
	//	Arguments:	None
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::loadPassives") ;

	hzList<hzPair>::Iter	lp ;	//	Passives iterator

	hzVect	<hzString>	dirs ;		//	Needed for ListDir call
	hzArray	<hzString>	files ;		//	Passive files matching the criteria

	ifstream	is ;				//	For reading in file
	const char*	i ;					//	Filename with/without leading slash
	hdsFile*	pFile ;				//	Passive file
	hzChain		Z ;					//	For storing file content
	hzPair		p ;					//	File criteria and HTML type
	hzString	S ;					//	Current criteria
	hzString	path ;				//	Current path
	hzString	fpath ;				//	Current full path (docroot + path)
	uint32_t	n ;					//	File iterator
	hzEcode		rc ;				//	Return code

	for (lp = m_Passives ; lp.Valid() ; lp++)
	{
		p = lp.Element() ;

		i = *p.name ;
		if (*i == CHAR_FWSLASH)
			i++ ;
		rc = FindfilesStd(files, i) ;

		if (rc == E_NOTFOUND)
		{
			m_pLog->Out("WARNING: file garner: Cannot find files matching %s\n", *p.name) ;
			continue ;
		}

		if (rc != E_OK)
		{
			m_pLog->Out("ERROR in file garner: Cannot utilize [%s], error=%s\n", *S, Err2Txt(rc)) ;
			break ;
		}

		for (n = 0 ; n < files.Count() ; n++)
		{
			S = files[n] ;
			is.open(*S) ;
			if (is.fail())
				{ m_pLog->Out("WARNING: file %s cannot be opened\n", *p.name) ; continue ; }
			Z << is ;
			is.close() ;
			is.clear() ;

			if (memcmp(*S, *m_Docroot, m_Docroot.Length()))
				m_pLog->Out("Please check basis for file [%s]\n", *p.name) ;
			else
			{
				pFile = new hdsFile() ;
				pFile->m_filepath = p.name ;
				pFile->m_Mimetype = Str2Mimetype(p.value) ;

				pFile->m_rawValue = Z ;
				if (pFile->m_Mimetype == HMTYPE_TXT_HTML)
					Gzip(pFile->m_zipValue, Z) ;

				//m_Fixed.Insert(pFile->m_filepath, pFile) ;
				m_ResourcesPath.Insert(pFile->m_filepath, pFile) ;
				m_pLog->Out("Added passive file %s (%d,%d)\n", *pFile->m_filepath, pFile->m_rawValue.Size(), pFile->m_zipValue.Size()) ;
			}

			Z.Clear() ;
		}
	}

	return rc ;
}

hzEcode	hdsApp::_readFixFile	(hzXmlNode* pN)
{
	//	Category:	Page and Article Config Read Functions
	//
	//	Read in an <xfixfile> tag and place the content as a fixed file in the app's m_Fixed map of URLs to fixed HTML or text content
	//
	//	Arguments:	1)	pN	Current XML node
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readFixFile") ;

	hzAttrset		ai ;		//	Attribute iterator
	hdsFile*		pFix ;		//	Passive file
	uint64_t		now ;		//	Start of zip
	uint64_t		then ;		//	End of zip
	hzEcode			rc = E_OK ;	//	Return code

	if (!pN)
		Fatal("%s: No node supplied\n", *_fn) ;


	if (!pN->NameEQ("xfixfile"))
		{ m_pLog->Out("%s. File %s Line %d Expected <xfixfile> got <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; return E_SYNTAX ; }

	pFix = new hdsFile() ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("url"))		pFix->m_Url = ai.Value() ;
		else if (ai.NameEQ("title"))	pFix->m_Title = ai.Value() ;
		else if (ai.NameEQ("path"))		pFix->m_filepath = ai.Value() ;
		else if (ai.NameEQ("mtype"))	pFix->m_Mimetype = Str2Mimetype(ai.Value()) ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xfixfile> Bad params (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ; }
	}

	//	Read in content - Either by loading file or from the node content
	pFix->m_rawValue = pN->m_fixContent ;

	if (pFix->m_rawValue.Size())
	{
		if (pFix->m_filepath)
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xfixfile> Conflicting content\n", *_fn, pN->Fname(), pN->Line()) ; }
	}
	else
	{
		//	No content from node

		if (!pFix->m_filepath)
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xfixfile> No content\n", *_fn, pN->Fname(), pN->Line()) ; }
		else
		{
			//	Load file
			ifstream	is ;	//	Input stream

			is.open(*pFix->m_filepath) ;
			if (is.fail())
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xfixfile> Content file %s cannot be opened\n", *_fn, pN->Fname(), pN->Line(), *pFix->m_filepath) ; }
			else
				pFix->m_rawValue << is ;
			is.close() ;

			if (!pFix->m_rawValue.Size())
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <xfixfile> Content file %s empty\n", *_fn, pN->Fname(), pN->Line(), *pFix->m_filepath) ; }
		}
	}

	if (rc == E_OK)
	{
		now = RealtimeNano() ;
		Gzip(pFix->m_zipValue, pFix->m_rawValue) ;
		then = RealtimeNano() ;
		//m_Fixed.Insert(pFix->m_Url, pFix) ;
		m_ResourcesPath.Insert(pFix->m_Url, pFix) ;
		m_pLog->Out("%s: Added fixed page %s (%d,%d) %u nS\n", *_fn, *pFix->m_Url, pFix->m_rawValue.Size(), pFix->m_zipValue.Size(), then - now) ;
	}

	return rc ;
}

/*
**	SECTION 7:	Project Level Config Read Functions
*/

hzEcode	hdsApp::_readInclFile	(hzXmlNode* pN)
{
	//	Category:	Project Level Config Read Functions
	//
	//	Parse the content of an included config file, named in an <includeCfg> tag.
	//
	//	Included config files are a matter of developer convenience, allowing application configs to be broken down into more managable sections. The set of tags allowed within an
	//	included config file and the rules that apply to them, are the same as in the root config file - exception for the <includeCfg> tag. Ths is not allowed in an included file
	//	as nesting is not permitted.
	//
	//	As the same set of tags and rules apply, the core of this function is the basically same as that of hdsApp::_readProject() which reads the root config. When a <includeCfg>
	//	tag is encountered in the root config file, the hdsApp::_loadInclFile() function is first called to load the config to be included into a separate hzDocXml instance. Then
	//	hdsApp::_loadInclFile() calls this function.
	//
	//	As the included config is an XML document it must have a root node. In the application root config file this is always <DisseminoWebsite> which sets out core parameters for
	//	the website application. However within an included config file, the root node can be anything as long as it exists and is properly closed. It is otherwise redundant and is
	//	ignored.
	//
	//	Arguments:	1)	filepath	Filepath of HTML include file
	//
	//	Returns:	E_ARGUMENT	If no include file is supplied
	//				E_OPENFAIL	If the include file could not be loaded
	//				E_SYNTAX	If there are errors in the include
	//				E_OK		If the include was read in

	_hzfunc("hdsApp::_readInclFile") ;

	hzXmlNode*	pN1 ;			//	Child node of document
	hzEcode		rc = E_OK ;		//	Return code

	m_cfgErr.Clear() ;

	m_pLog->Out("%s. File %s Line %d: Reading <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ;

	//	Read in XML
	for (pN1 = pN->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if		(pN1->NameEQ("fldspec"))	rc = _readFldspec(pN1) ;
		else if (pN1->NameEQ("rgxtype"))	rc = _readRgxType(pN1) ;
		//else if (pN1->NameEQ("selector"))	rc = _readDataEnum(pN1) ;
		else if (pN1->NameEQ("enum"))		rc = _readDataEnum(pN1) ;
		else if (pN1->NameEQ("class"))		rc = _readClass(pN1) ;
		else if (pN1->NameEQ("repos"))		rc = _readRepos(pN1) ;
		else if (pN1->NameEQ("xscript"))	rc = _readScript(pN1) ;
		else if (pN1->NameEQ("xpage"))		rc = _readPage(pN1) ;
		else if (pN1->NameEQ("xstyle"))		rc = _readCSS(pN1) ;
		else if (pN1->NameEQ("xformHdl"))	rc = _readFormHdl(pN1) ;
		else if (pN1->NameEQ("xformDef"))	rc = _readFormDef(pN1) ;
		else if (pN1->NameEQ("xtreeDcl"))	rc = _readXtreeDcl(pN1, 0) ;
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d tag <%s> unexpected\n", *_fn, pN->Fname(), pN1->Line(), pN1->TxtName()) ; }
	}

	if (rc != E_OK)
	{
		m_pLog->Out("%s. Aborted in config. Err=%s\n", *_fn, Err2Txt(rc)) ;
		if (m_cfgErr.Size())
			m_pLog->Out(m_cfgErr) ;
	}

	return rc ;
}

hzEcode	hdsApp::_loadInclFile	(const hzString& dir, const hzString& fname)
{
	//	Category:	Project Level Config Read Functions
	//
	//	Read in an included file (commonly these hold passive pages)
	//
	//	Apply the same per-page logic as readProject() does to tags except that there will be no project wide info or further includes in an included file.
	//
	//	Arguments:	1)	filepath	Filepath of HTML include file
	//
	//	Returns:	E_ARGUMENT	If no include file is supplied
	//				E_OPENFAIL	If the include file could not be loaded
	//				E_SYNTAX	If there are errors in the include
	//				E_OK		If the include was read in

	_hzfunc("hdsApp::_loadInclFile") ;

	hzDocXml	X ;				//	Included sub-document
	FSTAT		fs ;			//	File status
	hzDirent	de ;			//	Directory entry
	hzXmlNode*	pRoot ;			//	Root node of document
	hzString	fpath ;			//	File name (without path)
	hzEcode		rc = E_OK ;		//	Return code

	if (!fname)
		{ m_pLog->Out("%s. No filename supplied\n", *_fn) ; rc = E_NODATA ; }

	if (!dir)
		fpath = m_Configdir + "/" + fname ;
	else
		fpath = dir + "/" + fname ;

	if (lstat(*fpath, &fs) < 0)
		{ m_pLog->Out("%s. Filepath %s does not exist\n", *_fn, *fpath) ; return E_NODATA ; }

	de.InitStat(m_Configdir, fname, fs) ;
	//de.m_Parity.CalcMD5File(fpath) ;
	m_Configs.Insert(fname, de) ;

	rc = X.Load(fpath) ;
	if (rc != E_OK)
	{
		m_pLog->Out("%s. Could not load included project file %s. Err=%s\n", *_fn, *fpath, Err2Txt(rc)) ;
		m_pLog->Out(X.Error()) ;
		return rc ;
	}

	m_pLog->Out("%s. Loaded included project file [%s]\n", *_fn, *fpath) ;

	pRoot = X.GetRoot() ;
	if (!pRoot)
	{
		m_pLog->Out("%s. Could not obtain included project file root [%s]\n", *_fn, *fpath) ;
		return E_OPENFAIL ;
	}

	m_pLog->Out("%s. Obtained included project file XML root\n", *_fn) ;

	rc = _readInclFile(pRoot) ;

	return rc ;
}

hzEcode	hdsApp::ReloadConfig	(const char* cfgfile)
{
	//	Category:	Project Level Config Read Functions
	//
	//	In the event of a config file being changed, this function allows the config to be reloaded, thereby changing the website content without a restart. The
	//	function is called once for each changed config file.

	_hzfunc("hdsApp::reloadProject") ;

	hzDocXml	doc ;			//	XML document for reading and loading configs
	hzXmlNode*	pRoot ;			//	Root node of document
	hzString	fpath ;			//	Full file path of changed config file
	hzEcode		rc = E_OK ;		//	Return code

	fpath = m_Configdir + "/" + cfgfile ;
	m_pLog->Log(_fn, "NOTE: Reloading Config %s\n", *fpath) ;

	rc = doc.Load(fpath) ;
	if (rc != E_OK)
	{
		m_pLog->Log(_fn, "NOTE: Config %s did not load\n", *fpath) ;
		m_pLog->Out(doc.Error()) ;
		return rc ;
	}

	m_pLog->Log(_fn, "Loaded Config %s\n", *fpath) ;

	pRoot = doc.GetRoot() ;
	if (!pRoot)
	{
		m_pLog->Log(_fn, "NOTE: Config %s Document has no root\n", *fpath) ;
		return E_NODATA ;
	}
			
	m_pLog->Log(_fn, "Parsed Config %s\n", *fpath) ;
 
	rc = _readInclFile(pRoot) ;
	if (rc == E_OK)
		m_pLog->Log(_fn, "NOTE: Accepted Config %s\n", *fpath) ;

	return rc ;
}

hzEcode	hdsApp::CheckProject	(void)
{
	//	Category:	Project Level Config Read Functions
	//
	//	Checks that all pages are refered to. Writes out synopsis of the application.
	//	
	//	Arguments:	None
	//
	//	Returns:	E_OK	In all cases

	_hzfunc("hdsApp::checkProject") ;

	hzList<hdsFormref*>::Iter	fi ;	//	Forms iteration (within page)

	hzChain			report ;		//	Chain for ADP report
	hzChain			Z ;				//	For building robots.txt etc
	//hdsUsertype		ut ;			//	User type
	hdsFldspec		vd ;			//	Variable (in a class)
	hzPair			p ;				//	For adding name/link to navbar
	hdsSubject*		pSubj ;			//	Page subject
	hdsResource*	pRes ;			//	Resource
	hdsPage*		pPage ;			//	Page
	hdsFormdef*		pFormdef ;		//	Form
	hdsFormref*		pFormref ;		//	Form reference
	hdsFormhdl*		pFormHdl ;		//	Form handler
	hdsField*		pFld ;			//	Field
	hdsTree*		pAG ;			//	Article group
	hdsArticle*		pArt ;			//	Article pointer
	hzString		S_key ;			//	Temp string
	hzString		S_obj ;			//	Temp string
	uint32_t		x ;				//	General iterator
	uint32_t		y ;				//	General iterator
	hzEcode			rc = E_OK ;		//	Return code

	m_pLog->Out("APPLICATION REPORT: %s\n", *m_Appname) ;

	m_pLog->Out("User Categories\n") ;
	for (x = 0 ; x < m_UserTypes.Count() ; x++)
	{
		S_key = m_UserTypes.GetKey(x) ;
		m_pLog->Out(" -- %s (%08x)\n", *S_key, m_UserTypes.GetObj(x)) ;
	}
		///{ ut = m_UserTypes.GetObj(x) ; m_pLog->Out(" -- %s\n", *ut.m_Refname) ; }

	m_ADP.Report(report) ;
	m_pLog->Out(report) ;

	/*
	**	Forms report
	*/

	m_pLog->Out("Form Definitions\n") ;
	for (x = 0 ; x < m_FormDefs.Count() ; x++)
	{
		S_key = m_FormDefs.GetKey(x) ;
		pFormdef = m_FormDefs.GetObj(x) ;

		if (pFormdef)
			m_pLog->Out("\t\t -- Form def %s - %s (%p)\n", *S_key, *pFormdef->m_Formname, pFormdef) ;
		else
			m_pLog->Out("\t\t -- Form def %s - NULL\n", *S_key) ;
	}

	m_pLog->Out("Form Handlers\n") ;
	for (x = 0 ; x < m_FormHdls.Count() ; x++)
	{
		S_key = m_FormHdls.GetKey(x) ;
		pFormHdl = m_FormHdls.GetObj(x) ;

		if (pFormHdl)
			m_pLog->Out("\t\t -- Form handler %s applied to form %p %s\n", *S_key, pFormHdl->m_pFormdef, *pFormHdl->m_pFormdef->m_Formname) ;
		else
			m_pLog->Out("\t\t -- Form handler %s not created\n", *S_key) ;
	}

	m_pLog->Out("Form References\n") ;
	for (x = 0 ; x < m_FormRef2Url.Count() ; x++)
	{
		pFormref = m_FormRef2Url.GetKey(x) ;
		S_obj = m_FormRef2Url.GetObj(x) ;

		//pFormdef = pFormref->m_pFormdef ;
		pFormdef = m_FormDefs[pFormref->m_Formname] ;

		if (pFormdef)
			m_pLog->Out("\t\t -- Ref [%p] -> %p %s on %s\n", pFormref, pFormdef, *pFormdef->m_Formname, *S_obj) ;
		else
			m_pLog->Out("\t\t -- Ref [%p] -> NULL_FORM_DEF on %s\n", *S_obj) ;
	}

	m_pLog->Out("Submission URLs to Form Handlers\n") ;
	for (x = 0 ; x < m_FormUrl2Hdl.Count() ; x++)
	{
		S_key = m_FormUrl2Hdl.GetKey(x) ;
		S_obj = m_FormUrl2Hdl.GetObj(x) ;

		pFormHdl = m_FormHdls[S_obj] ;
		pFormref = m_FormUrl2Ref[S_obj] ;

		if (!pFormHdl)
			m_pLog->Out("\t\t -- URL %s -> NULL (ref %p)\n", *S_key, pFormref) ;
		else
			m_pLog->Out("\t\t -- URL %s -> %s (ref %p)\n", *S_key, *pFormHdl->m_Refname, pFormref) ;
	}

	/*
	**	Pages report
	*/

	m_pLog->Out("Pages (by incidence)\n") ;
	for (x = 0 ; x < m_vecPages.Count() ; x++)
	{
		pPage = m_vecPages[x] ;
		m_pLog->Out(" -- access=%08x %s (%s) subj=%s\n", pPage->m_Access, *pPage->m_Url, *pPage->m_Title, *pPage->m_Subj) ;

		if (pPage->m_Subj)
		{
			pSubj = m_setPgSubjects[pPage->m_Subj] ;
			if (!pSubj)
			{
				pSubj = new hdsSubject() ;
				pSubj->subject = pPage->m_Subj ;
				pSubj->first = pPage->m_Url ;

				m_setPgSubjects.Insert(pSubj->subject, pSubj) ;
				m_vecPgSubjects.Add(pSubj) ;
			}

			pSubj->pglist.Add(pPage) ;
		}

		for (fi = pPage->m_xForms ; fi.Valid() ; fi++)
		{
			pFormref = fi.Element() ;
			pFormdef = m_FormDefs[pFormref->m_Formname] ;

			//	if (!pForm->m_valJS)
			//		pForm->WriteValidationJS() ;
			if (pFormdef)
				pPage->m_bScriptFlags |= pFormdef->m_bScriptFlags ;
		}

		pPage->WriteValidationJS() ;
	}

	m_pLog->Out("Pages (by name)\n") ;
	//for (x = 0 ; x < m_PagesName.Count() ; x++)
	for (x = 0 ; x < m_ResourcesName.Count() ; x++)
	{
		pRes = m_ResourcesName.GetObj(x) ;
		pPage = dynamic_cast<hdsPage*>(pRes) ;
		if (!pPage)
			continue ;

		m_pLog->Out(" -- access=%08x %s (%s)\n", pPage->m_Access, *pPage->m_Url, *pPage->m_Title) ;

		for (fi = pPage->m_xForms ; fi.Valid() ; fi++)
		{
			pFormref = fi.Element() ;
			pFormdef = m_FormDefs[pFormref->m_Formname] ;

			if (!pFormdef)
				m_pLog->Out("\t -- form ref %p NULL form def\n", pFormref) ;
			else
			{
				m_pLog->Out("\t -- form ref %p -> %s\n", pFormref, *pFormdef->m_Formname) ;
				for (y = 0 ; y < pFormdef->m_vecFlds.Count() ; y++)
				{
					pFld = pFormdef->m_vecFlds[y] ;
					m_pLog->Out("\t\t -- fld %s\n", *pFld->m_Varname) ;
				}
			}
		}
	}

	m_pLog->Out("Pages (by path)\n") ;
	//for (x = 0 ; x < m_PagesPath.Count() ; x++)
	for (x = 0 ; x < m_ResourcesPath.Count() ; x++)
	{
		//pRes = m_PagesPath.GetObj(x) ;
		pRes = m_ResourcesPath.GetObj(x) ;
		pPage = dynamic_cast<hdsPage*>(pRes) ;
		if (!pPage)
			continue ;

		m_pLog->Out(" -- access=%08x %s (%s)\n", pPage->m_Access, *pPage->m_Url, *pPage->m_Title) ;

		for (fi = pPage->m_xForms ; fi.Valid() ; fi++)
		{
			pFormref = fi.Element() ;
			pFormdef = m_FormDefs[pFormref->m_Formname] ;

			if (!pFormdef)
				m_pLog->Out("\t -- form ref %p NULL form def\n", pFormref) ;
			else
			{
				m_pLog->Out("\t -- form ref %p -> %s\n", pFormref, *pFormdef->m_Formname) ;
				for (y = 0 ; y < pFormdef->m_vecFlds.Count() ; y++)
				{
					pFld = pFormdef->m_vecFlds[y] ;
					m_pLog->Out("\t\t -- fld %s\n", *pFld->m_Varname) ;
				}
			}
		}
	}

	m_pLog->Out("Verifying Links\n") ;
	for (x = 0 ; x < m_Links.Count() ; x++)
	{
		S_obj = m_Links.GetObj(x) ;
		//pRes = m_PagesPath[S_obj] ;
		pRes = m_ResourcesPath[S_obj] ;
		if (!pRes)
			m_pLog->Out("ERROR: No such link as %s\n", *S_obj) ;
		else
			m_pLog->Out("Verified link %s\n", *S_obj) ;
	}

	m_nLoadComplete++ ;

	/*
	**	Build Sitemap
	*/

	if (m_OpFlags & DS_APP_ROBOT)
	{
		Z.Clear() ;
		Z << "User-agent: *\r\nDisallow:\r\n" ;
		Z.Printf("Sitemap: http://%s/sitemap.xml\r\n", *m_Domain) ;
		Z.Printf("Sitemap: http://%s/sitemap.txt\r\n", *m_Domain) ;
		m_Robot = Z ;

		m_rawSitemapXml <<
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
		"<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\" " 
   			"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
   			"xsi:schemaLocation=\"http://www.sitemaps.org/schemas/sitemap/0.9 "
			"http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd\">\r\n" ;

		//for (x = 0 ; x < m_PagesPath.Count() ; x++)
		for (x = 0 ; x < m_ResourcesPath.Count() ; x++)
		{
			//pRes = m_PagesPath.GetObj(x) ;
			pRes = m_ResourcesPath.GetObj(x) ;
			pPage = dynamic_cast<hdsPage*>(pRes) ;
			if (!pPage)
				continue ;

			if (!pPage->m_Subj)
				continue ;

			if (pPage->m_Title == "Login")	continue ;
			if (pPage->m_Title == "Logout")	continue ;

			m_rawSitemapTxt.Printf("http://%s%s\r\n", *m_Domain, *pPage->m_Url) ;
			m_rawSitemapXml.Printf("\t<url><loc>http://%s%s</loc><changefreq>daily</changefreq></url>\r\n", *m_Domain, *pPage->m_Url) ;
		}

		for (y = 0 ; y < m_ArticleGroups.Count() ; y++)
		{
			pAG = m_ArticleGroups.GetObj(y) ;
			if (!pAG)
				continue ;

			for (x = 0 ; x < pAG->Count() ; x++)
			{
				pArt = pAG->Item(x) ;

				m_rawSitemapTxt.Printf("http://%s/%s?%s=%s\r\n", *m_Domain, *pAG->m_Hostpage, *pAG->m_Groupname, pArt->TxtName()) ;
				m_rawSitemapXml.Printf("\t<url><loc>http://%s/%s?%s=%s</loc><changefreq>daily</changefreq></url>\r\n",
					*m_Domain, *pAG->m_Hostpage, *pAG->m_Groupname, pArt->TxtName()) ;
			}
		}

		m_rawSitemapXml << "</urlset>\r\n" ;

		Gzip(m_zipSitemapTxt, m_rawSitemapTxt) ;
		Gzip(m_zipSitemapXml, m_rawSitemapXml) ;
	}

	if (m_OpFlags & DS_APP_GUIDE)
	{
		m_rawSiteguide << "<!DOCTYPE html>\n<head>\n<title>Site Guide</title>\n</head>\n<body>\n" ;
		m_rawSiteguide << "<p>Main Pages</p>\n" ;

		//for (x = 0 ; x < m_PagesPath.Count() ; x++)
		for (x = 0 ; x < m_ResourcesPath.Count() ; x++)
		{
			//pRes = m_PagesPath.GetObj(x) ;
			pRes = m_ResourcesPath.GetObj(x) ;
			pPage = dynamic_cast<hdsPage*>(pRes) ;
			if (!pPage)
				continue ;

			if (!pPage->m_Subj)
				continue ;
			m_rawSiteguide.Printf("<p><a href=\"http://%s%s\">%s</a></p>\n", *m_Domain, *pPage->m_Url, *pPage->m_Title) ;
		}

		for (y = 0 ; y < m_ArticleGroups.Count() ; y++)
		{
			pAG = m_ArticleGroups.GetObj(y) ;
			if (!pAG)
				continue ;

			m_rawSiteguide.Printf("<p>Article from %s</p>\n", *pAG->m_Groupname) ;

			for (x = 0 ; x < pAG->Count() ; x++)
			{
				pArt = pAG->Item(x) ;
				m_rawSiteguide.Printf("<p><a href=\"http://%s%s-%s\">%s</a></p>\n", *m_Domain, *pAG->m_Hostpage, pArt->TxtName(), *pArt->m_Title) ;
			}
		}
		m_rawSiteguide << "</body>\n</html>\n" ;

		Gzip(m_zipSiteguide, m_rawSiteguide) ;
	}

	/*
	**	Add the webmaster admin functions as CIFs
	*/

	/*
	if (rc == E_OK)	rc = AddCIFunc(&_masterMainMenu,	"/masterMainMenu",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterCfgList,		"/masterCfgList",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterCfgEdit,		"/masterCfgEdit",		ACCESS_ADMIN, HTTP_POST) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterResList,		"/masterResList",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterVisList,		"/masterVisList",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterDomain,		"/masterDomain",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterEmaddr,		"/masterEmaddr",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterStrFix,		"/masterStrFix",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterStrGen,		"/masterStrGen",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterBanned,		"/masterBanned",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterMemstat,		"/masterMemstat",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterUSL,			"/masterUSL",			ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterFileList,	"/masterFileList",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterFileEdit,	"/masterFileEdit",		ACCESS_ADMIN, HTTP_POST) ;
	//if (rc == E_OK)	rc = AddCIFunc(&_masterDataModel,	"/masterDataModel",		ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterCfgRestart,	"/masterCfgRestart",	ACCESS_ADMIN, HTTP_GET) ;
	if (rc == E_OK)	rc = AddCIFunc(&_masterLogout,		"/masterLogout",		ACCESS_ADMIN, HTTP_GET) ;
	*/

	m_pLog->Out(" -- END of REPORT --\n") ;
	return rc ;
}

hzEcode	hdsApp::ReadProject	(void)
{
	//	Category:	Dissemino Application Configuration
	//
	//	Load and process a Dissemino Application config XML document.
	//
	//	Note that there are no arguments to supply the path to the root file of the config XML document. This is becuse the application (hdsApp instance) is already instantiated by
	//	the preceeding ReadSphere function, and has been partially initialized with this information.
	//
	//	Arguments:	None
	//
	//	Returns:	E_ARGUMENT	If no project file is supplied
	//				E_OPENFAIL	If the project file could not be loaded
	//				E_SYNTAX	If there are errors in the XML config
	//				E_OK		If the project config was read in

	_hzfunc("hdsApp::ReadProject") ;

	hzDocXml		X ;				//	The config document
	hzChain			Z ;				//	For robot.txt etc
	hzAttrset		ai ;			//	XML node attribute iterator
	hzXmlNode*		pRoot ;			//	Config document root
	hzXmlNode*		pN ;			//	Current node
	hdbClass*		pClass ;		//	The class
	hzPair			p ;				//	Misc name/value pair
	hzString		S ;				//	Intermeadiate string
	hzString		appname ;		//	Application name
	hzString		domain ;		//	Domain name
	hzString		basedir ;		//	Base dir
	hzEcode			rc = E_OK ;		//	Return code

	if (!m_BaseDir || !m_RootFile)
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No project file") ;

	S = m_BaseDir ;
	S += "/config/" ;
	S += m_RootFile ;
	m_pLog->Out("%s. Processing Project File [%s]\n", *_fn, *S) ;

	rc = X.Load(*S) ;
	if (rc != E_OK)
	{
		m_pLog->Out(X.Error()) ;
		return hzerr(_fn, HZ_ERROR, rc, "Could not load project file [%s]", *S) ;
	}
	m_pLog->Out("%s. Loaded project file [%s]\n", *_fn, *S) ;

	pRoot = X.GetRoot() ;
	if (!pRoot)
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not obtain project file root [%s]", *S) ;
	m_pLog->Out("%s. Obtained project's XML root %s\n", *_fn, pRoot->TxtName()) ;

	if (!pRoot->NameEQ("DisseminoWebsite"))
		{ m_pLog->Out("Expected root tag of <project>. Tag <%s> disallowed\n", pRoot->TxtName()) ; return E_SYNTAX ; }
	m_pLog->Out("%s. Obtained project's XML root %s\n", *_fn, pRoot->TxtName()) ;

	//	Get project params
	for (ai = pRoot ; ai.Valid() ; ai.Advance())
	{
		//	m_pLog->Out("%s. File %s Line %d <DisseminoWebsite> tag: param (%s=%s)\n", *_fn, pRoot->Fname(), pRoot->Line(), ai.Name(), ai.Value()) ;

		if		(ai.NameEQ("name"))			appname			= ai.Value() ;
		else if (ai.NameEQ("docroot"))		m_Docroot		= ai.Value() ;
		else if (ai.NameEQ("configs"))		m_Configdir		= ai.Value() ;
		else if (ai.NameEQ("datadir"))		m_Datadir		= ai.Value() ;
		else if (ai.NameEQ("logroot"))		m_Logroot		= ai.Value() ;
		else if (ai.NameEQ("masterpath"))	m_MasterPath	= ai.Value() ;
		else if (ai.NameEQ("masteruser"))	m_MasterUser	= ai.Value() ;
		else if (ai.NameEQ("masterpass"))	m_MasterPass	= ai.Value() ;
		else if (ai.NameEQ("smtpAddr"))		m_SmtpAddr		= ai.Value() ;
		else if (ai.NameEQ("smtpUser"))		m_SmtpUser		= ai.Value() ;
		else if (ai.NameEQ("smtpPass"))		m_SmtpPass		= ai.Value() ;
		else if (ai.NameEQ("usernameFld"))	m_UsernameFld	= ai.Value() ;
		else if (ai.NameEQ("userpassFld"))	m_UserpassFld	= ai.Value() ;
		else if (ai.NameEQ("domain"))		domain			= ai.Value() ;
		else if (ai.NameEQ("loghits"))		m_AllHits		= ai.Value() ;
		else if (ai.NameEQ("language"))		m_DefaultLang	= ai.Value() ;
		else if (ai.NameEQ("login"))		m_OpFlags |= ai.ValEQ("true") ? DS_APP_SUBSCRIBERS : 0 ;
		else if (ai.NameEQ("robot"))		m_OpFlags |= ai.ValEQ("true") ? DS_APP_ROBOT : 0 ;
		else if (ai.NameEQ("siteGuide"))	m_OpFlags |= ai.ValEQ("true") ? DS_APP_GUIDE : 0 ;
		else if (ai.NameEQ("portSTD"))		m_nPortSTD = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("portSSL"))		m_nPortSSL = ai.Value() ? atoi(ai.Value()) : 0 ;
		else
		{
			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d <DisseminoWebsite> tag: Invalid param (%s=%s)\n", *_fn, pRoot->Fname(), pRoot->Line(), ai.Name(), ai.Value()) ;
		}
	}

	//rc = InitApplication(appname, domain, basedir) ;
	if (rc == E_OK)
	{
		if (!m_Appname)		{ rc = E_NOINIT ; m_pLog->Out("No Project/Application name\n") ; }
		if (!m_Domain)		{ rc = E_NOINIT ; m_pLog->Out("No project URL\n") ; }
		if (!m_Docroot)		{ rc = E_NOINIT ; m_pLog->Out("No document root directory\n") ; }
		if (!m_Datadir)		{ rc = E_NOINIT ; m_pLog->Out("No data repos directory\n") ; }
		if (!m_Logroot)		{ rc = E_NOINIT ; m_pLog->Out("No logs root directory\n") ; }
		if (!m_MasterUser)	{ rc = E_NOINIT ; m_pLog->Out("No admin usename\n") ; }
		if (!m_MasterPass)	{ rc = E_NOINIT ; m_pLog->Out("No admin password\n") ; }
		if (!m_DefaultLang)	{ rc = E_NOINIT ; m_pLog->Out("No default language\n") ; }
		//if (!m_nPortSTD)	{ rc = E_NOINIT ; m_pLog->Out("No port specified\n") ; }
	}

	if (rc != E_OK)
		return rc ;

	if (!m_Configdir)
		m_Configdir = m_Docroot ;

	if (m_AllHits)
	{
		//	Set up the hits repository
		pClass = new hdbClass(m_ADP, HDB_CLASS_DESIG_SYS) ;
		pClass->InitStart(m_AllHits) ;
		pClass->InitMember("tdstamp",	datatype_XDATE,		1, 1) ;
		pClass->InitMember("ipaddr",	datatype_IPADDR,	1, 1) ;
		pClass->InitMember("url",		datatype_IPADDR,	1, 1) ;
		pClass->InitDone() ;
		m_ADP.RegisterDataClass(pClass) ;
	}

	//	If no default language, set to en-US
	if (!m_pDfltLang)
	{
		m_DefaultLang = "en-US" ;

		m_pDfltLang = new hdsLang() ;
		m_pDfltLang->m_code = m_DefaultLang ;
		m_Languages.Insert(m_pDfltLang->m_code, m_pDfltLang) ;
	}

	if (m_OpFlags & DS_APP_SUBSCRIBERS)
	{
		//	Set up the subscriber class and repository
		m_ADP.InitSubscribers(m_Datadir) ;

		m_Allusers = (hdbObjCache*) m_ADP.GetObjRepos("subscriber") ;
		if (!m_Allusers)
			{ m_pLog->Out("%s. The subscriber cache is not found in the ADP\n", *_fn) ; return E_NOINIT ; }
	}

	m_Images = m_Docroot + "/img" ;

	//	Read in XML
	for (pN = pRoot->GetFirstChild() ; rc == E_OK && pN ; pN = pN->Sibling())
	{
		//	m_pLog->Out("%s. File %s Line %d Reading <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ;

		if		(pN->NameEQ("fldspec"))			rc = _readFldspec(pN) ;
		else if (pN->NameEQ("rgxtype"))			rc = _readRgxType(pN) ;
		else if (pN->NameEQ("enum"))			rc = _readDataEnum(pN) ;
		else if (pN->NameEQ("user"))			rc = _readUser(pN) ;
		else if (pN->NameEQ("class"))			rc = _readClass(pN) ;
		else if (pN->NameEQ("repos"))			rc = _readRepos(pN) ;
		else if (pN->NameEQ("login"))			rc = _readStdLogin(pN) ;
		else if (pN->NameEQ("logout"))			rc = _readLogout(pN) ;
		else if (pN->NameEQ("xpage"))			rc = _readPage(pN) ;
		else if (pN->NameEQ("xstyle"))			rc = _readCSS(pN) ;
		else if (pN->NameEQ("xinclude"))		rc = _readInclude(pN, 0, 0) ;
		else if (pN->NameEQ("xscript"))			rc = _readScript(pN) ;
		else if (pN->NameEQ("xtreeDcl"))		rc = _readXtreeDcl(pN, 0) ;
		else if (pN->NameEQ("xfixfile"))		rc = _readFixFile(pN) ;
		else if (pN->NameEQ("xfixdir"))			rc = _readFixDir(pN) ;
		else if (pN->NameEQ("xmiscdir"))		rc = _readMiscDir(pN) ;
		else if (pN->NameEQ("siteLanguages"))	rc = _readSiteLangs(pN) ;
		else if (pN->NameEQ("navigation"))		rc = _readNav(pN) ;
		else if (pN->NameEQ("initstate"))		rc = _readInitstate(pN) ;
		else if (pN->NameEQ("xformHdl"))		rc = _readFormHdl(pN) ;
		else if (pN->NameEQ("xformDef"))		rc = _readFormDef(pN) ;

		else if (pN->NameEQ("includeCfg"))
		{
			//	Get directory and filename for included file
			p.Clear() ;
			for (ai = pN ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("dir"))		p.name = ai.Value() ;
				else if (ai.NameEQ("fname"))	p.value = ai.Value() ;
				else
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d Reading <include> tag: Invalid param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
				}
			}

			rc = _loadInclFile(p.name, p.value) ;
		}
		else if (pN->NameEQ("passive"))
		{
			p.Clear() ;
			for (ai = pN ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("files"))	p.name = ai.Value() ;
				else if (ai.NameEQ("htype"))	p.value = ai.Value() ;
				else
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d Reading <passive> tag: Invalid param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
				}
			}

			if (!p.name)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <passive> No files specified\n", *_fn, pN->Fname(), pN->Line()) ; }
			if (!p.value)	{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <passive> No HTML type specified\n", *_fn, pN->Fname(), pN->Line()) ; }

			if (rc == E_OK)
				m_Passives.Add(p) ;
		}
		else if (pN->NameEQ("recaptcha"))
		{
			for (ai = pN ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("public"))	m_KeyPublic = ai.Value() ;
				else if (ai.NameEQ("private"))	m_KeyPrivate = ai.Value() ;
				else
				{
					rc = E_SYNTAX ;
					m_pLog->Out("%s. File %s Line %d Reading <recaptcha> tag: Invalid param (%s=%s)\n", *_fn, pN->Fname(), pN->Line(), ai.Name(), ai.Value()) ;
				}
			}

			m_pLog->Out("%s. File %s Line %d Set Google Recaptcha keys to public %s private %s\n", *_fn, pN->Fname(), pN->Line(), *m_KeyPublic, *m_KeyPrivate) ;
		}
		else
		{
			rc = E_SYNTAX ;
			m_pLog->Out("%s. File %s Line %d tag <%s> unexpected. Only the following are allowed\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ;
			m_pLog->Out("\tfldspec|rgxtype|enum|user|class|repos|login|logout|xpage|xstyle|xinclude|xscript|xtreeDcl|xfixfile|xfixdir|xmiscdir OR\n") ;
			m_pLog->Out("\tsiteLanguages|navigation|initstate|xformHdl|xformDef|includeCfg|passive|recaptcha\n") ;
		}
	}

	if (rc != E_OK)
	{
		m_pLog->Out("%s. Aborted in config. Err=%s\n", *_fn, Err2Txt(rc)) ;
		m_pLog->Out(m_cfgErr) ;
		return rc ;
	}

	/*
	**	Set up the subscriber class and repository
	*/

	//	If no default language, set to en-US
	if (!m_DefaultLang)
		m_DefaultLang = "en-US" ;

	//	Assert the document root
	rc = AssertDir(m_Docroot, 0777) ;
	if (rc != E_OK)
		{ m_pLog->Out("%s. Cannot assert document root %s\n", *_fn, *m_Docroot) ; return E_WRITEFAIL ; }

	//	Assert the data directory
	rc = AssertDir(m_Datadir, 0777) ;
	if (rc != E_OK)
		{ m_pLog->Out("%s. Cannot assert data directory %s\n", *_fn, *m_Datadir) ; return E_WRITEFAIL ; }

	if (chdir(*m_Docroot) < 0)
	{
		m_pLog->Out("Cannot CD to docroot [%s]\n", *m_Docroot) ;
		rc = E_NOINIT ;
	}
	else
		m_pLog->Out("Now operating in docroot [%s]\n", *m_Docroot) ;

	if (rc == E_OK)
		ImportStrings() ;

	m_pLog->Out("%s. Status=%s\n", *_fn, Err2Txt(rc)) ;
	return rc ;
}

/*
**	Config Entry Point
*/

hzEcode	Dissemino::ReadSphere	(const char* fpath)
{
	//	Category:	Dissemino Config
	//
	//	Reading the Dissemino 'sphere' is the starting point for the XML configs for a set of Dissemino applications (websites). The root node must be <DisseminoSphere> and this is
	//	where system wide parameters such as SSL security certificates are declared. Then for each application there will be an <Application> sub-tag which will specify parameters
	//	for the application.
	//
	//	Applications are expected to be in their own directory so where multiple sites are to be hosted, the root file for the sphere (containing the <DisseminoSphere> tag), should
	//	ideally be in none of them. This is not enforced however and it is OK to have the sphere file in the directory of the 'main' website asuming there is one. This would anyway
	//	be the arrangement in the single site case.

	_hzfunc("Dissemino::ReadSphere") ;

	hzDocXml		X ;				//	The config document
	hzXmlNode*		pRoot ;			//	Config document root
	hzXmlNode*		pN ;			//	Current node
	hdsApp*			pApp ;			//	Application pointer
	hzAttrset		ai ;			//	XML node attribute iterator
	hzString		domain ;		//	Application domain name
	hzString		baseDir ;		//	Application base directory
	hzString		rootFile ;		//	Application root filename
	hzString		cookieBase ;	//	Cookie base is set at global level
	uint32_t		nA ;			//	Application iterator
	uint32_t		portSTD ;		//	HTTP port
	uint32_t		portSSL ;		//	HTTPS port
	uint32_t		bOpFlags ;		//	Flags for robot, index pages etc
	hzEcode			rc = E_OK ;		//	Return code

	InitJS_Events() ;

	if (!fpath || !fpath[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No project file") ;
	m_pLog->Out("%s. Processing Project File [%s]\n", *_fn, fpath) ;

	rc = X.Load(fpath) ;
	if (rc != E_OK)
	{
		m_pLog->Out(X.Error()) ;
		return hzerr(_fn, HZ_ERROR, rc, "Could not load project file [%s]", fpath) ;
	}
	m_pLog->Out("%s. Loaded project file [%s]\n", *_fn, fpath) ;

	pRoot = X.GetRoot() ;
	if (!pRoot)
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not obtain project file root [%s]", fpath) ;
	m_pLog->Out("%s. Obtained project's XML root %s\n", *_fn, pRoot->TxtName()) ;

	if (!pRoot->NameEQ("DisseminoSphere"))
		{ m_pLog->Out("%s. Expected root tag of <DisseminoSphere>. Tag <%s> disallowed\n", *_fn, pRoot->TxtName()) ; return E_SYNTAX ; }

	/*
	**	Get system wide parameters	
	*/

	for (ai = pRoot ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("cookieName"))	cookieBase += ai.Value() ;
		else if	(ai.NameEQ("sslPvtKey"))	_hzGlobal_sslPvtKey	= ai.Value() ;
		else if (ai.NameEQ("sslCert"))		_hzGlobal_sslCert	= ai.Value() ;
		else if (ai.NameEQ("sslCertCA"))	_hzGlobal_sslCertCA	= ai.Value() ;
		else if (ai.NameEQ("portSTD"))		m_nPortSTD = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("portSSL"))		m_nPortSSL = ai.Value() ? atoi(ai.Value()) : 0 ;
		else
		{
			m_pLog->Out("%s. File %s Line %d <DisseminoSphere> tag: Expect cookieName|sslPvtKey|sslCert|sslCertCA|portSTD|portSSL only. Invalid param (%s=%s)\n",
				*_fn, pRoot->Fname(), pRoot->Line(), ai.Name(), ai.Value()) ;
			rc = E_SYNTAX ;
		}
	}

	if ((_hzGlobal_sslPvtKey || _hzGlobal_sslCert || _hzGlobal_sslCertCA) && !(_hzGlobal_sslPvtKey && _hzGlobal_sslCert && _hzGlobal_sslCertCA))
		{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <DisseminoSphere> Must set NONE or ALL SSL params\n", *_fn, pRoot->Fname(), pRoot->Line()) ; }

	if (rc != E_OK)
		return rc ;

	if (cookieBase)
		SetCookieName(cookieBase) ;
	else
		m_CookieName = "_hz_dissemino_" ;

	/*
	**	Get initial parameters for each application
	*/

	for (pN = pRoot->GetFirstChild() ; rc == E_OK && pN ; pN = pN->Sibling())
	{
		if (!pN->NameEQ("Application"))
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d. Expected <Application> tag, not <%s>\n", *_fn, pN->Fname(), pN->Line(), pN->TxtName()) ; break ; }

		bOpFlags = portSTD = portSSL = 0 ;

		for (ai = pN ; ai.Valid() ; ai.Advance())
		{
			if		(ai.NameEQ("domain"))		domain = ai.Value() ;
			else if (ai.NameEQ("basedir"))		baseDir = ai.Value() ;
			else if (ai.NameEQ("rootfile"))		rootFile = ai.Value() ;
			else if (ai.NameEQ("robot"))		bOpFlags |= ai.ValEQ("true") ? DS_APP_ROBOT : 0 ;
			else if (ai.NameEQ("index"))		bOpFlags |= ai.ValEQ("true") ? DS_APP_SITEINDEX : 0 ;
			else if (ai.NameEQ("login"))		bOpFlags |= ai.ValEQ("true") ? DS_APP_SUBSCRIBERS : 0 ;
			else if (ai.NameEQ("portSTD"))		portSTD = ai.Value() ? atoi(ai.Value()) : 0 ;
			else if (ai.NameEQ("portSSL"))		portSSL = ai.Value() ? atoi(ai.Value()) : 0 ;
			else
			{
				m_pLog->Out("%s. File %s Line %d <Application> tag: Expect domain|basedir|rootfile|index|portSTD|portSSL only. Invalid param (%s=%s)\n",
					*_fn, pRoot->Fname(), pRoot->Line(), ai.Name(), ai.Value()) ;
				rc = E_SYNTAX ;
			}
		}

		//	Check port settings do not conflict
		if (portSTD)
		{
			if (portSTD > 0xffff)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <Application> HTTP port illegal (%u)\n", *_fn, pRoot->Fname(), pRoot->Line(), portSTD) ; }

			if (m_nPortSTD)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <Application> HTTP port set for sphere. App setting illegal\n", *_fn, pRoot->Fname(), pRoot->Line()) ; }
		}
		else
		{
			if (!m_nPortSTD)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <Application> No HTTP port set in the Sphere or the APP\n", *_fn, pRoot->Fname(), pRoot->Line()) ; }
		}

		if (portSSL)
		{
			if (portSSL > 0xffff)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <Application> HTTP port illegal (%u)\n", *_fn, pRoot->Fname(), pRoot->Line(), portSTD) ; }

			if (m_nPortSSL)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <Application> HTTP port set for sphere. App setting illegal\n", *_fn, pRoot->Fname(), pRoot->Line()) ; }
		}
		else
		{
			if (!m_nPortSSL)
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d <Application> No HTTP port set in the Sphere or the APP\n", *_fn, pRoot->Fname(), pRoot->Line()) ; }
		}

		if (rc != E_OK)
			break ;

	    rc = _hzGlobal_Dissemino->AddApplication(domain, baseDir, rootFile, bOpFlags, portSTD, portSSL) ;
		if (rc == E_OK)
			m_pLog->Out("%s. Added Application %s %s %s\n", *_fn, *domain, *baseDir, *rootFile) ;
		else
			m_pLog->Out("%s. Failed to ADD Application %s %s %s\n", *_fn, *domain, *baseDir, *rootFile) ;
	}

	//	INIT the Apps
	for (nA = 1 ; rc == E_OK && nA <= _hzGlobal_Dissemino->Count() ; nA++)
	{
		pApp = _hzGlobal_Dissemino->GetApplication(nA) ;
		pApp->m_ADP.InitStandard(pApp->m_Appname) ;

		if (pApp->m_OpFlags & DS_APP_SITEINDEX)
			pApp->m_ADP.InitSiteIndex("../data") ;

		pApp->SetStdTypeValidations() ;
		m_pLog->Out("%s. Application %s:- Standard Types created\n", *_fn, *pApp->m_Appname) ;

		rc = pApp->ReadProject() ;
		if (rc == E_OK)
		{
			pApp->SetupMasterMenu() ;
			m_pLog->Out("%s. Application %s:- Data Model Editing initialized\n", *_fn, *pApp->m_Domain) ;

			rc = pApp->CheckProject() ;
		}
	}

	return rc ;
}
