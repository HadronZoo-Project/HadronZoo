//
//	File:	hzIpServer.h
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

//	Class:	hzIpServer
//
//	Category:	Internet
//
//	The hzIpServer class is intended as a general purpose TCP based server. There may only be one instance within any given server program but this can listen on multiple ports on
//	both TCP and UDP. hzIpServer operation is by means of epoll in edge-triggered mode.
//
//	Two further classes are involved, hzTcpListen and hzIpConnex. These are as follows:-
//
//		1) hzTcpListen	Is a listening socket which handles clients trying to connect. The hzIpServer will have one hzTcpListen instance for each port the program listens on. These
//						are all created during hzIpServer initialization and remain in place while hzIpServer remains in place (runtime duration).
//
//		2) hzIpConnex	Is an established client connection, created when a client connects and destroyed when the client disconnects. hzIpConnex has methods of Recv() and Send(),
//						through which all data in and out between the client and host program passes.
//
//	The hzIpServer class has four functions that add listening ports as follows:-
//
//		1) AddPortTCP	Used for any TCP client EXCEPT HTTP
//		2) AddPortHTTP	Used for HTTP conections ONLY
//		3) AddPortSess	Adds a TCP listening socket for client connections that are to be managed by a user defined thread handler function.
//		4) AddPortUDP	Adds a UDP socket
//
//	All the above functions set an inactivity timeout, the port number to listen on, the maximum number of simultaneous connections and state if SSL will be used. More importantly,
//	these functions assign one or more application specific 'call-back functions', which will do all the work. In more detail we have:-
//
//	AddPortTCP has three function pointer arguments namely:-
//
//		hzTcpCode (*OnIngress)(hzChain&, hzIpConnex*)	OnIngress is compulsory and will be called each time data has been sent in by the client. This is usually a complex function
//														covering multiple connection states. It is often implimented as a pair of functions with OnIngress aggregating incoming data
//														until it constitutes a complete message. Once a complete message has been collected, OnIngress then calls a second function
//														to process the message.
//
//		hzTcpCode (*OnConnect)(hzIpConnex*)				OnConnect is optional and is usually for where the server is expected to speak first (e.g. in SMTP and POP3 connections).
//
//		hzTcpCode (*OnDisconn)(hzIpConnex*)				OnDisconn is optional and is usually for tidying up after a connection has completed. 
//
//	HTTP connections are treated as a special case of TCP connections because there are HadronZoo in-built functions to handle the protocol. Upon initialization, a HTTP connection
//	creates an instance of hzHttpEvent (defined in hzHttpServer.h). A hzHttpEvent is a HTTP request and as data comes in on the connection, the hzHttpEvent is populated by means of
//	the function hzHttpEvent::ProcessEvent(). The OnIngress function is supplanted by an in-built function HandleHttpMsg which calls hzHttpEvent::ProcessEvent() until a complete
//	message has been assempled and then calls the application specific function supplied by AddPortHTTP. This must return a hzTcpCode and take a pointer to a hzHttpEvent as its
//	argument.
//
//	Where connections are to be handled by a separate thread, the handler must return void* and accept void* as the argument. AddPortSess has one function pointer argument namely
//	void* (*OnSession)(void*).
//
//	AddPortUDP has the same OnIngress, OnConnect and OnDisconn function pointer arguments that AddPortTCP has, in spite of the fact that UDP is a connectionless protocol. As with
//	AddPortTCP, OnConnect and OnDisconn are optional and their presence would depend on whether or not OnIngress required state maintenence of any form.

#ifndef hzIpServer_h
#define hzIpServer_h

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>

#include "hzTmplList.h"
#include "hzTmplVect.h"
#include "hzTmplMapS.h"
#include "hzChain.h"
#include "hzIpaddr.h"

/*
**	Definitions
*/

enum	hzSvrStatus
{
	//	Category:	Internet
	//
	//	Server status

	SERVER_OFFLINE,			//	Server offline or otherwise not accepting connections
	SERVER_ONLINE,			//	Server operating normally
	SERVER_SHUTDOWN			//	Server continues to operate but won't issue new sessions
} ;

