//
//	File:	hzHttpClient.cpp
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

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzCodec.h"
#include "hzHttpClient.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Prototypes
*/

uint32_t	_extractHttpHeader	(hzString& Attr, hzString& Value, hzChain::Iter& ci, bool bConvert) ;

/*
**	Section 1:	hzHttpClient member functions
*/

hzEcode	hzHttpClient::Connect	(const hzUrl& url)
{
	_hzfunc("hzHttpClient::Connect") ;

	hzEcode	rc ;	//	Return code

	if (url.IsSSL())
		rc = m_Webhost.ConnectSSL(url.Domain(), url.Port()) ;
	else
		rc = m_Webhost.ConnectStd(url.Domain(), url.Port()) ;

	if (rc != E_OK)
		m_Error.Printf("%s. Could not connect to domain [%s] on port %d (error=%s)\n", *_fn, *url.Domain(), url.Port(), Err2Txt(rc)) ;
	else
	{
		rc = m_Webhost.SetSendTimeout(30) ;
		if (rc != E_OK)
			m_Error.Printf("%s. Could not set send_timeout on connection to domain [%s] on port %d (error=%s)\n",
				*_fn, *url.Domain(), url.Port(), Err2Txt(rc)) ;
		else
		{
			rc = m_Webhost.SetRecvTimeout(30) ;
			if (rc != E_OK)
				m_Error.Printf("%s. Could not set recv_timeout on connection to domain [%s] on port %d (error=%s)\n",
					*_fn, *url.Domain(), url.Port(), Err2Txt(rc)) ;
		}
	}

	return rc ;
}

hzEcode	hzHttpClient::Close	(void)
{
	_hzfunc("hzHttpClient::Close") ;

	m_Webhost.Close() ;
	return E_OK ;
}

uint32_t	_extractHttpHeader	(hzString& Param, hzString& Value, hzChain::Iter& ci, bool bConvert)
{
	//	Support function for hzHttpEvent::ProcessEvent
	//
	//	Extract parameter name and value from a HTTP header (either that of a request or response). HTTP header lines are of the form
	//	param_name: param_value and are terminated with a CR/NL
	//
	//	Arguments:	1)	Param		The hzString to store the parameter name.
	//				2)	Value		The hzString to store the parameter value.
	//				3)	ci			A reference to the chain iterator processing the HTTP request.
	//				4)	bConvert	Flag to convert percent sign followed by two hex digits into single char value
	//
	//	Returns:	Number of charachters processed.

	_hzfunc("_extractHttpHeader") ;

	chIter		xi ;			//	For iterating line
	hzChain		temp ;			//	For building param and then value
	uint32_t	nCount = 0 ;	//	Returned length of HTTP header line
	uint32_t	nHex ;			//	Hex value
	char		cvHex[4] ;		//	Hex value buffer

	Param.Clear() ;
	Value.Clear() ;
	cvHex[2] = 0 ;

	xi = ci ;
	for (; !xi.eof() ;)
	{
		if (*xi == CHAR_PERCENT)
		{
			if (bConvert)
			{
				xi++ ; cvHex[0] = *xi ;
				xi++ ; cvHex[1] = *xi ;
				xi++ ;
				nCount += 3 ;

				if (IsHexnum(nHex, cvHex))
					temp.AddByte(nHex) ;
				continue ;
			}
		}

		if (*xi == CHAR_COLON && !Param)
		{
			xi++ ;
			nCount++ ;

			Param = temp ;
			temp.Clear() ;

			if (*xi == CHAR_SPACE)
				for (; !xi.eof() && (*xi == CHAR_SPACE || *xi == CHAR_TAB) ; xi++, nCount++) ;
		}

		if (xi == "\r\n")
			{ xi += 2 ; nCount += 2 ; break ; }
		if (*xi == CHAR_NL)
			{ xi++ ; nCount++ ; break ; }

		if (*xi < CHAR_SPACE)
			threadLog("%s: Illegal char (%u) in HTTP Header\n", *_fn, (uchar) *xi) ;

		if (*xi == CHAR_PLUS)
			temp.AddByte(CHAR_SPACE) ;
		else
			temp.AddByte(*xi) ;
		xi++ ;
		nCount++ ;
	}

	Value = temp ;
	return nCount ;
}

hzEcode	hzHttpClient::_procHttpResponse	(hzMapS<hzString,hzCookie>& cookies, HttpRC& hRet, const hzUrl& url)
{
	//	Support funtion to the hzHttpClient member functions GetPage() and PostForm(). The purpose is to gather the server response to
	//	an earlier HTTP GET, POST or HEAD request.
	//
	//	Arguments:	1)	cookies	Map of currently applicable cookies
	//				2)	hRet	HTTP return code
	//				3)	url		The URL
	//
	//	Returns:	E_NOSOCKET	If the external server has closed the connection
	//				E_NODATA	If nothing was recived
	//				E_FORMAT	If the response was malformed
	//				E_OK		If the response was recieved without error

	_hzfunc("hzHttpClient::_procHttpResponse") ;

	chIter		zi ;				//	To iterate the returned page
	chIter		hi ;				//	To re-iterate lines of interest in the header of the returned page
	chIter		ti ;				//	Temp iterator
	hzChain		Z ;					//	Request buffer
	hzChain		X ;					//	Temp buffer
	hzCookie	cookie ;			//	Cookie (to be checked against supplied map of cookies)
	hzString	S ;					//	Temp string
	hzString	param ;				//	Header parameter name
	hzString	value ;				//	Header parameter value
	uint32_t	nRecv ;				//	Bytes received
	uint32_t	nExpect = 0 ;		//	Size of current chunk
	uint32_t	nLen = 0 ;			//	Content length
	uint32_t	nLine ;				//	Line number (of header)
	uint32_t	nTry ;				//	Number of tries
	uint32_t	nCount ;			//	Number of bytes counted off from those expected
	bool		duHast = false ;	//	Have read a chunking directive or have a content len
	bool		bTerm = false ;		//	Terminate chunking (only set upon a 0 value on a line by itself
	hzEcode		sRet = E_OK ;		//	Return code
	char		numBuf[4] ;			//	For HTTP return code

	//	Clear variables
	m_CacheCtrl = (char*) 0 ;
	m_Pragma = (char*) 0 ;
	m_Redirect = (char*) 0 ;
	m_KeepAlive = (char*) 0 ;
	m_ContentType = (char*) 0 ;
	m_XferEncoding = (char*) 0 ;
	m_ContEncoding = (char*) 0 ;
	m_Etag = (char*) 0 ;
	m_bConnection = false ;
	m_nContentLen = 0 ;

	m_Content.Clear() ;
	m_Header.Clear() ;

	//	Garner first the header, from the response
	for (nTry = 0 ; nTry < 4 && !m_Header.Size() ; nTry++)
	{
		sRet = m_Webhost.Recv(m_buf, nRecv, HZ_MAXPACKET) ;
		if (sRet != E_OK)
		{
			if (sRet == E_NOSOCKET)
				m_Error.Printf("%s: Connection closed by server\n", *_fn) ;
			else
				m_Error.Printf("%s: Could not recv bytes (nbytes=%d) from page %s (error=%s)\n", *_fn, nRecv, *url.Resource(), Err2Txt(sRet)) ;
			break ;
		}

		if (!nRecv)
		{
			m_Error.Printf("%s: Got no response, retrying ...\n", *_fn) ;
			sleep(1) ;
			continue ;
		}

		Z.Append(m_buf, nRecv) ;

		//	Test for presence of \r\n\r\n to mark end of header
		for (zi = Z ; !zi.eof() ; zi++)
		{
			if (*zi != CHAR_CR)
				continue ;

			if (zi == "\r\n\r\n")
			{
				//	Bytes before the header's end are now copied from temp chain Z to the header
				for (ti = Z ; ti != zi ; ti++)
					m_Header.AddByte(*ti) ;
				zi += 4 ;
				break ;
			}
		}
	}

	if (nTry == 4)
		{ m_Error.Printf("%s. Given up!\n", *_fn) ; return E_NODATA ; }

	if (!m_Header.Size())
		{ m_Error.Printf("%s. Given up! Header is empty\n", *_fn) ; return E_NODATA ; }

	/*
	**	Examine header
	*/

	//	First part is the HTTP return code
	hi = m_Header ;
	if (hi == "HTTP/")
	{
		for (hi += 5 ; !hi.eof() && *hi > CHAR_SPACE ; hi++) ;
		hi++ ;
	}
	else
	{
		m_Error.Printf("%s. case 1: 1st line of server response should be HTTP/{version} followed by a 3 digit HTML return code\n", *_fn) ;
		m_Error.Printf("got %d bytes of header namely:-\n[", m_Header.Size()) ;
		m_Error << m_Header ;
		m_Error << "]\n" ;

		return E_FORMAT ;
	}

	m_Error << "Response\n" << m_Header << "\n--------------------------\n" ;

	numBuf[0] = *hi++ ;
	numBuf[1] = *hi++ ;
	numBuf[2] = *hi++ ;
	numBuf[3] = 0 ;

	if (*hi != CHAR_SPACE || !IsDigit(numBuf[0]) || !IsDigit(numBuf[1]) || !IsDigit(numBuf[2])) 
	{
		m_Error.Printf("%s. case 2: 1st line of server response should be HTTP/1.1 followed by a 3 digit HTML return code\n\n", *_fn) ;
		return E_FORMAT ;
	}

	hRet = (HttpRC) atoi(numBuf) ;
	for (hi++ ; !hi.eof() && *hi != CHAR_NL ; hi++) ;
	hi++ ;

	//	Next part is the header lines
	for (nLine = 1 ; !hi.eof() ; nLine++, hi += nLen)
	{
		nLen = _extractHttpHeader(param, value, hi, false) ;

		if (nLen == 0)
		{
			for (hi++ ; !hi.eof() && *hi != CHAR_NL ; hi++) ;
			hi++ ;
			m_Error.Printf("%s. Line %d of header rejected (param=%s, value=%s)\n", *_fn, nLine, *param, *value) ;
			continue ;
		}

		if (param.Equiv("Date"))				{ m_Accessed = value ; continue ; }
		if (param.Equiv("Expires"))				{ m_Expires = value ; continue ; }
		if (param.Equiv("Last-Modified"))		{ m_Modified = value ; continue ; }
		if (param.Equiv("Cache-Control"))		{ m_CacheCtrl = value ; continue ; }
		if (param.Equiv("Pragma"))				{ m_Pragma = value ; continue ; }
		if (param.Equiv("Location"))			{ m_Redirect = value ; continue ; }
		if (param.Equiv("Keep-Alive"))			{ m_KeepAlive = value ; continue ; }
		if (param.Equiv("Connection"))			{ m_bConnection = value == "close" ? false : true ; continue ; }
		if (param.Equiv("Content-Type"))		{ m_ContentType = value ; continue ; }
		if (param.Equiv("Content-Encoding"))	{ m_ContEncoding = value ; continue ; }
		if (param.Equiv("Transfer-Encoding"))	{ m_XferEncoding = value ; continue ; }
		if (param.Equiv("Alternate-Protocol"))	{ m_AltProto = value ; continue ; }
		if (param.Equiv("ETag"))				{ m_Etag = value ; continue ; }

		if (param.Equiv("Set-Cookie"))
		{
			//	Get the cookie value
			ti = hi ;

			for (ti += 12 ; !ti.eof() && *ti != CHAR_EQUAL ; ti++)
				X.AddByte(*ti) ;
			cookie.m_Name = X ;
			X.Clear() ;

			for (ti++ ; !ti.eof() && *ti != CHAR_SCOLON ; ti++)
				X.AddByte(*ti) ;
			cookie.m_Value = X ;
			//cookie.m_Value.FnameDecode() ;
			X.Clear() ;

			//	Get the path
			for (ti++ ; !ti.eof() && *ti == CHAR_SPACE ; ti++) ;

			if (ti == "path=")
			{
				for (ti += 5 ; !ti.eof() && *ti > CHAR_SPACE ; ti++)
					X.AddByte(*ti) ;
				cookie.m_Path = X ;
				X.Clear() ;
			}

			//	Get special directives (eg HttpOnly)
			for (ti++ ; !ti.eof() && *ti == CHAR_SPACE ; ti++) ;

			if (ti == "HttpOnly")
				cookie.m_Flags |= COOKIE_HTTPONLY ;

			cookies.Insert(cookie.m_Name, cookie) ;
			cookie.Clear() ;
			continue ;
		}

		if (param.Equiv("Content-Length"))
		{
			if (*value && value[0])
			{
				duHast = true ;
				m_nContentLen = atoi(*value) ;
			}
			continue ;
		}
	}

	/*
	**	Garner next the body, from the response
	*/

	m_Error.Printf("Getting body. xfer=%s, expect=%d, clen=%d\n", *m_XferEncoding, duHast?1:0, m_nContentLen) ;

	if (!duHast)
	{
		//	In chunked encoding the first part (directly after the header and the terminating \r\n\r\n), will be a hex number followed
		//	by a \r\n (on a line by itself). This hex number will mean the size of the following chunk. At the end of the chunk will be
		//	another hex number on a line by itself. Only when this number is zero are we at the end of the page.
		//
		//	While reading the chunk size and chunk, we will most probably, reach the end of the buffer and have to do a read operation
		//	on the socket.

		m_Error.Printf("Encoding is chunked\n") ;
		nExpect = nCount = 0 ;
		bTerm = false ;

		for (; !bTerm ;)
		{
			//	If we are at the end of the buffer, read more
			for (; zi.eof() ;)
			{
				//	If out of data, get more
				m_Webhost.Recv(m_buf, nRecv, HZ_MAXPACKET) ;
				if (nRecv <= 0)
					break ;

				m_Error.Printf("%s. Read buffer %d bytes\n", *_fn, nRecv) ;

				Z.Clear() ;
				Z.Append(m_buf, nRecv) ;

				for (zi = Z ; nExpect && !zi.eof() ; nExpect--, zi++)
					m_Content.AddByte(*zi) ;

				if (!nExpect)
					break ;
			}

			if (!nExpect)
			{
				//	We are on the 'chunk size' directive. This will be of the form \r\nXXX\r\n where X is a hex number

				//	Get rid of any \r\n sequences that are beyond the expected chars and before the chunk size directive
				for (; !zi.eof() && (*zi == CHAR_CR || *zi == CHAR_NL) ; zi++) ;

				if (zi.eof())
				{
					//	If out of input data, get more
					m_Webhost.Recv(m_buf, nRecv, HZ_MAXPACKET) ;
					if (nRecv)
					{
						m_Error.Printf("%s. Read extras %d bytes\n", *_fn, nRecv) ;

						Z.Clear() ;
						Z.Append(m_buf, nRecv) ;

						for (zi = Z ; !zi.eof() && (*zi == CHAR_CR || *zi == CHAR_NL) ; zi++) ;
					}
				}

				duHast = false ;

				for (;;)
				{
					if (zi.eof())
					{
						m_Webhost.Recv(m_buf, nRecv, HZ_MAXPACKET) ;
						if (nRecv)
						{
							m_Error.Printf("%s. Read extras %d bytes\n", *_fn, nRecv) ;

							Z.Clear() ;
							Z.Append(m_buf, nRecv) ;
							zi = Z ;
						}
					}

					//	Read the chunk size

					if (*zi >= '0' && *zi <= '9')	{ duHast = true ; nExpect *= 16 ; nExpect += (*zi - '0') ;  zi++ ; continue ; }
					if (*zi >= 'A' && *zi <= 'F')	{ duHast = true ; nExpect *= 16 ; nExpect += (*zi-'A'+10) ; zi++ ; continue ; }
					if (*zi >= 'a' && *zi <= 'f')	{ duHast = true ; nExpect *= 16 ; nExpect += (*zi-'a'+10) ; zi++ ; continue ; }

					if (zi == "\r\n")
						{ zi += 2 ; break ; }
					if (*zi == CHAR_CR)
						{ zi++ ; continue ; }
					if (*zi == CHAR_NL)
						{ zi++ ; break ; }

					sRet = E_FORMAT ;
					m_Error.Printf("%s: Unexpected char (%d) in chunking directive - from page %s\n", *_fn, *zi, *url.Resource()) ;
					break ;
				}

				if (!duHast)
				{
					m_Error.Printf("%s. Chunk notice missing\n", *_fn) ;
					sRet = E_FORMAT ;
				}

				if (sRet != E_OK)
					break ;

				if (nExpect == 0)
					bTerm = true ;

				//m_Error.Printf("%s: Chunk notice %d bytes\n", *_fn, nExpect) ;

				if (nExpect)
				{
					//	Play out rest of buffer but make sure we don't exceed the chunk size
					for (; !zi.eof() && nExpect ; zi++, nExpect--)
						m_Content.AddByte(*zi) ;
				}
				else
				{
					//	At end of page, just play out rest of buffer
					for (; !zi.eof() ; zi++) ;
						//m_Content.AddByte(*zi) ;
				}

				m_Error.Printf("%s. Chunk complete. Expect = %d\n", *_fn, nExpect) ;
			}
		}
	}
	else
	{
		//	Not chunked - just read until stated Content-Length is reached

		if (m_nContentLen)
		{
			for (; !zi.eof() ; zi++)
				m_Content.AddByte(*zi) ;
			Z.Clear() ;

			for (; m_Content.Size() < m_nContentLen ;)
			{
				sRet = m_Webhost.Recv(m_buf, nRecv, HZ_MAXPACKET) ;
				if (sRet != E_OK)
				{
					m_Error.Printf("%s: (1) Could not recv bytes from page %s (error=%s)\n", *_fn, *url.Resource(), Err2Txt(sRet)) ;
					break ;
				}

				if (nRecv == 0)
				{
					sRet = m_Webhost.Recv(m_buf, nRecv, HZ_MAXPACKET) ;
					if (sRet != E_OK)
					{
						m_Error.Printf("%s: (2) Could not recv bytes from page %s (error=%s)\n", *_fn, *url.Resource(), Err2Txt(sRet)) ;
						break ;
					}
				}

				if (nRecv <= 0)
				{
					m_Error.Printf("%s. Breaking after recv %d of %d bytes\n", *_fn, m_Content.Size(), m_nContentLen) ;
					break ;
				}

				m_Content.Append(m_buf, nRecv) ;
			}

			if (m_Content.Size() < m_nContentLen)
			{
				if (m_Content.Size() == (m_nContentLen - 4))
					m_Error.Printf("%s. Allowing 4-byte shortfall\n", *_fn) ;
				else
					sRet = E_READFAIL ;
			}
		}
	}

	if (hRet == 200)
	{
		if (!m_Content.Size())
		{
			m_Error.Printf("%s: No content (xfer_encoding=%s content_size=%d)\n", *_fn, *m_XferEncoding, m_nContentLen) ;
			sRet = E_NODATA ;
		}
	}

	if (sRet == E_OK && m_ContEncoding)
	{
		//	Must apply appropiate decoding to content

		if (m_ContEncoding == "gzip")
		{
			X = m_Content ;
			m_Content.Clear() ;

			m_Error.Printf("doing gunzip\n") ;
			sRet = Gunzip(m_Content, X) ;

			if (sRet != E_OK)
				m_Error.Printf("%s. Gunzip failed\n", *_fn) ;
		}
	}

	m_Error.Printf("%s. URL [%s] Header %d bytes, Content %d bytes (%d)\n\n", *_fn, *url, m_Header.Size(), m_Content.Size(), m_nContentLen) ;
	if (m_Content.Size() < 2000)
	{
		m_Error << "Content:\n" ;
		m_Error << m_Content ;
		m_Error << "------------------------\n" ;
	}

	return sRet ;
}

