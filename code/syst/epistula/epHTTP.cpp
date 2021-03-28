//
//	File:	epHTTP.cpp
//
//	Legal Notice: This file is part of HadronZoo::Epistula, otherwise known as the "Epistula Email Server".
//
//	Copyright 1998, 2021 HadronZoo Project (http://www.hadronzoo.com)
//
//	HadronZoo::Epistula is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation,
//	either version 3 of the License, or any later version.
//
//	HadronZoo::Epistula is distributed in the hope that it will be useful but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//	PURPOSE. See the GNU General Public License for more details.
//
//	HadronZoo::Epistula depends on the HadronZoo C++ Class Library, with which it distributed as part of the HadronZoo Download. The HadronZoo C++ Class Library is free software:
//	you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License,
//	or any later version.
//
//	The HadronZoo C++ Class Library is distributed in the hope that it will be useful but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
//	A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License along with the HadronZoo C++ Class Library and a copy of the GNU General Public License along with
//	HadronZoo::Epistula. If not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <fstream>

using namespace std ;

#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#include "hzCtmpls.h"
#include "hzCodec.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzDocument.h"
#include "hzIpServer.h"
#include "hzHttpServer.h"
#include "hzDNS.h"
#include "hzProcess.h"
#include "hzMailer.h"
#include "hzDissemino.h"

#include "epistula.h"

/*
**	Definitions
*/

/*
**	Variables
*/

extern	hzLogger	slog ;				//	System logs

extern	pthread_mutex_t	g_LockRS ;		//	Lock for relay task schedule
extern	hzIpServer*		g_pTheServer ;	//	The server instance for HTTP

//	Working (temp) maps and sets
extern	hzMapS	<hzString,hdsInfo*>	g_sessCookie ;		//	Mapping of cookie value to cookie
extern	hzMapS	<hzString,hdsInfo*>	g_sessMember ;		//	Mapping of usernames to cookie
extern	hzMapS	<hzString,mqItem*>	g_TheMailQue ;		//	List of outstanding relay tasks
extern	hzSet	<hzEmaddr>			g_setBanned ;		//	Globally banned senders (users may also ban senders)
extern	hzSet	<hzString>			g_setMboxLocks ;	//	Mail box locks (for POP3 deletes)

extern	hzVect<epAccount*>	g_vecAccount ;		//	All epistula ordinary users (by uid)

extern	epAccount*		g_pSuperuser ;				//	Epistula superuser (for epismail)
extern	hzLockS			g_LockMbox ;				//	Locks g_setMboxLocks

//	Ports and limits (set in main)
extern	uint32_t		g_nPortSMTP ;				//	Standard SMTP port (Normally port 25)
extern	uint32_t		g_nPortSMTPS ;				//	Secure SMTP port (Local users only, Normally port 587)
extern	uint32_t		g_nPortPOP3 ;				//	Standard POP3 port (Normally port 110, Epistula defaul 2110, can be any value)
extern	uint32_t		g_nPortPOP3S ;				//	Secure POP3 port (Normally port 995, Epistula uses 2995, can be any value)
extern	uint32_t		g_nMaxConnections ;			//	Number of simultaneous SMTP and POP3 connections
extern	uint32_t		g_nMaxHTTPConnections ;		//	Number of simultaneous HTTP connections

extern	uint32_t		g_nMaxMsgSize ;				//	Max email message size (2 million bytes)
extern	uint32_t		g_nSessIdAuth ;				//	Session ID for AUTH connections
extern	uint32_t		g_nSessIdPop3 ;				//	Session ID for POP3 connections
extern	bool			g_bShutdown ;				//	When set all thread loops must end

//	Fixed messages
static	const char*	msgSkunk = "550 Relaying denied. An email must originate from a registered server associated with the stated sender's domain.\r\n" ;
//	static	const char*	err_int_nosetuser = "-ERR Internal fault, could not set username\r\n" ;
//	static	const char*	err_int_cmdnosupp = "-ERR Internal fault, command not supported\r\n" ;
//	static	const char*	pop3hello = "+OK Epistula POP3 Server Ready\r\n" ;

