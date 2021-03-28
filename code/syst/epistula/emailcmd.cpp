//	File:	emailcmd.cpp
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
//	Description:	Main module for Epistula Command Line Mailer. Following options are available:-
//
//					1)	-t	Generate a mail item by taking data from stdin.
//					2)	-m	Mass email.
//					3)	start
//					4)	stop
//
//	Author:			Russell Ballard
//	Organization:	HadronZoo
//	Date Created:	2002-12-12
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

#include <fstream>
#include <iostream>

using namespace std ;

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "hzChars.h"
#include "hzChain.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzProcess.h"

#include "epistula.h"

/*
**	Variables
*/

bool	_hzGlobal_XM = false ;
bool	_hzGlobal_MT = false ;

hzProcess	proc ;					//	Need this for HadronZoo
hzLogger	elog ;					//	The log channel

hzVect<hzString>	g_Filelist ;	//	List of files

hzEmaddr	g_From ;				//	Sender
hzString	g_Username ;			//	Username for epistula
hzString	g_Password ;			//	Password for epistula
hzString	g_Realname ;			//	Username of sender
hzString	g_Subject ;				//	Username of sender
bool		s_bPeriod ;				//	True if lone period is not to be used as msg end
char		buf	[2048] ;			//	For reading stdin

extern	hzString	_hz_Hostname ;		//	Name of this server
extern	int32_t		g_nPortSMTP ;		//	SMTP port
extern	char		g_cvHostIP	[16] ;	//	IP of this server

/*
**	Prototypes
*/

/*
 * Function:	MassEmail
 *
 * Argepents:	1)	File containing list of emails
 * 				2)	File containing text body
 * 				3)	Sender's email address (must be local)
 * 				4)	Subject
 */

#if 0
int32_t	MassEmail	(const char * cpCfgfile)
{
	static	int32_t	anSessionId = 0 ;

	struct sockaddr_in	CliAddr ;	//	Address of real client
	struct sockaddr_in	SNFAddr ;	//	Address of (this) sniffer
	struct sockaddr_in	SvrAddr ;	//	Address of real server
	struct hostent *	pHost ;		//	Hostname of real server

	pthread_t	tid ;
	ifstream	is ;				//	Stream for file
	hzString	Username ;			//	Username of sender
	epResponse	sig ;				//	Return code of GetAddressInfo()

	char*	i ;
	char*	j ;
	int32_t	CliSock ;				//	Client socket
	uint32_t	cLen ;					//	Length of client socket
	int32_t	SvrSock ;				//	Server socket
	int32_t	nSize ;
	int32_t	nBytes ;
	int32_t	nLen ;					//	Length of server socket
	int32_t	nArg ;					//	Arg counter
	int32_t	nTimeElapsed ;
	int32_t	nNoBytes = 0 ;
	int32_t	nResult ;

	/*
	**	Initialize
	*/

	if (!ReadMEConfig(cpCfgfile))
		return 0 ;

	/*
	**	Check sender is local
	*/

	sig = GetUserFromAddress(Username, g_Sender) ;

	if (sig == EPRES_COMM_ERROR)
   	{
		elog.Out("421 Internal fault: CDB error during recipient validation\r\n") ;
		return false ;
   	}

	if (sig == EPRES_ADDR_NOEXIST)
	{
		elog.Out("massemail: Sender <%s> does not exist\n", *g_Sender) ;
		return false ;
	}

	if (sig == EPRES_ADDR_ALIEN)
	{
		elog.Out("massemail: Sender <%s> is not local\n", *g_Sender) ;
		return false ;
	}

	if (!Username.c_Str())
	{
		elog.Out("massemail: Sender <%s> is not permitted to send (no username)\n", *g_Sender) ;
		return 0 ;
	}

	/*
	**	Read in message body
	*/

	is.open(g_BodyFile.c_Str()) ;
	if (is.fail())
	{
		elog.Out("massemail: Could not open message body file (%s)\n", g_BodyFile.c_Str()) ;
		return 0 ;
	}

	is.seekg(0, ios::end) ;
	nSize = is.tellg() ;
	is.seekg(0) ;
	elog.Out("body file is %d bytes\n", nSize) ;

	if (!nSize)
	{
		cout << "massemail: Message body file is empty you asshole\n" ;
		return 0 ;
	}

	if (!(g_cpBody = (char *) Allocate(nSize + 1)))
	{
		cout << "massemail: Could not allocate memory for message body\n" ;
		return 0 ;
	}

	is.read(g_cpBody, nSize) ;
	nBytes = is.gcount() ;
	if (nBytes != nSize)
		cout << "massemail: Problem reading body file (" << nBytes << " of " << nSize << " bytes)\n" ;
	else
		cout << "message body imported\n" ;
	is.close() ;
	elog.Out("Body from file is %s\n", g_cpBody) ;

	PrepareMailHeader(g_zBody) ;
	g_zBody += g_cpBody ;

	/*
	**	Open address file
	*/

	is.open(g_RecipientFile.c_Str()) ;
	if (is.fail())
	{
		elog.Out("massemail: Could not open recipient file (%s)\n", g_RecipientFile.c_Str()) ;
		return 0 ;
	}

	elog.Out("Mass email in progress ...\n") ;

	/*
	**	Send the emails
	*/

	SendEmails(is) ;
	is.close() ;
	return 0 ;
}
#endif

