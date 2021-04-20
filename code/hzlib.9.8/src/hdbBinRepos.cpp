//
//	File:	hdbBinRepos.cpp
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

using namespace std ;

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hzChain.h"
#include "hzDirectory.h"
#include "hzDatabase.h"

/*
**	Variables
*/

/*
**	SECTION 1: hdbBinRepos Functions
*/

hdbBinRepos::hdbBinRepos	(hdbADP& adp)
{
	m_pADP = &adp ;
	m_nSize = 0 ;
	m_nDatum = 0 ;
	m_nInitState = 0 ;

	_hzGlobal_Memstats.m_numBincron++ ;
}

hdbBinRepos::~hdbBinRepos	(void)
{
	if (m_nInitState == 2)
		Close() ;

	_hzGlobal_Memstats.m_numBincron-- ;
}

/*
**	hdbBinRepos Init and Halt Functions
*/

hzEcode	hdbBinRepos::Init	(const hzString& name, const hzString& opdir)
{
	//	Initialize the hdbBinRepos instance.
	//
	//	Initialization ensures the working directory exists and both the index and data file exist and are open for reading and writing.
	//
	//	This is a matter of opening an input stream for the data file, an output stream for the data file and an output stream for the index file. Note both the
	//	output streams are opened with ios::app as they will only ever append and that there is no input stream for the index file as during normal operation of
	//	a binary datacron, the index is never read.
	//
	//	nd index file as append only files. The first step is to open the index file for reading and load the index into the memory. This file is then closed but
	//	opened again in write mode. If it does not exists then it is created and left open in write mode. The second step is to create or open the data file for
	//	both reading and writing. The hdbBinRepos is then ready to store and retrieve binary objects.
	//
	//	Arguments:	1) name		Name of object set to be stored in the hdbBinRepos (will be base of the two filenames)
	//				2) opdir	Full pathname of operational directory
	//
	//	Returns:	E_OPENFAIL	The files could not be opened or created (no space or wrong permissions)
	//				E_OK		Operation was successful.
	//
	//	Errors:		Any false return is due to an irrecoverable error. The calling function
	//				must check the return value.

	_hzfunc("hdbBinRepos::Init") ;

	FSTAT		fs ;		//	File status
	ifstream	is ;		//	Input stream for reading index file
	hzEcode		rc ;		//	Return code

	if (m_nInitState)
		return hzerr(_fn, HZ_ERROR, E_INITDUP, "Resource already initialized") ;

	//	Check we have a name and working directory
	if (!name)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No name") ;
	if (!opdir)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Repository %s: No working directory", *name) ;

	//	Check we do not already have a datacron of the name
	if (m_pADP->GetBinRepos(name))
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Binary datum cron %s already exists", *name) ;

	//	Assign internal structure, name and working directory
	m_Name = name ;
	m_Workdir = opdir ;
	threadLog("%s called with cron name %s and workdir %s\n", *_fn, *m_Name, *m_Workdir) ;

	//	Assert working directory
	if (lstat(*m_Workdir, &fs) == -1)
	{
		rc = AssertDir(*m_Workdir, 0777) ;
		if (rc != E_OK)
			return hzerr(_fn, HZ_ERROR, rc, "Could not assert working dir of %s", *m_Workdir) ;
	}

	//	Set the pathnames for the index and data file
	m_FileIndx = m_Workdir + "/" + m_Name + ".idx" ;
	m_FileData = m_Workdir + "/" + m_Name + ".dat" ;

	if (lstat(*m_FileIndx, &fs) == -1)
		m_nDatum = 0 ;
	else
		m_nDatum = fs.st_size / sizeof(_datum_hd) ;

	if (lstat(*m_FileData, &fs) == -1)
		m_nSize = 0 ;
	else
		m_nSize = fs.st_size ;

	//	Set init state etc
	threadLog("%s. A Initialized %s (%p) count %d with err=%s\n", *_fn, *m_Name, this, m_pADP->CountBinRepos(), Err2Txt(rc)) ;
	rc = m_pADP->RegisterBinRepos(this) ;
	threadLog("%s. B Initialized %s (%p) count %d with files %s and %s. err=%s\n",
		*_fn, *m_Name, this, m_pADP->CountBinRepos(), *m_FileIndx, *m_FileData, Err2Txt(rc)) ;

	m_nInitState = 1 ;
	return E_OK ;
}

