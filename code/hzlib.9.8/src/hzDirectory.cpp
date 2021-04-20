//
//	File:	hzDirectory.cpp
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
//	HadronZoo Directory and file management package
//

#include <fstream>

#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzErrcode.h"
#include "hzUnixacc.h"
#include "hzDirectory.h"
#include "hzTmplList.h"
#include "hzProcess.h"

using namespace	std ;

/*
**	Definitions
*/

#define PATHSIZE	 256

/*
**	Variables
*/

static	std::ofstream	s_Output ;			//	Archive file being created
static	std::ifstream	s_Input ;			//	Archive file being unfolded

static	hzList<char*>	s_Excludes ;		//	File endings to exclude

/*
**	SECTION 1:	hzDirent functions
*/

void	hzDirent::InitStat	(const hzString& pardir, const hzString& name, FSTAT& fs)
{
	//	Initialize a directory entry from a struct stat instance. This method is best when reading the filesystem directly. Please use InitNorm if populating a
	//	hzDirent instance from a file listing or config file.
	//
	//	Arguments:	1)	dir		The host directory
	//				2)	name	Directory entry name
	//				3)	fs		The supplied struct stat
	//
	//	Returns:	None

	m_parent = pardir ;
	m_Name = name ;

	m_Inode = fs.st_ino ;
	m_Size = fs.st_size ;
	m_Ctime = fs.st_ctime ;
	m_Mtime = fs.st_mtime ;
	m_Mode = fs.st_mode ;
	m_Owner = fs.st_uid ;
	m_Group = fs.st_gid ;
	m_Links = fs.st_nlink ;
	m_Status = 0 ;
}

void	hzDirent::InitNorm	(	const hzString&	pardir,
								const hzString&	name,
								uint64_t	size,
								uint32_t	ino,
								uint32_t	ctime,
								uint32_t	mtime,
								uint32_t	mode,
								uint32_t	uid,
								uint32_t	gid,
								uint32_t	nlink	)
{
	//	Initialize a directory entry explicitly. This method is best when populating from a file listing of config file. Please use InitStat when reading from
	//	the file system.
	//
	//	Arguments:	1)	dir		The host directory
	//				2)	name	Directory entry name
	//				3)	size	File size (only applies to files)
	//				4)	ino		I-node number
	//				5)	ctime	Time created
	//				6)	mtime	Time last modified
	//				7)	mode	Access permissions
	//				8)	uid		Owner's UNIX user id
	//				9)	gid		Owner's UNIX group id
	//				10)	nlink	Number of links
	//
	//	Returns:	None

	//parent = dir ;
	m_parent = pardir ;	//dir->Path() ;
	m_Name = name ;

	m_Inode = ino ;
	m_Size = size ;
	m_Ctime = ctime ;
	m_Mtime = mtime ;
	m_Mode = mode ;
	m_Owner = uid ;
	m_Group = gid ;
	m_Links = nlink ;
	m_Status = 0 ;
}

const hzString&	hzDirent::Path	(void) const
{
	//	If this is a dir return name (the path) or if this is a file return the path of the parent dir

	if (ISDIR(m_Mode))
		return m_Name ;

	return m_parent ;
	//if (parent)
	//	return parent->m_Name ;
	//return _hz_null_hzString ;
}

hzString	hzDirent::Pathname	(void) const
{
	//	If this is a dir return name (the path). If a file return path of parent dir + name

	//hzString	S ;		//	Target hzString instance

	if (ISDIR(m_Mode))
		return m_Name ;

	return m_parent ;
	//if (!parent)
	//	return m_Name ;

	//S = parent->m_Name + "/" + m_Name ;
	//return S ;
}

hzDirent&	hzDirent::operator=	(const hzDirent& op)
{
	m_parent = op.m_parent ;
	m_Name = op.m_Name ;
	m_Ctime = op.m_Ctime ;
	m_Mtime = op.m_Mtime ;
	m_Inode = op.m_Inode ;
	m_Size = op.m_Size ;
	m_Mode = op.m_Mode ;
	m_Owner = op.m_Owner ;
	m_Group = op.m_Group ;
	m_Links = op.m_Links ;
	m_Status = op.m_Status ;
	return *this ;
}

bool	hzDirent::operator<	(const hzDirent& op) const
{
	//	Both are dirs
	if (ISDIR(m_Mode) && ISDIR(op.m_Mode))
		return m_Name < op.m_Name ? true : false ;

	//	This is dir, op is file. Return true only if this dir is lower than that of the operand. Even if equal
	//	the file has a higher value because it equates to dirname/filename (i.e. is longer)
	if (ISDIR(m_Mode))
		return m_Name < op.Path() ? true : false ;

	//	This is file, op is dir and can only be less than the op if the parent is less than the operand
	if (ISDIR(op.m_Mode))
		return Path() < op.m_Name ? true : false ;

	//	Both are files
	if (Path() < op.Path())
		return true ;
	//if (parent == op.parent && m_Name < op.m_Name)
	if (m_parent == op.m_parent && m_Name < op.m_Name)
		return true ;
	return false ;
}

bool	hzDirent::operator>	(const hzDirent& op) const
{
	//	Both are dirs
	if (ISDIR(m_Mode) && ISDIR(op.m_Mode))
		return m_Name > op.m_Name ? true : false ;

	//	This is dir, op is file. Return true only if this dir is higher than that of the operand. Even if equal
	//	the file has a higher value because it equates to dirname/filename (i.e. is longer)
	if (ISDIR(m_Mode))
		return m_Name > op.Path() ? true : false ;

	//	This is file, op is dir and can only be less than the op if the parent is less than the operand
	if (ISDIR(op.m_Mode))
		return Path() < op.m_Name ? false : true ;

	//	Both are files
	if (Path() > op.Path())
		return true ;
	//if (parent == op.parent && m_Name > op.m_Name)
	if (m_parent == op.m_parent && m_Name > op.m_Name)
		return true ;
	return false ;
}

bool	hzDirent::operator==	(const hzDirent& op) const
{
	//	If both are directories they are deemed equal simply if the names match.

	if (ISDIR(m_Mode) && ISDIR(op.m_Mode))
		return m_Name == op.m_Name ? true : false ;

	//	Cannot be equal if one is a dir and the other is a file

	if (ISDIR(m_Mode))
		return false ;
	if (ISDIR(op.m_Mode))
		return false ;

	//	Case where both are file

	if (Path() != op.Path())
		return false ;
	return m_Name == op.m_Name && m_Mtime == op.m_Mtime && m_Size == op.m_Size ? true : false ;
}

/*
**	SECTION 1:	Non member functions
*/

hzEcode	GetCurrDir	(hzString& Dir)
{
	//	Category:	Directory
	//
	//	Populate a hzString with the name of the current working directory. This function calls the GNU system function get_current_dir_name().
	//
	//	Arguments:	1)	Dir		The hzString reference to populate
	//
	//	Returns:	E_NOTFOUND	If the current directory has access issues or has been unlinked
	//				E_OK		If the operation was successful

	_hzfunc("GetCurrDir") ;

	char*	cpDir ;		//	Buffer for current working directory

	Dir.Clear() ;

	cpDir = get_current_dir_name() ;

	if (!cpDir)
	{
		if (errno == EACCES)
			return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "The current working directory has access issues") ;
		if (errno == ENOENT)
			return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "The current working directory has been uninked") ;

		return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "Unspecified error") ;
	}

	Dir = cpDir ;
	free(cpDir) ;
	return E_OK ;
}

