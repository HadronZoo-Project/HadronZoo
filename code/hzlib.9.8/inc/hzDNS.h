//
//	File:	hzDNS.h
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
//	The primary purpose for the hzDNS class is too facilitate DNS lookup
//

#ifndef hzDNS_h
#define hzDNS_h

#include "hzChain.h"
#include "hzTmplList.h"
#include "hzIpaddr.h"

enum	DnsType
{
	//	Category:	Internet
	//
	//	Main types (from RFC 1035)

	DNSTYPE_A			= 1,		//	a host address
	DNSTYPE_NS			= 2,		//	an authoritative name server
	DNSTYPE_MD			= 3,		//	a mail destination (Obsolete - use MX)
	DNSTYPE_MF			= 4,		//	a mail forwarder (Obsolete - use MX)
	DNSTYPE_CNAME		= 5,		//	the canonical name for an alias
	DNSTYPE_SOA			= 6,		//	marks the start of a zone of authority
	DNSTYPE_MB			= 7,		//	a mailbox domain name (EXPERIMENTAL)
	DNSTYPE_MG			= 8,		//	a mail group member (EXPERIMENTAL)
	DNSTYPE_MR			= 9,		//	a mail rename domain name (EXPERIMENTAL)
	DNSTYPE_NULL		= 10,		//	a null RR (EXPERIMENTAL)
	DNSTYPE_WKS			= 11,		//	a well known service description
	DNSTYPE_PTR			= 12,		//	a domain name pointer
	DNSTYPE_HINFO		= 13,		//	host information
	DNSTYPE_MINFO		= 14,		//	mailbox or mail list information
	DNSTYPE_MX			= 15,		//	mail exchange
	DNSTYPE_TXT			= 16,		//	text strings

	//	From RFC 1183
	DNSTYPE_RP			= 17,		//	for Responsible Person
	DNSTYPE_AFSDB		= 18,		//	for AFS Data Base location
	DNSTYPE_X25			= 19,		//	for X.25 PSDN address
	DNSTYPE_ISDN		= 20,		//	for ISDN address
	DNSTYPE_RT			= 21,		//	for Route Through