int32_t	HandleStdin	(void)
{
	//	Handle the -t (bTerm is false) and -t -i options (bTerm is true)

	_hzfunc(__func__) ;

	hzEmail		Item ;				//	Mail item
	hzChain		Body ;				//	General MIME headers
	hzEmaddr	To ;				//	Main recipient
	hzEmaddr	rcpt ;				//	Other recipients

	bool		bInit = false ;		//	True when all the header fields entered
	bool		bDone = false ;		//	True when all data is entered
	bool		bAnnounce = false ;	//	No announce name set
	char*		i ;
	hzEcode		rc ;

	for (; !bDone ;)
	{
		if (!fgets(buf, 2048, stdin))
		{
			elog.Out("fgets got nothing!\n") ;
			bDone = true ;
			break ;
		}

		StripCRNL(buf) ;
		elog.Out("fgets -> %s\n", buf) ;

		if (!bInit)
		{
			//	Not initialized until there is a sender, one or more recipients and ...

			if (buf[0] == 0)
			{
				//	The blank line ends the headers and starts the body

				if (!g_From)
					{ elog.Out("No sender\n") ; return 100 ; }

				if (!Item.CountRecipientsStd())
					{ elog.Out("No recipients\n") ; bDone = true ; }

				bInit = true ;
				continue ;
			}

			if (!strncasecmp(buf, "from:", 5))
			{
				//	Set sender first but if we already have a sender address, treat this as the realname part
				if (g_From)
					g_Realname = buf ;
				else
				{
					for (i = buf + 5 ; *i && *i <= CHAR_SPACE ; i++) ;
					g_From = i ;
					if (!g_From)
						return 101 ;
					if (!g_Realname)
						g_Realname = g_From.GetDomain() ;
					Item.SetSender(*g_From, *g_Realname) ;
				}
				continue ;
			}

			if (!strncasecmp(buf, "to:", 3))
			{
				if (To)
					return 102 ;
				for (i = buf + 3 ; *i && *i <= CHAR_SPACE ; i++) ;
				To = i ;
				if (!To)
					return 103 ;

				Item.AddRecipient(To) ;
				continue ;
			}

			if (!strncasecmp(buf, "cc:", 3))
			{
				for (i = buf + 3 ; *i && *i <= CHAR_SPACE ; i++) ;
				rcpt = i ;
				if (!rcpt)
					return 104 ;

				Item.AddRecipientCC(rcpt) ;
				continue ;
			}

			if (!strncasecmp(buf, "bcc:", 4))
			{
				for (i = buf + 4 ; *i && *i <= CHAR_SPACE ; i++) ;
				rcpt = i ;
				if (!rcpt)
					return 105 ;
				Item.AddRecipientBCC(rcpt) ;
				continue ;
			}

			if (!strncasecmp(buf, "subject:", 8))
			{
				for (i = buf + 8 ; *i && *i <= CHAR_SPACE ; i++) ;
				g_Subject = i ;
				if (!g_Subject)
					return 106 ;
				Item.SetSubject(g_Subject) ;
				continue ;
			}

			if (buf[0] == 'X')
				continue ;

			//	If we have bInit=false and a line starting with other
			//	than from:, to: or subject: then this is invalid

			elog.Out("Epistula -t session terminated: Bad format [%s]\n", buf) ;
			return 107 ;
		}

		elog.Out("Got data of [%s]\n", buf) ;

		//	Now in actual message part

		if (!s_bPeriod && (buf[0] == CHAR_PERIOD && buf[1] == 0))
			bDone = true ;
		else
		{
			//	Add data to message

			if (buf[0])
			{
				Body << buf ;
				Body << "\r\n" ;
			}
		}
	}

	if (!bDone)
	{
		elog.Out("Item not done, returning false\n") ;
		return 107 ;
	}

	Item.AddBody(Body) ;

	rc = Item.Compose() ;
	if (rc != E_OK)
		{ elog.Out("Composition failed\n") ; return 108 ; }

	rc = Item.SendSmtp("127.0.0.1", "superuser", "dragonfly") ;

	if (rc != E_OK)
	{
		elog.Out("Delivery failed\n") ;
		return 109 ;
	}

	elog.Out("Delivery OK\n") ;
	return 0 ;
}