hzEcode	GetAbsPath	(hzString& abs, const char* rel)
{
	//	Category:	Directory
	//
	//	Translate the supplied null terminate string, assumed to be an path relative to the current directory, into an absolute path. Places the result into the
	//	supplied string recepticle.
	//
	//	Note that if the relative path is not supplied the output value will be the name of the current working directory.
	//
	//	Note also that this function DOES NOT TEST if the resulting path exists or that it can be accessed.
	//
	//	Arguments:	1)	Dir		The hzString reference to populate
	//
	//	Returns:	E_NOTFOUND	If the current directory has access issues or has been unlinked
	//				E_OK		If the operation was successful

	_hzfunc("GetCurrDir") ;

	char*	cpDir ;		//	Buffer for current working directory


	//	If the supplied relative path is actually absolute, just set absolte = relative
	if (rel[0] == CHAR_FWSLASH)
		{ abs = rel ; return E_OK ; }

	//	Look for the home directory sequence
	if (rel[0] == CHAR_TILDA && rel[1] == CHAR_FWSLASH)
	{
		abs = getenv("HOME") ;
		abs += "/" ;
		abs += (rel + 2) ;
		return E_OK ;
	}

	//	Clear string
	abs.Clear() ;

	//	Get current working dir
	cpDir = get_current_dir_name() ;
	if (!cpDir)
	{
		if (errno == EACCES)
			return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "The current working directory has access issues") ;
		if (errno == ENOENT)
			return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "The current working directory has been uninked") ;

		return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "Unspecified error") ;
	}

	//	If no relative path supplied, use the current directory
	if (!rel || !rel[0])
	{
		abs = cpDir ;
		free(cpDir) ;
		return E_OK ;
	}

	char*		i ;			//	Path iterator
	char*		j ;			//	Placeholder
	uint32_t	lev ;		//	Directory level
	uint32_t	oset ;		//	Offset into path
	uint32_t	step ;		//	Number of backward steps (../)

	if (rel[0] == CHAR_PERIOD && rel[1] == CHAR_PERIOD && rel[2] == CHAR_FWSLASH)
	{
		for (lev = 0, i = cpDir ; *i ; i++)
		{
			if (*i == CHAR_FWSLASH)
				{ j = i ; lev++ ; }
		}

		//	Count the sequences of ../
		for (step = 1, oset = 3 ; rel[oset] == CHAR_PERIOD && rel[oset+1] == CHAR_PERIOD && rel[oset+2] == CHAR_FWSLASH ; step++, oset += 3) ;
		if (step > lev)
			{ free(cpDir) ; return E_BADVALUE ; }

		for (; step ; step--)
		{
			for (j-- ; i != cpDir && *j != CHAR_FWSLASH ; j--) ;
		}
		*j = 0 ;
	}

	abs = cpDir ;
	abs += "/" ;
	abs += rel ;

	free(cpDir) ;
	return E_OK ;
}

hzEcode	ReadDir	(hzVect<hzDirent>& Dirs, hzVect<hzDirent>& Files, const char* cpPath, const char* cpCriteria)
{
	//	Category:	Directory
	//
	//	Populate vectors of directory entries for the sub-directories and files in a given directory where these meet the supplied
	//	selection criteria. This function will not recurse into the sub-directories.
	//
	//	Arguments:	1)	Dirs		The vector for the sub-directories
	//				2)	Files		The vector for the files
	//				3)	cpPath		The directory to be examined (null is taken as the current working directory)
	//				4)	cpCriteria	The file selection criteria (null equates to *)
	//
	//	Returns:	E_OPENFAIL	If the directory cannot be opened
	//				E_CORRUPT	If a directory entry could not be found by the stat function
	//				E_OK		If operation successful (even if nothing is found)

	_hzfunc("ReadDir(1)") ;

	FSTAT		fs ;			//	File status
	dirent*		pDE ;			//	Diectory entry
	DIR*		pDir ;			//	Directory
	hzDirent	meta ;			//	Directory entry meta data
	hzString	thePath ;		//	The path to here
	hzString	teststr ;		//	Fullpath test value
	hzString	de_name ;		//	Directory entry name
	hzEcode		rc ;			//	Return code

	/*
	**	Move thru current directory and read files and sub directories
	*/

	Dirs.Clear() ;
	Files.Clear() ;

	rc = GetAbsPath(thePath, cpPath) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_WARNING, rc, "Could not obtain absolute path for (%s)", cpPath) ;

	if (lstat(*thePath, &fs) < 0)
		return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "No such directory or file exists (%s)", cpPath) ;

	if (!S_ISDIR(fs.st_mode))
		return hzerr(_fn, HZ_WARNING, E_TYPE, "Given path (%s) is not a directory", cpPath) ;

	pDir = opendir(*thePath) ;
	if (!pDir)
		return hzerr(_fn, HZ_WARNING, E_OPENFAIL, "Directory (%s) could not be opened", cpPath) ;

	for (; pDE = readdir(pDir) ;)
	{
		if (pDE->d_name[0] == '.' && (pDE->d_name[1] == 0 || (pDE->d_name[1] == '.' && pDE->d_name[2] == 0)))
			continue ;

		if (cpCriteria)
		{
			if (!FormCheckCstr(pDE->d_name, cpCriteria))
				continue ;
		}

		if (cpPath && cpPath[0])
		{
			teststr = cpPath ;
			teststr += "/" ;
			teststr += pDE->d_name ;
		}
		else
			teststr = pDE->d_name ;

		if (stat(*teststr, &fs) == -1)
		{
			closedir(pDir) ;
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not stat directory entry %s", *teststr) ;
		}

		if (S_ISDIR(fs.st_mode))
		{
			//	The entry is a directory
			de_name = pDE->d_name ;
			meta.InitStat(thePath, de_name, fs) ;
			Dirs.Add(meta) ;
			continue ;
		}

		if (S_ISREG(fs.st_mode))
		{
			de_name = pDE->d_name ;
			meta.InitStat(thePath, de_name, fs) ;
			Files.Add(meta) ;
			continue ;
		}
	}

	closedir(pDir) ;
	return E_OK ;
}

