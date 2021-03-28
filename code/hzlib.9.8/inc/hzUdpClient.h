//
//	File:	hzUdpClient.h
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

#ifndef hzUdpClient_h
#define hzUdpClient_h

#include "hzChain.h"
#include "hzIpaddr.h"

class	hzUdpClient
{
	//	Category:	Internet
	//
	//	The hzUdpClient class enables an application to act as a client using UDP.

	hzPacket	m_pack ;			//	IP packet recepticle
	SOCKADDRIN	m_SvrAddr ;			//	address of server
	SOCKADDRIN	m_CliAddr ;			//	address of client
	HOSTENT*	m_pHost ;			//	the server
	socklen_t	m_SvrLen ;			//	Length of SvrAddr in bytes
	hzString	m_Hostname ;		//	Server client is connecting to
	uint32_t	m_nSock ;			//	Connected socket
	uint32_t	m_nPort ;			//	Port on server to connect to

public:
	hzUdpClient	(void)
	{
		m_pHost = 0 ;
		m_nSock = 0 ;
		m_nPort = 0 ;
	}

	~hzUdpClient	(void)
	{
	}

	char*	GetCbuf	(void)	{ return m_pack.m_data ; }				//	Return the dedicated IP packet recepticle as a char*
	uchar*	GetUbuf	(void)	{ return (uchar*) m_pack.m_data ; }		//	Return the dedicated IP packet recepticle as a uchar*

	hzEcode	Connect		(const hzString& Hostname, uint32_t nPort, bool bLocal = false) ;
	hzEcode	SendPkt		(hzPacket* pData, uint32_t nSize) ;
	hzEcode	SendChain	(hzChain& C) ;
	hzEcode	RecvPkt		(hzPacket* pData, uint32_t& nRecv) ;
	hzEcode	RecvChain	(hzChain& Z) ;
} ;

#endif	//	hzUdpClient_h
