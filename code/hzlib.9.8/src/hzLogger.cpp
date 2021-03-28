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
//	Implimentation of the hzLogger class.
//

#include <iostream>

#include <stdarg.h>

#include "hzChars.h"
#include "hzChain.h"
#include "hzIpServer.h"
#include "hzTcpClient.h"
#include "hzIpaddr.h"
#include "hzProcess.h"

/*
**	Variables
*/

static	hzString	s_LocalHost = "127.0.0.1" ;		//	This server IP address

/*
**	hzLogger functios
*/

hzLogger::hzLogger	(void)
{
	//	hzLogger constructor. Initialize logger, and set the logger for the thread to this logger.

	m_pFile = 0 ;
	m_pDataPtr = m_cvData + 11 ;
	m_nIndent = 0 ;
	m_nSessID = 0 ;
	m_pConnection = 0 ;
	m_bVerbose = false ;

	//	Set the logger for the calling thread to be this locker
	SetThreadLogger(this) ;
}

hzLogger::~hzLogger	(void)
{
	//	hzLogger destructor. There is currently no de-registration facility.
}

hzEcode	hzLogger::OpenFile	(const char* fpath, hzLogRotate eRotate)
{
	//	Purpose:	Open a LogChannel to use a file regime
	//
	//	Arguments:	1)	fpath		The logfile's base name or path
	//				2)	eRotate		The log rotate edict
	//
	//	Returns:	E_ARGUMENT	If no file path is supplied
	//				E_INITDUP	If logger already open
	//				E_OK		If channel opened OK

	_hzfunc("hzLogger::OpenFile") ;

	char	cvDir	[HZ_MAXPATHLEN] ;	//	Current working directory
	char*	cpCWD ;						//	Path iterator

	if (!fpath || !fpath[0])
		hzexit(_fn, 0, E_ARGUMENT, "No path supplied") ;

	if (IsOpen())
		hzexit(_fn, fpath, E_INITDUP, "This log channel is already open") ;

	if (fpath[0] == CHAR_FWSLASH)
		m_Base = fpath ;
	else
	{
		cpCWD = getcwd(cvDir, HZ_MAXPATHLEN) ;
		for (; *cpCWD ; cpCWD++) ;
		*cpCWD++ = CHAR_FWSLASH ;
		strcpy(cpCWD, fpath) ;
		m_Base = cvDir ;
	}

	//	Set this to non zero to indicate channel open
	m_nSessID = 0 ;

	//	Set rotate edict
	m_eRotate = eRotate ;

	//	Set data pointer
	m_pDataPtr = m_cvData ;

	return E_OK ;
}

hzEcode	hzLogger::OpenPublic	(const char* fpath, hzLogRotate eRotate)
{
	//	Purpose:	Open a LogChannel to use a public logfile
	//
	//	Arguments:	1)	fpath	The logfile's publicly known path which must be in /etc/logserver.d/public_logs.conf
	//				2)	eRotate	The log rotate edict
	//
	//	Returns:	E_HOSTFAIL	If no connection to the logserver could be established
	//				E_SENDFAIL	If the open command could not be sent to the logserver
	//				E_RECVFAIL	If the logserver's response could not be received
	//				E_WRITEFAIL If the logserver fails to find the named public logfile
	//				E_PROTOCOL	If the uid of the calling process is not permitted to write to the file
	//				E_OK		If the logserver has opened a channel to the named public logfile

	_hzfunc("hzLogger::OpenPublic") ;

	uint32_t	nSize ;		//	Size of logserver client request
	uint32_t	nUsr ;		//	UNIX user id
	uint32_t	nGrp ;		//	UNIX group id
	uint32_t	nProcID ;	//	Caller process id
	uchar*		u ;			//	Pointer into message buffer

	//	Connect to the logserver

	m_pConnection = new hzTcpClient() ;

	if (m_pConnection->ConnectStd(s_LocalHost, PORT_LOGSERVER) != E_OK)
		return E_HOSTFAIL ;

	//	Prepare header

	nSize = 16 + strlen(fpath) ;
	nProcID = getpid() ;
	nUsr = geteuid() ;
	nGrp = getegid() ;

	u = (uchar*) m_cvData ;

	//	Command
	u[0] = LS_OPEN_PUB ;

	//	Size of message
	u[1] = (nSize & 0xff00) >> 8 ;
	u[2] = (nSize & 0xff) ;

	//	Process ID
	u[3] = (nProcID & 0xff000000) >> 24 ;
	u[4] = (nProcID & 0xff0000) >> 16 ;
	u[5] = (nProcID & 0xff00) >> 8 ;
	u[6] = (nProcID & 0xff) ;

	//	User ID
	u[7] = (nUsr & 0xff000000) >> 24 ;
	u[8] = (nUsr & 0xff0000) >> 16 ;
	u[9] = (nUsr & 0xff00) >> 8 ;
	u[10] = (nUsr & 0xff) ;

	//	Group ID
	u[11] = (nGrp & 0xff000000) >> 24 ;
	u[12] = (nGrp & 0xff0000) >> 16 ;
	u[13] = (nGrp & 0xff00) >> 8 ;
	u[14] = (nGrp & 0xff) ;

	//	Filename
	strncpy(m_cvData + 15, fpath, HZ_MAXPATHLEN) ;

	//	Send message and recv response

	if (m_pConnection->Send(m_cvData, nSize) != E_OK)
		return E_SENDFAIL ;

	if (m_pConnection->Recv(m_cvData, nSize, 64) != E_OK)
		return E_RECVFAIL ;

	if (nSize == 4 && m_cvData[0] == LS_OK)
	{
		m_nSessID = 0 ;
		m_nSessID |= (u[1] << 16) ;
		m_nSessID |= (u[2] << 8) ;
		m_nSessID |= u[3] ;
		return E_OK ;
	}

	if (m_cvData[0] == LS_ERR_PERM)
		return E_WRITEFAIL ;

	//	Set data pointer
	m_pDataPtr = m_cvData + 10 ;
	return E_PROTOCOL ;
}