hzEcode	HandleFile	(void)
{
	//	Handle the -x

	_hzfunc(__func__) ;

	FSTAT		fs ;				//	To check file
	ifstream	is ;				//	To read file
	hzEmail		Item ;				//	Mail item
	hzChain		Body ;				//	General MIME headers
	hzString	pathname ;			//	Pathname of message file
	hzString	attfile ;			//	Pathname of message file
	hzEmaddr	To ;				//	Main recipient
	hzEmaddr	rcpt ;				//	Other recipients
	char*		i ;					//	For processing header lines
	int32_t		nLine = 1 ;			//	Line indicator
	int32_t		nFile = 1 ;			//	Attach file iterator
	bool		bInit = false ;		//	True when all the header fields entered
	bool		bDone = false ;		//	True when all data is entered
	bool		bAnnounce = false ;	//	No announce name set
	hzMimetype	mtype ;
	hzEcode		rc = E_OK ;

	pathname = g_Filelist[0] ;

	if (stat(*pathname, &fs))
	{
		cout << "No such file as [" << pathname << "]\n" ; 
		return E_NOTFOUND ;
	}

	is.open(*pathname) ;
	if (is.fail())
	{
		cout << "Cannot open file [" << pathname << "]\n" ;
		return E_OPENFAIL ;
	}

	for (nLine = 1 ; rc == E_OK && !bDone ; nLine++)
	{
		is.getline(buf, 2048) ;
		if (is.gcount() == 2048)
			{ cout << "Line exceeds 2048 bytes - aborting\n" ; return E_FORMAT ; }
		if (!is.gcount())
			break ;

		StripCRNL(buf) ;

		if (!bInit)
		{
			//	Not initialized until there is a sender, one or more recipients and ...

			if (buf[0] == 0)
			{
				//	The blank line ends the headers and starts the body

				if (!g_From)
					{ cout << "No sender\n" ; return E_NOINIT ; }

				if (!Item.CountRecipientsStd())
					{ cout << "No recipients\n" ; bDone = true ; }

				bInit = true ;
				continue ;
			}

			if (!strncasecmp(buf, "from:", 5))
			{
				//	Set sender first but if we already have a sender address, treat this as the realname part
				if (g_From)
					g_Realname = buf ;
				else
				{
					for (i = buf + 5 ; *i && *i <= CHAR_SPACE ; i++) ;
					g_From = i ;
					if (!g_From)
						return E_NOINIT ;
					if (!g_Realname)
						g_Realname = g_From.GetDomain() ;
					Item.SetSender(*g_From, *g_Realname) ;
				}
				continue ;
			}

			if (!strncasecmp(buf, "to:", 3))
			{
				if (To)
					return E_DUPLICATE ;
				for (i = buf + 3 ; *i && *i <= CHAR_SPACE ; i++) ;
				To = i ;
				if (!To)
					rc = E_NODATA ;
				else
					Item.AddRecipient(To) ;
				continue ;
			}

			if (!strncasecmp(buf, "cc:", 3))
			{
				for (i = buf + 3 ; *i && *i <= CHAR_SPACE ; i++) ;
				rcpt = i ;
				if (!rcpt)
					rc = E_NODATA ;
				else
					Item.AddRecipientCC(rcpt) ;
				continue ;
			}

			if (!strncasecmp(buf, "bcc:", 4))
			{
				for (i = buf + 4 ; *i && *i <= CHAR_SPACE ; i++) ;
				rcpt = i ;
				if (rcpt)
					Item.AddRecipientBCC(rcpt) ;
				else
					rc = E_NODATA ;
				continue ;
			}

			if (!strncasecmp(buf, "subject:", 8))
			{
				for (i = buf + 8 ; *i && *i <= CHAR_SPACE ; i++) ;
				g_Subject = i ;
				if (!g_Subject)
					rc = E_NODATA ;
				else
					Item.SetSubject(g_Subject) ;
				continue ;
			}

			if (!strncasecmp(buf, "username:", 9))
			{
				//for (i = buf + 9 ; *i && *i <= CHAR_SPACE ; i++) ;
				//g_Username = i ;
				continue ;
			}

			if (!strncasecmp(buf, "password:", 9))
			{
				//for (i = buf + 9 ; *i && *i <= CHAR_SPACE ; i++) ;
				//g_Password = i ;
				continue ;
			}

			//	If we have bInit=false and a line starting with other
			//	than from:, to: or subject: then this is invalid

			cout << "Epistula -x session terminated: Bad format. Allowed hdrs are From|To|Cc|Subject\n" ;
			return E_FORMAT ;
		}

		cout << "Got data of [" << buf << "]\n" ;

		//	Now in actual message part

		if (!s_bPeriod && (buf[0] == CHAR_PERIOD && buf[1] == 0))
			bDone = true ;
		else
		{
			//	Add data to message

			if (buf[0])
				Body << buf ;
			Body << "\r\n" ;
		}
	}

	is.close() ;

	if (!bDone)
		{ cout << "Item not done, returning false\n" ; return rc ; }

	Item.AddBody(Body) ;

	for (nFile = 1 ; nFile < g_Filelist.Count() ; nFile++)
	{
		attfile = g_Filelist[nFile] ;

		Item.AddAttachment(attfile, HMTYPE_APP_ZIP) ;
	}

	rc = Item.Compose() ;
	if (rc != E_OK)
		{ cout << "Composition failed\n" ; return rc ; }

	if (*g_Username && *g_Password)
		rc = Item.SendSmtp("127.0.0.1", *g_Username, *g_Password) ;
	else if (g_Username)
		{ cout << "No password supplied for user " << g_Username << "\n" ; rc = E_NOINIT ; }
	else if (g_Password)
		{ cout << "No username supplied\n" ; rc = E_NOINIT ; }
	else
		rc = Item.SendSmtp("127.0.0.1", "epistula.superuser", "dragonfly") ;

	if (rc != E_OK)
	{
		elog.Out("Delivery failed\n") ;
		return rc ;
	}

	elog.Out("Delivery OK\n") ;
	return E_OK ;
}

