//
//	File:	emailimp.cpp
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

//	Epistula email importer.
//
//	This program performs a wholesale import of email messages into a virgin Epistula environment. It should only be run once, prior to switching to Epistula from another mail server.
//
//	Before the import proceeds, the local domains must be set. Without this, the importer would not know what was an incoming or outgoing message. The messages found are assumed to
//	be genuine so recipient names and addresses derived from the messages are assumed to be legal addresses. The messages must be in Internet Message Format, and be supplied either
//	as an input file of concatenated email messages or as a directory with messages in individual files.
//
//	The headers of message id, ....are extracted as follows:-
//
//		- Message ID
//
//  Legal Notice: This file is part of the HadronZoo::Epistula program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
//
//  Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//  HadronZoo::Dissemino is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or any later version.
//
//  The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//  as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//  HadronZoo::Dissemino is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with HadronZoo::Dissemino. If not, see http://www.gnu.org/licenses.
//

#include <iostream>
#include <fstream>

using namespace std ;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>

#include "hzBasedefs.h"
#include "hzChars.h"
#include "hzChain.h"
#include "hzString.h"
#include "hzDirectory.h"
#include "hzDissemino.h"
#include "hzTextproc.h"
#include "hzMailer.h"
#include "hzProcess.h"

#include "epistula.h"

/*
**	Definitions
*/

class	_emsgInfo
{
	hzString	m_Id ;		//	Original email message id
	uint32_t	m_Posn ;	//	Position of entry in POP3 file
} ;

/*
**	Variables
*/

global	bool	_hzGlobal_XM = true ;		//	Not using operator new override
global	bool	_hzGlobal_MT = true ;		//	Using multiple threads

hzProcess	_proc ;							//	HadronZoo thread anchor

hzMapM	<hzXDate,uint32_t>	s_MsgPosns ;	//	Message ids by date

hzArray	<hzString>	s_Directories ;			//	Directories (of email messages files hopefully)
hzArray	<hzString>	s_POP3_files ;			//	All POP3 files to import

global	hzLogger	slog ;
extern	hdsApp*		theApp ;
extern	Epistula*	theEP ;					//	Epistula data instance

/*
**	Functions
*/

