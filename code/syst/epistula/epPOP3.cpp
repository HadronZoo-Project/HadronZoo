//
//	File:	epPOP3.cpp
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

//#include "hzCtmpls.h"
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
**	Definitions
*/

enum	POP3State
{
	//	POP3 session states

	POP3_EXPECT_AUTH,	//	Expect an AUTH username from client or an APOP
	POP3_EXPECT_PASS,	//	Expect a password from the client
	POP3_EXPECT_CMDS,	//	Expect commands - Normal transaction state
	POP3_EXPECT_QUIT,	//	Expect a QUIT from client
	POP3_TERMINAL		//	QUIT command recieved.
} ;

//hzTcpCode	ProcSMTP	(hzChain& Input, hzIpConnex* pCC) ;
//hzTcpCode	HelloSMTP	(hzIpConnex* pCC) ;
//hzTcpCode	FinishSMTP	(hzIpConnex* pCC) ;

/*
**	Variables
*/

extern	hzLogger		slog ;			//	System logs
extern	pthread_mutex_t	g_LockRS ;		//	Lock for relay task schedule

//	Working (temp) maps and sets
extern	hzMapS	<hzString,hdsInfo*>	g_sessCookie ;		//	Mapping of cookie value to cookie
extern	hzMapS	<hzString,hdsInfo*>	g_sessMember ;		//	Mapping of usernames to cookie
extern	hzMapS	<hzString,mqItem*>	g_TheMailQue ;		//	List of outstanding relay tasks
extern	hzSet	<hzEmaddr>			g_setBanned ;		//	Globally banned senders (users may also ban senders)

extern	hzVect<epAccount*>	g_vecAccount ;			//	All epistula ordinary users (by uid)

extern	epAccount*		g_pSuperuser ;				//	Epistula superuser (for epismail)

//	Ports and limits (set in main)
extern	uint32_t		g_nPortSMTP ;				//	Standard SMTP port (Normally port 25)
extern	uint32_t		g_nPortSMTPS ;				//	Secure SMTP port (Local users only, Normally port 587)
extern	uint32_t		g_nPortPOP3 ;				//	Standard POP3 port (Normally port 110, Epistula defaul 2110, can be any value)
extern	uint32_t		g_nPortPOP3S ;				//	Secure POP3 port (Normally port 995, Epistula uses 2995, can be any value)
extern	uint32_t		g_nMaxConnections ;			//	Number of simultaneous SMTP and POP3 connections
extern	uint32_t		g_nMaxHTTPConnections ;		//	Number of simultaneous HTTP connections

extern	uint32_t		g_nMaxMsgSize ;				//	Max email message size (2 million bytes)
extern	uint32_t		g_nSessIdAuth ;				//	Session ID for AUTH connections
extern	uint32_t		g_nSessIdPop3 ;				//	Session ID for POP3 connections
extern	bool			g_bShutdown ;				//	When set all thread loops must end

//	Fixed messages
//	static	const char*	err_int_nosetuser = "-ERR Internal fault, could not set username\r\n" ;
//	static	const char*	err_int_cmdnosupp = "-ERR Internal fault, command not supported\r\n" ;
//	static	const char*	pop3hello = "+OK Epistula POP3 Server Ready\r\n" ;

extern	hdsApp*			theApp ;			//  Dissemino HTML interface
extern	Epistula*		theEP ;				//	The singleton Epistula Instance

//	extern	hzString	_dsmScript_tog ;
//	extern	hzString	_dsmScript_loadArticle ;
//	extern	hzString	_dsmScript_navtree ;

/*
**	POP3 Definitions
*/

class	POP3Session	: public hzIpConnInfo
{
public:
	//hzMapS<uint64_t,hzNumPair>	msgList ;	//	List of emails in the mailbox
	hzMapS<uint32_t,uint32_t>	msgList ;	//	List of emails in the mailbox

