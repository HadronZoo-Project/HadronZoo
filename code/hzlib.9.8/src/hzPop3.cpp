//
//	File:	hzPop3.cpp
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
//	Method:			Contacts a POP3 server and retrieves emails.
//

#include <iostream>
#include <fstream>

#include <sys/stat.h>

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <pthread.h>

#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzIpServer.h"
#include "hzTcpClient.h"
#include "hzProcess.h"
#include "hzCodec.h"
#include "hzMailer.h"

using namespace std ;

/*
**	Functions
*/

hzEcode	hzPop3Acc::Init	(hzString Server, hzString Username, hzString Password, hzString Repos)
{
	//	Initialize a POP3 Account that our application will access as client.
	//
	//	Arguments:	1)	Server		Server/Hostname to connect to.
	//				2)	Username	Username for the account.
	//				3)	Password	Password for the account.
	//				4)	Repos		Pathname for the email repository (of downloaded messages and headers)
	//
	//	Returns:	E_INITDUP	If this POP3 account manager has already been initialized
	//				E_ARGUMENT	If the server, username, password or the email repository pathname are not supplied 
	//				E_OK		If this POP3 account manager is now initialized

	hzVect	<hzString>	dirs ;		//	Vector of directories populated by ListDir()
	hzVect	<hzString>	files ;		//	Vector of files populated by ListDir()

	hzString	S ;				//	Temp string (filename)
	uint32_t	nIndex ;		//	Files iterator
	hzEcode		rc ;			//	Return code

	m_Error.Clear() ;

	m_Server = Server ;
	m_Username = Username ;
	m_Password = Password ;
	m_Repos = Repos ;

	if (m_Repos)
	{
		m_Error.Printf("hzPop3Acc::Init - Duplicate call\n") ;
		return E_INITDUP ;
	}

	if (!m_Server || !m_Username || !m_Password || !m_Repos)
	{
		m_Error.Printf("hzPop3Acc::Init\n") ;

		if (!m_Server)		m_Error.Printf(" - No POP3 server specified\n") ;
		if (!m_Username)	m_Error.Printf(" - No POP3 Username\n") ;
		if (!m_Password)	m_Error.Printf(" - No POP3 Password\n") ;
		if (!m_Repos)		m_Error.Printf(" - No POP3 Repository specified\n") ;

		return E_ARGUMENT ;
	}

	rc = AssertDir(Repos, 0777) ;
	if (rc != E_OK)
		return rc ;

	//	Read the repository and populate the m_Already map
	rc = ListDir(dirs, files, *Repos, 0) ;
	for (nIndex = 0 ; nIndex < files.Count() ; nIndex++)
	{
		S = files[nIndex] ;
		m_Already.Insert(S) ;
	}

	return E_OK ;
}

