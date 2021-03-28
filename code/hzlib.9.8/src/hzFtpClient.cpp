//
//	File:	hzFtpClient.cpp
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

#include <fstream>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h>
#include <utime.h>

#include "hzTextproc.h"
#include "hzFtpClient.h"
#include "hzProcess.h"

using namespace std ;

/*
**	General FTP functions
*/

hzEcode	_checkpath	(hzString& ult, const hzString& current, const hzString& mod)
{
	//	Predicts the final directory path when a path is operated on by a change directory command.
	//
	//	Arguments:	1)	ult		The result of the current path operated on by the moderator
	//				2)	current	The current path that must begin with the root '/' (or [A-Z]: under windows)
	//				3)	mod		The moderator.
	//
	//	Returns:	E_FORMAT	Either the current path is not a valid and absolute path. Or the moderator is not valid.
	//				E_OK		The oparation was successful.

	const char*		i ;				//	For derivation of target dir
	const char*		j ;				//	For derivation of target dir
	hzString		tgt ;			//	Directory we should end up in
	uint32_t		nPosn ;			//	Positions for manipulation of m_ServerDir

	ult.Clear() ;

	/*
 	**	Check the current is an absolute path (must begin with '/')
	*/

	if (!mod)
	{
		ult = current ;
		if (!ult)
			ult = "/" ;
		return E_OK ;
	}

	if (!current)
	{
		i = *mod ;
		if (*i != CHAR_FWSLASH)
		{
			ult = "invalid moderator" ;
			return E_FORMAT ;
		}
		ult = mod ;
		return E_OK ;
	}

	i = *current ;
	if (*i != CHAR_FWSLASH)
	{
		ult = "invalid current path" ;
		return E_FORMAT ;
	}

	/*
 	**	Check the directory is reasonable
	*/

	i = *mod ;
	if (*i == CHAR_FWSLASH)
		ult = mod ;
	else
	{
		ult = current ;

		if (mod == "..")
		{
			//	truncate our record of the remote dir

			j = *ult ;
			nPosn = CstrLast(j, CHAR_FWSLASH) ;
			if (nPosn)
				ult.Truncate(nPosn) ;
		}

		if (!memcmp(i, "../", 3))
		{
			for (;;)
			{
				if (memcmp(i, "../", 3))
					break ;
				i += 3 ;

				j = *ult ;
				nPosn = CstrLast(j, CHAR_FWSLASH) ;
				ult.Truncate(nPosn) ;
			}
		}

		//	Add the relative path
		ult += "/" ;
		ult += i ;
	}

	return E_OK ;
}

hzEcode	hzFtpClient::_setmetafile	(hzDirent& meta, char* line)
{
	//	Set file metadata (hzDirent) with a line from the server's directory listing.
	//
	//	Arguments:	1)	meta	The hzDirent to be populated
	//				2)	line	The line from the directory listing
	//
	//	Returns:	E_FORMAT	If the data is garbled
	//				E_OK		If the operation successful

	_hzfunc("hzFtpClient::_setmetafile") ;

	hzXDate		now ;			//	Current date/time
	hzXDate		date ;			//	Date of file (according to server)
	char*		i ;				//	Line iterator
	char*		pUno ;			//	Pointer to user number in line
	char*		pUsr ;			//	Pointer to user name in line
	char*		pGrp ;			//	Pointer to group name in line
	char*		pSze ;			//	Pointer to file size in line
	char*		pMon ;			//	Pointer to month in line
	char*		pDay ;			//	Pointer to day in line
	char*		pTim ;			//	Pointer to time in line
	hzString	test ;			//	Set to line
	uint32_t	file_year ;		//	Year of file
	uint32_t	file_month ;	//	Month of file
	uint32_t	file_day ;		//	Day of file
	uint32_t	file_hour ;		//	Hour of file
	uint32_t	file_min ;		//	Minute of file
	hzEcode		rc = E_OK ;		//	Return code

	now.SysDateTime() ;
	test = line ;

	i = line + 10 ;
	for (    ; *i && *i <= CHAR_SPACE ; i++) ; pUno = i ; for (i++ ; *i > CHAR_SPACE ; i++) ; *i = 0 ; 
	for (i++ ; *i && *i <= CHAR_SPACE ; i++) ; pUsr = i ; for (i++ ; *i > CHAR_SPACE ; i++) ; *i = 0 ;
	for (i++ ; *i && *i <= CHAR_SPACE ; i++) ; pGrp = i ; for (i++ ; *i > CHAR_SPACE ; i++) ; *i = 0 ;
	for (i++ ; *i && *i <= CHAR_SPACE ; i++) ; pSze = i ; for (i++ ; *i > CHAR_SPACE ; i++) ; *i = 0 ;
	for (i++ ; *i && *i <= CHAR_SPACE ; i++) ; pMon = i ; for (i++ ; *i > CHAR_SPACE ; i++) ; *i = 0 ;
	for (i++ ; *i && *i <= CHAR_SPACE ; i++) ; pDay = i ; for (i++ ; *i > CHAR_SPACE ; i++) ; *i = 0 ;
	for (i++ ; *i && *i <= CHAR_SPACE ; i++) ; pTim = i ; for (i++ ; *i > CHAR_SPACE ; i++) ; *i = 0 ;

	meta.Name(i + 1) ;
	meta.Size((uint32_t) atol(pSze)) ;

	/*
	**	Get file date
	*/

	if		(!strcmp(pMon, "Jan")) file_month = 1 ;
	else if (!strcmp(pMon, "Feb")) file_month = 2 ;
	else if (!strcmp(pMon, "Mar")) file_month = 3 ;
	else if (!strcmp(pMon, "Apr")) file_month = 4 ;
	else if (!strcmp(pMon, "May")) file_month = 5 ;
	else if (!strcmp(pMon, "Jun")) file_month = 6 ;
	else if (!strcmp(pMon, "Jul")) file_month = 7 ;
	else if (!strcmp(pMon, "Aug")) file_month = 8 ;
	else if (!strcmp(pMon, "Sep")) file_month = 9 ;
	else if (!strcmp(pMon, "Oct")) file_month = 10 ;
	else if (!strcmp(pMon, "Nov")) file_month = 11 ;
	else if (!strcmp(pMon, "Dec")) file_month = 12 ;
	else
	{
		threadLog("%s. Invalid month: Line [%s]\n", *_fn, *test) ;
		return E_FORMAT ;
	}

	if (!IsPosint(file_day, pDay))
	{
		threadLog("%s. Invalid day: Line [%s]\n", *_fn, *test) ;
		return E_FORMAT ;
	}

	if (!IsPosint(file_hour, pTim))
	{
		threadLog("%s. Invalid Hour: Line [%s]\n", *_fn, *test) ;
		return E_FORMAT ;
	}

	if (file_hour > 2000)
		{ file_year = file_hour ; file_hour = 12 ; file_min = 0 ; }
	else
	{
		file_year = now.Year() ;
		if (!IsPosint(file_min, pTim + 3))
		{
			threadLog("%s. Invalid Minutes: Line [%s]\n", *_fn, *test) ;
			return E_FORMAT ;
		}
	}

	//	Do some logic to make up for the lack of year in date

	rc = date.SetDate(file_year, file_month, file_day) ;
	if (rc != E_OK)
	{
		threadLog("%s. Line [%s] Could not set date to %04d/%02d/%02d\n", *_fn, *test, file_year, file_month, file_day) ;
		return E_FORMAT ;
	}

	rc = date.SetTime(file_hour, file_min, 0) ;
	if (rc != E_OK)
	{
		threadLog("%s. Line [%s] Could not set time to %02d:%02d\n", *_fn, *test, file_hour, file_min) ;
		return E_FORMAT ;
	}

	if (date.NoDays() > now.NoDays())
	{
		date.SetDate(now.Year() - 1, file_month, file_day) ;
		if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
			threadLog("%s. Adjusted date that would be in the future if current year assumed - now set to %04d/%02d/%02d\n",
				*_fn, date.Year(), file_month, file_day) ;
	}

	meta.Ctime(date) ;
	meta.Mtime(date) ;
	return rc ;
}

