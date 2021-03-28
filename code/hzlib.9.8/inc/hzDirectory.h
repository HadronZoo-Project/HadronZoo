//
//	File:	hzDirectory.h
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

#ifndef hzDirectory_h
#define hzDirectory_h

//	System includes
#include <sys/stat.h>

//	Other HadronZoo includes

#include "hzChars.h"
#include "hzDate.h"
#include "hzChain.h"
#include "hzCodec.h"
#include "hzCtmpls.h"

/*
**	Definitions
*/

#define ISDIR	S_ISDIR

typedef	struct stat	FSTAT ;

/*
**	Control flags for directories & files for a filesystem state compare regime
*/

//	Status flags
#define	FSI_INERT	0x0000		//	File or dir exists in both current and previous snapshot and has not changed.
#define	FSI_MODIFY	0x0001		//	File or dir exists in both current and previous snapshot but has changed.
#define	FSI_CREATE	0x0002		//	File or dir exists only in the current snapshot and is thus new.
#define	FSI_DELETE	0x0004		//	File or dir exists only in the previous snapshot and is thus deprecated.

class	hzDirent
{
	//	Category:	Filesystem
	//
	//	Directory entry. Either file or directory

	uint64_t	m_Size ;		//	Size in bytes (assuming 64 bit OS)
	hzString	m_parent ;		//	The parent directory
	hzString	m_Name ;		//	This will be full path if this is a dir, only the name if a file
	uint32_t	m_Ctime ;		//	Date & time when file created
	uint32_t	m_Mtime ;		//	Date & time of last modification
	uint32_t	m_Inode ;		//	Inode value
	uint32_t	m_Mode ;		//	UNIX Permissions
	uint16_t	m_Owner ;		//	User Id of owner (unused under Windows)
	uint16_t	m_Group ;		//	Group Id of owner (unused under Windows)
	uint16_t	m_Links ;		//	Number of hard links
	uint16_t	m_Status ;		//	Appliction assigned value (eg version)

public:
	//hzList<hzDirent*>	m_Kinder ;	//	Subdirectories or files

	hzMD5	m_Parity ;		//	Paritiy value (files only, set by app rather then read dir funcs)

	hzDirent	(void)
	{
		m_Size = 0 ;
		m_Inode = m_Ctime = m_Mtime = m_Mode = 0 ;
		m_Owner = m_Group = m_Links = m_Status = 0 ;
	}

	~hzDirent	(void)
	{
		//m_Kinder.Clear() ;
	}

	//	Set functions

	void	InitStat	(const hzString& pardir, const hzString& name, FSTAT& fs) ;

	void	InitNorm	(	const hzString&	pardir,
							const hzString&	name,
							uint64_t	size,
							uint32_t	ino,
							uint32_t	ctime,
							uint32_t	mtime,
							uint32_t	mode,
							uint32_t	uid,
							uint32_t	gid,
							uint32_t	nlink) ;

	//	Set functions
	//	void	AddChild	(hzDirent* de)		{ if (de) m_Kinder.Add(de) ; }
	void	Name		(const char* name)	{ m_Name = name ; }
	void	Name		(hzString& name)	{ m_Name = name ; }
	void	Ctime		(hzXDate& d)		{ m_Ctime = d.AsEpoch() ; }
	void	Mtime		(hzXDate& d)		{ m_Mtime = d.AsEpoch() ; }
	void	Ctime		(uint32_t ctime)	{ m_Ctime = ctime ; }
	void	Mtime		(uint32_t mtime)	{ m_Mtime = mtime ; }
	void	Size		(uint32_t nSize)	{ m_Size = nSize ; }

	//	Get functions

	//const hzDirent*	Parent	(void) const	{ return parent ; }
	const hzString&	Name	(void) const	{ return m_Name ; }

	const hzString&	Path		(void) const ;
	hzString		Pathname	(void) const ;

	uint32_t	Ctime	(void) const	{ return m_Ctime ; }
	uint32_t	Mtime	(void) const	{ return m_Mtime ; }
	uint32_t	Inode	(void) const	{ return m_Inode ; }
	uint32_t	Mode	(void) const	{ return m_Mode ; }
	uint32_t	Size	(void) const	{ return m_Size ; }
	uint16_t	Owner	(void) const	{ return m_Owner ; }
	uint16_t	Group	(void) const	{ return m_Group ; }
	uint16_t	Nlink	(void) const	{ return m_Links ; }

	bool	IsDir		(void) const	{ return ISDIR(m_Mode) ? true : false ; }

	void	MkInert		(void)	{ m_Status = 0 ; }
	void	MkModify	(void)	{ m_Status = FSI_MODIFY ; }
	void	MkCreate	(void)	{ m_Status = FSI_CREATE ; }
	void	MkDelete	(void)	{ m_Status = FSI_DELETE ; }

