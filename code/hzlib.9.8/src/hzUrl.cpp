//
//	File:	hzUrl.cpp
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
//	Implimentation of the hzUrl class.
//

#include <iostream>

#include <stdarg.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzProcess.h"

#define	URL_FACTOR	9	//	This is added to the URL string size to accomodate the copy count, lengths of the protocol, domain, port and resource components as
						//	well as the null terminator.

#define URI_FACTOR	12	//	This is added to the URI string size to accomodate the copy count and URI components as defined in _uri_space below

class	_uri_space
{
	//	Internal structure for URI is scheme:[//authority]path[?query][#fragment] where the authority = [userinfo@]host[:port]. Note that while the authority part can include user
	//	information, in most cases only the host is present (and sometimes the port)

	uint16_t	m_Scheme ;		//	URI Scheme code
	uint16_t	m_nPort ;		//	Port number
	uint16_t	m_lenQuery ;	//	Length of query component
	uchar		m_copy ;		//	Copy counter
	uchar		m_lenUser ;		//	Length of authority part userinfo (usually 0)
	uchar		m_lenHost ;		//	Length of authority host
	uchar		m_lenPath ;		//	Length of path
	uchar		m_lenFrag ;		//	Length of fragment
	char		m_data[5] ;		//	First part of data

	_uri_space	(void)	{ m_Scheme = m_nPort = m_lenQuery = 0 ; m_copy = m_lenUser = m_lenHost = m_lenPath = m_lenFrag = 0 ; }

	uint32_t	Length	(void)	{ return URI_FACTOR + m_lenQuery + m_lenUser + m_lenHost + m_lenPath + m_lenFrag ; }
} ;

class	_url_space
{
	//	Internal structure for URL to facilitate soft copies	URI = scheme:[//authority]path[?query][#fragment]

public:
	uchar		m_copy ;		//	Copy counter
	uchar		m_lenProt ;		//	Length of protocol component
	uchar		m_lenDom ;		//	Length of domain component
	uchar		m_lenPort ;		//	Length of port component
	uint16_t	m_lenRes ;		//	Length of resource component
	uint16_t	m_port ;		//	Port number
	char		m_data[8] ;		//	First part of data

	_url_space	(void)	{ m_copy = m_lenRes = m_port = 0 ; m_lenProt = m_lenDom = m_lenPort = m_data[0] = 0 ; }
} ;

/*
**	Prototypes
*/

uint32_t	_strAlloc	(uint32_t size) ;
void		_strFree	(uint32_t obj, uint32_t size) ;
void*		_strXlate	(uint32_t addr) ;

/*
**	Global constants
*/

global	const hzUrl	_hz_null_hzUrl ;	//	Null URL

/*
**	hzUrl public methods
*/

void	hzUrl::Clear	(void)
{
	//	Clear the contents of this instance
	//
	//	Arguments:	None
	//	Returns:	None

	_url_space*	thisCtl ;	//	This URL space

	if (m_addr)
	{
		thisCtl = (_url_space*) _strXlate(m_addr) ;

		if (_hzGlobal_MT)
		{
			__sync_add_and_fetch(&(thisCtl->m_copy), -1) ;

			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->m_lenRes + thisCtl->m_lenProt + thisCtl->m_lenDom + thisCtl->m_lenPort + URL_FACTOR) ;
		}
		else
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->m_lenRes + thisCtl->m_lenProt + thisCtl->m_lenDom + thisCtl->m_lenPort + URL_FACTOR) ;
		}

		m_addr = 0 ;
	}
}

uint32_t	hzUrl::Length	(void) const
{
	//	Return the total length of the URL (including the http:// bit)
	//
	//	Arguments:	None
	//	Returns:	Number being length of whole URL string

	_url_space*	thisCtl ;	//	This URL space

	if (!m_addr)
		return 0 ;

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	return thisCtl->m_lenRes + thisCtl->m_lenProt + thisCtl->m_lenDom + thisCtl->m_lenPort ;
}