hzEcode	hzFtpClient::_openpasv	(hzTcpClient& X)
{
	//	Sends a PASV command to the FTP server and opens a data channel. Return values are as follows:-
	//
	//	Arguments:	1)	X		The control channel client connection instance
	//
	//	Returns:	E_SENDFAIL	The PASV command could not be sent.	The calling function should attempt a reconnect.
	//				E_RECVFAIL	The PASV response was not received.	The calling function should attempt a reconnect.
	//				E_HOSTFAIL	The data channel connection failed.	The calling function should attempt a reconnect.
	//				E_PROTOCOL	The PASV response was not expected. The caller should abort the session as server is unlikely to respond as expected next time.
	//				E_OK		If the operation was successful.
	//
	//	Scope:		Private to the hzFtpClient class.

	_hzfunc("hzFtpClient::_openpasv") ;

	uint32_t	nRecv ;		//	Bytes received
	uint32_t	len ;		//	Length of outgoing msg
	uint32_t	a ;			//	1st part of IP addr for data connection
	uint32_t	b ;			//	2nd part of IP addr for data connection
	uint32_t	c ;			//	3rd part of IP addr for data connection
	uint32_t	d ;			//	4th part of IP addr for data connection
	uint32_t	nA ;		//	First part of data port
	uint32_t	nB ;		//	Second part of data port
	char*		i ;			//	For processing prt nos etc
	hzEcode		rc ;		//	Return code

	/*
	**	Send PASV command
	*/

	sprintf(m_c_sbuf, "PASV\r\n") ;
	len = strlen(m_c_sbuf) ;

	if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{ threadLog("%s. PASV: Send failed\n", *_fn) ; return rc ; }

	if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{ threadLog("%s. Could not recv response to PASV command\n", *_fn) ; return rc ; }

	if (m_nRescode != 227)
		{ threadLog("%s. Expected code of 227: Got %s\n", *_fn, m_c_rbuf) ; return E_PROTOCOL ; }

	/*
	**	Convert recived message to a port number to connect to
	*/

	i = strchr(m_c_rbuf, '(') ;
	if (!i)
		{ threadLog("%s. Expected a substring of (...) containing data conn details\n", *_fn) ; return E_PROTOCOL ; }

	SplitCSV(m_Array, i + 1) ;	//, 6) ;
	a = m_Array[0] ? atoi(*m_Array[0]) : 0 ;
	b = m_Array[1] ? atoi(*m_Array[1]) : 0 ;
	c = m_Array[2] ? atoi(*m_Array[2]) : 0 ;
	d = m_Array[3] ? atoi(*m_Array[3]) : 0 ;
	nA = m_Array[4] ? atoi(*m_Array[4]) : 0 ;
	nB = m_Array[5] ? atoi(*m_Array[5]) : 0 ;

	m_nDataPort = (nA * 256) + nB ;

	if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
		threadLog("%s. Nos are %d:%d:%d:%d, %d,%d - Will connect for data on port %d\n", *_fn, a, b, c, d, nA, nB, m_nDataPort) ;

	/*
	**	Now set up data client
	*/

	rc = X.ConnectStd(m_Server, m_nDataPort) ;
	if (rc != E_OK)
		{ threadLog("%s. Could not conect to FTP data port\n", *_fn) ; return rc ; }

	X.SetSendTimeout(3) ;
	X.SetRecvTimeout(3) ;

	return rc ;
}

void	hzFtpClient::_logrescode	(const char* fn)
{
	//	Report FTP return codes to the logfile of the current thread
	//
	//	Arguments:	1)	fn	Function name
	//
	//	Returns:	None

	switch	(m_nRescode)
	{
	//	100 Codes: The requested action is being taken. Expect a reply before proceeding with a new command.
	case 110:	threadLog("%s: 110 - Restart marker reply\n", fn) ; break ;
	case 120:	threadLog("%s: 120 - Service ready in (n) minutes\n", fn) ; break ;
	case 125:	threadLog("%s: 125 - Data connection already open, transfer starting\n", fn) ; break ;
	case 150:	threadLog("%s: 150 - File status okay, about to open data connection\n", fn) ; break ;

	//	200 Codes: The requested action has been successfully completed.
	case 200:	threadLog("%s: 200 - Command okay\n", fn) ; break ;
	case 202:	threadLog("%s: 202 - Command not implemented\n", fn) ; break ;
	case 211:	threadLog("%s: 211 - System status, or system help reply\n", fn) ; break ;
	case 212:	threadLog("%s: 212 - Directory status\n", fn) ; break ;
	case 213:	threadLog("%s: 213 - File status\n", fn) ; break ;
	case 214:	threadLog("%s: 214 - Help message\n", fn) ; break ;
	case 215:	threadLog("%s: 215 - NAME system type (Official system name from the list in the Assigned Numbers document)\n", fn) ; break ;
	case 220:	threadLog("%s: 220 - Service ready for new user\n", fn) ; break ;
	case 221:	threadLog("%s: 221 - Service closing control connection (Logged out)\n", fn) ; break ;
	case 225:	threadLog("%s: 225 - Data connection open, no transfer in progress\n", fn) ; break ;
	case 226:	threadLog("%s: 226 - Closing data connection. Requested file action successful (file transfer, abort, etc)\n", fn) ; break ;
	case 227:	threadLog("%s: 227 - Entering Passive Mode\n", fn) ; break ;
	case 230:	threadLog("%s: 230 - User logged in, proceed\n", fn) ; break ;
	case 250:	threadLog("%s: 250 - Requested file action okay, completed\n", fn) ; break ;
	case 257:	threadLog("%s: 257 - Pathname created\n", fn) ; break ;

	//	300 Codes: The command has been accepted, but the requested action is being held pending receipt of further information.\n", fn) ;
	case 331:	threadLog("%s: 331 - User name okay, need password\n", fn) ; break ;
	case 332:	threadLog("%s: 332 - Need account for login\n", fn) ; break ;
	case 350:	threadLog("%s: 350 - Requested file action pending further information\n", fn) ; break ;

	//	400 Codes: The command was not accepted and the requested action did not take place.
	//	the error condition is temporary, however, and the action may be requested again.
	case 421:	threadLog("%s: 421 - Service not available, closing control connection. (Server is shuting down)\n", fn) ; break ;
	case 425:	threadLog("%s: 425 - Can't open data connection\n", fn) ; break ;
	case 426:	threadLog("%s: 426 - Connection closed, transfer aborted\n", fn) ; break ;
	case 450:	threadLog("%s: 450 - Requested file action not taken. File unavailable\n", fn) ; break ;
	case 451:	threadLog("%s: 451 - Requested action aborted, local error in processing\n", fn) ; break ;
	case 452:	threadLog("%s: 452 - Requested action not taken. Insufficient storage space in system\n", fn) ; break ;

	//	500 Codes: The command was not accepted and the requested action did not take place.
	case 500:	threadLog("%s: 500 - Syntax error, command unrecognized\n", fn) ; break ;
	case 501:	threadLog("%s: 501 - Syntax error in parameters or arguments\n", fn) ; break ;
	case 502:	threadLog("%s: 502 - Command not implemented\n", fn) ; break ;
	case 503:	threadLog("%s: 503 - Bad sequence of commands\n", fn) ; break ;
	case 504:	threadLog("%s: 504 - Command not implemented for that parameter\n", fn) ; break ;
	case 530:	threadLog("%s: 530 - User not logged in\n", fn) ; break ;
	case 532:	threadLog("%s: 532 - Need account for storing files\n", fn) ; break ;
	case 550:	threadLog("%s: 550 - Requested action not taken. File unavailable (e.g. file not found, no access)\n", fn) ; break ;
	case 552:	threadLog("%s: 552 - Requested file action aborted, storage allocation exceeded\n", fn) ; break ;
	case 553:	threadLog("%s: 553 - Requested action not taken. Illegal file name\n", fn) ; break ;

	default:
		threadLog("%s: No valid command received\n", fn) ;
		break ;
	}
}

