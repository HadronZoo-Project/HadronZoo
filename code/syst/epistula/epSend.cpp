//
//	File:	epSend.cpp
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

//	Synopsis:	Epistula.
//
//	Outgoing emails are placed in a mail que directory by ProcSMTP(). From there the function MonitorSEND(), running in
//	it's own thread, manages the sending process.
//
//	Sending an outgoing email involves one or more relays, depending on how many domains occur in the recipient list. There will be only one relay per domain, with each listing all
//	the recipients for that domain.
//
//	Beacuse mail servers can be offline, relays are queued until they are either concluded or expire. Likewise the email message itself is queued while there are outstanding relays
//	in respect of it.
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
**	Variables
*/

extern	hzMapS	<hzString,mqItem*>	g_TheMailQue ;	//	List of outstanding relay tasks

extern	hdsApp*		theApp ;		//  Dissemino HTML interface
extern	Epistula*	theEP ;			//	The singleton Epistula Instance
extern	hzLogger	logSend ;		//	Logger for sending monitor
extern	hzLockS		g_LockMbox ;	//	Locks g_setMboxLocks
extern	bool		g_bShutdown ;	//	When set all thread loops must end

/*
**	Section X:	Outgoing Relays
*/


void	epRelay::ClearRecipients	(void)
{
	if (!m_CommonRcpt.Count())
		return ;

	hzList<mqRcpt*>::Iter	ri ;
	mqRcpt*	pRecip ;

	for (ri = m_CommonRcpt ; ri.Valid() ; ri++)
	{
		pRecip = ri.Element() ;
		delete pRecip ;
	}

	m_CommonRcpt.Clear() ;
}

void	epRelay::SetResult		(epRlyStatus eResult)
{
	if (m_eResult == RELAY_EXPIRED)
		return ;
	m_eResult = eResult ;
}

void	epRelay::Delay		(int32_t nInterval)
{
	m_timeRun += 3 ;

	if ((m_timeRun - m_timeDue) >= 3600)
		m_eResult = RELAY_EXPIRED ;
}

void	mqItem::Clear	(void)
{
	//	Clear all relay tasks as part of destructor

	epRelay*	pRT ;
	uint32_t	nR ;

	m_Sender = 0 ;
	m_User = 0 ;
	m_Announce = 0 ;
	m_FQDN = 0 ;

	for (nR = 0 ; nR < m_RelayTasks.Count() ; nR++)
	{
		pRT = m_RelayTasks.GetObj(nR) ;
		delete pRT ;
	}

	m_RelayTasks.Clear() ;
}

hzEcode	mqItem::ReadHeaders	(void)
{
	//	Read header part of files (mail items) found in mail que

	_hzfunc("mqItem::ReadHeaders") ;

	ifstream		is ;			//	Input stream
	hzLogger*		plog ;			//	Logger for current thread
	epRelay*		pRT ;			//	Relay task
	mqRcpt*			pRecipient ;	//	Recipient
	hzXDate			Date ;
	hzEmaddr		addr ;
	hzString		domain ;
	char			cvTmp[500] ;
	hzEcode			rc = E_OK ;

	plog = GetThreadLogger() ;

	if (!m_Id)
		{ plog->Log(_fn, "Mail que item has no mail ID set\n") ; return E_NOINIT ; }

	plog->Log(_fn, "Reading Headers for %s\n", *m_Id) ;

	sprintf(cvTmp, "%s/%s.outg", *theEP->m_MailQue, *m_Id) ;

	is.open(cvTmp) ;
	if (is.fail())
	{
		plog->Log(_fn, "Cannot open mail item file (%s) for reading\n", cvTmp) ;
		return E_OPENFAIL ;
	}

	//	Record delivery info

	for (;;)
	{
		is.getline(cvTmp, 128) ;
		if (!is.gcount())
			break ;

		if (cvTmp[0] == 0)
			break ;

		StripCRNL(cvTmp) ;
		plog->Log(_fn, "Line is %s\n", cvTmp) ;

		if (!memcmp(cvTmp, "id: ", 4))
		{
			if (m_Id != (cvTmp + 4))
				plog->Log(_fn, "Id mismatch. Original id differs from that in file\n") ;
			continue ;
		}

		if (!memcmp(cvTmp, "from: ", 6))
		{
			if (!m_Sender)
				m_Sender = cvTmp + 6 ;
			else
			{
				plog->Log(_fn, "Item %s: sender already set to [%s], can't have [%s]\n", *m_Id, *m_Sender, cvTmp + 6) ;
				m_Sender = 0 ;
			}
			continue ;
		}

		if (!memcmp(cvTmp, "username: ", 10))	{ m_User = cvTmp + 10 ;	continue ; }
		if (!memcmp(cvTmp, "realname: ", 10))	{ m_Announce = cvTmp + 10 ;	continue ; }
		if (!memcmp(cvTmp, "ip: ", 4))			{ m_IP = cvTmp + 4 ; continue ; }
		if (!memcmp(cvTmp, "resolved: ", 10))	{ m_FQDN = cvTmp + 10 ; continue ; }

		if (!memcmp(cvTmp, "rcpt: ", 6))
		{
			//	Find the relay task and insert recipient in the relay task's list of recipients
			addr = cvTmp + 6 ;
			if (!addr)
				{ plog->Log(_fn, "Badly formed alien recipient [%s]\n", cvTmp + 6) ; continue ; }
			domain = addr.GetDomain() ;

			pRecipient = new mqRcpt() ;
			if (!pRecipient)
				Fatal("%s - Could not allocate recipient in mail item %s\n", *_fn, *m_Id) ;
			pRecipient->m_Addr = addr ;

			pRT = m_RelayTasks[domain] ;
			if (!pRT)
			{
				pRT = new epRelay() ;
				if (!pRT)
					Fatal("%s - Could not allocate epRelay instance\n", *_fn) ;

				pRT->SetMailID(m_Id) ;
				pRT->SetRealname(m_Announce) ;
				pRT->SetDomain(domain) ;
				if (m_RelayTasks.Insert(domain, pRT) != E_OK)
					Fatal("%s - Could not insert RelayTask in mail item %s\n", *_fn, *m_Id) ;
			}

			pRT->AddRecipient(pRecipient) ;
			continue ;
		}

		//	At this point we deal with invalid entries

		rc = E_FORMAT ;
		plog->Log(_fn, "Item %s has unknown item in header [%s]\n", *m_Id, cvTmp) ;
	}

	is.close() ;

	//	Check if all nessesary info is present

	if (!m_Sender)
		{ rc = E_NOINIT ; plog->Log(_fn, "Item %s has no sender\n", *m_Id) ; }
	if (!m_RelayTasks.Count())
		{ rc = E_NOINIT ; plog->Log(_fn, "Item %s has no relay tasks\n", *m_Id) ; }

	return rc ;
}

