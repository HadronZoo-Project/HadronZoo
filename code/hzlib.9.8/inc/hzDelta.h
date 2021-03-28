//
//	File:	hzDelta.h
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

#ifndef hzDelta_h
#define hzDelta_h

#include "hzTcpClient.h"

enum	hzDeltaReq
{
	//	Category:	System
	//
	//	Commands and notifications to and from Delta Server

	//	Client Commands to server
	DELTA_CLI_CONNECT		=  1,		//	Request by application to connect to the local Delta Server
	DELTA_CLI_QUIT			=  2,		//	Request by application to disconnect from the local Delta Server
	DELTA_CLI_QUEFILE		=  3,		//	Request by application for local Delta Server to que file for transmission
	DELTA_CLI_DELFILE		=  4,		//	Notification by application of a file deletion
	DELTA_CLI_DELTA			=  5,		//	Notification by application of a data repository update

	//	Server commands to server
	DELTA_SVR_CONNECT		= 10,		//	Other server seeks connection to here
	DELTA_SVR_QUIT			= 11,		//	Other server is to disconnect from this server
	DELTA_SVR_GETCONFIG		= 12,		//	Other server want's this server's config
	DELTA_SVR_GETFSTATE		= 13,		//	Other server want's this server's file states
	DELTA_SVR_GETFILE		= 14,		//	Other server tells this server to que up a file for transmission
	DELTA_SVR_GETDELTA		= 15,		//	Other server seeks to obtain a missing delta or set of deltas
	DELTA_SVR_HEAD			= 16,		//	First part of file transmission
	DELTA_SVR_PART			= 17,		//	Subsequent part of file transmission
	DELTA_SVR_DELFILE		= 18,		//	Notification by server of a file deletion by an application
	DELTA_SVR_DELTA			= 19,		//	Notification by server of a data repository update in an application

	//	Server notifications to client
	DELTA_NOT_FILE			= 20,		//	Server notifies application of a file placed in its space
	DELTA_NOT_DELFILE		= 21,		//	Server notifies application of a file deleted from its space
	DELTA_NOT_DELTA			= 22,		//	Server notifies application of a data repository update
} ;

enum	hzDeltaRes
{
	//	Category:	System
	//
	//	Responses and notifications from Delta Server

	DELTA_ERR_CONN			= 1,		//	Could not connect to Delta
	DELTA_ERR_DUPCONN		= 2,		//	Already connected to Delta
	DELTA_ERR_SEND			= 3,		//	Data could not be written to a socket
	DELTA_ERR_RECV			= 4,		//	Data could not be read from a socket
	DELTA_ERR_PERM			= 5,		//	Process does not have write permission
	DELTA_ERR_DUP			= 6,		//	Instruction already recived/Channel already open
	DELTA_ERR_NOMEM			= 7,		//	No memory on the Delta Server to execute command
	DELTA_NACK				= 8,		//	Request failed, no reason available
	DELTA_ACK				= 9,		//	Request succesful
} ;

class	hzDeltaClient
{
	//	Category:	System
	//
	//	The Delta Server client class. Note this is only used to notify the local Delta Server of repository updates and uploaded file occuring within
	//	an application. Notifications from the Delta Server advising of repository updates and uploaded file occuring within a sister application on
	//	another server within the Delta cluster, are handled by the ProcDelta() event handler defined in hzDelta.cpp

	hzTcpClient	m_Connection ;				//	Client connection to Delta Server
	hzString	m_AppDescFile ;				//	Filepath to application description (XML) file
	uint32_t	m_nSessID ;					//	Session ID
	uint32_t	m_nPort ;					//	The port this app will listen on for Delta notifications
	char		m_cvData[HZ_MAXPACKET+4] ;	//	Buffer for data transportation

	hzDeltaClient	(void)
	{
		m_nSessID = m_nPort = 0 ;
	}

public:
	static hzDeltaClient*	GetInstance	(void) ;

	~hzDeltaClient	(void)	{}

	hzEcode	InitOnce	(hzString& filepath) ;

	//	Client requests (by applicaton)
	hzEcode	Connect		(void) ;
	hzEcode	Quit		(void) ;
	hzEcode	SendFile	(hzString& filepath) ;
	hzEcode	DelFile		(hzString& filepath) ;
	hzEcode	DeltaWrite	(hzChain& Zd) ;

	//	Check connection
	bool		IsOpen	(void)	{ return m_nSessID ? true : false ; }
	uint32_t	SessID	(void)	{ return m_nSessID ; }

	//	Close client connection to Delta
	hzEcode	Close		(void) ;
} ;

/*
**	Prototypes
*/

//hzEcode	DeltaInit	(const char* dir, const char* appname, const char* arg, const char* version, bool bMustHave) ;

#endif	//	hzDelta_h
