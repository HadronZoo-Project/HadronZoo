//
//	File:	hzFsTbl.cpp
//
//	Legal Notice: This file is part of the HadronZoo C++ Class Library.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	The HadronZoo C++ Class Library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
//	as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
//	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with the HadronZoo C++ Class Library. If not, see
//	http://www.gnu.org/licenses.
//

#include <iostream>
#include <fstream>

using namespace std ;

#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hzDirectory.h"
#include "hzTextproc.h"
#include "hzFsTbl.h"

global	hzFsTbl*	_hzGlobal_StringTable ;	//	The singleton string table for the application
global	hzFsTbl*	_hzGlobal_FST_Domain ;	//	The singleton domain table for the application
global	hzFsTbl*	_hzGlobal_FST_Emaddr ;	//	The singleton emaddr table for the application

static	hzString	s_dict_blocks_name = "strtbl blocks" ;
static	hzString	s_dict_string_name = "strtbl strings" ;

/*
**	Private hzFsTbl Functions
*/

hzEcode	hzFsTbl::_loadStrings	(const hzString& backupFilename)
{
	//	Initialize file backup
	//
	//	This function may only be called after construction but before any strings have been added. The content file pathname is set by this function. If such a
	//	file exists and is of the correct format, it will be read in to populate the dictionary, otherwise it will be created. Then, an output stream is opened
	//	and left open during runtime. During normal operation, any new words in the dictionary are committed to file by Insert()
	//
	//	The strings in the file are of the form "string_number value" where the string number is represented as block_number/block_position. The strings are in
	//	order of incidence since the file is always appended on the introduction of a new string. The strings are read in line by line however the m_Strings The purpose of the file is to preserve the string number value
	//	relationship. This means this function cannot simply read in the strings and call Insert() since this would assign different string 
	//
	//	Argument:	backupFilename	The full path of the dictionary backup file
	//
	//	Returns:	E_SEQUENCE	If the operation is being attempted on an already populated hzFsTbl instance
	//				E_FORMAT	If there are file format errors
	//				E_CORRUPT	If any string is given a different address to that in the backup file
	//				E_OK		If the strings were loaded and the backup file open for updates

	_hzfunc("hzFsTbl::_loadStrings") ;

	ifstream		is ;			//	Temporary input stream
	char*			pBuf ;			//	Temporary buffer
	char*			i ;				//	Buffer iterator
	char*			pWord ;			//	String destination
	const char*		ptr ;			//	Existing string if any
	uint32_t		blkNo ;			//	Block number part of string number
	uint32_t		nOset ;			//	Block offset part of string number
	//uint32_t		nLen ;			//	Word length
	uint32_t		strNo_rec ;		//	String number as recorded in the file
	uint32_t		strNo_ins ;		//	String number as assigned by Insert()
	uint32_t		strPosn ;		//	String position in file (line number)
	hzEcode			rc = E_OK ;		//	Return code

	//	Check population is zero
	if (m_Strings.Count() > 1)
		return E_SEQUENCE ;

	threadLog("Called %s on %s\n", *_fn, *backupFilename) ;

	m_Filename = backupFilename ;

	//	If file exists, read it in to import data
	is.open(*m_Filename) ;
	if (!is.fail())
	{
		pBuf = new char[10240] ;

		for (strPosn = 1 ; rc == E_OK ; strPosn++)
		{
			is.getline(pBuf, 10239) ;

			if (!is.gcount())
				break ;

			if (!pBuf[0])
				continue ;

			pBuf[is.gcount()] = 0 ;

			//	String number is first number, then address is ddddd/ddddd then string value
			blkNo = 0; ;
			for (i = pBuf ; IsDigit(*i) ; i++)
				{ blkNo *= 10 ; blkNo += (*i - '0') ; }

			if (*i != CHAR_FWSLASH)
				{ rc = E_FORMAT ; break ; }

			nOset = 0 ;
			for (i++ ; IsDigit(*i) ; i++)
				{ nOset *= 10 ; nOset += (*i - '0') ; }

			if (*i != CHAR_SPACE)
				{ rc = E_FORMAT ; break ; }

			pWord = ++i ;
			//nLen = strlen(pWord) ;

			if (blkNo > 0xffff || nOset > 0xffff)
				{ rc = E_FORMAT ; break ; }

			strNo_rec = blkNo << 16 ;
			strNo_rec |= nOset ;

			strNo_ins = Insert(pWord) ;
			if (strNo_ins != strNo_rec)
				{ rc = E_CORRUPT ; threadLog("%s. String number %u is now %u (%s)\n", *_fn, strNo_rec, strNo_ins, ptr) ; break ; }
		}
		is.close() ;
		delete pBuf ;
	}

	//	Create and open output stream
	if (rc == E_OK)
		m_Outstream.open(*m_Filename, ios::app) ;

	return rc ;
}

