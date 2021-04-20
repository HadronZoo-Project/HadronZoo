//
//	File:	hzDelta.cpp
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
//	Interface to Delta Server
//

#include <iostream>
#include <fstream>

//	#include <unistd.h>
#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/stat.h>

#include "hzDocument.h"
#include "hzDatabase.h"
#include "hzDirectory.h"
#include "hzProcess.h"
#include "hzIpServer.h"
#include "hzDelta.h"

using namespace std ;

/*
**	Variables
*/

global	hzDeltaClient*	_hzGlobal_DeltaClient ;						//	The singleton connecttion to Delta server.

const	hzString	s_deltaConf = "/etc/hzDelta.d/cluster.xml" ;	//	Standard pathname of Delta config file

static	uint32_t	s_deltaPort ;									//	Delta server port on local machine
static	uint32_t	s_deltaPort_in ;								//	Delta server port on local machine

/*
**	Non-member Functions
*/

hzEcode	hdbADP::DeltaInit	(const char* dir, const char* app, const char* arg, const char* ver, bool bMustHave)
{
	//	Category:	System
	//
	//	DeltaInit() is an initialization function strictly for use within programs that make use of the HadronZoo Database Suite. DeltaInit() initiates a client
	//	connection with the Delta server on the local machine and exchanges an application description with it. The view of the application description as seen
	//	by the application must be in exact agreement with that seen by the Delta Server otherwise the application cannot proceed.
	//
	//	It stands to reason that the two views of the application description cannot be compared until the application configs are loaded, as without this step,
	//	the appliction has no data classes defined and no repositories declared. However, it should be noted that the process of reading the application configs is aided by prior knowledge i
	//
	//	on the local machine. It is strictly for use within programs that need to mirror data transactions within data repositories - which in general means programs that are Internet applications. The
	//	DeltaInit() must be called AFTER configuration of a program's data model but BEFORE any server instance is invoked. DeltaInit() iterates the
	//	global collection of declared repositories (ADP mapRepositories) to build an application profile. This comprises the application name, an
	//	invokation argument and a version number together with a list of all the data repositories the application utilizes and such things as data
	//	directories and files. In essence, an application profile is everything the Delta server will need to know in order to apply data mirroring -
	//	which is why it must be called after the data model is established.
	//
	//	The application profile is exported as an XML file which will reside at a location chosen by the application, normally in the application data
	//	directories or alongside the application's config files. The XML file will have a name of the form 'appname.argument.version.xml', for example
	//	"dissemino.hadronzoo.v1.xml". All the fields are nessesary for the avoidance of ambiguity. The argument is required because programs such as
	//	Dissemino can be deployed to serve several different websites and thus several different object models from the same machine. And the version
	//	refers to the version of the object model, not the application.
	//
	//	Note that if such a file already exists in the application space, DeltaInit() will compare the data model view it implies to the view of the
	//	data model it has just compiled. If there is a difference, the file is moved by appending the current date and time to the filename. Then the
	//	new XML file is written in its place. If there is no difference, the file will be left unchanged.
	//
	//	DeltaInit() then creates a singleton instance of the hzDeltaClient class and initializes it with full pathname of the application profile XML
	//	file. It then calls hzDeltaClient::Connect() to authenticate the application to the Delta server on the local machine. The Connect() function
	//	sends the local Delta server the full pathname of this file. The file itself to be sent to the Delta server as it resides on the same machine.
	//
	//	The Delta server will either have or will create a file of exactly the same name but in the Delta config directory (usually /etc/hzDelta.d).
	//	This internal file contains additional information, mostly meta data of the application's version of the file. This Delta internal view of an
	//	application must be compatible with the application view for the Delta server to accept the application as a client. To determine if the two
	//	files match, the Delta server reads both.
	//
	//	There are inherent limits to how far DeltaInit() can go to be failsafe. The rules are that if no Delta server is running but /etc/hzDelta.d/
	//	exists and had a file of a matching name, then DeltaInit() will perform the comparison and terminate execution if the two files show an object
	//	model discrepancy.
	//
	//	Arguments:	1)	dir			The directory chosen by the application in which the application profile file will reside
	//				2)	app			The base name of the application (e.g. dissemino)
	//				3)	arg			The key parameter for the application (if dissemino then this will be an identifier for the website)
	//				4)	ver			This is the version of the data model employed, rather than version of the application
	//				5)	bMustHave	If set this will terminate execution if a connection to a delta server cannot be fully set up. The program will
	//								still terminate if other less forgivable errors apply such as calling with duff arguments.
	//
	//	Returns:	E_ARGUMENT	If one or more arguments is null or invalid
	//				E_INITFAIL	If there is currently no hostname and none could be established
	//				E_FORMAT	If there is the application profile held in /ect/hzDelta.d/ is not of the expected form
	//				E_OPENFAIL	If this function cannot write out the application's version of the profile
	//				E_OK		If a connection to a delta server was established or if bMustHave is false

	_hzfunc("hzDeltaClient::DeltaInit") ;

	static bool	bBeenHere = false ;

	const hdbObjRepos*	pRepos ;	//	Repository pointer

	ofstream		os ;			//	Output stream
	FSTAT			fs ;			//	File status
	hzDocXml		X ;				//	XML document
	hzChain			Z ;				//	Output chain for compiling XML
	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pRoot ;			//	XML node/tag
	hzXmlNode*		pN ;			//	XML node/tag
	const char*		anam ;			//	Attribute name
	const char*		aval ;			//	Attribute value
	hzString		adpFile ;		//	Standard pathname of Delta config file
	hzString		pathname ;		//	Combined dir/appname.arguemt.version.xml
	hzString		portNote ;		//	Combined dir/appname.arguemt.version.xml
	hzString		portApps ;		//	Combined dir/appname.arguemt.version.xml
	hzIpaddr		ipa ;			//	Ip address
	uint32_t		nIndex ;		//	Repository iterator
	uint32_t		nPort ;			//	Port number
	hzEcode			rc = E_OK ;		//	Return code

	if (bBeenHere)
		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Repeat call") ;

	//	Check arguments
	if (!dir || !dir[0])	{ rc = E_ARGUMENT ; Z << "No application profile directory supplied\n" ; }
	if (!app || !app[0])	{ rc = E_ARGUMENT ; Z << "No application profile appname supplied\n" ; }
	if (!arg || !arg[0])	{ rc = E_ARGUMENT ; Z << "No application profile argument supplied\n" ; }
	if (!ver || !ver[0])	{ rc = E_ARGUMENT ; Z << "No application profile version supplied\n" ; }

	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, Z) ;

	//	Establish port
	if (!_hzGlobal_Hostname)
	{
		if (SetupHost() != E_OK)
			hzexit(_fn, 0, E_INITFAIL, "Could not setup hostname") ;
		if (!_hzGlobal_Hostname)
			hzexit(_fn, 0, E_INITFAIL, "No hostname established") ;
	}

	//	Check that application configs have defined at least one data class and repository
	if (!CountDataClass())
		return hzerr(_fn, HZ_ERROR, E_NODATA, "No Data Clases defined") ;
	if (!CountObjRepos())
		return hzerr(_fn, HZ_ERROR, E_NODATA, "No Data Repositories defined") ;

	/*
	**	Read the delta config to obtain delta connection info
	*/

	if (lstat(s_deltaConf, &fs) < 0)
	{
		if (bMustHave)
			return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Delta config file %s not found\n", *s_deltaConf) ;
		return E_OK ;
	}

	rc = X.Load(s_deltaConf) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not open conf file (%s)\n", *s_deltaConf) ;

	pRoot = X.GetRoot() ;
	if (!pRoot->NameEQ("deltaCluster"))
		return hzerr(_fn, HZ_ERROR, rc, "No root <deltaCluster> tag in conf file (%s)\n", *s_deltaConf) ;

	for (pN = pRoot->GetFirstChild() ; pN ; pN = pN->Sibling())
	{
		if		(pN->NameEQ("pointA"))	nIndex = 0 ;
		else if	(pN->NameEQ("pointB"))	nIndex = 1 ;
		else if	(pN->NameEQ("pointC"))	nIndex = 2 ;
		else if	(pN->NameEQ("pointD"))	nIndex = 3 ;
		else
			return hzerr(_fn, HZ_ERROR, E_FORMAT, "Illegal tag <%s> in <deltaCluster>. Only <pointA/B/C/D> allowed\n", pN->TxtName()) ;

		for (ai = pN ; ai.Valid() ; ai.Advance())
		{
			anam = ai.Name() ; aval = ai.Value() ;

			if		(!strcmp(anam, "addr"))		ipa = aval ;
			else if (!strcmp(anam, "portNote"))	portNote = aval ;
			else if (!strcmp(anam, "portApps"))	portApps = aval ;
			else
				return hzerr(_fn, HZ_ERROR, E_FORMAT, "Line %d: Illegal attribute %s. Only addr|portNote|portApps allowed", pN->Line(), anam) ;
		}

		//	Check we have evrything
		if (ipa == _hzGlobal_nullIP)
			return hzerr(_fn, HZ_ERROR, E_FORMAT, "Line %d: Invalid IP address\n", pN->Line()) ;

		if (!portNote)	return hzerr(_fn, HZ_ERROR, E_FORMAT, "Line %d: 2nd attribute of <%s> must be 'portNote'\n", pN->Line(), pN->TxtName()) ;
		if (!portApps)	return hzerr(_fn, HZ_ERROR, E_FORMAT, "Line %d: 3rd attribute of <%s> must be 'portApps'\n", pN->Line(), pN->TxtName()) ;

		//	Get port for notifications
		nPort = atoi(*portNote) ;
		if (ipa == _hzGlobal_livehost)
			s_deltaPort_in = nPort ;

		//	Get port for uploads to delta
		nPort = atoi(*portApps) ;
		if (ipa == _hzGlobal_livehost)
			s_deltaPort = nPort ;
	}

	/*
	**	Read the application delta profile
	*/

	//	Check supplied filename if of the form appname.version[-argument].xml
	Z.Printf("%s/%s.%s.%s.xml", dir, app, arg, ver) ;
	pathname = Z ;
	Z.Clear() ;

	//	Create the description
	Z.Printf("<appProfile appname=\"%s\" arg=\"%s\" ver=\"%s\">\n", app, arg, ver) ;

	for (nIndex = 0 ; nIndex < CountObjRepos() ; nIndex++)
	{
		pRepos = GetObjRepos(nIndex) ;
		pRepos->DescRepos(Z, 1) ;
	}
	Z << "</appProfile>\n" ;

	os.open(*pathname) ;
	if (os.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot stat application profile [%s]", *pathname) ;
	os << Z ;
	os.close() ;

	/*
	**	Obtain a Delta Client instance and call Connect
	*/

	hzDeltaClient::GetInstance() ;
	_hzGlobal_DeltaClient->InitOnce(pathname) ;
	rc = _hzGlobal_DeltaClient->Connect() ;
	if (rc != E_OK && bMustHave)
		return rc ;
	return E_OK ;
}