uint32_t	hzUrl::Port		(void) const
{
	//	Return the port (default is 80) that the URL states (if any)
	//
	//	Arguments:	None
	//
	//	Returns:	Number being any port number specified in the URL

	_url_space*	thisCtl ;	//	This URL space

	if (!m_addr)
		return 0 ;

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	return thisCtl->m_port ;
}

bool	hzUrl::IsSSL	(void) const
{
	//	Return true is the URL indicates a would-be connection would use SSL
	//
	//	Arguments:	None
	//
	//	Returns:	True	If the port is set to 443
	//				False	Otherwise

	_url_space*	thisCtl ;	//	This URL space

	if (!m_addr)
		return false ;

	thisCtl = (_url_space*) _strXlate(m_addr) ;

	if (Port() == 443)
		return true ;
	return false ;
}

hzString	hzUrl::Whole	(void) const
{
	//	Return a pointer to the whole string as a null terminated sequence.
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString by value being whole URL

	_url_space*		thisCtl ;	//	This URL space
	hzString		S ;			//	Target hzString for whole URL

	if (m_addr)
	{
		thisCtl = (_url_space*) _strXlate(m_addr) ;
		S = thisCtl->m_data ;
	}

	return S ;
}

hzString	hzUrl::Domain	(void) const
{
	//	Create and return a string consisting of the domain name only.
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString by value being domain part of the URL

	_url_space*		thisCtl ;	//	This URL space
	hzString		S ;			//	Target hzString for domain part of URL

	if (m_addr)
	{
		thisCtl = (_url_space*) _strXlate(m_addr) ;
		S.SetValue(thisCtl->m_data + thisCtl->m_lenProt, (uint32_t) thisCtl->m_lenDom) ;
	}

	return S ;
}

hzString	hzUrl::Resource	(void) const
{
	//	Create and return a string consisting of the resource component only.
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString by value being resource part of the URL

	_url_space*		thisCtl ;	//	This URL space
	hzString		S ;			//	Target hzString for resource part of URL

	if (m_addr)
	{
		thisCtl = (_url_space*) _strXlate(m_addr) ;
		S = thisCtl->m_data + thisCtl->m_lenProt + thisCtl->m_lenDom + thisCtl->m_lenPort ;
	}

	return S ;
}

const char*	hzUrl::operator*	(void) const
{
	//	Returns the URL data (a null terminated string)
	//
	//	Arguments:	None
	//	Returns:	Pointer to value as null terminated string

	_url_space*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return 0 ;

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	return thisCtl->m_data ;
}

hzUrl::operator const char*	(void) const
{
	//	Returns the string data (a null terminated string)
	//
	//	Arguments:	None
	//	Returns:	Pointer to value as null terminated string

	_url_space*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return 0 ;

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	return thisCtl->m_data ;
}