hzEcode	hzFtpClient::_ftprecv	(uint32_t& nRecv, const char* callFn)
{
	//	Recieve data from the FTP control channel
	//
	//	Arguments:	1)	nRecv	Bytes received
	//				2)	callFn	Calling function
	//
	//	Returns:	E_RECVFAIL	Connection breakdown
	//				E_PROTOCOL	Invalid response (Give up)
	//				E_OK		Operation successful

	hzChain		msg ;			//	Recived FTP control message chain
	_ftpline	fline ;			//	Line of FTP control message
	char*		i ;				//	Line iterator
	uint32_t	nTry ;			//	Reconnection retry count
	uint32_t	code ;			//	FTP server return code
	hzEcode		rc = E_OK ;		//	Return code

	m_nRescode = 0 ;

	if (!m_Stack.Count())
	{
		//	No stored response (from an earlier multi-line response) so we have to do a recv

		for (nTry = 0 ; nTry < 10 ; nTry++)
		{
			rc = m_ConnControl.Recv(m_c_rbuf, nRecv, HZ_MAXPACKET) ;

			if (rc == E_TIMEOUT)
				continue ;

			if (rc != E_OK)
				threadLog("%s. Failed to recv from control channel (%d bytes, error=%s)\n", callFn, nRecv, Err2Txt(rc)) ;
			break ;
		}

		if (nTry == 10)
		{
			threadLog("%s. Stalled - 10 counts and out\n", callFn) ;
			return E_RECVFAIL ;
		}

		if (nTry)
			threadLog("%s. Stalled - %d counts and resumed\n", callFn, nTry) ;

		if (rc != E_OK)
			{ threadLog("%s. Failed error=%s\n", callFn, Err2Txt(rc)) ; return rc ; }

		m_c_rbuf[nRecv] = 0 ;

		//	Separate the lines in the response. If there is more than one they need to be stacked. Then
		//	subsequent calls to this function can take a line from the que.

		i = m_c_rbuf ;

		if (IsDigit(i[0]) && IsDigit(i[1]) && IsDigit(i[2]))
			fline.m_code = ((i[0] - '0') * 100) + ((i[1] - '0') * 10) + (i[2] - '0') ;
		else
		{
			threadLog("%s. Invalid server response: %s\n", callFn, i) ;
			return E_PROTOCOL ;
		}

		for (;;)
		{
			if (*i == 0)
			{
				if (msg.Size())
				{
					fline.m_msg = msg ;
					m_Stack.Add(fline) ;
				}
				break ;
			}

			if (i[0] == CHAR_NL || (i[0] == CHAR_CR && i[1] == CHAR_NL))
			{
				if (i[0] == CHAR_CR)
					*i++ = 0 ;
				*i++ = 0 ;

				code = ((i[0] - '0') * 100) + ((i[1] - '0') * 10) + (i[2] - '0') ;
				if (code == fline.m_code)
					continue ;

				if (msg.Size())
				{
					fline.m_msg = msg ;
					m_Stack.Add(fline) ;
					msg.Clear() ;
					fline.m_msg = 0 ;
					fline.m_code = code ;
				}
				continue ;
			}

			msg.AddByte(*i) ;
			i++ ;
		}

		if (m_Stack.Count() > 1)
		{
			//	Check if these multiple lines are part of the same message (ie all have same code), if so the lines are merged.

			if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
			{
				threadLog("%s. xple control lines (%d)\n", callFn, m_Stack.Count()) ;
				for (nTry = 0 ; nTry < m_Stack.Count() ; nTry++)
				{
					fline = m_Stack[nTry] ;
					threadLog(" - %d %s\n", fline.m_code, *fline.m_msg) ;
				}
			}
		}
	}

	/*
 	**	Process response (from server or stack)
	*/

	if (!m_Stack.Count())
	{
		threadLog("%s. Empty message stack\n", callFn) ;
		return E_PROTOCOL ;
	}

	fline = m_Stack[0] ;
	m_nRescode = fline.m_code ;

	strcpy(m_c_rbuf, *fline.m_msg) ;

	if (m_Stack.Count() == 1)
		m_Stack.Clear() ;
	else
	{
		rc = m_Stack.Delete(0) ;
		if (rc != E_OK)
			threadLog("%s. Corrupt m_Stack delete\n", callFn) ;
	}

	if (rc != E_OK)
		 threadLog("%s. _ftprecv error=%s\n", callFn, Err2Txt(rc)) ;

	return rc ;
}

hzEcode	hzFtpClient::_reconnect	(void)
{
	//	In the event of a disconnection, reconnect to server and restore the session in full. Note that this differs from the original connect call in that the
	//	possible outcomes no longer scanarios like server not found.
	//
	//	Arguments:	None
	//
	//	Returns:	E_HOSTFAIL	Reconnection unsuccessful breakdown
	//				E_PROTOCOL	Invalid response (Give up)
	//				E_OK		Operation successful

	_hzfunc("hzFtpClient::_reconnect") ;

	std::ofstream	os ;	//	Out to dirlist file
	std::ifstream	is ;	//	Read back dirlist file

	hzChain		CR ;		//	Receiver chain (control)
	hzChain		XR ;		//	Receiver chain (data)
	hzEcode		rc ;		//	Return code from publication functions

	uint32_t	nRecv ;		//	Bytes recv
	uint32_t	len ;		//	Length of client request

	threadLog("%s. -> %s::%s\n", *_fn, *m_Server, *m_ServerDir) ;

	/*
	**	Connect to server and set socket timeouts
	*/

	m_ConnControl.Close() ;

	for (; m_nTries < 10 ;)
	{
		sleep(1) ;

		if ((rc = m_ConnControl.ConnectStd(m_Server, 21)) != E_OK)
		{
			if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
				threadLog("%s. Attempt %d: Could not conect to FTP server\n", *_fn, m_nTries) ;
			m_nTries++ ;
			continue ;
		}

 		//	Now reconnected, receive server hello
		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Did not get server hello\n", *_fn) ;
			m_ConnControl.Close() ;
			m_nTries++ ;
			continue ;
		}

		if (m_nRescode != 220)
		{
			threadLog("%s. Expected a 220, got %s\n", *_fn, m_c_rbuf) ;
			rc = E_PROTOCOL ;
			break ;
		}

		//	Send username
		sprintf(m_c_sbuf, "USER %s\r\n", *m_Username) ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Failed to send USER command\n", *_fn) ;
			m_ConnControl.Close() ;
			m_nTries++ ;
			continue ;
		}

 		//	Receive response
		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Failed to receive respose to USER command\n", *_fn) ;
			m_ConnControl.Close() ;
			m_nTries++ ;
			continue ;
		}

		_logrescode(_fn) ;

		if (m_nRescode >= 400)
		{
			threadLog("%s. Aborting Session (case 1)\n", *_fn) ;
			rc = E_PROTOCOL ;
			break ;
		}

		//	Send password
		sprintf(m_c_sbuf, "PASS %s\r\n", *m_Password) ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Failed to send PASS command (password)\n", *_fn) ;
			m_ConnControl.Close() ;
			m_nTries++ ;
			continue ;
		}

 		//	Receive server response to password
		sleep(1) ;

		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Failed to receive respose to password\n", *_fn) ;
			m_ConnControl.Close() ;
			m_nTries++ ;
			continue ;
		}

		_logrescode(_fn) ;

		if (m_nRescode >= 400)
		{
			threadLog("%s. Aborting Session (case 2)\n", *_fn) ;
			rc = E_PROTOCOL ;
			m_ConnControl.Close() ;
			break ;
		}

 		//	Now because we are re-connecting, we just want to go to the directory we were last in. Send CWD command.
		sprintf(m_c_sbuf, "CWD %s\r\n", *m_ServerDir) ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Could not send CWD command\n", *_fn) ;
			m_ConnControl.Close() ;
			m_nTries++ ;
			continue ;
		}

 		//	Receive the response
		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Failed to receive CWD respose\n", *_fn) ;
			m_ConnControl.Close() ;
			m_nTries++ ;
			continue ;
		}

		_logrescode(_fn) ;

		if (m_nRescode == 250)
			rc = E_OK ;
		else
		{
			rc = E_PROTOCOL ;
			m_ConnControl.Close() ;
		}
		break ;
	}

	return rc ;
}