hzEcode	ProcessMsg	(hzChain& Z)
{
	_hzfunc(__func__) ;

	hzList<hzEmaddr>::Iter	rcpt ;		//	For iterating the recipients
	hzArray<hzEmaddr>		arrRcpt ;	//	Expected array rather than list

	hzEmail		msg ;			//	Message construct
	hzChain		X ;				//	For building ISAM key
	chIter		xi ;			//	For checking stray chars
	hzString	key ;			//	ISAM key
	hzString	obj ;			//	ISAM object
	uint32_t	objId ;			//	Set by insert of short form message
	uint32_t	datumId ;		//	Set by Insert of binary IMF
	uint32_t	corresId ;		//	Set by Insert of correspondent
	hzEcode		rc ;			//	Return code
	bool		bExists ;

	static	int32_t	testNo = 0;

	++testNo ;

	//	Import the binary IMF into an actual message
	rc = msg.Import(Z) ;
	if (rc != E_OK)
	{
		slog.Out("%s: Could not Import Message. Err=%s\n", *_fn, Err2Txt(rc)) ;
		Z.Clear() ;
		return rc ;
	}

	//	Message loaded OK so we have message-id and can check if we have this already
	bExists = theEP->m_pIsamFormal->Exists(msg.m_Id) ;
	if (theEP->m_pIsamFormal->m_Cond != E_OK)
		slog.Out(theEP->m_pIsamFormal->m_Error) ;
	if (bExists)
		{ slog.Out("ALREADY HAVE [%s]\n", *msg.m_Id) ; return E_OK ; }

	/*
	**	Now contend with the short-form message
	*/

	rc = theEP->UpdateCorres(corresId, msg.m_AddrFrom, msg.m_RealFrom, X) ;
	if (rc != E_OK)
		slog.Out(X) ;
	X.Clear() ;

	for (rcpt = msg.GetRecipientsStd() ; rcpt.Valid() ; rcpt++)
	{
		arrRcpt.Add(rcpt.Element()) ;
	}

    rc = theEP->UpdateShortForm(objId, datumId, arrRcpt, msg.m_Id, msg.m_Date, msg.m_AddrFrom, msg.m_RealFrom, msg.m_Subject, Z, X) ;
	if (rc != E_OK)
		slog.Out(X) ;
	X.Clear() ;

	//	Place binary in central repository
	//	rc = theEP->m_pCronCenter->Insert(datumId, Z) ;
	//	Z.Clear() ;
	//	if (rc != E_OK)
	//		{ slog.Out("%s: Could not insert message in the central repository. Err=%s\n", *_fn, Err2Txt(rc)) ; return rc ; }
	//	slog.Out("%s: inserted datum id %d masId %s\n", *_fn, datumId, *msg.m_Id) ;

	//	With the datum id (internal message id), we can now store the formal message id and datum id
	/*
	X.Printf("%08x", datumId) ;
	obj = X ;
	rc = theEP->m_pIsamFormal->Insert(msg.m_Id, obj) ;
	if (rc != E_OK)
		{ slog.Out(theEP->m_pIsamCenter->m_Error) ; return rc ; }
	*/

	/*
	**	Now contend with the short-form message
	*/

	/*
	if (theEP->m_LocAddrs.Exists(msg.m_AddrFrom))
	{
		//	Sender is local so correspondent is the primary recipient
		sf.m_Local = msg.m_AddrFrom ;
		sf.m_CorrName = 0 ;
		sf.m_CorrAddr = msg.m_AddrTo ;
	}
	else
	{
		sf.m_Local = msg.m_AddrTo ;
		sf.m_CorrName = msg.m_RealFrom ;
		sf.m_CorrAddr = msg.m_AddrFrom ;
	}

	sf.m_Date = msg.m_Date ;
	sf.m_Subject = msg.m_Subject ;
	//sf.m_User = accname ;

	//	First create SF key
	X.AddByte(CHAR_AT) ;	X << *sf.m_CorrName ;
	X.AddByte(CHAR_SOH) ;	X << *sf.m_CorrAddr ;
	X.AddByte(CHAR_SOH) ;	X << sf.m_Date.Str() ;
	X.AddByte(CHAR_AT) ;
	key = X ;
	X.Clear() ;

	//	Then the SF object
	X.AddByte(CHAR_AT) ;	X << *sf.m_CorrName ;
	X.AddByte(CHAR_SOH) ;	X << *sf.m_CorrAddr ;
	X.AddByte(CHAR_SOH) ;	X << sf.m_Date.Str() ;
	X.AddByte(CHAR_SOH) ;	X << *sf.m_Local ;
	X.AddByte(CHAR_SOH) ;	X << *sf.m_Subject ;
	X.AddByte(CHAR_SOH) ;	X.Printf("%08x", datumId) ;
	X.AddByte(CHAR_AT) ;
	obj = X ;

	for (xi = X ; !xi.eof() ; xi++)
	{
		if (*xi == CHAR_NL)
			slog.Out("%s: Item [%s] NEWLINE\n", *_fn, *key) ;
		if (*xi == 0)
			slog.Out("%s: Item [%s] TERMCHAR\n", *_fn, *key) ;
	}
	X.Clear() ;
	*/

	//	Place SF message in central repository
	//	rc = theEP->m_pIsamCenter->Insert(key, obj) ;
	//	if (rc != E_OK)
	//		{ slog.Out(theEP->m_pIsamCenter->m_Error) ; return rc ; }
	//	slog.Out("%s: Insert item %d [%s] in core ISAM\n", *_fn, testNo, *key) ;

	//	Place SF in the central Inbox
	//	s_SFMsgs.Add(sf) ;

	return E_OK ;
}