hzEcode	ReadDir	(hzVect<hzDirent>& entries, const char* cpPath, const char* cpCriteria)
{
	//	Category:	Directory
	//
	//	Read the directory (cpPath) and place the directory entries for the sub-directories and files, where they meet the supplied
	//	filename criteria (cpCriteria), in a single supplied vector. This function does not recurse into sub-directories.
	//
	//	Arguments:	1)	entries		The vector for the sub-directories and files
	//				2)	cpPath		The directory to be examined (null is taken as the current working directory)
	//				3)	cpCriteria	The file selection criteria (null equates to *)
	//
	//	Returns:	E_NOTFOUND	If the requested directory does not exist.
	//				E_OPENFAIL	If the requested directory could not be opened (eg insufficient permissions)
	//				E_CORRUPT	If an entry found in the directory could not subsequently be stat-ed.
	//				E_OK		The operation was successful.

	_hzfunc("ReadDir(2)") ;

	FSTAT		fs ;			//	File status
	dirent*		pDE ;			//	Directory entry
	DIR*		pDir ;			//	Directory pointer
	hzDirent	meta ;			//	Directory entry metadata
	hzString	thePath ;		//	Path of directory being read
	hzString	teststr ;		//	Test string (filenames)
	hzString	filename ;		//	Filename
	hzEcode		rc ;			//	Return code

	entries.Clear() ;

	//	Establish applicable directory
	if (cpPath && cpPath[0])
	{
		//	Directory supplied
		rc = GetAbsPath(thePath, cpPath) ;
		if (rc != E_OK)
			return hzerr(_fn, HZ_WARNING, rc, "Could not obtain absolute path for (%s)", cpPath) ;

		if (lstat(*thePath, &fs) < 0)
			return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "No such directory or file exists (%s)", *thePath) ;

		if (!S_ISDIR(fs.st_mode))
			return hzerr(_fn, HZ_WARNING, E_TYPE, "Given path (%s) is not a directory", *thePath) ;

		pDir = opendir(*thePath) ;
	}
	else
	{
		//	No directory supplied, use current
		GetCurrDir(thePath) ;
		pDir = opendir(".") ;
	}

	if (!pDir)
		return hzerr(_fn, HZ_WARNING, E_OPENFAIL, "Directory (%s) could not be opened", *thePath) ;
	
	//	Perform the directory read
	for (; pDE = readdir(pDir) ;)
	{
		if (pDE->d_name[0] == '.' && (pDE->d_name[1] == 0 || (pDE->d_name[1] == '.' && pDE->d_name[2] == 0)))
			continue ;

		if (!FormCheckCstr(pDE->d_name, cpCriteria))
			continue ;

		if (thePath)
		{
			teststr = thePath ;
			teststr += "/" ;
			teststr += pDE->d_name ;
		}
		else
			teststr = pDE->d_name ;

		if (stat(*teststr, &fs) == -1)
		{
			closedir(pDir) ;
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not stat directory entry %s", *teststr) ;
		}

		filename = pDE->d_name ;
		meta.InitStat(thePath, filename, fs) ;
		//threadLog("%s: Adding %s\n", *_fn, *teststr) ;
		entries.Add(meta) ;
	}

	closedir(pDir) ;
	return E_OK ;
}

hzEcode	ListDir	(hzVect<hzString>& Dirs, hzVect<hzString>& Files, const char* cpPath, const char* cpCriteria)
{
	//	Category:	Directory
	//
	//	Populate vectors of directory entries for the sub-directories and files in a given directory where these meet the supplied
	//	selection criteria. This function will not recurse into the sub-directories. Note that this function is essentially the same as
	//	ReadDir	(hzVect<hzDirent>& Dirs, hzVect<hzDirent>& Files, const char* cpPath, const char* cpCriteria) - except that the two vectors
	//	are of strings rather than hzDirent instances.
	//
	//	Arguments:	1)	Dirs		The vector for the sub-directory names
	//				2)	Files		The vector for the file names
	//				3)	cpPath		The directory to be examined (null is taken as the current working directory)
	//				4)	cpCriteria	The file selection criteria (null equates to *)
	//
	//	Returns:	E_OPENFAIL	If the directory cannot be opened
	//				E_CORRUPT	If a directory entry could not be found by the stat function
	//				E_OK		If operation successful (even if nothing is found)

	_hzfunc("ListDir") ;

	FSTAT		fs ;			//	File status
	dirent*		pDE ;			//	Directory entry
	DIR*		pDir ;			//	Directory pointer
	hzDirent	meta ;			//	Directory entry metadata
	hzString	teststr ;		//	Test string (filenames)
	hzString	name ;			//	Directory entry name

	Dirs.Clear() ;
	Files.Clear() ;

	/*
	**	Move thru current directory and read files and sub directories
	*/

	if (cpPath && cpPath[0])
		pDir = opendir(cpPath) ;
	else
		pDir = opendir(".") ;

	if (!pDir)
		return E_OPENFAIL ;
	
	for (; pDE = readdir(pDir) ;)
	{
		if (pDE->d_name[0] == '.' && (pDE->d_name[1] == 0 || (pDE->d_name[1] == '.' && pDE->d_name[2] == 0)))
			continue ;

		if (!FormCheckCstr(pDE->d_name, cpCriteria))
			continue ;

		if (cpPath && cpPath[0])
		{
			teststr = cpPath ;
			teststr += "/" ;
			teststr += pDE->d_name ;
		}
		else
			teststr = pDE->d_name ;

		if (stat(*teststr, &fs) == -1)
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not stat directory entry %s", *teststr) ;

		if (S_ISDIR(fs.st_mode))
		{
			name = pDE->d_name ;
			Dirs.Add(name) ;
			continue ;
		}

		if (S_ISREG(fs.st_mode))
		{
			name = pDE->d_name ;
			Files.Add(name) ;
			continue ;
		}
	}

	return E_OK ;
}

/*
**	Directory and File operations
*/

hzEcode	BlattDir	(const char* dirname)
{
	//	Category:	Directory
	//
	//	Blatts the named directory (if correct permissions). Uses recursion to delete any sub-directories
	//
	//	Arguments:	1)	dirname	The directory name (full path)
	//
	//	Returns:	E_ARGUMENT	If the directory is not supplied
	//				E_OPENFAIL	If the supplied directory could not be opened
	//				E_CORRUPT	If any directory entry cannot be stated
	//				E_WRITEFAIL	If the directory and any entry within it could not be unlinked
	//				E_OK		If the directory was successfully removed

	_hzfunc("BlattDir") ;

	FSTAT		fs ;			//	To obtain directory entry info
	dirent*		pDE ;			//	Directory entry
	DIR*		pDir ;			//	An open directory
	hzString	teststr ;		//	Full path of directory entry
	int32_t		sys_rc = 0 ;	//	Return code from std calls
	hzEcode		rc = E_OK ;

	/*
	**	Move thru current directory and read files and sub directories
	*/

	if (!dirname || !dirname[0])
		return E_ARGUMENT ;

	//threadLog("%s. Blatting dir %s\n", *_fn, dirname) ;
	
	pDir = opendir(dirname) ;
	if (!pDir)
		return E_OPENFAIL ;
	
	for (; rc == E_OK && (pDE = readdir(pDir)) ;)
	{
		if (pDE->d_name[0] == '.' && (pDE->d_name[1] == 0 || (pDE->d_name[1] == '.' && pDE->d_name[2] == 0)))
			continue ;

		teststr = dirname ;
		teststr += "/" ;
		teststr += pDE->d_name ;

		//threadLog("\t%s testing entry %s\n", *_fn, *teststr) ;

		if (stat(*teststr, &fs) == -1)
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not stat directory entry %s", *teststr) ;

		if (S_ISDIR(fs.st_mode))
		{
			rc = BlattDir(*teststr) ;
			continue ;
		}

		if (S_ISREG(fs.st_mode))
		{
			sys_rc = unlink(*teststr) ;
			if (sys_rc)
				rc = E_WRITEFAIL ;
			continue ;
		}
	}

	//threadLog("%s. Closing dir %s\n", *_fn, dirname) ;
	closedir(pDir) ;

	if (rc == E_OK)
	{
		//threadLog("%s. Deleting dir %s\n", *_fn, dirname) ;
		sys_rc = rmdir(dirname) ;
		if (sys_rc)
			rc = E_WRITEFAIL ;
		//threadLog("%s - Done!\n", *_fn) ;
	}

	return rc ;
}