hzEcode	hzFtpClient::StartSession	(void)
{
	// 	Start an FTP session (establish a connection witht the FTP server)
	//
	// 	Arguments:	None
	//
	// 	Returns:	E_HOSTFAIL	If the initial control channel connection could not be established. The usual remedy is to try again later.
	// 				E_SENDFAIL	If either the username or password could not be sent. Broken pipe so try again later.
	// 				E_RECVFAIL	If the expected reponses were not recieved. Again a broken pipe so try again later.
	// 				E_PROTOCOL	If either the username or password was invalid. Fatal error as this must be sorted out before trying again.
	// 				E_OK		If the operation was successful.

	_hzfunc("hzFtpClient::StartSession") ;

	std::ofstream	os ;		//	Out to dirlist file
	std::ifstream	is ;		//	Read back dirlist file

	hzChain		CR ;		//	Receiver chain (control)
	hzChain		XR ;		//	Receiver chain (data)
	hzEcode		rc ;		//	Return code from publication functions

	uint32_t	nRecv ;			//	Bytes recv
	uint32_t	len ;

	/*
	**	Connect to server and set socket timeouts
	*/

	for (m_nTries = 0 ; m_nTries < 5 ; m_nTries++)
	{
		if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
			threadLog("%s. Calling Connect for control channel at server %s\n", *_fn, *m_Server) ;

		rc = m_ConnControl.ConnectStd(m_Server, 21) ;
		if (rc != E_OK)
		{
			if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
				threadLog("%s. Attempt %d: Could not connect to FTP server control channel. (error=%s)\n", *_fn, m_nTries, Err2Txt(rc)) ;
			continue ;
		}
		m_nTries = 0 ;
		break ;
	}

	if (rc != E_OK)
	{
		threadLog("%s. Given up!\n", *_fn) ;
		return rc ;
	}

	if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
		threadLog("%s. Connected to control channel at server %s with sock %d on port 21\n", *_fn, *m_Server, m_ConnControl.Sock()) ;

	rc = m_ConnControl.SetSendTimeout(10) ;
	if (rc != E_OK)
	{
		threadLog("%s. Could not set send timeout: Quitting\n", *_fn) ;
		m_ConnControl.Close() ;
		return rc ;
	}

	rc = m_ConnControl.SetRecvTimeout(10) ;
	if (rc != E_OK)
	{
		threadLog("%s. Could not set send timeout: Quitting\n", *_fn) ;
		m_ConnControl.Close() ;
		return rc ;
	}

	if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
		threadLog("%s. Send & Recv timeouts set\n", *_fn) ;

	/*
 	**	Receive server hello
	*/

	rc = _ftprecv(nRecv, *_fn) ;
	if (rc != E_OK)
	{
		threadLog("%s. Did not get server hello\n", *_fn) ;
		m_ConnControl.Close() ;
		return rc ;
	}

	if (m_nRescode != 220)
	{
		threadLog("%s. Expected a 220, got %s\n", *_fn, m_c_rbuf) ;
		m_ConnControl.Close() ;
		return E_PROTOCOL ;
	}

	if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
		threadLog("%s. Got server hello ....\n", *_fn) ;

	/*
	**	Send username
	*/

	sprintf(m_c_sbuf, "USER %s\r\n", *m_Username) ;
	len = strlen(m_c_sbuf) ;

	rc = m_ConnControl.Send(m_c_sbuf, len) ;
	if (rc != E_OK)
	{
		threadLog("%s. Failed to send USER command\n", *_fn) ;
		m_ConnControl.Close() ;
		return rc ;
	}

	rc = _ftprecv(nRecv, *_fn) ;
	if (rc != E_OK)
	{
		threadLog("%s. Failed to receive respose to USER command\n", *_fn) ;
		m_ConnControl.Close() ;
		return rc ;
	}

	_logrescode(_fn) ;

	if (m_nRescode != 331)
	{
		threadLog("%s. Bad response to USER. Expected a 331, got %d. Presume username %s invalid\n", *_fn, m_nRescode, *m_Username) ;
		QuitSession() ;
		rc = E_PROTOCOL ;
		return rc ;
	}

	/*
	**	Send password
	*/

	sprintf(m_c_sbuf, "PASS %s\r\n", *m_Password) ;
	len = strlen(m_c_sbuf) ;

	if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
	{
		threadLog("%s. PASS: Send failed\n", *_fn) ;
		m_ConnControl.Close() ;
		return rc ;
	}

	sleep(1) ;

	if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
	{
		threadLog("%s. Could not recv response to PASS command\n", *_fn) ;
		m_ConnControl.Close() ;
		return rc ;
	}

	_logrescode(_fn) ;

	if (m_nRescode >= 400)
	{
		threadLog("%s. Aborting Session\n", *_fn) ;
		QuitSession() ;
		rc = E_PROTOCOL ;
		return rc ;
	}

	rc = GetServerDir() ;
	if (rc != E_OK)
	{
		threadLog("%s. Could not get server directory\n", *_fn) ;
		return rc ;
	}

	threadLog("%s. Logged in to %s as %s\n", *_fn, *m_Server, *m_Username) ;
	m_nTries = 0 ;

	return rc ;
}

hzEcode	hzFtpClient::GetServerDir	(void)
{
	//	Obtain the current working directory on the server
	//
	//	Arguments:	None
	//
	//	Returns:	E_HOSTFAIL	If there was a communication failure and reconnect failed
	//				E_PROTOCOL	If the server did not respond as expected
	//				E_OK		If the current working directory on the server was obtained

	_hzfunc("hzFtpClient::GetServerDir") ;

	char*		i ;			//	Start of pathname (in quotes)
	char*		j ;			//	End of pathname (in quotes)
	uint32_t	nRecv ;		//	Bytes received
	uint32_t	len ;		//	Outgoing msg length
	uint32_t	nTry ;		//	Connection retries
	hzEcode		rc ;		//	Standard return code

	/*
	**	Send PWD command and receive resposns, reconnect if required.
	*/

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
		if (nTry == 1)
		{
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

		/*
		**	Send the PWD command, recv response
		*/

		sprintf(m_c_sbuf, "PWD\r\n") ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Could not send PWD command\n", *_fn) ;
			continue ;
		}

		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Could not recv response to PWD command\n", *_fn) ;
			continue ;
		}

		break ;
	}

	if (rc == E_OK)
	{
		/*
 		**	Check respons is valid
		*/

		_logrescode(_fn) ;

		if (m_nRescode != 257)
		{
			threadLog("%s. Expected a 257 return code, got %d\n", *_fn, m_nRescode) ;
			rc = E_PROTOCOL ;
		}
	}

	if (rc == E_OK)
	{
		/*
 		**	Set our record of the current server directory
		*/

		for (i = m_c_rbuf ; *i && *i != CHAR_DQUOTE ; i++)
		if (*i)
		{
			i++ ;
			for (j = i ; *j && *j != CHAR_DQUOTE ; j++) ;
			*j = 0 ;
		}

		if (m_ServerDir != i)
			m_ServerDir = i ;
		m_nTries = 0 ;

		return E_OK ;
	}

	threadLog("%s. Could not get server directory\n", *_fn) ;
	return rc ;
}

hzEcode	hzFtpClient::SetLocalDir	(const hzString& dir)
{
	//	Set the working directory on the local machine
	//
	//	Arguments:	1)	dir	The taget local directory
	//
	//	Returns:	E_NOTFOUND	If the local directory does not exist
	//				E_TYPE		If the specified local directory is not a directory
	//				E_OK		If the local directory is set

	FSTAT	fs ;	//	Local file status

	if (lstat(*dir, &fs) == -1)
		return E_NOTFOUND ;

	if (!ISDIR(fs.st_mode))
		return E_TYPE ;

	m_LocalDir = dir ;

	if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
		threadLog("hzFtpClient::SetLocalDir. Set local directory to %s\n", *m_LocalDir) ;

	return E_OK ;
}

hzEcode	hzFtpClient::SetRemoteDir	(const hzString& dir)
{
	//	Changes the directory on the server to either an absolute or relative path. This also sets the member m_ServerDir which tracks the server directory. The
	//	protocol is simple. The client sends a CWD with the supplied argument. The server either returns a 250 if the directory was changed or it returns a 550
	//	if there is no such directory.
	//
	//	Argument:	dir		The taget remote directory
	//
	//	Returns:	E_HOSTFAIL	If there was a communication failure and reconnect failed
	//				E_NOTFOUND	If the requested directory does not exist on the server or does not permit the client to access it
	//				E_OK		If the remote working directory was set

	_hzfunc("hzFtpClient::SetRemoteDir") ;

	hzString	tgt ;		//	Directory we should end up in
	uint32_t	nRecv ;		//	Length of server response
	uint32_t	len ;		//	Length of client request
	uint32_t	nTry ;		//	Connection retries
	hzEcode		rc ;		//	Return code

	/*
 	**	Check the directory is reasonable
	*/

	rc = _checkpath(tgt, m_ServerDir, dir) ;
	if (rc != E_OK)
	{
		threadLog("%s. In dir %s, acting on CD %s results in %s\n", *_fn, *m_ServerDir, *dir, *tgt) ;
		return rc ;
	}

	if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
		threadLog("%s. Tgt dir is %s\n", *_fn, *tgt) ;

	/*
	**	Send CWD command and receive resposns, reconnect if required.
	*/

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
		if (nTry == 1)
		{
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

		/*
		**	Send the CWD command, recv response
		*/

		sprintf(m_c_sbuf, "CWD %s\r\n", *dir) ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Could not send CWD command - Reconnecting ...\n", *_fn) ;
			continue ;
		}

		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Could not recv CWD response - Reconnecting ...\n", *_fn) ;
			continue ;
		}

		break ;
	}

	if (rc == E_OK)
	{
		/*
 		**	Check respons is valid
		*/

		if (m_nRescode == 250)
			rc = E_OK ;
		else
			rc = E_NOTFOUND ;
	}

	if (rc == E_OK)
	{
		m_ServerDir = tgt ;

		if (_hzGlobal_Debug & HZ_DEBUG_CLIENT)
			threadLog("%s. Remote dir is now %s\n", *_fn, *m_ServerDir) ;
		m_nTries = 0 ;
		return E_OK ;
	}

	if (rc == E_NOTFOUND)
	{
		threadLog("%s. Could not CD to %s (%s)\n", *_fn, *tgt, m_c_rbuf) ;
		return rc ;
	}

	threadLog("%s: Failure report (err=%s)\n", *_fn, Err2Txt(rc)) ;
	return rc ;
}

hzEcode	hzFtpClient::RemoteDirCreate	(const hzString& dir)
{
	//	Create a directory on the server
	//
	//	Argument:	dir		The taget remote directory
	//
	//	Returns:	E_HOSTFAIL	If there was a communication failure and reconnect failed
	//				E_WRITEFAIL	If the requested server directory was not created
	//				E_OK		If the remote working directory was created

	_hzfunc("hzFtpClient::RemoteDirCreate") ;

	uint32_t	nRecv ;		//	Incoming message length
	uint32_t	len ;		//	Outgoing message length
	uint32_t	nTry ;		//	Reconections
	hzEcode		rc ;		//	Return code

	/*
	**	Send MKD command and receive resposns, reconnect if required.
	*/

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
		if (nTry == 1)
		{
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

		/*
		**	Send the CWD command
		*/

		sprintf(m_c_sbuf, "MKD %s\r\n", *dir) ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Could not send MKD command\n", *_fn) ;
			continue ;
		}

		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Could not recv MKD response\n", *_fn) ;
			continue ;
		}

		break ;
	}

	/*
 	**	Check response code
	*/

	_logrescode(_fn) ;

	if (m_nRescode != 257)
	{
		threadLog("%s: Expected a 257 - Got %s\n", *_fn, m_c_rbuf) ;
		return E_WRITEFAIL ;
	}

	return E_OK ;
}

