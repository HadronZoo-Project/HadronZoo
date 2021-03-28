//
//	File:	hzTcpClient.h
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

#ifndef hzTcpClient_h
#define hzTcpClient_h

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>

#include "hzChain.h"
#include "hzIpaddr.h"

/*
**	Definitions
*/

class	hzTcpClient
{
	//	Category:	Internet
	//
	//	The hzTcpClient manages a TCP client connection to a local or remote service. This may be to any legal port and opperate using any protocol.

	SOCKADDRIN	m_SvrAddr ;				//	address of server
	SSL*		m_pSSL ;				//	SSL entity for SSL connections
	HOSTENT*	m_pHost ;				//	the server
	hzString	m_Hostname ;			//	Hostname (for diagnostics)
	hzIpaddr	m_Ipa ;					//	IP address of host
	uint32_t	m_nTimeoutS ;			//	Send timeout
	uint32_t	m_nTimeoutR ;			//	Recv timeout
	uint32_t	m_nBytesRecv ;			//	Bytes received
	uint32_t	m_nSock ;				//	Client socket given by server
	uint32_t	m_nPort ;				//	Port
	bool		m_bDebug ;				//	Debug mode on (threadLog)
	bool		m_bSSL ;				//	Connection will be SSL
	bool		m_bError ;				//	Send/Recv failed. Must re-connect
	char		m_Buf[HZ_MAXPACKET+4] ;	//	Internal buffer for I/O

public:
	hzTcpClient	(void)
	{
		m_pSSL = 0 ;
		m_pHost = 0 ;
		m_nSock = 0 ;
		m_nPort = 0 ;
		m_nTimeoutS = m_nTimeoutR = 0 ;
		m_bDebug = false ;
		m_bSSL = false ;
		m_bError = false ;
	}

	~hzTcpClient	(void)
	{
		Close() ;
	}

	void		SetDebug	(bool bDebug)	{ m_bDebug = bDebug ; }
	uint32_t	Sock		(void)			{ return m_nSock ; }
	char*		GetBuffer	(void)			{ return m_Buf ; }

	hzEcode		ConnectStd	(const char* hostname, uint32_t port, uint32_t timeoutRecv = 0, uint32_t timeoutSend = 0) ;
	hzEcode		ConnectIP	(const hzIpaddr& host, uint32_t port, uint32_t timeoutRecv = 0, uint32_t timeoutSend = 0) ;
	hzEcode		ConnectSSL	(const char* hostname, uint32_t Port, uint32_t timeoutRecv = 0, uint32_t timeoutSend = 0) ;
	hzEcode		ConnectLoc	(uint32_t Port) ;

	//hzEcode		Init		(const char* cpHostname, uint32_t Port, uint32_t nTimeoutR = 0, uint32_t nTimeoutS = 0, bool bSSL = false) ;
	//hzEcode		Connect		(const hzString& Host, uint32_t nPort)		{ return Connect(*Host, nPort) ; }
	//hzEcode		Connect		(const hzIpaddr& Host, uint32_t nPort, uint32_t nTimeoutR = 0, uint32_t nTimeoutS = 0)
	//	{ return Connect(*Host.Str(), nPort, nTimeoutR, nTimeoutS) ; }

	hzEcode		SetSendTimeout	(uint32_t nSecs) ;
	hzEcode		SetRecvTimeout	(uint32_t nSecs) ;

	void		Show	(hzChain& Z) ;

	hzEcode		Send	(const void* pData, uint32_t nLen) ;
	hzEcode		Send	(const hzChain& C) ;

	hzEcode		Recv	(void* pData, uint32_t& nRecv, uint32_t nMax) ;
	hzEcode		Recv	(hzChain& C) ;

	void		Close	(void) ;
} ;

#endif	//	hzTcpClient_h