hzEcode	mqItem::Bounce	(void)
{
	//	Generate a bounce message from enumerated result of the previous call to RelaySMTP(). This happens if one or more recipients of
	//	an email are deemed non-existant or otherwise in error by this or an alien server. This applies to all emails accepted by SMTP
	//	or SMTP AUTH, regardless of the status of the sender or any of the recipients.

	//	Check if any part of the email item is still due to run.

	_hzfunc("mqItem::Bounce") ;

	hzList<epRelay*>::Iter	iRT ;
	hzList<mqRcpt*>::Iter	ri ;

	hzList<mqRcpt*>	Rcpt ;

	ifstream	is ;				//	Input stream to read failed email's body
	ofstream	os ;				//	Output stream to append POP3 file
	hzLogger*	plog ;				//	Logger for current thread
	mqRcpt*		pRecipient ;
	epRelay*	pRT ;
	hzXDate		d ;					//	For making date strings
	uint32_t	nR ;				//	Relay task iterator
	char		cvBuf	[200] ;		//	General purpose buffer
	char		cvDate	[ 32] ;		//	Date string

	/*
	**	Open the failed email's body file
	*/

	plog = GetThreadLogger() ;

	sprintf(cvBuf, "%s/%s.outg", *theEP->m_MailQue, *m_Id) ;
	is.open(cvBuf) ;
	if (is.fail())
	{
		plog->Log(_fn, "Could not open failed email body for mail id %s\n", *m_Id) ;
		return E_OPENFAIL ;
	}

	//	If there is a username the sender is local and thus has a mailbox. If so
	//	we can open a file directly in the user's mailbox. Otherwise we create a
	//	mail item in a file and send it to the sender via SMTP

	if (*m_User)
	{
		//	Create a POP3 file for the report
		plog->Log(_fn, "SENDING ERROR REPORT to %s (%s)\n", *m_Sender, *m_User) ;
		//	sprintf(cvBuf, "%s/%s/%s_pop3", *g_POP3Dir, *m_User, *m_Id) ;
		//	sprintf(cvPop3, "%s/%s.pop3", *pBox->m_Base, *mailId) ;

		os.open(cvBuf) ;
		if (os.fail())
		{
			plog->Log(_fn, "Could not open user's POP3 mailbox\n") ;
			return E_OPENFAIL ;
		}

		/*
		**	Generate headers for report's POP3 file
		*/

		d.SysDateTime() ;
		sprintf(cvDate, "%s %s %02d %02d:%02d:%02d %04d",
			hz_daynames_abrv[d.Dow()], hz_monthnames_abrv[d.Month() - 1], d.Day(), d.Hour(), d.Min(), d.Sec(), d.Year()) ;
		os << "From Epistula  " << cvDate << "\r\n" ;

		os << "Return-Path: <>" << "\r\n" ;
		os << "    by " << _hzGlobal_Hostname << " (Epistula 1.0.0) id " << *m_Id << "\r\n" ;
		os << "    for " << m_User << "; " << cvDate << "\r\n" ;

		os << "MIME-Version: 1.0\r\n" ;
		os << "Content-Type: multipart/report;\r\n" ;
		os << "    report-type=delivery-status;\r\n" ;
		os << "    boundary=\"----_=_NextPart_" << *m_Id << ".Epistula\"\r\n" ;

		os << "Date: " << cvDate << "\r\n" ;
		os << "From: Epistula Error Reporting <Epistula@" << _hzGlobal_Hostname << ">" << "\r\n" ;
		os << "Message-Id: " << *m_Id << "\r\n" ;
		os << "To: <" << m_Sender << ">" << "\r\n" ;
		os << "Subject: Returned Email Report\r\n\r\n" ;

		/*
		**	Construct body of report here
		*/

		os << "This is a multi-part message in MIME format.\r\n\r\n" ;
		os << "------_=_NextPart_" << *m_Id << ".Epistula\r\n" ;
		os << "Content-Type: text/plain;\r\n\r\n" ;

		os << "This is a report generated by Epistula concerning your email recived\r\n" ;
		os << "by " << _hzGlobal_Hostname << " at " << cvDate << "\r\n\r\n" ;

		for (;;)
		{
			is.getline(cvBuf, 200) ;
			if (is.gcount() == 0)
				break ;
			if (!strncasecmp(cvBuf, "To:", 3))
			{
				os << "To:     " << cvBuf + 3 << "\r\n" ;
				break ;
			}
		}

		for (;;)
		{
			is.getline(cvBuf, 200) ;
			if (is.gcount() == 0)
				break ;
			if (!strchr(cvBuf, ':'))
				os << "         " << cvBuf << "\r\n" ;
			else
			{
				if (!strncasecmp(cvBuf, "Subject:", 8))
					os << "Subject: " << cvBuf + 8 << "\r\n\r\n" ;
				break ;
			}
		}

		//	Pass through relay tasks to provide reasons

		if (!m_RelayTasks.Count())
			os << "The email item had no viable relay tasks\r\n\r\n" ;
		else
		{
			os << "The following addresses did not recieve this message for the following reasons\r\n\r\n" ;

			for (nR = 0 ; nR < m_RelayTasks.Count() ; nR++)
			{
				pRT = m_RelayTasks.GetObj(nR) ;

				switch	(pRT->GetResult())
				{
				case RELAY_COMPLETE:		//	Some recipients failed with permanent errors

					plog->Log(_fn, "Status = RELAY_COMPLETE\n") ;
					for (ri = pRT->m_CommonRcpt ; ri.Valid() ; ri++)
					{
						pRecipient = ri.Element() ;
						if (!pRecipient->m_Reason)
							continue ;
						os << pRecipient->m_Addr << "\r\n" ;
						os << "    " << pRecipient->m_Reason << "\r\n" ;
					}
					break ;

				case RELAY_NODOM:

					plog->Log(_fn, "Status = RELAY_NODOM\n") ;
					os << "The domain " << pRT->GetDomain() << " could not be located by the DNS\r\n" ;
					for (ri = pRT->m_CommonRcpt ; ri.Valid() ; ri++)
					{
						pRecipient = ri.Element() ;
						os << "\t" << pRecipient->m_Addr << "\r\n" ;
					}
					break ;

				case RELAY_BADSENDER:

					plog->Log(_fn, "Status = RELAY_BADSENDER\n") ;
					os << "The mail server at " << pRT->GetDomain() << " has rejected the sender\r\n" ;
					os << "reason: " << pRT->GetReason() << "\r\n" ;
					for (ri = pRT->m_CommonRcpt ; ri.Valid() ; ri++)
					{
						pRecipient = ri.Element() ;
						os << "\t" << pRecipient->m_Addr << "\r\n" ;
					}
					break ;

				case RELAY_TOOLARGE:

					os << "The mail servers at " << pRT->GetDomain() << " have a size limit\r\n" ;
					os << pRT->GetReason() << "\r\n" ;
					for (ri = pRT->m_CommonRcpt ; ri.Valid() ; ri++)
					{
						pRecipient = ri.Element() ;
						os << "\t" << pRecipient->m_Addr << "\r\n" ;
					}
					break ;

				case RELAY_BADPROTO:

					os << "The mail servers at " << pRT->GetDomain() << " have a protocol problem\r\n" ;
					os << pRT->GetReason() << "\r\n" ;
					for (ri = pRT->m_CommonRcpt ; ri.Valid() ; ri++)
					{
						pRecipient = ri.Element() ;
						os << "\t" << pRecipient->m_Addr << "\r\n" ;
					}
					break ;

				case RELAY_EXPIRED:

					plog->Log(_fn, "Status = RELAY_EXPIRED\n") ;
					os << "No SMTP session within the alloted time with the mail servers at " << pRT->GetDomain() << "\r\n" ;
					for (ri = pRT->m_CommonRcpt ; ri.Valid() ; ri++)
					{
						pRecipient = ri.Element() ;
						os << pRecipient->m_Addr << "\r\n" ;
					}
					break ;
				}
			}
		}

		/*
		**	Attach body of errant email here
		*/

		os << "------_=_NextPart_Epistula.Error\r\n" ;
		os << "Content-Type: message/rfc822\r\n\r\n" ;

		is.seekg(0) ;
		for (;;)
		{
			is.getline(cvBuf, 200) ;
			if (is.gcount() == 0)
				break ;
			StripCRNL(cvBuf) ;
			os << cvBuf << "\r\n" ;
		}

		//	Close out

		os << "------_=_NextPart_Epistula.Error\r\n\r\n" ;
		os.close() ;
		is.close() ;
	}
	else
	{
		//	Send an email to the alien sender
		plog->Log(_fn, "SENDING ERROR REPORT to %s (alien)\n", *m_Sender) ;
		sprintf(cvBuf, "%s_error", *m_Id) ;
	}

	//	If sender not local send file to SMTP and then delete it
	return E_OK ;
}