enum	hzCliStatus
{
	//	Category:	Internet
	//
	//	Client status

	CLIENT_STATE_NONE	= 0,		//	Client is created in this state
	CLIENT_INITIALIZED	= 0x0001,	//	Client accepted but no data in as yet
	CLIENT_HELLO		= 0x0002,	//	Client has been sent a hello
	CLIENT_READING		= 0x0004,	//	Client can read
	CLIENT_READ_WHOLE	= 0x0008,	//	A whole message has been read (may be more to come)
	CLIENT_HANGUP		= 0x0010,	//	An epoll HANGUP condition has been detected
	CLIENT_TERMINATION	= 0x0020,	//	The client has sent a 0-byte formal termination packet
	CLIENT_WRITING		= 0x0040,	//	A response has been formulated and is in the process of being sent to the client
	CLIENT_WRITE_WHOLE	= 0x0080,	//	The response has been written to the clinet
	CLIENT_BAD			= 0x0100	//	Server has deemed the client to be bad and will not send a response
} ;

enum	hzTcpCode
{
	//	Category:	Internet
	//
	//	TCP session return codes to direct if session is to be terminated or kept alive

	TCP_TERMINATE,			//	Terminate connection
	TCP_KEEPALIVE,			//	Keep connection alive for next transmission even if current transmission has completed
	TCP_INCOMPLETE,			//	Keep connection alive as transmission is incomplete
	TCP_INVALID				//	Message processing failed for syntax/protocol reasons. Connection to terminate
} ;

#define HZMAX_SERVNAMLEN	64		//	Max domain name?
#define HZMAX_IPADDRLEN		16		//	IPV-4 Address length

/*
**	The ListeningSocket class
*/

#define	HZ_LISTEN_SECURE	0x01	//	Connections under the listening socket must use SSL
#define	HZ_LISTEN_INTERNET	0x02	//	Connections under the listening socket must use AF_INET (access from outside) else use AF_UNIX (local clients only)
#define	HZ_LISTEN_TCP		0x04	//	Connections under the listening socket must use TCP
#define	HZ_LISTEN_HTTP		0x08	//	Connections under the listening socket must use HTTP
#define	HZ_LISTEN_UDP		0x10	//	Connections under the listening socket must use UDP
#define	HZ_LISTEN_SESSION	0x20	//	Connections under the listening socket must use separate thread function

class	hzIpConnex ;
class	hzHttpEvent ;

class	hzTcpListen
{
	//	Category:	Internet
	//
	//	The hzTcpListen class forms a listening socket and maintains connections to the port the socket is listening on. It is an
	//	important part of the hzIpServer repotoire but has no use outside this environment.

	hzLogger*	m_pLog ;				//	Log channel
	SSL*		m_pSSL ;				//	SSL controller

	SOCKADDRIN	m_Address ;				//	IP Addres of socket
	uint32_t	m_nSocket ;				//	The actual listening socket
	uint32_t	m_nTimeout ;			//	Timeout appllied to all connections to this port
	uint16_t	m_nPort ;				//	Port number server listens on
	uint16_t	m_nMaxConnections ;		//	Max number of simultaneous TCP connections on this port
	uint16_t	m_nCurConnections ;		//	Current number of simultaneous TCP connections on this port
	uint16_t	m_bOpflags ;			//	Operational flags (HZ_LISTEN_SECURE | HZ_LISTEN_INTERNET | HZ_LISTEN_UDP)

	//	User supplied function. Each end point must have a 'OnIngress'
	//	function to handle messages comming in on the specific port. Each
	//	port can have a different handler or the same handler can be used
	//	for more than one or all ports the server listens too.

public:
	hzTcpCode	(*m_OnIngress)(hzChain& Input, hzIpConnex* pCx) ;	//	To be called as soon as input exists.
	hzTcpCode	(*m_OnConnect)(hzIpConnex* pCx) ;					//	For when protocol requires server goes first.
	hzTcpCode	(*m_OnDisconn)(hzIpConnex* pCx) ;					//	For extra tidying up on dissconnection.
	void*		(*m_OnSession)(void*) ;								//	Function that will handle session in it's own thread
	void*		m_appFn ;											//	Cast to the specific callback function (eg HTTP)

