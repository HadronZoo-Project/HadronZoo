//
//	File:	hzHttpServer.h
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

//
//	This header describes the two further classes needed to interface to the hzIpServer instance in order to impliment an internet application.
//	These are:-
//
//	1)	hzHttpSession
//	2)	hzHttpFile
//	3)	hzHttpEvent
//

#ifndef hzHttpServer_h
#define hzHttpServer_h

#include "hzChain.h"
#include "hzCtmpls.h"
#include "hzIpServer.h"
#include "hzHttpProto.h"

#define	HZ_MAX_HTTP_HDR		8192	//	Note this is much smaller than that specified by Apache for example. There is no good reason for excessive proprietary
									//	lines in the HTTP header and use of data submissions are discouraged under Dissemino guidlines.

class	hzHttpSession
{
	//	Category:	Internet
	//
	//	The hzHttpSession is a pure virtual class whose sole purpose is to unify any application-level classes that allow the server to maintain state.
	//	The only menifestation thusfar, is the Dissemino class hzwInfo.

public:
	virtual	~hzHttpSession	() {}
} ;

class	hzHttpFile
{
	//	Category:	Internet
	//
	//	Support class for hzHttpEvent to handle file uploads

public:
	hzChain		m_file ;		//	The file itself
	hzString	m_fldname ;		//	Name of field
	hzString	m_filename ;	//	Name of uploaded file
	int32_t		m_resv ;		//	Reserved
	hzMimetype	m_mime ;		//	MIME type of content

	hzHttpFile	(void)
	{
		m_resv = 0 ;
		m_mime = HMTYPE_INVALID ;
	}

	hzHttpFile&	operator=	(const hzHttpFile& op)
	{
		m_file = op.m_file ;
		m_fldname = op.m_fldname ;
		m_filename = op.m_filename ;
		m_resv = op.m_resv ;
		m_mime = op.m_mime ;
		return *this ;
	}
} ;

class	hzHttpEvent
{
	//	Category:	Internet
	//
	//	The hzHttpEvent class is the focal point of a HadronZoo/Dissemino Internet application. All HTTP requests (GET) or submissions (POST) generate
	//	and populates a hzHttpEvent instance which initially contains data supplied in the incoming request. Each time this occurs, the hzHttpEvent is
	//	passed to a user defined 'callback' function which processes it and generates a response. This process may add further data to the hzHttpEvent
	//	which is used to formulate the header for the outgoing HTTP response.

	hzIpConnex*		m_pCx ;			//	Connection to TCP server
	hzLogger*		m_pLog ;		//	Log channel (from connection)
	hzHttpSession*	m_pSession ;	//	HTTP session (set by app)

	char*		m_pBuf ;			//	Buffer for header values
	char*		m_pAccept ;			//	Accept mimetype eg text/plain
	char*		m_pAcceptCharset ;	//	Accept character set
	char*		m_pAcceptLang ;		//	Language e.g. US English
	char*		m_pAcceptCode ;		//	Encoding
	char*		m_pCacheControl ;	//	Cache Control
	char*		m_pConnection ;		//	Connection directive
	char*		m_pContentType ;	//	Content Type
	char*		m_pETag ;			//	Page ID checking
	char*		m_pPragma ;			//	Misc command
	char*		m_pUserAgent ;		//	Type of browser
	char*		m_pProcessor ;		//	Type of processor on browser computer
	char*		m_pVia ;			//	Some proxy servers send this
	char*		m_pCliIP ;			//	IP From Apache
	char*		m_pHost ;			//	The desired hostname
	char*		m_pXost ;			//	The desired hostname
	char*		m_pFwrdIP ;			//	IP From Apache
	char*		m_pProxIP ;			//	IP From Apache
	char*		m_pServer ;			//	The desired server
	char*		m_pFrom ;			//	Email address of person making request (not sure why this is but hey what the heck)
	char*		m_pReferer ;		//	Email address of person making request (not sure why this is but hey what the heck)
	char*		m_pReqPATH ;		//	Request path
	char*		m_pReqFRAG ;		//	Request path fragment
	hzXDate		m_LastMod ;			//	If last modified directive
	hzUrl		m_Referer ;			//	Refered from URL.
	hzSysID		m_CookieSub ;		//	Cookie submitted by the browser
	hzSysID		m_CookieNew ;		//	New cookie as set by server
	hzSysID		m_CookieOld ;		//	Redundant cookie from browser
	hzString	m_Redirect ;		//	Directive to browser URI
	hzString	m_Auth ;			//	Basic authorization
	hzSDate		m_CookieExpire ;	//	Expiry date (for setting permanent cookies only)
	hzIpaddr	m_ClientIP ;		//	This is either copied from the connection or if apache proxypass is in use, then it is set by an X-param
	uint32_t	m_nVersion ;		//	HTTP/1.0 or whatever.
	uint32_t	m_nHeaderLen ;		//	Actual length of header
	uint32_t	m_nContentLen ;		//	Actual length of the form data
	uint32_t	m_nQueryLen ;		//	Query length
	uint32_t	m_nMaxForwards ;	//	Max forwards
	uint32_t	m_nConnection ;		//	Positive value for keep-alive (no of seconds to live), 0 for close
	uint32_t	m_nCountry ;		//	Country code
	HttpRC		m_eRetCode ;		//	HTTP return code
	HttpMethod	m_eMethod ;			//	E.g. GET, HEAD or POST
	bool		m_bHdrComplete ;	//	False if hit's header incomplete
	bool		m_bMsgComplete ;	//	False if hit incomplete
	bool		m_bZipped ;			//	True if Accept-Encoding contains 'gzip'