/*
**	Public hzFsTbl Functions
*/

hzFsTbl::hzFsTbl	(void)
{
	m_pLastBlock = 0 ;
	m_nInserts = 0 ;

	m_Blocks.SetLock(HZ_MUTEX) ;
	m_Blocks.SetName(s_dict_blocks_name) ;
	m_Strings.SetLock(HZ_MUTEX) ;
	m_Strings.SetName(s_dict_string_name) ;
}

hzFsTbl::~hzFsTbl	(void)
{
	Clear() ;
}

hzFsTbl*	hzFsTbl::StartStrings	(const hzString& backupFilename)
{
	//	If the singleton string table already exists, simply return the pointer to it. Otherwise create and initialize it, then return the pointer.
	//
	//	The initialization involves creation of the first 64K block and creation of the first string as just the null terminator. This ensures that the string number 0 is always a
	//	NULL string.
	//
	//	Arguments:	None
	//	Returns:	Pointer to the string table

	_hzfunc("hzFsTbl::StartStrings") ;

	_fxstr_blk*	pBlk ;		//	Destination block
	hzEcode		rc ;		//	Return code

	if (!_hzGlobal_StringTable)
	{
		_hzGlobal_StringTable = new hzFsTbl() ;

		//	In first block, leave the first byte of the space as a null terminator so the zeroth word is always null
		_hzGlobal_StringTable->m_pLastBlock = pBlk = new _fxstr_blk() ;
		_hzGlobal_StringTable->m_Blocks.Add(pBlk) ;
		_hzGlobal_StringTable->m_pLastBlock->m_Usage = 1 ;
		_hzGlobal_StringTable->m_Strings.Add(0) ;
	}

	if (!backupFilename)
		return _hzGlobal_StringTable ;

	//	Load pre-existing strings from backup file if it exists and is populated
	rc = _hzGlobal_StringTable->_loadStrings(backupFilename) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Failed to load STRINGS") ;

	return _hzGlobal_StringTable ;
}

hzFsTbl*	hzFsTbl::StartDomains	(const hzString& backupFilename)
{
	//	If the singleton string table already exists, simply return the pointer to it. Otherwise create and initialize it, then return the pointer.
	//
	//	The initialization involves creation of the first 64K block and creation of the first string as just the null terminator. This ensures that the string number 0 is always a
	//	NULL string.
	//
	//	Arguments:	None
	//	Returns:	Pointer to the string table

	_hzfunc("hzFsTbl::StartDomains") ;

	_fxstr_blk*	pBlk ;		//	Destination block
	hzEcode		rc ;		//	Return code

	if (!_hzGlobal_FST_Domain)
	{
		_hzGlobal_FST_Domain = new hzFsTbl() ;

		//	In first block, leave the first byte of the space as a null terminator so the zeroth word is always null
		_hzGlobal_FST_Domain->m_pLastBlock = pBlk = new _fxstr_blk() ;
		_hzGlobal_FST_Domain->m_Blocks.Add(pBlk) ;
		_hzGlobal_FST_Domain->m_pLastBlock->m_Usage = 1 ;
		_hzGlobal_FST_Domain->m_Strings.Add(0) ;
	}

	if (!backupFilename)
		return _hzGlobal_FST_Domain ;

	//	Load pre-existing strings from backup file if it exists and is populated
	rc = _hzGlobal_FST_Domain->_loadStrings(backupFilename) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Failed to load DOMAINS") ;

	return _hzGlobal_FST_Domain ;
}