bool	mqItem::DoNextTask	(void)
{
	//	Purpose:		Provide a simple interface for the RelayTaskSchedule function
	//
	//	Method:			1)	Find a relay task that is due to run
	//					2)	Run the task
	//					3)	Check is there are any more tasks. If so return false.
	//					4)	Check if a report needs generating and call Bounce() if so.
	//
	//	Returns:		1)	True if mqItem has completed all tasks.
	//					2)	False otherwise

	_hzfunc("mqItem::DoNextTask") ;

	hzList<epRelay*>::Iter	iRT ;

	epRelay*	pRT ;
	uint32_t	now ;
	uint32_t	nTasksRemaining = 0 ;
	uint32_t	nTasksFailed = 0 ;
	uint32_t	nIndex = 0 ;
	bool		bRelayAction = false ;

	//	Find a due task

	now = time(0) ;

	logSend.Log(_fn, "Have %d relay tasks - ", m_RelayTasks.Count()) ;

	for (nIndex = 0 ; nIndex < m_RelayTasks.Count() ; nIndex++)
	{
		pRT = m_RelayTasks.GetObj(nIndex) ;

		if (pRT->GetResult() == RELAY_DUETORUN || pRT->GetResult() == RELAY_DELAYED)
		{
			if (now >= pRT->m_timeRun)
			{
				//	Due task found
				bRelayAction = true ;
				RelaySMTP(pRT) ;
				break ;
			}
		}
	}

	//	Find out status of whole email item

	if (!bRelayAction)
	{
		logSend.Out("no action taken\n") ;
		sleep(10) ;
		return false ;
	}

	logSend.Out("proceeding ...\n") ;

	for (nIndex = 0 ; nIndex < m_RelayTasks.Count() ; nIndex++)
	{
		pRT = m_RelayTasks.GetObj(nIndex) ;

		switch (pRT->GetResult())
		{
		case RELAY_DUETORUN:
		case RELAY_DELAYED:		nTasksRemaining++ ;
								break ;

		case RELAY_SUCCESS:		break ;

		case RELAY_COMPLETE:	nTasksFailed++ ;
								break ;

		case RELAY_NODOM:
		case RELAY_BADSENDER:
		case RELAY_TOOLARGE:
		case RELAY_BADPROTO:
		case RELAY_EXPIRED:		nTasksFailed++ ;
								break ;
		}
	}

	if (nTasksRemaining)
	{
		logSend.Log(_fn, "have %d tasks remaining\n", nTasksRemaining) ;
		return false ;
	}

	//	No more tasks remain. Do we need to send a report?

	if (!nTasksFailed)
		logSend.Log(_fn, "Complete\n") ;
	else
	{
		logSend.Log(_fn, "bounced\n") ;
		Bounce() ;
	}

	return true ;
}

