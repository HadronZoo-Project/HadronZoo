//
//	File:	hzIpaddr.cpp
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
//	Implimentation of the hzIpaddr (IP address) class.
//

#include <fstream>
#include <iostream>

using namespace std ;

#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <errno.h>

#include "hzChars.h"
#include "hzCtmpls.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzIpaddr.h"

global	const hzIpaddr	_hzGlobal_nullIP ;								//	Null IP address

bool	IsIPAddr	(uint32_t& nIpValue, const char* cpIpa)
{
	//	Category:	Text Processing
	//
	//	Test if supplied string is of the form of a valid IP address (four numbers between 0 and 255 separated by a period)
	//
	//	Arguments:	1) nIpValue	The IP value, set if supplied string is of the form
	//				2) cpIpa	The string to be tested
	//
	//	Returns:	True	If the supplied string amounts to a valid IP address
	//				False	If it doesn't

	const char*	i ;				//	Text IP address iterator
	uint32_t	nVal = 0 ;		//	32-bit IP address value
	uint32_t	nTmp ;			//	IP segment value
	int32_t		nNos = 0 ;		//	Count of IP numeric segments

	nIpValue = IPADDR_BAD ;

	if (!cpIpa)		return false ;
	if (!cpIpa[0])	return false ;

	for (i = cpIpa ; *i ; i++)
	{
		if (*i < '0' || *i > '9')
			return false ;

		for (nTmp = 0 ; *i >= '0' && *i <= '9' ; i++)
			{ nTmp *= 10 ; nTmp += (*i - '0') ; }

		if (nTmp > 255)
			return false ;

		nVal *= 256 ;
		nVal += nTmp ;
		nNos++ ;

		if (*i == 0 || *i == CHAR_COMMA)
			break ;

		if (*i != CHAR_PERIOD)
			return false ;
	}

	if (nNos != 4)
		return false ;

	nIpValue = nVal ;
	return true ;
}

bool	IsIPAddr	(const char* cpIpa)
{
	//	Category:	Text Processing
	//
	//	Test if supplied string is of the form of a valid IP address (four numbers between 0 and 255 separated by a period)
	//
	//	Arguments:	1)	The string to be tested
	//
	//	Returns:	True	If the supplied string amounts to a valid IP address
	//				False	If it doesn't

	uint32_t	nVal ;			//	32-bit IP address value

	return IsIPAddr(nVal, cpIpa) ;
}

hzEcode		hzIpaddr::SetValue	(const char* cpIpa)
{
	//	Sets the IP address to the value provided as a character string. The string must be of the form x.x.x.x where x can be any number between 0 and 255
	//
	//	Arguments:	1)	cpIpa	IP address as a cstr
	//
	//	Returns:	E_NODATA	If the input is not supplied
	//				E_FORMAT	If the input cstr is no an IP address
	//				E_OK		If this IP address instance is set

	if (!cpIpa || !cpIpa[0])
		return E_NODATA ;

	if (IsIPAddr(m_Ipa, cpIpa))
		return E_OK ;

	return E_FORMAT ;
}

hzIpaddr&	hzIpaddr::operator=	(const char* cpIpa)
{
	//	Sets the IP address to the operand provided as a character string. The string must be of the form x.x.x.x where x can be any number between 0 and 255
	//
	//	Arguments:	1)	cpIpa	IP address as a cstr
	//
	//	Returns:	Reference to this instance

	if (!cpIpa || !cpIpa[0])
		m_Ipa = 0 ;
	else
		IsIPAddr(m_Ipa, cpIpa) ;

	return *this ;
}

/*
**	Get value methods
*/

