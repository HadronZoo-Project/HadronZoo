//
//	File:	hdsGenerate.cpp
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

#include <fstream>

#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>

#include "hzDissemino.h"

using namespace std ;

/*
**	Prototypes
*/

const char* Exec2Txt	(Exectype eType) ;

/*
**	Shorthand for generating HTML
*/

static	hzString	s_Recaptcha = "<script src=\"https://www.google.com/recaptcha/api.js\" async defer></script>\n" ;
				//	Google script

static	hzString	s_std_metas =	//	Standard meta tags for generated pages
	"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n"
	"<meta http-equiv=\"Expires\" content=\"-1\"/>\n"
	"<meta http-equiv=\"Cache-Control\" content=\"no-cache\"/>\n"
	"<meta http-equiv=\"Cache-Control\" content=\"no-store\"/>\n"
	"<meta http-equiv=\"Pragma\" content=\"no-cache\"/>\n"
	"<meta http-equiv=\"Pragma\" content=\"no-store\"/>\n" ;

/*
**	Variables
*/

static	hzString	tagInput = "input" ;		//	String for HTML tag input
static	hzString	tagImg = "img" ;			//	String for HTML tag img
static	hzString	tagBr = "br" ;				//	String for HTML tag br

extern	hzString	_dsmScript_ckEmail ;		//	Javascript to validate email address
extern	hzString	_dsmScript_ckUrl ;			//	Javascript to validate URL
extern	hzString	_dsmScript_ckExists ;		//	Javascript to run AJAX check for value already exists
extern	hzString	_dsmScript_tog ;			//	Javascript to toggle nav-tree items
extern	hzString	_dsmScript_loadArticle ;	//	Javascript to load aricles upon selection from nav-tree
extern	hzString	_dsmScript_navtree ;		//	Javascript to display nav-tree
extern	hzString	_dsmScript_gwp ;			//	Javascript to obtain window params

/*
**	Pull down menu driver
*/

void	hdsNavbar::Generate	(hzChain& Z, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Aggregates into the supplied chain, the HTML nessesary to display the navbar
	//
	//	Arguments:	1)	Z		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsNavbar::Generate") ;

	hzPair		p ;				//	NV pair representing page url and page title
	hdsInfo*	pInfo = 0 ;		//	Session
	hdsSubject*	pSubj ;			//	Subject
	hdsPage*	pPage ;			//	Page within subject
	uint32_t	x ;				//	Subhect iterator
	uint32_t	y ;				//	Page iterator (within subject)

	if (pE)
		pInfo = (hdsInfo*) pE->Session() ;

	if (m_JS)
	{
		Z << "<script language=\"javascript\" src=\"/jsc/navbarItems.js\"></script>\n" ;
		Z << "<script language=\"javascript\" src=\"/jsc/navbarMenu.js\"></script>\n" ;
		return ;
	}

	Z <<
	"\n<table cellspacing='0' cellpadding='0' width='90%' background-color='#000000' border='0'>\n"
	"<tr>\n"
	"\t<td width='1'>&nbsp;</td><td height='20' valign='center'>\n" ;

	//	Prepare headings
	for (x = 0 ; x < m_pApp->m_vecPgSubjects.Count() ; x++)
	{
		pSubj = m_pApp->m_vecPgSubjects[x] ;

		if (!pSubj->pglist.Count())
			continue ;

		if (pSubj->pglist.Count() <= 1)
			Z.Printf("\t<span id='Pdm%d' class='measure'><a href='%s' onmouseover='hideLast()' class='top'>", x, *pSubj->first) ;
		else
			Z.Printf("\t<span id='Pdm%d' class='measure'><a href=\"#\" class='top' onmouseover='doSub(%d)' onclick='return false'>", x, x) ;

		Z.Printf("%s &nbsp; &nbsp;</a></span>\n", *pSubj->subject) ;
	}
	Z <<
	"\t</td>\n"
	"</tr>\n"
	"</table>\n" ;

	//	Prepare lists (sub-menus)
	for (x = 0 ; x < m_pApp->m_vecPgSubjects.Count() ; x++)
	{
		Z.Printf("<div id='Sub%d' style='visibility:hidden;position:absolute;width:relative;' onmouseover='IEBum(0,%d)' onmouseout='IEBum(1,%d)'>\n",
			x, x, x) ;
		Z << "\t<table border='0' background-color='#000000' cellspacing=0 cellpadding=0 width='200'>\n" ; 

		pSubj = m_pApp->m_vecPgSubjects[x] ;

		if (!pSubj->pglist.Count())
			continue ;

		for (y = 0 ; y < pSubj->pglist.Count() ; y++)
		{
			pPage = pSubj->pglist[y] ;

			if (pPage->m_Access == ACCESS_PUBLIC
					|| (pPage->m_Access == ACCESS_NOBODY && (!pInfo || !(pInfo->m_Access & ACCESS_MASK)))
					|| (pInfo && (pInfo->m_Access & ACCESS_ADMIN || (pInfo->m_Access & ACCESS_MASK) == pPage->m_Access)))
				Z.Printf("\t<tr><td height='13' valign='center'>&nbsp;<a href='%s' id='link' class='top' onmouseover='IEBum(0,%d)'>%s</a></td></tr>\n",
					*pPage->m_Url, x, *pPage->m_Title) ;
		}

		Z << "\t</table>\n</div>\n" ;
	}
}

/*
**	Error message
*/

void	hdsApp::_doHead	(hzChain& Z, const char* cpPage)
{
	//	Formulate a standard HTML header (the <head> tag) and agregate it to the supplied chain. The title is set to the supplied value and the header
	//	contains a link to the application's stylesheet.
	//
	//	Arguments:	1)	Z		The target chain
	//				2)	cpPage	The page title
	//
	//	Returns:	None

	_hzfunc("hdsApp::_doHead") ;

	Z <<
	"<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n"
	"<title>" << cpPage << "</title>\n"
	<< s_std_metas <<
	"<link rel=\"stylesheet\" href=\"" << m_namCSS << "\">\n"
	"</head>\n\n" ;
	//"<link rel=\"icon\" href=\"data:;base64,iVBORw0KGgo=\">\n"
}

void	hdsApp::_doHeadR	(hzChain& Z, const char* cpPage, const char* cpUrl, int nDelay)
{
	//	Formulate a standard HTML header as with _doHead() but with the addition of a <meta> refresh tag to direct the browser to another URL after a
	//	stated time interval.
	//
	//	Arguments:	1)	Z		The target chain
	//				2)	cpPage	The page title
	//				3)	cpUrl	The redirection RL
	//				4)	nDelay	The delay in seconds
	//
	//	Returns:	None

	_hzfunc("hdsApp::_doHeadR") ;

	Z <<
	"<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n"
	"<title>" << cpPage << "</title>\n" ;

	if (!nDelay)
		nDelay = 10 ;
	if (cpUrl)
		Z.Printf("<meta http-equiv=\"Refresh\" content=\"%d; url='%s'\">\n", nDelay, cpUrl) ;
	else
		Z.Printf("<meta http-equiv=\"Refresh\" content=\"%d\">\n", nDelay) ;

	Z << s_std_metas <<
	"<link rel=\"stylesheet\" href=\"" << m_namCSS << "\">\n"
	"</head>\n\n" ;
}

//	FnGrp:	hdsApp::SendErrorPage
//
//	Generate HTML to display a system error report. This will name the function in which the error occured and the error message
//
//	Returns:	None 
//
//	Func:	hdsApp::_xerror	(hzHttpEvent*, HttpRC, const char*, const char* ...)
//	Func:	hdsApp::_xerror	(hzHttpEvent*, HttpRC, const char*, hzChain&)

void	hdsApp::SendErrorPage	(hzHttpEvent* pE, HttpRC rv, const char* func, const char* va_alist ...)
{
	//	Arguments:	1)	pE			The current HTTP event
	//				2)	rv			The required HTML return code
	//				3)	func		The function name
	//				4)	va_alist	The error message either as a varargs string
	//
	//	Returns:	None

	_hzfunc("hdsApp::SendErrorPage(1)") ;

	va_list			ap1 ;		//	Variable argument list
	hzChain			C ;			//	Output chain
	hzChain			E ;			//	Error chain
	hzIpConnex*		pConn ;		//	Client

	va_start(ap1, va_alist) ;

	E._vainto(va_alist, ap1) ;

	pConn = pE->GetConnex() ;
	if (pConn)
		pConn->m_Track << E ;
	else
	{
		m_pLog->Record("%d: ", rv) ;
		m_pLog->Out(E) ;
		m_pLog->Out("\n") ;
	}

	_doHead(C, "Error") ;
	C << "<body marginwidth=\"0\" marginheight=\"0\" leftmargin=\"0\" topmargin=\"0\">\n\n" ;
	C << "<center><h2>Oops!</h2></center>\n" ;
	C << "<table width=\"300\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
	C << "<tr height=\"100\"><td>Function " << func << " Has produced the following error<td></tr>\n" ;
	C << "<tr height=\"350\"><td>" << E << "</td></tr>\n" ;
	C.Printf("<tr height=\"150\"><td><input type=\"button\" value=\"Go Back\" onclick=\"window.location.href='%s'\"></td></tr>\n", *pE->Referer()) ;
	C << "</table>\n\n</body>\n</html>\n" ;

	pE->SendRawChain(rv, HMTYPE_TXT_HTML, C, 0, false) ;
}

void	hdsApp::SendErrorPage	(hzHttpEvent* pE, HttpRC rv, const char* func, hzChain& error)
{
	//	Arguments:	1)	pE		The current HTTP event
	//				2)	rv		The required HTML return code
	//				3)	func	The function name
	//				4)	error	The error message as a preformulated chain
	//
	//	Returns:	None

	_hzfunc("hdsApp::SendErrorPage(1)") ;

	hzChain			 C ;		//	Output chain
	hzIpConnex*		pConn ;		//	Client

	pConn = pE->GetConnex() ;
	if (pConn)
		pConn->m_Track << error ;
	else
		m_pLog->Out(error) ;

	_doHead(C, "Error") ;
	C << "<body marginwidth=\"0\" marginheight=\"0\" leftmargin=\"0\" topmargin=\"0\">\n\n" ;
	C << "<center><h2>Oops!</h2></center>\n" ;
	C << "<table width=\"300\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
	C << "<tr height=\"100\"><td>Function " << func << " Has produced the following error<td></tr>\n" ;
	C << "<tr height=\"350\"><td>" << error << "</td></tr>\n" ;
	C.Printf("<tr height=\"150\"><td><input type=\"button\" value=\"Go Back\" onclick=\"window.location.href='%s'\"></td></tr>\n", *pE->Referer()) ;
	C << "</table>\n\n</body>\n</html>\n" ;

	pE->SendRawChain(rv, HMTYPE_TXT_HTML, C, 0, false) ;
}

/*
**	Web Component display functions
*/