hzEcode	hzLogger::OpenPrivate	(const char* fpath, hzLogRotate eRotate, uint32_t nPerms)
{
	//	Purpose:	Open a LogChannel to use a private logserver file
	//
	//	Arguments:	1)	fpath	The base pathname of the logfile
	//				2)	eRotate	The log rotate edict
	//				3)	nPerms	Access permissions to be applied to the file. This will be ignored by the logserver if the file already exists and the calling
	//							process has write permission.
	//
	//	Returns:	E_HOSTFAIL	If no connection to the logserver could be established
	//				E_SENDFAIL	If the open command could not be sent to the logserver
	//				E_RECVFAIL	If the logserver's response could not be received
	//				E_WRITEFAIL If the logserver fails to find the named public logfile
	//				E_PROTOCOL	If the uid of the calling process is not permitted to write to the file
	//				E_OK		If the logserver has opened a channel to the named public logfile

	_hzfunc("hzLogger::OpenPrivate") ;

	uint32_t	nSize ;		//	Size of logserver client request
	uint32_t	nUsr ;		//	UNIX user id
	uint32_t	nGrp ;		//	UNIX group id
	uint32_t	nProcID ;	//	Caller process id
	uchar*		u ;			//	Pointer into message buffer

	//	Connect to the logserver

	if (m_pConnection->ConnectStd(s_LocalHost, PORT_LOGSERVER) != E_OK)
		return E_HOSTFAIL ;

	//	Prepare header

	nSize = 21 + strlen(fpath) ;
	nProcID = getpid() ;
	nUsr = geteuid() ;
	nGrp = getegid() ;

	u = (uchar*) m_cvData ;

	//	Command
	u[ 0] = LS_OPEN_PUB ;

	//	Size of message
	u[ 1] = (nSize & 0xff00) >> 8 ;
	u[ 2] = (nSize & 0xff) ;

	//	Process ID
	u[ 3] = (nProcID & 0xff000000) >> 24 ;
	u[ 4] = (nProcID & 0xff0000) >> 16 ;
	u[ 5] = (nProcID & 0xff00) >> 8 ;
	u[ 6] = (nProcID & 0xff) ;

	//	User ID
	u[ 7] = (nUsr & 0xff000000) >> 24 ;
	u[ 8] = (nUsr & 0xff0000) >> 16 ;
	u[ 9] = (nUsr & 0xff00) >> 8 ;
	u[10] = (nUsr & 0xff) ;

	//	Group ID
	u[11] = (nGrp & 0xff000000) >> 24 ;
	u[12] = (nGrp & 0xff0000) >> 16 ;
	u[13] = (nGrp & 0xff00) >> 8 ;
	u[14] = (nGrp & 0xff) ;

	//	Permissions
	u[15] = (nPerms & 0xff000000) >> 24 ;
	u[16] = (nPerms & 0xff0000) >> 16 ;
	u[17] = (nPerms & 0xff00) >> 8 ;
	u[18] = (nPerms & 0xff) ;

	//	Rotate edict
	u[19] = eRotate ;

	//	Filename
	strncpy(m_cvData + 20, fpath, HZ_MAXPATHLEN) ;

	//	Send message and recv response

	if (m_pConnection->Send(m_cvData, nSize) != E_OK)
		return E_SENDFAIL ;

	if (m_pConnection->Recv(m_cvData, nSize, 64) != E_OK)
		return E_RECVFAIL ;

	if (nSize == 4 && m_cvData[0] == LS_OK)
	{
		m_nSessID = 0 ;
		m_nSessID |= (u[1] << 16) ;
		m_nSessID |= (u[2] << 8) ;
		m_nSessID |= u[3] ;

		//	Set data pointer
		m_pDataPtr = m_cvData + 10 ;

		return E_OK ;
	}

	return E_PROTOCOL ;
}