char*	hzIpaddr::AsBytes	(hzRecep08& r) const
{
	//	Populates the supplied buffer with the IP address as a series of 4 bytes. This is for the benefit of network functions.
	//
	//	Argument:	buf		The buffer to be populated
	//
	//	Returns:	Pointer to the populated buffer

	r.m_buf[0] = (m_Ipa & 0xff000000) >> 24 ;
	r.m_buf[1] = (m_Ipa & 0xff0000) >> 16 ;
	r.m_buf[2] = (m_Ipa & 0xff00) >> 8 ;
	r.m_buf[3] = m_Ipa & 0xff ;

	return r.m_buf ;
}

char*	hzIpaddr::Full	(hzRecep16& r) const
{
	//	Populates the supplied buffer with the fully expanded text value of the IP address. This is for the benefit of diagnostics.
	//
	//	Argument:	r	The 16-byte recepticle to be populated
	//
	//	Returns:	Pointer to the populated buffer

	sprintf(r.m_buf, "%03u.%03u.%03u.%03u", (m_Ipa & 0xff000000) >> 24, (m_Ipa & 0xff0000) >> 16, (m_Ipa & 0xff00) >> 8,  m_Ipa & 0xff) ;

	return r.m_buf ;
}

char*	hzIpaddr::Txt	(hzRecep16& r) const
{
	//	Populates the supplied buffer with the text value of the IP address. This is for the benefit of diagnostics.
	//
	//	Argument:	r	The 16-byte recepticle to be populated
	//
	//	Returns:	Pointer to the populated buffer

	sprintf(r.m_buf, "%u.%u.%u.%u", (m_Ipa & 0xff000000) >> 24, (m_Ipa & 0xff0000) >> 16, (m_Ipa & 0xff00) >> 8,  m_Ipa & 0xff) ;

	return r.m_buf ;
}

char*	hzIpaddr::Txt	(hzRecep32& r) const
{
	//	Populates the supplied buffer with the text value of the IP address. This is for the benefit of diagnostics.
	//
	//	Argument:	r	The 32-byte recepticle to be populated
	//
	//	Returns:	Pointer to the populated buffer

	sprintf(r.m_buf, "%u.%u.%u.%u", (m_Ipa & 0xff000000) >> 24, (m_Ipa & 0xff0000) >> 16, (m_Ipa & 0xff00) >> 8,  m_Ipa & 0xff) ;

	return r.m_buf ;
}

hzString	hzIpaddr::Str	(void) const
{
	//	Populates a string with the textual form of the IP address and returns this string by value.
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString by value

	hzString	S ;			//	IP address as string
	char		buf	[16] ;	//	IP address test buffer

	sprintf(buf, "%u.%u.%u.%u", (m_Ipa & 0xff000000) >> 24, (m_Ipa & 0xff0000) >> 16, (m_Ipa & 0xff00) >> 8, (m_Ipa & 0xff)) ;
	S = buf ;
	return S ;
}

//	Stream operator
std::ostream&	operator<<	(std::ostream& os, const hzIpaddr& op)
{
	//	Category:	Data Output
	//
	//	Streams out the text form of the IP address to the supplied output stream
	//
	//	Arguments:	1)	os	Reference to the output stream
	//				2)	op	The IP address instance
	//
	//	Returns:	Reference to the supplied output stream

	uint32_t	val ;			//	32-bit IP address value
	char		buf	[20] ;		//	Text formulation buffer

	val = (uint32_t) op ;
	sprintf(buf, "%d.%d.%d.%d", (val & 0xff000000) >> 24, (val & 0xff0000) >> 16, (val & 0xff00) >> 8, (val & 0xff)) ;

	os << buf ;
	return os ;
}

/*
**	IP Location and Country Codes
*/

//	The s_CC series just deals with country codes and country code lookup.
static	const char*		s_CC_buffer ;		//	Array of 2-char null terminated country codes plus country name
static	const uint16_t*	s_CC_offsets ;		//	Array of country code offsets - one per IP range
static	uint32_t		s_CC_max ;			//	Total country codes
static	uint32_t		s_CC_start ;		//	Country code binary chop start point
static	uint32_t		s_CC_div ;			//	Country code initial binary divider