/*
**	hzDeltaClient Functions
*/

hzDeltaClient*	hzDeltaClient::GetInstance	(void)
{
	_hzfunc("hzDeltaClient::GetInstance") ;

	if (_hzGlobal_DeltaClient)
		hzexit(_fn, 0, E_DUPLICATE, "hzDeltaClient is a singleton and already allocated") ;

	_hzGlobal_DeltaClient = new hzDeltaClient() ;
	return _hzGlobal_DeltaClient ;
}

hzEcode	hzDeltaClient::InitOnce	(hzString& pathname)
{
	_hzfunc("hzDeltaClient::InitOnce") ;

	FSTAT	fs ;	//	File status

	if (m_AppDescFile)
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Illegal repeat call") ;

	if (lstat(*pathname, &fs) < 0)
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Cannot stat application profile file [%s]", *pathname) ;

	m_AppDescFile = pathname ;
	return E_OK ;
}

hzEcode	hzDeltaClient::Connect	(void)
{
	//	Category:	System
	//
	//	This function creates a client connection to the local Delta Server and authenticates to it. It must be called directly after all repositories
	//	have been fully initialized but BEFORE any data transactions are conducted. The full pathname of the application profile is then sent to the
	//	delta server which will compare it to it's own record of the model. The two must agree, otherwise this function will terminate execution.
	//
	//	Note the client connection to the Delta server is a singleton and that the CONNECT command sends an extended heade containing the Unix user id
	//	and group id of the application owner. This is stored alongside other info on the app in the Delta server. The Delta server needs this in order
	//	to ensure any repository delta files associated with the app are written out with the right owner and group.
	//
	//	Arguments:	None
	//
	//	Returns:	E_SENDFAIL	If the delta session login request could not be sent
	//				E_RECVFAIL	If the delta session ack was not recived
	//				E_PROTOCOL	If the server does not send the session ack
	//				E_OK		If the delta session is set up

	_hzfunc("hzDeltaClient::Connect") ;

	hzIpaddr	ipa ;			//	This is always the local host
	uint32_t	len ;			//	Message size (8 + length of app desc file pathname)
	uint32_t	nUser ;			//	UNIX user id
	uint32_t	nGroup ;		//	UNIX user group id
	uint32_t	nProcID ;		//	Process id (used as session id on Delta server)
	hzEcode		rc = E_OK ;		//	Return code

	if (!m_AppDescFile)
		hzexit(_fn, 0, E_NOINIT, "No previous call to InitOnce") ;

	/*
	**	Connect to delta server and send
	*/

	ipa = "127.0.0.1" ;
	//rc = m_Connection.ConnectIP(ipa, s_deltaPort) ;
	rc = m_Connection.ConnectStd(*ipa.Str(), s_deltaPort) ;
	if (rc != E_OK)
		return rc ;

	nProcID = getpid() ;
	nUser = geteuid() ;
	nGroup = getegid() ;

	//	Formulate command - use process id in place of the session id as this will notify the Delta server of the procss id.
	m_cvData[0] = (nProcID & 0xff000000) >> 24 ;
	m_cvData[1] = (nProcID & 0xff0000) >> 16 ;
	m_cvData[2] = (nProcID & 0xff00) >> 8 ;
	m_cvData[3] = (nProcID & 0xff) ;
	m_cvData[4] = DELTA_CLI_CONNECT ;
	len = 16 + m_AppDescFile.Length() ;
	m_cvData[5] = (len & 0xff00) >> 8 ;
	m_cvData[6] = (len & 0xff) ;
	m_cvData[7] = (nUser & 0xff000000) >> 24 ;
	m_cvData[8] = (nUser & 0xff0000) >> 16 ;
	m_cvData[9] = (nUser & 0xff00) >> 8 ;
	m_cvData[10] = (nUser & 0xff) ;
	m_cvData[11] = (nGroup & 0xff000000) >> 24 ;
	m_cvData[12] = (nGroup & 0xff0000) >> 16 ;
	m_cvData[13] = (nGroup & 0xff00) >> 8 ;
	m_cvData[14] = (nGroup & 0xff) ;
	strcpy(m_cvData + 15, *m_AppDescFile) ;

	threadLog("Sending app desc file %d %d bytes [%s]\n", m_AppDescFile.Length(), len, *m_AppDescFile) ;

	//	Send message and recv response
	if (m_Connection.Send(m_cvData, len) != E_OK)
		return E_SENDFAIL ;

	if (m_Connection.Recv(m_cvData, len, HZ_MAXPACKET) != E_OK)
		return E_RECVFAIL ;

	if (m_cvData[4] == DELTA_ACK)
	{
		m_nSessID = 0 ;
		m_nSessID |= (m_cvData[0] << 16) ;
		m_nSessID |= (m_cvData[1] << 16) ;
		m_nSessID |= (m_cvData[2] << 8) ;
		m_nSessID |= m_cvData[3] ;
		return E_OK ;
	}

	return E_PROTOCOL ;
}