hzEcode	hzFtpClient::RemoteDirDelete	(const hzString& SvrDirname)
{
	//	Delete a directory on the server
	//
	//	Arguments:	1)	dir	The taget remote directory
	//
	//	Returns:	E_HOSTFAIL	If there was a communication failure and reconnect failed
	//				E_WRITEFAIL	If the requested server directory was not deleted
	//				E_OK		If the remote working directory was deleted

	_hzfunc("hzFtpClient::RemoteDirDelete") ;

	uint32_t	nRecv ;		//	Incoming message length
	uint32_t	len ;		//	Outgoing message length
	uint32_t	nTry ;		//	Reconections
	hzEcode		rc ;		//	Return code

	/*
	**	Send RMD command and receive resposns, reconnect if required.
	*/

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
		if (nTry == 1)
		{
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

		/*
		**	Send the RMD command
		*/

		sprintf(m_c_sbuf, "RMD %s\r\n", *SvrDirname) ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Could not send RMD command\n", *_fn) ;
			continue ;
		}

		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Could not recv response to RMD command\n", *_fn) ;
			continue ;
		} 

		break ;
	}

	/*
 	**	Check response code
	*/

	if (m_nRescode != 257)
	{
		threadLog("%s. Could not delete directory\n", *_fn) ;
		return E_NOTFOUND ;
	}

	return E_OK ;
}

hzEcode	hzFtpClient::GetDirList	(hzVect<hzDirent>& listing, const hzString& Criteria)
{
	//	Get a directory listing filtered by search criteria
	//
	//	Arguments:	1)	listing		The vector of directory entries populated by this operation
	//				2)	Criteria	The file/directory search criteria
	//
	//	Returns:	E_HOSTFAIL	If there was a communication failure and reconnect failed
	//				E_PROTOCOL	If the server did not provide a listing
	//				E_OK		If the directory listing was recieved

	_hzfunc("hzFtpClient::GetDirList") ;

	hzTcpClient		X ;				//	Temporary data connection
	hzDirent		meta ;			//	Metafile
	hzChain			Listing ;		//	Temporary store for listing
	hzChain::Iter	z ;				//	For processing naked listing
	hzString		filename ;		//	Name of file
	uint32_t		nRecv ;			//	Bytes recieved this packet
	uint32_t		nTotal ;		//	Total bytes recieved
	uint32_t		len ;			//	Length of content to send
	uint32_t		nTry ;			//	Reconnections
	uint32_t		nTryData ;		//	Timeouts on the data channel
	char*			j ;				//	For loading line buffer
	char			cvLine [300] ;	//	Line buffer
	hzEcode			rc = E_OK ;		//	Return code

	/*
 	**	Loop round until success
	*/

	//	X.SetDebug(_hzGlobal_Debug & HZ_DEBUG_CLIENT) ;

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
		if (nTry == 1)
		{
			X.Close() ;
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

 		//	First step is to open a data channel
		rc = _openpasv(X) ;
		if (rc != E_OK)
		{
			threadLog("%s: Failed PASV .. aborting\n", *_fn) ;
			continue ;
		}

		//	Send the LIST command
		if (Criteria)
			sprintf(m_c_sbuf, "LIST %s\r\n", *Criteria) ;
		else
			sprintf(m_c_sbuf, "LIST\r\n") ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Could not send LIST command\n", *_fn) ;
			continue ;
		}

		//	Get the expected 150 in the control client
		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Did not recieve a response to LIST command\n", *_fn) ;
			continue ;
		}

		if (m_nRescode != 150)
		{
			threadLog("%s. Expected code of 150 but got %d. Aborting\n", *_fn, m_nRescode) ;
			_logrescode(_fn) ;
			
			rc = E_PROTOCOL ;
			break ;
		}

		//	Get the listing in the data client
		nTotal = 0 ;
		Listing.Clear() ;

		for (nTryData = 0 ; nTryData < 3 ;)
		{
			rc = X.Recv(m_x_rbuf, nRecv, HZ_MAXPACKET) ;
			if (rc != E_OK)
			{
				threadLog("%s. LIST: Read failed (total so far %d bytes)\n", *_fn, nTotal) ;
				break ;
			}

			if (!nRecv)
				break ;
			nTotal += nRecv ;
			m_x_rbuf[nRecv] = 0 ;

			Listing << m_x_rbuf ;
		}

		X.Close() ;

		//	Unless the transfer is a complete success, do a reconnect.
		if (rc == E_TIMEOUT || nTryData == 3)
			continue ;
		if (rc == E_RECVFAIL)
			continue ;

		if (Listing.Size())
			threadLog("%s. LIST: Total listing %d bytes\n", *_fn, nTotal) ;
		else
			threadLog("%s. LIST: Empty list. No files\n", *_fn) ;

 		//	If a list has been sent there will be a 226 sent by the server which we will need to read - if only to get rid of it before we move on.
		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Did not recieve a response to LIST command\n", *_fn) ;
			continue ;
		}

		if (m_nRescode != 226)
		{
			threadLog("%s. LIST: Expected code of 226 (Xfer complete) but got %d. Aborting\n", *_fn, m_nRescode) ;
			rc = E_PROTOCOL ;
		}

		//	No further communication required so drop out
		break ;
	}

	if (rc == E_OK)
	{
		//	Examine the dir listing (if there is one)
		if (!Listing.Size())
			return rc ;

		j = cvLine ;
		for (z = Listing ; !z.eof() ; z++)
		{
			if (*z == CHAR_CR)
				continue ;

			if (*z == CHAR_NL)
			{
				//	Process the line
				*j = 0 ;
				j = cvLine ;

				//	Set meta data for the file
				rc = _setmetafile(meta, cvLine) ;
				if (rc != E_OK)
					break ;

				//	Add the meta data
				rc = listing.Add(meta) ;
				if (rc != E_OK)
					break ;
				continue ;
			}

			*j++ = *z ;
		}

		m_nTries = 0 ;
		return rc ;
	}

	threadLog("%s. Could not get directory listing\n\n", *_fn) ;
	return rc ;
}