hzEcode		Filecopy	(const hzString& tgt, const hzString& src)
{
	//	Category:	Directory
	//
	//	Copies a single file. The source must exist, contain data and be readable. The target file will be overwritten it is already exists OR created as new if
	//	the target directory exists and the program the correct access permissions.
	//
	//	Both the source and target must be fully specified and not contain wildcard characters.
	//
	//	Arguments:	1)	tgt		The target file path.
	//				2)	src		The source file path.
	//
	//	Returns:	E_ARGUMENT	If either the source or the target filepath is NULL
	//				E_NOTFOUND	If any filepath specified does not exist as a directory entry of any kind
	//				E_TYPE		If any filepath names a directory entry that is not a file
	//				E_NODATA	If the source file is empty
	//				E_OPENFAIL	If the source file could not be opened for reading
	//				E_OK		If the operation was successful

	_hzfunc("Filecopy") ;

	ifstream	is ;			//	Input stream
	ofstream	os ;			//	Output stream
	char		buf	[1028] ;	//	Working buffer
	hzEcode		rc = E_OK ;		//	Return code

	if (!tgt)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No target filepath supplied") ;

	//	Open source for reading
	rc = OpenInputStrm(is, src, *_fn) ;
	if (rc != E_OK)
		return rc ;

	//	Open target for writing
	os.open(tgt) ;
	if (os.fail())
	{
		threadLog("%s: Could not open/create target file %s\n", *_fn, *tgt) ;
		is.close() ;
		return E_WRITEFAIL ;
	}

	for (; rc == E_OK ;)
	{
		is.read(buf, 1024) ;
		if (!is.gcount())
			break ;

		os.write(buf, is.gcount()) ;
		if (os.fail())
		{
			threadLog("%s: Could not write %d bytes to target file %s\n", *_fn, is.gcount(), *tgt) ;
			rc = E_WRITEFAIL ;
		}
	}

	is.close() ;
	os.close() ;

	threadLog("%s. File %s copied to %s\n", *_fn, *src, *tgt) ;
	return rc ;
}

hzEcode		Dircopy	(const hzString& tgt, const hzString& src, bool bRecurse)
{
	//	Category:	Directory
	//
	//	Copy a directory
	//
	//	Arguments:	1)	tgt			This must specify a directory that either exists or can be created
	//				2)	src			This must specify a directory that exists.
	//				3)	bRecurse	If set, the directory copy will be recursive.
	//
	//	Returns:	E_NOTFOUND	If the specified source directory cannot be found.
	//				E_TYPE		If the either the specified source or target is not a directory.
	//				E_WRITEFAIL	If the target directory cannot be asserted or a file could not be created or written.
	//				E_EXCLUDE	If file system permissions were the cause of (3)
	//				E_OK		If the operation was fully successful

	FSTAT	fs ;			//	Directory entry status
	hzEcode	rc = E_OK ;		//	Return code

	if (!tgt || !src)
		return E_ARGUMENT ;

	if (lstat(src, &fs) == -1)
		return E_NOTFOUND ;

	if (!S_ISDIR(fs.st_mode))
		return E_CORRUPT ;

	rc = AssertDir(tgt, (uint32_t) 0777) ;
	if (rc != E_OK)
		return rc ;

	return rc ;
}

hzEcode		Filemove	(const hzString& src, const hzString& tgt)
{
	//	Category:	Directory
	//
	//	Move a single file from the source filepath to the target filepath. This is functionally similar to the UNIX mv command or the C library function rename
	//	except that overwriting a file is not permitted.
	//
	//	Arguments:	1)	source	The source filepath
	//				2)	target	The target filepath
	//
	//	Returns:	E_ARGUMENT	If either the source or the target is not supplied
	//				E_NOTFOUND	If the source file does not exist
	//				E_DUPLICATE	If the target file does exist
	//				E_TYPE		If either the source or target is a directory
	//				E_WRITEFAIL	If the rename operation failed
	//				E_OK		If the file was moved

	_hzfunc(__func__) ;

	FSTAT		fs ;			//	File status

	if (!tgt || !src)
		return E_ARGUMENT ;

	if (lstat(src, &fs) < 0)
		return E_NOTFOUND ;

	if (S_ISDIR(fs.st_mode))
		return E_TYPE ;

	//	Check if target exists
	if (lstat(tgt, &fs) == 0)
	{
		if (S_ISDIR(fs.st_mode))
			return E_TYPE ;

		return E_DUPLICATE ;
	}

	if (rename(*src, *tgt) < 0)
		return E_WRITEFAIL ;
	return E_OK ;
}

/*
**	Section 2: File listings
**
**	Provides the application functions of:-
**	1)	hzEcode	FindfilesStd	(hzArray<hzString>& files, const char* criteria) ;
**	2)	hzEcode	FindfilesTar	(hzArray<hzString>& files, const char* criteria) ;
*/

static	hzEcode	_scanfiles_r	(hzArray<hzString>& files, hzArray<hzString>& parts, const hzString& pathsofar, uint32_t nLevel, bool bLimit)
{
	//	Category:	Directory
	//
	//	Recursive directory scan.
	//
	//	Method:		Scan a directory for files.
	//
	//	Arguments:	1)	files		Repository for files found. This is only populated here if we are on the last part of the criteria given in the 2nd argument
	//				2)	parts		The parts of the path (eg /home/username has parts of home and username)
	//				3)	Pathsofar	the directory to be read in this invokation.
	//				4)	Level		this determines if we are in the last part of the directory to be scanned.
	//				5)	Limit		If set we do not go into sub-directories of the directory to be scanned for files.
	//
	//	Returns:	E_ARGUMENT	If the pathsofar is empty or no parts are specified
	//				E_OPENFAIL	If the directory cannot be opened
	//				E_CORRUPT	If a directory entry cannot be stated
	//				E_OK		If the directory is successfully scanned

	_hzfunc("_scanfiles_r") ;

	hzVect<hzString>	levels ;	//	Partial paths
	hzVect<hzString>	dirs ;		//	Needed only for ListDir function

	FSTAT		fs ;			//	Directory entry status
	dirent*		pDE ;			//	Directory entry
	DIR*		pDir ;			//	Directory pointer
	hzString	teststr ;		//	File/dir to be tested with stat
	hzString	part ;			//	This part (of the criteria)

	//	Check arguments
	if (!pathsofar || !parts.Count())
		return E_ARGUMENT ;

	if (bLimit)
	{
		//	Search for files is limited to specified directories. The last part of the criteria will be matched only on files
		if (nLevel < 0 || nLevel >= parts.Count())
			return E_ARGUMENT ;

		part = parts[nLevel] ;

		//	Are we on the last part of the path in which the filename criteria is specified?
		if (nLevel == (parts.Count() - 1))
		{
			//	We are so we are just looking for file that match. No directories are taken into account

			pDir = opendir(*pathsofar) ;
			if (!pDir)
				return E_OPENFAIL ;
	
			for (; pDE = readdir(pDir) ;)
			{
				if (pDE->d_name[0] == '.' && (pDE->d_name[1] == 0 || (pDE->d_name[1] == '.' && pDE->d_name[2] == 0)))
					continue ;

				if (!FormCheckCstr(pDE->d_name, *part))
					continue ;

				//	Build the complete path to the file before calling stat. This is nessesary since we are not actually in the directory
				//	as we have never called chdir()
				if (pathsofar.Length() == 1)
					teststr = "/" ;
				else
					teststr = pathsofar + "/" ;
				teststr += pDE->d_name ;

				if (stat(*teststr, &fs) == -1)
				{
					closedir(pDir) ;
					return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not stat directory entry %s", *teststr) ;
				}

				//	Only add the entry if it is a file
				if (S_ISREG(fs.st_mode))
					files.Add(teststr) ;
			}

			closedir(pDir) ;
			return E_OK ;
		}

		//	We are not on the last part so olny look for directories matching the current part criteria. Then for each directory call the
		//	next level.
		pDir = opendir(*pathsofar) ;
		if (!pDir)
			return E_OPENFAIL ;
	
		for (; pDE = readdir(pDir) ;)
		{
			if (pDE->d_name[0] == '.' && (pDE->d_name[1] == 0 || (pDE->d_name[1] == '.' && pDE->d_name[2] == 0)))
				continue ;

			if (!FormCheckCstr(pDE->d_name, *part))
				continue ;

			if (pathsofar.Length() == 1)
				teststr = "/" ;
			else
				teststr = pathsofar + "/" ;
			teststr += pDE->d_name ;

			if (stat(*teststr, &fs) == -1)
			{
				closedir(pDir) ;
				return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not stat directory entry %s", *teststr) ;
			}

			//	Only call next level if the entry is a directory
			if (S_ISDIR(fs.st_mode))
				_scanfiles_r(files, parts, teststr, nLevel + 1, bLimit) ;
		}
	}
	else
	{
		//	Search for files is not limited to specified directories. The last part of the criteria will be matched on both on files
		//	and directories. Where files match this last part they are included as they are in the limited search. Where directories
		//	match this last part all thier files and all the files of all thier subdirectories are included. The purpose of this
		//	mode of operation is to facilitate a tar-like interpretation of a directory. Eg the criteria dvlp/* means the directory
		//	of dvlp (in the current dir) and every file in it and below it.

		if (nLevel > (parts.Count() - 1))
			part = parts[parts.Count() - 1] ;
		else
			part = parts[nLevel] ;

		//	We are not on the last part so olny look for directories matching the current part criteria. Then for each directory call the
		//	next level.
		pDir = opendir(*pathsofar) ;
		if (!pDir)
			return E_OPENFAIL ;
	
		for (; pDE = readdir(pDir) ;)
		{
			if (pDE->d_name[0] == '.' && (pDE->d_name[1] == 0 || (pDE->d_name[1] == '.' && pDE->d_name[2] == 0)))
				continue ;

			if (!FormCheckCstr(pDE->d_name, *part))
				continue ;

			if (pathsofar.Length() == 1)
				teststr = "/" ;
			else
				teststr = pathsofar + "/" ;
			teststr += pDE->d_name ;

			if (stat(*teststr, &fs) == -1)
			{
				closedir(pDir) ;
				return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not stat directory entry %s", *teststr) ;
			}

			//	Only call next level if the entry is a directory
			if (S_ISDIR(fs.st_mode))
				_scanfiles_r(files, parts, teststr, nLevel + 1, bLimit) ;

			if (S_ISREG(fs.st_mode))
			{
				//	If on last part or above add the files
				if (nLevel >= (parts.Count() - 1))
					files.Add(teststr) ;
			}
		}
	}

	closedir(pDir) ;
	return E_OK ;
}

