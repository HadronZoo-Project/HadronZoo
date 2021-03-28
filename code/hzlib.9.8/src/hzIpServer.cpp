//
//	File:	hzIpServer.cpp
//
//	Legal Notice: This file is part of the HadronZoo C++ Class Library.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	The HadronZoo C++ Class Library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//	FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License along with the HadronZoo C++ Class Library. If not, see http://www.gnu.org/licenses.
//

//
//	Implementation of HadronZoo server and client socket classes
//

#include <cstdio>
#include <fstream>
#include <iostream>

#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/epoll.h>
#include <signal.h>
#include <pthread.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzHttpServer.h"
#include "hzIpServer.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Variables
*/

global	hzMapS	<hzIpaddr,hzIpinfo>	_hzGlobal_StatusIP ;	//	Black and white listed IP addresses

global	hzString	_hzGlobal_sslPvtKey ;					//	SSL Private Key
global	hzString	_hzGlobal_sslCert ;						//	SSL Certificate
global	hzString	_hzGlobal_sslCertCA ;					//	SSL CA Certificate

global	hzString	_hzGlobal_Hostname ;					//	This server hostname
global	hzString	_hzGlobal_HostIP ;						//	This server IP address
global	hzIpaddr	_hzGlobal_livehost ;					//	This server IP address (copy)

static	hzIpServer*			s_pTheOneAndOnlyServer = 0 ;	//	The singleton server instance
static	SSL_CTX*			s_svrCTX = 0 ;					//	SSL CTX structure
static	const SSL_METHOD*	s_svrMeth = 0 ;					//	SSL method

static	int32_t	(*verify_callback)(int, X509_STORE_CTX*) ;	//	SSL callback function

static	hzPacket*	s_pTcpBuffer_freelist = 0 ;				//	Freelist of IP packet holders

static	ofstream	s_status_ip_os ;		//	Black and white list stream

static	hzString	s_status_ip_fname ;		//	Blacklist file path

static	hzString	s_str_hangup = "EPOLL HANGUP" ;			//	Epoll hangup error message
static	hzString	s_str_error = "EPOLL ERROR" ;			//	Epoll general error message
static	int32_t		epollSocket ;							//	The epoll 'master' socket

/*
**	System Init Functions
*/

hzEcode	SetupHost   (void)
{
	//	Category:	Internet Server
	//
	//	Determine the hostname of this server. This is called as part of the initialization sequence of a server application, if not also a client application.
	//	It should generally not be called mid way though. The hostname could change during runtime, but in a live server environment proving an online service,
	//	performing such a change is absurd.
	//
	//	The function calls the standard functions gethostname() and gethostbyname(). The former to establish the hostname and set _hzGlobal_Hostname. The latter
	//	to establish the physical server IP address and set _hzGlobal_HostIP.
	//
	//	Although it is intended that this function be called as part of program initialization, there is still a need to take account of the condition in which
	//	the DNS advises a retry (h_errno set to DNS_TRY_AGAIN). This is because it should not be assumed the program is to be started manually. This function
	//	will repeat the call to gethostbyname() for a maximum of 10 times before failing.
	//
	//	The application should check that the hostname is not set to localhost if it is to provide an online service.
	//
	//	Arguments:	None. The function sets two global parameters, both of type
	//				hzString, namely _hzGlobal_Hostname and _hzGlobal_HostIP
	//
	//	Returns:	E_NODATA	If the call to gethostname fails
	//				E_NOTFOUND	If the call to gethostbyname does not return a host
	//				E_DNS_RETRY	If the number of retries reaches 10
	//				E_OK		If the hostname established

	_hzfunc("SetupHost") ;

	struct hostent*		pHost ;		//  Host entry
	struct sockaddr_in  SvrAddr ;   //  Server address

	uint32_t	nTries ;			//	Number of DNS tries
	char		buf [256] ;			//  Buffer for gethostname function

	if (gethostname(buf, 256))
		return hzerr(_fn, HZ_ERROR, E_NODATA, "System call 'gethostname' failed. Cannot establish hostname") ;

	_hzGlobal_Hostname = buf ;

	for (nTries = 0 ; nTries < 10 ; nTries++)
	{
		pHost = gethostbyname(*_hzGlobal_Hostname) ;

		if (pHost)
			break ;

		if (h_errno == TRY_AGAIN)
			continue ;
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Could not lookup the hostname") ;
	}

	if (nTries == 10)
		return hzerr(_fn, HZ_ERROR, E_DNS_RETRY, "Could not lookup the hostname") ;

	memset(&SvrAddr, 0, sizeof(SvrAddr)) ;
	SvrAddr.sin_family = AF_INET ;
	memcpy(&SvrAddr.sin_addr, pHost->h_addr, pHost->h_length) ;
	_hzGlobal_HostIP = inet_ntoa(SvrAddr.sin_addr) ;
	_hzGlobal_livehost = _hzGlobal_HostIP ;

	return E_OK ;
}

hzIpServer*	hzIpServer::GetInstance	(hzLogger* pLogger)
{
	//	Purpose:	Creates a single instance of a hzIpServer with the Singleton method of calling a private constructor.
	//
	//	Arguments:	1)	pLogger	The logger for the server (this is compulsory)
	//
	//	Returns:	Pointer to the hzIpServer instance. The same instance is returned however many times this function is called.
	//				NULL if no logger is provided

	_hzfunc("hzIpServer::GetInstance") ;

	hzIpServer*	pSvr = 0 ;	//	TCP server

	if (!pLogger)
	{
		hzerr(_fn, HZ_ERROR, E_NOINIT, "No logfile supplied") ;
		return 0 ;
	}

	if (s_pTheOneAndOnlyServer)
	{
		//pSvr = dynamic_cast<hzIpServer*>(s_pTheOneAndOnlyServer) ;
		pSvr = s_pTheOneAndOnlyServer ;

		if (!pSvr)
		{
			hzerr(_fn, HZ_ERROR, E_CONFLICT, "Application is attempting to allocate a TCP Server when a UDP Server is already allocated") ;
			return 0 ;
		}

		hzerr(_fn, HZ_ERROR, E_CONFLICT, "Application is attempting to allocate more than one server instance") ;
	}
	else
	{
		pSvr = new hzIpServer() ;
		pSvr->m_pLog = pLogger ;
		s_pTheOneAndOnlyServer = pSvr ;
	}

	return pSvr ;
}

/*
**	Packet Allocation Functions
*/

hzPacket*	_tcpbufAlloc	(void)
{
	//	Allocate a hzPacket instance to service a TCP client connection. Allocation is from a freelist regime.
	//
	//	Arguments:	None
	//	Returns:	Pointer to a hzPacket instance

	hzPacket*	pTB ;	//	Target hzPacket pointer

	if (!s_pTcpBuffer_freelist)
		pTB = new hzPacket() ;
	else
	{
		pTB = s_pTcpBuffer_freelist ;
		s_pTcpBuffer_freelist = s_pTcpBuffer_freelist->next ;

		pTB->next = 0 ;
		pTB->m_size = 0 ;
	}
	return pTB ;
}

void	_tcpbufDeleteOne	(hzPacket* pTB)
{
	//	Delete a single hzPacket instance after the data held in it has been successfully sent to the client.
	//
	//	Arguments:	1)	pTB	Pointer to hzPacket instance
	//
	//	Returns:	None

	pTB->next = s_pTcpBuffer_freelist ;
	s_pTcpBuffer_freelist = pTB ;
}

void	_tcpbufDeleteAll	(hzPacket* first, hzPacket* last)
{
	//	Delete a series of hzPacket instances. This is called when the outgoing message to a connected client is complete or has been abandoned.
	//
	//	Arguments:	1)	first	Start of series
	//				2)	last	End of series
	//
	//	Returns:	None

	last->next = s_pTcpBuffer_freelist ;
	s_pTcpBuffer_freelist = first ;
}

int32_t	_hz_SSL_verify	(int32_t x, X509_STORE_CTX* pTax)
{
	return 1 ;
}

hzEcode	InitServerSSL	(const char* pvtKey, const char* sslCert, const char* sslCA, bool bVerifyClient)
{
	//	Category:	Internet Server
	//
	//	Initializes SSL and sets up full paths for the .pem, .key and .cert files. Please note that the dissemino web interface usually sets up the global cert
	//	values in the application config files. If this is the prectice, call this function AFTER the config files have been read and with all three cert values
	//	as NULL.
	//
	//	Arguments:	1)	pvtKey			Server Private Key
	//				2)	sslCert			Server Certificate
	//				3)	sslCA			Server CA Certificate
	//				4)	bVerifyClient	Flag
	//
	//	Returns:	E_INITDUP	If SSL has already been initialized
	//				E_ARGUMENT	If either the private key, the server SSL certificate or the certificate authority are not supplied
	//				E_INITFAIL	If any of the SSL functions return an error
	//				E_OK		If the SSL is set up OK

	_hzfunc("InitServerSSL") ;

	FILE*		sslErrors ;		//	SSL error log file
	int32_t		sys_rc ;		//	System return value

	if (s_svrCTX)
		return hzerr(_fn, HZ_ERROR, E_INITDUP, "SSL Init called already") ;

	if (!_hzGlobal_sslPvtKey)
		_hzGlobal_sslPvtKey = pvtKey ;

	if (!_hzGlobal_sslCert)
		_hzGlobal_sslCert = sslCert ;

	if (!_hzGlobal_sslCertCA)
		_hzGlobal_sslCertCA = sslCA ;

	if (!_hzGlobal_sslPvtKey || !_hzGlobal_sslCert || !_hzGlobal_sslCertCA)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "SSL params missing [PvtKey=%s][Cert=%s][CertCA=%s]", *_hzGlobal_sslPvtKey, *_hzGlobal_sslCert, *_hzGlobal_sslCertCA) ;

	sslErrors = fopen("ssl_errors.txt", "a") ;

	sys_rc = SSL_library_init() ;
	threadLog("%s Returned from SSL_library_init with %d\n", *_fn, sys_rc) ;
    SSL_load_error_strings();	

    sys_rc = OpenSSL_add_ssl_algorithms();
	threadLog("%s Returned from OpenSSL_add_ssl_algorithms with %d\n", *_fn, sys_rc) ;

	s_svrMeth = SSLv23_server_method() ;
	threadLog("%s Returned from SSLv23_server_method errno %d\n", *_fn, errno) ;

	s_svrCTX = SSL_CTX_new(s_svrMeth) ;
	threadLog("%s Returned from SSL_CTX_new errno %d\n", *_fn, errno) ;

	//	Load server certificate into the SSL context
	sys_rc = SSL_CTX_use_certificate_file(s_svrCTX, *_hzGlobal_sslCert, SSL_FILETYPE_PEM) ;
	threadLog("%s Returned from SSL_CTX_use_certificate_file with %d\n", *_fn, sys_rc) ;
	if (sys_rc <= 0)
	{
		ERR_print_errors_fp(sslErrors);
		fclose(sslErrors) ;
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "No SSL certificate. File %s Error %d", *_hzGlobal_sslCert, sys_rc) ;
	}
 
	//	Load the server private-key into the SSL context
	sys_rc = SSL_CTX_use_PrivateKey_file(s_svrCTX, *_hzGlobal_sslPvtKey, SSL_FILETYPE_PEM) ;
	threadLog("%s Returned from SSL_CTX_use_PrivateKey_file with %d\n", *_fn, sys_rc) ;
	if (sys_rc <= 0)
	{
		/*
		ERR_print_errors_fp(sslErrors);
		fclose(sslErrors) ;
		*/
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "No SSL private key. File %s Error %d", *_hzGlobal_sslPvtKey, sys_rc) ;
	}

	//	Check if the server certificate and private-key matches
	if (!SSL_CTX_check_private_key(s_svrCTX))
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Private key does not match the certificate public key") ;

	if (bVerifyClient)
	{
		//  Load the RSA CA certificate into the SSL_CTX structure
		sys_rc = SSL_CTX_load_verify_locations(s_svrCTX, *_hzGlobal_sslCertCA, 0) ;
		if (!sys_rc)
		{
			/*
			ERR_print_errors_fp(sslErrors);
			fclose(sslErrors) ;
			*/
			return hzerr(_fn, HZ_ERROR, E_INITFAIL, "No SSL verifications. File %s Error %d", *_hzGlobal_sslCertCA, sys_rc) ;
		}

		//	Set to require peer (client) certificate verification
		SSL_CTX_set_verify(s_svrCTX, SSL_VERIFY_PEER, 0);

		//	Set the verification depth to 1
		SSL_CTX_set_verify_depth(s_svrCTX,1);

		verify_callback = _hz_SSL_verify ;
	}
 
	//	Load trusted CA.
	sys_rc = SSL_CTX_load_verify_locations(s_svrCTX, *_hzGlobal_sslCertCA, 0) ;
	threadLog("%s Returned from SSL_CTX_load_verify_locations with %d\n", *_fn, sys_rc) ;
 
	//	Set to require peer (client) certificate verification
	SSL_CTX_set_verify(s_svrCTX, SSL_VERIFY_PEER, _hz_SSL_verify);
	threadLog("%s Returned from SSL_CTX_set_verify (void)\n", *_fn) ;

	//	Set the verification depth to 1
	SSL_CTX_set_verify_depth(s_svrCTX, 1);
	threadLog("%s Returned from SSL_CTX_set_verify_depth (void)\n", *_fn) ;

	return E_OK ;
}

