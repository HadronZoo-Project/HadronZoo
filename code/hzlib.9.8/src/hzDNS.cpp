//
//	File:	hzDNS.cpp
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
//	DNS/MX Record lookup functions
//

#include <iostream>

#include <netdb.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <string.h>

#include "hzCtmpls.h"
#include "hzChars.h"
#include "hzString.h"
#include "hzErrcode.h"
#include "hzProcess.h"
#include "hzDNS.h"

/*
**	Variables
*/

static	bool	s_bInitResolver ;	//	Resolver initialized flag

hzEcode	GetHostByAddr	(hzString& Host, const char* cpIPAddr)
{
	//	Category:	Internet
	//
	//	GetHostByAddr: Populate the supplied string with the servername (hostname) at the supplied IP address. Return the DNS
	//	return code.
	//
	//	Arguments:	1)	Host		The Hostname (at the supplied IP address)
	//				2)	cpIPAddr	The IP address
	//
	//	Returns:	E_DNS_NOHOST	If the host is unknown.
	//				E_DNS_NODATA	If the name is valid but does not have an IP address.
	//				E_DNS_FAILED	If a non-recoverable name server error occurred.
	//				E_DNS_RETRY		If a temporary error occurred on an authoritative name server.
	//				E_OK			If the hostname is found by the DNS

	_hzfunc(__func__) ;

	HOSTENT*	pHost ;		//	Pointer to host from gethostbyaddr()
	in_addr		x ;			//	Internet address translated by inet_addr()

	x.s_addr = inet_addr(cpIPAddr) ;

	//	Unix only: win xlate:	pHost = gethostbyaddr(cpIPAddr, 4, AF_INET) ;
	pHost = gethostbyaddr(&x, 4, AF_INET) ;

	Host = 0 ;
	if (pHost)
	{
		Host = pHost->h_name ;
		return E_OK ;
	}

	switch (h_errno)
	{
	case HOST_NOT_FOUND:	return E_DNS_NOHOST ;	//	The host is unknown.
	case NO_DATA:			return E_DNS_NODATA ;	//	The name is valid but does not have an IP address.
	case NO_RECOVERY:		return E_DNS_FAILED ;	//	A non-recoverable name server error occurred.
	case TRY_AGAIN:			return E_DNS_RETRY ;	//	A temporary error occurred on an authoritative name server.
	}

	return E_DNS_FAILED ;
}

hzString	hzDNS::_procraw	(uchar** cpPtr)
{
	//	Obtain a null terminated server name from the DNS response buffer
	//
	//	The supplied argument is a pointer to the pointer into the DNS response buffer. The string extraction process is as follows:-
	//
	//	We assume we are one a byte which either describes a lenght or a location. A byte of 0xC0 or greater states that the next byte
	//	specifies a location (within the first 240 bytes of the buffer) where the string can be found. A byte lower than this indicates
	//	the length of the string which can be found directly after this byte.
	//
	//	Note that a string can be comprised of a mix of earlier strings (0xC0) and new strings.
	//
	//	Arguments:	1)	cpPtr	A pointer to the pointer into the DNS response buffer
	//
	//	Returns:	Instance of hzString by value being server name.

	_hzfunc("hzDNS::_procraw") ;

	hzChain		Z ;			//	For formulating string to be returned
	uchar*		p ;			//	Pointer into orig buf
	uchar*		i ;			//	DNS response iterator
	hzString	S ;			//	String to be returned (domain/server name)
	uint32_t	len ;		//	Length of pre-parsed chunk
	uint32_t	c ;			//	Counter upto length
	uint32_t	nOset ;		//	Offset into orig buf

	/*
	**	Process answer
	*/

	i = *cpPtr ;

	for (; *i ;)
	{
		if (*i & 0xC0)
		{
			//	Can we use an offset to a pre parsed base name?
			nOset = ((*i & 0x3F) << 8) + i[1] ;

			if (nOset < 240)
			{
				//	Yes - Set p to this locn and proceed until a 0
				i += 2 ;
				p = m_cpDns + nOset ;

				for (; *p ;)
				{
					if (*p & 0xC0)
					{
						nOset = ((*p & 0x3F) << 8) + p[1] ;
						p = m_cpDns + nOset ;
					}

					//	Get no of bytes before next period
					len = (uint32_t) *p ;
					p++ ;

					for (c = 0 ; c < len ; c++)
						Z.AddByte(*p++) ;
					if (*p)
						Z.AddByte(CHAR_PERIOD) ;
				}
				break ;
			}
		}

		//	No backward pre-parsed to be applied to string, just len and string
		len = (int) *i ;
		p = i + 1 ;
		i = p + len ;

		for (c = 0 ; c < len ; c++)
			Z.AddByte(*p++) ;
		if (*p)
			Z.AddByte(CHAR_PERIOD) ;

		if (i[0] == 0)
			{ i++ ; break ; }
	}

	*cpPtr = i ;
	S = Z ;
	return S ;
} 

