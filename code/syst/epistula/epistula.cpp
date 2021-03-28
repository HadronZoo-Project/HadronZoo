//
//	File:	epistula.cpp
//
//	Legal Notice: This file is part of HadronZoo::Epistula, otherwise known as the "Epistula Email Server".
//
//	Copyright 1998, 2021 HadronZoo Project (http://www.hadronzoo.com)
//
//	HadronZoo::Epistula is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation,
//	either version 3 of the License, or any later version.
//
//	HadronZoo::Epistula is distributed in the hope that it will be useful but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//	PURPOSE. See the GNU General Public License for more details.
//
//	HadronZoo::Epistula depends on the HadronZoo C++ Class Library, with which it distributed as part of the HadronZoo Download. The HadronZoo C++ Class Library is free software:
//	you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License,
//	or any later version.
//
//	The HadronZoo C++ Class Library is distributed in the hope that it will be useful but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
//	A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License along with the HadronZoo C++ Class Library and a copy of the GNU General Public License along with
//	HadronZoo::Epistula. If not, see <http://www.gnu.org/licenses/>.
//

//
//	Synopsis:	Epistula.
//

#include <iostream>
#include <fstream>

using namespace std ;

#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#include "hzCtmpls.h"
#include "hzCodec.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzDocument.h"
#include "hzIpServer.h"
#include "hzHttpServer.h"
#include "hzDNS.h"
#include "hzProcess.h"
#include "hzMailer.h"
#include "hzDissemino.h"

#include "epistula.h"

/*
**	Variables
*/

global	bool	_hzGlobal_XM = true ;	//	Not using operator new override
global	bool	_hzGlobal_MT = true ;	//	Using multiple threads

global	hzProcess	proc_epistula ;		//	Need this for HadronZoo kernel
global	hzLogger	slog ;				//	System logs
global	hzLogger	logSend ;			//	Logger for sending monitor

global	pthread_mutex_t	g_LockRS ;		//	Lock for relay task schedule
global	hzIpServer*		g_pTheServer ;	//	The server instance for HTTP

//	Working (temp) maps and sets
global	hzMapS	<hzString,hdsInfo*>	g_sessCookie("mtxCookie") ;		//	Mapping of cookie value to cookie
global	hzMapS	<hzString,hdsInfo*>	g_sessMember("mtxMember") ;		//	Mapping of usernames to cookie
global	hzMapS	<hzString,mqItem*>	g_TheMailQue("mtxMailque") ;	//	List of outstanding relay tasks
global	hzSet	<hzEmaddr>			g_setBanned("mtxBanned") ;		//	Globally banned senders (users may also ban senders)
global	hzSet	<hzString>			g_setMboxLocks ;				//	Mail box locks (for POP3 deletes)

global	hzVect<epAccount*>	g_vecAccount ;		//	All epistula ordinary users (by uid)

global	epAccount*		g_pSuperuser ;				//	Epistula superuser (for epismail)
global	hzLockS			g_LockMbox ;				//	Locks g_setMboxLocks

//	Ports and limits (set in main)
global	uint32_t		g_nPortAlienSMTP ;			//	SMTP port for aliens to send messages to local users. This is always port 25.
global	uint32_t		g_nPortLocalSMTP ;			//	SMTP port for local users to originate messages (Must NOT be 25 and can be set to expect SSL connections).
global	uint32_t		g_nPortPOP3 ;				//	Standard POP3 port (Normally port 110, Epistula defaul 2110, can be any value)
global	uint32_t		g_nPortPOP3S ;				//	Secure POP3 port (Normally port 995, Epistula uses 2995, can be any value)
global	uint32_t		g_nMaxConnections ;			//	Number of simultaneous SMTP and POP3 connections
global	uint32_t		g_nMaxHTTPConnections ;		//	Number of simultaneous HTTP connections

global	uint32_t		g_nMaxMsgSize ;				//	Max email message size (2 million bytes)
global	uint32_t		g_nSessIdAuth ;				//	Session ID for AUTH connections
global	uint32_t		g_nSessIdPop3 ;				//	Session ID for POP3 connections
global	bool			g_bShutdown ;				//	When set all thread loops must end

extern	hdsApp*			theApp ;			//  Dissemino HTML interface
extern	Epistula*		theEP ;				//	The singleton Epistula Instance

//global	hzString	g_str_Inbox = "Inbox" ;
//global	hzString	g_str_Misc = "Misc" ;

/*
**	Prototypes
*/

hzTcpCode	ProcAlienSMTP	(hzChain& Input, hzIpConnex* pCC) ;
hzTcpCode	ProcLocalSMTP	(hzChain& Input, hzIpConnex* pCC) ;
hzTcpCode	HelloSMTP		(hzIpConnex* pCC) ;
hzTcpCode	FinishSMTP		(hzIpConnex* pCC) ;