	//	Others
	DNSTYPE_NSAP		= 22,		//	for NSAP address, NSAP style A record	(RFC1706)
	DNSTYPE_NSAP_PTR	= 23,		//	Not sure!
	DNSTYPE_SIG			= 24,		//	for security signature					(RFC2535, RFC3755, RFC4034)
	DNSTYPE_KEY			= 25,		//	for security key						(RFC2535, RFC3755, RFC4034)
	DNSTYPE_PX			= 26,		//	X.400 mail mapping information			(RFC2163)
	DNSTYPE_GPOS		= 27,		//	Geographical Position					(RFC1712)
	DNSTYPE_AAAA		= 28,		//	IP6 Address								(RFC3596)
	DNSTYPE_LOC			= 29,		//	Location Information					(RFC1876)
	DNSTYPE_NXT			= 30,		//	Next Domain - OBSOLETE					(RFC2535, RFC3755)
	DNSTYPE_EID			= 31,		//	Endpoint Identifier						(Patton)
	DNSTYPE_NIMLOC		= 32,		//	Nimrod Locator							(Patton)
	DNSTYPE_SRV			= 33,		//	Server Selection						(RFC2782)
	DNSTYPE_ATMA		= 34,		//	ATM Address								(Dobrowski)
	DNSTYPE_NAPTR		= 35,		//	Naming Authority Pointer				(RFC2168, RFC2915)
	DNSTYPE_KX			= 36,		//	Key Exchanger							(RFC2230)
	DNSTYPE_CERT		= 37,		//	CERT									(RFC2538)
	DNSTYPE_A6			= 38,		//	IPV6									(RFC2874, RFC3226)
	DNSTYPE_DNAME		= 39,		//	DNAME									(RFC2672)
	DNSTYPE_SINK		= 40,		//	SINK									(Eastlake)
	DNSTYPE_OPT			= 41,		//	OPT										(RFC2671)
	DNSTYPE_APL			= 42,		//	APL										(RFC3123)
	DNSTYPE_DS			= 43,		//	Delegation Signer						(RFC3658)
	DNSTYPE_SSHFP		= 44,		//	SSH Key Fingerprint						(RFC4255)
	DNSTYPE_IPSECKEY	= 45,		//	IPSECKEY								(RFC4025)
	DNSTYPE_RRSIG		= 46,		//	RRSIG									(RFC3755)
	DNSTYPE_NSEC		= 47,		//	NSEC									(RFC3755)
	DNSTYPE_DNSKEY		= 48,		//	DNSKEY									(RFC3755)
	DNSTYPE_DHCID		= 49,		//	DHCID									(RFC4701)
	DNSTYPE_HIP			= 55,		//	Host Identity Protocol					(RFC-ietf-hip-dns-09.txt)
	DNSTYPE_SPF			= 99,		//	Oh no, not these!						(RFC4408)
	DNSTYPE_UINFO		= 100,		//	IANA-Reserved
	DNSTYPE_UID			= 101,		//	IANA-Reserved
	DNSTYPE_GID			= 102,		//	IANA-Reserved
	DNSTYPE_UNSPEC		= 103,		//	IANA-Reserved
	DNSTYPE_TKEY		= 249,		//	Transaction Key							(RFC2930)
	DNSTYPE_TSIG		= 250,		//	Transaction Signature					(RFC2845)
	DNSTYPE_IXFR		= 251,		//	incremental transfer					(RFC1995)
	DNSTYPE_AXFR		= 252,		//	transfer of an entire zone				(RFC1035)
	DNSTYPE_MAILB		= 253,		//	mailbox-related RRs (MB, MG or MR)		(RFC1035)
	DNSTYPE_MAILA		= 254,		//	mail agent RRs (Obsolete - see MX)		(RFC1035)
	DNSTYPE_GLOBAL		= 255,		//	A request for all records				(RFC1035)
	DNSTYPE_TA			= 32768,	//	DNSSEC Trust Authorities				(Weiler)
	DNSTYPE_DLV			= 32769,	//	DNSSEC Lookaside Validation				(RFC4431)
} ;

/*
**	DNS Header
*/

struct	DnsHdr
{
	//	Category:	Internet
	//
	//	Used in the interpretation of a DNS query

	uint16_t	m_nQueryID ;			//	Useful only if set of records require more than one UDP packet. We send this with next request.
	uint16_t	m_nDNACode ;			//	Flags for interpretation - currently ignored
	uint16_t	m_nNoQuestions ;		//	Question count
	uint16_t	m_nNoAnswers ;			//	Answer count
	uint16_t	m_nNoAuth ;				//	Name server record count
	uint16_t	m_nNoAdditional ;		//	Name server record count

	void	_clear	(void)
	{
		m_nQueryID = 0 ;
		m_nDNACode = 0 ;
		m_nNoQuestions = 0 ;
		m_nNoAnswers = 0 ;
		m_nNoAuth = 0 ;
		m_nNoAdditional = 0 ;
	}

	bool		QueryResponse	(void)	{ return m_nDNACode & 0x8000 ? true : false ; }
	uint32_t	OpCode			(void)	{ return (m_nDNACode & 0x7800) >> 11 ; }
	bool		Authoritative	(void)	{ return m_nDNACode & 0x0400 ? true : false ; }
	bool		Truncation		(void)	{ return m_nDNACode & 0x0200 ? true : false ; }
	bool		RecursionDesire	(void)	{ return m_nDNACode & 0x0100 ? true : false ; }
	bool		RecursionAvail	(void)	{ return m_nDNACode & 0x0080 ? true : false ; }
	uint32_t	ResponseCode	(void)	{ return m_nDNACode & 0x000F ; }
} ;

/*
**	DNS Record
*/

struct	DnsRec
{
	//	Category:	Internet
	//
	//	DNS Record - Used in the interpretation of a DNS query