	hzTcpListen	(hzLogger* plog)
	{
		m_nPort = 0 ;
		m_nSocket = 0 ;
		m_nTimeout = 0 ;
		m_nMaxConnections = 0 ;
		m_nCurConnections = 0 ;
		m_OnIngress = 0 ;
		m_OnConnect = 0 ;
		m_OnSession = 0 ;
		m_appFn = 0 ;
		m_pLog = plog ;
		m_pSSL = 0 ;

		m_bOpflags = 0 ;
		//m_bSecure = 0 ;
		//m_bInternet = 0 ;
	}

	~hzTcpListen	(void)
	{
	}

	//	Initialization

	hzEcode	Init
	(
		hzTcpCode	(*OnIngress)(hzChain&, hzIpConnex*),	//	App's packet handler function
		hzTcpCode	(*OnConnect)(hzIpConnex*),				//	App's server hello func (if used)
		hzTcpCode	(*OnDisconn)(hzIpConnex*),				//	App's server hello func (if used)
		void*		(*OnSession)(void*),					//	App's session hendler (if used)
		uint32_t	nTimeout,								//	Timeout applied to all connections on port
		uint32_t	nPort,									//	Port application will listen on
		uint32_t	nMaxClients,							//	Max number of simultaneous clients
		uint32_t	bOpflags								//	Operational flags (HZ_LISTEN_SECURE | HZ_LISTEN_INTERNET | HZ_LISTEN_UDP)
	) ;

	hzEcode	Activate	(void) ;

	//	Get functions
	hzLogger*	GetLogger			(void) const	{ return m_pLog ; }
	uint32_t	GetPort				(void) const	{ return m_nPort ; }
	uint32_t	GetSocket			(void) const	{ return m_nSocket ; }
	uint32_t	GetTimeout			(void) const	{ return m_nTimeout ; }
	uint32_t	GetMaxConnections	(void) const	{ return m_nMaxConnections ; }
	uint32_t	GetCurConnections	(void) const	{ return m_nCurConnections ; }
	bool		UseUDP				(void) const	{ return m_bOpflags & HZ_LISTEN_UDP ? true : false ; }
	bool		UseSSL				(void) const	{ return m_bOpflags & HZ_LISTEN_SECURE ? true : false ; }
	uint16_t	Opflags				(void) const	{ return m_bOpflags ; }
} ;

/*
**	Connection Info class
*/

class	hzProcInfo
{
	//	Category:	System
	//
	//	The hzProcInfo or 'process data' class is used to pass process information in cases where a client connection needs to be handled in a separate thread.
	//	A thread handler function nessesarily have a single void* argument. This is not enough to the client IP address and socket, and if applicable, the SSL
	//	structure. This information is used to populate a hzProcInfo instance and a pointer to this is passed instead. The thread handler function will cast it
	//	back to hzProcInfo and can then extract the information about the client as required.
 
	SSL*		m_pSSL ;			//	SSL session info (if applicable)
	hzIpaddr	m_Ipa ;				//	IP address
	uint32_t	m_Socket ;			//	The client socket

public:
	hzProcInfo	(void)
	{
		m_pSSL = 0 ;
		m_Socket = 0 ; 
	}

	hzProcInfo	(SSL* pSSL, uint32_t nSock, hzIpaddr ipa)
	{
		m_pSSL = pSSL ;
		m_Socket = nSock ;
		m_Ipa = ipa ;
	}

	~hzProcInfo	(void)
	{
		if (m_pSSL)
			SSL_free(m_pSSL) ;
		close(m_Socket) ;
	}

	void	SetParams	(SSL* pSSL, uint32_t nSock, hzIpaddr ipa)
	{
		m_pSSL = pSSL ;
		m_Socket = nSock ;
		m_Ipa = ipa ;
	}