hzEcode	hzPop3Acc::Collect	(hzVect<hzString>& messages)
{
	//	Collect emails from the POP3 Account (Conduct a POP3 session with the designated server). The emails are each placed in thier own file.
	//
	//	Arguments:	1)	messages	A vector of strings that is populated with filename of collected emails.
	//
	//	Returns:	E_NOINIT	The POP3 account is not initialized
	//				E_PROTOCOL	The session message not as expected
	//				E_OK		The email collection was successful


	_hzfunc("hzPop3Acc::Collect") ;

	hzMapS<int32_t,hzString>	temp ;	//	Emails available from the POP3 server

	ofstream		os ;			//	Output stream
	hzTcpClient		P ;				//	POP3 client connection
	chIter			zi ;			//	For iterating server responses
	hzChain			Z ;				//	For sending commands and receiving responses
	hzChain			word ;			//	For capturing small strings (eg mail number and mail id)
	hzChain			msg ;			//	For garnering messages
	hzString		S ;				//	For conversion of 'word' into a string
	hzString		path ;			//	For email filenames
	uint32_t		mailId ;		//	Mail number as given by server
	uint32_t		nIndex ;		//	For iterating avail emails
	hzEcode			rc ;			//	Return code

	m_Error.Clear() ;

	if (!m_Repos)
	{
		threadLog("POP3 Account not initialized\n") ;
		return E_NOINIT ;
	}

	rc = P.ConnectStd(m_Server, 110) ;
	if (rc != E_OK)
	{
		threadLog("%s: Cannot connect to email server %s (error=%s)\n", *_fn, *m_Server, Err2Txt(rc)) ;
		return rc ;
	}

	rc = P.SetSendTimeout(30) ;
	if (rc != E_OK)
	{
		threadLog("%s: Could not set send_timeout on connection to POP3 server (error=%s)\n", *_fn, Err2Txt(rc)) ;
		return rc ;
	}

	rc = P.SetRecvTimeout(30) ;
	if (rc != E_OK)
	{
		threadLog("%s: Could not set recv_timeout on connection to POP3 server (error=%s)\n", *_fn, Err2Txt(rc)) ;
		return rc ;
	}

	//	Expect the server to talk first with a +OK
	Z.Clear() ;
	rc = P.Recv(Z) ;
	if (rc != E_OK)
	{
		threadLog("%s: Cannot recv server hello (error=%s)\n", *_fn, Err2Txt(rc)) ;
		goto done ;
	}
	S = Z ;
	threadLog("%s. Server: [%s]\n", *_fn, *S) ;

	zi = Z ;
	if (zi != "+OK")
	{
		rc = E_PROTOCOL ;
		threadLog("%s: Expected +OK as hello from server. (error=%s)\n", *_fn, Err2Txt(rc)) ;
		goto done ;
	}

	//	Send the initial username
	Z.Clear() ;
	Z.Printf("USER %s\r\n", *m_Username) ;

	S = Z ;
	threadLog("%s. Client: [%s]\n", *_fn, *S) ;

	rc = P.Send(Z) ;
	if (rc != E_OK)
	{
		threadLog("%s: Cannot send username to email server (error=%s)\n") ;
		goto done ;
	}

	//	Recv the +OK\r\n (to the username)
	Z.Clear() ;
	rc = P.Recv(Z) ;
	if (rc != E_OK)
	{
		threadLog("%s: Cannot recv response to username from email server (error=%s)\n") ;
		goto done ;
	}
	S = Z ;
	threadLog("%s. Server: [%s]\n", *_fn, *S) ;

	zi = Z ;
	if (zi != "+OK")
	{
		rc = E_PROTOCOL ;
		S = Z ;
		threadLog("%s: Expected +OK response to username (got=%s)\n", *_fn, *S) ;
		goto done ;
	}

	//	Send the password
	Z.Clear() ;
	Z.Printf("PASS %s\r\n", *m_Password) ;

	S = Z ;
	threadLog("%s. Client: [%s]\n", *_fn, *S) ;

	rc = P.Send(Z) ;
	if (rc != E_OK)
	{
		threadLog("%s: Cannot send password to email server (error=%s)\n", *_fn, Err2Txt(rc)) ;
		rc = E_PROTOCOL ;
		goto done ;
	}

	//	Recv the +OK\r\n (to the password)
	Z.Clear() ;
	rc = P.Recv(Z) ;
	if (rc != E_OK)
	{
		threadLog("%s: Cannot recv response to username from email server (error=%s)\n", *_fn, Err2Txt(rc)) ;
		goto done ;
	}
	S = Z ;
	threadLog("%s. Server: [%s]\n", *_fn, *S) ;

	zi = Z ;
	if (zi != "+OK")
	{
		S = Z ;
		threadLog("%s: Expected +OK to password. (got=%s)\n", *_fn, *S) ;
		rc = E_PROTOCOL ;
		goto done ;
	}

	//	Send the UIDL command
	Z.Clear() ;
	Z.Printf("UIDL\r\n") ;

	S = Z ;
	threadLog("%s. Client: [%s]\n", *_fn, *S) ;

	rc = P.Send(Z) ;
	if (rc != E_OK)
	{
		threadLog("%s: Cannot send UIDL command to email server (error=%s)\n", *_fn, Err2Txt(rc)) ;
		goto done ;
	}

	//	Recv the +OK\r\n (to the UIDL command)
	Z.Clear() ;
	for (;;)	//nIndex = 0 ; nIndex < 3 ; nIndex++)
	{
		rc = P.Recv(Z) ;
		if (rc != E_OK)
		{
			threadLog("%s: Cannot recv response to UIDL from email server (error=%s)\n", *_fn, Err2Txt(rc)) ;
			goto done ;
		}

		threadLog("Server - Response to UIDL of %d bytes\n", Z.Size()) ;

		//	Test for the \r\n.\r\n
		zi = Z ;
		zi += (Z.Size() - 5) ;

		if (zi == "\r\n.\r\n")
			break ;
	}
	S = Z ;
	threadLog("%s. Server: [%s]\n", *_fn, *S) ;

	zi = Z ;
	if (zi != "+OK")
	{
		rc = E_PROTOCOL ;
		threadLog("%s: Expected +OK response to UIDL (error=%s)\n", *_fn, Err2Txt(rc)) ;
		goto done ;
	}

	//	Recieve the list of available messages and place them in the temporary map
	for (; !zi.eof() && *zi != CHAR_NL ; zi++) ;
	zi++ ;

	for (; !zi.eof() ; zi++)
	{
		for (; !zi.eof() && IsDigit(*zi) ; zi++)
			word.AddByte(*zi) ;

		if (!word.Size())
			break ;

		//zi.Skipwhite() ;
		S = word ;
		mailId = atoi(*S) ;
		word.Clear() ;

		for (zi++ ; !zi.eof() ; zi++)
		{
			if (*zi == CHAR_CR)
				zi++ ;
			if (*zi == CHAR_NL)
				break ;

			word.AddByte(*zi) ;
		}

		S = word ;
		word.Clear() ;
		if (!m_Already.Exists(S))
			temp.Insert(mailId, S) ;

		if (zi.eof())
			break ;
	}

	for (nIndex = 0 ; nIndex < temp.Count() ; nIndex++)
	{
		mailId = temp.GetKey(nIndex) ;
		S = temp.GetObj(nIndex) ;

		threadLog("Mailbox has %d %d (%s)\n", nIndex, mailId, *S) ;
	}

	//	Recv the email list. Some of these we may already have
	for (nIndex = 0 ; rc == E_OK && nIndex < temp.Count() ; nIndex++)
	{
		mailId = temp.GetKey(nIndex) ;
		S = temp.GetObj(nIndex) ;

		//	Send the RETR command
		Z.Clear() ;
		Z.Printf("RETR %d\r\n", mailId) ;
		rc = P.Send(Z) ;
		if (rc != E_OK)
			{ threadLog("Cannot send RETR command to email server\n") ; break ; }

		//	Recv the +OK (to the RETR command). This involves repeated calls to Recv until the chain ends with a \r\n.\r\n sequence.
		Z.Clear() ;

		for (;;)
		{
			rc = P.Recv(Z) ;
			if (rc != E_OK)
				{ threadLog("Cannot recv response to RETR command\n") ; break ; }

			threadLog("Server - Message of %d bytes\n", Z.Size()) ;

			//	Test for the \r\n.\r\n
			zi = Z ;
			zi += (Z.Size() - 5) ;

			if (zi == "\r\n.\r\n")
				break ;
		}

		//	Go to end of line, then message body, the the CR/NL period CR/NL sequence
		msg.Clear() ;
		zi = Z ;
		if (zi != "+OK")
		{
			rc = E_OK ;
			S = Z ;
			threadLog("Expected +OK in response to RETR command (got=%s)\n", *S) ;
			continue ;
		}
		for (zi += 3 ; !zi.eof() && *zi != CHAR_NL ; zi++) ;
		if (*zi != CHAR_NL)
		{
			threadLog("Malformed +OK response to RETR command\n") ;
			continue ;
		}

		for (zi++ ; !zi.eof() ; zi++)
		{
			if (*zi == CHAR_CR)
			{
				if (zi == "\r\n.\r\n")
				{
					zi += 5 ;
					if (zi.eof())
						break ;
					zi -= 5 ;
				}
			}

			msg.AddByte(*zi) ;
		}

		path = m_Repos + "/" + S ;
		os.open(*path) ;
		if (os.fail())
			threadLog("Cannot open file %s for writting\n", *path) ;
		else
		{
			threadLog("Writing message file %s\n", *path) ;
			os << msg ;
			os.close() ;
			os.clear() ;
			messages.Add(S) ;
		}
	}

	os.close() ;

done:
	P.Close() ;
	return rc ;
}

