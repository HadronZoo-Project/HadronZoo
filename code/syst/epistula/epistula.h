//
//	File:	epistula.h
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

#if !defined(EPISTULA_H)
#define EPISTULA_H

#include <fstream>

using namespace std ;

#include "hzCtmpls.h"
#include "hzString.h"
#include "hzMailer.h"
#include "hzDissemino.h"
#include "hzTree.h"
#include "hzHttpServer.h"

//	Standard Ports
#define	STD_CDBS_PORT		19127				//	Port central database is listening on
#define	STD_SMTP_PORT		587					//	Port SMTP is listening on
#define	STD_SMTP_AUX_PORT	587					//	Port SMTP is listening on
#define	STD_POP3_PORT		2110				//	Port POP3 is listening on
#define	STD_HTTP_PORT		19023				//	Port HTTP is listening on

//	Other Definitions
#define	LOCAL_SVR			"127.0.0.1"			//	This physical server
#define	TIMVAL				struct timeval		//	For select

/*
**	OUTGOING messages
*/

enum	epRlyStatus
{
	//	Possible results from a relay task

	RELAY_DUETORUN	= 0,	//	Relay is due for running.
	RELAY_SUCCESS	= 1,	//	Relay was completely successful, all recipients passed and the message was accepted.
	RELAY_COMPLETE	= 2,	//	Relay only partly successful. The message was accepted by the target domain's mailserver but not on behalf of all recipients.
	RELAY_DELAYED	= 3,	//	Relay delayed because of a temporary failure.
	RELAY_NODOM		= 4,	//	Conclusive failure. Relay aborted because the DNS has established that the target domain does not exist or has no registered mailservers.
	RELAY_BADSERVER	= 5,	//	Conclusive failure. Relay aborted because this server is blocked by the target domain mailserver.
	RELAY_BADSENDER	= 6,	//	Conclusive failure. Relay aborted because the sender is blocked by the target domain mailserver.
	RELAY_TOOLARGE	= 7,	//	Conclusive failure. Relay aborted because the message is too large for the target domain mailserver.
	RELAY_BADPROTO	= 8,	//	Conclusive failure. Relay aborted because target domain mailserver has given a 500 series return code for reasons other than those listed above.
	RELAY_INT_ERR	= 9,	//	Conclusive failure. Relay aborted because of an internal error (such as file could not be opened)
	RELAY_EXPIRED	= 10,	//	The message has timed out and has been abandoned. No recipients in the target domain will receive the message.
} ;

/*
**	The mqRcpt (recipient) class
*/

//	#define RCPT_LOCAL	0x01	//	Recipient is local
//	#define RCPT_RELAY	0x02	//	Recipient is local but needs forwarding
//	#define RCPT_SENT	0x04	//	Recipient has been delivered
//	#define RCPT_DELAY	0x08	//	Delivery has been delayed (Alien SMTP server down)
//	#define RCPT_FATAL	0x10	//	Recipient does not exist.

struct	mqRcpt
{
	hzEmaddr	m_Addr ;		//	Full email address
	hzString	m_Reason ;		//	Reason for failure (if any)
} ;

class	epRTR
{
	//	Relay Task Record
public:
	hzString	m_Tag ;			//	Mail item ID and Destination domain
	hzString	m_Server ;		//	IP address of server mailitem sent to
	bool		m_bStatus ;

	epRTR	(void)	{}
	~epRTR	(void)	{}
} ;

class	epRelay
{
	//	A relay equates to one outgoing SMTP conversation in which an email is relayed to an alien server with a set of one or more recipient addresses, all belonging to the domain
	//	of the alien server. A single email with multiple recipients implying multiple domains, will produce one relay task per implied domain.

	hzString	m_Reason ;			//	Reason given by alien server for reject
	hzString	m_Realname ;		//	Temporary real name of sender
	hzString	m_Domain ;			//	Domain
	hzString	m_MsgId ;			//	Email item identifier
	epRlyStatus	m_eResult ;			//	Result of the relay

public:
	hzList<mqRcpt*>	m_CommonRcpt ;	//	Recipients for this relay (this domain)
	//hzList<hzEmaddr>	m_CommonRcpt ;	//	Recipients for this relay (this domain)

	int32_t	m_timeDue ;				//	Date email received by SMTP server
	int32_t	m_timeRun ;				//	Date of next attempt to relay