void	hdsButton::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Aggregate to the supplied chain, HTML to manifest a button.
	//
	//	The button can be a link (GET request) OR a form action (POST request). In the link case, button member m_Linkto will give the location. In the form action case, there will
	//	need to be submission URL which is obtained from the form reference (given in HTTP event variable m_pContextForm).
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsButton::Generate") ;

	hdsFormref*	pFR ;	//	Form reference
	hzString	url ;	//	URL to use (if form action)
	uint32_t	x ;		//	Indent counter

	if (m_Line != nLine)
	{
		C.AddByte(CHAR_NL) ;
		for (x = m_Indent ; x ; x--)
			C.AddByte(CHAR_TAB) ;
		nLine = m_Line ;
	}

	if (pE->m_pContextForm)
	{
		pFR = (hdsFormref*) pE->m_pContextForm ;
		x = m_pApp->m_FormRef2Url.First(pFR) ;
		if (x >= 0)
			url = m_pApp->m_FormRef2Url.GetObj(x + m_Resv - 1) ;
	}

	if (m_Formname)
	{
		if (url)
			C.Printf("<input type=\"submit\" formaction=\"%s\" value=\"%s\">", *url, *m_strContent) ;
		else
			C.Printf("<input type=\"submit\" value=\"%s\">", *m_strContent) ;
	}
	else
	{
		if (pE && m_Linkto[0] == CHAR_PERCENT)
			C.Printf("<input type=\"button\" value=\"%s\" onclick=\"window.location.href='%s'\">", *m_strContent, *m_pApp->ConvertText(m_Linkto, pE)) ;
		else
			C.Printf("<input type=\"button\" value=\"%s\" onclick=\"window.location.href='%s'\">", *m_strContent, *m_Linkto) ;
	}
}

void	hdsRecap::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Aggregates to the supplied chain (the HTML body), the RECAPTCHA tag responsible for the appearence of the google 'human being' test icon. This
	//	will contain the public key which must be supplied as an argument to the <project> tag.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsRecap::Generate") ;

	if (m_pApp->m_KeyPublic)
		C.Printf("<div class=\"g-recaptcha\" data-sitekey=\"%s\"></div>", *m_pApp->m_KeyPublic) ;
	else
		C << "<div class=\"g-recaptcha\" invalid data-sitekey=\"not supplied\"></div>" ;
}

void	hdsDirlist::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Does not in itself produce HTML but the subtags do. The complete set of subtags are called for each member of the list the hdsTable controls.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsDirlist::Generate") ;

	hzMapS	<hzString,hzDirent>	byName ;	//	Directory entries by name
	hzMapM	<uint32_t,hzDirent>	byDate ;	//	Directory entries by name

	hzVect	<hzDirent>		dirents ;		//	List of directories
	hzList	<hdsCol>::Iter	ci ;			//	Column iterator

	hzDirent	de ;		//	Directory entry
	hzXDate		date ;		//	Last mod date
	hdsCol		col ;		//	Column
	hzAtom		atom ;		//	Current value
	hdsVE*		pVE ;		//	Visible entitiy
	hzString	apath ;		//	Translated path (absolute so includes doc root)
	hzString	rpath ;		//	Translated path (relative to doc root)
	hzString	crit ;		//	Resolved criteria
	uint32_t	nDirs ;		//	Number of directories
	uint32_t	nFiles ;	//	Number of files
	uint32_t	x ;			//	Table controller
	uint32_t	y ;			//	Table controller

	//	Do the heading
	C.AddByte(CHAR_NL) ;
	for (x = m_Indent ; x ; x--)
		C.AddByte(CHAR_TAB) ;

	//	Obtain resolved parmeters
	rpath = m_pApp->ConvertText(m_Directory, pE) ;
	apath = m_pApp->m_Docroot + rpath ;
	crit = m_pApp->ConvertText(m_Criteria, pE) ;

	//	Provide filelist matching the criteria
	ReadDir(dirents, *apath, *crit) ;

	if (!dirents.Count())
	{
		//	Print the ifnone tags
		for (pVE = m_pNone ; pVE ; pVE = pVE->Sibling())
			pVE->Generate(C, pE, nLine) ;
	}
	else
	{
		//	Sort directory entries
		for (nDirs = nFiles = x = 0 ; x < dirents.Count() ; x++)
		{
			de = dirents[x] ;

			if (de.IsDir())
				nDirs++ ;
			else
				nFiles++ ;

			if (m_Order == 1 || m_Order == 2)
				byName.Insert(de.Name(), de) ;
			if (m_Order == 3 || m_Order == 4)
				byDate.Insert(de.Mtime(), de) ;
		}

		if (m_Order)
		{
			//	Now place back in array
			if (m_Order == 1)
				for (x = 0 ; x < dirents.Count() ; x++)
					dirents[x] = byName.GetObj(x) ;

			if (m_Order == 3)
				for (x = 0 ; x < dirents.Count() ; x++)
					dirents[x] = byDate.GetObj(x) ;

			if (m_Order == 2)
				for (y = dirents.Count() - 1, x = 0 ; x < dirents.Count() ; y--, x++)
					dirents[x] = byName.GetObj(y) ;

			if (m_Order == 4)
				for (y = dirents.Count() - 1, x = 0 ; x < dirents.Count() ; y--, x++)
					dirents[x] = byDate.GetObj(y) ;
		}
			
		//	Create HTML table for listing
		C.Printf("<table width=\"%d\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"%s\">\n", m_Width, *m_CSS) ;
		C.Printf("<tr valign=\"top\" height=\"20\"><td>Listing %d directories and %d files</td></tr>\n", nDirs, nFiles) ;
		C.Printf("<tr valign=\"top\" height=\"%d\">\n\t<td>\n", m_Height) ;
		C.Printf("\t<table width=\"%d\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"%s\">\n", m_Width, *m_CSS) ;

		//	Do table headers
		C << "\t<tr>\n" ;
		for (ci = m_Cols ; ci.Valid() ; ci++)
		{
			col = ci.Element() ;

			C.Printf("\t<th width=\"%d\">%s</th>\n", col.m_nSize, *col.m_Title) ;
		}
		C << "\t</tr>\n" ;

		//	Do first rows as parent directory
		if (pE->m_Resarg)
		{
			C << "\t<tr>" ;
			for (ci = m_Cols ; ci.Valid() ; ci++)
			{
				col = ci.Element() ;

				if (col.m_Title == "Mtime")
					C << "<td>---</td>" ;
				else if (col.m_Title == "Type")
					C << "<td>DIR</td>" ;
				else if (col.m_Title == "Size")
					C << "<td>---</td>" ;
				else if (col.m_Title == "Name")
					C.Printf("<td><a href=\"%s\">Back to parent dir</a></td>", *m_Url) ;
				else
					C << "<td>&nbsp;</td>" ;
			}
			C << "\t</tr>\n" ;
		}

		//	Do table rows (directories)
		for (x = 0 ; x < dirents.Count() ; x++)
		{
			de = dirents[x] ;
			if (!de.IsDir())
				continue ;

			C << "\t<tr>" ;
			for (ci = m_Cols ; ci.Valid() ; ci++)
			{
				col = ci.Element() ;

				if (col.m_Title == "Mtime")
					{ date.SetByEpoch(de.Mtime()) ; C.Printf("<td>%s</td>", *date.Str()) ; }
				else if (col.m_Title == "Type")
					C << "<td>DIR</td>" ;
				else if (col.m_Title == "Size")
					C.Printf("<td alight=\"right\">%s</td>", *FormalNumber(de.Size())) ;
				else if (col.m_Title == "Name")
					C.Printf("<td><a href=\"%s-%s\"> %s</a></td>", *m_Url, *de.Name(), *de.Name()) ;
				else
					C << "<td>&nbsp;</td>" ;
			}
			C << "\t</tr>\n" ;
		}

		//	Do table rows (files)
		for (x = 0 ; x < dirents.Count() ; x++)
		{
			de = dirents[x] ;
			if (de.IsDir())
				continue ;

			C << "\t<tr>" ;
			for (ci = m_Cols ; ci.Valid() ; ci++)
			{
				col = ci.Element() ;

				if (col.m_Title == "Mtime")
					{ date.SetByEpoch(de.Mtime()) ; C.Printf("<td>%s</td>", *date.Str()) ; }
				else if (col.m_Title == "Type")
					C << "<td>File</td>" ;
				else if (col.m_Title == "Size")
					C.Printf("<td alight=\"right\">%s</td>", *FormalNumber(de.Size())) ;
				else if (col.m_Title == "Name")
				{
					if (pE->m_Resarg)
						C.Printf("<td><a href=\"/userdir/%s/%s\"> %s</a></td>", *pE->m_Resarg, *de.Name(), *de.Name()) ;
					else
						C.Printf("<td><a href=\"/userdir/%s\"> %s</a></td>", *de.Name(), *de.Name()) ;
				}
				else
					C << "<td>&nbsp;</td>" ;
			}
			C << "\t</tr>\n" ;
		}
		C << "\t</table>\n</table>\n" ;
	}
}

void	hdsTable::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Does not in itself produce HTML but the subtags do. The complete set of subtags are called for each member of the list the hdsTable controls.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsTable::Generate") ;

	hzList<hdsCol>::Iter	ci ;		//	Column iterator
	hzVect<uint32_t>		items ;		//	Items drawn from class member (of table) to be listed as rows

	hdbIdset	srchResult ;			//	Items selected from applicable repository
	hdsCol		col ;					//	Column
	hdsVE*		pVE ;					//	For processing subtags
	hzAtom		atom ;					//	Current value
	hzString	value ;					//	Value in session if set
	uint32_t	nFound ;				//	Table controller
	uint32_t	objId ;					//	Object id

	//	Do the heading
	C.AddByte(CHAR_NL) ;

	//	Do the select
	if (m_Criteria == "all")
		nFound = m_pCache->Count() ;
	else
	{
		m_pCache->Select(srchResult, *m_Criteria) ;
		nFound = srchResult.Count() ;
	}

	//	Display results
	if (nFound == 0)
	{
		//	In the event of no objects found, print the ifnone tags
		for (pVE = m_pNone ; pVE ; pVE = pVE->Sibling())
			pVE->Generate(C, pE, nLine) ;
	}
	else
	{
		//	Create HTML table for listing
		C.Printf("<table width=\"%d\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"%s\">\n", m_nWidth, *m_CSS) ;
		C.Printf("<tr valign=\"top\" height=\"20\"><td>Listing %d objects</td></tr>\n", nFound) ;
		C.Printf("<tr valign=\"top\" height=\"%d\">\n\t<td>\n", m_nHeight) ;
		C.Printf("\t<table width=\"%d\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"%s\">\n", m_nWidth, *m_CSS) ;

		//	Do table headers
		C << "\t<tr>\n" ;
		for (ci = m_Cols ; ci.Valid() ; ci++)
		{
			col = ci.Element() ;

			C.Printf("\t<th width=\"%d\">%s</th>\n", col.m_nSize, *col.m_Title) ;
		}
		C << "\t</tr>\n" ;

		//	Provide filelist matching the criteria
		for (objId = 1 ; objId <= nFound ; objId++)
		{
			//	Now display object as row
			C << "\t<tr>" ;
			for (ci = m_Cols ; ci.Valid() ; ci++)
			{
				col = ci.Element() ;

				m_pCache->Fetchval(atom, col.m_mbrNo, objId) ;
				if (atom.IsSet())
					value = atom.Str() ;
				else
					value = 0 ;

				if (col.m_nSize)
					C.Printf("<td width=\"%d\">%s</td>", col.m_nSize, *value) ; //atom.Str()) ;
				else
					C.Printf("<td>%s</td>", *value) ;	//atom.Str()) ;
			}
			C << "</tr>\n" ;
		}
		C << "</table>\n" ;
	}
}