hzFsTbl*	hzFsTbl::StartEmaddrs	(const hzString& backupFilename)
{
	//	If the singleton string table already exists, simply return the pointer to it. Otherwise create and initialize it, then return the pointer.
	//
	//	The initialization involves creation of the first 64K block and creation of the first string as just the null terminator. This ensures that the string number 0 is always a
	//	NULL string.
	//
	//	Arguments:	None
	//	Returns:	Pointer to the string table

	_hzfunc("hzFsTbl::StartEmaddrs") ;

	_fxstr_blk*	pBlk ;		//	Destination block
	hzEcode		rc ;		//	Return code

	if (!_hzGlobal_FST_Emaddr)
	{
		_hzGlobal_FST_Emaddr = new hzFsTbl() ;

		//	In first block, leave the first byte of the space as a null terminator so the zeroth word is always null
		_hzGlobal_FST_Emaddr->m_pLastBlock = pBlk = new _fxstr_blk() ;
		_hzGlobal_FST_Emaddr->m_Blocks.Add(pBlk) ;
		_hzGlobal_FST_Emaddr->m_pLastBlock->m_Usage = 1 ;
		_hzGlobal_FST_Emaddr->m_Strings.Add(0) ;
	}

	if (!backupFilename)
		return _hzGlobal_FST_Emaddr ;

	//	Load pre-existing strings from backup file if it exists and is populated
	rc = _hzGlobal_FST_Emaddr->_loadStrings(backupFilename) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Failed to load EMADDRS") ;

	return _hzGlobal_FST_Emaddr ;
}

uint32_t	hzFsTbl::Insert	(const char* str)
{
	//	Insert the supplied null terminated string into the string table if it does not already exists. Return the string number in any event.
	//
	//	Arguments:	1)	str	The null terminated string to be inserted
	//
	//	Returns:	Number being the string id

	_hzfunc("hzFsTbl::Insert") ;

	const char*	ptr ;		//	The string value to compare
	uint32_t	nMax ;		//	Number of string -1
	uint32_t	nDiv ;		//	Binary chop divider
	uint32_t	nPos ;		//	Starting position
	uint32_t	nLen ;		//	len of new string/limit checker
	uint32_t	strNo ;		//	String number found or new
	int32_t		res ;		//	Result of compare

	if (!str || !str[0])
		return 0 ;

	m_nInserts++ ;
	nPos = 0 ;

	if (!m_pLastBlock)
	{
		m_pLastBlock = new _fxstr_blk() ;
		m_Blocks.Add(m_pLastBlock) ;
		m_pLastBlock->m_Usage = 1 ;
		m_Strings.Add(0) ;
		strNo = nPos = 1 ;
	}
	else
	{
		//	Set up initial values
		nMax = m_Strings.Count() - 1 ;
		for (nLen = 2 ; nLen < nMax ; nLen *= 2) ;
		nDiv = nLen / 2 ;
		nPos = nLen - 1 ;

		for (;;)
		{
			if (nPos > nMax)
			{
				if (!nDiv)
					break ;
				nPos -= nDiv ;
				nDiv /= 2 ;
				continue ;
			}

			if (!nPos)
				res = 1 ;
			else
			{
				strNo = m_Strings[nPos] ;
				ptr = Xlate(strNo) ;
				if (!ptr)
					Fatal("%s: Position %d xlate to null str\n", *_fn, nPos) ;
				res = strcmp(ptr, str) ;
			}

			if (res > 0)
			{
				//	Go higher
				if (!nDiv)
					{ nPos++ ; break ; }
				nPos += nDiv ;
				nDiv /= 2 ;
				continue ;
			}

			if (res < 0)
			{
				//	Go lower
				if (!nDiv)
					break ;
				nPos -= nDiv ;
				nDiv /= 2 ;
				continue ;
			}

			//	The string already has a string number so just return it
			return strNo ;
		}
	}

	/*
	**	New string so insert it at the curent position
	*/

	//	First check space on last block
	nLen = strlen(str) + 1 ;
	if (nLen >= HZ_STRTBL_BLKSZE)
	{
		threadLog("%s. WARNING Attempting to add a string of %d bytes\n", *_fn, nLen) ;
		return 0 ;
	}

	if ((nLen + m_pLastBlock->m_Usage) > HZ_STRTBL_BLKSZE)
	{
		threadLog("%s Adding a new block after %d inserts and %d strings inserted\n", *_fn, m_nInserts, m_Strings.Count()) ;
		m_pLastBlock = new _fxstr_blk() ;
		m_Blocks.Add(m_pLastBlock) ;
	}

	//	Write the new string to the last block
	strcpy(m_pLastBlock->m_Space + m_pLastBlock->m_Usage, str) ;
	strNo = ((m_Blocks.Count() - 1) << 16) + m_pLastBlock->m_Usage ;
	m_pLastBlock->m_Usage += nLen ;
	m_Strings.Insert(strNo, nPos) ;

	if (m_Filename)
	{
		//	Export the new string to backup file
		char	buf [32] ;

		sprintf(buf, "%05d/%05d ", ((strNo & 0xffff0000) >> 16), strNo & 0x0000ffff) ;
		m_Outstream << buf << str << "\n" ;
		m_Outstream.flush() ;
	}

	return strNo ;
}

