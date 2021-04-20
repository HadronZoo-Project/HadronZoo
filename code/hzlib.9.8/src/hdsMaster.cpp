//
//	File:	hdsMaster.cpp
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

//	Description: Dissemino Web-Masters Management Facility
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

static	const hzString	s_master_head_nav =
	"<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n"
    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n"
    "<meta http-equiv=\"Expires\" content=\"-1\"/>\n"
    "<meta http-equiv=\"Cache-Control\" content=\"no-cache\"/>\n"
    "<meta http-equiv=\"Cache-Control\" content=\"no-store\"/>\n"
    "<meta http-equiv=\"Pragma\" content=\"no-cache\"/>\n"
    "<meta http-equiv=\"Pragma\" content=\"no-store\"/>\n"
	"<style>\n"
	"body			{ margin:0px; background-color:#FFFFFF; }\n"
	".main			{ text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000; }\n"
	".title			{ text-decoration:none; font-family:verdana; font-size:15px; font-weight:normal; color:#000000; }\n"
	".nbar			{ text-decoration:none; font-family:verdana; font-size:10px; font-weight:bold; color:#FFFFFF; cursor:hand; }\n"
	".nfld			{ list-style-type:none; font-family:verdana; font-size:10px; font-weight:bold; color:#000000; cursor:hand; }\n"
	".objtitle		{ height:30px; border:0px; margin:0px; list-style-type:none; background-color:#A0F0AF; overflow-x:none; overflow-y:none; }\n"
	".navtree 		{ height:calc(100% - 88px); border:1px; margin:0px; list-style-type:none; white-space:nowrap; background-color:#E0E0E0; overflow-x:auto; overflow-y:scroll; }\n"
	".objpage		{ height:calc(100% - 118px); border:0px; margin:0px; list-style-type:none; white-space:nowrap; background-color:#A0F0AF; overflow-x:auto; overflow-y:auto; }\n"
	".fixVP			{ height:calc(100% - 118px); width:auto; position:relative; background-color:#FFFFFF; overflow-x:hidden; overflow-y:hidden; table-layout:auto; border-collapse:collapse; }\n"
	".fixVP th		{ text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; padding:2px; }\n"
	".fixVP td		{ text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; padding:2px; }\n"
	".fixVP tbody	{ overflow-y:auto; }\n"
	".fixVP thead	{ background:white; color:#000000; }\n"
	"</style>\n"
	"<script>\n"
	"function mgwp()\n"
	"{\n"
	"	var screenW = window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth;\n"
	"	var	screenH = window.innerHeight || document.documentElement.clientHeight || document.body.clientHeight;\n"
	"	var txt;\n"
	"	var h;\n"
	"	h=screenH-88;\n"
	"	txt=h+\"px\";\n"
	"	var x=document.getElementById(\"navlhs\");			x.style.minHeight=txt;	x.style.maxHeight=txt;\n"
	"	h=screenH-118;\n"
	"	txt=h+\"px\";\n"
	"	var y=document.getElementById(\"articlecontent\");	y.style.minHeight=txt;	y.style.maxHeight=txt;\n"
	"}\n"
	"</script>\n"
	"</head>\n\n"
	"<body onload=\"mgwp()\" onpageshow=\"mgwp()\" onresize=\"mgwp()\">\n\n" ;

	//	".fixVP			{ width:auto; position:fixed; background-color:#FFFFFF; overflow-x:hidden; overflow-y:hidden; table-layout:auto; border-collapse:collapse; }\n"

static	const hzString	s_master_entry_pt = "masterEntryPt" ;
static  const hzString	s_articleTitle = "x-title" ;			//	Name of HTTP header supplied upon request of an article

extern	hzString	_dsmScript_tog ;			//	Navtree toggle script
extern	hzString	_dsmScript_loadArticle ;	//	Navtree load article script
extern	hzString	_dsmScript_navtree ;		//	Navtree script

//static	hdsTree		s_AdmTree ;		//	Nav-tree for admin
hzEcode	_masterAppParam		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterCfgList		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterFldSpecs		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterEnumEdit		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterClassEdit	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterMbrEdit		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterReposEdit	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterFileList		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterUSL			(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterStrGen		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterStrFix		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterEmaddr		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterDomain		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterMemstat		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterBanned		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterVisList		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterResList		(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE) ;
hzEcode	_masterCfgEdit		(hzHttpEvent* pE, const hzString& cname) ;
hzEcode	_masterMainMenu		(hzHttpEvent* pE) ;

/*
**	Init Functions
*/

void	hdsApp::SetupMasterMenu	(void)
{
	//	Setup the webmaster menu as a tree of C-Interface articles.
	//
	//	This defines the master page templates for viewing/managing the data model. This includes the forms and form handlers for creating/editing classes, class
	//	members and repositories.

	_hzfunc("hdsApp::SetupDataModel") ;

	const hdbMember*		pMbr ;		//	Class member
	const hdbDatatype*		pType ;		//	Data type (base class)
	const hdbClass*			pSub ;		//	Sub-class
	const hdbEnum*			pEnum ;		//	Data Enum
	const hdbClass*			pClass ;	//	Class
	const hdbObjRepos*		pRepos ;	//	Repository

	hdsTree*		pAG ;			//	Tree pointer
	hdsArticle*		pRoot ;			//	Tree item
	hdsArticle*		pHead ;			//	Tree heading
	//hdsArticle*		pItem_F ;		//	Tree item "Field Specs"
	hdsArticle*		pItem_E ;		//	Tree item "Enums"
	hdsArticle*		pItem_C ;		//	Tree item "Classes"
	hdsArticle*		pItem_R ;		//	Tree item "Repositories"
	hdsArticle*		pItemCC ;		//	Tree item of current class
	hdsArticleCIF*	pArtCIF ;		//	Current article
	hzString		refname ;		//	Article ref name
	hzString		title ;			//	Article title
	hzString		S ;				//	Temp string
	uint32_t		n ;				//	Loop iterator
	uint32_t		mbrNo ;			//	Member number
	char			namBuf[16] ;	//	Numeric based name

	if (!this)
		hzexit(_fn, 0, E_SEQUENCE, "No application") ;

	//	Set name of hdsApp::m_DataModel to 'master'
	pAG = &m_DataModel ;
	m_DataModel.m_Groupname = "master" ;
    m_DataModel.m_USL.SetGroup(m_ArticleGroups.Count()) ;
    //pAG->m_Hostpage = page ;
    m_ArticleGroups.Insert(m_DataModel.m_Groupname, &m_DataModel) ;

	//	LOAD TREE
	refname = "Master_Main_Menu" ;
	title = m_Appname ;
	pRoot = m_DataModel.AddHead(0, refname, title, true) ;

	//	Add the menu items applicable at the Sphere level
	refname = "SphereLevel" ;
	title = "Sphere Level Resources" ;
	pHead = m_DataModel.AddHead(pRoot, refname, title, true) ;

	refname = "S.1" ;	title = "Blacklisted IPs" ;	pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterBanned, false) ;	
	refname = "S.2" ;	title = "Memory Stats" ;	pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterMemstat, false) ;
	refname = "S.3" ;	title = "Domain names" ;	pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterDomain, false) ;	
	refname = "S.4" ;	title = "Email addresses" ;	pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterEmaddr, false) ;	
	refname = "S.5" ;	title = "Strings (Fixed)" ;	pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterStrFix, false) ;	

	//	Add the resources and stats stuff
	refname = "AppResoures" ;
	title = "Application Resources" ;
	pHead = m_DataModel.AddHead(pRoot, refname, title, true) ;

	refname = "A.1" ;	title = "Application Parameters" ;	pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterAppParam, false) ;
	refname = "A.2" ;	title = "Config Files" ;			pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterCfgList, false) ;
	refname = "A.3" ;	title = "Misc Fixed Files" ;		pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterFileList, false) ;
	refname = "A.4" ;	title = "Pages" ;					pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterResList, false) ;
	refname = "A.5" ;	title = "Article Groups" ;			pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterResList, false) ;
	refname = "A.6" ;	title = "Site Visitors" ;			pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterVisList, false) ;
	refname = "A.7" ;	title = "Strings (Standard)" ;		pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterStrGen, false) ;
	refname = "A.8" ;	title = "USL Assignments" ;			pArtCIF = m_DataModel.AddArticleCIF(pHead, refname, title, &_masterUSL, false) ;

	//	Add the Data Object Model stuff
	refname = "masterDOM" ;
	title = "Data Object Model" ;
	pHead = m_DataModel.AddHead(pRoot, refname, title, true) ;

	//if (m_Fldspecs.Count())			{ refname = "fldspec" ;	title = "Field Specs" ;		pItem_F = m_DataModel.AddHead(pHead, refname, title, true) ; }
	if (m_ADP.CountDataEnum())		{ refname = "enum" ;	title = "Enums" ;			pItem_E = m_DataModel.AddHead(pHead, refname, title, true) ; }
	if (m_ADP.CountDataClass())		{ refname = "class" ;	title = "Classes" ;			pItem_C = m_DataModel.AddHead(pHead, refname, title, true) ; }
	if (m_ADP.CountObjRepos())		{ refname = "repos" ;	title = "Repositories" ;	pItem_R = m_DataModel.AddHead(pHead, refname, title, true) ; }

	//	Field Specs
	//	threadLog("%s %d field specs\n", *_fn, m_Fldspecs.Count()) ;
	//	for (n = 0 ; n < m_Fldspecs.Count() ; n++)
	//	{
	//	}

	//	Enums
	threadLog("%s %d enums\n", *_fn, m_ADP.CountDataEnum()) ;
	for (n = 0 ; n < m_ADP.CountDataEnum() ; n++)
	{
		//pEnum = m_ADP.mapEnums.GetObj(n) ;
		pEnum = m_ADP.GetDataEnum(n) ;
		if (pEnum)
		{
			sprintf(namBuf, "E.%d", n+1) ;
			refname = namBuf ;
			title = pEnum->StrTypename() ;
			pArtCIF = m_DataModel.AddArticleCIF(pItem_E, refname, title, &_masterEnumEdit, false) ;
			pArtCIF->m_flagVE |= VE_ACTIVE ;

		}
	}

	//	Classes
	threadLog("%s %d classes\n", *_fn, m_ADP.CountDataClass()) ;
	for (n = 0 ; n < m_ADP.CountDataClass() ; n++)
	{
		pClass = m_ADP.GetDataClass(n) ;
		if (pClass)
		{
			sprintf(namBuf, "C.%d", pClass->ClassId()) ;
			refname = namBuf ;
			title = pClass->StrTypename() ;
			pItemCC = (hdsArticle*) m_DataModel.AddArticleCIF(pItem_C, refname, title, &_masterClassEdit, false) ;
			pItemCC->m_flagVE |= VE_ACTIVE ;

			for (mbrNo = 0 ; mbrNo < pClass->MbrCount() ; mbrNo++)
			{
				pMbr = pClass->GetMember(mbrNo) ;

				pType = pMbr->Datatype() ;
				pSub = dynamic_cast<const hdbClass*>(pType) ;

				if (pSub)
					sprintf(namBuf, "C%d.S.%d",pClass->ClassId(), mbrNo) ;
				else
					sprintf(namBuf, "C%d.M.%d",pClass->ClassId(), mbrNo) ;

				refname = namBuf ;
				title = pMbr->StrName() ;
				pArtCIF = m_DataModel.AddArticleCIF(pItemCC, refname, title, &_masterMbrEdit, false) ;
				pArtCIF->m_flagVE |= VE_ACTIVE ;
			}
		}
	}

	//	Repositories
	threadLog("%s %d repos\n", *_fn, m_ADP.CountObjRepos()) ;
	for (n = 0 ; n < m_ADP.CountObjRepos() ; n++)
	{
		pRepos = m_ADP.GetObjRepos(n) ;
		if (pRepos)
		{
			sprintf(namBuf, "R.%d", n+1) ;
			refname = namBuf ;
			title = pRepos->Name() ;
			pArtCIF = m_DataModel.AddArticleCIF(pItem_R, refname, title, &_masterReposEdit, false) ;
			//pArt->m_USL.SetArticle(pAG->m_USL, pAG->Count()) ;
			pArtCIF->m_flagVE |= VE_ACTIVE ;
		}
	}

	//	CREATE PAGES and ARTICLES
	threadLog("%s done\n", *_fn) ;
}