//	The s_IpBasic series deals with Basic level IP location
static	const uchar*	s_IpBasic_codes ;	//	Array of country codes. Each element is an 8-bit unsigned number indicating the country. There is an element for each IP range.
static	const uint32_t*	s_IpBasic_zones ;	//	Array of IP ranges. These comprise only the starting IP. The range limit is given by the entry above or 255.255.255.255.
static	uint32_t		s_IpBasic_max ;		//	Total population of IP ranges in the base level data
static	uint32_t		s_IpBasic_start ;	//	IP range binary chop start point
static	uint32_t		s_IpBasic_div ;		//	IP range initial binary divider

//	The s_IpCity series deals with City level IP location
static	const char*		s_IpCity_text ;		//	Array of city-level locations (as concatenated Cstr)
static	const uint32_t*	s_IpCity_zones ;	//	Array of IP ranges. These comprise only the starting IP. The range limit is given by the entry above or 255.255.255.255.
static	const uint32_t*	s_IpCity_osets ;	//	Array of offsets into the city-level location (as they vary in length)
static	uint32_t		s_IpCity_max ;		//	Total population of IP ranges in the city level data
static	uint32_t		s_IpCity_start ;	//	IP range binary chop start point
static	uint32_t		s_IpCity_div ;		//	IP range initial binary divider

hzEcode	InitCountryCodes	(void)
{
	//	Category:	Internet
	//
	//	Read in the HadronZoo data file global.country.dat which provides a mapping of two letter country codes to country names. This file is expected to be in
	//	the HadronZoo data directory $HADRONZOO/data which is supplied as a part of the HadronZoo Suite download.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NODATA	If the Country code source could not be established
	//				E_NOTFOUND	If the Country code file does not exist
	//				E_OPENFAIL	If the Country code file could not be opened
	//				E_READFAIL	If the Country code file could not be read
	//				E_FORMAT	If the Country code file does not have the expected format
	//				E_OK		If the Country code tables were initialized

	_hzfunc(__func__) ;

	ifstream	is ;		//	Input stream
	FSTAT		fs ;		//	File status
	char*		pCC_buf ;	//	Shared mem segment
	uint16_t*	pShort ;	//	Offsets in mem segment
	uint32_t	fd ;		//	Shered mem 'file descriptor'
	uint32_t	n ;			//	Buffer iterator
	hzEcode		rc = E_OK ;	//	Return code

    fd = shm_open("deltaCountryCodes", O_RDONLY, 0) ;
	fstat(fd, &fs);
    threadLog("%s: Set fd to %d\n", *_fn, fd) ;

	//pCC_buf = (char*) mmap(0, fs.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	pCC_buf = (char*) mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (!pCC_buf)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Shared memory address at %p errno is %d\n", pCC_buf, errno) ;
	threadLog("%s: Country codes mem at %p, size is %d\n", *_fn, pCC_buf, fs.st_size) ;

	pShort = (uint16_t*) pCC_buf ;
	s_CC_max = *pShort++ ;
	threadLog("%s: Total codes %d\n", *_fn, s_CC_max) ;

	s_CC_offsets = pShort ;
	threadLog("%s: Offsets at %p\n", *_fn, s_CC_offsets) ;

	pShort += s_CC_max ;
	s_CC_buffer = (char*) pShort ;
	threadLog("%s: Buffer at %p\n", *_fn, s_CC_buffer) ;

	for (n = 2 ; n <= s_CC_max ; n *= 2) ;
	s_CC_div = n / 2 ;
	s_CC_start = n - 1 ;

	threadLog("%s: Total codes %d (div %d start %d)\n\n", *_fn, s_CC_max, s_CC_div, s_CC_start) ;

	for (n = 0 ; n < s_CC_max ; n++)
		threadLog("%s -> %s (%d)\n", s_CC_buffer + s_CC_offsets[n], s_CC_buffer + s_CC_offsets[n] + 3, s_CC_offsets[n]) ;

	return rc ;
}