const char*	hzFsTbl::Xlate	(uint32_t strNo) const
{
	//	Translate string number to actual null terminated string
	//
	//	Arguments:	1)	strNo	The string number
	//
	//	Returns:	Pointer to string if the supplied string number is valid
	//				NULL if the supplied string number does not exist
	//
	//	Note that a string number of 0 is invalid as the minimum usage is a block is 2 (sizeof the usage counter itself). Note also that the string returned is
	//	not byte aligned. This contrasts with hzString where the actual char* value is always aligned along 8 byte boundaries.

	_hzfunc("hzFsTbl::Xlate") ;

	_fxstr_blk*	pBlk ;		//	The block where target string is located
	uint32_t	blkNo ;		//	Block number
	uint32_t	bOset ;		//	Block offset

	if (!strNo)
		return 0 ;

	blkNo = (strNo & 0xffff0000) >> 16 ;
	bOset = strNo & 0x0000ffff ;

	pBlk = m_Blocks[blkNo] ;
	if (!pBlk)
		return 0 ;
	if (bOset >= pBlk->m_Usage)
		return 0 ;

	return pBlk->m_Space + bOset ;
}

const char*	hzFsTbl::GetStr	(uint32_t nth)
{
	//	Get the Nth string in the dictionary
	//
	//	Arguments:	1)	strNo	The string position
	//
	//	Returns:	Pointer to string if the supplied string number is valid
	//				NULL if the supplied string number does not exist
	//
	//	Note that a string number of 0 is invalid as the minimum usage is a block is 2 (sizeof the usage counter itself). Note also that the string returned is
	//	not byte aligned. This contrasts with hzString where the actual char* value is always aligned along 8 byte boundaries.

	_hzfunc("hzFsTbl::GetStr") ;

	const char*	ptr ;		//	The string value to compare
	uint32_t	found ;		//	Number found/limit checker

	if (!nth || nth > m_Strings.Count())
		return 0 ;

	found = m_Strings[nth-1] ;
	ptr = Xlate(found) ;

	return ptr ;

	//	return Xlate(m_Strings[nth-1]) ;
}