	uint32_t	_setnvpairs		(hzChain::Iter& cIter) ;
	hzEcode		_formhead		(hzChain& Z, HttpRC hrc, hzMimetype type, uint32_t nSize, uint32_t nExpires, bool bZip) ;

	//	Prevent copies
	hzHttpEvent		(const hzHttpEvent&) ;
	hzHttpEvent&	operator=	(const hzHttpEvent&) ;

public:
	hzMapS	<hzString,hzString>		m_mapStrings ;	//	Populated by form submission (POST) events but also by SetVar() calls by the application
	hzMapS	<hzString,hzChain>		m_mapChains ;	//	Populated by SetVar() function (used in Dissemino)
	hzMapS	<hzString,hzHttpFile>	m_Uploads ;		//	List of uploaded files
	hzMapS	<hzString,uint32_t>		m_ObjIds ;		//	Map of caches and objects ids (Dissemino only)

	hzArray	<hzPair>		m_Inputs ;				//	Data submissions
	hzList	<hzPair>		m_HdrsResponse ;		//	HTTP headers set in the response

	void*		m_pContextApp ;		//	The applicable Dissemino application (set by the 'Host:' header in the HTTP request)
	void*		m_pContextLang ;	//	The applicable language (set by HTTP request processing)
	void*		m_pContextForm ;	//	Application specific context. Dissemino current form reference.
	void*		m_pContextObj ;		//	Application specific context. Dissemino current object.
	hzXDate		m_Occur ;			//	Exact time of hit
	hzString	m_Resarg ;			//	Resource argument (dissemino)
	hzString	m_LangCode ;		//	Page Language (in response)
	hzString	m_appError ;		//	This can be set by the application (eg form submission error)

	//	Variables to cope with Dissemino
	hzChain		m_Error ;			//	Set during form processing in order to pass error messages to the form response
	hzChain		m_Report ;			//	Set by function processing the hits

	hzHttpEvent		(hzChain& ZI, hzIpConnex* pCx) ;
	hzHttpEvent		(void) ;
	~hzHttpEvent	(void) ;

	void	Clear	(void) ;

	//	Simple Set Functions
	void	SetLogger	(hzLogger* pLog)	{ m_pLog = pLog ; }
	void	SetURI		(const char* cpURI)	{ m_Redirect = cpURI ; }
	void	SetRetCode	(HttpRC RetCode)	{ m_eRetCode = RetCode ; }

	//	Simple Get Functions
	hzLogger*	GetLogger	(void) const	{ return m_pLog ; }
	HttpRC		GetRetCode	(void) const	{ return m_eRetCode ; }
	uint32_t	EventNo		(void) const	{ return m_pCx ? m_pCx->EventNo() : 0 ; }
	uint32_t	CliSocket	(void) const	{ return m_pCx ? m_pCx->CliSocket() : 0 ; }
	uint32_t	HeaderLen	(void) const	{ return m_nHeaderLen ; }
	uint32_t	ExpectSize	(void) const	{ return m_nHeaderLen + m_nContentLen ; }
	uint32_t	QueryLen	(void) const	{ return m_nQueryLen ; }
	bool		HdrComplete	(void) const	{ return m_bHdrComplete ; }
	bool		MsgComplete	(void) const	{ return m_bMsgComplete ; }
	bool		Zipped		(void) const	{ return m_bZipped ; }

