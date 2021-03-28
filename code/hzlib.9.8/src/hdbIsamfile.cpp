//
//	File:	hdbIsamfile.cpp
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

#include <iostream>
#include <fstream>
#include <cstdio>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzDatabase.h"

using namespace std ;

/*
**	Variables
*/

/*
**	SECTION 1: hdbIsamfile Functions
*/

hdbIsamfile::hdbIsamfile	(void)
{
	m_nElements = 0 ;
	m_nBlocks = 0 ;
	m_nKeyLimit = 256 ;
	m_nObjLimit = 256 ;
	m_nBlkSize = HZ_BLOCKSIZE ;
	m_nInitState = 0 ;

	//_hzGlobal_Memstats.m_numIsamfile++ ;
}

hdbIsamfile::~hdbIsamfile	(void)
{
	if (m_nInitState == 2)
		Close() ;

	//_hzGlobal_Memstats.m_numIsamfile-- ;
}

/*
**	hdbIsamfile Init and Halt Functions
*/

hzEcode	hdbIsamfile::Init	(hdbADP& adp, const hzString& name, const hzString& opdir, uint32_t keySize, uint32_t objSize, uint32_t blkSize)
{
	//	Initialize the hdbIsamfile instance.
	//
	//	Initialization ensures the working directory exists and both the index and data file exist and are open for reading and writing.
	//
	//	This is a matter of opening an input stream for the data file, an output stream for the data file and an output stream for the index file. Note both the
	//	output streams are opened with ios::app as they will only ever append and that there is no input stream for the index file as during normal operation of
	//	a binary datacron, the index is never read.
	//
	//	nd index file as append only files. The first step is to open the index file for reading and load the index into the memory. This file is then closed but
	//	opened again in write mode. If it does not exists then it is created and left open in write mode. The second step is to create or open the data file for
	//	both reading and writing. The hdbIsamfile is then ready to store and retrieve binary objects.
	//
	//	Arguments:	1) name		Name of object set to be stored in the hdbIsamfile (will be base of the two filenames)
	//				2) opdir	Full pathname of operational directory
	//
	//	Returns:	E_OPENFAIL	The files could not be opened or created (no space or wrong permissions)
	//				E_OK		Operation was successful.
	//
	//	Errors:		Any false return is due to an irrecoverable error. The calling function
	//				must check the return value.

	_hzfunc("hdbIsamfile::Init") ;

	FSTAT		fs ;		//	File status
	ifstream	is ;		//	Input stream for reading index file
	int32_t		fd ;		//	File descriptor
	hzEcode		rc ;		//	Return code

	if (m_nInitState)
		return hzerr(_fn, HZ_ERROR, E_INITDUP, "Resource already initialized") ;

	//	Check we have a name and working directory
	if (!name)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No name") ;
	if (!opdir)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Repository %s: No working directory", *name) ;

	//	Check we do not already have a datacron of the name
	if (adp.GetBinRepos(name))
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Binary datum cron %s already exists", *name) ;

	//	Assign internal structure, name and working directory
	m_Name = name ;
	m_Workdir = opdir ;
	m_Error.Printf("%s called with cron name %s and workdir %s\n", *_fn, *m_Name, *m_Workdir) ;

	//	Assert working directory
	if (lstat(*m_Workdir, &fs) < 0)
	{
		rc = AssertDir(*m_Workdir, 0777) ;
		if (rc != E_OK)
			return hzerr(_fn, HZ_ERROR, rc, "Could not assert working dir of %s", *m_Workdir) ;
	}

	//	Set the pathnames for the index and data file
	m_FileIndx = m_Workdir + "/" + m_Name + ".idx" ;
	m_FileData = m_Workdir + "/" + m_Name + ".dat" ;

	m_Error.Printf("%s Have index file of %s and data file of %s\n", *_fn, *m_FileIndx, *m_FileData) ;

	//	memset(m_Buf, 0, HZ_BLOCKSIZE) ;
	//	m_Buf[0] = m_Buf[1] = m_Buf[2] = m_Buf[3] = m_Buf[4] = m_Buf[5] = m_Buf[6] = m_Buf[7] = '0' ;
	//	m_Buf[8] = CHAR_EOT ;
	//	m_Buf[9] = CHAR_NL ;

	if (lstat(*m_FileIndx, &fs) < 0)
	{
		fd = open(*m_FileIndx, O_RDWR|O_CREAT|O_TRUNC, 0600) ;
		if (fd < -1)
			return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot create %s, error %s", *m_FileIndx, Err2Txt(errno)) ;
		close(fd) ;
	}

	if (lstat(*m_FileData, &fs) < 0)
	{
		fd = open(*m_FileData, O_RDWR|O_CREAT|O_TRUNC, 0600) ;
		if (fd < -1)
			return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot create %s, error %s", *m_FileData, Err2Txt(errno)) ;
		close(fd) ;
	}

	//	Set init state etc
	m_nInitState = 1 ;
	return E_OK ;
}