void	hdsField::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Aggregates to the supplied chain, HTML code for presenting an input field as part of a form. The resulting field will be unpopulated unless a
	//	data source is specified as an attribute to <xfield> and only then if that source successfully evaluates.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsField::Generate") ;

	hzAtom			atom ;		//	For fetching field value from db
	hdbEnum*		pSlct ;		//	Selector
	const char*		pDictStr ;	//	String from dictionary string number
	hzString		value ;		//	Value in session if set
	hzString		S ;			//	Temp string
	uint32_t		rows ;		//	Selector rows for radio
	uint32_t		n ;			//	Selector items iterator
	uint32_t		strNo ;		//	Dictionary string number
	hzEcode			rc ;		//	Return code

	if (pE && m_Source)
	{
		rc = m_pApp->PcEntConv(atom, m_Source, pE) ;
		if (atom.IsSet())
			value = atom.Str() ;
	}

	switch	(m_Fldspec.htype)
	{
	case HTMLTYPE_HIDDEN:		//	Insert hidden value
		C.Printf("<input type=\"hidden\" name=\"%s\" value=\"%s\"/>", *m_Varname, *value) ;	//atom.Str()) ;
		break ;

	case HTMLTYPE_SELECT:		//	Write out the selector with already selected options marked as selected

		pSlct = (hdbEnum*) m_Fldspec.m_pType ;

		if (m_flagVE & VE_MULTIPLE)
			C.Printf("<select name=\"%s\" multiple>", *m_Varname) ;
		else
			C.Printf("<select name=\"%s\">", *m_Varname) ;

		for (n = 0 ; n < pSlct->m_Items.Count() ; n++)
		{
			strNo = pSlct->m_Items[n] ;
			pDictStr = _hzGlobal_StringTable->Xlate(strNo) ;

			if (value == pDictStr)
				C.Printf("<option selected>%s</option>", pDictStr) ;
			else
				C.Printf("<option>%s</option>", pDictStr) ;
		}
		C << "</select>" ;
		break ;

	case HTMLTYPE_RADIO:
		//	Write out the selector as a radio button set. The seletor cannot be  multiple but the number of colums must be specified. The radio set will appear
		//	within a <table> in which each column except the last, will contain N items. N is calculated as the total selector population divided by the number
		//	of columns, then plus one. The last column will contain the remainder. The items will be displayed in the order they appear in the selector but are
		//	used to fill each column before moving on to the next. The columns are from left to right.

		pSlct = (hdbEnum*) m_Fldspec.m_pType ;
		rows = (pSlct->m_Items.Count() / m_Fldspec.nCols) + 1 ;

		C << "<table>\n" ;
		C << "\t<tr>\n" ;
		C << "\t\t<td>\n" ;

		for (n = 0 ; n < pSlct->m_Items.Count() ; n++)
		{
			strNo = pSlct->m_Items[n] ;
			pDictStr = _hzGlobal_StringTable->Xlate(strNo) ;

			if (n && (n % rows) == 0)
			{
				C.Printf("<input type=\"radio\" name=\"%s\" value=\"%d\">%s\n", *m_Varname, n, pDictStr) ;
				C << "\t\t<td>\n\t\t</td>\n" ;
				continue ;
			}

			C.Printf("<input type=\"radio\" name=\"%s\" value=\"%d\">%s<br>\n", *m_Varname, n, pDictStr) ;
		}
		C << "\t\t</td>\n" ;
		C << "\t</tr>\n" ;
		C << "</table>\n" ;
		break ;


	case HTMLTYPE_CHECKBOX:	if (value)
								C.Printf("<input type=\"checkbox\" name=\"%s\" checked>", *m_Varname) ;
							else
								C.Printf("<input type=\"checkbox\" name=\"%s\">", *m_Varname) ;
							break ;

	case HTMLTYPE_FILE:		C.Printf("<input type=\"file\" name=\"%s\" value=\"%s\">", *m_Varname, *value) ;
							break ;

	case HTMLTYPE_TEXT:
	case HTMLTYPE_TEXTAREA:

		if (m_Fldspec.nRows <= 1)
		{
			if (m_flagVE & VE_UNIQUE)
				C.Printf("<input type=\"text\" name=\"%s\" size=\"%d\" maxlength=\"%d\" onchange=\"ckUnique_%s()\"",
					*m_Varname, m_Fldspec.nCols, m_Fldspec.nSize, *m_Varname) ;
			else
				C.Printf("<input type=\"text\" name=\"%s\" size=\"%d\" maxlength=\"%d\"", *m_Varname, m_Fldspec.nCols, m_Fldspec.nSize) ;

			if (m_CSS)
				C.Printf(" class=\"%s\"", *m_CSS) ;
			if (value)
				C.Printf(" value=\"%s\"", *value) ;
			if (m_flagVE & VE_DISABLED)
				C << " disabled" ;
			C << "/>" ;
		}
		else
		{
			C.Printf("<textarea name=\"%s\" rows=\"%d\" cols=\"%d\" maxlength=\"%d\"", *m_Varname, m_Fldspec.nRows, m_Fldspec.nCols, m_Fldspec.nSize) ;

			if (m_CSS)
				C.Printf(" class=\"%s\"", *m_CSS) ;
			C.AddByte(CHAR_MORE) ;

			if (value)
				C << value ;
			C << "</textarea>" ;
		}
		break ;

	case HTMLTYPE_PASSWORD:

		C.Printf("<input type=\"password\" name=\"%s\" size=\"%d\" maxlength=\"%d\"", *m_Varname, m_Fldspec.nCols, m_Fldspec.nSize) ;
		if (m_CSS)
			C.Printf(" class=\"%s\"", *m_CSS) ;
		if (value)
			C.Printf(" value=\"%s\"", *value) ;
		C << "/>" ;
		break ;

	default:
		C.Printf("Sorry: HTML Unknown type. FldDesc=%s Name=%s Type=%s htype=%d Rows=%d Cols=%d Size=%d",
			*m_Fldspec.m_Refname, *m_Varname, m_Fldspec.m_pType->TxtTypename(), m_Fldspec.htype, m_Fldspec.nRows, m_Fldspec.nCols, m_Fldspec.nSize) ;
		break ;
	}
}

void	hdsFormref::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Aggregates to the supplied chain (the HTML body), the HTML code for presenting a form. This will present the whole form so everything from the
	//	opening <form> tag to the closing </form> antitag.
	//
	//	The HTML for the form is actually vested with the form definition so generating HTML for a form reference is a matter of ....
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsFormref::Generate") ;

	hzList<hdsVE*>::Iter	ih ;	//	Html entity iterator

	hdsFormdef*		pFormdef ;		//	Actual form referenced
	hdsVE*			pVE ;			//	Html entity
	hzString		url ;			//	URL from hdsApp map m_FormRef2Url
	uint32_t		nV ;			//	Visual entity iterator
	uint32_t		relLn ;			//	Relative line
	uint32_t		x ;				//	Tab controller

	if (m_Line != nLine)
	{
		C.AddByte(CHAR_NL) ;
		for (x = m_Indent ; x ; x--)
			C.AddByte(CHAR_TAB) ;
		nLine = m_Line ;
	}

	pFormdef = m_pApp->m_FormDefs[m_Formname] ;

	if (!pFormdef)
	{
		C.Printf("<p>ERROR: NULL FORM DEF</p>\n") ;
		return ;
	}

	if (pFormdef->m_nActions == 1)
	{
		x = m_pApp->m_FormRef2Url.First(this) ;
		url = m_pApp->m_FormRef2Url.GetObj(x) ;
		//threadLog("%s URL is given as %s (form ref is %p)\n", *_fn, *url, this) ;

		if (m_flagVE & VE_MULTIPART)
			C.Printf("<form name=\"%s\" method=\"POST\" action=\"%s\" onsubmit=\"return ck%s()\" enctype=\"multipart/form-data\">\n",
				*m_Formname, *url, *m_Formname) ;
		else
			C.Printf("<form name=\"%s\" method=\"POST\" action=\"%s\" onsubmit=\"return ck%s()\">\n", *m_Formname, *url, *m_Formname) ;
	}
	else
	{
		if (m_flagVE & VE_MULTIPART)
			C.Printf("<form name=\"%s\" method=\"POST\" enctype=\"multipart/form-data\">\n",
				*m_Formname, *url, *url) ;
		else
			C.Printf("<form name=\"%s\" method=\"POST\">\n", *m_Formname, *url, *url) ;
	}

	relLn = nLine ;
	pE->m_pContextForm = this ;
	for (nV = 0 ; nV < pFormdef->m_VEs.Count() ; nV++)
	{
		pVE = pFormdef->m_VEs[nV] ; pVE->Generate(C, pE, relLn) ;
	}

	if (nLine != m_Line)
	{
		C.AddByte(CHAR_NL) ;
		for (x = m_Indent ; x ; x--)
			C.AddByte(CHAR_TAB) ;
	}
	C << "</form>\n" ;
}