	epRelay		(void)
	{
		m_timeDue = time(0) ;
		m_timeRun = m_timeDue ;
		m_eResult = RELAY_DUETORUN ;
	}

	~epRelay	(void)
	{
		ClearRecipients() ;
	}

	void	ClearRecipients	(void) ;

	//	Set functions

	void	SetMailID	(hzString& mailId)		{ m_MsgId = mailId ; }
	void	SetDomain	(hzString& domain)		{ m_Domain = domain ; }
	void	SetRealname	(hzString& realname)	{ m_Realname = realname ; }
	void	SetReason	(hzString& reason)		{ m_Reason = reason ; }
	void	SetReason	(const char* res)		{ m_Reason = res ; }
	void	SetResult	(epRlyStatus eResult) ;
	void	Delay		(int32_t nInterval = 0) ;

	//	Get functions

	hzString&	GetMailID	(void)	{ return m_MsgId ; }
	hzString&	GetDomain	(void)	{ return m_Domain ; }
	hzString&	GetRealname	(void)	{ return m_Realname ; }
	hzString&	GetReason	(void)	{ return m_Reason ; }

	//	List functions

	bool	AddRecipient	(mqRcpt* rcpt)
	//bool	AddRecipient	(hzEmaddr& rcpt)
	{
		return m_CommonRcpt.Add(rcpt) == E_OK ? true : false ;
	}

	int32_t	GetNoRecipients	(void)	{ return m_CommonRcpt.Count() ; }

	epRlyStatus	GetResult	(void)	{ return m_eResult ; }

	bool	RelaySMTP	(void) ;
} ;

class	mqItem
{
	//	The email as it is manifest in the mail que

public:
	hzMapS<hzString,epRelay*>	m_RelayTasks ;		//	Resultant relay tasks
	hzSet<mqRcpt*>				m_Recipients ;		//	Recipients named in the email

	hzChain		m_Data ;		//	For building email body
	hzChain		m_Report ;		//	For report in case of a bounce
	hzXDate		m_Date ;		//	Time message received by SMTP server (date of staging file)
	hzXDate		m_Due ;			//	Due for next run
	hzString	m_Sender ;		//	Sender's email address
	hzString	m_User ;		//	Sender's username if local
	hzString	m_Announce ;	//	Sender's announcement
	hzString	m_FQDN ;		//	Resolved IP address of sender
	hzString	m_Id ;			//	Epistula assigned mail ID
	hzIpaddr	m_IP ;			//	IP Address of sender

	mqItem	(void)
	{
		m_RelayTasks.SetDefaultObj((epRelay*)0) ;
		m_Recipients.SetDefault((mqRcpt*)0) ;
	}

	~mqItem	(void)
	{
		Clear() ;
	}

	void	Clear		(void) ;

	//	Set Functions
	bool	SetId			(hzString& mailId)	{ m_Id = mailId ; }
	hzEcode	SetSender		(const char* cpSender) ;
	bool	SetAnnounce		(const char* cpAnnounce) ;
	bool	SetIPAddr		(const char* cpIPAddr) ;
	bool	SetFQDN			(const char* cpFQDN) ;
	bool	AddRecipient	(mqRcpt* pRecipient) ;

	bool	CommitHeaders	(void) ;
	bool	CommitMessage	(const char* cpData, int32_t nNoBytes) ;
	bool	CommitMessage	(void) ;
	bool	CommitFinal		(void) ;
	bool	AddData			(const char* cpData, int32_t nNoBytes) ;

	hzEcode	ReadHeaders		(void) ;
	hzEcode	DelQueEntry		(void) ;

	//	Get Functions

	hzString&	GetMailID	(void)	{ return m_Id ; }
	hzString&	GetSender	(void)	{ return m_Sender ; }
	hzString&	GetAnnounce	(void)	{ return m_Announce ; }
	bool	IsSenderLocal	(void)	{ return m_User ? true : false ; }

	bool	IsDue			(void) ;

	//	Proc Functions

	bool	AppendPOP3File	(const char* cpUsername, const char* cpEMAddr) ;
	void	RelaySMTP		(epRelay* pRT) ;
	bool	Deliver			(void) ;
	bool	DoNextTask		(void) ;
	hzEcode	Bounce			(void) ;

	static	bool	CheckFiles		(void) ;
} ;

/*
**	Section X:	Mailbox
*/

