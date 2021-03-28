//
//	File:	hzTcpClient.cpp
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
//	Implimentation of the hzTcpClient class
//

#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "hzCtmpls.h"
#include "hzProcess.h"
#include "hzTcpClient.h"

/*
**	SECTION 1:	Set up client side SSL parameters
*/

static	SSL_CTX*			s_SSL_ClientCtx = 0 ;		//	SSL client CTX structure
static	const SSL_METHOD*	s_SSL_ClientMethod = 0 ;	//	SSL client method

hzEcode	InitClientSSL	(void)
{
	//	Category:	Internet
	//
	//	Initializes client side SSL
	//
	//	Arguments:	None
	//
	//	Returns:	E_INITDUP	If this has already been done
	//				E_INITFAIL	If the SSL client method could not be obtained
	//				E_OK		If the client side SSL is initialized

	_hzfunc("InitServerSSL") ;

	//FILE*	sslErrors ;
	//int32_t	sys_rc ;

	if (s_SSL_ClientCtx)
		return hzerr(_fn, HZ_ERROR, E_INITDUP, "Client SSL Init called already") ;

	//sslErrors = fopen("/home/hats/ssl_client_errors.txt", "a") ;

	SSL_library_init() ;
	SSL_load_error_strings() ;
	s_SSL_ClientMethod = SSLv3_method() ;

	if (!s_SSL_ClientMethod)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "No SSL Client Method issued") ;

	s_SSL_ClientCtx = SSL_CTX_new(s_SSL_ClientMethod) ;

	//	LATER
	//	if (!SSL_CTX_load_verify_locations(s_SSL_ClientCtx, RSA_CLIENT_CA_CERT, NULL))
	//		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Not able to verify locations") ;

	SSL_CTX_set_verify(s_SSL_ClientCtx,SSL_VERIFY_PEER,NULL);
	SSL_CTX_set_verify_depth(s_SSL_ClientCtx,1);

	return E_OK ;
}

/*
**	Section 2:	hzTcpClient member functions
*/

