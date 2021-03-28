//
//	File:	hdsResource.cpp
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

//
//	Dissemino Resource Management
//
//	Resources such as pages, articles and include blocks have inherently unique and human readable identifiers such as pathname and title but internal operation
//	is enhanced by allocation of a unique numeric id. This is best done internally as it is a nuicence to have to allocate ids to these entities in the configs.
//	Instead, the human operator only needs to set the version number of each resource. This will be ver="1" in all cases to begin with but will be increased by
//	1 if editied.
//
//	It is important to understand that Dissemino makes a distinction between resorces that are explicitly defined in the configs and those that are derived from
//	other sources such as uploads from users. While the latter may amount to whole pages or articles to Dissemino they are meaningless binaries to be stored in
//	the database and given addresses accordingly. The regime herein described applies only to config defined resources.
//
//	To this end, Dissemino manages a series of files in the document root as follows:-
//
//	1) website.res.xml: This lists all config defined resources and adds the internally assigned id, the date and time stamp and the version to the pathname and
//	title. The 
//
//	2) website.$$.txt:	Where $$ in this case is the default language code. This file will contain a series of strings extracted from the currently applicable
//						resources.
//
//	If ANY of the above files do not exist in the document root, Dissemino will take the configs as autorative and create a complete set of new files missing files from the configs
//	a veriety of reasons, it is these have practical limitations. In
//	particular, change over time

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>

#include "hzErrcode.h"
#include "hzTokens.h"
#include "hzTextproc.h"
#include "hzDissemino.h"

using namespace std ;

/*
**	Variables
*/

static	hdsTree		s_treeDataModel ;			//	Tree for object model display

extern	hzString	_dsmScript_tog ;			//	Navtree toggle script
extern	hzString	_dsmScript_loadArticle ;	//	Navtree load article script
extern	hzString	_dsmScript_navtree ;		//	Navtree script

/*
**	Functions
*/

hzEcode	hdsApp::IndexPages	(void)
{
	//	Go through all pages found in the config files and index them
	//
	//	Arguments:	None
	//
	//	Returns:	E_FORMAT	If any page could not be tokenized
	//				E_OK		If the pages were indexed

	_hzfunc("hdsApp::IndexPages") ;

	hzVect<hzToken>	toks ;			//	Token list

	hzChain			pageVal ;		//	Extract content from tags into chain, then tokenize to get words to index
	hzToken			T ;				//	Tokens
	hdsResource*	pRes ;			//	Resource under consideration
	hdsPage*		pPage ;			//	Current page
	uint32_t		nD ;			//	Document number
	uint32_t		nCount ;		//	Loop counter
	uint32_t		nDone ;			//	Count of actual inserts
	hzEcode			rc = E_OK ;		//	Return code

	/*
	**	Allocate working buffers and load HTML page
	*/

	//for (nD = 0 ; nD < m_PagesName.Count() ; nD++)
	for (nD = 0 ; nD < m_ResourcesName.Count() ; nD++)
	{
		//pRes = m_PagesName.GetObj(nD) ;
		pRes = m_ResourcesName.GetObj(nD) ;
		pPage = dynamic_cast<hdsPage*>(pRes) ;
		if (!pPage)
			continue ;

		if (!pPage->m_Bodytext.Size())
			continue ;

		//	Pass thru page tags looking for indexable content. This will include the page title, description metatags and the content of paragraphs.
		//	Note tha paragraph content must be assumed to be complex and so is comprised of the pretext of the subtags and only lastly the content.

		pageVal.Clear() ;
		pageVal << pPage->m_Title ;
		pageVal.AddByte(CHAR_NL) ;
		pageVal << pPage->m_Desc ;
		pageVal.AddByte(CHAR_NL) ;
		pageVal << pPage->m_Bodytext ;

		rc = TokenizeChain(toks, pageVal, TOK_MO_WHITE) ;
		if (rc != E_OK)
		{
			m_pLog->Out("Abandoning indexation of page %s (%s)\n", *pPage->m_Url, *pPage->m_Title) ;
			break ;
		}

		for (nDone = nCount = 0 ; rc == E_OK && nCount < toks.Count() ; nCount++)
		{
			T = toks[nCount] ;
			if (!T.Value())
				continue ;

			nDone++ ;
			rc = m_PageIndex.Insert(T.Value(), nD) ;
		}

		m_pLog->Out("Indexing page %s (%s), %d of %d tokens\n", *pPage->m_Url, *pPage->m_Title, nDone, toks.Count()) ;
	}

	return rc ;
}

/*
**	Language Support, Text String Import/Export
*/