/*
**	Webmaster Display Functions
*/

hzEcode	_masterLoginPage	(hzHttpEvent* pE)
{
	//	Display the master account login form
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("hdsApp::_masterLoginPage") ;

	hzChain		C ;			//	Page output
	hdsApp*		pApp ;		//	Application

	pApp = (hdsApp*) pE->m_pContextApp ;
	C << s_master_head_nav ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"	<tr height=\"50\"><td align=\"center\" class=\"title\">Dissemino Master Account Login</td></tr>\n"
	"</table>\n"
	"<form method=\"POST\" action=\"master\">\n"
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
	"	<tr><td height=\"20\">&nbsp;</td></tr>\n"
	"	<tr><td align=\"center\">" << pApp->m_Domain << "</td></tr>\n"
	"	<tr><td height=\"60\">&nbsp;</td></tr>\n"
	"</table>\n"
	"<table width=\"66%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
	"	<tr height=\"25\"><td>Username: </td><td><input type=\"text\" name=\"username_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
	"	<tr height=\"25\"><td>Password: </td><td><input type=\"password\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
	"</table>\n"
	"<table width=\"20%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
	"	<tr><td height=\"200\">&nbsp;</td></tr>\n"
	"	<tr>\n"
	"		<td><input type=\"button\" onclick=\"location.href='/index.html';\" value=\"Abort\"/></td>\n"
	"		<td>&nbsp;</td>\n"
	"		<td align=\"right\"><input type=\"submit\" name=\"x-action\" value=\"Login\"></td>\n"
	"	</tr>\n"
	"</table>\n"
	"</form>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	_masterProcAuth	(hzHttpEvent* pE)
{
	//	Authenticate the master account login. If the supplied username and password is a match, display the master account main menu. If not redisplay the master account login
	//	page.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("hdsApp::_masterProcAuth") ;

	hdsApp*		pApp ;		//	Application
	hdsInfo*	pInfo ;		//	Session
	hzAtom		atom ;		//	For session value
	hzString	unam ;		//	Submitted username
	hzString	pass ;		//	Submitted password
	hzString	unamFld ;	//	Username field
	hzString	passFld ;	//	Password field
	hzSysID		newCookie ;	//	For cookie generation
	hzIpaddr	ipa ;		//	Client IP address
	hzSDate		expires ;	//	For cookie expiry

	pApp = (hdsApp*) pE->m_pContextApp ;

	unamFld = "username_master" ;
	passFld = "password_master" ;

	unam = pE->m_mapStrings[unamFld] ;
	pass = pE->m_mapStrings[passFld] ;

	threadLog("%s. Trying username (%s) password (%s)\n", *_fn, *unam, *pass) ;
	threadLog("%s. Trying username (%s) password (%s)\n", *_fn, *pApp->m_MasterUser, *pApp->m_MasterPass) ;

	if (unam == pApp->m_MasterUser && pass == pApp->m_MasterPass)
	{
		threadLog("MASTER LOGIN SUCCESS!!!\n") ;

		pInfo = (hdsInfo*) pE->Session() ;

		if (!pInfo)
		{
			newCookie = pApp->MakeCookie(pE->ClientIP(), pE->EventNo()) ;
			expires.SysDate() ;
			expires += 365 ;

			pE->SetSessCookie(newCookie) ;

			pInfo = new hdsInfo() ;
			pE->SetSession(pInfo) ;

			pApp->m_pLog->Out("%s: new cookie %016x new info %p\n", *_fn, newCookie, pInfo) ;
			pApp->m_SessCookie.Insert(newCookie, pInfo) ;
		}

		pInfo->m_Access |= ACCESS_ADMIN ;
		atom = pE->Referer() ;
		pInfo->m_Sessvals.Insert(s_master_entry_pt, atom) ;

		return _masterMainMenu(pE) ;
	}

	return _masterLoginPage(pE) ;
}

hzEcode	_masterMainMenu	(hzHttpEvent* pE)
{
	//	Display the master list of config files for the current website. These may be selected for editing.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterMainMenu") ;

	hzChain			C ;			//	Page output
	hdsApp*			pApp ;		//	Application
	//hdsArticle*		pRoot ;		//	Tree item
	//hdsArticle*		pItem_E ;	//	Tree item "Enums"
	hzString		refname ;	//	Reference name of tree item
	hzString		title ;		//	Title of tree item

	pApp = (hdsApp*) pE->m_pContextApp ;

	C << s_master_head_nav ;

	C << "\n<script language=\"javascript\">\n" ;
    C << _dsmScript_tog ;
    C << _dsmScript_loadArticle ;
    C << _dsmScript_navtree ;
	pApp->m_DataModel.ExportDataScript(C) ;
	C << "</script>\n\n" ;

	//	Create HTML table for listing
	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"	<tr height=\"44\"><td align=\"center\" class=\"title\">Webmaster Control Panel</td></tr>\n"
	"</table>\n" ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"<tr>\n"
	"	<td width=\"100%\">\n"
	"	<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
	"	<tr>\n"
	"		<td width=\"20%\" valign=\"top\" class=\"main\">\n"
	"		<div id=\"navlhs\" class=\"navtree\">\n"
	"			<script type=\"text/javascript\">makeTree('master');</script>\n"
	"		</div>\n"
	"		</td>\n"
	"		<td width=\"80%\" valign=\"top\">\n"
	"			<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
	"			<tr><td align=\"center\" class=\"title\"><div id=\"articletitle\" class=\"objtitle\"></div></td></tr>\n"
	"			<tr><td valign=\"top\" class=\"main\"><div id=\"articlecontent\" class=\"objpage\"></div></td></tr>\n"
	"			</table>\n"
	"		</td>\n"
	"	</tr>\n"
	"	</table>\n"
	"	</td>\n"
	"</tr>\n"
	"</table>\n" ;

	C << 
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"	<tr height=\"44\"><td align=\"center\"><input type=\"button\" onclick=\"location.href='/masterLogout';\" value=\"Webmaster Account Exit\"/></td></tr>\n"
	"</table>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	_masterLogout	(hzHttpEvent* pE)
{
	//	Log the user out from the master section. Then use a redirect to take user back to the 'master entry point' - the page they were on before entering the
	//	master section.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterLogout") ;

	hdsApp*		pApp ;		//	Application
	hdsInfo*	pInfo ;		//	Session
	hzAtom		atom ;		//	For session value

	pApp = (hdsApp*) pE->m_pContextApp ;
	pInfo = (hdsInfo*) pE->Session() ;
	atom = pInfo->m_Sessvals[s_master_entry_pt] ;
	pApp->m_SessCookie.Delete(pE->Cookie()) ;
	delete pInfo ;
	pE->SetSession(0) ;

	return pE->Redirect(atom.Str(), 0, false) ;
}

/*
**	C-Interface Article Functions
*/

hzEcode	_masterAppParam	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the application parameters

	_hzfunc(__func__) ;

	hdsApp*			pApp ;		//	Application

	//	Set the article title
	pE->SetHdr(s_articleTitle, pArtCIF->m_Title) ;

	//	Set the article content
	pApp = (hdsApp*) pE->m_pContextApp ;
	C.Clear() ;
	C << "<table>\n" ;
	C.Printf("<tr><td>Name:</td>		<td>%s</td><tr>\n", *pApp->m_Appname) ;
	C.Printf("<tr><td>Docroot:</td>		<td>%s</td><tr>\n", *pApp->m_Docroot) ;
	C.Printf("<tr><td>Configs:</td>		<td>%s</td><tr>\n", *pApp->m_Configdir) ;
	C.Printf("<tr><td>Datadir:</td>		<td>%s</td><tr>\n", *pApp->m_Datadir) ;
	C.Printf("<tr><td>Logroot:</td>		<td>%s</td><tr>\n", *pApp->m_Logroot) ;
	C.Printf("<tr><td>Masterpath:</td>	<td>%s</td><tr>\n", *pApp->m_MasterPath) ;
	C.Printf("<tr><td>Masteruser:</td>	<td>%s</td><tr>\n", *pApp->m_MasterUser) ;
	C.Printf("<tr><td>Masterpass:</td>	<td>%s</td><tr>\n", *pApp->m_MasterPass) ;
	C.Printf("<tr><td>SmtpAddr:</td>	<td>%s</td><tr>\n", *pApp->m_SmtpAddr) ;
	C.Printf("<tr><td>SmtpUser:</td>	<td>%s</td><tr>\n", *pApp->m_SmtpUser) ;
	C.Printf("<tr><td>SmtpPass:</td>	<td>%s</td><tr>\n", *pApp->m_SmtpPass) ;
	C.Printf("<tr><td>UsernameFld:</td>	<td>%s</td><tr>\n", *pApp->m_UsernameFld) ;
	C.Printf("<tr><td>UserpassFld:</td>	<td>%s</td><tr>\n", *pApp->m_UserpassFld) ;
	C.Printf("<tr><td>Domain:</td>		<td>%s</td><tr>\n", *pApp->m_Domain) ;
	C.Printf("<tr><td>Loghits:</td>		<td>%s</td><tr>\n", *pApp->m_AllHits) ;
	C.Printf("<tr><td>Language:</td>	<td>%s</td><tr>\n", *pApp->m_DefaultLang) ;
	C.Printf("<tr><td>Login:</td>		<td>%s</td><tr>\n", pApp->m_OpFlags & DS_APP_SUBSCRIBERS ? "true":"false") ;
	C.Printf("<tr><td>Robot:</td>		<td>%s</td><tr>\n", pApp->m_OpFlags & DS_APP_ROBOT ? "true":"false") ;
	C.Printf("<tr><td>SiteGuide:</td>	<td>%s</td><tr>\n", pApp->m_OpFlags & DS_APP_GUIDE ? "true":"false") ;
	C.Printf("<tr><td>PortSTD:</td>		<td>%d</td><tr>\n", pApp->m_nPortSTD) ;
	C.Printf("<tr><td>PortSSL:</td>		<td>%d</td><tr>\n", pApp->m_nPortSSL) ;
	C << "</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterCfgList	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the master account list of config files for the current website. These may be selected for editing.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("hdsApp::_masterCfgList") ;

	hdsApp*			pApp ;		//	Application
	hzChain			Z ;			//	XML Content of key resource (page/navbar/stylesheet etc)
	hzDirent		de ;		//	Config file info
	hzXDate			date ;		//	File date
	hzString		title ;		//	Article Title
	uint32_t		n ;			//	Loop iterator
	hzEcode			rc ;		//	Return code

	pApp = (hdsApp*) pE->m_pContextApp ;

	//	Set the article title
	C.Clear() ;
	C << "Config File Listing (" << FormalNumber(pApp->m_Configs.Count()) << " items)" ;
	title = C ;
	pE->SetHdr(s_articleTitle, title) ;

	//	Set the article content
	C.Clear() ;
	C <<
	"<table width=\"100%\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"main\">\n"
	"<tr>\n"
	"	<th>Last Modified</th>\n"
	"	<th>Size</th>\n"
	"	<th>Config File Name</th>\n"
	"</tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	for (n = 0 ; n < pApp->m_Configs.Count() ; n++)
	{
		de = pApp->m_Configs.GetObj(n) ;

		date.SetByEpoch(de.Mtime()) ;

		C.Printf("<tr><td>%s</td><td>%s</td><td><a href=\"/masterAction?CfgEdit=%d\">%s</a></td></tr>\n",
			*date.Str(), *FormalNumber(de.Size()), n, *de.Name()) ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterResList	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the master's list of resources for the current website. These may be selected for editing.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterResList") ;

	hdsApp*			pApp ;		//	Application
	hdsResource*	pRes ;		//	Resource pointer
	hzString		title ;		//	Article title
	uint32_t		n ;			//	Loop iterator
	hzEcode			rc ;		//	Return code

	pApp = (hdsApp*) pE->m_pContextApp ;

	//	Set the article title
	C.Clear() ;
	C << "Resource Listing (" << FormalNumber(pApp->m_ResourcesPath.Count()) << " items)" ;
	title = C ;
	pE->SetHdr(s_articleTitle, title) ;

	//	Set the article content
	C.Clear() ;
	C <<
	"<table id=\"theTable\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<tr>\n"
	"	<th>Type</th>\n"
	"	<th>Title</th>\n"
	"	<th>URL</th>\n"
	"	<th>Hits</th>\n"
	"</tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	for (n = 0 ; n < pApp->m_ResourcesPath.Count() ; n++)
	{
		pRes = pApp->m_ResourcesPath.GetObj(n) ;

		C << "<tr><td width=\"100\">" ;

		switch	(pRes->Whatami())
		{
		case DS_RES_PAGE:		C << "PAGE</td>" ;		break ;
		case DS_RES_ARTICLE:	C << "ARTICLE</td>" ;	break ;
		case DS_RES_FILE:		C << "FILE</td>" ;		break ;
		case DS_RES_PAGE_CIF:	C << "PAGE_CIF</td>" ;	break ;
		}

		//	If the resource is a page, give link to page editor
		if (pRes->Whatami() == DS_RES_PAGE)
			C.Printf("<td width=\"240\"><a href='#' onclick=\"loadArticle('/masterAction?editfile=%s');\">%s</a></td><td width=\"320\">%s</td>", *pRes->m_Url, *pRes->m_Title, *pRes->m_Url) ;
		else
			C.Printf("<td width=\"240\">%s</td><td width=\"320\">%s</td>", *pRes->m_Title, *pRes->m_Url) ;
		C.Printf("<td align=\"right\" width=\"140\">%s</td></tr>\n", *FormalNumber(pRes->m_HitCount)) ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterDomain	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the master's list of resources for the current website. These may be selected for editing.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterDomain") ;

	const char*	i ;		//	String value
	hzString	S ;		//	Temp string
	uint32_t	n ;		//	Loop iterator

	//	Set the article title
	C.Clear() ;
	C << "Known Domains (" << FormalNumber(_hzGlobal_FST_Domain->Count()) << " items)" ;
	S = C ;
	pE->SetHdr(s_articleTitle, S) ;

	//	Set the article content
	C <<
	"<table id=\"theTable\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"<tr>\n"
	"	<th>String Number</th>\n"
	"	<th>String Value</th>\n"
	"</tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	for (n = 1 ; n <= _hzGlobal_FST_Domain->Count() ; n++)
	{
		S = FormalNumber(n) ;
		i = _hzGlobal_FST_Domain->GetStr(n) ;

		if (i)
			C.Printf("\t<tr><td align=\"right\">%s</td><td>%s</td></tr>\n", *S, i) ;
		else
			C.Printf("\t<tr><td align=\"right\">%s</td><td>NULL</td></tr>\n", *S) ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterEmaddr	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the master's list of resources for the current website. These may be selected for editing.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterEmaddr") ;

	const char*	i ;		//	String value
	hzString	S ;		//	Temp string
	uint32_t	n ;		//	Loop iterator

	//	Set the article title
	C.Clear() ;
	C << "Known Email Addresses (" << FormalNumber(_hzGlobal_FST_Emaddr->Count()) << " items)" ;
	S = C ;
	pE->SetHdr(s_articleTitle, S) ;

	//	Set the article content
	C <<
	"<table id=\"theTable\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"<tr>\n"
	"	<th>String Number</th>\n"
	"	<th>String Value</th>\n"
	"</tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	for (n = 1 ; n <= _hzGlobal_FST_Emaddr->Count() ; n++)
	{
		S = FormalNumber(n) ;
		i = _hzGlobal_FST_Emaddr->GetStr(n) ;

		if (i)
			C.Printf("\t<tr><td align=\"right\">%s</td><td>%s</td></tr>\n", *S, i) ;
		else
			C.Printf("\t<tr><td align=\"right\">%s</td><td>NULL</td></tr>\n", *S) ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterStrFix	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the master's list of resources for the current website. These may be selected for editing.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterStrList") ;

	const char*	i ;		//	String value
	hzString	S ;		//	Temp string
	uint32_t	n ;		//	Loop iterator

	//	Set the article title
	C.Clear() ;
	C << "Fixed String List (" << FormalNumber(_hzGlobal_StringTable->Count()) << " items)" ;
	S = C ;
	pE->SetHdr(s_articleTitle, S) ;

	//	Set the article content
	C.Clear() ;
	C <<
	"<table id=\"theTable\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"<tr>\n"
	"	<th>String Number</th>\n"
	"	<th>String Value</th>\n"
	"</tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	for (n = 1 ; n <= _hzGlobal_StringTable->Count() ; n++)
	{
		S = FormalNumber(n) ;
		i = _hzGlobal_StringTable->GetStr(n) ;

		if (i)
			C.Printf("\t<tr><td>%s</td><td>%s</td></tr>\n", *S, i) ;
		else
			C.Printf("\t<tr><td>%s</td><td>NULL</td></tr>\n", *S) ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterStrGen	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the current set of hzString instances.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterStrGen") ;

	hzString		title ;		//	Article title

	//	Set the article title
	title = "Dynamic String Allocations" ;
	pE->SetHdr(s_articleTitle, title) ;

	//	Set the article content
	C.Clear() ;
	C <<
	"<table id=\"theTable\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"<tr>\n"
	"	<th>Size</th>\n"
	"	<th>Population</th>\n"
	"	<th>Free</th>\n"
	"	<th>Memory Used</th></tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	ReportStringAllocations(C) ;

	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterVisList	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the master's list of resources for the current website. These may be selected for editing.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterVisList") ;

	hdsApp*			pApp ;		//	Application
	hdsProfile*		pProf ;		//	Visitor profile pointer
	hzString		title ;		//	Article title
	uint32_t		n ;			//	Loop iterator
	hzRecep16		r16 ;		//	Receptacle for IP address text value
	hzEcode			rc ;		//	Return code

	pApp = (hdsApp*) pE->m_pContextApp ;

	//	Set the article title
	C.Clear() ;
	C << "Dissemino Hit List (" << FormalNumber(pApp->m_Visitors.Count()) << " visitors)" ;
	title = C ;
	pE->SetHdr(s_articleTitle, title) ;

	//	Set the article content
	C.Clear() ;
	C <<
	"<table id=\"theTable\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"<tr>\n"
	"	<th>IP addr</th>\n"
	"	<th>Locn</th>\n"
	"	<th>Robot</th>\n"
	"	<th>Favicon</th>\n"
	"	<th>Articles</th>\n"
	"	<th>Pages</th>\n"
	"	<th>Scripts</th>\n"
	"	<th>Images</th>\n"
	"	<th>Special</th>\n"
	"	<th>Fixed</th>\n"
	"	<th>Posts</th>\n"
	"	<th>Fail-G</th>\n"
	"	<th>Fail-P</th>\n"
	"</tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	for (n = 0 ; n < pApp->m_Visitors.Count() ; n++)
	{
		pProf = pApp->m_Visitors.GetObj(n) ;

		C << "<tr align=\"right\">" ;
		C.Printf("<td align=\"left\">%s</td>", pProf->m_addr.Full(r16)) ;
		C.Printf("<td align=\"left\">%s</td>", GetIpLocation(pProf->m_addr)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_robot)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_favicon)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_art)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_page)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_scr)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_img)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_spec)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_fix)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_post)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_G404)) ;
		C.Printf("<td>%s</td>", *FormalNumber(pProf->m_P404)) ;
		C << "</tr>\n" ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterBanned	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display IP blacklist and number of attempts by each banned IP
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterBanned") ;

	hzMapM<uint16_t,hzIpaddr>	mapTmp ;	//	Temp map, in order of number of attempts

	hzIpinfo	ipi ;		//	IP address block note
	hzIpaddr	ipa ;		//	Blacklisted IP
	hzString	title ;		//	Article title
	uint32_t	n ;			//	Loop iterator
	uint32_t	nAtt ;		//	Number of attempts
	hzRecep16	r16 ;		//	Receptacle for IP address text value
	hzEcode		rc ;		//	Return code

	//	Re-order blacklist by number of attempts
	for (n = 0 ; n < _hzGlobal_StatusIP.Count() ; n++)
	{
		ipa = _hzGlobal_StatusIP.GetKey(n) ;
		ipi = _hzGlobal_StatusIP.GetObj(n) ;

		mapTmp.Insert(ipi.m_nSince, ipa) ;
	}

	//	Set the article title
	C.Clear() ;
	C << "Blacklisted IP Addresses (" << FormalNumber(mapTmp.Count()) << " items)" ;
	title = C ;
	pE->SetHdr(s_articleTitle, title) ;

	//	Set the article content
	C.Clear() ;
	C <<
	"<table id=\"theTable\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"<tr>\n"
	"	<th>IP Address</th>\n"
	"	<th>No Attempts</th>\n"
	"</tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	for (n = mapTmp.Count()-1 ; n ; n--)
	{
		nAtt = mapTmp.GetKey(n) ;
		ipa = mapTmp.GetObj(n) ;

		C.Printf("<tr><td>%s</td><td align=\"right\">%s</td></tr>\n", ipa.Full(r16), *FormalNumber(nAtt)) ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterMemstat	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	_hzfunc("_masterMemstat") ;

	//	Set the article title
	pE->SetHdr(s_articleTitle, pArtCIF->m_Title) ;

	//	Set the article content
	C.Clear() ;

	ReportMemoryUsage(C) ;

	//C <<
	//"<center><td align=\"center\"><input type=\"button\" onclick=\"location.href='/masterMainMenu';\" value=\"Main Menu\"/></center>\n"
	//"</body>\n"
	//"</html>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterUSL	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the master's list of resources for the current website. These may be selected for editing.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterVisList") ;

	hdsApp*			pApp ;		//	Application
	hdsUSL			usl ;		//	USL
	hzString		str ;		//	String value
	uint32_t		n ;			//	Loop iterator
	hzEcode			rc ;		//	Return code
	hzRecep32		r32 ;		//	For USL text value

	pApp = (hdsApp*) pE->m_pContextApp ;

	//	Set the article title
	C.Clear() ;
	C << "USL Assignements (" << FormalNumber(pApp->m_pDfltLang->m_LangStrings.Count()) << " items)" ;
	str = C ;
	pE->SetHdr(s_articleTitle, str) ;

	//	Create HTML table for listing
	C.Clear() ;
	C <<
	"<table id=\"theTable\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"<tr>\n"
	"	<th>Order</th>\n"
	"	<th>USL</th>\n"
	"	<th>String Value</th>\n"
	"</tr>\n"
	"</thead>\n"
	"<tbody>\n" ;

	for (n = 0 ; n < pApp->m_pDfltLang->m_LangStrings.Count() ; n++)
	{
		usl = pApp->m_pDfltLang->m_LangStrings.GetKey(n) ;
		str = pApp->m_pDfltLang->m_LangStrings.GetObj(n) ;

		if (str)
			C.Printf("\t<tr><td align=\"right\">%u</td><td>%s</td><td>%s</td></tr>\n", n, usl.Txt(r32), *str) ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterFileList	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	Display the master's list files for the current website that may be selected for editing. These must not be data files (viewable under a separate
	//	master menu item) and nor must the be config files (editable under a separate master menu item), but they can be text files for any other purpose.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterFileList") ;

	hdsApp*			pApp ;		//	Application
	hzChain			Z ;			//	XML Content of key resource (page/navbar/stylesheet etc)
	hzDirent		de ;		//	Directory entry
	hzString		title ;		//	Article title
	hzXDate			date ;		//	File date (Mtime)
	uint32_t		n ;			//	Loop iterator
	hzEcode			rc ;		//	Return code

	pApp = (hdsApp*) pE->m_pContextApp ;

	//	Set the article title
	C.Clear() ;
	C.Printf("Misc Files (%d items)", pApp->m_Misc.Count()) ;
	title = C ;
	pE->SetHdr(s_articleTitle, title) ;

	//	Set the article content
	C.Clear() ;
	C <<
	"<table align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"<tr>\n"
	"	<th width=180>Date</th>\n"
	"	<th width=150>Size</th>\n"
	"	<th width=350>Dir</th>\n"
	"	<th width=350>Filename</th>\n"
	"</tr>\n" 
	"<thead>\n"
	"<tbody>\n" ;

	for (n = 0 ; n < pApp->m_Misc.Count() ; n++)
	{
		de = pApp->m_Misc.GetObj(n) ;

		date.SetByEpoch(de.Mtime()) ;

		C.Printf("<tr><td width=180>%s</td><td width=150>%s</td><td width=350>%s</td><td width=350><a href=\"/masterFileEdit-%d\">%s</a></td></tr>\n",
			*date.Str(), *FormalNumber(de.Size()), *de.Path(), n, *de.Name()) ;
	}
	C << "</tbody>\n</table>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	_masterFileEdit	(hzHttpEvent* pE)
{
	//	Display the master's edit resource page. This essentially comprises a large text box and a submit button. The text box is populated with the XML
	//	for the resource if it exists. Note that the argument is numeric and identifies the object (directory entry) in the m_Misc map.
	//
	//	Argument:	pE	The HTTP event
	//
	//	Returns:	None

	_hzfunc("_masterFileEdit") ;

	ifstream		is ;		//	For reading file in
	hzChain			C ;			//	Page output
	hzChain			Z ;			//	File content
	hzMD5			md5 ;		//	MD5 for the old content
	hzDirent		de ;		//	Directory entry
	hdsApp*			pApp ;		//	Application
	const char*		pArg ;		//	File identifier
	hzString		fpath ;		//	File path of original
	hzString		fnew ;		//	File path plus .new
	uint32_t		posn ;		//	Posn of file in m_Misc
	hzEcode			rc ;		//	Return code

	pApp = (hdsApp*) pE->m_pContextApp ;
	pArg = pE->GetResource() ;
	pArg += 16 ;

	if (pArg[0] && IsPosint(posn, pArg))
	{
		de = pApp->m_Misc.GetObj(posn) ;

		fpath = de.Path() + "/" + de.Name() ;
		fnew = fpath + ".new" ;

		rc = TestFile(*fnew) ;
		if (rc == E_OK)
			fpath = fnew ;

		is.open(*fpath) ;
		if (is.fail())
			Z << "Error: Cannot Open File" << fpath ;
		else
		{
			Z << is ;
			is.close() ;
		}
	}

	//	Set the article title
	C.Clear() ;
	if (!de.Name())
		fpath = "Dissemino Misc Files Editor: Untitled" ;
	else
	{
		C.Printf("Dissemino Misc Files Editor: %s", *fnew) ;
		fpath = C ;
	}
	fpath = C ;
	pE->SetHdr(s_articleTitle, fpath) ;

	//	Set the article content

	C << "<form method=\"POST\" action=\"masterFileEditHdl\">\n" ;
	if (fnew)
		C.Printf("<input type=\"hidden\" name=\"fname\" value=\"%s\"/>\n", *fnew) ;
	else
		C.Printf("Filename: <input type=\"text\" name=\"fname\"/>\n") ;

	C <<
	"<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
	"<tr>\n"
	"\t<td>\n"
	"\t<textarea name=\"pgData\" rows=\"40\" cols=\"200\" class=\"main\">" ;
	if (Z.Size())
		C << Z ;
	C << "</textarea>\n" ;

	C <<
	"\t</td>\n"
	"</tr>\n"
	"</table>\n"
	"<table width=\"20%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
	"<tr>\n"
	"<td align=\"left\"><input type=\"button\" onclick=\"location.href='/masterFileList';\" value=\"Abort Edit\"/></td>\n"
	"<td align=\"rigth\"><input type=\"submit\" name=\"x-action\" value=\"Submit\"></td>\n"
	"</tr>\n"
	"</table>\n"
	"</form>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	_masterFileEditHdl	(hzHttpEvent* pE)
{
	//	This just saves the file

	ofstream	os ;		//	Output file
	hzChain		C ;			//	Page output
	hzChain		Z ;			//	File content
	hzString	val ;		//	The submitted textarea
	hzString	S ;			//	Temp value
	hzString	fnew ;		//	File path plus .new
	hzEcode		rc ;		//	Return code

	_hzfunc("_masterFileEditHdl") ;

	//	Get file name
	S = "fname" ;
	fnew = pE->m_mapStrings[S] ;

	//	Get new file data
	S = "pgData" ;
	val = pE->m_mapStrings[S] ;
	Z = val ;

	//	Write data to file.new
	fnew += ".new" ;
	os.open(*fnew) ;
	if (os.fail())
		threadLog("%s. Open file failed (%s)\n", *_fn, *fnew) ;
	else
	{
		os << Z ;
		os.close() ;
	}

	//	CHECK THIS
	C << "<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n" ;
	if (!fnew)
		C << "\t<tr height=\"50\"><td align=\"center\" class=\"title\">Dissemino Misc Files Editor: Untitled</td></tr>\n" ;
	else
		C.Printf("\t<tr height=\"50\"><td align=\"center\" class=\"title\">Dissemino Misc Files Editor: %s</td></tr>\n", *fnew) ;
	C << "</table>\n" ;

	C << "<form method=\"POST\" action=\"masterFileEditHdl\">\n" ;
	if (fnew)
		C.Printf("<input type=\"hidden\" name=\"fname\" value=\"%s\"/>\n", *fnew) ;
	else
		C.Printf("Filename: <input type=\"text\" name=\"fname\"/>\n") ;

	C <<
	"<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
	"<tr>\n"
	"\t<td>\n"
	"\t<textarea name=\"pgData\" rows=\"40\" cols=\"200\" class=\"main\">" ;
	if (Z.Size())
		C << Z ;
	C << "</textarea>\n" ;

	C <<
	"\t</td>\n"
	"</tr>\n"
	"</table>\n"
	"<table width=\"20%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
	"<tr>\n"
	"<td align=\"left\"><input type=\"button\" onclick=\"location.href='/masterFileList';\" value=\"Back to files\"/></td>\n"
	"<td align=\"rigth\"><input type=\"submit\" name=\"x-action\" value=\"Commit changes\"></td>\n"
	"</tr>\n"
	"</table>\n"
	"</form>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	_masterFldSpecs	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
}

hzEcode	_masterEnumEdit	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	const hdbEnum*	pEnum = 0 ;		//	Data Enum
	hdsApp*			pApp ;			//	Application
	hdsInfo*		pInfo ;			//	Session
	const char*		pRes ;			//	Requested resource
	const char*		i ;				//	Requested resource iterator
	hzString		entName ;		//	Name of enum/class/member/repos
	uint32_t		strNo ;			//	String number
	uint32_t		numId ;			//	Numeric id
	uint32_t		n ;				//	Loop iterator

	//	Enum is obtained by decoding the resoure query component
	pInfo = (hdsInfo*) pE->Session() ;
	if (!pInfo)
	{
		threadLog("%s. Forbiden\n", *_fn) ;
		pE->SendAjaxResult(HTTPMSG_FORBIDDEN, "No Access") ;
		return E_OK ;
	}

	pApp = (hdsApp*) pE->m_pContextApp ;
    pRes = pE->GetResource() ;

	pApp->m_pLog->Out("%s: Called with res = [%s] cif = [%s]\n", *_fn, pRes, *pArtCIF->m_Refname) ;
	

	//	Locate Enum
	i = *pArtCIF->m_Refname ;
	if (IsPosint(numId, i + 2))
		pEnum = pApp->m_ADP.GetDataEnum(numId-1) ;
	if (pEnum)
		entName = pEnum->TxtTypename() ;

	//	Set the article title
	pE->SetHdr(s_articleTitle, pArtCIF->m_Title) ;

	//	Set the article content
	C.Clear() ;
	if (!pEnum)
		C << "<p>No Enum found</p>\n" ;
	else
	{
		C <<
		"<table width=\"100%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
		"<tr>\n"
		"	<th>Name</th>\n"
		"	<th>Value</th>\n"
		"</tr>\n" ;

		for (n = 0 ; n < pEnum->m_Items.Count() ; n++)
		{
			strNo = pEnum->m_Items[n] ;
			i = _hzGlobal_StringTable->Xlate(strNo) ;

			C.Printf("<tr><td>%d</td><td>%s</td></tr>\n", n+1, i) ;
		}
		C << "</table>\n" ;
	}

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterClassEdit	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	const hdbClass*		pClass = 0 ;	//	Class
	const hdbMember*	pMbr = 0 ;		//	Class member
	hdsApp*				pApp ;			//	Application
	hdsInfo*			pInfo ;			//	Session
	const char*			i ;				//	Requested class
	hzString			title ;			//	Name of class
	uint32_t			mbrNo ;			//	Member number
	uint32_t			numId ;			//	Numeric id
	//uint32_t			n ;				//	Loop iterator

	pInfo = (hdsInfo*) pE->Session() ;
	if (!pInfo)
	{
		threadLog("%s. Forbiden\n", *_fn) ;
		pE->SendAjaxResult(HTTPMSG_FORBIDDEN, "No Access") ;
		return E_OK ;
	}

	pApp = (hdsApp*) pE->m_pContextApp ;

	//	Obtain the class in question
	i = *pArtCIF->m_Refname ;
	if (IsPosint(numId, i + 2))
		pClass = pApp->m_ADP.GetDataClass(numId) ;

	//	Set the article title
	C.Clear() ;
	if (pClass)
		C << "Editing Data Class " << pClass->TxtTypename() ;
	else
		C << "Create New Data Class" ;
	title = C ;
	pE->SetHdr(s_articleTitle, title) ;

	//	Set the article content
	if (pClass)
	{
		C.Clear() ;
		C <<
		"<table width=\"100%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
		"<tr>\n"
		"	<th>Posn</th>\n"
		"	<th>Name</th>\n"
		"	<th>Data type</th>\n"
		"	<th>MinP</th>\n"
		"	<th>MaxP</th>\n"
		"	<th>Description</th>\n"
		"</tr>\n" ;

		for (mbrNo = 0 ; mbrNo < pClass->MbrCount() ; mbrNo++)
		{
			pMbr = pClass->GetMember(mbrNo) ;

			C << "<tr>" ;
			C.Printf("<td align=\"right\">%d</td>", mbrNo) ;
			C.Printf("<td><a href=\"#\" onclick=\"loadArticle('/master_dme-M.%d.%d');\">%s</a></td>", pClass->ClassId(), mbrNo, pMbr->TxtName()) ;
			C.Printf("<td>%s</td>", pMbr->Datatype()->TxtTypename()) ;
			C.Printf("<td align=\"right\">%d</td>", pMbr->MinPop()) ;
			C.Printf("<td align=\"right\">%d</td>", pMbr->MaxPop()) ;
			C.Printf("<td>%s</td>", pMbr->TxtDesc()) ;
			C << "</tr>" ;
		}
		C << "</table>\n" ;
	}

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterMbrEdit	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	const hdbClass*		pClass = 0 ;	//	Class
	const hdbMember*	pMbr = 0 ;		//	Class member

	//	Set the article title
	pE->SetHdr(s_articleTitle, pArtCIF->m_Title) ;

	//	Set the article content
	C.Clear() ;
	if (pMbr)
		C.Printf("<tr><td align=\"center\">%s::%s</td></tr>\n", pClass->TxtTypename(), pMbr->TxtName()) ;
	else
		C << "<tr><td align=\"center\">Class Member Editor</td></tr>\n" ;

	C <<
	"<tr><td height=\"60\">&nbsp;</td></tr>\n"
	"</table>\n"
	"<table width=\"66%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

	C <<
	"<tr height=\"25\"><td>Name: </td><td><input type=\"text\"   name=\"username_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
	"<tr height=\"25\"><td>Type: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
	"<tr height=\"25\"><td>Min Value: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
	"<tr height=\"25\"><td>Max Value: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
	"<tr height=\"25\"><td>Compusory: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
	"<tr height=\"25\"><td>Multiple: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterReposEdit	(hzChain& C, hdsArticleCIF* pArtCIF, hzHttpEvent* pE)
{
	//	MUST DO THIS!

	_hzfunc(__func__) ;

	//	Set the article title
	pE->SetHdr(s_articleTitle, pArtCIF->m_Title) ;

	//	Set the article content

	//const hdbObjRepos*	pRepos = 0 ;	//	Repository
}

void	hdsApp::MasterArticle	(hzHttpEvent* pE)
{
	//	Serve an master article. These are articles in that a partial page is served by AJAX but they are an in-built feature of Dissemino rather than defined as
	//	a part of the application configs. At present the only example of AJAX in the master section is the Data Model Editing regime. Here, there is a host page
	//	and a navigation tree listing all data enums, classes and class members, and repositories in the current application data model. An article containing a
	//	form exists for each of the following:-
	//
	//	1)	Addition of a new enum or displaying/editing an existing enum. The enum values are listed in a table with an blank row and an add button for adding
	//		new values. The values have a checkbox for setting them active/inactive and there is a checkbox for setting the enum as a whole to active/inactive.
	//		The URL of the blank article/form for adding a new enum is /master_dme_enum with no further arguments. This will be extended by -enum_name for an
	//		existing enum.
	//
	//	2)	Addition of a new class or displaying/editing an existing class. The class members are listed in a table showing the data type and other parameters.
	//		The table cannot be edited in-situ. The class members, which can themselves be classes, are edited by clicking on the member name. The URL of the
	//		blank article/form for adding a new class is /master_dme_class and for an existing class this will be extended by -N where N is the class number.
	//
	//	3)	Addition of a new class member or the editing of an existing one. This included the ability to set the member ative/inactive. The URL of the blank
	//		is /master_dme_mbr and for an existing member this will be extended by -N where N is the member number. Note it is not necessary to have a class id and
	//		a member id.
	//
	//	4)	Addition of a new respository or the editing of an existing one. This must name a single host class.

	_hzfunc("hdsApp::MasterArticle") ;

	const hdbEnum*		pEnum = 0 ;		//	Data Enum
	const hdbClass*		pClass = 0 ;	//	Class
	const hdbMember*	pMbr = 0 ;		//	Class member
	const hdbObjRepos*	pRepos = 0 ;	//	Repository

	hzChain			Z ;					//	For building partial HTML output
	hdsInfo*		pInfo ;				//	Session
	hzIpConnex*		pConnex ;			//	Client Connection
	const char*		pRes ;				//	Requested resource
	const char*		i ;					//	Requested resource iterator

	hzString		entName ;			//	Name of enum/class/member/repos
	hzString		argA ;				//	1st argument (e.g. lang subdir or article group name)
	hzString		argB ;				//	2nd argument (e.g. article name)

	uint32_t		numId ;				//	Numeric id
	uint32_t		mbrNo ;				//	Member number
	uint32_t		strNo ;				//	String number
	uint32_t		n ;					//	Loop iterator

	pInfo = (hdsInfo*) pE->Session() ;

	if (!pInfo)
	{
		threadLog("%s. Forbiden\n", *_fn) ;
		pE->SendAjaxResult(HTTPMSG_FORBIDDEN, "No Access") ;
		return ;
	}

	pRes = pE->GetResource() ;

	if (memcmp(pRes, "/masterAction", 13))
	{
		//	Not a web-master option so illegal
		threadLog("%s. No such path\n", *_fn) ;
		pE->SendAjaxResult(HTTPMSG_NOTFOUND, "No such path") ;
		return ;
	}

	pConnex = pE->GetConnex() ;

	if (!pE->QueryLen())
	{
		pConnex->m_Track.Printf("%s: Called with no arguments\n", *_fn) ;
		pE->SendAjaxResult(HTTPMSG_NOTFOUND, "No such path") ;
		return ;
	}

	argA = pE->m_mapStrings.GetKey(0) ;
	argB = pE->m_mapStrings.GetObj(0) ;
	pConnex->m_Track.Printf("%s: Called with %s=%s\n", *_fn, *argA, *argB) ;


	if (argA == "editfile")
	{
		_masterCfgEdit(pE, argB) ;
		return ;
	}

	//	This may be a webmaster command unrelated to the data object model
	//	if (!memcmp(pRes + 8, "A.0", 4))	{ _masterCfgList(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.1", 4))	{ _masterResList(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.2", 4))	{ _masterVisList(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.3", 4))	{ _masterDomain(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.4", 4))	{ _masterEmaddr(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.5", 4))	{ _masterStrFix(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.6", 4))	{ _masterStrGen(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.7", 4))	{ _masterBanned(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.8", 4))	{ _masterMemstat(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.9", 4))	{ _masterUSL(pE) ;		return ; }
	//	if (!memcmp(pRes + 8, "A.10", 5))	{ _masterFileList(pE) ;	return ; }
	//	if (!memcmp(pRes + 8, "A.11", 5))	{ _masterFileEdit(pE) ;	return ; }

	if (memcmp(pRes, "/master_dme-", 12))
	{
		threadLog("%s. No such path\n", *_fn) ;
		pE->SendAjaxResult(HTTPMSG_NOTFOUND, "No such path") ;
		return ;
	}

	if (pRes[12] != 'E' && pRes[12] != 'C' && pRes[12] != 'M' && pRes[12] != 'R')
	{
		threadLog("%s. No such path\n", *_fn) ;
		pE->SendAjaxResult(HTTPMSG_NOTFOUND, "No such path") ;
		return ;
	}

	//	DERIVE ENTITY. A is general article (not related to Data Object Model), E is enum, C is class, M is class member, R is repository.
	if (pRes[13] == CHAR_PERIOD)
	{
		switch	(pRes[12])
		{
		case 'E':	if (IsPosint(numId, pRes + 14))
						//pEnum = m_ADP.mapEnums.GetObj(numId) ;
						pEnum = m_ADP.GetDataEnum(numId) ;
					if (pEnum)
						entName = pEnum->TxtTypename() ;
					break ;

		case 'C':	if (IsPosint(numId, pRes + 14))
					{
						for (n = 0 ; n < m_ADP.CountDataClass() ; n++)
						{
							pClass = m_ADP.GetDataClass(n) ;
							if (pClass->ClassId() == numId)
							{
								entName = pClass->TxtTypename() ;
								break ;
							}
						}
						if (!entName)
							pClass = 0 ;
					}

		case 'M':	if (IsPosint(numId, pRes + 14))
					{
						for (n = 0 ; n < m_ADP.CountDataClass() ; n++)
						{
							pClass = m_ADP.GetDataClass(n) ;
							if (pClass->ClassId() == numId)
							{
								entName = pClass->TxtTypename() ;
								break ;
							}
						}
						if (!entName)
							pClass = 0 ;
						//pClass = m_ADP.mapClasses.GetObj(numId-1) ;

						if (pClass)
						{
							for (i = pRes + 14 ; *i && *i != CHAR_PERIOD ; i++) ;
							if (*i == CHAR_PERIOD)
							{
								if (IsPosint(numId, i+1))
									pMbr = pClass->GetMember(numId) ;
							}
						}
					}
					if (pMbr)
						{ entName = pClass->TxtTypename() ; entName += "::" ; entName += pMbr->StrName() ; }

		case 'R':	if (IsPosint(numId, pRes + 14))
						pRepos = m_ADP.GetObjRepos(numId) ;
					if (pRepos)
						entName = pRepos->Name() ;
		}
	}

	//	CREATE HTML
	Z.Printf("<form method=\"POST\" action=\"%s\">\n", pRes) ;
	Z << "<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

	//	Deal with non-data model requests
	switch	(pRes[9])
	{
	case 'E':

		Z <<
		"<tr><td align=\"center\">Enum Editor</td></tr>\n"
		"<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"</table>\n"
		"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

		Z.Printf("<tr height=\"25\"><td>Name: </td><td><input type=\"text\" name=\"username_master\" size=\"40\" maxlen=\"40\" value=\"%s\"/></td></tr>\n", *entName) ;
		Z <<
		"</table>\n"
		"<table width=\"100%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

		for (n = 0 ; n < pEnum->m_Items.Count() ; n++)
		{
			strNo = pEnum->m_Items[n] ;
			i = _hzGlobal_StringTable->Xlate(strNo) ;

			Z.Printf("<tr><td>%d</td><td>%s</td></tr>\n", n+1, i) ;
		}
		break ;

	case 'C':

		Z <<
		"<tr><td align=\"center\">Class Editor</td></tr>\n"
		"<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"</table>\n"
		"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

		Z.Printf("<tr height=\"25\"><td>Name: </td><td><input type=\"text\" name=\"username_master\" size=\"40\" maxlen=\"40\" value=\"%s\"/></td></tr>\n", *entName) ;
		Z <<
		"</table>\n"
		"<table width=\"100%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
		"<tr>\n"
		"	<th>Posn</th>\n"
		"	<th>Name</th>\n"
		"	<th>Data type</th>\n"
		"	<th>MinP</th>\n"
		"	<th>MaxP</th>\n"
		"	<th>Description</th>\n"
		"</tr>\n" ;

		for (mbrNo = 0 ; mbrNo < pClass->MbrCount() ; mbrNo++)
		{
			pMbr = pClass->GetMember(mbrNo) ;

			Z << "<tr>" ;
			Z.Printf("<td align=\"right\">%d</td>", mbrNo) ;
			Z.Printf("<td><a href=\"#\" onclick=\"loadArticle('/master_dme-M.%d.%d');\">%s</a></td>", pClass->ClassId(), mbrNo, pMbr->TxtName()) ;
			Z.Printf("<td>%s</td>", pMbr->Datatype()->TxtTypename()) ;
			Z.Printf("<td align=\"right\">%d</td>", pMbr->MinPop()) ;
			Z.Printf("<td align=\"right\">%d</td>", pMbr->MaxPop()) ;
			Z.Printf("<td>%s</td>", pMbr->TxtDesc()) ;
			Z << "</tr>" ;
		}
		break ;

	case 'M':
	
		if (pMbr)
			Z.Printf("<tr><td align=\"center\">%s::%s</td></tr>\n", pClass->TxtTypename(), pMbr->TxtName()) ;
		else
			Z << "<tr><td align=\"center\">Class Member Editor</td></tr>\n" ;

		Z <<
		"<tr><td height=\"60\">&nbsp;</td></tr>\n"
		"</table>\n"
		"<table width=\"66%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

		Z <<
		"<tr height=\"25\"><td>Name: </td><td><input type=\"text\"   name=\"username_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
		"<tr height=\"25\"><td>Type: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
		"<tr height=\"25\"><td>Min Value: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
		"<tr height=\"25\"><td>Max Value: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
		"<tr height=\"25\"><td>Compusory: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
		"<tr height=\"25\"><td>Multiple: </td><td><input type=\"select\" name=\"password_master\" size=\"40\" maxlen=\"40\"/></td></tr>\n" ;

		break ;

	case 'R':	if (pRes[10] == CHAR_PERIOD)
				{
					if (IsPosint(numId, pRes + 11))
						pRepos = m_ADP.GetObjRepos(numId) ;
				}
				break ;
	}

	Z <<
	"</table>\n"
	"</form>\n" ;

	//	Send result
	if (entName)
		pE->SetHdr("x-title", entName) ;
	else
		pE->SetHdr("x-title", pRes) ;
	pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 0, false) ;
}

#if 0
hzEcode	_masterDataModel	(hzHttpEvent* pE)
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

	_hzfunc("hdsApp::_masterDataModel") ;

	hdsTree			T ;				//	Temp tree for object model display
	hzChain			Z ;				//	XML Content of key resource (page/navbar/stylesheet etc)
	hzChain			C ;				//	Page output
	hzDirent		de ;			//	Config file info
	hdsApp*			pApp ;			//	Application
	hzXDate			date ;			//	File date
	hzString		editorCls ;		//	Class editor base URL
	hzString		editorMbr ;		//	Class member editor base URL
	hzString		S ;				//	Temp string
	hzEcode			rc ;			//	Return code

	//	BUILD HTML
	pApp = (hdsApp*) pE->m_pContextApp ;
	C << s_master_head_std ;

	C << "\n<script language=\"javascript\">\n" ;
    C << _dsmScript_tog ;
    C << _dsmScript_loadArticle ;
    C << _dsmScript_navtree ;
	pApp->m_DataModel.ExportDataScript(C) ;
	C << "</script>\n\n" ;

	//	Create HTML table for listing
	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"	<tr height=\"50\"><td align=\"center\" class=\"title\">Object Model Editor</td></tr>\n"
	"</table>\n" ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
	"<tr>\n"
	"	<td width=\"100%\">\n"
	"	<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
	"	<tr>\n"
	"		<td width=\"20%\" valign=\"top\">\n"
	"		<div id=\"navlhs\" class=\"navtree\">\n"
	"		<script type=\"text/javascript\">makeTree('master');</script>\n"
	"		</div>\n"
	"		</td>\n"
	"		<td width=\"80%\" valign=\"top\">\n"
	"		<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
	"			<tr height=\"44\">	<td align=\"center\" class=\"title\">	<div id=\"articletitle\"></div>						</td></tr>\n"
	"			<tr>				<td valign=\"top\" class=\"main\">		<div id=\"articlecontent\" class=\"objpage\"></div>	</td></tr>\n"
	"		</table>\n"
	"		</td>\n"
	"	</tr>\n"
	"	</table>\n"
	"	</td>\n"
	"</tr>\n"
	"</table>\n" ;

	//	Show buttons
	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"<tr height=\"50\">\n"
	"	<td align=\"center\">\n" ;

	C << "\t\t<input type=\"button\" onclick=\"location.href='/masterMainMenu';\" value=\"Master Account Menu\"/>" ;

	if (pApp->m_CfgEdits.Count())
		C << "&nbsp;<input type=\"button\" onclick=\"location.href='/masterMainMenu';\" value=\"Apply Changes\"/>" ;

	C <<
	"\n\t</td>\n"
	"</tr>\n"
	"</table>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}
#endif

/*
**	CONFIG CHANGES
*/

hzEcode	_masterPageEdit	(hzHttpEvent* pE, const hzString& rname)
{
	//	Display the form for creating a new page or editing an existing page. This essentially comprises a large text box, populated with the XML for the page (empty in the case of
	//	a new page). In addition there will be a 'Save' and a 'Test' button. Save simply saves the XML to a staging file for the page, which will exist until the page is committed.
	//	Test does not test the page but checks the proposed configs are viable. Pages are instead tested in a separate tab of the browser. This will return the same cookie i.e. the
	//	one associated with the webmaster session. The page displayed to a logged in webmaster will be the test version, if there is one.
	//
	//

	_hzfunc("_masterPageEdit") ;
}

hzEcode	_masterCfgEdit	(hzHttpEvent* pE, const hzString& cname)
{
	//	Display the master edit config page. This essentially comprises a large text box and a submit button. The text box is populated with the XML
	//	for the resource if it exists. Note that the argument is numeric and identifies the object (directory entry) in the m_Configs map.
	//
	//	Argument:	pE	pE		The HTTP event
	//				2)	arg		Argument (part beyond URL /masterCfgEdit-)
	//
	//	Returns:	None

	_hzfunc("hdsApp::_masterCfgEdit") ;

	hdsApp*			pApp ;		//	Application
	ifstream		is ;		//	Input stream
	hzDirent		de ;		//	Config file info
	hzMD5			md5 ;		//	MD5 for the old content
	hzAtom			atom ;		//	For session values
	hzChain			C ;			//	HTML chain
	hzChain			Z ;			//	Config file content
	const char*		pArg ;		//	File identifier
	hzString		fpath ;		//	File path of original
	hzString		fnew ;		//	File path plus .new
	uint32_t		posn ;		//	Posn of file in m_Configs
	hzEcode			rc ;		//	Return code

	pApp = (hdsApp*) pE->m_pContextApp ;
	pArg = pE->GetResource() ;
	pArg += 12 ;

	if (pArg[0] && IsPosint(posn, pArg))
	{
		de = pApp->m_Configs.GetObj(posn) ;

		fpath = de.Path() + "/" + de.Name() ;
		fnew = fpath + ".new" ;

		rc = TestFile(*fnew) ;
		if (rc == E_OK)
			fpath = fnew ;
			
		is.open(*fpath) ;

		if (is.fail())
			Z << "Error: Cannot Open File" << fpath ;
		else
		{
			Z << is ;
			is.close() ;
		}
	}

	//	Set the article title
	C.Clear() ;
	C << "Dissemino Config Editor: Page " << fpath ;
	fnew = C ;
	pE->SetHdr(s_articleTitle, fnew) ;

	//	Set the article content
	C.Clear() ;
	C << "<form method=\"POST\" action=\"masterCfgEditHdl1\">\n" ;
	if (fpath)
		C.Printf("<input type=\"hidden\" name=\"fname\" value=\"%s\"/>\n", *fpath) ;
	else
		C.Printf("Filename: <input type=\"text\" name=\"fname\"/>\n") ;

	C << "<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;
	C << "<tr>\n" ;
	C << "<td>\n" ;
	C << "<textarea name=\"pgData\" rows=\"40\" cols=\"200\" class=\"main\">" ;
	if (Z.Size())
		C << Z ;
	C << "</textarea>\n" ;

	C <<
	"\t</td>\n"
	"</tr>\n"
	"</table>\n"
	"<table width=\"20%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
	"<tr>\n"
	"<td align=\"left\"><input type=\"button\" onclick=\"location.href='/masterCfgList';\" value=\"Back to Listings\"/></td>\n"
	"<td align=\"rigth\"><input type=\"submit\" name=\"x-action\" value=\"Save Changes\"></td>\n"
	"</tr>\n"
	"</table>\n"
	"</form>\n" ;

	return pE->SendAjaxResult(HTTPMSG_OK, C) ;
}

hzEcode	_masterCfgEditHdl	(hzHttpEvent* pE)
{
	//	Handle submission to the master 'stage 1' edit config file form.
	//
	//	Begins with producing an MD5 from the text-box content. If this differs from the previous MD5 for 'pathname.new' or just 'pathname' if the .new does not
	//	yet exist, write out the contents to 'pathname.new'
	//
	//	Config editing involves a sequence of events. The submit button in the stage 1 form will cause a HTTP POST event which will be processed by form-handler
	//	_masterCfgEditHdl1(). This takes the text-box content and produces an MD5. If the config file existed beforehand and the MD5 of the text-box content is a
	//	match to that of the original config file, _masterCfgEditHdl1() will redisplay the stage 1 config editor form along with a message "No changes detected".
	//	Where the MD5 is not a match, either because the origional config has been changed or is new, the text-box content is committed to disk and the stage 2
	//	config editor form is displayed. Note that the disk commit will copy the original file if it exists from 'pathname' to 'pathname.X' where X is the last
	//	modified date of the original expressed as YYYYMMDD-hhmmss. The content be it new or changed, is then written to pathname.new.
	//
	//	The stage 2 form contains a test button which causes a HTTP POST event which will be processes by form-handler _masterCfgEditHd2(). This loads the config
	//	as an XML document and parses it using the same set of config read functions used at startup. If either the load or the parse fails, _masterCfgEditHd2()
	//	redisplays the stage 2 form with the error messages. If the load and the parse succeed _masterCfgEditHd2() redisplays the stage 2 form but this time with
	//	an 'adopt change' button. At this point the updated or new resources specified in the updated or new config are quarentined. Pressing the 'adopt change'
	//	button causes another HTTP POST event which will be processed by form-handler _masterCfgEditHd3(). This overwrites the config held at 'pathname' with the
	//	latest 'pathname.new' and moves the resources out of quarentine into mainstream.

	hdsApp*			pApp ;		//	Application
	ofstream		os ;		//	Output stream (to save file)
	hzMD5			md5 ;		//	MD5 for the new content
	hzDirent		de ;		//	Config file info
	hzChain			C ;			//	Page output
	hzChain			Z ;			//	The XML to be tested
	hzDocXml		doc ;		//	The XML document for the resource
	hzString		S ;			//	Arg as string
	hzString		val ;		//	The submitted textarea
	hzString		fnew ;		//	Config file full pathname
	hzEcode			rc ;		//	Return code

	_hzfunc("hdsApp::_masterCfgEditHdl") ;

	pApp = (hdsApp*) pE->m_pContextApp ;

	//	Get file name
	S = "fname" ;
	fnew = pE->m_mapStrings[S] ;

	//	Get new file data
	S = "pgData" ;
	if (pE->m_mapChains.Exists(S))
		Z = pE->m_mapChains[S] ;
	else
	{
		val = pE->m_mapStrings[S] ;
		Z = val ;
	}

	//	Write data to file.new
	fnew += ".new" ;
	os.open(*fnew) ;
	if (os.fail())
		threadLog("%s. Open file failed (%s)\n", *_fn, *fnew) ;
	else
	{
		os << Z ;
		os.close() ;
	}

	if (!pApp->m_CfgEdits.Exists(de.Name()))
	{
		//	We have not sought to edit this file before
		pApp->m_CfgEdits.Insert(de.Name()) ;
	}

	//	Compile HTML output
	//C << s_master_head_std ;

	C << "<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n" ;
	if (!fnew)
		C << "\t<tr height=\"50\"><td align=\"center\" class=\"title\">Dissemino Config Editor: Untitled</td></tr>\n" ;
	else
		C.Printf("\t<tr height=\"50\"><td align=\"center\" class=\"title\">Dissemino Config Editor: Page %s</td></tr>\n", *fnew) ;
	C << "</table>\n" ;

	C << "<form method=\"POST\">\n" ;
	C.Printf("<p>Processing config file %s of %d bytes</p>\n", *fnew, Z.Size()) ;
	C << "<textarea name=\"pgData\" rows=\"36\" cols=\"200\" class=\"main\">" ;
	C << Z ;
	C << "</textarea>\n" ;

	C <<
	"\t</td>\n"
	"</tr>\n"
	"</table>\n"
	"<table width=\"20%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
	"<tr>\n"
	"<td align=\"left\"><input type=\"button\" onclick=\"location.href='/masterCfgList';\" value=\"Back to Listings\"/></td>\n"
	"<td align=\"rigth\"><input type=\"submit\" name=\"x-action\" value=\"Save Changes\"></td>\n"
	"</tr>\n"
	"</table>\n"
	"</form>\n" ;

	/*
	rc = doc.Load(Z) ;
	if (rc != E_OK)
	{
		C << "<p>Config did not load</p>\n" ;
		C << doc.m_Error ;
	}
	else
	{
		pRoot = doc.GetRoot() ;
		if (!pRoot)
		{
			C << "<p>Document has no root</p>\n" ;
			C << m_cfgErr ;
			rc = E_NODATA ;
		}
		else
		{
			rc = _readInclFile(pRoot) ;
			if (rc != E_OK)
			{
				C << "<p>Document did not parse</p>\n" ;
				C << m_cfgErr ;
			}
		}

		C <<
		"<table>\n"
		"<tr>\n"
		"\t<td align=\"center\">\n"
		"\t<td><input type=\"button\" onclick=\"location.href='/masterCfgList';\" value=\"Abort Edit\"/></td>\n"
		"\t<td width=\"20\">&nbsp;</td>\n"
		"\t<input type=\"submit\" formaction=\"masterCfgEditHdl1\" value=\"Submit\">\n" ;
		if (rc == E_OK)
		{
			C <<
			"\t<td width=\"20\">&nbsp;</td>\n"
			"\t<input type=\"submit\" formaction=\"masterCfgEditHdl2\" value=\"Apply\">\n" ;
		}

		C <<
		"\t</td>\n"
		"</tr>\n"
		"</table>\n" ;
	}

	C <<
	"</form>\n"
	"</body>\n"
	"</html>\n" ;
	*/

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	_masterCfgRestart	(hzHttpEvent* pE)
{
	//	Checks for config updates and if any found, seeks to incorporate the changes into the live application.
	//
	//	MD5 differences 

	hzVect<hzString>	Dirs ;		//	All directories in m_Configdir ending in .new
	hzVect<hzString>	Files ;		//	All files in m_Configdir ending in .new

	hdsApp*			pApp ;			//	Application
	hzDirent		de ;			//	Directory entry
	hzDocXml		doc ;			//	The XML document for the resource
	hzXmlNode*		pRoot ;			//	Root node of document
	hzChain			C ;				//	For HTML output
	hzString		S ;
	hzString		fname ;			//	Name of .new file in config dir
	uint32_t		n ;				//	File iterator
	hzEcode			rc = E_OK ;		//	Return code

	pApp = (hdsApp*) pE->m_pContextApp ;
	ListDir(Dirs, Files, pApp->m_Configdir, "*.new") ;

	//	Compile HTML output
	//C << s_master_head_std ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"	<tr height=\"50\"><td align=\"center\" class=\"title\">Dissemino Config Change Acceptor</td></tr>\n"
	"</table>\n" ;

	C.Printf("<p>Re-configuring for %d files in dir %s</p>\n", Files.Count(), *pApp->m_Configdir) ;

	for (n = 0 ; rc == E_OK && n < Files.Count() ; n++)
	{
		S = Files[n] ;
		//de = Files[n] ;
		C.Printf("<p>Re-configuring for file %s</p>\n", *S) ;	//*de.Name()) ;

		if (de.Name().Contains("Main"))
			continue ;

		//fname = m_Configdir + "/" + de.Name() ;
		fname = pApp->m_Configdir + "/" + S ;

		rc = doc.Load(fname) ;
		if (rc != E_OK)
		{
			C.Printf("<p>Config %s did not load</p>\n", *fname) ;
			C << doc.Error() ;
			continue ;
		}

		C.Printf("<p>Loaded Config %s</p>\n", *fname) ;

		pRoot = doc.GetRoot() ;
		if (!pRoot)
		{
			C << "<p>Document has no root</p>\n" ;
			C << pApp->m_cfgErr ;
			//rc = E_NODATA ;
			continue ;
		}

		C.Printf("<p>Parsed Config %s</p>\n", *fname) ;

		/*
 * 			FIX
		rc = pApp->_readInclFile(pRoot) ;
		*/
		if (rc != E_OK)
		{
			C << "<p>Document did not compile</p>\n" ;
			C << pApp->m_cfgErr ;
			continue ;
		}

		C.Printf("<p>Accepted Config %s</p>\n", *fname) ;
	}

	C << "</form>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

#if 0
void	hdsApp::ReportStatus	(hzHttpEvent* pE)
{
	//	Display HTML Memory Usage Report
	//
	//	Arguments:	1)	pE		The HTTP event being responded to
	//
	//	Returns:	None

	hzChain		C ;		//	Working chain for HTML output

	ReportMemoryUsage(C, true) ;
	ReportMutexContention(C, true) ;
	pE->SendAjaxResult(HTTPMSG_OK, C) ;
}
#endif