uint32_t	GetCountryByCode	(const char* ccode)
{
	//	Category:	Internet
	//
	//	Locate country by country code
	//
	//	Argument:	ccode	Two letter country code
	//
	//	Returns:	Number being RID of country or 0 if not found.

	_hzfunc(__func__) ;

	const char*	i ;			//	In-table country code
	uint32_t	nDiv ;		//	Binary chop divider
	uint32_t	nPos ;		//	Starting position
	int32_t		res ;		//	Comparison result
	bool		bFound ;	//	Position if redult

	//	Ensure country code is two upper case letters
	if (!ccode)		return 0 ;
	if (!ccode[0])	return 0 ;
	if (ccode[2])	return 0 ;

	if (ccode[0] < 'A' || ccode[0] > 'Z')	return 0 ;
	if (ccode[1] < 'A' || ccode[1] > 'Z')	return 0 ;

	//	Perform binary chop
	if (!s_CC_start)
		return 0 ;

	nPos = s_CC_start ;
	bFound = false ;

	for (nDiv = s_CC_div ;; nDiv /= 2)
	{
		if (nPos > s_CC_max)
		{
			if (!nDiv)
				break ;
			nPos -= nDiv ;
			continue ;
		}

		//	Compare
		i = s_CC_buffer + s_CC_offsets[nPos] ;
		if (i[0] > ccode[0])
			res = -1 ;
		else if (i[0] < ccode[0])
			res = 1 ;
		else
			res = i[1] > ccode[1] ? -1 : i[1] < ccode[1] ? 1 : 0 ;

		//	Chop
		if (res > 0)
		{
			if (!nDiv)
				break ;
			nPos += nDiv ;
			continue ;
		}

		if (res < 0)
		{
			if (!nDiv)
				break ;
			nPos -= nDiv ;
			continue ;
		}

		bFound = true ;
		break ;
	}

	return bFound ? nPos : 0 ;
}

const char*	GetCountryCode	(uint32_t countryRID)
{
	//	Category:	Internet
	//
	//	Return the 2-byte country code

	_hzfunc(__func__) ;

	if (countryRID > s_CC_max)
		return s_CC_buffer ;

	return s_CC_buffer + s_CC_offsets[countryRID] ;
}

const char*	GetCountryName	(uint32_t countryRID)
{
	//	Category:	Internet
	//
	//	Return the country Name

	_hzfunc(__func__) ;

	if (countryRID > s_CC_max)
		return s_CC_buffer ;

	return s_CC_buffer + s_CC_offsets[countryRID] + 3 ;
}