hzUrl&	hzUrl::SetValue		(const hzString domain, const hzString resource, bool bSecure, uint32_t nPort)
{
	//	This is used as an alternative to the assignement operator to deal with such as links in webpages. These may be full URLs with
	//	or without the http:// or https:// but with the domain - or they can start with a / and consist only of the resource. Where the
	//	link lacks the domain, the supplied domain is used. Where the link does have a domain specified this takes precedence.
	//
	//	Arguments:	1)	domain		The domain name part of the URL
	//				2)	resource	The resource part
	//				3)	bSecure		Use SSL so https:// instead of http://
	//				4)	nPort		Port number if any
	//
	//	Returns:	Reference to this URL intance

	_url_space*		thisCtl ;		//	This URL space
	const char*		r ;				//	Resource iterator
	hzString		res ;			//	Resource (allowing a truncate after ?)
	uint32_t		lenTotal ;		//	Length of whole URL
	uint32_t		lenProt ;		//	Lenth of protocol part
	uint32_t		lenDom ;		//	Length of domain part
	uint32_t		lenRes ;		//	Length of resource part
	uint32_t		lenPort = 0 ;	//	Length of port part
	bool			bPort = false ;	//	Port specified indicator 

	/*
	**	Test arguments
	*/

	Clear() ;

	if (!domain)	return *this ;
	if (!resource)	return *this ;
	res = resource ;
	res.TruncateUpto("?") ;

	if (memcmp(*res, "http://", 7) == 0)
	{
		operator=(res) ;
		return *this ;
	}

	//	If the port is not set it is determined by the protocol (http is 80 or https is 443).

	if (!nPort)
		nPort = bSecure ? 443 : 80 ;
	else
	{
		//	Don't include the :port_no notation unless we have a non-standard port

		if (bSecure)
			bPort = nPort == 443 ? false : true ;
		else
			bPort = nPort == 80 ? false : true ;
	}

	if (bPort)
		lenPort = nPort > 9999 ? 6 : nPort > 999 ? 5 : nPort > 99 ? 4 : nPort > 9 ? 3 : 2 ;
	else
		lenPort = 0 ;

	lenProt = bSecure ? 8 : 7 ;
	lenDom = domain.Length() ;
	r = *res ;
	if (r[0] == CHAR_FWSLASH)
	{
		lenRes = res.Length() ;
		r++ ;
	}
	else
		lenRes = res.Length() + 1 ;

	/*
	**	Compile finished URL
	*/

	lenTotal = lenProt + lenDom + lenPort + lenRes ;
	m_addr = _strAlloc(lenTotal + URL_FACTOR) ;
	thisCtl = (_url_space*) _strXlate(m_addr) ;

	thisCtl->m_copy = 1 ;				//	No copies
	thisCtl->m_lenProt = lenProt ;	//	Length of protocol component
	thisCtl->m_lenDom = lenDom ;		//	Length of domain component
	thisCtl->m_lenPort = lenPort ;	//	Length of port component
	thisCtl->m_lenRes = lenRes ;		//	MSB of length of resource component
	thisCtl->m_port = nPort ;			//	Port number

	if (bSecure)
		memcpy(thisCtl->m_data, "https://", lenProt) ;
	else
		memcpy(thisCtl->m_data, "http://", lenProt) ;

	memcpy(thisCtl->m_data + lenProt, *domain, lenDom) ;

	if (bPort)
		sprintf(thisCtl->m_data + lenProt + lenDom, ":%d", nPort) ;

	thisCtl->m_data[lenProt + lenDom + lenPort] = CHAR_FWSLASH ;
	memcpy(thisCtl->m_data + lenProt + lenDom + lenPort + 1, r, lenRes - 1) ;
	thisCtl->m_data[lenTotal] = 0 ;

	return *this ;
}

//	Encoding:	If the internal buffer exists the first seven bytes will have the following meanings:-
//
//		m_buf[0] - No copies
//		m_buf[1] - Length of protocol component
//		m_buf[2] - Length of domain component
//		m_buf[3] - Length of port component
//		m_buf[4] - MSB of length of resource component
//		m_buf[5] - LSB of length of resource component
//		m_buf[6] - MSB of port
//		m_buf[7] - LSB of port