hzEcode	hzHttpClient::TestPage	(hzChain& Z, const hzUrl& url)
{
	//	Get a HTTP page from a website but do not process it in any way. This is for speed testing only.
	//
	//	Note:	The website (server) must already be connected to.
	//			No account is taken of redirected pages.
	//
	//	Arguments:	1)	Z	The chain into which page content is to be received
	//				2)	url	The URL of the page
	//
	//	Returns:	E_ARGUMENT	If no URL was specified
	//				E_NODATA	If nothing was recived
	//				E_OK		If the response was recieved without error

	_hzfunc("hzHttpClient::Testpage") ;

	chIter		zi ;			//	To iterate the returned page
	chIter		hi ;			//	To re-iterate lines of interest in the header of the returned page
	chIter		ti ;			//	Temp iterator
	hzChain		X ;				//	Temp buffer
	hzCookie	cookie ;		//	Cookie (drawn from supplied map of cookies)
	hzString	S ;				//	Temp string
	hzString	param ;			//	Header parameter name
	hzString	value ;			//	Header parameter value
	hzString	encoding ;		//	Page content is encoded, eg gzip
	uint32_t	nRecv ;			//	Bytes received
	uint32_t	nTry ;			//	Number of tries
	hzEcode		rc = E_OK ;		//	Return code

	//	Clear buffers
	Z.Clear() ;
	m_Header.Clear() ;
	m_Content.Clear() ;

	if (!url.Domain())
		{ m_Error.Printf("%s: - No host to locate\n", *_fn) ; return E_ARGUMENT ; }

	/*
 	**	Formulate HTTP request
	*/

	m_Request.Clear() ;
	if (url.Resource())
		m_Request << "GET " <<  url.Resource() << " HTTP/1.1\r\n" ;
	else
		m_Request << "GET / HTTP/1.1\r\n" ;

	m_Request <<
	"Accept: */*\r\n"
	"Accept-Language: en-gb\r\n" ;

	if (m_AuthBasic)
		m_Request << "Authorization: Basic " << m_AuthBasic << "\r\n" ;

	m_Request << "User-Agent: HadronZoo/0.8 Linux 2.6.18\r\n" ;
	m_Request << "Host: " << url.Domain() << "\r\n" ;
	if (m_Referer)
		m_Request << "Referer: " << m_Referer << "\r\n" ;
	m_Request << "Connection: Keep-Alive\r\n\r\n" ;

	/*
	**	Send request
	*/

	m_Error << _fn << " Sending [" << m_Request << "] to domain " << url.Domain() << "\n" ;

	rc = m_Webhost.Send(m_Request) ;
	if (rc != E_OK)
	{
		m_Error.Printf("%s. Could not send request to domain [%s] (error=%s)\n", *_fn, *url.Domain(), Err2Txt(rc)) ;
		return rc ;
	}

	//	Garner response
	for (nTry = 0 ; nTry < 4 && !m_Header.Size() ; nTry++)
	{
		rc = m_Webhost.Recv(m_buf, nRecv, HZ_MAXPACKET) ;
		if (rc != E_OK)
		{
			if (rc == E_NOSOCKET)
				m_Error.Printf("%s: Connection closed by server\n", *_fn) ;
			else
				m_Error.Printf("%s: Could not recv bytes (nbytes=%d) from page %s (error=%s)\n", *_fn, nRecv, *url.Resource(), Err2Txt(rc)) ;
			break ;
		}

		if (!nRecv)
		{
			m_Error.Printf("%s: Got no response, retrying ...\n", *_fn) ;
			sleep(1) ;
			continue ;
		}

		Z.Append(m_buf, nRecv) ;
	}

	//	rc = _procHttpResponse(cookies, hRet, url) ;
	if (rc != E_OK)
	{
		m_Error.Printf("%s. Could not process response from [%s] (error=%s)\n", *_fn, *url, Err2Txt(rc)) ;
		return rc ;
	}

	m_Referer = url ;
	return rc ;
}

hzEcode	hzHttpClient::_getpage	(hzMapS<hzString,hzCookie>& cookies, HttpRC& hRet, const hzUrl& url, const hzString& etag)
{
	//	Get a HTTP page from a website but do not redirect. This is a support function for GetPage()
	//
	//	Arguments:	1) cookies	Set of applicable cookies
	//				2) hRet		The HTTP return code from server
	//				3) url		The URL
	//				4) etag		Entity tag
	//
	//	Returns:	E_ARGUMENT	If the URL is not supplied or no domain specified
	//				E_NOSOCKET	If the external server has closed the connection
	//				E_NODATA	If nothing was recived
	//				E_FORMAT	If the response was malformed
	//				E_OK		If the response was recieved without error

	_hzfunc("hzHttpClient::_getpage") ;

	chIter		zi ;				//	To iterate the returned page
	chIter		hi ;				//	To re-iterate lines of interest in the header of the returned page
	chIter		ti ;				//	Temp iterator
	hzChain		Z ;					//	Request buffer
	hzChain		X ;					//	Temp buffer
	hzCookie	cookie ;			//	Cookie (drawn from supplied map of cookies)
	hzString	S ;					//	Temp string
	hzString	param ;				//	Header parameter name
	hzString	value ;				//	Header parameter value
	hzString	encoding ;			//	Page content is encoded, eg gzip
	uint32_t	x = 0 ;				//	Size of current chunk
	bool		bFirstCookie ;		//	Controls form of cookie header
	hzEcode		rc = E_OK ;

	//	Clear buffers
	m_Header.Clear() ;
	m_Content.Clear() ;

	if (!url.Domain())
		{ m_Error.Printf("%s: - No host to locate\n", *_fn) ; return E_ARGUMENT ; }

	/*
 	**	Formulate HTTP request
	*/

	m_Request.Clear() ;
	if (url.Resource())
		m_Request << "GET " <<  url.Resource() << " HTTP/1.1\r\n" ;
	else
		m_Request << "GET / HTTP/1.1\r\n" ;

	m_Request << "Accept: */*\r\n" ;
	//m_Request << "Accept-Encoding: gzip\r\n" ;
	m_Request << "Accept-Language: en-gb\r\n" ;

	if (cookies.Count())
	{
		m_Request << "Cookie: " ;
		bFirstCookie = false ;
		for (x = 0 ; x < cookies.Count() ; x++)
		{
			cookie = cookies.GetObj(x) ;

			if (bFirstCookie)
				m_Request << "; " ;

			m_Request.Printf("%s=%s", *cookie.m_Name, *cookie.m_Value) ;
			bFirstCookie = true ;
		}
		m_Request << "\r\n" ;
	}

	if (etag)
		m_Request << "If-None-Match: " << etag << "\r\n" ;

	if (m_AuthBasic)
		m_Request << "Authorization: Basic " << m_AuthBasic << "\r\n" ;

	m_Request << "User-Agent: HadronZoo/0.8 Linux 2.6.18\r\n" ;
	m_Request << "Host: " << url.Domain() << "\r\n" ;
	if (m_Referer)
		m_Request << "Referer: " << m_Referer << "\r\n" ;
	m_Request << "Connection: keepalive\r\n\r\n" ;

	//	Connect to server
	if (url.IsSSL())
		rc = m_Webhost.ConnectSSL(url.Domain(), url.Port()) ;
	else
		rc = m_Webhost.ConnectStd(url.Domain(), url.Port()) ;
	if (rc != E_OK)
	{
		m_Error.Printf("%s. Could not connect to domain [%s] on port %d (error=%s)\n", *_fn, *url.Domain(), url.Port(), Err2Txt(rc)) ;
		return rc ;
	}

	//	Send request
	m_Error << _fn << " Sending [" << m_Request << "] to domain " << url.Domain() << "\n" ;

	rc = m_Webhost.Send(m_Request) ;
	if (rc != E_OK)
	{
		m_Error.Printf("%s. Could not send request to domain [%s] (error=%s)\n", *_fn, *url.Domain(), Err2Txt(rc)) ;
		return rc ;
	}

	//	Garner response
	rc = _procHttpResponse(cookies, hRet, url) ;
	if (rc != E_OK)
	{
		m_Error.Printf("%s. Could not process response from [%s] (error=%s)\n", *_fn, *url, Err2Txt(rc)) ;
		return rc ;
	}

	m_Referer = url ;
	m_Webhost.Close() ;
	return rc ;
}