static	hzEcode	_scanfiles	(hzArray<hzString>& files, const char* criteria, bool bLimit)
{
	//	Category:	Directory
	//
	//	This finds all files matching the supplied criteria and starting from the supplied root directory. The relative
	//	paths [to the root] of the files found will populate the supplied vector (arg 1)
	//
	//	Arguments:	1)	files		The vector of files found
	//				2)	criteria	The scaning criteria
	//				3)	bLimit		Boolean limit
	//
	//	Returns:	E_ARGUMENT	If the pathsofar is empty or no parts are specified
	//				E_OPENFAIL	If the directory cannot be opened
	//				E_CORRUPT	If a directory entry cannot be stated
	//				E_OK		If the directory is successfully scanned
	//
	//	This function uses recursion to read all the directories implied by the criteria. The criteria is split by the forward slash
	//	and each part is examined in turn. Those parts occuring before the last forward slash are called directory parts and the part
	//	occuring after the last forward slash is the file part.
	//
	//	If the criteria begins with a forward slash the root directory (from which the process starts) is set to the filesystem root
	//	(/). The criteria is then advanced one place before being split.
	//
	//	If the criteria begins with a ../ sequence we take note of the current directory and step back one place. If there are multiple
	//	../ sequences we step back multiple times. The ./ sequence is ignored.
	//
	//	If the criteria begins with any other sequence this is assumed to be a part that will be applied to the contents of the current
	//	directory.
	//
	//	If bLimit is true then the search for files will be limited to files matching the last part in the directories starting from
	//	the root and matching all the previous parts. But if false the files found will include the above but if the last part also
	//	matches to directories these will be examined as well. False is the default so beware!

	hzArray<hzString>	parts ;		//	Parts of full path

	hzChain			rootVal ;		//	For building the path so far
	const char*		i ;				//	Pathname iterator
	hzString		rootDir ;		//	The path so far
	hzString		cwd ;			//	Current working dir
	hzString		root ;			//	Root (rebuilt directory path)
	hzString		curr ;			//	Current working directory excluding leading slash
	hzString		crit ;			//	Basename criteria
	uint32_t		nLevel ;		//	Directory step backs (../)
	uint32_t		nCount ;		//	Step back iterator

	i = criteria ;
	if (!i || !i[0])
		return E_OK ;

	if (i[0] == CHAR_FWSLASH)
	{
		//	Start at the root directory
		rootVal.AddByte(CHAR_FWSLASH) ;
		i++ ;
	}
	else
	{
		//	If the criteria starts with a ../ sequence we have to go back from the current directory but if not then we will start at
		//	the current directory
		GetCurrDir(cwd) ;
		rootVal = cwd ;

		if (i[0] == CHAR_PERIOD && i[1] == CHAR_PERIOD && i[2] == CHAR_FWSLASH)
		{
			curr = *cwd + 1 ;
			SplitStrOnChar(parts, curr, CHAR_FWSLASH) ;

			for (nLevel = parts.Count() - 1 ; i[0] == CHAR_PERIOD && i[1] == CHAR_PERIOD && i[2] == CHAR_FWSLASH ; nLevel--, i += 3) ;
			if (nLevel < 0)
				return E_NOTFOUND ;

			root.Clear() ;
			for (nCount = 0 ; nCount < nLevel ; nCount++)
			{
				rootVal += "/" ;
				rootVal += parts[nCount] ;
			}
		}

		if (i[0] == CHAR_PERIOD && i[1] == CHAR_FWSLASH)
			i += 2 ;
	}

	crit = i ;
	SplitStrOnChar(parts, crit, CHAR_FWSLASH) ;

	rootDir = rootVal ;
	return _scanfiles_r(files, parts, rootDir, 0, bLimit) ;
}

hzEcode	FindfilesStd	(hzArray<hzString>& files, const char* criteria)
{
	//	Category:	Directory
	//
	//	Find files strictly conforming to the supplied criteria. Current Working directory is treated as the root.
	//
	//	Arguments:	1)	files		The vector of files found
	//				2)	criteria	The scaning criteria
	//
	//	Returns:	E_ARGUMENT	If the pathsofar is empty or no parts are specified
	//				E_OPENFAIL	If the directory cannot be opened
	//				E_CORRUPT	If a directory entry cannot be stated
	//				E_OK		If the directory is successfully scanned

	return _scanfiles(files, criteria, true) ;
}

hzEcode	FindfilesTar	(hzArray<hzString>& files, const char* criteria)
{
	//	Category:	Directory
	//
	//	Find files according to the tar interpretation of the supplied criteria. Current Working directory is treated as the root.
	//
	//	Arguments:	1)	files		The vector of files found
	//				2)	criteria	The scaning criteria
	//
	//	Returns:	E_ARGUMENT	If the pathsofar is empty or no parts are specified
	//				E_OPENFAIL	If the directory cannot be opened
	//				E_CORRUPT	If a directory entry cannot be stated
	//				E_OK		If the directory is successfully scanned

	return _scanfiles(files, criteria, false) ;
}