	hzIpaddr&	Ipaddr	(void)	{ return m_Ipa ; }
	uint32_t	Socket	(void)	{ return m_Socket ; }
} ;


class	hzIpConnInfo
{
	//	Category:	Internet
	//
	//	The hzIpConnInfo class is the pure virtual base class for any form of session class. The session classes are application specific devices that maintain
	//	state.

public:
	virtual	~hzIpConnInfo	(void)	{}
} ;

/*
**	Conections (Inbound and outbound)
*/

class	hzPktQue
{
	//	Category:	Internet
	//
	//	The hzPktQue class queues instances of hzPacket on behalf of the hzIpConnex (client connection) class. Each hzIpConnex instance has a pair of hzPktQue
	//	instances, one for incoming requests and the other for outgoing responses. The class has conceptual similarities to both the hzChain class and the hzQue
	//	class template. The packets form a chain which is always appended and read from the begining. However, with hzChain the unit of data is the byte so only
	//	the last block may be left partially filled. Also the aggregation process is expected to run to completion before the chain is iterated. With hzPktQue,
	//	partially filled packets are routine and both the write and iteration processes are considered ongoing, with packets being removed after they have been
	//	processed.

public:
	hzPacket*	m_pStart ;				//	The first IP packet in the que
	hzPacket*	m_pFinal ;				//	The last IP packet in the que
	uint32_t	m_nSize ;				//	Current size in bytes
	uint32_t	m_nSeq ;				//	Packet sequence

	hzPktQue	(void)
	{
		m_pStart = m_pFinal = 0 ;
		m_nSize = m_nSeq = 0 ;
	}

	uint32_t	Size	(void) const	{ return m_nSize ; }
	hzPacket*	Peek	(void) const	{ return m_pStart ; }

	hzEcode		Push	(hzPacket& pkt) ;
	hzEcode		Push	(const hzChain& Z) ;

	hzEcode		Pull	(void) ;
	hzEcode		Clear	(void) ;
} ;

class	hzIpConnex
{
	hzChain			m_Input ;			//	Incomming message chain
	hzChain::Iter	m_MsgStart ;		//	For iteration of pipelined requests
	hzLogger*		m_pLog ;			//	Log channel
	hzIpConnInfo*	m_pInfo ;			//	Connection specific information
	SSL*			m_pSSL ;			//	SSL session info
	hzPktQue		m_Outgoing ;		//	Outgoing message stream

	uint64_t		m_ConnExpires ;		//	Nanosecond Epoch expiry
	uint64_t		m_nsAccepted ;		//	Nanosecond Epoch connection accepted
	uint64_t		m_nsRecvBeg ;		//	Nanosecond Epoch first packet of message
	uint64_t		m_nsRecvEnd ;		//	Nanosecond Epoch request considered complete
	uint64_t		m_nsSendBeg ;		//	Nanosecond Epoch response transmission began
	uint64_t		m_nsSendEnd ;		//	Nanosecond Epoch response transmission ended
	SOCKADDRIN		m_CliAddr ;			//	IP Addres of client socket
	socklen_t		m_nCliLen ;			//	Length of socket address
	hzIpaddr		m_ClientIP ;		//	IP address of client
	uint32_t		m_nSock ;			//	Socket of client (note that socket of connection cannot be -1) 
	uint32_t		m_nMsgno ;			//	Event/Message number
	uint32_t		m_nGlitch ;			//	Extent of incomplete write
	uint32_t		m_nStart ;			//	Start position of current incomming message within chain
	uint32_t		m_nTotalIn ;		//	Total size of outgoing response
	uint32_t		m_nTotalOut ;		//	Total size of outgoing response
	uint32_t		m_nExpected ;		//	Expected size of incomming request
	uint32_t		m_bState ;			//	Client state
	uint16_t		m_nPort ;			//	Incomimg port
	uint16_t		m_bListen ;			//	Operational flags from listening socket (HZ_LISTEN_SECURE | HZ_LISTEN_INTERNET | HZ_LISTEN_UDP)
	char			m_ipbuf[48] ;		//	Text form of IP address

public:
	hzChain		m_Track ;				//	Used by application to report on progress during a session. Logged when connection terminated.
	void*		m_appFn ;				//	Application message event handler
	void*		m_pEventHdl ;			//	HTTP Event instance