	hzString	m_Domain ;			//	Domain name
	hzString	m_Server ;			//	Server name

	hzIpaddr	m_Ipa ;				//	IP address (V4)
	uint32_t	m_anorakA ;			//	Part of IVP6
	uint32_t	m_anorakB ;			//	Part of IVP6
	uint32_t	m_anorakC ;			//	Part of IVP6
	uint32_t	m_anorakD ;			//	Part of IVP6

	uint32_t	m_nTTL ;			//	Time to live
	uint16_t	m_nType ;			//	Type of record, eg IN
	uint16_t	m_nClass ;			//	Class of record, eg MX
	uint16_t	m_nLen ;			//	Record length
	uint16_t	m_nValue ;			//	Eg MX value

	DnsRec	(void)
	{
		Clear() ;
	}

	~DnsRec	(void)
	{
	}

	void	Clear	(void)
	{
		m_Ipa.Clear() ;
		m_anorakA = m_anorakB = m_anorakC = m_anorakD = 0 ;
		m_nTTL = 0 ;
		m_nType = 0 ;
		m_nClass = 0 ;
		m_nLen = 0 ;
		m_nValue = 0 ;
	}
} ;

struct	hzResServer
{
	//	Category:	Internet
	//
	//	Cut-down version of a DNS record as used by applications

	hzString	m_Servername ;		//	Name of server being queried
	hzIpaddr	m_Ipa ;				//	IP address (V4)
	uint32_t	m_nTTL ;			//	Time to live
	uint16_t	m_nValue ;			//	MX records only
	uint16_t	m_nMisc ;			//	Reserved

	hzResServer	(void)
	{
		m_nTTL = 0 ;
		m_nValue = 0 ;
		m_nMisc = 0 ;
	}
} ;

class	hzDNS
{
	//	Category:	Internet
	//
	//	The DnsQuery class

	uchar*		m_cpDns ;		//	Buffer for DNS output

	uint16_t	m_qID ;			//	So we can check this against ID of query
	uint16_t	m_DNA ;			//	Exactly
	uint32_t	m_Resv ;		//	Reserved

	hzString	_procraw	(uchar** cpPtr) ;
	void		_clear		(void) ;

public:
	hzList<DnsRec>	m_arQus ;	//	Question section
	hzList<DnsRec>	m_arAns ;	//	Answer section
	hzList<DnsRec>	m_arAut ;	//	Authorative section
	hzList<DnsRec>	m_arAdd ;	//	Additional section

	hzDNS	(void)
	{
		m_cpDns = 0 ;
	}

	~hzDNS	(void)
	{
		m_arQus.Clear() ;
		m_arAns.Clear() ;
		m_arAut.Clear() ;
		m_arAdd.Clear() ;
		delete [] m_cpDns ;
	}

	hzEcode		Query		(const char* dom, DnsType eType) ;
	hzEcode		SelectMX	(hzList<hzResServer>& ServersMX, const char* dom) ;

	hzEcode		QueryA		(const char* dom)	{ Query(dom, DNSTYPE_A) ; }
	hzEcode		QueryPTR	(const char* dom)	{ Query(dom, DNSTYPE_PTR) ; }
	hzEcode		QueryTXT	(const char* dom)	{ Query(dom, DNSTYPE_TXT) ; }
	hzEcode		QuerySPF	(const char* dom)	{ Query(dom, DNSTYPE_SPF) ; }
	hzEcode		QueryMX		(const char* dom)	{ Query(dom, DNSTYPE_MX) ; }

	void		Show			(hzChain& Result) const ;

	uint32_t	NoAnswers		(void) const	{ return m_arAns.Count() ; }
	uint32_t	NoAuth			(void) const	{ return m_arAut.Count() ; }
	uint32_t	NoAdditional	(void) const	{ return m_arAdd.Count() ; }
} ;

/*
**	Prototypes
*/

hzEcode	GetHostByAddr	(hzString& Host, const char* cpIPAddr) ;

#endif	//	hzDNS_h