bool	DoRelay	(epRelay* pRT, hzResServer& Svr, hzString& Sender, const char* cvMailID)
{
	//	Relay email contents to an SMTP server

	_hzfunc(__func__) ;

	static	char	cvBuf	[HZ_BLOCKSIZE] ;	//	Outgoing data buffer
	static	char	cvReq	[HZ_BLOCKSIZE] ;	//	Request buffer
	static	char	cvTmp	[64] ;

	hzList<mqRcpt*>::Iter	ri ;				//	Iterator of recipients

	SOCKADDRIN	SvrAddr ;				//  Address of real server

	TIMVAL		tv ;						//  For setting timeouts
	ifstream	is ;						//	Staging file
	hzXDate		date ;						//	For date/time stamp
	mqRcpt*		pRecipient ;

	char*		i ;							//	Iterator for filling send buffer
	int32_t		Sock ;						//	Socket to target alien server
	int32_t		nRecv ;						//	Bytes recieved
	int32_t		nRead ;						//	Bytes read
	int32_t		n2Write ;					//	Bytes to write
	int32_t		nWritten ;					//	Bytes written by write()
	int32_t		nTotal = 0 ;				//	Total bytes of message
	int32_t		nRecipOK ;					//	Number of recipients initialy accepted by the target server
	hzRecep08	ipbuf ;						//	IP address as 4 bytes
	epRlyStatus	rs ;						//	Relay status (to be returned)
	hzEcode		rc ;						//	Return code

	/*
	**	Prepare connection to alien mail server
	*/

	//	Set IP address and port, obtain socket, set socket options and connect
	memset(&SvrAddr, 0, sizeof(SvrAddr)) ;
	SvrAddr.sin_family = AF_INET ;
	memcpy(&SvrAddr.sin_addr, Svr.m_Ipa.AsBytes(ipbuf), 4) ;
	SvrAddr.sin_port = htons(25) ;

	if ((Sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{ logSend.Log(_fn, "Cannot create mail server socket for %s action delayed\n", *pRT->GetDomain()) ; return false ; }

	tv.tv_sec = 40 ;
	tv.tv_usec = 0 ;

	if (setsockopt(Sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		{ logSend << "Could not set socket options\n" ; return false ; }

	if (connect(Sock, (struct sockaddr *) &SvrAddr, sizeof(SvrAddr)) < 0)
		{ logSend.Log(_fn, "Cannot connect to mail server for %s: - action delayed\n", *pRT->GetDomain()) ; return false ; }

	logSend.Log(_fn, "Connected to dest mail server\n") ;

	/*
	**	Open  mail que file using m_Id as the basis for the file name
	*/

	sprintf(cvTmp, "%s/%s.outg", *theEP->m_MailQue, cvMailID) ;
	is.open(cvTmp) ;
	if (is.fail())
	{
		rs = RELAY_INT_ERR ;
		pRT->SetReason("Cannot open message file") ;
		pRT->SetResult(rs) ;
		logSend.Log(_fn, "Cannot open file %s\n\n", cvTmp) ;
		goto quitPerm ;
	}

	/*
	**	Expect Greeting
	*/

	if ((nRecv = recv(Sock, cvReq, 1024, 0)) <= 0)
	{
		rs = RELAY_DELAYED ;
		pRT->Delay() ;
		pRT->SetResult(rs) ;
		logSend << "Read error on socket whilst awaiting greeting\n" ;
		goto quitTemp ;
	}
	cvReq[nRecv] = 0 ;
	logSend.Log(_fn, "Alien Helo <- %s\n", cvReq) ;

	//	Any 400 series respose causes a delay
	if (cvReq[0] == '4')
		{ logSend << "Relay delayed\n" ; pRT->Delay() ; pRT->SetResult(RELAY_DELAYED) ; goto quitTemp ; }

	//	A 220 'Bad Sender' at this point means this server is blacklisted by the recipient server and so is a conclusive fail
	if (memcmp(cvReq, "220", 3))
		{ rs = RELAY_BADSENDER ; logSend.Log(_fn, "Bad sender [%s]\n", cvReq) ; pRT->SetResult(RELAY_BADSENDER) ; pRT->SetReason(cvReq) ; goto quitPerm ; }

	/*
	**	Issue EHLO command
	*/

	sprintf(cvReq, "HELO %s\r\n", *_hzGlobal_Hostname) ;
	logSend.Log(_fn, "Local Send -> %s", cvReq) ;
	if (write(Sock, cvReq, strlen(cvReq)) <= 0)
	{
		logSend << "Write failed sending HELO\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		rs = RELAY_DELAYED ;
		goto quitTemp ;
	}

	//	Expect Status codes
	if ((nRecv = recv(Sock, cvReq, 1024, 0)) <= 0)
	{
		logSend << "Recv failed while expecting status codes\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		rs = RELAY_DELAYED ;
		goto quitTemp ;
	}
	cvReq[nRecv] = 0 ;
	logSend.Log(_fn, "Alien Repl <- %s\n", cvReq) ;

	if (cvReq[0] == '4')
	{
		//	Any 400 series respose causes a delay
		logSend.Log(_fn, "Relay delayed [%s]\n", cvReq) ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}

	if (strncasecmp(cvReq, "250", 3))
	{
		//	Any response other than 250 will be considered a conclusive fail
		rs = RELAY_BADPROTO ;
		pRT->SetResult(rs) ;
		pRT->SetReason(cvReq) ;
		logSend.Log(_fn, "%s - Expected Status Codes, got [%s]\n", __FUNCTION__, cvReq) ;
		goto quitPerm ;
	}

	/*
	**	State the Sender
	*/

	sprintf(cvReq, "MAIL FROM: <%s>\r\n", *Sender) ;
	logSend.Log(_fn, "Local Send -> %s", cvReq) ;
	if (write(Sock, cvReq, strlen(cvReq)) <= 0)
	{
		logSend << "Write failed during MAIL FROM command\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}

	//	Expect Sender OK
	if ((nRecv = recv(Sock, cvReq, 1024, 0)) <= 0)
	{
		logSend << "Recv failed while expecting 'sender ok' or error\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}
	cvReq[nRecv] = 0 ;
	logSend.Log(_fn, "Alien Repl <- %s\n", cvReq) ;

	if (cvReq[0] == '4')
	{
		rs = RELAY_DELAYED ;
		logSend.Log(_fn, "Relay delayed [%s]\n", cvReq) ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}

	if (strncasecmp(cvReq, "250", 3))
	{
		rs = RELAY_BADSENDER ;
		pRT->SetResult(RELAY_BADSENDER) ;
		pRT->SetReason(cvReq) ;
		logSend.Log(_fn, "Expected Sender OK. Got [%s]\n", cvReq) ;
		goto quitPerm ;
	}

	/*
	**	Send list of recipients
	*/

	nRecipOK = 0 ;
	for (ri = pRT->m_CommonRcpt ; ri.Valid() ; ri++)
	{
		pRecipient = ri.Element() ;

		sprintf(cvReq, "RCPT TO: <%s>\r\n", *pRecipient->m_Addr) ;
		logSend.Log(_fn, "Local Send -> %s", cvReq) ;

		if (write(Sock, cvReq, strlen(cvReq)) <= 0)
		{
			logSend << "Write failed during RCPT TO command\n" ;
			pRT->Delay() ;
			pRT->SetResult(RELAY_DELAYED) ;
			goto quitTemp ;
		}

		//	Expect recipient OK
		if ((nRecv = recv(Sock, cvReq, 1024, 0)) <= 0)
		{
			logSend << "Recv failed while expecting recipient ok or error\n" ;
			pRT->Delay() ;
			pRT->SetResult(RELAY_DELAYED) ;
			goto quitTemp ;
		}
		cvReq[nRecv] = 0 ;
		logSend.Log(_fn, "Alien Repl <- %s\n", cvReq) ;

		if (!strncasecmp(cvReq, "250", 3))
			nRecipOK++ ;
		else
		{
			logSend.Log(_fn, "Expected Recipient OK, Got [%s]\n", cvReq) ;
			//if (!memcmp(cvReq, "421", 3))
			if (cvReq[0] == '4')
			{
				pRT->Delay() ;
				pRT->SetResult(RELAY_DELAYED) ;
				goto quitTemp ;
			}

			//if (cvReq[0] == '5')
			pRecipient->m_Reason = cvReq ;
		}
	}

	/*
	**	If some recipients have been accepted then send DATA command otherwise QUIT
	*/
	
	if (!nRecipOK)
	{
		logSend << "None of the recipients were successful\n" ;
		rs = RELAY_COMPLETE ;
		pRT->SetResult(rs) ;
		goto quitOk ;
	}

	/*
	**	Send DATA command
	*/

	logSend.Log(_fn, "Local Send -> DATA\n") ;
	sprintf(cvReq, "DATA\r\n") ;

	if (write(Sock, cvReq, strlen(cvReq)) <= 0)
	{
		logSend << "Write failed during DATA command\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}

	/*
	**	Expect the period on a line by itself notice
	*/

	if ((nRecv = recv(Sock, cvReq, 1024, 0)) <= 0)
	{
		logSend << "Recv failed while expecting 'period on line by itself' message\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}
	cvReq[nRecv] = 0 ;
	logSend.Log(_fn, "Alien Repl <- %s\n", cvReq) ;

	if (memcmp(cvReq, "354", 3))
	{
		if (memcmp(cvReq, "250", 3))
		{
			logSend.Log(_fn, "Expected a 250 OK or a 354 Period on line by itself thing. Got [%s]\n", cvReq) ;
			pRT->Delay() ;
			pRT->SetResult(RELAY_DELAYED) ;
			goto quitTemp ;
		}
	}

	date.SysDateTime() ;

	sprintf(cvBuf, "Received: from <%s>\r\n    by %s (Epistula 1.0.0) ESMTP id %s\r\n    %s\r\n",
		*Sender, *_hzGlobal_Hostname, cvMailID, *date.Str(FMT_DT_INET)) ;
	logSend.Log(_fn, "Local Send -> %s", cvBuf) ;
	if (write(Sock, cvBuf, strlen(cvBuf)) < 0)
	{
		logSend << "Write failed while sending initial epistula headings (first part of msg)\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}

	/*
	**	Send the mail body
	*/

	//	First skip headers
	for (;;)
	{
		is.getline(cvBuf, 2048) ;
		if (!is.gcount())
			break ;
		StripCRNL(cvBuf) ;
		if (cvBuf[0] == 0)
			break ;
		//threadLog("Skiping header [%s]\n", cvBuf) ;
	}

	//	Then read body content
	for (;;)
	{
		for (i = cvBuf, n2Write = 0 ; n2Write < 900 ;)
		{
			is.getline(i, 2048) ;
			if (!is.gcount())
				break ;
			nRead = StripCRNL(i) ;
		//threadLog("Using line [%s]\n", cvBuf) ;
			i += nRead ;
			n2Write += nRead ;
			*i++ = '\r' ;
			*i++ = '\n' ;
			n2Write += 2 ;
		}

		if (!n2Write)
			break ;

		cvBuf[n2Write] = 0 ;
		nTotal += n2Write ;
		//Report.Printf("Local Send -> Data (%d bytes) total %d bytes of %s ...", n2Write, nTotal, cvMailID) ;

		nWritten = write(Sock, cvBuf, n2Write) ;
		if (nWritten != n2Write)
		{
			/*
			**	We have a write error during data send
			*/

			if ((nRecv = recv(Sock, cvReq, 1024, 0)) > 0)
			{
				if (!memcmp(cvReq, "552", 3))
				{
					pRT->SetReason(cvReq) ;
					pRT->SetResult(RELAY_TOOLARGE) ;
					goto quitPerm ;
				}
			}

			logSend << "Socket error while relaying message body\n" ;
			pRT->Delay() ;
			pRT->SetResult(RELAY_DELAYED) ;
			goto quitTemp ;
		}
		logSend.Log(_fn, "done %d bytes\n", nWritten) ;
	}

	/*
	**	Send the period on a line by itself
	*/

	logSend.Log(_fn, "Local Send -> . on line by itself ...") ;
	if (write(Sock, "\r\n.\r\n", 5) <= 0)
	{
		logSend << "Socket error while relaying period on line by itself\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}
	logSend.Log(_fn, "done\n") ;

	/*
	**	Expect 250 message accepted for delivery
	*/

	if ((nRecv = recv(Sock, cvReq, 1024, 0)) <= 0)
	{
		logSend << "Read error while expecting 250 message accepted\n" ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		goto quitTemp ;
	}
	cvReq[nRecv] = 0 ;
	logSend.Log(_fn, "Alien Repl <- %s\n", cvReq) ;

	if (strncasecmp(cvReq, "250", 3))
	{
		logSend.Log(_fn, "Expected 250 message accepted Got <- %s\n", cvReq) ;

		if (cvReq[0] == '4')
		{
			logSend << "Relay delayed\n" ;
			pRT->Delay(600) ;
			pRT->SetResult(RELAY_DELAYED) ;
			goto quitTemp ;
		}

		logSend.Log(_fn, "Expected 250 message accepted Got <- %s\n", cvReq) ;
		pRT->SetResult(RELAY_BADPROTO) ;
		pRT->SetReason(cvReq) ;
		goto quitPerm ;
	}

	logSend.Log(_fn, "Message %s delivered to %s\n", *pRT->GetMailID(), *pRT->GetDomain()) ;

	/*
	**	Issue QUIT command
	*/

quitOk:
	//	The email was a success so send a QUIT command. If sending this fails or recieving an ack to the QUIT fails, such failures are of no consequence.
	logSend.Log(_fn, "Local Send -> QUIT\n") ;
	if (write(Sock, "QUIT\r\n", 6) <= 0)
		logSend << "Write failed while sending QUIT command (ignored)\n" ;
	else
	{
		if ((nRecv = recv(Sock, cvReq, 1024, 0)) <= 0)
			logSend << "Read error while expecting ack to quit (ignored)\n" ;
		else
		{
			cvReq[nRecv] = 0 ;
			logSend.Log(_fn, "Alien Repl <- %s\n", cvReq) ;

			if (strncasecmp(cvReq, "221", 3))
				logSend.Log(_fn, "Expected 221 Ack of QUIT (ignored), Got <- %s\n", cvReq) ;
		}
	}

	if (Sock > 0)
		close(Sock) ;
	is.close() ;
	pRT->SetResult(RELAY_SUCCESS) ;
	return true ;

quitPerm:
	//	This is where a socket error has occured. Return 0 for a temporary failure.
	logSend.Log(_fn, "Local Send -> QUIT\n") ;
	write(Sock, "QUIT\r\n", 6) ;
	if (Sock > 0)
		close(Sock) ;
	is.close() ;
	logSend << "Relay failed\n" ;
	return true ;

quitTemp:
	//	A temporary error has occured such as a 421 from the alien.
	logSend.Log(_fn, "Local Send -> QUIT (Error)\n") ;
	if (write(Sock, "QUIT\r\n", 6) <= 0)
		logSend << "Write failed while sending QUIT command (ignored)\n" ;
	else
	{
		if ((nRecv = recv(Sock, cvReq, 1024, 0)) <= 0)
			logSend << "Read error while expecting ack to quit (ignored)\n" ;
	}

	if (nRecipOK < pRT->GetNoRecipients())
		pRT->SetResult(RELAY_COMPLETE) ;

	if (Sock > 0)
		close(Sock) ;
	is.close() ;

	logSend << "Relay quit in error\n" ;
	return true ;

quitSock:
	//	This is where a socket error has occured. Return 0 for a temporary failure.
	if (Sock > 0)
		close(Sock) ;
	is.close() ;
	logSend << "Relay abandoned\n" ;
	return true ;
}

void	mqItem::RelaySMTP	(epRelay* pRT)
{
	_hzfunc("mqItem::RelaySMTP") ;

	hzList<hzResServer>			ServersMX ;	//	Server names from MX query
	hzList<hzResServer>::Iter	iSvr ;		//	Server interator

	struct  hostent*	pHost ;				//  Hostname of real server
	struct  addrinfo*	pAddrInfo ;			//  Info on target server
	struct  addrinfo*	pAIRes ;			//  Info results on target server
	struct  sockaddr_in	SvrAddr ;			//  Address of real server

	hzLogger*		plog ;					//	Current thread logger
	hzResServer		Svr ;					//	Server instance
	TIMVAL			tv ;					//  For setting timeouts
	hzDNS			dq ;					//	DNS query structure
	DnsRec*			dr ;					//	DNS record
	const char*		pEMAddr ;				//	For obtaining email address of a recipient in the staging file
	char*			cpServ ;
	int32_t			nIndex ;				//	For looping through DNS records
	int32_t			nAns ;					//	Number of mail servers (max 8)
	int32_t			nAns2 ;					//	Number of PTRs for a mail server
	int32_t			nServers = 0 ;			//	Number of mail servers found
	int32_t			Sock ;					//	Socket to target alien server
	int32_t			rc ;					//	Return code from SMTP converstion
	bool			bFound ;				//	Found a match between MX and additional records
	bool			bConnect = false ;		//	Did we connect?
	hzEcode			drc ;					//	Return code from DNS lookup functions

	plog = GetThreadLogger() ;

	//	Initialize result.
	pRT->SetResult(RELAY_SUCCESS) ;

	if (!pRT->GetDomain())
	{
		plog->Log(_fn, "No domain in relay task object\n") ;
		pRT->SetReason("No domain in relay task object") ;
		pRT->SetResult(RELAY_NODOM) ;
		return ;
	}

	plog->Log(_fn, "Relaying %s to domain %s\n", *m_Id, *pRT->GetDomain()) ;

	//	Lookup MX records in DNS
	m_Report.Clear() ;
	drc = dq.SelectMX(ServersMX, pRT->GetDomain()) ;

	if (drc == E_DNS_NOHOST)
	{
		plog->Log(_fn, "DNS MX error (DNS_HOST_NOT_FOUND)\n") ;
		pRT->SetReason("No such domain") ;
		pRT->SetResult(RELAY_NODOM) ;
		return ;
	}
	if (drc == E_DNS_NODATA)
	{
		plog->Log(_fn, "DNS MX error (DNS_NO_DATA)\n") ;
		pRT->SetReason("No mail servers for domain") ;
		pRT->SetResult(RELAY_NODOM) ;
		return ;
	}
	if (drc == E_DNS_FAILED)
	{
		plog->Log(_fn, "DNS MX error (DNS_NO_RECOVERY)\n") ;
		pRT->SetReason("Invalid DNS entry for domain") ;
		pRT->SetResult(RELAY_NODOM) ;
		return ;
	}
	if (drc == E_DNS_RETRY)
	{
		plog->Log(_fn, "DNS MX error (DNS_TRY_AGAIN)\n") ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
		return ;
	}

	//	If we have servers do the relay, otherwise abort
	if (!ServersMX.Count())
	{
		pRT->SetResult(RELAY_NODOM) ;
		plog->Log(_fn, "DNS found no MX servers for the %s domain\n", *pRT->GetDomain()) ;
		pRT->SetReason("No MX records from DNS") ;
		return ;
	}

	plog->Log(_fn, "We have %d mail servers\n", ServersMX.Count()) ;

	/*
	**	Starting with 'best' server, iterate thru list of servers and try to connect
	*/

	for (iSvr = ServersMX ; !bConnect && iSvr.Valid() ; iSvr++)
	{
		Svr = iSvr.Element() ;

		if (Svr.m_Ipa == IPADDR_NULL || Svr.m_Ipa == IPADDR_LOCAL)
		{
			plog->Log(_fn, "Skipping mailserver %s [v=%d] [ip=%s] (would loop back to server)\n", *Svr.m_Servername, Svr.m_nValue, *Svr.m_Ipa.Str()) ;
			continue ;
		}

		plog->Log(_fn, "Using mailserver %s [v=%d] [ip=%s]\n", *Svr.m_Servername, Svr.m_nValue, *Svr.m_Ipa.Str()) ;

		m_Report.Clear() ;
		bConnect = DoRelay(pRT, Svr, m_Sender, *m_Id) ;
	}

	if (!bConnect)
	{
		plog->Log(_fn, "Could not establish a connection at this point, relay delayed\n\n") ;
		pRT->Delay() ;
		pRT->SetResult(RELAY_DELAYED) ;
	}
}

void*	MonitorSEND	(void* pThreadArg)
{
	//	Processes outgoing emails (files) in the mail que directory.
	//
	//	Method:		Runs concurrent to the SMTP server in it's own thread. For each mail item in the que,
	//				process one relay task. When all the relay tasks for a given mail item have completed,
	//				call the mail item's Report function. This will generate an email if any of the items
	//				failed to reach thier destination.
	//
	//	Note:		Emails that cannot be delivered because the server is down are not considered to have
	//				failed until 72 hours have elapsed. The sender is not notified of this until 1 hour
	//				has elapsed and will not be notifed again unless the full 72 hours elapses.
	//
	//	Arguments:	void*	(as required by threads)
	//	Returns:	void*	(as required by threads)

	_hzfunc("MonitorSEND") ;

	hzProcess		_procSend ;			//	HadronZoo process
	hzProcInfo*		pPD ;				//	Process descriptor
	FSTAT			fs ;				//	For time-date stamp checking
	dirent*			pDE ;				//	Directory entry
	DIR*			pDir ;				//	Open directory
	mqItem*			pMQ ;				//	Mail item (email) in mail que
	hzString		mailId ;			//	Mail id derived from filenames (stripped of .outg)
	hzString		filename ;			//	Of file(s) in mail que dir
	uint32_t		now ;				//	Time now
	uint32_t		dirCk = 0 ;			//	Time to check mail que dir for new emails
	int32_t			last_mtime = 0 ;	//	Last recorded m-time of mail que directory.
	int32_t			nIndex ;			//	Iterator for mail que
	bool			bFound ;
	char			cvSerno	[32] ;		//	extra chars to allow for extn
	char			cvTmp	[80] ;
	char			cvNext	[80] ;

	pPD = (hzProcInfo*) pThreadArg ;

	SetThreadLogger(&logSend) ;
	logSend.OpenFile("/home/epistula/logs/send", LOGROTATE_DAY) ;
	logSend.Verbose(false) ;
	logSend.Log(_fn, "Starting Epistula mail sending agent\n") ;

	//	Proceed
	for (; !g_bShutdown ;)
	{
		now = time(0) ;

		if (now > dirCk)
		{
			dirCk = now + 15 ;

			//	Check central database for new relay tasks
			if (stat(*theEP->m_MailQue, &fs) < 0)
			{
				logSend.Log(_fn, "Serious Error - Cannot stat mailque directory (%s)\n", *theEP->m_MailQue) ;
				g_bShutdown = true ;
				break ;
			}

			if (fs.st_mtime != last_mtime)
			{
				last_mtime = fs.st_mtime ;

				//	Open the directory
				if (!(pDir = opendir(*theEP->m_MailQue)))
				{
					logSend.Log(_fn, "Cannot open mail que\n") ;
					g_bShutdown = true ;
					break ;
				}

				//	Read the entries
				for (; pDE = readdir(pDir) ;)
				{
					if (!strcmp(pDE->d_name, "."))	continue ;
					if (!strcmp(pDE->d_name, ".."))	continue ;

					filename = theEP->m_MailQue + "/" + pDE->d_name ;

					if (stat(*filename, &fs) == -1)
					{
						logSend.Log(_fn, "Serious Error - Cannot stat mail que file (%s/%s)\n", *filename) ;
						closedir(pDir) ;
						g_bShutdown = true ;
						break ;
					}

					//	Ignore sub-directories and non-file directory enrties
					if (S_ISDIR(fs.st_mode))	continue ;
					if (!S_ISREG(fs.st_mode))	continue ;

					//	Eliminate any other files which are not mail item headers
					if (!strstr(pDE->d_name, ".outg"))
						continue ;

					//	Apply filter to eliminate already included mail items
					memcpy(cvSerno, pDE->d_name, 16) ;
					cvSerno[16] = 0 ;
					mailId = cvSerno ;

					pMQ = g_TheMailQue[mailId] ;
					if (pMQ)
						continue ;

					//	Schedule new items
					pMQ = new mqItem() ;
					pMQ->SetId(mailId) ;

					logSend.Log(_fn, "Created mail item instance %s\n", *mailId) ;

					if (pMQ->ReadHeaders() != E_OK)
					{
						logSend.Log(_fn, "Read header operation failed, item %s excluded\n", *mailId) ;
						sprintf(cvTmp, "%s/%s.outg", *theEP->m_MailQue, *mailId) ;
						sprintf(cvNext, "%s/%s.fail", *theEP->m_MailQue, *mailId) ;
						if (rename(cvTmp, cvNext))
							logSend.Log(_fn, "Could not rename %s to %s\n", cvTmp, cvNext) ;

						continue ;
					}

					if (g_TheMailQue.Insert(mailId, pMQ) != E_OK)
					{
						logSend.Log(_fn, "Could not insert item into mail que\n") ;
						g_bShutdown = true ;
						break ;
					}

					logSend.Log(_fn, "Inserted a new mail item header file [%s]\n", *mailId) ;
				}
				closedir(pDir) ;
			}
		}

		//	If que empty do nothing

		if (!g_TheMailQue.Count())
			{ sleep(5) ; continue ; }

		//	Move thru the mail items and for each one call 'DoNextTask'
		logSend.Log(_fn, "Have %d items in mail que\n", g_TheMailQue.Count()) ;

		for (nIndex = 0 ; nIndex < g_TheMailQue.Count() ; nIndex++)
		{
			mailId = g_TheMailQue.GetKey(nIndex) ;
			pMQ = g_TheMailQue.GetObj(nIndex) ;

			logSend.Log(_fn, "Doing mail que item %d: %s\n", nIndex, *mailId) ;

			if (pMQ->DoNextTask())
			{
				//	Last task completed delete mail item files
				sprintf(cvTmp, "%s/%s.outg", *theEP->m_MailQue, *mailId) ;
				sprintf(cvNext, "%s/%s.sent", *theEP->m_MailQue, *mailId) ;
				if (rename(cvTmp, cvNext))
					logSend.Log(_fn, "Could not rename %s to %s\n", cvTmp, cvNext) ;

				g_TheMailQue.Delete(mailId) ;
				delete pMQ ;

				logSend.Log(_fn, "Mail Item removed from Schedule [%s]\n", *mailId) ;
				nIndex-- ;
			}
		}
	}

	logSend.Log(_fn, "Epistula-Send shuting down\n") ;
	delete pPD ;
	pthread_exit(pThreadArg) ;
	return pThreadArg ;
}

/*
**	epMbox class members
*/

#if 0
bool	epAccount::BuildList	(bool bDetails)

bool	epAccount::GetMailitemDetails	(epMBItem& Item, const char* cpFilename)
{
	//	Looks for mail item files in the mail que directory and loads them into the que

	_hzfunc("epAccount::GetMailitemDetails") ;

	ifstream	is ;

	hzChain	Z ;						//	For all recipients
	char*	i ;						//	For processing cvLine
	char*	j ;						//	For processing cvBuf
	char	cvBuf	[128] ;			//	Gen purpose buffer
	char	cvLine	[512] ;			//	Line buffer for getline
	bool	bFrom = false ;			//	The sender
	bool	bTo = false ;			//	Your email account
	bool	bAll = false ;			//	All recipients
	bool	bSubject = false ;		//	The subject
	bool	bPriority = false ;		//	Priority
	bool	bAttach = false ;		//	Attachment?
	bool	bTagged = false ;		//	Tagged for reply?

	sprintf(cvBuf, "%s/%s/%s", *g_MailBox, *m_Unam, cpFilename) ;
	is.open(cvBuf) ;
	if (is.fail())
	{
		threadLog("%s - Could not open file [%s]\n", *_fn, cvBuf) ;
		return false ;
	}

	for (;;)
	{
		is.getline(cvLine, 512) ;
		if (!is.gcount())
			break ;

		if (!memcmp(cvLine, "Return-Path: <", 14))
		{
			i = cvLine + 14 ;
			j = cvBuf ;
			for (; *i != CHAR_MORE ; *j++ = *i++) ;
			*j = 0 ;
			Item.m_From = cvBuf ;
			bFrom = true ;
			continue ;
		}

		if (!memcmp(cvLine, "    for <", 9))
		{
			i = cvLine + 9 ;
			j = cvBuf ;
			for (; *i != CHAR_MORE ; *j++ = *i++) ;
			*j = 0 ;
			Item.m_To = cvBuf ;
			bTo = true ;
		}

		if (!memcmp(cvLine, "To: ", 4))
		{
			if (!strchr(cvLine + 4, CHAR_COMMA))
			{
				Z = cvLine + 4 ;
				Item.m_All = Z ;	//SetAllRecipients(Z) ;
				bAll = true ;
				continue ;
			}

			//	Multiple recipients, build zone
			Z += (cvLine + 4) ;
			for (;;)
			{
				is.getline(cvLine, 512) ;
				if (is.gcount() == 0)
					break ;
				if (!memcmp(cvLine, "    ", 4))
				{
					Z += (cvLine + 4) ;
					continue ;
				}

				//	We are now at end of recipient list
				break ;
			}

			Item.m_All = Z ; 	//SetAllRecipients(Z) ;
			bAll = true ;
		}

		if (!memcmp(cvLine, "Subject: ", 9))
			{ Item.m_Subject = cvLine + 9 ; bSubject = true ; }

		if (bFrom && bTo && bAll && bSubject)
			break ;
	}

	is.close() ;
	return true ;
}

bool	epAccount::GetMailitemContent	(epMBItem& Item, const char* cpFilename)
{
	//	Looks for mail item files in the mail que directory and loads them

	_hzfunc("epAccount::GetMailitemContent") ;

	ifstream	is ;

	hzChain	Z ;						//	For all recipients
	char*	i ;						//	For processing cvLine
	char*	j ;						//	For processing cvBuf
	char	cvBuf	[128] ;			//	Gen purpose buffer
	char	cvLine	[512] ;			//	Line buffer for getline
	bool	bFrom = false ;			//	The sender
	bool	bTo = false ;			//	Your email account
	bool	bAll = false ;			//	All recipients
	bool	bSubject = false ;		//	The subject
	bool	bPriority = false ;		//	Priority
	bool	bAttach = false ;		//	Attachment?
	bool	bTagged = false ;		//	Tagged for reply?

	sprintf(cvBuf, "%s/%s/%s", *theEP->m_MailBox, *m_Unam, cpFilename) ;
	is.open(cvBuf) ;
	if (is.fail())
	{
		threadLog("%s - Could not open file [%s]\n", *_fn, cvBuf) ;
		return false ;
	}

	for (;;)
	{
		is.getline(cvLine, 512) ;
		if (!is.gcount())
			break ;

		if (!memcmp(cvLine, "Return-Path: <", 14))
		{
			i = cvLine + 14 ;
			j = cvBuf ;
			for (; *i != CHAR_MORE ; *j++ = *i++) ;
			*j = 0 ;
			Item.m_From = cvBuf ;
			bFrom = true ;
			continue ;
		}

		if (!memcmp(cvLine, "    for <", 9))
		{
			i = cvLine + 9 ;
			j = cvBuf ;
			for (; *i != CHAR_MORE ; *j++ = *i++) ;
			*j = 0 ;
			Item.m_To = cvBuf ;
			bTo = true ;
		}

		if (!memcmp(cvLine, "To: ", 4))
		{
			if (!strchr(cvLine + 4, CHAR_COMMA))
			{
				Z = cvLine + 4 ;
				Item.m_All = Z ; 	//	SetAllRecipients(Z) ;
				bAll = true ;
				continue ;
			}

			//	Multiple recipients, build zone
			Z += (cvLine + 4) ;
			for (;;)
			{
				is.getline(cvLine, 512) ;
				if (is.gcount() == 0)
					break ;
				if (!memcmp(cvLine, "    ", 4))
				{
					Z += (cvLine + 4) ;
					continue ;
				}

				//	We are now at end of recipient list
				break ;
			}

			Item.m_All = Z ;
			bAll = true ;
		}

		if (!memcmp(cvLine, "Subject: ", 9))
			{ Item.m_Subject = cvLine + 9 ; bSubject = true ; }

		if (bFrom && bTo && bAll && bSubject)
			break ;
	}

	is.close() ;
	return true ;
}

void	epAccount::UnDeleteAll	(void)
void	epAccount::DeleteFiles	(void)
#endif