void	hdsPage::WriteValidationJS	(void)
{
	//	This is run once upon startup and applies to each page in which there is one or more forms. It Creates the JavaScript that must be run to pre
	//	validate form content before submission to the server.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hdsPage::WriteValidationJS") ;

	hzList<hdsFormref*>::Iter	fi ;	//	Form iterator

	hzChain			Z ;					//	For building page validation script(s)
	hdsFormref*		pFormref ;			//	Form in page
	hdsFormdef*		pFormdef ;			//	Form in page
	hdsField*		pFld ;				//	Field in form
	uint32_t		n ;					//	Field iterator

	//	Check for pre-submission requirements. This is where one or more fields can only be validated by querying the server. For example in a form to
	//	register a new user, the email address must be new. The 'onchange' event is used to check on the server for the existance of the field class,
	//	member and value.

	_hzGlobal_Dissemino->m_pLog->Out("%s. DOING PAGE %s\n", *_fn, *m_Title) ;

	for (fi = m_xForms ; fi.Valid() ; fi++)
	{
		pFormref = fi.Element() ;
		m_pApp->m_pLog->Out("%s. DOING FORM REF %p\n", *_fn, pFormref) ;

		if (!pFormref)
			{ _hzGlobal_Dissemino->m_pLog->Out("%s. ERROR Page %s has a NULL form reference\n", *_fn, *m_Title) ; continue ; }

		//pFormdef = pFormref->m_pFormdef ;
		pFormdef = m_pApp->m_FormDefs[pFormref->m_Formname] ;
		if (!pFormdef)
			{ m_pApp->m_pLog->Out("%s. ERROR Page %s has a NULL form definition\n", *_fn, *m_Title) ; continue ; }

		for (n = 0 ; n < pFormdef->m_vecFlds.Count() ; n++)
		{
			pFld = pFormdef->m_vecFlds[n] ;

			if (!pFld->m_Varname)
				continue ;

			if (!(pFld->m_flagVE & VE_UNIQUE))
				continue ;

			if (!pFld->m_pClass || !pFld->m_pMem)
				continue ;

			Z.Printf("function ckUnique_%s()\n{\n", *pFld->m_Varname) ;
			Z.Printf("\tif (ckExists('%s','%s',document.%s.%s.value))\n", pFld->m_pClass->TxtTypename(), pFld->m_pMem->TxtName(), *pFormdef->m_Formname, *pFld->m_Varname) ;
			Z << "\t{\n" ;
			Z.Printf("\t\talert(\"%s already in use\");\n", *pFld->m_Varname) ;
			Z.Printf("\t\tdocument.%s.%s.value=\"\";\n", *pFormdef->m_Formname, *pFld->m_Varname) ;
			Z.Printf("\t\tdocument.%s.%s.focus();\n", *pFormdef->m_Formname, *pFld->m_Varname) ;
			Z << "\t}\n}\n" ;

			m_bScriptFlags |= INC_SCRIPT_EXISTS ;
		}

		//	Now do the ordinary format checking validation scripting
		Z.Printf("function ck%s()\n{\n", *pFormdef->m_Formname) ;

		for (n = 0 ; n < pFormdef->m_vecFlds.Count() ; n++)
		{
			pFld = pFormdef->m_vecFlds[n] ;

			if (!pFld->m_Varname)
				continue ;

			if (!(pFld->m_flagVE & VE_COMPULSORY))
				continue ;

			if (pFld->m_Fldspec.m_pType->Basetype() == BASETYPE_ENUM)
				Z.Printf("\tif (document.%s.%s.value==\"0\")\n", *pFormdef->m_Formname, *pFld->m_Varname) ;
			else
				Z.Printf("\tif (document.%s.%s.value==\"\")\n", *pFormdef->m_Formname, *pFld->m_Varname) ;
			Z.Printf("\t\t{ alert(\"Please provide your %s\"); document.%s.%s.focus(); return false; }\n", *pFld->m_Varname, *pFormdef->m_Formname, *pFld->m_Varname) ;

			if (pFld->m_Fldspec.m_pType->Basetype() == BASETYPE_EMADDR)
			{
				Z.Printf("\tif (!ckEmaddr(document.%s.%s.value))\n", *pFormdef->m_Formname, *pFld->m_Varname) ;
				Z.Printf("\t\t{ document.%s.%s.focus(); return false }\n", *pFormdef->m_Formname, *pFld->m_Varname) ;
				m_bScriptFlags |= INC_SCRIPT_CKEMAIL ;
			}

			if (pFld->m_Fldspec.m_pType->Basetype() == BASETYPE_URL)
			{
				Z.Printf("\tif (!ckUrl(document.%s.%s.value))\n", *pFormdef->m_Formname, *pFld->m_Varname) ;
				Z.Printf("\t\t{ document.%s.%s.focus(); return false }\n", *pFormdef->m_Formname, *pFld->m_Varname) ;
				m_bScriptFlags |= INC_SCRIPT_CKURL ;
			}
		}

		if (pFormdef->m_bScriptFlags & INC_SCRIPT_RECAPTCHA)
		{
			Z.Printf("\tif (grecaptcha.getResponse()==\"\")\n") ;
			Z.Printf("\t\t{ alert(\"Please prove you are human!\"); return false; }\n") ;
		}

		Z.Printf("\tdocument.%s.submit();\n\treturn true\n}\n", *pFormdef->m_Formname) ;
	}

	m_validateJS = Z ;
}

/*
**	Grapics
*/

void	DrawTexts	(hzList<hdsText>& texts, hzChain& Z, hzString& currfont)
{
	//	Category:	HTML Generation
	//
	//	Display text item (tx) into chain (Z) setting current font (currfont) and align code (ac)
	//
	//	Arguments:	1)	texts		The list of all text items for the diagram
	//				2)	Z			The HTML output chain
	//				3)	currfont	The applicable font
	//
	//	Returns:	None

	_hzfunc(__func__) ;

	hzList<hdsText>::Iter	ti ;	//	Text iterator

	hdsText		TX ;				//	Text part
	uint32_t	alignCode = 0 ;		//	Text align
	uint32_t	color = -1 ;		//	Current color

	for (ti = texts ; ti.Valid() ; ti++)
	{
		TX = ti.Element() ;

		if (currfont != TX.font)
			{ currfont = TX.font ; Z.Printf("ctx.font=\"%s\";\n", *currfont) ; }
		if (color != TX.linecolor)
			{ color = TX.linecolor ; Z.Printf("ctx.fillStyle=\"#%06x\";\n", color) ; }

		if (alignCode != TX.alignCode)
		{
			alignCode = TX.alignCode ;
			if		(alignCode == 1)	Z << "ctx.textAlign=\"center\";\n" ;
			else if (alignCode == 2)	Z << "ctx.textAlign=\"right\";\n" ;
			else
				Z << "ctx.textAlign=\"left\";\n" ;
		}

		Z.Printf("ctx.fillText(\"%s\",%d,%d);\n", *TX.text, TX.ypos, TX.xpos) ;
	}
}

void	hdsPieChart::Generate	(hzChain& Z, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Category:	HTML Generation
	//
	//	Displays a pie chart by means of the <canvas> tag and associated javascript.
	//
	//	Arguments:	1)	Z		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsPieChart::Generate") ;

	hzList<_pie*>::Iter	pi ;	//	Pie component iterator

	_pie*		pPie ;			//	Pie value
	hdsText		TX ;			//	Text part
	hzString	currfont ;		//	Current font
	uint32_t	y = 0 ;			//	y-coord
	uint32_t	x = 0 ;			//	x-coord
	uint32_t	n ;				//	Parts iterator

	//	Declare <canvas> and <script> tags
	Z.Printf("\n<canvas id=\"%s\" height=\"%d\" width=\"%d\" style=\"border:1px solid #000000; background:#%06x;\">",
		*m_Id, m_Height, m_Width, m_BgColor) ;
	Z << "Your Browser does not support the HTML5 canvas tag</canvas>\n" ;
	Z << "<script>\n" ;
	Z.Printf("var cvs = document.getElementById(\"%s\");\n", *m_Id) ;
	Z << "var ctx = cvs.getContext(\"2d\");\n" ;

	//	Draw header and footer if applicable
	DrawTexts(m_Texts, Z, currfont) ;

	if (currfont != m_Font)
	{
		currfont = m_Font ;
		Z.Printf("ctx.font=\"%s\";\n", *currfont) ;
	}

	//	Draw circle
	y = m_Rad + 20 ;
	x = m_Rad + 40 ;
	Z << "var\tdata=[" ;
	for (pi = m_Parts, n = m_Parts.Count() ; pi.Valid() ; n--, pi++)
	{
		pPie = pi.Element() ;
		Z.Printf("%d", pPie->value) ;
		Z.AddByte(n>1 ? CHAR_COMMA:CHAR_SQCLOSE) ;
	}
	Z << ";\nvar\tcolors=[" ;
	for (pi = m_Parts, n = m_Parts.Count() ; pi.Valid() ; n--, pi++)
	{
		pPie = pi.Element() ;
		Z.Printf("\"#%06x\"", pPie->color) ;
		Z.AddByte(n>1 ? CHAR_COMMA:CHAR_SQCLOSE) ;
	}
	Z << ";\n"
	"var\tlastPosn=Math.PI*1.5;\n"
	"var\ttotal=0;\n"
	"for(var i in data) { total += data[i]; }\n"
	"for (var i = 0; i < data.length; i++)\n"
	"{\n"
	"\tctx.fillStyle = colors[i];\n"
	"\tctx.beginPath();\n" ;
	Z.Printf("\tctx.moveTo(%d,%d);\n", y, x) ;
	Z.Printf("\tctx.arc(%d,%d,%d,lastPosn,lastPosn+(Math.PI*2*(data[i]/total)),false);\n", y, x, m_Rad) ;
	Z.Printf("\tctx.lineTo(%d,%d);\n", y, x) ;
	Z <<
	"\tctx.fill();\n"
	"\tlastPosn += Math.PI*2*(data[i]/total);\n"
	"}\n" ;

	//	Draw Index
	y = m_IdxY ;
	x = m_IdxX ;
	for (pi = m_Parts, n = m_Parts.Count() ; pi.Valid() ; n--, pi++)
	{
		pPie = pi.Element() ;
		Z.Printf("ctx.fillStyle=\"#%06x\";\n", pPie->color) ;
		Z.Printf("ctx.fillRect(%d,%d,%d,%d);\n", y, x, 12, 12) ;
		x += 15 ;
	}
	Z.Printf("ctx.fillStyle=\"#%06x\";\n", m_FgColor) ;
	Z << "ctx.textAlign=\"left\";\n" ;
	y += 15 ;
	x = m_IdxX + 10 ;
	for (pi = m_Parts, n = m_Parts.Count() ; pi.Valid() ; n--, pi++)
	{
		pPie = pi.Element() ;
		Z.Printf("ctx.fillText(\"%s\",%d,%d);\n", *pPie->header, y, x) ;
		x += 15 ;
	}

	Z += "cix.stroke();\n" ;	//	Draw it
	Z << "</script>\n" ;
}

