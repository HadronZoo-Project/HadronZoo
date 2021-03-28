//
//	File:	hzHttpServer.cpp
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
//	Implimentation of the hzHttpEvent class described in hzHttpEvent.h
//

#include <fstream>

#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <openssl/ssl.h>

#include "hzErrcode.h"
#include "hzChars.h"
#include "hzTextproc.h"
#include "hzCodec.h"
#include "hzDirectory.h"
#include "hzHttpServer.h"
#include "hzProcess.h"
#include "hzDissemino.h"

using namespace std ;

#define HZ_COOKIESIZE	28

/*
**	Variables
*/

global	hzMapS<hzString,hzChain>	s_SSIncludes ;		//	Server side file includes
global	hzMapS<hzString,hzChain*>	s_PageStore ;		//	Stored pages

global	hzString	_hzGlobal_runstart ;				//	Date string (of time of first serve this runtime)

/*
**	Non member functions
*/

static	uint32_t	_hexconvert	(char* pStr, uint32_t nLen)
{
	//	Expected the supplied char ptr (arg 1) to be at the start of a hexadecimal number of nLen bytes (arg 2). If it is the value is converted to
	//	uint32_t and returned
	//
	//	Arguments:	1)	pStr	Pointer into an input string
	//				2)	nLen	Max number of chars to convert
	//
	//	Returns:	Value of hex conversion

	_hzfunc(__func__) ;

	uint32_t	nCount ;		//	Character count
	uint32_t	nValue = 0 ;	//	Value established

	if (!pStr)
		return 0 ;

	for (nCount = 0 ; *pStr && nCount < nLen ; nCount++)
	{
		nValue *= 16 ;

		if (pStr[nCount] >= '0' && pStr[nCount] <= '9')
			{ nValue += (pStr[nCount] - '0') ; continue ; }
		if (pStr[nCount] >= 'A' && pStr[nCount] <= 'F')
			{ nValue += 10 ; nValue += (pStr[nCount] - 'A') ; continue ; }
		if (pStr[nCount] >= 'a' && pStr[nCount] <= 'f')
			{ nValue += 10 ; nValue += (pStr[nCount] - 'a') ; continue ; }

		return 0 ;
	}

	return nValue ;
}

hzHttpEvent::hzHttpEvent	(hzChain& ZI, hzIpConnex* pCx)
{
	m_Occur.SysDateTime() ;
	m_pCx = pCx ;
	if (pCx)
	{
		m_pLog = m_pCx->GetLogger() ; 
		m_ClientIP = m_pCx->ClientIP() ;
	}
	m_pContextApp = m_pContextLang = m_pContextForm = m_pContextObj = 0 ;
	m_pBuf = 0 ;
	Clear() ;
}

hzHttpEvent::hzHttpEvent	(void)
{
	m_Occur.SysDateTime() ;
	m_pLog = 0 ;
	m_pCx = 0 ; 
	m_pContextApp = m_pContextLang = m_pContextForm = m_pContextObj = 0 ;
	m_pBuf = 0 ;
	Clear() ;
}

hzHttpEvent::~hzHttpEvent	(void)
{
	m_pLog = 0 ;
	m_pCx = 0 ;
	m_pSession = 0 ;
	m_pContextApp = m_pContextLang = m_pContextForm = m_pContextObj = 0 ;
	Clear() ;
}

void	hzHttpEvent::Clear	(void)
{
	m_ClientIP.Clear() ;	// = (char*) 0 ;

	if (m_pBuf)
		delete m_pBuf ;
	//m_CookieNew = 0 ;
	//m_CookieOld = 0 ;
	m_Referer = 0 ;
	m_Redirect = 0 ;
	m_Auth = 0 ;
	//m_CookieSub = 0 ;

	m_pAccept = m_pAcceptCharset = m_pAcceptLang = m_pAcceptCode = m_pCacheControl = m_pConnection = m_pContentType = m_pETag = m_pPragma = 0 ;
	m_pUserAgent = m_pProcessor = m_pVia = m_pCliIP = m_pHost = m_pXost = m_pFwrdIP = m_pProxIP = m_pServer = m_pFrom = m_pReferer = 0 ;
	//m_pReqPATH = m_pReqQURY = m_pReqFRAG = 0 ;
	m_pReqPATH = m_pReqFRAG = 0 ;

	m_LastMod.Clear() ;
	m_CookieExpire.Clear() ;

	m_pSession = 0 ;
	m_nHeaderLen = 0 ;
	m_nContentLen = 0 ;
	m_nQueryLen = 0 ;
	m_nMaxForwards = 0 ;
	m_eRetCode = HTTPMSG_OK ;
	m_eMethod = HTTP_INVALID ;
	m_bHdrComplete = false ;
	m_bMsgComplete = false ;
	m_bZipped = false ;
	m_nConnection = 0 ;
}

uint32_t	hzHttpEvent::_setnvpairs	(chIter& ci)
{
	//	Gather up the submitted data as a set of name-value pairs. Advance the supplied iterator to the end of the header.
	//
	//	Arguments:	1)	ci	Chain iterator to process submission data
	//
	//	Returns:	Number of places iterator has advanced

	_hzfunc("hzHttpEvent::_setnvpairs") ;

	hzChain		C ;			//	For building names/values
	hzPair		P ;			//	Name value pair
	uint32_t	nCount ;	//	Counter
	char		hex[4] ;	//	For hex conversion

	for (nCount = 0 ; !ci.eof() && *ci != CHAR_SPACE && *ci != CHAR_HASH ; nCount++, ci++)
	{
		//	Consider hex encoding first

		if (*ci == CHAR_PERCENT)
		{
			ci++ ; hex[0] = *ci ;
			ci++ ; hex[1] = *ci ;
			hex[2] = 0 ;
			nCount += 2 ;

			C.AddByte(_hexconvert(hex, 2)) ;
			continue ;
		}

		//	Hex encoding safe

		if (*ci == CHAR_EQUAL)
		{
			P.name = C ;
			C.Clear() ;
			continue ;
		}

		if (*ci == CHAR_AMPSAND || *ci <= CHAR_SPACE)
		{
			if (C.Size() > 4000)
				m_mapChains.Insert(P.name, C) ;
			else
			{
				P.value = C ;
				m_Inputs.Add(P) ;
				m_mapStrings.Insert(P.name, P.value) ;
				P.Clear() ;
			}
			C.Clear() ;

			if (*ci == CHAR_AMPSAND)
				continue ;
			else
				break ;
		}

		if (*ci == CHAR_PLUS)
			C.AddByte(CHAR_SPACE) ;
		else
			C.AddByte(*ci) ;
	}

	if (C.Size())
	{
		P.value = C ;
		m_Inputs.Add(P) ;
		m_mapStrings.Insert(P.name, P.value) ;
	}

	return nCount ;
}

void	hzHttpEvent::SetSessCookie	(const hzSysID& Cookie)
{
	//	Set a new cookie and expire any old.
	//
	//	Arguments:	1)	Cookie	The full cookie string
	//
	//	Returns:	None

	_hzfunc("hzHttpEvent::SetSessCookie") ;

	m_CookieNew = Cookie ;
	m_CookieExpire.Clear() ;
}

void	hzHttpEvent::SetPermCookie	(const hzSysID& Cookie, hzSDate& expires)
{
	//	Set a new cookie and expire any old.
	//
	//	Arguments:	1)	Cookie	The full cookie string
	//				2)	expires	Short form date
	//
	//	Returns:	None

	_hzfunc("hzHttpEvent::SetPermCookie") ;

	m_CookieNew = Cookie ;
	m_CookieExpire = expires ;
}

