//
//	File:	hzIpaddr.h
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
//	Enumerations for HadronZoo datatypes and various data presentation formats.
//

#ifndef hzIpaddr_h
#define hzIpaddr_h

//	Other Includes
#include "hzBasedefs.h"
#include "hzString.h"

/*
**	Common networking typedefs
*/

typedef	struct sockaddr		SOCKADDR ;
typedef	struct sockaddr_in	SOCKADDRIN ;
typedef	struct hostent		HOSTENT ;

/*
**	IP addresses
*/

#define IPADDR_NULL		(uint32_t) 0x00000000
#define IPADDR_BAD		(uint32_t) 0xffffffff
#define IPADDR_LOCAL	(uint32_t) 0x7f000001

class	hzIpaddr
{
	//	Category:	Data
	//
	//	hzIpaddr is a HadronZoo data type representing an IP (v4) address

private:
	uint32_t	m_Ipa ;		//	IP address held as a unsigned 32 bit integer

public:
	hzIpaddr	(void)	{ m_Ipa = IPADDR_NULL ; }

	~hzIpaddr	(void)	{}

	//	Set value methods
	hzEcode	SetValue	(const char* ipstr) ;	//	Set IP address by cstr

	void	Clear		(void)					{ m_Ipa = 0 ; }
	void	SetValue	(uint32_t nIPAddr)		{ m_Ipa = nIPAddr ; }		//	Set IP address by unsigned int form
	void	SetValue	(const hzIpaddr& op)	{ m_Ipa = op.m_Ipa ; }		//	Set this IP address to another

	//	Set IP address by four unsigned numbers
	void	SetValue	(uchar a, uchar b, uchar c, uchar d)
		{ m_Ipa = a ; m_Ipa <<= 8 ; m_Ipa |= b ; m_Ipa <<= 8 ; m_Ipa |= c ; m_Ipa <<= 8 ; m_Ipa |= d ; } 

	//	Get value methods
	char*		AsBytes	(hzRecep08& r) const ;
	char*		Full	(hzRecep16& r) const ;
	char*		Txt		(hzRecep16& r) const ;
	char*		Txt		(hzRecep32& r) const ;
	hzString	Str		(void) const ;

	//	Assignment Operators
	hzIpaddr&	operator=	(const char* cpStr) ;
	hzIpaddr&	operator=	(const hzIpaddr& op)	{ m_Ipa = op.m_Ipa ; return *this ; }
	hzIpaddr&	operator=	(uint32_t nIPAddr)		{ m_Ipa = nIPAddr ; return *this ; }

	//	Compare Operators
	bool	operator==	(const hzIpaddr& op) const	{ return m_Ipa == op.m_Ipa ? true : false ; }
	bool	operator!=	(const hzIpaddr& op) const	{ return m_Ipa != op.m_Ipa ? true : false ; }
	bool	operator<	(const hzIpaddr& op) const	{ return m_Ipa <  op.m_Ipa ? true : false ; }
	bool	operator<=	(const hzIpaddr& op) const	{ return m_Ipa <= op.m_Ipa ? true : false ; }
	bool	operator>	(const hzIpaddr& op) const	{ return m_Ipa >  op.m_Ipa ? true : false ; }
	bool	operator>=	(const hzIpaddr& op) const	{ return m_Ipa >= op.m_Ipa ? true : false ; }

	bool	operator==	(uint32_t x) const	{ return m_Ipa == x ? true : false ; }
	bool	operator!=	(uint32_t x) const	{ return m_Ipa != x ? true : false ; }
	bool	operator!	(void) const	{ return m_Ipa == IPADDR_NULL ? true : false ; }

	//	Casting Operators
	operator uint32_t		(void) const	{ return m_Ipa ; }

	//	Stream operator
	friend	std::ostream&	operator<<	(std::ostream& os, const hzIpaddr& op) ;
};

