//
//	File:	hzHttpClient.h
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

//	The hzHttpClient suite in conjunction with the hzDocument classes, allows applications to act as HTTP clients to external websites. The following
//	classes are provided:-
//
//	1)	hzCookie.		Used to maintain session state.
//	2)	hzHttpClient.	Manages the HTTP client connection to the remote website and provides inter alia, the functions of GetPage() and PostForm().
//	3)	hzWebhost.		A hzWebhost instance is a view of the external website as a whole and is used to manage client sessions with said website.

#ifndef hzHttpClient_h
#define hzHttpClient_h

#include "hzTcpClient.h"
#include "hzTmplList.h"
#include "hzTmplVect.h"
#include "hzTmplSet.h"
#include "hzHttpProto.h"
#include "hzDocument.h"

enum	hzAuthmode
{
	//	Category:	Internet
	//
	//	Authentication methods

	HZ_AUTH_NONE,			//	No authentication regime in place
	HZ_AUTH_BASIC,			//	A base64 password is passed to the server with each HTTP request
	HZ_AUTH_FORM_POST,		//	A form submission of username and password by HTTP POST
	HZ_AUTH_FORM_GET,		//	A form submission of username and password by HTTP GET
} ;

#define COOKIE_HTTPONLY		0x01

struct	hzCookie
{
	//	Category:	Internet
	//
	//	A cookie is a session tracking device set by a server and returned by the client in subsequent HTTP request.

	hzXDate		m_Expires ;		//	If not set the cookie is permanent, otherwise stop using it after this date
	hzString	m_Path ;		//	Send the cookie if the path is equal to or greater than this
	hzString	m_Name ;		//	The cookie name
	hzString	m_Value ;		//	The cookie value
	uint32_t	m_Flags ;		//	Operational flags (eg HttpOnly)
	uint32_t	m_Resv ;		//	Reserved

	hzCookie	(void)
	{
		m_Flags = m_Resv = 0 ;
	}

	void	Clear	(void)
	{
		m_Expires.Clear() ;
		m_Path.Clear() ;
		m_Name.Clear() ;
		m_Value.Clear() ;

		m_Flags = m_Resv = 0 ;
	}

	hzCookie&	operator=	(const hzCookie& op)
	{
		m_Expires = op.m_Expires ;
		m_Path = op.m_Path ;
		m_Name = op.m_Name ;
		m_Value = op.m_Value ;
		m_Flags = op.m_Flags ;

		return *this ;
	}
} ;

class	hzHttpClient
{
	//	Category:	Internet
	//
	//	The hzHttpClient class enables applications to operate as HTTP clients (eg be an auto-browser/webbot). It is a simple class providing
	//	two basic functions GetPage() and PostForm(). Nothing is known or assumed about the pages being requested or the forms being posted.
	//
	//	These two functions both require an map of hzString (strings) to hzCookie (cookies), to be supplied. The HTTP header line 'Set-Cookie' in
	//	the server's response (to either a GET or a POST) will add cookies to this map (assuming they do not yet exist). All cookies in the map,
	//	except those marked 'HttpOnly' will be sent in the header of all GET or POST requests.
	//
	//	The hzString->hzCookie map is maintained by the caller which may be the application or the hzWebhost class.

	hzTcpClient	m_Webhost ;			//	Current connection

	hzEcode	_procHttpResponse	(hzMapS<hzString,hzCookie>& cookies, HttpRC& hRet, const hzUrl& url) ;