hzEcode	hzHttpEvent::ProcessEvent	(hzChain& ZI)
{
	//	Purpose:	Process a HTTP event (an incoming HTTP GET or POST request)
	//
	//	Arguments:	1)	ZI		Input chain poplated by the incomming HTTP request or submission
	//
	//	Returns:	E_FORMAT	If the request is malformed in some way
	//				E_ARGUMENT	If the request does not indicate a URL or a host.
	//				E_OK		If the operation was successful.

	_hzfunc("hzHttpEvent::ProcessEvent") ;

	hzChain::BlkIter	bi ;		//	To get directly at input chain inner buffer

	hzChain			Head ;			//	The HTTP header
	hzChain			Word ;			//	For building tokens
	chIter			zi ;			//	Chain iterator
	chIter			xi ;			//	Chain iterator
	chIter			mkA ;			//	1st part of header line
	chIter			mkB ;			//	2nd part of header line
	chIter			mkC ;			//	End 2nd part of header line
	hzHttpFile		upload ;		//	To cope with file uploads
	hzPair			Pair ;			//	Name value pair
	const char*		i ;				//	Loop control
	char*			j ;				//	For string iteration
	char*			ph ;			//	Offset into m_pBuf ;
	uint64_t		cookie ;		//	Cookie value
	hzString		S ;				//	Temp string
	hzString		boundary ;		//	MIME boundary for multipart
	uint32_t		nLine = 0 ;		//	Header line number
	uint32_t		bErr = 0 ;		//	Format error
	uint32_t		hSofar ;		//	For header value extraction
	uint32_t		n ;				//	For cookie extraction
	uint32_t		len ;			//	Offset into buffer
	uint32_t		nFst ;			//	No of colon-space sequences in header
	uint32_t		nSnd ;			//	No of CR-NL sequences in header (excluding the last pair)

	if (!this)		Fatal("%s. No Instance\n", *_fn) ;
	if (!m_pLog)	Fatal("%s. Cannot process requests. No logfile\n", *_fn) ;
	if (!m_pCx)		Fatal("%s. Cannot process requests. No client info\n", *_fn) ;

	//	If we already have completed header part, skip
	m_pCx->m_Track.Printf("%s: Called with input chain of %d bytes\n", *_fn, ZI.Size()) ;
	m_pCx->m_Track << ZI ;

	if (m_bHdrComplete)
	{
		//if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
			m_pCx->m_Track.Printf("HTTP HEADER COMPLETE: Goto stage 2 with Header of %d bytes (conn=%p)\n", bi.Data(), m_nHeaderLen, m_pCx) ;
		goto stage_two ;
	}

	if (ZI.Size() < 80)
	{
		//	If size too small, check for illegal methods but if OK, return and wait for more data
		zi = ZI ;

		if (!(*zi >= 'A' && *zi <= 'Z') || zi == "CONNECT ")
		{
			m_pCx->m_Track << "Invalid Header [" << ZI << "]\n" ;
			SetStatusIP(m_pCx->ClientIP(), HZ_IPSTATUS_BLACK_HTTP, 9000) ;
			return E_FORMAT ;
		}

		m_pCx->m_Track << "Incomplete Header [" << ZI << "]\n" ;
		return E_OK ;
	}

	m_ClientIP = m_pCx->ClientIP() ;
	m_Occur.SysDateTime() ;

	//	Establish header size
	if (!m_nHeaderLen)
	{
		//	Read up to end of first line to measure length required to accommodate the request path and any query or fragment
		nFst = nSnd = hSofar = 0 ;
		for (zi = ZI ; !zi.eof() && Head.Size() < HZ_MAX_HTTP_HDR ; zi++)
		{
			if (*zi == CHAR_CR && zi == "\r\n")
				{ Head << "\r\n" ; zi += 2 ; break ; }
			Head.AddByte(*zi) ;
		}
		hSofar = Head.Size() ;

		//	Then get rest of HTTP header
		for (; !zi.eof() && Head.Size() < HZ_MAX_HTTP_HDR ; zi++)
		{
			if (*zi == CHAR_CR && zi == "\r\n\r\n")
				{ Head << "\r\n\r\n" ; m_nHeaderLen = Head.Size() ; break ; }

			if (zi == ": ")
				{ nFst++ ; hSofar++ ; }
			if (nFst > nSnd)
				hSofar++ ;
			if (zi == "\r\n")
				nSnd++ ;
			Head.AddByte(*zi) ;
		}

		if (!m_nHeaderLen)
		{
			m_pCx->m_Track.Printf("\n%s: REJECTED REQUEST (No header end)\n", *m_Occur.Str()) ;
			SendError(HTTPMSG_NOTFOUND, "Excessive HTTP Header\n") ;
			return E_RANGE ;
		}

		if (m_nHeaderLen >= HZ_MAX_HTTP_HDR)
		{
			m_pCx->m_Track.Printf("\n%s: REJECTED REQUEST (too large)\n", *m_Occur.Str()) ;
			SendError(HTTPMSG_ENTITY_TOO_LARGE, "Excessive HTTP Header\n") ;
			return E_RANGE ;
		}

		/*
		**	Extract HTTP header values
		*/

		if (hSofar < 1024)
			hSofar = 1024 ;
		ph = m_pBuf = new char[hSofar] ;
		zi = Head ;

		if		(zi == "GET ")		{ zi += 4 ; m_eMethod = HTTP_GET ; }
		else if (zi == "HEAD ")		{ zi += 5 ; m_eMethod = HTTP_HEAD ; }
		else if (zi == "POST ")		{ zi += 5 ; m_eMethod = HTTP_POST ; }
		else if (zi == "OPTIONS ")	{ zi += 8 ; m_eMethod = HTTP_OPTIONS ; }
		else if (zi == "PUT ")		{ zi += 4 ; m_eMethod = HTTP_PUT ; }
		else if (zi == "DELETE ")	{ zi += 7 ; m_eMethod = HTTP_DELETE ; }
		else if (zi == "TRACE ")	{ zi += 6 ; m_eMethod = HTTP_TRACE ; }
		else if (zi == "CONNECT ")	{ zi += 8 ; m_eMethod = HTTP_CONNECT ; }
		else
			bErr |= 0x01 ;

		//	Obtain the requested path and if present, the query and the fragment
		if (!bErr)
		{
			//	Get the requested path first
			for (m_pReqPATH = ph ; !zi.eof() && *zi != CHAR_SPACE && *zi != CHAR_QUERY && *zi != CHAR_HASH ; ph++, zi++)
				*ph = *zi ;
			*ph++ = 0 ;

			if (*zi == CHAR_QUERY)
			{
				//	Read until either a SPACE or a HASH
				zi++ ;
				m_nQueryLen = _setnvpairs(zi) ;
			}

			if (*zi == CHAR_HASH)
			{
				//	Read until a SPACE
				for (m_pReqFRAG = ph ; !zi.eof() && *zi == CHAR_SPACE ; ph++, zi++)
					*ph = *zi ;
				*ph++ = 0 ;
			}

			if (*zi != CHAR_SPACE)
				bErr |= 0x02 ;
			if (!m_pReqPATH[0])
				bErr |= 0x02 ;
			zi++ ;
		}

		//	Obtain the HTTP version
		if (!bErr)
		{
			if (zi != "HTTP/")
				bErr |= 0x04 ;
			else
			{
				zi += 5 ;

				if		(zi == "1.0\r\n")	{ zi += 5 ; m_nVersion = 0 ; }
				else if (zi == "1.1\r\n")	{ zi += 5 ; m_nVersion = 1 ; }
				else if (zi == "2.0\r\n")	{ zi += 5 ; m_nVersion = 2 ; }
				else
					bErr |= 0x08 ;
			}
		}

		/*
		**	Now grab the other headers of interest. Note headers not of interest are ignored. The main objective is to reject HTTP requests that are malformed. The process assumes
		**	each header is in it's own line and is of the form "header_name: value\r\n".
		*/

		//for (nLine = 2 ; !zi.eof() && !bErr && nLine <= nSnd ; nLine++)
		for (nLine = 2 ; !zi.eof() && !bErr ; nLine++)
		{
			//	Should be at the start of a line so establish line contents
			if (zi == "\r\n")
				break ;

			//	Discover 1st half of header line (upto colon and space)
			for (mkA = mkB = mkC = zi ; !zi.eof() ; zi++)
			{
				if (*zi == CHAR_COLON && zi[1] == CHAR_SPACE)
				{
					//	Discover 2nd part of line (from colon and space to end of line)
					zi += 2 ;
					for (len = 0, mkB = zi ; !zi.eof() ; len++, zi++)
					{
						if (*zi == CHAR_CR && zi[1] == CHAR_NL)
							{ mkC = zi ; zi += 2 ; break ; }
					}
					if (!len)
						bErr |= 0x10 ;
					break ;
				}
			}

			//	Now have a complete line
			switch	(toupper(*mkA))
			{
			case 'A':

				if (mkA.Equiv("Accept"))			{ for (m_pAccept = ph,			xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("Accept-Charset"))	{ for (m_pAcceptCharset = ph,	xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("Accept-Language"))	{ for (m_pAcceptLang = ph,		xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("Accept-Encoding"))	{ for (m_pAcceptCode = ph,		xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }

				if (mkA.Equiv("Authorization: basic"))
					{ for (j = ph, xi = mkB ; xi != mkC ; j++, xi++) *j = *xi ; *j++ = 0 ; m_Auth = Base64Decode(ph) ; }
				break ;

			case 'C':

				if (mkA.Equiv("Cache-Control"))		{ for (m_pCacheControl = ph,	xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("Content-Type"))		{ for (m_pContentType = ph,		xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("Client-ip"))			{ for (m_pCliIP = ph,			xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("Content-Length"))	{ for (j = ph, xi = mkB ; xi != mkC ; j++, xi++) *j = *xi ; *j++ = 0 ; m_nContentLen = *ph ? atoi(ph) : 0 ; break ; }
				if (mkA.Equiv("Connection"))		{ m_nConnection = mkB.Equiv("keep-alive") ? 0 : 15 ; break ; }

				if (mkA.Equiv("Cookie"))
				{
					//	Only interested in cookies with names matching the application name
					for (xi = mkB ; *xi ; xi++)
					{
						//if (xi == _hzGlobal_HtmlApp->m_CookieName)
						if (xi == _hzGlobal_Dissemino->m_CookieName)
						{
							//	Found a cookie begining with _hz so copy upto the semicolon
							//xi += _hzGlobal_HtmlApp->m_CookieName.Length() + 1 ;
							xi += _hzGlobal_Dissemino->m_CookieName.Length() + 1 ;
							for (j = ph, n = 0 ; *xi && n < 32 && *xi > CHAR_SPACE && *xi != CHAR_SCOLON ; j++, xi++, n++)
								*j = *xi ;
							*j++ = 0 ;
							IsHexnum(cookie, ph) ;
							m_CookieSub = cookie ;
							break ;
						}
					}
				}
				break ;

			case 'F':	if (mkA.Equiv("From"))	{ for (m_pFrom = ph, xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; }	break ;

			case 'H':	if (mkA.Equiv("Host"))
						{
							for (m_pHost = ph, xi = mkB ; *xi != CHAR_COLON && xi != mkC ; ph++, xi++)
								*ph = *xi ;
							*ph++ = 0 ;
						}
						if (xi == CHAR_COLON)
							for (; xi != mkC ; xi++) ;
						break ;

			case 'I':	if (mkA.Equiv("If-Modified-Since"))	{ for (j = ph, xi = mkB ; xi != mkC ; j++, xi++) *j = *xi ; *j++ = 0 ; m_LastMod = ph ; break ; }
						if (mkA.Equiv("If-None-Match"))		{ for (m_pETag = ph, xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; }
						break ;

			case 'K':	if (mkA.Equiv("Keep-Alive"))
							m_nConnection = 15 ;
						break ;

			case 'M':	if (mkA.Equiv("Max-Forwards"))	{ for (j = ph, xi = mkB ; xi != mkC ; j++, xi++) *j = *xi ; *j++ = 0 ; m_nMaxForwards = *ph ? atoi(ph) : 0 ; }	break ;
			case 'P':	if (mkA.Equiv("Pragma"))		{ for (m_pPragma = ph,	xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; }	break ;
			case 'R':	if (mkA.Equiv("Referer"))		{ for (m_pReferer = ph,	xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; m_Referer = m_pReferer ; }	break ;

			case 'U':	if (mkA.Equiv("User-Agent"))	{ for (m_pUserAgent = ph, xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
						if (mkA.Equiv("UA-CPU"))		{ for (m_pProcessor = ph, xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; }
						break ;

			case 'V':	if (mkA.Equiv("Via"))
							{ for (m_pVia = ph, xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; }
						break ;

			case 'x':
			case 'X':

				if (mkA.Equiv("X-Forwarded-For"))		{ for (m_pFwrdIP = ph,	xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("X-ProxyUser-IP"))		{ for (m_pProxIP = ph,	xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("X-Forwarded-Host"))		{ for (m_pXost = ph,	xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; break ; }
				if (mkA.Equiv("X-Forwarded-Server"))	{ for (m_pServer = ph,	xi = mkB ; xi != mkC ; ph++, xi++) *ph = *xi ; *ph++ = 0 ; }
				break ;

			default:
				m_pCx->m_Track.Printf("None of the above line %d\n", nLine) ;
				break ;
			}
		}

		//	If no other errors, check we have an IP address
		if (m_ClientIP == IPADDR_BAD || m_ClientIP == IPADDR_NULL || m_ClientIP == IPADDR_LOCAL)
		{
			if (m_pCliIP)
				m_ClientIP = m_pCliIP ;
		}

		if (m_ClientIP == IPADDR_BAD || m_ClientIP == IPADDR_NULL || m_ClientIP == IPADDR_LOCAL)
		{
			if (m_pFwrdIP)
				m_ClientIP = m_pFwrdIP ;
		}

		if (m_ClientIP == IPADDR_BAD || m_ClientIP == IPADDR_NULL || m_ClientIP == IPADDR_LOCAL)
		{
			if (m_pProxIP)
				m_ClientIP = m_pProxIP ;
		}

		if (m_ClientIP == IPADDR_BAD || m_ClientIP == IPADDR_NULL || m_ClientIP == IPADDR_LOCAL)
			bErr |= 0x20 ;

		if (!m_pHost)
			bErr |= 0x40 ;

		if (bErr)
		{
			m_bMsgComplete = true ;

			m_pCx->m_Track.Printf("%s: %s Sock %d/%d REQUEST [\n", *_fn, *m_Occur.Str(), m_pCx->CliSocket(), m_pCx->CliPort()) ;
			m_pCx->m_Track << ZI ;
			m_pCx->m_Track << "]\n" ;

			if (bErr & 0x01)	m_pCx->m_Track.Printf("Line 1: Failed to find HTTP Method\n") ;
			if (bErr & 0x02)	m_pCx->m_Track.Printf("Line 1: Malformed HTTP resource request\n") ;
			if (bErr & 0x04)	m_pCx->m_Track.Printf("Line 1: Invalid HTTP Version\n") ;
			if (bErr & 0x08)	m_pCx->m_Track.Printf("Line %d: No space after colon\n", nLine) ;
			if (bErr & 0x10)	m_pCx->m_Track.Printf("Line %d: Could not evaluate line\n", nLine) ;
			if (bErr & 0x20)	m_pCx->m_Track.Printf("Could not detect client IP\n") ;
			if (bErr & 0x40)	m_pCx->m_Track.Printf("No Host header supplied\n") ;

			if (m_eMethod == HTTP_CONNECT && (!m_pHost || m_pHost != _hzGlobal_Hostname))
			{
				//	This is an automatic ban

				if (!(bErr & 0x20))
				{
					SetStatusIP(m_ClientIP, HZ_IPSTATUS_BLACK_PROT, 9000) ;
					return E_FORMAT ;
				}
			}

			if (m_pBuf)				m_pCx->m_Track.Printf("m_pBuf           = %s\n", m_pBuf) ;
			if (m_pAccept)			m_pCx->m_Track.Printf("m_pAccept        = %s\n", m_pAccept) ;
			if (m_pAcceptCharset)	m_pCx->m_Track.Printf("m_pAcceptCharset = %s\n", m_pAcceptCharset) ;
			if (m_pAcceptLang)		m_pCx->m_Track.Printf("m_pAcceptLang    = %s\n", m_pAcceptLang) ;
			if (m_pAcceptCode)		m_pCx->m_Track.Printf("m_pAcceptCode    = %s\n", m_pAcceptCode) ;
			if (m_pCacheControl)	m_pCx->m_Track.Printf("m_pCacheControl  = %s\n", m_pCacheControl) ;
			if (m_pConnection)		m_pCx->m_Track.Printf("m_pConnection    = %s\n", m_pConnection) ;
			if (m_pContentType)		m_pCx->m_Track.Printf("m_pContentType   = %s\n", m_pContentType) ;
			if (m_pETag)			m_pCx->m_Track.Printf("m_pETag          = %s\n", m_pETag) ;
			if (m_pPragma)			m_pCx->m_Track.Printf("m_pPragma        = %s\n", m_pPragma) ;
			if (m_pUserAgent)		m_pCx->m_Track.Printf("m_pUserAgent     = %s\n", m_pUserAgent) ;
			if (m_pProcessor)		m_pCx->m_Track.Printf("m_pProcessor     = %s\n", m_pProcessor) ;
			if (m_pVia)				m_pCx->m_Track.Printf("m_pVia           = %s\n", m_pVia) ;
			if (m_pCliIP)			m_pCx->m_Track.Printf("m_pCliIP         = %s\n", m_pCliIP) ;
			if (m_pHost)			m_pCx->m_Track.Printf("m_pHost          = %s\n", m_pHost) ;
			if (m_pXost)			m_pCx->m_Track.Printf("m_pXost          = %s\n", m_pXost) ;
			if (m_pFwrdIP)			m_pCx->m_Track.Printf("m_pFwrdIP        = %s\n", m_pFwrdIP) ;
			if (m_pProxIP)			m_pCx->m_Track.Printf("m_pProxIP        = %s\n", m_pProxIP) ;
			if (m_pServer)			m_pCx->m_Track.Printf("m_pServer        = %s\n", m_pServer) ;
			if (m_pFrom)			m_pCx->m_Track.Printf("m_pFrom          = %s\n", m_pFrom) ;
			if (m_pReferer)			m_pCx->m_Track.Printf("m_pReferer       = %s\n", m_pReferer) ;
			if (m_pReqPATH)			m_pCx->m_Track.Printf("m_pReqPATH       = %s\n", m_pReqPATH) ;
			if (m_pReqFRAG)			m_pCx->m_Track.Printf("m_pReqFRAG       = %s\n", m_pReqFRAG) ;


			SendError(HTTPMSG_NOTFOUND, "SORRY! INTERNAL ERROR\n") ;
			return E_FORMAT ;
		}

		if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
		{
			m_pCx->m_Track.Printf("%s: %s Sock %d/%d REQUEST [\n", *_fn, *m_Occur.Str(), m_pCx->CliSocket(), m_pCx->CliPort()) ;
			m_pCx->m_Track << ZI ;
			m_pCx->m_Track << "]\n" ;
		}
	}

	//	Header complete
	m_bHdrComplete = true ;

	//	The header has been processed and the content-length is known. The whole hit may not be in but we can test the header.


	//m_Resource.UrlDecode() ;

stage_two:
	//	We now can test that the hit has been sent in full
	if (ZI.Size() < (m_nHeaderLen + m_nContentLen))
	{
		m_pCx->m_Track.Printf("%s. Not recv in full. Hdr %d, cont %d, actual %d\n", *_fn, m_nHeaderLen, m_nContentLen, ZI.Size()) ;
		return E_OK ;
	}

	if (ZI.Size() > (m_nHeaderLen + m_nContentLen))
		m_pCx->m_Track.Printf("%s. Msg over: Hdr %d, cont %d, actual %d\n", *_fn, m_nHeaderLen, m_nContentLen, ZI.Size()) ;
	m_bMsgComplete = true ;

	//	Obtain POST data if applicable
	zi = ZI ;
	zi += m_nHeaderLen ;

	if (m_eMethod == HTTP_POST)
	{
		if (m_pContentType)
		{
			for (n = 0, i = m_pContentType ; *i ; i++)
			{
				if (*i == CHAR_SCOLON)
					{ n = 1 ; continue ; }

				if (n)
				{
					if (!memcmp(i, "boundary=", 9))
						{ i += 9 ; boundary = i ; break ; }
				}
			}
		}

		if (!boundary)
			_setnvpairs(zi) ;
		else
		{
			for (; !zi.eof() ;)
			{
				if (zi != boundary)
					{ zi++ ; continue ; }

				/*
				**	In the general case, data occurs in the form:-
				**		boundary\nContent-Disposition: form-data; name="fldname"
				**		data
				**		blank line.
				**	Note that in the empty field case, there are two blank lines (3 \r\n sequences)
				**
				**	In the file upload case in the form:-
				**		boundary\nContent-Disposition: form-data; name="fldname"; filename="filename"
				**		Content-Type: ...
				**		blank line
				**		filedata
				**	Note that the filedata is terminated by the appearence of the boundary on a line by itself and that the whole submission is
				**	terminated by the boundary followed directly by two minus signs.
				*/

				zi += boundary.Length() ;
				if (zi == "--")
					break ;

				zi.Skipwhite() ;

				if (zi != "Content-Disposition: form-data; name=\"")
				{
					m_pCx->m_Track.Printf("Malformed multipart form submission (case 1)\n") ;
					for (; !zi.eof() && *zi != CHAR_NL ; zi++) ;
					break ;
				}

				//	Get name part of name-value pair
				for (zi += 38 ; !zi.eof() && *zi != CHAR_DQUOTE ; zi++)
					Word.AddByte(*zi) ;
				zi++ ;
				Pair.name = Word ;
				Word.Clear() ;

				//	Get value part of name-value pair
				if (*zi == CHAR_SCOLON)
				{
					//	Now expect [; filename="filename"]
					if (zi != "; filename=\"")
					{
						m_pCx->m_Track.Printf("Malformed multipart form submission (case 2)\n") ;
						for (; !zi.eof() && *zi != CHAR_NL ; zi++) ;
						break ;
					}

					for (zi += 12 ; !zi.eof() && *zi != CHAR_DQUOTE ; zi++)
						Word.AddByte(*zi) ;
					zi++ ;
					Pair.value = Word ;
					Word.Clear() ;

					upload.m_fldname = Pair.name ;
					upload.m_filename = Pair.value ;

					//	Now get file content
					zi.Skipwhite() ;
					if (zi != "Content-Type: ")
					{
						m_pCx->m_Track.Printf("%s. Expected Content-Type for submitted file\n", *_fn) ;
						break ;
					}
					for (zi += 14 ; !zi.eof() && *zi != CHAR_NL ; zi++)
					{
						if (*zi == CHAR_CR)
							continue ;
						if (*zi == CHAR_NL)
							break ;
						Word.AddByte(*zi) ;
					}
					S = Word ;
					Word.Clear() ;
					upload.m_mime = Str2Mimetype(S) ;
					if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
						threadLog("%s. MIME type of %s is %d (%s)\n", *_fn, *Pair.value, upload.m_mime, *S) ;

					zi++ ;
					if (zi == "\r\n")
						zi += 2 ;

					for (; !zi.eof() ; zi++)
					{
						if (zi == "\r\n--")
						{
							zi += 4 ;
							if (zi == boundary)
								break ;
							upload.m_file << "\r\n--" ;
							continue ;
						}
						upload.m_file.AddByte(*zi) ;
					}
					//m_Files.Add(W) ;
					m_Inputs.Add(Pair) ;
					m_mapStrings.Insert(Pair.name, Pair.value) ;
					m_pCx->m_Track.Printf("%s. Field name/value %s=%s\n", *_fn, *Pair.name, *Pair.value) ;
					m_Uploads.Insert(upload.m_fldname, upload) ;
					m_pCx->m_Track.Printf("%s. Got file of %d bytes\n", *_fn, upload.m_file.Size()) ;
				}
				else
				{
					//	Expect \r\n then a line of data terminated by \r\n\r\n
					if (*zi != CHAR_CR)
						m_pCx->m_Track.Printf("%s. Warning fld not at CR, char=%c instead\n", *_fn, *zi) ;

					zi.Skipwhite() ;
					for (; !zi.eof() ; zi++)
					{
						if (zi == "\r\n")
						{
							zi += 2 ;
							if (zi == "--")
							{
								zi += 2 ;
								if (zi == boundary)
									break ;
							}

							Word.AddByte(CHAR_NL) ;
							continue ;
						}
						Word.AddByte(*zi) ;
					}
					Pair.value = Word ;
					Word.Clear() ;
					m_Inputs.Add(Pair) ;
					m_mapStrings.Insert(Pair.name, Pair.value) ;
					m_pCx->m_Track.Printf("%s. Field name/value %s=%s\n", *_fn, *Pair.name, *Pair.value) ;
				}
			}
		}
	}

	return E_OK ;
}

hzEcode	hzHttpEvent::Storeform	(const char* cpPath)
{
	//	Appends submitted forms to a file of the supplied pathname. This is quite separate from any processing of the form by the application.
	//	This can be convient for diagnostics or even serve as a rudimantry form of backup.
	//
	//	Arguments:	1)	cpPath	Filename to store form submissions
	//
	//	Returns:	E_ARGUMENT	If the pathname is not supplied
	//				E_OPENFAIL	If the store form file cannot be opened for writing
	//				E_WRITEFAIL	If a write error occurs
	//				E_OK		If the form submission was stored

	_hzfunc("hzHttpEvent::Storeform") ;

	static	char cvLine [80] ;	//	Buffer for time stamp and field id

	std::ofstream	os ;		//	Output file for form submission storage

	hzPair		P ;				//	Field name/value pair
	hzXDate		now ;			//	System time stamp
	uint32_t	nIndex ;		//	Field iterator

	if (!cpPath || !cpPath[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No pathname supplied\n") ;

	os.open(cpPath, std::ios::app) ;
	if (os.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open file (%s) for writing\n", cpPath) ;

	now.SysDateTime() ;
	sprintf(cvLine, "@Date: %04d%02d%02d\n", now.Year(), now.Month(), now.Day()) ;
	os << cvLine ;

	sprintf(cvLine, "@Time: %02d%02d%02d\n", now.Hour(), now.Min(), now.Sec()) ;
	os << cvLine ;

	for (nIndex = 0 ; GetAt(P, nIndex) == E_OK ; nIndex++)
	{
		if (P.name && P.value)
			os << "@" << P.name << ":\t" << P.value << "\n" ;

		if (os.fail())
		{
			os.close() ;
			return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "_storeform: Write error on file (%s)\n", cpPath) ;
		}
	}

	os << "@end:\n\n" ;
	os.close() ;
	return E_OK ;
}

hzEcode	_storeform	(hzHttpEvent* pE, FILE* fp)
{
	//	This stores forms submitted by HTTP clients in the supplied file. This is essentially for diagnostics and more formal mathods
	//	are recomended for the operation of an Internet based application.
	//
	//	Arguments:	1)	cpPath	Filename to store form submissions
	//				2)	fp		FILE pointer (to open file)
	//
	//	Returns:	E_NOTOPEN	If the store form file pointer is not supplied
	//				E_WRITEFAIL	If a write error occurs
	//				E_OK		If the form submission was stored

	_hzfunc("_storeform") ;

	hzPair		P ;				//	Field name/value pair
	hzXDate		now ;			//	System time stamp
	uint32_t	nIndex ;		//	Field iterator

	if (!fp)
		return E_NOTOPEN ;

	now.SysDateTime() ;
	if (fprintf(fp, "%04d%02d%02d-%02d%02d%02d\n", now.Year(), now.Month(), now.Day(), now.Hour(), now.Min(), now.Sec()) == -1)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "_storeform: Write error on file (1)") ;

	for (nIndex = 1 ; pE->GetAt(P, nIndex) == E_OK ; nIndex++)
	{
		if (fprintf(fp, "@%s:\t%s\n", *P.name, *P.value) == -1)
			return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "_storeform: Write error on file (2)") ;
	}

	fflush(fp) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::SetHdr	(const hzString& name, const hzString& value)
{
	//	Set a variable to be transmitted as a name/value pair in the header of the HTTP response. With suitable JavaScript in the response page or article, this
	//	variable can influence what is displayed.
	//
	//	This allows a page that is otherwise fixed, to have different manifestations to different users. Normally, the only way to do this is by having the page
	//	generated each time it is requested. A fixed content page or article can be pre-zipped so is quicker to serve and consumes less bandwidth.
	//
	//	Arguments:	1)	name	The header parameter name.
	//				2)	value	The header parameter value.
	//
	//	Returns:	E_ARGUMENT	If provided with blank variable name.
	//				E_NODATA	If no value is supplied..
	//				E_OK		If operation successful.

	_hzfunc("hzHttpEvent::SetHdr") ;

	hzPair	p ;		//	Header name/value pair

	if (!name)
		return E_ARGUMENT ;
	if (!value)
		return E_NODATA ;

	p.name = name ;
	p.value = value ;
	m_HdrsResponse.Add(p) ;

	return E_OK ;
}

hzEcode	hzHttpEvent::SetVarString	(const hzString& name, const hzString& value)
{
	//	Sets a string value to the supplied value and places it in the hzHttpEvent's map of string values. Note that if there is a chain value
	//	of the same name, this is not allowed.
	//
	//	Note that the hzHttpEvent is very short lived. An instance is populated by a HTTP request, passed to the event handler ProcHTTP(), which processes it to
	//	formulate a response. Then one of its member functions is called to write out that response to the client socket. The only purpose in setting a value in
	//	the event's m_mapStrings member, is to influence the output by means of a percent-entity of the form [%e:var_name;] that is assumed to exist in the page
	//	template being served.
	//
	//	Arguments:	1)	name	The variable name.
	//				2)	value	The variable value as a string
	//
	//	Returns:	E_ARGUMENT	If the name is not supplied
	//				E_DUPLICATE	If the name is already used
	//				E_OK		If the variable was added

	_hzfunc("hzHttpEvent::SetVariable[2]") ;

	if (!name)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Blank variable names are not allowed") ;

	if (m_mapStrings.Exists(name))
		m_mapStrings[name] = value ;
	else
	{
		if (m_mapChains.Exists(name))
			return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Cannot assign value to an existing chain") ;

		if (m_mapStrings.Insert(name, value) != E_OK)
			return hzerr(_fn, HZ_ERROR, E_MEMORY, "Could not insert variable %s", *name) ;
	}

	return E_OK ;
}

hzEcode	hzHttpEvent::SetVarChain	(const hzString& name, const hzChain& Z)
{
	//	Sets a chain value to the supplied chain and places it in the hzHttpEvent's map of chain values. Note that if there is a string value
	//	of the same name, this is not allowed.
	//
	//	Note that the hzHttpEvent is very short lived. An instance is populated by a HTTP request, passed to the event handler ProcHTTP(), which processes it to
	//	formulate a response. Then one of its member functions is called to write out that response to the client socket. The only purpose in setting a value in
	//	the event's m_mapChains member, is to influence the output by means of a percent-entity of the form [%e:var_name;] that is assumed to exist in the page
	//	template being served.
	//
	//	Arguments:	1)	name	The variable name.
	//				2)	value	The variable value as a string
	//
	//	Returns:	E_ARGUMENT	If the name is not supplied
	//				E_DUPLICATE	If the name is already used
	//				E_OK		If the variable was added

	_hzfunc("hzHttpEvent::SetVar(chain)") ;

	if (!name)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Blank variable names are not allowed") ;

	if (m_mapChains.Exists(name))
	{
		m_mapChains[name].Clear() ;
		m_mapChains[name] = Z ;
	}
	else
	{
		if (m_mapStrings.Exists(name))
			return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Cannot assign value to an existing string") ;

		if (m_mapChains.Insert(name, Z) != E_OK)
			return hzerr(_fn, HZ_ERROR, E_MEMORY, "Could not insert variable %s", *name) ;
	}

	return E_OK ;
}

hzEcode	hzHttpEvent::GetVar	(hzChain& Z, const hzString& name)
{
	//	Append the supplied chain assumed to be HTML output, with the value found in the variable of the supplied name
	//
	//	Arguments:	1)	Z		The chain to be aggregated with the variable's value
	//				2)	name	Of the variable to evaluate
	//
	//	Returns:	E_ARGUMENT	If a variable name is not supplied
	//				E_NOTFOUND	If the named varaible is not found in the event handler's maps
	//				E_OK		If the named variable is appended to the HTML output

	_hzfunc("hzHttpEvent::GetVar") ;

	if (!name)
		return E_ARGUMENT ;

	if (m_mapStrings.Exists(name))	{ Z << m_mapStrings[name] ; return E_OK ; }
	if (m_mapChains.Exists(name))	{ Z << m_mapChains[name] ; return E_OK ; }

	return E_NOTFOUND ;
}

hzEcode	hzHttpEvent::_formhead	(hzChain& Z, HttpRC hrc, hzMimetype mtype, uint32_t nSize, uint32_t nExpires, bool bZip)
{
	//	Formulate HTTP header for outgoing response.
	//
	//	Arguments:	1) Z			The chain to aggregate to
	//				2) hrc			The HTTP return code
	//				3) mtype		The MIME type
	//				4) nSize		The size (content length)
	//				5) nExpires		The number of seconds the page should be considered valid for by the browser (if any)
	//				6) bZip			A boolean directive to state if the yet to be attached content is zipped
	//
	//	Returns:	E_ARGUMENT	If sent an invalid HTTP return code
	//				E_OK			If the operation was successful

	_hzfunc("hzHttpEvent::_formhead") ;

	hzXDate		now ;	//	For header dates
	hzString	S ;		//	Temp string

	Z.Clear() ;

	switch	(hrc)
	{
	case HTTPMSG_OK:						Z << "HTTP/1.1 200 OK\r\n" ;						break ;
	case HTTPMSG_NOCONTENT:					Z << "HTTP/1.1 204 OK\r\n" ;						break ;

	//	Request is OK but no page supplied because ...
	case HTTPMSG_REDIRECT_PERM:				Z << "HTTP/1.1 301 Temp Redirect\r\n" ;				break ;
	case HTTPMSG_FOUND_GOTO:				Z << "HTTP/1.1 301 Found\r\n" ;						break ;
	case HTTPMSG_NOT_MODIFIED:				Z << "HTTP/1.1 304 Not Modified\r\n" ;				break ;
	case HTTPMSG_REDIRECT_TEMP:				Z << "HTTP/1.1 307 Temp Redirect\r\n" ;				break ;

	//	Errors in Request (not found or otherwise denied)
	case HTTPMSG_BAD_REQUEST:				Z << "HTTP/1.1 400 Bad Request\r\n" ;				break ;
	case HTTPMSG_UNAUTHORIZED:				Z << "HTTP/1.1 401 Unauthorized\r\n" ;				break ;
	case HTTPMSG_FORBIDDEN:					Z << "HTTP/1.1 403 Forbidden\r\n" ;					break ;
	case HTTPMSG_NOTFOUND:					Z << "HTTP/1.1 404 Not found\r\n" ;					break ;
	case HTTPMSG_METHOD_NOT_ALLOWED:		Z << "HTTP/1.1 405 Not Allowed\r\n" ;				break ;
	case HTTPMSG_REQUEST_TIME_OUT:			Z << "HTTP/1.1 408 Request Timed Out\r\n" ;			break ;
	case HTTPMSG_GONE:						Z << "HTTP/1.1 410 Page Gone\r\n" ;					break ;
	case HTTPMSG_LENGTH_REQUIRED:			Z << "HTTP/1.1 411 Length Required\r\n" ;			break ;
	case HTTPMSG_PRECONDITION_FAILED:		Z << "HTTP/1.1 412 Precondition Failed\r\n" ;		break ;
	case HTTPMSG_ENTITY_TOO_LARGE:			Z << "HTTP/1.1 413 Entity Too Large\r\n" ;			break ;
	case HTTPMSG_REQUEST_URI_TOO_LARGE:		Z << "HTTP/1.1 414 URI Too Large\r\n" ;				break ;
	case HTTPMSG_UNSUPPORTED_MEDIA_TYPE:	Z << "HTTP/1.1 415 Unsupported Media Type\r\n" ;	break ;

	//	System Errors. Can't help you regardless of how reasonable the request!
	case HTTPMSG_INTERNAL_SERVER_ERROR:		Z << "HTTP/1.1 500 Internal Server Error\r\n" ;		break ;
	case HTTPMSG_NOT_IMPLEMENTED:			Z << "HTTP/1.1 501 Not Implimented\r\n" ;			break ;
	case HTTPMSG_BAD_GATEWAY:				Z << "HTTP/1.1 502 Bad Gateway\r\n" ;				break ;
	case HTTPMSG_SERVICE_UNAVAILABLE:		Z << "HTTP/1.1 503 Service Unavailable\r\n" ;		break ;
	case HTTPMSG_VARIANT_ALSO_VARIES:		Z << "HTTP/1.1 506 Variant Also Varies\r\n" ;		break ;

	default:
		m_Error.Printf("Invalid HTTP return code (%d)\n", (uint32_t) hrc) ;
		return E_ARGUMENT ;
	} ;

	now.SysDateTime() ;
	S = now.Str(FMT_DT_INET) ;

	if (!_hzGlobal_runstart)
		_hzGlobal_runstart = S ;

	Z << "Date: " << S << "\r\n" ;
	Z << "Server: HTTP/1.1 (HadronZoo::Dissemino 9.7, Linux)\r\n" ;
	Z << "Last-Modified: " << _hzGlobal_runstart << "\r\n" ;

	if (!nExpires)
	{
		Z << "Pragma: No-cache\r\n" ;
		Z << "Cache-Control: no-cache\r\n" ;
		Z << "Expires: 0\r\n" ;
	}
	else
	{
		now.altdate(SECOND, nExpires) ;
		S = now.Str(FMT_DT_INET) ;
		Z.Printf("Expires: %s\r\n", *S) ;
	}
	now.altdate(SECOND, -10000000) ;

	//	If there is a cookie being sent by the browser which is no longer in use this is deleted by DelSessCookie() which sets m_CookieOld to the redundant
	//	submited cookie. Thus if m_CookieOld is set we tell the browser to delete it here
	if (m_CookieOld)
	{
		if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
			threadLog("Expiring old cookie %016X\n", m_CookieOld) ;
		//Z.Printf("Set-Cookie: %s=%016X; path=/; expires: %s\r\n", *_hzGlobal_HtmlApp->m_CookieName, m_CookieOld, *now.Str(FMT_DT_INET)) ;
		Z.Printf("Set-Cookie: %s=%016X; path=/; expires: %s\r\n", *_hzGlobal_Dissemino->m_CookieName, m_CookieOld, *now.Str(FMT_DT_INET)) ;
	}

	//	Send a new session cookie
	if (m_CookieNew)
	{
		//	If there is a cookie from the browser and it does not agree with a new cookie from the server, give the directive to the browser to delete it.
		if (m_CookieSub && (m_CookieNew != m_CookieSub))
		{
			if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
				threadLog("Expiring cookie %016X and setting cookie %016x\n", m_CookieSub, m_CookieNew) ;
			//Z.Printf("Set-Cookie: %s=%016X; path=/; expires: %s\r\n", *_hzGlobal_HtmlApp->m_CookieName, m_CookieOld, *now.Str(FMT_DT_INET)) ;
			Z.Printf("Set-Cookie: %s=%016X; path=/; expires: %s\r\n", *_hzGlobal_Dissemino->m_CookieName, m_CookieOld, *now.Str(FMT_DT_INET)) ;
		}
		//Z.Printf("Set-Cookie: %s=%016X; path=/\r\n", *_hzGlobal_HtmlApp->m_CookieName, m_CookieNew) ;
		Z.Printf("Set-Cookie: %s=%016X; path=/\r\n", *_hzGlobal_Dissemino->m_CookieName, m_CookieNew) ;
	}

	//	Send the content location if app has set URI
	if (m_Redirect)
	{
		if (hrc == HTTPMSG_FOUND_GOTO)
			Z.Printf("Location: %s\r\n", *m_Redirect) ;
		else
			Z.Printf("Content-Location: %s\r\n", *m_Redirect) ;
	}

	Z << "Accept-Ranges: bytes\r\n" ;

	if (bZip)
		Z << "Content-Encoding: gzip\r\n" ;
	Z.Printf("Content-Length: %d\r\n", nSize) ;

	if (m_LangCode)
		Z.Printf("Content-Language: %s\r\n", *m_LangCode) ;
	else
		Z.Printf("Content-Language: en-US\r\n") ;

	if (m_HdrsResponse.Count())
	{
		hzList<hzPair>::Iter	ip ;	//	Resident headers iterator

		for (ip = m_HdrsResponse ; ip.Valid() ; ip++)
		{
			Z << ip.Element().name ;
			Z << ": " ;
			Z << ip.Element().value ;
			Z << "\r\n" ;
		}
	}

	if (!m_nConnection)
		Z << "Connection: close\r\n" ;
	else
	{
		//	Z.Printf("keep-alive: timeout=%d\r\n", m_nConnection) ;
		Z << "Connection: close\r\n" ;
		//	Z << "Connection: keep-alive\r\n" ;
	}

	Z << "Content-Type: " << Mimetype2Txt(mtype) << "\r\n" ;
	Z << "X-Powered-By: HadronZoo::Dissemino 9.6\r\n\r\n" ;

	if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
		threadLog(Z) ;

	return E_OK ;
}

hzEcode	hzHttpEvent::SendRawChain	(HttpRC hrc, hzMimetype type, const hzChain& Data, uint32_t nExpires, bool bZip)
{
	//	Compile a send a HTML response to the HTTP client. The HTML page content is supplied as a hzChain
	//
	//	Arguments:	1)	hrc			The HTTP return code to appear in the header.
	//				2)	type		The MIME type of HTTP message
	//				3)	Data		The page content
	//				4)	nExpires	The expiry time for the page
	//				5)	bZip		A boolean flag to indicate if the content has been zipped. It sets an indicator in the outgoing header.
	//
	//	Returns:	E_ARGUMENT	If any of the arguments are invalid
	//				E_WRITEFAIL	If the HTTP response could not be sent to the browser.
	//				E_OK		If the operation was successful.
	//

	_hzfunc("hzHttpEvent::SendRawChain") ;

	hzChain	Z ;		//	For building header
	hzEcode	rc ;	//	Return code

	rc = _formhead(Z, hrc, type, Data.Size(), nExpires, bZip) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not formulate HTTP header (sock=%d)", m_pCx->CliSocket()) ;

	//Z << Data ;
	if (m_pCx->SendData(Z, Data) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Event %p Failed to send response (size=%d + %d, sock=%d)", this, Z.Size(), Data.Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::SendRawString	(HttpRC hrc, hzMimetype type, const hzString& Content, uint32_t nExpires, bool bZip)
{
	//	Compile a send a HTML response to the HTTP client. The HTML page content is supplied as a hzString
	//
	//	Arguments:	1)	hrc			The HTTP return code to appear in the header.
	//				2)	type		The MIME type of HTTP message
	//				3)	Contyent	The page content
	//				4)	nExpires	The expiry time for the page
	//				5)	bZip		A boolean flag to indicate if the content has been zipped. It sets an indicator in the outgoing header.
	//
	//	Returns:	E_ARGUMENT	If any of the arguments are invalid
	//				E_WRITEFAIL	If the HTTP response could not be sent to the browser.
	//				E_OK		If the operation was successful.
	//
	//	Note:		This function is deprecated. Please use hzHttpEvent::SendRawChain instead

	_hzfunc("hzHttpEvent::SendRawString") ;

	hzChain	Z ;		//	For building header
	hzEcode	rc ;	//	Return code

	rc = _formhead(Z, hrc, type, Content.Length(), nExpires, bZip) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not formulate HTTP header (sock=%d)", m_pCx->CliSocket()) ;

	Z << Content ;

	if (m_pCx->SendData(Z) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "hzHttpEvent %p Failed to send response (size=%d, sock=%d)", this, Z.Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::SendFilePage	(const char* cpDir, const char* cpFilename, uint32_t nExpires, bool bZip)
{
	//	This sends a file assumed to be a whole HTML page. The HTML must not contain a server side include as there is no processing in this function to detect
	//	any such construct. The page may of course contain links to other resources as a means to complete rendering by the browser.
	//
	//	Arguments:	1)	cpDir		The directory of the file (can be relative to current dir)
	//				2)	cpFilename	The file name.
	//				3)	nExpires	Expire time for page (for browser use only)
	//				4)	bZip		Flag to indicate if the content is to be zipped
	//
	//	Returns:	E_NOTFOUND	If the directory or filename cannot be accessed.
	//				E_OPENFAIL	If the file could not be opened.
	//				E_NODATA	If the file is empty.
	//				E_WRITEFAIL	If the HTML data could not be sent to the browser.
	//				E_OK		If the operation was successful.
	//
	//	Note this function does not use OpenInputStrm to open the file as it does not need the error code detail.

	_hzfunc("hzHttpEvent::SendFilePage") ;

	ifstream		is ;			//	Read file stream
	FSTAT			fs ;			//	File info
	hzChain			Z ;				//	Response for browser is built here
	const char*		pEnd ;			//	Filename extension and hence type
	hzString		Pagename ;		//	Full name (inc path) of page
	hzMimetype		type ;			//	File's HTTP type
	hzEcode			rc ;			//	Return code from sending function

 	//	Formulate full path of page and then use lstat

	if (cpDir && cpDir[0])
		{ Z << cpDir ; Z.AddByte(CHAR_FWSLASH) ; }

	if (!cpFilename || !cpFilename[0] || (cpFilename[0] == CHAR_FWSLASH && cpFilename[1] == 0))
		Z += "index.html" ;
	else
		Z += cpFilename ;

	Pagename = Z ;
	Z.Clear() ;

	if (lstat(*Pagename, &fs) == -1)
	{
		SendError(HTTPMSG_NOTFOUND, "Could not locate %s\n", *Pagename) ;
		return E_NOTFOUND ;
	}

	//	Determine the type of file so that the correct header can be sent to the browser

	pEnd = strrchr(*Pagename, CHAR_PERIOD) ;
	if (!pEnd)
		type = HMTYPE_TXT_PLAIN ;
	else
		type = Filename2Mimetype(pEnd) ;

	//	Read in the file

	is.open(*Pagename) ;
	if (is.fail())
	{
		SendError(HTTPMSG_NOTFOUND, "Could not open requested file (%s\n", *Pagename) ;
		return E_OPENFAIL ;
	}
	Z += is ;
	is.close() ;

	if (!Z.Size())
	{
		SendError(HTTPMSG_NOTFOUND, "File (%s) of zero size\n", *Pagename) ;
		return E_NODATA ;
	}

	//	If not a html file, no server side includes are possible so just send
	rc = SendRawChain(HTTPMSG_OK, type, Z, nExpires, bZip) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Response data not sent to browser (sock=%d)", m_pCx->CliSocket()) ;

	return E_OK ;
}

hzEcode	hzHttpEvent::SendFileHead	(const char* cpDir, const char* cpFilename, uint32_t nExpires)
{
	//	Sends a HTML or other file to the browser
	//
	//	Arguments:	1)	cpDir		The directory of the file (can be relative to current dir)
	//				2)	cpFilename	The file name.
	//				3)	nExpires	Expiry date
	//
	//	Returns:	E_NOTFOUND	If the directory or filename cannot be accessed.
	//				E_OPENFAIL	If the file could not be opened.
	//				E_NODATA	If the file is empty.
	//				E_WRITEFAIL	If the HTML data could not be sent to the browser.
	//				E_OK		If the operation was successful.

	_hzfunc("hzHttpEvent::SendFileHead") ;

	ifstream	is ;			//	Read file stream
	FSTAT		fs ;			//	File info
	hzXDate		d ;				//	Date for header lines
	hzChain		Z ;				//	Response for browser is built here
	const char*	pEnd ;			//	Filename extension and hence type
	hzString	Pathname ;		//	File to load
	uint32_t	nLen = 0 ;		//	File size
	hzMimetype	type ;			//	File's HTTP type
	HttpRC		hrc ;			//	HTTP return code
	hzEcode		rc ;			//	Return code from sending function

	//	Establish real filename, either cpFilename or index.htm(l)

	Pathname = cpDir ;

	if (cpFilename[0] == CHAR_FWSLASH && cpFilename[1] == 0)
		Pathname += "/index.html" ;
	else
	{
		if (cpFilename[0] == CHAR_FWSLASH)
			Pathname += cpFilename ;
		else
		{
			Pathname += "/" ;
			Pathname += cpFilename ;
		}
	}

	if (stat(*Pathname, &fs) == -1)
	{
		hrc = HTTPMSG_NOTFOUND ;
		nLen = 0 ;
	}
	else
	{
		hrc = HTTPMSG_OK ;
		nLen = fs.st_size ;
	}

	//	Determine the type of file so that the correct header can be
	//	sent to the browser

	pEnd = strrchr(*Pathname, CHAR_PERIOD) ;
	if (!pEnd)
		type = HMTYPE_TXT_PLAIN ;
	else
		type = Filename2Mimetype(pEnd) ;

	//	Send the header
	rc = _formhead(Z, hrc, type, nLen, nExpires, false) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not formulate HTTP header (sock=%d)", m_pCx->CliSocket()) ;

	if (m_pCx->SendData(Z) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "hzHttpEvent %p Failed to send response to browser (sock=%d)", this, m_pCx->CliSocket()) ;

	return E_OK ;
}

//	FnGrp:	hzHttpEvent::SendHttpHead
//
//	Sends a HTML header only, to the browser
//
//	Arguments:	1)	fixContent	The output string or chain
//				2)	type		The MIME type.
//				3)	nExpiry		The expiry interval
//
//	Returns:	E_NODATA	If the file is empty.
//				E_WRITEFAIL	If there was a transmission failure
//				E_OK		If the operation was successful.
//
//	Func:	hzHttpEvent::SendHttpHead(const hzString&,hzMimetype,uint32_t)
//	Func:	hzHttpEvent::SendHttpHead(const hzChain&,hzMimetype,uint32_t)

hzEcode	hzHttpEvent::SendHttpHead	(const hzString& fixContent, hzMimetype type, uint32_t nExpires)
{
	_hzfunc("hzHttpEvent::SendHttpHead(str)") ;

	hzChain	Z ;		//	Output chain (for head)
	hzEcode	rc ;	//	Return code

	if (!fixContent)
		return E_NODATA ;

	rc = _formhead(Z, HTTPMSG_OK, type, fixContent.Length(), nExpires, false) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not formulate HTTP header (sock=%d)", m_pCx->CliSocket()) ;

	if (m_pCx->SendData(Z) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "hzHttpEvent %p Failed to send response to browser (sock=%d)", this, m_pCx->CliSocket()) ;

	return rc ;
}

hzEcode	hzHttpEvent::SendHttpHead	(const hzChain& fixContent, hzMimetype type, uint32_t nExpires)
{
	_hzfunc("hzHttpEvent::SendHttpHead(ch)") ;

	hzChain	Z ;		//	Output chain (for head)
	hzEcode	rc ;	//	Return code

	if (!fixContent.Size())
		return E_NODATA ;

	rc = _formhead(Z, HTTPMSG_OK, type, fixContent.Size(), nExpires, false) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not formulate HTTP header (sock=%d)", m_pCx->CliSocket()) ;

	if (m_pCx->SendData(Z) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "hzHttpEvent %p Failed to send response to browser (sock=%d)", this, m_pCx->CliSocket()) ;

	return rc ;
}

hzEcode	hzHttpEvent::SendPageE	(const char* dir, const char* fname, uint32_t nExpires, bool bZip)
{
	//	Purpose:	Sends a HTML or other file to the browser but from memory. The first time the file is requested it is loaded into
	//				memory and then sent. On subsequent requests it is served from memory.
	//
	//	Arguments:	1)	dir			The directory of the file (can be relative to current dir)
	//				2)	fname		The file name.
	//				3)	nExpires	Expire time for page (for browser use only)
	//				4)	bZip		Flag to indicate if the content is to be zipped
	//
	//	Returns:	E_NOTFOUND	If the directory or filename cannot be accessed.
	//				E_OPENFAIL	If the file could not be opened.
	//				E_NODATA	If the file is empty.
	//				E_WRITEFAIL	If the HTML data could not be sent to the browser.
	//				E_OK		If the operation was successful.
	//
	//	Note this function does not use OpenInputStrm to open the file as it does not need the error code detail.

	_hzfunc("hzHttpEvent::SendPageE") ;

	hzChain*		pChain ;		//	Response for browser is built here
	const char*		pEnd ;			//	Filename extension and hence type
	hzString		Pagename ;		//	File to load
	hzString		Filename ;		//	File to load
	hzString		Content ;		//	Page content
	hzMimetype		type ;			//	File's HTTP type
	hzEcode			rc ;			//	Return code from sending function

	if (!dir || !dir[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No directory supplied") ;
	if (!fname || !fname[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No filename supplied") ;

	//	Lookup resource in page store

	//	Establish real filename, either fname or index.htm(l)

	if (fname[0] != CHAR_FWSLASH)
		Filename = fname ;
	else
	{
		if (fname[1] == 0)
			Filename = "index.html" ;
		else
			Filename = fname + 1 ;
	}

	Pagename = dir ;
	Pagename += "/" ;
	Pagename += Filename ;

	if (s_PageStore.Exists(Pagename))
	{
		pChain = s_PageStore[Pagename] ;
		if (!pChain)
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Null entry in stored page for %s\n", *Pagename) ;
	}
	else
	{
		//	Check directory and filename of resource

		ifstream	is ;			//	Read file stream
		FSTAT		fs ;			//	File info

		if (stat(*Pagename, &fs) == -1)
		{
			//return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "could not locate (%s)", *Pagename) ;

			SendError(HTTPMSG_NOTFOUND, "Could not locate %s\n", *Pagename) ;
			return E_NOTFOUND ;
		}

		if (fs.st_size == 0)
		{
			//return hzerr(_fn, HZ_ERROR, E_NODATA, "File (%s) of zero size!", *Pagename) ;

			SendError(HTTPMSG_NOTFOUND, "No content available for file %s\n", *Pagename) ;
			return E_NODATA ;
		}

		pChain = new hzChain() ;

		//	Read in the file

		is.open(*Pagename) ;
		if (is.fail())
			return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open requested file (%s)", *Pagename) ;

		*pChain += is ;
		is.close() ;

		if (pChain->Size() == 0)
			return hzerr(_fn, HZ_ERROR, E_MEMORY, "Could not load file (%s)", *Pagename) ;

		//	Check chain has loaded

		if (s_PageStore.Insert(Pagename, pChain) != E_OK)
			return hzerr(_fn, HZ_ERROR, E_MEMORY, "Could not store file (%s)", *Pagename) ;
	}

	//	Determine the type of file so that the correct header can be
	//	sent to the browser

	pEnd = strrchr(*Filename, CHAR_PERIOD) ;
	if (!pEnd)
		type = HMTYPE_TXT_PLAIN ;
	else
		type = Filename2Mimetype(pEnd) ;

	//	If not a html file, no server side includes are possible so just send
	rc = SendRawChain(HTTPMSG_OK, type, *pChain, nExpires, bZip) ;

	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Response data not sent to browser (size=%d, sock=%d)", pChain->Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::Redirect	(const hzUrl& url, uint32_t nExpires, bool bZip)
{
	//	Send a temporary redirection to the browser
	//
	//	Arguments:	1)	url			The redirection URL
	//				2)	nExpires	Expire time for page (for browser use only)
	//				3)	bZip		Flag to indicate if the content is to be zipped
	//
	//	Returns:	E_WRITEFAIL	If the error message could not be sent
	//				E_OK		If the operation was successful.

	_hzfunc("hzHttpEvent::Redirect") ;

	hzChain		Z ;		//	Output chain

	m_Redirect = url ;
 
	Z << "<html>\n<head>\n<title>Moved</title>\n" ;
	Z << "</head>\n<body>\n<h1>Moved</h1>\n" ;
	Z.Printf("<p>You are being redirected. Please <a href=\"%s\">click here</a></p>\n", *url) ;
	Z << "</body>\n</html>\n" ;

	if (SendRawChain(HTTPMSG_FOUND_GOTO, HMTYPE_TXT_HTML, Z, nExpires, bZip) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Response data not sent (size=%d, sock=%d)", Z.Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::SendNotFound	(hzUrl& url)
{
	//	Purpose:	Send a 404 not found message to browser. This message will be a default message if the global variable g_WebPageNotFound is not
	//				set within an application.
	//
	//	Arguments:	1)	url		The resource (page) that was not found
	//
	//	Returns:	E_WRITEFAIL	If the error message could not be sent
	//				E_OK		If the operation was successful.

	_hzfunc("hzHttpEvent::SendNotFound") ;

	hzChain	Z ;		//	Output chain

	Z <<
	"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 5.0//EN\">\n"
	"<html><head>\n"
	"<title>404 Not Found</title>\n"
	"</head><body>\n"
	"<h1>Not Found</h1>\n"
	"<p>The requested URL " << url << " was not found on this server.</p>\n"
	"<hr>\n" ;
	Z.Printf("<address>HadronZoo Internet: %s Port %d</address>\n", *url.Domain(), url.Port()) ;
	Z << "</body></html>\n" ;

	if (SendRawChain(HTTPMSG_NOTFOUND, HMTYPE_TXT_HTML, Z, 0, false) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Response data not sent (size=%d, sock=%d)", Z.Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::SendError	(HttpRC hrc, const char* va_alist ...)
{
	//	Purpose:	Send a general error message to browser
	//
	//	Arguments:	1)	hrc			HTTP return code
	//				2)	va_alist	Varaiable printf style arguments
	//
	//	Returns:	E_WRITEFAIL	If the error message could not be sent
	//				E_OK		If the operation was successful.

	_hzfunc("hzHttpEvent::SendError") ;

	va_list		ap ;	//	Variable argument list
	const char*	fmt ;	//	Format control
	hzChain		Z ;		//	Output chain

	va_start(ap, va_alist) ;
	fmt = va_alist ;

	Z << "<html>\n<head>\n<title>HadronZoo Internet Error Report</title>\n\n" ;
	Z << "<style type=\"text/css\">\n" ;
	Z << "<!--\n" ;
	Z << ".a1 {text-decoration:none; font-family:arial; font-size:24px; font-weight:bold; color:#FFFFFF;}\n" ;
	Z << ".a2 {text-decoration:none; font-family:verdana; font-size:13px; font-weight:bold; color:#000000;}\n" ;
	Z << ".a3 {text-decoration:none; font-family:verdana; font-size:12px; font-weight:bold; color:#000000;}\n" ;
	Z << "-->\n" ;
	Z << "</style>\n" ;
	Z << "</head>\n" ;
	Z += "<body bgcolor=#CCCCCC>\n" ;

	Z << "<table width=100% border=0 cellspacing=0 cellpadding=0>\n" ;
	Z << "<tr height=100 bgcolor=#000000>\n" ;
	Z << " <td align=center class=a1>HadronZoo Internet Error Report<td>\n" ;
	Z << "</tr>\n" ;
	Z << "<tr height=400 bgcolor=#F0FFF0>\n" ;

	Z << " <td align=center class=a2>" ;
	Z._vainto(fmt, ap) ;
	Z << "</td>\n" ;

	Z << "</tr>\n" ;
	Z << "<tr height=50 bgcolor=#CCCCCC>\n" ;
	Z << " <td align=center class=a3>Powered by HadronZoo</td>\n" ;
	Z << "</tr>\n" ;
	Z << "</table>\n" ;

	Z << "</body>\n" ;
	Z << "</html>\n" ;

	if (SendRawChain(hrc, HMTYPE_TXT_HTML, Z, 0, false) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Response data not sent (size=%d, sock=%d)", Z.Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::SendAjaxResult (HttpRC hrc)
{
	//	Send a HTTP response code only.
	//
	//	Note: This function should only be invoked in response to AJAX HTTP requests
	//
	//	Arguments:	1)	hrc			HTTP return code
	//
	//	Returns:	E_WRITEFAIL	If the response could not be sent
	//				E_OK		If the AJAX result was sent

	_hzfunc("hzHttpEvent::SendCmdResult(1)") ;

	hzChain	Z ;		//	For building header
	hzEcode	rc ;	//	Return code

	rc = _formhead(Z, hrc, HMTYPE_TXT_HTML, 0, 0, false) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not formulate HTTP header (sock=%d)", m_pCx->CliSocket()) ;

	if (m_pCx->SendData(Z) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "hzHttpEvent %p Failed to send response (size=%d, sock=%d)", this, Z.Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::SendAjaxResult (HttpRC hrc, const char* va_alist ...)
{
	//	Send a string, without headers of any kind, as a response.
	//
	//	Note: This function should only be invoked in response to AJAX HTTP requests
	//
	//	Arguments:	1)	hrc			HTTP return code
	//				2)	va_alist	Varaiable printf style arguments
	//
	//	Returns:	E_WRITEFAIL	If the response could not be sent
	//				E_OK		If the AJAX result was sent

	_hzfunc("hzHttpEvent::SendCmdResult(2)") ;

	va_list		ap ;	//	Variable argument list
	const char*	fmt ;	//	Format control
	hzChain		Z ;		//	Output chain

	va_start(ap, va_alist) ;
	fmt = va_alist ;

	Z._vainto(fmt, ap) ;

	if (SendRawChain(hrc, HMTYPE_TXT_HTML, Z, 0, false) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Response data not sent (size=%d, sock=%d)", Z.Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}

hzEcode	hzHttpEvent::SendAjaxResult (HttpRC hrc, hzChain& Z)
{
	//	Send a string, without headers of any kind, as a response.
	//
	//	Note: This function should only be invoked in response to AJAX HTTP requests
	//
	//	Arguments:	1)	hrc		HTTP return code
	//				2)	Z		Varaiable printf style arguments
	//
	//	Returns:	E_WRITEFAIL	If the response could not be sent
	//				E_OK		If the AJAX result was sent

	_hzfunc("hzHttpEvent::SendCmdResult(3)") ;

	if (SendRawChain(hrc, HMTYPE_TXT_HTML, Z, 0, false) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Response data not sent (size=%d, sock=%d)", Z.Size(), m_pCx->CliSocket()) ;
	return E_OK ;
}