hzEcode	hdbBinRepos::Open	(void)
{
	//	Open the hdbBinRepos instance.
	//
	//	This is a matter of opening an input stream for the data file, an output stream for the data file and an output stream for the index file. Note both the
	//	output streams are opened with ios::app as they will only ever append and that there is no input stream for the index file as during normal operation of
	//	a hdbBinRepos, the index is never read.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	If the repository has not been initialized
	//				E_SEQUENCE	If the repository is already open
	//				E_OPENFAIL	If either the index or data file cannot be opened for either reading or writing
	//				E_OK		If the operation was successful

	_hzfunc("hdbBinRepos::Open") ;

	if (m_nInitState == 0)	hzexit(_fn, 0, E_NOINIT, "Cannot open an uninitialized datacron") ;
	if (m_nInitState == 2)	hzexit(_fn, m_Name, E_SEQUENCE, "Datacron is already open") ;

	m_WrI.open(*m_FileIndx, std::ios::app) ;
	if (m_WrI.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot open index file for writing: Repos %s", *m_FileIndx) ;
	threadLog("%s: Opened index file for writing: Repos %s\n", *_fn, *m_FileIndx) ;

	m_WrD.open(*m_FileData, std::ios::app) ;
	if (m_WrD.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open file (%s) for writing", *m_FileData) ;
	threadLog("%s: Opened data file for writing: Repos %s\n", *_fn, *m_FileData) ;

	m_RdI.open(*m_FileIndx) ;
	if (m_RdI.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open index file (%s) for reading", *m_FileIndx) ;
	threadLog("%s: Opened index file for reading: Repos %s\n", *_fn, *m_FileIndx) ;

	m_RdD.open(*m_FileData) ;
	if (m_RdD.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open data file (%s) for reading", *m_FileData) ;
	threadLog("%s: Opened index file for reading: Repos %s\n", *_fn, *m_FileData) ;

	threadLog("%s: INITIALIZED %s\n", *_fn, *m_Name) ;
	m_nInitState = 2 ;

	return E_OK ;
}

hzEcode	hdbBinRepos::Close	(void)
{
	//	Close the datacron.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	The datacron is not initialized
	//				E_SEQUENCE	The datacron is not open
	//				E_OK		The operation was successful

	_hzfunc("hdbBinRepos::Close") ;

	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	if (m_RdI.is_open())	m_RdI.close() ;
	if (m_RdD.is_open())	m_RdD.close() ;
	if (m_WrI.is_open())	m_WrI.close() ;
	if (m_WrD.is_open())	m_WrD.close() ;

	threadLog("%s: CLOSED %s\n", *_fn, *m_Name) ;
	m_nInitState = 1 ;
	return E_OK ;
}

hzEcode	hdbBinRepos::Insert	(uint32_t& datumId, const hzChain& datum)	//, uint32_t an1, uint32_t an2)
{
	//	Purpose:	Insert data contained in the supplied row source. This could be any of the row source
	//				classes. This function is for mass import
	//
	//	Arguments:	1) datumId	The datum id - the address the will occupy (if specified)
	//				2) datum	The datum.
	//
	//	Returns:	E_NOINIT	If this hdbBinRepos instance has not been initialized
	//				E_NODATA	If the supplied datum is of zero size
	//				E_WRITEFAIL	If the data could not be written to disk.
	//				E_OK		If operation successful

	_hzfunc("hdbBinRepos::Insert") ;

	_datum_hd	hdr ;			//	Datum header
	hzEcode		rc = E_OK ;		//	Return code

	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	datumId = 0 ;
	if (!datum.Size())
		return E_NODATA ;

	hdr.m_DTStamp.SysDateTime() ;
	hdr.m_Size = datum.Size() ;
	hdr.m_Prev = 0 ;

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

hzEcode	hdbBinRepos::Update	(uint32_t& datumId, const hzChain& datum)	//, uint32_t an1, uint32_t an2)
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

	_hzfunc("hdbBinRepos::Update") ;

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

hzEcode	hdbBinRepos::Fetch	(hzChain& datum, uint32_t datumId)
{
	//	Purpose:	Fetches a single row of data into the supplied object.
	//
	//	Arguments:	1)	obj		The object to be populated
	//				2)	datumId	The object id
	//
	//	Returns:	E_NOTOPEN	If the reader stream is not open for reading
	//				E_READFAIL	The reader stream is already in a fialed state
	//				E_CORRUPT	Row metadata not as expected. Possible overwrites
	//				E_FORMAT	Data format error
	//				E_OK		The operation was successful

	_hzfunc("hdbBinRepos::Fetch") ;

	_datum_hd	hdr ;			//	Datum header
	uint64_t	nAddr ;			//	Addr of object in file
	uint64_t	nPosn ;			//	Posn of m_Reader
	uint32_t	toGet ;			//	Bytes to get in the next read operation
	uint32_t	soFar ;			//	Bytes read so far
	hzEcode		rc = E_OK ;		//	Return code

	datum.Clear() ;

	//	Check we have an internal structure (have initialized)
	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	//	If object id is zero return blank object
	if (!datumId)
		return E_OK ;

	if (datumId > m_nDatum)
	{
		threadLog("Out of range\n") ;
		rc = E_RANGE ;
	}

	m_Error.Clear() ;

	//	Read in requested object
	m_LockIrd.Lock() ;

		//	Get datum header
		nAddr = (datumId-1) * sizeof(_datum_hd) ;

		m_RdI.seekg((streamoff) nAddr) ;
		nPosn = m_RdI.tellg() ;
		if (nPosn != nAddr)
		{
			rc = E_READFAIL ;
			threadLog("Could not seek header address %ul for datum %d\n", nAddr, datumId) ;
		}
		else
		{
			m_RdI.read((char*) &hdr, sizeof(_datum_hd)) ;
			if (m_RdI.fail())
			{
				rc = E_READFAIL ;
				threadLog("Could not read header for datum %d", datumId) ;
				m_RdI.clear() ;
			}
		}

	m_LockIrd.Unlock() ;

	if (rc != E_OK)
		return rc ;

	m_LockDrd.Lock() ;

		//	Get datum
		m_RdD.seekg((streamoff) hdr.m_Addr) ;
		nPosn = m_RdD.tellg() ;
		if (nPosn != hdr.m_Addr)
		{
			rc = E_READFAIL ;
			threadLog("Could not seek data address %ul for datum %d\n", hdr.m_Addr, datumId) ;
		}
		else
		{
			toGet = HZ_BLOCKSIZE - (nAddr % HZ_BLOCKSIZE) ;
			if (toGet > hdr.m_Size)
				toGet = hdr.m_Size ;
			soFar = 0 ;

			m_RdD.read(m_Data, toGet) ;
			datum.Append(m_Data, toGet) ;
			soFar += toGet ;

			for (; soFar < hdr.m_Size ;)
			{
				toGet = hdr.m_Size - soFar ;
				if (toGet > HZ_BLOCKSIZE)
					toGet = HZ_BLOCKSIZE ;

				m_RdD.read(m_Data, toGet) ;
				datum.Append(m_Data, toGet) ;
				soFar += toGet ;
			}

			if (m_RdD.fail())
				m_RdD.clear() ;
		}

	m_LockDrd.Unlock() ;
	return rc ;
}

hzEcode	hdbBinRepos::Integ	(hzLogger& log)
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

	_hzfunc("hdbBinRepos::Integ") ;

	ifstream	is ;				//	Input stream for index file (separate from the in-built)
	_datum_hd*	pHd ;				//	Datum header pointer
	_datum_hd	arrHd[128] ;		//	Datum header array (128 headers)
	uint32_t	nBytes ;			//	Bytes read in from index file
	uint32_t	nItems ;			//	Number of headers read in from index file
	uint32_t	nC ;				//	Header iterator
	uint32_t	totalItems = 0 ;	//	Bytes to get in the next read operation
	uint32_t	totalSize ;			//	Bytes to get in the next read operation
	hzEcode		rc = E_OK ;			//	Return code

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
			totalItems++ ;
			//log.Out("Datum addr %u id %u time/date %s size %u\n", pHd->m_Addr, totalItems, *pHd->m_DTStamp.Str(), pHd->m_Size) ;

			totalSize += pHd->m_Size ;
		}

		if (nItems < 128)
			break ;
	}

	is.close() ;

	log.Out("%s Total items %u\n", *_fn, totalItems) ;
	log.Out("%s Total size %u\n", *_fn, totalSize) ;
	return rc ;
}

hzEcode	hdbBinRepos::Delete	(uint32_t datumId)
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

	_hzfunc("hdbBinRepos::Delete") ;

	if (m_nInitState < 1)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (m_nInitState < 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Binary datum cron (%s) is not open", *m_Name) ;

	if (datumId < 1 || datumId >= m_nDatum)
		return E_RANGE ;

	return E_OK ;
}