uint32_t	hzFsTbl::Locate	(const char* str) const
{
	//	Locate the string number for the given string
	//
	//	Arguments:	1)	str	The test string
	//
	//	Returns:	>0	String id if the string exists in the string table
	//				0	If string not found

	_hzfunc("hzFsTbl::Locate") ;

	const char*	ptr ;		//	The string value to compare
	uint32_t	nMax ;		//	Number of string -1
	uint32_t	nDiv ;		//	Binary chop divider
	uint32_t	nPos ;		//	Starting position
	uint32_t	found ;		//	Number found/limit checker
	int32_t		res ;		//	Result of compare

	if (!this)
		Fatal("%s. No Dictionary present\n", *_fn) ;

	if (!m_Blocks.Count())
		return 0 ;
	if (!str || !str[0])
		return 0 ;

	nMax = m_Strings.Count() - 1 ;
	for (found = 2 ; found < nMax ; found *= 2) ;
	nDiv = found / 2 ;
	nPos = found - 1 ;

	for (;;)
	{
		if (nPos > nMax)
		{
			if (!nDiv)
				break ;
			nPos -= nDiv ;
			nDiv /= 2 ;
			continue ;
		}

		found = m_Strings[nPos] ;
		ptr = Xlate(found) ;
		//	res = StringCompare(ptr, str) ;
		//	threadLog("comparing %s to %s\n", ptr, str) ;
		res = CstrCompare(ptr, str) ;

		if (res > 0)
		{
			//	Go higher
			if (!nDiv)
				break ;
			nPos += nDiv ;
			nDiv /= 2 ;
			continue ;
		}

		if (res < 0)
		{
			//	Go lower
			if (!nDiv)
				break ;
			nPos -= nDiv ;
			nDiv /= 2 ;
			continue ;
		}

		return found ;
	}

	return 0 ;
}

hzEcode	hzFsTbl::Export	(const hzString& fullpath)
{
	//	Export the string table
	//
	//	Arguments:	1)	The full path of export file
	//
	//	Returns:	E_ARGUMENT	If not export file path has been provided
	//				E_OPENFAIL	If the file could not be opened
	//				E_OK		If the opperation was successful

	_hzfunc("hzFsTbl::Export") ;

	std::ofstream	os ;		//	Output stream
	hzChain			L ;			//	Line
	const char*		str ;		//	Null termiated string
	uint32_t		n ;			//	String iterator
	uint32_t		strNo ;		//	String number
	uint32_t		blkNo ;		//	Block number
	uint32_t		bOset ;		//	Block offset
	hzEcode			rc ;		//	Return code

	//	If the export is to the existing content file, do nothing
	if (m_Filename == fullpath)
		return E_OK ;

	if (!fullpath)
	{
		threadLog("%s: Total number of strings %d, no of insert calls %d\n", *_fn, m_Strings.Count(), m_nInserts) ;
		return hzerr(_fn, HZ_WARNING, E_ARGUMENT, "No export file provided") ;
	}

	os.open(*fullpath) ;
	if (os.fail())
		return hzerr(_fn, HZ_WARNING, E_OPENFAIL, "Could not open export file %s", *fullpath) ;

	for (n = 0 ; n < m_Strings.Count() ; n++)
	{
		strNo = m_Strings[n] ;
		blkNo = (strNo & 0xffff0000) >> 16 ;
		bOset = strNo & 0x0000ffff ;

		str = Xlate(strNo) ;

		L.Printf("[%u] [%05d/%05d] %s\n", n, blkNo, bOset, str) ;
		os << L ;
		L.Clear() ;
	}

	L.Printf("Total number of strings %d, no of insert calls %d\n", m_Strings.Count(), m_nInserts) ;
	os << L ;
	os.close() ;

	return E_OK ;
}

void	hzFsTbl::Clear	(void)
{
	//	Clear everything
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzFsTbl::Clear") ;

	_fxstr_blk*	pBlock ;	//	Block pointer
	uint32_t	n ;			//	Block iterator

	for (n = 0 ; n < m_Blocks.Count() ; n++)
	{
		pBlock = m_Blocks[n] ;
		delete pBlock ;
	}

	m_Blocks.Clear() ;
	m_Strings.Clear() ;
}

/*
**	hzFixStr Functions
*/

hzFixStr	hzFixStr::operator=	(const char* cstr)
{
	if (!cstr || !cstr[0])
		m_strno = 0 ;
	else
	{
		m_strno = _hzGlobal_StringTable->Locate(cstr) ;
		if (!m_strno)
			m_strno = _hzGlobal_StringTable->Insert(cstr) ;
	}

	return *this ;
}

const char*	hzFixStr::operator*   (void) const
{
	return _hzGlobal_StringTable->Xlate(m_strno) ;
}