hzEcode	AdminCommand	(const char* domain)
{
	//	Issue a command to add or disable a user, mailbox or email address
}

/*
** Function:	Main
*/

int32_t	main	(int argc, char ** argv)
{
	hzString	S ;					//	The value of the arg
	int32_t	nArg = 0 ;			//	Argument iterator
	int32_t	sys_rc = 0 ;		//	Return code (of this fn)
	hzEcode	rc ;

	/*
	**	Check args and set flags
	*/

	//	Get current working directory and create logfile in there
	GetCurrDir(S) ;
	if (!S)
		return -100 ;
	sprintf(buf, "/usr/epistula/logs/epismail_%05d", getpid()) ;
	elog.OpenFile(buf, LOGROTATE_NEVER) ;
	elog.Out("User %d (effective %d) calls epismail as [%s]\n", getuid(), geteuid(), argv[0]) ;
	elog.Out("CWD is %s\n", *S) ;

	if (argc < 2)
		goto error ;

	elog.Out("arg[1] %s\n", argv[1]) ;

	if (!memcmp(argv[1], "-x", 2))
	{
		for (nArg = 2 ; nArg < argc ; nArg++)
		{
			elog.Out("arg[%d] %s\n", nArg, argv[nArg]) ;

			if		(!memcmp(argv[nArg], "user=", 5))	S=0;//g_Username = argv[nArg] + 5 ; 
			else if (!memcmp(argv[nArg], "pass=", 5))	S=0;//g_Password = argv[nArg] + 5 ; 
			else if (!memcmp(argv[nArg], "from=", 5))	g_From = argv[nArg] + 5 ;
			else if (!memcmp(argv[nArg], "subj=", 5))	g_Subject = argv[nArg] + 5 ;
			else if (!memcmp(argv[nArg], "subj=", 5))	g_Subject = argv[nArg] + 5 ;
			else
			{
				S = argv[nArg] ;
				g_Filelist.Add(S) ;
			}
		}

		if (!g_Filelist.Count())
			{ elog << "No parameter file\n" ; goto error ; }
		rc = HandleFile() ;
		if (rc != E_OK)
			{ sys_rc = 2 ; goto error ; }
	}
	else if (!memcmp(argv[1], "-t", 2))
	{
		if (argc > 2 && !memcmp(argv[2], "-i", 2))
			s_bPeriod = true ;
		sys_rc = HandleStdin() ;
	}
	else if (!memcmp(argv[1], "-m", 2))
	{
		elog.Out("Mass emailer not implimented\n") ;
	}
	else
	{
		elog << "epismail: Usage -[x|t|m] ...\n" ;
		sys_rc = 1 ;
		goto error ;
	}

result:
	return sys_rc ;

error:
	elog << "Usage: epismail -c start,stop\n" ;
	elog << "Usage: epismail -s email_address,username\n" ;
	elog << "Usage: epismail -t\n" ;
	elog << "Usage: epismail -m recipient_list_file message_body_file\n" ;
	return sys_rc ;
}