hzEcode	hzDeltaClient::Quit	(void)
{
	//	Close a client connection to Delta server.
	//
	//	Arguments:	None.
	//
	//	Returns:	E_SENDFAIL	If the quit message could not be sent
	//				E_RECVFAIL	If the response was not recieved
	//				E_NOTFOUND	If file could not be closed (does not belong to this process)
	//				E_OK		If file closed.

	_hzfunc("hzDeltaClient::Quit") ;

	uint32_t	nBytes ;
	hzEcode	rc = E_OK ;

	//	Command
	m_cvData[0] = (m_nSessID & 0xff000000) >> 24 ;
	m_cvData[1] = (m_nSessID & 0xff0000) >> 16 ;
	m_cvData[2] = (m_nSessID & 0xff00) >> 8 ;
	m_cvData[3] = (m_nSessID & 0xff) ;
	m_cvData[4] = DELTA_CLI_QUIT ;
	m_cvData[5] = 0 ;
	m_cvData[6] = 3 ;

	//	Send message and receive response
	rc = m_Connection.Send(m_cvData, 7) ;
	if (rc != E_OK)
		return E_SENDFAIL ;

	rc = m_Connection.Recv(m_cvData, nBytes, HZ_MAXPACKET) ;
	if (rc != E_OK)
		return E_RECVFAIL ;

	return E_OK ;
}