	bool	IsInert		(void)	{ return m_Status ? false : true ; }
	bool	IsModify	(void)	{ return m_Status & FSI_MODIFY ? true : false ; }
	bool	IsCreate	(void)	{ return m_Status & FSI_CREATE ? true : false ; }
	bool	IsDelete	(void)	{ return m_Status & FSI_DELETE ? true : false ; }
	bool	IsChanged	(void)	{ return m_Status & (FSI_MODIFY | FSI_CREATE) ? true : false ; }

	//	Operators

	hzDirent&	operator=	(const hzDirent& op) ;

	bool	operator<	(const hzDirent& op) const ;
	bool	operator>	(const hzDirent& op) const ;
	bool	operator==	(const hzDirent& op) const ;
} ;

class	hzFileset
{
	//	Category:	Filesystem
	//
	//	The hzFileset (file set) class represents a snapshot of a 'file system subset'. The aim of hzFileset is to keep track of a list of directories and files
	//	that have been deemed by an application to require some sort of management process. Examples could be backup automation or managing an install process.
	//
	//	The data (the list of directories and files), is held as a one to one map of pathnames to directories and a many to one map of filenames to files. It is
	//	not held as a tree as one might expect.
	//
	//	The hzFileset can be populated by importing a ready made directory/file list, or it can scan the file system. The scan is driven by a set of roots. Each
	//	root forms the starting point for a hierarchical scan.
	//
	//	The directories and files are held as hzDirent instances meaning that MD5 values and inode values are available to the application.

	hzChain		m_Error ;		//	Report errors during scan
	uint32_t	m_nBlocs ;		//	No of block devices
	uint32_t	m_nChars ;		//	No of char devices
	uint32_t	m_nLinks ;		//	No of links
	uint32_t	m_nSocks ;		//	No of open sockets
	uint64_t	m_nBytes ;		//	Total bytes in all files on all disks

	hzEcode	_scan_r	(hzDirent*, hzString& curdir) ;
	void	_clear	(void) ;

public:
	hzMapS<hzString,hzDirent*>	m_dirs ;	//	Index of directories by name
	hzMapM<hzString,hzDirent*>	m_file ;	//	Index of files by name

	hzVect<hzString>	m_Roots ;			//	Root directories

	hzFileset	(void)
	{
		m_dirs.SetDefaultObj((hzDirent*)0) ;
		m_file.SetDefaultObj((hzDirent*)0) ;
		_clear() ;
	}

	~hzFileset	(void)
	{
		_clear() ;
	}

	hzEcode	AddRoot	(hzString& pathname)
	{
		if (pathname)
			m_Roots.Add(pathname) ;
	}

	hzEcode	Scan	(void) ;
	hzEcode	Import	(const char* cpFilename) ;
	hzEcode	Export	(const char* cpFilename) ;

	hzChain&	Error	(void)	{ return m_Error ; }

	uint32_t	TotalDirs	(void) const	{ return m_dirs.Count() ; }
	uint32_t	TotalFiles	(void) const	{ return m_file.Count() ; }
	uint64_t	TotalBytes	(void) const	{ return m_nBytes ; }
} ;

/*
**	Non member prototypes
*/

hzEcode	GetCurrDir		(hzString& Dir) ;
hzEcode	GetAbsPath		(hzString& abs, const char* rel) ;
hzEcode	ReadDir			(hzVect<hzDirent>& Dirs, hzVect<hzDirent>& Files, const char* cpPath = 0, const char* cpCriteria = 0) ;
hzEcode	ReadDir			(hzVect<hzDirent>& entries, const char* cpPath = 0, const char* cpCriteria = 0) ;
hzEcode	ListDir			(hzVect<hzString>& Dirs, hzVect<hzString>& Files, const char* cpPath = 0, const char* cpCriteria = 0) ;
hzEcode	BlattDir		(const char* dirname) ;
hzEcode	Filecopy		(const hzString& tgt, const hzString& src) ;
hzEcode	Dircopy			(const hzString& tgt, const hzString& src, bool bRecurse) ;
hzEcode	Filemove		(const hzString& tgt, const hzString& src) ;

//	FindfilesStd (find files strictly conforming to the supplied criteria)
hzEcode	FindfilesStd	(hzVect<hzString>& files, const char* criteria) ;

//	FindfilesTar (find files according to the tar interpretation of the supplied criteria)
hzEcode	FindfilesTar	(hzVect<hzString>& files, const char* criteria) ;

hzEcode AssertDir		(const char* cpDir, uint32_t nPerms) ;
hzEcode	DirExists		(const char* dirname) ;
hzEcode	TestFile		(const char* pathname) ;
hzEcode	OpenInputStrm	(std::ifstream& is, const char* pathname, const char* callfn = 0) ;

#endif	//	hzDirectory_h
