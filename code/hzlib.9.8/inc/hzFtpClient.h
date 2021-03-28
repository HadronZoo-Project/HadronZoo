//
//	File:	hzFtpClient.h
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
//	Purpose:	Header for the FTP client class
//

#ifndef hzFtpClient_h
#define hzFtpClient_h

#include "hzChain.h"
#include "hzCtmpls.h"
#include "hzDirectory.h"
#include "hzTcpClient.h"

class	hzFtpClient
{
	//	Category:	Internet
	//
	//	The hzFtpClient class operates as an FTP client allowing applications to download and upload files from and to a remote server.

	struct	_ftpline
	{
		//	Used to process FTP server response (directory listings)

		hzString		m_msg ;		//	The message body
		uint32_t	m_code ;	//	The return code

		_ftpline	()
		{
			m_code = 0 ;
		}

		_ftpline&	operator=	(const _ftpline& op)	{ m_msg = op.m_msg ; m_code = op.m_code ; }
	} ;

	hzVect<_ftpline>	m_Stack ;			//	Server response stack
	hzVect<hzString>	m_Array ;			//	For processing port nos etc

	hzTcpClient m_ConnControl ;				//	TCP control client object

	hzString	m_Server ;					//	Server to connect to
	hzString	m_Username ;				//	Username
	hzString	m_Password ;				//	Password
	hzString	m_LocalDir ;				//	Current local directory
	hzString	m_ServerDir ;				//	Current remote directory
	hzIpaddr	m_SvrAddr ;					//	Server IP

	char		m_c_rbuf [HZ_MAXPACKET+4] ;	//	Control recv buffer
	char		m_c_sbuf [HZ_MAXPACKET+4] ;	//	Control send buffer
	char		m_x_rbuf [HZ_MAXPACKET+4] ;	//	Data recv buffer
	char		m_x_sbuf [HZ_MAXPACKET+4] ;	//	Data send buffer

	uint32_t	m_nRescode ;				//	Return code in server response
	uint32_t	m_nDataPort ;				//	Port for connecting for data
	uint32_t	m_nTries ;					//	Reconnection tries
	bool		m_bInit ;					//	FTP client initialized
	bool		m_bDebug ;					//	Report conversation in full rather than errors only

	hzEcode	_setmetafile	(hzDirent& meta, char* line) ;
	hzEcode	_openpasv		(hzTcpClient& x) ;
	void	_logrescode		(const char* fn) ;
	hzEcode	_ftprecv		(uint32_t& nRecv, const char* funcname) ;
	hzEcode	_reconnect		(void) ;

public:
	hzFtpClient	(void)
	{
		m_nRescode = 0 ;
		m_nDataPort = 0 ;
		m_nTries = 0 ;
		m_bInit = false ;
		m_bDebug = false ;
	}

	~hzFtpClient	(void)
	{
	}

	hzEcode Initialize	(	const hzString&	server,		//	Server to connect to
							const hzString&	username,	//	User to log in as
							const hzString&	password	//	Password to use
						)
	{
		if (!server)	return E_ARGUMENT ;
		if (!username)	return E_ARGUMENT ;
		if (!password)	return E_ARGUMENT ;

		m_Server = server ;
		m_Username = username ;
		m_Password = password ;
		m_bInit = true ;

		return E_OK ;
	}

	//void	SetLogger	(hzLogger* plog)	{ m_pLog = plog ; }
	void	SetDebug	(bool bDebug)		{ m_bDebug = bDebug ; m_ConnControl.SetDebug(bDebug) ; }

	hzEcode StartSession	(void) ;
	hzEcode GetServerDir	(void) ;

	hzEcode	SetLocalDir		(const hzString& LocDirname) ;
	hzEcode	SetRemoteDir	(const hzString& SvrDirname) ;
	hzEcode	RemoteDirCreate	(const hzString& SvrDirname) ;
	hzEcode	RemoteDirDelete	(const hzString& SvrDirname) ;
	hzEcode GetDirList		(hzVect<hzDirent>& listing, const hzString& Criteria) ;
	hzEcode FileDownload	(hzDirent& finfo) ;
	hzEcode FileUpload		(const hzString& SvrFilename, const hzString& LocFilename) ;
	hzEcode	FileDelete		(const hzString& SvrFilename) ;
	hzEcode	FileRename		(const hzString& oldsvrname, const hzString& newsvrname) ;

	hzEcode QuitSession		(void) ;
} ;

class	hzFtpHost
{
	//	Category:	Internet
	//
	//	This is a class that is purely for mass downloads from an FTP host.

	class	_hz_FtpDnhist
	{
		//	Support class - Download/Upload Controller for FTP

		FILE*	_file ;			//	For download history file
		char	buf	[256] ;		//	For processing entires

	public:
		_hz_FtpDnhist	()	{ _file = 0 ; }

		~_hz_FtpDnhist	()
		{
			if (_file)
				fclose(_file) ;
			_file = 0 ;
		}

		hzEcode	Load	(hzMapS<hzString,hzDirent>& dlist, hzString& opdir) ;
		hzEcode	Append	(uint32_t epoch, uint32_t size, const hzString& name) ;
	} ;

	hzMapS	<hzString,hzDirent>	m_Inventory ;	//	Files already downloaded in the repository

	_hz_FtpDnhist	dnh ;		//	Download history manager

	hzString	m_Host ;			//	For FTP collections only
	hzString	m_Username ;		//	Ftp username if applicable
	hzString	m_Password ;		//	Ftp password if applicable
	hzString	m_Criteria ;		//	The download criteria (none would mean all files)
	hzString	m_Source ;			//	The directory from which files are collected (on FTP server)
	hzString	m_Repos ;			//	The directory into which files are collected
	bool		m_bInit ;			//	Parameters supplied


public:
	hzSDate			m_Issue ;		//	Issue date (WHOLE publications only)

	hzFtpHost	(void)	{ m_bInit = false ; }
	~hzFtpHost	(void)	{}

	hzEcode	Init	(	const hzString&	host,
						const hzString&	user,
						const hzString&	pass,
						const hzString&	remDir,
						const hzString&	locDir,
						const hzString&	criteria	) ;

	hzEcode	GetAll	(void) ;
} ;

#endif	//	hzFtpClient_h