hzUrl&	hzUrl::operator=	(const char* url)
{
	//	Assign the URL by character string
	//
	//	Arguments:	1)	url	The whole URL as string
	//
	//	Returns:	Reference to this URL intance

	_hzfunc("hzUrl::operator=") ;

	_url_space*	thisCtl ;			//	This URL space
	const char*	pDom ;				//	Start of domain component
	const char*	pRes ;				//	Start of resource component
	const char*	_ptr ;				//	Char iterator

	uint32_t	lenProt = 0 ;		//	Length of protocol indicator (eg http://)
	uint32_t	lenDom = 0 ;		//	Length of domain name
	uint32_t	lenPort = 0 ;		//	Length of port indicator if present
	uint32_t	lenRes = 0 ;		//	Length of resource
	uint32_t	lenTotal ;			//	Length of whole string

	uint32_t	nPort = 80 ;		//	Port number
	uint32_t	nProto = 80 ;		//	Presumed port number of protocol
	uint32_t	nAlphas = 0 ;		//	Number of alphanum chars (must be at least one)
	uint32_t	nPeriod = 0 ;		//	Number of periods (must be at least one)
	uint32_t	nWhite = 0 ;		//	Number of whitespace chars
	bool		bPort = false ;		//	True only if a port component specified (:num)

	Clear() ;
	if (!url || !url[0])
		return *this ;

	threadLog("URL [%s]\n", url) ;

	//	Strip leading spaces to find real start
	for (_ptr = url ; *_ptr <= CHAR_SPACE ; _ptr++) ;

	/*
	**	Handle protocol component - Remove http:// or https:// to get to domain
	*/

	if (strstr(_ptr, "//"))
	{
		if		(!memcmp(_ptr, "http://", 7))	{ nProto = nPort = 80 ; lenProt = 7 ; }
		else if (!memcmp(_ptr, "https://", 8))	{ nProto = nPort = 443 ; lenProt = 8 ; }
		else if (!memcmp(_ptr, "ftp://", 6))	{ nProto = nPort = 21 ; lenProt = 6 ; }
		else if (!memcmp(_ptr, "ftp://", 6))	{ nProto = nPort = 21 ; lenProt = 6 ; }
		else
		{
			threadLog("%s. Bad scheme\n", *_fn) ;
			return *this ;
		}

		_ptr += lenProt ;
	}

	pDom = _ptr ;

	/*
	**	Handle domain component: Read up to the end of the domain name. This could be the end of the test string or it
	**	could be a forward slash or a colon (for the port number)
	*/

	for (; *_ptr ; lenDom++, _ptr++)
	{
		if (*_ptr == CHAR_COLON)	break ;
		if (*_ptr == CHAR_FWSLASH)	break ;

		if (*_ptr == CHAR_PERIOD)	{ nPeriod++ ; continue ; }
		if (*_ptr == CHAR_MINUS)	{ nAlphas++ ; continue ; }
		if (*_ptr <= CHAR_SPACE)	{ nWhite++ ; continue ; }

		nAlphas++ ;
	}

	//	Deal with failures
	if (nWhite)		{ threadLog("%s. Has whitespace\n", *_fn) ; return *this ; }
	if (!nPeriod)	{ threadLog("%s. No periods\n", *_fn) ; return *this ; }
	if (!nAlphas)	{ threadLog("%s. No alphas\n", *_fn) ; return *this ; }

	/*
	**	Handle port component: Deal with case where domain string is terminated by a port indicator
	*/

	if (*_ptr == CHAR_COLON)
	{
		_ptr++ ;
		for (nPort = 0, lenPort++ ; IsDigit(*_ptr) ; lenPort++)
			{ nPort *= 10 ; nPort += (*_ptr - '0') ; _ptr++ ; }

		if (nPort >= 65536)
		{
			threadLog("%s. Bad port\n", *_fn) ;
			return *this ;
		}
		bPort = true ;
	}

	//	Should now have a terminator (<= space) or the / marking start of resource
	if (*_ptr >= CHAR_SPACE && *_ptr != CHAR_FWSLASH)
		return *this ;

	/*
	**	Handle resource component: Read up to end of resource
	*/

	pRes = _ptr ;
	for (; *_ptr > ' ' && *_ptr != CHAR_QUERY ; lenRes++, _ptr++) ;

	/*
	**	Compile finished URL
	*/

	lenTotal = lenProt + lenDom + lenPort + lenRes ;
	m_addr = _strAlloc(lenTotal + URL_FACTOR) ;
	thisCtl = (_url_space*) _strXlate(m_addr) ;

	//	Do control part
	thisCtl->m_copy = 1 ;				//	No copies
	thisCtl->m_lenProt = lenProt ;		//	Length of protocol component
	thisCtl->m_lenDom = lenDom ;		//	Length of domain component
	thisCtl->m_lenPort = lenPort ;		//	Length of port component
	thisCtl->m_lenRes = lenRes ;		//	MSB of length of resource component
	thisCtl->m_port = nPort ;			//	Port number

	//	Do protocol part
	if		(nProto == 80)	memcpy(thisCtl->m_data, "http://", lenProt) ;
	else if (nProto == 443)	memcpy(thisCtl->m_data, "https://", lenProt) ;
	else if (nProto == 21)	memcpy(thisCtl->m_data, "ftp://", lenProt) ;
	else
	{
		threadLog("%s. Bad scheme\n", *_fn) ;
		return *this ;
	}

	//	Do domain part
	memcpy(thisCtl->m_data + lenProt, pDom, lenDom) ;

	//	Do port part
	if (bPort)
		sprintf(thisCtl->m_data + lenProt + lenDom, ":%d", nPort) ;

	//	Do resource part
	memcpy(thisCtl->m_data + lenProt + lenDom + lenPort, pRes, lenRes) ;
	thisCtl->m_data[lenTotal] = 0 ;
	
	return *this ;
}