hzEcode	hzFtpClient::FileDownload	(hzDirent& finfo)
{
	//	Downloads a file from the FTP server. This is a two-step process of firstly sending of a PASV command to open the data channel
	//	and secondly the download over the data channel. The data channel is closed after the download so this function is called for
	//	every file required.
	//
	//	Arguments:	1)	finfo	The hzDirent for the server-side file. This will have been obtained by a prior directory listing.
	//
	//	Returns:	E_HOSTFAIL	If there was a communication failure and reconnect failed
	//				E_NOTFOUND	The file on the server does not exist.
	//				E_OPENFAIL	The target file on the local machine could not be opened. Presumed fatal
	//				E_WRITEFAIL	The target file on the local machine could not be written too. Presumed fatal.
	//				E_OK		If the operation successful

	_hzfunc("hzFtpClient::FileDownload") ;

	struct utimbuf	svr_mtime ;		//	Used to alter date on files to match server version

	std::ofstream	os ;			//	Target file stream
	hzTcpClient		X ;				//	Data channel
	hzString		tmpFile ;		//	Download to a temporary file first, rename upon success
	hzString		tgtFile ;		//	Download to a temporary file first, rename upon success
	uint32_t		epoch ;			//	Time in seconds since 1970
	uint32_t		nRecv ;			//	Incoming message size
	uint32_t		nTotal = 0 ;	//	Total bytes downloaded
	uint32_t		len ;			//	Outgoing message length
	uint32_t		nTry ;			//	Local limit on reconnects
	hzEcode			rc ;			//	Return code

	//	Elimintate zero size files
	if (!finfo.Size())
		return E_OK ;

	//	X.SetDebug(_hzGlobal_Debug & HZ_DEBUG_CLIENT) ;

	tmpFile = m_LocalDir + "/" ;
	tmpFile += finfo.Name() ;
	tmpFile += ".tmp" ;

	tgtFile = m_LocalDir + "/" + finfo.Name() ;

	/*
 	**	Loop round until success
	*/

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
start:
		nTotal = 0 ;

		if (nTry == 1)
		{
			X.Close() ;
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

		/*
 		**	First step is to open a data channel
		*/

		rc = _openpasv(X) ;
		if (rc != E_OK)
		{
			threadLog("%s. Failed PASV\n", *_fn) ;
			continue ;
		}

		/*
 		**	Send the RETR command and recv response
		*/

		sprintf(m_c_sbuf, "RETR %s\r\n", *finfo.Name()) ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
		{
			threadLog("%s. Could not send RETR command to get file %s\n", *_fn, *finfo.Name()) ;
			continue ;
		}

		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Could not get RETR respeonse (file=%s)\n", *_fn, *finfo.Name()) ;
			continue ;
		}

		/*
 		**	Check response code
		*/

		if (m_nRescode >= 400)
		{
			threadLog("%s. Got bad response to RETR (%d)\n", *_fn, m_nRescode) ;
			rc = E_PROTOCOL ;
			break ;
		}

		/*
		**	Then if all is well, download the file
		*/

		os.open(*tmpFile) ;
		if (os.fail())
		{
			threadLog("%s. Failed to open download temp file %s\n", *_fn, *tmpFile) ;
			rc = E_OPENFAIL ;
			break ;
		}

		for (nTotal = 0 ; nTotal < finfo.Size() ;)
		{
			rc = X.Recv(m_x_rbuf, nRecv, HZ_MAXPACKET) ;
			if (rc != E_OK)
			{
				threadLog("%s. Socket error during download of file %s\n", *_fn, *finfo.Name()) ;
				X.Close() ;
				os.close() ;
				os.clear() ;
				goto start ;
			}

			if (!nRecv)
				break ;
			nTotal += nRecv ;

			os.write(m_x_rbuf, nRecv) ;
			if (os.fail())
			{
				threadLog("%s. Failed to write data to the target file %s\n", *_fn, *finfo.Name()) ;
				rc = E_WRITEFAIL ;
				break ;
			}
		}

		X.Close() ;
		os.close() ;

		if (rc == E_WRITEFAIL)
		{
			threadLog("%s. Aborted operation to download %d of %d bytes to the target file %s - No disk space\n",
				*_fn, nTotal, finfo.Size(), *finfo.Name()) ;
		}

		//	Initial comms phase over so exit loop
		break ;
	}

	if (rc == E_OK)
	{
		/*
 		**	Rename the file to the target name, set mtime of the local file to match that of the server
		*/

		rename(*tmpFile, *tgtFile) ;
		threadLog("%s. Downloaded %d of %d bytes to the target file %s\n", *_fn, nTotal, finfo.Size(), *finfo.Name()) ;

		epoch = finfo.Mtime() ;
		svr_mtime.actime = time(0) ;
		svr_mtime.modtime = epoch ;
		utime(*finfo.Name(), &svr_mtime) ;

		/*
		**	Get server progress report of download. If broken pipe reconnect but as we have the file already we don't
		**	have to repeat any steps.
		*/

		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Could not get response to RETR command (file=%s)\n", *_fn, *finfo.Name()) ;
			rc = _reconnect() ;
		}
		else
		{
			if (m_nRescode != 226)
				threadLog("%s. Expected code of 226 (Xfer complete), got %d\n", *_fn, m_nRescode) ;

			if (m_nRescode >= 400)
				rc = E_RECVFAIL ;
		}
	}

	if (rc != E_OK)
		threadLog("%s. Could not download %s\n\n", *_fn, *finfo.Name()) ;
	else
	{
		threadLog("%s. Downloaded %s\n", *_fn, *finfo.Name()) ;
		m_nTries = 0 ;
	}

	return rc ;
}

hzEcode	hzFtpClient::FileUpload	(const hzString& SvrFilename, const hzString& LocFilename)
{
	//	Uploads a file to the FTP server. This involves both the sending of a PASV command to open the data channel and the upload over the data
	//	channel. The data channel is closed after the upload so this function is called for every file required.
	//
	//	Arguments:	1)	SvrFilename		Name file is to be called on the server
	//				2)	LocFilename		Name file has on local machine
	//
	//	Returns:	E_NOTFOUND	Named file does not exists.
	//				E_OPENFAIL	Named file could not be opened.
	//				E_PROTOCOL	Protocol error.
	//				E_OK		Operation successful, file fully uploaded.

	_hzfunc("hzFtpClient::FileDownload") ;

	hzTcpClient	X ;				//	TCP client
	FSTAT		fs ;			//	File info
	ifstream	is ;			//	Target file stream
	uint32_t	nSize ;			//	Total bytes to upload (file size)
	uint32_t	nRecv ;			//	Bytes in server response
	uint32_t	nDone = 0 ;		//	Bytes uploaded
	uint32_t	len ;			//	Command length
	uint32_t	nTry ;			//	Reconnections
	hzEcode		rc ;			//	Return code

	/*
 	**	Because the source file is local, first thing we do is check it exists and the size
	*/

	//	X.SetDebug(_hzGlobal_Debug & HZ_DEBUG_CLIENT) ;

	if (lstat(*LocFilename, &fs) == -1)
		{ threadLog("%s. Failed to locate source file for upload %s\n", *_fn, *LocFilename) ; return E_NOTFOUND ; }
	nSize = fs.st_size ;

	rc = OpenInputStrm(is, *LocFilename, *_fn) ;
	if (rc != E_OK)
		return rc ;

	/*
 	**	Now upload the file: Loop round until success
	*/

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
		nDone = 0 ;

		if (nTry)
		{
			X.Close() ;
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

 		//	Next step is to open a data channel
		rc = _openpasv(X) ;
		if (rc != E_OK)
			{ threadLog("%s. Failed PASV ... trying again\n", *_fn) ; continue ; }

 		//	Send the STOR command and recv the response
		sprintf(m_c_sbuf, "STOR %s\r\n", *SvrFilename) ;
		len = strlen(m_c_sbuf) ;

		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
			{ threadLog("%s. Could not send STOR command to upload file %s (attempt %d of 3)\n", *_fn, *SvrFilename, nTry) ; continue ; }

		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
		{
			threadLog("%s. Could not recv STOR response (file=%s, attempt %d of 3)\n", *_fn, *SvrFilename, nTry) ;
			rc = _reconnect() ;
			continue ;
		}

 		//	Check response code
		if (m_nRescode >= 400)
			{ threadLog("%s. Got bad STOR response (%d) - aborting\n", *_fn, m_nRescode) ; rc = E_PROTOCOL  ; break ; }

		//	Then if all is well, upload the file
		for (nDone = 0 ; nDone < nSize && rc == E_OK ; nDone += is.gcount())
		{
			is.read(m_x_sbuf, HZ_MAXPACKET) ;
			if (!is.gcount())
				break ;

			rc = X.Send(m_x_sbuf, is.gcount()) ;
			if (rc != E_OK)
				threadLog("%s. No socket during upload of file %s to %s\n", *_fn, *LocFilename, *SvrFilename) ;
		}

		//	If not OK then retry
		if (rc != E_OK)
			continue ;

		//	Otherwise close channel and exit loop
		is.close() ;
		X.Close() ;
		break ;
	}

	//	Get server progress report of upload
	if (rc == E_OK)
	{
		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
			threadLog("%s. Could not get progress report to STOR command. Giving up (file=%s)\n", *_fn, *SvrFilename) ;
		else
		{
			if (m_nRescode != 226)
				{ threadLog("%s. Expected code of 226 (Xfer complete), got %d\n", *_fn, m_nRescode) ; rc = E_PROTOCOL ; }
		}
	}

	if (nDone != nSize)
	{
		threadLog("%s. File size of %d bytes, uploaded %d\n", *_fn, nSize, nDone) ;
		if (rc == E_OK)
			rc = E_SENDFAIL ;
	}

	if (rc != E_OK)
		threadLog("%s. Could not upload %s\n\n", *_fn, *LocFilename) ;
	else
	{
		threadLog("%s. Uploaded %s\n", *_fn, *LocFilename) ;
		m_nTries = 0 ;
	}

	return rc ;
}

hzEcode	hzFtpClient::FileDelete	(const hzString& SvrFilename)
{
	//	Delete a file on the server
	//
	//	Arguments:	1)	SvrFilename		Name file has on the server
	//
	//	Returns:	E_HOSTFAIL	If there was a communication failure and reconnect failed
	//				E_WRITEFAIL	If the remote file was not deleted
	//				E_OK		If the remote file was deleted

	_hzfunc("hzFtpClient::FileDelete") ;

	uint32_t	nRecv ;		//	Length of server response
	uint32_t	len ;		//	Length of client request
	uint32_t	nTry ;		//	Reconnection retry count
	hzEcode		rc ;		//	Return code

	/*
	**	Send DELE command and receive resposns, reconnect if required.
	*/

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
		if (nTry == 1)
		{
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

    	/*
		**	Send the DELE command and check the response
		*/

    	sprintf(m_c_sbuf, "DELE %s\r\n", *SvrFilename) ;
    	len = strlen(m_c_sbuf) ;

    	if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
    	{
			threadLog("%s. Could not send DELE command (for file %)\n", *_fn, *SvrFilename) ;
			continue ;
		}

    	if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
    	{
			threadLog("%s. Could not recv DELE response (for file %)\n", *_fn, *SvrFilename) ;
			continue ;
		}

		break ;
    }

	/*
 	**	Check server response
	*/

    if (m_nRescode >= 400)
    {
		threadLog("%s. Got bad response (%d) to DELE %s\n", *_fn, m_nRescode, *SvrFilename) ;
        return E_NODATA ;
	}

	return E_OK ;
}