void	hdsBarChart::Generate	(hzChain& Z, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Category:	HTML Generation
	//
	//	Displays a chart by placing in the page HTML, a <canvas> tag. This has to refer to a JavaScript which must appear earlier in the HTML
	//
	//	Arguments:	1)	Z		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsBarChart::Generate") ;

	hzList<_bset*>::Iter	ri ;	//	Component iterator

	_bset*		pSet ;		//	Component
	double		yval ;		//	For ploting y
	double		xval ;		//	For ploting x
	hzString	currfont ;	//	Current font
	uint32_t	div ;		//	Value divider
	uint32_t	pxrX ;		//	Pixel ratio x
	uint32_t	x ;			//	x-coord
	uint32_t	z ;			//	x-size
	uint32_t	y ;			//	y-coord
	uint32_t	n ;			//	Loop iterator

	Z.Printf("<canvas id=\"%s\" height=\"%d\" width=\"%d\" style=\"border:1px solid #000000; background:#%06x;\">",
		*m_Id, m_Height, m_Width, m_BgColor) ;
	Z << "Your Browser does not support the HTML5 canvas tag</canvas>\n" ;
	Z << "<script>\n" ;
	Z.Printf("var cvs = document.getElementById(\"%s\");\n", *m_Id) ;
	Z << "var ctx = cvs.getContext(\"2d\");\n" ;

	//	Build y-axis values and positions
	yval = (m_maxY - m_minY)/m_ydiv ;
	div = yval ;
	div++ ;
	m_slotw = m_ypix/div ;

	Z << "var yav=[" ;
	for (y = m_yLft + m_gap, yval = m_minY ; yval <= m_maxY ; yval += m_ydiv)
	{
		if (yval < m_maxY)
			Z.Printf("\"%d\",", (uint32_t) yval) ;
		else
			Z.Printf("\"%d\"];\n", (uint32_t) yval) ;
		y += m_slotw ;
	}

	Z << "var yap=[" ;
	for (y = m_yLft + m_gap, yval = m_minY ; yval <= m_maxY ; yval += m_ydiv)
	{
		if (yval < m_maxY)
			Z.Printf("%d,", y) ;
		else
			Z.Printf("%d];\n", y) ;
		y += m_slotw ;
	}

	//	Build x-axis values and positions
	xval = (m_maxX - m_minX)/m_xdiv ;
	div = xval ;
	pxrX = m_xpix/div ;

	Z << "var xav=[" ;
	for (x = m_xTop, xval = m_maxX ; xval >= m_minX ; xval -= m_xdiv)
	{
		if (xval > m_minX)
			Z.Printf("\"%d\",", (uint32_t) xval) ;
		else
			Z.Printf("\"%d\"];\n", (uint32_t) xval) ;
		x += pxrX ;
	}

	Z << "var xap=[" ;
	for (x = m_xTop, xval = m_maxX ; xval >= m_minX ; xval -= m_xdiv)
	{
		if (xval > m_minX)
			Z.Printf("%d,", x) ;
		else
			Z.Printf("%d];\n", x) ;
		x += pxrX ;
	}

	//	Set out colors
	ri = m_Sets ;
	pSet = ri.Element() ;
	Z.Printf("var colors=[\"#%06x\"", pSet->color) ;
	for (ri++ ; ri.Valid() ; ri++)
	{
		pSet = ri.Element() ;
		Z.Printf(",\"#%06x\"", pSet->color) ;
	}
	Z << "];\n" ;

	//	Set out datasets
	if (!m_xpad)
		m_xpad = new uint16_t[pSet->popl] ;
	for (n = 0 ; n < pSet->popl ; n++)
		m_xpad[n] = 0 ;

	ri = m_Sets ;
	pSet = ri.Element() ;
	for (z = n = 0 ; n < pSet->popl ; n++)
	{
		xval = (m_vals[pSet->start+n]-m_minX)/m_ratX ;
		x = xval ;
		if (!n)
			Z.Printf("var dset%d=[%d", z, x) ;
		else
			Z.Printf(",%d", x) ;
		m_xpad[n] += x ;
	}
	Z << "];\n" ;

	for (ri++ ; ri.Valid() ; ri++)
	{
		pSet = ri.Element() ;
		for (z++, n = 0 ; n < pSet->popl ; n++)
		{
			xval = (m_vals[pSet->start+n]-m_minX)/m_ratX ;
			x = xval ;
			if (!n)
				Z.Printf("var dset%d=[%d", z, x) ;
			else
				Z.Printf(",%d", (uint32_t) xval) ;
			m_xpad[n] += x ;
		}
		Z << "];\n" ;
	}

	//	Start draw instrutions
	Z.Printf("ctx.fillStyle=\"#%06x\";\n", m_FgColor) ;

	//	Draw header and footer if applicable
	DrawTexts(m_Texts, Z, currfont) ;
		
	//	Draw out x and y axis and markers
	Z << "ctx.textAlign=\"left\";\n" ;
	x = 30 ;
	Z.Printf("ctx.fillText(\"%s\",%d,%d);\n", *m_HdrX, m_marginLft, x) ;
	Z.Printf("var y=%d;\n", m_marginLft) ;
	Z.Printf("var x=%d;\n", m_xBot + 15) ;
	Z.Printf("var bot=%d;\n", m_xBot) ;
	Z.Printf("var wid=%d;\n", m_slotw-m_gap) ;
	Z.Printf("var x=%d;\n", m_xBot + 15) ;
	Z << "var i=0;\n" ;
	Z << "for (i=0; i<xav.length; i++) { ctx.fillText(xav[i],y,xap[i]); }\n" ;

	//	Draw x and then y axis lines
	Z.Printf("ctx.moveTo(%d,%d);\n", m_yLft, m_xTop) ;
	Z.Printf("ctx.lineTo(%d,%d);\n", m_yLft, m_xBot) ;
	Z.Printf("ctx.lineTo(%d,%d);\n", m_yRht, m_xBot) ;

	Z << "for (i=0; i<yav.length; i++) { ctx.fillText(yav[i],yap[i],x); }\n" ;
	
	//	Plot values: Note that in a bar chart, the y-axis values when translated into pixels, correspond to the right hand side of the bar. The fillRect method
	//	is based on the top left coordinate plus a width and a height. 

	ri = m_Sets ;
	pSet = ri.Element() ;
	Z.Printf("ctx.fillStyle=\"#%06x\";\n", pSet->color) ;
	Z << "for (i=0; i<yap.length; i++) { ctx.fillRect(yap[i],bot-dset0[i],wid,dset0[i]); }\n" ;

	for (z = 1, ri++ ; ri.Valid() ; z++, ri++)
	{
		pSet = ri.Element() ;
		Z.Printf("ctx.fillStyle=\"#%06x\";\n", pSet->color) ;
		Z.Printf("for (i=0; i<yap.length; i++) { dset0[i]+=dset%d[i]; ctx.fillRect(yap[i], bot-dset0[i],wid,dset%d[i]); }\n", z, z) ;
	}

	//	Do the component index if applicable
	if (m_Sets.Count() > 1)
	{
		Z << "//  Index\n" ;
		y = m_yLft ;
		x = m_xBot + 30 ;
		for (ri = m_Sets ; ri.Valid() ; ri++)
		{
			pSet = ri.Element() ;
			Z.Printf("ctx.fillStyle=\"#%06x\"; ", pSet->color) ;
			Z.Printf("ctx.fillRect(%d,%d,%d,%d);\n", y, x-12, 12, 12) ;
			Z.Printf("ctx.fillStyle=\"#%06x\"; ", m_FgColor) ;
			Z.Printf("ctx.fillText(\"%s\",%d,%d);\n", *pSet->header, y + 15, x) ;
			y += 7 * (pSet->header.Length()) ;
			y += 15 ;
		}
	}
	Z += "ctx.stroke();\n" ;	//	Draw it
	Z << "</script>\n" ;
}

void	hdsStdChart::Generate	(hzChain& Z, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Category:	HTML Generation
	//
	//	Displays a chart by placing in the page HTML, a <canvas> tag. This has to refer to a JavaScript which must appear earlier in the HTML
	//
	//	Arguments:	1)	Z		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsStdChart::Generate") ;

	hzList<_rset*>::Iter	ri ;	//	Dataset iterator

	_rset*		pSet ;		//	Dataset
	double		yval ;		//	For ploting y
	double		xval ;		//	For ploting x
	hzString	currfont ;	//	Current font
	uint32_t	div ;		//	Value divider
	uint32_t	slotw ;		//	Value divider
	uint32_t	pxrX ;		//	Pixel ratio x
	uint32_t	pxrY ;		//	Pixel ratio y
	uint32_t	x ;			//	x-coord
	uint32_t	z ;			//	x-size
	uint32_t	y ;			//	y-coord
	uint32_t	n ;			//	Loop iterator

	Z.Printf("<canvas id=\"%s\" height=\"%d\" width=\"%d\" style=\"border:1px solid #000000; background:#%06x;\">",
		*m_Id, m_Height, m_Width, m_BgColor) ;
	Z << "Your Browser does not support the HTML5 canvas tag</canvas>\n" ;
	Z << "<script>\n" ;
	Z.Printf("var cvs = document.getElementById(\"%s\");\n", *m_Id) ;
	Z << "var ctx = cvs.getContext(\"2d\");\n" ;

	//	Build y-axis values and positions
	yval = (m_maxY - m_minY)/m_ydiv ;
	div = yval ;
	div++ ;
	slotw = m_ypix/div ;

	Z << "var yav=[" ;
	for (y = m_yLft, yval = m_minY ; yval <= m_maxY ; yval += m_ydiv)
	{
		if (yval < m_maxY)
			Z.Printf("\"%d\",", (uint32_t) yval) ;
		else
			Z.Printf("\"%d\"];\n", (uint32_t) yval) ;
		y += slotw ;
	}

	Z << "var yap=[" ;
	for (y = m_yLft, yval = m_minY ; yval <= m_maxY ; yval += m_ydiv)
	{
		if (yval < m_maxY)
			Z.Printf("%d,", y) ;
		else
			Z.Printf("%d];\n", y) ;
		y += slotw ;
	}

	//	Build x-axis values and positions
	xval = (m_maxX - m_minX)/m_xdiv ;
	div = xval ;
	pxrX = m_xpix/div ;

	Z << "var xav=[" ;
	for (x = m_xTop, xval = m_maxX ; xval >= m_minX ; xval -= m_xdiv)
	{
		if (xval > m_minX)
			Z.Printf("\"%d\",", (uint32_t) xval) ;
		else
			Z.Printf("\"%d\"];\n", (uint32_t) xval) ;
		x += pxrX ;
	}

	Z << "var xap=[" ;
	for (x = m_xTop, xval = m_maxX ; xval >= m_minX ; xval -= m_xdiv)
	{
		if (xval > m_minX)
			Z.Printf("%d,", x) ;
		else
			Z.Printf("%d];\n", x) ;
		x += pxrX ;
	}

	//	Set out colors
	ri = m_Sets ;
	pSet = ri.Element() ;
	Z.Printf("var colors=[\"#%06x\"", pSet->color) ;
	for (ri++ ; ri.Valid() ; ri++)
	{
		pSet = ri.Element() ;
		Z.Printf(",\"#%06x\"", pSet->color) ;
	}
	Z << "];\n" ;

	//	Set out headers
	ri = m_Sets ;
	pSet = ri.Element() ;
	Z.Printf("var titles=[\"%s\"", *pSet->header) ;
	for (ri++ ; ri.Valid() ; ri++)
	{
		pSet = ri.Element() ;
		Z.Printf(",\"%s\"", *pSet->header) ;
	}
	Z << "];\n" ;

	//	Set out datasets
	for (z = 0, ri = m_Sets ; ri.Valid() ; z++, ri++)
	{
		pSet = ri.Element() ;

		for (n = 0 ; n < pSet->popl ; n += 2)
		{
			yval = (m_vals[pSet->start+n]-m_minY)/m_ratY ;
			y = yval ;

			if (n == 0)
				Z.Printf("var dsY%d=[%d", z, y) ;
			else
				Z.Printf(",%d", y) ;
		}
		Z += "];\n" ;
	}

	for (z = 0, ri = m_Sets ; ri.Valid() ; z++, ri++)
	{
		pSet = ri.Element() ;

		for (n = 1 ; n < pSet->popl ; n += 2)
		{
			xval = (m_vals[pSet->start+n]-m_minX)/m_ratX ;
			x = xval ;

			if (n == 1)
				Z.Printf("var dsX%d=[%d", z, x) ;
			else
				Z.Printf(",%d", x) ;
		}
		Z += "];\n" ;
	}

	//	Start draw instrutions
	Z.Printf("ctx.fillStyle=\"#%06x\";\n", m_FgColor) ;

	//	Draw header and footer if applicable
	DrawTexts(m_Texts, Z, currfont) ;

	if (currfont != m_Font)
		{ currfont = m_Font ; Z.Printf("ctx.font=\"%s\";\n", *m_Font) ; }
		
	//	Draw out x and y axis and markers
	Z << "ctx.textAlign=\"left\";\n" ;
	x = 30 ;
	y = 20 ;
	Z.Printf("ctx.fillText(\"%s\",%d,%d);\n", *m_HdrX, y, x) ;

	xval = (m_maxX - m_minX)/m_xdiv ;
	div = xval ;
	//div++ ;
	pxrX = m_xpix/div ;

	for (x = m_xTop, xval = m_maxX ; xval >= m_minX ; xval -= m_xdiv)
	{
		Z.Printf("ctx.fillText(\"%d\",%d,%d);\n", (uint32_t) xval, y, x) ;
		x += pxrX ;
	}

	Z << "ctx.beginPath();\n" ;
	Z.Printf("ctx.strokeStyle=\"#%06x\";\n", m_FgColor) ;
	Z.Printf("ctx.moveTo(%d,%d);\n", m_yLft, m_xTop) ;
	Z.Printf("ctx.lineTo(%d,%d);\n", m_yLft, m_xBot) ;
	Z.Printf("ctx.lineTo(%d,%d);\n", m_yRht, m_xBot) ;
	Z += "ctx.stroke();\n" ;
	
	yval = (m_maxY - m_minY)/m_ydiv ;
	div = yval ;
	div++ ;
	pxrY = m_ypix/div ;

	for (y = m_yLft, x = m_xBot + 15, yval = m_minY ; yval <= m_maxY ; yval += m_ydiv)
	{
		Z.Printf("ctx.fillText(\"%d\",%d,%d);\n", (uint32_t) yval, y, x) ;
		y += pxrY ;
	}

	//	Plot values
	Z << "//  Line chart\n" ;
	for (ri = m_Sets ; ri.Valid() ; ri++)
	{
		pSet = ri.Element() ;
		Z.Printf("ctx.strokeStyle=\"#%06x\";\n", pSet->color) ;

		yval = (m_vals[pSet->start]-m_minY)/m_ratY ;
		xval = (m_vals[pSet->start+1]-m_minX)/m_ratX ;
		y = yval ;
		x = xval ;
		Z << "ctx.beginPath();\n" ;
		Z.Printf("ctx.moveTo(%d,%d);\n", m_yLft+y, m_xBot-x) ;

		for (n = 2 ; n < pSet->popl ; n += 2)
		{
			yval = (m_vals[pSet->start+n]-m_minY)/m_ratY ;
			xval = (m_vals[pSet->start+n+1]-m_minX)/m_ratX ;
			y = yval ;
			x = xval ;
			Z.Printf("ctx.lineTo(%d,%d);\n", m_yLft+y, m_xBot-x) ;
		}
		Z += "ctx.stroke();\n" ;	//	Draw it
	}

	//	Do the component index if applicable
	if (m_Sets.Count() > 1)
	{
		Z << "//  Index\n" ;
		y = m_yLft ;
		x = m_xBot + 30 ;
		for (ri = m_Sets ; ri.Valid() ; ri++)
		{
			pSet = ri.Element() ;
			Z.Printf("ctx.fillStyle=\"#%06x\";\n", pSet->color) ;
			Z.Printf("ctx.fillRect(%d,%d,%d,%d);\n", y, x-12, 12, 12) ;
			Z.Printf("ctx.fillStyle=\"#%06x\";\n", m_FgColor) ;
			Z.Printf("ctx.fillText(\"%s\",%d,%d);\n", *pSet->header, y + 15, x) ;
			y += 7 * (pSet->header.Length()) ;
			y += 15 ;
		}
	}

	Z += "ctx.stroke();\n" ;	//	Draw it
	Z << "</script>\n" ;
}