hzEcode	hzHttpClient::GetPage	(HttpRC& hRet, const hzUrl& url, hzMapS<hzString,hzCookie>& cookies, const hzString& etag)
{
	//	Get a HTTP page from a website. Note that the whole page is retrieved or abandoned before this function returns. Some servers send pages with
	//	the header 'Transfer-Encoding: chunked' instead of the 'Content-Length:' header. This is done because the size of the page is not known at the
	//	start of transmission. The body part of the message is sent in chunks with the chunk size given (in hex on a line by itself) at the start of
	//	each chunk. Because of the existance of the chunked approach, this function has to handle it but it is currently not possible for applications
	//	to take advantage in the intended way. Instead applications calling this function have to wait until it returns with a complete page, however
	//	long!
	//
	//	Note that no assumptions can be made about packets that are sent except that since the connection is TCP, they will be in order. The header
	//	may be comprised of a number of whole packets or it may be that a packet stradles the end of the header and the start of the contents.
	//
	//	Arguments:	1) hRet		HTTP return code from the server.
	//				2) url		The URL of the page to retrieve.
	//				3) cookies	Applicable cookie (as maintained by hzWebhost instance)
	//				4) etag		Page entity tag (as maintained by hzWebhost instance)
	//
	//	Returns:	E_ARGUMENT	If the URL is not supplied or no domain specified
	//				E_NOSOCKET	If the external server has closed the connection
	//				E_NODATA	If nothing was recived
	//				E_FORMAT	If the response was malformed
	//				E_OK		If the response was recieved without error

	_hzfunc("hzHttpClient::GetPage") ;

	hzUrl		dest ;			//	Actual URL for downloading - may be result of a redirection
	hzString	dom ;			//	This is set first to the called URL's domain but afterwards to any redirected domain
	hzString	etag2 ;			//	Set as null for the benefit of _getpage() in the case of redirection
	hzEcode		rc = E_OK ;		//	Return code

	//	Considered a top-level function so we clear the error chain
	m_Error.Clear() ;
	m_Error.Printf("%s. GETTING PAGE %s\n", *_fn, *url) ;

	dest = url ;

	m_rtRequest = RealtimeNano() ;
	rc = _getpage(cookies, hRet, dest, etag) ;
	m_rtResponse = RealtimeNano() ;

	if (rc != E_OK)
	{
		m_Error.Printf("%s. ABORTED (_getpage failure)\n", *_fn) ;
		return rc ;
	}

	for (; hRet == HTTPMSG_REDIRECT_PERM || hRet == HTTPMSG_REDIRECT_TEMP ;)
	{
		//Clear() ;

		if (!m_Redirect)
			m_Error.Printf("Oops - no URL to redirect to\n") ;
		else
		{
			if (m_Redirect[0] == CHAR_FWSLASH)
				{ dom = dest.Domain() ; dest.SetValue(dom, m_Redirect) ; }
			else
				dest = m_Redirect ;

			m_Error.Printf("redirecting to %s\n", *dest) ;

			rc = _getpage(cookies, hRet, dest, etag2) ;

			if (rc != E_OK)
			{
				m_Error.Printf("%s: Redirect FAILED (error=%s)\n", *_fn, Err2Txt(rc)) ;
				return rc ;
			}
		}
	}

	//	Obtain document type. If HTML then also get links

	m_Error.Printf("%s. Got response %d (size %d bytes)\n", *_fn, hRet, m_Content.Size()) ;
	return rc ;
}

hzEcode	hzHttpClient::_postform	(HttpRC& hRet, const hzUrl& url, hzMapS<hzString,hzCookie>& cookies, hzVect<hzString>& hdrs, const hzChain& formData)
{
	//	Support function for hzHttpClient::PostForm(). Compiles the HTTP request and adds the supplied form. The functionality herin would just
	//	appear in PostForm() except for the need to cope with redirection. This requires that the request ...
	//
	//	Arguments:	1)	hRet		Reference to HTTP return code, set by this operation
	//				2)	url			The URL to post the form to
	//				3)	cookies		Map of applicable cookies to submit with the post
	//				4)	hdrs		Vector of additional HTTP headers
	//				5)	formData	The actual form data
	//
	//	Returns:	E_ARGUMENT	If the URL is not supplied or no domain specified
	//				E_NOSOCKET	If the external server has closed the connection
	//				E_NODATA	If nothing was recived
	//				E_FORMAT	If the response was malformed
	//				E_OK		If the form was posted and the response was recieved without error

	_hzfunc("hzHttpClient::PostForm") ;

	hzCookie	cookie ;		//	Cookie (drawn from supplied map of cookies)
	hzString	dom ;			//	Domain part of URL
	hzString	res ;			//	Resource part of URL
	uint32_t	nPort ;			//	Port (from URL)
	uint32_t	nIndex ;		//	Form data iterator
	bool		bFirstCookie ;	//	Controls form of cookie header
	hzEcode		rc ;			//	Return code

	m_Request.Clear() ;

	dom = url.Domain() ;
	res = url.Resource() ;
	nPort = url.Port() ;

	if (url.IsSSL())
		m_Request.Printf("POST https://%s%s HTTP/1.1\r\n", *dom, *res) ;
	else
		m_Request.Printf("POST http://%s%s HTTP/1.1\r\n", *dom, *res) ;

	m_Request << "Host: " << dom << "\r\n" ;
	m_Request << "User-Agent: HadronZoo/0.8 Linux 2.6.18\r\n" ;
	m_Request << "Accept: */*\r\n" ;
	m_Request << "Accept-Language: en-gb,en;q=0.5\r\n" ;
	//m_Request << "Accept-Encoding: gzip, deflate\r\n" ;
	m_Request << "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n" ;

	if (m_Referer)
		m_Request << "Referer: " << m_Referer << "\r\n" ;

	m_Request.Printf("Content-Length: %d\r\n", formData.Size()) ;

	if (cookies.Count())
	{
		m_Request << "Cookie: " ;
		bFirstCookie = false ;
		for (nIndex = 0 ; nIndex < cookies.Count() ; nIndex++)
		{
			cookie = cookies.GetObj(nIndex) ;

			if (bFirstCookie)
				m_Request << "; " ;

			m_Request.Printf("%s=%s", *cookie.m_Name, *cookie.m_Value) ;
			bFirstCookie = true ;
		}
		m_Request << "\r\n" ;
	}

	if (hdrs.Count())
	{
		for (nIndex = 0 ; nIndex < hdrs.Count() ; nIndex++)
			//m_Request << hdrs.Element(nIndex) ;
			m_Request << hdrs[nIndex] ;
	}

	m_Request << "Connection: keep-alive\r\n" ;
	m_Request << "Pragma: no-cache\r\n" ;
	m_Request << "Cache-Control: no-cache\r\n\r\n" ;
	m_Request << formData ;

	//	Connect to server
	if (url.IsSSL())
		rc = m_Webhost.ConnectSSL(dom, nPort) ;
	else
		rc = m_Webhost.ConnectStd(dom, nPort) ;

	if (rc != E_OK)
	{
		m_Error.Printf("%s. Could not connect to %s on port %d\n", *_fn, *dom, nPort) ;
		return rc ;
	}

	m_Error.Printf("%s. Connected to %s on port %d\n[\n", *_fn, *dom, nPort) ;
	m_Error << m_Request ;
	m_Error << "\n-------------------------\n\n" ;

	rc = m_Webhost.Send(m_Request) ;
	if (rc != E_OK)
		m_Error.Printf("%s. Could not send request (error=%s)\n", *_fn, Err2Txt(rc)) ;
	else
	{
		rc = _procHttpResponse(cookies, hRet, url) ;
		if (rc != E_OK)
			m_Error.Printf("%s. Could not get response (error=%s)\n", *_fn, Err2Txt(rc)) ;
	}

	return rc ;
}

hzEcode	hzHttpClient::PostForm	(HttpRC& hRet, const hzUrl& url, hzMapS<hzString,hzCookie>& cookies, hzVect<hzString>& hdrs, const hzList<hzPair>& formData)
{
	//	Post a form to the server. Note that this will normally result in a HTTP response. This response must be processed in the same
	//	way (ie values are extracted from lines in the HTTP header).
	//
	//	Arguments:	1) hRet		HTTP return code
	//				2) url		The URL
	//				3) cookies	Map of cookies supplied by the server to be submitted with each request/submission
	//				4) hdrs		Lines in HTTP header
	//				5) formData	The form data to be submitted
	//
	//	Returns:	E_ARGUMENT	If the URL is not supplied or no domain specified
	//				E_NOSOCKET	If the external server has closed the connection
	//				E_NODATA	If nothing was recived
	//				E_FORMAT	If the response was malformed
	//				E_OK		If the form was posted and the response was recieved without error

	_hzfunc("hzHttpClient::PostForm") ;

	hzList<hzPair>::Iter	iD ;	//	Form data iterator

	hzChain		F ;					//	Form data in submissible form
	hzCookie	cookie ;			//	Cookie (drawn from supplied map of cookies)
	hzPair		P ;					//	Form data field
	hzUrl		dest ;				//	Url may change due to redirection
	hzString	dom ;				//	Domain part of URL
	hzString	res ;				//	Resource part of URL
	hzString	etag ;				//	Temp string for reading form data
	hzEcode		rc ;				//	Return code

	//	Considered a top-level function so we clear the error chain
	m_Error.Clear() ;
	m_Error.Printf("%s. POSTING FORM %s\n", *_fn, *url) ;

	//Clear() ;
	m_Header.Clear() ;
	m_Content.Clear() ;
	m_Request.Clear() ;

	if (!formData.Count())
		return E_NODATA ;

	for (iD = formData ; iD.Valid() ; iD++)
	{
		P = iD.Element() ;

		if (F.Size())
			F.AddByte(CHAR_AMPSAND) ;

		F << P.name ;
		F.AddByte(CHAR_EQUAL) ;
		P.value.UrlEncode() ;
		F << P.value ;
	}

	dest = url ;

	rc = _postform(hRet, dest, cookies, hdrs, F) ;
	if (rc != E_OK)
	{
		m_Error.Printf("%s: FAILED (error=%s)\n", *_fn, Err2Txt(rc)) ;
		return rc ;
	}

	for (; hRet == HTTPMSG_REDIRECT_PERM || hRet == HTTPMSG_REDIRECT_TEMP ;)
	{
		if (!m_Redirect)
			m_Error.Printf("%s. Oops - no URL to redirect to\n", *_fn) ;
		else
		{
			if (m_Redirect[0] == CHAR_FWSLASH)
				{ dom = dest.Domain() ; dest.SetValue(dom, m_Redirect) ; }
			else
				dest = m_Redirect ;

			m_Error.Printf("%s. redirecting to %s\n", *_fn, *dest) ;

			etag = (char*) 0 ;
			rc = _getpage(cookies, hRet, dest, etag) ;
			if (rc != E_OK)
			{
				m_Error.Printf("%s: Redirect FAILED (error=%s)\n", *_fn, Err2Txt(rc)) ;
				break ;
			}
		}
	}

	return rc ;
}