	hzTcpCode	(*m_OnIngress)(hzChain& Input, hzIpConnex* pCx) ;	//	Required: Function to handle messages comming in on the specific port.
	hzTcpCode	(*m_OnConnect)(hzIpConnex* pCx) ;					//	Optional: Called on connection e.g. Server hello.
	hzTcpCode	(*m_OnDisconn)(hzIpConnex* pCx) ;					//	Optional: Called on disconnection for any tidying up.

	//	Constructor and destructor
	hzIpConnex	(hzLogger* pLog) ;
	~hzIpConnex	(void) ;

	hzEcode	Initialize	(hzTcpListen* pLS, SSL* pSSL, const char* cpIPAddr, uint32_t cliSock, uint32_t cliPort, uint32_t eventNo) ;

	void	Oxygen	(void)	{ m_ConnExpires = RealtimeNano() ; m_ConnExpires += 25000000000 ; }	//	Adds 25 seconds to the time to live
	void	Hypoxia	(void)	{ m_ConnExpires = 0 ; }												//	Expires the connection

	void	Terminate	(void) ;	//	Terminate connection

	//	Set functions
	void	SetInfo	(hzIpConnInfo* pInfo)	{ m_pInfo = pInfo ; }

	//	Get functions
	hzIpConnInfo*	GetInfo	(void) const	{ return m_pInfo ; }

	hzLogger*	GetLogger	(void) const	{ return m_pLog ; }
	hzChain&	InputZone	(void)			{ return m_Input ; }
	hzIpaddr	ClientIP	(void) const	{ return m_ClientIP ; }
	const char*	GetClientIP	(void) const	{ return m_ipbuf ; }
	uint64_t	Expires		(void) const	{ return m_ConnExpires ; }
	uint64_t	TimeAcpt	(void) const	{ return m_nsRecvBeg - m_nsAccepted ; }
	uint64_t	TimeRecv	(void) const	{ return m_nsRecvEnd - m_nsRecvBeg ; }
	uint64_t	TimeProc	(void) const	{ return m_nsSendBeg - m_nsRecvEnd ; }
	uint64_t	TimeXmit	(void) const	{ return m_nsSendEnd - m_nsSendBeg ; }
	uint32_t	EventNo		(void) const	{ return m_nMsgno ; }
	uint32_t	CliSocket	(void) const	{ return m_nSock ; }
	uint32_t	CliPort		(void) const	{ return m_nPort ; }
	uint32_t	SizeIn		(void) const	{ return m_Input.Size() - m_nStart ; }
	uint32_t	TotalIn		(void) const	{ return m_nTotalIn ; }
	uint32_t	TotalOut	(void) const	{ return m_nTotalOut ; }
	bool		IsVirgin	(void) const	{ return m_bState == CLIENT_INITIALIZED ; }
	bool		IsCliTerm	(void) const	{ return m_bState & CLIENT_TERMINATION ; }
	bool		IsCliBad	(void) const	{ return m_bState & CLIENT_BAD ; }
	bool		_isxmit		(void) const	{ return m_Outgoing.m_pStart ? true : false ; }

	//	Operational functions
	int32_t		Recv		(hzPacket& Buf) ;
	hzEcode		SendData	(const hzChain& Hdr, const hzChain& Body) ;
	hzEcode		SendData	(const hzChain& Z) ;
	void		SendKill	(void) ;
	int32_t		_xmit		(hzPacket& buf) ;

	//	Message size expectations
	void		ExpectSize	(uint32_t nBytes)	{ m_nExpected = nBytes ; }
	uint32_t	ExpectSize	(void)	{ return m_nExpected ; }
	bool		MsgComplete	(void)	{ return m_nExpected && m_Input.Size() >= (m_nExpected + m_nStart) ? true : false ; }