/*
**	TEST Files and Directories
*/

#if 0
hzEcode	TestFile	(hzDirent& de, const char* cpPathname)
{
	//	Category:	Directory
	//
	//	Determine if a file exists at the supplied pathname and populate the supplied hzDirent instance with the file info if it does
	//
	//	Arguments:	1)	de			The hzDirent instance to be populated
	//				2)	cpPathname	The full pathname of the anticipated file
	//
	//	Returns:	E_ARGUMENT	If the pathname was not supplied
	//				E_NOTFOUND	If the file does not exist
	//				E_TYPE		If the supplied pathname is that of a directory or non-file
	//				E_OK		If the files exists

	FSTAT		fs ;		//	File status
	hzString	name ;		//	Directory entry name

	if (!cpPathname || !cpPathname[0])
		return E_ARGUMENT ;

	if (lstat(cpPathname, &fs) == -1)
		return E_NOTFOUND ;

	if (ISDIR(fs.st_mode))
		return E_TYPE ;

	de.InitStat(0, name, fs) ;

	return E_OK ;
}
#endif

hzEcode	TestFile	(const char* fullpath)
{
	//	Category:	Directory
	//
	//	Determine if a file exists at the supplied pathname.
	//
	//	Arguments:	1)	fullpath	The full pathname of the anticipated file
	//
	//	Returns:	E_ARGUMENT	If the pathname was not supplied
	//				E_NODATA	If permissions were denied
	//				E_CORRUPT	If the supplied pathname was nonsensical (stat operation produced an errno of EIO, ELOOP or EOVERFLOW)
	//				E_SYNTAX	If the supplied pathname was too long
	//				E_NOTFOUND	If the anticipated file does not exist
	//				E_TYPE		If the supplied pathname is that of a directory or non-file
	//				E_OK		If the files exists

	FSTAT	fs ;			//	File status
	hzEcode	rc = E_OK ;		//	Return code

	if (lstat(fullpath, &fs) == -1)
	{
		switch	(errno)
		{
		case EACCES:		rc = E_NODATA ;		break ;
		case EIO:			rc = E_CORRUPT ;	break ;
		case ELOOP:			rc = E_CORRUPT ;	break ;
		case ENAMETOOLONG:	rc = E_SYNTAX ;		break ;
		case ENOENT:		rc = E_NOTFOUND ;	break ;
		case ENOTDIR:		rc = E_SYNTAX ;		break ;
		case EOVERFLOW:		rc = E_CORRUPT ;	break ;
		}
	}
	else
	{
		if (ISDIR(fs.st_mode))
			rc = E_TYPE ;
	}

	return rc ;
}

#if 0
hzEcode	TestDir		(hzDirent& de, const char* cpPathname)
{
	//	Category:	Directory
	//
	//	Determine if a directory exists at the supplied pathname and populate the supplied hzDirent instance with the file info if it does
	//
	//	Arguments:	1)	de			The hzDirent instance to be populated
	//				2)	cpPathname	The full pathname of the anticipated directory
	//
	//	Returns:	E_ARGUMENT	If the pathname was not supplied
	//				E_NOTFOUND	If the directory does not exist
	//				E_TYPE		If the supplied pathname is not that of a directory
	//				E_OK		If the directory exists

	FSTAT	fs ;		//	Directory status
	hzString	name ;		//	Directory name

	if (!cpPathname || !cpPathname[0])
		return E_ARGUMENT ;

	if (lstat(cpPathname, &fs) == -1)
		return E_NOTFOUND ;

	if (!ISDIR(fs.st_mode))
		return E_TYPE ;

	de.InitStat(0, name, fs) ;

	return E_OK ;
}
#endif

hzEcode	DirExists	(const char* dirname)
{
	//	Category:	Directory
	//
	//	Determine if a directory exists at the supplied pathname.
	//
	//	Arguments:	1)	dirname		The full pathname of the anticipated file
	//
	//	Returns:	E_ARGUMENT	If the pathname was not supplied
	//				E_NOTFOUND	If the directory does not exist
	//				E_TYPE		If the supplied pathname is not that of a directory
	//				E_OK		If the directory exists

	FSTAT	fs ;		//	Directory status

	if (!dirname || !dirname[0])
		return E_ARGUMENT ;

	if (stat(dirname, &fs) == -1)
		return E_NOTFOUND ;

	if (!S_ISDIR(fs.st_mode))
		return E_TYPE ;

	return E_OK ;
}

hzEcode	OpenInputStrm	(std::ifstream& input, const char* filepath, const char* callfn)
{
	//	Category:	Directory
	//
	//	Uses the supplied input stream to open a file at the specified filepath. The main reason for this function is to standarize error condition reporting as
	//	all file open operations have essentially the same set of issues namely:-
	//
	//		1)	The file either is not there
	//		2)	The pathname names a directory or other file system object, instead of a file
	//		3)	The file is empty (an error when intending to read)
	//		4)	The file cannot be read (program does not have access permissions
	//
	//	Arguments:	1)	input		Reference to an input stream which this function will open
	//				2)	filepath	Full pathname of file to open
	//				3)	callfn		This function's caller. If this is zero, this function refrains from logging error and just returns them.
	//
	//	Returns:	E_ARGUMENT	If the filepath is NULL
	//				E_NOTFOUND	If the filepath specified does not exist as a directory entry of any kind
	//				E_TYPE		If the filepath names a directory entry that is not a file
	//				E_NODATA	If the filepath is a file but empty
	//				E_OPENFAIL	If the file specified could not be opened for reading
	//				E_OK		If the input stream was opened OK

	_hzfunc(__func__) ;

	FSTAT	fs ;		//	File status
	bool	bLog ;		//	Log errors

	//	Check arguments
	bLog = (callfn && callfn[0]) ? true : false ;

	if (!filepath || !filepath[0])
	{
		if (bLog)
			return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Function %s - No input filename specified", callfn) ;
		else
			return E_ARGUMENT ;
	}

	//	Check if filepath names a file
	if (lstat(filepath, &fs) < 0)
	{
		if (bLog)
			return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Function %s - Filename %s does not exist", callfn, filepath) ;
		else
			return E_NOTFOUND ;
	}

	if (!S_ISREG(fs.st_mode))
	{
		if (bLog)
			return hzerr(_fn, HZ_ERROR, E_TYPE, "Function %s - Filename %s is not a file", callfn, filepath) ;
		else
			return E_TYPE ;
	}

	if (!fs.st_size)
	{
		if (bLog)
			return hzerr(_fn, HZ_ERROR, E_NODATA, "Function %s - Filename %s is empty", callfn, filepath) ;
		else
			return E_NODATA ;
	}

	if (input.is_open())
		hzerr(_fn, HZ_WARNING, E_DUPLICATE, "Function %s - Filename %s is already open", callfn, filepath) ;
	else
	{
		input.open(filepath) ;
		if (input.fail())
		{
			if (bLog)
				return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Function %s - Filename %s could not be opened", callfn, filepath) ;
			else
				return E_OPENFAIL ;
		}
	}

	return E_OK ;
}

/*
**	SECTION 2:	hzFileset functions
*/