hzTcpCode	HandleHttpMsg	(hzChain& Input, hzIpConnex* pCx)
{
	//	Category:	Internet Server
	//
	//	While no assumptions can be made about incoming messages within the hzIpServer regime itself, for HTTP it is expedient to ensure that messages (HTTP requests), are complete
	//	before being passed to the applicable callback function (by default hdsApp::ProcHTTP).
	//
	//	This is acheived by creating a hzHttpEvent instance for the connection and calling hzHttpEvent::ProcessEvent on the data recieved so far. If this sets a flag indicating the
	//	message does constitute a complete HTTP request, then the callback function is invoked.
	//
	//	Arguments:	1)	Input	The input chain (maintained by the hzIpServer instance)
	//				2)	pCx		The TCP connection the message is being received on
	//
	//	Returns:	A hzTcpCode to indicate keep-alive or terminate.
	//
	//	Notes:		This function is itself a callback function that is called every time a packet arrives on a port deemed to be using
	//				the HTTP protocol. It is defined as static to prevent it being called directly by the application.

	_hzfunc("HandleHttpMsg") ;

	hzTcpCode	(*fnOnHttpEvent)(hzHttpEvent*) ;	//	HTTP event handle

	hzHttpEvent*	pE ;		//	The HTTP event message
	hzTcpCode		tcp_rc ;	//	TCP return code


	if (!pCx->m_pEventHdl)
		hzexit(_fn, 0, E_MEMORY, "Could not allocate a HTTP Event") ;

	pE = (hzHttpEvent*) pCx->m_pEventHdl ;

	//	Know message complete - don't keep the pointer
	if (pCx->MsgComplete())
		pCx->m_pEventHdl = 0 ;

	//	First process message to see if it is complete. This also populates the hzHttpEvent instance
	if (pE->ProcessEvent(Input) != E_OK)
	{
		if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
			pCx->GetLogger()->Out("%s. case 1 About to delete the HTTP event input\n", *_fn) ;
		Input.Clear() ;
		return TCP_TERMINATE ;
	}

	if (!pE->MsgComplete())
	{
		//	Know message incomplete - set the pointer
		pCx->m_pEventHdl = pE ;
		pCx->ExpectSize(pE->ExpectSize()) ;
		return TCP_INCOMPLETE ;
	}

	//	Set the connection state to HZCONNEX_PROC
	//	pCx->SetState(*_fn, HZCONNEX_PROC) ;

	//	Call the applcation's function
	if (pCx->m_appFn)
	{
		fnOnHttpEvent = (hzTcpCode(*)(hzHttpEvent*)) pCx->m_appFn ;
		tcp_rc = fnOnHttpEvent(pE) ;
	}

	if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
		pCx->GetLogger()->Out("%s. case 2 About to delete the HTTP event input\n", *_fn) ;
	Input.Clear() ;
	return tcp_rc ;
}

hzEcode	InitIpInfo	(const hzString& dataDir)
{
	//	Category:	Internet Server
	//
	//	Reads in BOTH the blacklist AND whitelist file

	_hzfunc(__func__) ;

	ifstream	is ;			//	For reading the lists
	hzIpinfo	ipi ;			//	IP block info
	hzIpaddr	ipa ;			//	IP address
	uint32_t	nLine ;			//	Line number
	char		buf	[24] ;		//	Line buffer
	hzEcode		rc = E_OK ;		//	Return code

	if (!dataDir)
		return E_ARGUMENT ;

	rc = AssertDir(dataDir, 0666) ;
	if (rc != E_OK)
		return rc ;

	s_status_ip_fname = dataDir + "/status_ip.ips" ;

	rc = TestFile(s_status_ip_fname) ;
	if (rc == E_OK)
	{
		is.open(*s_status_ip_fname) ;
		if (is.fail())
			return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot open file %s\n", *s_status_ip_fname) ;

		for (nLine = 1 ;; nLine++)
		{
			is.getline(buf, 23) ;
			if (!is.gcount())
				break ;
			ipa = buf ;
			if (!ipa)
				threadLog("%s: Line %d of file %s, is not a valid IP address\n", *_fn, nLine, *s_status_ip_fname) ;
			else
				_hzGlobal_StatusIP.Insert(ipa, ipi) ;
		}
		is.close() ;
		is.clear() ;
		threadLog("%s: Loaded %d IP address in WHITELIST\n", *_fn, _hzGlobal_StatusIP.Count()) ;
	}
	else
	{
		if (rc != E_NOTFOUND)
			return hzerr(_fn, HZ_ERROR, rc, "File error %s\n", *s_status_ip_fname) ;
	}

	rc = E_OK ;
	s_status_ip_os.open(*s_status_ip_fname, ios::app) ;
	if (s_status_ip_os.fail())
		{ rc = E_OPENFAIL ; threadLog("%s: Cannot open %s for writing\n", *_fn, *s_status_ip_fname) ; }

	return rc ;
}

void	SetStatusIP	(hzIpaddr ipa, hzIpStatus reason, uint32_t nDelay)
{
	//	Category:	Internet Server
	//
	//	Set reason for noting the IP address. Add the IP address to _hzGlobal_StatusIP if not already present.

	_hzfunc(__func__) ;

	hzIpinfo	ipi ;		//	IP test blocker

	if (ipa == IPADDR_BAD || ipa == IPADDR_NULL || ipa == IPADDR_LOCAL)
		return ;

	//threadLog("%s: Setting Status of IP %s\n", *_fn, *ipa.Str()) ;

	if (!_hzGlobal_StatusIP.Exists(ipa))
	{
		s_status_ip_os << ipa.Str() << "\n" ;
		if (!s_status_ip_os.fail())
			//threadLog("BLOCKED IP ADDR NOT RECORDED %s\n", *ipa.Str()) ;
		//else
			s_status_ip_os.flush() ;

		ipi.m_nSince = 1 ;
		ipi.m_nTotal = 1 ;
		ipi.m_bInfo |= (reason & 0xff) ;

		if (reason & HZ_IPSTATUS_BLACK)
			ipi.m_tBlack = time(0) ;
		if (reason & HZ_IPSTATUS_WHITE)
			ipi.m_tWhite = time(0) ;
		_hzGlobal_StatusIP.Insert(ipa, ipi) ;
			//note.Printf("BLOCKED IP ADDR ADDED TO LOCAL %s (count %u)\n", *ipa.Str(), _hzGlobal_StatusIP.Count()) ;
	}
	else
	{
		ipi = _hzGlobal_StatusIP[ipa] ;

		//	If status has changed, note this in the file
		if ((ipi.m_bInfo | reason) != ipi.m_bInfo)
		{
			ipi.m_bInfo |= reason ;
			if (reason & HZ_IPSTATUS_BLACK)
				ipi.m_tBlack = nDelay ? nDelay + time(0) : 0 ;
			if (reason & HZ_IPSTATUS_WHITE)
				ipi.m_tWhite = nDelay ? nDelay + time(0) : 0 ;
		}

		s_status_ip_os << ipa.Str() << "\n" ;
		if (s_status_ip_os.fail())
			threadLog("%s: BLOCKED IP ADDR NOT RECORDED %s\n", *_fn, *ipa.Str()) ;
		else
			s_status_ip_os.flush() ;
	}

	//note.Printf("IP ADDR BLOCKED %s %u times\n", *ipa.Str(), _hzGlobal_Blacklist[ipa].m_Count++) ;
	//_hzGlobal_Blacklist[IpTest].m_Count++ ;
}

hzIpStatus	GetStatusIP	(hzIpaddr ipa)
{
	//	Category:	Internet Server
	//
	//	Retrieve the record for the IP address.

	_hzfunc(__func__) ;

	hzIpinfo	ipi ;		//	IP test blocker
	uint32_t	now ;		//	Epoch time
	uint32_t	ips ;		//	IP status as it currently applies

	ips = (uint32_t) HZ_IPSTATUS_NULL ;
	if (!_hzGlobal_StatusIP.Exists(ipa))
		return HZ_IPSTATUS_NULL ;

	//	We have info on the IP address, but does it still apply?
	now = time(0) ;
	ipi = _hzGlobal_StatusIP[ipa] ;

	if (ipi.m_bInfo & HZ_IPSTATUS_WHITE && (!ipi.m_tWhite || ipi.m_tWhite > now))
		ips |= HZ_IPSTATUS_WHITE ;

	if (ipi.m_bInfo & HZ_IPSTATUS_BLACK && (!ipi.m_tWhite || ipi.m_tBlack > now))
		ips |= HZ_IPSTATUS_BLACK ;

	return (hzIpStatus) ips ;
}

#if 0
#endif

#if 0
#endif

/*
**	The hzTcpListen class members
*/

hzEcode	hzTcpListen::Init	(	hzTcpCode	(*OnIngress)(hzChain&, hzIpConnex*),
								hzTcpCode	(*OnConnect)(hzIpConnex*),
								hzTcpCode	(*OnDisconn)(hzIpConnex*),
								void*		(*OnSession)(void*),
								uint32_t	nTimeout,
								uint32_t	nPort,
								uint32_t	nMaxConnections,
								uint32_t	bOpflags	)
{
	//	Purpose:	Initialize a listening socket.
	//
	//	Arguments:	1)	OnIngress		The Application Specific 'OnIngress' function - called each time client sends in data.
	//				2)	OnConnect		The Application Specific 'Server Hello' function - only where protocol requires server to speak first.
	//				3)	OnSession		Application Specific Session Handler function - only where client sessions are handled in a separate thread.
	//				4)	nTimeout		Timeout in seconds.
	//				5)	nPort			The port number to listen on.
	//				6)	nMaxConnections	Max number of simulataneous connections to this port.
	//				7)	bSecure			Use SSL
	//
	//	Returns:	E_NOINIT	If the SSL server method is not set up (SSL applications only)
	//				E_OK		If the listening socket is set up.

	_hzfunc("hzTcpListen::Init") ;

	if (OnIngress)	m_OnIngress = OnIngress ;
	if (OnConnect)	m_OnConnect = OnConnect ;
	if (OnDisconn)	m_OnDisconn = OnDisconn ;
	if (OnSession)	m_OnSession = OnSession ;

	m_nTimeout = nTimeout ;
	m_nPort = nPort ;
	m_nMaxConnections = nMaxConnections ;

	if (bOpflags & HZ_LISTEN_SECURE)
	{
		if (!s_svrMeth)
			return hzerr(_fn, HZ_ERROR, E_NOINIT, "No SSL initialization") ;
		m_pSSL = SSL_new(s_svrCTX) ;
	}

	m_bOpflags = bOpflags ;

	return E_OK ;
}

#if 0
#endif