	epAccount*	m_pAcc ;					//	User account
	hzEmaddr	m_Sender ;					//	Sender address
	hzEmaddr	m_Recipient ;				//	Recipient address
	hzString	m_Username ;				//	Username
	hzString	m_Password ;				//	Password
	hzString	m_Pop3Path ;				//	Full path of pop3 file (mbox_dir/pop3)
	hzString	m_Pop3Back ;				//	Full path of pop3 file (mbox_dir/pop3.bak)
	POP3State	m_eState ;					//	Current session state
	uint32_t	m_nPop3Size ;				//	Size of pop3 file
	uint32_t	m_nTotalSize ;				//	Size of all messages
	uint32_t	m_nDeletes ;				//	Number of deletes ordered in POP3 session
	bool		m_bAuth ;					//	Set when authenticated
	char		tmpBuf[HZ_MAXPACKET+4] ;	//	Line buffer for reading mailbox entries

	POP3Session		(void)
	{
		m_nTotalSize = m_nPop3Size = m_nDeletes = 0 ;
		m_bAuth = false ;
	}

	~POP3Session	(void)
	{
	}

	hzEcode	LoadPop3File	(void) ;
	hzEcode	SavePop3File	(void) ;
} ;

/*
**	POP3 Functions
*/

hzEcode		POP3Session::LoadPop3File	()
{
	//	Loads the message list from the POP3 file in the mailbox directory.

	_hzfunc("POP3Session::LoadPop3File") ;

	ifstream	is ;		//	Stream for reading mailbox entries
	FSTAT		fs ;		//	For checking existance of files
	hzNumPair	pair ;		//	Message datum id and size
	uint64_t	msgId ;		//	Message date/id
	uint32_t	datumId ;	//	Message iterator
	uint32_t	nSize ;		//	Message iterator

	//	If POP3 file does not exist, just return
  	if (lstat(*m_Pop3Path, &fs) < 0)
		return E_OK ;

	is.open(*m_Pop3Path) ;
	if (is.fail())
	{
		m_eState = POP3_EXPECT_QUIT ;
		return E_OPENFAIL ;
	}

	if (m_nPop3Size)
		is.seekg(m_nPop3Size) ;
	else
		m_nPop3Size = fs.st_size ;

	for (;;)
	{
		is.getline(tmpBuf, 40) ;
		if (!is.gcount())
			break ;

		//	Get the mail 'date id' and load it into the array
		IsHexnum(msgId, tmpBuf) ;
		IsPosint(datumId, tmpBuf + 2) ;
		IsPosint(nSize, tmpBuf + 13) ;
		msgList.Insert(datumId, nSize) ;
		m_nTotalSize += nSize ;
	}
	is.close() ;
	return E_OK ;
}

hzEcode		POP3Session::SavePop3File	()
{
	//	The pop3 file in a mailbox is appended by ProcSMTP() whenever a new message comes in. Upon the start of a POP3 session, it is loaded in full to the message list. During the
	//	message list load the file is locked so that ProcSMTP() is forced to wait, however this is usually a very short process. The POP3 session supplies messages to the client as
	//	requested, from the message list. Any DELE commands issued by the POP3 client will update the message list. The file is not altered in any way by the POP3 session UNTIL the
	//	session comes to an end. During the session, it is likely that the file has been appended by ProcSMTP(). It is imperitive these additions are not lost.
	//
	//	The process is as follows:-
	//
	//		- Check if any deletes have occured. If not do nothing.
	//		- Lock the pop3 file
	//		- Check if is now larger than it was when it was read.
	//			If the file is the same size and the message list is empty
	//				- delete the file, remove the lock and return.
	//
	//			Else read the rest of the file.
	//		- rename the file to mboxname.pop3.bak and write out a new version of the file from the message list
	//		- remove lock.

	_hzfunc("POP3Session::SavePop3File") ;

	ifstream	is ;		//	Stream for reading mailbox entries
	ofstream	os ;		//	Stream for writing out surviving mailbox entries
	FSTAT		fs ;		//	For checking existance of files
	uint32_t	datumId ;	//	Message iterator
	uint32_t	nSize ;		//	Message iterator
	uint32_t	n ;			//	Message iterator

	//	If no DELE commands have been issues during the session, the pop3 file does not change
	if (!m_nDeletes)
		return E_OK ;

	//	There will be a change so lock the file to delay ProcSMTP() from writing to it
	m_pAcc->m_LockPOP3.Lock() ;

		//	Copy the pop3 file to .bak
		Filecopy(m_Pop3Back, m_Pop3Path) ;

		//	Check to see if the POP3 file has grown during the session
  		lstat(*m_Pop3Path, &fs) ;

		if (m_nDeletes == msgList.Count() && fs.st_size == m_nPop3Size)
			unlink(*m_Pop3Path) ;
		else
		{
			if (fs.st_size > m_nPop3Size)
			{
				//	Read in remainder
				is.open(*m_Pop3Path) ;
				is.seekg(m_nPop3Size) ;

				for (;;)
				{
					is.getline(tmpBuf, 40) ;
					if (!is.gcount())
						break ;

					//	Get the mail 'date id' and load it into the array
					IsHexnum(datumId, tmpBuf + 2) ;
					IsPosint(nSize, tmpBuf + 13) ;
					msgList.Insert(datumId, nSize) ;
					m_nTotalSize += nSize ;
				}
				is.close() ;
			}

			//	Export the message list to the new file
			unlink(*m_Pop3Path) ;
			os.open(*m_Pop3Path, ios::app) ;
			for (n = 0 ; n < msgList.Count() ; n++)
			{
				datumId = msgList.GetKey(n) ;
				nSize = msgList.GetObj(n) ;

				if (datumId && nSize)
				{
					sprintf(tmpBuf, "ep%010u,%010u\n", datumId, nSize) ;
					os << tmpBuf ;
				}
			}
			os.close() ;
		}

	m_pAcc->m_LockPOP3.Unlock() ;
}
		