void	hzFileset::_clear	(void)
{
	//	Clear the file set. This function returns void and has no arguments. There are no reportable errors.
	//
	//	Arguments:	None
	//	Returns:	None

	uint32_t	nIndex ;	//	Directory iterator

	m_nBlocs = 0 ;
	m_nChars = 0 ;
	m_nLinks = 0 ;
	m_nSocks = 0 ;
	m_nBytes = 0 ;

	for (nIndex = 0 ; nIndex < m_dirs.Count() ; nIndex++)
	{
		delete m_dirs.GetObj(nIndex) ;
	}
	m_dirs.Clear() ;

	for (nIndex = 0 ; nIndex < m_file.Count() ; nIndex++)
	{
		delete m_file.GetObj(nIndex) ;
	}
	m_file.Clear() ;
}

hzEcode	hzFileset::_scan_r	(hzDirent* parent, hzString& curdir)
{
	//	Move thru current directory and read files and sub directories. As this is a private function of the hzFileset
	//	class and cannot be called directly by an application, certain assumptions have been permitted. At the point
	//	where it is called by hzFileset::Scan() and the parent is null the directory we are actually in is one of the
	//	roots of the hzFileset instance.
	//
	//	Arguments:	1)	parent	Pointer to parent directory entry
	//				2)	curdir	Current directory name
	//
	//	Returns:	E_ARGUMENT	If the current directory is not supplied
	//				E_OPENFAIL	If the directory cannot be opened
	//				E_CORRUPT	If the supplied parent is not a directory or if an listed entity cannot be stated
	//				E_OK		If the directory was scanned successfully

	_hzfunc("hzFileset::_scan_r") ;

	FSTAT		fs ;				//	File status
	dirent*		pDE ;				//	Directory entry
	DIR*		pDir ;				//	Directory
	hzDirent*	pThisdir ;			//	Directory being processed
	hzDirent*	pFX ;				//	New directory entry
	hzString	nextent ;			//	Next directory entry
	hzString	filename ;			//	Filename
	hzEcode		rc = E_OK ;			//	Return code

	/*
 	**	Open, check and init the directory
	*/

	if (!curdir)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Error: No current directory\n") ;

	if (parent)
	{
		//	If a parent has been supplied, this must be a directory

		if (!parent->IsDir())
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Error: Supplied parent (%s) is not a directory\n", *parent->Name()) ;
	}

	//	Ignore certain system/kernel directories
	if (curdir == "/proc")	return E_OK ;
	if (curdir == "/dev")	return E_OK ;

	if (stat(*curdir, &fs) < 0)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not stat supplied 'current directory' (%s)\n", *curdir) ;

	//	Create the dir-ent for this current directory and insert it into the FS-Image directory maps
	pThisdir = new hzDirent() ;
	if (!pThisdir)
		Fatal("Could not create hzDirent instance\n") ;
	pThisdir->InitStat(parent->Name(), curdir, fs) ;

	if (!pThisdir->IsDir())
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Error: Supplied directory (%s) is not a directory\n", *pThisdir->Name()) ;

	m_dirs.Insert(pThisdir->Path(), pThisdir) ;

	/*
	**	Read the entries for this directory
	*/

	if (!(pDir = opendir(*curdir)))
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open directory %s", *curdir) ;

	for (; (pDE = readdir(pDir)) ;)
	{
		if (!strcmp(pDE->d_name, "."))	continue ;
		if (!strcmp(pDE->d_name, ".."))	continue ;

		nextent = curdir ;
		nextent += "/" ;
		nextent += pDE->d_name ;

		if (lstat(*nextent, &fs) < 0)
		{
			m_Error.Printf("Error: lstat returned (errno) %s\n", ShowErrno()) ;
			m_Error.Printf("Listed entry (%s) could not be stat-ed\n", *nextent) ;
			return E_CORRUPT ;
		}

		if (S_ISDIR(fs.st_mode))
		{
			//	The entry is a directory

			if (curdir == "/")
			{
				if (!strcmp(pDE->d_name, "proc"))	continue ;
				if (!strcmp(pDE->d_name, "dev"))	continue ;
			}

			rc = _scan_r(pThisdir, nextent) ;
			if (rc != E_OK)
				return rc ;

			continue ;
		}

		if (S_ISREG(fs.st_mode))
		{
			//	The etry is a file so add/update

			if (!(pFX = new hzDirent()))
				Fatal("Could not create hzDirent instance\n") ;

			filename = pDE->d_name ;
			pFX->InitStat(pThisdir->Name(), filename, fs) ;
			m_file.Insert(pFX->Pathname(), pFX) ;
			m_nBytes += pFX->Size() ;

			//pThisdir->AddChild(pFX) ;

			continue ;
		}

		if (S_ISBLK(fs.st_mode))	m_nBlocs++ ;
		if (S_ISCHR(fs.st_mode))	m_nChars++ ;
		if (S_ISLNK(fs.st_mode))	m_nLinks++ ;
		if (S_ISSOCK(fs.st_mode))	m_nSocks++ ;
	}

	closedir(pDir) ;
	return E_OK ;
}

hzEcode	hzFileset::Scan	(void)
{
	//	Run a fileset scan. Firstly mark all files for delete before a run. Then during the run the new files will be marked as CREATE and existing files either
	//	as MODIFY or NOACTION
	//
	//	Arguments:	None
	//
	//	Returns:	E_ARGUMENT	If the directory name is missing from the fileset root
	//				E_OPENFAIL	If the directory cannot be opened
	//				E_CORRUPT	If the supplied parent is not a directory or if an listed entity cannot be stated
	//				E_OK		If the directory was scanned successfully

	_hzfunc("hzFileset::Scan") ;

	FSTAT		fs ;			//	To check if dir exists
	hzString	opdir ;			//	Operations (root) directory.
	uint32_t	nCount ;		//	General iterator
	hzEcode		rc = E_OK ;		//	Return code

	m_Error.Clear() ;

	_clear() ;

	/*
 	**	Save the current directory then iterate through the root directories, building the dependancy tree of each.
	**	When done return to the original current directory.
	*/

	for (nCount = 0 ; nCount < m_Roots.Count() ; nCount++)
	{
		opdir = m_Roots[nCount] ;

		if (lstat(*opdir, &fs) < 0)
		{
			m_Error.Printf("Warning: root of %s does not exist\n", *opdir) ;
			continue ;
		}

		rc = _scan_r(0, opdir) ;
		if (rc != E_OK)
			return rc ;
	}

	return E_OK ;
}