#if 0
epMBItem
#endif

/*
**	The account class
*/

class	epFolder	//	: public hzTreeitem
{
	//	NOTE

public:
	hzString	m_Parent ;			//	Folder this will be a sub-folder of (if any)
	hzString	m_Name ;			//	Visible folder name
	uint32_t	m_CorresId ;		//	Correspondent object id
	uint32_t	m_Info ;			//	Flags

	epFolder	(void)	{ m_CorresId = 0 ; m_Info = 0 ; }

	bool	operator<	(const epFolder& op) const
	{
		//if (m_UserId > op.m_UserId)		return false ;
		//if (m_UserId < op.m_UserId)		return true ;
		if (m_CorresId < op.m_CorresId)	return true ;

		return false ;
	}

	bool	operator>	(const epFolder& op) const
	{
		//if (m_UserId < op.m_UserId)		return false ;
		//if (m_UserId > op.m_UserId)		return true ;
		if (m_CorresId > op.m_CorresId)	return true ;

		return false ;
	}
} ;

class	epAccount
{
	//	An epistula user or an epistula account has a single unique username, one or more email dedicated unique addresses. Each account has a set of correspondents. An address is
	//	a correspondent (of the user) as long as the user retains at least one message from or two that address. Each correspondent gives rise to a folder 

public:
	hzMapM	<hzFxDomain,hzFxEmaddr>	m_Correspondents ;		//	Correspondent addresses grouped by domain
	hzSet	<hzFxDomain>			m_CorresDomains ;		//	Correspondent domains

	hzMapS	<uint32_t,epFolder*>	m_FoldersByID ;
	hzMapS	<hzString,epFolder*>	m_FoldersByNAME ;
	
	//hzTree	<hzString,epFolder*>	m_Folders ;
	hdsTree		m_Folders ;

	//hzTree		m_Folders ;			//	User's tree of folders
	hzChain		m_Body ;			//	Current mail item body
	hzLockS		m_LockPOP3 ;		//	POP3 File lock
	hzString	m_Unam ;			//	Customer's username
	hzString	m_Pass ;			//	Customer's password
	hzString	m_Mboxdir ;			//	Directory of mailbox (not stored in db, set on first reading of mailbox)
	uint32_t	m_epoch ;			//	Time of load
	uint32_t	m_Limit ;			//	Max size of incoming email
	uint32_t	m_ruid ;			//	RUID for user account
	uint32_t	m_nTotalSize ;		//	Total size of mailbox

	epAccount	(void)
	{
		m_ruid = -1 ;
	}
	~epAccount	(void)	{}

	static	hzEcode	Load	(void) ;
	static	hzEcode	Save	(void) ;

	hzChain&	GetBody	(void) ;
	//const epMBItem&	GetItem	(hzString& MailID) ;
	//const epMBItem&	GetItem	(int32_t nIndex) ;

	bool		BuildList			(bool bDetails) ;
	//	bool		GetMailitemDetails	(epMBItem& Item, const char* cpFilename) ;
	//bool		GetMailitemContent	(epMBItem& Item, const char* cpFilename) ;
	void		UnDeleteAll			(void) ;
	void		DeleteFiles			(void) ;
} ;

class	Epistula
{
	//	This holds all data entities

	//	Private constructor, singleton instance
	Epistula	(void)
	{
		m_pClassDomain = 0 ;
		m_pClassAddr = 0 ;
		m_pClassForward = 0 ;
		m_pClassEmsg = 0 ;

		m_pCacheDomain = 0 ;
		m_pCacheAddr = 0 ;
		m_pCacheForward = 0 ;
		m_pCacheEmsg = 0 ;

		m_pCronCenter = 0 ;
	}

public:
	hzSet	<hzString>				m_LocalDomains ;	//	All local domains
	hzSet	<hzString>				m_setOrigLoc ;		//	All viable account names and local addresses for message origination

	hzMapM	<hzEmaddr,hzString>		m_Locaddr2Orig ;	//	Local addresses to the users who may originate emails using the address.
	hzMapM	<hzEmaddr,hzString>		m_Locaddr2Mbox ;	//	Local addresses to mailboxes
	hzMapM	<hzEmaddr,hzString>		m_Locaddr2Frwd ;	//	Local addresses to their alien forwards (if any)
	hzMapM	<hzEmaddr,hzPair>		m_Locaddr2Filt ;	//	Local addresses to user defined reserved repositories within mailboxes (e.g. Google Alerts)
	hzMapS	<hzEmaddr,hzString>		m_LocAddrs ;		//	All local addresses to the user accounts per local address 
	hzMapS	<hzString,epAccount*>	m_Accounts ;		//	Usernames to the mailboxes per username 