hzEcode	hzDeltaClient::SendFile		(hzString& filepath)
{
	//	Notify Delta server of newly uploaded file. The server will read the file itself in order to mirror it, all we do here is tell the server the
	//	pathname.
	//
	//	Arguments:	1)	filepath	Full path of file to be mirrored.
	//
	//	Returns:	E_ARGUMENT	If the filepath is not supplied
	//				E_NOTFOUND	If the specified filepath does not exist
	//				E_SENDFAIL	If the filepath could not be sent
	//				E_RECVFAIL	If the ack is not recieved
	//				E_PROTOCOL	If the ack is a nack
	//				E_OK		If the sendfile notification is successful

	_hzfunc("hzDeltaClient::SendFile") ;

	FSTAT		fs ;		//	To stat file on local machine
	uint32_t	nRecv ;		//	Bytes received this message
	uint32_t	len ;		//	Filename length
	hzEcode		rc ;		//	Return code

	if (!filepath)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No local filename specified") ;

	if (stat(*filepath, &fs) == -1)
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Local file [%s] does not exist", *filepath) ;

	//	Formulate Command
	m_cvData[0] = (m_nSessID & 0xff000000) >> 24 ;
	m_cvData[1] = (m_nSessID & 0xff0000) >> 16 ;
	m_cvData[2] = (m_nSessID & 0xff00) >> 8 ;
	m_cvData[3] = m_nSessID & 0xff ;
	m_cvData[4] = DELTA_CLI_QUEFILE ;
	len = 8 + filepath.Length() ;
	m_cvData[5] = (len & 0xff00) >> 8 ;
	m_cvData[6] = len & 0xff ;
	strcpy(m_cvData + 7, *filepath) ;

	//	Send message
	if ((rc = m_Connection.Send(m_cvData, len)) != E_OK)
		return rc ;

	//	Await ACK
	if ((rc = m_Connection.Recv(m_cvData, nRecv, HZ_MAXPACKET)) != E_OK)
		return rc ;

	//	Got NACK reason in tail
	if (m_cvData[4] != DELTA_ACK)
		return hzerr(_fn, HZ_ERROR, E_PROTOCOL, "Operation failed") ;
	return E_OK ;
}