hzEcode	InitIpBasic	(void)
{
	//	Category:	Internet
	//
	//	The HadronZoo data file global.ipaddr.dat should be read into shard memory managed by the HadronZoo Delta Server. This function meerly connects to this
	//	shared memory segment.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NODATA	If there are no IP ranges
	//				E_OPENFAIL	If the IP range file cannot be opened

	_hzfunc(__func__) ;

	ifstream	is ;			//	Input stream
	FSTAT		fs ;			//	Shared memory 'file' stat
	uint32_t*	pRanges ;		//	Pointer to IP Ranges block
	uint32_t*	pLocations ;	//	Pointer to IP Locations block
	uint32_t	n ;				//	Loop control
	int32_t		m_fd ;			//	Sheared memory file descriptor

	//	Check we have country codes. This will ensure _hzGlobal_HadronZooBase has a value and from this, _hzGlobal_IpRanges can be derived.

    m_fd = shm_open("deltaIpBaseZone", O_RDONLY, 0) ;
	if (m_fd < 0)
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open deltaIpBaseZone") ;

	fstat(m_fd, &fs);
	pRanges = (uint32_t*) mmap(0, fs.st_size, PROT_READ, MAP_SHARED, m_fd, 0);
	if (!pRanges)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Shared memory address at %p errno is %d\n", pRanges, errno) ;
	threadLog("%s: ranges size %u mem at %p\n", *_fn, fs.st_size, pRanges) ;

    m_fd = shm_open("deltaIpBaseCode", O_RDONLY, 0) ;
	if (m_fd < 0)
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open deltaIpBaseCode") ;

	fstat(m_fd, &fs);
	s_IpBasic_max = fs.st_size ;

	pLocations = (uint32_t*) mmap(0, fs.st_size, PROT_READ, MAP_SHARED, m_fd, 0);
	if (!pLocations)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Shared memory address at %p errno is %d\n", pLocations, errno) ;
	threadLog("%s: locations size %u mem at %p\n", *_fn, fs.st_size, pLocations) ;

	s_IpBasic_zones = pRanges ;
	s_IpBasic_codes = (uchar*) pLocations ;

	//	Calculate start and divider
	for (n = 2 ; n < s_IpBasic_max ; n *= 2) ;
	s_IpBasic_div = n / 2 ;
	s_IpBasic_start = n - 1 ;

	threadLog("%s: Total ranges %d (div %d start %d)\n\n", *_fn, s_IpBasic_max, s_IpBasic_div, s_IpBasic_start) ;
	return E_OK ;
}

hzEcode	InitIpCity		(void)
{
	//	Category:	Internet
	//
	//	The HadronZoo data file global.ipaddr.dat should be read into shard memory managed by the HadronZoo Delta Server. This function meerly connects to this
	//	shared memory segment.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NODATA	If there are no IP ranges
	//				E_OPENFAIL	If the IP range file cannot be opened

	_hzfunc(__func__) ;

	ifstream	is ;			//	Input stream
	FSTAT		fs ;			//	Shared memory 'file' stat
	uint32_t*	pIpr ;			//	Pointer to IP Ranges block
	uint32_t	m_fd ;			//	Loop control
	uint32_t	n ;				//	Loop control

	//	Check we have country codes. This will ensure _hzGlobal_HadronZooBase has a value and from this, _hzGlobal_IpRanges can be derived.
    m_fd = shm_open("deltaIpCityText", O_RDONLY, 0) ;
	fstat(m_fd, &fs);
    threadLog("%s: Set fd to %d\n", *_fn, m_fd) ;
	s_IpCity_text = (char*) mmap(0, fs.st_size, PROT_READ, MAP_SHARED, m_fd, 0);
	if (!s_IpCity_text)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Shared memory address at %p errno is %d\n", s_IpCity_text, errno) ;
	threadLog("%s: text mem at %p\n", *_fn, s_IpCity_text) ;

    m_fd = shm_open("deltaIpCityZone", O_RDONLY, 0) ;
	fstat(m_fd, &fs);
    threadLog("%s: Set fd to %d\n", *_fn, m_fd) ;
	pIpr = (uint32_t*) mmap(0, fs.st_size, PROT_READ, MAP_SHARED, m_fd, 0);
	if (!pIpr)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Shared memory address at %p errno is %d\n", pIpr, errno) ;
	s_IpCity_max = pIpr[0] ;
	s_IpCity_zones = pIpr + 1 ;
	threadLog("%s: zone mem at %p\n", *_fn, pIpr) ;

    m_fd = shm_open("deltaIpCityLocn", O_RDONLY, 0) ;
	fstat(m_fd, &fs);
    threadLog("%s: Set fd to %d\n", *_fn, m_fd) ;
	s_IpCity_osets = (uint32_t*) mmap(0, fs.st_size, PROT_READ, MAP_SHARED, m_fd, 0);
	if (!s_IpCity_osets)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Shared memory address at %p errno is %d\n", s_IpCity_osets, errno) ;
	threadLog("%s: locn mem at %p\n", *_fn, s_IpCity_osets) ;

	//	Calculate start and divider
	for (n = 2 ; n < s_IpCity_max ; n *= 2) ;
	s_IpCity_div = n / 2 ;
	s_IpCity_start = n - 1 ;
	threadLog("%s: Have %d zones, div %d start %d\n", *_fn, s_IpCity_max, s_IpCity_div, s_IpCity_start) ;

	return E_OK ;
}