bool	_strhasalphas	(const char* str)
{
	//	Does string contain alpha chars?
	//
	//	Arguments:	1)	str		Test string
	//
	//	Returns:	True	If the supplied cstr contains alpha chars
	//				False	If it does not

	const char*	i ;				//	Input string iterator
	uint32_t	uVal ;			//	Entity value
	uint32_t	nLen ;			//	Entity length
	uint32_t	nAlphas = 0 ;	//	Count of alpha characters

	if (!str || !str[0])
		return false ;

	for (i = str ; *i ; i++)
	{
		if (IsEntity(uVal, nLen, i))
			{ i += (nLen - 1) ; continue ; }

		if (IsAlpha(*i))
			nAlphas++ ;
	}

	return nAlphas > 1 ? true : false ;
}
			
void	hdsApp::_exportStr	(hzChain& Z, hdsVE* pVE)
{
	//  Now for each subtag, output first the pretext (part of this tag's content) and then call recursively on the subtag
	//
	//  Arguments:	1)	Z	Export output chain
	//  			2)	pVE	Visual entity
	//
	//  Returns:	None

	hdsVE*		pX ;		//	Visual entity pointer
	uint32_t	nLen ;		//	Lenght of visual entity strings
	hzRecep32	r32 ;		//	For USL text value

	if (!pVE)
		Fatal("hdsApp::_exportStr: No visible entity poiter\n") ;

	if (pVE->m_Tag == "pre")	return ;
	if (pVE->m_Tag == "script")	return ;

	for (pX = pVE->Children() ; pX ; pX = pX->Sibling())
	{
		if (pX->m_strPretext)
		{
			//	Test string for alpha chars first, if none, don't export the string
			if (_strhasalphas(*pX->m_strPretext))
			{
				//Z.Printf("%s) %s\n", *pX->m_usiPretext, *pX->m_strPretext) ;
				Z.Printf("%s) %s\n", pX->m_usiPretext.Txt(r32), *pX->m_strPretext) ;

				nLen = pX->m_strPretext.Length() ;
				if (pX->m_strPretext[nLen-1] != CHAR_NL)
					Z.AddByte(CHAR_NL) ;
			}
		}

		_exportStr(Z, pX) ;
	}

	if (pVE->m_strContent)
	{
		if (_strhasalphas(*pVE->m_strContent))
		{
			Z.Printf("%s) %s\n", pVE->m_usiContent.Txt(r32), *pVE->m_strContent) ;

			nLen = pVE->m_strContent.Length() ;
			if (pVE->m_strContent[nLen-1] != CHAR_NL)
				Z.AddByte(CHAR_NL) ;
		}
	}
}