hzEcode	hzFileset::Import	(const char* filepath)
{
	//	Populate the hzFileset instance from a import file
	//
	//	Arguments:	1)	filepath	The import file
	//
	//	Returns:	E_ARGUMENT	If the import filename is not supplied
	//				E_OPENFAIL	If the import file cannot be opened
	//				E_READFAIL	If the import file cannot be read
	//				E_OK		If the import was successful

	_hzfunc("hzFileset::Import()") ;

	std::ifstream	is ;		//	File input stream

	hzDirent*	pDX ;			//	New directory
	hzDirent*	pFX ;			//	New file
	char*		i ;				//	Buffer iterator
	hzString	newname ;		//	New dir/file name
	uint32_t	nLine ;			//	Line number
	uint32_t	nInode ;		//	Inode (used in copy protection etc)
	uint32_t	nSize ;			//	File size
	uint32_t	nCtime ;		//	File create date
	uint32_t	nMtime ;		//	File modified date
	uint32_t	nMode ;			//	File mode
	uint32_t	nOwner ;		//	UNIX owner id
	uint32_t	nGroup ;		//	UNIX group id
	bool		bOk ;			//	Format OK
	hzEcode		rc ;			//	Return code
	char		cvLine[512] ;	//	Line buffer

	//	Check filepath
	if (!filepath || !filepath[0])
		return E_ARGUMENT ;

	//	Open for reading
	is.open(filepath) ;
	if (is.fail())
		return E_OPENFAIL ;

	//	Read in line by line
	for (nLine = 1 ;; nLine++)
	{
		is.getline(cvLine, 512) ;
		if (!is.gcount())
			break ;

		i = cvLine ;

		if (i[0] == '#')
			continue ;

		//	These apply to both dirs and files

					bOk = IsPosint(nInode, i + 2) ;
		if (bOk)	bOk = IsHexnum(nMode,  i + 13) ;
		if (bOk)	bOk = IsPosint(nSize,  i + 22) ;
		if (bOk)	bOk = IsPosint(nCtime, i + 33) ;
		if (bOk)	bOk = IsPosint(nMtime, i + 44) ;
		if (bOk)	bOk = IsPosint(nOwner, i + 55) ;
		if (bOk)	bOk = IsPosint(nGroup, i + 61) ;

		if (!bOk)
		{
			is.close() ;
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Line %d of %s is malformed", nLine, filepath) ;
		}

		newname = i + 67 ;

		if (i[0] == 'D')
		{
			//	Directory entry
			pDX = new hzDirent() ;
			pDX->InitNorm(0, newname, nInode, nSize, nCtime, nMtime, nMode, nOwner, nGroup, 0) ;
			m_dirs.Insert(pDX->Path(), pDX) ;
			
			continue ;
		}

		if (i[0] == 'F')
		{
			//	File entry

			pFX = new hzDirent() ;
			pFX->InitNorm(pDX->Name(), newname, nInode, nSize, nCtime, nMtime, nMode, nOwner, nGroup, 0) ;
			m_nBytes += pFX->Size() ;
			m_file.Insert(pFX->Pathname(), pFX) ;
		}
	}

	is.close() ;

	return E_OK ;
}

hzEcode	hzFileset::Export	(const char* filepath)
{
	//	Export the hzDirSync to a file
	//
	//	Arguments:	1)	filepath	The 'horizon' file
	//
	//	Returns:	E_ARGUMENT	If the export filename is not supplied
	//				E_OPENFAIL	If the export file cannot be opened
	//				E_WRITEFAIL	If the export file cannot be written to or there was a write failure
	//				E_OK		If the export was successful

	_hzfunc("hzFileset::Export()") ;

	hzList<hzDirent*>::Iter	I ;		//	Directory listing

	std::ofstream	os ;			//	For writing file

	hzChain		Z ;					//	For formulating output
	hzDirent*	pDX ;				//	Directory
	hzDirent*	pFX ;				//	File
	uint32_t	totSize = 0 ;		//	Total size of all files
	uint32_t	nD ;				//	Fileset directory iterator
	uint32_t	nF ;				//	Fileset file iterator
	uint32_t	fileLo ;			//	Object position of first file in the given directory
	uint32_t	fileHi ;			//	Object position of last file in the given directory
	char		cvLine	[512] ;		//	Working buffer

	if (!filepath || !filepath[0])
		return E_ARGUMENT ;

	os.open(filepath) ;
	if (os.fail())
		return E_OPENFAIL ;

	Z.Printf("# Export: %s Directories\n", *FormalNumber(m_dirs.Count(),0)) ;
	Z.Printf("# Export: %s Files\n", *FormalNumber(m_file.Count(),0)) ;
	os << Z ;

	if (os.fail())
		return E_WRITEFAIL ;

	for (nD = 0 ; nD < m_dirs.Count() ; nD++)
	{
		pDX = m_dirs.GetObj(nD) ;

		sprintf(cvLine, "D %010d %08x %010u %010u %010u %05d %05d %s\n",
			pDX->Inode(), pDX->Mode(), pDX->Size(), pDX->Ctime(), pDX->Mtime(), pDX->Owner(), pDX->Group(), *pDX->Name()) ;
		os << cvLine ;

		fileLo = m_file.First(pDX->Name()) ;
		if (fileLo < 0)
			continue ;

		fileHi = m_file.Last(pDX->Name()) ;

		for (nF = fileLo ; nF <= fileHi ; nF++)
		//for (I = pDX->m_Kinder ; I.Valid() ; I++)
		{
			//pFX = I.Element() ;
			pFX = m_file.GetObj(nF) ;

			if (pFX->IsDir())
				continue ;

			totSize += pFX->Size() ;

			sprintf(cvLine, "F %010d %08x %010u %010u %010u %05d %05d %s\n",
				pFX->Inode(), pFX->Mode(), pFX->Size(), pFX->Ctime(), pFX->Mtime(), pFX->Owner(), pFX->Group(), *pFX->Name()) ;
			os << cvLine ;
		}
	}

	if (os.fail())
		return E_WRITEFAIL ;

	os << "# Export Complete - total " << FormalNumber(totSize,0) << " bytes\n" ;
	os.close() ;
	return E_OK ;
}

/*
**	Section 1:	Directory assertion. Create directory if not already in existance
*/

hzEcode	AssertDir	(const char* dirname, uint32_t nPerms)
{
	//	Category:	Directory
	//
	//	Test if a directory at the supplied path exists and if not, attempts to create it.
	//
	//	Arguments:	1)	dirname		The directory pathname
	//				2)	nPerms		The permissions that will be given to the directory if created by this function.
	//
	//	Returns:	E_ARGUMENT	If the directory pathname is not supplied.
	//				E_NOCREATE	If the directory could not be created
	//				E_OK		Operation successful

	_hzfunc("AssertDir") ;

	FSTAT		fs ;			//	File status
	const char*	i ;				//	Directory path iterator
	char*		j ;				//	Path part terminator
	char*		cpPath ;		//	Working buffer (for path parts)
	hzEcode		rc = E_OK ;		//	Return code

	//	Check arguments
	if (!dirname || !dirname[0])
	{
		threadLog("%s. No arguments supplied\n", __func__) ;
		return E_ARGUMENT ;
	}

	//	If this path exists return true if it is a directory or false if it is a file
	if (stat(dirname, &fs) != -1)
	{
		if (fs.st_mode & S_IFDIR)
			return E_OK ;

		return hzerr(_fn, HZ_ERROR, E_NOCREATE, "Target dir [%s] exists as non-directory", dirname) ;
	}

	//	The path does not exist so create. Begin with asserting the first part of the path and move through each '/' in turn.
	cpPath = new char[strlen(dirname) + 1] ;

	j = cpPath ;
	i = dirname ;

	for (; *i ;)
	{
		for (*j++ = *i++ ; *i && *i != CHAR_FWSLASH ; *j++ = *i++) ;
		*j = 0 ;

		if (stat(cpPath, &fs) == -1)
		{
			if (mkdir(cpPath, (nPerms & 0x01ff)) == -1)
			{
				hzString	err ;	//	Error message

				if		(errno == EACCES)	err = "No write permission" ;
				else if (errno == EEXIST)	err = "Entity is file" ;
				else if (errno == EMLINK)	err = "parent dir beyond link limit" ;
				else if (errno == ENOSPC)	err = "No room on device" ;
				else if (errno == EROFS)	err = "File system is read-only" ;
				else
					err = "unknown errno" ;

				rc = hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot make dir [%s] %s", cpPath, *err) ;
				delete cpPath ;
				return rc ;
			}

			continue ;
		}

		if (fs.st_mode & S_IFDIR)
			continue ;

		rc = hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Directory entity [%s] exists as non-directory", cpPath) ;
		delete cpPath ;
		return rc ;
	}

	delete cpPath ;
	return E_OK ;
}
