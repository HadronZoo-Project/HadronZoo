//
//	File:	epSMTP.cpp
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

enum	SmtpState
{
	SMTP_EXPECT_HELLO		= 0,	//	Expect a HELO or EHLO
	SMTP_EXPECT_SENDER		= 1,	//	Expect an alien sender or local user's AUTH
	SMTP_EXPECT_AUTHUSER	= 2,	//	User is local so expect UNIX username
	SMTP_EXPECT_AUTHPASS	= 3,	//	Username supplied, expecting password
	SMTP_EXPECT_RECIPIENT	= 4,	//	Expect list of recipients or DATA command
	SMTP_EXPECT_DATA		= 5,	//	Receiving data or the period on a line by itself
	SMTP_EXPECT_QUIT		= 6, 	//	Message delivred, expecting client to quit
} ;

class	SmtpSession	: public hzIpConnInfo
{
public:
	hzMapS	<hzEmaddr,hzString>	mailboxes ;	//	All implicated mailboxes

	hzSet	<hzEmaddr>		localRcpts ;	//	Complied list of all recipients (local and alien) of the email
	hzSet	<hzEmaddr>		alienRcpts ;	//	Complied list of all recipients (local and alien) of the email
	hzArray	<hzEmaddr>		allRcpts ;		//	All recipients
	hzVect	<epAccount*>	m_accounts ;	//	Accounts implicated from the mailboxes

	ofstream		os_item ;				//	Open POP3 temporary file
	ofstream		os_mbox ;				//	POP3 notification file
	hzEmail			theMsg ;				//	The email message
	hdbObject		objEmsg ;				//	The email message database object
	hdbObject		objCorres ;				//	The email message database object
	hzChain			msgBody ;				//	Message body
	hzXDate			now ;					//	Current (connect) time
	hzEmaddr		m_Sender ;				//	Sender address
	hzEmaddr		Recipient ;				//	Recipient address
	hzIpaddr		ipa ;					//	IP address
	hzString		ipstr ;					//	Ipaddr in text form
	hzString		AsciiUsername ;			//	For SMTP auth
	hzString		AsciiPassword ;			//	For SMTP auth
	hzString		m_SenderDomain ;		//	Domain part of sender address
	hzString		fqdn ;					//	Resolved FQDN of sender
	hzString		mailKey ;				//	ISAM key for short form message (formed of local-user-id, correspondent and date-time)
	hzString		m_Realname ;			//	Sender's 'real' name
	hzString		reason ;				//	For rejection, if any
	SmtpState		m_eState ;				//	Current session state

	uint32_t		m_ObjId ;				//	Object ID of short form header record (obtained upon commit to central message repository)
	uint32_t		m_DatumId ;				//	Datum ID of message body (obtained upon commit to central message repository)
	uint32_t		m_CorresId ;			//	Correspondent id (as per central repos of correspondents)
	uint32_t		m_eBlock ;				//	Blocking status
	uint32_t		m_nBlocktime ;			//	Number of seconds to block
	bool			m_bQtine ;				//	The message will be quarantined
	bool			m_bAuth ;				//	Set when authenticated
	bool			m_bSenderLocalDom ;		//	Sender is local
	bool			m_bSenderLocalAddr ;	//	Sender is local
	bool			m_bRecipient ;			//	Flag to indicate at least one recipient has been received
	bool			m_bSenderAuth ;			//  Flag to say sender has authenticated
	bool			m_bSkunk ;				//	Sender is at fault (eg can't resolve, addr nonexistant)
	bool			m_bCallback ;			//	Callback message
	bool			m_bPeriod ;				//	Period on a line by itself
	bool			m_bDone ;				//	Session complete
	bool			m_bAccError ;			//	Email has not been accepted

	char			testPad[1024] ;			//	Test only

	SmtpSession		(void)
	{
		m_bQtine = false ;
		m_bAuth = false ;
		m_bSenderLocalDom = false ;
		m_bSenderLocalAddr = false ;
		m_bRecipient = false ;
		m_bSenderAuth = false ;
		m_bSkunk = false ;
		m_bCallback = false ;
		m_bPeriod = false ;
		m_bDone = false ;
		m_bAccError = false ;
		m_eBlock = HZ_IPSTATUS_NULL ;
		m_nBlocktime = 0 ;
		m_ObjId = m_DatumId = m_CorresId = 0 ;
	}

	~SmtpSession	(void)
	{
		if (os_item.is_open())
			os_item.close() ;
	}
} ;

/*
**	Variables
*/

extern	hzLogger	slog ;						//	System logs

extern	pthread_mutex_t		g_LockRS ;			//	Lock for relay task schedule
extern	hzIpServer*			g_pTheServer ;		//	The server instance for HTTP

//	Working (temp) maps and sets
extern	hzSet<hzEmaddr>		g_setBanned ;		//	Globally banned senders (users may also ban senders)
extern	hzVect<epAccount*>	g_vecAccount ;		//	All epistula ordinary users (by uid)

//	Ports and limits (set in main)
extern	uint32_t	g_nPortSMTP ;				//	Standard SMTP port (Normally port 25)
extern	uint32_t	g_nPortSMTPS ;				//	Secure SMTP port (Local users only, Normally port 587)
extern	uint32_t	g_nPortPOP3 ;				//	Standard POP3 port (Normally port 110, Epistula defaul 2110, can be any value)
extern	uint32_t	g_nPortPOP3S ;				//	Secure POP3 port (Normally port 995, Epistula uses 2995, can be any value)
extern	uint32_t	g_nMaxConnections ;			//	Number of simultaneous SMTP and POP3 connections
extern	uint32_t	g_nMaxHTTPConnections ;		//	Number of simultaneous HTTP connections

extern	uint32_t	g_nMaxMsgSize ;				//	Max email message size (2 million bytes)
extern	uint32_t	g_nSessIdAuth ;				//	Session ID for AUTH connections
extern	uint32_t	g_nSessIdPop3 ;				//	Session ID for POP3 connections
extern	bool		g_bShutdown ;				//	When set all thread loops must end

//	Fixed messages
static	const char*	msgSkunk = "550 Relaying denied. An email must originate from a registered server associated with the stated sender's domain.\r\n" ;
//	static	const char*	err_int_nosetuser = "-ERR Internal fault, could not set username\r\n" ;
//	static	const char*	err_int_cmdnosupp = "-ERR Internal fault, command not supported\r\n" ;
//	static	const char*	pop3hello = "+OK Epistula POP3 Server Ready\r\n" ;

static	const char*	_smtpstates	[]	=
{
	"SMTP_EXPECT_HELLO",
	"SMTP_EXPECT_SENDER",
	"SMTP_EXPECT_AUTHUSER",
	"SMTP_EXPECT_AUTHPASS",
	"SMTP_EXPECT_RECIPIENT",
	"SMTP_EXPECT_DATA",
	"SMTP_EXPECT_QUIT",
} ;

extern	hdsApp*			theApp ;		//  Dissemino HTML interface
extern	Epistula*		theEP ;					//	The singleton Epistula Instance

extern	hzString	g_str_Inbox ;
extern	hzString	g_str_Misc ;

extern	hzString	_dsmScript_tog ;
extern	hzString	_dsmScript_loadArticle ;
extern	hzString	_dsmScript_navtree ;

/*
**	Epistula SMTP Functions
*/