hzUrl&	hzUrl::operator=	(const hzString& S)
{
	//	Assign the URL by string instance
	//
	//	Arguments:	1)	S	The string value
	//	Returns:	Reference to this URL intance

	Clear() ;
	return operator=(*S) ;
}

hzUrl&	hzUrl::operator=	(const hzUrl& url)
{
	//	Assign the URL by copying another hzUrl instance
	//
	//	Argument:	U	The URL value
	//
	//	Returns:	Reference to this URL intance

	_url_space*	suppCtl ;		//	Supplied URL space

	Clear() ;
	if (!url.m_addr)
		return *this ;

	suppCtl = (_url_space*) _strXlate(url.m_addr) ;

	if (_hzGlobal_MT)
		__sync_add_and_fetch(&(suppCtl->m_copy), 1) ;
	else
		suppCtl->m_copy++ ;

	m_addr = url.m_addr ;
	return *this ;
}

/*
**	Compare operators
*/

bool	hzUrl::operator==	(const hzUrl& testUrl) const
{
	//	Test for equality between this URL and a supplied test value
	//
	//	Argument:	U	The test URL
	//
	//	Returns:	True	If this hzUrl is equal to the operand hzUrl
	//				False	Otherwise

	if (m_addr == testUrl.m_addr)	return true ;
	if (!m_addr && testUrl.m_addr)	return false ;
	if (m_addr && !testUrl.m_addr)	return false ;

	_url_space*	thisCtl ;		//	This URL space
	_url_space*	suppCtl ;		//	Supplied URL space

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	suppCtl = (_url_space*) _strXlate(testUrl.m_addr) ;

	if (thisCtl->m_lenRes != suppCtl->m_lenRes)		return false ;
	if (thisCtl->m_port != suppCtl->m_port)			return false ;
	if (thisCtl->m_lenProt != suppCtl->m_lenProt)	return false ;
	if (thisCtl->m_lenDom != suppCtl->m_lenDom)		return false ;
	if (thisCtl->m_lenPort != suppCtl->m_lenPort)	return false ;

	return strcmp(thisCtl->m_data, suppCtl->m_data) ? false : true ;
}

bool	hzUrl::operator<	(const hzUrl& testUrl) const
{
	//	Test for this URL being less than the supplied test URL. The domain part takes precedence, followed by the resource part and finally the port.
	//
	//	Argument:	U	The test URL
	//
	//	Returns:	True	If this hzUrl is equal to the operand hzUrl
	//				False	Otherwise

	if (m_addr == testUrl.m_addr)	return false ;
	if (!m_addr && testUrl.m_addr)	return true ;
	if (m_addr && !testUrl.m_addr)	return false ;

	_url_space*	thisCtl ;		//	This URL space
	_url_space*	suppCtl ;		//	Supplied URL space

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	suppCtl = (_url_space*) _strXlate(testUrl.m_addr) ;

	return Whole() < testUrl.Whole() ? true : false ;
}

bool	hzUrl::operator<=	(const hzUrl& testUrl) const
{
	//	Test for this URL being lexically less or equal to than the supplied test URL
	//
	//	Argument:	U	The test URL
	//
	//	Returns:	True	If this hzUrl is equal to the operand hzUrl
	//				False	Otherwise

	if (m_addr == testUrl.m_addr)	return false ;
	if (!m_addr && testUrl.m_addr)	return true ;
	if (m_addr && !testUrl.m_addr)	return false ;

	_url_space*	thisCtl ;		//	This URL space
	_url_space*	suppCtl ;		//	Supplied URL space

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	suppCtl = (_url_space*) _strXlate(testUrl.m_addr) ;

	return Whole() <= testUrl.Whole() ? true : false ;
}