	bool	MsgReady	(void)
	{
		//	If a message has an expected size this condition will only be true if the input so far has reached or exceeded this size. In the common
		//	scenario where an incoming message has a header stating the length, the expected size starts at zero and this function returns true. The
		//	input is then provisionally processed and this will establish the expected size. This function will then not return true until the whole
		//	message in in.

		if (!m_nExpected)
			return true ;

		if (m_Input.Size() >= (m_nExpected + m_nStart))
			return true ;
		return false ;
	}
} ;

/*
**	The SERVER itself
*/

class	hzIpServer
{
	//	Category:	Internet
	//
	//	The singleton server class hzIpServer will have a listening socket (hzTcpListen instance) for every port it listens on. Client connections are accepted
	//	and initialized by the applicable hzTcpListen instance which dictates much of how the connections operate. There are two fundamental methods: Either the
	//	connection is handled by a dedicated function running in a separate thread; OR the connection is placed in the epoll pool. The former method is regarded
	//	as lagacy (Older versions of the Epistula email server used the method to handle SMTP and POP3 clients). The latter is the recommended method and under
	//	this regime, each client connections is manifest as a hzIpConnex instance.
	//
	//	During initialization of the hzIpServer, the hzTcpListen instances are supplied with two function pointers. As hzIpConnex instances are created, these
	//	function pointers are copied from the applicable hzTcpListen instance. This arrangement allows the functions to be defined within the application should
	//	this be expiedient. In both hzTcpListen and hzIpConnex the function pointers are named m_OnIngress and m_OnConnect. Of these, the latter generally only
	//	applies to protocols such as SMTP in which the server sends the first message to greet the client. For protocols such as HTTP, m_OnConnect will normally
	//	be NULL. Should it be considered desirable to perform checks prior to recieving packets, then an m_OnConnect function can be defined in the application
	//	for any protocol. The m_OnIngress function is compulsory for all protocols as this is expected to process client requests and formulate responses.
	//
	//	The m_OnIngress function will run every time packets arrive from a connected client UNLESS there is an expected message size and the packets recieved so
	//	far do not total the expected size. The initial expected size is always zero so the first tranche of packets to arrive will always trigger a m_OnConnect
	//	call. As an example, in the HTTP protocol the HTTP header will commonly be all there is to a HTTP request. If form data is submitted however, then there
	//	will be more but the expected size of what follows, will be stated in the header. Where a large file is to be uploaded, it may take several tranches of
	//	packets before the message reaches the expected size. On the other hand, the m_OnIngress function can be designed to take whatever packets have arrived
	//	and file them imediately. As it happens, the standard HadronZoo library function for handling HTTP connections sets an expected size.
	//
	//	In all cases, the m_OnIngress function returns a value which serves as directive as to what should happen to the client connection. The possible return
	//	values are as follows:-
	//
	//		1)	TCP_COMPLETE	//	The message was successfully put in the queue and hzIpConnex must close the socket as soon as it is sent.
	//		2)	TCP_KEEPALIVE	//	The message was successfully put in the queue but other request messages may be forthcomming.
	//		3)	TCP_INVALID		//	The handler could not process the request and directs that the connection be closed after the error response is sent.
	//
	//	Several scenarios may arise as follows:-
	//
	//		1)	The request message may never be recieved in full either because the client is deliberately leaving requests incomplete as part of a DOS or DDOS
	//			(denial of service) attack, OR because the wrong size was sent in the header OR because of a broken pipe.
	//
	//		2)	The requests may be pipelined with other requests or the wrong size was sent in the header, meaning there are extra byte at the end. These must
	//			be treated as belonging to the next message and must therefore be aggregated to a separate message chain.
	//
	//		3)	A client might send a null packet (of zero size) to indicate it will send no further data. But equally it might not or this packet may otherwise
	//			fail to arrive. Or the null packet could arrive too late, after the response to the request has been sent and after the connection has timed out
	//			and been closed by the server. In this latter scenario the connection is gone so in theory the null packet is never seen. Where the null packet
	//			is seen it sets the 'client done' flag so that the connection will be closed even if the handler returns TCP_KEEPALIVE.
	//
	//		4)	The sending out of a response message from the hzIpConnex outgoing message queue encounters a broken pipe error. This is non-recoverable so the
	//			outgoing data is destructed. This can happen while an incoming message is being aggregated or processed.