hzEcode	hzTcpListen::Activate	(void)
{
	//	Purpose:	Acivate listening socket. This creates a sock and then binds it to a port.
	//
	//	Arguments:	None.
	//
	//	Returns:	E_NOSOCKET	If listening socket bound to port,
	//				E_OK		If operation successful

	_hzfunc("hzTcpListen::Activate") ;

	uint32_t	nTries ;		//	Number of attempts to bind (give up after 5)
	int32_t		sys_rc ;		//	Return from bind call

	//	Allocate and bind a listening socket
	if (m_bOpflags & HZ_LISTEN_UDP)
	{
		if ((m_nSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Could not create UDP server socket (errno=%d)", errno) ;
	}
	else
	{
		if ((m_nSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
			return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Could not create TCP server socket (errno=%d)", errno) ;
	}

	memset(&m_Address, 0, sizeof(SOCKADDRIN)) ;
	m_Address.sin_family = AF_INET ;
	m_Address.sin_addr.s_addr = htonl(INADDR_ANY) ;
	m_Address.sin_port = htons(m_nPort) ;

	for (nTries = 1 ; nTries <= 5 ; nTries++)
	{
		sys_rc = bind(m_nSocket, (SOCKADDR*) &m_Address, sizeof(m_Address)) ;

		if (sys_rc < 0)
		{
			hzerr(_fn, HZ_WARNING, E_NOSOCKET, "Server could not bind to port %d (attempt %d)", m_nPort, nTries) ;
			if (close(m_nSocket) < 0)
				return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not close socket %d (port %d) after bind_fail. Errno=%d", m_nSocket, m_nPort, errno) ;

			if (nTries == 5)
				return E_NOSOCKET ;

			sleep(5) ;
			continue ;
		}
		break ;
	}


	/*
	if (bind(m_nSocket, (SOCKADDR*) &m_Address, sizeof(m_Address)) < 0)
	{
		if (close(m_nSocket) < 0)
			hzerr(_fn, HZ_WARNING, E_CORRUPT, "Could not close socket %d after bind_fail. Errno=%d", m_nSocket, errno) ;
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Server could not bind to port %d", m_nPort) ;
	}
	*/

	if (!(m_bOpflags & HZ_LISTEN_UDP))
	{
		if (listen(m_nSocket, 5) < 0)
		{
			hzerr(_fn, HZ_WARNING, E_NOSOCKET, "Server listening socket (sock %d) is not listening on port %d. Errno=%d", m_nSocket, m_nPort, errno) ;
			if (close(m_nSocket) < 0)
				hzerr(_fn, HZ_WARNING, E_CORRUPT, "Could not close socket %d after listen_fail. Errno=%d", m_nSocket, errno) ;

			return hzerr(_fn, HZ_ERROR, E_NOSOCKET) ;
		}
	}

	return E_OK ;
}

/*
**	hzIpConnex members
*/

hzIpConnex::hzIpConnex	(hzLogger* pLog)
{
	m_ConnExpires = m_nsAccepted = m_nsRecvBeg = m_nsRecvEnd = m_nsSendBeg = m_nsSendEnd = RealtimeNano() ;
	m_pInfo = 0 ;
	m_nSock = 0 ;
	m_nPort = 0 ;
	m_nMsgno = 0 ;
	m_nGlitch = 0 ;
	m_nStart = 0 ;
	m_nExpected = 0 ;
	m_pEventHdl = 0 ;
	m_pLog = pLog ;
	m_pSSL = 0 ;
	m_bState = CLIENT_STATE_NONE ;
	memset(m_ipbuf, 0, 44) ;
}

hzIpConnex::~hzIpConnex	(void)
{
	if (m_nSock)
	{
		if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
			threadLog("NOTE: Connection under Destruction still has a socket %d\n", m_nSock) ;
		close(m_nSock) ;
		m_nSock = 0 ;
	}

	if (m_pSSL)			SSL_free(m_pSSL) ;
	if (m_pInfo)		delete m_pInfo ;
	if (m_pEventHdl)	delete (hzHttpEvent*) m_pEventHdl ;
}

hzEcode	hzIpConnex::Initialize	(hzTcpListen* pLS, SSL* pSSL, const char* cpIPAddr, uint32_t cliSock, uint32_t cliPort, uint32_t eventNo)
{
	//	Initialize a TCP client connection (hzIpConnex instance). This is called from within the select and the epoll server functions and in general, server
	//	programs based on HadronZoo would use one of the server functions and so have no need to call this function directly.
	//
	//	Arguments:	1)	Sock	The socket file descriptor
	//				2)	nEvent	The event number (usually Nth connection)
	//				3)	pLS		Pointer to the listening socket (hzTcpListen instance)
	//				4)	pSSL	The SSL connection (if applicable)
	//				5)	cpIpa	The text representation of the client IP address
	//
	//	Returns:	E_ARGUMENT	If either the socket or the pointer to the listening socket is not supplied
	//				E_DUPLICATE	If the connection has already got a socket
	//				E_OK		If the TCP connection is initialized

	_hzfunc("hzIpConnex::Initialize") ;

	if (m_nSock)
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Attempt to init connection to socket %d - already has socket %d", cliSock, m_nSock) ;
	if (!pLS)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No listening socket class instance provided") ;

	//	SetState(*_fn, HZCONNEX_ACPT) ;
	m_nsAccepted = RealtimeNano() ;

	m_nSock = cliSock ;
	m_nPort = cliPort ;
	m_nMsgno = eventNo ;

	m_OnIngress = pLS->m_OnIngress ;
	m_OnConnect = pLS->m_OnConnect ;
	m_appFn = pLS->m_appFn ;
	m_pSSL = pSSL ;
	m_pLog = pLS->GetLogger() ;

	strcpy(m_ipbuf, cpIPAddr) ;
	m_ClientIP = cpIPAddr ;
	Oxygen() ;
	m_bState = CLIENT_INITIALIZED ;

	if (pLS->Opflags() & HZ_LISTEN_HTTP)
		m_pEventHdl = (void*) new hzHttpEvent(m_Input, this) ;
	return E_OK ;
}

void	hzIpConnex::Terminate	(void)
{
	//	This is called within ServeEpollST or ServeEpollMT in the event of an epoll error being detected. The origin of the epoll error would be such
	//	as connection reset by peer, or a broken pipe.
	//
	//	The remedial action is to remove any remaining outgoing data as this has no chance of being delivered to the client. However the socket itself
	//	is not closed immeadiately. This is left open for a limited period so that it is not assigned to another connection.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzIpConnex::Terminate") ;

	struct epoll_event	epEv ;		//	Epoll event for depricated connection
	hzXDate				now ;		//	Time now

	m_Input.Clear() ;
	m_nExpected = 0 ;
	if (m_Outgoing.Size())
		m_Outgoing.Clear() ;

	if (!m_nSock)
		m_Track.Printf("%s: No socket - Has Terminate already been called?\n", *_fn) ;
	else
	{
		if (epoll_ctl(epollSocket, EPOLL_CTL_DEL, m_nSock, &epEv) < 0)
			m_Track.Printf("%s: EPOLL ERROR: Could not del client connection handler on sock %d/%d. Error=%s\n", *_fn, m_nSock, m_nPort, strerror(errno)) ;

		if (close(m_nSock) < 0)
			m_Track.Printf("%s: Could not close socket %d for event %d on IP %s. Errno=%s", *_fn, m_nSock, m_nMsgno, m_ipbuf, strerror(errno)) ;
		else
			m_Track.Printf("%s: Socket %d closed\n", *_fn, m_nSock) ;

		m_nSock = 0 ;
	}

	if (m_Track.Size())
	{
		now.SysDateTime() ;

		m_Track.Printf("Terminated at %s\n\n", *now.Str(FMT_TIME_USEC)) ;
		m_pLog->Out("\n-- Start -- \n") ;
		m_pLog->Out(m_Track) ;
	}
}

hzEcode	hzPktQue::Push	(const hzChain& Z)
{
	//	Send a response to the client. This function should always be called by applications rather than writing to the client socket directly as this
	//	ensures proper multiplexing between clients.
	//
	//	Arguments:	1)	Z	Output chain. Response to be sent to the client.
	//
	//	Returns:	E_SENDFAIL	If the connection is in an error state
	//				E_OK		If supplied hzChain is accepted for the response,

	_hzfunc("hzPktQue::Push") ;

	hzChain::BlkIter	bi ;	//	Block iterator

	//uint32_t	sofar = 0 ;		//	Bytes so far
	hzEcode		rc ;			//	Return code

	//	if (m_eState == HZCONNEX_FAIL)
	//		return E_SENDFAIL ;

	//m_nSize = Z.Size() ;

	if (Z.Size())
	{
		for (bi = Z ; bi.Data() ; bi.Advance())
		{
			if (!m_pFinal)
				m_pFinal = m_pStart = _tcpbufAlloc() ;
			else
			{
				m_pFinal->next = _tcpbufAlloc() ;
				m_pFinal = m_pFinal->next ;
			}
			//	m_pFinal->m_msgId = m_nMsgno ;

			memcpy(m_pFinal->m_data, bi.Data(), bi.Size()) ;
			m_pFinal->m_size = bi.Size() ;
			m_nSize += bi.Size() ;
			//sofar += m_pFinal->m_size ;
			m_pFinal->m_seq = ++m_nSeq ;
		}
	}

	//	SetState(*_fn, HZCONNEX_XMIT) ;
	return E_OK ;
}

hzEcode	hzPktQue::Pull	(void)
{
	//	Remove the first block in the packet queue

	_hzfunc("hzPktQue::Pull") ;

	hzPacket*	pkt ;		//	Temp packet pointer

	pkt = m_pStart ;
	if (pkt)
	{
		m_nSize -= pkt->m_size ;
		m_pStart = m_pStart->next ;
		_tcpbufDeleteOne(pkt) ;

		if (!m_pStart)
		{
			m_pFinal = 0 ;
			m_nSize = 0 ;
		}
	}
}

hzEcode	hzPktQue::Clear	(void)
{
	//	Remove all packets in packet queue

	_hzfunc("hzPktQue::Clear") ;

	if (m_pFinal)
	{
		m_pFinal->next = s_pTcpBuffer_freelist ;
		s_pTcpBuffer_freelist = m_pStart ;
	}
	m_pStart = m_pFinal = 0 ;
	m_nSize = 0 ;
}

hzEcode	hzIpConnex::SendData	(const hzChain& Hdr, const hzChain& Body)
{
	//	Send a response to the client as a separate header and message body. This approach means a cost of two partial IP packets instead of one, but
	//	it saves having to concatenate the two chains which is a heavy copying operation.
	//
	//	Note that this function should always be called by applications rather than writing to the client socket directly as this ensures proper multiplexing
	//	between clients.
	//
	//	Arguments:	1)	Hdr		Header of outgoing response
	//				2)	Body	Body of outgoing response
	//
	//	Returns:	E_OK if hzChain is accepted for the response,
	//				False otherwise.

	_hzfunc("hzIpConnex::SendData(1)") ;

	struct epoll_event	epEventNew ;		//	Epoll event for new connections

	if (!Hdr.Size() && !Body.Size())
	{
		m_Track.Printf("%s: No data queued for output\n", *_fn) ;
		return E_NODATA ;
	}

	if (Hdr.Size() && Body.Size())
	{
		m_Outgoing.Push(Hdr) ;
		m_Outgoing.Push(Body) ;
	}

	m_nTotalOut = Hdr.Size() + Body.Size() ;

	m_nsSendBeg = RealtimeNano() ;

	epEventNew.data.fd = m_nSock ;
	epEventNew.events = EPOLLIN | EPOLLOUT | EPOLLET ;

	if (epoll_ctl(epollSocket, EPOLL_CTL_MOD, m_nSock, &epEventNew) < 0)
	{
		m_Track.Printf("%s: EPOLL ERROR: Could not add client connection write handler on sock %d/%d. Error=%s\n", *_fn, m_nSock, m_nPort, strerror(errno)) ;
		if (close(m_nSock) < 0)
			m_Track.Printf("%s: NOTE: Could not close socket %d after epoll error. errno=%d\n", *_fn, m_nSock, errno) ;
	}

	return E_OK ;
}

hzEcode	hzIpConnex::SendData	(const hzChain& Z)
{
	//	Send a response to the client. This function should always be called by applications rather than writing to the client socket directly as this
	//	ensures proper multiplexing between clients.
	//
	//	Arguments:	1)	Z	Output chain. Response to be sent to the client.
	//
	//	Returns:	E_SENDFAIL	If the connection is in an error state
	//				E_OK		If supplied hzChain is accepted for the response,

	_hzfunc("hzIpConnex::SendData(2)") ;

	struct epoll_event	epEventNew ;		//	Epoll event for new connections

	m_nTotalOut = Z.Size() ;

	if (!Z.Size())
	{
		m_Track.Printf("%s: No data queued for output\n", *_fn) ;
		return E_NODATA ;
	}

	if (Z.Size())
		m_Outgoing.Push(Z) ;

	m_nsSendBeg = RealtimeNano() ;

	epEventNew.data.fd = m_nSock ;
	epEventNew.events = EPOLLIN | EPOLLOUT | EPOLLET ;

	if (epoll_ctl(epollSocket, EPOLL_CTL_MOD, m_nSock, &epEventNew) < 0)
	{
		m_Track.Printf("%s: EPOLL ERROR: Could not add client connection write handler on sock %d/%d. Error=%s\n", *_fn, m_nSock, m_nPort, strerror(errno)) ;
		if (close(m_nSock) < 0)
			m_Track.Printf("%s: NOTE: Could not close socket %d after epoll error. errno=%d\n", *_fn, m_nSock, errno) ;
	}

	return E_OK ;
}

void	hzIpConnex::SendKill	(void)
{
	//	Sent by connection handler in response to illegal message. The status is set to CLIENT_BAD but the socket is still made ready for epoll write events. As soon as the socket
	//	is available for writes however, the connection is terminated and deleted. The client recieves no response to their request.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzIpConnex::SendKill") ;

	struct epoll_event	epEventDead ;		//	Epoll event for depricated connection

	m_Input.Clear() ;
	m_nExpected = 0 ;
	if (m_Outgoing.Size())
		m_Outgoing.Clear() ;

	m_nsSendBeg = RealtimeNano() ;
	m_bState |= CLIENT_BAD ;

	epEventDead.data.fd = m_nSock ;
	epEventDead.events = EPOLLOUT | EPOLLET ;

	if (epoll_ctl(epollSocket, EPOLL_CTL_MOD, m_nSock, &epEventDead) < 0)
		m_Track.Printf("%s: EPOLL ERROR: Could not add client connection write handler on sock %d/%d. Error=%s\n", *_fn, m_nSock, m_nPort, strerror(errno)) ;
	else
		m_Track.Printf("%s: BAD CLIENT: Connection killed by app. Sock %d/%d\n", *_fn, m_nSock, m_nPort) ;

	//	if (close(m_nSock) < 0)
	//		m_Track.Printf("%s: NOTE: Could not close socket %d after epoll error. errno=%d\n", *_fn, m_nSock, errno) ;
}

int32_t	hzIpConnex::Recv	(hzPacket& tbuf)
{
	//	Category:	Internet Server
	//
	//	Receive a packet of data from the client socket and append it to the input chain.
	//
	//	Arguments:	1)	tbuf		The recepticle (buffer) for reading the socket.
	//
	//	Returns:	Total number of bytes read
	//
	//	Scope:		This function should not be called the application!

	_hzfunc("hzIpConnex::Recv") ;

	int32_t		nRecv ;		//	Bytes recieved

	if (!m_Input.Size())
		m_nsRecvBeg = RealtimeNano() ;

	if (m_pSSL)
		nRecv = SSL_read(m_pSSL, tbuf.m_data, HZ_MAXPACKET) ;
	else
		nRecv = recv(m_nSock, tbuf.m_data, HZ_MAXPACKET, 0) ;

	if (!nRecv)
		m_bState |= CLIENT_TERMINATION ;

	if (nRecv > 0)
	{
		m_nTotalIn += nRecv ;
		m_bState |= CLIENT_READING ;
		m_Input.Append(tbuf.m_data, nRecv) ;
	}

	m_nsRecvEnd = RealtimeNano() ;

	return nRecv ;
}

int32_t	hzIpConnex::_xmit	(hzPacket& tbuf)
{
	//	Category:	Internet Server
	//
	//	Writes data to a client socket until either it cannot write (buffer full) or it has exhausted the outgoing message. This function is called in turn for
	//	each connected socket for which there is a write event on the socket (socket becomes available for writing) and output is pending. This approach serves
	//	to multiplex output so that no socket is left hanging during large downloads.
	//
	//	Note: This function is private to the hzIpConnex class so cannot be called directly by an application. It is called only when _isxmit() returns true to
	//	indicate there is outgoing data to transmit.
	//
	//	Arguments:	1)	tbuf	The hzPacket supplied by the caller to contain data to write to the socket.
	//
	//	Returns:	-1		If the write operation failed
	//				>0		If the write operation is delayed (errno either a EAGAIN or WOULDBLOCK)
	//				0		If the write operation completely wrote the outgoing message

	_hzfunc("hzIpConnex::_xmit") ;

	hzPacket*	pTB ;		//	Packet buffer block
	int32_t		nSend ;		//	Bytes to write
	int32_t		nSent ;		//	Bytes actually written

	/*
	**	Write out outgoing packets until exhauted or we get an EWOULDBLOCK
	*/

	for (; m_Outgoing.Size() ;)
	{
		pTB = m_Outgoing.Peek() ;

		nSend = pTB->m_size - m_nGlitch ;

		if (m_pSSL)
			nSent = SSL_write(m_pSSL, pTB->m_data + m_nGlitch, nSend) ;
		else
			nSent = write(m_nSock, pTB->m_data + m_nGlitch, nSend) ;

		if (nSent < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				m_Track.Printf("%s: WBLOCK: Client %d IP %s Event %d Sock %d/%d TOTAL OUT %d (pkt %d posn %d)\n",
					*_fn, m_nMsgno, *m_ClientIP.Str(), pTB->m_msgId, m_nSock, m_nPort, m_nTotalOut, pTB->m_size, pTB->m_seq) ;
				return errno ;
			}

			m_Track.Printf("%s: FAILED: Client %d IP %s Event %d Sock %d/%d TOTAL OUT %d (pkt %d posn %d) Error=%s\n",
				*_fn, m_nMsgno, *m_ClientIP.Str(), pTB->m_msgId, m_nSock, m_nPort, m_nTotalOut, pTB->m_size, pTB->m_seq, strerror(errno)) ;
			return -1 ;
		}

		if (nSent != nSend)
		{
			m_nGlitch += nSent ;
			m_Track.Printf("%s: GLITCH: Client %d IP %s Event %d Sock %d/%d Sent only %d of %d: TOTAL OUT %d (glitch %d pkt %d posn %d)\n",
				*_fn, m_nMsgno, *m_ClientIP.Str(), pTB->m_msgId, m_nSock, m_nPort, nSent, nSend, m_nTotalOut, m_nGlitch, pTB->m_size, pTB->m_seq) ;
			return 1 ;
		}

		m_nGlitch = 0 ;

		if (!pTB->next)
		{
			m_nsSendEnd = RealtimeNano() ;
			m_Track.Printf("%s: COMPLETE 1: Client Event %d Msg %d Sock %d Port %d Bytes (%d/%d) Times: recv %l proc %l xmit %l so total (%l ns)\n",
				*_fn,
				m_nMsgno,
				pTB->m_seq,
				m_nSock,
				m_nPort,
				SizeIn(),
				TotalOut(),
				TimeRecv(),
				TimeProc(),
				TimeXmit(),
				TimeRecv() + TimeProc() + TimeXmit()) ;
			//m_pLog->Out(m_Track) ;

			m_Outgoing.Pull() ;
			break ;
		}

		if (pTB->next->m_msgId != pTB->m_msgId)
		{
			m_nsSendEnd = RealtimeNano() ;
			m_Track.Printf("%s: COMPLETE 2: Client Event %d Msg %d Sock %d Port %d Bytes IN (%d/%d) Times: recv %l proc %l xmit %l so total (%l ns)\n",
				*_fn,
				m_nMsgno,
				pTB->m_seq,
				m_nSock,
				m_nPort,
				SizeIn(),
				TotalOut(),
				TimeRecv(),
				TimeProc(),
				TimeXmit(),
				TimeRecv() + TimeProc() + TimeXmit()) ;
		}

		m_Outgoing.Pull() ;
	}

	return 0 ;
}

/*
**	hzIpServer members
*/

hzEcode	hzIpServer::AddPortTCP	(	hzTcpCode	(*OnIngress)(hzChain&, hzIpConnex*),
									hzTcpCode	(*OnConnect)(hzIpConnex*),
									hzTcpCode	(*OnDisconn)(hzIpConnex*),
									uint32_t	nTimeout,
									uint32_t	nPort,
									uint32_t	nMaxClients,
									bool		bSecure	)
{
	//	Category:	Internet Server
	//
	//	This function adds a listening socket operating on the assigned port, to a server. The client connections (hzIpConnex instances) call an
	//	application specific function each time data comes in on the port. Nothing is assumed about the incoming data so it is nessesary that the
	//	application specific function determines if the data forms a complete message or not and acts accordingly.
	//
	//	Arguments:	1)	OnIngress		Application Specific 'Do Something' function to be called each time data has come in from a connected client.
	//				2)	OnConnect		Application Specific 'Server Hello' function - only required if the protocol used dictates that the server
	//									speaks first.
	//				3)	nTimeout		Timeout for connections to the port in seconds
	//				4)	nPort			Listening port number
	//				5)	nMaxClients		Max number of connections on the port (before server blocks listening socket)
	//				6)	bSecure			Flag to indicate SSL
	//
	//	Returns:	E_ARGUMENT	If the 'OnIngress' function pointer is null.
	//				E_RANGE		If the port number is out of range
	//				E_INITDUP	If there is already a listening socket on the supplied port number
	//				E_MEMORY	If the is insufficient memory to allocate the hzTcpListen for the port
	//				E_INITFIAL	If the hzTcpListen could not be initialized
	//				E_OK		If the operation was successful.

	_hzfunc("hzIpServer::AddPort") ;

	if (!OnIngress)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Each listening port must have a 'OnIngress' handler") ;

	if (nPort < 1 || nPort > 65535)
		return hzerr(_fn, HZ_ERROR, E_RANGE, "Port %d out of range", nPort) ;

	//	Check that we are not already using port

	hzList<hzTcpListen*>::Iter	I ;		//	Listening socket iterator

	hzTcpListen*	pLS ;				//	Listening socket
	uint32_t		opflags ;			//	Operational flags

	for (I = m_LS ; I.Valid() ; I++)
	{
		pLS = I.Element() ;

		if (pLS->GetPort() == nPort)
			return hzerr(_fn, HZ_ERROR, E_INITDUP, "Socket already initialized and listening on port %d\n", pLS->GetPort()) ;
	}

	if (!(pLS = new hzTcpListen(m_pLog)))
		return hzerr(_fn, HZ_ERROR, E_MEMORY, "SERIOUS ERROR - Could not allocate an LS for server") ;

	opflags = HZ_LISTEN_TCP ;
	if (bSecure)
		opflags |= HZ_LISTEN_SECURE ;

	if (pLS->Init(OnIngress, OnConnect, OnDisconn, 0, nTimeout, nPort, nMaxClients, opflags) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "SERIOUS ERROR - Could not initialize an LS for server") ;

	if (m_LS.Add(pLS) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "SERIOUS ERROR - Could not insert an LS for server") ;

	return E_OK ;
}

//hzTcpCode	HandleHttpMsg	(hzChain& Input, hzIpConnex* pCx) ;

hzEcode	hzIpServer::AddPortHTTP	(	hzTcpCode	(*OnHttpReq)(hzHttpEvent*),
									uint32_t	nTimeout,
									uint32_t	nPort,
									uint32_t	nMaxClients,
									bool		bSecure	)
{
	//	Category:	Internet Server
	//
	//	Purpose:	Add a listening port, customized for HTTP, to the singleton hzIpServer instance. This will allow the application
	//				to act as a web server.
	//
	//	Arguments:	1)	OnHttpReq		Application Specific 'Do Something' function called each time a complete HTTP request has come from a connected client. 
	//				2)	nTimeout		Timeout in seconds.
	//				3)	nPort			Port number
	//				4)	nMaxClients		Max number of connections on the port (before server blocks listening socket)
	//				5)	bSecure			Flag to indicate SSL
	//
	//	Returns:	E_ARGUMENT	If the 'HandleHttpEvent' function pointer is null.
	//				E_RANGE		If the port number is out of range
	//				E_INITDUP	If there is already a listening socket on the supplied port number
	//				E_MEMORY	If the is insufficient memory to allocate the hzTcpListen for the port
	//				E_INITFIAL	If the hzTcpListen could not be initialized
	//				E_OK		If the operation was successful.

	_hzfunc("hzIpServer::AddPortHTTP") ;

	if (!OnHttpReq)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Each listening HTTP port must have a 'HandleHttpEvent' handler") ;

	if (nPort < 1 || nPort > 65535)
		return hzerr(_fn, HZ_ERROR, E_RANGE, "Port %d out of range", nPort) ;

	hzList<hzTcpListen*>::Iter	I ;		//	Listening socket iterator

	hzTcpListen*	pLS ;				//	Listening socket
	uint32_t		opflags ;			//	Operational flags

	//	Check that we are not already using port
	for (I = m_LS ; I.Valid() ; I++)
	{
		pLS = I.Element() ;

		if (pLS->GetPort() == nPort)
			return hzerr(_fn, HZ_ERROR, E_INITDUP, "Socket already initialized and listening on port %d\n", pLS->GetPort()) ;
	}

	if (!(pLS = new hzTcpListen(m_pLog)))
		return hzerr(_fn, HZ_ERROR, E_MEMORY, "SERIOUS ERROR - Could not allocate an LS for server") ;

	opflags = HZ_LISTEN_HTTP ;
	if (bSecure)
		opflags |= HZ_LISTEN_SECURE ;

	if (pLS->Init(HandleHttpMsg, 0, 0, 0, nTimeout, nPort, nMaxClients, opflags) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "SERIOUS ERROR - Could not initialize an LS for server") ;

	pLS->m_appFn = (void*) OnHttpReq ;

	if (m_LS.Add(pLS) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "SERIOUS ERROR - Could not insert an LS for server") ;

	return E_OK ;
}

hzEcode	hzIpServer::AddPortSess	(void* (*HandleSession)(void*), uint32_t nTimeout, uint32_t	nPort, uint32_t	nMaxClients, bool bSecure)
{
	//	Category:	Internet Server
	//
	//	This provides a listening socket for connections that are to be handled in a separate thread.
	//
	//	Arguments:	1)	HandleSession	Application Specific thread habdler function called each time a complete HTTP request has come from a connected client. 
	//				2)	nTimeout		Timeout in seconds.
	//				3)	nPort			Port number
	//				4)	nMaxClients		Max number of connections on the port (before server blocks listening socket)
	//				5)	bSecure			Flag to indicate SSL
	//
	//	Returns:	E_ARGUMENT	If an event handler has not been supplied
	//				E_RANGE		If the port number does not fall within range 1-65536
	//				E_INITDUP	If the port is in use (another AddPort call)
	//				E_INITFAIL	If the listening socket cannot be added or initialized
	//				E_OK		If the listening port is assigned

	_hzfunc("hzIpServer::AddPortSess") ;

	if (!HandleSession)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Each listening HTTP port must have a 'HandleHttpEvent' handler") ;

	if (nPort < 1 || nPort > 65535)
		return hzerr(_fn, HZ_ERROR, E_RANGE, "Port %d out of range", nPort) ;

	//	Check that we are not already using port

	hzList<hzTcpListen*>::Iter	I ;		//	Listening socket iterator

	hzTcpListen*	pLS ;				//	Listening socket
	uint32_t		opflags ;			//	Operational flags

	for (I = m_LS ; I.Valid() ; I++)
	{
		pLS = I.Element() ;

		if (pLS->GetPort() == nPort)
			return hzerr(_fn, HZ_ERROR, E_INITDUP, "Socket already initialized and listening on port %d\n", pLS->GetPort()) ;
	}

	if (!(pLS = new hzTcpListen(m_pLog)))
		hzexit(_fn, 0, E_MEMORY, "SERIOUS ERROR - Could not allocate an LS for server") ;

	opflags = HZ_LISTEN_SESSION ;
	if (bSecure)
		opflags |= HZ_LISTEN_SECURE ;

	if (pLS->Init(0, 0, 0, HandleSession, nTimeout, nPort, nMaxClients, opflags) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "SERIOUS ERROR - Could not initialize an LS for server") ;

	pLS->m_appFn = (void*) HandleSession ;

	if (m_LS.Add(pLS) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "SERIOUS ERROR - Could not insert an LS for server") ;

	return E_OK ;
}

hzEcode	hzIpServer::AddPortUDP	(	hzTcpCode	(*OnIngress)(hzChain&, hzIpConnex*),
									hzTcpCode	(*OnConnect)(hzIpConnex*),
									hzTcpCode	(*OnDisconn)(hzIpConnex*),
									uint32_t	nTimeout,
									uint32_t	nPort,
									uint32_t	nMaxClients,
									bool		bSecure	)
{
	//	Category:	Internet Server
	//
	//	This function adds a listening socket operating on the assigned port, to a server. The client connections (hzIpConnex instances) call an
	//	application specific function each time data comes in on the port. Nothing is assumed about the incoming data so it is nessesary that the
	//	application specific function determines if the data forms a complete message or not and acts accordingly.
	//
	//	Arguments:	1)	OnIngress		Application Specific 'Do Something' function to be called each time data has come in from a connected client.
	//				2)	OnConnect		Application Specific 'Server Hello' function - only required if the protocol used dictates that the server
	//									speaks first.
	//				3)	nTimeout		Timeout for connections to the port in seconds
	//				4)	nPort			Listening port number
	//				5)	nMaxClients		Max number of connections on the port (before server blocks listening socket)
	//				6)	bSecure			Flag to indicate SSL
	//
	//	Returns:	E_ARGUMENT	If the 'OnIngress' function pointer is null.
	//				E_RANGE		If the port number is out of range
	//				E_INITDUP	If there is already a listening socket on the supplied port number
	//				E_MEMORY	If the is insufficient memory to allocate the hzTcpListen for the port
	//				E_INITFIAL	If the hzTcpListen could not be initialized
	//				E_OK		If the operation was successful.

	_hzfunc("hzIpServer::AddUdpPort") ;

	if (!OnIngress)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Each listening port must have a 'OnIngress' handler") ;

	if (nPort < 1 || nPort > 65535)
		return hzerr(_fn, HZ_ERROR, E_RANGE, "Port %d out of range", nPort) ;

	//	Check that we are not already using port

	hzList<hzTcpListen*>::Iter	I ;		//	Listening socket iterator

	hzTcpListen*	pLS ;				//	Listening socket
	uint32_t		opflags ;			//	Operational flags

	for (I = m_LS ; I.Valid() ; I++)
	{
		pLS = I.Element() ;

		if (pLS->GetPort() == nPort)
			return hzerr(_fn, HZ_ERROR, E_INITDUP, "Socket already initialized and listening on port %d\n", pLS->GetPort()) ;
	}

	if (!(pLS = new hzTcpListen(m_pLog)))
		return hzerr(_fn, HZ_ERROR, E_MEMORY, "SERIOUS ERROR - Could not allocate an LS for server") ;

	opflags = HZ_LISTEN_UDP ;
	if (bSecure)
		opflags |= HZ_LISTEN_SECURE ;

	if (pLS->Init(OnIngress, OnConnect, OnDisconn, 0, nTimeout, nPort, nMaxClients, opflags) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "SERIOUS ERROR - Could not initialize an LS for server") ;

	if (m_LS.Add(pLS) != E_OK)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "SERIOUS ERROR - Could not insert an LS for server") ;

	return E_OK ;
}

hzEcode	hzIpServer::Activate	(void)
{
	//	Purpose:	Activates all listening sockets added to the server by AddPort(). This must only be called once.
	//
	//	Arguments:	None
	//
	//	Returns:	E_INITDUP	If this has already been called
	//				E_INITFAIL	If one or more listening sockets fail to activate
	//				E_OK		If the operation was successful.

	_hzfunc("hzIpServer::Activate") ;

	if (m_bActive)
		return hzerr(_fn, HZ_ERROR, E_INITDUP, "All ports already active") ;

	hzList<hzTcpListen*>::Iter	I ;		//	Listening socket iterator

	hzTcpListen*	pLS ;				//	Listening socket

	for (I = m_LS ; I.Valid() ; I++)
	{
		pLS = I.Element() ;

		//	Activate port
		if (pLS->Activate() != E_OK)
			return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Could not activate listening port (sock %d)", pLS->GetPort()) ;
	}

	m_bActive = true ;
	return E_OK ;
}

/*
**	Slow loris defence
*/

struct	_loris
{
	//	Recording of IP address of client and port client used to connect

	hzIpaddr	ip ;		//	IP address of rogue client
	uint32_t	port ;		//	Port rogue client connected on

	_loris	()	{ port = 0 ; }

	bool	operator<	(const _loris op) const
	{
		if (port < op.port)	return true ;
		if (port > op.port)	return false ;
		if (ip > op.ip)		return false ;
		return true ;
	}

	bool	operator>	(const _loris op) const
	{
		if (port > op.port)	return true ;
		if (port < op.port)	return false ;
		if (ip < op.ip)		return false ;
		return true ;
	}
} ;

hzMapS<_loris,uint32_t>	s_black ;	//	Monitor for slow lorris attacks

static	bool	_ipcheck	(const char* addr, uint32_t port)
{
	_loris		I ;		//	Client IP address and connected port
	uint32_t	t ;		//	Time of last connect
	uint32_t	x ;		//	Time before next connection allowed

	I.ip = addr ;
	I.port = port ;
	t = time(0) ;

	if (s_black.Exists(I))
	{
		x = s_black[I] ;
		x -= 15 ;
		return x <= t ? true : false ;
	}

	s_black.Insert(I,t) ;
	return false ;
}

/*
**	SECTION X:	Epoll Method (Single Threaded)
*/

#define MAXEVENTS	100

hzIpConnex*	currCC[2000] ;		//	Connected clients

void	hzIpServer::ServeEpollST	(void)
{
	//	Category:	Internet Server
	//
	//	This is executed in a single thread and acts as server using epoll in edge-triggered mode. The thread must be dedicated for the purpose as it is blocked
	//	if there is no incoming requests to contend with. In edge-triggered mode, notification that a socket has data to be read, is only given at the point the
	//	data arrives and so must always be read in full.
	//
	//	Arguments:	None
	//
	//	Returns:	None
	//
	//	Note:		Call only once for all listening sockets! This function will not return until the server is shut down

	_hzfunc("hzIpServer::ServeEpollST") ;

	//	Here we stay in a loop consiting of three stages:-
	//	1)	Setup all array of all active entities for the epoll system call
	//	2)	Call epoll to wait for something to come in.
	//	3)	Go through all active entries to see which one recieved a message

	hzMapS	<hzIpaddr,hzIpConnex*>		udpClients ;	//	Map of UDP clients by IP address
	hzMapS	<uint32_t,hzIpConnex*>		ConnInError ;	//	Map of TCP clients in error
	hzMapS	<uint32_t,hzTcpListen*>		Listen ;		//	Listening sockets map
	hzList	<hzTcpListen*>::Iter		I ;				//	Listening sockets iterator

	struct epoll_event	eventAr[MAXEVENTS] ;			//	Epoll event array
	struct epoll_event	epEventNew ;					//	Epoll event for new connections

	hzPacket		tbuf ;				//	Fixed buffer for single IP packet
	SOCKADDR		cliAddr ;			//	Client address
	SOCKADDRIN		cliAddrIn ;			//	Client address
	socklen_t		cliLen ;			//	Client address length
	hzTcpListen*	pLS ;				//	Listening socket
	hzIpConnex*		pCC ;				//	Connected client
	SSL*			pSSL ;				//	SSL session (if applicable)
 	X509*			client_cert ;		//	Client certificate
	hzProcInfo		proc_data ;			//	Process data (Client socket & IP address, must be destructed by the called thread function)
	char*			clicert_str ;		//	Client certificate string
	hzIpinfo*		ipi ;				//	IP address for testing against black/white lists
	hzXDate			now ;				//	Time now (used to log connections)
	pthread_attr_t	tattr ;				//	Thread attribute (to support thread invokation)
	pthread_t		tid ;				//	Needed for multi-threading
	timeval			tv ;				//	Time limit for epoll call
	uint64_t		nsThen ;			//	Before the wait
	uint64_t		nsNow ;				//	After the wait
	hzIpaddr		ipa ;				//	IP address
	uint32_t		nCliSeq ;			//	Client connection id allocator
	uint32_t		nLoop ;				//	Number of times round the epoll event loop
	uint32_t		nSlot ;				//	Connections iterator
	uint32_t		nEpollEvents ;		//	Result of the epoll call
	uint32_t		nError ;			//	Errno of the epoll call
	uint32_t		cSock ;				//	Client socket (from accept)
	uint32_t		cPort ;				//	Client socket (from accept)
	uint32_t		prc ;				//	Return from pthread_create
	uint32_t		nBannedAttempts ;	//	Connection attempts by blocked IP address
	int32_t			aSock ;				//	Client socket (validated)
	int32_t			flags ;				//	Flags to mak socket non-blocking
	int32_t			nRecv ;				//	No bytes read from client socket after epoll (in one read)
	int32_t			nRecvTotal ;		//	No bytes read from client socket after epoll (in a series of reads)
	int32_t			sys_rc ;			//	Return from system library calls
	int32_t			xmitState ;			//	Return from _xmit() call
	hzTcpCode		trc ;				//	Return code
	char			sbuf[24] ;			//	Client IP address textform buffer
	char			ipbuf[44] ;			//	Client IP address textform buffer

	//	Pre-define thread attributes
	pthread_attr_init(&tattr) ;
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED) ;

	//	Set list default
	Listen.SetDefaultObj((hzTcpListen*)0) ;

	//	Init connected client regime
	for (nSlot = 0 ; nSlot < 1024 ; nSlot++)
		currCC[nSlot] = 0 ;
	cliLen = sizeof(cliAddrIn) ;

	//	Create epoll socket and and listening sockets to the controller
	epollSocket = epoll_create1(0) ;
	if (epollSocket == -1)
		Fatal("%s. Could not create epoll socket\n", *_fn) ;
	m_pLog->Log(_fn, "Epoll socket is %d\n", epollSocket) ;

	for (I = m_LS ; I.Valid() ; I++)
	{
		pLS = I.Element() ;

		epEventNew.data.fd = pLS->GetSocket() ;
		epEventNew.events = EPOLLIN ;	//| EPOLLET ;
		if (epoll_ctl(epollSocket, EPOLL_CTL_ADD, pLS->GetSocket(), &epEventNew) < 0)
			Fatal("%s. Could not add listening socket %d to the epoll controller\n", *_fn, pLS->GetSocket()) ;

		m_pLog->Log(_fn, "Added listening socket is %d\n", pLS->GetSocket()) ;
		Listen.Insert(pLS->GetSocket(), pLS) ;
	}

	//	Main loop - waiting for events
	nLoop = nCliSeq = nBannedAttempts = 0 ;

	for (;;)
	{
		//	Check for shutdown condition
		if (m_bShutdown)
		{
			//	Check for outstanding connections
			for (nSlot = 0 ; nSlot < 1024 ; nSlot++)
			{
				if (currCC[nSlot])
					break ;
			}

			if (nSlot == 1024)
				break ;
		}

		//	Check for expired connections
		for (nSlot = 0 ; nSlot < ConnInError.Count() ; nSlot++)
		{
			pCC = ConnInError.GetObj(nSlot) ;
		}

		//	Wait for events
		nsThen = RealtimeNano() ;
		nEpollEvents = epoll_wait(epollSocket, eventAr, MAXEVENTS, 60000) ;
		nsNow = RealtimeNano() ;
		nLoop++ ;

		if (!nEpollEvents)
			m_pLog->Log(_fn, "No action\n") ;

		for (nSlot = 0 ; nSlot < nEpollEvents ; nSlot++)
		{
			if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
			{
				if (nSlot)
					m_pLog->Log(_fn, "Loop %u slot %u: Event on socket %d\n", nLoop, nSlot, eventAr[nSlot].data.fd) ;
				else
					m_pLog->Log(_fn, "Loop %u: Waited %lu nanoseconds for %d events: 1st sock %u\n", nLoop, nsNow - nsThen, nEpollEvents, eventAr[0].data.fd) ;
			}

			/*
			**	Check for hangups and other connection errors
			*/

			if (eventAr[nSlot].events & EPOLLHUP)
			{
				//	Hangup signal
				cSock = eventAr[nSlot].data.fd ;
				pCC = currCC[cSock] ;
				nError = 0 ;

				if (!pCC)				{ m_bShutdown = true ; m_pLog->Log(_fn, "Loop %u slot %u: HANGUP CORRUPT Sock %d No connector\n", nLoop, nSlot, cSock) ; continue ; }
				if (!pCC->CliSocket())	{ m_bShutdown = true ; m_pLog->Log(_fn, "Loop %u slot %u: HANGUP CORRUPT Sock %d Connector defunct\n", nLoop, nSlot, cSock) ; continue ; }

				if (getsockopt(cSock, SOL_SOCKET, SO_ERROR, (void *)&nError, &cliLen) == 0)
					pCC->m_Track.Printf("Loop %u slot %u: HANGUP on socket %d (%s)\n", nLoop, nSlot, cSock, strerror(nError)) ;
				else
					pCC->m_Track.Printf("Loop %u slot %u: HANGUP on socket %d (no details)\n", nLoop, nSlot, cSock) ;
			}

			if (eventAr[nSlot].events & EPOLLERR)
			{
				//	An error has occured on this fd, or the socket is not ready for reading
				cSock = eventAr[nSlot].data.fd ;
				pCC = currCC[cSock] ;
				nError = 0 ;

				if (!pCC)				{ m_bShutdown = true ; m_pLog->Log(_fn, "Loop %u slot %u: EPOLLERR CORRUPT Sock %d No connector\n", nLoop, nSlot, cSock) ; continue ; }
				if (!pCC->CliSocket())	{ m_bShutdown = true ; m_pLog->Log(_fn, "Loop %u slot %u: EPOLLERR CORRUPT Sock %d Connector defunct\n", nLoop, nSlot, cSock) ; continue ; }

				if (getsockopt(cSock, SOL_SOCKET, SO_ERROR, (void *)&nError, &cliLen) == 0)
					pCC->m_Track.Printf("Loop %u slot %u: EPOLLERR on socket %d (%s)\n", nLoop, nSlot, cSock, strerror(nError)) ;
				else
					pCC->m_Track.Printf("Loop %u slot %u: EPOLLERR on socket %d (no details)\n", nLoop, nSlot, cSock) ;
			}

			/*
			**	Check for events on listening sockets
			*/

			if (Listen.Exists(eventAr[nSlot].data.fd))
			{
				//	We have a notification on the listening socket so either a new incoming connection or if the listening socket is UDP, an incoming UDP packet.

				pLS = Listen[eventAr[nSlot].data.fd] ;
				pSSL = 0 ;

				if (pLS->UseUDP())
				{
					/*
					**	Packet is UDP
					*/

					cSock = pLS->GetSocket() ;
					flags = fcntl(pLS->GetSocket(), F_GETFL, 0) ;
					if (flags == -1)
					{
						m_pLog->Log(_fn, "UDP: Loop %u: NOTE: Could not make client socket %d non blocking (case 1)\n", nLoop, pLS->GetSocket()) ;
						if (close(pLS->GetSocket()) < 0)
							m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after non_block_fail(1) errno=%d\n", nLoop, pLS->GetSocket(), errno) ;
						continue ;
					}
					flags |= O_NONBLOCK ;
					if (fcntl(pLS->GetSocket(), F_SETFL, flags) == -1)
					{
						m_pLog->Log(_fn, "UDP: Loop %u: NOTE: Could not make client socket %d non blocking (case 2)\n", nLoop, pLS->GetSocket()) ;
						if (close(pLS->GetSocket()) < 0)
							m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after non_block_fail(2) errno=%d\n", nLoop, pLS->GetSocket(), errno) ;
						continue ;
					}

					if ((nRecv = recvfrom(pLS->GetSocket(), tbuf.m_data, HZ_MAXPACKET, 0, (SOCKADDR*) &cliAddr, &cliLen)) < 0)
					{
						m_pLog->Log(_fn, "UDP: Loop %u: NOTE: Could not read from UDP client\n", nLoop) ;
						continue ;
					}

					ipa = (uint32_t) cliAddrIn.sin_addr.s_addr ;
					cPort = ntohs(cliAddrIn.sin_port) ;
					tbuf.m_data[nRecv] = 0 ;

					pCC = udpClients[ipa] ;
					if (!pCC)
					{
						pCC = new hzIpConnex(m_pLog) ;
						udpClients.Insert(ipa, pCC) ;

						//	Initialize and oxygenate the connection
						pCC->Initialize(pLS, pSSL, ipbuf, cSock, cPort, nCliSeq++) ;
						if (pCC->m_OnConnect)
							pCC->m_OnConnect(pCC) ;
					}

					m_pLog->Out("Received UDP message %s from port %d internet address %s\n", tbuf.m_data, cPort, *ipa.Str()) ;

					if (pCC->MsgReady())
						trc = pCC->m_OnIngress(pCC->InputZone(), pCC) ;
					else
						trc = TCP_INCOMPLETE ;

					switch (trc)
					{
					case TCP_TERMINATE:

						if (pCC->_isxmit())
						{
							//	If outgoing data not complete, transmit some more.
							xmitState = pCC->_xmit(tbuf) ;
							if (xmitState < 0)
							{
								m_pLog->Log(_fn, "UDP: Loop %u slot %u: TCP_TERMINATE: Write error - Sock %d/%d to be removed\n", nLoop, nSlot, cSock, pCC->CliPort()) ;
								pCC->Terminate() ;
								udpClients.Delete(ipa) ;
								delete pCC ;
								break ;
							}

							if (xmitState > 0)
							{
								//	We have EAGAIN or EWOULDBLOCK and so now we make epoll entry to look for write event (when ocket becomes writable)
								m_pLog->Log(_fn, "UDP: Loop %u slot %u: TCP_TERMINATE: Write delay - Sock %d /%d, looking for write event\n",
									nLoop, nSlot, cSock, pCC->CliPort()) ;
								break ;
							}
						}
	
						//	No more outgoing data so close
						m_pLog->Log(_fn, "UDP: Loop %u slot %u: TCP_TERMINATE: Normal termination, sock %d/%d to be removed\n", nLoop, nSlot, cSock, pCC->CliPort()) ;
						pCC->Terminate() ;
						udpClients.Delete(ipa) ;
						delete pCC ;
						break ;

					case TCP_KEEPALIVE:

						m_pLog->Log(_fn, "UDP: Client %d SERVER - data processed, awaiting further messages\n", pCC->EventNo()) ;
						pCC->Oxygen() ;
						if (pCC->_isxmit())
						{
							xmitState = pCC->_xmit(tbuf) ;
							if (xmitState < 0)
							{
								m_pLog->Log(_fn, "UDP: Loop %u slot %u: TCP_KEEPALIVE: Write error, sock %d/%d to be removed\n", nLoop, nSlot, cSock, pCC->CliPort()) ;
								pCC->Terminate() ;
								udpClients.Delete(ipa) ;
								delete pCC ;
								break ;
							}

							if (xmitState > 0)
							{
								//	We have EAGAIN or EWOULDBLOCK and so now we make epoll entry to look for write event (when ocket becomes writable)
								m_pLog->Log(_fn, "UDP: Loop %u slot %u: TCP_KEEPALIVE: Write delay - Sock %d/%d retained\n", nLoop, nSlot, cSock, pCC->CliPort()) ;
								break ;
							}

							//	We have xmitState of 0 which means the message was written in full to the socket
							m_pLog->Log(_fn, "UDP: Loop %u slot %u: TCP_KEEPALIVE: Write OK, sock %d/%d retained\n", nLoop, nSlot, cSock, pCC->CliPort()) ;
						}

						if (pCC->IsCliTerm())
						{
							//	No longer any incoming requests and as we have finished writing, close
							m_pLog->Log(_fn, "UDP: Loop %u loop %u: TCP_KEEPALIVE: Client done so sock %d/%d to be removed\n", nLoop, nSlot, cSock, pCC->CliPort()) ;
							pCC->Terminate() ;
							udpClients.Delete(ipa) ;
							delete pCC ;
							break ;
						}
						break ;

					case TCP_INCOMPLETE:

						m_pLog->Log(_fn, "UDP: Client %d SERVER - data incomplete\n", pCC->EventNo()) ;
						pCC->Oxygen() ;
						break ;

					case TCP_INVALID:		//	The message processor cannot dechipher the message. Kill the connection

						if (m_pLog)
							m_pLog->Log(_fn, "UDP: Loop %u: Sock %d/%d INVALID TCP code\n", nLoop, cSock, pCC->CliPort()) ;
						pCC->Terminate() ;
						udpClients.Delete(ipa) ;
						delete pCC ;
						break ;
					}

					continue ;
					//	End of UDP section
				}

				/*
				**	Now the TCP case: Firstly accept the client no matter what, if there is a problem close the connection.
				*/

				aSock = accept(pLS->GetSocket(), &cliAddr, &cliLen) ;
				if (aSock < 0)
				{
					m_pLog->Log(_fn, "Loop %u: NOTE: Listening socket %d ignored client on port %d\n", nLoop, pLS->GetSocket(), pLS->GetPort()) ;

					if (errno && errno != EAGAIN && errno != EWOULDBLOCK)
					{
						m_bShutdown = true ;
						m_pLog->Log(_fn, "Loop %u: SHUTDOWN: Listening socket %d failure: Port %d (errno=%d)\n", nLoop, pLS->GetSocket(), pLS->GetPort(), errno) ;
					}
					continue ;
				}

				sys_rc = getnameinfo(&cliAddr, cliLen, ipbuf, 24, sbuf, 24, NI_NUMERICHOST | NI_NUMERICSERV) ;
                if (sys_rc)
				{
					m_pLog->Log(_fn, "Loop %u: Accepted connection on socket %d port %d but no name info - closing connection\n", nLoop, aSock, pLS->GetPort()) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after name info fail. errno=%d\n", nLoop, cSock, errno) ;
					continue ;
				}

				cSock = aSock ;
				cPort = atoi(sbuf) ;
				ipa = ipbuf ;

				//	Check if client is blocked. Note this does not work when connections are coming via Apache proxypass because getnameinfo will give the IP address as 127.0.0.1
				if (_hzGlobal_StatusIP.Exists(ipa))
				{
					ipi = &(_hzGlobal_StatusIP[ipa]) ;

					if (!(ipi->m_bInfo & HZ_IPSTATUS_WHITE) || ipi->m_tWhite < time(0))
					{
						if (!(ipi->m_nSince%100))	m_pLog->Log(_fn, "BLOCKED IP %s reaches %u attempts\n", ipbuf, ipi->m_nSince) ;
						if (close(cSock) < 0)		m_pLog->Log(_fn, "ERROR: Could not close socket %d after blocked IP address detected. errno=%d\n", cSock, errno) ;

						ipi->m_nSince++ ;
						ipi->m_nTotal++ ;

						nBannedAttempts++ ;
						if (!(nBannedAttempts%10000))
							m_pLog->Log(_fn, "BLOCKED IP TOTAL reaches %u attempts\n", nBannedAttempts) ;
						continue ;
					}
				}

				if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
					m_pLog->Log(_fn, "Loop %u: Accepted connection (1): socket %d/%s host %s port %d\n", nLoop, aSock, sbuf, ipbuf, pLS->GetPort()) ;

				if (currCC[cSock])
					m_pLog->Log(_fn, "WARNING: Loop %u: New connection on socket %d does not have a clean slot %p\n", nLoop, cSock, currCC[cSock]) ;

				if (!pLS->m_OnSession)
				{
					//	If the connection is not to be handled by a designated session, make the incoming socket non-blocking (to be handled by epoll)
					flags = fcntl(cSock, F_GETFL, 0) ;
					if (flags == -1)
					{
						m_pLog->Log(_fn, "Loop %u: NOTE: Could not make client socket %d non blocking (case 1)\n", nLoop, cSock) ;
						if (close(cSock) < 0)
							m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after non_block_fail(1) errno=%d\n", nLoop, cSock, errno) ;
						continue ;
					}
					flags |= O_NONBLOCK ;
					if (fcntl(cSock, F_SETFL, flags) == -1)
					{
						m_pLog->Log(_fn, "Loop %u: NOTE: Could not make client socket %d non blocking (case 2)\n", nLoop, cSock) ;
						if (close(cSock) < 0)
							m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after non_block_fail(2) errno=%d\n", nLoop, cSock, errno) ;
						continue ;
					}
				}

				//	Deal with the case where we have too many connections or we are shutting down.
				if (m_bShutdown || pLS->GetCurConnections() >= pLS->GetMaxConnections())
				{
					m_pLog->Log(_fn, "Loop %u: NOTE: System too busy: Curr %d Max %d stat %d\n",
						nLoop, pLS->GetCurConnections(), m_nMaxClients, m_bShutdown ? 1 : 0) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after sys_too_busy errno=%d\n", nLoop, cSock, errno) ;
					continue ;
				}

				//	Client is already connected. Do we allow further connections?
				if (_ipcheck(ipbuf, pLS->GetPort()))
				{
					m_pLog->Log(_fn, "Loop %u: NOTE: Lorris Attack from IP %s port %d\n", ipbuf, pLS->GetPort()) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after lorris_attack errno=%d\n", nLoop, cSock, errno) ;
					continue ;
				}

				//	All is well so set socket options
				tv.tv_sec = 20 ;
				tv.tv_usec = 0 ;
				if (setsockopt(cSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
				{
					m_pLog->Log(_fn, "Loop %u: NOTE: Could not set send socket options\n", nLoop) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after setop_send errno=%d\n", nLoop, cSock, errno) ;
					continue ;
				}
				if (setsockopt(cSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
				{
					m_pLog->Log(_fn, "Loop %u: NOTE: Could not set recv socket options\n", nLoop) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after setop_recv errno=%d\n", cSock, errno) ;
					continue ;
				}

				//	Do we have SSL on top of connection?
				if (pLS->UseSSL())
				{
					//	Create a new SSL session instance
					m_pLog->Log(_fn, "Loop %u: NOTE: SSL instance on port %d\n", nLoop, pLS->GetPort()) ;
					pSSL = SSL_new(s_svrCTX) ;
					if (!pSSL)
					{
						m_pLog->Log(_fn, "Loop %u: NOTE: Failed to allocate an SSL instance on port %d\n", nLoop, pLS->GetPort()) ;
						continue ;
					}

					//if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
						m_pLog->Log(_fn, "Loop %u: Allocated SSL instance on port %d\n", nLoop, pLS->GetPort()) ;

					//	Set the SSL session socket
					SSL_set_accept_state(pSSL) ;
					SSL_set_fd(pSSL, cSock) ;

					sys_rc = SSL_accept(pSSL) ;
					if (sys_rc <= 0)
					{
						m_pLog->Log(_fn, "Loop %u: NOTE: Failed to accept SSL client port %d sock %d (err=%d)\n",
							nLoop, pLS->GetPort(), cSock, SSL_get_error(pSSL, sys_rc)) ;
						SSL_free(pSSL) ;
						if (close(cSock) < 0)
							m_pLog->Log(_fn, "Loop %u: NOTE: Could not close socket %d after SSL no_alloc errno=%d\n", nLoop, cSock, errno) ;
						continue ;
					}

					if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
						m_pLog->Log(_fn, "Loop %u: SSL connection using %s\n", nLoop, SSL_get_cipher(pSSL)) ;

					if (verify_callback)
					{
  						//	Get the client's certificate (optional)
  						client_cert = SSL_get_peer_certificate(pSSL);

						if (client_cert == NULL)
							m_pLog->Log(_fn, "Loop %u: The SSL client does not have certificate.\n", nLoop) ;
						else
						{
							clicert_str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
							if (!clicert_str)
							{
								m_pLog->Log(_fn, "Loop %u: No Client Certificate found\n", nLoop) ;
								continue ;
							}

							m_pLog->Log(_fn, "Loop %u: Subject of Client Cert: %s\n", nLoop, clicert_str) ;
							free(clicert_str) ;

							clicert_str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
							if (!clicert_str)
							{
								m_pLog->Log(_fn, "Loop %u: No Client Cert Issuer found\n") ;
								continue ;
							}

							m_pLog->Log(_fn, "Loop %u: Issuer of Client Cert: %s\n", nLoop, clicert_str) ;
							free(clicert_str) ;
							X509_free(client_cert);
						}
					}
				}

				/*
				**	If the listening socket is for a session handler, i.e. the connection is to be handled by a separate thread, deal with this here
				*/

				if (pLS->m_OnSession)
				{
					//	Call the session handler in it's own thread
					proc_data.SetParams(pSSL, cSock, ipa) ;
					prc = pthread_create(&tid, &tattr, pLS->m_OnSession, &proc_data) ;
					if (prc == 0)
					{
						m_pLog->Log(_fn, "Thread %u accepted connection from %s on socket %d\n", tid, ipbuf, cSock) ;
						if (cSock >= m_nMaxSocket)
							m_nMaxSocket = cSock + 1 ;
					}
					else
					{
						m_pLog->Log(_fn, "NOTE: Could not create thread for client %s on socket %d. Closing conection\n", ipbuf, cSock) ;
						if (close(cSock) < 0)
							m_pLog->Log(_fn, "NOTE: Could not close socket %d after SSL fail_thread_create. errno=%d\n", cSock, errno) ;
						//delete pPD ;
					}

					continue ;
				}

				/*
				**	Allocate the connected client object
				*/

				pCC = currCC[cSock] ;
				if (pCC)
				{
					m_pLog->Log(_fn, "Loop %u slot %u: CORRUPT: Existing client connection handler on sock %d/%d.\n", nLoop, nSlot, cSock, pCC->CliPort()) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "NOTE: Could not close socket %d after memory allocation failure. errno=%d\n", cSock, errno) ;
					m_bShutdown = true ;
					continue ;
				}

				pCC = new hzIpConnex(m_pLog) ;
				if (!pCC)
				{
					m_pLog->Log(_fn, "ERROR: No memory for client %s on socket %d. Closing conection\n", ipbuf, cSock) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "NOTE: Could not close socket %d after memory allocation failure. errno=%d\n", cSock, errno) ;
					m_bShutdown = true ;
					continue ;
				}

				//	Add new client socket to the epoll control
				epEventNew.data.fd = cSock ;
				epEventNew.events = EPOLLIN ;	//| EPOLLET ;

				if (epoll_ctl(epollSocket, EPOLL_CTL_ADD, cSock, &epEventNew) < 0)
				{
					m_pLog->Log(_fn, "Loop %u slot %u: EPOLL ERROR: Could not add client connection handler on sock %d/%d. Error=%s\n",
						nLoop, nSlot, cSock, pCC->CliPort(), strerror(errno)) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "NOTE: Could not close socket %d after memory allocation failure. errno=%d\n", cSock, errno) ;
					m_bShutdown = true ;
					delete pCC ;
					continue ;
				}

				//	Initialize and oxygenate the connection
				currCC[cSock] = pCC ;
				pCC->Initialize(pLS, pSSL, ipbuf, cSock, cPort, nCliSeq++) ;
				if (pCC->m_OnConnect)
					pCC->m_OnConnect(pCC) ;
				now.SysDateTime() ;
				pCC->m_Track.Printf("%s %s: Loop %u slot %u: NEW Connection on socket %d/%d from %s:%u\n",
					*_fn, *now.Str(FMT_TIME_USEC), nLoop, nSlot, cSock, pCC->CliPort(), ipbuf, pLS->GetPort()) ;
				continue ;

				//	End of TCP Listen stuff
			}

			if (eventAr[nSlot].events & EPOLLOUT)
			{
				//	Subsequent to a call to hzIpConnex::SendData to place a response in the output queue and to modify the epoll entry for the connection so the socket is monitored
				//	for writability, we now have notification that the socket is writable.

				cSock = eventAr[nSlot].data.fd ;
				pCC = currCC[cSock] ;
				if (!pCC)
				{
					m_pLog->Log(_fn, "Loop %u slot %u: CORRUPT: Write event on socket %d/%d but no connector!\n", nLoop, nSlot, cSock, pCC->CliPort()) ;
					m_bShutdown = true ;
					continue ;
				}

				if (cSock != pCC->CliSocket())
				{
					m_pLog->Log(_fn, "Loop %u slot %u: CORRUPT: Write event on sock %d/%d but attached client object has socket of %d\n",
						nLoop, nSlot, cSock, pCC->CliPort(), pCC->CliSocket()) ;
					m_bShutdown = true ;
					continue ;
				}

				if (pCC->IsCliBad())
				{
					pCC->m_Track.Printf("%s: Loop %u slot %u: BAD Client, connection on socket %d/%d to be removed\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
					pCC->Terminate() ;
					delete pCC ;
					currCC[cSock] = 0 ;
					continue ;
				}

				if (!pCC->_isxmit() && _hzGlobal_Debug & HZ_DEBUG_SERVER)
					pCC->m_Track.Printf("%s: Loop %u slot %u: Write event with nothin socket %d/%d\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;

				if (pCC->_isxmit())
				{
					pCC->m_Track.Printf("%s: Loop %u slot %u: Write event with output on socket %d/%d\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;

					xmitState = pCC->_xmit(tbuf) ;
					if (xmitState < 0)
					{
						pCC->m_Track.Printf("%s: Loop %u slot %u: EPOLLOUT Write error, connection on socket %d/%d to be removed\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
						pCC->Terminate() ;
						delete pCC ;
						currCC[cSock] = 0 ;
						continue ;
					}
				}
			}

			if (eventAr[nSlot].events & EPOLLIN)
			{
				//	At this point we have data on the socket to be read

				cSock = eventAr[nSlot].data.fd ;
				pCC = currCC[cSock] ;
				if (!pCC)
				{
					m_pLog->Log(_fn, "Loop %u: CORRUPT: Read event on socket %d/%d but no connector!\n", nLoop, cSock, pCC->CliPort()) ;
					m_bShutdown = true ;
					continue ;
				}

				if (cSock != pCC->CliSocket())
				{
					m_pLog->Log(_fn, "Loop %u slot %u: CORRUPT: Read event on sock %d/%d but attached client object has socket of %d\n",
						nLoop, nSlot, cSock, pCC->CliPort(), pCC->CliSocket()) ;
					m_bShutdown = true ;
					continue ;
				}

				//	Now because we are in edge-triggered mode, continue reading from the socket until we get -1
				for (nRecvTotal = 0 ;; nRecvTotal += nRecv)
				{
					nRecv = pCC->Recv(tbuf) ;
					if (nRecv <= 0)
						break ;
				}

				if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
				{
					if (!pCC->IsCliTerm())
						pCC->m_Track.Printf("%s: Loop %u slot %u: Client LIVE sock %d/%d. Recv %d Have %d\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort(), nRecvTotal, pCC->SizeIn()) ;
					else
						pCC->m_Track.Printf("%s: Loop %u slot %u: Client DONE sock %d/%d. Revc %d Have %d\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort(), nRecvTotal, pCC->SizeIn()) ;
				}

				//	Data to be processed has come in
				if (!nRecvTotal)
				{
					pCC->m_Track.Printf("%s: Loop %u slot %u: Read event with zero data on socket %d - disconnecting\n", *_fn, nLoop, nSlot, cSock) ;
					if (!pCC->_isxmit())
					{
						//	Nothing has come in and nothing is due to go out so clear off the connection
						pCC->m_Track.Printf("%s: Loop %u slot %u: Terminating unused connection on socket %d\n", *_fn, nLoop, nSlot, cSock) ;
						pCC->Terminate() ;
						delete pCC ;
						currCC[cSock] = 0 ;
					}
				}
				else
				{
					if (pCC->MsgReady())
						trc = pCC->m_OnIngress(pCC->InputZone(), pCC) ;
					else
					{
						pCC->m_Track.Printf("%s: Loop %u slot %u: NOTE: Client message not yet complete on socket %d/%d\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
						trc = TCP_INCOMPLETE ;
					}

					switch (trc)
					{
					case TCP_TERMINATE:	//	Directive is to close after outgoing message is complete

						if (pCC->_isxmit())
						{
							//	If outgoing data not complete, transmit some more.
							xmitState = pCC->_xmit(tbuf) ;
							if (xmitState < 0)
							{
								pCC->m_Track.Printf("%s: Loop %u slot %u: TCP_TERMINATE: Write error - Sock %d/%d doomed\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
								pCC->Terminate() ;
								delete pCC ;
								currCC[cSock] = 0 ;
								break ;
							}
						}

						if (!pCC->_isxmit())
						{
							//	No longer any outgoing data so close
							pCC->m_Track.Printf("%s: Loop %u slot %u: TCP_TERMINATE: Normal termination, sock %d/%d to be removed\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
							pCC->Terminate() ;
							delete pCC ;
							currCC[cSock] = 0 ;
							break ;
						}

						pCC->m_Track.Printf("%s: Loop %u slot %u: TCP_TERMINATE: Write delay - Sock %d/%d\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
						break ;

					case TCP_KEEPALIVE:	//	Directive is to keep open after outgoing message is complete

						if (pCC->_isxmit())
						{
							xmitState = pCC->_xmit(tbuf) ;
							if (xmitState < 0)
							{
								pCC->m_Track.Printf("%s: Loop %u slot %u: TCP_KEEPALIVE: Write error, sock %d/%d doomed\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
								pCC->Terminate() ;
								delete pCC ;
								currCC[cSock] = 0 ;
								break ;
							}

							if (xmitState > 0)
							{
								//	We have EAGAIN or EWOULDBLOCK and so now we make epoll entry to look for write event (when ocket becomes writable)
								pCC->m_Track.Printf("%s: Loop %u slot %u: TCP_KEEPALIVE: Write delay - Sock %d/%d\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
							}
						}

						if (pCC->IsCliTerm())
						{
							//	No longer any outgoing data so close
							pCC->m_Track.Printf("%s: Loop %u slot %u: TCP_KEEPALIVE: Client terminated so sock %d/%d to be removed\n", *_fn, nLoop, nSlot, cSock, pCC->CliPort()) ;
							pCC->Terminate() ;
							delete pCC ;
							currCC[cSock] = 0 ;
							break ;
						}

						pCC->Oxygen() ;
						break ;

					case TCP_INCOMPLETE:	//	The message processor has indicated it is still waiting for more imput

						pCC->Oxygen() ;
						break ;

					case TCP_INVALID:		//	The message processor cannot dechipher the message. Kill the connection

						pCC->m_Track.Printf("%s: Loop %u: Sock %d/%d INVALID TCP code\n", *_fn, nLoop, cSock, pCC->CliPort()) ;
						pCC->Terminate() ;
						delete pCC ;
						currCC[cSock] = 0 ;
						break ;
					}
				}
			}
		}
	}

	threadLog("%s. SHUTDOWN COMPLETE\n", *_fn) ;
}

/*
**	SECTION X:	Epoll Method (Multi-threaded)
*/

static	hzDiode	s_queRequests ;			//	Lock free que for passing clients from the input thread to the request processor thread
static	hzDiode	s_queResponses ;		//	Lock free que for passing clients from the request processor thread to output thread

static	pthread_mutex_t	s_request_mutex = PTHREAD_MUTEX_INITIALIZER ;	//	Request mutex for server threads
static	pthread_cond_t  s_request_cond = PTHREAD_COND_INITIALIZER ;		//	Request condition for server threads
static	pthread_mutex_t	s_response_mutex = PTHREAD_MUTEX_INITIALIZER ;	//	Response mutex for server threads
static	pthread_cond_t  s_response_cond = PTHREAD_COND_INITIALIZER ;	//	Response condition for server threads

static	uint32_t	s_nReqThreads ;			//	Number of request server threads

void	hzIpServer::ServeRequests	(void)
{
	//	Calculate number of request server thread as this will serve as the divisor
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzIpServer::ServeRequests") ;

	hzPacket		tbuf ;		//	Fixed buffer for single IP packet
	hzIpConnex*	pCC ;		//	Connected client
	hzLogger*		pLog ;		//	Separate logger
	hzTcpCode		rc ;		//	Return code from event handler

	pLog = GetThreadLogger() ;

    pthread_mutex_lock(&s_request_mutex);
	s_nReqThreads++ ;
    pthread_mutex_unlock(&s_request_mutex);

	for (; !m_bShutdown ;)
    {
        pthread_mutex_lock(&s_request_mutex);
        pthread_cond_wait(&s_request_cond, &s_request_mutex);

		for (;;)
		{
			pCC = (hzIpConnex*) s_queRequests.Pull() ;
			if (!pCC)
				break ;

			//	Handle the request
			rc = pCC->m_OnIngress(pCC->InputZone(), pCC) ;

			switch	(rc)
			{
			case TCP_TERMINATE:		pCC->Hypoxia() ;
									s_queResponses.Push(pCC) ;
									pthread_cond_signal(&s_response_cond) ;
									break ;

			case TCP_KEEPALIVE:		pCC->Oxygen() ;
									s_queResponses.Push(pCC) ;
									pthread_cond_signal(&s_response_cond) ;
									break ;

			case TCP_INCOMPLETE:	pCC->Oxygen() ;
									pLog->Log(_fn, "Client %d (sock %d): Data incomplete\n", pCC->EventNo(), pCC->CliSocket()) ;
									break ;
			}
		}

        pthread_mutex_unlock(&s_request_mutex);
	}
}

void	hzIpServer::ServeResponses	(void)
{
	_hzfunc("hzIpServer::ServeResponses") ;

	hzList<hzIpConnex*>		lc ;	//	List of connections with outgoing matter remaining
	hzList<hzIpConnex*>::Iter	ic ;	//	List iterator of connections with outgoing matter remaining

	hzPacket		tbuf ;				//	Fixed buffer for single IP packet
	hzIpConnex*	pCC ;				//	Connected client

	for (; !m_bShutdown ;)
    {
        pthread_mutex_lock(&s_response_mutex);
        pthread_cond_wait(&s_response_cond, &s_response_mutex);

		for (;;)
		{
			pCC = (hzIpConnex*) s_queResponses.Pull() ;
			if (pCC)
				lc.Add(pCC) ;

			if (!lc.Count())
				break ;

			for (ic = lc ; ic.Valid() ;)
			{
				pCC = ic.Element() ;

				//	Write out
				if (pCC->_isxmit())
				{
					pCC->_xmit(tbuf) ;

					if (!pCC->_isxmit())
					{
						ic.Delete() ;
						continue ;
					}
				}
				ic++ ;
			}
		}

        pthread_mutex_unlock(&s_response_mutex);
	}
}

void	hzIpServer::ServeEpollMT	(void)
{
	//	Category:	Internet Server
	//
	//	Act as server based on the epoll method. This function is strickly for use in multi threaded servers. In the epoll method, the epoll socket
	//	(the one which activates the polling) is allocated and used to set up the controlling array of sockets. This array includes listening sockets
	//	as well as the sockets of connected clients.
	//
	//	Arguments:	None
	//
	//	Returns:	None
	//
	//	Note:		Call only once for all listening sockets! This function will not return until the server is shut down

	_hzfunc("hzIpServer::ServeEpollMT") ;

	//	Here we stay in a loop consiting of three stages:-
	//	1)	Setup all array of all active entities for the epoll system call
	//	2)	Call epoll to wait for something to come in.
	//	3)	Go through all active entries to see which one recieved a message

	hzMapS<uint32_t,hzTcpListen*>	ls ;	//	Listening sockets map
	hzMapS<uint32_t,hzIpConnex*>	allCC ;	//	Connected clients
	hzMapS<uint32_t,hzIpConnex*>	duff ;	//	Failed clients
	hzList<hzTcpListen*>::Iter		I ;		//	Listening sockets iterator

	struct epoll_event	eventAr[100] ;		//	Array of epoll events
	struct epoll_event	epEventNew ;		//	Epoll event for new connections
	//struct epoll_event	epEventDead ;		//	Epoll event for dead connections

	hzPacket		tbuf ;					//	Fixed buffer for single IP packet
	SOCKADDR		cliAddr ;				//	Client address
	SOCKADDRIN		cliAddrIn ;				//	Client address
	socklen_t		cliLen ;				//	Client address length
	hzTcpListen*	pLS ;					//	Listening socket
	hzIpConnex*	pCC ;						//	Connected client
	hzProcInfo*		pPD ;					//	Process data (Client socket & IP address, must be destructed by the called thread function)
	SSL*			pSSL ;					//	SSL session (if applicable)
 	X509*			client_cert ;			//	Client certificate
	char*			clicert_str ;			//	Client certificate string
	//const char*		errstr ;				//	Either 'hangup' or 'error'
	timeval			tv ;					//	Time limit for epoll call
	pthread_attr_t	tattr ;					//	Thread attribute (to support thread invokation)
	pthread_t		tid ;					//	Needed for multi-threading
	//uint64_t		then ;					//	Before the wait
	//uint64_t		now ;					//	After the wait
	uint64_t		nsThen ;				//	Before the wait
	uint64_t		nsNow ;					//	After the wait
	hzIpaddr		ipa ;					//	IP address
	uint32_t		nBannedAttempts ;		//	Total connection attempts by banned IP addresses
	uint32_t		nCliSeq = 1 ;			//	Client connection id allocator
	uint32_t		nX ;					//	Dead/failed connections counter
	uint32_t		nLoop ;					//	Number of times round the epoll event loop
	uint32_t		nSlot ;					//	Connections iterator
	uint32_t		nError ;				//	Errno of the epoll call
	uint32_t		cSock ;					//	Client socket (from accept)
	uint32_t		cPort ;					//	Client socket (from accept)
	int32_t			aSock ;					//	Client socket (validated)
	int32_t			epollSocket ;			//	The epoll 'master' socket
	int32_t			flags ;					//	Flags to mak socket non-blocking
	int32_t			nC ;					//	Connections iterator
	int32_t			nRecv ;					//	No bytes read from client socket after epoll
	int32_t			nEpollEvents ;			//	Result of the epoll call
	int32_t			sys_rc ;				//	Return from system library calls
	hzEcode			err ;					//	Returns by called functions
	char			ipbuf[44] ;				//	Client IP address textform buffer

	ls.SetDefaultObj(0) ;
	allCC.SetDefaultObj(0) ;

	//	Pre-define thread attributes
	pthread_attr_init(&tattr) ;
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED) ;

	//	Create epoll socket and and listening sockets to the controller
	epollSocket = epoll_create1(0) ;
	if (epollSocket == -1)
		Fatal("%s. Could not create epoll socket\n", *_fn) ;
	m_pLog->Log(_fn, "Epoll socket is %d\n", epollSocket) ;

	for (I = m_LS ; I.Valid() ; I++)
	{
		pLS = I.Element() ;

		epEventNew.data.fd = pLS->GetSocket() ;
		epEventNew.events = EPOLLIN ;
		if (epoll_ctl(epollSocket, EPOLL_CTL_ADD, pLS->GetSocket(), &epEventNew) < 0)
			Fatal("%s. Could not add listening socket %d to the epoll controller\n", *_fn, pLS->GetSocket()) ;
		m_pLog->Log(_fn, "Added listening socket is %d\n", pLS->GetSocket()) ;

		ls.Insert(pLS->GetSocket(), pLS) ;
	}

	//	Main loop - waiting for events
	nLoop = nCliSeq = nBannedAttempts = 0 ;

	for (;;)
	{
		nsThen = RealtimeNano() ;
		nEpollEvents = epoll_wait(epollSocket, eventAr, MAXEVENTS, 30000) ;
		nsNow = RealtimeNano() ;

		for (nC = 0 ; nC < nEpollEvents ; nC++)
		{
			if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
			{
				if (nSlot)
					m_pLog->Log(_fn, "Loop %u slot %u: Event on socket %d\n", nLoop, nSlot, eventAr[nSlot].data.fd) ;
				else
					m_pLog->Log(_fn, "Loop %u: Waited %lu nanoseconds for %d events: 1st sock %u\n", nLoop, nsNow - nsThen, nEpollEvents, eventAr[0].data.fd) ;
			}

			/*
			**	Check for hangups and other connection errors
			*/

			if (eventAr[nSlot].events & EPOLLHUP)
			{
				//	Hangup signal
				cSock = eventAr[nSlot].data.fd ;
				pCC = currCC[cSock] ;
				nError = 0 ;

				if (!pCC)				{ m_bShutdown = true ; m_pLog->Log(_fn, "Loop %u slot %u: HANGUP CORRUPT Sock %d No connector\n", nLoop, nSlot, cSock) ; continue ; }
				if (!pCC->CliSocket())	{ m_bShutdown = true ; m_pLog->Log(_fn, "Loop %u slot %u: HANGUP CORRUPT Sock %d Connector defunct\n", nLoop, nSlot, cSock) ; continue ; }

				if (getsockopt(cSock, SOL_SOCKET, SO_ERROR, (void *)&nError, &cliLen) == 0)
					pCC->m_Track.Printf("Loop %u slot %u: HANGUP on socket %d (%s)\n", nLoop, nSlot, cSock, strerror(nError)) ;
				else
					pCC->m_Track.Printf("Loop %u slot %u: HANGUP on socket %d (no details)\n", nLoop, nSlot, cSock) ;
			}

			if (eventAr[nSlot].events & EPOLLERR)
			{
				//	An error has occured on this fd, or the socket is not ready for reading
				cSock = eventAr[nSlot].data.fd ;
				pCC = currCC[cSock] ;
				nError = 0 ;

				if (!pCC)				{ m_bShutdown = true ; m_pLog->Log(_fn, "Loop %u slot %u: EPOLLERR CORRUPT Sock %d No connector\n", nLoop, nSlot, cSock) ; continue ; }
				if (!pCC->CliSocket())	{ m_bShutdown = true ; m_pLog->Log(_fn, "Loop %u slot %u: EPOLLERR CORRUPT Sock %d Connector defunct\n", nLoop, nSlot, cSock) ; continue ; }

				if (getsockopt(cSock, SOL_SOCKET, SO_ERROR, (void *)&nError, &cliLen) == 0)
					pCC->m_Track.Printf("Loop %u slot %u: EPOLLERR on socket %d (%s)\n", nLoop, nSlot, cSock, strerror(nError)) ;
				else
					pCC->m_Track.Printf("Loop %u slot %u: EPOLLERR on socket %d (no details)\n", nLoop, nSlot, cSock) ;
			}

			/*
			**	Check for events on listening sockets
			*/

			if (ls.Exists(eventAr[nC].data.fd))
			{
				//	We have a notification on the listening socket, which means one or more incoming connections.
				//	Firstly accept the client no matter what. If there is a problem we will imeadiately close the connection.

				pLS = ls[eventAr[nC].data.fd] ;
				pSSL = 0 ;
				cliLen = sizeof(SOCKADDRIN) ;

				if ((aSock = accept(pLS->GetSocket(), &cliAddr, &cliLen)) < 0)
				{
					m_pLog->Log(_fn, "Listening socket %d will not accept client on port %d\n", aSock, pLS->GetPort()) ;
					continue ;
				}

				//	Make the incoming socket non-blocking and add it to the list of fds to monitor.
				cSock = aSock ;
				flags = fcntl(cSock, F_GETFL, 0) ;
				if (flags == -1)
					Fatal("%s. Could not make client socket %d non blocking (case 1)\n", *_fn, cSock) ;
				flags |= O_NONBLOCK ;
				if (fcntl(cSock, F_SETFL, flags) == -1)
					Fatal("%s. Could not make client socket %d non blocking (case 2)\n", *_fn, cSock) ;

				//	Add client socker to the epoll control
				epEventNew.data.fd = cSock ;
				epEventNew.events = EPOLLIN | EPOLLET ;
				if (epoll_ctl(epollSocket, EPOLL_CTL_ADD, cSock, &epEventNew) < 0)
					Fatal("%s. Could not add client socket %d to control\n", *_fn, cSock) ;

				//	Get the IP address
				inet_ntop(AF_INET, &cliAddrIn.sin_addr, ipbuf, 16) ;
				ipa = ipbuf ;

				//	Deal with the case where we have too many connections or we are shutting down.
				if (m_bShutdown || (pLS->GetCurConnections() >= pLS->GetMaxConnections()))
				{
					m_pLog->Log(_fn, "NOTE: System too busy: Curr %d Max %d stat %d\n", m_nCurrentClients, m_nMaxClients, m_bShutdown ? 1 : 0) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "NOTE: Could not close socket %d after sys_too_busy\n", cSock) ;
					continue ;
				}

				//	Client is already connected. Do we allow further connections?
				if (_ipcheck(ipbuf, pLS->GetPort()))
				{
					m_pLog->Log(_fn, "Lorris Attack from IP %s port %d\n", ipbuf, pLS->GetPort()) ;
					if (close(cSock) < 0)
						m_pLog->Log(_fn, "NOTE: Could not close socket %d after lorris_attack\n", cSock) ;
					continue ;
				}

				//	All is well so set socket options
				tv.tv_sec = 1 ;
				tv.tv_usec = 0 ;

				if (setsockopt(cSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
					{ m_pLog->Log(_fn, "Could not set send socket options\n") ; continue ; }
				if (setsockopt(cSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
					{ m_pLog->Log(_fn, "Could not set recv socket options\n") ; continue ; }

				//	Do we have SSL on top of connection?
				if (pLS->UseSSL())
				{
					//	Create a new SSL session instance
					pSSL = SSL_new(s_svrCTX) ;
					if (!pSSL)
						{ m_pLog->Log(_fn, "Failed to allocate an SSL instance on port %d\n", pLS->GetPort()) ; continue ; }
					m_pLog->Log(_fn, "Allocated SSL instance on port %d\n", pLS->GetPort()) ;

					//	Set the SSL session socket
					SSL_set_fd(pSSL, cSock) ;

					sys_rc = SSL_accept(pSSL) ;
					if (sys_rc <= 0)
					{
						m_pLog->Log(_fn, "NOTE: Failed to accept SSL client on port %d socket %d (error=%d,%d)\n",
							pLS->GetPort(), cSock, sys_rc, SSL_get_error(pSSL, sys_rc)) ;
						SSL_free(pSSL) ;
						if (close(cSock) < 0)
							m_pLog->Log(_fn, "NOTE: Could not close socket %d after SSL fail_accept\n", cSock) ;
						m_pLog->Log(_fn, "Closed socket\n") ;
						continue ;
					}

					m_pLog->Log(_fn, "SSL connection using %s\n", SSL_get_cipher(pSSL)) ;

					if (verify_callback)
					{
  						//	Get the client's certificate (optional)
  						client_cert = SSL_get_peer_certificate(pSSL);

						if (client_cert != NULL)
						{
							clicert_str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
							if (!clicert_str)
								{ m_pLog->Log(_fn, "No Client Certificate found\n") ; continue ; }
							m_pLog->Log(_fn, "Subject of Client Cert: %s\n", clicert_str) ;
							free(clicert_str) ;

							clicert_str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
							if (!clicert_str)
								{ m_pLog->Log(_fn, "No Client Cert Issuer found\n") ; continue ; }
							m_pLog->Log(_fn, "Issuer of Client Cert: %s\n", clicert_str) ;
							free(clicert_str) ;

							X509_free(client_cert);
						}
						else
							printf("The SSL client does not have certificate.\n");
					}
				}

				//	If the listening socket is for a session handler, deal with this here
				if (pLS->m_OnSession)
				{
					//	Call the session handler in it's own thread
					pPD = new hzProcInfo(pSSL, cSock, ipa) ;

					if (pthread_create(&tid, &tattr, pLS->m_OnSession, pPD) == 0)
			  			m_pLog->Log(_fn, "Thread %u accepted connection from %s on socket %d\n", tid, ipbuf, cSock) ;
					else
		 			{
						m_pLog->Log(_fn, "NOTE: Could not create thread for client %s on socket %d. Closing conection\n", ipbuf, cSock) ;
						if (close(cSock) < 0)
							m_pLog->Log(_fn, "NOTE: Could not close socket %d after fail_thread_create\n", cSock) ;
						delete pPD ;
					}
					continue ;
				}

				/*
				**	Allocate the connected client object
				*/

				pCC = allCC[cSock] ;
				if (!pCC)
				{
					pCC = new hzIpConnex(m_pLog) ;
					if (!pCC)
						hzexit(_fn, 0, E_MEMORY, "No memory for new client connection\n") ;
					allCC.Insert(cSock, pCC) ;
				}

				m_pLog->Log(_fn, "New client connection on socket %d\n", cSock) ;

				//	Initialize and oxygenate the connection
				pCC->Initialize(pLS, pSSL, ipbuf, cSock, cPort, nCliSeq++) ;

				if (pCC->m_OnConnect)
				{
					if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
						m_pLog->Log(_fn, "Client %d (sock %d): Issuing server hello\n", pCC->EventNo(), pCC->CliSocket()) ;
					pCC->m_OnConnect(pCC) ;
				}

				continue ;
			}

			//	Deal with connected clients here.

			if (eventAr[nC].events & EPOLLOUT)
			{
				//	Should not get this as we are not asking for these notifications
				cSock = eventAr[nC].data.fd ;

				m_pLog->Log(_fn, "WHAT??? write event on socket %d\n", cSock) ;
				continue ;
			}

			if (eventAr[nC].events & EPOLLIN)
			{
				//	At this point we have date on the socket to be read, at least in theory
				cSock = eventAr[nC].data.fd ;

				pCC = allCC[cSock] ;
				if (!pCC)
				{
					m_pLog->Log(_fn, "WHAT??? read event on socket %d but no connector!\n", cSock) ;
					continue ;
				}

				if (pCC->IsCliTerm())
					m_pLog->Log(_fn, "NOTE: Client %d (sock %d) has a read event after sending 0 byte packet\n", pCC->EventNo(), pCC->CliSocket()) ;

				//	Now because we are in edge-triggered mode, continue reading from the socket until we get -1
				//	m_pLog->Log(_fn, "Client %d (sock %d): Got %d bytes\n", pCC->EventNo(), pCC->CliSocket(), nRecv) ;
				for (;;)
				{
					nRecv = pCC->Recv(tbuf) ;
					m_pLog->Log(_fn, "Client %d (sock %d): Got %d bytes\n", pCC->EventNo(), pCC->CliSocket(), nRecv) ;

					if (nRecv <= 0)
						break ;
				}

				if (!pCC->SizeIn())
				{
					m_pLog->Log(_fn, "Client %d (sock %d): Final read - NO DATA\n", pCC->EventNo(), pCC->CliSocket()) ;
					if (nsNow > pCC->Expires())
					{
						m_pLog->Log(_fn, "Client %d (sock %d): Final read - NO DATA deleted\n", pCC->EventNo(), pCC->CliSocket()) ;
						pCC->Terminate() ;
					}
				}
				else
				{
					//	Data to be processed has come in
					//	if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
					//	{
					//		m_pLog->Log(_fn, "Client %d (sock %d): Triggered with %d bytes\n[\n", pCC->EventNo(), pCC->CliSocket(), pCC->SizeIn()) ;
					//		m_pLog->Out(pCC->InputZone()) ;
					//		m_pLog->Out("]\n") ;
					//	}

					s_queRequests.Push(pCC) ;
					pthread_cond_signal(&s_request_cond) ;
				}
			}
		}

		//	Kill off any inactive and out of date keep-alive connections - but only if there is a delay between polls
		if ((nsNow - nsThen) > 100000000)
		{
			if (_hzGlobal_Debug & HZ_DEBUG_SERVER)
				m_pLog->Log(_fn, "Purging ...\n") ;

			for (nX = 0 ; nX < allCC.Count() ; nX++)
			{
				pCC = allCC.GetObj(nX) ;

				if (!pCC)
					continue ;

				if (!pCC->CliSocket())
					continue ;

				if (pCC->_isxmit())
				{
					err = pCC->_xmit(tbuf) ;
					continue ;
				}

				/*
				if (pCC->State() == HZCONNEX_FAIL)
				{
					if (now < pCC->Expires())
						continue ;
					
					//pCC->StateDone() ;
					pCC->SetState(*_fn, HZCONNEX_DONE) ;
					if (m_pLog)
						m_pLog->Log(_fn, "Client (ev %d sock %d state %d) vgn %d: Failed, removed\n",
							pCC->EventNo(), pCC->CliSocket(), pCC->State(), pCC->IsVirgin() ? 1 : 0) ;
					pCC->Terminate() ;
					continue ;
				}
				*/

				if (!pCC->SizeIn() && (nsNow > pCC->Expires()))
				{
					//pCC->StateDone() ;
					//pCC->SetState(*_fn, HZCONNEX_DONE) ;
					//m_pLog->Log(_fn, "Client (ev %d sock %d state %d) vgn %d: Inactive, removed\n",
					//	pCC->EventNo(), pCC->CliSocket(), pCC->State(), pCC->IsVirgin() ? 1 : 0) ;
					m_pLog->Log(_fn, "Client (ev %d sock %d) vgn %d: Inactive, removed\n", pCC->EventNo(), pCC->CliSocket(), pCC->IsVirgin() ? 1 : 0) ;
					pCC->Terminate() ;
				}
			}
		}
	}

	threadLog("%s. Shutdown\n", *_fn) ;
}