class	hzIpRange
{
	//	Category:	Internet
	//
	//	For IP location

public:
	hzString	m_State ;		//	State/Country
	hzIpaddr	m_Start ;		//	Start of IP block
	hzIpaddr	m_End ;			//	End of IP block

	hzIpRange&	operator=	(const hzIpRange& op)
	{
		m_State = op.m_State ;
		m_Start = op.m_Start ;
		m_End = op.m_End ;
	}

	bool	operator==	(const hzIpRange& op)	{ return m_Start == op.m_Start && m_End == op.m_End ? true : false ; }
	bool	operator<	(const hzIpRange& op)	{ return m_Start < op.m_Start ? true : m_Start > op.m_Start ? false : m_End < op.m_End ? true : false ; }
	bool	operator>	(const hzIpRange& op)	{ return m_Start > op.m_Start ? true : m_Start < op.m_Start ? false : m_End > op.m_End ? true : false ; }
} ;

/*
**	IP address status
*/

enum	hzIpStatus
{
	HZ_IPSTATUS_NULL		= 0,	//	IP address has no associated notes
	HZ_IPSTATUS_WHITE_SMTP	= 0x0001,	//	IP address is whitelisted by a successful SMTP authentication
	HZ_IPSTATUS_WHITE_POP3	= 0x0002,	//	IP address is whitelisted by a successful POP3 authentication
	HZ_IPSTATUS_WHITE_HTTP	= 0x0004,	//	IP address is whitelisted by a successful HTTP authentication
	HZ_IPSTATUS_WHITE_DATA	= 0x0008,	//	IP address is whitelisted by a successful data authentication
	HZ_IPSTATUS_WHITE		= 0x000f,	//	Mask to determine if IP address is whitelisted for any reason
	HZ_IPSTATUS_BLACK_SMTP	= 0x0010,	//	IP address is blacklisted by a failed SMTP authentication
	HZ_IPSTATUS_BLACK_POP3	= 0x0020,	//	IP address is blacklisted by a failed POP3 authentication
	HZ_IPSTATUS_BLACK_HTTP	= 0x0040,	//	IP address is blacklisted by a failed HTTP authentication
	HZ_IPSTATUS_BLACK_DATA	= 0x0080,	//	IP address is blacklisted by a failed data authentication
	HZ_IPSTATUS_BLACK_PROT	= 0x0100,	//	IP address is blacklisted because of a malformed request
	HZ_IPSTATUS_BLACK		= 0x01f0,	//	Mask to determine if IP address is blacklisted for any reason
} ;

class	hzIpinfo
{
	//	Records a blocked IP address.

public:
	uint32_t	m_tBlack ;		//	Compact epoch time for status to remain in force
	uint32_t	m_tWhite ;		//	Compact epoch time for status to remain in force
	uint32_t	m_nTotal ;		//	Count of attempts since first block
	uint16_t	m_nSince ;		//	Count of attempts since last block
	uint16_t	m_bInfo ;		//	Status of address

	hzIpinfo	(void)	{ m_nTotal = m_tBlack = m_tWhite = 0 ; m_nSince = m_bInfo = 0 ; }
} ;


//	Prototypes for IP-range/Country mapping
hzEcode		InitCountryCodes	(void) ;
uint32_t	GetCountryByCode	(const char* ccode) ;
const char*	GetCountryCode		(uint32_t countryNo) ;
const char*	GetCountryName		(uint32_t countryNo) ;
hzEcode		InitIpBasic			(void) ;
hzEcode		InitIpCity			(void) ;
const char*	GetIpLocation		(const hzIpaddr& ipa) ;

//	Prototypes for IP status
hzEcode		InitIpInfo		(const hzString& dataDir) ;
void		SetStatusIP		(hzIpaddr ipa, hzIpStatus status, uint32_t nDelay) ;
hzIpStatus	GetStatusIP		(hzIpaddr ipa) ;

//	Globals
extern	const hzIpaddr	_hzGlobal_nullIP ;		//	Null IP address

#endif	//	hzIpaddr_h