hzEcode	ReadMsgFile	(const char* filename)
{
	//	Read a file of one message

	ifstream	is ;			//	Input stream
	hzChain		Z ;				//	Email item chain
	chIter		zi ;			//	Chain iterator
	char*		i ;				//	Line iterator
	uint32_t	nLine ;			//	Line number
	int32_t		col ;			//	Column
	hzEcode		rc ;			//	Return code
	char		buf [10240] ;	//	Read in buffer

	is.open(filename) ;
	if (is.fail())
		{ slog.Out("Cannot open in read mode, file: %s\n", *filename) ; return E_OPENFAIL ; }

	Z << is ;
	is.close() ;

	rc = ProcessMsg(Z) ;
	if (rc != E_OK)
		slog.Out("Could not insert POP3. Err=%s\n", Err2Txt(rc)) ;
	return rc ;

	/*
 	for (nLine = 1 ; rc == E_OK ; nLine++)
	{
		is.getline(buf, 10240) ;
		buf[is.gcount()] = 0 ;

		if (!is.gcount())
		{
			if (Z.Size())
			{
				//	Output POP3 'file'
				rc = ProcessMsg(Z) ;
				if (rc != E_OK)
					{ slog.Out("Could not insert POP3. Err=%s\n", Err2Txt(rc)) ; return false ; }
			}

			if (is.fail())
			{
				slog.Out("READ FAILURE\n") ;
				is.clear() ;
			}
			continue ;
		}

		for (col = 0, i = buf ; *i ; i++, col++)
		{
			if (*i == 'R' && !memcmp(i, "Return-Path: ", 13) && Z.Size())
			{
				//	Have reached the end of one email and the start of another. Process what we have got
				if (col)
					Z << "\r\n" ;

				//	Output POP3 'file'
				rc = ProcessMsg(Z) ;
				if (rc != E_OK)
					{ slog.Out("Could not insert POP3. Err=%s\n", Err2Txt(rc)) ; return false ; }
				Z.Clear() ;
			}

			Z.AddByte(*i) ;
		}
		Z.AddByte(CHAR_NL) ;
	}
	is.close() ;
	*/
}

hzEcode	ProcMsgDir	(const hzString& dir)
{
	_hzfunc(__func__) ;

	hzVect<hzDirent>	files ;	//	Files found in directory

	hzDirent	de ;			//	Directory entry
	hzString	filepath ;		//	Message Input file
	hzString	criteria ;		//	File criteria
	uint32_t	nIndex ;		//	Directory entry iterator
	hzEcode		rc ;			//	Return code

	criteria = "*.pop3" ;
	rc = ReadDir(files, dir, criteria) ;

	if (rc != E_OK)
	{
		slog.Out("%s. Failure while reading Dir %s with criteria %s (err=%s)\n", *_fn, *dir, *criteria, Err2Txt(rc)) ;
		return rc ;
	}

	slog.Out("%s. Reading Dir %s with crit %s finds %d files\n", *_fn, *dir, *criteria, files.Count()) ;

	for (nIndex = 0 ; rc == E_OK && nIndex < files.Count() ; nIndex++)
	{
		de = files[nIndex] ;
		slog.Out("\t- %s\n", *de.Name()) ;
	}

	for (nIndex = 0 ; rc == E_OK && nIndex < files.Count() ; nIndex++)
	{
		de = files[nIndex] ;
		filepath = dir + "/" + de.Name() ;

		rc = ReadMsgFile(filepath) ;
	}

	return rc ;
}

