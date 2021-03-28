//
//	File:	epData.cpp
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

extern	hzLogger	slog ;		//	System logs
global	hdsApp*		theApp ;	//  Dissemino HTML interface
global	Epistula*	theEP ;		//	The singleton Epistula Instance

global	hzString	g_str_Inbox	= "Inbox" ;
global	hzString	g_str_Draft	= "Drafts" ;
global	hzString	g_str_Sent	= "Sent" ;
global	hzString	g_str_Ads	= "Adverts" ;
global	hzString	g_str_Qtine	= "Quarantine" ;
global	hzString	g_str_Misc	= "Miscellaneous" ;

/*
**	Epistula Database Functions
*/

Epistula*	Epistula::GetInstance	(void)
{
	_hzfunc("Epistula::GetInstance") ;

	if (!theEP)
		theEP = new Epistula() ;
	return theEP ;
}

hzEcode	Epistula::InitLocations	(const hzString& epistulaHome)
{
	_hzfunc("Epistula::InitLocations") ;

	hzString	S ;

	if (!epistulaHome)
		S = "/home/epistula" ;
	else
	{
		if (epistulaHome[epistulaHome.Length()-1] == CHAR_FWSLASH)
			S = epistulaHome ;
		else
			S = epistulaHome + "/" ;
	}

	m_Configs		= S + "conf" ;
	m_ConfHttp		= S + "conf/episHttp.xml" ;
	m_LogsDir		= S + "logs" ;
	m_MailQue		= S + "mque" ;
	m_MailBox		= S + "mbox" ;
	m_Relays		= S + "mque/relay.dat" ;
	m_EpisData		= S + "data" ;
	m_Quarantine	= S + "qtine" ;
	m_EpisStrings	= S + "data/episStrings" ;
	m_EpisDomains	= S + "data/episDomains" ;
	m_EpisEmaddrs	= S + "data/episEmaddrs" ;

	if (AssertDir(*m_Configs,	0755) != E_OK)	{ cout << "Epistula could not make a directory of " << m_Configs << "\n" ;		return E_OPENFAIL ; }
	if (AssertDir(*m_LogsDir,	0755) != E_OK)	{ cout << "Epistula could not make a directory of " << m_LogsDir << "\n" ;		return E_OPENFAIL ; }
	if (AssertDir(*m_MailQue,	0755) != E_OK)	{ cout << "Epistula could not make a directory of " << m_MailQue << "\n" ;		return E_OPENFAIL ; }
	if (AssertDir(*m_MailBox,	0755) != E_OK)	{ cout << "Epistula could not make a directory of " << m_MailBox << "\n" ;		return E_OPENFAIL ; }
	if (AssertDir(*m_EpisData,	0755) != E_OK)	{ cout << "Epistula could not make a directory of " << m_EpisData << "\n" ;		return E_OPENFAIL ; }
	if (AssertDir(*m_Quarantine,0755) != E_OK)	{ cout << "Epistula could not make a directory of " << m_Quarantine << "\n" ;	return E_OPENFAIL ; }

	return E_OK ;
}