hzEcode	hzLogger::Close	(void)
{
	//	Close a log channel and remove it from the thread-logger table.
	//
	//	Arguments:	None
	//
	//	Returns:	E_SENDFAIL	If the open command could not be sent to the logserver
	//				E_RECVFAIL	If the logserver's response could not be received
	//				E_PROTOCOL	If the uid of the calling process is not permitted to write to the file
	//				E_OK		If the log channel was successfully closed

	_hzfunc("hzLogger::Close") ;

	uint32_t	nBytes ;	//	Size of logserver client request
	uint32_t	nProcID ;	//	Caller process id
	uchar*		u ;			//	Pointer into message buffer

	if (m_File)
	{
		//	Log channel is using a direct file

		printf("%s case 1\n", *_fn) ;
		if (m_pFile)
			fclose(m_pFile) ;
		m_pFile = 0 ;
		printf("%s case 3\n", *_fn) ;
		m_nSessID = 0 ;
		printf("%s case 4\n", *_fn) ;
		m_Base.Clear() ;
		m_File.Clear() ;
		printf("%s case 5\n", *_fn) ;
		return E_OK ;
	}
		printf("%s case 6\n", *_fn) ;

	//	Log channel is using the logserver

	u = (uchar*) m_cvData ;
	nProcID = getpid() ;

	//	Command
	u[0] = LS_STOP ;

	//	Size of message is 8 bytes
	u[1] = 0 ;
	u[2] = 8 ;

	//	Session ID
	u[3] = (m_nSessID & 0x00ff0000) >> 16 ;
	u[4] = (m_nSessID & 0x0000ff00) >> 8 ;
	u[5] = (m_nSessID & 0x000000ff) ;

	//	Proc ID (not applicable)
	u[6] = (nProcID & 0xff00) >> 8 ;
	u[7] = (nProcID & 0xff00) >> 8 ;
	u[8] = (nProcID & 0xff00) >> 8 ;
	u[9] = (nProcID & 0xff) ;
	nBytes = 10 ;

	//	Send message and receive response

	if (m_pConnection->Send(m_cvData, nBytes) != E_OK)
		return E_SENDFAIL ;

	if (m_pConnection->Recv(m_cvData, nBytes, 64) != E_OK)
		return E_RECVFAIL ;

	if (m_cvData[0] == LS_OK)
		return E_OK ;
	return E_PROTOCOL ;
}