/*
**	IP Lookup Functions
*/

const char*	GetIpLocation	(const hzIpaddr& ipa)
{
	//	Category:	Internet
	//
	//	This returns a location description for the supplied IP address if the IP location table has been loaded. Otherwise this function calls LocateCountry(),
	//	which will return the country code for the IP address - if the country codes table has been set up.
	//
	//	This function never returns NULL. A pointer to a static string is returned in the event of lookup failure. If the supplied IP address is invalid but the
	//	IP location table has been loaded, this will read "-- Invalid IP location". If the IP location table is not loaded and LocateCountry() is used instead,
	//	this will read "-- Invalid Country Code".
	//
	//	The lookup is by binary chop array of known IP blocks to see if the IP address falls within any of them.
	//	
	//	Arguments:	1)	The client IP address
	//
	//	Returns:	Pointer to either the country code or the location code

	_hzfunc(__func__) ;

	static	hzString	ip_not_found = "-- IP Location Not-found" ;
	static	hzString	ip_error = "-- IP Location Error" ;

	const uint32_t*	pZones ;	//	IP ranges
	uint32_t		nDiv ;		//	Binary chop divider
	uint32_t		nPos ;		//	Position found within IP range table
	uint32_t		nMax ;		//	Total IP zones in operation
	uint32_t		ipval ;		//	IP as 32 bit uint
	bool			bFound ;	//	Position if redult

	if (s_IpCity_max)
	{
		//	There is a city level IP location table

		pZones = s_IpCity_zones ;
		nDiv = s_IpCity_div ;
		nPos = s_IpCity_start ;
		nMax = s_IpCity_max ;
	}
	else
	{
		if (!s_IpBasic_max)
			return *ip_not_found ;

		pZones = s_IpBasic_zones ;
		nDiv = s_IpBasic_div ;
		nPos = s_IpBasic_start ;
		nMax = s_IpBasic_max ;
	}

	bFound = false ;
	ipval = (uint32_t) ipa ;

	for (;;)
	{
		if (nPos >= nMax)
		{
			if (!nDiv)
				break ;
			nPos -= nDiv ;
			nDiv /= 2 ;
			continue ;
		}

		if (ipval >= pZones[nPos+1])
		{
			if (!nDiv)
				break ;
			nPos += nDiv ;
			nDiv /= 2 ;
			continue ;
		}

		if (ipval < pZones[nPos])
		{
			if (!nDiv)
				break ;
			nPos -= nDiv ;
			nDiv /= 2 ;
			continue ;
		}

		bFound = true ;
		break ;
	}

	if (!bFound)
	{
		threadLog("%s Not found\n", *_fn) ;
		return *ip_not_found ;
	}

	if (s_IpCity_max)
	{
		//threadLog("%s: %d -> %d -> %s", *_fn, nPos, s_IpCity_osets[nPos], s_IpCity_text + s_IpCity_osets[nPos]) ;
		//threadLog("%s: %s", *_fn, s_IpCity_text + s_IpCity_osets[nPos]) ;
		return s_IpCity_text + s_IpCity_osets[nPos] ;
	}

	//threadLog("%s Revert to CC pos %d -> ", *_fn, nPos) ;
	//threadLog("rid %d\n", s_IpBasic_codes[nPos]) ;
	//return GetCountryCode((uint32_t) s_IpBasic_codes[nPos]) ;
	return GetCountryCode(s_IpBasic_codes[nPos]) ;
}