hzTcpCode	HelloPOP3		(hzIpConnex* pCC) ;
hzTcpCode	ProcPOP3		(hzChain& Input, hzIpConnex* pCC) ;

hzEcode		admStart		(hzHttpEvent* pE) ;
hzEcode		admSlctDomains	(hzHttpEvent* pE) ;
hzEcode		admEditDomain	(hzHttpEvent* pE) ;
hzEcode		admEditAccounts	(hzHttpEvent* pE) ;
hzEcode		admAddresses	(hzHttpEvent* pE) ;
hzEcode		UsrMsgList		(hzHttpEvent* pE) ;
hzEcode		UsrMsgView		(hzHttpEvent* pE) ;
hzEcode		UsrMsgReply		(hzHttpEvent* pE) ;
hzEcode		UsrMsgNew		(hzHttpEvent* pE) ;
hzTcpCode   ProcHTTP		(hzHttpEvent* pE) ;

void*		MonitorSEND		(void* pThreadArg) ;

/*
**	Functions
*/

/*
**	Epistula-Main
*/


int32_t	main	(int argc, char** argv)
{
	//	Sets up the separate threads for SMTP standard, SMTP auth and POP3. Run HTTP in this thread.

	_hzfunc("Epistula-Main") ;

	pthread_attr_t	tattr ;			//	Thread attributes
	pthread_t		tidSmtp ;		//	Thread for handling SMTP sessions
	pthread_t		tidSend ;		//	Thread for handling SEND facility
	pthread_t		tidPop3 ;		//	Thread for handling POP3 sessions
	pthread_t		tidUpdate ;		//	Thread for handling config updates

	hzString	logpath ;			//	Path of main logfile
	epAccount*	pUser ;				//	User (superuser)
	hzString	tmpStr ;			//	Temp string
	int32_t		nCount ;			//	Used to count args and port-bind retries
	bool		bDemon = false ;	//	Run in demon mode
	bool		bVerbo = false ;	//	Run in verbose (test) mode
	bool		bCities = false ;	//	Use city level IP location
	hzEcode		rc ;				//	Return codes

	//	Setup interupts
	signal(SIGHUP,	CatchCtrlC) ;
	signal(SIGINT,	CatchCtrlC) ;
	signal(SIGQUIT,	CatchCtrlC) ;
	signal(SIGILL,	CatchSegVio) ;
	signal(SIGTRAP,	CatchSegVio) ;
	signal(SIGABRT,	CatchSegVio) ;
	signal(SIGBUS,	CatchSegVio) ;
	signal(SIGFPE,	CatchSegVio) ;
	signal(SIGKILL,	CatchSegVio) ;
	signal(SIGUSR1,	CatchCtrlC) ;
	signal(SIGSEGV,	CatchSegVio) ;
	signal(SIGUSR2,	CatchCtrlC) ;
	signal(SIGPIPE,	SIG_IGN) ;
	signal(SIGALRM,	CatchCtrlC) ;
	signal(SIGTERM,	CatchCtrlC) ;
	signal(SIGCHLD,	CatchCtrlC) ;
	signal(SIGCONT,	CatchCtrlC) ;
	signal(SIGSTOP,	CatchCtrlC) ;

	//	Process args
	for (nCount = 1 ; nCount < argc ; nCount++)
	{
		if		(!strcmp(argv[nCount], "-d"))	bDemon = true ;
		else if (!strcmp(argv[nCount], "-v"))	bVerbo = true ;
		else if (!strcmp(argv[nCount], "-c"))	bCities = true ;
		else if (!strcmp(argv[nCount], "-b"))	_hzGlobal_Debug |= HZ_DEBUG_SERVER ;
		else
			{ cout << "Usage: Illegal argument (" << argv[nCount] << "\n" ; return -1 ; }
	}

	if (bDemon && bVerbo)
		{ cout << "Usage: epistula -v(erbose) OR -d(emon). Cannot select both.\n" ; return -1 ; }
	if (!bDemon && !bVerbo)
		{ cout << "Usage: epistula -v(erbose) or -d(emon). Must select one.\n" ; return -1 ; }

	rc = HadronZooInitEnv() ;
	if (rc != E_OK)
	{
		if (!_hzGlobal_HOME)		cout << "Sorry - No path set for $HOME\n" ;
		if (!_hzGlobal_HADRONZOO)	cout << "Sorry - No path set for $HADRONZOO\n" ;

		cout << "Epistula cannot proceed\n" ;
		return -2 ;
	}

	//	Set ports and limits
	g_nPortAlienSMTP = 25 ;
	g_nPortLocalSMTP = 587 ;
	g_nPortPOP3 = 2110 ;
	g_nPortPOP3S = 2995 ;
	g_nMaxConnections = 100 ;
	g_nMaxHTTPConnections = 100 ;
	g_nMaxMsgSize = 2000000 ;

	//	Start Epistua server as a singleton daemon
	SingleProc() ;
	if (bDemon)
		Demonize() ;

	//	Start the log functions
	slog.OpenFile("/home/epistula/logs/main", LOGROTATE_DAY) ;
	slog.Verbose(bVerbo) ;
	slog.Log(_fn, "Starting Epistula\n") ;

	//	Setup host
	if (SetupHost() != E_OK)
		{ slog.Log(_fn, "Could not setup hostname\n") ; return 0 ; }

	slog.Log(_fn, "Hostname is (%s)\n", *_hzGlobal_Hostname) ;
	slog.Log(_fn, "Hostname IP (%s)\n", *_hzGlobal_HostIP) ;

	//	Init Standard Data Types
	//	rc = theApp->m_ADP.InitStandard("Epistula") ;
	//	if (rc != E_OK)
	//		{ slog.Out("Could not init the HDS\n") ; return false ; }

	//	Setup epistula.superuser
	g_pSuperuser = pUser = new epAccount() ;
	pUser->m_ruid = 0 ;
	g_pSuperuser->m_Unam = "superuser" ;
	g_vecAccount.Add(pUser) ;

	/*
	**	Initialize IP Ranges
	*/

	rc = InitCountryCodes() ;
	if (rc != E_OK)
		{ slog.Out("No Country Codes\n") ; return 105 ; }
	slog.Out("Initialized Country Codes\n") ;

	if (bCities)
	{
		rc = InitIpCity() ;
		if (rc != E_OK)
		{
			slog.Out("No City level IP Location table, trying basic level\n") ;
			bCities = false ;
		}
	}

	if (!bCities)
	{
		rc = InitIpBasic() ;
		if (rc != E_OK)
			slog.Out("No Basc level IP Location table\n") ;
	}

	/*
	**	Initialize the Database
	*/

	//	Init standard directories
	theEP = Epistula::GetInstance() ;
	slog.Out("Initialized Epistula Data Sphere\n") ;
	rc = theEP->InitLocations("/home/epistula") ;
	slog.Out("Initialized Epistula Directories\n") ;

	//	Create a Dissemino Instance and a Dissemino app
	Dissemino::GetInstance(slog) ;
	theApp = hdsApp::GetInstance(slog) ;
	theApp->m_ADP.InitStandard("Epistula") ;
	theApp->m_Docroot = "." ;
	theApp->m_Images = _hzGlobal_HADRONZOO + "/img" ;

        theApp->m_DefaultLang = "en-US" ;

        theApp->m_pDfltLang = new hdsLang() ;
        theApp->m_pDfltLang->m_code = theApp->m_DefaultLang ;
        theApp->m_Languages.Insert(theApp->m_pDfltLang->m_code, theApp->m_pDfltLang) ;

	//	Data load
	rc = theEP->InitEpistulaClasses() ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not configure dissemino caches (error=%s)\n", Err2Txt(rc)) ; return -9 ; }

	/*
	**	DO THE DISSEMINO SETUP
	*/

	//	Add the C++ Interface functions
	if (rc == E_OK)	rc = theApp->AddCIFunc(&admStart, "/admStart", ACCESS_MASK, HTTP_GET) ;
	if (rc == E_OK)	rc = theApp->AddCIFunc(&admSlctDomains, "/admSlctDomains", ACCESS_MASK, HTTP_GET) ;
	if (rc == E_OK)	rc = theApp->AddCIFunc(&admEditAccounts, "/admEditAccounts", ACCESS_MASK, HTTP_GET) ;
	if (rc == E_OK)	rc = theApp->AddCIFunc(&admAddresses, "/admaddresses", ACCESS_MASK, HTTP_GET) ;
	if (rc == E_OK)	rc = theApp->AddCIFunc(&UsrMsgList, "/", ACCESS_PUBLIC, HTTP_GET) ;
	if (rc == E_OK)	rc = theApp->AddCIFunc(&UsrMsgView, "/msgView", ACCESS_MASK, HTTP_GET) ;
	if (rc == E_OK)	rc = theApp->AddCIFunc(&UsrMsgReply, "/msgReply", ACCESS_MASK, HTTP_GET) ;
	if (rc == E_OK)	rc = theApp->AddCIFunc(&UsrMsgNew, "/msgNew", ACCESS_MASK, HTTP_GET) ;

	if (rc == E_OK)
	{
		tmpStr = "subscriber" ;
		theApp->m_UsernameFld = "username" ;
		theApp->m_UserpassFld = "userpass" ;
		rc = theApp->CreateDefaultForm(tmpStr) ;
		rc = theApp->SetLoginPost("/", "/", "/", "/") ;
	}

	if (rc != E_OK)
		{ slog.Out("%s. Could not configure HTTP dissemino interface (error=%s)\n", *_fn, Err2Txt(rc)) ; return -12 ; }

	/*
	**	Set up security keys
	*/

	_hzGlobal_sslPvtKey = "" ;		//  Put SSL Private Key here
	_hzGlobal_sslCert = "" ;		//  Put SSL Certificate here
	_hzGlobal_sslCertCA = "" ;		//  Put SSL CA Certificate here

	if (_hzGlobal_sslPvtKey && _hzGlobal_sslCert && _hzGlobal_sslCertCA)
	{
		rc = InitServerSSL(_hzGlobal_sslPvtKey, _hzGlobal_sslCert, _hzGlobal_sslCertCA, true) ;
		if (rc != E_OK)
			Fatal("Failed to init SSL\n") ;
	}

	//	Invoke SEND
	pthread_create(&tidSend, 0, &MonitorSEND, 0) ;
	slog.Log(_fn, "Started SEND Monitor in thread %u\n", tidSend) ;

	//	Create HTTP Server instance
	g_pTheServer = hzIpServer::GetInstance(&slog) ;
	if (!g_pTheServer)
		Fatal("Failed to allocate server instance\n") ;

	/*
	**	Add alien and local SMTP
	*/

	rc = g_pTheServer->AddPortTCP(&ProcAlienSMTP, &HelloSMTP, &FinishSMTP, 900, g_nPortAlienSMTP, g_nMaxConnections, false) ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not add Alien SMTP port (error=%s)\n", Err2Txt(rc)) ; return 0 ; }

	if (g_nPortLocalSMTP)
	{
		//	if (_hzGlobal_sslPvtKey && _hzGlobal_sslCert && _hzGlobal_sslCertCA)
		//		rc = g_pTheServer->AddPortTCP(&ProcLocalSMTP, &HelloSMTP, &FinishSMTP, 900, g_nPortLocalSMTP, g_nMaxConnections, true) ;
		//	else
			rc = g_pTheServer->AddPortTCP(&ProcLocalSMTP, &HelloSMTP, &FinishSMTP, 900, g_nPortLocalSMTP, g_nMaxConnections, false) ;

		if (rc != E_OK)
			{ slog.Log(_fn, "Could not add Local SMTP port (error=%s)\n", Err2Txt(rc)) ; return 0 ; }
	}

	/*
	**	Add POP3 standard and secure
	*/

	rc = g_pTheServer->AddPortTCP(&ProcPOP3, &HelloPOP3, 0, 900, g_nPortPOP3, g_nMaxConnections, false) ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not add POP3 port (error=%s)\n", Err2Txt(rc)) ; return 0 ; }

	if (_hzGlobal_sslPvtKey && _hzGlobal_sslCert && _hzGlobal_sslCertCA)
	{
		rc = g_pTheServer->AddPortTCP(&ProcPOP3, &HelloPOP3, 0, 900, g_nPortPOP3S, g_nMaxConnections, true) ;
		if (rc != E_OK)
			{ slog.Log(_fn, "Could not add POP3 port (error=%s)\n", Err2Txt(rc)) ; return 0 ; }
	}

	/*
	**	Add HTTP and HTTPS
	*/

	//rc = g_pTheServer->AddPortHTTP(&ProcHTTP, 900, theApp->m_nPortSTD, g_nMaxHTTPConnections, false) ;
	rc = g_pTheServer->AddPortHTTP(&ProcHTTP, 900, 19100, g_nMaxHTTPConnections, false) ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not add HTTP port (error=%s)\n", Err2Txt(rc)) ; return 0 ; }

	if (_hzGlobal_sslPvtKey && _hzGlobal_sslCert && _hzGlobal_sslCertCA)
	{
		//rc = g_pTheServer->AddPortHTTP(&ProcHTTP, 900, theApp->m_nPortSSL, g_nMaxHTTPConnections, true) ;
		rc = g_pTheServer->AddPortHTTP(&ProcHTTP, 900, 19101, g_nMaxHTTPConnections, true) ;
		if (rc != E_OK)
			{ slog.Log(_fn, "Could not add HTTP port (error=%s)\n", Err2Txt(rc)) ; return 0 ; }
	}

	//	Activate Server
	if (g_pTheServer->Activate() != E_OK)
	{
		slog.Log(_fn, "HTTP Server cannot run. Please check existing TCP connections with netstat -a\n") ;
		return 0 ;
	}

	slog.Log(_fn, "#\n#\tStarting Epistula 3.0 Server\n#\n") ;
	g_pTheServer->ServeEpollST() ;

shutdown:
	slog.Log(_fn, "Epistula 3.0 Server shuting down\n") ;
	return 0 ;
}