hzEcode	hzHttpClient::PostAjax	(HttpRC& hRet, const hzUrl& url, hzMapS<hzString,hzCookie>& cookies, hzVect<hzString>& hdrs, const hzList<hzPair>& formData)
{
	//	Post a form to the server but do not seek a HTTP response.
	//
	//	Arguments:	1)	hRet		HTTP return code
	//				2)	url			The URL
	//				3)	cookies		Map of cookies supplied by the server to be submitted with each request/submission
	//				4)	hdrs		Lines in HTTP header
	//				5)	formData	The form data to be submitted
	//
	//	Returns:	E_ARGUMENT	If the URL is not supplied or no domain specified
	//				E_NOSOCKET	If the external server has closed the connection
	//				E_NODATA	If nothing was recived
	//				E_FORMAT	If the response was malformed
	//				E_OK		If the AJAX request was sent and the response was recieved without error

	_hzfunc("hzHttpClient::PostAjax") ;

	hzList<hzPair>::Iter	iD ;	//	Form data iterator

	hzChain		F ;					//	Form data in submissible form
	hzCookie	cookie ;			//	Cookie (drawn from supplied map of cookies)
	hzPair		P ;					//	Form data field
	hzString	dom ;				//	Domain part of URL
	hzString	res ;				//	Resource part of URL
	hzString	S ;					//	Temp string for reading form data
	uint32_t	nPort ;				//	Port (from URL)
	uint32_t	nIndex ;			//	Form data iterator
	hzEcode		rc ;				//	Return code

	//Clear() ;
	m_Header.Clear() ;
	m_Content.Clear() ;
	m_Request.Clear() ;

	if (!formData.Count())
		return E_NODATA ;

	for (iD = formData ; iD.Valid() ; iD++)
	{
		P = iD.Element() ;

		if (F.Size())
			F.AddByte(CHAR_AMPSAND) ;

		F << P.name ;
		F.AddByte(CHAR_EQUAL) ;
		F << P.value ;
	}

	dom = url.Domain() ;
	res = url.Resource() ;
	nPort = url.Port() ;

	if (url.IsSSL())
		m_Request.Printf("POST https://%s%s HTTP/1.1\r\n", *dom, *res) ;
	else
		m_Request.Printf("POST http://%s%s HTTP/1.1\r\n", *dom, *res) ;

	//m_Request << "POST " << "http://" << dom << res << " HTTP/1.1\r\n" ;
	m_Request << "Accept: text/*\r\n" ;
	m_Request << "Accept-Language: en-gb\r\n" ;
	//m_Request << "Accept-Encoding:\r\n" ;
	//m_Request << "Accept-Encoding: gzip, deflate\r\n" ;

	for (nIndex = 0 ; nIndex < cookies.Count() ; nIndex++)
	{
		cookie = cookies.GetObj(nIndex) ;
		if (cookie.m_Flags & COOKIE_HTTPONLY)
			continue ;

		m_Request.Printf("Cookie: %s=%s\r\n", *cookie.m_Name, *cookie.m_Value) ;
	}

	//m_Request << "User-Agent: HadronZoo/0.8 (compatible; MSIE 6.0;)\r\n" ;
	m_Request << "User-Agent: HadronZoo/0.8 Linux 2.6.18\r\n" ;
	m_Request.Printf("Content-Length: %d\r\n", F.Size()) ;
	m_Request << "Host: " << dom << "\r\n" ;

	if (hdrs.Count())
	{
		for (nIndex = 0 ; nIndex < hdrs.Count() ; nIndex++)
			//m_Request << hdrs.Element(nIndex) ;
			m_Request << hdrs[nIndex] ;
	}

	m_Request << "Connection: close\r\n\r\n" ;
	m_Request << F ;

	S = m_Request ;
	threadLog("%s - Sending [\n%s]\n", *_fn, *S) ;

	//	Connect to server
	if (url.IsSSL())
		rc = m_Webhost.ConnectSSL(dom, nPort) ;
	else
		rc = m_Webhost.ConnectStd(dom, nPort) ;

	if (rc != E_OK)
		return rc ;

	//	Send request
	rc = m_Webhost.Send(m_Request) ;
	return rc ;
}

/*
**	Section 2, Subsect-A:	hzWebhost private functions
*/

void	hzWebhost::_clear	(void)
{
	//	Clears the hzWebhost for shutdown or for re-initialization for syncing another website
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzWebhost::_clear") ;

	hzDocMeta*	pMark ;		//	Document info
	uint32_t	nIndex ;	//	History itterator

	m_Offsite.Clear() ;
	m_Domains.Clear() ;
	m_Roots.Clear() ;
	m_Feeds.Clear() ;
	m_Emails.Clear() ;
	m_Banned.Clear() ;

	for (nIndex = 0 ; nIndex < m_mapHist.Count() ; nIndex++)
	{
		pMark = m_mapHist.GetObj(nIndex) ;
		delete pMark ;
	}

	m_mapHist.Clear() ;
	m_vecHist.Clear() ;
}

hzEcode	hzWebhost::_loadstatus	(void)
{
	//	Load visit status file (called upon startup). This way we do not re-fetch pages that have already been loaded unless they are out of date.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	If the repository for the webhost has not previously been defined
	//				E_OPENFAIL	If the visit status file could not be opened
	//				E_OK		If the visit status file is read in or was empty

	_hzfunc("hzWebhost::_loadstatus") ;

	hzDocXml		X ;				//	The manifest as XML document
	hzWebCMD		wc ;			//	Current web command
	hzAttrset		ai ;			//	Attribute itterator
	hzDocMeta*		pMark ;			//	Link meta data
	hzXmlNode*		pRoot ;			//	Root XML node
	hzXmlNode*		pN1 ;			//	Level 1 XML node
	hzXmlNode*		pN2 ;			//	Level 2 XML node
	hzXmlNode*		pN3 ;			//	Level 3 XML node
	_pageList*		pgl ;			//	List of lists of pages
	hzPair			p ;				//	Pair from formdata
	hzUrl			url ;			//	in-page link
	hzString		vs_fname ;		//	Vistation status file
	hzString		anam ;			//	Attribute name
	hzString		aval ;			//	Attribute value
	hzEcode			rc = E_OK ;		//	Return

	m_mapHist.Clear() ;
	m_vecHist.Clear() ;

	if (!m_Repos)
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No repository specified. Cannot determine data state") ;

	vs_fname = m_Repos + "/manifest" ;

	rc = TestFile(vs_fname) ;
	if (rc == E_NOTFOUND)
		{ threadLog("%s. No status file found. Repository in virgin state\n", *_fn) ; return E_OK ; }

	if (rc != E_OK)
		{ threadLog("%s. manifest file lookup error (%s)\n", *_fn, Err2Txt(rc)) ; return rc ; }

	rc = X.Load(vs_fname) ;
	if (rc != E_OK)
		{ threadLog("%s. Could not open Visit Status File %s for writing\n", *_fn, *vs_fname) ; return E_OPENFAIL ; }

	pRoot = X.GetRoot() ;

	for (pN1 = pRoot->GetFirstChild() ; rc == E_OK && pN1 ; pN1 = pN1->Sibling())
	{
		if (pN1->NameEQ("pagelists"))
		{
			for (pN2 = pN1->GetFirstChild() ; rc == E_OK && pN2 ; pN2 = pN2->Sibling())
			{
				if (pN2->NameEQ("pagelist"))
				{
					ai = pN2 ;

					if (ai.Valid())
					{
						anam = ai.Name() ; aval = ai.Value() ;

						pgl = new _pageList() ;

						if (anam == "name")
							pgl->name = aval ;
					}

					for (pN3 = pN2->GetFirstChild() ; rc == E_OK && pN3 ; pN3 = pN3->Sibling())
					{
						if (pN3->NameEQ("page"))
						{
							ai = pN3 ;
							if (ai.Valid())
							{
								anam = ai.Name() ; aval = ai.Value() ;

								if (anam == "url")
									pgl->links.Add(aval) ;
							}
						}
					}
				}
			}
		}

		if (pN1->NameEQ("commands"))
		{
			ai = pN1 ;
			if (ai.Valid())
			{
				anam = ai.Name() ; ai.Value() ;

				if (anam == "sofar")
					m_Sofar = atoi(*aval) ;
			}
		
			for (pN2 = pN1->GetFirstChild() ; rc == E_OK && pN2 ; pN2 = pN2->Sibling())
			{
				if (pN2->NameEQ("command"))
					continue ;

				for (ai = pN2 ; ai.Valid() ; ai.Advance())
				{
					anam = ai.Name() ; ai.Value() ;

					if		(anam == "url")		wc.m_Url = aval ;
					else if (anam == "crit")	wc.m_Crit = aval ;
					else if (anam == "slct")	wc.m_Slct = aval ;
					else if (anam == "inps")	wc.m_Inputs = aval ;
					else if (anam == "outs")	wc.m_Output = aval ;
				}

				pN3 = pN2->GetFirstChild() ;
				if (pN3 && pN3->NameEQ("form"))
				{
					for (ai = pN3 ; ai.Valid() ; ai.Advance())
					{
						anam = ai.Name() ; ai.Value() ;

						p.name = anam ;
						p.value = aval ;
						wc.m_Formdata.Add(p) ;
					}
				}
			}
		}

		if (pN1->NameEQ("history"))
		{
			for (pN2 = pN1->GetFirstChild() ; rc == E_OK && pN2 ; pN2 = pN2->Sibling())
			{
				if (pN2->NameEQ("page"))
				{
					pMark = new hzDocMeta() ;

					for (ai = pN2 ; ai.Valid() ; ai.Advance())
					{
						anam = ai.Name() ; ai.Value() ;

						if		(anam == "urlReq")	pMark->m_urlReq = aval ;
						else if (anam == "urlAct")	pMark->m_urlAct = aval ;
						else if (anam == "title")	pMark->m_Title = aval ;
						else if (anam == "desc")	pMark->m_Desc = aval ;
						else if (anam == "fname")	pMark->m_Filename = aval ;
						else if (anam == "etag")	pMark->m_Etag = aval ;
						else if (anam == "dtDnl")	pMark->m_Download.SetDateTime(aval) ;
						else if (anam == "dtMod")	pMark->m_Modified.SetDateTime(aval) ;
						else if (anam == "dtExp")	pMark->m_Expires.SetDateTime(aval) ;
						else if (anam == "type")	pMark->m_Doctype = (hzDoctype) atoi(*aval) ;
						else
							threadLog("%s. Unexpected page attribute %s=%s\n", *_fn, *anam, *aval) ;
					}

					m_vecHist.Add(pMark) ;
				}
			}
		}
	}

	return rc ;
}