void	hdsDiagram::Generate	(hzChain& Z, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Displays a diagram by placing in the page HTML, a <canvas> tag. This has to refer to a JavaScript which must appear earlier in the HTML
	//
	//	Arguments:	1)	Z		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsDiagram::Generate") ;

	hdsText			TX ;					//	Current text
	hdsLine			LN ;					//	Current line
	hdsGraphic*		pG ;					//	Current shape
	hzString		currfont ;				//	Current font
	uint32_t		linewidth = 1 ;			//	Current line width
	uint32_t		linecolor = 0 ;			//	Current line color
	uint32_t		fillcolor = 0xffffff ;	//	Current line color
	uint32_t		n ;						//	Shapes iterator
	uint32_t		alignCode = 0 ;			//	Current align code (0=left, 1=center, 2=right)
	bool			bFill ;					//	Where shapes are built within paths

	Z.Printf("<canvas id=\"%s\" height=\"%d\" width=\"%d\" style=\"border:1px solid #000000; background:#%06x;\">",
		*m_Id, m_Height, m_Width, m_BgColor) ;
	Z << "Your Browser does not support the HTML5 canvas tag</canvas>\n" ;

	Z << "<script>\n" ;
	Z.Printf("var cvs = document.getElementById(\"%s\");\n", *m_Id) ;
	Z << "var ctx = cvs.getContext(\"2d\");\n" ;

	if (currfont != m_Font)
		{ currfont = m_Font ; Z.Printf("ctx.font=\"%s\";\n", *currfont) ; }
	Z += "ctx.beginPath();\n" ;

	for (n = 0 ; n < m_Shapes.Count() ; n++)
	{
		pG = m_Shapes[n] ;
		bFill = false ;

		if (pG->thick != linewidth)
			{ linewidth = pG->thick ; Z.Printf("ctx.lineWidth=\"%d\";\n", linewidth) ; }
		if (pG->linecolor != linecolor)
			{ linecolor = pG->linecolor ; Z.Printf("ctx.strokeStyle=\"#%06x\";\n", linecolor) ; }

		switch	(pG->type)
		{
		case HZSHAPE_LINE:			//	Line from y1,x1 to y2,x2
			break ;

		case HZSHAPE_DIAMOND:		//	Comprises four lines where bot-top = rht-lft
			Z.Printf("ctx.moveTo(%d,%d);\n", pG->northY, pG->northX) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->eastY, pG->eastX) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->southY, pG->southX) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->westY, pG->westX) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->northY, pG->northX) ;
			bFill = true ;
			break ;

		case HZSHAPE_ARROW:			//	connecting line plus arrow head
			//	Z.Printf("ctx.moveTo(%d,%d);\n", pG->lft, pG->top) ;
			//	Z.Printf("ctx.lineTo(%d,%d);\n", pG->rht, pG->bot) ;
			//	Z.Printf("ctx.lineTo(%d,%d);\n", pG->rht, (pG->bot - pG->top)/2) ;
			//	Z.Printf("ctx.lineTo(%d,%d);\n", pG->lft, pG->top) ;
			break ;

		case HZSHAPE_RECT:			//	Draw a straight forward rectangle (y,x,width,height)
			Z.Printf("ctx.rect(%d,%d,%d,%d);\n", pG->lft, pG->top, pG->rht - pG->lft, pG->bot - pG->top) ;
			break ;

		case HZSHAPE_RRECT:			//	Draw a rounded rectangle.
			Z.Printf("ctx.moveTo(%d,%d);\n", pG->lft + pG->rad, pG->top) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->rht - pG->rad, pG->top) ;
			Z.Printf("ctx.arcTo(%d,%d,%d,%d,Math.PI*0.5);\n", pG->rht, pG->top, pG->rht, pG->top + pG->rad) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->rht, pG->bot - pG->rad) ;
			Z.Printf("ctx.arcTo(%d,%d,%d,%d,Math.PI*0.5);\n", pG->rht, pG->bot, pG->rht - pG->rad, pG->bot) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->lft + pG->rad, pG->bot) ;
			Z.Printf("ctx.arcTo(%d,%d,%d,%d,Math.PI*0.5);\n", pG->lft, pG->bot, pG->lft, pG->bot - pG->rad) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->lft, pG->top + pG->rad) ;
			Z.Printf("ctx.arcTo(%d,%d,%d,%d,Math.PI*0.5);\n", pG->top, pG->lft, pG->top + pG->rad, pG->lft) ;
			bFill = true ;
			break ;

		case HZSHAPE_STADIUM:		//	Two semi-circles joined by two paralell lines
			Z.Printf("ctx.moveTo(%d,%d);\n", pG->lft + pG->rad, pG->top) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->rht - pG->rad, pG->top) ;
			Z.Printf("ctx.arc(%d,%d,%d,Math.PI*1.5,Math.PI*0.5);\n", pG->rht - pG->rad, pG->top + pG->rad, pG->rad) ;
			Z.Printf("ctx.lineTo(%d,%d);\n", pG->lft + pG->rad, pG->bot) ;
			Z.Printf("ctx.arc(%d,%d,%d,Math.PI*0.5,Math.PI*1.5);\n", pG->lft + pG->rad, pG->top + pG->rad, pG->rad) ;
			bFill = true ;
			break ;

		case HZSHAPE_CIRCLE:		//	Circle (center y,x) of radius r
			//Z.Printf("ctx.arcTo(%d,%d,%d,%d,%d);\n", rht,top,rht,top+rad,rad) ;		//	Create the top right arc
			break ;

		case HZSHAPE_LGATE_AND:		//	AND gate
			break ;

		case HZSHAPE_LGATE_OR:		//	OR gate
			break ;

		case HZSHAPE_LGATE_NOT:		//	NOT gate (arrow head)
			break ;

		case HZSHAPE_LGATE_NAND:	//	NAND gate
			break ;

		case HZSHAPE_LGATE_NOR:		//	NOR gate
			break ;

		default:
			break ;
		}

		if (bFill)
		{
			Z << "ctx.closePath();\n" ;
			Z << "ctx.stroke();\n" ;
			if (fillcolor != pG->fillcolor)
				{ fillcolor = pG->fillcolor ; Z.Printf("ctx.fillStyle=\"#%06x\";\n", fillcolor) ; }
			Z << "ctx.fill();\n" ;
		}
	}

	for (n = 0 ; n < m_Lines.Count() ; n++)
	{
		LN = m_Lines[n] ;
		Z.Printf("ctx.moveTo(%d,%d); ctx.lineTo(%d,%d);\n", LN.y1, LN.x1, LN.y2, LN.x2) ;
	}

	for (n = 0 ; n < m_Texts.Count() ; n++)
	{
		TX = m_Texts[n] ;

		if (fillcolor != TX.linecolor)
			{ fillcolor = TX.linecolor ; Z.Printf("ctx.fillStyle=\"#%06x\";\n", fillcolor) ; }

		if (alignCode != TX.alignCode)
		{
			alignCode = TX.alignCode ;
			if		(alignCode == 1)	Z << "ctx.textAlign=\"center\";\n" ;
			else if (alignCode == 2)	Z << "ctx.textAlign=\"right\";\n" ;
			else
				Z << "ctx.textAlign=\"left\";\n" ;
		}

		Z.Printf("ctx.fillText(\"%s\",%d,%d);\n", *TX.text, TX.ypos, TX.xpos) ;
	}

	Z += "ctx.stroke();\n" ;
	Z << "</script>\n" ;
}