	hzEcode	_getpage	(hzMapS<hzString,hzCookie>& cookies, HttpRC& hRet, const hzUrl& url, const hzString& etag) ;
	hzEcode	_postform	(HttpRC& hRet, const hzUrl& url, hzMapS<hzString,hzCookie>& cookies, hzVect<hzString>& headers, const hzChain& formData) ;

public:
	hzChain		m_Error ;				//	Error logging
	hzChain		m_Request ;				//	Chain formed by GetHttpPage() to send a request to a server
	hzChain		m_Header ;				//	Header of server's response
	hzChain		m_Content ;				//	HTML page content of server's response
	hzXDate		m_Accessed ;			//	Date of last access (downloaded). Derived from 'Date:' field in header.
	hzXDate		m_Modified ;			//	Date of last modification (from header). Default will be date/time of download.
	hzXDate		m_Expires ;				//	Date page expires (after which it must be re-loaded)
	uint64_t	m_rtRequest ;			//	Request sent time
	uint64_t	m_rtResponse ;			//	Response complete time
	hzString	m_CacheCtrl ;			//	Cache control
	hzString	m_Host ;				//	Domain name or host currently connected to
	hzString	m_Pragma ;				//	Pragma directive
	hzString	m_Redirect ;			//	Where to go to actually load the page
	hzString	m_KeepAlive ;			//	Keep-Alive parameters (currently ignored)
	hzString	m_ContentType ;			//	Content type
	hzString	m_ContEncoding ;		//	Eg 'gzip'
	hzString	m_XferEncoding ;		//	The only expected value here is 'chunked'
	hzString	m_AltProto ;			//	The only expected value here is 'chunked'
	hzString	m_Encoding ;			//	Content encoding eg UTF-8
	hzString	m_Etag ;				//	Entity tag if present in the header
	hzString	m_AuthBasic ;			//	If set, this is supplied with each GetPage() call.
	hzUrl		m_Referer ;				//	Last page (used in constructing 'Referer:' header)
	uint32_t	m_nTimeout ;			//	Maximum timeout
	uint32_t	m_nMaxConnects ;		//	Maximum connections (we only ever use one)
	uint32_t	m_nContentLen ;			//	Content length
	uint32_t	m_Retcode ;				//	HTML return code
	bool		m_bConnection ;			//	True if server returns Keep-Alive
	bool		m_bChunked ;			//	Server's response was chunked (diagnostics only)
	char		m_buf[HZ_MAXPACKET+4] ;	//	Buffer for IP packets

	hzHttpClient	(void)
	{
		m_buf[0] = 0 ;
		m_bConnection = false ;
		m_bChunked = false ;
		m_nTimeout = 0 ;
		m_nMaxConnects = 0 ;
		m_nContentLen = 0 ;
		m_Retcode = 0 ;
	}
	~hzHttpClient	(void)
	{
	}

	hzEcode	Connect		(const hzUrl& url) ;
	hzEcode	Close		(void) ;

	hzEcode	TestPage	(hzChain& Z, const hzUrl& url) ;
	hzEcode	GetPage		(HttpRC& hRet, const hzUrl& url, hzMapS<hzString,hzCookie>& cookies, const hzString& etag) ;
	hzEcode	PostForm	(HttpRC& hRet, const hzUrl& url, hzMapS<hzString,hzCookie>& cookies, hzVect<hzString>& hdrs, const hzList<hzPair>& data) ;
	hzEcode	PostAjax	(HttpRC& hRet, const hzUrl& url, hzMapS<hzString,hzCookie>& cookies, hzVect<hzString>& hdrs, const hzList<hzPair>& data) ;
} ;

//
//	See synopsis 	Web Syncing/Scraping (automated downloading)
//
//

enum	webcmd
{
	//	Category:	Internet
	//
	//	Command for automation of HTTP client

	WEBCMD_UNDEF,			//	No step defined
	WEBCMD_LOAD_PAGE,		//	Load a single page (regardless of history)
	WEBCMD_LOAD_LIST,		//	Load pages from a list of links
	WEBCMD_SLCT_PAGE,		//	Select links from a page
	WEBCMD_SLCT_LIST,		//	Select links from a set of pages (set of links)
	WEBCMD_RGET,			//	Recursive get from a root page
	WEBCMD_POST,			//	Post a form
	WEBCMD_RSS				//	Get an RSS feed
} ;

//	Web syncing operational flags that affect website login
#define	HZ_WEBSYNC_AUTH_BASIC	0x0001	//	A base64 password is passed to the server with each HTTP request
#define HZ_WEBSYNC_AUTH_POST	0x0002	//	A form submission of username and password by HTTP POST
#define HZ_WEBSYNC_AUTH_GET		0x0004	//	A form submission of username and password by HTTP GET

//	Web syncing operational flags that affect timeout handling
#define	HZ_WEBSYNC_NOWAIT		0x008	//	If this is set, the hzWebhost::Sync function exits on timeout. The calling application then needs to
										//	check if the sync operation ran to completion and if not then recall it after a suitable delay.

#define WEBFLG_FORCE	0x01	//	Fetch the page even if in the history
#define WEBFLG_NOSTORE	0x02	//	Don't record the page content in a file