hzEcode	hzWebhost::_savestatus	(void)
{
	//	Write out visit status file. This keeps a record of what URL's have already been downloaded and to which files, and the expiry
	//	date (after which the page will have to be fetched again)
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	If the repository for the webhost has not previously been defined
	//				E_OPENFAIL	If the visit status file could not be opened
	//				E_OK		If the visit status file is read in or was empty

	_hzfunc("hzWebhost::_status") ;

	hzList<hzUrl>::Iter		li ;	//	Links iterator (for pagelists)
	hzList<hzWebCMD>::Iter	ci ;	//	Iterator for web commands
	hzList<hzPair>::Iter	pi ;	//	Iterator for web commands

	ofstream	os ;				//	Output stream
	hzWebCMD	wc ;				//	Current web command
	hzCookie	cook ;				//	Cookie instance
	hzChain		Z ;					//	For building status file
	_pageList*	pgl ;				//	Pagelist
	hzDocMeta*	pMark ;				//	Document meta data
	hzPair		p ;					//	Pair from formdata
	hzString	vs_fname ;			//	Vistation status file
	hzString	S ;					//	Tmp string
	hzUrl		url ;				//	Link
	uint32_t	nIndex ;			//	Links iterator
	uint32_t	x ;					//	Links iterator
	hzEcode		rc = E_OK ;			//	Return

	if (!m_Repos)
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No repository specified. Cannot determine data state") ;

	vs_fname = m_Repos + "/manifest" ;
	os.open(*vs_fname) ;
	if (os.fail())
	{
		threadLog("%s. Could not open Visit Status File %s for writing\n", *_fn, *vs_fname) ;
		return E_OPENFAIL ;
	}

	threadLog("savestat: case 1\n") ;

	if (m_Cookies.Count())
	{
		Z << "<cookies>\n" ;
		for (x = 0 ; x < m_Cookies.Count() ; x++)
		{
			cook = m_Cookies.GetObj(x) ;
			Z.Printf("\t<cookie sig=\"%s\" name=\"%s\" path=\"%s\" flg=\"%d\" expire=\"%s\"/>\n",
				*cook.m_Value, *cook.m_Name, *cook.m_Path, cook.m_Flags, *cook.m_Expires.Str()) ;
		}
		Z << "</cookies>\n" ;
	}

	threadLog("savestat: case 2\n") ;

	if (m_Pagelists.Count())
	{
		Z << "<pagelists>\n" ;

		for (x = 0 ; x < m_Pagelists.Count() ; x++)
		{
			pgl = m_Pagelists.GetObj(x) ;

			Z.Printf("\t<pagelist name=\"%s\">\n", *pgl->name) ;

			if (pgl->links.Count())
			{
				for (li = pgl->links ; li.Valid() ; li++)
				{
					url = li.Element() ;
					Z.Printf("\t\t<page url=\"%s\">\n", *url.Whole()) ;
				}
			}

			Z << "\t</pagelist>\n" ;
		}
		Z << "</pagelists>\n" ;
	}
	threadLog("savestat: case 3\n") ;

	/*
	**	Do command list and status
	*/

	Z.Printf("<commands sofar=\"%d\">\n", m_Sofar) ;
	for (ci = m_Commands ; ci.Valid() ; ci++)
	{
		wc = ci.Element() ;

		if (wc.m_Cmd == WEBCMD_LOAD_PAGE)	Z << "\t<command type=^WEBCMD_LOAD_PAGE^" ;
		if (wc.m_Cmd == WEBCMD_LOAD_LIST)	Z << "\t<command type=^WEBCMD_LOAD_LIST^" ;
		if (wc.m_Cmd == WEBCMD_SLCT_PAGE)	Z << "\t<command type=^WEBCMD_SLCT_PAGE^" ;
		if (wc.m_Cmd == WEBCMD_SLCT_LIST)	Z << "\t<command type=^WEBCMD_SLCT_LIST^" ;
		if (wc.m_Cmd == WEBCMD_RGET)		Z << "\t<command type=^WEBCMD_RGET^" ;
		if (wc.m_Cmd == WEBCMD_POST)		Z << "\t<command type=^WEBCMD_POST^" ;
		if (wc.m_Cmd == WEBCMD_RSS)			Z << "\t<command type=^WEBCMD_RSS^" ;

		if (wc.m_Url)		Z.Printf(" url=\"%s\"", *wc.m_Url) ; 
    	if (wc.m_Crit)		Z.Printf(" crit=\"%s\"", *wc.m_Crit) ;
    	if (wc.m_Slct)		Z.Printf(" slct=\"%s\"", *wc.m_Slct) ;
    	if (wc.m_Inputs)	Z.Printf(" inps=\"%s\"", *wc.m_Inputs) ;
    	if (wc.m_Output)	Z.Printf(" outs=\"%s\"", *wc.m_Output) ;

		if (!wc.m_Formdata.Count())
			Z << " />\n" ;
		else
		{
			Z << ">\n" ;
			Z << "\t\t<form " ;

			for (pi = wc.m_Formdata ; pi.Valid() ; pi++)
			{
				p = pi.Element() ;
	
				Z.Printf(" %s=\"%s\"", *p.name, *p.value) ;
			}

			Z << " />\n" ;
			Z << "\t</command>\n" ;
		}

	}
	Z << "</commands>\n" ;
	threadLog("savestat: case 4\n") ;

	/*
	**	Do History
	*/

	Z << "<history>\n" ;
	for (nIndex = 0 ; nIndex < m_vecHist.Count() ; nIndex++)
	{
		pMark = m_vecHist[nIndex] ;

		Z.Printf("\t<webpage id=\"%d\" type=\"%d\"", pMark->m_Id, (uint32_t) pMark->m_Doctype) ;

		if (pMark->m_urlReq)			Z.Printf("\n\t\turlReq=\"%s\"", *pMark->m_urlReq) ;
		if (pMark->m_urlAct)			Z.Printf("\n\t\turlAct=\"%s\"", *pMark->m_urlAct) ;
		if (pMark->m_Title)				Z.Printf("\n\t\ttitle=\"%s\"",  *pMark->m_Title) ;
		if (pMark->m_Desc)				Z.Printf("\n\t\tdesc=\"%s\"",   *pMark->m_Desc) ;
		if (pMark->m_Filename)			Z.Printf("\n\t\tfname=\"%s\"",  *pMark->m_Filename) ;
		if (pMark->m_Etag)				Z.Printf("\n\t\e-tag=\"%s\"",   *pMark->m_Etag) ;
		if (pMark->m_Download.IsSet())	Z.Printf("\n\t\tdtDnl=\"%s\"",  *pMark->m_Download.Str()) ;
		if (pMark->m_Modified.IsSet())	Z.Printf("\n\t\tdtMod=\"%s\"",  *pMark->m_Modified.Str()) ;
		if (pMark->m_Expires.IsSet())	Z.Printf("\n\t\tdtExp=\"%s\"",  *pMark->m_Expires.Str()) ;

		Z << "/>\n" ;
	}
	Z << "</history>\n" ;
	threadLog("savestat: case 5\n") ;

	if (m_Trace.Size())
	{
		Z << "<trace>\n" ;
		Z << m_Trace ;
		Z << "</trace>\n" ;
	}
	threadLog("savestat: case 6\n") ;

	//Rat4Html(Z) ;
	os << Z ;
	os.close() ;

	return rc ;
}

hzEcode hzWebhost::AddRoot (hzUrl& url, hzString& criteria)
{
	//	Adds a root URL for the target website
	//
	//	Arguments:	1)	url			The root URL of the website
	//				2)	criteria	The resource we want as the entry point
	//
	//	Returns:	E_ARGUMENT	If the URL is not specified
	//				E_OK		If the root is added

	_hzfunc("hzWebhost::AddRoot") ;

	hzPair	X ;		//	URL/Search critiria pair

	if (!url)
		return E_ARGUMENT ;

	X.name = url.Whole() ;
	X.value = criteria ;
	m_Roots.Add(X) ;

	return E_OK ;
}

hzEcode	hzWebhost::AddRSS  (hzUrl& rss)
{
	//	Adds an RSS feed URL for the target website
	//
	//	Arguments:	1)	rss		The URL of the website's RSS feed
	//
	//	Returns:	E_ARGUMENT	If the URL is not specified
	//				E_OK		If the root is added

	_hzfunc("hzWebhost::AddRSS") ;

	m_Feeds.Add(rss) ;
	return E_OK ;
}

#define	SITEPARAM_USE_FIRST_COOKIE	0x01	//	Use the first cookie provided for the rest of session
#define SITEPARAM_USE_LOGIN_COOKIE	0x02	//	Use the cookie in the login response for the rest of session

hzEcode	hzWebhost::AuthBasic	(const char* username, const char* password)
{
	//	Sets the basic authentication string for the website (if the site uses this method). Once set all requests to the target website will be
	//	submitted with this string in the HTTP header.
	//
	//	Arguments:	1)	username	The user account username
	//				2)	password	The user account password
	//
	//	Returns:	E_ARGUMENT	If either the username or password is not supplied
	//				E_OK		If the root is added

	_hzfunc("hzWebhost::AuthBasic") ;

	hzChain	Enc ;			//	The encrypted sequence
	hzChain	Raw ;			//	The raw sequence

	if (!username || !username[0] || !password || !password[0])
	{
		threadLog("hzWebhost::AuthBasic. Must supply both a username and password\n") ;
		return E_ARGUMENT ;
	}

	Raw << username ;
	Raw.AddByte(CHAR_COLON) ;
	Raw << password ;

	Base64Encode(Enc, Raw) ;
	HC.m_AuthBasic = m_AuthBasic = Enc ;

	return E_OK ;
}

hzEcode	hzWebhost::Login	(void)
{
	//	Execute the login process. This is always a case of downloading each page listed in m_Authspteps (if any) and then posting to the URL
	//	given in m_Authpage (if provided) with the name-value pairs listed in in m_Authform.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOTFOUND	If the login page was not located
	//				E_WRITEFAIL	If the form recieved was not written to the repository
	//				E_OK		If the login form was posted (not the same thing as a successful login)

	_hzfunc("hzWebhost::Login") ;

	hzList<hzUrl>::Iter		ias ;		//	Iterator for URLs in m_Authsteps
	hzList<hzPair>::Iter	inv ;		//	Iterator for name-value pairs in m_Authform
	hzVect<hzString>		hdrs ;		//	Extra headers, needed for submit form (not generally applicable)

	ofstream	os ;					//	For exporting to file
	hzDocument*	pDoc ;					//	Downoaded document
	hzPair		P ;						//	Name-value pair instance
	hzUrl		url ;					//	URL instance
	hzString	S ;						//	Temp string
	hzString	etag ;					//	For GetPage() call
	HttpRC		hRet ;					//	HTML return code
	bool		bAuthpage = false ;		//	Set to true if the login form (if used) is correctly listed in m_Authsteps
	hzEcode		rc = E_OK ;				//	Return code

	threadLog("%s. Starting Login Sequence\n", *_fn) ;

	//	Werify we have to log on and if so, that the parameters are in place to support the login
	//if (m_Authmode == HZ_AUTH_NONE)

	//if (m_Authmode == HZ_AUTH_BASIC)
	if (m_Opflags & HZ_WEBSYNC_AUTH_BASIC)
		{ threadLog("%s. Basis Authentication. No login process required\n", *_fn) ; return E_OK ; }

	if (!(m_Opflags & (HZ_WEBSYNC_AUTH_POST | HZ_WEBSYNC_AUTH_GET)))
	{
		threadLog("%s. No Authentication method\n", *_fn) ;

		if (!m_Authsteps.Count() && !m_Authform.Count())
			{ threadLog("%s. No Authentication steps or form submission. No login process required\n", *_fn) ; return E_OK ; }
	}

	//	Download all pages listed in m_Authsteps (note the download must happen een if the page is in the history because we need the cookies)
	for (ias = m_Authsteps ; rc == E_OK && ias.Valid() ; ias++)
	{
		url = ias.Element() ;
		if (url == m_Authpage)
			bAuthpage = true ;

		rc = HC.GetPage(hRet, url, m_Cookies, etag) ;
		if (rc != E_OK)
			{ rc = E_NOTFOUND ; threadLog("%s. Could not download %s\n", *_fn, *url) ; }
	}

	if (rc != E_OK)
		return rc ;

	if (!bAuthpage && m_Authpage)
	{
		pDoc = Download(m_Authpage) ;
		if (!pDoc)
			{ threadLog("%s. Could not download %s\n", *_fn, *url) ; return E_NOTFOUND ; }
	}

	//	Now if there is a login form, post this now
	if (m_Authform.Count())
	{
		//	Write out login form to file
		if (m_Repos)
		{
			S = m_Repos + "/login_form" ;

			os.open(*S) ;
			if (os.fail())
				{ threadLog("%s. Cannot write out header file %s\n", *_fn, *S) ; return E_WRITEFAIL ; }

			os << HC.m_Header ;
			os << "\r\n\r\n" ;
			os << HC.m_Content ;
			os.close() ;
			os.clear() ;
		}

		//	Post the form
		rc = HC.PostForm(hRet, m_Authpage, m_Cookies, hdrs, m_Authform) ;
		if (rc != E_OK)
			{ threadLog("%s. Could not post form to %s\n", *_fn, *m_Authpage) ; return rc ; }

		//	Write out the login response
		if (m_Repos)
		{
			S = m_Repos + "/login_response" ;
	
			os.open(*S) ;
			if (os.fail())
				{ threadLog("%s. Cannot write out header file %s\n", *_fn, *S) ; return E_WRITEFAIL ; }

			os << HC.m_Header ;
			os << "\r\n\r\n" ;
			os << HC.m_Content ;
			os.close() ;
		}
	}

	return rc ;
}

void	hzWebhost::Logout	(void)
{
	//	Execute the logout process.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzWebhost::Logout") ;

	//	STUB
}