void	hdsHtag::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Display the inactive HTML tag as part of page
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsHtag::Generate") ;

	hdsVE*		pVE ;		//	For processing subtags
	hdsLang*	pLang ;		//	Applicable language
	hzFixPair	pa ;		//	Tag attribute
	hzString	S ;			//	Temp string
	uint32_t	n ;			//	Tab counter
	int32_t		aLo ;		//	First attribute
	int32_t		aHi ;		//	Last attribute
	int32_t		nA ;		//	Attribute iterator
	hzHtagform	tagForm ;	//	HTML tag type

	//	Write out newline and tabs if the current tag's line is greater than the supplied line
	if (m_Line != nLine)
	{
		C.AddByte(CHAR_NL) ;
		for (n = m_Indent ; n ; n--)
			C.AddByte(CHAR_TAB) ;
		nLine = m_Line ;
	}

	//	Write out the opening tag
	C.AddByte(CHAR_LESS) ;
	C << m_Tag ;
	if (m_nAttrs)
	{
		aLo = m_pApp->m_VE_attrs.First(m_VID) ;

		if (aLo >= 0)
		{
			aHi = m_pApp->m_VE_attrs.Last(m_VID) ;

			for (nA = aLo ; nA <= aHi ; nA++)
			{
				pa = m_pApp->m_VE_attrs.GetObj(nA) ;

				C.AddByte(CHAR_SPACE) ;
				C << *pa.name ;
				C.AddByte(CHAR_EQUAL) ;
				C.AddByte(CHAR_DQUOTE) ;

				if (pE && (m_flagVE & VE_AT_ACTIVE))
					C << m_pApp->ConvertText(*pa.value, pE) ;
				else
					C << *pa.value ;

				C.AddByte(CHAR_DQUOTE) ;
			}
		}
	}
	C.AddByte(CHAR_MORE) ;

	//	Now for each subtag, output first the pretext (part of this tag's content) and then call Display on the subtag
	pLang = (hdsLang*) pE->m_pContextLang ;

	for (pVE = Children() ; pVE ; pVE = pVE->Sibling())
	{
		if (pVE->m_strPretext)
		{
			if (pLang && pVE->m_usiPretext)
			{
				S = pLang->m_LangStrings[pVE->m_usiPretext] ;
			}
			else
				S = pVE->m_strPretext ;

			if (pE && (pVE->m_flagVE & VE_PT_ACTIVE))
				C << m_pApp->ConvertText(S, pE) ;
			else
				C << S ;
		}

		pVE->Generate(C, pE, nLine) ;
	}

	//	Now write out the content
	if (m_strContent)
	{
		if (pLang && m_usiContent)
			S = pLang->m_LangStrings[m_usiContent] ;
		else
			S = m_strContent ;

		if (pE && m_flagVE & VE_CT_ACTIVE)
			C << m_pApp->ConvertText(S, pE) ;
		else
			C << S ;
	}

	//	Now write the antitag
	S = *m_Tag ;
	tagForm = TagLookup(S) ;

	if (tagForm.rule != HTRULE_SINGLE)
	{
		if (nLine != m_Line)
		{
			C.AddByte(CHAR_NL) ;
			for (n = m_Indent ; n ; n--)
				C.AddByte(CHAR_TAB) ;
			nLine = m_Line ;
		}

		C.AddByte(CHAR_LESS) ;
		C.AddByte(CHAR_FWSLASH) ;
		C << m_Tag ;
		C.AddByte(CHAR_MORE) ;
	}
}

void	hdsXtag::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	The <x> tag does not itself generate a HTML tag but instead inserts textual content into the parent tag.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsHtag::Generate") ;

	hdsVE*		pVE ;		//	For processing subtags
	hdsLang*	pLang ;		//	Applicable language
	hzString	S ;			//	Temp string
	uint32_t	n ;			//	Tab counter

	//	Write out newline and tabs if the current tag's line is greater than the supplied line
	if (m_Line != nLine)
	{
		C.AddByte(CHAR_NL) ;
		for (n = m_Indent ; n ; n--)
			C.AddByte(CHAR_TAB) ;
		nLine = m_Line ;
	}

	//	Now for each subtag, output first the pretext (part of this tag's content) and then call Display on the subtag
	pLang = (hdsLang*) pE->m_pContextLang ;

	for (pVE = Children() ; pVE ; pVE = pVE->Sibling())
	{
		if (pVE->m_strPretext)
		{
			if (pLang && pVE->m_usiPretext)
				S = pLang->m_LangStrings[pVE->m_usiPretext] ;
			else
				S = pVE->m_strPretext ;

			if (pE && (pVE->m_flagVE & VE_PT_ACTIVE))
				C << m_pApp->ConvertText(S, pE) ;
			else
				C << S ;
		}

		pVE->Generate(C, pE, nLine) ;
	}

	//	Now write out the content
	if (m_strContent)
	{
		if (pLang && m_usiContent)
			S = pLang->m_LangStrings[m_usiContent] ;
		else
			S = m_strContent ;

		if (pE && m_flagVE & VE_CT_ACTIVE)
			C << m_pApp->ConvertText(S, pE) ;
		else
			C << S ;
	}
}

void	hdsBlock::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Aggregate to the supplied chain (the HTML body), the set of tags found within this hdsBlock instance. This is the functional equivelent of a
	//	server side include.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsBlock::Generate") ;

	hzList<hdsVE*>::Iter	ih ;	//	Visible entity iterator

	hdsVE*		pVE ;		//	Child visible entity
	uint32_t	relLn ;		//	Relative line
	uint32_t	nV ;		//	Visual entity iterator

	m_pApp->m_pLog->Out("%s. BK %p %d %s\n", *_fn, this, m_VID, *m_Refname) ;
	relLn = nLine ;

	for (nV = 0 ; nV < m_VEs.Count() ; nV++)
	{
		pVE = m_VEs[nV] ;
		m_pApp->m_pLog->Out("%s. VE %p %d %s\n", *_fn, pVE, pVE->m_VID, *pVE->m_Tag) ;
		pVE->Generate(C, pE, nLine) ;
	}

	nLine = relLn ;
}

void	hdsXdiv::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	On the condition that the user's access matches that of this <xdiv> tag, aggregate to the supplied chain (the HTML body), all child tags. Do
	//	nothing otherwise.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsXdiv::Generate") ;

	hdsVE*		pVE ;		//	Visible entity
	hdsInfo*	pInfo ;		//	Client session data
	bool		bShow ;		//	Print or not to print

	bShow = false ;

	pInfo = (hdsInfo*) pE->Session() ;

	if (!pInfo)
	{
		if (m_Access == ACCESS_PUBLIC || m_Access == ACCESS_NOBODY)
			bShow = true ;
	}
	else
	{
		if (m_Access == ACCESS_PUBLIC || (m_Access == ACCESS_ADMIN && pInfo->m_Access & ACCESS_ADMIN)
				|| (pInfo->m_Access & ACCESS_MASK) == m_Access)
			bShow = true ;
	}

	if (bShow)
	{
		for (pVE = Children() ; pVE ; pVE = pVE->Sibling())
			pVE->Generate(C, pE, nLine) ;
	}
}

void	hdsCond::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	If the flags are 0, then the variable named in m_Tag must be null for the subtags of <xcond> to be executed. Otherwise the named variable must
	//	exist and be non-null for the subtags to be executed.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				3)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsCond::Generate") ;

	hzFixPair	pa ;		//	Tag attribute
	hdsVE*		pVE ;		//	Subtag pointer
	hdsInfo*	pInfo ;		//	User session if applicable
	hzString	nam ;		//	The variable value, if applicable
	hzString	val ;		//	The variable value, if applicable
	int32_t		aLo ;		//	First attribute
	int32_t		aHi ;		//	Last attribute
	int32_t		nA ;		//	Attribute iterator
	bool		bPassed ;	//	True if condition passed

	bPassed = true ;

	if (pE)
	{
		if (m_nAttrs)
		{
			pInfo = (hdsInfo*) pE->Session() ;
			if (pInfo)
			{
				aLo = m_pApp->m_VE_attrs.First(pVE->m_VID) ;

				if (aLo >= 0)
				{
					aHi = m_pApp->m_VE_attrs.Last(pVE->m_VID) ;

					for (nA = aLo ; nA <= aHi ; nA++)
					{
						pa = m_pApp->m_VE_attrs.GetObj(nA) ;

						if (*pa.value)
							val = m_pApp->ConvertText(*pa.value, pE) ;
						nam = *pa.name ;

						if ((nam == "isnull" && val) || (nam == "exists" && !val))
							{ bPassed = false ; break ; }
					}
				}
			}
		}
	}

	if (bPassed)
	{
		for (pVE = Children() ; pVE ; pVE = pVE->Sibling())
			pVE->Generate(C, pE, nLine) ;
	}
}

void	hdsPage::Head	(hzHttpEvent* pE)
{
	//	Send only the HTTP header for a page to the browser.
	//
	//	Arguments:	1)	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("hdsPage::Head") ;

	ifstream	is ;			//	Read file stream
	hzXDate		d ;				//	Date for header lines
	hzChain		Z ;				//	Response for browser is built here
	const char*	pEnd ;			//	Filename extension and hence type
	hzMimetype	type ;			//	File's HTTP type
	HttpRC		hrc ;			//	HTTP return code
	hzEcode		rc ;			//	Return code from sending function

	//	Establish real filename, either cpFilename or index.htm(l)

	//	Determine the type of file so that the correct header can be
	//	sent to the browser

	pEnd = strrchr(*m_Url, CHAR_PERIOD) ;
	if (pEnd)
		type = Filename2Mimetype(pEnd) ;
	else
		type = HMTYPE_TXT_HTML ;

	//	Send the header

	Z.Clear() ;

	switch	(hrc)
	{
	case HTTPMSG_OK:		Z << "HTTP/1.1 200 OK\r\n" ;		break ;
	case HTTPMSG_NOTFOUND:	Z << "HTTP/1.1 404 Not found\r\n" ;	break ;
	default:
		Z.Printf("HTTP/1.1 %03d\r\n", hrc) ;
	} ;

	d.SysDateTime() ;

	Z.Printf("Date: %s\r\n", *d.Str(FMT_DT_INET)) ;
	Z << "Server: HTTP/1.0 (HadronZoo, Linux)\r\n" ;
	Z.Printf("Last-Modified: %s\r\n", *d.Str(FMT_DT_INET)) ;

	d.altdate(SECOND, 1000) ;
	Z.Printf("Expires: %s\r\n", *d.Str(FMT_DT_INET)) ;

	Z << "Accept-Ranges: bytes\r\n" ;
	if (hrc == HTTPMSG_OK)
		Z.Printf("Content-Length: 0\r\n") ;
	Z << "Content-Type: " << Mimetype2Txt(type) << "\n\n" ;

	pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 0, false) ;
}

void	hdsArtref::Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine)
{
	//	Aggregate to the supplied chain (the HTML body), this article's preformulated value (the tags found within this article)
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//				4)	nLine	Line number tracker (controls NL printing)
	//
	//	Returns:	None

	_hzfunc("hdsArtref::Generate") ;

	hzList<hdsVE*>::Iter	ih ;	//	Html entity iterator

	hdsInfo*		pInfo = 0 ;		//	Session
	hdsLang*		pLang ;			//	Applicable language
	hdsTree*		pAG = 0 ;		//	Article group
	hdsArticle*		pArt = 0 ;		//	The article
	hdsArticleStd*	pArtStd = 0 ;	//	The article as a visible entity container
	hdsArticleCIF*	pArtCIF = 0 ;	//	The article as a C-Interface function
	hzString		S ;				//	Temp string

	if (!this)
		threadLog("%s. No instance\n", *_fn) ;

	//	Get session if one applies
	if (pE)
		pInfo = (hdsInfo*) pE->Session() ;

	//	Lookup the tree of articles. This could be in the hdsApp map m_ArticleGroups or it could be vested with the session
	pAG = m_pApp->m_ArticleGroups[m_Group] ;
	if (!pAG)
	{
		if (pInfo && pInfo->m_pTree)
			if (pInfo->m_pTree->m_Groupname == m_Group)
				pAG = pInfo->m_pTree ;
	}

	if (!pAG)
	{
		C.Printf("SORRY: No such article group (%s)", *m_Group) ;
		return ;
	}

	if (m_Show == 300)
	{
		//	Export tree as script
		C << "\n<script language=\"javascript\">\n" ;
		pAG->ExportDataScript(C) ;
		C.Printf("makeTree('%s');\n", *m_Group) ;
		C << "</script>\n" ;
		return ;
	}

	if (m_Article[0] == CHAR_AT)
	{
		if (!memcmp(*m_Article, "@resarg;/", 9))
		{
			if (pE && pE->m_Resarg)
				pArt = (hdsArticle*) pAG->Item(pE->m_Resarg) ;
			else
			{
				S = *m_Article + 9 ;
				pArt = (hdsArticle*) pAG->Item(S) ;
			}
		}
	}
	else
		pArt = (hdsArticle*) pAG->Item(m_Article) ;

	if (!pArt)
		C.Printf("SORRY: No article with group (%s) and name (%s)", *m_Group, *m_Article) ;
	else
	{
		if (m_Show == 100)
		{
			C << pArt->m_Title ;
		}
		else if (m_Show == 200)
		{
			//	threadLog("%s. LOCATE %s\n", *_fn, *pArt->m_Title) ;
			pArtStd = dynamic_cast<hdsArticleStd*>(pArt) ;
			if (pArtStd)
			{
				//	pLang = (hdsLang*) pE->m_pContextLang ;
				//	C << pLang->m_rawItems[pArtStd->m_USL] ;
				pArtStd->Generate(C, pE) ;
			}
			else
			{
				pArtCIF = dynamic_cast<hdsArticleCIF*>(pArt) ;
				if (pArtCIF && pE)
					pArtCIF->m_pFunc(C, pArtCIF, pE) ;
			}
		}
		else
			C.Printf("FOUND: article with group (%s) and name (%s) but no show directive", *m_Group, pArt->TxtName()) ;
	}
}