#if 0
hzEcode	hzTcpClient::Init	(const char* hostname, uint32_t nPort, uint32_t nTimeoutR, uint32_t nTimeoutS, bool bSSL)
{
	//	Initialize a hzTcpClient instance ahead of calling Connect(). The reason for this call sequence is to avoid unnessesary DNS lookups if the connection is
	//	unreliable and numerous calls to Connect are required to re-establish the connection in order to complete the session. FTP servers being a good example!
	//
	//	This function establishes the IP address of the server the application is to connect to as client. The Connect function then simply uses this to connect
	//	to without DNS lookup.
	//
	//	Arguments:	1)	hostname	The server name (or IP address)
	//				2)	nPort		The port number
	//				3)	nTimoutR	Socket option read timeout (default 30 seconds)
	//				4)	nTimoutW	Socket option write timeout (default 30 seconds)
	//
	//	Returns:	E_DNS_NOHOST	If the domain does not exist
	//				E_DNS_FAILED	If the domain settings were invalid
	//				E_DNS_NODATA	If the domain exists but no server found
	//				E_DNS_RETRY		If the DNS was busy
	//				E_INITFAIL		If the SSL client side settings faile (SSL only)
	//				E_OK			If the connection parameters for the host are initialized

	_hzfunc("hzTcpClient::Connect") ;

	hzEcode	rc = E_OK ;		//	Return code

	//	Check we are not already connected. If we are clear the settings.
	if (m_nSock)
	{
		if (m_Hostname == hostname && m_nPort == nPort)
			return E_OK ;

		m_Hostname.Clear() ;
		m_pHost = 0 ;
		Close() ;
	}

	//	If using SSL
	if (bSSL)
	{
		if (!s_SSL_ClientCtx)
		{
	   		//  Init SSL
			rc = InitClientSSL() ;
			if (rc != E_OK)
				return hzerr(_fn, HZ_ERROR, E_HOSTFAIL, "Could not Init client side SSL") ;
		}
	}

	if (m_Hostname && m_Hostname != hostname)
	{
		//	At the point of call there was no socket but this hzTcpClient instance has a hostname from a previous connection. If this differs from the host now
		//	being sought then the m_pHost value will be invalid and a fresh call to gethostbyname is required.

		m_Hostname = hostname ;
		m_pHost = 0 ;
	}

	if (!m_Hostname)
		m_Hostname = hostname ;

	//	If we have not got the hostname from a previous connect, get the hostname now
	if (!m_pHost)
	{
		m_pHost = gethostbyname(hostname) ;

		if (!m_pHost)
		{
			if (h_errno == TRY_AGAIN)		return E_DNS_RETRY ;
			if (h_errno == HOST_NOT_FOUND)	return E_DNS_NOHOST ;
			if (h_errno == NO_RECOVERY)		return E_DNS_FAILED ;

			if (h_errno == NO_DATA || h_errno == NO_ADDRESS)
				return E_DNS_NODATA ;

			m_Hostname.Clear() ;
			return hzerr(_fn, HZ_ERROR, E_DNS_NOHOST, "Unknown Host [%s]\n", hostname) ;
		}
	}
	
	//	Create the socket
	m_nPort = nPort ;

	memset(&m_SvrAddr, 0, sizeof(m_SvrAddr)) ;
	m_SvrAddr.sin_family = AF_INET ;
	memcpy(&m_SvrAddr.sin_addr, m_pHost->h_addr, m_pHost->h_length) ;
	m_SvrAddr.sin_port = htons(nPort) ;

	if ((m_nSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Could not create socket (errno=%d)", errno) ;
	
	//	Connect stage
	if (connect(m_nSock, (SOCKADDR*) &m_SvrAddr, sizeof(m_SvrAddr)) < 0)
		return hzerr(_fn, HZ_ERROR, E_HOSTFAIL, "Could not connect to host [%s] on port %d (errno=%d)", *m_Hostname, m_nPort, errno) ;

	//	Apply timeouts
	m_nTimeoutS = nTimeoutS ;
	m_nTimeoutR = nTimeoutR ;

	return E_OK ;
}
#endif

hzEcode	hzTcpClient::ConnectStd	(const char* hostname, uint32_t nPort, uint32_t nTimeoutR, uint32_t nTimeoutS)
{
	//	Establish a standard, non-local, non-SSL TCP connection to a server. Note that to re-connect in the event of a drop-out, just call this function again.
	//	A subsequent call will not repeat the DNS query unless it is to a different hostname. Note also that this function will do nothing if called with the
	//	same hostname, no error flag has been set and the socket is non-zero. The error flag is set by send or recv errors.
	//
	//	Arguments:	1)	hostname	The server name or IP address
	//				2)	nPort		The port number
	//				3)	nTimoutR	Socket option read timeout (default 30 seconds)
	//				4)	nTimoutW	Socket option write timeout (default 30 seconds)
	//
	//	Returns:	E_DNS_NOHOST	If the domain does not exist
	//				E_DNS_FAILED	If the domain settings were invalid
	//				E_DNS_NODATA	If the domain exists but no server found
	//				E_DNS_RETRY		If the DNS was busy
	//				E_NOSOCKET		If a socket could not be obtained
	//				E_HOSTFAIL		If no connection could be established or if socket options were not set.
	//				E_OK			If a connection to the host was established

	_hzfunc("hzTcpClient::ConnectStd") ;

	hzEcode	rc = E_OK ;		//	Return code

	//	Check we are not already connected
	if (m_nSock)
	{
		if (m_Hostname == hostname && m_nPort == nPort)
			return E_OK ;

		m_Hostname.Clear() ;
		m_pHost = 0 ;
		Close() ;
	}

	if (m_Hostname && m_Hostname != hostname)
	{
		//	At the point of call there was no socket but this hzTcpClient instance has a hostname from a previous connection. If this differs from the host now
		//	being sought then the m_pHost value will be invalid and a fresh call to gethostbyname is required.

		m_Hostname = hostname ;
		m_pHost = 0 ;
	}

	if (!m_Hostname)
		m_Hostname = hostname ;

	//	If we have not got the hostname from a previous connect, get the hostname now
	if (!m_pHost)
	{
		m_pHost = gethostbyname(hostname) ;

		if (!m_pHost)
		{
			if (h_errno == TRY_AGAIN)		return E_DNS_RETRY ;
			if (h_errno == HOST_NOT_FOUND)	return E_DNS_NOHOST ;
			if (h_errno == NO_RECOVERY)		return E_DNS_FAILED ;

			if (h_errno == NO_DATA || h_errno == NO_ADDRESS)
				return E_DNS_NODATA ;

			m_Hostname.Clear() ;
			return hzerr(_fn, HZ_ERROR, E_DNS_NOHOST, "Unknown Host [%s]\n", hostname) ;
		}
	}
	
	//	Create the socket

	m_nPort = nPort ;

	memset(&m_SvrAddr, 0, sizeof(m_SvrAddr)) ;
	m_SvrAddr.sin_family = AF_INET ;
	memcpy(&m_SvrAddr.sin_addr, m_pHost->h_addr, m_pHost->h_length) ;
	m_SvrAddr.sin_port = htons(nPort) ;

	if ((m_nSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Could not create socket (errno=%d)", errno) ;
	
	//	Connect stage
	if (connect(m_nSock, (SOCKADDR*) &m_SvrAddr, sizeof(m_SvrAddr)) < 0)
		return hzerr(_fn, HZ_ERROR, E_HOSTFAIL, "Could not connect to host [%s] on port %d (errno=%d)", *m_Hostname, m_nPort, errno) ;

	//	Apply timeouts
	rc = SetRecvTimeout(nTimeoutR) ;
	if (rc == E_OK)
		rc = SetSendTimeout(nTimeoutR) ;

	return E_OK ;
}

hzEcode	hzTcpClient::ConnectIP	(const hzIpaddr& ipa, uint32_t nPort, uint32_t nTimeoutR, uint32_t nTimeoutS)
{
	//	Establish a standard, non-local, non-SSL TCP connection to a server. Note that to re-connect in the event of a drop-out, just call this function again.
	//	A subsequent call will not repeat the DNS query unless it is to a different hostname. Note also that this function will do nothing if called with the
	//	same hostname, no error flag has been set and the socket is non-zero. The error flag is set by send or recv errors.
	//
	//	Arguments:	1)	hostname	The server name or IP address
	//				2)	nPort		The port number
	//				3)	nTimoutR	Socket option read timeout (default 30 seconds)
	//				4)	nTimoutW	Socket option write timeout (default 30 seconds)
	//
	//	Returns:	E_DNS_NOHOST	If the domain does not exist
	//				E_DNS_FAILED	If the domain settings were invalid
	//				E_DNS_NODATA	If the domain exists but no server found
	//				E_DNS_RETRY		If the DNS was busy
	//				E_NOSOCKET		If a socket could not be obtained
	//				E_HOSTFAIL		If no connection could be established or if socket options were not set.
	//				E_OK			If a connection to the host was established

	_hzfunc("hzTcpClient::ConnectStd") ;

	hzEcode	rc = E_OK ;		//	Return code

	//	Check we are not already connected
	if (m_nSock)
	{
		//if (m_Hostname == hostname && m_nPort == nPort)
		if (m_nPort == nPort)
			return E_OK ;

		m_Hostname.Clear() ;
		m_pHost = 0 ;
		Close() ;
	}

	//	if (m_Hostname && m_Hostname != hostname)
	//	{
		//	At the point of call there was no socket but this hzTcpClient instance has a hostname from a previous connection. If this differs from the host now
		//	being sought then the m_pHost value will be invalid and a fresh call to gethostbyname is required.

		//	m_Hostname = hostname ;
		//	m_pHost = 0 ;
	//	}

	//	if (!m_Hostname)
	//		m_Hostname = hostname ;

	//	If we have not got the hostname from a previous connect, get the hostname now
	/*
	if (!m_pHost)
	{
		m_pHost = gethostbyname(hostname) ;

		if (!m_pHost)
		{
			if (h_errno == TRY_AGAIN)		return E_DNS_RETRY ;
			if (h_errno == HOST_NOT_FOUND)	return E_DNS_NOHOST ;
			if (h_errno == NO_RECOVERY)		return E_DNS_FAILED ;

			if (h_errno == NO_DATA || h_errno == NO_ADDRESS)
				return E_DNS_NODATA ;

			m_Hostname.Clear() ;
			return hzerr(_fn, HZ_ERROR, E_DNS_NOHOST, "Unknown Host [%s]\n", hostname) ;
		}
	}
	*/
	
	//	Create the socket

	m_nPort = nPort ;

	memset(&m_SvrAddr, 0, sizeof(m_SvrAddr)) ;
	m_SvrAddr.sin_family = AF_INET ;
	memcpy(&m_SvrAddr.sin_addr, m_pHost->h_addr, m_pHost->h_length) ;
	m_SvrAddr.sin_port = htons(nPort) ;

	if ((m_nSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Could not create socket (errno=%d)", errno) ;
	
	//	Connect stage
	if (connect(m_nSock, (SOCKADDR*) &m_SvrAddr, sizeof(m_SvrAddr)) < 0)
		return hzerr(_fn, HZ_ERROR, E_HOSTFAIL, "Could not connect to host [%s] on port %d (errno=%d)", *m_Hostname, m_nPort, errno) ;

	//	Apply timeouts
	rc = SetRecvTimeout(nTimeoutR) ;
	if (rc == E_OK)
		rc = SetSendTimeout(nTimeoutR) ;

	return E_OK ;
}

hzEcode	hzTcpClient::ConnectSSL	(const char* hostname, uint32_t nPort, uint32_t nTimeoutR, uint32_t nTimeoutS)
{
	//	Purpose:	Establish a TCP connection to a server
	//
	//	Arguments:	1)	hostname	The server name or IP address
	//				2)	nPort		The port number
	//				3)	nTimeoutR	Timeout (Recv)
	//				4)	nTimeoutS	Timeout (Send)
	//
	//	Returns:	E_DNS_NOHOST	If the domain does not exist
	//				E_DNS_FAILED	If the domain settings were invalid
	//				E_DNS_NODATA	If the domain exists but no server found
	//				E_DNS_RETRY		If the DNS was busy
	//				E_INITFAIL		If the SSL client side settings fail
	//				E_NOSOCKET		If a socket could not be obtained
	//				E_HOSTFAIL		If no connection could be established or if socket options were not set.
	//				E_OK			If a connection to the host was established

	_hzfunc("hzTcpClient::Connect") ;

	int32_t		sys_rc ;	//	Return from connect call
	hzEcode		rc ;		//	Return code

	if (!s_SSL_ClientCtx)
	{
	   //  Init SSL
		rc = InitClientSSL() ;
		if (rc != E_OK)
			{ threadLog("%s. SSL Init Failed\n", *_fn) ; return rc ; }
	}

	//	Check we are not already connected
	if (m_nSock)
	{
		if (m_Hostname == hostname && m_nPort == nPort)
			return E_OK ;
		Close() ;
	}

	m_Hostname = hostname ;
	m_nPort = nPort ;

	//	Get the hostname
	m_pHost = gethostbyname(hostname) ;

	if (!m_pHost)
	{
		if (h_errno == TRY_AGAIN)		return E_DNS_RETRY ;
		if (h_errno == HOST_NOT_FOUND)	return E_DNS_NOHOST ;
		if (h_errno == NO_RECOVERY)		return E_DNS_FAILED ;

			if (h_errno == NO_DATA || h_errno == NO_ADDRESS)
		return E_DNS_NODATA ;

			m_Hostname.Clear() ;
		return hzerr(_fn, HZ_ERROR, E_DNS_NOHOST, "Unknown Host [%s]\n", hostname) ;
	}
	
	//	Create the socket

	memset(&m_SvrAddr, 0, sizeof(m_SvrAddr)) ;
	m_SvrAddr.sin_family = AF_INET ;
	memcpy(&m_SvrAddr.sin_addr, m_pHost->h_addr, m_pHost->h_length) ;
	m_SvrAddr.sin_port = htons(nPort) ;

	if ((m_nSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Could not create socket (returns %d, errno=%d)", m_nSock, errno) ;
	}

	//	First establish a TCP/IP connection
	sys_rc = connect(m_nSock, (SOCKADDR*) &m_SvrAddr, sizeof(m_SvrAddr)) ;
	if (sys_rc < 0)
	{
		threadLog("%s: Could not connect to host (returns %d)", *_fn, sys_rc) ;
		return E_HOSTFAIL ;
	}

	//	Apply timeouts
	rc = SetRecvTimeout(nTimeoutR) ;
	if (rc == E_OK)
		rc = SetSendTimeout(nTimeoutR) ;

	//	SSL Connect stage
	m_pSSL = SSL_new(s_SSL_ClientCtx) ;
	SSL_set_fd(m_pSSL, m_nSock);
	sys_rc = SSL_connect(m_pSSL);

	if (sys_rc != 1)
	{
		threadLog("%s. Could not connect: %s\n", *_fn, ShowErrorSSL(SSL_get_error(m_pSSL, sys_rc))) ;
		return E_HOSTFAIL ;
	}
	/*
	if (bCheckCert)
	{
		if (SSL_get_peer_certificate(m_pSSL) != NULL)
		{
			if (SSL_get_verify_result(m_pSSL) == X509_V_OK)
				threadLog("%s. client verification with SSL_get_verify_result() succeeded.\n", *_fn) ;
			else
				threadLog("%s. client verification with SSL_get_verify_result() failed.\n", *_fn) ;
		}
	}
	*/

	return E_OK ;
}

hzEcode	hzTcpClient::ConnectLoc	(uint32_t nPort)
{
	//	Purpose:	Establish a UNIX domain TCP connection to a server
	//
	//	Arguments:	1)	nPort	The port number
	//
	//	Returns:	E_NOSOCKET	If the socket could not be created
	//				E_HOSTFAIL	If not connection can be established
	//				E_OK		If operation successfull

	_hzfunc("hzTcpClient::Connect") ;

	//	Check we are not already connected
	if (m_nSock)
	{
		if (m_nPort == nPort)
			return E_OK ;
		Close() ;
	}

	m_Hostname = "127.0.0.1" ;
	m_nPort = nPort ;

	//	Get the hostname
	if (!(m_pHost = gethostbyname(*m_Hostname)))
	{
		threadLog("%s: Unknown Host [%s]\n", *_fn, *m_Hostname) ;
		return E_HOSTFAIL ;
	}
	
	//	Create the socket

	memset(&m_SvrAddr, 0, sizeof(m_SvrAddr)) ;
	m_SvrAddr.sin_family = AF_UNIX ;
	memcpy(&m_SvrAddr.sin_addr, m_pHost->h_addr, m_pHost->h_length) ;
	m_SvrAddr.sin_port = htons(nPort) ;

	if ((m_nSock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Could not create socket (returns %d, errno %d)", m_nSock, errno) ;
	}
	
	//	Connect stage
	if (connect(m_nSock, (struct sockaddr *) &m_SvrAddr, sizeof(m_SvrAddr)) < 0)
		return hzerr(_fn, HZ_ERROR, E_HOSTFAIL, "Could not connect to host (errno=%d)", errno) ;

	if (m_nSock <= 0)
	{
		threadLog("%s: Unspecified error. Socket is %d\n", *_fn, m_nSock) ;
		return E_HOSTFAIL ;
	}

	return E_OK ;
}

hzEcode	hzTcpClient::SetSendTimeout	(uint32_t nInterval)
{
	//	Set the socket options so that the timeout for outgoing packets is set to the supplied interval
	//
	//	Arguments:	1)	nInterval	Timeout interval in seconds
	//
	//	Returns:	E_HOSTFAIL	If the timeout could not be set
	//				E_OK		If the timeout was successfully set

	timeval	tv ;	//	Timeout structure

	tv.tv_sec = nInterval > 0 ? nInterval : 30 ;
	tv.tv_usec = 0 ;

	if (setsockopt(m_nSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
		return E_HOSTFAIL ;
	return E_OK ;
}

hzEcode	hzTcpClient::SetRecvTimeout	(uint32_t nInterval)
{
	//	Set the socket options so that the timeout for receiving packets is set to the supplied interval
	//
	//	Arguments:	1)	nInterval	Timeout interval in seconds
	//
	//	Returns:	E_HOSTFAIL	If the timeout could not be set
	//				E_OK		If the timeout was successfully set

	struct timeval	tv ;	//	Timeout structure

	tv.tv_sec = nInterval > 0 ? nInterval : 30 ;
	tv.tv_usec = 0 ;

	if (setsockopt(m_nSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		return E_HOSTFAIL ;
	return E_OK ;
}

void	hzTcpClient::Show	(hzChain& Z)
{
	//	Diagnostic purposes. Show details for host if currently connected
	//
	//	Arguments:	1)	Z	Chain to aggregate report to
	//
	//	Returns:	None

	if (!m_pHost)
		Z << "hzTcpClient::Show: Not Connected\n" ;
	else
	{
		Z.Printf("hzTcpClient::Show: Connected to: %u.%u.%u.%u %s (type %d, len %d)\n",
			m_pHost->h_addr[0], m_pHost->h_addr[1], m_pHost->h_addr[2], m_pHost->h_addr[3],
			m_pHost->h_name, m_pHost->h_addrtype, m_pHost->h_length) ;
		Z.Printf(" (alias) %s\n", m_pHost->h_aliases[0]) ;
		Z.Printf(" (addr) %s\n", m_pHost->h_addr_list[0] + 4) ;
		Z << "------\n" ;
	}
}

hzEcode	hzTcpClient::Send	(const void* pIn, uint32_t nLen)
{
	//	Purpose:	Write buffer content to a socket
	//
	//	Arguments:	1)	pIn		The buffer (void*)
	//				2)	nLen	Number of bytes to send
	//
	//	Returns:	E_NOSOCKET	If the connection is not open
	//				E_WRITEFAIL	If the number of bytes written falls short of the intended number. In this event the connection is closed.
	//				E_OK		If the operation is successfull.

	_hzfunc("hzTcpClient::Send(void*,uint32_t)") ;

	uint32_t	nSent ;		//	Bytes actually sent

	if (!m_nSock)	return E_NOSOCKET ;
	if (nLen == 0)	return E_OK ;

	if (m_pSSL)
		//nSent = SSL_write(m_pSSL, (const char*) pIn, nLen) ;
		nSent = SSL_write(m_pSSL, pIn, nLen) ;
	else
		//nSent = write(m_nSock, (const char*) pIn, nLen) ;
		nSent = write(m_nSock, pIn, nLen) ;

	if (nSent == 0)
	{
		threadLog("%s: Socket timed out while writing to server %s port %d socket %d", *_fn, *m_Hostname, m_nPort, m_nSock) ;
		return E_TIMEOUT ;
	}
	if (nSent != nLen)
	{
		Close() ;
		threadLog("%s: Socket write error to server %s port %d socket %d", *_fn, *m_Hostname, m_nPort, m_nSock) ;
		return E_WRITEFAIL ;
	}

	return E_OK ;
}

hzEcode	hzTcpClient::Send	(const hzChain& C)
{
	//	Purpose:	Write chain content to a socket
	//
	//	Arguments:	1)	C	The chain to send. No size indicator is needed as whole chain is sent.
	//
	//	Returns:	E_NOSOCKET	If the connection is not open
	//				E_WRITEFAIL	If the number of bytes written falls short of the intended number. In this event the connection is closed.
	//				E_TIMEOUT	If the operation timed out.
	//				E_OK		If the operation is successfull.

	_hzfunc("hzTcpClient::Send(hzChain&)") ;

	chIter		ci ;			//	To iterate input chain
	char*		i ;				//	To populate output buffer
	uint32_t	nSend ;			//	Bytes to send in current packet
	uint32_t	nSent ;			//	Bytes actually sent according to write operation
	uint32_t	nTotal = 0 ;	//	Total sent so far
	hzEcode		rc = E_OK ;		//	Return code

	if (!m_nSock)
		return E_NOSOCKET ;

	ci = C ;

	for (; rc == E_OK && nTotal < C.Size() ;)
	{
		for (i = m_Buf, nSend = 0 ; !ci.eof() && nSend < HZ_MAXPACKET ; nSend++, ci++)
			*i++ = *ci ;

		if (!nSend)
			break ;

		//	Do the send
		if (m_pSSL)
			nSent = SSL_write(m_pSSL, m_Buf, nSend) ;
		else
			nSent = write(m_nSock, m_Buf, nSend) ;

		if (nSent < 0)
			rc = errno == ETIMEDOUT ? E_TIMEOUT : E_SENDFAIL ;
		else
			nTotal += nSent ;
	}

	return rc ;
}

hzEcode	hzTcpClient::Recv	(void* vpOut, uint32_t& nRecv, uint32_t nMax)
{
	//	Purpose:	Read from a socket into a buffer.
	//
	//	Arguments:	1)	vpOut	The buffer to populate
	//				2)	nRecv	A reference to number of bytes received
	//				3)	nMax	The maximum number of bytes to receive
	//
	//	Returns:	E_NOSOCKET	If the connection has been closed
	//				E_RECVFAIL	If the socket read operation fails
	//				E_OK		If operation successfull

	_hzfunc("hzTcpClient::Recv(1)") ;

	char*	cpOut ;		//	Buffer recast to char*

	cpOut = (char*) vpOut ;
	cpOut[0] = 0 ;

	if (!m_nSock)
		return E_NOSOCKET ;
	if (nMax == 0)
		return E_OK ;

	if (m_pSSL)
		nRecv = SSL_read(m_pSSL, cpOut, nMax) ;
	else
		nRecv = recv(m_nSock, cpOut, nMax, 0) ;

	if (nRecv < 0)
	//if ((nRecv = (int32_t) recv(m_nSock, cpOut, nMax, 0)) < 0)
	{
		if (errno == EAGAIN)
		{
			if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
				threadLog("%s. call returns %d with errno of %d\n", *_fn, nRecv, errno) ;
			nRecv = 0 ;
			return E_OK ;
		}

		if (errno == ETIMEDOUT)
		{
			if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
				threadLog("%s. call returns %d with errno of %d\n", *_fn, nRecv, errno) ;
			return E_TIMEOUT ;
		}

		threadLog("%s. call returns %d with errno of %d\n", *_fn, nRecv, errno) ;
		Close() ;
		return E_RECVFAIL ;
	}

	if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
		threadLog("%s. gets %d bytes\n", *_fn, nRecv) ;
	return E_OK ;
}

hzEcode	hzTcpClient::Recv	(hzChain& Z)
{
	//	Purpose:	Read from a socket into a chain. Note this will not nessesarily recieve the complete message as the connection may
	//				time out. It is up to the application to examine the message to determine if it is complete. If not this function
	//				should be repeatedly called as it always appends to the chain.
	//
	//	Arguments:	1)	Z	The chain to populate
	//
	//	Returns:	E_NOSOCKET	If the connection has been closed
	//				E_RECVFAIL	If the socket read operation fails
	//				E_MEMORY	If there was insufficent memory to complete the operation.
	//				E_OK		If operation successfull

	_hzfunc("hzTcpClient::Recv(2)") ;

	int32_t		nRecv ;		//	Bytes read by recv() or SSL_read()

	if (!m_nSock)
		return E_NOSOCKET ;

	for (;;)
	{
		if (m_pSSL)
			nRecv = SSL_read(m_pSSL, m_Buf, HZ_MAXPACKET) ;
		else
			nRecv = recv(m_nSock, m_Buf, HZ_MAXPACKET, 0) ;

		if (!nRecv)
			break ;

		if (nRecv < 0)
		{
			if (errno == EAGAIN)
				return E_TIMEOUT ;
			if (errno == ETIMEDOUT)
				return E_TIMEOUT ;

			Close() ;
			return E_RECVFAIL ;
		}

		Z.Append(m_Buf, nRecv) ;
	}

	return E_OK ;
}

void	hzTcpClient::Close	(void)
{
	//	Closes the client TCP connection. If there is an SSL connection, this is shutdown and then the handle for the SSL is freed.
	//
	//	Arguments:	None
	//	Returns:	None

	if (m_pSSL)
	{
		SSL_shutdown(m_pSSL) ;
		SSL_free(m_pSSL) ;
		m_pSSL = 0 ;
	}

	if (m_nSock)
		close(m_nSock) ;
	m_nSock = 0 ;
}