hzEcode	hzDeltaClient::DelFile	(hzString& filepath)
{
	//	Notify server of a file deletion
	//
	//	Arguments:	1)	filepath	Full path of file to be mirrored.
	//
	//	Returns:	E_ARGUMENT	If the filepath is not supplied
	//				E_NOTFOUND	If the specified filepath does not exist
	//				E_SENDFAIL	If the filepath could not be sent
	//				E_RECVFAIL	If the ack is not recieved
	//				E_PROTOCOL	If the ack is a nack
	//				E_OK		If the sendfile notification is successful

	_hzfunc("hzDeltaClient::DelFile") ;

	FSTAT		fs ;		//	To stat file on local machine
	uint32_t	nRecv ;		//	Bytes received this message
	uint32_t	len ;		//	Filename length
	hzEcode		rc ;		//	Return code

	if (!filepath)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No local filename specified") ;

	if (stat(*filepath, &fs) == -1)
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Local file [%s] does not exist", *filepath) ;

	//	Formulate command
	m_cvData[0] = (m_nSessID & 0xff000000) >> 24 ;
	m_cvData[1] = (m_nSessID & 0xff0000) >> 16 ;
	m_cvData[2] = (m_nSessID & 0xff00) >> 8 ;
	m_cvData[3] = m_nSessID & 0xff ;
	m_cvData[4] = DELTA_CLI_DELFILE ;
	len = 8 + filepath.Length() ;
	m_cvData[5] = (len & 0xff00) >> 8 ;
	m_cvData[6] = len & 0xff ;
	strcpy(m_cvData + 7, *filepath) ;

	//	Send message
	if ((rc = m_Connection.Send(m_cvData, len)) != E_OK)
		return rc ;

	//	Await ACK
	if ((rc = m_Connection.Recv(m_cvData, nRecv, HZ_MAXPACKET)) != E_OK)
		return rc ;

	//	Got NACK reason in tail
	if (m_cvData[0] != DELTA_ACK)
		return hzerr(_fn, HZ_ERROR, E_PROTOCOL, "Operation failed") ;
	return E_OK ;
}