hzEcode	hzFtpClient::FileRename	(const hzString& oldsvrname, const hzString& newsvrname)
{
	//	Rename a file on the server
	//
	//	Arguments:	1)	oldsvrname	Old name of file/directory on server
	//				2)	newname		New name of file/directory on server
	//
	//	Returns:	E_HOSTFAIL	If there was a communication failure and reconnect failed
	//				E_WRITEFAIL	If the requested server file was not renamed
	//				E_OK		If the remote file was renamed

	_hzfunc("hzFtpClient::FileRename") ;

	uint32_t	nRecv ;		//	Length of server response
	uint32_t	len ;		//	Length of client request
	uint32_t	nTry ;		//	Reconnection retry count
	hzEcode		rc ;		//	Return code

	/*
	**	Send RNFR & RNTO commands and receive resposns, reconnect if required.
	*/

	for (nTry = 0 ; nTry < 2 ; nTry++)
	{
		if (nTry == 1)
		{
			m_ConnControl.Close() ;
			rc = _reconnect() ;
			if (rc != E_OK)
				break ;
		}

    	/*
		**	Send the RNFR (rename from) command and check the response
		*/

    	//sprintf(m_c_sbuf, "RENAME %s %s\r\n", *oldsvrname, *newsvrname) ;
    	sprintf(m_c_sbuf, "RNFR %s\r\n", *oldsvrname) ;
    	len = strlen(m_c_sbuf) ;

    	if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
    	{
			threadLog("%s. Could not send RNFR command (file %s to %s)\n", *_fn, *oldsvrname, *newsvrname) ;
			continue ;
		}

    	if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
    	{
			threadLog("%s. Could not get RNFR response (file %s to %s)\n", *_fn, *oldsvrname, *newsvrname) ;
			continue ;
		}

		if (m_nRescode == 350)
		{
			/*
 			**	The RNFR was OK so now send the RNTO
			*/

			sprintf(m_c_sbuf, "RNTO %s\r\n", *newsvrname) ;
			len = strlen(m_c_sbuf) ;

    		if ((rc = m_ConnControl.Send(m_c_sbuf, len)) != E_OK)
    		{
				threadLog("%s. Could not send RNTO command (file %s to %s)\n", *_fn, *oldsvrname, *newsvrname) ;
				continue ;
			}

    		if ((rc = _ftprecv(nRecv, *_fn)) != E_OK)
    		{
				threadLog("%s. Could not get RNTO response (file %s to %s)\n", *_fn, *oldsvrname, *newsvrname) ;
				continue ;
			}
		}

		break ;
    }

	/*
 	**	Check server response
	*/

    if (m_nRescode >= 400)
    {
		threadLog("%s. Got bad response (%d) to RNFR/RNTO command (file %s to %s)\n", *_fn, m_nRescode, *oldsvrname, *newsvrname) ;
        return E_NODATA ;
	}

	return E_OK ;
}

hzEcode	hzFtpClient::QuitSession	(void)
{
	//	Sends a QUIT command to the FTP server to end the session
	//
	//	Arguments:	None
	//
	//	Returns:	E_SENDFAIL	If the command could not be sent
	//				E_RECVFAIL	If the server failed to acknowledge the command
	//				E_OK		If the operation was succussful

	uint32_t	nRecv ;		//	Length of server response
	uint32_t	len ;		//	Length of client request
	hzEcode		rc ;		//	Return code

	threadLog("hzFtpClient::QuitSession. Quitting session: - ") ;

	sprintf(m_c_sbuf, "QUIT\r\n") ;
	len = strlen(m_c_sbuf) ;

	rc = m_ConnControl.Send(m_c_sbuf, len) ;
	if (rc == E_TIMEOUT)
		threadLog("Timed out whilst sending QUIT command\n") ;
	else if (rc != E_OK)
		threadLog("Could not send QUIT command\n") ;
	else
	{
		rc = m_ConnControl.Recv(m_c_sbuf, nRecv, 1024) ;

		if (rc == E_TIMEOUT)
			threadLog("Timed out whilst awaiting QUIT response\n") ;
		else if (rc != E_OK)
			threadLog("Could not recv QUIT response\n") ;
		else
			threadLog("FTP Quit Successful\n") ;
	}

	m_ConnControl.Close() ;
	return E_OK ;
}

/*
**	Section 2:	Management of mass downloads from a known FTP host
*/

hzEcode	hzFtpHost::_hz_FtpDnhist::Load	(hzMapS<hzString,hzDirent>& dlist, hzString& opdir)
{
	//	If no history file create one, open it for writing and return. If there is read it in and then open it for appending.
	//
	//	Arguments:	1)	dlist	Directory listing
	//				2)	opdir	Operational directory
	//
	//	Returns:	E_ARGUMENT	If the operational directory is not supplied
	//				E_OPENFAIL	If the history file cannot be opened for reading
	//				E_OK		If the history file has been created afresh or read in

	FSTAT		fs ;		//	Local file status
	ifstream	is ;		//	Input stream
	hzDirent	de ;		//	Directory entry to be inserted
	hzString	hname ;		//	Name of history file
	hzString	fname ;		//	Name of downloaded file
	uint32_t	epoch ;		//	Recorded file epoch
	uint32_t	size ;		//	Recorded file size

	hname = opdir + "/histdn.dat" ;

	if (stat(*hname, &fs) == -1)
	{
		_file = fopen(*hname, "a") ;
		return E_OK ;
	}

	is.open(*hname) ;
	if (is.fail())
		return E_OPENFAIL ;

	for (;;)
	{
		is.getline(buf, 255) ;
		if (!is.gcount())
			break ;

		buf[10] = buf[21] = 0 ;
		IsPosint(epoch, buf) ;
		IsPosint(size, buf + 11) ;
		fname = buf + 22 ;

		de.InitNorm(0, fname, -1, size, epoch, epoch, 0777, 0, 0, 1) ;
		dlist.Insert(fname, de) ;
	}

	is.close() ;

	_file = fopen(*hname, "a") ;
	return E_OK ;
}

hzEcode	hzFtpHost::_hz_FtpDnhist::Append	(uint32_t epoch, uint32_t size, const hzString& name)
{
	//	Append the download history with file data
	//
	//	Arguments:	1)	epoch	Epoch time
	//				2)	size	Size of file
	//				3)	name	Filename
	//
	//	Returns:	E_ARGUMENT	If the filename is not supplied
	//				E_WRITEFAIL	If the file to be appeneded has not been opened
	//				E_OK		If the history file was appended

	if (!name)
		return E_ARGUMENT ;
	if (!_file)
		return E_WRITEFAIL ;

	fprintf(_file, "%010u %010d %s\n", epoch, size, *name) ;
	return E_OK ;
}

hzEcode	hzFtpHost::Init	(	const hzString&	host,
							const hzString&	user,
							const hzString&	pass,
							const hzString&	remDir,
							const hzString&	locDir,
							const hzString&	criteria	)
{
	//	Initialize an FTP host or account. This means setting up all the parameters for connecting to an FTP server
	//
	//	Arguments:	1)	host		The host name
	//				2)	user		The username for the account
	//				3)	pass		The password to the account
	//				4)	remDir		The opening directory on the server (usually /)
	//				5)	locDir		The opening directory on the local machine
	//				6)	criteria	Opening search criteria for listing (if any)
	//
	//	Returns:	E_ARGUMENT	If either the hostname, username, password or local directory is not supplied
	//				E_NOCREATE	If the local directory does not exist and could not be created
	//				E_OK		If the operation was successful

	_hzfunc("hzFtpSite::Initialize") ;

	hzEcode	rc = E_OK ;		//	Return code

	/*
	**	Check we have an action, that the action is associated with a publication and that it has an issue date
	*/

	m_Host = host ;
	m_Username = user ;
	m_Password = pass ;
	m_Source = remDir ;
	m_Repos = locDir ;
	m_Criteria = criteria ;

	if (!host || !user || !pass || !locDir)
	{
		if (!host)		threadLog("%s. No FTP host supplied\n", *_fn) ;
		if (!user)		threadLog("%s. No FTP username supplied\n", *_fn) ;
		if (!pass)		threadLog("%s. No FTP password supplied\n", *_fn) ;
		if (!locDir)	threadLog("%s. No FTP local directory supplied\n", *_fn) ;

		return E_ARGUMENT ;
	}

	/*
	**	Formulate directory to collect files from and assert that directory
	*/

	rc = AssertDir(*m_Repos, 0755) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_WARNING, rc, "Failed to assert local dir (%s)\n", *m_Repos) ;

	m_bInit = true ;
	return rc ;
}