	hdbClass*		m_pClassDomain ;			//	Epistula domain
	hdbClass*		m_pClassAddr ;				//	Epistula email address
	hdbClass*		m_pClassForward ;			//	Epistula email forward
	hdbClass*		m_pClassDlvr ;				//	Delivery record class
	hdbClass*		m_pClassEmsg ;				//	The email message class
	hdbClass*		m_pClassCorres ;			//	Correspondent class
	hdbClass*		m_pClassFolder ;			//	Folder class

	hdbObjCache*	m_pCacheDomain ;			//	Repository of all users
	hdbObjCache*	m_pCacheAddr ;				//	Repository of all local addresses
	hdbObjCache*	m_pCacheForward ;			//	Repository of forwards
	hdbObjCache*	m_pCacheDlvr ;				//	Repository of deliveries
	hdbObjCache*	m_pCacheEmsg ;				//	Central repository of short form messages
	hdbObjCache*	m_pCacheCorres ;			//	Central repository of correspondents

	hdbBinRepos*	m_pCronCenter ;				//	Central repository of IMF binaries (whole form messages)
	hdbIsamfile*	m_pIsamCenter ;				//	Core ISAM of all short form messages
	hdbIsamfile*	m_pIsamFormal ;				//	ISAM of formal message ids to IMF datum ids

	//	Standard locations
	hzString		m_Configs ;					//	Epistula HTML configs directory
	hzString		m_ConfHttp ;				//	File for HTTP config
	hzString		m_LogsDir ;					//	Directory for logfiles
	hzString		m_MailQue ;					//	Directory for remote delivery (/home/epistula/mque)
	hzString		m_MailBox ;					//	Epistula mailbox directory
	hzString		m_Relays ;					//	Directory for remote delivery (/home/epistula/mque)
	hzString		m_EpisData ;				//	Epistula data directory
	hzString		m_Quarantine ;				//	Epistula quarantine directory
    hzString		m_EpisStrings ;				//	Epistula string table file for strings
    hzString		m_EpisDomains ;				//	Epistula string table file for domains
    hzString		m_EpisEmaddrs ;				//	Epistula string table file for email addresses


	static	Epistula*	GetInstance	(void) ;

	hzEcode		InitLocations		(const hzString& epistulaHome) ;
	hzEcode		InitEpistulaClasses	(void) ;
	hzEcode		UpdateCorres		(uint32_t& corresId, const hzEmaddr& addr, const hzString& rname, hzChain& err) ;

	hzEcode		UpdateShortForm
	(
		uint32_t&					objId,
		uint32_t&					datumId,
		const hzArray<hzEmaddr>&	rcpts,
		const hzString&				msgId,
		hzXDate&					now,
		const hzEmaddr&				fromAddr,
		const hzString&				rname,
		const hzString&				subject,
		const hzChain&				msgBody,
		hzChain&					err
	) ;
} ;

/*
**	Prototypes
*/

bool		CDBConnect	(void) ;
bool		CDBError	(void) ;

hzEcode		GetMailID	(hzString& Id) ;
void		GenMailID	(hzString& Id) ;

hzEcode		Deliver			(hzEmail& E) ;
bool		AddDomains		(const char* cpAddr) ;
bool		LocateDomain	(const char* cpAddr) ;
bool		UserAdd			(const char* cpUsername, const char* cpPassword) ;

char*		CheckIPAddr			(const char* cpIPAddr) ;
hzEcode		ReadConfig			(hzChain& error) ;
int			ConnectToMailServer	(const char* cpDomain) ;
void*		ProcSMTPConnection	(void* arg) ;
bool		EmailValid			(const char* cpEMAddr) ;
bool		DisasterRecovery	(void) ;
hzEcode		AppendPOP3File		(hzEmail& em, hzString& Mailbox, hzEmaddr& prime_recipient, hzString& fqdn, hzIpaddr sender_ip, hzChain& error) ;
void*		DeliveryManager		(void* pV) ;

#endif	//	EPISTULA_H