	hzEcode	GetAt	(hzPair& P, uint32_t nIndex)
	{
		if (nIndex < 0 || nIndex >= m_Inputs.Count())
			return E_RANGE ;
		P = m_Inputs[nIndex] ;
		return E_OK ;
	}

	uint32_t	Inputs		(void) const	{ return m_Inputs.Count() ; }
	HttpMethod	Method		(void) const	{ return m_eMethod ; }
	hzIpaddr	ClientIP	(void) const	{ return m_ClientIP ; }

	hzUrl		Referer		(void) const	{ return m_Referer ; }
	hzString	Auth		(void) const	{ return m_Auth ; }
	hzSysID		Cookie		(void) const	{ return m_CookieSub ; }

	const char*	TxtClientIP	(void) const	{ return m_pCliIP ; }
	const char*	TxtFwrdIP	(void) const	{ return m_pFwrdIP ; }
	const char*	TxtProxIP	(void) const	{ return m_pProxIP ; }
	const char*	ETag		(void) const	{ return m_pETag ; }
	const char*	Hostname	(void) const	{ return m_pHost ; }
	const char*	Server		(void) const	{ return m_pServer ; }
	const char*	UserAgent	(void) const	{ return m_pUserAgent ; }
	const char*	GetResource	(void) const	{ return m_pReqPATH ; }
	//const char*	GetQuery	(void) const	{ return m_pReqQURY ; }
	const char*	GetFragment	(void) const	{ return m_pReqFRAG ; }

	uint32_t	Connection	(void) const	{ return m_nConnection ; }

	hzIpConnex*		GetConnex	(void) const	{ return m_pCx ; }
	hzHttpSession*	Session		(void) const	{ return m_pSession ; }

	//	Set header (for response) to any type/any value
    hzEcode		SetHdr		(const hzString& name, const hzString& value) ;

	//	Set variable to any type/any value
    hzEcode		SetVarString	(const hzString& name, const hzString& value) ;
    hzEcode		SetVarChain		(const hzString& name, const hzChain& value) ;

	//	Get variable functions
	hzString	GetVarStr		(const hzString& name)	{ return m_mapStrings[name] ; }
	hzChain		GetVarChain		(const hzString& name)	{ return m_mapChains[name] ; }
	hzSDate		CookieExpire	(void)					{ return m_CookieExpire ; }
	hzEcode		GetVar			(hzChain& Z, const hzString& name) ;

	//	Other operations
	void		SetSessCookie	(const hzSysID& Cookie) ;
	void		SetPermCookie	(const hzSysID& Cookie, hzSDate& expires) ;
	void		DelSessCookie	(const hzSysID& Cookie)	{ m_CookieOld = Cookie ; }
	void		SetSession		(hzHttpSession* pSession)	{ m_pSession = pSession ; }
	hzEcode		ProcessEvent	(hzChain& Z) ;
	hzEcode		Storeform		(const char* cpPath) ;
	hzEcode		SendRawChain	(HttpRC hrc, hzMimetype type, const hzChain& Data, uint32_t nExpires, bool bZip) ;
	hzEcode		SendRawString	(HttpRC hrc, hzMimetype type, const hzString& fixContent, uint32_t nExpires, bool bZip) ;
	hzEcode		SendFilePage	(const char* pDir, const char* cpFilename, uint32_t nExpires, bool bZip) ;
	hzEcode		SendPageE		(const char* pDir, const char* cpFilename, uint32_t nExpires, bool bZip) ;
	hzEcode		SendFileHead	(const char* pDir, const char* cpFilename, uint32_t nExpires = 0) ;
	hzEcode		SendHttpHead	(const hzString& fixContent, hzMimetype type, uint32_t nExpires = 0) ;
	hzEcode		SendHttpHead	(const hzChain& fixContent, hzMimetype type, uint32_t nExpires = 0) ;
	hzEcode		Redirect		(const hzUrl& url, uint32_t nExpires, bool bZip) ;
	hzEcode		SendNotFound	(hzUrl& url) ;
	hzEcode		SendError		(HttpRC hrc, const char* va_alist ...) ;
	hzEcode		SendAjaxResult	(HttpRC hrc) ;
	hzEcode		SendAjaxResult	(HttpRC hrc, const char* va_alist ...) ;
	hzEcode		SendAjaxResult	(HttpRC hrc, hzChain& Z) ;
} ;

#endif	//	hzHttpServer_h