bool	hzUrl::operator>	(const hzUrl& testUrl) const
{
	//	Test for this URL being lexically greater than the supplied test URL
	//
	//	Argument:	U	The test URL
	//
	//	Returns:	True	If this hzUrl is equal to the operand hzUrl
	//				False	Otherwise

	if (m_addr == testUrl.m_addr)	return false ;
	if (!m_addr && testUrl.m_addr)	return false ;
	if (m_addr && !testUrl.m_addr)	return true ;

	_url_space*	thisCtl ;		//	This URL space
	_url_space*	suppCtl ;		//	Supplied URL space

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	suppCtl = (_url_space*) _strXlate(testUrl.m_addr) ;

	return Whole() > testUrl.Whole() ? true : false ;
}

bool	hzUrl::operator>=	(const hzUrl& testUrl) const
{
	//	Test for this URL being lexically greater than or equal the supplied test URL
	//
	//	Argument:	U	The test URL
	//
	//	Returns:	True	If this hzUrl is equal to the operand hzUrl
	//				False	Otherwise

	if (m_addr == testUrl.m_addr)	return false ;
	if (!m_addr && testUrl.m_addr)	return false ;
	if (m_addr && !testUrl.m_addr)	return true ;

	_url_space*	thisCtl ;		//	This URL space
	_url_space*	suppCtl ;		//	Supplied URL space

	thisCtl = (_url_space*) _strXlate(m_addr) ;
	suppCtl = (_url_space*) _strXlate(testUrl.m_addr) ;

	return Whole() >= testUrl.Whole() ? true : false ;
}

bool	hzUrl::operator==	(const hzString& S) const
{
	//	Return true if this hzUrl is equal to the supplied string

	if (!S && !m_addr)	return true ;
	if (!S)				return false ;
	if (!m_addr)		return false ;

	return Whole() == S ? true : false ;
}

bool	hzUrl::operator!=	(const hzString& S) const
{
	//	Return true if this hzUrl is not equal to the supplied string

	if (!S && !m_addr)	return false ;
	if (!S)				return true ;
	if (!m_addr)		return true ;

	return Whole() == S ? false : true ;
}

bool	hzUrl::operator==	(const char* cpStr) const
{
	//	Return true if this hzUrl is equal to the supplied character string

	if ((!cpStr || !cpStr[0]) && !m_addr)
		return true ;

	if (!cpStr || !cpStr[0])	return false ;
	if (!m_addr)				return false ;

	return Whole() == cpStr ? true : false ;
}

bool	hzUrl::operator!=	(const char* cpStr) const
{
	//	Return true if this hzUrl is not equal to the supplied character string

	if ((!cpStr || !cpStr[0]) && !m_addr)
		return false ;

	if (!cpStr || !cpStr[0])	return true ;
	if (!m_addr)				return true ;

	return Whole() == cpStr ? false : true ;
}

std::ostream&	operator<<	(std::ostream& os, const hzUrl& obj)
{
	//	Category:	Data Output
	//
	//	Write the whole value of the hzUrl to the output stream

	if (*obj)
		os << *obj ;
	return os ;
}

/*
**	Section 2:	Independent URL test functions
*/

//	Tests if a string is of the form of a URL (Universal Resource Locator). To qualify, a string must at least amount to an Internet domain
//	name. This may optionally be preceeded by either a 'http://' or 'https://' sequence and may be optionally followed by a port indicator
//	and/or a forward slash. If the forward slash is present it can appear on its own or be followed by a resource specifier and may be
//	further followed by a resource qualifier (a query). Or the forward slash can be followed by the resource qualifier directly.
//
//	Dealing with each of these entities in turn:-
//	1)	An Internet domain is a string of characters from the set [a-z], [A-Z], [0-9], [_,-,.] which must contain at least one period and
//		must not start or end with any punctuation character (including the period)
//
//	2)	A port indicator specifies the IP port to connect to. It is a colon followed by the port number expressed as a decimal.
// 
//	3)	A resource specifier is one or more strings separated by a forward slash representing 'directories' off of the domain's 'root'. The
//		allowed chars are [a-z], [A-Z], [0-9], [_,-] and hash (#) if that appears in the last string.
//
//	4)	A resource qualifier is very similar to the resource specifier except that it uses a query (?) to mark the start of a search
//	parameter that will be expressed in the form (some_name=some_value).