hzEcode	hzWebhost::Sync	(void)
{
	//	Run the series of hzWebCMD directives to sync key pages from a website to a repository
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	If no repository, no domain or no homepage has been specified
	//				E_NOTFOUND	If the login page was not located
	//				E_WRITEFAIL	If the login form recieved was not written to the repository
	//				E_OPENFAIL	If the visit status file could not be opened
	//				E_OK		If the scrape operation was successful

	_hzfunc("hzWebhost::Sync") ;

	hzMapS	<hzUrl,hzDocument*>	cur ;		//	Currently loaded documents
	hzMapS	<hzString,hzString>	fvals ;		//	Form values to be submitted
	hzVect	<hzHtmElem*>		elems ;		//	Elements selected by the web selector command

	hzList	<hzWebCMD>::Iter	ci ;		//	Iterator for web commands
	hzList	<hzPair>::Iter		pi ;		//	Iterator for form data
	hzList	<hzUrl>::Iter		si ;		//	Iterator for pagelist
	hzList	<hzHtmForm*>::Iter	fi ;		//	Iterator for forms

	hzSet	<hzUrl>		set_ctrl ;			//	Initial links from processing config params
	hzVect	<hzUrl>		pglinks ;			//	Links encountered within a given pages
	hzVect	<hzUrl>		allinks ;			//	Links encountered within a given pages
	hzVect	<hzString>	hdrs ;				//	Extra headers, needed for submit form
	hzList	<hzPair>	flist ;				//	Filtered list of form values

	ofstream		os ;					//	For writing form respose
	_pageList*		pgl = 0 ;				//	Primary pagelist instance
	_pageList*		pgl2 = 0 ;				//	Secondary pagelist instance
	hzWebCMD		wc ;					//	Current web command
	hzDocument*		pDoc ;					//	Downloaded document
	hzDocHtml*		pHdoc ;					//	Set if downloaded document is a HTML page.
	hzHtmElem*		pElem ;					//	HTML element (tag) lifted from page
	hzHtmForm*		pForm ;					//	Form found in page
	hzPair			P ;						//	Name value pair
	hzXDate			now ;					//	Date/time now (for cheking is pages have expired
	hzAttrset		ai ;					//	HTML element attribute iterator
	hzString		anam ;					//	Attribute name
	hzString		aval ;					//	Attribute value
	hzString		S ;						//	Temp string
	hzUrl			url ;					//	Temp link
	uint32_t		nStart ;				//	Links iterator
	uint32_t		nLimit ;				//	Links iterator
	uint32_t		nCount ;				//	Links iterator
	uint32_t		n ;						//	Aggregation iterator
	HttpRC			hRet = HTTPMSG_OK ;		//	HTML return code
	hzEcode			rc ;					//	Return code

	threadLog("%s. Called hzWebhost::Sync\n", *_fn) ;

	//	Check if repository and list of command is set up
	if (!m_Repos)
		{ threadLog("%s. Website is not properly initialized (no repository)\n", *_fn) ; return E_NOINIT ; }
	if (!m_Commands.Count())
		{ threadLog("%s. Website is not properly initialized (no commands)\n", *_fn) ; return E_NOINIT ; }

	//	Read in any existing manifest file
	rc = _loadstatus() ;
	if (rc != E_OK)
		{ threadLog("%s. Error on loading status - aborting\n", *_fn) ; return rc ; }

	//	If resuming execution, start we left off
	for (n = 0, ci = m_Commands ; n < m_Sofar ; n++, ci++) ;

	//	Execute commands in order
	for (; rc == E_OK && hRet == HTTPMSG_OK && ci.Valid() ; ci++)
	{
		pDoc = 0 ;
		wc = ci.Element() ;

		switch	(wc.m_Cmd)
		{
		case WEBCMD_LOAD_PAGE:	//  Get a page (no conditions)

			if (!wc.m_Url)
				{ threadLog("%s. Invalid loadPage command - no URL\n", *_fn) ; rc = E_NOINIT ; break ; }
			threadLog("%s. Doing WEBCMD_LOAD_PAGE\n", *_fn) ;

			pDoc = Download(wc.m_Url) ;
			if (!pDoc)
				{ threadLog("%s. case 1. Could not fetch page %s\n", *_fn, *wc.m_Url) ; rc = E_NOTFOUND ; break ; }

			cur.Insert(wc.m_Url, pDoc) ;

			if (pDoc->Whatami() == DOCTYPE_HTML)
			{
				pHdoc = (hzDocHtml*) pDoc ;
				if (pHdoc->m_Forms.Count())
				{
					//	Add the forms to the m_Forms map in the hzWebhost instance
					for (fi = pHdoc->m_Forms ; fi.Valid() ; fi++)
					{
						pForm = fi.Element() ;
						m_Forms.Insert(pForm->name, pForm) ;
					}
				}
			}

			break ;

		case WEBCMD_LOAD_LIST:	//  Get a list of pages (list supplied in command)

			threadLog("%s. Doing WEBCMD_LOAD_LIST\n", *_fn) ;

			if (!wc.m_Inputs)
				{ threadLog(" - Invalid loadList command - no list of links named\n", *_fn) ; rc  = E_NOTFOUND ; break ; }

			if (!m_Pagelists.Exists(wc.m_Inputs))
				{ threadLog(" - No such list of links as %s\n", *_fn, *wc.m_Inputs) ; rc  = E_NOTFOUND ; break ; }

			pgl = m_Pagelists[wc.m_Inputs] ;
			for (si = pgl->links ; si.Valid() ; si++)
			{
				url = si.Element() ;
				pDoc = Download(url) ;
				if (!pDoc)
					{ threadLog(" - case 3. Could not fetch page %s\n", *_fn, *url) ; rc = E_NOTFOUND ; }
				else
					threadLog(" - Fetched page %s\n", *url) ;
			}

			threadLog("%s. Ending WEBCMD_LOAD_LIST (%s)\n", *_fn, *wc.m_Inputs) ;
			break ;

		case WEBCMD_SLCT_PAGE:	//  Select links from a page

			threadLog("%s. Doing WEBCMD_SLCT_PAGE\n", *_fn) ;

			if (wc.m_Url && wc.m_Inputs)	{ rc = E_NOINIT ; threadLog("%s. Invalid request. Both a URL and an Input set specified\n", *_fn) ; }
			if (!wc.m_Url && !wc.m_Inputs)	{ rc = E_NOINIT ; threadLog("%s. Invalid request. No URL or Input set specified\n", *_fn) ; }
			if (!wc.m_Output)				{ rc = E_NOINIT ; threadLog("%s. Invalid linkSlct command - no name for output list\n", *_fn) ; }
			if (!wc.m_Slct && !wc.m_Crit)	{ rc = E_NOINIT ; threadLog("%s. Invalid linkSlct command - no node selection or globing criteria\n", *_fn) ; }

			if (rc != E_OK)
				break ;

			if (cur.Exists(wc.m_Url))
				pDoc = cur[wc.m_Url] ;
			else
				pDoc = Download(wc.m_Url) ;

			if (!pDoc)
				{ rc = E_NOTFOUND ; threadLog("%s. case 2. Could not fetch page %s\n", *_fn, *wc.m_Url) ; break ; }

			pgl = new _pageList() ;
			pgl->name = wc.m_Output ;

			if (pDoc->Whatami() != DOCTYPE_HTML)
				threadLog("%s. Not a HTML document\n", *_fn) ;
			else
			{
				pHdoc = (hzDocHtml*) pDoc ;

				for (n = 0 ; n < pHdoc->m_vecTags.Count() ; n++)
				{
					pElem = pHdoc->m_vecTags[n] ;
					threadLog("%s. VEC TAG %d <%s ", *_fn, n, *pElem->Name()) ;
					for (ai = pElem ; ai.Valid() ; ai.Advance())
					{
						threadLog(" %s=%s", ai.Name(), ai.Value()) ;
					}
					threadLog(" />\n") ;
				}

				rc = pHdoc->FindElements(elems, wc.m_Slct) ;

				for (n = 0 ; n < elems.Count() ; n++)
				{
					pElem = elems[n] ;

					threadLog("%s. GOT <%s ", *pElem->Name()) ;

					for (ai = pElem ; ai.Valid() ; ai.Advance())
					{
						anam = ai.Name() ; aval = ai.Value() ;

						threadLog(" %s=%s", *anam, *aval) ;

						if (anam == "href")
						{
							url = aval ;
							pgl->links.Add(url) ;
						}
					}
					threadLog(" />\n") ;
				}
			}

			threadLog("%s. Inserting pagelist %s of %d items\n", *_fn, *pgl->name, pgl->links.Count()) ;
			m_Pagelists.Insert(pgl->name, pgl) ;
			break ;

		case WEBCMD_SLCT_LIST:	//  Select links from a set of pages (supplied as a set of links)

			threadLog("%s. Doing WEBCMD_SLCT_LIST (%s)\n", *_fn, *wc.m_Url) ;

			if (!wc.m_Inputs)
				{ threadLog("%s. Invalid slctList command - no source list of links\n", *_fn) ; rc = E_NOINIT ; break ; }

			if (!wc.m_Output)
				{ rc = E_NOINIT ; threadLog("%s. Invalid slctList command - no name for output list\n", *_fn) ; }
			if (!wc.m_Slct && !wc.m_Crit)
				{ rc = E_NOINIT ; threadLog("%s. Invalid slctList command - no node selection or globing criteria\n", *_fn) ; }
			if (rc != E_OK)
				break ;

			pgl2 = new _pageList() ;
			pgl2->name = wc.m_Output ;

			//	Begin
			pgl = m_Pagelists[wc.m_Inputs] ;
			if (!pgl)
				{ rc = E_CORRUPT ; threadLog("%s. Pagelist of %s not found\n", *_fn, *wc.m_Inputs) ; break ; }

			for (si = pgl->links ; si.Valid() ; si++)
			{
				url = si.Element() ;

				if (cur.Exists(url))
					pDoc = cur[url] ;
				else
					pDoc = Download(url) ;

				if (!pDoc)
					{ rc = E_NOTFOUND ; threadLog("%s. case 2.2 Could not fetch page %s\n", *_fn, *url) ; break ; }

				if (pDoc->Whatami() == DOCTYPE_HTML)
				{
					pHdoc = (hzDocHtml*) pDoc ;

					rc = pHdoc->FindElements(elems, wc.m_Slct) ;

					for (n = 0 ; n < elems.Count() ; n++)
					{
						pElem = elems[n] ;

						threadLog("%s. GOT <%s ", *pElem->Name()) ;

						for (ai = pElem ; ai.Valid() ; ai.Advance())
						{
							anam = ai.Name() ; aval = ai.Value() ;

							threadLog(" %s=%s", *anam, *aval) ;

							if (anam == "href")
							{
								url = aval ;
								pgl2->links.Add(url) ;
							}
						}
						threadLog(" />\n") ;
					}
				}
			}

			threadLog("%s. Case 2. Inserting pagelist %s of %d items\n", *_fn, *pgl2->name, pgl2->links.Count()) ;
			m_Pagelists.Insert(pgl2->name, pgl2) ;
			break ;

		case WEBCMD_RGET:	//  Get a root page

			threadLog("%s. Doing WEBCMD_RGET\n", *_fn) ;
			threadLog("%s. Page=%s Crit=%s\n", *_fn, *wc.m_Url, *wc.m_Crit) ;

			//	Get root page first
			pDoc = Download(wc.m_Url) ;
			if (!pDoc)
				threadLog("%s. case 4. Could not fetch page %s\n", *_fn, *wc.m_Url) ;
			else
			{
				if (pDoc->Whatami() != DOCTYPE_HTML)
					threadLog("%s. Page %s not HTML\n", *_fn, *wc.m_Url) ;
				else
				{
					pHdoc = (hzDocHtml*) pDoc ;
					pHdoc->ExtractLinksBasic(pglinks, m_Domains, wc.m_Crit) ;
				}

				delete pDoc ;
			}

			//	Now aggregate the vector of links from the page to a vector of all links from all pages. Use a set to avoid repeats.
			for (n = 0 ; n < pglinks.Count() ; n++)
			{
				url = pglinks[n] ;
				if (!set_ctrl.Exists(url))
					allinks.Add(url) ;
			}

 			//	Starting at the site root and for each page, grab all links and go to each link in turn

			threadLog("%s. STAGE TWO Have %d links in history, %d links in 'all-links'\n", *_fn, m_vecHist.Count(), allinks.Count()) ;

			for (nStart = 0 ; nStart < allinks.Count() ; nStart = nCount)
			{
				now.SysDateTime() ;
				pglinks.Clear() ;

				for (nCount = nStart, nLimit = allinks.Count() ; nCount < nLimit ; nCount++)
				{
					url = allinks[nCount] ;

					threadLog("%s. Cosidering link %s - ", *_fn, *url.Whole()) ;

					if (m_mapHist.Exists(url))				{ threadLog("historic\n") ; continue ; }
					if (url == m_Authexit)					{ threadLog("exit-page\n") ; continue ; }
					if (!m_Domains.Exists(url.Domain()))	{ threadLog("URL %s outside domain\n", *url) ; continue ; }

					//	Page not yet visted so we visit it, put it in list of pages visited and get the links. Some of these links may add to
					//	the list of links.

					threadLog("Fetching\n") ;

					pDoc = Download(url) ;
					if (!pDoc)
						threadLog("%s. case 2. Could not fetch page %s\n", *_fn, *url) ;
					else
					{
						if (pDoc->Whatami() == DOCTYPE_HTML)
						{
							pHdoc = (hzDocHtml*) pDoc ;
							pHdoc->ExtractLinksBasic(pglinks, m_Domains, wc.m_Crit) ;

							//	Re-aggregate the all-links vector
							for (n = 0 ; n < pglinks.Count() ; n++)
							{
								url = pglinks[n] ;
								if (!set_ctrl.Exists(url))
									allinks.Add(url) ;
							}
						}

						delete pDoc ;
					}
				}
			}
			break ;

		case WEBCMD_POST:	//  Post a form. The form should have been previously downloaded and will be looked for by name

			threadLog("%s. Doing WEBCMD_POST\n", *_fn) ;
			pForm = m_Forms[wc.m_Output] ;
			if (!pForm)
				threadLog("%s. Warning: No such form as [%s]\n", *_fn, *wc.m_Output) ;

			//	Take the command's formdata and use it to populate the form's set of fields

			/*
			for (pi = pForm->fields ; pi.Valid() ; pi++)
				{ P = pi.Element() ; fvals.Insert(P.name, P.value) ; }
			for (pi = wc.m_Formdata ; pi.Valid() ; pi++)
				{ P = pi.Element() ; fvals.Insert(P.name, P.value) ; }

			for (n = 0 ; n < fvals.Count() ; n++)
			{
				P.name = fvals.GetKey(n) ;
				P.value = fvals.GetObj(n) ;
				flist.Add(P) ;
			}
			*/

			rc = HC.PostForm(hRet, wc.m_Url, m_Cookies, hdrs, wc.m_Formdata) ;
			if (rc != E_OK)
				{ threadLog("%s. Could not post form to %s\n", *_fn, *wc.m_Url) ; return rc ; }
			if (hRet != HTTPMSG_OK)
				{ threadLog("%s. Invalid response to post form (to %s)\n", *_fn, *wc.m_Url) ; return rc ; }

			//	Write out the login response
			if (m_Repos)
			{
				url = wc.m_Url ;
				S = m_Repos + "/" + url.Filename() ;
				S += ".response" ;
	
				os.open(*S) ;
				if (os.fail())
					{ threadLog("%s. Cannot write out header file %s\n", *_fn, *S) ; return E_WRITEFAIL ; }

				os << HC.m_Header ;
				os << "\r\n\r\n" ;
				os << HC.m_Content ;
				os.close() ;
			}
			break ;

		case WEBCMD_RSS:	//  Get an RSS feed

			threadLog("%s. Doing WEBCMD_RSS\n", *_fn) ;

			//	If XML selectors for RSS feed are not initialized, set them here
			if (!m_tagItem.m_Slct)	{ m_tagItem.m_Filt = (char*) 0 ; m_tagItem.m_Info = "node" ; m_tagItem.m_Slct = "item" ; }
			if (!m_tagUqid.m_Slct)	{ m_tagUqid.m_Filt = (char*) 0 ; m_tagUqid.m_Info = "node" ; m_tagUqid.m_Slct = "guid" ; }
			if (!m_tagLink.m_Slct)	{ m_tagLink.m_Filt = (char*) 0 ; m_tagLink.m_Info = "node" ; m_tagLink.m_Slct = "link" ; }
			if (!m_tagDesc.m_Slct)	{ m_tagDesc.m_Filt = (char*) 0 ; m_tagDesc.m_Info = "node" ; m_tagDesc.m_Slct = "description" ; }
			if (!m_tagDate.m_Slct)	{ m_tagDate.m_Filt = (char*) 0 ; m_tagDate.m_Info = "node" ; m_tagDate.m_Slct = "pubDate" ; }

			//	Get the feed
			rc = getRss_r(hRet, wc.m_Url, 0) ;
			threadLog("%s. Processed items\n", *_fn) ;
			break ;
		}
	}

	//	Write out manifest file
	rc = _savestatus() ;

	//	Clear documents
	for (n = 0 ; n < m_Pagelists.Count() ; n++)
	{
		pgl = m_Pagelists.GetObj(n) ;
		delete pgl ;
	}

	for (n = 0 ; n < cur.Count() ; n++)
	{
		pDoc = cur.GetObj(n) ;
		delete pDoc ;
	}

	return rc ;
}