void	hdsArticleStd::Generate	(hzChain& C, hzHttpEvent* pE)
{
	//	Generate HTML for an article.
	//
	//	If the applicable article contains active elements and so is itself active, this function will be called to serve the article in response to an AJAX request. Otherwise this
	//	function is only called to create a fixed HTML value for the article during program initialization.
	//
	//	Note this function is never called by hdsPage::Display() since an hdsArticle cannot be a page component.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pE		The HTTP event being responded to
	//
	//	Returns:	None

	_hzfunc("hdsArticleStd::Generate") ;

	hdsVE*		pVE ;			//	Html entity
	uint32_t	nV ;			//	Visual entity iterator
	uint32_t	relLn ;			//	Relative line

	for (nV = 0 ; nV < m_VEs.Count() ; nV++)
	{
		pVE = m_VEs[nV] ; pVE->Generate(C, pE, relLn) ;
	}
}

void	hdsPage::EvalHtml	(hzChain& C, hdsLang* pLang)
{
	//	Create HTML for a page that is inactive and so can be stored. This will be done once per inactive pager per supported language. Because this is part of the initialization
	//	process there will not be an actual HTTP event so a blank one is created. This is necessary because the language is carried by the HTTP event m_pContextLang variable.
	//
	//	Arguments:	1)	C		The HTML output chain
	//				2)	pLang	Current language
	//
	//	Returns:	None

	_hzfunc("hdsPage::EvalHtml") ;

	hzList<hdsVE*>::Iter		ih ;	//	Html entity iterator
	hzList<hdsFormref*>::Iter	iF ;	//	Form iterator

	hzHttpEvent		httpEv ;			//	Artificial HTTP event
	hzChain			X ;					//	For zipping output if applicable
	hdsVE*			pH ;				//	Html entity
	hdsFormref*		pFormref ;			//	Form reference
	hdsFormdef*		pFormdef ;			//	Form definition
	uint32_t		relLn ;				//	Relative line
	uint32_t		nV ;				//	Visual entity iterator
	hzEcode			rc ;				//	Return code

	if (!pLang)
		Fatal("%s. No language supplied\n", *_fn) ;
	httpEv.m_pContextLang = pLang ;

	m_pApp->m_pLog->Out("%s. PAGE %s Script Flags %x\n", *_fn, *m_Title, m_bScriptFlags) ;

	//	Construct the output HTML
	if (!m_Title)
		m_Title = "untitled" ;

	C << "<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n" ;

	C << "<title>" << m_Title << "</title>\n" ;

	if (m_Desc)
		C.Printf("<meta name=\"description\" content=\"%s\"/>\n", *m_Desc) ;
	else
		C.Printf("<meta name=\"description\" content=\"%s\"/>\n", *m_Title) ;

	if (m_Keys)
		C.Printf("<meta name=\"keywords\" content=\"%s\"/>\n", *m_Keys) ;

	C << s_std_metas ;
	C << "<link rel=\"stylesheet\" href=\"" << m_pApp->m_namCSS << "\"/>\n" ;

	if (m_bScriptFlags || m_xForms.Count())
	{
		if (m_bScriptFlags & INC_SCRIPT_RECAPTCHA)
			C << s_Recaptcha ;

		C << "<script language=\"javascript\">\n" ;

		//	If page contains forms then it must include validation javascript in the HTML
		if (m_xForms.Count())
		{
			if (m_bScriptFlags & INC_SCRIPT_CKEMAIL)
				C << _dsmScript_ckEmail ;
			if (m_bScriptFlags & INC_SCRIPT_CKURL)
				C << _dsmScript_ckUrl ;
			if (m_bScriptFlags & INC_SCRIPT_EXISTS)
				C << _dsmScript_ckExists ;

			for (iF = m_xForms ; iF.Valid() ; iF++)
			{
				pFormref = iF.Element() ;
				pFormdef = m_pApp->m_FormDefs[pFormref->m_Formname] ;
				
				C << pFormdef->m_ValJS ;
				//C << pFormref->m_pFormdef->m_ValJS ;
			}

			C << m_validateJS ;
		}

		C << "</script>\n" ;
	}
	C << "</head>\n\n" ;

	//	Construct the <body> tag
	C << "<body" ;
	if (m_CSS)
		C.Printf(" class=\"%s\"", *m_CSS) ;
	else
		C.Printf(" bgcolor=\"#%06x\" marginwidth=\"%d\" marginheight=\"%d\" leftmargin=\"%d\" topmargin=\"%d\"",
			m_BgColor, m_Width, m_Height, m_Width, m_Top) ;

	if (m_Onpage) C.Printf(" onpageshow=\"%s\"", *m_Onpage) ;
	if (m_Onload) C.Printf(" onload=\"%s\"", *m_Onload) ;
	if (m_Resize) C.Printf(" onresize=\"%s\"", *m_Resize) ;
	C << ">\n" ;
	m_pApp->m_pLog->Out("%s. PAGE %s Script Flags %x\n", *_fn, *m_Title, m_bScriptFlags) ;

	//	Now construct the page elements
	relLn = m_Line ;

	for (nV = 0 ; nV < m_VEs.Count() ; nV++)
	{
		pH = m_VEs[nV] ; pH->Generate(C, &httpEv, relLn) ;
	}

	//	End page construction
	C << "</body>\n</html>\n" ;
	m_pApp->m_pLog->Out("%s. PAGE %s Script Flags %x\n", *_fn, *m_Title, m_bScriptFlags) ;
}

void	hdsPage::Display	(hzHttpEvent* pE)
{
	//	Display HTML according to listed entities in the page.
	//
	//	Argument:	pE		The HTTP event being responded to
	//
	//	Returns:	None

	_hzfunc("hdsPage::Display") ;

	hzList<hdsExec*>::Iter		ei ;	//	Page exec commands iterator
	hzList<hdsVE*>::Iter		ih ;	//	Html entity iterator
	hzList<hdsFormref*>::Iter	iF ;	//	Form iterator

	hzChain			C ;					//	For formulating HTML
	hdsExec*		pExec ;				//	Exec function
	hdsVE*			pVE ;				//	Html entity
	hdsFormref*		pFormref ;			//	Form
	hdsFormdef*		pFormdef ;			//	Form definition
	uint32_t		relLn ;				//	Relative line
	uint32_t		nV ;				//	Visual entity iterator
	hzEcode			rc ;				//	Return code

	if (!pE)					Fatal("%s. No HTTP Event\n") ;
	if (!pE->m_pContextLang)	Fatal("%s. No Language\n", *_fn) ;

	m_pApp->m_pLog->Out("%s. PAGE %s Script Flags %x\n", *_fn, *m_Title, m_bScriptFlags) ;

	//	Run executable commands
	for (ei = m_Exec ; ei.Valid() ; ei++)
	{
		pExec = ei.Element() ;
        m_pApp->m_pLog->Out("%s. HAVE EXEC %s\n", *_fn, Exec2Txt(pExec->m_Command)) ;

        rc = pExec->Exec(C, pE) ;
        m_pApp->m_pLog->Out(C) ;
        C.Clear() ;
	}

	//	Construct the output HTML
	if (!m_Title)
		m_Title = "untitled" ;

	C << "<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n" ;

	if (pE && pE->m_Resarg)
		C.Printf("<title>%s-%s</title>\n", *m_Title, *pE->m_Resarg) ;
	else
		C << "<title>" << m_Title << "</title>\n" ;

	if (m_Desc)
		C.Printf("<meta name=\"description\" content=\"%s\"/>\n", *m_Desc) ;
	else
		C.Printf("<meta name=\"description\" content=\"%s\"/>\n", *m_Title) ;
	if (m_Keys)
		C.Printf("<meta name=\"keywords\" content=\"%s\"/>\n", *m_Keys) ;

	C << s_std_metas ;
	C << "<link rel=\"stylesheet\" href=\"" << m_pApp->m_namCSS << "\"/>\n" ;

	if (m_bScriptFlags || m_xForms.Count())
	{
		if (m_bScriptFlags & INC_SCRIPT_RECAPTCHA)
			C << s_Recaptcha ;

		C << "<script language=\"javascript\">\n" ;

		if (m_bScriptFlags & INC_SCRIPT_WINDIM)
			C << _dsmScript_gwp ;

		if (m_bScriptFlags & INC_SCRIPT_NAVTREE)
		{
			C << _dsmScript_tog ;
			C << _dsmScript_loadArticle ;
			C << _dsmScript_navtree ;
		}

		//	If page contains forms then it must include validation javascript in the HTML
		if (m_xForms.Count())
		{
			if (m_bScriptFlags & INC_SCRIPT_CKEMAIL)	C << _dsmScript_ckEmail ;
			if (m_bScriptFlags & INC_SCRIPT_CKURL)		C << _dsmScript_ckUrl ;
			if (m_bScriptFlags & INC_SCRIPT_EXISTS)		C << _dsmScript_ckExists ;

			for (iF = m_xForms ; iF.Valid() ; iF++)
			{
				pFormref = iF.Element() ;
				pFormdef = m_pApp->m_FormDefs[pFormref->m_Formname] ;

				C << pFormdef->m_ValJS ;
			}

			C << m_validateJS ;
		}

		C << "</script>\n" ;
	}
	C << "</head>\n\n" ;

	//	Construct the <body> tag
	C << "<body" ;
	if (m_CSS)
		C.Printf(" class=\"%s\"", *m_CSS) ;
	else
		C.Printf(" bgcolor=\"#%06x\" marginwidth=\"%d\" marginheight=\"%d\" leftmargin=\"%d\" topmargin=\"%d\"",
			m_BgColor, m_Width, m_Height, m_Width, m_Top) ;

	if (m_Onpage) C.Printf(" onpageshow=\"%s\"", *m_Onpage) ;
	if (m_Onload) C.Printf(" onload=\"%s\"", *m_Onload) ;
	if (m_Resize) C.Printf(" onresize=\"%s\"", *m_Resize) ;
	C << ">\n" ;

	//	Now construct the page elements
	relLn = m_Line ;

	for (nV = 0 ; nV < m_VEs.Count() ; nV++)
	{
		pVE = m_VEs[nV] ; pVE->Generate(C, pE, relLn) ;
	}

	//	End page construction
	C << "</body>\n</html>\n" ;

	//	Send out page. Note all generated pages are sent out raw, not zipped
	rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}