hzEcode	hzDeltaClient::DeltaWrite	(hzChain& Zd)
{
	//	Send repository update in delta notation

	_hzfunc("hzDeltaClient::DeltaWrite()") ;

	if (!Zd.Size())
		return E_OK ;

	hzChain	X ;			//	Final outgoing msg
	uint32_t	len ;
	uint32_t	nRecv ;
	hzEcode	rc ;

	//	Formulate command
	m_cvData[0] = (m_nSessID & 0xff000000) >> 24 ;
	m_cvData[1] = (m_nSessID & 0xff0000) >> 16 ;
	m_cvData[2] = (m_nSessID & 0xff00) >> 8 ;
	m_cvData[3] = m_nSessID & 0xff ;

	X.AddByte(m_cvData[0]) ; X.AddByte(m_cvData[1]) ; X.AddByte(m_cvData[2]) ; X.AddByte(m_cvData[3]) ;

	m_cvData[0] = DELTA_CLI_DELTA ;
	len = 8 + Zd.Size() ;
	m_cvData[1] = (len & 0xff0000) >> 16 ;
	m_cvData[2] = (len & 0xff00) >> 8 ;
	m_cvData[3] = len & 0xff ;

	X.AddByte(m_cvData[0]) ; X.AddByte(m_cvData[1]) ; X.AddByte(m_cvData[2]) ; X.AddByte(m_cvData[3]) ;
	X << Zd ;

	//	Send message
	if ((rc = m_Connection.Send(X)) != E_OK)
		return rc ;

	//	Await ACK
	if ((rc = m_Connection.Recv(m_cvData, nRecv, HZ_MAXPACKET)) != E_OK)
		return rc ;

	//	Got NACK reason in tail
	if (m_cvData[0] != DELTA_ACK)
		return hzerr(_fn, HZ_ERROR, E_PROTOCOL, "Operation failed") ;
	return E_OK ;
}

/*
**	Handle notifications from server
*/

hzTcpCode	ProcDelta	(hzChain& ZI, hzIpConnex* pCx)
{
	//	Category:	System
	//
	//	This accepts notifications from the Delta server of repository updates and loaded files on siste applications running on other machines in the
	//	Delta cluster. Repository updates are in reduced delta notation meaning that only the repository id and data class member id are supplied. The
	//	task is to locate the repository and apply the update. In the event of an uploaded file, 
	//
	//	To impliment full data mirroring you need to call AddPort on the TCP server instance

	_hzfunc(__func__) ;

	uint32_t	sessId ;
	char	res	[8] ;

	if (!_hzGlobal_DeltaClient)
	{
		res[0] = res[1] = res[2] = res[3] = 0 ;
		res[4] = DELTA_NACK ;
		res[5] = res[6] = res[7] = 0 ;
	}
	else
	{
		//	Formulate command
		sessId = _hzGlobal_DeltaClient->SessID() ;
		res[0] = (sessId & 0xff000000) >> 24 ;
		res[1] = (sessId & 0xff0000) >> 16 ;
		res[2] = (sessId & 0xff00) >> 8 ;
		res[3] = sessId & 0xff ;
		res[4] = DELTA_ACK ;
		res[5] = res[6] = res[7] = 0 ;
	}

    if (write(pCx->CliSocket(), res, 8) < 0)
    {
        hzerr(_fn, HZ_ERROR, E_SENDFAIL, "Send failed, killing connection\n") ;
        return TCP_TERMINATE ;
    }

	return TCP_KEEPALIVE ;
}