hzEcode	hzWebhost::Scrape	(void)
{
	//	In general a website can be thought of as a source of 'rolling' news updates in which old pages are deleted, new pages created and existing pages can be
	//	modified on an ad-hoc basis. A scrape captures the current state of the website or a limited portion of it to file.
	//
	//	The scraping process runs through a set of known links for the website, downloading the page for each in turn. Each downloaded page is then examined for
	//	links. Links to domains other than the one in qestion are ignored. Links to such things as images are also ignored. Remaining links not found in the set
	//	of known links are added to this set. The process terminates when all the links have been attempted.
	//
	//	The set of known links will need to comprise the site's home-page and a login page if this exists and if it is not the same as the home page. These will
	//	usually be enough to 'bootstrap' the rest of the site.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	If no repository, no domain or no homepage has been specified
	//				E_NOTFOUND	If the login page was not located
	//				E_WRITEFAIL	If the login form recieved was not written to the repository
	//				E_OPENFAIL	If the visit status file could not be opened
	//				E_OK		If the scrape operation was successful

	_hzfunc("hzWebhost::Scrape") ;

	hzMapS<hzString,hzString>	formData ;	//	Set of name value pairs
	hzVect<hzString>			hdrs ;		//	Extra headers, needed for submit form

	hzList<hzPair>::Iter		ci ;		//	Root commands iterator

	hzSet<hzUrl>	set_ctrl ;		//	Initial links from processing config params
	hzVect<hzUrl>	pglinks ;		//	Links encountered within a given pages
	hzVect<hzUrl>	allinks ;		//	Links encountered within a given pages
	hzVect<hzUrl>	todo ;			//	Links encountered in the pages in ctrl

	ifstream		is ;			//	For reading in visit status file
	ofstream		os ;			//	For writing out visit status file at end of scrape

	hzDocMeta		mark ;			//	Document meta data

	hzChain			Response ;		//	Response from form submission
	hzDocument*		pDoc ;			//	Downloaded document
	hzDocHtml*		pHdoc ;			//	Set if downloaded document is a HTML page.
	hzPair			X ;				//	Root comand instance
	hzXDate			now ;			//	Date/time now (for cheking is pages have expired
	hzUrl			url ;			//	Temp link
	hzString		vs_fname ;		//	Visit status filename
	hzString		pagepath ;		//	Filepath for file to store downloaded page
	hzString		S ;				//	Temp string
	hzString		etag ;			//	Temp string
	uint32_t		nStart ;		//	Links iterator
	uint32_t		nLimit ;		//	Links iterator
	uint32_t		nCount ;		//	Links iterator
	uint32_t		n ;				//	Aggregation iterator
	hzEcode			rc = E_OK ;		//	Return code

	threadLog("%s. Called hzWebhost::Scrape\n", *_fn) ;

	//	Check if repository is set up (website is initialized)
	if (!m_Repos)
		{ threadLog("%s. Website is not properly initialized (no repository)\n", *_fn) ; return E_NOINIT ; }

	//	Is there anything to do?
	if (!m_Roots.Count())
		{ threadLog("%s. Website has no starting point (URL) for a WEB SCRAPE.\n", *_fn) ; return E_NOINIT ; }

	//	Get the home page
	if (*m_Homepage)
	{
		etag = (char*) 0 ;
		pDoc = Download(m_Homepage) ;
		if (!pDoc)
			{ threadLog("%s. Could not download page %s\n", *_fn, *m_Homepage) ; return E_NOINIT ; }
		m_docHome = pDoc ;
		threadLog("%s. HOMEPAGE SUCCESS\n", *_fn) ;
	}

	//	Login
	rc = Login() ;
	if (rc != E_OK)
		{ threadLog("%s. Login failed\n", *_fn) ; return rc ; }
	threadLog("%s. Login SUCCESS\n", *_fn) ;

	//	Run the root commands to obtain the set of roots. A root command may have either a URL or a 'link criteria' or both. If only a
	//	URL is present, this URL and ALL links found within it are added to the list of pages to process. If only a link criteria is
	//	present, the links found in the HOME page and the LOGIN RESPONSE page are tested against the criteria. If they match the link
	//	is added to the list of pages to process. If both a URL and a link criteria is found then the URL and any matching links found
	//	within it are added to the list of pages to process.

	threadLog("%s. Have %d root commands\n", *_fn, m_Roots.Count()) ;

	for (ci = m_Roots ; ci.Valid() ; ci++)
	{
		X = ci.Element() ;

		threadLog("%s. Page=%s Crit=%s\n", *_fn, *X.name, *X.value) ;

		//	Get the page
		if (X.name == "homepage")
		{
			//	No page to get, just compare the criteria to the home
			pHdoc = (hzDocHtml*) m_docHome ;
			pHdoc->ExtractLinksBasic(pglinks, m_Domains, X.value) ;
		}
		else if (X.name == "loginResponse")
		{
			//	No page to get, just compare the criteria to the login response
			pHdoc = (hzDocHtml*) m_resAuth ;
			pHdoc->ExtractLinksBasic(pglinks, m_Domains, X.value) ;
		}
		else
		{
			url = X.name ;
			if (!url)
				{ threadLog("%s. Root command invalid page %s\n", *_fn, *X.name) ; continue ; }

			etag = (char*) 0 ;
			pDoc = Download(url) ;
			if (!pDoc)
				threadLog("%s. case 1. Could not fetch page %s\n", *_fn, *url) ;
			else
			{
				if (pDoc->Whatami() != DOCTYPE_HTML)
					threadLog("%s. Page %s not HTML\n", *_fn, *url) ;
				else
				{
					pHdoc = (hzDocHtml*) pDoc ;
					pHdoc->ExtractLinksBasic(pglinks, m_Domains, X.value) ;
					threadLog("%s. Got page content, extracted %d links\n", *_fn, pglinks.Count()) ;
				}

				delete pDoc ;
			}
		}

		//	Now aggregate the vector of links from the page to a vector of all links from all pages. Use a set to avoid repeats.
		for (n = 0 ; n < pglinks.Count() ; n++)
		{
			url = pglinks[n] ;
			if (!set_ctrl.Exists(url))
				allinks.Add(url) ;
		}
	}

	/*
 	**	Starting at the site root and for each page, grab all links and go to each link in turn
	*/

	threadLog("%s. STAGE TWO Have %d links in history, %d links in 'all-links'\n", *_fn, m_vecHist.Count(), allinks.Count()) ;

	for (nStart = 0 ; nStart < allinks.Count() ; nStart = nCount)
	{
		now.SysDateTime() ;
		todo.Clear() ;

		for (nCount = nStart, nLimit = allinks.Count() ; nCount < nLimit ; nCount++)
		{
			url = allinks[nCount] ;

			threadLog("%s. Cosidering link %s - ", *_fn, *url.Whole()) ;

			if (m_mapHist.Exists(url))				{ threadLog("historic\n") ; continue ; }
			if (url == m_Authexit)					{ threadLog("exit-page\n") ; continue ; }
			if (!m_Domains.Exists(url.Domain()))	{ threadLog("URL %s outside domain\n", *url) ; continue ; }

			//	Page not yet visted so we visit it, put it in list of pages visited and get the links. Some of these links may add to
			//	the list of links.

			threadLog("Fetching\n") ;

			pDoc = Download(url) ;
			threadLog("Fetched page %p\n", pDoc) ;
			if (!pDoc)
				threadLog("%s. case 2. Could not fetch page %s\n", *_fn, *url) ;
			else
			{
				if (pDoc->Whatami() == DOCTYPE_HTML)
				{
					pHdoc = (hzDocHtml*) pDoc ;
					pHdoc->ExtractLinksBasic(pglinks, m_Domains, X.value) ;

					//	Re-aggregate the all-links vector
					for (n = 0 ; n < pglinks.Count() ; n++)
					{
						url = pglinks[n] ;
						if (!set_ctrl.Exists(url))
							allinks.Add(url) ;
					}
				}

				delete pDoc ;
			}
		}

		/*
		for (nAdded = nX = 0 ; nX < todo.Count() ; nX++)
		{
			//url = todo.GetObj(nX) ;
			url = todo[nX] ;	//.GetObj(nX) ;

			if (set_ctrl.Exists(url))
				continue ;
			nAdded++ ;
			set_ctrl.Insert(url) ;
		}

		todo.Clear() ;

		if (!nAdded)
			break ;
		*/
	}

	//	Write out manifest file
	rc = _savestatus() ;
	return rc ;
}

hzEcode	hzWebhost::getRss_r	(HttpRC& hRet, const hzUrl& feed, uint32_t nLevel)
{
	//	Recursive fetch of RSS documents. The supplied URL is downloaded and loaded into an XML document. There it is tested to ensure it is an
	//	XML document. The RSS feed is assumed to contain only links. These links may be to HTML pages or other (sub RSS feeds). The HTML pages
	//	are end points of the process. They are downloaded but any links they may contain are recorded but not followed. The sub-RSS feeds are
	//	then processed by recursive call to this function.
	//
	//	Arguments:	1) hRet		Set by this operation
	//				2) feed		The RSS URL
	//				3) nLevel	RSS Hierarchy
	//
	//	Returns:	E_NODATA	If the download failed
	//				E_TYPE		If the downloaded material does not appear to be XML
	//				E_FORMAT	If the downloaded material could not be loaded into an XML document
	//				E_ARGUMENT	If the RSS tags are not defined
	//				E_NOTFOUND	If no tags were found in the RSS
	//				E_OK		If the RSS data was collected

	_hzfunc("hzWebhost::getRss_r") ;

	hzVect<hzXmlNode*>	linx ;		//	Links found in (this) RSS feed page
	hzVect<hzUrl>		todo ;		//	Links found in RSS feed page (additions to this are controlled by the set above)

	hzDocXml		X ;				//	For loading of RSS feed pages and extraction of links
	hzXmlNode*		pN1 ;			//	Nodes (containing <item>)
	hzXmlNode*		pN2 ;			//	Nodes (containing <item> subnodes of title, link, description)
	hzDocMeta*		pMark ;			//	Document meta data
	hzDocument*		pDoc ;			//	Document found at URL (could be XML of HTML)
	hzUrl			page ;			//	Temp link
	hzString		desc ;			//	RSS article description
	hzString		dstr ;			//	RSS article date
	hzString		uqid ;			//	Unique ID of RSS item
	hzString		title ;			//	RSS article title
	uint32_t		nIndex ;		//	Links iterator
	hzEcode			rc = E_OK ;		//	Return code

	//	Fetch the current RSS document
	pDoc = Download(feed) ;
	if (rc != E_OK)
		{ threadLog("%s. Could not fetch URL %s\n", *_fn, *feed) ; return rc ; }

	//	If not an XML document then it is just a page. Nothing further.
	if (pDoc->Whatami() != DOCTYPE_XML)
		{ threadLog("%s. case 1. Fetched feed (%s) is not of doctype XML\n", *_fn, *feed) ; return E_TYPE ; }

	nLevel++ ;

	//	Load current RSS document into XML document tree
	rc = X.Load(HC.m_Content) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not load feed %s", *feed) ;

	//	The page is an RSS document so select the <itme> tags
	rc = X.FindNodes(linx, m_tagItem.m_Slct) ;
	threadLog("%s. Found %d <item> tags in feed %s\n", *_fn, linx.Count(), *feed) ;
	if (rc != E_OK)
		return rc ;

	for (nIndex = 0 ; nIndex < linx.Count() ; nIndex++)
	{
		threadLog("case 1\n") ;
		pN1 = linx[nIndex] ;

		title = (char*) 0 ; desc = (char*) 0 ; page = (char*) 0 ; uqid = (char*) 0 ; dstr = (char*) 0 ;

		for (pN2 = pN1->GetFirstChild() ; pN2 ; pN2 = pN2->Sibling())
		{
			threadLog("case 2\n") ;
			if (pN2->NameEQ(*m_tagTitl.m_Slct))	{ title = pN2->m_fixContent ; continue ; }
			if (pN2->NameEQ(*m_tagDesc.m_Slct))	{ desc = pN2->m_fixContent ; continue ; }
			if (pN2->NameEQ(*m_tagLink.m_Slct))	{ page = pN2->m_fixContent ; continue ; }
			if (pN2->NameEQ(*m_tagUqid.m_Slct))	{ uqid = pN2->m_fixContent ; continue ; }
			if (pN2->NameEQ(*m_tagDate.m_Slct))	{ dstr = pN2->m_fixContent ; continue ; }
		}
		threadLog("case 3\n") ;

		if (!page)
			{ threadLog("%s. case 1: title=%s; link=null uqid=%s\n", *_fn, *title, *uqid) ; page = uqid ; }

		if (!page)
			{ threadLog("%s. case 2: title=%s; link=null uqid=%s\n", *_fn, *title, *uqid) ; continue ; }

		threadLog("%s. title=%s; link=%s\n", *_fn, *title, *page) ;

		if (m_mapHist.Exists(page))
			threadLog("%s. Exists in history, page %s\n", *page) ;
		else
		{
			pMark = new hzDocMeta() ;
			pMark->m_Title = title ;
			pMark->m_Desc = desc ;
			pMark->m_urlReq = page ;
			if (dstr)
				pMark->m_Modified.SetDateTime(*dstr) ;
			//todo.Insert(page) ;
			todo.Add(page) ;
			threadLog("%s. Adding to history, page %s\n", *page) ;
		}
	}

	//	Fetch all the new links found above by recursive call
	for (nIndex = 0 ; nIndex < todo.Count() ; nIndex++)
	{
		page = todo[nIndex] ;
		//pMark = m_mapHist[page] ;

		threadLog("%s. Processing %s\n", *_fn, *page) ;
		rc = getRss_r(hRet, page, nLevel) ;
	}

	return rc ;
}