hzEcode	Epistula::InitEpistulaClasses	(void)
{
	//	Add members to user, address and message classes and set up repositories. Note that while this can be done in the dissemino XML config, that approach is for
	//	website development rather than here where we are using dissemino as a C++ front end. The percent entities in the dissemino pages will link to repositories
	//	regardless of how they were initialized.

	_hzfunc("Epistula::InitEpistulaClasses") ;

	hzChain			err ;			//	Error message compilation
	hzString		title ;			//	Entity title
	hzString		tmpStr ;		//	Temp string
	//hzFixStr		tmpFix ;		//	Temp string
	hzAtom			atom ;			//	Atomic value
	hzAtom			atomB ;			//	Atomic value
	hzAtom			atomC ;			//	Atomic value
	hzAtom			atomD ;			//	Atomic value
	hzPair			pair ;			//	For filters
	hzEmaddr		ema ;			//	Email address
	hzString		user ;			//	User username
	hzString		mbox ;			//	Mailbox name
	hzString		mboxdir ;		//	Mailbox directory
	hdbObjStore*	pRepos ;		//	Attach repository for the account
	epAccount*		pAcc ;			//	User account
	hdbIsamfile*	pIsamFile ;		//	User mailbox ISAM of message headers
	uint32_t		objId ;			//	Object iterator
	uint32_t		n ;				//	Object iterator
	hzEcode			rc = E_OK ;		//	Return code

	//	Init the string table
	hzFsTbl::StartStrings(m_EpisStrings) ;
	hzFsTbl::StartDomains(m_EpisDomains) ;
	hzFsTbl::StartEmaddrs(m_EpisEmaddrs) ;

	if (!_hzGlobal_StringTable)	{ slog.Out("Could not create global string table\n") ; return E_MEMORY ; }
	if (!_hzGlobal_FST_Domain)	{ slog.Out("Could not create global domain table\n") ; return E_MEMORY ; }
	if (!_hzGlobal_FST_Emaddr)	{ slog.Out("Could not create global emaddr table\n") ; return E_MEMORY ; }

	/*
	**	The domain class
	*/

	m_pClassDomain = new hdbClass(theApp->m_ADP, HDB_CLASS_DESIG_CFG) ;
	title = "domains" ;

	if (rc == E_OK)	rc = m_pClassDomain->InitStart(title) ;
	if (rc == E_OK)	rc = m_pClassDomain->InitMember("name", "domain", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassDomain->InitDone() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterDataClass(m_pClassDomain) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init class 'domains'\n") ; return rc ; }

	m_pCacheDomain = new hdbObjCache(theApp->m_ADP) ;

	if (rc == E_OK)	rc = m_pCacheDomain->InitStart(m_pClassDomain, title, m_EpisData) ;
	if (rc == E_OK)	rc = m_pCacheDomain->InitMbrIndex("name", true) ;
	if (rc == E_OK)	rc = m_pCacheDomain->InitDone() ;
	if (rc == E_OK)	rc = m_pCacheDomain->Open() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterObjRepos(m_pCacheDomain) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init cache 'domains'\n") ; return rc ; }

	for (n = 1 ; n <= m_pCacheDomain->Count() ; n++)
	{
		m_pCacheDomain->Fetchval(atom, 0, n) ;
		tmpStr = atom.Str() ;
		m_LocalDomains.Insert(tmpStr) ;
		slog.Out("Inserted domain %s (pop now %d)\n", *tmpStr, m_LocalDomains.Count()) ;
	}

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init cache 'domains'\n") ; return rc ; }

	/*
	**	Subscribers (account usernames/passwords)
	*/

	rc = theApp->m_ADP.InitSubscribers(m_EpisData) ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init subscriber cache\n") ; return rc ; }
	if (!theApp->m_Allusers)
		theApp->m_Allusers = (hdbObjCache*) theApp->m_ADP.GetObjRepos("subscriber") ;
	if (!theApp->m_Allusers)
		{ slog.Log(_fn, "Could not init subscriber cache\n") ; return rc ; }
	rc = theApp->m_Allusers->Open() ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not open subscriber cache\n") ; return rc ; }
	slog.Out("Created Subscribers\n") ;

	/*
	**	Local Addresses
	*/

	m_pClassAddr = new hdbClass(theApp->m_ADP, HDB_CLASS_DESIG_CFG) ;
	title = "local" ;

	if (rc == E_OK)	rc = m_pClassAddr->InitStart(title) ;
	if (rc == E_OK)	rc = m_pClassAddr->InitMember("emaddr", "emaddr", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassAddr->InitMember("user",   "string", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassAddr->InitMember("mbox",   "string", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassAddr->InitMember("LUID",   "uint16", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassAddr->InitDone() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterDataClass(m_pClassAddr) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init class 'local'\n") ; return rc ; }

	m_pCacheAddr = new hdbObjCache(theApp->m_ADP) ;

	if (rc == E_OK)	rc = m_pCacheAddr->InitStart(m_pClassAddr, title, m_EpisData) ;
	if (rc == E_OK)	rc = m_pCacheAddr->InitMbrIndex("emaddr", true) ;
	if (rc == E_OK)	rc = m_pCacheAddr->InitDone() ;
	if (rc == E_OK)	rc = m_pCacheAddr->Open() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterObjRepos(m_pCacheAddr) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init cache 'local'\n") ; return rc ; }

	for (n = 1 ; n <= m_pCacheAddr->Count() ; n++)
	{
		tmpStr = "emaddr" ;	m_pCacheAddr->Fetchval(atomB, tmpStr, n) ;	//	ema = atomB.Str() ;
		tmpStr = "user" ;	m_pCacheAddr->Fetchval(atomC, tmpStr, n) ;	//	user = atomC.Str() ;
		tmpStr = "mbox" ;	m_pCacheAddr->Fetchval(atomD, tmpStr, n) ;	//	mbox = atomD.Str() ;

		slog.Out("case 1: HAVE LOCAL of %s (%s) (%s)\n", *atomB.Str(), *atomC.Str(), *atomD.Str()) ;
		ema = atomB.Str() ;
		user = atomC.Str() ;
		mbox = atomD.Str() ;
		slog.Out("case 2: HAVE LOCAL of %s (%s) (%s)\n", *ema, *user, *mbox) ;

		if (!ema)
			{ slog.Log(_fn, "Could not init cache 'local'\n") ; return E_BADVALUE ; }

		if (ema)
		{
			if (user)
			{
				m_Locaddr2Orig.Insert(ema, user) ;
				//tmpStr = user + *ema ;
				m_setOrigLoc.Insert(tmpStr) ;
				slog.Out("HAVE user/orig of %s - %s\n", *user, *ema) ;
			}

			if (mbox)
			{
				m_LocAddrs.Insert(ema, mbox) ;
				m_Locaddr2Mbox.Insert(ema, mbox) ;
			}

			if (user && mbox)
			{
				pAcc = new epAccount() ;
				pAcc->m_Unam = user ;
				pAcc->m_Mboxdir = m_MailBox + "/" + mbox ;
				//tmpFix = 0 ;
				pAcc->m_Folders.AddHead(0, g_str_Inbox, g_str_Inbox, true) ;
				pAcc->m_Folders.AddHead(0, g_str_Draft, g_str_Draft, true) ;
				pAcc->m_Folders.AddHead(0, g_str_Sent, g_str_Sent, true) ;
				pAcc->m_Folders.AddHead(0, g_str_Ads, g_str_Ads, true) ;
				pAcc->m_Folders.AddHead(0, g_str_Qtine, g_str_Qtine, true) ;
				pAcc->m_Folders.AddHead(0, g_str_Misc, g_str_Misc, true) ;

				AssertDir(pAcc->m_Mboxdir, 0755) ;
				slog.Out("HAVE user/mbox of %s - %s\n", *user, *mbox) ;

				m_Accounts.Insert(user, pAcc) ;
				pIsamFile = new hdbIsamfile() ;
				pIsamFile->Init(theApp->m_ADP, "mbox", pAcc->m_Mboxdir, 256, 256, 4) ;
				pIsamFile->Open() ;
			}
		}
	}

	/*
	**	Mbox Deliveries
	*/

	m_pClassDlvr = new hdbClass(theApp->m_ADP, HDB_CLASS_DESIG_CFG) ;
	title = "Dlvr" ;
	if (rc == E_OK)	rc = m_pClassDlvr->InitStart(title) ;
	if (rc == E_OK)	rc = m_pClassDlvr->InitMember("LUID", "uint16",	1, 1) ;		//	Delivered to mbox id
	if (rc == E_OK)	rc = m_pClassDlvr->InitMember("pop3", "bool",	1, 1) ;		//	Deleted by a POP3 client
	if (rc == E_OK)	rc = m_pClassDlvr->InitMember("read", "bool",	1, 1) ;		//	Viewed in webmail
	if (rc == E_OK)	rc = m_pClassDlvr->InitDone() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterDataClass(m_pClassDlvr) ;
	m_pCacheDlvr = new hdbObjCache(theApp->m_ADP) ;
	if (rc == E_OK)	rc = m_pCacheDlvr->InitStart(m_pClassDlvr, title, m_EpisData) ;
	//if (rc == E_OK)	rc = m_pCacheDlvr->InitMbrIndex("LUID", false) ;
	if (rc == E_OK)	rc = m_pCacheDlvr->InitDone() ;
	if (rc == E_OK)	rc = m_pCacheDlvr->Open() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterObjRepos(m_pCacheDlvr) ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init cache 'Dlvr'\n") ; return rc ; }

	/*
	**	Correspondents
	*/

	m_pClassCorres = new hdbClass(theApp->m_ADP, HDB_CLASS_DESIG_CFG) ;
	title = "corres" ;

	if (rc == E_OK)	rc = m_pClassCorres->InitStart(title) ;
	if (rc == E_OK)	rc = m_pClassCorres->InitMember("domain", "domain", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassCorres->InitMember("emaddr", "emaddr", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassCorres->InitMember("rname",  "string", 0, 1) ;
	if (rc == E_OK)	rc = m_pClassCorres->InitDone() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterDataClass(m_pClassCorres) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init class 'local'\n") ; return rc ; }

	m_pCacheCorres = new hdbObjCache(theApp->m_ADP) ;

	if (rc == E_OK)	rc = m_pCacheCorres->InitStart(m_pClassCorres, title, m_EpisData) ;
	if (rc == E_OK)	rc = m_pCacheCorres->InitMbrIndex("emaddr", true) ;
	if (rc == E_OK)	rc = m_pCacheCorres->InitDone() ;
	if (rc == E_OK)	rc = m_pCacheCorres->Open() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterObjRepos(m_pCacheCorres) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init cache 'local'\n") ; return rc ; }

	/*
	**	Forwards
	*/

	m_pClassForward = new hdbClass(theApp->m_ADP, HDB_CLASS_DESIG_CFG) ;
	title = "forward" ;

	if (rc == E_OK)	rc = m_pClassForward->InitStart("forward") ;
	if (rc == E_OK)	rc = m_pClassForward->InitMember("emaddr", "emaddr", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassForward->InitMember("forward", "emaddr", 1, 1) ;
	if (rc == E_OK)	rc = m_pClassForward->InitDone() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterDataClass(m_pClassForward) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init class 'filters'\n") ; return rc ; }

	m_pCacheForward = new hdbObjCache(theApp->m_ADP) ;

	if (rc == E_OK)	rc = m_pCacheForward->InitStart(m_pClassForward, "forward", m_EpisData) ;
	if (rc == E_OK)	rc = m_pCacheForward->InitMbrIndex("emaddr", true) ;
	if (rc == E_OK)	rc = m_pCacheForward->InitDone() ;
	if (rc == E_OK)	rc = m_pCacheForward->Open() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterObjRepos(m_pCacheForward) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init cache 'forward'\n") ; return rc ; }

	for (n = 1 ; n <= m_pCacheForward->Count() ; n++)
	{
		m_pCacheForward->Fetchval(atom, 0, n) ;	ema = atom.Str() ;
		m_pCacheForward->Fetchval(atom, 1, n) ;

		m_Locaddr2Frwd.Insert(ema, atom.Str()) ;
	}

	//	Address Summary
	slog.Log(_fn, "Have now %d domains, %d local %d mailboxes and %d allowed sender\n",
		m_LocalDomains.Count(), m_LocAddrs.Count(), m_Locaddr2Mbox.Count(), m_Locaddr2Orig.Count()) ;

	/*
	**	Email Messages. Stores as POP3 entries in m_dcronPOP3 as well as entries in the m_pCacheEmsg
	*/

	//	Init and Open m_pCronCenter
	m_pCronCenter = new hdbBinRepos(theApp->m_ADP) ;
	rc = m_pCronCenter->Init("MsgsCenter", m_EpisData) ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init email POP3 datacron\n") ; return rc ; }
	slog.Log(_fn, "Initialized email POP3 datacron\n") ;
	m_pCronCenter->Integ(slog) ;

	rc = m_pCronCenter->Open() ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not open email POP3 datacron\n") ; return rc ; }

	m_pClassEmsg = new hdbClass(theApp->m_ADP, HDB_CLASS_DESIG_CFG) ;

	if (rc == E_OK)	rc = m_pClassEmsg->InitStart("message") ;
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("dateID",		"xdate",  1, 1) ;	//	Exact date serves as ID
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("sdomain",	"domain", 1, 1) ;	//	Sender's domain (as string).
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("realname",	"string", 1, 1) ;	//	Realname of sender.
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("subject",	"string", 0, 1) ;	//	Message subject
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("from",		"emaddr", 1, 1) ;	//	Sender email address
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("rcpt",		"emaddr", 1, 0) ;	//	Recipients (all accepted by server)
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("attaches",	"bool",   0, 1) ;	//	Attachments (0 to many)
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("recipt",		"bool",   0, 1) ;	//	Recipt required
	//if (rc == E_OK)	rc = m_pClassEmsg->InitMember("bodyHTML",	"uint32", 0, 1) ;	//	Body text in HTML
	//if (rc == E_OK)	rc = m_pClassEmsg->InitMember("bodyTEXT",	"uint32", 0, 1) ;	//	Body text in HTML
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("bodyPOP3",	"uint32", 1, 1) ;	//	POP3 format of message
	if (rc == E_OK)	rc = m_pClassEmsg->InitMember("Delivery",	"uint32", 1, 1) ;	//	POP3 format of message
	if (rc == E_OK)	rc = m_pClassEmsg->InitDone() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterDataClass(m_pClassEmsg) ;

	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init class 'message'\n") ; return rc ; }

	m_pCacheEmsg = new hdbObjCache(theApp->m_ADP) ;

	//	Init the email message cache
	title = "Messages" ;
	if (rc == E_OK)	rc = m_pCacheEmsg->InitStart(m_pClassEmsg, title, m_EpisData) ;
	if (rc == E_OK)	rc = m_pCacheEmsg->InitMbrIndex("dateID", true) ;
	//if (rc == E_OK)	rc = g_pCacheEmsg.InitMbrStore("attaches", "Attachments", false) ;
	//if (rc == E_OK)	rc = g_pCacheEmsg.InitMbrStore("bodyHTML", "MsgsBODY", false) ;
	//if (rc == E_OK)	rc = g_pCacheEmsg.InitMbrStore("bodyTEXT", "MsgsBODY", false) ;
	//if (rc == E_OK)	rc = g_pCacheEmsg.InitMbrStore("bodyPOP3", "MsgsPOP3", false) ;
	if (rc == E_OK)	rc = m_pCacheEmsg->InitDone() ;
	if (rc == E_OK)	rc = m_pCacheEmsg->Open() ;
	if (rc == E_OK)	rc = theApp->m_ADP.RegisterObjRepos(m_pCacheEmsg) ;
	if (rc != E_OK)
		{ slog.Log(_fn, "Could not init email massages object cache\n") ; return rc ; }
	slog.Log(_fn, "Initialized message object cache\n") ;

	/*
	**	For each mailbox, set up repositories for messages and attachments
	*/

	for (n = 0 ; rc == E_OK && n < m_Accounts.Count() ; n++)
	{
		pAcc = m_Accounts.GetObj(n) ;

		mboxdir = m_MailBox + "/" + mbox ;

		//	Do mailbox message
		tmpStr = mbox + "-msg" ;
		slog.Log(_fn, "Initializing mailbox message repos %s in dir %s\n", *tmpStr, *mboxdir) ;

		//	Do mailbox attach
		tmpStr = mbox + "-attach" ;
		slog.Log(_fn, "Initializing mailbox attach repos %s in dir %s\n", *tmpStr, *mboxdir) ;
	}

	/*
	**	POPULATE FOLDERS
	*/

	for (n = 0 ; n < m_pCacheEmsg->Count() ; n++)
	{
		m_pCacheEmsg->Fetchval(atom, 5, n) ;
	}

	/*
	**	Init Core and Formal-Message-ID ISAMs
	*/

	m_pIsamCenter = new hdbIsamfile() ;
	m_pIsamCenter->Init(theApp->m_ADP, "Core", m_EpisData, 256, 10, 1) ;
	m_pIsamCenter->Open() ;
	m_pIsamFormal = new hdbIsamfile() ;
	m_pIsamFormal->Init(theApp->m_ADP, "fmid", m_EpisData, 256, 10, 1) ;
	m_pIsamFormal->Open() ;

	//	Check project
	if (rc != E_OK)
		slog.Log(_fn, "DATABASE INIT FAILED case 1\n") ;
	else
	{
		rc = theApp->CheckProject() ;
		if (rc != E_OK)
			Fatal("%s. Project Integrity Test Failed\n", *_fn) ;
		slog.Out("Project Checked\n") ;

		theApp->SetupScripts() ;
		if (rc != E_OK)
			Fatal("%s. Standard Script Generation Failure\n", *_fn) ;
		slog.Out("Scripts Formulated\n") ;

		rc = theApp->IndexPages() ;
		if (rc != E_OK)
			Fatal("%s. Page Indexation Failure\n", *_fn) ;
		slog.Out("Pages Indexed\n") ;
	}

	if (rc == E_OK)
		slog.Log(_fn, "DATABASE INIT COMPLETE\n") ;
	else
		slog.Log(_fn, "DATABASE INIT FAILED case 2\n") ;
	return rc ;
}

hzEcode	Epistula::UpdateCorres	(uint32_t& corresId, const hzEmaddr& addr, const hzString& rname, hzChain& err)
{
	//	Check if the correspondent exists on the basis of address, if not add the address plus the real name if supplied. If the address exists but the real name is missing but is
	//	supplied here, update the central repository
	//
	//	domian, email, rname

	_hzfunc(__func__) ;

	hdbObject	objCorres ;		//	The email message database object
	hzAtom		atom ;			//	For data xfer
	//uint32_t	corresId ;		//	Correspondent id (as per central repos of correspondents)
	hzEcode		rc ;			//	Return code

	rc = objCorres.Init("epistulaCorres", m_pClassCorres) ;
	if (rc != E_OK)
	{
		//	Serious internal error
		err.Printf("%s: Could not init email message object\n", *_fn) ;
		return rc ;
	}

	atom = addr ;
	m_pCacheCorres->Identify(corresId, 1, atom) ;

	if (!corresId)
	{
		objCorres.SetValue(1, atom) ;
		atom = addr.GetDomain() ;
		objCorres.SetValue(0, atom) ;
		if (rname)
			{ atom = rname ; objCorres.SetValue(2, atom) ; }

		rc = m_pCacheCorres->Insert(corresId, objCorres) ;
		if (rc != E_OK)
			threadLog("%s: Could not insert correspondent err=%s\n", *_fn, Err2Txt(rc)) ;
	}
	else
	{
		//	The correspondent exists
		/*
		FIX!
		if (rname)
		{
			m_pCacheCorres->Fetchval(atom, 0, corresId) ;
			if (atom.IsNull())
			{
				m_pCacheCorres->Fetch(objCorres, corresId) ;
				objCorres.SetValue(2, atom) ;
				rc = m_pCacheCorres->Update(objCorres, corresId) ;
			}
		}
		*/
	}

	return corresId ;
}

hzEcode	Epistula::UpdateShortForm
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
	)
{
	//	Ensure a short form of the message is committed to the central repository
	//
	//	Arguments:	1)	objId		(Set by this function) The object id of the short form message (in m_pCacheEmsg)
	//				2)	datumId		(Set by this function) The datum id on the whole form message in the central binary repository
	//				3)	msgId		Formal message id
	//				4)	now			Date & time of message
	//				5)	fromAddr	The sender address
	//				6)	rname		Sender real name (if any)
	//				7)	subject		Message subject
	//				8)	msgBody		The whole form message
	//				9)	err			For error reporting

	_hzfunc(__func__) ;

	hzAtom		atom ;			//	For data xfer
	hdbObject	objEmsg ;		//	The email message database object
	hzString	formalNo ;		//	For writing the message datum id to the formal ISAM
	//uint32_t	objId ;			//	For user authentication
	int32_t		n ;				//	Loop counter
	hzEcode		rc ;			//	Return code from DNS lookup functions
	char		buf[12] ;		//	For creating the formalNo

	//	Init the message object
	err.Printf("%s: Initializing message object\n", *_fn) ;

	rc = objEmsg.Init("epistulaCentral", m_pClassEmsg) ;
	if (rc != E_OK)
	{
		//	Serious internal error
		err.Printf("%s: Could not init email message object\n", *_fn) ;
		return rc ;
	}

	//	Commit the IMF whole form message to the central binary repository
	rc = m_pCronCenter->Insert(datumId, msgBody) ;
	if (rc != E_OK)
		err.Printf("Could not commit message to POP3 datacron\n") ;
	else
		err.Printf("Committed message %u to POP3 datacron\n", datumId) ;

	//	Now that we have the IMF datumId we can insert the short form message into g_pCacheEmsg
	if (rc == E_OK)	{ atom = now ;					rc = objEmsg.SetValue(0, atom) ; }
	if (rc == E_OK)	{ atom = fromAddr.GetDomain() ;	rc = objEmsg.SetValue(1, atom) ; }
	if (rc == E_OK)	{ atom = rname ;				rc = objEmsg.SetValue(2, atom) ; }
	if (rc == E_OK)	{ atom = subject ;				rc = objEmsg.SetValue(3, atom) ; }
	if (rc == E_OK)	{ atom = fromAddr ;				rc = objEmsg.SetValue(4, atom) ; }

	threadLog("%s: case 1 rcpt count is %d\n", *_fn, rcpts.Count()) ;
	if (rc == E_OK)
	{
		for (n = 0 ; n < rcpts.Count() ; n++)
		{
			atom = rcpts[n] ;
			threadLog("%s: rcpt is %s\n", *_fn, *atom.Str()) ;
			rc = objEmsg.SetValue(5, atom) ;
			break ;
		}
	}
	threadLog("%s: case 2\n", *_fn) ;

	if (rc == E_OK)	{ atom = datumId ; rc = objEmsg.SetValue(9, atom) ; }
	threadLog("%s: case 1\n", *_fn) ;

	if (rc == E_OK)
		err.Printf("%s: Set values to %016lX sender %s sdom %d real %s subject %s and %d byte\n",
			*_fn, now.AsVal(), *fromAddr, fromAddr.GetDomain(), *rname, *subject, msgBody.Size()) ;
	else
	{
	threadLog("%s: case 3\n", *_fn) ;
		err.Printf("%s: Could not populate message object\n", *_fn) ;
		return rc ;
	}
	threadLog("%s: case 4\n", *_fn) ;

	rc = m_pCacheEmsg->Insert(objId, objEmsg) ;
	threadLog("%s: case 5\n", *_fn) ;

	if (objId != datumId)
		err.Printf("Short and Whole form IDs Out of Sync - SF %u WF %u\n", objId, datumId) ;
	threadLog("%s: case 6\n", *_fn) ;

	//	Final step - record the message in the formal ISAM

	sprintf(buf, "%08x", datumId) ;
    formalNo = buf ;
    rc = m_pIsamFormal->Insert(msgId, formalNo) ;
	threadLog("%s: case 7\n", *_fn) ;
    if (rc != E_OK)
        { err << m_pIsamCenter->m_Error ; return rc ; }
	threadLog("%s: case 8\n", *_fn) ;

	return rc ;
}