hzEcode	ExtractEmailAddr	(hzEmaddr& Address, chIter& zi, hzChain& error)
{
	//	Extracts an email address from input line.
	//
	//	Arguments:	1)	Enail address. This is populated if the operation is successful.
	//				2)	The input line containing the email address.
	//				3)	A chain to add error messages to.

	hzChain		W ;			//	For aggregation (of the email address)
	hzString	S ;			//	String version

	//	Ignore everything up to the first '<'
	for (; !zi.eof() && *zi != CHAR_LESS ; zi++) ;

	if (*zi != CHAR_LESS)
	{
		error.Printf("Email address does not have an opening '<'\n") ;
		return E_FORMAT ;
	}

	//	Set the pointer and find the '>'
	for (zi++ ; !zi.eof() && *zi >= CHAR_SPACE && *zi != CHAR_MORE ; zi++)
	{
		if (*zi >= 'A' && *zi <= 'Z')
			W.AddByte(_tolower(*zi)) ;
		else
			W.AddByte(*zi) ;
	}
	if (*zi != CHAR_MORE)
	{
		error.Printf("Email address does not have a closing '>'\n") ;
		return E_FORMAT ;
	}

	S = W ;

	if (!IsEmaddr(*S))
	{
		error.Printf("Email address <%s> is not legal\n", *S) ;
		return E_SYNTAX ;
	}

	Address = *S ;
	if (!Address)
	{
		error.Printf("Could not set email address [%s]\n", *S) ;
		return E_SYNTAX ;
	}

	error.Printf("Got email address of <%s>\n", *S) ;
	return E_OK ;
}

bool	_skunk	(const char* fqdn, const char* ipaddr)
{
	//	If the FQDN contains the IP address of the form A-B-C-D rather than A.B.C.D, then the sender is a skunk

	const char*	i = ipaddr ;

	char*	j ;
	char	a[4] ;
	char	b[4] ;
	char	c[4] ;
	char	d[4] ;

	if (!fqdn)		return false ;
	if (!ipaddr)	return false ;

	for (j = a ; *i && *i != '.' ; *j++ = *i++) ;	*j = 0 ; i++ ;
	for (j = b ; *i && *i != '.' ; *j++ = *i++) ;	*j = 0 ; i++ ;
	for (j = c ; *i && *i != '.' ; *j++ = *i++) ;	*j = 0 ; i++ ;
	for (j = d ; *i && *i != '.' ; *j++ = *i++) ;	*j = 0 ;

	//return false ;

	if (strstr(fqdn, a) && strstr(fqdn, b) && strstr(fqdn, c) && strstr(fqdn, d))
		return true ;
	return false ;
}

hzTcpCode	HelloSMTP	(hzIpConnex* pCC)
{
	hzChain			hello ;				//	Server hello message
	SmtpSession*	pInfo ;				//	SMTP Session Info
	char			msgBuf[64] ;		//	Buffer

	pInfo = new SmtpSession() ;
	pInfo->m_eState = SMTP_EXPECT_HELLO ;
	pCC->SetInfo(pInfo) ;

	pInfo->now.SysDateTime() ;
	pInfo->ipa = pCC->ClientIP() ;
	pInfo->ipstr = pInfo->ipa.Str() ;

	hello.Printf("220 %s ESMTP Epistula 1.0.0; %s\r\n", *_hzGlobal_Hostname, *pInfo->now.Str(FMT_DT_INET)) ;
	pCC->SendData(hello) ;
	pCC->m_Track.Printf("\nNEW SMTP Client %d Connects from %s on sock %d\n", pCC->EventNo(), *pInfo->ipstr, pCC->CliSocket()) ;
}

hzTcpCode	FinishSMTP	(hzIpConnex* pCC)
{
	//	This will ensure a POP3 file is saved

	SmtpSession*	pInfo ;				//	SMTP Session Info

	pInfo = (SmtpSession*) pCC->GetInfo() ;

	if (pInfo->m_eBlock != HZ_IPSTATUS_NULL)
	{
		if (GetStatusIP(pInfo->ipa) & HZ_IPSTATUS_WHITE)
			pCC->m_Track.Printf("BLOCK On Whitelist IP address %s\n", *pInfo->ipstr) ;
		else
			pCC->m_Track.Printf("BLOCK On IP address %s\n", *pInfo->ipstr) ;

		SetStatusIP(pInfo->ipa, (hzIpStatus) pInfo->m_eBlock, pInfo->m_nBlocktime) ;
	}

	slog.Out(pCC->m_Track) ;
}

/*
**	SMTP Session Support Functions. These exist because ProcAlienSMTP and ProcLocalSMTP are very similar state-machine functions. The support functions handle the steps needed for
**	each state that would be common to both.
*/