	hzList<hzTcpListen*>	m_LS ;		//	Listening sockets
	hzVect<hzIpConnex*>		m_Inbound ;	//	Connected clients

	hzLogger*	m_pLog ;				//	Log channel to use for events
	hzLogger*	m_pStats ;				//	Log channel to use for stats
	socklen_t	m_nCliLen ;				//	Length of client
	uint32_t	m_nMaxClients ;			//	Maximum number of simultaneous connections.
	uint32_t	m_nCurrentClients ;		//	Current number of connected clients.
	uint32_t	m_nTimeout ;			//	Timeout for select
	uint32_t	m_eError ;				//	Error code
	uint32_t	m_nMaxSocket ;			//	Highest socket for the select function
	bool		m_bActive ;				//	Socket to start listening
	bool		m_bShutdown ;			//	Socket to stop listening

	/*
	**	Private constructor for singleton method
	*/

	hzIpServer	(void)
	{
		m_pLog = 0 ;
		m_nMaxClients = 0 ;
		m_nCurrentClients = 0 ;
		m_bActive = false ;
		m_bShutdown = false ;
		m_nMaxSocket = 0 ;
		m_nTimeout = 30 ;
	}

public:
	static	hzIpServer*	GetInstance	(hzLogger* pLogger) ;

	~hzIpServer	(void)	{}

	void	SetTimeout	(uint32_t Timeout)	{ m_nTimeout = Timeout ; }
	void	SetLogger	(hzLogger* pLog)	{ m_pLog = pLog ; }
	void	SetStats	(hzLogger* pStats)	{ m_pStats = pStats ; }

	//	Adds a TCP listening socket for invoking a user defined function that handles general client connections
	hzEcode	AddPortTCP	(	hzTcpCode	(*OnIngress)(hzChain&, hzIpConnex*),
							hzTcpCode	(*OnConnect)(hzIpConnex*),
							hzTcpCode	(*OnDisconn)(hzIpConnex*),
							uint32_t	nTimeout,
							uint32_t	nPort,
							uint32_t	nMaxClients,
							bool		bSecure = false	) ;

	//	Adds a listening socket for HTTP connections. HTTP is a special case in which applications specify an OnHtpRq function to process complete HTTP requests
	//	rather than an OnIngress function. The OnIngress function is in-built and extracts HTTP requests from the incomming data.
	hzEcode	AddPortHTTP	(	hzTcpCode	(*OnHttpReq)(hzHttpEvent*),
							uint32_t	nTimeout,
							uint32_t	nPort,
							uint32_t	nMaxClients,
							bool		bSecure = false	) ;

	//	Adds a TCP listening socket for client connections that are to be managed by a user defined thread handler function.
	hzEcode	AddPortSess	(	void*		(*OnSession)(void*),
							uint32_t	nTimeout,
							uint32_t	nPort,
							uint32_t	nMaxClients,
							bool		bSecure = false	) ;

	//	Adds a UDP socket. Note there cannot be an OnConnect function in the normal sense as UDP has no connections. However applications will need to ...
	hzEcode	AddPortUDP	(	hzTcpCode	(*OnIngress)(hzChain&, hzIpConnex*),
							hzTcpCode	(*OnConnect)(hzIpConnex*),
							hzTcpCode	(*OnDisconn)(hzIpConnex*),
							uint32_t	nTimeout,
							uint32_t	nPort,
							uint32_t	nMaxClients,
							bool		bSecure = false	) ;

	//	followed by a single call to Activate sets all ports to listen
	hzEcode	Activate	(void) ;

	//	After above initialization (generally within the main thread) you may now invoke the Serve() functions below. If you only need a single thread
	//	server just call either ServeSelect() to serve using the select method or ServeEpollST() to serve using the epoll method. The latter is prefered
	//	because it has lower operational overheads and hence better performance.

	void	ServeSelect		(void) ;
	void	ServeEpollST	(void) ;