void	hzLogger::_logrotate	(void)
{
	//	Private method to change the hzLogger output file so as to seperate logfiles into approapriate time periods.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzLogger::_logrotate") ;

	bool	bChange = false ;	//	Log rotation on/off indicator
	char	buf	[32] ;			//	Log filename buffer

	m_datCurr.SysDateTime() ;

	switch	(m_eRotate)
	{
	case LOGROTATE_NEVER:	bChange = (m_Base && m_pFile == 0) ;	break ;
	case LOGROTATE_OFLOW:	bChange = (m_Base && m_pFile == 0) ;	break ;

	case LOGROTATE_YEAR:	bChange = (m_datCurr.Year() != m_datLast.Year()) ;					break ;
	case LOGROTATE_QTR:		bChange = ((m_datCurr.Month() / 3) != (m_datLast.Month() / 3)) ;	break ;
	case LOGROTATE_MONTH:	bChange = (m_datCurr.Month() != m_datLast.Month()) ;				break ;

	case LOGROTATE_MON:		bChange = (m_datCurr.NoDays() != m_datLast.NoDays() && m_datCurr.Dow() == 0) ;	break ;
	case LOGROTATE_TUE:		bChange = (m_datCurr.NoDays() != m_datLast.NoDays() && m_datCurr.Dow() == 1) ;	break ;
	case LOGROTATE_WED:		bChange = (m_datCurr.NoDays() != m_datLast.NoDays() && m_datCurr.Dow() == 2) ;	break ;
	case LOGROTATE_THR:		bChange = (m_datCurr.NoDays() != m_datLast.NoDays() && m_datCurr.Dow() == 3) ;	break ;
	case LOGROTATE_FRI:		bChange = (m_datCurr.NoDays() != m_datLast.NoDays() && m_datCurr.Dow() == 4) ;	break ;
	case LOGROTATE_SAT:		bChange = (m_datCurr.NoDays() != m_datLast.NoDays() && m_datCurr.Dow() == 5) ;	break ;
	case LOGROTATE_SUN:		bChange = (m_datCurr.NoDays() != m_datLast.NoDays() && m_datCurr.Dow() == 6) ;	break ;

	case LOGROTATE_DAY:		bChange = (m_datCurr.NoDays() != m_datLast.NoDays()) ;				break ;
	case LOGROTATE_12HR:	bChange = ((m_datCurr.Hour() / 12) != (m_datLast.Hour() / 12)) ;	break ;
	case LOGROTATE_HOUR:	bChange = (m_datCurr.Hour() != m_datLast.Hour()) ;					break ;
	}

	if (!bChange && m_pFile == 0 && *m_Base)
		bChange = true ;

	if (bChange)
	{
		m_datLast = m_datCurr ;
		if (m_pFile)
			fclose(m_pFile) ;

		switch	(m_eRotate)
		{
		case LOGROTATE_NEVER:	sprintf(buf, ".log") ;	break ;
		case LOGROTATE_OFLOW:	sprintf(buf, ".log") ;	break ;

		case LOGROTATE_YEAR:	sprintf(buf, "_%04d.log", m_datCurr.Year()) ;								break ;
		case LOGROTATE_QTR:		sprintf(buf, "_%04dQ%d.log", m_datCurr.Year(), (m_datCurr.Month()/3) + 1) ;	break ;
		case LOGROTATE_MONTH:	sprintf(buf, "_%04d%02d.log", m_datCurr.Year(), m_datCurr.Month()) ;		break ;

		case LOGROTATE_MON:		sprintf(buf, "_%04d%02d%02dMon.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day()) ;	break ;
		case LOGROTATE_TUE:		sprintf(buf, "_%04d%02d%02dTue.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day()) ;	break ;
		case LOGROTATE_WED:		sprintf(buf, "_%04d%02d%02dWed.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day()) ;	break ;
		case LOGROTATE_THR:		sprintf(buf, "_%04d%02d%02dThr.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day()) ;	break ;
		case LOGROTATE_FRI:		sprintf(buf, "_%04d%02d%02dFri.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day()) ;	break ;
		case LOGROTATE_SAT:		sprintf(buf, "_%04d%02d%02dSat.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day()) ;	break ;
		case LOGROTATE_SUN:		sprintf(buf, "_%04d%02d%02dSun.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day()) ;	break ;
		case LOGROTATE_DAY:		sprintf(buf, "_%04d%02d%02d.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day()) ;		break ;

		case LOGROTATE_12HR:	sprintf(buf, "_%04d%02d%02d%s.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day(), m_datCurr.Hour() > 11 ? "PM" : "AM") ;
								break ;

		case LOGROTATE_HOUR:	sprintf(buf, "_%04d%02d%02d%02d.log", m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day(), m_datCurr.Hour()) ;
								break ;
		}

		m_File = m_Base ;
		m_File += buf ;

		if (m_eRotate == LOGROTATE_NEVER)
			m_pFile = fopen(*m_File, "w") ;
		else
			m_pFile = fopen(*m_File, "a") ;

		if (!m_pFile)
			Fatal("Cannot open logfile [%s] Terminating process\n", *m_File) ;
	}
}

hzEcode	hzLogger::_write	(uint32_t nBytes)
{
	//	Prinate support function to unify logfile writing to either a logfile or a virtual logfile managed by the logserver.
	//
	//	Arguments:	1)	nBytes	Number of bytes to write
	//
	//	Returns:	E_SENDFAIL	If the open command could not be sent to the logserver
	//				E_RECVFAIL	If the logserver's response could not be received
	//				E_PROTOCOL	If the uid of the calling process is not permitted to write to the file
	//				E_OK		If the log channel was successfully closed

	_hzfunc("hzLogger::_write") ;

	uint32_t	nSize ;		//	Size of logserver client request
	uint32_t	nProcID ;	//	Caller process id

	//	If log channel not open just return
	if (!IsOpen())
		return E_OK ;

	/*
	**	If log channel is using a direct file
	*/

	if (m_nSessID == 0)
	{
		if (m_bVerbose)
		{
			std::cout.write(m_pDataPtr, nBytes) ;
			fflush(stdout) ;
		}

		if (m_pFile)
		{
			fwrite(m_pDataPtr, nBytes, 1, m_pFile) ;
			fflush(m_pFile) ;
		}

		return E_OK ;
	}

	/*
	**	Log channel is using the logserver
	*/

	uchar*	u = (uchar*) m_cvData ;		//	Pointer into logserver transport buffer

	nSize = 11 + nBytes ;
	nProcID = getpid() ;

	//	Command
	u[0] = LS_LOG ;

	//	Size of message
	u[1] = (nSize & 0xff00) >> 8 ;
	u[2] = (nSize & 0xff) ;

	//	Session ID is 0 in this case
	u[3] = (m_nSessID & 0xff0000) >> 16 ;
	u[4] = (m_nSessID & 0xff00) >> 8 ;
	u[5] = (m_nSessID & 0xff) ;

	//	Process ID
	u[6] = (nProcID & 0xff00) >> 24 ;
	u[7] = (nProcID & 0xff00) >> 16 ;
	u[8] = (nProcID & 0xff00) >> 8 ;
	u[9] = (nProcID & 0xff) ;

	//	Send message

	if (m_pConnection->Send(m_cvData, nSize) != E_OK)
		return E_SENDFAIL ;

	if (m_pConnection->Recv(m_cvData, nSize, 64) != E_OK)
		return E_RECVFAIL ;

	if (m_cvData[0] == LS_OK)
		return E_OK ;
	return E_PROTOCOL ;
}

hzEcode	hzLogger::Log	(hzFuncname& funcname, const char* va_alist ...)
{
	//	Log a variable argument message to the log channel. Deprecated as the hzFuncname class is to be reviewed.
	//
	//	Arguments:	1)	funcname	The function header
	//				2)	va_alist	Variable argument format string
	//
	//	Returns:	E_SENDFAIL	If logging is to the logserver and could not be sent
	//				E_RECVFAIL	If logging is to the logserver and acknowledgemnt was not recieved
	//				E_PROTOCOL	If logging is to the logserver and said logserver did not observe protocol
	//				E_OK		If the log action was completed

	_hzfunc("hzLogger::Log") ;

	va_list		ap1 ;		//	Variable arguments list
	va_list		ap2 ;		//	Copy of variable arguments list
	const char*	fmt ;		//	Format control string
	uchar*		u ;			//	Pointer into message buffer
	uint32_t	i ;			//	Indent counter
	uint32_t	nSize ;		//	Size of logserver client request
	uint32_t	nProcID ;	//	Caller process id

	va_start(ap1, va_alist) ;
	va_copy(ap2, ap1) ;

	fmt = va_alist ;

	//	If log channel not open just return

	if (!IsOpen())
		return E_OK ;

	//	If log channel is using a direct file

	if (m_nSessID == 0)
	{
		m_Lock.Lock() ;

		_logrotate() ;

		if (m_bVerbose)
		{
			for (i = m_nIndent ; i ; i--)
				printf("\t") ;

			printf("%04d/%02d/%02d-%02d:%02d:%02d.%06d [%u] %s: ",
				m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day(), m_datCurr.Hour(), m_datCurr.Min(), m_datCurr.Sec(),
				m_datCurr.uSec(), (uint32_t) pthread_self(), *funcname) ;

			vprintf(fmt, ap1) ;
			va_end(ap1) ;
			fflush(stdout) ;
		}

		if (m_pFile)
		{
			for (i = m_nIndent ; i ; i--)
				fprintf(m_pFile, "\t") ;

			fprintf(m_pFile, "%04d/%02d/%02d-%02d:%02d:%02d.%06d [%u] %s: ",
				m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day(), m_datCurr.Hour(), m_datCurr.Min(), m_datCurr.Sec(),
				m_datCurr.uSec(), (uint32_t) pthread_self(), *funcname) ;

			vfprintf(m_pFile, fmt, ap2) ;
			va_end(ap2) ;
			fflush(m_pFile) ;
		}

		m_Lock.Unlock() ;
		return E_OK ;
	}

	//	Log channel is using the logserver

	u = (uchar*) m_cvData ;
	vsprintf(m_cvData + 10, fmt, ap1) ;

	nSize = 11 + strlen(m_cvData + 10) ;
	nProcID = getpid() ;

	//	Command
	u[0] = LS_LOG ;

	//	Size of message
	u[1] = (nSize & 0xff00) >> 8 ;
	u[2] = (nSize & 0xff) ;

	//	Session ID is 0 in this case
	u[3] = (m_nSessID & 0xff0000) >> 16 ;
	u[4] = (m_nSessID & 0xff00) >> 8 ;
	u[5] = (m_nSessID & 0xff) ;

	//	Process ID
	u[6] = (nProcID & 0xff00) >> 24 ;
	u[7] = (nProcID & 0xff00) >> 16 ;
	u[8] = (nProcID & 0xff00) >> 8 ;
	u[9] = (nProcID & 0xff) ;

	//	Send message

	if (m_pConnection->Send(m_cvData, nSize) != E_OK)
		return E_SENDFAIL ;

	if (m_pConnection->Recv(m_cvData, nSize, 64) != E_OK)
		return E_RECVFAIL ;

	if (m_cvData[0] == LS_OK)
		return E_OK ;
	return E_PROTOCOL ;
}

hzEcode	hzLogger::Record	(const char* va_alist ...)
{
	//	Log a variable argument message to the log channel pre-pending it with a microsecond date-time stamp.
	//
	//	Arguments:	1)	va_alist	Variable argument format string
	//
	//	Returns:	E_SENDFAIL	If logging is to the logserver and could not be sent
	//				E_RECVFAIL	If logging is to the logserver and acknowledgemnt was not recieved
	//				E_PROTOCOL	If logging is to the logserver and said logserver did not observe protocol
	//				E_OK		If the log action was completed

	_hzfunc("hzLogger::Record") ;

	va_list		ap1 ;		//	Variable arguments list
	va_list		ap2 ;		//	Copy of variable arguments list
	const char*	fmt ;		//	Format control string
	uchar*		u ;			//	Pointer into message buffer
	uint32_t	i ;			//	Indent counter
	uint32_t	nSize ;		//	Size of logserver client request
	uint32_t	nProcID ;	//	Caller process id

	va_start(ap1, va_alist) ;
	va_copy(ap2, ap1) ;

	fmt = va_alist ;

	//	If log channel not open just return

	if (!IsOpen())
		return E_OK ;

	//	If log channel is using a direct file

	if (m_nSessID == 0)
	{
		m_Lock.Lock() ;

		_logrotate() ;

		if (m_bVerbose)
		{
			for (i = m_nIndent ; i ; i--)
				printf("\t") ;

			printf("%04d/%02d/%02d-%02d:%02d:%02d.%06d [%u] ",
				m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day(), m_datCurr.Hour(), m_datCurr.Min(), m_datCurr.Sec(), m_datCurr.uSec(), (uint32_t) pthread_self()) ;

			vprintf(fmt, ap1) ;
			va_end(ap1) ;
			fflush(stdout) ;
		}

		if (m_pFile)
		{
			for (i = m_nIndent ; i ; i--)
				fprintf(m_pFile, "\t") ;

			fprintf(m_pFile, "%04d/%02d/%02d-%02d:%02d:%02d.%06d [%u] ",
				m_datCurr.Year(), m_datCurr.Month(), m_datCurr.Day(), m_datCurr.Hour(), m_datCurr.Min(), m_datCurr.Sec(), m_datCurr.uSec(), (uint32_t) pthread_self()) ;

			vfprintf(m_pFile, fmt, ap2) ;
			va_end(ap2) ;
			fflush(m_pFile) ;
		}

		m_Lock.Unlock() ;
		return E_OK ;
	}

	//	Log channel is using the logserver

	u = (uchar*) m_cvData ;
	vsprintf(m_cvData + 10, fmt, ap1) ;

	nSize = 11 + strlen(m_cvData + 10) ;
	nProcID = getpid() ;

	//	Command
	u[0] = LS_LOG ;

	//	Size of message
	u[1] = (nSize & 0xff00) >> 8 ;
	u[2] = (nSize & 0xff) ;

	//	Session ID is 0 in this case
	u[3] = (m_nSessID & 0xff0000) >> 16 ;
	u[4] = (m_nSessID & 0xff00) >> 8 ;
	u[5] = (m_nSessID & 0xff) ;

	//	Process ID
	u[6] = (nProcID & 0xff00) >> 24 ;
	u[7] = (nProcID & 0xff00) >> 16 ;
	u[8] = (nProcID & 0xff00) >> 8 ;
	u[9] = (nProcID & 0xff) ;

	//	Send message

	if (m_pConnection->Send(m_cvData, nSize) != E_OK)
		return E_SENDFAIL ;

	if (m_pConnection->Recv(m_cvData, nSize, 64) != E_OK)
		return E_RECVFAIL ;

	if (m_cvData[0] == LS_OK)
		return E_OK ;
	return E_PROTOCOL ;
}

hzEcode	hzLogger::Out	(const hzChain& Z)
{
	//	Writes out the supplied chain to the logfile 'as is' with no time-stamp or other supporting information.
	//
	//	Arguments:	1)	Z	Chain to output to logfile
	//
	//	Returns:	E_SENDFAIL	If logging is to the logserver and could not be sent
	//				E_RECVFAIL	If logging is to the logserver and acknowledgemnt was not recieved
	//				E_PROTOCOL	If logging is to the logserver and said logserver did not observe protocol
	//				E_OK		If the log action was completed

	_hzfunc("hzLogger::Out(hzChain)") ;

	hzChain::Iter	zi ;		//	Message chain iterator

	char*		i ;				//	For population of buffer
	uint32_t	nBytes ;		//	Number of bytes in buffer
	hzEcode		rc = E_OK ;		//	Return code

	//	m_Temp.Clear() ;

	if (Z.Size())
	{
		m_Lock.Lock() ;

		_logrotate() ;

		for (zi = Z ; rc == E_OK && !zi.eof() ;)
		{
			for (i = m_pDataPtr, nBytes = 0 ; !zi.eof() && nBytes < HZ_LOGCHUNK ; nBytes++, zi++)
				*i++ = *zi ;

			if (!nBytes)
				break ;

			if (nBytes < 0)
			{
				strcpy(m_pDataPtr, "ERROR: Cannot export zone to logfile\n") ;
				nBytes = strlen(m_pDataPtr) ;
				rc = _write(nBytes) ;
				break ;
			}

			m_pDataPtr[nBytes] = 0 ;
			rc = _write(nBytes) ;
		}

		m_Lock.Unlock() ;
	}

	return rc ;
}

hzEcode	hzLogger::Out	(const char* va_alist ...)
{
	//	Writes out the supplied character string with varags to the logfile 'as is' with no time-stamp or other supporting information.
	//
	//	Arguments:	1)	va_alist	Variable argument format string
	//
	//	Returns:	E_SENDFAIL	If logging is to the logserver and could not be sent
	//				E_RECVFAIL	If logging is to the logserver and acknowledgemnt was not recieved
	//				E_PROTOCOL	If logging is to the logserver and said logserver did not observe protocol
	//				E_OK		If the log action was completed

	_hzfunc("hzLogger::Out(...)") ;

	va_list		ap1 ;		//	Variable arguments list
	va_list		ap2 ;		//	Copy of variable arguments list
	const char*	fmt ;		//	Format control string
	uchar*		u ;			//	Pointer into message buffer
	uint32_t	i ;			//	Indent counter
	uint32_t	nSize ;		//	Size of logserver client request
	uint32_t	nProcID ;	//	Caller process id

	va_start(ap1, va_alist) ;
	va_copy(ap2, ap1) ;

	fmt = va_alist ;

	//	m_Temp.Clear() ;

	//	If log channel not open just return

	if (!IsOpen())
		return E_OK ;

	//	If log channel is using a direct file

	if (m_nSessID == 0)
	{
		m_Lock.Lock() ;
		_logrotate() ;

		if (m_bVerbose)
		{
			for (i = m_nIndent ; i ; i--)
				printf("\t") ;

			vprintf(fmt, ap1) ;
			va_end(ap1) ;
			fflush(stdout) ;
		}

		if (m_pFile)
		{
			for (i = m_nIndent ; i ; i--)
				fprintf(m_pFile, "\t") ;

			vfprintf(m_pFile, fmt, ap2) ;
			va_end(ap2) ;
			fflush(m_pFile) ;
		}

		m_Lock.Unlock() ;
		return E_OK ;
	}

	//	Log channel is using the logserver

	u = (uchar*) m_cvData ;
	vsprintf(m_cvData + 10, fmt, ap1) ;

	nSize = 11 + strlen(m_cvData + 10) ;
	nProcID = getpid() ;

	//	Command
	u[0] = LS_LOG ;

	//	Size of message
	u[1] = (nSize & 0xff00) >> 8 ;
	u[2] = (nSize & 0xff) ;

	//	Session ID is 0 in this case
	u[3] = (m_nSessID & 0xff0000) >> 16 ;
	u[4] = (m_nSessID & 0xff00) >> 8 ;
	u[5] = (m_nSessID & 0xff) ;

	//	Process ID
	u[6] = (nProcID & 0xff00) >> 24 ;
	u[7] = (nProcID & 0xff00) >> 16 ;
	u[8] = (nProcID & 0xff00) >> 8 ;
	u[9] = (nProcID & 0xff) ;

	//	Send message

	if (m_pConnection->Send(m_cvData, nSize) != E_OK)
		return E_SENDFAIL ;

	if (m_pConnection->Recv(m_cvData, nSize, 64) != E_OK)
		return E_RECVFAIL ;

	if (m_cvData[0] == LS_OK)
		return E_OK ;
	return E_PROTOCOL ;
}

hzLogger&	hzLogger::operator<<	(hzChain& Z)
{
	//	Stream operator so that an entire hzChain contents can be output to the logfile.
	//
	//	Arguments:	1)	Z	Chain to aggregate to logger
	//
	//	Returns:	Reference to this logger

	_hzfunc("hzLogger::operator<<(hzChain)") ;

	hzChain::Iter	zi ;		//	Message chain iterator

	char*		i ;				//	For population of buffer
	uint32_t	nBytes ;		//	Number of bytes in buffer
	hzEcode		rc = E_OK ;		//	Return code

	//	m_Temp.Clear() ;

	if (Z.Size())
	{
		m_Lock.Lock() ;
		_logrotate() ;

		zi = Z ;
		for (; rc == E_OK ;)
		{
			for (i = m_pDataPtr, nBytes = 0 ; !zi.eof() && nBytes < HZ_LOGCHUNK ; nBytes++, zi++)
				*i++ = *zi ;

			if (!nBytes)
				break ;

			if (nBytes < 0)
			{
				strcpy(m_pDataPtr, "ERROR: Cannot export zone to logfile\n") ;
				nBytes = strlen(m_pDataPtr) ;
				rc = _write(nBytes) ;
				break ;
			}

			m_pDataPtr[nBytes] = 0 ;
			rc = _write(nBytes) ;
		}

		m_Lock.Unlock() ;
	}

	return *this ;
}

hzLogger& hzLogger::operator<<	(hzString& S)
{
	//	Stream operator so that a hzString can be output to the logfile.
	//
	//	Arguments:	1)	S	String to aggregate to logger
	//
	//	Returns:	Reference to this logger

	_hzfunc("hzLogger::operator<<(hzString)") ;

	const char*	i ;				//	Message as null terminated string
	char*		j ;				//	Message iterator
	uint32_t	nBytes ;		//	Size of message
	hzEcode		rc = E_OK ;		//	Return code

	//	m_Temp.Clear() ;

	if (S.Length())
	{
		m_Lock.Lock() ;
		_logrotate() ;

		i = *S ;
		for (; rc == E_OK ;)
		{
			for (j = m_pDataPtr, nBytes = 0 ; *i && nBytes < HZ_LOGCHUNK ; nBytes++)
				*j++ = *i++ ;

			if (!nBytes)
				break ;

			m_pDataPtr[nBytes] = 0 ;
			rc = _write(nBytes) ;
		}

		m_Lock.Unlock() ;
	}

	return *this ;
}

hzLogger& hzLogger::operator<<	(const char* str)
{
	//	Stream operator so that a char string can be output to the logfile.
	//
	//	Arguments:	1)	str	Char string to aggregate to logger
	//
	//	Returns:	Reference to this logger

	_hzfunc("hzLogger::operator<<(cstr)") ;

	const char*	i ;				//	Message as null terminated string
	char*		j ;				//	Message iterator
	uint32_t	nBytes ;		//	Size of message
	hzEcode		rc = E_OK ;		//	Return code

	//	m_Temp.Clear() ;

	if (str && str[0])
	{
		m_Lock.Lock() ;
		_logrotate() ;

		i = str ;
		for (; rc == E_OK ;)
		{
			for (j = m_pDataPtr, nBytes = 0 ; *i && nBytes < HZ_LOGCHUNK ; nBytes++)
				*j++ = *i++ ;

			if (!nBytes)
				break ;

			m_pDataPtr[nBytes] = 0 ;
			rc = _write(nBytes) ;
		}

		m_Lock.Unlock() ;
	}

	return *this ;
}

hzEcode	threadLog	(const char* va_alist ...)
{
	//	Category:	Diagnostics
	//
	//	Send a message supplied as a character string with varargs, to the defualt log-channel of the current thread (if invoked). Note if no logger verbose mode
	//	is assumed and the message will go to stdout.
	//
	//	Arguments:	1)	va_alist	Variable args format string
	//
	//	Returns:	E_SENDFAIL	If logging is to the logserver and could not be sent
	//				E_RECVFAIL	If logging is to the logserver and acknowledgemnt was not recieved
	//				E_PROTOCOL	If logging is to the logserver and said logserver did not observe protocol
	//				E_OK		If the log action was completed

	_hzfunc(__func__) ;

	va_list		ap ;		//	Variable argument list
	hzLogger*	plog ;		//	The calling thread's logfile
	hzChain		errmsg ;	//	The log message

	//	Do the varargs
	va_start(ap, va_alist) ;
	errmsg._vainto(va_alist, ap) ;
	va_end(ap) ;

	//	Obtain the log if available
	plog = GetThreadLogger() ;
	if (!plog)
	{
        std::cout << errmsg ;
        fflush(stdout) ;
		return E_NOINIT ;
	}

	//	Do the log
	return plog->Out(errmsg) ;
}

hzEcode	threadLog	(const hzChain& msg)
{
	//	Category:	Diagnostics
	//
	//	Send a message supplied as a chain, to the defualt log-channel of the current thread (if invoked). Note if no logger verbose mode is assumed and the message
	//	will go to stdout.
	//
	//	Arguments:	1)	msg		Output message as chain
	//
	//	Returns:	E_SENDFAIL	If logging is to the logserver and could not be sent
	//				E_RECVFAIL	If logging is to the logserver and acknowledgemnt was not recieved
	//				E_PROTOCOL	If logging is to the logserver and said logserver did not observe protocol
	//				E_OK		If the log action was completed

	_hzfunc(__func__) ;

	hzLogger*	plog ;		//	The calling thread's logfile

	plog = GetThreadLogger() ;
	if (!plog)
	{
        std::cout << msg ;
        fflush(stdout) ;
		return E_NOINIT ;
	}

	return plog->Out(msg) ;
}