hzEcode	hdbIsamfile::Open	(void)
{
	//	Open the hdbIsamfile instance.
	//
	//	This is a matter of opening an input stream for the data file, an output stream for the data file and an output stream for the index file. Note both the
	//	output streams are opened with ios::app as they will only ever append and that there is no input stream for the index file as during normal operation of
	//	a hdbIsamfile, the index is never read.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	If the repository has not been initialized
	//				E_SEQUENCE	If the repository is already open
	//				E_OPENFAIL	If either the index or data file cannot be opened for either reading or writing
	//				E_OK		If the operation was successful

	_hzfunc("hdbIsamfile::Open") ;

	hzMapS<uint32_t,hzString>	tmp ;	//	Temporary map

	ifstream	is ;			//	Index input stream
	hzString	key ;			//	Key
	uint32_t	addr ;			//	Block address
	uint32_t	nLines = 0 ;	//	Line count
	uint32_t	n ;				//	Temp index iterator

	if (m_nInitState == 0)	hzexit(_fn, 0, E_NOINIT, "Cannot open an uninitialized datacron") ;
	if (m_nInitState == 2)	hzexit(_fn, m_Name, E_SEQUENCE, "Datacron is already open") ;

	//m_WrI.open(*m_FileIndx, std::ios::app) ;
	m_WrI.open(*m_FileIndx) ;
	if (m_WrI.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot open index file for writing: Repos %s", *m_FileIndx) ;

	//m_WrD.open(*m_FileData, std::ios::app) ;
	m_WrD.open(*m_FileData) ;
	if (m_WrD.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open file (%s) for writing", *m_FileData) ;

	is.open(*m_FileIndx) ;
	if (is.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open index file (%s) for reading", *m_FileData) ;

	m_nBlocks = 0 ;
	for (;;)
	{
		is.getline(m_Buf, 500) ;
		if (!is.gcount())
			break ;
		nLines++ ;

		//	Block address is first part of line, key is remainder
		m_Buf[8] = 0 ;
		IsHexnum(addr, m_Buf) ;
		key = m_Buf + 9 ;

		tmp.Insert(addr, key) ;
		m_nBlocks++ ;
	}
	is.close() ;

	//	Transpose tmp index to operational index
	for (n = 0 ; n < tmp.Count() ; n++)
	{
		addr = tmp.GetKey(n) ;
		key = tmp.GetObj(n) ;

		m_Index.Insert(key, addr) ;
	}

	tmp.Clear() ;

	//	m_RdI.open(*m_FileIndx) ;
	//	if (m_RdI.fail())
	//		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open index file (%s) for reading", *m_FileData) ;

	m_RdD.open(*m_FileData) ;
	if (m_RdD.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open data file (%s) for reading", *m_FileData) ;

	m_nInitState = 2 ;

	return E_OK ;
}

hzEcode	hdbIsamfile::Close	(void)
{
	//	Close the datacron.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	The datacron is not initialized
	//				E_SEQUENCE	The datacron is not open
	//				E_OK		The operation was successful

	_hzfunc("hdbIsamfile::Close") ;

	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	//if (m_RdI.is_open())	m_RdI.close() ;
	if (m_RdD.is_open())	m_RdD.close() ;
	if (m_WrI.is_open())	m_WrI.close() ;
	if (m_WrD.is_open())	m_WrD.close() ;

	return E_OK ;
}

hzEcode	hdbIsamfile::Insert	(const hzString& newKey, const hzString& newObj)
{
	//	Insert a key/object pair into the ISAM file. Note it is permissable for the object to be blank but not the key.
	//
	//	This involves the following steps:-
	//
	//		- Identify the target data block for the new key from the index. We then have both the target block address and the available space.
	//		- Read in target data block, assemble new version of it including the new key.
	//		- If the target has enough space
	//			- Write out target block.
	//			- Advice the index map of a new higher key, if applicable.
	//		- Else
	//			- Write out half the target to the original location
	//			- Write out upper half to a new location
	//			- Advice the index map.
	//
	//	Note that no attempt is made to move elements into adjacent data blocks. Disk space is cheap but disk operations are expensive.
	//
	//	Arguments:	1) key	The key
	//				2) obj	The object
	//
	//	Returns:	E_NOINIT	If this hdbIsamfile instance has not been initialized
	//				E_NODATA	If the supplied datum is of zero size
	//				E_WRITEFAIL	If the data could not be written to disk.
	//				E_OK		If operation successful

	_hzfunc("hdbIsamfile::Insert") ;

	hzMapM<hzString,hzString>	tmp ;	//	Temp map of key/object pairs found in target data area

	hzChain		Zout ;			//	For writing out the temp map to a block
	hzChain		Xind ;			//	For writing out index entries
	chIter		zi ;			//	Chain iterator
	char*		i ;				//	Buffer iterator
	char*		j ;				//	Buffer iterator
	const char*	k ;				//	Key iterator
	hzString	strA ;			//	Key string
	hzString	strB ;			//	Object string
	uint32_t	addr = 0 ;		//	Address of target data block
	uint32_t	nSpace = 0 ;	//	Space consumed
	uint32_t	nC ;			//	Counter
	int32_t		nLo ;			//	Position of target within m_Index map
	hzEcode		rc = E_OK ;		//	Return code

	//	Check init state
	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	//	Check validity of supplied key (must contain no newlines)
	if (!newKey)
		return E_NODATA ;
	m_Error.Clear() ;

	nSpace = newKey.Length() ;
	k = *newKey ;
	for (nC = 0 ; nC < nSpace ; nC++)
	{
		if (k[nC] == 0 || k[nC] == CHAR_NL)
			{ m_Error.Printf("Illegal value %c (%d) found in key [%s]\n", k[nC], k[nC], *newKey) ; return E_BADVALUE ; }
	}

	//	If virgin ISAM
	if (!m_Index.Count())
	{
		nC = newKey.Length() ;
		memset(m_Buf, 0, HZ_BLOCKSIZE) ;
		sprintf(m_Buf, "%s\n%s\n", *newKey, *newObj) ;

		m_WrD.write(m_Buf, HZ_BLOCKSIZE) ;
		if (m_WrD.fail())
			{ m_Error.Printf("Could not write to virgin data file\n") ; return E_WRITEFAIL ; }
		m_WrD.flush() ;

		m_WrI << "00000000," << "\n" ;
		if (m_WrI.fail())
			{ m_Error.Printf("Could not write to virgin index file\n") ; return E_WRITEFAIL ; }

		m_Index.Insert(_hzGlobal_nullString, 0) ;
		m_nBlocks = 1 ;
		m_WrI.flush() ;

		return E_OK ;
	}

	//	Identify target data block
	nLo = m_Index.Target(newKey) ;
	strA = m_Index.GetKey(nLo) ;
	if (nLo >= (int32_t) m_Index.Count())
		{ nLo = m_Index.Count() -1 ; m_Error.Printf("%s: Case 0: identified index posn %d\n", *_fn, nLo) ; }
	else if (strA > newKey)
		{ nLo-- ; m_Error.Printf("%s: Case 1: identified index posn %d\n", *_fn, nLo) ; }
	else
		m_Error.Printf("%s: Case 2: identified index posn %d\n", *_fn, nLo) ;
	strA = 0 ;
	
	addr = m_Index.GetObj(nLo) ;
		m_Error.Printf("%s: Case 2: identified block %d\n", *_fn, nLo) ;

	//	Read in target data block
	m_RdD.seekg(addr * m_nBlkSize) ;
	if (m_RdD.fail())
		{ m_Error.Printf("%s: Failed to seek to block %d (position %d) in data block\n", *_fn, addr, addr * m_nBlkSize) ; return E_READFAIL ; }

	m_RdD.read(m_Buf, HZ_BLOCKSIZE) ;
	if (m_RdD.fail())
		{ m_Error.Printf("%s: Failed to read block %d (position %d) in data block\n", *_fn, addr, addr * m_nBlkSize) ; return E_READFAIL ; }

	for (i = j = m_Buf, nC = 0 ; nC < HZ_BLOCKSIZE ; i++, nC++)
	{
		if (m_Buf[nC] == CHAR_NL)
		{
			*i = 0 ;
			if (!strA)
				{ strA = j ; j = i + 1 ; continue ; }

			if (*j)
				strB = j ;
			j = i + 1 ;

			if (!strA)
				break ;

			tmp.Insert(strA, strB) ;
			strA = 0 ;
			strB = 0 ;
		}
	}

	//	Insert the new key
	tmp.Insert(newKey, newObj) ;

	//	Seek to the original block
	m_WrD.seekp(addr * m_nBlkSize) ;
	if (m_WrD.fail())
		{ m_Error.Printf("Could not seek to posn %u in data file\n", addr * m_nBlkSize) ; return E_WRITEFAIL ; }
	m_Error.Printf("Seeked to posn %u in data file\n", m_WrD.tellp()) ;

	//	Write out map, add block if necessary
	Zout.Clear() ;
	for (nC = 0 ; nC < tmp.Count() ; nC++)
	{
		strA = tmp.GetKey(nC) ;
		strB = tmp.GetObj(nC) ;

		nSpace = Zout.Size() + strA.Length() + strB.Length() + 2 ;

		if (nSpace > m_nBlkSize)
		{
			memset(m_Buf, 0, HZ_BLOCKSIZE) ;
			for (i = m_Buf, zi = Zout ; !zi.eof() ; i++, zi++)
				*i = *zi ;
			m_WrD.write(m_Buf, HZ_BLOCKSIZE) ;
			if (m_WrD.fail())
				{ m_Error.Printf("Could not write to data file\n") ; return E_WRITEFAIL ; }
			Zout.Clear() ;
			m_WrD.flush() ;

			//	Create new block
			Xind.Printf("%08x,%s\n", m_nBlocks, *strA) ; m_WrI << Xind ; m_WrI.flush() ; Xind.Clear() ;
			m_Index.Insert(strA, m_nBlocks) ;
			m_nBlocks++ ;

			//	Allocate new block
			m_WrD.seekp(0, ios_base::end) ;
			if (m_WrD.fail())
				{ m_Error.Printf("Alloc - Could not seek to end in data file\n") ; return E_WRITEFAIL ; }
		}

		Zout << strA ;
		Zout.AddByte(CHAR_NL) ;
		if (strB)
			Zout << strB ;
		Zout.AddByte(CHAR_NL) ;
	}

	if (Zout.Size())
	{
		memset(m_Buf, 0, HZ_BLOCKSIZE) ;
		for (i = m_Buf, zi = Zout ; !zi.eof() ; i++, zi++)
			*i = *zi ;

		m_WrD.write(m_Buf, HZ_BLOCKSIZE) ;
		if (m_WrD.fail())
			{ m_Error.Printf("Could not write to posn %u in data file\n", m_WrD.tellp()) ; return E_WRITEFAIL ; }
		m_WrD.flush() ;
	}

	return rc ;
}

bool	hdbIsamfile::Exists	(const hzString& key)
{
	//	Determine if the supplied key matches any found in the ISAM
	//
	//	Argument:	key		The search key
	//
	//	Returns:	True	If the key is found
	//				False	Otherwise

	_hzfunc("hdbIsamfile::Exists") ;

	hzMapM<hzString,hzString>	tmp ;	//	Temp map of key/object pairs found in target data area

	char*		i ;				//	Buffer iterator
	char*		j ;				//	Buffer iterator
	hzString	strA ;			//	Key string
	hzString	strB ;			//	Object string
	uint32_t	nPos ;			//	Position of target within m_Index map
	uint32_t	addr ;			//	Address of target data block
	uint32_t	nC ;			//	Counter

	//	Check init state
	m_Error.Clear() ;
	m_Cond = E_OK ;

	m_Error.Printf("%s called\n", *_fn) ;

	if (m_nInitState < 1)	{ m_Cond = E_NOINIT ; hzerr(_fn, HZ_ERROR, m_Cond) ; return false ; }
	if (m_nInitState < 2)	{ m_Cond = E_NOTOPEN ; hzerr(_fn, HZ_ERROR, m_Cond) ; return false ; }

	if (!key)
		return false ;
	if (!m_Index.Count())
		return false ;

	//	Identify target data block
	nPos = m_Index.Target(key) ;
	if (nPos >= m_Index.Count())
		nPos = m_Index.Count() -1 ;
	strA = m_Index.GetKey(nPos) ;
	if (strA > key)
		nPos-- ;

	for (; nPos < m_Index.Count() ; nPos++)
	{
		if (m_Index.GetKey(nPos) > key)
			break ;

		//	Read in target data block
		addr = m_Index.GetObj(nPos) ;
		m_RdD.seekg(addr * m_nBlkSize) ;
		if (m_RdD.fail())
			{ m_Cond = E_READFAIL ; m_Error.Printf("%s: Failed to seek to block %d (position %d) in data block\n", *_fn, addr, addr * m_nBlkSize) ; return false ; }

		m_RdD.read(m_Buf, HZ_BLOCKSIZE) ;
		if (m_RdD.fail())
			{ m_Cond = E_READFAIL ; m_Error.Printf("%s: Failed to read block %d (position %d) in data block\n", *_fn, addr, addr * m_nBlkSize) ; return false ; }

		for (i = j = m_Buf, nC = 0 ; nC < HZ_BLOCKSIZE ; i++, nC++)
		{
			if (m_Buf[nC] == CHAR_NL)
			{
				*i = 0 ;
				if (!strA)
					{ strA = j ; j = i + 1 ; continue ; }

				if (*j)
					strB = j ;
				j = i + 1 ;

				if (!strA)
					break ;

				tmp.Insert(strA, strB) ;
				strA = 0 ;
				strB = 0 ;
			}
		}
	}

	return tmp.Exists(key) ;
}

hzEcode	hdbIsamfile::Fetch	(hzArray<hzPair>& result, const hzString& keyLo, const hzString& keyHi)
{
	//	Fetches objects matching the key or falling within a range of two keys.
	//
	//	Arguments:	1)	obj		The object to be populated
	//				2)	datumId	The object id
	//
	//	Returns:	E_NOTOPEN	If the reader stream is not open for reading
	//				E_READFAIL	The reader stream is already in a fialed state
	//				E_CORRUPT	Row metadata not as expected. Possible overwrites
	//				E_FORMAT	Data format error
	//				E_OK		The operation was successful

	_hzfunc("hdbIsamfile::Fetch") ;

	hzMapM<hzString,hzString>	tmp ;	//	Temp map of key/object pairs found in target data area

	char*		i ;				//	Buffer iterator
	char*		j ;				//	Buffer iterator
	hzString	keyA ;			//	Key string
	hzString	keyB ;			//	Key string
	hzString	strA ;			//	Key string
	hzString	strB ;			//	Object string
	hzPair		pair ;			//	For output
	uint32_t	nPos ;			//	Position of target within m_Index map
	uint32_t	addr ;			//	Address of target data block
	uint32_t	nC ;			//	Counter
	int32_t		nLo ;			//	Position of first target within tmp map
	int32_t		nHi ;			//	Position of last target within tmp map
	hzEcode		rc = E_OK ;		//	Return code

	//	Check init state
	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	result.Clear() ;
	m_Error.Clear() ;

	//	Sort the keys
	if (!keyLo && !keyHi)
		{ m_Error.Printf("No keys supplied\n") ; return E_ARGUMENT ; }
	if (!m_Index.Count())
		return E_NOTFOUND ;

	if (!keyHi)
		keyA = keyB = keyLo ;
	else if (!keyLo)
		keyA = keyB = keyHi ;
	else
	{
		if (keyHi < keyLo)
			{ keyA = keyHi ; keyB = keyLo ; }
		else
			{ keyA = keyLo ; keyB = keyHi ; }
	}

	//	Identify target data block
	nPos = m_Index.Target(keyA) ;
	if (nPos >= m_Index.Count())
		nPos = m_Index.Count() -1 ;
	strA = m_Index.GetKey(nPos) ;
	if (strA > keyA)
		nPos-- ;

	for (; nPos < m_Index.Count() ; nPos++)
	{
		if (m_Index.GetKey(nPos) > keyB)
			break ;

		//	Read in target data block
		addr = m_Index.GetObj(nPos) ;

		m_RdD.seekg(addr * m_nBlkSize) ;
		if (m_RdD.fail())
			{ m_Error.Printf("%s: Failed to seek to block %d (position %d) in data block\n", *_fn, addr, addr * m_nBlkSize) ; return E_READFAIL ; }

		m_RdD.read(m_Buf, HZ_BLOCKSIZE) ;
		if (m_RdD.fail())
			{ m_Error.Printf("%s: Failed to read block %d (position %d) in data block\n", *_fn, addr, addr * m_nBlkSize) ; return E_READFAIL ; }

		for (i = j = m_Buf, nC = 0 ; nC < HZ_BLOCKSIZE ; i++, nC++)
		{
			if (m_Buf[nC] == CHAR_NL)
			{
				*i = 0 ;
				if (!strA)
					{ strA = j ; j = i + 1 ; continue ; }

				if (*j)
					strB = j ;
				j = i + 1 ;

				if (!strA)
					break ;

				tmp.Insert(strA, strB) ;
				strA = 0 ;
				strB = 0 ;
			}
		}
	}

	//	If object id is zero return blank object
	nLo = tmp.First(keyA) ;
	if (nLo < 0)
		return E_NOTFOUND ;
	nHi = tmp.Last(keyB) ;

	for (; nLo <= nHi ; nLo++)
	{
		pair.name = tmp.GetKey(nLo) ;
		pair.value = tmp.GetObj(nLo) ;
		result.Add(pair) ;
	}
	return rc ;
}

hzEcode	hdbIsamfile::Delete	(const hzString& key)
{
	//	Delete an object from the datacron.
	//
	//	This function appears only for consistency with the base class and does nothing.
	//
	//	Arguments:	1)	datumId	The object id to be deleted
	//
	//	Returns:	E_NOINIT	The datacron is not initialized
	//				E_SEQUENCE	The datacron is not open for writing
	//				E_RANGE		The requested item does not exist
	//				E_OK		Operation successful

	_hzfunc("hdbIsamfile::Delete") ;

	//	Check init state
	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	return E_OK ;
}

#if 0
hzEcode	hdbIsamfile::Update	(uint32_t& datumId, const hzChain& datum)	//, uint32_t an1, uint32_t an2)
{
	//	Replace the object named by the supplied id (arg2) with the contents of the supplied hzChain (arg1). This will:-
	//
	//		1) Locate the object and in so doing, obtain the most recent (current) address and version of the existing object
	//		2) Determine the new location of the object
	//		3) Assemble the object header which will be the new size and version number, the current date and the most recent address.
	//		4) Append the data file
	//		5) Update the index
	//
	//	Arguments:	1) datumId	Datum ID - Set first to the original, set by this function to the new version of the object
	//				2) datum	Binary datum value
	//
	//	Returns:	E_NOINIT	The datacron is not initialized
	//				E_NOTOPEN	Attempt to modify object zero
	//				E_NOTFOUND	Attempt to modify non existant object
	//				E_WRITEFAIL	The datacron is not open for writing
	//				E_RANGE		The requested item does not exist
	//				E_OK		The operation was successful

	_hzfunc("hdbIsamfile::Update") ;

	_datum_hd	hdr ;			//	Datum header
	//	uint32_t	rem ;			//	Remainder (to align to 32 byte chunks)
	//	uint32_t	tot ;			//	Total size of datum and padding
	hzEcode		rc = E_OK ;		//	Return code
	//char		buf [20] ;		//	Buffer for padding

	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	if (datumId == 0)		return E_NOTOPEN ;
	if (datumId > m_nDatum)	return E_NOTFOUND ;
	if (!datum.Size())		return E_NODATA ;

	hdr.m_DTStamp.SysDateTime() ;
	hdr.m_Size = datum.Size() ;
	hdr.m_Prev = datumId ;

	//	rem = ((16 - datum.Size()) % 16) % 16 ;
	//	tot = datum.Size() + rem ;

	//	Write to the index
	m_LockIwr.Lock() ;

		hdr.m_Addr = m_nSize ;
		datumId = ++m_nDatum ;
		m_nSize += datum.Size() ;

		m_WrI.write((char*) &hdr, sizeof(_datum_hd)) ;

		if (m_WrI.fail())
			{ m_Error.Printf("Could not update index delta for datum %d\n", datumId) ; rc = E_WRITEFAIL ; }
	m_LockIwr.Unlock() ;
	m_LockDwr.Lock() ;

		if (rc == E_OK)
		{
			//	Write to the data file
			m_WrD << datum ;

			if (m_WrD.fail())
				{ m_Error.Printf("Could not update data file for datum %d\n", datumId) ; rc = E_WRITEFAIL ; }
		}

	m_LockDwr.Unlock() ;

	if (rc == E_OK)
	{
		m_WrI.flush() ;
		m_WrD.flush() ;
	}

	return rc ;
}
#endif

#if 0
hzEcode	hdbIsamfile::Integ	(hzLogger& log)
{
	//	Performs an integrity check on the binary datum repository. The test expects to find in the index file, the time/date stamp and addresses of datum to be
	//	in ascending order. It further expects to find the sizes total exactly matches the data file size.
	//
	//	Argument:	log		The log-channel for the integrity report
	//
	//	Returns:	E_NOTOPEN	If the reader stream is not open for reading
	//				E_READFAIL	The reader stream is already in a fialed state
	//				E_CORRUPT	Row metadata not as expected. Possible overwrites
	//				E_FORMAT	Data format error
	//				E_OK		The operation was successful

	_hzfunc("hdbIsamfile::Integ") ;

	ifstream	is ;			//	Input stream for index file (separate from the in-built)
	_datum_hd*	pHd ;			//	Datum header pointer
	_datum_hd	arrHd[128] ;	//	Datum header array (128 headers)
	uint32_t	nBytes ;		//	Bytes read in from index file
	uint32_t	nItems ;		//	Number of headers read in from index file
	uint32_t	nC ;			//	Header iterator
	uint32_t	totalItems ;	//	Bytes to get in the next read operation
	uint32_t	totalSize ;		//	Bytes to get in the next read operation
	hzEcode		rc = E_OK ;		//	Return code

	//	Check we have an internal structure (have initialized)
	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	//	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	is.open(*m_FileIndx) ;
	if (is.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open index file %s\n", *m_FileIndx) ;

	for (;;)
	{
		is.read((char*) arrHd, 4096) ;
		nBytes = is.gcount() ;

		if (nBytes % sizeof(_datum_hd))
		{
			log.Out("%s Unexpected read size %d\n", *_fn, nBytes) ;
			break ;
		}
		nItems = nBytes/sizeof(_datum_hd) ;

		for (pHd = arrHd, nC = 0 ; nC < nItems ; nC++, pHd++)
		{
			log.Out("Datum %u (%u) %s %u\n", pHd->m_Addr, nC, *pHd->m_DTStamp.Str(), pHd->m_Size) ;

			totalItems++ ;
			totalSize += pHd->m_Size ;
		}

		if (nItems < 128)
			break ;
	}

	is.close() ;

	log.Out("%s Total items %u\n", *_fn, totalItems) ;
	log.Out("%s Total size %u\n", *_fn, totalSize) ;
}
#endif