hzEcode	hdsApp::ExportStrings	(void)
{
	//	Export all strings for external language translation
	//
	//	Arguments:	None
	//
	//	Returns:	E_OPENFAIL	If the string export output stream cannot be opened
	//				E_OK		If the strings are exported

	_hzfunc("hdsApp::ExportStrings") ;

	hzList<hdsVE*>::Iter	ih ;	//	Html entity iterator

	hdsArticle*		pArt ;			//	Generic article pointer
	hdsArticleStd*	pArtStd ;		//	Standar article pointer
	ofstream		os ;			//	Output stream
	hzChain			Z ;				//	Output chain
	hdsTree*		pAG ;			//	Article group
	hdsPage*		pPage ;			//	Page pointer
	hdsBlock*		pBlok ;			//	Include Block
	hdsSubject*		pSubj ;			//	Subject (navbar heading)
	hdsVE*			pVE ;			//	Visible entity
	hzString		filepath ;		//	File path
	hzString		S ;				//	Temp string
	uint32_t		nG ;			//	Article group iterator
	uint32_t		n ;				//	Page iterator
	uint32_t		nV ;			//	Visual entity iterator

	//	Do strings by page
	Z.Printf("%s/website.%s.txt", *m_Docroot, *m_DefaultLang) ;
	filepath = Z ;
	Z.Clear() ;

	os.open(*filepath) ;
	if (os.fail())
	{
		m_pLog->Out("%s. Cannot open file [%s]\n", *filepath) ;
		return E_OPENFAIL ;
	}

	for (n = 0 ; n < m_vecPgSubjects.Count() ; n++)
	{
		pSubj = m_vecPgSubjects[n] ;
		Z.Printf("s%d) %s\n\n", n, *pSubj->subject) ;
	}

	for (n = 0 ; n < m_Includes.Count() ; n++)
	{
		pBlok = m_Includes.GetObj(n) ;

		for (pVE = pBlok->Children() ; pVE ; pVE = pVE->Sibling())
			_exportStr(Z, pVE) ;
	}

	//for (n = 0 ; n < m_PagesUid.Count() ; n++)
	for (n = 0 ; n < m_vecPages.Count() ; n++)
	{
		//pPage = m_PagesUid.GetObj(n) ;
		pPage = m_vecPages[n] ;

		if (!(pPage->m_flagVE & VE_LANG))
			{ m_pLog->Out("Skipping page %s\n", *pPage->m_Url) ; continue ; }
		m_pLog->Out("%s. Doing Page %s\n", *_fn, *pPage->m_Url) ;

		//	for (ih = pPage->m_VEs ; ih.Valid() ; ih++)
		//	{
		//		pVE = ih.Element() ;
		//		_exportStr(Z, pVE) ;
		//	}
		for (nV = 0 ; nV < pPage->m_VEs.Count() ; nV++)
		{
			pVE = pPage->m_VEs[nV] ;
			_exportStr(Z, pVE) ;
		}
	}

	os << Z ;
	os.close() ;
	os.clear() ;
	Z.Clear() ;
	m_pLog->Out("%s. File %s Total %d pages\n", *_fn, *filepath, n) ;

	//	Do article groups
	for (nG = 0 ; nG < m_ArticleGroups.Count() ; nG++)
	{
		pAG = m_ArticleGroups.GetObj(nG) ;
		//m_pLog->Out("Doing article group %s (total %d articles)\n", *pAG->m_Groupname, pAG->m_Articles.Count()) ;
		m_pLog->Out("Doing article group %s (total %d articles)\n", *pAG->m_Groupname, pAG->Count()) ;

		Z.Printf("%s/%s.%s.txt", *m_Docroot, *pAG->m_Groupname, *m_DefaultLang) ;
		filepath = Z ;
		Z.Clear() ;

		os.open(*filepath) ;
		if (os.fail())
		{
			m_pLog->Out("%s. Cannot open file [%s]\n", *filepath) ;
			return E_OPENFAIL ;
		}

		for (n = 0 ; n < pAG->Count() ; n++)
		{
			pArt = (hdsArticle*) pAG->Item(n) ;

			pArtStd = dynamic_cast<hdsArticleStd*>(pArt) ;
			if (pArtStd)
			{
				if (!(pArtStd->m_flagVE & VE_LANG))
					{ m_pLog->Out("Skipping article %s\n", pArtStd->TxtName()) ; continue ; }

				//	for (pVE = pArt->Root() ; pVE ; pVE = pVE->Sibling())
				//		_exportStr(Z, pVE) ;
				for (nV = 0 ; nV < pArtStd->m_VEs.Count() ; nV++)
				{
					pVE = pArtStd->m_VEs[nV] ;
					_exportStr(Z, pVE) ;
				}
			}
		}

		os << Z ;
		os.close() ;
		os.clear() ;
		Z.Clear() ;
		m_pLog->Out("%s. File %s Total %d articles (size now %d)\n", *_fn, *filepath, n, Z.Size()) ;
	}

	return E_OK ;
}