hzTcpCode	HelloPOP3	(hzIpConnex* pCC)
{
	_hzfunc(__func__) ;

	POP3Session*	pInfo ;		//	POP3 Session info
	hzChain			hello ;		//	Hello msg
	hzString		ipstr ;		//	Ipaddr in text form
	hzXDate			now ;		//	Current time
	hzIpaddr		ipa ;		//	IP address
	
	pInfo = new POP3Session() ;
	pInfo->m_eState = POP3_EXPECT_AUTH ;
	pCC->SetInfo(pInfo) ;

	now.SysDateTime() ;
	ipa = pCC->ClientIP() ;
	ipstr = ipa.Str() ;

	hello << "+OK Epistula POP3 Server Ready\r\n" ;
	pCC->SendData(hello) ;
	pCC->m_Track.Printf("POP3 Client %d Connects from %s on sock %d\n", pCC->EventNo(), *ipstr, pCC->CliSocket()) ;
}

hzTcpCode	ProcPOP3		(hzChain& Input, hzIpConnex* pCC)
{
	//	Handles clinet input within the context of a POP3 Session. This function is called every time there is a new command from the POP3 client.

	_hzfunc(__func__) ;

	hzChain			Z ;						//	For retrieval of POP3 body
	FSTAT			fs ;					//	For checking existance of files
	chIter			zi ;					//	Input chain iterator
	hzChain			tmpCh ;					//	For sending email content
	hzChain			W ;						//	Word/phrase extraction
	hzChain			R ;						//	Response chain
	hdbObject		objEmsg ;				//	The email message database object
	hzAtom			atom ;					//	For authentication
	POP3Session*	pInfo ;					//	POP3 Session info
	hzString		S ;						//	Temp string
	hzIpaddr		ipa ;					//	Client IP address
	hzIpinfo		ipi ;					//	IP status (if blocked)
	uint32_t		nSize ;					//	Mail message POP3 size
	uint32_t		datumId ;				//	POP3 message address
	uint32_t		objId ;					//	For user authentication
	//uint32_t		mailId ;				//	For user authentication
	int32_t			nCount ;				//	Loop control
	int32_t			nMsgNo = 0 ;			//	Mail message number
	bool			bSockerr = false ;		//	Socket error
	bool			bLog = true ;			//	Temp measure to control logging of responses
	hzEcode			rc ;					//	Return code from call to LoadPop3File
	char			spbuf[24] ;

	/*
	**	Initialize connection and say hello
	*/

	ipa = pCC->ClientIP() ;
	//now.SysDateTime() ;
	pInfo = (POP3Session*) pCC->GetInfo() ;

	/*
	**	Handle message
	*/

	for (zi = Input, nCount = 0 ; !zi.eof() && nCount < HZ_MAXPACKET && *zi >= CHAR_SPACE ; zi++, nCount++)
		pInfo->tmpBuf[nCount] = *zi ;
	pInfo->tmpBuf[nCount] = 0 ;

	pCC->m_Track.Printf("POP3 Client %d status %d with %d bytes [%s]\n", pCC->EventNo(), pInfo->m_eState, Input.Size(), pInfo->tmpBuf) ;

	zi = Input ;

	//	Handle notice to quit
	if (zi == "QUIT")
	{
		pInfo->m_eState = POP3_TERMINAL ;
		R += "+OK\r\n" ;
		pInfo->SavePop3File() ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_TERMINATE ;
	}

	//	Handle capabilities request
	if (zi == "CAPA")
	{
		R += "+OK\r\nUSER\r\nUIDL\r\nSTAT\r\n.\r\n" ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == POP3_EXPECT_AUTH)
	{
		//	Expect user to identify themselves

		if (zi == "AUTH")
		{
			//	The LOGIN method - Send 'username' in base 64
			pInfo->m_eState = POP3_EXPECT_AUTH ;
			R.Printf("334 VXNlcm5hbWU6\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "USER ")
		{
			//	Get Realname from hello command

			//	Lookup user/mailbox
			for (zi += 5 ; !zi.eof() && *zi >= CHAR_SPACE ; zi++)
				W.AddByte(*zi) ;
			pInfo->m_Username = W ;
			W.Clear() ;

			if (!pInfo->m_Username)
			{
				pCC->m_Track.Printf("ERROR: Could not set info username\n") ;
				R += "-ERR Internal fault, could not set username\r\n" ;
			}
			else
			{
				pInfo->m_eState = POP3_EXPECT_PASS ;
				R.Printf("+OK %s\r\n", *pInfo->m_Username) ;
				pInfo->m_pAcc = theEP->m_Accounts[pInfo->m_Username] ;
			}

			Input.Clear() ; pCC->SendData(R) ; return TCP_KEEPALIVE ;
		}

		//	Digest not currently supported
		if (zi == "APOP ")
			{ R += "-ERR Internal fault, APOP command not supported\r\n" ; Input.Clear() ; pCC->SendData(R) ; return TCP_KEEPALIVE ; }

		S = Input ;
		pCC->m_Track.Printf("Expected USER or APOP command. Got %s\n", *S) ;
		R += "-ERR Protocol error. Did not understand last command\r\n" ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == POP3_EXPECT_PASS)
	{
		//	Expect Auth or Sender. If there is going to be an Auth this will come first.
		if (!pInfo->m_Username)
		{
			pCC->m_Track.Printf("ERROR: no username set at EXPECT_PASS stage\n") ;
			R += "-ERR internal fault\r\n" ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	At this point error - in this state we only expect a PASS command
		if (zi != "PASS ")
		{
			pCC->m_Track.Printf("ERROR: no password at EXPECT_PASS stage\n") ;
			R += "-ERR expected PASS\r\n" ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		for (zi += 5 ; !zi.eof() && *zi >= CHAR_SPACE ; zi++)
			W.AddByte(*zi) ;
		pInfo->m_Password = W ;
		W.Clear() ;
		pCC->m_Track.Printf("Password for %s is %s\n", *pInfo->m_Username, *pInfo->m_Password) ;

		/*
 		**	Authenticate against subscriber repository
		*/

		theApp->m_Allusers->Identify(objId, SUBSCRIBER_USERNAME, pInfo->m_Username) ;

		if (!objId)
		{
			pInfo->m_eState = POP3_EXPECT_QUIT ;
			pCC->m_Track.Printf("No such user as %s\n", *pInfo->m_Username) ;
		}
		else
		{
			theApp->m_Allusers->Fetchval(atom, SUBSCRIBER_PASSWORD, objId) ;
		
			pCC->m_Track.Printf("Password on record for %s is %s\n", *pInfo->m_Username, *atom.Str()) ;
			if (atom.Str() == pInfo->m_Password)
				{ pInfo->m_bAuth = true ; pInfo->m_eState = POP3_EXPECT_CMDS ; }
			else
				pInfo->m_eState = POP3_EXPECT_QUIT ;
		}

		if (!pInfo->m_bAuth)
		{
			//	Authentication failed
			if (!(GetStatusIP(ipa) & HZ_IPSTATUS_BLACK_POP3))
				pCC->m_Track.Printf("Initial BLOCK on POP3 IP address %s\n", *ipa.Str()) ;
			SetStatusIP(ipa, HZ_IPSTATUS_BLACK_POP3, 900) ;

			R += "-ERR authentication failed\r\n" ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		/*
		**	Authentication complete. Build list of mail items in the mailbox.
		*/

   		pInfo->m_Pop3Path = theEP->m_MailBox + "/" + pInfo->m_Username + "/mbox.pop3" ;
   		pInfo->m_Pop3Back = pInfo->m_Pop3Path + ".bak" ;
		pCC->m_Track.Printf("Mailbox calculated as %s\n", *pInfo->m_Pop3Path) ;

		//	Get list of messages
		rc = pInfo->LoadPop3File() ;
		if (rc == E_OPENFAIL)
		{
			pCC->m_Track.Printf("SERIOUS ERROR. Cannot open mailbox file\n", *pInfo->m_Pop3Path) ;
			R += "-ERR internal fault, list failed\r\n" ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		pCC->m_Track.Printf("Completed checking for mail item %d files\n", pInfo->msgList.Count()) ;

		pInfo->m_eState = POP3_EXPECT_CMDS ;
		R.Printf("+OK %s is cool\r\n", *pInfo->m_Username) ;
		pCC->SendData(R) ;
		Input.Clear() ;
		return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == POP3_EXPECT_CMDS)
	{
		//	Receiving data or the period on a line by itself

		if (zi == "STAT")
		{
			//	Return +OK with the number of messages and the number of bytes
			R.Printf("+OK %u %u\r\n", pInfo->msgList.Count(), pInfo->m_nTotalSize) ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "LIST")
		{
			//	The list command with no arguments (no message number). Respond with +OK and then a line for each message

			R.Clear() ;
			R.Printf("+OK %u %u\r\n", pInfo->msgList.Count(), pInfo->m_nTotalSize) ;
			for (nCount = 0 ; nCount < pInfo->msgList.Count() ; nCount++)
			{
				nSize = pInfo->msgList.GetObj(nCount) ;
				R.Printf("%u %u\r\n", nCount+1, nSize) ;
			}
			R += ".\r\n" ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "LIST ")
		{
			//	The list command with a message number argument supplied. Respond with +OK and then a line for the message, or with -ERR if no such message number.

			nMsgNo = 0 ;
			for (zi += 5 ; !zi.eof() && IsDigit(*zi) ; zi++)
				{ nMsgNo *= 10 ; nMsgNo += (*zi - '0') ; }

			if (nMsgNo > pInfo->msgList.Count())
				R += "-ERR No such message\r\n" ;
			else
			{
				nSize = pInfo->msgList.GetObj(nMsgNo-1) ;
				R.Printf("+OK\r\n%u %u\r\n", nMsgNo, nSize) ;
			}

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "UIDL")
		{
			//	The UIDL command with no argepents (no message number) Respond with +OK and then a line for each message

			R.Clear() ;
			R.Printf("+OK %u %u\r\n", pInfo->msgList.Count(), pInfo->m_nTotalSize) ;

			for (nCount = 0 ; nCount < pInfo->msgList.Count() ; nCount++)
			{
				datumId = pInfo->msgList.GetKey(nCount) ;
				R.Printf("%d ep%010u\r\n", nCount+1, datumId) ;
			}
			R += ".\r\n" ;
			slog.Out(R) ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "UIDL ")
		{
			//	The UIDL command with a message number argepent. Respond with +OK and then a line for the message or with -ERR if no such msg
			nMsgNo = 0 ;
			for (zi += 5 ; !zi.eof() && IsDigit(*zi) ; zi++)
				{ nMsgNo *= 10 ; nMsgNo += (*zi - '0') ; }

			if (nMsgNo > pInfo->msgList.Count())
				R += "-ERR No such message\r\n" ;
			else
			{
				datumId = pInfo->msgList.GetKey(nMsgNo-1) ;
				R.Printf("%d ep%010u\r\n", nCount+1, datumId) ;
			}

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "TOP ")
		{
			//	Send the header and the top x lines of the numbered message 

			nMsgNo = 0 ;
			for (zi += 4 ; !zi.eof() && IsDigit(*zi) ; zi++)
				{ nMsgNo *= 10 ; nMsgNo += (*zi - '0') ; }

			if (nMsgNo < 1 || nMsgNo > pInfo->msgList.Count())
			{
				R += "-ERR No such message (range)\r\n" ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			datumId = pInfo->msgList.GetKey(nMsgNo-1) ;
			nSize = pInfo->msgList.GetObj(nMsgNo-1) ;

			if (!nSize)
			{
				R += "-ERR No such message (deleted)\r\n" ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			//	Fetch full POP3 rendition of the message
			theEP->m_pCronCenter->Fetch(Z, datumId) ;

			//	Deliver the header
			R += "+OK\r\n" ;
			for (zi = Z ;; zi++)
			{
				if (zi == "\r\n\r\n")
					break ;
				if (zi == "\r\n") ;
					{ zi++ ; continue ; }
				R.AddByte(*zi) ;
			}
			R += "\n" ;

			//	This provides Status: header and then a blank line
			R += "Status:   \r\n\r\n" ;
			R += ".\r\n" ;

			pCC->m_Track.Printf("Sent TOP of message %d\n", nMsgNo) ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "RETR ")
		{
			//	Send the numbered message 
			nMsgNo = 0 ;
			for (zi += 5 ; !zi.eof() && IsDigit(*zi) ; zi++)
				{ nMsgNo *= 10 ; nMsgNo += (*zi - '0') ; }

			if (nMsgNo < 1 || nMsgNo > pInfo->msgList.Count())
			{
				R += "-ERR No such message (range)\r\n" ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			datumId = pInfo->msgList.GetKey(nMsgNo-1) ;
			rc = theEP->m_pCronCenter->Fetch(Z, datumId) ;
			if (rc != E_OK)
				pCC->m_Track.Printf("Could not locate whole form message %d\n", datumId) ;

			if (!Z.Size())
			{
				R += "-ERR No such message (deleted)\r\n" ;
				pCC->m_Track.Printf("Could not locate whole form message %d\n", datumId) ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			R << "+OK\r\n" ;
			R << Z ;
			R << "\r\n.\r\n" ;

			bLog = false ;
			pCC->m_Track.Printf("Sent message %d (%d bytes)\n", nMsgNo, R.Size()) ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "DELE ")
		{
			//	This just marks message for delete

			nMsgNo = 0 ;
			for (zi += 5 ; !zi.eof() && IsDigit(*zi) ; zi++)
				{ nMsgNo *= 10 ; nMsgNo += (*zi - '0') ; }

			if (nMsgNo > pInfo->msgList.Count())
				R += "-ERR No such message\r\n" ;
			else
			{
				R += "+OK\r\n" ;
				datumId = pInfo->msgList.GetKey(nMsgNo-1) ;
				nSize = pInfo->msgList.GetObj(nMsgNo-1) ;
				pInfo->msgList.Insert(datumId, nSize) ;
				pInfo->m_nDeletes++ ;
			}

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (zi == "NOOP ")	{ R += "+OK\r\n" ; pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ; }
		if (zi == "RSET ")	{ R += "+OK\r\n" ; pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ; }

		if (zi == "QUIT")
		{
			pInfo->m_eState = POP3_EXPECT_QUIT ;
			R += "+OK\r\n" ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		S = Input ;
		R.Printf("-ERR cannot understand command [%s]\r\n", *S) ;
	}

	if (pInfo->m_eState == POP3_EXPECT_QUIT)
	{
		if (zi != "QUIT")
			R.Printf("-ERR Expected a QUIT command\r\n") ;
		else
			R.Printf("+OK\r\n") ;

		if (!pInfo->m_bAuth)
			pCC->m_Track.Printf("POP3 1: Session ended before mailbox connection established\n") ;
		else
			pInfo->SavePop3File() ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_TERMINATE ;
	}

	//	POP3 State undefined - terminate

	R.Printf("-ERR POP3 State undefined\r\n") ;
	pCC->SendData(R) ;
	Input.Clear() ;
	return TCP_TERMINATE ;
}