hzTcpCode	_smtpExpectHello	(hzChain& Input, hzIpConnex* pCC)
{
	//	Expect HELO or EHLO command (we have already sent the SMTP server hello)

	_hzfunc(__func__) ;

	hzChain			W ;						//	Word/phrase buffer (for extracting data from Input chain)
	hzChain			R ;						//	Response to client
	chIter			zi ;					//	Input chain iterator
	SmtpSession*	pInfo ;					//	SMTP Session info

	pInfo = (SmtpSession*) pCC->GetInfo() ;
	zi = Input ;

	if (pInfo->m_eState != SMTP_EXPECT_HELLO)
		Fatal("%s: Wrong state\n", __func__) ;

	if (zi.Equiv("HELO "))
	{
		//	Get Realname from hello command
		for (zi += 5 ; !zi.eof() && *zi >= CHAR_SPACE ; zi++)
			W.AddByte(*zi) ;
		pInfo->m_Realname = W ;
		W.Clear() ;

		//	Reply to HELO
		pInfo->m_eState = SMTP_EXPECT_SENDER ;
		R.Printf("250 %s Hello %s [%s], pleased to meet you\r\n", *_hzGlobal_Hostname, *pInfo->m_Realname, *pInfo->ipstr) ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	if (zi.Equiv("EHLO "))
	{
		//	Get Realname from hello command
		for (zi += 5 ; !zi.eof() && *zi >= CHAR_SPACE ; zi++)
			W.AddByte(*zi) ;
		pInfo->m_Realname = W ;
		W.Clear() ;

		//	Reply to EHLO
		pInfo->m_eState = SMTP_EXPECT_SENDER ;
		R.Printf("250-%s Hello %s [%s]\r\n250-ENHANCEDSTATUSCODES\r\n250-SIZE 4000000\r\n250-DSN\r\n250-HELP\r\n250 AUTH LOGIN\r\n",
			*_hzGlobal_Hostname, *pInfo->m_Realname, *pInfo->ipstr) ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	//	At this point, niether the expected HELO or EHLO have been found

	R.Printf("503 Expected a HELO or EHLO command\r\n") ;
	pCC->m_Track << R ;

	pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
}

hzTcpCode	_smtpExpectAuthuser	(hzChain& Input, hzIpConnex* pCC)
{
	//	Expect Username and ask for password

	_hzfunc(__func__) ;

	SmtpSession*	pInfo ;					//	SMTP Session info
	hzChain			oup ;					//	Decoded data
	hzChain			R ;						//	Response to client

	pInfo = (SmtpSession*) pCC->GetInfo() ;

	if (pInfo->m_eState != SMTP_EXPECT_AUTHUSER)
		Fatal("%s: Wrong state\n", __func__) ;

	//	Expect Auth username of Sender.

	//	Recv encrypted username
	oup.Clear() ;
	Base64Decode(oup, Input) ;
	pInfo->AsciiUsername = oup ;
	pCC->m_Track.Printf("Got username of %s - ", *pInfo->AsciiUsername) ;

	//	Stated username must be that of one of the users allowed to originate messages from the stated sender address

	//	Send 'password' encrypted in base 64
	pInfo->m_eState = SMTP_EXPECT_AUTHPASS ;
	R.Printf("334 UGFzc3dvcmQ6\r\n") ;
	pCC->SendData(R) ;
		Input.Clear() ;

	return TCP_KEEPALIVE ;
}

hzTcpCode	_smtpExpectAuthpass	(hzChain& Input, hzIpConnex* pCC)
{
	//	Expect password, then authenticate against subscriber

	_hzfunc(__func__) ;

	SmtpSession*	pInfo ;					//	SMTP Session info
	hzChain			oup ;					//	Decoded data
	hzChain			R ;						//	Response to client
	hzAtom			atom ;					//	For authentication
	uint32_t		objId ;					//	For user authentication

	pInfo = (SmtpSession*) pCC->GetInfo() ;

	if (pInfo->m_eState != SMTP_EXPECT_AUTHPASS)
		Fatal("%s: Wrong state\n", __func__) ;

	//	Expect Auth password of Sender.

	//	Recv encrypted password
	oup.Clear() ;
	Base64Decode(oup, Input) ;
	pInfo->AsciiPassword = oup ;
	pCC->m_Track.Printf("Got password of %s\n", *pInfo->AsciiPassword) ;

	if (pInfo->m_bQtine)
	{
		//	Fraud message. Don't bother authenticating
		pInfo->m_eState = SMTP_EXPECT_SENDER ;
		R.Printf("235 Go Ahead\r\n") ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

 	//	Authenticate against subscriber. As the sequence for a local user is to authenticate first, it is not yet known which address they intend to use to originate the email
 	//	message. This check will have to be performed when the sender address is sent in which is the next stage.

	theApp->m_Allusers->Identify(objId, SUBSCRIBER_USERNAME, pInfo->AsciiUsername) ;

	if (objId)
	{
		theApp->m_Allusers->Fetchval(atom, SUBSCRIBER_PASSWORD, objId) ;

		if (atom.Str() == pInfo->AsciiPassword)
		{
			pInfo->m_bAuth = true ;
			pInfo->m_bSenderAuth = true ;
			R.Printf("235 Go Ahead\r\n") ;
			pInfo->m_eState = SMTP_EXPECT_SENDER ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}
	}

	//	Authentication failed
	R.Printf("535 Auth Failure\r\n") ;
	pCC->m_Track << R ;
	pInfo->m_eBlock |= HZ_IPSTATUS_BLACK_SMTP ;
	pInfo->m_eState = SMTP_EXPECT_QUIT ;

	pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
}

hzTcpCode	_smtpExpectRcpt		(hzChain& Input, hzIpConnex* pCC)
{
	//	Handle the SMTP_EXPECT_RECIPIENT state.

	_hzfunc(__func__) ;

	hzChain			R ;						//	Response to client
	hzChain			Z ;						//	For helping to write mail que file
	chIter			zi ;					//	Input chain iterator
	SmtpSession*	pInfo ;					//	SMTP Session info
	epAccount*		pAcc ;					//	Affected accounts
	hzEmaddr		testAddr ;				//	Temporary email address
	hzEmaddr		testRcpt ;				//	Recipient
	int32_t			fstMbox ;				//	First mailbox found
	int32_t			lstMbox ;				//	Last mailbox found
	int32_t			fstFwrd ;				//	First forwarding address found
	int32_t			lstFwrd ;				//	Last forwarding address found
	int32_t			nIndex ;				//	General iterator
	int32_t			nTest ;					//	General iterator

	pInfo = (SmtpSession*) pCC->GetInfo() ;
	zi = Input ;

	if (pInfo->m_eState != SMTP_EXPECT_RECIPIENT)
		Fatal("%s: Wrong state\n", __func__) ;

	//	Expect list of recipients or the DATA command.
	if (zi.Equiv("RCPT TO"))
	{
		if (ExtractEmailAddr(pInfo->Recipient, zi, pCC->m_Track) != E_OK)
		{
			R.Printf("550 Malformed recipient address\r\n") ;
			pCC->m_Track << R ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	Check to see if recipient is local
		if (!theEP->m_Locaddr2Mbox.Exists(pInfo->Recipient))
		{
			if (theEP->m_LocalDomains.Exists(pInfo->Recipient.GetDomain()))
			{
				//	Domain hosted but address not listed (Q: will this happen?)
				R.Printf("550 5.1.1 No such mailbox\r\n") ;
				pCC->m_Track << R ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			//	Recipient has an alien domain so unless sender is local, deny relay
			if (!pInfo->m_bSenderLocalAddr)
			{
				R.Printf("550 Relaying Denied\r\n") ;
				pCC->m_Track << R ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			//	Recipient is alien and sender is local - relay will happen
			pCC->m_Track.Printf("recipient is alien\n") ;

			if (!pInfo->alienRcpts.Exists(pInfo->Recipient))
			{
				//	if (!doms.Exists(pInfo->Recipient.GetDomain()))
				//		doms.Insert(pInfo->Recipient.GetDomain()) ;

				pInfo->alienRcpts.Insert(pInfo->Recipient) ;
				pInfo->allRcpts.Add(pInfo->Recipient) ;
			}

			R.Printf("250 2.1.5 <%s>... Recipient ok\r\n", *pInfo->Recipient) ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//if (g_LocAddrs.Exists(pInfo->Recipient))
		if (theEP->m_Locaddr2Mbox.Exists(pInfo->Recipient))
		{
			//	Email address is local and we have the username (if set) and the list of mailboxes (if any) and the set of forwards (if any).

			/*
			//	Check if local recipient has blocked the sender (if so then deny mailbox exists)
			if (IsBlocked(Recipient, mailunit.GetSender()))
			{
				//pCC->m_Track.Printf("Recipient blocked\n") ;
				pCC->m_Track.Printf("Recipient blocked\n") ;
				sprintf(msgBuf, "550 No such recipient\r\n") ;
				continue ;
			}
			*/

			//	If the address has no delivery points it effectively does not exist
			fstMbox = theEP->m_Locaddr2Mbox.First(pInfo->Recipient) ;
			fstFwrd = theEP->m_Locaddr2Frwd.First(pInfo->Recipient) ;

			if (fstMbox == -1 && fstFwrd == -1)
			{
				R.Printf("550 No such mailbox/recipient\r\n") ;
				pCC->m_Track << R ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			//	Compile list of mailboxes
			if (fstMbox >= 0)
			{
				lstMbox = theEP->m_Locaddr2Mbox.Last(pInfo->Recipient) ;

				for (nIndex = fstMbox ; nIndex <= lstMbox ; nIndex++)
				{
					pCC->m_Track.Printf("\tcase 1: Adding account/mailbox %s\n", *theEP->m_Locaddr2Mbox[pInfo->Recipient]) ;
					pInfo->mailboxes.Insert(pInfo->Recipient, theEP->m_Locaddr2Mbox[pInfo->Recipient]) ;
				}
			}

			if (fstFwrd >= 0)
			{
				lstFwrd = theEP->m_Locaddr2Frwd.Last(pInfo->Recipient) ;

				for (nIndex = fstFwrd ; nIndex <= lstFwrd ; nIndex++)
				{
					testAddr = theEP->m_Locaddr2Frwd.GetObj(nIndex) ;
					pCC->m_Track.Printf("\tAdding forward %s\n", *testAddr) ;

					//	if (!forwarding.Exists(addr))
					//		forwarding.Insert(addr) ;

					if (pInfo->localRcpts.Exists(testAddr))
						continue ;
					if (pInfo->alienRcpts.Exists(testAddr))
						continue ;

					testRcpt = theEP->m_LocAddrs[testAddr] ;

					if (!testRcpt)
					{
						if (testAddr != pInfo->m_Sender && !pInfo->alienRcpts.Exists(testAddr))
							pInfo->alienRcpts.Insert(testAddr) ;
					}
					else
					{
						fstMbox = theEP->m_Locaddr2Mbox.First(testRcpt) ;

						if (fstMbox >= 0)
						{
							lstMbox = theEP->m_Locaddr2Mbox.Last(testRcpt) ;

							for (nTest = fstMbox ; nTest <= lstMbox ; nTest)
							{
								pCC->m_Track.Printf("\tcase 2: Adding account/mailbox %s\n", *theEP->m_Locaddr2Mbox[testRcpt]) ;
								pAcc = theEP->m_Accounts[theEP->m_Locaddr2Mbox[testRcpt]] ;
								pInfo->m_accounts.Add(pAcc) ;
								pInfo->mailboxes.Insert(pInfo->Recipient, theEP->m_Locaddr2Mbox[testRcpt]) ;
							}
						}
					}
				}
			}

			pInfo->m_bRecipient = true ;
			if (!pInfo->localRcpts.Exists(pInfo->Recipient))
			{
				pInfo->allRcpts.Add(pInfo->Recipient) ;
				pInfo->localRcpts.Insert(pInfo->Recipient) ;
			}

			R.Printf("250 2.1.5 <%s>... Recipient ok\r\n", *pInfo->Recipient) ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		pInfo->m_eState = SMTP_EXPECT_QUIT ;
		pCC->m_Track.Printf("Unexpected return from CDB\n") ;
		R.Printf("421 Internal fault\r\n") ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	if (zi.Equiv("DATA"))
	{
		//	The list of recipients should now be complete and contain at least one valid recipient. If this is not the case we send an error.

		if (!pInfo->localRcpts.Count() && !pInfo->alienRcpts.Count())
		{
			pInfo->m_eState = SMTP_EXPECT_QUIT ;
			R.Printf("503 No valid recipients supplied\r\n") ;
			pCC->m_Track << R ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	There are valid recipients so prepare to receive data
		pInfo->m_eState = SMTP_EXPECT_DATA ;

		//	Start the message body
		Z.Printf("Return-Path: <%s>\r\n", *pInfo->m_Sender) ;
		Z.Printf("Received: from %s (%s [%s])\r\n", *pInfo->m_Realname, *pInfo->fqdn, *pInfo->ipstr) ;
		Z.Printf("    by %s (Epistula) at %s %s %02d %02d:%02d:%02d %04d\r\n",
			*_hzGlobal_Hostname,
			hz_daynames_abrv[pInfo->now.Dow()], hz_monthnames_abrv[pInfo->now.Month() - 1],
			pInfo->now.Day(), pInfo->now.Hour(), pInfo->now.Min(), pInfo->now.Sec(), pInfo->now.Year()) ;

		pInfo->msgBody << Z ;
		Z.Clear() ;

		//	Ask for mail contents
		R.Printf("354 Enter mail, end with . on a line by itself\r\n") ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	//	Entity is niether a recipient or a DATA command
	if (!pInfo->m_bRecipient)
		R.Printf("503 Expected a recipient\r\n") ;
	else
		R.Printf("503 Expected a recipient or the DATA command\r\n") ;
	pCC->m_Track << R ;

	pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
}

hzTcpCode	_smtpExpectData		(hzChain& Input, hzIpConnex* pCC)
{
	//	Receiving data or the period on a line by itself. If the latter the message is complete and so needs to be committed. This function will return ...

	_hzfunc(__func__) ;

	hzChain			R ;						//	Response to client
	chIter			zi ;					//	Input chain iterator
	SmtpSession*	pInfo ;					//	SMTP Session info
	hzPair			pair ;					//	Header/parameter pair
	hzString		accname ;				//	Name of user account
	int32_t			n ;						//	Loop counter
	hzEcode			rc ;					//	Return code from DNS lookup functions

	pInfo = (SmtpSession*) pCC->GetInfo() ;
	zi = Input ;

	if (pInfo->m_eState != SMTP_EXPECT_DATA)
		Fatal("%s: Wrong state\n", __func__) ;

	pInfo->m_bPeriod = false ;

	//	Collect data
	for (; !zi.eof() ; )
	{
		if (zi == "\r\n.\r\n")
		{
			pInfo->msgBody << "\r\n.\r\n" ;
			zi += 5 ;
			if (zi.eof())
				{ pInfo->m_bPeriod = true ; break ; }
			continue ;
		}

		pInfo->msgBody.AddByte(*zi) ;
		zi++ ;
	}

	//	Ensure max size not exceeded
	if (pInfo->msgBody.Size() > 4000000)
	{
		R.Printf("552 Message exceeds limit. Closing connection\r\n") ;
		pCC->m_Track << R ;
		pInfo->m_bAccError = true ;
		pInfo->m_eState = SMTP_EXPECT_QUIT ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	//	If data incomplete, return until more comes
	if (!pInfo->m_bPeriod)
		return TCP_KEEPALIVE ;

	//	Message complete so apply checks
	pInfo->m_eState = SMTP_EXPECT_QUIT ;

	if (!pInfo->m_bCallback)
	{
		//	Load the message instance. This should have a message id in the header
		rc = pInfo->theMsg.Import(pInfo->msgBody) ;
		if (rc != E_OK)
		{
			for (n = 0 ; n < pInfo->theMsg.m_Hdrs.Count() ; n++)
			{
				pair = pInfo->theMsg.m_Hdrs[n] ;
				pCC->m_Track.Printf("Hdr %d: %s:\t%s\n", n, *pair.name, *pair.value) ;
			}

			pCC->m_Track << pInfo->theMsg.m_Err ;
			R.Printf("421 Internal Fault\r\n") ;
			pCC->m_Track.Printf("Mail item MALFORMED so rejected\n") ;
			pInfo->m_eState = SMTP_EXPECT_QUIT ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}
		pCC->m_Track << pInfo->theMsg.m_Err ;
	}
}

hzEcode	_smtpCheckSPAM	(hzIpConnex* pCC)
{
	//	Check for SPAM

	_hzfunc(__func__) ;

	chIter			zi ;					//	Input chain iterator
	SmtpSession*	pInfo ;					//	SMTP Session info
	bool			bSpam ;					//	Spam Content

	pInfo = (SmtpSession*) pCC->GetInfo() ;

	//	Test content here
	bSpam = false ;
	for (zi = pInfo->msgBody ; !zi.eof() ; zi++)
	{
		if (zi == "Elidor99")
			{ bSpam = true ; break ; }
	}

	if (!bSpam)
		pCC->m_Track.Printf("Mail item passed SPAM test\n") ;
	else
		pCC->m_Track.Printf("Mail item failed SPAM test\n") ;

	return bSpam ? E_BADVALUE : E_OK ;
}

hzEcode	_smtpCoreCommit	(hzIpConnex* pCC)
{
 	//	Commit the message to the database

	_hzfunc(__func__) ;

	SmtpSession*	pInfo ;			//	SMTP Session info
	hzEcode			rc = E_OK ;		//	Return code

	pInfo = (SmtpSession*) pCC->GetInfo() ;

	if (!pInfo->m_bQtine)
	{
		rc = theEP->UpdateShortForm	(	pInfo->m_ObjId,
										pInfo->m_DatumId,
										pInfo->allRcpts,
										pInfo->theMsg.m_Id,
										pInfo->now,
										pInfo->theMsg.m_AddrSender,
										pInfo->m_Realname,
										pInfo->theMsg.m_Subject,
										pInfo->msgBody,
										pCC->m_Track	) ;
		if (rc != E_OK)
			pInfo->m_bAccError = true ;
	}

	return rc ;
}

hzEcode	_smtpPOP3Commit	(hzIpConnex* pCC)
{
	//	COmmit the message to a POP3 file

	_hzfunc(__func__) ;

	hzChain			Z ;				//	For helping to write mail que file
	SmtpSession*	pInfo ;			//	SMTP Session info
	epAccount*		pAcc ;			//	Affected accounts
	epFolder*		pFolder ;		//	Target automatic folder
	hzFxDomain		tmpDom ;		//	Domain of sender
	hzFxEmaddr		tmpFro ;		//	Address of sender
	hzEmaddr		testAddr ;		//	Temporary email address
	hzString		accname ;		//	Name of user account
	hzString		tmpStr ;		//	Temporary string
	int32_t			nIndex ;		//	General iterator

	pInfo = (SmtpSession*) pCC->GetInfo() ;
	Z.Clear() ;

	if (pInfo->m_bQtine)
	{
		sprintf(pInfo->testPad, "%s/%s.pop3", *theEP->m_Quarantine, *pInfo->now.Str()) ;
		pCC->m_Track.Printf("Quarantine POP3 file [%s]\n", pInfo->testPad) ;
		pInfo->os_item.open(pInfo->testPad, ios::app) ;
		pInfo->os_item << pInfo->msgBody ;
		pInfo->os_item.close() ;
	}
	else
	{
		sprintf(pInfo->testPad, "%s/%010d.pop3", *theEP->m_EpisData, pInfo->m_DatumId) ;
		pCC->m_Track.Printf("Starting POP3 file [%s] for (%s)\n", pInfo->testPad, *accname) ;
		pInfo->os_item.open(pInfo->testPad, ios::app) ;
		pInfo->os_item << pInfo->msgBody ;
		pInfo->os_item.close() ;

		//	UPDATE the Central Repository of Correspondents. Check if the sender is present, if not insert. Then the same for each recipient.
		for (nIndex = 0 ; nIndex < pInfo->allRcpts.Count() ; nIndex++)
			theEP->UpdateCorres(pInfo->m_CorresId, pInfo->allRcpts[nIndex], _hzGlobal_nullString, pCC->m_Track) ;
		theEP->UpdateCorres(pInfo->m_CorresId, pInfo->theMsg.m_AddrSender, pInfo->m_Realname, pCC->m_Track) ;

		//	Deliver to all mailboxes
		for (nIndex = 0 ; nIndex < pInfo->mailboxes.Count() ; nIndex++)
		{
			testAddr = pInfo->mailboxes.GetKey(nIndex) ;
			accname = pInfo->mailboxes.GetObj(nIndex) ;

			pAcc = theEP->m_Accounts[accname] ;

			/* TEMP DISABLE
			pAcc->m_CorresDomains.Insert(tmpDom) ;
			pAcc->m_Correspondents.Insert(tmpDom, tmpFro) ;

			if (!pAcc->m_FoldersByID.Exists(pInfo->m_CorresId))
			{
				pFolder = new epFolder() ;
				pFolder->m_CorresId = pInfo->m_CorresId ;
				pFolder->m_Name = pInfo->theMsg.m_RealFrom ;

				pAcc->m_FoldersByID.Insert(pInfo->m_CorresId, pFolder) ;
				if (pFolder->m_Name)
					pAcc->m_FoldersByNAME.Insert(pFolder->m_Name, pFolder) ;
			}
			*/

			//	Initialize the database object for the message repository
			pAcc->m_LockPOP3.Lock() ;

				tmpStr = theEP->m_MailBox + "/" + accname + "/mbox.pop3" ;
				pInfo->os_mbox.open(*tmpStr, ios::app) ;
				if (pInfo->os_mbox.fail())
					pCC->m_Track.Printf("AppendPOP3File failed to add item %s\n", *pInfo->theMsg.m_Id) ;
				else
				{
					sprintf(pInfo->testPad, "ep%010u,%010u\n", pInfo->m_DatumId, pInfo->msgBody.Size()) ;
					pInfo->os_mbox << pInfo->testPad ;
					pInfo->os_mbox.close() ;
				}
				pInfo->os_mbox.clear() ;

			pAcc->m_LockPOP3.Unlock() ;
		}
	}

	return E_OK ;
}

hzEcode	_smtpRelayCommit	(hzIpConnex* pCC)
{
	//	If there are any remote recipients, place item in mail que

	_hzfunc(__func__) ;

	ofstream		os ;			//	For writing to mailboxes and mail que files
	hzChain			W ;				//	Word/phrase buffer (for extracting data from Input chain)
	hzChain			Z ;				//	For helping to write mail que file
	SmtpSession*	pInfo ;			//	SMTP Session info
	hzString		outFname ;		//	For writing email to mailque
	int32_t			nIndex ;		//	General iterator
	hzEcode			rc = E_OK ;		//	Return code

	pInfo = (SmtpSession*) pCC->GetInfo() ;

	if (!pInfo->alienRcpts.Count())
		return rc ;
	if (pInfo->m_bQtine)
		return rc ;

	W.Clear() ;
	W.Printf("%s/%s.outg", *theEP->m_MailQue, *pInfo->theMsg.m_Id) ;
	outFname = W ;
	W.Clear() ;

	os.open(*outFname) ;
	if (os.fail())
	{
		pCC->m_Track.Printf("Could not open file in the MAILQUE %s\n", *outFname) ;

		os.clear() ;
		os.open(*outFname) ;
		if (os.fail())
		{
			//pCC->m_Track.Printf("Could not open file in the MAILQUE - given up\n") ;
			pCC->m_Track.Printf("Could not open file in the MAILQUE %s - given up\n", *outFname) ;
			pInfo->m_bAccError = true ;
		}
	}

	if (os.good())
	{
		//	Record delivery info
		Z.Clear() ;
		Z.Printf("from: %s\n", *pInfo->m_Sender) ;

		if (pInfo->m_bSenderAuth && pInfo->AsciiUsername)
			Z.Printf("username: %s\n", *pInfo->AsciiUsername) ;

		Z.Printf("realname: %s\n", pInfo->m_Realname ? *pInfo->m_Realname : *pInfo->m_Sender) ;
		Z << "ip: " << pInfo->ipstr << "\n" ;
		Z.Printf("resolved: %s\n", pInfo->fqdn ? *pInfo->fqdn : "unknown") ;

		Z << "id: " << pInfo->theMsg.m_Id << "\n" ;

		for (nIndex = 0 ; nIndex < pInfo->alienRcpts.Count() ; nIndex++)
			Z << "rcpt: " << pInfo->alienRcpts.GetObj(nIndex) << "\n" ;
		Z.AddByte(CHAR_NL) ;
		os << Z ;
	}

	if (os.fail())
	{
		pCC->m_Track.Printf("Could not write outgoing header to file %s\n", *outFname) ;
		pInfo->m_bAccError = true ;
		rc = E_WRITEFAIL ;
	}

	if (os.good())
	{
		DosifyChain(pInfo->msgBody) ;
		os << pInfo->msgBody ;
		os << "\r\n" ;
		os.close() ;
	}
	os.clear() ;
	return rc ;
}

hzTcpCode	ProcAlienSMTP		(hzChain& Input, hzIpConnex* pCC)
{
	//	Process an SMTP client connection on the standard SMTP port (25). Epistula only accepts incoming messages from alien senders, on this port. To originate an outgoing message
	//	via SMTP, local users are required to use the submission port (587). Client connections on port 587 are handled by ProcLocalSMTP().
	//
	//	In all SMTP sessions, clients will state a sender address and one or more recipient addresses. Where the sender address has an alien domain, the service proceeds normally. Where the
	//	sender address is of a local domain, the message will always fail. However the proceedure will differ, depending on the status of the client IP address and whether or not a
	//	valid username and password are supplied.
	//
	//	Any delinquent activity must be countered but of particular interest are incidents that originate or appear to originate, from an IP address associated with past or present
	//	successful authentication. Such incidents show that the computer or network of computers used by legitimate users of the service, have been compromized. To detect incidents
	//	of this nature, Epistula whitelists for 24 hours, any IP addresses used in a successful authentication to any of the services it offers.
	//
	//	In this alien SMTP service, the SMTP authentication proceeds whenever the stated sender address is of a local domain, as would be expected. This authentication always fails
	//	regardless of the supplied username and password. If the IP address is NOT whitelisted, the connection is terminated and the IP address is blocked. The block is limited to
	//	port 25 if the supplied username and password were valid, otherwise it is comprehensive and the client will not be able to establish another connection.
	//
	//	If the client IP address is whitelisted the authorization will always appear to succeed. The message relay is allowed to proceed but the message is permanently quarantined.
	//	Once the message has been recieved it is rejected, the connection is terminated and the IP address is blocked for port 25.
	//
	//	Arguments:	1) Input	Client input
	//				2) pCC		Pointer to client connection
	//
	//	Returns:	TCP_TERMINATE	If connection is to be terminated.
	//				TCP_KEEPALIVE	If connection is kept open (session is ongoing).

	_hzfunc(__func__) ;

	hzChain			R ;						//	Response to client
	chIter			zi ;					//	Input chain iterator
	SmtpSession*	pInfo ;					//	SMTP Session info
	hzRecep16		recepA ;				//	Used in reporting of IP addresses
	hzString		accname ;				//	Name of user account
	hzFxDomain		tmpDom ;				//	Domain of sender
	hzFxEmaddr		tmpFro ;				//	Address of sender
	uint32_t		domainNo ;				//	Domain number as given by _hzGlobal_FST_Domain
	uint32_t		emaddrNo ;				//	Emaddr number as given by _hzGlobal_FST_Emaddr
	int32_t			nTest ;					//	General iterator
	int32_t			n ;						//	Loop counter
	hzEcode			rc ;					//	Return code from DNS lookup functions

	/*
	**	Initialize session
	*/

	pInfo = (SmtpSession*) pCC->GetInfo() ;
	g_nSessIdAuth++ ;

	/*
	**	Handle client requests
	*/

	if (Input.Size() < 100)
	{
		for (zi = Input, nTest = 0 ; !zi.eof() && nTest < 100 && *zi >= CHAR_SPACE ; zi++, nTest++)
			pInfo->testPad[nTest] = *zi ;
		pInfo->testPad[nTest] = 0 ;
		pCC->m_Track.Printf("%s. SMTP client %d [%s]\n", *_fn, pCC->EventNo(), pInfo->testPad) ;
	}
	else
		pCC->m_Track.Printf("%s. SMTP client %d (%d bytes)\n", *_fn, pCC->EventNo(), Input.Size()) ;
	zi = Input ;

	/*
	**	Handle message
	*/

	if (zi.Equiv("QUIT\r\n"))
	{
		R.Printf("221 Epistula SMTP session terminated normally\r\n") ;
		pCC->SendData(R) ;
		Input.Clear() ;
		return TCP_TERMINATE ;
	}

	if (zi.Equiv("RSET "))
	{
		R += "+OK\r\n" ; pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == SMTP_EXPECT_HELLO)
		return _smtpExpectHello(Input, pCC) ;

	if (pInfo->m_eState == SMTP_EXPECT_SENDER)
	{
		//	Establish Sender Domain and Address

		if (zi.Equiv("AUTH"))
		{
			//	This should NOT happen with an alien connection - so we pretend all is well by setting the state to SMTP_EXPECT_AUTHUSER, but set the quarantine flag.
			pCC->m_Track.Printf("Unexpected AUTH\n") ;
			pInfo->m_eState = SMTP_EXPECT_AUTHUSER ;
			pInfo->m_bQtine = true ;
			R.Printf("334 VXNlcm5hbWU6\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (!zi.Equiv("MAIL FROM"))
		{
			R.Printf("503 Expected a sender address\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	Extract sender from message of the form "realname <email@address>"
		if (ExtractEmailAddr(pInfo->m_Sender, zi, pCC->m_Track) != E_OK)
		{
			pInfo->m_eState = SMTP_EXPECT_QUIT ;
			R.Printf("501 Sender email address could not be deciphered.\r\n") ;
			pCC->m_Track << R ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}
		pInfo->m_SenderDomain = pInfo->m_Sender.GetDomain() ;
		pCC->m_Track.Printf("SETTING Sender %s domain %s\r\n", *pInfo->m_Sender, *pInfo->m_SenderDomain) ;

		//	Deal with special case of callback (blank sender)
		if (!pInfo->m_Sender)
		{
			pInfo->m_bCallback = true ;
			pInfo->m_eState = SMTP_EXPECT_RECIPIENT ;
			R.Printf("250 2.1.0 <>... Sender ok (callback)\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	Check if sender is local or alien
		if (theEP->m_LocalDomains.Exists(pInfo->m_SenderDomain))
		{
			pInfo->m_bSenderLocalDom = true ;
			pInfo->m_bQtine = true ;
			if (theEP->m_Locaddr2Orig.Exists(pInfo->m_Sender))
				pInfo->m_bSenderLocalAddr = true ;
		}
		else
		{
			//	The sender is alien - this allowed but is the domain banned?
			if (g_setBanned.Exists(pInfo->m_SenderDomain))
			{
				pInfo->m_eState = SMTP_EXPECT_QUIT ;
				R.Printf("550 <%s>... Sender not accepted. terminated\r\n", *pInfo->m_Sender) ;
				pCC->m_Track << R ;
				pCC->m_Track << "Sender is banned\n" ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}
		}

		//	The domain is alien so we cannot determine if the sender address exists so we assume it does. We check the sender's domain. If no reply from DNS assume the domain to be
		//	legal but if there is a reply and the domain has no mail servers, reject it

		if (!pInfo->m_bSenderLocalDom)
		{
			hzList<hzResServer>			ServersMX ;	//	Server names from MX query
			hzList<hzResServer>::Iter	iSvr ;		//	Server iterator
			hzDNS						dq ;		//	DNS object
			hzResServer					Svr ;		//	Server instance

			rc = GetHostByAddr(pInfo->fqdn, *pInfo->ipstr) ;
			if (rc != E_OK)
				pCC->m_Track.Printf("Sender %s, IP not found by GetHostByAddr (%s)\n", *pInfo->m_Sender, *pInfo->ipstr) ;
			else
				pCC->m_Track.Printf("GetHostByAddr set FQDN to %s for sender <%s> IP (%s)\n", *pInfo->fqdn, *pInfo->m_Sender, *pInfo->ipstr) ;

			//	Look for mail servers
			rc = dq.SelectMX(ServersMX, pInfo->m_SenderDomain) ;

			if (rc != E_OK)
			{
				pInfo->m_eState = SMTP_EXPECT_QUIT ;

				if (rc == E_DNS_FAILED)	R.Printf("421 Sender IP [%s] not resolved. DNS Failure, session terminated\r\n", *pInfo->m_Sender) ;
				if (rc == E_DNS_NOHOST)	R.Printf("550 <%s>... Sender not accepted. No host, session terminated\r\n", *pInfo->m_Sender) ;
				if (rc == E_DNS_NODATA)	R.Printf("550 <%s>... Sender not accepted. No host data, session terminated\r\n", *pInfo->m_Sender) ;
				if (rc == E_DNS_RETRY)	R.Printf("421 Sender IP [%s] not resolved. Please try later\r\n", *pInfo->m_Sender) ;

				pCC->m_Track << R ;
				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			if (!ServersMX.Count())
			{
				pInfo->m_eState = SMTP_EXPECT_QUIT ;
				R.Printf("550 <%s>... Sender not accepted. No mail servers, session terminated\r\n", *pInfo->m_Sender) ;
				pCC->m_Track << R ;
				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}

			//	Now we have the mailservers for the suppossed sender's domain, check if the server sending the email is one of these.
			pInfo->m_bSkunk = true ;
			for (iSvr = ServersMX ; iSvr.Valid() ; iSvr++)
			{
				Svr = iSvr.Element() ;

				pCC->m_Track.Printf("Trying mailserver %s [v=%d] [ip=%s]\n", *Svr.m_Servername, Svr.m_nValue, Svr.m_Ipa.Txt(recepA)) ;
				//test_ip.SetValue(Svr.m_ip[0], Svr.m_ip[1], Svr.m_ip[2], Svr.m_ip[3]) ;

				if (pInfo->ipa == Svr.m_Ipa)
					{ pInfo->m_bSkunk = false ; break ; }
				if (pInfo->fqdn == Svr.m_Servername)
				 	{ pInfo->m_bSkunk = false ; break ; }
			}

			if (pInfo->m_bSkunk)
				pCC->m_Track.Printf("Sender <%s> IP (%s) resolves to %s - skunk\n", *pInfo->m_Sender, *pInfo->ipstr, *pInfo->fqdn) ;
			else
				pCC->m_Track.Printf("Sender <%s> IP (%s) resolves to %s\n", *pInfo->m_Sender, *pInfo->ipstr, *pInfo->fqdn) ;
		}

		pInfo->m_eState = SMTP_EXPECT_RECIPIENT ;
		R.Printf("250 2.1.0 <%s>... Sender ok\r\n", *pInfo->m_Sender) ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == SMTP_EXPECT_AUTHUSER)
		return _smtpExpectAuthuser(Input, pCC) ;

	if (pInfo->m_eState == SMTP_EXPECT_AUTHPASS)
		return _smtpExpectAuthpass(Input, pCC) ;

	if (pInfo->m_eState == SMTP_EXPECT_RECIPIENT)
		return _smtpExpectRcpt(Input, pCC) ;

	if (pInfo->m_eState == SMTP_EXPECT_DATA)
	{
		//	Receiving data or the period on a line by itself
		_smtpExpectData(Input, pCC) ;

		if (pInfo->m_bAccError)
			return TCP_KEEPALIVE ;
		if (!pInfo->m_bPeriod)
			return TCP_KEEPALIVE ;

		rc = _smtpCheckSPAM(pCC) ;
		if (rc != E_OK)
		{
			R.Printf("554 5.7.1 Message rejected for SPAM content\r\n") ;
			pInfo->m_eState = SMTP_EXPECT_QUIT ;
			pInfo->m_bAccError = true ;
			pInfo->m_eBlock |= HZ_IPSTATUS_BLACK_SMTP ;
			pInfo->m_eBlock |= HZ_IPSTATUS_BLACK_PROT ;
			pCC->m_Track.Printf("IP BLOCKED %s Mail item SPAM so rejected\n", *pInfo->ipstr) ;

			sprintf(pInfo->testPad, "%s/%s.pop3", *theEP->m_Quarantine, *pInfo->theMsg.m_Id) ;
			pCC->m_Track.Printf("Starting Quarantine POP3 file [%s] for (%s)\n", pInfo->testPad, *accname) ;
			pInfo->os_item.open(pInfo->testPad, ios::app) ;
			pInfo->os_item << pInfo->msgBody ;
			pInfo->os_item.close() ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	Incomming messages must have an ID
		if (!pInfo->theMsg.m_Id)
		{
			/*
			R.Printf("554 5.7.1 Message rejected: No message id\r\n") ;
			pInfo->m_eState = SMTP_EXPECT_QUIT ;
			pInfo->m_bAccError = true ;
			pCC->m_Track.Printf("No Mail id so rejected\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			*/
		}

		//	Update the alien domain list
		if (!pInfo->m_bSenderLocalDom)
		{
			domainNo = _hzGlobal_FST_Domain->Locate(pInfo->theMsg.m_AddrFrom.GetDomain()) ;
			if (!domainNo)
			{
				_hzGlobal_FST_Domain->Insert(pInfo->theMsg.m_AddrFrom.GetDomain()) ;
				pCC->m_Track.Printf("Inserted new domain of %s\n", pInfo->theMsg.m_AddrFrom.GetDomain()) ;
			}

			emaddrNo = _hzGlobal_FST_Emaddr->Locate(*pInfo->theMsg.m_AddrFrom) ;
			if (!emaddrNo)
			{
				_hzGlobal_FST_Emaddr->Insert(*pInfo->theMsg.m_AddrFrom) ;
				pCC->m_Track.Printf("Inserted new address of %s\n", *pInfo->theMsg.m_AddrFrom) ;
			}

			tmpDom = pInfo->theMsg.m_AddrFrom.GetDomain() ;
			tmpFro = *pInfo->theMsg.m_AddrFrom ;
		}

		_smtpCoreCommit(pCC) ;
		_smtpPOP3Commit(pCC) ;

		if (pInfo->alienRcpts.Count())
			rc = _smtpRelayCommit(pCC) ;

		if (pInfo->m_bAccError)
			R.Printf("421 Internal fault\r\n") ;
		else
			R.Printf("250 2.6.0 Message accepted for delivery\r\n") ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == SMTP_EXPECT_QUIT)
	{
		//	Here we expect a QUIT command and everything else is rejected
		//	with a 503 (bad sequence of commands)

		if (zi != "QUIT\r\n")
		{
			R.Printf("503 Expecting a QUIT and nothing else\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		R.Printf("221 Epistula SMTP session terminated normally\r\n") ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_TERMINATE ;
	}

	for (n = 0 ; !zi.eof() && *zi >= CHAR_SPACE && n < 123 ; n++, zi++)
		pInfo->testPad[n] = *zi ;
	pInfo->testPad[n] = 0 ;

	R.Printf("503 Could not understand command [%s]\r\n", pInfo->testPad) ;
	pCC->m_Track << R ;
	pCC->m_Track.Printf("\t[%s]\n", pInfo->testPad) ;
	pCC->SendData(R) ;
	Input.Clear() ;
	return TCP_KEEPALIVE ;
}

hzTcpCode	ProcLocalSMTP		(hzChain& Input, hzIpConnex* pCC)
{
	//	Process an SMTP client connection on the port for local user message origination, as set by the system administrator. Epistula will only accept incoming messages originated
	//	by alien users on the standard SMTP port. There are a number of different steps when a 
	//
	//	Unlike incoming messages from alien senders, which are required to have a message id, the message created as new on the client computer, will either be without a message id
	//	or it will have an id that is unlikely to conform to what Epistula would assign.
	//
	//	Arguments:	1) Input	Client input
	//				2) pCC		Pointer to client connection
	//
	//	Returns:	TCP_TERMINATE	If connection is to be terminated.
	//				TCP_KEEPALIVE	If connection is kept open (session is ongoing).

	_hzfunc(__func__) ;

	SmtpSession*	pInfo ;			//	SMTP Session info
	hzChain			R ;				//	Response to client
	chIter			zi ;			//	Input chain iterator
	hzFxDomain		tmpDom ;		//	Domain of sender
	hzFxEmaddr		tmpFro ;		//	Address of sender
	uint32_t		domainNo ;		//	Domain number as given by _hzGlobal_FST_Domain
	uint32_t		emaddrNo ;		//	Emaddr number as given by _hzGlobal_FST_Emaddr
	int32_t			nTest ;			//	General iterator
	int32_t			n ;				//	Loop counter
	hzEcode			rc ;			//	Return code from DNS lookup functions

	/*
	**	Initialize session
	*/

	pInfo = (SmtpSession*) pCC->GetInfo() ;
	g_nSessIdAuth++ ;

	/*
	**	Handle client requests
	*/

	if (Input.Size() < 100)
	{
		for (zi = Input, nTest = 0 ; !zi.eof() && nTest < 100 && *zi >= CHAR_SPACE ; zi++, nTest++)
			pInfo->testPad[nTest] = *zi ;
		pInfo->testPad[nTest] = 0 ;
		pCC->m_Track.Printf("%s. SMTP client %d with %d bytes [%s]\n", *_fn, pCC->EventNo(), Input.Size(), pInfo->testPad) ;
	}
	else
		pCC->m_Track.Printf("%s. SMTP client %d with %d bytes\n", *_fn, pCC->EventNo(), Input.Size()) ;
	zi = Input ;

	/*
	**	Handle message
	*/

	if (zi.Equiv("QUIT\r\n"))
	{
		R.Printf("221 Epistula SMTP session terminated normally\r\n") ;
		pCC->SendData(R) ;
		Input.Clear() ;
		return TCP_TERMINATE ;
	}

	if (zi.Equiv("RSET "))
	{
		R += "+OK\r\n" ; pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == SMTP_EXPECT_HELLO)
		return _smtpExpectHello(Input, pCC) ;

	if (pInfo->m_eState == SMTP_EXPECT_SENDER)
	{
		//	Establish Sender Domain and Address

		if (zi.Equiv("AUTH"))
		{
			pInfo->m_eState = SMTP_EXPECT_AUTHUSER ;
			R.Printf("334 VXNlcm5hbWU6\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	Obtain the Sender
		if (!zi.Equiv("MAIL FROM"))
		{
			R.Printf("503 Expected a sender address\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		if (ExtractEmailAddr(pInfo->m_Sender, zi, pCC->m_Track) != E_OK)
		{
			pInfo->m_eState = SMTP_EXPECT_QUIT ;
			R.Printf("501 Sender email address could not be deciphered.\r\n") ;
			pCC->m_Track << R ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}
		pInfo->m_SenderDomain = pInfo->m_Sender.GetDomain() ;
		pCC->m_Track.Printf("SETTING Sender %s domain %s\r\n", *pInfo->m_Sender, *pInfo->m_SenderDomain) ;

		//	Deal with special case of callback (blank sender)
		if (!pInfo->m_Sender)
		{
			pInfo->m_bCallback = true ;
			pInfo->m_eState = SMTP_EXPECT_RECIPIENT ;
			R.Printf("250 2.1.0 <>... Sender ok (callback)\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	Check if sender is local or alien
		if (theEP->m_LocalDomains.Exists(pInfo->m_SenderDomain))
		{
			pInfo->m_bSenderLocalDom = true ;
			if (theEP->m_Locaddr2Orig.Exists(pInfo->m_Sender))
				pInfo->m_bSenderLocalAddr = true ;
			else
				pInfo->m_bQtine = true ;
		}
		else
		{
			//	The sender is alien, not allowed here so quarantine
			pInfo->m_bQtine = true ;
			if (g_setBanned.Exists(pInfo->m_SenderDomain))
			{
				pInfo->m_eState = SMTP_EXPECT_QUIT ;
				R.Printf("550 <%s>... Sender not accepted. terminated\r\n", *pInfo->m_Sender) ;
				pCC->m_Track << R ;
				pCC->m_Track << "Sender is banned\n" ;

				pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
			}
		}
		pInfo->m_eState = SMTP_EXPECT_RECIPIENT ;
		R.Printf("250 2.1.0 <%s>... Sender ok\r\n", *pInfo->m_Sender) ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == SMTP_EXPECT_AUTHUSER)
		return _smtpExpectAuthuser(Input, pCC) ;

	if (pInfo->m_eState == SMTP_EXPECT_AUTHPASS)
		return _smtpExpectAuthpass(Input, pCC) ;

	if (pInfo->m_eState == SMTP_EXPECT_RECIPIENT)
		return _smtpExpectRcpt(Input, pCC) ;

	if (pInfo->m_eState == SMTP_EXPECT_DATA)
	{
		//	Receiving data or the period on a line by itself
		_smtpExpectData(Input, pCC) ;

		if (pInfo->m_bAccError)
			return TCP_KEEPALIVE ;
		if (!pInfo->m_bPeriod)
			return TCP_KEEPALIVE ;

		rc = _smtpCheckSPAM(pCC) ;
		if (rc != E_OK)
		{
			R.Printf("554 5.7.1 Message rejected for SPAM content\r\n") ;
			pInfo->m_eState = SMTP_EXPECT_QUIT ;
			pInfo->m_bAccError = true ;
			pInfo->m_eBlock |= HZ_IPSTATUS_BLACK_SMTP ;
			pInfo->m_eBlock |= HZ_IPSTATUS_BLACK_PROT ;
			pCC->m_Track.Printf("IP BLOCKED %s Mail item SPAM so rejected\n", *pInfo->ipstr) ;

			sprintf(pInfo->testPad, "%s/%s.pop3", *theEP->m_Quarantine, *pInfo->now.Str()) ;
			pCC->m_Track.Printf("Quarantine POP3 file [%s]\n", pInfo->testPad) ;
			pInfo->os_item.open(pInfo->testPad, ios::app) ;
			pInfo->os_item << pInfo->msgBody ;
			pInfo->os_item.close() ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	Incomming messages must have an ID
		if (!pInfo->theMsg.m_Id)
		{
			R.Printf("554 5.7.1 Message rejected: No message id\r\n") ;
			pInfo->m_eState = SMTP_EXPECT_QUIT ;
			pInfo->m_bAccError = true ;
			pCC->m_Track.Printf("No Mail id so rejected\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		//	Update the alien domain list
		if (!pInfo->m_bSenderLocalDom)
		{
			domainNo = _hzGlobal_FST_Domain->Locate(pInfo->theMsg.m_AddrFrom.GetDomain()) ;
			if (!domainNo)
			{
				_hzGlobal_FST_Domain->Insert(pInfo->theMsg.m_AddrFrom.GetDomain()) ;
				pCC->m_Track.Printf("Inserted new domain of %s\n", pInfo->theMsg.m_AddrFrom.GetDomain()) ;
			}

			emaddrNo = _hzGlobal_FST_Emaddr->Locate(*pInfo->theMsg.m_AddrFrom) ;
			if (!emaddrNo)
			{
				_hzGlobal_FST_Emaddr->Insert(*pInfo->theMsg.m_AddrFrom) ;
				pCC->m_Track.Printf("Inserted new address of %s\n", *pInfo->theMsg.m_AddrFrom) ;
			}

			tmpDom = pInfo->theMsg.m_AddrFrom.GetDomain() ;
			tmpFro = *pInfo->theMsg.m_AddrFrom ;
		}

		_smtpCoreCommit(pCC) ;
		_smtpPOP3Commit(pCC) ;

		if (pInfo->alienRcpts.Count())
			rc = _smtpRelayCommit(pCC) ;

		if (pInfo->m_bAccError)
			R.Printf("421 Internal fault\r\n") ;
		else
			R.Printf("250 2.6.0 Message accepted for delivery\r\n") ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		return TCP_KEEPALIVE ;
	}

	if (pInfo->m_eState == SMTP_EXPECT_QUIT)
	{
		//	Here we expect a QUIT command and everything else is rejected
		//	with a 503 (bad sequence of commands)

		if (zi != "QUIT\r\n")
		{
			R.Printf("503 Expecting a QUIT and nothing else\r\n") ;

			pCC->SendData(R) ; Input.Clear() ; return TCP_KEEPALIVE ;
		}

		R.Printf("221 Epistula SMTP session terminated normally\r\n") ;

		pCC->SendData(R) ; Input.Clear() ; return TCP_TERMINATE ;
	}

	for (n = 0 ; !zi.eof() && *zi >= CHAR_SPACE && n < 123 ; n++, zi++)
		pInfo->testPad[n] = *zi ;
	pInfo->testPad[n] = 0 ;

	R.Printf("503 Could not understand command [%s]\r\n", pInfo->testPad) ;
	pCC->m_Track << R ;
	pCC->m_Track.Printf("\t[%s]\n", pInfo->testPad) ;
	pCC->SendData(R) ;
	Input.Clear() ;
	return TCP_KEEPALIVE ;
}