hzEcode	hzWebhost::GetRSS	(void)
{
	//	In general a website can be thought of as a source of 'rolling' news updates in which old pages are deleted, new pages created
	//	and existing pages can be modified on an ad-hoc basis. The RSS feeds allow greter ease when syncing an external website to the
	//	local machine. By periodically reading one or more RSS feeds one can obtain a set of links which can generally be taken as the
	//	set of pages deemed 'current' by the website. By comparing these links to a history file of already fetched links, new pages
	//	can be added to a respository as they appear on the site. The RSS feeds are just XML files containing links.
	//
	//	This function will obtain all the RSS feeds from the site, garner all the links from them and then download any pages from the
	//	links that are not already in the site history. The feeds themselves are not saved as these will be fetched again.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	If the repository for the webhost has not previously been defined
	//				E_OPENFAIL	If the visit status file could not be opened
	//				E_NODATA	If the download failed
	//				E_TYPE		If the downloaded material does not appear to be XML
	//				E_FORMAT	If the downloaded material could not be loaded into an XML document
	//				E_ARGUMENT	If the RSS tags are not defined
	//				E_NOTFOUND	If no tags were found in the RSS
	//				E_OK		If the RSS data was collected

	_hzfunc("hzWebhost::GetRSS") ;

	hzList<hzUrl>::Iter	fi ;		//	RSS feeds iterator

	hzUrl		feed ;				//	Temp link
	HttpRC		hRet ;				//	HTML return code
	hzEcode		rc = E_OK ;			//	Return code

	threadLog("%s. Called\n", *_fn) ;

	//	Login
	rc = Login() ;
	if (rc != E_OK)
		{ threadLog("%s. Login failed\n", *_fn) ; return rc ; }

	//	Get the home page if one applies. Do this regardless of weather we already have it because we need the cookie
	if (!m_Feeds.Count())
		{ threadLog("%s. Website has no starting point (URL) for an RSS feed.\n", *_fn) ; return E_NOINIT ; }

	//	If XML selectors for RSS feed are not initialized, set them here
	if (!m_tagItem.m_Slct)	{ m_tagItem.m_Filt = (char*) 0 ; m_tagItem.m_Info = "node" ; m_tagItem.m_Slct = "item" ; }
	if (!m_tagUqid.m_Slct)	{ m_tagUqid.m_Filt = (char*) 0 ; m_tagUqid.m_Info = "node" ; m_tagUqid.m_Slct = "guid" ; }
	if (!m_tagLink.m_Slct)	{ m_tagLink.m_Filt = (char*) 0 ; m_tagLink.m_Info = "node" ; m_tagLink.m_Slct = "link" ; }
	if (!m_tagDesc.m_Slct)	{ m_tagDesc.m_Filt = (char*) 0 ; m_tagDesc.m_Info = "node" ; m_tagDesc.m_Slct = "description" ; }
	if (!m_tagDate.m_Slct)	{ m_tagDate.m_Filt = (char*) 0 ; m_tagDate.m_Info = "node" ; m_tagDate.m_Slct = "pubDate" ; }

	/*
	**	Fetch all the feed XML documents from the RSS source(s)
	*/

	for (fi = m_Feeds ; fi.Valid() ; fi++)
	{
		feed = fi.Element() ;

		//	Get the feed
		rc = getRss_r(hRet, feed, 0) ;
		threadLog("%s. Processed items\n", *_fn) ;
	}

	//	Write out visit status file
	rc = _savestatus() ;
	return rc ;
}

hzDocument*	hzWebhost::Download	(const hzUrl& url)
{
	//	Fetch the page found at the supplied URL and return as a document (either XML or HTML).
	//
	//	Note that if the page has already been downloaded (is in the site's history) then it is only downloaded again if it the time to
	//	live has expired. If the page is not downloaded then this function will reload it from file.
	//
	//	Arguments:	1) url		The URL of the file/resource to download
	//
	//	Returns:	Pointer to newly allocated document. Must be deleted after use.

	_hzfunc("hzWebhost::Download") ;

	static uint32_t	nlast = 0 ;		//	Last point reached (for download rsumption)

	ofstream	os ;				//	To write out page contents
	hzDocument*	pDoc = 0 ;			//	Document downloaded
	hzDocXml*	pXdoc = 0 ;			//	XML Document downloaded
	hzDocHtml*	pHdoc = 0 ;			//	HTML Document downloaded

	hzDocMeta*	pMark ;				//	Document meta data
	hzXDate		now ;				//	Date & Time now
	hzString	S ;					//	Temp string
	HttpRC		hc ;				//	HTTP server return code
	hzEcode		rc ;				//	Return code
	bool		bHist = false ;		//	Set if url is already in history and downloaded again because of being out of date
	char		numbuf [8] ;		//	Working buffer

 	/*
	**	Check URL, insert in visited links if not already there
	*/

	if (!url)
		{ threadLog("%s. No supplied address\n", *_fn) ; return 0 ; }
	threadLog("%s: FETCHING PAGE: %s\n", *_fn, *url) ;

	now.SysDateTime() ;

	if (!(m_Opflags & WEBFLG_FORCE))
	{
		if (m_mapHist.Exists(url))
		{
			//	The requested URL exists in the repository already. We check if it has expired and if not we terminate with OK

			pMark = m_mapHist[url] ;
			bHist = true ;
			threadLog("%s: Page %s is historic\n", *_fn, *url) ;

			//	Create a document of the right type (XML or HTML)

			if (pMark->m_Doctype == DOCTYPE_HTML)
				pDoc = pHdoc = new hzDocHtml() ;
			else if (pMark->m_Doctype == DOCTYPE_XML)
				pDoc = pXdoc = new hzDocXml() ;
			else
				pDoc = pHdoc = new hzDocHtml() ;

			pDoc->SetMeta(*pMark) ;

			//	Check if expiry is known and if so if it has expired

			if (pMark->m_Expires.IsSet())
			{
				if (pMark->m_Expires < now)
				{
					//	Set the markers and return
					if (pMark->m_Doctype == DOCTYPE_XML)
					{
						//	XML
						pDoc = pXdoc = new hzDocXml() ;
						pDoc->SetMeta(*pMark) ;
						rc = pDoc->Load(HC.m_Content) ;
					}
					else
					{
						//	HTML
						pDoc = pHdoc = new hzDocHtml() ;
						pDoc->SetMeta(*pMark) ;
						rc = pDoc->Load(HC.m_Content) ;
					}

					threadLog("%s: DOWNLOAD PREVIOUS (error=%s)\n\n", *_fn, Err2Txt(rc)) ;
					return pDoc ;
				}
			}

			//	At this point either the expiry date is unknown or it is known and has expired. Load from file

			if (!HC.m_Content.Size())
			{
				threadLog("Case 1 Bloody thing is empty!\n") ;
				return 0 ;
			}

			rc = pDoc->Load(HC.m_Content) ;
			if (rc != E_OK)
				threadLog("%s: LOAD failed (error=%s)\n\n", *_fn, Err2Txt(rc)) ;

			return pDoc ;
		}
	}

	//	The requested URL is not in the history. Create the document meta for it and download it.

	S = url.Filename() ;

	pMark = new hzDocMeta() ;
	pMark->m_urlReq = url ;
	pMark->m_urlAct = url ;
	pMark->m_Id = m_mapHist.Count() ;
	sprintf(numbuf, "/%04d", pMark->m_Id) ;
	pMark->m_Filename = m_Repos + numbuf + S ;

	/*
 	**	Get page content and process it into a tree
	*/

	threadLog("%s: GETTIG PAGE: %s\n", *_fn, *url) ;
	rc = HC.GetPage(hc, url, m_Cookies, pMark->m_Etag) ;
	if (rc != E_OK)
	{
		threadLog("%s: FAILED (error=%s) synopsis\n", *_fn, Err2Txt(rc)) ;
		threadLog(HC.m_Error) ;
		return 0 ;
	}

	if (HC.m_Redirect)
		pMark->m_urlAct = HC.m_Redirect ;
	pMark->m_Modified = HC.m_Modified ;

	threadLog("%s. HTTP Return code = %d, cookie (value %s, path %s)\n", *_fn, (uint32_t) hc, *m_CookieSess, *m_CookiePath) ;

	/*
 	**	Write out header to .hdr file and content to .con file
	*/

	if (m_Repos)
	{
		os.open(*pMark->m_Filename) ;
		if (os.fail())
			threadLog("%s. Cannot write out header file %s\n", *_fn, *pMark->m_Filename) ;
		else
		{
			os << HC.m_Content ;
			os.close() ;
		}
		os.clear() ;
	}

	/*
 	**	Add the page but only process pages that are of a known HTML type .htm, .html, .shtml, .xhtml etc
	*/

	threadLog("%s. PROCESSING Content: %d bytes\n", *_fn, HC.m_Content.Size()) ;
	if (!HC.m_Content.Size())
	{
		threadLog("Case 2 Bloody thing is empty!\n") ;
		//threadLog(HC.m_Error) ;
		return 0 ;
	}
	pMark->m_Doctype = DeriveDoctype(HC.m_Content) ;

	rc = E_NODATA ;
	if (pMark->m_Doctype == DOCTYPE_XML)
	{
		//	XML
		pDoc = pXdoc = new hzDocXml() ;
		pXdoc->Init(url) ;
		rc = pXdoc->Load(HC.m_Content) ;
	}
	else
	{
		//	HTML
		pDoc = pHdoc = new hzDocHtml() ;
		pHdoc->Init(url) ;
		rc = pHdoc->Load(HC.m_Content) ;

		if (rc != E_OK)
		threadLog("Case 2 Bloody thing failed (error=%s)!\n", Err2Txt(rc)) ;
	}

	if (rc != E_OK)
	{
		threadLog("%s. Load page failed error=%s\n", *_fn, Err2Txt(rc)) ;
		//delete pDoc ;
		//return 0 ;
	}

	pDoc->SetMeta(*pMark) ;

	//	Place the URL in the site's history
	m_mapHist.Insert(pMark->m_urlReq, pMark) ;
	threadLog("%s. Inserted URL %s\n", *_fn, *pMark->m_urlReq) ;
	if (pMark->m_urlAct != pMark->m_urlReq)
	{
		m_mapHist.Insert(pMark->m_urlAct, pMark) ;
		threadLog("%s. Inserted URL %s\n", *_fn, *pMark->m_urlAct) ;
	}

	if (!bHist)
		m_vecHist.Add(pMark) ;

	if (pXdoc)
		threadLog("%s: DOWNLOAD SUCCESS XML Page %s. Now have %d (%d) items in history\n\n", *_fn, *url, m_mapHist.Count(), nlast) ;
	if (pHdoc)
		threadLog("%s: DOWNLOAD SUCCESS Page %s has %d links. Now have %d (%d) items in history\n\n",
			*_fn, *url, pHdoc->m_vecLinks.Count(), m_mapHist.Count(), nlast) ;

	threadLog(HC.m_Error) ;
	return pDoc ;
}