hzEcode	hzFtpHost::GetAll	(void)
{
	//	Connect to FTP Server with username and password. Go to remote pre-determined directory and obtain listing according to given criteria. For each file in the
	//	listing, check if we do not already have the (complete) file from a previous run. If not, download it to the target directory and then record this action.
	//
	//	Arguments:	None
	//
	//	Returns:	E_HOSTFAIL	If the collection is cut off and cannot be re-established within an acceptable period.
	//				E_PROTOCOL	If the action fails for reasons of protocol.
	//				E_OK		If the collection runs to completion

	_hzfunc("hzFtpSite::GetAll") ;

	hzMapS	<hzString,hzDirent>	mapFilesCurr ;	//	Files already downloaded for the current issue (in name order)
	hzVect	<hzDirent>			vecFilesCurr ;	//	Files already downloaded for the current issue

	hzVect	<hzDirent>	listing ;	//	The list of files

	hzFtpClient	ftp ;				//	The FTP client connection
	hzDirent	de ;				//	Directory entry
	hzDirent	de_a ;				//	Directory entry of file we have already (if applicable)

	hzString	localfile ;			//	Target file
	hzString	rento ;				//	Rename server file to ...
	uint32_t	nIndex ;			//	Directory entry iterator
	uint32_t	nAlready = 0 ;		//	When a file is skipped (it is in the listing for the criteria but we already have it), we
									//	increment this value.

	uint32_t	nDnloads = 0 ;		//	Number of files downloaded (this attempt).

	uint32_t	nListed = 0 ;		//	Set from the count of server files meeting the criteria. At the end we add up nAlready and
									//	nDnloads and only when these add up to nListed can the status be set to ACTION_COLLECTED
									//	otherwise status should be ACTION_STARTED

	int32_t		sys_rc ;			//	Return from system call
	char*		cmd ;				//	For system call command when needed
	hzEcode		rc = E_OK ;			//	Return from FTP functions

	/*
 	**	Check we have everything we need: The publication and the olddata and newdata dirs
	*/

	if (!m_bInit)
		Fatal("Permenant Failure: No FTP Parameters\n") ;

	threadLog("%s. Collecting from server [%s,%s,(%s)] (user=%s, pass=%s) to dir %s ->",
		*_fn, *m_Host, *m_Source, *m_Criteria, *m_Username, *m_Password, *m_Repos) ;
	threadLog("OK GO!\n") ;

 	//	Get a combined listing of the historic and current data input directories
	dnh.Load(mapFilesCurr, m_Repos) ;

	rc = ReadDir(vecFilesCurr, *m_Repos) ;
	if (rc != E_OK)
	{
		threadLog("%s. Failed to read local dir (%s)\n", *_fn, *m_Repos) ;
		return rc ;
	}

	for (nIndex = 0 ; nIndex < vecFilesCurr.Count() ; nIndex++)
	{
		de = vecFilesCurr[nIndex] ;
		mapFilesCurr.Insert(de.Name(), de) ;
	}

 	//	Set up FTP params
	rc = ftp.Initialize(m_Host, m_Username, m_Password) ;
	if (rc != E_OK)
	{
		threadLog("%s. Could not INIT FTP session to server [%s] (user=%s, pass=%s). Error is %s\n",
			*_fn, *m_Host, *m_Username, *m_Password, Err2Txt(rc)) ;
		return rc ;
	}

 	//	Start FTP session
	rc = ftp.StartSession() ;
	if (rc != E_OK)
	{
		threadLog("%s. Could not start FTP session to server [%s] (user=%s, pass=%s). Error is %s\n",
			*_fn, *m_Host, *m_Username, *m_Password, Err2Txt(rc)) ;
		return rc ;
	}

 	//	Go to the relevent remote directory
	rc = ftp.SetRemoteDir(m_Source) ;
	if (rc != E_OK)
	{
		if (rc == E_NOTFOUND)
		{
			threadLog("%s. Permenant Failure: Expected source directory %s has not been found on the server\n", *_fn, *m_Source) ;
			ftp.QuitSession() ;
			return rc ;
		}

		threadLog("%s. Temporary error: Could not CD to %s\n", *_fn, *m_Source) ;
		return rc ;
	}

	rc = ftp.SetLocalDir(m_Repos) ;
	if (rc != E_OK)
	{
		threadLog("%s. Permenant Failure: Expected target directory %s has not been found or is otherwise in error on the server\n",
			*_fn, *m_Repos) ;
		ftp.QuitSession() ;
		return rc ;
	}

 	//	Get the server directory listing
	rc = ftp.GetDirList(listing, m_Criteria) ;
	if (rc != E_OK)
	{
		if (rc == E_PROTOCOL)
		{
			threadLog("%s. Permanent error: Could not get a listing. Protocol error\n", *_fn) ;
			return rc ;
		}

		threadLog("%s. We have %d files in server listing but an error of %s - will rerun\n", *_fn, listing.Count(), Err2Txt(rc)) ;
		return rc ;
	}

 	//	Got listing but is there anything in it
	if (listing.Count() == 0)
	{
		//	Expected files were not there so try again later
		threadLog("%s. Temporary error: Got directory listing but there were no files matching [%s]\n", *_fn, *m_Criteria) ;
		ftp.QuitSession() ;
		return rc ;
	}

 	//	Compare files and download new ones
	nListed = listing.Count() ;

	for (nIndex = 0 ; nIndex < listing.Count() ; nIndex++)
	{
		de = listing[nIndex] ;

		if (!mapFilesCurr.Exists(de.Name()))
			threadLog("%s. Downloading file (%d of %d) %s\n", *_fn, nIndex, listing.Count(), *de.Name()) ;
		else
		{
			de_a = mapFilesCurr[de.Name()] ;

			if (de_a.Size() == de.Size())
			{
				if (de_a.Mtime() >= de.Mtime())
				{
					threadLog("%s. Skiping file (%d of %d) %s\n", *_fn, nIndex, listing.Count(), *de.Name()) ;
					nAlready++ ;
					continue ;
				}
			}

			threadLog("%s. Re-fetching file (%d of %d) %s\n", *_fn, nIndex, listing.Count(), *de.Name()) ;
		}

		if (!de.Name())
		{
			threadLog("%s. Critical error: An Un-named meta file has been found in the listing. Corruption suspected\n", *_fn) ;
			ftp.QuitSession() ;
			return rc ;
		}

		if (!de.Size())
			continue ;

		//	We have a new file so fetch
		localfile = m_Repos + "/" + de.Name() ;

		rc = ftp.FileDownload(de) ;
		if (rc != E_OK)
		{
			if (rc == E_OPENFAIL || rc == E_WRITEFAIL)
			{
				threadLog("%s. Problem opening/writing file (%s) (error=%s)\n", *_fn, *de.Name(), Err2Txt(rc)) ;
				return rc ;
			}

			ftp.QuitSession() ;
			return rc ;
		}

		//	The file is OK - Add it to download history
		dnh.Append(de.Mtime(), de.Size(), de.Name()) ;
		nDnloads++ ;

		//	Deal with case where file is zipped
		if (strstr(*de.Name(), ".zip"))
		{
			cmd = new char[300] ;

			sprintf(cmd, "/usr/bin/unzip -q -j -n -d %s %s", *m_Repos, *localfile) ;
			sys_rc = system(cmd) ;

			if (sys_rc)
			{
				threadLog("%s. System call [%s] returns %d\n", *_fn, cmd, sys_rc) ;
				delete cmd ;
				return rc ;
			}

			threadLog("%s. System call [%s] returns OK\n", *_fn, cmd) ;
			unlink(*localfile) ;
			delete cmd ;
		}
	}

	//	Quit FTP session, return to original dir and finish.
	ftp.QuitSession() ;

	if ((nDnloads + nAlready) == nListed)
	{
		rc = ReadDir(vecFilesCurr, *m_Repos) ;
		if (rc != E_OK)
		{
			threadLog("%s. Failed to read target dir (%s) after download - delaying for 15 mins\n", *_fn, *m_Repos) ;
			return rc ;
		}
		else
		{
			hzXDate		now ;		//	System date and time
			uint32_t	eHi ;		//	Latest file found
			uint32_t	eIs ;		//	System date and time as epoch

			now.SysDateTime() ;
			eIs = now.AsEpoch() ;
			eHi = 0 ;

			for (nIndex = 0 ; nIndex < vecFilesCurr.Count() ; nIndex++)
			{
				de = vecFilesCurr[nIndex] ;

				if (eHi < de.Mtime())
					eHi = de.Mtime() ;
			}

			if (eHi > (eIs - 600))
				threadLog("%s. Oldest file less than 10 mins old - delaying just in case\n", *_fn) ;
		}
	}
	else
	{
		threadLog("%s. Total of %d listed on server. We have %d already and %d were downloaded. %d files remain outstanding\n",
			*_fn, nListed, nAlready, nDnloads, (nListed - nAlready - nDnloads)) ;
	}

	return rc ;
}