class	hzWebCMD
{
	//	Category:	Internet
	//
	//	The syncing of (or automated downloading from) a website (see hzWebhost class description), is effected by executing a series of steps or commands. Each
	//	command is parameterized by a hzWebCMD class instance.
	//
	//	As a minimum, the hzWebCMD must contain a URL and a HTTP command such as GET. If the required action is to POST a form then the URL and the command must
	//	be accompanied by a series of one or more name-value pairs serving as the form data.
	//
	//	The hzWebCMD can also specify recursive downloading. Here the URL is treated as the 'root' page wh
	//	and the  these commands

public:
	hzList<hzPair>	m_Formdata ;	//	List of name value pairs to submit to the site's login form (given as m_Authpage see below)

	hzUrl		m_Url ;				//	Page to fetch
	hzString	m_Crit ;			//	This concerns the (globing) form the link (URL) must take in order to qualify for download.
	hzString	m_Slct ;			//	This creates a set of nodes based on tagname and attributes
	hzString	m_Inputs ;			//	List of pages to fetch (input list)
	hzString	m_Output ;			//	Name of object, eg form or list of links (if applicable)
	uint32_t	m_Flags ;			//	Reserved
	webcmd		m_Cmd ;				//	The command to execute

	hzWebCMD	(void)	{ m_Flags = 0 ; }
	~hzWebCMD	(void)	{ Clear() ; }

	void	Clear	(void)
	{
		m_Formdata.Clear() ;

		m_Url = (char*) 0 ;
		m_Crit = m_Slct = m_Inputs = m_Output = (char*) 0 ;
		m_Flags = 0 ;
		m_Cmd = WEBCMD_UNDEF ;
	}

	hzWebCMD&	operator=	(const hzWebCMD& op)
	{
		m_Formdata = op.m_Formdata ;

		m_Url = op.m_Url ;
		m_Crit = op.m_Crit ;
		m_Slct = op.m_Slct ;
		m_Inputs = op.m_Inputs ;
		m_Output = op.m_Output ;
		m_Flags = op.m_Flags ;
		m_Cmd = op.m_Cmd ;

		return *this ;
	}
} ;

class	hzWebhost
{
	//	Category:	Internet
	//
	//	The hzWebhost class facilitates automated downloading from the set of documents available at any given domain. This is a parameter driven and generally
	//	recursive process. Starting from a list of one or more 'root' pages such as the home page, pages are downloaded and any links these may contain to other
	//	pages are garnered. Then subject to specified limiting criteria, these latter pages are downloaded. The process terminates when all discovered pages are
	//	downloaded.
	//
	//	By default links are limited to other pages on the same website or on other websites listed as related to this. Other criteria may apply such as date of
	//	file and file type these pages are alse read in.
	//
	//	Where authentication is required, the authentication sequence is normally by login form submission. The login form will be downloaded from a particular
	//	URL, the username and password filled in and sent back to the URL indicated in the form (this may or may not be the same). NOTE this will not work where
	//	anti-robot mechanisms are in place, such as google recaptcha forms.
	//
	//	Note also that the sequence of pages to visit may have to include a seemingly pointless visit to a page (normally the home page), purely for the client
	//	to be issued with a cookie in order for the login to be accepted.

	struct	_nodeList
	{
		//	List of nodes selected within a downloaded page

		hzList<hzHtmElem*>	nodes ;		//	List of nodes

		hzString	name ;				//	Name of list
		uint32_t	sofar ;				//	Count of node fetched

		_nodeList	(void)	{ sofar = 0 ; }
	} ;

	struct	_pageList
	{
		//	Config for page to visit

		hzList<hzUrl>	links ;		//	List of URLs

		hzString	name ;			//	Name of list
		uint32_t	sofar ;			//	Count of pages fetched

		_pageList	(void)	{ sofar = 0 ; }
	} ;

	hzEcode	getRss_r	(HttpRC& hRet, const hzUrl& feed, uint32_t nLevel) ;

	hzEcode	_loadstatus	(void) ;	//	Load log of which files have been downloaded
	hzEcode	_savestatus	(void) ;	//	Write out log of which files have been downloaded
	void	_clear		(void) ;	//	Clears history and resets

public:
	hzMapS<hzString,hzCookie>	m_Cookies ;		//	All cookies needed for the session with server
	hzMapS<hzUrl,hzDocMeta*>	m_mapHist ;		//	Links to other pages occuring in this page's body
	hzMapS<hzString,hzHtmForm*>	m_Forms ;		//	Map of forms found in loaded pages.
	hzMapS<hzString,_nodeList*>	m_Nodelists ;	//	Map of lists of selected nodes
	hzMapS<hzString,_pageList*>	m_Pagelists ;	//	Map of lists of selected links
	hzVect<hzDocMeta*>			m_vecHist ;		//	Links to other pages occuring in this page's body
	hzList<hzWebCMD>			m_Commands ;	//	List of commands to effect a SYNC operation