void	hzDNS::_clear	(void)
{
	//	Resets the hzDNS instance back to an unpopulated state. Note this does not delete the operating buffer. This is only deleted
	//	by the destructor.
	//
	//	Arguments:	None
	//	Returns:	None

	m_arQus.Clear() ;
	m_arAns.Clear() ;
	m_arAut.Clear() ;
	m_arAdd.Clear() ;

	m_qID = 0 ;
	m_DNA = 0 ;
	//m_nQus = 0 ;
	//m_nAns = 0 ;
	//m_nAut = 0 ;
	//m_nAdd = 0 ;

	if (m_cpDns)
		memset(m_cpDns, 0, 2048) ;
}

hzEcode	hzDNS::Query	(const char* dom, DnsType eType)
{
	//	Do the actual DNS query.
	//
	//	Arguments:	1)	eType	Query type	Either MX or A
	//				2)	dom		Domain name
	//
	//	Returns:	E_ARGUMENT		If the domain name is not supplied
	//				E_INITFAIL		If res_init() has not been called before and now returns an error
	//				E_OVERFLOW		If the res_search function owverfills the buffer
	//				E_DNS_NOHOST	If the host does not exist
	//				E_DNS_NODATA	If the domain exists but there is no host for the applicable service
	//				E_DNS_RETRY		If the DNS could not provide data at this time
	//				E_DNS_FAILED	If the DNS failed
	//				E_OK			If the DNS query was successful

	_hzfunc("hzDNS::_dnsquery") ;

	hzChain		E ;			//	Debug mode
	DnsRec		dr ;		//	DNS record
	uchar*		ix ;		//	DNS record iterator
	hzLogger*	pLog ;		//	For debug mode
	uchar*		jx ;		//	DNS record forward marker
	uint32_t	nQus ;		//	Questions
	uint32_t	nAns ;		//	Questions
	uint32_t	nAut ;		//	Questions
	uint32_t	nAdd ;		//	Questions
	uint32_t	nCount ;	//	Interator res_search response
	int32_t		nSize ;		//	Bytes returned by res_search

	//	Check args
	if (!dom || !dom[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No domain name supplied") ;

	//	Check buffer
	if (!m_cpDns)
		m_cpDns = new uchar[2048] ;
	ix = m_cpDns ;
	if (!ix)
		hzexit(_fn, 0, E_MEMORY, "Could not allocate buffers for DNS query") ;

	//	Chekck res_init() has been called (only once)
	if (!s_bInitResolver)
	{
		if (res_init() == -1)
			return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Could not init DNS resolver") ;
		s_bInitResolver = true ;
	}
	_clear() ;

	pLog = GetThreadLogger() ;

	/*
	**	Query the DNS
	*/

	nSize = res_search(dom, C_IN, eType, m_cpDns, 2047) ;

	if (nSize >= 2000)
		return hzerr(_fn, HZ_ERROR, E_OVERFLOW, "DNS buffer overflow for %s", dom) ;

	if (nSize == -1)
	{
		switch (h_errno)
		{
		case HOST_NOT_FOUND:	return E_DNS_NOHOST ;	//	The specified host is unknown.
		case NO_DATA:			return E_DNS_NODATA ;	//	The requested name is valid but does not have an IP address.
		case NO_RECOVERY:		return E_DNS_FAILED ;	//	A non-recoverable name server error occurred.
		case TRY_AGAIN:			return E_DNS_RETRY ;	//	A temporary error occurred on an authoritative name server. Try again later.
		}

		return hzerr(_fn, HZ_ERROR, E_DNS_FAILED, "Unspecified error (%s) for domain %s", hstrerror(h_errno), dom) ;
	}

	m_cpDns[nSize] = 0 ;

	if (_hzGlobal_Debug & HZ_DEBUG_DNS)
	{
		E.Printf("DNS BUFFER (%d bytes) = [\n", nSize) ;
		for (nCount = 0 ; nCount < (uint32_t) nSize ;)
		{
			if (nCount < 240 && m_cpDns[nCount] > 32 && m_cpDns[nCount] < 128)
				E.Printf(" %c", m_cpDns[nCount]) ;
			else
				E.Printf(" %02x", m_cpDns[nCount]) ;

			nCount++ ;
			if (!(nCount % 32))
				E.AddByte(CHAR_NL) ;
			else
				//pNote->AddByte(CHAR_COMMA) ;
				E.AddByte(CHAR_SPACE) ;
		}
		if (nCount % 32)
			E.AddByte(CHAR_NL) ;
		E.Printf("] END\n") ;
		pLog->Out(E) ;
	}

	//	Get result params
	m_qID =  (m_cpDns[0]  << 8) + m_cpDns[1] ;
	m_DNA =  (m_cpDns[2]  << 8) + m_cpDns[3] ;
	nQus = (m_cpDns[4]  << 8) + m_cpDns[5] ;
	nAns = (m_cpDns[6]  << 8) + m_cpDns[7] ;
	nAut = (m_cpDns[8]  << 8) + m_cpDns[9] ;
	nAdd = (m_cpDns[10] << 8) + m_cpDns[11] ;

	if (_hzGlobal_Debug & HZ_DEBUG_DNS)
	{
		pLog->Out("Query id:      %d\n", m_qID) ;
		pLog->Out("DNA code:      %d\n", m_DNA) ;
		pLog->Out("No questions:  %d\n", nQus) ;
		pLog->Out("No answers:    %d\n", nAns) ;
		pLog->Out("No authority:  %d\n", nAut) ;
		pLog->Out("No additional: %d\n", nAdd) ;
	}

	/*
	**	Bypass the 'pre-defined' strings to get the answers
	*/

	for (ix = m_cpDns + 12 ; *ix ; ix++) ;
	ix += 5 ;

	//	Now have answers
	for (nCount = 0 ; nCount < nAns ; nCount++)
	{
		dr.Clear() ;

		dr.m_Domain = _procraw(&ix) ;
		if (!dr.m_Domain)
			return hzerr(_fn, HZ_ERROR, E_DNS_FAILED, "Answer record without domain") ;

		dr.m_nType =  (ix[0] <<  8) + ix[1] ;
		dr.m_nClass = (ix[2] <<  8) + ix[3] ;
		dr.m_nTTL =   (ix[4] << 24) + (ix[5] << 16) + (ix[6] << 8) + ix[7] ;
		dr.m_nLen =   (ix[8] <<  8) + ix[9] ;
		dr.m_nValue = 0 ;
		ix += 10 ;
		jx = ix + dr.m_nLen ;

		if (dr.m_nType == DNSTYPE_MX)
			{ dr.m_nValue = (ix[0] << 8) + ix[1] ; ix += 2 ; }
		
		if (dr.m_nType == DNSTYPE_A)
			dr.m_Ipa.SetValue(ix[0], ix[1], ix[2], ix[3]) ; 
		else if (dr.m_nType == DNSTYPE_AAAA)
		{
			dr.m_anorakA = (ix[0]  << 24) + (ix[1]  << 16) + (ix[2]  << 8) + (ix[3]) ;
			dr.m_anorakA = (ix[4]  << 24) + (ix[5]  << 16) + (ix[6]  << 8) + (ix[7]) ;
			dr.m_anorakA = (ix[8]  << 24) + (ix[9]  << 16) + (ix[10] << 8) + (ix[11]) ;
			dr.m_anorakA = (ix[12] << 24) + (ix[13] << 16) + (ix[14] << 8) + (ix[15]) ;
			ix += 16 ;
		}
		else
		{
			dr.m_Server = _procraw(&ix) ;
			if (!dr.m_Server)
				return hzerr(_fn, HZ_ERROR, E_DNS_FAILED, "Answer record without server") ;
		}

		m_arAns.Add(dr) ;
		ix = jx ;
	}

	//	Get authority records
	for (nCount = 0 ; nCount < nAut ; nCount++)
	{
		dr.Clear() ;

		//	Get the domain name

		dr.m_Domain = _procraw(&ix) ;
		if (!dr.m_Domain)
			return hzerr(_fn, HZ_ERROR, E_DNS_FAILED, "Authority record without domain") ;

		dr.m_nType =  (ix[0] <<  8) + ix[1] ;
		dr.m_nClass = (ix[2] <<  8) + ix[3] ;
		dr.m_nTTL =   (ix[4] << 24) + (ix[5] << 16) + (ix[6] << 8) + ix[7] ;
		dr.m_nLen =   (ix[8] <<  8) + ix[9] ;
		dr.m_nValue = 0 ;
		ix += 10 ;
		jx = ix + dr.m_nLen ;

		if (dr.m_nType == DNSTYPE_MX)
			{ dr.m_nValue = (ix[0] << 8) + ix[1] ; ix += 2 ; }

		if (dr.m_nType == DNSTYPE_A)
			dr.m_Ipa.SetValue(ix[0], ix[1], ix[2], ix[3]) ; 
		else if (dr.m_nType == DNSTYPE_AAAA)
		{
			dr.m_anorakA = (ix[0]  << 24) + (ix[1]  << 16) + (ix[2]  << 8) + (ix[3]) ;
			dr.m_anorakA = (ix[4]  << 24) + (ix[5]  << 16) + (ix[6]  << 8) + (ix[7]) ;
			dr.m_anorakA = (ix[8]  << 24) + (ix[9]  << 16) + (ix[10] << 8) + (ix[11]) ;
			dr.m_anorakA = (ix[12] << 24) + (ix[13] << 16) + (ix[14] << 8) + (ix[15]) ;
			ix += 16 ;
		}
		else
		{
			dr.m_Server = _procraw(&ix) ;
			if (!dr.m_Server)
				return hzerr(_fn, HZ_ERROR, E_DNS_FAILED, "Authority record without server") ;
		}

		m_arAut.Add(dr) ;
		ix = jx ;
	}

	//	Get additional records
	for (nCount = 0 ; nCount < nAdd ; nCount++)
	{
		dr.Clear() ;

		dr.m_Domain = _procraw(&ix) ;
		if (!dr.m_Domain)
			return hzerr(_fn, HZ_ERROR, E_DNS_FAILED, "Additional record without domain") ;

		dr.m_nType =  (ix[0] <<  8) + ix[1] ;
		dr.m_nClass = (ix[2] <<  8) + ix[3] ;
		dr.m_nTTL =   (ix[4] << 24) + (ix[5] << 16) + (ix[6] << 8) + ix[7] ;
		dr.m_nLen =   (ix[8] <<  8) + ix[9] ;
		dr.m_nValue = 0 ;
		ix += 10 ;
		jx = ix + dr.m_nLen ;

		if (dr.m_nType == DNSTYPE_MX)
			{ dr.m_nValue = (ix[0] << 8) + ix[1] ; ix += 2 ; }

		if (dr.m_nType == DNSTYPE_A)
			dr.m_Ipa.SetValue(ix[0], ix[1], ix[2], ix[3]) ; 
		else if (dr.m_nType == DNSTYPE_AAAA)
		{
			dr.m_anorakA = (ix[0]  << 24) + (ix[1]  << 16) + (ix[2]  << 8) + (ix[3]) ;
			dr.m_anorakA = (ix[4]  << 24) + (ix[5]  << 16) + (ix[6]  << 8) + (ix[7]) ;
			dr.m_anorakA = (ix[8]  << 24) + (ix[9]  << 16) + (ix[10] << 8) + (ix[11]) ;
			dr.m_anorakA = (ix[12] << 24) + (ix[13] << 16) + (ix[14] << 8) + (ix[15]) ;
			ix += 16 ;
		}
		else
		{
			dr.m_Server = _procraw(&ix) ;
			if (!dr.m_Server)
				return hzerr(_fn, HZ_ERROR, E_DNS_FAILED, "Additional record without server") ;
		}

		m_arAdd.Add(dr) ;
		ix = jx ;
	}

	return E_OK ;
}

hzEcode	hzDNS::SelectMX	(hzList<hzResServer>& ServersMX, const char* dom)
{
	//	Select mail servers for a domain
	//
	//	Arguments:	1)	ServersMX	The list of mail servers
	//				2)	dom			Domain name
	//
	//	Returns:	E_ARGUMENT		If the domain name is not supplied
	//				E_INITFAIL		If res_init() has not been called before and now returns an error
	//				E_OVERFLOW		If the res_search function owverfills the buffer
	//				E_DNS_NOHOST	If the host does not exist
	//				E_DNS_NODATA	If the domain exists but there is no host for the applicable service
	//				E_DNS_RETRY		If the DNS could not provide data at this time
	//				E_DNS_FAILED	If the DNS failed
	//				E_OK			If the DNS query was successful

	_hzfunc("hzDNS::SelectMX") ;

	hzList<hzResServer>			smx ;	//	Server names from MX query
	hzList<hzResServer>::Iter	I ;		//	Server interator
	hzList<DnsRec>::Iter		ri ;	//	DNS record iterator	(primary)
	hzList<DnsRec>::Iter		si ;	//	DNS record iterator (secondary)

	hzDNS			dq ;		//	DNS query primary
	hzDNS			dq2 ;		//	DNS query secondary
	hzResServer		S ;			//	Server instance
	hzResServer		X ;			//	Server instance
	DnsRec			dr ;		//	DNS record
	DnsRec			dr2 ;		//	DNS record
	hzEcode			rc ;		//	Return code from DNS lookup functions

	/*
	**	Initialize result.
	*/

	if (!dom || !dom[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No domain name supplied") ;

	//	Lookup MX records in DNS
	rc = dq.QueryMX(dom) ;
	if (rc != E_OK)
		return rc ;


	if (_hzGlobal_Debug & HZ_DEBUG_DNS)
		threadLog("DNS found %d answers, %d auth and %d additional\n", dq.NoAnswers(), dq.NoAuth(), dq.NoAdditional()) ;

	//	Now have MX records for the domain. Sometimes mail servers listed here will have multiple IP addresses.
	for (ri = dq.m_arAns ; ri.Valid() ; ri++)
	{
		dr = ri.Element() ;

		if (dr.m_Ipa == IPADDR_NULL || dr.m_Ipa == IPADDR_LOCAL)
		{
			if (_hzGlobal_Debug & HZ_DEBUG_DNS)
				threadLog("Rejecting - Email would loop back to server (case 1)\n") ;
			continue ;
		}

		S.m_Servername = dr.m_Server ;
		S.m_nValue = dr.m_nValue ;
		S.m_Ipa = dr.m_Ipa ;
		ServersMX.Add(S) ;
	}

	if (!ServersMX.Count())
	{
		//	This is where the initial answer did not for whatever reason, provide an IP address along with the servername.

		if (_hzGlobal_Debug & HZ_DEBUG_DNS)
			threadLog("Initial pass did not produce IP addresses for the mail servers\n") ;

		//for (nIndex = 0 ; nIndex < dq.NoAnswers() ; nIndex++)
		for (ri = dq.m_arAns ; ri.Valid() ; ri++)
		{
			dr = ri.Element() ;

			//	Do a direct (type A) query on the mail server's name
			rc = dq2.QueryA(*dr.m_Server) ;
			if (rc != E_OK)
			{
				if (_hzGlobal_Debug & HZ_DEBUG_DNS)
					threadLog("DNS Unavailable (case 2)\n") ;
				return rc ;
			}

			if (!dq2.NoAnswers())
			{
				if (_hzGlobal_Debug & HZ_DEBUG_DNS)
					threadLog("Type A query on mail-server <%s> produced no answers\n", *dr.m_Server) ;
				return E_DNS_FAILED ;
			}

			for (si = dq2.m_arAns ; si.Valid() ; si++)
			{
				dr2 = si.Element() ;

				if (dr2.m_Ipa == IPADDR_NULL || dr2.m_Ipa == IPADDR_LOCAL)
				{
					if (_hzGlobal_Debug & HZ_DEBUG_DNS)
						threadLog("Rejecting - Email would loop back to server (case 2)\n") ;
					continue ;
				}

				S.m_Servername = dr2.m_Server ;
				S.m_nValue = dr2.m_nValue ;
				S.m_Ipa = dr2.m_Ipa ;
				ServersMX.Add(S) ;
			}
		}
	}

	return rc ;
}

void	hzDNS::Show	(hzChain& Result) const
{
	//	Output search results to the supplied hzChain
	//
	//	Arguments:	1)	Result	The output chain
	//
	//	Returns:	None

	_hzfunc("hzDNS::Show") ;

	hzList<DnsRec>::Iter	ir ;	//	DNS record iterator

	DnsRec		dr ;		//	DNS record

	//	Print the results

	Result.Printf("Querry id:   %d\n", m_qID) ;
	Result.Printf("Questions:   %d\n", m_arQus.Count()) ;
	Result.Printf("Answers:     %d\n", m_arAns.Count()) ;
	Result.Printf("Authorative: %d\n", m_arAut.Count()) ;
	Result.Printf("Additional:  %d\n", m_arAdd.Count()) ;

	if (m_arAns.Count())
	{
		Result.Printf("\n;; ANSWER SECTION             Type Class    TTL  Len Value Server\n") ;

		for (ir = m_arAns ; ir.Valid() ; ir++)
		{
			dr = ir.Element() ;

			if (dr.m_Server)
				Result.Printf("%-29s %4d  %4d %6d %4d  %4d %-30s",
					*dr.m_Domain, dr.m_nType, dr.m_nClass, dr.m_nTTL, dr.m_nLen, dr.m_nValue, *dr.m_Server) ;
			else
				Result.Printf("%-29s %4d  %4d %6d %4d  %4d",
					*dr.m_Domain, dr.m_nType, dr.m_nClass, dr.m_nTTL, dr.m_nLen, dr.m_nValue) ;

			if (dr.m_Ipa)
				Result.Printf("\t(%s)", *dr.m_Ipa.Str()) ;
			Result.AddByte(CHAR_NL) ;
		}
	}

	if (m_arAut.Count())
	{
		Result.Printf("\n;; AUTHORITY SECTION          Type Class    TTL  Len Value Server               Address\n") ;

		for (ir = m_arAut ; ir.Valid() ; ir++)
		{
			dr = ir.Element() ;

			if (dr.m_Server)
				Result.Printf("%-29s %4d  %4d %6d %4d  %4d %-30s",
					*dr.m_Domain, dr.m_nType, dr.m_nClass, dr.m_nTTL, dr.m_nLen, dr.m_nValue, *dr.m_Server) ;
			else
				Result.Printf("%-29s %4d  %4d %6d %4d  %4d",
					*dr.m_Domain, dr.m_nType, dr.m_nClass, dr.m_nTTL, dr.m_nLen, dr.m_nValue) ;

			if (dr.m_Ipa)
				Result.Printf("\t(%s)", *dr.m_Ipa.Str()) ;
			Result.AddByte(CHAR_NL) ;
		}
	}

	if (m_arAdd.Count())
	{
		Result.Printf("\n;; ADDITIONAL SECTION         Type Class    TTL  Len Value Server               Address\n") ;

		for (ir = m_arAdd ; ir.Valid() ; ir++)
		{
			dr = ir.Element() ;

			if (dr.m_Server)
				Result.Printf("%-29s %4d  %4d %6d %4d  %4d %-30s",
					*dr.m_Domain, dr.m_nType, dr.m_nClass, dr.m_nTTL, dr.m_nLen, dr.m_nValue, *dr.m_Server) ;
			else
				Result.Printf("%-29s %4d  %4d %6d %4d  %4d",
					*dr.m_Domain, dr.m_nType, dr.m_nClass, dr.m_nTTL, dr.m_nLen, dr.m_nValue) ;

			if (dr.m_Ipa)
				Result.Printf("\t(%s)", *dr.m_Ipa.Str()) ;
			Result.AddByte(CHAR_NL) ;
		}
	}
}