int		main	(int argc, char ** argv)
{
	//	hzSet	<hzString>	links ;		//	Set of unique links found

	_hzfunc("main_function") ;

	FSTAT		fs ;			//	File status
	hzString	directory ;		//	Input file argv[1]
	int32_t		nArg ;			//	Links iterator
	hzEcode		rc ;			//	Return code

	//	Setup interupts
	signal(SIGHUP,  CatchCtrlC) ;
	signal(SIGINT,  CatchCtrlC) ;
	signal(SIGQUIT, CatchCtrlC) ;
	signal(SIGILL,  CatchSegVio) ;
	signal(SIGTRAP, CatchSegVio) ;
	signal(SIGABRT, CatchSegVio) ;
	signal(SIGBUS,  CatchSegVio) ;
	signal(SIGFPE,  CatchSegVio) ;
	signal(SIGKILL, CatchSegVio) ;
	signal(SIGUSR1, CatchCtrlC) ;
	signal(SIGSEGV, CatchSegVio) ;
	signal(SIGUSR2, CatchCtrlC) ;
	signal(SIGPIPE, SIG_IGN) ;
	signal(SIGALRM, CatchCtrlC) ;
	signal(SIGTERM, CatchCtrlC) ;
	signal(SIGCHLD, CatchCtrlC) ;
	signal(SIGCONT, CatchCtrlC) ;
	signal(SIGSTOP, CatchCtrlC) ;

	//	Open logfile
	slog.OpenFile("epImp", LOGROTATE_NEVER) ;
	slog.Verbose(true) ;

	for (nArg = 1 ; nArg < argc ; nArg++)
	{
		if (!memcmp(argv[nArg], "-dir=", 5))
		{
			directory = argv[nArg] + 5 ;
			if (stat(*directory, &fs) == -1)
				{ slog.Out("Directory %s does not exist\n", *directory) ; return -1 ; }

			s_Directories.Add(directory) ;
		}
		else
		{
			cout << "Usage: Illegal argument (" << argv[nArg] << "\n" ;
			return -1 ;
		}
	}

	//	Gather up POP3 files
    rc = theApp->m_ADP.InitStandard("emailimp") ;
    if (rc != E_OK)
        { slog.Out("Could not init the HDS\n") ; return false ; }

	//	Init HDS
	theEP = Epistula::GetInstance() ;
	rc = theEP->InitLocations("/home/rballard/epistula/") ;
	if (rc != E_OK)
		{ slog.Out("Could not init locations\n") ; return false ; }

	rc = theEP->InitEpistulaClasses() ;
	if (rc != E_OK)
		{ slog.Out("Could not init the epistula classes\n") ; return false ; }

	/*
	**	First step is to read concatenated file to organize the messages by date
	*/

	for (nArg = 0 ; nArg < s_Directories.Count() ; nArg++)
	{ 
		ProcMsgDir(s_Directories[nArg]) ;
	}

	/*
	testFile = theEP->m_EpisData + "/tmpPOP3.idx" ;
	if (TestFile(testFile) == E_NOTFOUND)
	{
		//	Create temporary data cron (not yet ordered by date)
		rc = s_dcronTmpPOP3.Init("tmpPOP3", theEP->m_EpisData) ;
		if (rc != E_OK)
			{ slog.Log(_fn, "Could not init email messages datacron\n") ; return rc.Value() ; }
		slog.Log(_fn, "Initialized message datacron\n") ;

		rc = s_dcronTmpPOP3.Open() ;
		if (rc != E_OK)
			{ slog.Log(_fn, "Could not open email messages datacron\n") ; return rc.Value() ; }

		//	Open it
		is.open(filename) ;
		if (is.fail())
			{ slog.Out("Cannot open in read mode, file: %s\n", *filename) ; return false ; }

 		for (nLine = 1 ; rc == E_OK ; nLine++)
		{
			is.getline(buf, 10240) ;
			buf[is.gcount()] = 0 ;

			if (!is.gcount())
			{
				if (Z.Size())
				{
					nEmails++ ;
					slog.Out("Line %u: Processing final item %d of %d bytes - ", nLine, nEmails, Z.Size()) ;

					//	Output POP3 'file'
					rc = s_dcronTmpPOP3.Insert(datumId, Z) ;
					if (rc != E_OK)
						{ slog.Out("Could not insert POP3. Err=%s\n", Err2Txt(rc)) ; return false ; }
					slog.Out("inserted datum id %d\n", datumId) ;
					Z.Clear() ;
				}

				if (nTry++ == 10)
					break ;
				if (is.fail())
				{
					slog.Out("READ FAILURE\n") ;
					is.clear() ;
				}
				continue ;
			}

			nTry = 0 ;

			for (col = 0, i = buf ; *i ; i++, col++)
			{
				if (*i == 'R' && !memcmp(i, "Return-Path: ", 13) && Z.Size())
				{
					//	Have reached the end of one email and the start of another. Process what we have got
					nEmails++ ;
					slog.Out("Line %u: Processing item %d of %d bytes - ", nLine, nEmails, Z.Size()) ;
					if (col)
						Z << "\r\n" ;

					//	Output POP3 'file'
					rc = s_dcronTmpPOP3.Insert(datumId, Z) ;
					if (rc != E_OK)
						{ slog.Out("Could not insert POP3. Err=%s\n", Err2Txt(rc)) ; return false ; }
					slog.Out("inserted datum id %d\n", datumId) ;
					Z.Clear() ;
				}

				Z.AddByte(*i) ;
			}
			Z.AddByte(CHAR_NL) ;
		}
		is.close() ;

		slog.Out("Now have %d items\n", s_dcronTmpPOP3.Count()) ;

		for (n = 1 ; n <= s_dcronTmpPOP3.Count() ; n++)
		{
			rc = s_dcronTmpPOP3.Fetch(Z, n) ;
			slog.Out("Fetched record %d at %d bytes\n", n, Z.Size()) ;

			if (rc != E_OK)
			{
				slog.Out("Could not FETCH datum %d posn %d\n", n, posn) ;
				continue ;
			}
			rc = em.Import(Z) ;
			if (rc != E_OK)
				slog.Out("Error in import %s\n", Err2Txt(rc)) ;
			slog.Out("Imported record %d at %d bytes date=%s\n", n, Z.Size(), *em.m_Date.Str()) ;
			s_MsgPosns.Insert(em.m_Date, n) ;
		}

		slog.Out("Now have %d sorted items\n", s_MsgPosns.Count()) ;

		for (n = 0 ; n < s_MsgPosns.Count() ; n++)
		{
			posn = s_MsgPosns.GetObj(n) ;

			rc = s_dcronTmpPOP3.Fetch(Z, posn) ;
			if (rc == E_OK)
				slog.Out("Re-fetched record %d at %d bytes - ", posn, Z.Size()) ;
			else
			{
				slog.Out("Re-fetched failed %d at %d bytes\n", posn, Z.Size()) ;
				continue ;
			}

			rc = theEP->m_pCronPOP3->Insert(datumId, Z) ;
			slog.Out("inserted datum id %d\n", datumId) ;
		}
	}
	else
	{
 		//	Read file in and process
 		for (; rc == E_OK ;)
		{
			is.getline(buf, 1024) ;
			if (!is.gcount())
			{
				if (Z.Size())
				{
					nEmails++ ;
					slog.Out("Processing item %d of %d bytes\n", nEmails, Z.Size()) ;

					rc = em.Import(Z) ;
					if (rc != E_OK)
						slog.Out("Error in import %s\n", Err2Txt(rc)) ;
					Z.Clear() ;
				}
				break ;
			}

			for (col = 0, i = buf ; *i ; i++, col++)
			{
				if (*i == 'R' && !memcmp(i, "Return-Path:", 12) && Z.Size())
				{
					//	Have reached the end of one email and the start of another. Process what we have got
					nEmails++ ;
					slog.Out("Processing item %d of %d bytes\n", nEmails, Z.Size()) ;
					if (col)
						Z << "\r\n" ;
					rc = em.Import(Z) ;
					if (rc != E_OK)
						slog.Out("Error in import %s\n", Err2Txt(rc)) ;

					//	Output POP3 'file'
					slog.Out("Banking raW POP3\n") ;
					rc = theEP->m_pCronPOP3->Insert(datumId, Z) ;
					if (rc != E_OK)
						{ slog.Out("Could not insert POP3. Err=%s\n", Err2Txt(rc)) ; return false ; }
					Z.Clear() ;

					//	Commit message to repository
					if (rc == E_OK)		{ atom.Clear() ; atom = em.m_Id ; rc = objEmsg.SetValue(0, atom) ; }
					if (rc == E_OK)		{ atom.Clear() ; atom = em.m_Date ; rc = objEmsg.SetValue(1, atom) ; }
					if (rc == E_OK)		{ atom.Clear() ; atom = em.m_AddrSender ; rc = objEmsg.SetValue(2, atom) ; }

					if (rc == E_OK)
					{
						for (rI = em.GetRecipientsStd() ; rI.Valid() ; rI++)
						{
							rcpt = rI.Element() ;
							atom.Clear() ;
							atom = rcpt ;
							rc = objEmsg.SetValue(3, atom) ;
						}
					}

					if (rc == E_OK)
						{ atom.Clear() ; atom = em.m_Subject ; rc = objEmsg.SetValue(4, atom) ; }

					if (rc == E_OK)
					{
						slog.Out("Email body of %d bytes\n", em.m_Text.Size()) ;

						atom.SetValue(BASETYPE_TXTDOC, em.m_Text) ;
						rc = objEmsg.SetValue(5, atom) ;
					}

					if (rc != E_OK)
						{ slog.Out("Could not populate object\n") ; return false ; }

					rc = theEP->m_pCacheEmsg->Insert(objId, objEmsg) ;
					if (rc != E_OK)
						{ slog.Out("Could not insert object. Err=%s\n", Err2Txt(rc)) ; return false ; }
					objEmsg.Clear() ;
				}

				Z.AddByte(*i) ;
			}
			Z.AddByte(CHAR_NL) ;
		}
		is.close() ;
	}
	*/

	slog.Out("Process complete: %d emails processed\n", theEP->m_pCacheEmsg->Count()) ;
	return 0 ;
}