extern	hdsApp*			theApp ;			//  Dissemino HTML interface
extern	Epistula*		theEP ;				//	The singleton Epistula Instance

extern	hzString	_dsmScript_tog ;
extern	hzString	_dsmScript_loadArticle ;
extern	hzString	_dsmScript_navtree ;

/*
**	HTTP
*/

static	hzString	s_epis_html_head =
	"<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n"
	"<style>\n"
	".stdbody			{ margin:0px; background-color:#FFFFFF; }\n"
	".main				{ text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000; }\n"
	".title				{ text-decoration:none; font-family:verdana; font-size:15px; font-weight:normal; color:#000000; }\n"
	".stdPg				{ height:calc(100% - 100px); border:0px; margin-left:5px; overflow-x:auto; overflow-y:auto; }\n"
	".admStpg			{ height:calc(100% - 110px); width:100%; position:fixed; border:0px; margin:0px; background-color:#FFFFFF; overflow-x:auto; overflow-y:auto; }\n"
	".nbar				{ text-decoration:none; font-family:verdana; font-size:10px; font-weight:bold; color:#FFFFFF; cursor:hand; }\n"
	".nfld				{ list-style-type:none; font-family:verdana; font-size:10px; font-weight:bold; color:#000000; cursor:hand; }\n"
	".navtree			{ height:calc(100% - 100px); border:0px; margin:0px; list-style-type:none; white-space:nowrap; background-color:#E0E0E0; overflow-x:auto; overflow-y:scroll; }\n"
	".objpage			{ height:calc(100% - 144px); border:0px; margin:0px; list-style-type:none; white-space:nowrap; background-color:#A0F0AF; overflow-x:auto; overflow-y:auto; }\n"
	".fixVP				{ width:auto; table-layout:auto; border-collapse:collapse; }\n"
	".fixVP tbody		{ display:block; overflow-y:auto; }\n"
	".fixVP thead		{ background:white; color:#000000; }\n"
	".fixVP thead tr	{ display: block; }\n"
	".fixVP th			{ text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; padding:2px; }\n"
	".fixVP td			{ text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; padding:2px; }\n"
	"</style>\n"
	"<script>\n"
	"var	creenDim_W;\n"
	"var	creenDim_H;\n"
	"function gwp()\n"
	"{\n"
	"	screenDim_W = window.innerWidth;\n"
	"	screenDim_H = window.innerHeight;\n"
	"	var txt;\n"
	"	var h;\n"
	"	h=screenDim_H-100;\n"
	"	txt=h+\"px\";\n"
	"	document.getElementById(\"theTable\").style.height=txt;\n"
	"	document.getElementById(\"theBody\").style.height=txt;\n"
	"	document.getElementById(\"navlhs\").style.height=txt;\n"
	"	h-=44;\n"
	"	txt=h+\"px\";\n"
	"	document.getElementById(\"articlecontent\").style.height=txt;\n"
	"}\n"
	"</script>\n"
	"</head>\n\n"
	"<body class=\"stdbody\" onpageshow=\"admin_gwp();\" onresize=\"admin_gwp();\">\n\n" ;

static	hzString	s_epis_html_tail =
	"</tbody>\n"
	"</table>\n"
	"<table width=\"90%\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"main\">\n"
	"\t<tr><td align=\"center\"><input type=\"button\" onclick=\"window.location='/admMainMenu'\" value=\"Return to Admin Menu\"></td></tr>\n"
	"</table>\n"
	"</body>\n"
	"</html>\n" ;

static	void	_episDisplayHead (hzChain& C, const char** hdrs, const char* title, uint32_t nPop)
{
	_hzfunc(__func__) ;

	uint32_t	n ;		//	Header iterator

	C << s_epis_html_head ;
	C << "<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n" ;
	if (title)
		C.Printf("\t<tr height=\"35\"><td align=\"center\" class=\"title\">%s</td></tr>\n", title) ;
	else
		C << "\t<tr height=\"35\"><td align=\"center\" class=\"title\">Untitled</td></tr>\n" ;

	C.Printf("\t<tr height=\"20\"><td valign=\"top\" align=\"center\" class=\"main\">Listing %u items</td></tr>\n", nPop) ;
	C << "</table>\n\n" ;

	C <<
	"<table id=\"theTable\" width=\"90%\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n"
	"<thead>\n"
	"\t<tr>\n" ;
	for (n = 0 ; *hdrs[n] ; n++)
		C.Printf("\t<th>%s</th>\n", hdrs[n]) ;
	C <<
	"\t</tr>\n"
	"</thead>\n"
	"<tbody id=\"theBody\">\n" ;
}