hzEcode	hzPop3Acc::GetEmail	(hzEmail& theMessage, hzString& mailId)
{
	//	Go to the archive file for emails (one produced each day), and retrieve the email header and body for the given id
	//
	//	Arguments:	1)	em		The email instance populated by this operation
	//				2)	mailId	The email unique id
	//
	//	Returns:	E_OPENFAIL	If the mailbox file could not be opened.
	//				E_FORMAT	If the email is malformed
	//				E_OK		If the email was successfully retrieved from the mailbox

	_hzfunc("hzPop3Acc::GetEmail") ;

	hzList<hzEmpart>::Iter	pi ;	//	Iterator for the email parts

	ifstream	is ;				//	Input stream
	hzChain		Z ;					//	For loading message
	hzString	filepath ;			//	Full path to email file
	hzEcode		rc ;				//	Return code

	m_Error.Clear() ;
	theMessage.Clear() ;

	//	Open email file and read in the header. This starts with a line of the form 'From email_addr date' and is followed by lines of
	//	the form 'param: args'. These lines may be followed by continuation lines that begin with whitespace. We read in continuation
	//	lines as though they are just extensions to the preceeding parameter line.

	filepath = m_Repos + "/" + mailId ;

	rc = OpenInputStrm(is, filepath, *_fn) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Failed tp open email file %s", *filepath) ;
	Z << is ;
	is.close() ;

	rc = theMessage.Import(Z) ;
	return rc ;
}