bool	hzFixStr::operator<	(const hzFixStr& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_StringTable->Xlate(m_strno) ;
	suppStr = _hzGlobal_StringTable->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) < 0 ? true : false ;
}

bool	hzFixStr::operator<=	(const hzFixStr& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_StringTable->Xlate(m_strno) ;
	suppStr = _hzGlobal_StringTable->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) <= 0 ? true : false ;
}

bool	hzFixStr::operator>	(const hzFixStr& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_StringTable->Xlate(m_strno) ;
	suppStr = _hzGlobal_StringTable->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) > 0 ? true : false ;
}

bool	hzFixStr::operator>=	(const hzFixStr& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_StringTable->Xlate(m_strno) ;
	suppStr = _hzGlobal_StringTable->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) >= 0 ? true : false ;
}

/*
**	hzFxDomain Functions
*/

hzFxDomain	hzFxDomain::operator=	(const char* cstr)
{
	if (!cstr || !cstr[0])
		m_strno = 0 ;
	else
	{
		m_strno = _hzGlobal_FST_Domain->Locate(cstr) ;
		if (!m_strno)
			m_strno = _hzGlobal_FST_Domain->Insert(cstr) ;
	}

	return *this ;
}

const char*	hzFxDomain::operator*   (void) const
{
	return _hzGlobal_FST_Domain->Xlate(m_strno) ;
}

bool	hzFxDomain::operator<	(const hzFxDomain& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_FST_Domain->Xlate(m_strno) ;
	suppStr = _hzGlobal_FST_Domain->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) < 0 ? true : false ;
}

bool	hzFxDomain::operator<=	(const hzFxDomain& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_FST_Domain->Xlate(m_strno) ;
	suppStr = _hzGlobal_FST_Domain->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) <= 0 ? true : false ;
}

bool	hzFxDomain::operator>	(const hzFxDomain& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_FST_Domain->Xlate(m_strno) ;
	suppStr = _hzGlobal_FST_Domain->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) > 0 ? true : false ;
}

bool	hzFxDomain::operator>=	(const hzFxDomain& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_FST_Domain->Xlate(m_strno) ;
	suppStr = _hzGlobal_FST_Domain->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) >= 0 ? true : false ;
}

/*
**	hzFxEmaddr Functions
*/

hzFxEmaddr	hzFxEmaddr::operator=	(const char* cstr)
{
	m_strno = 0 ;

	if (cstr && cstr[0])
	{
		m_strno = _hzGlobal_FST_Emaddr->Locate(cstr) ;
		if (!m_strno)
		{
			if (IsEmaddr(cstr))
				m_strno = _hzGlobal_FST_Emaddr->Insert(cstr) ;
		}
	}

	return *this ;
}

const char*	hzFxEmaddr::operator*   (void) const
{
	return _hzGlobal_FST_Emaddr->Xlate(m_strno) ;
}

bool	hzFxEmaddr::operator<	(const hzFxEmaddr& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_FST_Emaddr->Xlate(m_strno) ;
	suppStr = _hzGlobal_FST_Emaddr->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) < 0 ? true : false ;
}

bool	hzFxEmaddr::operator<=	(const hzFxEmaddr& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_FST_Emaddr->Xlate(m_strno) ;
	suppStr = _hzGlobal_FST_Emaddr->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) <= 0 ? true : false ;
}

bool	hzFxEmaddr::operator>	(const hzFxEmaddr& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_FST_Emaddr->Xlate(m_strno) ;
	suppStr = _hzGlobal_FST_Emaddr->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) > 0 ? true : false ;
}

bool	hzFxEmaddr::operator>=	(const hzFxEmaddr& op)
{
	const char*	thisStr ;		//	This string ID actual value
	const char*	suppStr ;		//	Supplied value for comparison

	thisStr = _hzGlobal_FST_Emaddr->Xlate(m_strno) ;
	suppStr = _hzGlobal_FST_Emaddr->Xlate(op.m_strno) ;

	return strcmp(thisStr, suppStr) >= 0 ? true : false ;
}