hzEcode	admStart	(hzHttpEvent* pE)
{
	//	Admin 'home' page

	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output

	C << s_epis_html_head ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"\t<tr height=\"55\"><td align=\"center\" class=\"title\">Epistula Domain/Account Manager</td></tr>\n"
	"</table>\n\n" ;

	C <<
	"<table width=\"90%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"fixVP\">\n"
	"\t<tr height=\"50\"><td>&nbsp;</td></tr>\n"
	"\t<tr height=\"50\"><td><a href=\"/admSlctDomains\">Edit Domains</a></td></tr>\n"
	"\t<tr height=\"50\"><td>&nbsp;</td></tr>\n"
	"\t<tr height=\"50\"><td><a href=\"/admEditAccounts\">Edit Accounts</a></td></tr>\n"
	"\t<tr height=\"50\"><td>&nbsp;</td></tr>\n"
	"</table>\n" ;

	C <<
	"<table width=\"90%\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"main\">\n"
	"\t<tr><td align=\"center\"><input type=\"button\" onclick=\"window.location='/'\" value=\"Admin Logout\"></td></tr>\n"
	"</table>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	admSlctDomains	(hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output
	hzString	tmpStr ;	//	Temp string (for domain names)
	uint32_t	n ;			//	Domain iterator

	C << s_epis_html_head ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"\t<tr height=\"55\"><td align=\"center\" class=\"title\">Epistula Domain Manager</td></tr>\n"
	"</table>\n\n" ;

	C << "<table width=\"90%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"fixVP\">\n" ;
	if (theEP->m_LocalDomains.Count())
	{
		C <<
		"\t<tr><td align=\"center\">Acting as mailserver for the following domains</td><tr>\n"
		"\t<tr height=\"20\"><td>&nbsp;</td></tr>\n" ;

		for (n = 0 ; n < theEP->m_LocalDomains.Count() ; n++)
		{
			tmpStr = theEP->m_LocalDomains.GetObj(n) ;
			C.Printf("\t<tr height=\"18\"><td><a href=\"%s\">%s</td></tr>\n", n, *tmpStr) ;
		}
		C << "</table>\n" ;
	}

	C <<
	"<table width=\"90%\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"main\">\n"
	"\t<tr><td align=\"center\"><input type=\"button\" onclick=\"window.location='/'\" value=\"Admin Logout\"></td></tr>\n"
	"</table>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	admEditDomain	(hzHttpEvent* pE)
{
	//	Edit a single domain

	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output
	hzString	tmpStr ;	//	Temp string (for domain names)
	uint32_t	n ;			//	Domain iterator

	C << s_epis_html_head ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"\t<tr height=\"55\"><td align=\"center\" class=\"title\">Epistula Domain Manager</td></tr>\n"
	"</table>\n\n" ;

	C << "<table width=\"90%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"fixVP\">\n" ;
	if (theEP->m_LocalDomains.Count())
	{
		C <<
		"\t<tr><td align=\"center\">Acting as mailserver for the following domains</td><tr>\n"
		"\t<tr height=\"20\"><td>&nbsp;</td></tr>\n" ;

		for (n = 0 ; n < theEP->m_LocalDomains.Count() ; n++)
		{
			tmpStr = theEP->m_LocalDomains.GetObj(n) ;
			C.Printf("\t<tr height=\"18\"><td><a href=\"%s\">%s</td></tr>\n", n, *tmpStr) ;
		}
		C << "</table>\n" ;
	}

	C <<
	"<table width=\"90%\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"main\">\n"
	"\t<tr><td align=\"center\"><input type=\"button\" onclick=\"window.location='/'\" value=\"Admin Logout\"></td></tr>\n"
	"</table>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	admEditAccounts	(hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output
	hzString	tmpStr ;	//	Temp string (for domain names)
	uint32_t	n ;			//	Domain iterator

	C << s_epis_html_head ;

	C <<
	"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
	"\t<tr height=\"55\"><td align=\"center\" class=\"title\">Epistula Domain Manager</td></tr>\n"
	"</table>\n\n" ;

	C << "<table width=\"90%\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"fixVP\">\n" ;
	if (theEP->m_LocalDomains.Count())
	{
		C <<
		"\t<tr><td align=\"center\">Acting as mailserver for the following domains</td><tr>\n"
		"\t<tr height=\"20\"><td>&nbsp;</td></tr>\n" ;

		for (n = 0 ; n < theEP->m_LocalDomains.Count() ; n++)
		{
			tmpStr = theEP->m_LocalDomains.GetObj(n) ;
			C.Printf("\t<tr height=\"18\"><td><a href=\"%s\">%s</td></tr>\n", n, *tmpStr) ;
		}
		C << "</table>\n" ;
	}

	C <<
	"<table width=\"90%\" align=\"center\" border=\"1\" cellspacing=\"1\" cellpadding=\"1\" class=\"main\">\n"
	"\t<tr><td align=\"center\"><input type=\"button\" onclick=\"window.location='/'\" value=\"Admin Logout\"></td></tr>\n"
	"</table>\n"
	"</body>\n"
	"</html>\n" ;

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	admAddresses	(hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	UsrMsgList	(hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output
	hdsInfo*	pInfo ;		//	HTTP Session
	epAccount*	pAcc ;		//	User account

	pInfo = (hdsInfo*) pE->Session() ;
	C << s_epis_html_head ;

	if (!pInfo)
		pAcc = 0 ;
	else
	{
		//	Get account from info
		pAcc = (epAccount*) pInfo->m_pMisc ;
		if (!pAcc)
		{
			pInfo->m_pMisc = pAcc = theEP->m_Accounts[pInfo->m_Username] ;
			//pInfo->m_pTree = pAcc->m_Folders ;
		}
	}

	if (!pAcc)
	{
		//	Show login

		C <<
		"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
		"\t<tr height=\"25\"><td align=\"center\" class=\"title\">Epistula Login</td></tr>\n"
		"\t<tr height=\"20\"><td align=\"center\" class=\"main\">At server: " << theApp->m_Domain << "</td></tr>\n"
		"</table>\n"
		"<form method=\"POST\" action=\"/\">\n"
		"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
		"\t<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"\t<tr><td align=\"center\">" << theApp->m_Domain << "</td></tr>\n"
		"\t<tr><td height=\"60\">&nbsp;</td></tr>\n"
		"</table>\n"
		"<table width=\"66%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n"
		"\t<tr height=\"25\"><td>Username: </td><td><input type=\"text\" name=\"username\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
		"\t<tr height=\"25\"><td>Password: </td><td><input type=\"userpass\" name=\"userpass\" size=\"40\" maxlen=\"40\"/></td></tr>\n"
		"</table>\n"
		"<table width=\"20%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
		"\t<tr><td height=\"200\">&nbsp;</td></tr>\n"
		"\t<tr>\n"
		"\t\t<td><input type=\"button\" onclick=\"location.href='/index.html';\" value=\"Abort\"/></td>\n"
		"\t\t<td>&nbsp;</td>\n"
		"\t\t<td align=\"right\"><input type=\"submit\" name=\"x-action\" value=\"Login\"></td>\n"
		"\t</tr>\n"
		"</table>\n"
		"</form>\n"
		"</body>\n"
		"</html>\n" ;
	}
	else
	{
		//	Show current folder
		C << "\n<script language=\"javascript\">\n" ;
		C << _dsmScript_tog ;
		C << _dsmScript_loadArticle ;
		C << _dsmScript_navtree ;

		pAcc->m_Folders.ExportDataScript(C) ;
		C << "</script>\n\n" ;
		C <<
		"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
		"	<tr height=\"40\"><td align=\"center\" class=\"title\">HadronZoo::Epistula</td></tr>\n" ;
		C.Printf("<tr height=\"40\"><td align=\"center\" class=\"main\">%s</td></tr>\n", *pAcc->m_Unam) ;
		C << "</table>\n" ;

		C <<
		"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
		"<tr>\n"
		"\t<td width=\"100%\">\n"
		"\t\t<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
		"\t\t<tr>\n"
		"\t\t\t<td width=\"20%\" valign=\"top\" class=\"main\">\n"
		"\t\t\t\t<div id=\"navlhs\" class=\"navtree\">\n"
		"\t\t\t\t\t<script type=\"text/javascript\">makeTree('" << pAcc->m_Unam << "');</script>\n"
		"\t\t\t\t</div>\n"
		"\t\t\t</td>\n"
		"\t\t\t<td width=\"80%\" valign=\"top\">\n"
		"\t\t\t\t<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
		"\t\t\t\t<tr height=\"40\">\n"
		"\t\t\t\t\t<td align=\"center\" class=\"title\"><div id=\"articletitle\"></div></td>\n"
		"\t\t\t\t</tr>\n"
		"\t\t\t\t<tr>\n"
		"\t\t\t\t\t<td valign=\"top\" class=\"main\"><div id=\"articlecontent\" class=\"objpage\"><xtreeCtl group=\"%x:user;\" article=\"Inbox\" show=\"content\"/></div></td>\n"
		"\t\t\t\t</tr>\n"
		"\t\t\t\t</table>\n"
		"\t\t\t</td>\n"
		"\t\t</tr>\n"
		"\t\t</table>\n"
		"\t</td>\n"
		"</tr>\n"
		"</table>\n" ;

		C <<
		"<table width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#b3fff0\">\n"
		"	<tr height=\"40\"><td align=\"center\"><input type=\"button\" onclick=\"location.href='/logout';\" value=\"Quit\"/></td></tr>\n"
		"</table>\n"
		"</body>\n"
		"</html>\n" ;
	}

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	UsrMsgView	(hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	UsrMsgReply	(hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

hzEcode	UsrMsgNew	(hzHttpEvent* pE)
{
	_hzfunc(__func__) ;

	hzChain		C ;			//	Page output

	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, C, 0, false) ;
}

/*
**	Section X:	HTTP Webmail
**
123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
**	Account holders log in to view and reply to incoming messages, originate new messages and perform such tasks as arranging messages in folders. Each account has a single mailbox
**	but may be associated with several local email addresses. The 1:many map g_Locaddr2Orig maps local email addresses to accounts that are permitted to use the address for sending
**	(either as a reply or the origination of a new message).   replying to messages

There is a list of addresses incoming message (header
**	only), is placed in an account holder's mailbox by ProcSMTP() if one or more of the recipient addresses is local and deemed accessible b
**
**
123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
**	With a webmail service the ratio of concurrent sessions to registered users will typically be much higher than with Internet applications generally. In a corporate environment,
**	this ratio can approach unity during office hours. This anticipation has a material impact on server capacity planning and on the Epistula design. Although message bodies   could approach the total number of registered users
it is likely that   for all users to be logged in at the same time. HTTP sessions can be prolific. Unless users actively log out, sessions remain live until they time out. In a corporate environment, many users would hold their accounts open throughout the entire working day, simply by
*/

hzTcpCode   ProcHTTP	(hzHttpEvent* pE)
{
	//	This is needed as a function to pass to the AddPort() function of the HTTP server instance as it is not possible to pass it a non static
	//	class member function directly. The HTTP handler of the hdsApp class is what does all the work.

    hdsApp* 	pApp ;
	hzIpConnex*	pCx ;

	/*
    pApp = _hzGlobal_Dissemino->GetApplication(pE->Hostname()) ;
    if (!pApp)
    {
        threadLog("NO SUCH APP %s\n", pE->Hostname()) ;
        return TCP_TERMINATE ;
    }

    //  NOTE: Add any intercept code here!

	return pApp->ProcHTTP(pE) ;
	*/

	pCx = pE->GetConnex() ;
	slog.Out(pCx->InputZone()) ;

	if (!theApp)
		return TCP_TERMINATE ;
	return theApp->ProcHTTP(pE) ;
}