void	hdsApp::ImportStrings	(void)
{
	//	For each supported language other than the default, import translated text. The export produces files of the form 'projectname.lang' for pages declared
	//	with an <xpage> and 'projectname.articlegroup.lang' for all articles groups. The import process expects these to have been manually converted offline to
	//	files of the form 'projectname.lc' and 'projectname.articlegroup.lc' where 'lc' is the non-default language code. The supported languages are indicated
	//	in the <siteLanguages> tag.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hdsApp::ImportStrings") ;

	hzList<hdsVE*>::Iter	ih ;	//	Html entity iterator

	ifstream		is ;			//	Output stream
	chIter			zi ;			//	File iterator
	hzChain			Z ;				//	Output chain
	hzChain			Para ;			//	Construction chain
	hdsLang*		pLang ;			//	Langauge
	hdsTree*		pAG ;			//	Article group
	hzString		filepath ;		//	File path
	hzString		tmpStr ;		//	Temp string for value
	hzString		S ;				//	Temp string for value
	hdsUSL			U ;				//	Temp string for uid
	uint32_t		nL ;			//	Language iterator
	uint32_t		n ;				//	Article group iterator

	for (nL = 0 ; nL < m_Languages.Count() ; nL++)
	{
		pLang = m_Languages.GetObj(nL) ;
		if (pLang->m_name == m_DefaultLang)
			continue ;

		//	Do strings by page
		Z.Printf("%s/website.%s.txt", *m_Docroot, *pLang->m_code) ;
		filepath = Z ;
		Z.Clear() ;
		is.open(*filepath) ;
		if (is.fail())
			{ m_pLog->Out("%s. Cannot open file [%s]\n", *_fn, *filepath) ; continue ; }
		Z << is ;
		is.close() ;
		is.clear() ;
		m_pLog->Out("%s. Opened language file [%s] (total %d bytes)\n", *_fn, *filepath, Z.Size()) ;

		//	Scan the file content and load strings into the string table for the language. Pages begin with a http:// type URL and are identified from this. The
		//	strings for the page will begin with the form 'page_uid.string_id)' and end with a double newline.
		//U.Clear() ;
		U.Clear() ;
		S = (char*) 0 ;
		for (zi = Z ; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
		for (; !zi.eof() ;)
		{
			//	First part is the id string followed by the ')'
			for (; !zi.eof() && *zi > CHAR_SPACE ; zi++)
			{
				if (*zi == CHAR_PARCLOSE)
					break ;
				Para.AddByte(*zi) ;
			}
			for (zi++ ; !zi.eof() && *zi < CHAR_SPACE ; zi++) ;
			//U = Para ;
			tmpStr = Para ;
			U.SetByText(*tmpStr) ;
			//stRef = U ;
			Para.Clear() ;

			//	Second part is the content terminated by double newline
			for (; !zi.eof() ; zi++)
			{
				if (zi == "\n\n")
				{
					S = Para ;
					Para.Clear() ;
					//if (!m_pDfltLang->m_Strings.Exists(stRef))
					if (!m_pDfltLang->m_LangStrings.Exists(U))
					{
						//m_pLog->Out("%s. WARNING No match on string veId (%s=[%s])\n", *_fn, *U, *S) ;
					}
					else
					{
						//pLang->m_Strings.Insert(stRef, S) ;
						pLang->m_LangStrings.Insert(U, S) ;
						//	m_pLog->Out("%s. Inserted veId (%s=[%s])\n", *_fn, *U, *S) ;
					}
					U.Clear() ;
					S = (char*) 0 ;
					for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
					break ;
				}
				else
					Para.AddByte(*zi) ;
			}
		}

		Z.Clear() ;
		m_pLog->Out("%s. File %s Total %d strings\n", *_fn, *filepath, pLang->m_LangStrings.Count()) ;

		//	Do article groups
		for (n = 0 ; n < m_ArticleGroups.Count() ; n++)
		{
			pAG = m_ArticleGroups.GetObj(n) ;
			//m_pLog->Out("Doing article group %s (total %d articles)\n", *pAG->m_Groupname, pAG->m_Articles.Count()) ;
			m_pLog->Out("Doing article group %s (total %d articles)\n", *pAG->m_Groupname, pAG->Count()) ;

			Z.Printf("%s/%s.%s.txt", *m_Docroot, *pAG->m_Groupname, *pLang->m_code) ;
			filepath = Z ;
			Z.Clear() ;
			is.open(*filepath) ;
			if (is.fail())
				{ m_pLog->Out("%s. Cannot open file [%s]\n", *_fn, *filepath) ; continue ; }
			Z << is ;
			is.close() ;
			is.clear() ;
			m_pLog->Out("%s. Opened language file [%s] (total %d bytes)\n", *_fn, *filepath, Z.Size()) ;

			//	Scan the file content and load strings into the string table for the language. Articles begin with a http:// type URL and are identified from this. The
			//	strings for the page will begin with the form 'page_uid.string_id)' and end with a double newline.
			U.Clear() ;
			S = (char*) 0 ;
			for (zi = Z ; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
			for (; !zi.eof() ;)
			{
				//	First part is the id string followed by the ')'
				for (; !zi.eof() && *zi > CHAR_SPACE ; zi++)
				{
					if (*zi == CHAR_PARCLOSE)
						break ;
					Para.AddByte(*zi) ;
				}
				for (zi++ ; !zi.eof() && *zi < CHAR_SPACE ; zi++) ;
				//U = Para ;
				tmpStr = Para ;
				U.SetByText(*tmpStr) ;
				//stRef = U ;
				Para.Clear() ;

				//	Second part is the content terminated by double newline
				for (; !zi.eof() ; zi++)
				{
					if (zi == "\n\n")
					{
						S = Para ;
						Para.Clear() ;
						//if (!m_pDfltLang->m_Strings.Exists(stRef))
						if (!m_pDfltLang->m_LangStrings.Exists(U))
						{
							//	m_pLog->Out("%s. WARNING No match on string veId (%s=[%s])\n", *_fn, *U, *S) ;
						}
						else
						{
							//pLang->m_Strings.Insert(stRef, S) ;
							pLang->m_LangStrings.Insert(U, S) ;
							//	m_pLog->Out("%s. Inserted veId (%s=[%s])\n", *_fn, *U, *S) ;
						}
						U.Clear() ;
						S = (char*) 0 ;
						for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
						break ;
					}
					else
						Para.AddByte(*zi) ;
				}
			}

			Z.Clear() ;
			m_pLog->Out("%s. File %s Total %d strings\n", *_fn, *filepath, pLang->m_LangStrings.Count()) ;
		}
	}
}

hzEcode	hdsApp::CreateDefaultForm	(const hzString& cname)
{
	//	Create a form definition and form handler for the named data class.

	_hzfunc("hdsApp::CreateDefaultForm") ;

	const hdbClass*		pClass ;	//	Data class pointer
	const hdbMember*	pMbr ;		//	Data class member

	hdsFormdef*		pFormdef ;		//	Form definition
	hdsFormhdl*		pFormhdl ;		//	Form handler
	hdsFormref*		pFormref ;		//	Form reference
	hdsField*		pFld ;			//	Field being processed
	hzString		S ;				//	Temp string
	uint32_t		mbrNo ;			//	Member number
	hzEcode			rc = E_OK ;		//	Return code

	//	Establish the class exists and does not already have a default form and from-hamdler (it can have other forms and form-handlers in the configs).

	if (cname == "subscriber")
	{
		//	Special case. Both success and failure lead to the re-displaying the current page
		m_pLog->Log(_fn, "The subscriber class is a special case. No default form is required\n") ;
		return E_OK ;
	}

	pClass = m_ADP.GetPureClass(cname) ;
	if (!pClass)
		{ m_pLog->Out("No such class as %s\n", *cname) ; return E_NOTFOUND ; }
	m_pLog->Out("%s. Doing class %s with %d members\n", *_fn, *cname, pClass->MbrCount()) ;

	//	Create the form definition and add the fields
	pFormdef = new hdsFormdef() ;
	pFormdef->m_Formname = "dflt_form_" + cname ;
	pFormdef->m_DfltAct = "/dflt_fhdl_" + cname ;
	pFormdef->m_pClass = pClass ;

	m_FormDefs.Insert(pFormdef->m_Formname, pFormdef) ;


	for (mbrNo = 0 ; mbrNo < pClass->MbrCount() ; mbrNo++)
	{
		pMbr = pClass->GetMember(mbrNo) ;

		//	Create feild for each atomic member
		pFld = new hdsField(this) ;
		pFld->InitVE(this) ;
		pFld->m_strPretext = pMbr->StrName() ;
		pFld->m_Line = mbrNo ;
		pFld->m_Indent = 0 ;

        //	pFld->m_Fldspec.m_pType = m_ADP.GetDatatype(dtS) ;
        //	if (!pFld->m_Fldspec.m_pType)
           //	 { m_pLog->Out("%s. File %s Line %d: xfield %s: Illegal fld type %s\n", *_fn, *cfg, pN->Line(), *vnam, *dtS) ; goto failed ; }

        //	pFld->m_Fldspec.m_Refname = pFormdef->m_Formname + "->" + vnam ;
        //	pFld->m_Varname = vnam ;
        //	pFld->m_flagVE |= flags ;
        //	pFld->m_Fldspec.nRows = nRows ;
        //	pFld->m_Fldspec.nCols = nCols ;
        //	pFld->m_Fldspec.nSize = nSize ;

	    //	if (!pFormdef->m_mapFlds.Exists(pFld->m_Varname))
        //		pFormdef->m_mapFlds.Insert(pFld->m_Varname, pFld) ;
    	pFormdef->m_vecFlds.Add(pFld) ;
	}

	pFormhdl = new hdsFormhdl() ;
	pFormhdl->m_Refname = "dflt_fhdl_" + cname ;
	pFormhdl->m_pFormdef = pFormdef ;
	pFormhdl->m_flgFH |= VE_COOKIES ;

	m_FormHdls.Insert(pFormhdl->m_Refname, pFormhdl) ;

	m_FormUrl2Hdl.Insert(pFormdef->m_DfltAct, pFormhdl->m_Refname) ;

	pFormref = new hdsFormref(this) ;
	pFormref->m_Formname = pFormdef->m_Formname ;
	//pFormref->m_flagVE |= VE_LOGINFORM ;
	m_FormUrl2Ref.Insert(pFormdef->m_DfltAct, pFormref) ;

	m_FormRef2Url.Insert(pFormref, pFormdef->m_DfltAct) ;



	return rc ;
}

hzEcode	hdsApp::ExportDefaultForm	(const hzString& cname)
{
	//	Export data classes found in the configs as XML fragments that define default forms and form-handlers. This facility is provided as a development aid in
	//	order to save time. The output is to file name "classforms.def" in the current directory which should always be the development config directory for the
	//	website. The facility is invoked by calling Dissemino with "-forms" supplied as an argument. Dissemino will load the configs, write the classforms.def
	//	file and then exit.
	//
	//	There will be one form and one form-handler generated for each class found in the configs, including those for which form(s) currently exists. Forms in
	//	HTML cannot be nested so in cases where class members are composite (have a data type which is another class), the form produced will contain an extra
	//	button for each composite member. The class which a member has as its data type will have to have been declare beforehand (otherwise the configs cannot
	//	be read in), and this will have a form and form handler. The ...

	_hzfunc(__func__) ;

	const hdsFldspec*	pSpec ;		//	Field spec
	const hdbMember*	pMem ;		//	Data class member pointer

	ofstream		os ;			//	Output file
	hzChain			Z ;				//	Output chain
	const hdbClass*	pClass ;		//	Data class pointer
	hdsBlock*		pIncl ;			//	Pointer to default include block
	hzString		dfltInclude ;	//	Use a <xinclude> block in pages?
	const char*		mname ;			//	Member name
	uint32_t		nM ;			//	Data class member iterator
	bool			bUser ;			//	True is class is a user class
	char			fname[80] ;		//	Filename
	char			pebuf[4] ;		//	Percent entity form
	hzEcode			rc = E_OK ;		//	Return code

	m_pLog->Out("%s. Have %d classes and %d repositories\n", *_fn, m_ADP.CountDataClass(), m_ADP.CountObjRepos()) ;

	pebuf[0] = CHAR_PERCENT ;
	pebuf[1] = 's' ;
	pebuf[2] = ':' ;
	pebuf[3] = 0 ;

	if (m_Includes.Count())
	{
		pIncl = m_Includes.GetObj(0) ;
		dfltInclude = pIncl->m_Refname ;
	}

	pClass = m_ADP.GetPureClass(cname) ;
	if (!pClass)
		{ m_pLog->Out("No such class as %s\n", *cname) ; return E_NOTFOUND ; }
	m_pLog->Out("%s. Doing class %s with %d members\n", *_fn, *cname, pClass->MbrCount()) ;

	bUser = false ;
	if (m_UserTypes.Exists(cname))
		bUser = true ;

		//	Start the XML output
		Z << "<disseminoInclude>\n" ;

		/*
		**	Write out a basic list page for the class
		*/

		Z.Printf("<xpage path=\"/list_%s\" subject=\"%s\" title=\"List %s\" access=\"any\" bgcolor=\"eeeeee\" ops=\"noindex\">\n",
			*cname, *pClass->m_Category, *cname) ;
		Z << "\t<desc>None</desc>\n" ;
		if (dfltInclude)
			Z.Printf("\t<xblock name=\"%s\"/>\n", *dfltInclude) ;
		Z.AddByte(CHAR_NL) ;

		Z.Printf("\t<xtable repos=\"%s\" criteria=\"all\" rows=\"10000\" width=\"90\" height=\"500\" css=\"main\">\n", *cname) ;
		Z <<
		"\t\t<ifnone><p>Sorry no records ...</p></ifnone>\n"
		"\t\t<header>Table header ...</header>\n"
		"\t\t<footer>Table footer ...</footer>\n" ;

		for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
		{
			pMem = pClass->GetMember(nM) ;
			mname = pMem->TxtName() ;

			Z.Printf("\t\t<xcol member=\"%s\" title=\"%s\"/>\n", mname, mname) ;
		}

		Z << "\t</xtable>\n" ;
		Z << "</xpage>\n" ;

		/*
		**	Write out a basic host page for the form for the class
		*/

		Z.Printf("<xpage path=\"/add_%s\" subject=\"%s\" title=\"Add %s\" access=\"any\" bgcolor=\"eeeeee\" ops=\"noindex\">\n",
			*cname, *pClass->m_Category, *cname) ;
		Z << "\t<desc>None</desc>\n" ;
		if (dfltInclude)
			Z.Printf("\t<xblock name=\"%s\"/>\n", *dfltInclude) ;
		Z.AddByte(CHAR_NL) ;

		Z <<
		"<style>\n"
		".tooltip { position: relative; display: inline-block; border-bottom: 1px dotted black; }\n"
		".tooltip .tooltiptext { visibility: hidden; width: 120px; background-color: black; color: #fff; text-align: center; padding: 5px 0; border-radius: 6px; position: absolute; z-index: 1; }\n"
		".tooltip:hover .tooltiptext { visibility: visible; }\n"
		"</style>\n\n" ;

		Z <<
		"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
		"\t\t<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"\t\t<tr><td class=\"title\" align=\"center\">Title: Add a " << cname << "</td></tr>\n"
		"\t\t<tr>\n"
		"\t\t<td valign=\"top\" align=\"center\" class=\"main\">\n"
		"\t\t<p>\n"
		"\t\tUse this form to create an instance of " << cname << "\n"
		"\t\t</p>\n"
		"\t\t</td>\n"
		"\t</tr>\n"
		"\t<xdiv user=\"public\">\n"
		"\t<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"\t<tr>\n"
		"\t\t<td height=\"20\" align=\"center\" class=\"main\">\n"
		"\t\tPlease be aware that by submitting this form, you are consenting to the use of cookies. Please read our cookie policy to <a href=\"/cookies\">find out more</a>\n"
		"\t\t</td>\n"
		"\t</tr>\n"
		"\t</xdiv>\n"
		"\t<tr><td height=\"40\">&nbsp;</td></tr>\n"
		"\t</table>\n\n" ;

		//	Now write out the form itself
		if (bUser)
			Z.Printf("\t<xform name=\"form_reg_%s\" action=\"fhdl_phase1_%s\" class=\"%s\">\n", *cname, *cname, *cname) ;
		else
			Z.Printf("\t<xform name=\"form_add_%s\" action=\"fhdl_add_%s\" class=\"%s\">\n", *cname, *cname, *cname) ;
		Z << "\t<xhide name=\"lastpage\" value=\"%x:referer;\"/>\n\n" ;

		Z << "\t<table width=\"56%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

		for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
		{
			pMem = pClass->GetMember(nM) ;
			mname = pMem->TxtName() ;

			pSpec = pMem->GetSpec() ;

			if (!pSpec)
				{ m_pLog->Out("%s. Warning No Field specification for member %s\n", *_fn, mname) ; continue ; }

			if (pSpec->m_Desc)
				Z.Printf("\t\t<tr height=\"18\"><td class=\"fld\"><div class=\"tooltip\">%s<span class=\"tooltiptext\">%s</span></div>:</td>\t<td><xfield member=\"%s\"",
					*pSpec->m_Title, *pSpec->m_Desc, mname) ;
			else
				Z.Printf("\t\t<tr height=\"18\"><td class=\"fld\">%s:</td>\t<td><xfield member=\"%s\"", *pSpec->m_Title, mname) ;

			Z << "\tdata=\"" ; Z << pebuf ; Z << mname << "\"" ;

			Z << "\tflags=\"req\"/></td></tr>\n" ;
		}
		Z << "\t</table>\n" ;

		//	Write out a rudimentry page footer
		Z <<
		"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n"
		"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Abort\"/> <xbutton show=\"Submit\"/></td></tr>\n"
		"\t\t<tr height=\"50\" bgcolor=\"#CCCCCC\"><td class=\"vb10blue\"><center>Site Name</center></td></tr>\n"
		"\t</table>\n" ;

		Z << "\t</xform>\n" ;
		Z << "</xpage>\n\n" ;

		//	Write form-handler for class

		if (bUser)
		{
			//	Write out user registration form-handler. This will be the first form-handler (with name of the form fhdl_classname), with a sendemail step to
			//	conduct email verification and reference to a second form-handler. This second form handler will also be produced and will commit a new user.

			Z.Printf("<formhdl name=\"fhdl_phase1_%s\" form=\"form_reg_%s\" ops=\"cookie\">\n", *cname, *cname) ;
			Z << "\t<procdata>\n" ;

			//	Do the <setvar tags for all the members
			Z << "\t\t<setvar name=\"iniCode\" type=\"uint32_t\" value=\"%x:usecs;\"/>\n" ;
			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("\t\t<setvar name=\"%s\" type=\"%s\" value=\"%ce:%s;\"/>\n", mname, pSpec->m_pType->TxtTypename() , CHAR_PERCENT, mname) ;
			}

			//	Do the send email code thing

			Z.Printf("\t\t<sendemail from=\"%s\" to=\"%ce:email;\" smtpuser=\"%s\" smtppass=\"%s\" subject=\"Membership Application\">\n",
				*m_SmtpAddr, CHAR_PERCENT, *m_SmtpUser, *m_SmtpPass) ;

			//Dear %e:salute; %e:fname; %e:lname; of %e:orgname;

			Z.Printf("Thank you for registering with %s. Your initial login code is %cs:iniCode;i\n\n", *m_Domain, CHAR_PERCENT) ;
			Z << "The code is good for 1 hour. Enter this code instead of your selected password in the login screen.\n\n" ;

			Z << "This email has been generated by a submission to the Registration form at " ;
			Z << m_Domain ;
			Z << ". If you have recived this in error, please ignore\n" ;
			Z << "\t\t</sendemail>\n" ;
			Z << "\t</procdata>\n" ;

			Z << "\t<error goto=\"/register\"/>\n\n" ;

			Z.Printf("\t<response name=\"form_reg_%s\" bgcolor=\"eeeeee\">\n", *cname) ;
			Z << "\t\t<xblock name=\"gdInclude\"/>\n" ;
			Z.Printf("<xform name=\"formCkCorp1\" action=\"hdlCkCorp\">\n") ;

			Z <<
			"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t<tr><td align=center class=title>Email Verification</td></tr>\n"
			"\t\t<tr height=\"250\">\n"
			"\t\t<td class=\"main\">\n"
			"\t\tThank you %s:fname; %s:lname; for your registration on belhalf of %s:orgname; to the gdpr360 XLS document processing service.\n"
			"\t\t</td>\n"
			"\t\t</tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\" class=\"fld\">Code:</td><td><xfield fldspec=\"fs_Usec\" var=\"testCode\" flags=\"req\"/></td></tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Complete Registration\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t</xform>\n"
			"\t</response>\n"
			"</formhdl>\n\n" ;

			Z.Printf("<formhdl name=\"fhdl_phase2_%s\" form=\"form_reg_%s\" ops=\"cookie\">\n", *cname, *cname) ;
			Z <<
			"\t<procdata>\n"
			"\t\t<testeq param1=\"%e:testCode;\" param2=\"%s:iniCode;\">\n"
			"\t\t\t<error name=\"InvalidCode\" bgcolor=\"eeeeee\">\n"
			"\t\t\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t\t\t<xform name=\"formCkCorp2\" action=\"hdlCkCorp\">\n"
			"\t\t\t\t\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t\t\t\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t\t\t\t\t<tr><td align=center class=title>Oops! Wrong Code!</td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"250\"><td class=\"main\">Please check the email again and type in or cut and paste the code.</td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"50\"><td align=\"center\" class=\"fld\">Code:</td><td><xfield fldspec=\"fs_Usec\" var=\"testCode\" flags=\"req\"/></td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Complete Registration\"/></td></tr>\n"
			"\t\t\t\t\t</table>\n"
			"\t\t\t\t</xform>\n"
			"\t\t\t</error>\n"
			"\t\t</testeq>\n" ;

			Z.Printf("<addSubscriber class=\"%s\" userName=\"%css:orgname;\" userPass=\"%css:password;\" userEmail=\"%css:email;\">\n",
				*cname, CHAR_PERCENT, CHAR_PERCENT, CHAR_PERCENT) ;

			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("\t\t\t<seteq member=\"%s\" input=\"%cs%s\"/>\n", mname, CHAR_PERCENT, mname) ;
			}

			Z <<
			"\t\t</addSubscriber>\n"
			"\t\t<logon user=\"%s:orgname;\"/>\n"
			"\t</procdata>\n" ;

			Z <<
			"\t<error name=\"RegistrationError\" bgcolor=\"eeeeee\">\n"
			"\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t<div class=\"stdpage\">\n"
			"\t\t<table bgcolor=\"#FFDDDD\" width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t<tr><td align=center class=title>Internal Error</td></tr>\n"
			"\t\t<tr height=\"160\"><td>&nbsp;</td></tr>\n"
			"\t\t<tr>\n"
			"\t\t<td>An Internal error has occured and system administration has been notified. Please try again later.</td>\n"
			"\t\t</tr>\n"
			"\t\t<tr height=\"160\"><td>&nbsp;</td></tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Return to Site\" goto=\"/index.html\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t\t</div>\n"
			"\t</error>\n" ;

			Z <<
			"\t<response name=\"CorpCodeOK\" bgcolor=\"eeeeee\">\n"
			"\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t\t<tr><td align=center class=title>Registration Complete</td></tr>\n"
			"\t\t\t<tr height=\"250\">\n"
			"\t\t\t\t<td class=\"main\">\n"
			"\t\t\t\tYour registration as %u:orgname; is now complete. You are logged in and may now use the document processing facility.\n"
			"\t\t\t\t</td>\n"
			"\t\t\t</tr>\n"
			"\t\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Continue\" goto=\"/menu\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t</response>\n"
			"</formhdl>\n" ;
		}
		else
		{
			//	Write out standard commit form-handler. (fhdl_classname)
			Z.Printf("<formhdl name=\"fhdl_add_%s\" form=\"form_add_%s\">\n", *cname, *cname) ;
			Z << "\t<procdata>\n" ;
			Z.Printf("<commit class=\"%s\">\n", *cname) ;

			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("<seteq member=\"%s\" input=\"%ce:%s;\"/>\n", mname, CHAR_PERCENT, mname) ;
			}

			Z <<
			"\t\t</commit>\n"
			"\t</procdata>\n\n" ;

			Z.Printf("<error goto=\"/add_%s\"/>\n", *cname) ;

			Z.Printf("<response name=\"%s\" bgcolor=\"eeeeee\">\n", *cname) ;

			Z <<
			"\t</response>\n"
			"</formhdl>\n" ;
		}

		Z << "</disseminoInclude>\n" ;

		//	Commit the pro-forma page, form and form-handlers to a file called 'classname.forms' This can then be adapted and included in the main XML file for
		//	the site
		if (Z.Size())
		{
			sprintf(fname, "%s/%s.forms", *m_Configdir, *cname) ;

			m_pLog->Out("%s. Writing form of %d bytes to %s\n", *_fn, Z.Size(), fname) ;

			os.open(fname) ;
			if (os.fail())
				threadLog("Could not open export forms file (classforms.dat)\n") ;
			else
			{
				os << Z ;
				if (os.fail())
					threadLog("Could not write export forms file (classforms.dat)\n") ;
				os.close() ;
			}
			os.clear() ;
		}

		Z.Clear() ;
	//}

	return rc ;
}