bool	IsUrl	(const char* url)
{
	//	Category:	Text processing
	//
	//	Determine if the supplied cstr amounts to a valid URL.
	//
	//	Arguments:	1)	url		The URL as cstr, string
	//
	//	Returns:	True	If the chain iterator is at the start of a valid URL
	//				False	Otherwise

	_hzfunc("IsUrl") ;

	hzUrl	U ;		//	Test URL

	U = url ;
	return !U ? false : true ;
}

bool	IsUrl	(hzUrl& url, uint32_t& nLen, chIter& ci)
{
	//	Category:	Text processing
	//
	//	Determine if the supplied chain-iterator is at the start of a valid URL. Note this does not bypass leading whitespace and it
	//	allows a terminating period if this is followed by either whitespace, a non-URL character or end of file.
	//
	//	Arguments:	1)	url		A hzUrl reference; Populated by chain content if that content is of the form of a URL
	//				2)	nLen	The string length used to make the URL. This is usually needed by the calling function to advance the chain
	//							iterator in the event that a URL is found
	//				3)	ci		The chain iterator into the content being tested.
	//
	//	Returns:	True	If the chain iterator is at the start of a valid URL
	//				False	Otherwise

	_hzfunc("IsUrl") ;

	hzChain		W ;					//	For building tokens
	chIter		xi ;				//	Iterator
	hzString	S ;					//	Token as a string
	uint32_t	nPeriod = 0 ;		//	Number of periods
	uint32_t	nPeriodCont = 0 ;	//	Number of contiguous periods
	uint32_t	nAlpha = 0 ;		//	Number of periods
	uint32_t	nPort = 0 ;			//	Port number

	url.Clear() ;
	nLen = 0 ;
	if (ci.eof())
		return false ;

	//	Strip leading spaces
	for (xi = ci ; !xi.eof() && *xi <= CHAR_SPACE ; xi++) ;

	//	Remove http:// or https://
	if (*xi == 'h')
	{
		if (xi == "http://")
			{ W << "http://" ; nLen = 7 ; xi += 7 ; }

		if (xi == "https://")
			{ W << "https://" ; nLen = 8 ; xi += 8 ; }
	}

 	//	Read up to the end of the domain name. This could be the end of the test string or it could be a forward slash or a colon (for
	//	the port number). This part cannot legally end with a period but it could have a period on the end if the URL was the last word
	//	in a sentence for example.

	for (; !xi.eof() && IsUrlnorm(*xi) ; xi++)
	{
		W.AddByte(*xi) ;

		if (*xi == CHAR_PERIOD)
		{
			nPeriod++ ;
			nPeriodCont++ ;
			if (nPeriodCont == 2)
				return false ;
		}
		else
		{
			nPeriodCont = 0 ;
			nAlpha++ ;
		}
	}

	if (nAlpha < 3 || nPeriod < 2)
		return false ;

	//	Check for port number
	if (*xi == CHAR_COLON)
	{
		xi++ ;
		if (!IsDigit(*xi))
			return false ;
		for (nPort = 0 ; !xi.eof() && IsDigit(*xi) ; xi++)
		{
			nPort *= 10 ; nPort += (*xi - '0') ;
		}

		if (nPort > 0x10000)
			return false ;
	}

	//	The URL may end here with any allowed incident punctuation char or space - or it may continue with a slash
	if (*xi == CHAR_FWSLASH)
	{
		for (xi++ ; !xi.eof() && IsUrlnorm(*xi) ; xi++)
		{
			if (*xi == CHAR_PERIOD)
				nPeriod++ ;
			else
				nAlpha++ ;
		}

		if (*xi == CHAR_QUERY)
		{
			for (xi++ ; !xi.eof() && IsUrlresv(*xi) ; xi++)
			{
				if (*xi == CHAR_PERCENT)
				{
					xi++ ;
					if (!IsHex(*xi))
						return false ;
					xi++ ;
					if (!IsHex(*xi))
						return false ;
					nLen += 2 ;
				}
			}
		}
	}

	if (*xi <= CHAR_SPACE)
	{
		xi-- ;
		if (*xi == CHAR_PERIOD)
			xi-- ;
	}

	//ci.GetString(S, xi) ;
	S = W ;
	url = *S ;

	if (!url.Whole())
		return false ;

	nLen = S.Length() ;
	return false ;
}