	hzSet<hzUrl>	m_Offsite ;		//	Links discovered that are to pages in other domains or websites
	hzSet<hzEmaddr>	m_Emails ;		//	Email addresses occuring in this page's body
	hzSet<hzString>	m_Banned ;		//	Filter for banning visitation. Links meeting this are not visted, stored or processed.
	hzSet<hzString>	m_Domains ;		//	Allowed domains for the site and it's links
	hzList<hzUrl>	m_Authsteps ;	//	Initial URL requests that must be made for cookie collecting before the login form can be submitted.
	hzList<hzPair>	m_Authform ;	//	List of name value pairs to submit to the site's login form (given as m_Authpage see below)
	hzList<hzPair>	m_Roots ;		//	List of root commands (Webscraping only)
	hzList<hzUrl>	m_Feeds ;		//	List of root commands (Webscraping only)

	hzHttpClient	HC ;			//	HTTP client instance

	//	XML selectors (for selecting info from RSS feeds)
	hzXmlSlct		m_tagItem ;		//	For of extraction of a item
	hzXmlSlct		m_tagTitl ;		//	For of extraction of a title
	hzXmlSlct		m_tagDesc ;		//	For of extraction of a description
	hzXmlSlct		m_tagUqid ;		//	For of extraction of a unique item id
	hzXmlSlct		m_tagLink ;		//	For of extraction of a link
	hzXmlSlct		m_tagDate ;		//	For of extraction of a date

	//	Session data
	hzDocument*		m_docHome ;		//	Home page
	hzDocument*		m_docAuth ;		//	Login page
	hzDocument*		m_resAuth ;		//	Login page response
	hzDocument*		m_resLast ;		//	Last page downloaded
	hzChain			m_Trace ;		//	List of data items garnered (XML format)
	hzChain			m_Styles ;		//	Stylesheet
	hzString		m_Username ;	//	For controlled access to site
	hzString		m_Password ;	//	For controlled access to site
	hzString		m_AuthBasic ;	//	If set, this is supplied with each GetPage() call.
	hzString		m_Repos ;		//	Target directory for download pages.
	hzString		m_CookieSess ;	//	Session cookie (set when new cookie is offered by server, used in all subsequent requests)
	hzString		m_CookiePath ;	//	Session cookie (set when new cookie is offered by server, used in all subsequent requests)
	hzString		m_Name ;		//	Canonical name of site eg 'positive news'

	//	Common significant addresses (usually set by config file)
	hzUrl			m_Homepage ;	//	Root URL for site
	hzUrl			m_Authpage ;	//	Login page for site (if applicable, may be same as home)
	hzUrl			m_Authexit ;	//	Logout URL
	hzUrl			m_ContactUs ;	//	Used to post messages to websites

	//	General
	uint32_t		m_Opflags ;		//	Operational flags
	uint32_t		m_Sofar ;		//	Count of Sync commands executed

	hzWebhost	(void)
	{
		m_docHome = 0 ;
		m_docAuth = 0 ;
		m_resAuth = 0 ;
		m_resLast = 0 ;
		m_Opflags = 0 ;
		m_Sofar = 0 ;
	}

	~hzWebhost	(void)
	{
		_clear() ;

		if (m_docHome)	delete m_docHome ;
		if (m_docAuth)	delete m_docAuth ;
		if (m_resAuth)	delete m_resAuth ;
		if (m_resLast)	delete m_resLast ;
	}

	//hzEcode	Init	(const hzString& repos, const hzUrl& url, hzAuthmode authmode = HZ_AUTH_NONE) ;
	hzEcode	AuthBasic	(const char* username, const char* password) ;
	hzEcode	AddRoot		(hzUrl& url, hzString& criteria) ;
	hzEcode	AddRSS		(hzUrl& rss) ;

	hzEcode	AddBan		(hzString& pageEnding)
	{
		if (!pageEnding)
			return E_NODATA ;
		return m_Banned.Insert(pageEnding) ;
	}

	hzDocument*	Download	(const hzUrl& url) ;

	hzEcode	Login		(void) ;
	void	Logout		(void) ;
	hzEcode	Sync		(void) ;
	hzEcode	Scrape		(void) ;
	hzEcode	GetRSS		(void) ;
} ;

#endif	//	hzHttpClient_h
