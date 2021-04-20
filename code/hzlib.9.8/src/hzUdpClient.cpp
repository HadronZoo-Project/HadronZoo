//
//	File:	hzUdpClient.cpp
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
//	Implimentation of the generic UDP client class, hzUdpClient.
//

#include <iostream>

#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#include "hzProcess.h"
#include "hzUdpClient.h"

hzEcode	hzUdpClient::Connect	(const hzString& Hostname, uint32_t nPort, bool bLocal)
{
	//	There is of course, no such thing as a UDP connection in the same way there are TCP connections. However the host must still be looked up and a socket
	//	obtained before one can send and receive data so in terms of function call sequence, the two forms of communication are conceptually similar.
	//
	//	It is the role of this function to do the host lookup and obtain the datagram socket.
	//
	//	Arguments:	1)	Hostname	The server name or IP address (as hzString)
	//				2)	nPort		The port number
	//				3)	bLocal		Optional local flag (if host is this machine)
	//
	//	Returns:	E_ARGUMENT		If either the hostname or port is not specified.
	//				E_DNS_NOHOST	If the domain does not exist
	//				E_DNS_NODATA	If the domain exists but not a service
	//				E_DNS_FAILED	If the domain has invalid settings
	//				E_DNS_RETRY		If the DNS is busy
	//				E_NOSOCKET		If the socket could not be created or timeouts set
	//				E_OK			If host established and socket obtained

	_hzfunc("hzUdpClient::Connect") ;

	timeval	tv ;	//	Socket timer

	tv.tv_sec = 20 ;
	tv.tv_usec = 0 ;

	/*
	**	Initialize
	*/

	m_Hostname = Hostname ;
	m_nPort = nPort ;

	if (!Hostname)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Hostname not set") ;

	m_pHost = gethostbyname(*m_Hostname) ;
	if (!m_pHost)
	{
		if (h_errno == TRY_AGAIN)		return E_DNS_RETRY ;
		if (h_errno == HOST_NOT_FOUND)	return E_DNS_NOHOST ;
		if (h_errno == NO_RECOVERY)		return E_DNS_FAILED ;

		if (h_errno == NO_DATA || h_errno == NO_ADDRESS)
			return E_DNS_NODATA ;

		return hzerr(_fn, HZ_ERROR, E_DNS_FAILED, "Could not resolve hostname") ;
	}

	if (m_nPort == 0)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Port not set") ;

	m_SvrLen = sizeof(m_SvrAddr) ;

	/*
	**	Set up socket
	*/

	memset(&m_SvrAddr, 0, sizeof(m_SvrAddr)) ;
	m_SvrAddr.sin_family = AF_INET ;
	memcpy(&m_SvrAddr.sin_addr, m_pHost->h_addr, m_pHost->h_length) ;
	m_SvrAddr.sin_port = htons(m_nPort) ;

	if ((m_nSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Could not create client socket") ;

	if (setsockopt(m_nSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Could not set send timeout on UDP client socket") ;

	if (setsockopt(m_nSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Could not set recv timeout on UDP client socket") ;

	return E_OK ;
}

hzEcode	hzUdpClient::SendPkt	(hzPacket* pData, uint32_t nLen)
{
	//	Purpose:	Write buffer content to a socket
	//
	//	Arguments:	1)	pData	The buffer (void*)
	//				2)	nLen	Number of bytes to send
	//
	//	Returns:	E_NOSOCKET	If no socket has been set by Connect()
	//				E_ARGUMENT	If the data is not provided
	//				E_RANGE		If the number of bytes to send exceeds packet size (1460 bytes)
	//				E_SENDFAIL	If the send operation fails. In this event the connection is closed.
	//				E_OK		If the operation is successfull.

	_hzfunc("hzUdpClient::Send(void*,uint32_t)") ;

	if (m_nSock == 0)
		return E_NOSOCKET ;

	if (!pData)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Nothing to send") ;

	if (nLen > HZ_MAXPACKET)
		return hzerr(_fn, HZ_ERROR, E_RANGE, "Length of message must be between 1 and 1460 bytes") ;

	//	send a message
	if (sendto(m_nSock, pData->m_data, nLen, 0, (struct sockaddr*) &m_SvrAddr, m_SvrLen) < 0)
	{
		close(m_nSock) ;
		m_nSock = 0 ;
		return hzerr(_fn, HZ_ERROR, E_SENDFAIL, "Could not send to host (%s) on port %d", *m_Hostname, m_nPort) ;
	}

	threadLog("%s. Client sock %d sends msg of %d bytes [%s]\n", *_fn, m_nSock, nLen, pData->m_data) ;

	return E_OK ;
}

hzEcode	hzUdpClient::SendChain	(hzChain& C)
{
	//	Write supplied chain content to a socket
	//
	//	Arguments:	1)	C	The chain to send. No size indicator is needed as whole chain is sent.
	//
	//	Returns:	E_NOSOCKET	If the connection is not open
	//				E_NODATA	If the supplied chain contains no data
	//				E_SENDFAIL	If the send operation fails. In this event the connection is closed.
	//				E_OK		If the operation is successfull.

	_hzfunc("hzUdpClient::Send(hzChain&)") ;

	chIter		ci ;		//	To iterate input chain
	char*		i ;			//	To populate buffer
	uint32_t	nSend ;		//	Bytes in buffer to send
	uint32_t	nSofar ;	//	Bytes sent so far

	if (m_nSock == 0)
		return E_NOSOCKET ;
	if (!C.Size())
		return E_NODATA ;

	//	Init
	ci = C ;
	nSofar = 0 ;

	//	Read from chain to populate rest of buffer with start of message
	for (nSend = 0, i = m_pack.m_data ; !ci.eof() && nSend < HZ_MAXPACKET ; *i = *ci, i++, nSend++, ci++) ;
	nSofar = nSend ;

	//	Send buffer content to socket
	if (sendto(m_nSock, m_pack.m_data, nSend, 0, (struct sockaddr*) &m_SvrAddr, m_SvrLen) < 0)
	{
		close(m_nSock) ;
		m_nSock = 0 ;
		return hzerr(_fn, HZ_ERROR, E_SENDFAIL, "Could not send to host (%s) on port %d", *m_Hostname, m_nPort) ;
	}

	//	Repeat read and send steps for rest of message
	for (; nSofar < C.Size() ;)
	{
		for (nSend = 0, i = m_pack.m_data ; !ci.eof() && nSend < HZ_MAXPACKET ; *i = *ci, nSend++, i++, ci++) ;
		nSofar += nSend ;

		if (!nSend)
			break ;

		if (sendto(m_nSock, m_pack.m_data, nSend, 0, (struct sockaddr*) &m_SvrAddr, m_SvrLen) < 0)
		{
			close(m_nSock) ;
			m_nSock = 0 ;
			return hzerr(_fn, HZ_ERROR, E_SENDFAIL, "Could not send to host (%s) on port %d", *m_Hostname, m_nPort) ;
		}
	}

	return E_OK ;
}

hzEcode	hzUdpClient::RecvPkt	(hzPacket* pData, uint32_t& nRecv)
{
	//	Read a packet from the socket into the client buffer.
	//
	//	Arguments:	1)	pData	The packet recepticle
	//				2)	nRecv	Reference to number of bytes received
	//
	//	Returns:	E_ARGUMENT	If the packet recipticle is not supplied
	//				E_NOSOCKET	If the connection has been closed
	//				E_NODATA	If no data was recieved
	//				E_RECVFAIL	If the socket read operation fails
	//				E_OK		If operation successfull

	_hzfunc("hzUdpClient::Recv(buf,recv,max)") ;

	if (!pData)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No IP recepticle supplied") ;
	if (m_nSock == 0)
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Client has no connection") ;

	//	Get response
	//nRecv = recvfrom(m_nSock, pData->m_data, HZ_MAXPACKET, 0, (struct sockaddr*) &m_SvrAddr, &m_SvrLen) ;
	nRecv = recvfrom(m_nSock, pData->m_data, HZ_MAXPACKET, 0, (SOCKADDR*) &m_SvrAddr, &m_SvrLen) ;

	if (nRecv == 0)
		return E_NODATA ;

	if (nRecv < 0)
	{
		close(m_nSock) ;
		m_nSock = 0 ;
		return hzerr(_fn, HZ_ERROR, E_RECVFAIL, "Could not recv from server (%s) on port %d", *m_Hostname, m_nPort) ;
	}

	pData->m_data[nRecv] = 0 ;

	threadLog("%s. Client sock %d recv %d bytes\n", *_fn, m_nSock, nRecv) ;
	return E_OK ;
}

hzEcode	hzUdpClient::RecvChain	(hzChain& C)
{
	//	Reads one or more packets from the socket into the supplied chain until a zero length read occurs.
	//
	//	Argument:	C	The chain to populate
	//
	//	Returns:	E_NOSOCKET	If the connection has been closed
	//				E_RECVFAIL	If the socket read operation fails
	//				E_OK		If operation successfull

	_hzfunc("hzUdpClient::Recv(hzChain&)") ;

	uint32_t	nRecv ;		//	Bytes recieved in IP packet

	//	CHECK

	C.Clear() ;

	if (m_nSock == 0)
		return hzerr(_fn, HZ_ERROR, E_NOSOCKET, "Client has no connection") ;

	if ((nRecv = recvfrom(m_nSock, m_pack.m_data, HZ_MAXPACKET, 0, (struct sockaddr*) &m_SvrAddr, &m_SvrLen)) < 0)
	{
		close(m_nSock) ;
		m_nSock = 0 ;
		return hzerr(_fn, HZ_ERROR, E_RECVFAIL, "Could not recv from server (%s) on port %d", *m_Hostname, m_nPort) ;
	}

	C.Append(m_pack.m_data, nRecv) ;

	for (;;)
	{
		nRecv = recvfrom(m_nSock, m_pack.m_data, HZ_MAXPACKET, 0, (struct sockaddr*) &m_SvrAddr, &m_SvrLen) ;

		if (nRecv < 0)
		{
			close(m_nSock) ;
			m_nSock = 0 ;
			return hzerr(_fn, HZ_ERROR, E_RECVFAIL, "Could not recv from server (%s) on port %d", *m_Hostname, m_nPort) ;
		}

		if (nRecv == 0)
			break ;

		C.Append(m_pack.m_data, nRecv) ;
	}

	return E_OK ;
}