hzString	hzUrl::Filename	(void) const
{
	//	Convert a URL to a string suitable for a filename as used in webscraping. The following conversions occur:-
	//
	//	1)	The sequence http:// is converted to h: (but only if it occurs at the start)
	//	2)	The sequence https:// is converted to s: (but only if it occurs at the start)
	//	3)	The slash is converted to an @
	//	4)	The @ (which should not exist
	//
	//	Converts non-URL and non-filename chars into %xx form.
	//
	//	Note that no assumptions can be made about the input except that it may contain chars unsuitable for filenames (eg the forward
	//	slash). The encoding must therefore be reversible.
	//
	//	This function assumes the chars a-z, A-Z, 0-9, the period and the underscore are the only valid filename chars. Any other char
	//	will be converted to a set of chars consisting of a percent sign and two hexidecimal numbers. This means that when it comes to
	//	decoding, such a set will be converted to a single char. This would be fine if we could assume that no input would ever have
	//	such a sequence but alas we cannot assume this.
	//
	//	It is nessesary therefore to convert percent chars in the input to a %hh set even if they are blatently part of such a set
	//	already!
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString being the URL in same filename form

	_hzfunc("hzUrl::Filename") ;

	hzChain			Z ;			//	Used to construct the (longer) encoded string value
	_url_space*		thisCtl ;	//	This URL space
	uchar*			i ;			//	For iteration
	hzString		S ;			//	Return string
	uint32_t		val ;		//	For casting
	char			buf	[4] ;	//	Fox hex-conversion

	if (!m_addr)
		return S ;
	thisCtl = (_url_space*) _strXlate(m_addr) ;

	i = (uchar*) thisCtl->m_data ;
	if (!memcmp(i, "http", 4))
	{
		if (!memcmp(i + 4, "://", 3))
			{ i += 7 ; Z << "h:" ; }
		if (!memcmp(i + 4, "s://", 4))
			{ i += 8 ; Z << "s:" ; }
	}

	//	Count chars that are to be converted as these will occupy 3 chars in the new string
	for (; *i ; i++)
	{
		if (*i >= 'A' && *i <= 'Z')
			{ Z.AddByte(conv2lower(*i)) ; continue ; }

		if (*i >= 'a' && *i <= 'z')	{ Z.AddByte(*i) ; continue ; }
		if (*i >= '0' && *i <= '9')	{ Z.AddByte(*i) ; continue ; }

		if (*i == CHAR_FWSLASH)		{ Z.AddByte(CHAR_AT) ; continue ; }
		if (*i == CHAR_AMPSAND)		{ Z.AddByte(CHAR_COLON) ; continue ; }

		if (*i == CHAR_USCORE || *i == CHAR_PERIOD || *i == CHAR_PERCENT || *i == CHAR_EQUAL || *i == CHAR_QUERY || *i == CHAR_PLUS ||
			*i == CHAR_MINUS)
		{
			Z.AddByte(*i) ;
			continue ;
		}

		Z.AddByte(CHAR_PERCENT) ;
		val = (uchar) *i ;
		sprintf(buf, "%02x", val) ;
		Z.AddByte(buf[0]) ;
		Z.AddByte(buf[1]) ;
	}

	S = Z ;
	return S ;
}