	//	If you need a multi-threaded server, you are forced to use thread specialization. The ServeEpollMT() function, normally called within the main
	//	thread, will ONLY listen on the listening sockets, accept client connections and disconnections and read incoming client requests. These is no
	//	call to a request handler function. Instead client requests are placed in a lock free queue from where they are drawn by ServeRequests() which
	//	must be called from a separate thread in your application created by means of a thread entry function and a pass to pthread_create(). You may
	//	set up several ServeRequests threads as required. Then yet another thread, ServeResponses() is called. This is nessesary as ServeEpollMT(),
	//	unlike its single threaded counterpart, does not write to client sockets. ServeReponses() does this job instead and does it by drawing the
	//	responses from a queue fed by ServeRequests calls to the Handler passed in AddPort().

	void	ServeRequests	(void) ;
	void	ServeResponses	(void) ;
	void	ServeEpollMT	(void) ;

	void	Halt	(void)	{ m_bShutdown = true ; }
} ;

#if 0
enum	hzIpStatus
{
	HZ_IPSTATUS_NULL		= 0,	//	IP address has no associated notes
	HZ_IPSTATUS_WHITE_SMTP	= 0x0001,	//	IP address is whitelisted by a successful SMTP authentication
	HZ_IPSTATUS_WHITE_POP3	= 0x0002,	//	IP address is whitelisted by a successful POP3 authentication
	HZ_IPSTATUS_WHITE_HTTP	= 0x0004,	//	IP address is whitelisted by a successful HTTP authentication
	HZ_IPSTATUS_WHITE_DATA	= 0x0008,	//	IP address is whitelisted by a successful data authentication
	HZ_IPSTATUS_BLACK_SMTP	= 0x0010,	//	IP address is blacklisted by a failed SMTP authentication
	HZ_IPSTATUS_BLACK_POP3	= 0x0020,	//	IP address is blacklisted by a failed POP3 authentication
	HZ_IPSTATUS_BLACK_HTTP	= 0x0040,	//	IP address is blacklisted by a failed HTTP authentication
	HZ_IPSTATUS_BLACK_DATA	= 0x0080,	//	IP address is blacklisted by a failed data authentication
	HZ_IPSTATUS_BLACK_PROT	= 0x0100,	//	IP address is blacklisted because of a malformed request
} ;

class	hzIpinfo
{
	//	Records a blocked IP address.

public:
	uint16_t	m_nSince ;	//	Count of attempts since last block
	uint16_t	m_nTotal ;	//	Count of attempts since first block
	uint16_t	m_nDelay ;	//	Compact epoch time for status to remain in force
	uint16_t	m_bInfo ;	//	Status of address

	hzIpinfo	(void)	{ m_nSince = m_nTotal = m_nDelay = m_bInfo = 0 ; }
} ;
#endif

/*
**	Prototypes
*/

hzEcode		SetupHost		(void) ;
hzEcode		InitServerSSL	(const char* pvtKey, const char* sslCert, const char* sslCA, bool bVerifyClient) ;
//void		SetStatusIP		(hzIpaddr ipa, hzIpStatus status) ;
//uint32_t	GetStatusIP		(hzIpaddr ipa) ;

/*
**	Globals
*/

extern	hzMapS	<hzIpaddr,hzIpinfo>	_hzGlobal_StatusIP ;	//	Black and white listed IP addresses

extern	hzString	_hzGlobal_Hostname ;		//	String form of actual hostname of this server
extern	hzString	_hzGlobal_HostIP ;			//	String form of assigned IP address of this server
extern	hzIpaddr	_hzGlobal_localhost ;		//	Always the default IP address 127.0.0.1
extern	hzIpaddr	_hzGlobal_livehost ;		//	Assigned IP address of this server

extern	hzString	_hzGlobal_sslPvtKey ;		//  SSL Private Key
extern	hzString	_hzGlobal_sslCert ;			//  SSL Certificate
extern	hzString	_hzGlobal_sslCertCA ;		//  SSL CA Certificate

#endif	//	hzIpServer_h
