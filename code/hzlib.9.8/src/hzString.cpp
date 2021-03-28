//
//	File:	hzString.cpp
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

#include <stdarg.h>
#include <execinfo.h>

#include "hzChars.h"
#include "hzCtmpls.h"
#include "hzProcess.h"
#include "hzTextproc.h"
#include "hzString.h"

/*
**	String addressing markers
**
**	String address. This is a 32-bit entity encoded as follows: The most significant 3 bits state if the string is directly addressed by a pointer - or held
**	in a slot in a managed block. If all 3 bits are zero, direct addressing applies and the remaining 29 bits serve as the offset into a vector of pointers.
**	If the 3 bits are non-zero, then they serve to state which block management regime holds the string. In this case the next 21 bits form the block number
**	and the remaining 8 the slot number.
*/

#define HZ_STRADDR_OSIZE	0x80000000		//	Top bit set indicates string is 'oversized'
#define HZ_STRADDR_OMASK	0x7fffffff		//	Oversized address mask (remaining 31 bits)
#define HZ_STRADDR_BLKID	0x7FFF0000		//	Mid 20 bits for block part of address

#define HZ_STRING_FACTOR	5				//	Size of string space meta data plus null terminator
#define HZ_STRING_SBSPACE	65536			//	Total size of the string space blocks in multiples of 8 bytes
#define HZ_STRING_PBLK_SZ	1024			//	Number of pointers in a _ptrBloc

/*
**	Definitions
*/

struct	_strFLE
{
	//	The slot is used as an entry in a free list. Upon string space allocation, slots are either advanced or removed from the free list. The space occupied
	//	by the slot is then used as string space.

	uint32_t	m_fleSelf ;		//	Address of this slot
	uint32_t	m_fleNext ;		//	Pointer to next free slot
} ;

struct	_strBloc
{
	//	Small string space superblock

	uint32_t	m_blkSelf ;						//	Address of block
	uint32_t	m_Usage ;						//	Space used (so position of free space)
	uint64_t	m_Space[HZ_STRING_SBSPACE] ;	//	Areas for string spaces
	uchar		m_Alloc[HZ_STRING_SBSPACE] ;	//	Temp buffer to check allocs and frees
} ;

struct	_ptrBloc
{
	//	Pointer block. Currently only used to accomodate oversized strings. Each block has 1,024 64-bit entities that in the root node, are treated as pointers
	//	to data level nodes. At at the data level, these are cast to string space pointers (upon address translation). The pointer blocks are arraged as a fixed
	//	two-tier system with a root pointer block addressing 1024 pointer blocks that each address 1024 string spaces.

	uint64_t	m_Ptrs[HZ_STRING_PBLK_SZ] ;		//	Pointers to next block or to entity

	_ptrBloc	(void)	{ memset(m_Ptrs, 0, HZ_STRING_PBLK_SZ * sizeof(uint64_t)) ; }
} ;

class	_strItem
{
	//	hzString control area

public:
	uchar		m_copy ;		//	Copy count (min value 1)
	uchar		m_xple ;		//	Sizes above 64Kb
	uint16_t	m_xize ;		//	Size
	char		m_data[4] ;		//	Start of string data

	_strItem	(void)	{ m_copy = m_xple = 0 ; m_xize = 0 ; m_data[0] = 0 ; }

	void		_setSize	(uint32_t nSize)	{ m_xple = (uchar) ((nSize & 0x070000) >> 16) ; m_xize = (uint16_t) (nSize & 0xffff) ; }
	uint32_t	_getSize	(void)				{ return (m_xple << 16) + m_xize ; }
} ;

class	_strRegime
{
public:
	//	Blocks
	_ptrBloc	m_Osize ;			//	Root pointer block for oversized strings
	_strBloc*	m_Super[4096] ;		//	Max of 4096 string value 512K superblocks
	_strBloc*	m_pTopBlock ;		//	Latest small string space superblock (only one from which new string spaces can be allocated)

	//	Locks
	hzLockRWD	m_lockSmall[32] ;	//	Small string allocation locks
	hzLockRWD	m_lockSbloc ;		//	Lock for allocating superblocks
	hzLockRWD	m_lockOsize ;		//	Lock for allocating/freeing of oversize strings

	//	Freelists (depends on size)
	uint32_t	m_flistSmall[32] ;	//	Freelists for string spaces of sizes 8 to 256 bytes
	uint32_t	m_flistOsize ;		//	Single freelist for string spaces over 256 bytes

	uint32_t	m_nBloc ;			//	Population of small string superblocks
	uint32_t	m_nOver ;			//	Population of oversize strings

	_strRegime	()
	{
		//printf("STR REGIME CONSTRUCTOR\n") ;
		memset(m_flistSmall, 0, 32 * sizeof(uint32_t)) ;
		memset(m_Super, 0, 4096 * sizeof(_strBloc*)) ;
		m_flistOsize = 0 ;
		m_pTopBlock = 0 ;
		m_nBloc = m_nOver = 0 ;
	}
} ;

/*
**	Variables
*/

static	_strRegime*	s_pStrRegime ;	//	The one and only string regime

global	const hzString	_hzGlobal_nullString ;							//	Null string
global	const hzString	_hzString_TooLong = "-- String Too Long --" ;	//	To be returned when limit exceeded
global	const hzString	_hzString_Fault = "-- String Fault --" ;		//	To be returned when string corrupted

static	char	_hz_NullBuffer	[8] ;

void*	_strXlate	(uint32_t strAddr)
{
	_hzfunc(__func__) ;

	_strBloc*	pBloc ;		//	Pointer to superblock
	_ptrBloc*	pPB ;		//	Pointer block
	uint64_t*	pSeg ;		//	Data segment
	uint32_t	blkNo ;		//	Offset within vector of blocks
	uint32_t	slotNo ;	//	String slot within block

	if (!strAddr)
		return 0 ;

	//if ((strAddr & HZ_STRADDR_MASK) == HZ_STRADDR_OSIZE)
	if (strAddr & HZ_STRADDR_OSIZE)
	{
		slotNo = strAddr & HZ_STRADDR_OMASK ;
		slotNo-- ;
		blkNo = slotNo/HZ_STRING_PBLK_SZ ;
		pPB = (_ptrBloc*) s_pStrRegime->m_Osize.m_Ptrs[blkNo] ;

		return (void*) pPB->m_Ptrs[slotNo % HZ_STRING_PBLK_SZ] ;
	}

	//	String is <= 256 bytes
	slotNo = strAddr & 0xffff ;
	blkNo = (strAddr & HZ_STRADDR_BLKID) >> 16 ;

	if (blkNo > s_pStrRegime->m_nBloc)
		{ threadLog("%s. CORRUPT: Cannot xlate address %u:%u. No such superblock (%u issued)\n", *_fn, blkNo, slotNo, s_pStrRegime->m_nBloc) ; return 0 ; }

	pBloc = s_pStrRegime->m_Super[blkNo-1] ;
	if (!pBloc)
		{ threadLog("%s. CORRUPT: No block found for address %u:%u. Total of %u superblocks issued)\n", *_fn, blkNo, slotNo, s_pStrRegime->m_nBloc) ; return 0 ; }

	pSeg = pBloc->m_Space + slotNo ;
	return (void*) pSeg ;
}

bool	_strTest	(uint32_t strAddr, uint32_t nSize)
{
	_strBloc*	pBloc ;		//	Pointer to superblock
	uint32_t	blkNo ;		//	Offset within vector of blocks
	uint32_t	slotNo ;	//	String slot within block

	if (strAddr & HZ_STRADDR_OSIZE)
		return true ;

	slotNo = strAddr & 0xffff ;
	blkNo = (strAddr & HZ_STRADDR_BLKID) >> 16 ;
	pBloc = s_pStrRegime->m_Super[blkNo-1] ;

	if (pBloc->m_Alloc[slotNo] == (nSize-1))
		return true ;

	threadLog("TEST: CORRUPT: Address %u:%u has size %u, not %u\n", blkNo, slotNo, pBloc->m_Alloc[slotNo]+1, nSize) ;
	return false ;
}

uint32_t	_strAlloc	(uint32_t nSize)
{
	//	Allocate memory from the memory regime if required size is 64 bytes or less and from the heap otherwise. Under the regime, allocation is
	//	always from a freelist of segments of either 16,24,32,48 or 64 bytes. If the appropriate freelist is empty, a new block is allocated and
	//	divided into multiple segments.
	//
	//	Argument:	nSize	Total size required (including sting space meta data and null terminator)
	//
	//	Returns:	Pointer to the required string space. If is a fatal condition if this cannot be obtained.

	_hzfunc(__func__) ;

	uint32_t	strAddr = 0 ;	//	Address of string (block + slot)
	uint32_t	nUnit ;			//	Number of 8-byte units required

	if (!s_pStrRegime)
		s_pStrRegime = new _strRegime() ;

	if (!nSize)
		return 0 ;
	nUnit = (nSize/8) + (nSize%8 ? 1:0) ;

	if (nSize <= 256)
	{
		_strBloc*	pBloc = 0 ;		//	Pointer to superblock
		_strFLE*	pSlot = 0 ;		//	Pointer to freelist slot
		uint64_t*	pSeg = 0 ;		//	Pointer to segment

		//	Small string space (8 to 256 bytes)
		if (s_pStrRegime->m_flistSmall[nUnit-1])
		{
			if (_hzGlobal_MT)
				s_pStrRegime->m_lockSmall[nUnit-1].LockWrite() ;

			if (s_pStrRegime->m_flistSmall[nUnit-1])
			{
				//	Slot of the exact size needed is free so grab it

				_hzGlobal_Memstats.m_strSm_u[nUnit-1]++ ;
				_hzGlobal_Memstats.m_strSm_f[nUnit-1]-- ;

				strAddr = s_pStrRegime->m_flistSmall[nUnit-1] ;
				pSlot = (_strFLE*) _strXlate(strAddr) ;
				if (!pSlot)
					hzexit(_fn, 0, E_CORRUPT, "Illegal freelist (%d) string address %u:%u", nUnit-1, (strAddr&0x7fff0000)>>16, strAddr&0xffff) ;

				//	Since the slot is being allocated from the free list, it should contain its own address
				if (pSlot->m_fleSelf != strAddr)
					threadLog("%s: CORRUPT: %u unit Slot in free list addr (%u:%u), points elsewhere\n", *_fn, nUnit, (strAddr&0xffff0000)>>16, strAddr&0xffff) ;

				s_pStrRegime->m_flistSmall[nUnit-1] = pSlot->m_fleNext ;
				memset(pSlot, 0, nUnit * 8) ;
			}

			if (_hzGlobal_MT)
				s_pStrRegime->m_lockSmall[nUnit-1].Unlock() ;

			if (strAddr)
			{
				pBloc = s_pStrRegime->m_Super[((strAddr & HZ_STRADDR_BLKID)>>16)-1] ;
				pBloc->m_Alloc[strAddr&0xffff] = nSize-1 ;
				return strAddr ;
			}
		}

		//	Are there larger slots free?

		//	No free slots so create another superblock
		if (_hzGlobal_MT)
			s_pStrRegime->m_lockSbloc.LockWrite() ;

		if (!s_pStrRegime->m_pTopBlock || ((s_pStrRegime->m_pTopBlock->m_Usage + nUnit) > HZ_STRING_SBSPACE))
		{
			//	Assign any remaining free space on the highest block to the small freelist of the size

			//	Then create a new highest block
			s_pStrRegime->m_pTopBlock = pBloc = new _strBloc() ;
			memset(pBloc, 0, sizeof(_strBloc)) ;

			s_pStrRegime->m_Super[s_pStrRegime->m_nBloc] = pBloc ;
			s_pStrRegime->m_nBloc++ ;
			_hzGlobal_Memstats.m_numSblks = s_pStrRegime->m_nBloc ;

			pBloc->m_blkSelf = (s_pStrRegime->m_nBloc << 16) ;
			pBloc->m_Usage = 0 ;

			if (s_pStrRegime->m_nBloc > 1)
				threadLog("tid %ul CREATED SUPERBLOCK %u at %p\n", pthread_self(), pBloc->m_blkSelf >> 16, pBloc) ;
		}

		//	Assign from the superblock free space
		pSeg = s_pStrRegime->m_pTopBlock->m_Space + s_pStrRegime->m_pTopBlock->m_Usage ;
		strAddr = s_pStrRegime->m_pTopBlock->m_blkSelf + s_pStrRegime->m_pTopBlock->m_Usage ;
		s_pStrRegime->m_pTopBlock->m_Alloc[strAddr&0xffff] = nSize-1 ;
		s_pStrRegime->m_pTopBlock->m_Usage += nUnit ;
		memset(pSeg, 0, nUnit * 8) ;
		_hzGlobal_Memstats.m_strSm_u[nUnit-1]++ ;

		if (_hzGlobal_MT)
			s_pStrRegime->m_lockSbloc.Unlock() ;
		return strAddr ;
	}

	//	Size > 256
	_ptrBloc*	pPB ;		//	Applicable pointer block
	uint64_t	ptr ;		//	Address as uint64_t
	void*		pVoid ;		//	Address allocated as void*

	if (_hzGlobal_MT)
		s_pStrRegime->m_lockOsize.LockWrite() ;

	_hzGlobal_Memstats.m_numStrOver++ ;
	_hzGlobal_Memstats.m_ramStrOver += (nUnit * 8) ;

	if (s_pStrRegime->m_flistOsize)
	{
		//	Use a previously freed large string pointer
		strAddr = s_pStrRegime->m_flistOsize ;

		pPB = (_ptrBloc*) s_pStrRegime->m_Osize.m_Ptrs[(strAddr-1)/HZ_STRING_PBLK_SZ] ;
		ptr = (uint64_t) pPB->m_Ptrs[(strAddr-1) % HZ_STRING_PBLK_SZ] ;
		s_pStrRegime->m_flistOsize = ptr & 0xffffffff ;

		pVoid = (void*) new char[nUnit * 8] ;
		pPB->m_Ptrs[(strAddr-1) % HZ_STRING_PBLK_SZ] = (uint64_t) pVoid ;
	}
	else
	{
		//	Allocate a new large string pointer
		pPB = (_ptrBloc*) s_pStrRegime->m_Osize.m_Ptrs[s_pStrRegime->m_nOver/HZ_STRING_PBLK_SZ] ;
		if (!pPB)
		{
			pVoid = new _ptrBloc() ;
			s_pStrRegime->m_Osize.m_Ptrs[s_pStrRegime->m_nOver/HZ_STRING_PBLK_SZ] = (uint64_t) pVoid ;
			pPB = (_ptrBloc*) s_pStrRegime->m_Osize.m_Ptrs[s_pStrRegime->m_nOver/HZ_STRING_PBLK_SZ] ;
		}

		pVoid = new char[nUnit * 8] ;
		pPB->m_Ptrs[s_pStrRegime->m_nOver % HZ_STRING_PBLK_SZ] = (uint64_t) pVoid ;
		strAddr = ++s_pStrRegime->m_nOver ;
	}

	memset(pVoid, 0, nUnit * 8) ;
	strAddr |= HZ_STRADDR_OSIZE ;

	if (_hzGlobal_MT)
		s_pStrRegime->m_lockOsize.Unlock() ;

	return strAddr ;
}

void	_strFree	(uint32_t strAddr, uint32_t nSize)
{
	//	Places object in freelist if it is of one of the precribed sizes, otherwise it frees it from the OS managed heap
	//
	//	Arguments:	1)	pMemobj	A pointer to what is assumed to be string space to be freed
	//				2)	nSize	The size of the string space
	//
	//	Returns:	None

	_hzfunc(__func__) ;

	_strItem*	pItem ;		//	Start of string space
	uint32_t	nUnit ;		//	Number of 8-byte units in string being freed
	uint32_t	nSlot ;		//	Number of 8-byte units in string being freed

	pItem = (_strItem*) _strXlate(strAddr) ;

	if (!pItem || !nSize)
		{ hzerr(_fn, HZ_ERROR, E_CORRUPT, "WARNING freeing obj %p of size %d bytes\n", pItem, nSize) ; return ; }
	//	printf("Freeing %u size %u %s\n", strAddr, size, pItem->m_data) ;

	if (nSize <= 256)
	{
		_strFLE*	pSlot ;

		nUnit = (nSize/8) + (nSize%8 ? 1:0) ;

		if (!_strTest(strAddr, nSize))
			hzexit(_fn, 0, E_CORRUPT, "Bad string address %u:%u (%s)", (strAddr&0x7fff0000)>>16, strAddr&0xffff, pItem->m_data) ;

		if (_hzGlobal_MT)
			s_pStrRegime->m_lockSmall[nUnit-1].LockWrite() ;

		pSlot = (_strFLE*) pItem ;
		pSlot->m_fleSelf = strAddr ;
		pSlot->m_fleNext = s_pStrRegime->m_flistSmall[nUnit-1] ;
		s_pStrRegime->m_flistSmall[nUnit-1] = strAddr ;

		_hzGlobal_Memstats.m_strSm_u[nUnit-1]-- ;
		_hzGlobal_Memstats.m_strSm_f[nUnit-1]++ ;

		if (_hzGlobal_MT)
			s_pStrRegime->m_lockSmall[nUnit-1].Unlock() ;
		return ;
	}

	//	Size > 256 bytes so obtain the pointer to the string space from the oversize array, delete it, set the pointer to the freelist and the freelist to the
	//	position in the array.

	_ptrBloc*	pPB ;		//	Applicable pointer block

	if (_hzGlobal_MT)
		s_pStrRegime->m_lockOsize.LockWrite() ;

	nSlot = strAddr & HZ_STRADDR_OMASK ;
	pPB = (_ptrBloc*) s_pStrRegime->m_Osize.m_Ptrs[(nSlot-1)/HZ_STRING_PBLK_SZ] ;
	pItem = (_strItem*) pPB->m_Ptrs[(nSlot-1) % HZ_STRING_PBLK_SZ] ;
	delete [] (char*) pItem ;

	pPB->m_Ptrs[(nSlot-1) % HZ_STRING_PBLK_SZ] = s_pStrRegime->m_flistOsize ;
	s_pStrRegime->m_flistOsize = nSlot ;

	_hzGlobal_Memstats.m_numStrOver-- ;
	_hzGlobal_Memstats.m_ramStrOver -= nSize ;

	if (_hzGlobal_MT)
		s_pStrRegime->m_lockOsize.Unlock() ;
}

/*
**	hzString Constructors/Destructors
*/

hzString::hzString	(void)
{
	_hzfunc("hzString::hzString") ;

	m_addr = 0 ;
	_hzGlobal_Memstats.m_numStrings++ ;
}

hzString::hzString	(const char* pStr)
{
	_hzfunc("hzString::hzString(char*)") ;

	m_addr = 0 ;
	_hzGlobal_Memstats.m_numStrings++ ;
	operator=(pStr) ;
}

hzString::hzString	(const hzString& op)
{
	_hzfunc("hzString::hzString(copy)") ;

	m_addr = 0 ;
	_hzGlobal_Memstats.m_numStrings++ ;
	operator=(op) ;
}

hzString::~hzString	(void)
{
	_hzfunc("hzString::~hzString") ;

	Clear() ;
	_hzGlobal_Memstats.m_numStrings-- ;
}

/*
**	hzString private methods
*/

int32_t hzString::_cmp	(const hzString& s) const
{
	//	Case sensitive compare based on strcmp
	//
	//	Arguments:	1)	s	The test string
	//
	//	Returns:	-1	If this string is less than the test string
	//				1	If ths string is greater than the test string
	//				0	If the two strings are equal

	_hzfunc("hzString::_cmp(hzString&)") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	suppCtl ;		//	This string's control area

	if (!m_addr)
		return s.m_addr ? -1 : 0 ;
	if (!s.m_addr)
		return m_addr ? 1 : 0 ;
	if (m_addr == s.m_addr)
		return 0 ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	suppCtl = (_strItem*) _strXlate(s.m_addr) ;

	if (!thisCtl || !suppCtl)
		hzexit(_fn, 0, E_MEMORY, "Illegal string address") ;

	return strcmp(thisCtl->m_data, suppCtl->m_data) ;
}

int32_t hzString::_cmpI	(const hzString& s) const
{
	//	Case insensitive compare based on strcasecmp
	//
	//	Arguments:	1)	s	The test string
	//
	//	Returns:	-1	If this string is less than the test string
	//				1	If ths string is greater than the test string
	//				0	If the two strings are equal

	_hzfunc("hzString::_cmpI(hzString&)") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	suppCtl ;		//	This string's control area

	if (!m_addr)
		return s.m_addr ? -1 : 0 ;
	if (!s.m_addr)
		return m_addr ? 1 : 0 ;
	if (m_addr == s.m_addr)
		return 0 ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	suppCtl = (_strItem*) _strXlate(s.m_addr) ;

	if (!thisCtl || !suppCtl)
		hzexit(_fn, 0, E_MEMORY, "Illegal string address") ;

	return strcasecmp(thisCtl->m_data, suppCtl->m_data) ;
}

int32_t hzString::_cmp	(const char* s) const
{
	//	Case sensitive compare based on strcmp
	//
	//	Arguments:	1)	s	The null terminated test string 
	//
	//	Returns:	-1	If this string is less than the test string
	//				1	If ths string is greater than the test string
	//				0	If the two strings are equal

	_hzfunc("hzString::_cmp(char*)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return (!s || !s[0]) ? 0 : -1 ;
	if (!s || !s[0])
		return m_addr ? 1 : 0 ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_MEMORY, "Illegal string address") ;

	return strcmp(thisCtl->m_data, s) ;
}

int32_t hzString::_cmpI	(const char* s) const
{
	//	Case insensitive compare based on strcasecmp
	//
	//	Arguments:	1)	s	The null terminated test string 
	//
	//	Returns:	-1	If this string is less than the test string
	//				1	If ths string is greater than the test string
	//				0	If the two strings are equal

	_hzfunc("hzString::_cmpI(char*)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return (!s || !s[0]) ? 0 : -1 ;
	if (!s || !s[0])
		return m_addr ? 1 : 0 ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_MEMORY, "Illegal string address") ;

	return strcasecmp(thisCtl->m_data, s) ;
}

int32_t	hzString::_cmpF	(const hzString& tS) const
{
	//	Fast compare. This provides a string comparison based on 32-bit chunks rather than bytes. It is faster but only the equivelent to lexical comparison in
	//	big-endian architetures. This method should be considered only for unordered collections where only uniqueness of entries is important.
	//
	//	Arguments:	1)	S	The test string 
	//
	//	Returns:	-1	If this string is less than the test string
	//				1	If ths string is greater than the test string
	//				0	If the two strings are equal

	_hzfunc("hzString::_cmpF") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	suppCtl ;		//	This string's control area

	if (!m_addr)	return tS.m_addr ? -1 : 0 ;
	if (!tS.m_addr)	return 1 ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	suppCtl = (_strItem*) _strXlate(tS.m_addr) ;

	if (!thisCtl || !suppCtl)
		hzexit(_fn, 0, E_MEMORY, "Illegal string address") ;

	if (thisCtl->_getSize() < suppCtl->_getSize())	return -1 ;
	if (thisCtl->_getSize() > suppCtl->_getSize())	return 1 ;

	int32_t*	pA ;	//	Pointer into data this
	int32_t*	pB ;	//	Pointer into data supplied
	uint32_t	n ;		//	Offset
	uint32_t	x ;		//	Counter (4 byte increments)

	pA = (int32_t*) thisCtl->m_data ;
	pB = (int32_t*) suppCtl->m_data ;

	for (x = n = 0 ; x < thisCtl->_getSize() ; x += 4, n++)
	{
		if (pA[n] < pB[n])	return -1 ;
		if (pA[n] > pB[n])	return 1 ;
	}
	return 0 ;
}

bool	hzString::_feq	(const hzString& tS) const
{
	//	Determine equality by fast compare technique. This works in both big and little endian architectures.
	//
	//	Arguments:	1)	tS	The test string 
	//
	//	Returns:	True	If this string is lexically equal to the supplied test string
	//				False	Otherwise

	_hzfunc("hzString::_feq") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	suppCtl ;		//	This string's control area

	if (m_addr == tS.m_addr)
		return true ;

	if (!m_addr)	return tS.m_addr ? false : true ;
	if (!tS.m_addr)	return false ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	suppCtl = (_strItem*) _strXlate(tS.m_addr) ;

	if (!thisCtl || !suppCtl)
		hzexit(_fn, 0, E_MEMORY, "Illegal string address") ;

	if (thisCtl->_getSize() != suppCtl->_getSize())
		return false ;

	int32_t*	pA ;	//	Pointer into data this
	int32_t*	pB ;	//	Pointer into data supplied
	uint32_t	n ;		//	Offset
	uint32_t	x ;		//	Counter (4 byte increments)

	pA = (int32_t*) thisCtl->m_data ;
	pB = (int32_t*) suppCtl->m_data ;

	for (x = n = 0 ; x < thisCtl->_getSize() ; x += 4, n++)
		if (pA[n] != pB[n])	return false ;
	return true ;
}

/*
**	hzString public methods
*/

uint32_t	hzString::Length	(void) const
{
	//	Returns length of string, not including a null terminator
	//
	//	Arguments:	None
	//	Returns:	Data length

	_hzfunc("hzString::Length") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return 0 ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;

	return thisCtl->_getSize() ;
}

const char*	hzString::operator*	(void) const
{
	//	Returns the string data (a null terminated string)
	//
	//	Arguments:	None
	//	Returns:	Content as null terminated string

	_hzfunc("hzString::operator*") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!this)
		hzexit(_fn, E_CORRUPT, "No instance") ;

	if (!m_addr)
		return 0 ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;

	return thisCtl->m_data ;
}

hzString::operator const char*	(void) const
{
	//	Returns the string data (a null terminated string)
	//
	//	Arguments:	None
	//	Returns:	Content as null terminated string

	_hzfunc("hzString::operator const char*") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!this)
		hzexit(_fn, E_CORRUPT, "No instance") ;

	if (!m_addr)
		return 0 ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;

	return thisCtl->m_data ;
}

void	hzString::Clear	(void)
{
	//	Clear this string.
	//
	//	The end result will be this string has a null pointer and the string space that was being pointed to, has its copy count reduced by 1. If this means the
	//	copy count falls to zero, then the string space shall be freed.
	//
	//	Clears the string. Note that if other string instances share the same internal string space (have equal contents) then all that occurs is a decrement of
	//	the copy count. This leaves the string value intact maintaining the integrity of the other string instances. Only if there are no other string instances
	//	sharing the internal string space (copy count is zero) is the internal string space is released (deleted). In both cases the local internal string space
	//	pointer is then set to null. Subsequent setting of this hzString instance will then allocate fresh memory.
	//
	//	Arguments:	None
	//	Returns:	None
 
	_hzfunc("hzString::Clear()") ;

	_strItem*	thisCtl ;		//	This string's control area
	uint32_t	nLen ;			//	Length of string

	if (m_addr)
	{
		thisCtl = (_strItem*) _strXlate(m_addr) ;
		if (!thisCtl)
			hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

		nLen = thisCtl->_getSize() ;
		if (!nLen)
			hzexit(_fn, 0, E_CORRUPT, "Zero string size %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

		if (thisCtl->m_copy < 100)
		{
			if (_hzGlobal_MT)
			{
				if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
					_strFree(m_addr, nLen + HZ_STRING_FACTOR) ;
			}
			else
			{
				thisCtl->m_copy-- ;
				if (!thisCtl->m_copy)
					_strFree(m_addr, nLen + HZ_STRING_FACTOR) ;
			}
		}

		m_addr = 0 ;
	}
}

//	Index operator
const char	hzString::operator[]	(uint32_t nIndex) const
{
	//	Returns the (char) value of the Nth position in the buffer.
	//
	//	A zero is returned if N is less than zero or overshoots the text part of the buffer
	//
	//	Arguments:	1)	nIndex	Position of char to be returned
	//
	//	Returns:	0+	Value of the Nth byte in the string if N is less than the length of the string

	_hzfunc("hzString::operator[]") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return _hz_NullBuffer[0] ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	if (nIndex > thisCtl->_getSize())
		return 0 ;

	return thisCtl->m_data[nIndex] ;
}

//	Stream operator
std::istream&	operator>>	(std::istream& is, hzString& obj)
{
	//	Category:	Data Input
	//
	//	This facilitates appendage of the string value with data from a stream
	//
	//	Arguments:	1)	is		Input stream
	//				2)	obj		String to be populated by the read operation
	//
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::operator>>") ;

	hzChain		Z ;		//	Working chain
	hzString	S ;		//	String from working chain

	is >> Z ;
	S = Z ;
	obj += S ;

	return is ;
}

std::ostream&	operator<<	(std::ostream& os, const hzString& obj)
{
	//	Category:	Data Output
	//
	//	This facilitates output of the string value to a stream
	//
	//	Arguments:	1)	is		Output stream
	//				2)	obj		String to be written out
	//
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::operator<<") ;

	if (*obj)
		os << *obj ;
	return os ;
}

hzString&	hzString::ToLower	(void)
{
	//	Convert string to all lower case.
	//
	//	As this function can alter string content, it follows the protocol described in charper 1.2 of the synopsis.
	//
	//	Arguments:	None
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::ToLower") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	destCtl ;		//	New internal structure if required
	char*		i ;				//	Iterator
	char*		j ;				//	Iterator
	uint32_t	count ;			//	Char counter
	uint32_t	destAddr ;		//	New string address if required
	bool		bChange ;		//	Flag to indicate if operation chages content

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Is change needed?
	bChange = false ;
	for (i = (char*) thisCtl->m_data, count = thisCtl->_getSize() ; count ; count--, i++)
	{
		if (*i >= 'A' && *i <= 'Z')
			{ bChange = true ; break ; }
	}

	if (!bChange)
		return *this ;

	//	Allocate new space
	count = thisCtl->_getSize() ;
	destAddr = _strAlloc(count + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal tgt string address %u:%u", (destAddr&0x7fff0000)>>16, destAddr&0xffff) ;
	destCtl->_setSize(count) ;
	destCtl->m_copy = 1 ;

	i = (char*) thisCtl->m_data ;
	j = (char*) destCtl->m_data ;

	//	The new string space is populated
	for (; count ; count--)
		*j++ = tolower(*i++) ;

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::ToUpper	(void)
{
	//	Convert string to all upper case.
	//
	//	As this function can alter string content, it follows the protocol described in charper 1.2 of the synopsis.
	//
	//	Arguments:	None
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::ToUpper") ;

	_strItem*		thisCtl ;		//	This string's control area
	_strItem*		destCtl ;		//	New internal structure if required
	char*		i ;				//	Iterator
	char*		j ;				//	Iterator
	uint32_t	nLen ;			//	Length
	uint32_t	destAddr ;		//	New string address if required
	bool		bChange ;		//	Flag to indicate if operation chages content

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Is change needed?
	bChange = false ;
	for (i = (char*) thisCtl->m_data, nLen = thisCtl->_getSize() ; nLen ; i++, nLen--)
	{
		if (*i >= 'a' && *i <= 'z')
			{ bChange = true ; break ; }
	}

	if (!bChange)
		return *this ;

	//	Allocate new space
	nLen = thisCtl->_getSize() ;
	destAddr = _strAlloc(nLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;
	destCtl->_setSize(nLen) ;
	destCtl->m_copy = 1 ;

	i = (char*) thisCtl->m_data ;
	j = (char*) destCtl->m_data ;

	for (; nLen ; nLen--)
		*j++ = toupper(*i++) ;

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::UrlEncode	(bool bResv)
{
	//	Performs URL-Encoding on the current string content.
	//
	//	This is transformation is only carried out if URL characters exist in the string value
	//
	//	Arguments:	1)	bResv	With bResv false (default), only the standard URL characters are encoded. But with bResv true, an extended set of URL characters
	//							are converted. Note that these include the forward slash character.
	//
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::UrlEncode") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	destCtl ;		//	New internal structure if required
	char*		i ;				//	For iteration
	char*		j ;				//	For iteration
	uint32_t	nLen ;			//	Lenght of original string
	uint32_t	destAddr ;		//	New string address if required
	char		buf [4] ;		//	For Hex conversion

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Is change needed? Count chars that are to be converted as these will occupy 3 chars in the new string
	nLen = thisCtl->_getSize() ;
	//nLen = Length() ;
	for (i = (char*) thisCtl->m_data ; *i ; i++)
	{
		//	If the char is a % (string may already be encoded), pass this by
		if (*i == CHAR_PERCENT)
			continue ;

		//	If the char is a normal URL char, pass
		if (IsUrlnorm(*i))
			continue ;

		//	If the char is a reserved URL char, pass olny if bResv is false
		if (IsUrlresv(*i))
		{
			if (bResv)
				nLen += 2 ;
			continue ;
		}

		//	Only increas the expected length if there is any char that must be encoded
		nLen += 2 ;
	}

	if (nLen == thisCtl->_getSize())
		return *this ;

	//	Allocate new space
	destAddr = _strAlloc(nLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;
	destCtl->_setSize(nLen) ;
	destCtl->m_copy = 1 ;

	j = (char*) destCtl->m_data ;

	for (i = (char*) thisCtl->m_data ; *i ; i++)
	{
		if (*i == CHAR_PERCENT)
			*j++ = *i ;
		else if (IsUrlnorm(*i))
			*j++ = *i ;
		else if (IsUrlresv(*i) && !bResv)
			*j++ = *i ;
		else
		{
			*j++ = CHAR_PERCENT ;
			sprintf(buf, "%02x", (uint32_t) *i) ;
			*j++ = buf[0] ;
			*j++ = buf[1] ;
		}
	}

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::UrlDecode	(void)
{
	//	Performs URL-Decoding on the current string content. If the current string does not contain URL-encoded sequences, the string will be unchanged.
	//
	//	Arguments:	None
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::UrlDecode") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	destCtl ;		//	New string's control area
	char*		i ;				//	Pointer into old string data
	char*		j ;				//	Pointer into new string data
	uint32_t	destAddr ;		//	New string space address
	uint32_t	newLen = 0 ;	//	Length of new string
	uint32_t	val ;			//	Hex value

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Is change needed? Not unless there are incidences of a percent followed by two hex chars
	for (i = thisCtl->m_data ; *i ; i++)
	{
		if (*i == CHAR_PERCENT && IsHex(i[1]) && IsHex(i[2]))
			i += 2 ;
		newLen++ ;
	}

	if (newLen == thisCtl->_getSize())
		return *this ;

	//	Allocate new space
	destAddr = _strAlloc(newLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;
	destCtl->_setSize(newLen) ;
	destCtl->m_copy = 1 ;

	//	Create new string
	for (j = destCtl->m_data, i = thisCtl->m_data ; *i ; i++)
	{
		if (*i == CHAR_PERCENT)
		{
			if (IsHex(i[1]) && IsHex(i[2]))
			{
				i++ ;
				val = (*i >='0' && *i <='9' ? *i-'0' : *i >= 'A' && *i<= 'F' ? *i+10-'A' : *i >= 'a' && *i<='f' ? *i+10-'a' : 0) ;
				val *= 16 ;
				i++ ;
				val += (*i >='0' && *i <='9' ? *i-'0' : *i >= 'A' && *i<= 'F' ? *i+10-'A' : *i >= 'a' && *i<='f' ? *i+10-'a' : 0) ;
				*j++ = (char) val ;
				continue ;
			}
		}

		*j++ = *i ;
	}

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

#if 0
hzString&	hzString::FnameEncode	(void)
{
	//	Converts non-URL and non-filename chars into %xx form.
	//
	//	Note that no assumptions can be made about the input except that it may contain chars unsuitable for filenames e.g. the forward slash and in the opinion
	//	of HadronZoo, the space. The encoding must therefore be reversible.
	//
	//	This function assumes the chars a-z, A-Z, 0-9, the period and the underscore are the only valid filename chars. Any other char
	//	will be converted to a set of chars consisting of a percent sign and two hexidecimal numbers. This means that when it comes to
	//	decoding, such a set will be converted to a single char. This would be fine if we could assume that no input would ever have
	//	such a sequence but alas we cannot assume this.
	//
	//	It is nessesary therefore to convert percent chars in the input to a %hh set even if they are blatently part of such a set
	//	already!
	//
	//	Arguments:	None
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::FnameEncode") ;

	_strItem*		thisCtl ;		//	This string's control area
	_strItem*		destCtl ;		//	New string's control area
	char*		i ;				//	Pointer into old string data
	char*		j ;				//	Pointer into new string data
	uint32_t	destAddr ;		//	New string space address
	uint32_t	newLen = 0 ;	//	Length of new string
	char		buf	[4] ;		//	Fox hex-conversion

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Is change needed? Not unless there are incidences of a percent followed by two hex chars
	for (i = thisCtl->m_data ; *i ; i++)
	{
		if (*i >= 'a' && *i <= 'z')	{ newLen++ ; continue ; }
		if (*i >= 'A' && *i <= 'Z')	{ newLen++ ; continue ; }
		if (*i >= '0' && *i <= '9')	{ newLen++ ; continue ; }
		if (*i == CHAR_USCORE)		{ newLen++ ; continue ; }
		if (*i == CHAR_PERIOD)		{ newLen++ ; continue ; }
		if (*i == CHAR_EQUAL)		{ newLen++ ; continue ; }

		newLen += 3 ;
	}

	if (newLen == thisCtl->_getSize())
		return *this ;

	//	Allocate new space
	destAddr = _strAlloc(newLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;
	destCtl->_setSize(newLen) ;
	destCtl->m_copy = 1 ;

	//	Create new string with all non-filename chars converted to %xx
	j = destCtl->m_data ;
	for (i = thisCtl->m_data ; *i ; i++)
	{
		if (*i >= 'a' && *i <= 'z')	{ *j++ = *i ; continue ; }
		if (*i >= 'A' && *i <= 'Z')	{ *j++ = *i ; continue ; }
		if (*i >= '0' && *i <= '9')	{ *j++ = *i ; continue ; }
		if (*i == CHAR_USCORE)		{ *j++ = *i ; continue ; }
		if (*i == CHAR_PERIOD)		{ *j++ = *i ; continue ; }
		if (*i == CHAR_EQUAL)		{ *j++ = *i ; continue ; }

		*j++ = CHAR_PERCENT ;
		sprintf(buf, "%02x", (uint32_t) *i) ;
		*j++ = buf[0] ;
		*j++ = buf[1] ;
	}
	*j = 0 ;

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::FnameDecode	(void)
{
	//	Converts all spaces (and other non url chars) into underscores
	//	Decodes a string previously encoded by FnameEncode. Firstly the string is tested to ensure it does not contain chars ...
	//
	//	Arguments:	None
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::FnameDecode") ;

	_strItem*		thisCtl ;		//	This string's control area
	_strItem*		destCtl ;		//	New string's control area
	char*		i ;				//	Pointer into old string data
	char*		j ;				//	Pointer into new string data
	uint32_t	destAddr ;		//	New string space address
	uint32_t	newLen = 0 ;	//	Length of new string
	uint32_t	val ;			//	Hex value

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Is change needed? Not unless there are incidences of a percent followed by two hex chars
	for (i = thisCtl->m_data ; *i ; i++)
	{
		if (*i == CHAR_PERCENT && IsHex(i[1]) && IsHex(i[2]))
			i += 2 ;
		newLen++ ;
	}

	if (newLen == thisCtl->_getSize())
		return *this ;

	//	Allocate new space
	destAddr = _strAlloc(newLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;
	destCtl->_setSize(newLen) ;
	destCtl->m_copy = 1 ;

	//	Create new string with all %xx sequences converted back to chars
	i = thisCtl->m_data ;
	j = destCtl->m_data ;

	for (; *i ;)
	{
		if (*i == CHAR_PERCENT && IsHex(i[1]) && IsHex(i[2]))
		{
			i++ ;
			val = (*i >='0' && *i <='9' ? *i-'0' : *i >= 'A' && *i<= 'F' ? *i+10-'A' : *i >= 'a' && *i<='f' ? *i+10-'a' : 0) ;
			val *= 16 ;

			i++ ;
			val += (*i >='0' && *i <='9' ? *i-'0' : *i >= 'A' && *i<= 'F' ? *i+10-'A' : *i >= 'a' && *i<='f' ? *i+10-'a' : 0) ;

			*j++ = val ;
			i++ ;
			continue ;
		}

		*j++ = *i++ ;
	}
	*j = 0 ;

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}
#endif

hzString&	hzString::Reverse	(void)
{
	//	Reverse the string content. This first ensures the string is independent.
	//
	//	Arguments:	None
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::Reverse") ;

	_strItem*		thisCtl ;	//	This string's control area
	_strItem*		destCtl ;	//	New internal structure if required
	uint32_t	destAddr ;	//	New string address if required
	uint32_t	nLen ;		//	Length
	uint32_t	c_up ;		//	Ascending iterator
	uint32_t	c_down ;	//	Decending iterator

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Allocate new space
	nLen = thisCtl->_getSize() ;

	destAddr = _strAlloc(nLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;
	destCtl->_setSize(nLen) ;
	destCtl->m_copy = 1 ;

	//	Create new string value as the reverse of the original
	c_down = nLen - 1 ;
	for (c_up = 0 ; c_up < c_down ; c_up++, c_down--)
	{
		destCtl->m_data[c_up] = thisCtl->m_data[c_down] ;
	}

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::Truncate	(uint32_t limit)
{
	//	Truncate the string
	//
	//	Arguments:	1)	limit	Sets max length for the string and truncates beyond this.
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::Truncate") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	destCtl ;		//	New control area
	uint32_t	destAddr = 0 ;	//	New string address if required

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Do nothing if the size limit exceeds length of string value
	if (limit >= thisCtl->_getSize())
		return *this ;

	//	Full truncation, delete
	if (limit)
	{
 		//	Perform the (partial) truncation
		destAddr = _strAlloc(limit + HZ_STRING_FACTOR) ;
		if (!destAddr)
			hzexit(_fn, 0, E_MEMORY, "Could not assign string space of (%d) bytes", limit + HZ_STRING_FACTOR) ;
		destCtl = (_strItem*) _strXlate(destAddr) ;
		if (!destCtl)
			hzexit(_fn, 0, E_CORRUPT, "Allocated an illegal string address %u:%u of %u bytes", (destAddr&0x7fff0000)>>16, destAddr&0xffff, limit + HZ_STRING_FACTOR) ;

		memcpy(destCtl->m_data, thisCtl->m_data, limit) ;
		destCtl->m_data[limit] = 0 ;
		destCtl->_setSize(limit) ;
		destCtl->m_copy = 1 ;
	}

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::TruncateUpto	(const char* patern)
{
	//	Category:	Text Processing
	//
	//	Conditional, content based truncation. If the supplied patern exists in the string, the new string will be everything upto but not including the first
	//	instance of the patern. If the pattern is empty or does not exist in the string, the string content will be unchanged.
	//
	//	Arguments:	1)	pattern	The pattern at which the string will be truncated.
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::TruncateUpto") ;

	_strItem*		thisCtl ;	//	This string's control area
	const char*		i ;			//	Pointer into string data
	const char*		j ;			//	Pointer to patern instance
	uint32_t		nLen ;		//	Number of bytes from start of existing string to patern instance

	//	If no pattern supplied, do nothing
	if (!patern || !patern[0])
		return *this ;

	//	If NULL return
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	//	Test if change required
	i = (char*) thisCtl->m_data ;
	j = strstr(i, patern) ;

	if (!j)
		return *this ;

	for (nLen = 0 ; i != j ; i++, nLen++) ;

	return Truncate(nLen) ;
}

hzString&	hzString::TruncateBeyond	(const char* patern)
{
	//	Category:	Text Processing
	//
	//	Conditional, content based truncation. If the supplied patern exists in the string, the new string will be everything beyond but not including the first
	//	instance of the patern. If the pattern is empty or does not exist in the string, the string content will be unchanged.
	//
	//	Arguments:	1)	pattern	The pattern at which the string will be truncated.
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::TruncateBeyond") ;

	_strItem*		thisCtl ;		//	This string's control area
	_strItem*		destCtl ;		//	New control area
	char*			i ;				//	Pointer into old string data
	char*			j ;				//	Pointer into new string data
	uint32_t		destAddr ;		//	New string address if required
	uint32_t		newLen ;		//	New string length

	//	If no pattern supplied, do nothing
	if (!patern || !patern[0])
		return *this ;

	//	If NULL return
	if (!m_addr)
		return *this ;

	//	Test if change required
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;
	i = strstr((char*) thisCtl->m_data, patern) ;
	if (!i)
		return *this ;

	//	Allocate new space
	newLen = i - thisCtl->m_data ;

	destAddr = _strAlloc(newLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;
	destCtl->_setSize(newLen) ;
	destCtl->m_copy = 1 ;

	i += strlen(patern) ;
	for (j = destCtl->m_data ; *i ; *j++ = *i++) ;
	*j = 0 ;

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::DelWhiteLead	(void)
{
	//	Removes leading whitespace from the string
	//
	//	Arguments:	None
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::DelWhiteLead") ;

	_strItem*		thisCtl ;	//	This string's control area
	_strItem*		destCtl ;	//	New string space
	const char*		i ;			//	Pointer into string data
	uint32_t		destAddr ;	//	New string address if required
	uint32_t		wc ;		//	Count of whitespace chars
	uint32_t		count ;		//	Iterator
	uint32_t		nLen ;		//	Length of original string
	uint32_t		nusize ;	//	The size the string will be once leading whitespace removed

	//	If NULL return
	if (!m_addr)
		return *this ;

	//	Check that change is needed
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	nLen = Length() ;
	i = (char*) thisCtl->m_data ;

	for (wc = count = 0 ; count < nLen ; count++)
	{
		if (i[count] <= CHAR_SPACE)
			wc++ ;
		else
			break ;
	}

	if (!wc)
		return *this ;

	//	Must alter content

	nusize = nLen - wc ;
	if (nusize <= 0)
	{
		Clear() ;
		return *this ;
	}

	destAddr = _strAlloc(nusize + HZ_STRING_FACTOR) ;
	if (!destAddr)
		hzexit(_fn, 0, E_MEMORY, "Buffer of (%d) bytes", nusize + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;

	memcpy(destCtl->m_data, thisCtl->m_data + wc, nusize) ;
	destCtl->m_data[nusize] = 0 ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(nusize) ;

	//	Tidy up
	if (thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::DelWhiteTrail	(void)
{
	//	Removes tariling whitespace
	//
	//	Arguments:	None
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::DelWhiteTrail") ;

	_strItem*		thisCtl ;	//	This string's control area
	_strItem*		destCtl ;	//	New string space
	const char*		i ;			//	Pointer into string data
	uint32_t		destAddr ;	//	New string address if required
	uint32_t		wc ;		//	Count of whitespace chars
	uint32_t		nLen ;		//	Length of original string
	uint32_t		nusize ;	//	The size the string will be once leading whitespace removed
	int32_t			count ;		//	Iterator

	if (!m_addr)
		return *this ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal src string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	i = (char*) thisCtl->m_data ;
	nLen = Length() ;
	wc = 0 ;

	for (count = nLen - 1 ; count >= 0 ; count--)
	{
		if (i[count] <= CHAR_SPACE)
			wc++ ;
		else
			break ;
	}

	if (!wc)
		return *this ;

	//	Must alter content

	nusize = nLen - wc ;
	if (nusize <= 0)
	{
		Clear() ;
		return *this ;
	}

	destAddr = _strAlloc(nusize + HZ_STRING_FACTOR) ;
	if (!destAddr)
		hzexit(_fn, 0, E_MEMORY, "Buffer of (%d) bytes", nusize + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;

	memcpy(destCtl->m_data, thisCtl->m_data, nusize) ;
	destCtl->m_data[nusize] = 0 ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(nusize) ;

	Clear() ;
	m_addr = destAddr ;

	return *this ;
}

hzString&	hzString::TopAndTail	(void)
{
	//	Text substitution
	//
	//	Removes leading and trailing whitespce from this string. If no whitespace exists within this string, it is unchanged
	//
	//	Arguments:	None
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::TopAndTail") ;

	DelWhiteLead() ;
	DelWhiteTrail() ;
	return *this ;
}

hzString&	hzString::Replace	(const char* strA, const char* strB)
{
	//	Text substitution
	//
	//	Replace within this string, all instances of strA with strB. This string is unchanged if strA does not exist within it.
	//
	//	Arguments:	1)	strA	Patern to be substituted out
	//				2)	strB	Patern to be used instead
	//
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::Replace") ;

	hzChain		Z ;					//	Working chain buffer
	_strItem*		thisCtl ;			//	This string's control area
	const char*	i ;					//	Pointer into string data
	uint32_t	nLen ;				//	Lenth of supplied strings
	bool		bFound = false ;	//	Indicates string to be replace is found

	if (!strA)
		return *this ;
	if (!m_addr)
		return *this ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	nLen = strlen(strA) ;
	i = (char*) thisCtl->m_data ;

	if (strstr(i, strA))
	{
		for (; *i ;)
		{
			if (*i == strA[0])
			{
				if (!memcmp(i, strA, nLen))
				{
					bFound = true ;
					if (strB && strB[0])
						Z << strB ;
					i += nLen ;
					continue ;
				}
			}

			Z.AddByte(*i) ;
			i++ ;
		}
	}

	if (bFound)
		{ Clear() ; operator=(Z) ; }
	return *this ;
}

hzEcode	hzString::SetValue	(const char* cpStr, uint32_t nLen)
{
	//	Set a string to a non-terminated char string
	//
	//	Arguments:	1)	cpStr	The char* pointer
	//				2)	nLen	The length
	//
	//	Returns:	E_OK	If operation successful
	//				E_RANGE	If length is -ve or too uint32_t

	_hzfunc("hzString::SetValue(a)") ;

	_strItem*	destCtl ;	//	New string space

	Clear() ;
	if (!cpStr || !cpStr[0])
		return E_OK ;

	if (nLen <= 0 || nLen > HZSTRING_MAXLEN)
	{
		operator=(_hzString_TooLong) ;
		return E_RANGE ;
	}

	m_addr = _strAlloc(nLen + HZ_STRING_FACTOR) ;
	if (!m_addr)
		hzexit(_fn, 0, E_MEMORY, "Cannot allocate string of %d bytes", nLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(m_addr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	memcpy(destCtl->m_data, cpStr, nLen) ;
	destCtl->m_data[nLen] = 0 ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(nLen) ;

	return E_OK ;
}

hzEcode	hzString::SetValue	(const char* cpStr, const char* cpTerm)
{
	//	Set a string to a char string that is terminated by a char sequence rather than a null
	//
	//	Arguments:	1)	cpStr	The non-null terminated string value
	//				2)	cpTerm	The teminator sequence
	//
	//	Returns:	E_OK	If operation successful
	//				E_RANGE	If length is -ve or too long

	_hzfunc("hzString::SetValue(b)") ;

	_strItem*	destCtl ;	//	New string space
	const char*	i ;			//	Source string iterator
	uint32_t	nLen ;		//	Length to be allocated

	Clear() ;
	if (!cpStr || !cpStr[0])
		return E_OK ;

	for (nLen = 0, i = cpStr ; *i && i != cpTerm ; i++, nLen++) ;
	if (nLen > HZSTRING_MAXLEN)
	{
		operator=(_hzString_TooLong) ;
		return E_RANGE ;
	}

	m_addr = _strAlloc(nLen + HZ_STRING_FACTOR) ;
	if (!m_addr)
		hzexit(_fn, 0, E_MEMORY, "Cannot allocate string of %d bytes", nLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(m_addr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	memcpy(destCtl->m_data, cpStr, nLen) ;
	destCtl->m_data[nLen] = 0 ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(nLen) ;

	return E_OK ;
}

hzEcode	hzString::SetValue	(const char* cpStr, char termchar)
{
	//	Set a string to a char string that is terminated by a char other than a null
	//
	//	Arguments:	1)	cpStr		The non-null terminated string value
	//				2)	termchar	The teminator char
	//
	//	Returns:	E_RANGE	If the supplied terminated string exceeds HZSTRING_MAXLEN characters
	//				E_OK	If the string is set

	_hzfunc("hzString::SetValue(c)") ;

	_strItem*	destCtl ;	//	New string space
	const char*	i ;			//	Source string iterator
	uint32_t	nLen ;		//	Length to be allocated

	Clear() ;
	if (!cpStr || !cpStr[0])
		return E_OK ;

	for (nLen = 0, i = cpStr ; *i && *i != termchar ; i++, nLen++) ;
	if (nLen > HZSTRING_MAXLEN)
	{
		operator=(_hzString_TooLong) ;
		return E_RANGE ;
	}

	m_addr = _strAlloc(nLen + HZ_STRING_FACTOR) ;
	if (!m_addr)
		hzexit(_fn, 0, E_MEMORY, "Cannot allocate string of %d bytes", nLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(m_addr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	memcpy(destCtl->m_data, cpStr, nLen) ;
	destCtl->m_data[nLen] = 0 ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(nLen) ;

	return E_OK ;
}

hzString	hzString::SubString	(uint32_t nPosn, uint32_t nBytes) const
{
	//	Purpose:	Populate a string with a substring of this string. Return an empty string if the
	//				requested position goes beyong length of string, Return a partial string if the
	//				length requested goes beyond the end of the string
	//
	//	Arguments:	1)	nPosn	Starting offset within this string.
	//				2)	nBytes	Length from here. A value of 0 indicates remainder of this string.
	//
	//	Returns:	Instance of hzString by value being the substring result

	_hzfunc("hzString::SubString") ;

	_strItem*		thisCtl ;		//	This string's control area
	_strItem*		destCtl ;		//	Result string control area
	hzString	Dest ;			//	Target string
	uint32_t	nRemainder ;	//	Remainder of original past the stated position

	if (!m_addr)
		return Dest ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address") ;

	nRemainder = Length() - nPosn ;
	if (nRemainder <= 0)
		return Dest ;

	if (nBytes == 0)
		nBytes = nRemainder ;
	if (nBytes > nRemainder)
		nBytes = nRemainder ;

	Dest.m_addr = _strAlloc(nBytes + HZ_STRING_FACTOR) ;
	if (!Dest.m_addr)
		hzexit(_fn, 0, E_MEMORY, "Buffer of (%d) bytes", nBytes + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(Dest.m_addr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (Dest.m_addr&0x7fff0000)>>16, Dest.m_addr&0xffff) ;

	//	Go to position
	memcpy(destCtl->m_data, thisCtl->m_data + nPosn, nBytes) ;
	destCtl->m_data[nBytes] = 0 ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(nBytes) ;

	return Dest ;
}

//	Find first/last instance of a test char in this string (I denotes case insensitive)
int32_t	hzString::First		(const char c) const
{
	_hzfunc("hzString::First") ;

	_strItem*		thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrFirst(thisCtl->m_data, c) ;
}

int32_t	hzString::FirstI	(const char c) const
{
	_hzfunc("hzString::FirstI") ;

	_strItem*		thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrFirstI(thisCtl->m_data, c) ;
}

int32_t	hzString::Last		(const char c) const
{
	_hzfunc("hzString::Last") ;

	_strItem*		thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrLast(thisCtl->m_data, c) ;
}

int32_t	hzString::LastI		(const char c) const
{
	_hzfunc("hzString::LastI") ;

	_strItem*		thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrLastI(thisCtl->m_data, c) ;
}

//	Find first/last instance of a test cstr in this string (I denotes case insensitive)
int32_t	hzString::First		(const char* str) const
{
	_hzfunc("hzString::First(char*)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrFirst(thisCtl->m_data, str) ;
}

int32_t	hzString::FirstI	(const char* str) const
{
	_hzfunc("hzString::FirstI(char*)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrFirstI(thisCtl->m_data, str) ;
}

int32_t	hzString::Last		(const char* str) const
{
	_hzfunc("hzString::Last(char*)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrLast(thisCtl->m_data, str) ;
}

int32_t	hzString::LastI		(const char* str) const
{
	_hzfunc("hzString::LastI(char*)") ;
	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrLastI(thisCtl->m_data, str) ;
}

//	Find first/last instance of a test string in this string (I denotes case insensitive)
int32_t	hzString::First		(const hzString& S) const
{
	_hzfunc("hzString::First(hzStr)") ;

	_strItem*	thisCtl ;		//	This string's control area
	_strItem*	suppCtl ;		//	This string's control area
	const char*	test = 0 ;		//	Supplied string value

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	if (S.m_addr)
	{
		suppCtl = (_strItem*) _strXlate(S.m_addr) ;
		if (!suppCtl)
			hzexit(_fn, 0, E_CORRUPT, "Illegal test string address %u:%u", (S.m_addr&0x7fff0000)>>16, S.m_addr&0xffff) ;
		test = suppCtl->m_data ;
	}

	return CstrFirst(thisCtl->m_data, test) ;
}

int32_t	hzString::FirstI	(const hzString& S) const
{
	_hzfunc("hzString::FirstiI(hzStr)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrFirstI(thisCtl->m_data, *S) ;
}

int32_t	hzString::Last		(const hzString& S) const
{
	_hzfunc("hzString::Last(hzStr)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrLast(thisCtl->m_data, *S) ;
}

int32_t	hzString::LastI		(const hzString& S) const
{
	_hzfunc("hzString::LastI(hzStr)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return -1 ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrLastI(thisCtl->m_data, *S) ;
}

bool	hzString::Contains	(const char c) const
{
	//	Test if the string contains one or more instances of a test char
	//
	//	Arguments:	1)	c	The char to test for
	//
	//	Returns:	True	If the test char exists within the string
	//				False	otherwise

	_hzfunc("hzString::Contains(char)") ;

	if (!m_addr)
		return false ;

	_strItem*	thisCtl ;	//	This string's control area
	const char*	i ;			//	Pointer into string data
	uint32_t	len ;		//	Lenth of this string

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;
	i = (char*) thisCtl->m_data ;
	len = Length() ;

	for (; len ; len--)
	{
		if (i[len] == c)
			return true ;
	}
	return false ;
}

bool	hzString::Contains	(const char* cpNeedle) const
{
	//	Test if the string contains a char string (case sensitive)
	//
	//	Arguments:	1)	cpStr	The char sequence to test for
	//
	//	Returns:	True	If this string contains the supplied test sequence
	//				False	Otherwise

	_hzfunc("hzString::Contains(char*)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!cpNeedle || !cpNeedle[0])
		return true ;
	if (!m_addr)
		return false ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return strstr((char*) thisCtl->m_data, cpNeedle) ? true : false ;
}

bool	hzString::ContainsI	(const char* cpNeedle) const
{
	//	Test if the string contains a char string (case insensitive)
	//
	//	Arguments:	1)	cpStr	The char sequence to test for
	//
	//	Returns:	True	If this string is lexically eqivelent to the supplied cstr
	//				False	Otherwise

	_hzfunc("hzString::ContainsI(char*)") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!cpNeedle || !cpNeedle[0])
		return true ;

	if (!m_addr)
		return false ;

	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrContainsI((char*) thisCtl->m_data, cpNeedle) ? true : false ;
}

bool	hzString::Equiv		(const char* cpStr) const
{
	//	Test if this string is equal to the operand char string (on a case insensitive basis)
	//
	//	Arguments:	1)	cpStr	The char sequence to test for
	//
	//	Returns:	True	If this string is lexically eqivelent to the supplied cstr
	//				False	Otherwise

	_hzfunc("hzString::Equiv") ;

	_strItem*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return false ;
	thisCtl = (_strItem*) _strXlate(m_addr) ;
	if (!thisCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	return CstrCompareI(cpStr, (char*) thisCtl->m_data) == 0 ? true : false ;
}

/*
**	Assignment operators
*/

hzString&	hzString::operator=	(const hzString& op)
{
	//	Purpose:	Make this string instance have the same content as the operand
	//				instance. Note this cannot take a const hztring as the operand
	//				because the copy count, an integral part of the hzString data,
	//				will be altered even though the string itself will not be.
	//
	//	Arguments:	1)	op	A reference to the operand hzString instance.
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::operator=(hzString&)") ;

	_strItem*	suppCtl ;		//	Supplied string's control area

	if (!this)
		hzerr(_fn, HZ_ERROR, E_CORRUPT, "No instance") ;

	//	It the this string's internal pointer and that of the operand already point to the same internal structure in memory, do nothing
	if (m_addr == op.m_addr)
		return *this ;

	//	If this call is to make a string equal to itself, do nothing.
	if (*this == op)
		return *this ;

	//	Clear this string and if the supplied string is blank, return
	Clear() ;
	if (!op.m_addr)
		return *this ;

	//	If the operand has content, increment the copy count and make this string address equal to that of the supplied.
	suppCtl = (_strItem*) _strXlate(op.m_addr) ;
	if (!suppCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal op string address %u:%u", (op.m_addr&0x7fff0000)>>16, op.m_addr&0xffff) ;

	if (suppCtl->m_copy < 100)
	{
		if (_hzGlobal_MT)
			__sync_add_and_fetch((uint32_t*)&(suppCtl->m_copy), 1) ;
		else
			suppCtl->m_copy++ ;
	}
	m_addr = op.m_addr ;

	return *this ;
}

hzString&	hzString::operator=	(const hzChain& C)
{
	//	Set string equal to content of the supplied chain. Note this function will fail (with the string empty) if the chain contents
	//	are too large
	//
	//	Arguments:	1)	C	The operand chain
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::op=(hzChain&)") ;

	chIter		ci ;		//	Chain iterator
	_strItem*	destCtl ;	//	New string space
	char*		i ;			//	New string space populator

	Clear() ;

	if (C.Size())
	{
		if (C.Size() >= HZSTRING_MAXLEN)
			operator=(_hzString_TooLong) ;
		else
		{
			//	Create new internal structure
			//threadLog("%s - requesting %d bytes - ", *_fn, C.Size()+HZ_STRING_FACTOR) ;
			m_addr = _strAlloc(C.Size() + HZ_STRING_FACTOR) ;
			//threadLog("addr %08x\n", m_addr) ;

			if (!m_addr)
				hzexit(_fn, 0, E_MEMORY, "Could not allocate internal buffer") ;
			destCtl = (_strItem*) _strXlate(m_addr) ;
			if (!destCtl)
				hzexit(_fn, 0, E_CORRUPT, "Illegal tgt string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

			i = destCtl->m_data ;
			for (ci = C ; !ci.eof() ; ci++)
				*i++ = *ci ;
			*i = 0 ;

			destCtl->_setSize(C.Size()) ;
			destCtl->m_copy = 1 ;
		}
	}

	return *this ;
}

hzString&	hzString::operator=	(const char* cpStr)
{
	//	Set the value of this hzString instance to the operand null terminated char string. Note that it is possible for the operand to
	//	have come from the string itself. For this reason we do not clear the existing string until we have allocated and populated the
	//	new buffer.
	//
	//	Arguments:	1)	cpStr	The operand null terminated char string.
	//	Returns:	Reference to this string instance

	_hzfunc("hzString::op=(const char*)") ;

	_strItem*	destCtl ;	//	New string space
	uint32_t	nLen ;		//	Required length of new string

	Clear() ;
	if (!cpStr || !cpStr[0])
		return *this ;

	nLen = strlen(cpStr) ;
	if (!nLen || nLen > HZSTRING_MAXLEN)
	{
		operator=(_hzString_TooLong) ;
		return *this ;
	}

	m_addr = _strAlloc(nLen + HZ_STRING_FACTOR) ;
	if (!m_addr)
		hzexit(_fn, 0, E_MEMORY, "Cannot allocate string of %d bytes for value [%s]", nLen + HZ_STRING_FACTOR, cpStr) ;
	destCtl = (_strItem*) _strXlate(m_addr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal tgt string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

	strcpy((char*) destCtl->m_data, cpStr) ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(nLen) ;

	return *this ;
}

//	FnGrp:		operator+
//	Category:	Text Processing
//
//	Add two strings forming a new string. Neither of the two input strings are effected in the process.
//
//	Variants:	1)	Add two strings
//				2)	Add a string and a cstr
//				3)	Add a cstr and a string
//
//	Note there is no char* plus char* operator.
//
//	Arguments:	1)	a	The first string (as char* or hzString)
//				2)	b	The second string (as char* or hzString)
//
//	Returns:	Instance of new hzString by value

hzString	operator+	(const hzString a, const hzString b)
{
	_hzfunc("friend hzString operator+(1)") ;

	hzString	r ;		//	Return string

	r = a ;
	r += b ;
	return r ;
}

hzString	operator+	(const hzString a, const char* cpStr)
{
	_hzfunc("friend hzString operator+(2)") ;

	hzString	r ;		//	Return string

	r = a ;
	r += cpStr ;
	return r ;
}

hzString	operator+	(const char* cpStr, const hzString S)
{
	_hzfunc("friend hzString operator+(3)") ;

	hzString	r ;		//	Return string

	r = cpStr ;
	r += S ;
	return r ;
}

/*
**	Appending operators
*/

hzString&	hzString::operator+=	(const hzString& op)
{
	//	Append the operand string to the contents of this.
	//
	//	Arguments:	1)	op	Operand string
	//	Returns:	Reference to this string in all cases

	_hzfunc("hzString::op+=(hzString&)") ;

	_strItem*	thisCtl = 0 ;	//	This string control area
	_strItem*	suppCtl ;		//	Supplied string control area
	_strItem*	destCtl ;		//	Result string space
	uint32_t	destAddr ;		//	New string address if required
	uint32_t	crlen ;			//	Len of this string
	uint32_t	nulen ;			//	Len of combined string

	//	If operand is empty do nothing
	if (!op.m_addr)
		return *this ;

	suppCtl = (_strItem*) _strXlate(op.m_addr) ;
	if (!suppCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal op string address %u:%u", (op.m_addr&0x7fff0000)>>16, op.m_addr&0xffff) ;

	crlen = 0 ;
	if (m_addr)
	{
		thisCtl = (_strItem*) _strXlate(m_addr) ;
		if (!thisCtl)
			hzexit(_fn, 0, E_CORRUPT, "Illegal this string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;
		crlen = thisCtl->_getSize() ;
	}

	//threadLog("%s case 1 [%u]\n", *_fn, crlen) ;

	//	Calculate required length
	nulen = crlen + suppCtl->_getSize() ;

	//	Allocate and populate a new buffer
	destAddr = _strAlloc(nulen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal tgt string address %u:%u", (destAddr&0x7fff0000)>>16, destAddr&0xffff) ;

	if (crlen)
		memcpy(destCtl->m_data, thisCtl->m_data, crlen) ;

	//threadLog("%s case 2\n", *_fn) ;

	memcpy(destCtl->m_data + crlen, suppCtl->m_data, suppCtl->_getSize() + 1) ;
	destCtl->m_data[nulen] = 0 ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(nulen) ;
	//threadLog("%s case 3\n", *_fn) ;

	//	Tidy up
	if (thisCtl && thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}
	//threadLog("%s case 4\n", *_fn) ;

	m_addr = destAddr ;
	return *this ;
}

hzString&	hzString::operator+=	(const char* cpStr)
{
	//	Append the operand char string to the contents of this.
	//
	//	Arguments:	1)	op	Operand string
	//	Returns:	Reference to this string

	_hzfunc("hzString::op+=(const char*)") ;

	_strItem*	thisCtl = 0 ;	//	This string's control area
	_strItem*	destCtl ;		//	Result string space
	uint32_t	destAddr ;		//	New string address if required
	uint32_t	strLen ;		//	Len of operand string
	uint32_t	oldLen = 0 ;	//	Len of this string

	//	If operand is empty do nothing
	if (!cpStr || !cpStr[0])
		return *this ;

	strLen = strlen(cpStr) ;

	if (m_addr)
	{
		thisCtl = (_strItem*) _strXlate(m_addr) ;
		if (!thisCtl)
			hzexit(_fn, 0, E_CORRUPT, "Illegal this string address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;
		oldLen = thisCtl->_getSize() ;
	}

	//	Allocate and populate a new buffer
	destAddr = _strAlloc(oldLen + strLen + HZ_STRING_FACTOR) ;
	destCtl = (_strItem*) _strXlate(destAddr) ;
	if (!destCtl)
		hzexit(_fn, 0, E_CORRUPT, "Illegal tgt string address %u:%u", (destAddr&0x7fff0000)>>16, destAddr&0xffff) ;
	if (oldLen)
		memcpy(destCtl->m_data, thisCtl->m_data, oldLen) ;

	memcpy(destCtl->m_data + oldLen, cpStr, strLen) ;
	destCtl->m_data[oldLen + strLen] = 0 ;
	destCtl->m_copy = 1 ;
	destCtl->_setSize(oldLen + strLen) ;

	//	Tidy up
	if (thisCtl && thisCtl->m_copy < 100)
	{
		if (!_hzGlobal_MT)
		{
			thisCtl->m_copy-- ;
			if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
		else
		{
			if (__sync_add_and_fetch(&(thisCtl->m_copy), -1) == 0)
				_strFree(m_addr, thisCtl->_getSize() + HZ_STRING_FACTOR) ;
		}
	}

	m_addr = destAddr ;
	return *this ;
}

int32_t	StringCompare	(const hzString& A, const hzString& B)
{
	//	Category:	Text Processing
	//
	//	Compare two hzString instances, case sensitive.
	//
	//	Arguments:	1)	A	First test string
	//				2)	B	Second test string
	//
	//	Returns:	<0	If A is lexically less than B
	//				>0	If A is lexically more than B
	//				0	If A abs B are equal.

	_hzfunc(__func__) ;

	const char* t = *A ;	//	Pointer to string A value
	const char* s = *B ;	//	Pointer to string B value

	if (!t)
		return (!s || !s[0]) ? 0 : -*s ;
	if (!s || !s[0])
		return *t ;

	for (; *t && *s && *t == *s ; t++, s++) ;
	return *t - *s ;
}

int32_t	StringCompareI	(const hzString& A, const hzString& B)
{
	//	Category:	Text Processing
	//
	//	Compare two hzString instances, ignore case.
	//
	//	Arguments:	1)	A	First test string
	//				2)	B	Second test string
	//
	//	Returns:	<0	If A is lexically less than B
	//				>0	If A is lexically more than B
	//				0	If A abs B are eqivelent.

	_hzfunc(__func__) ;

	const char* t = *A ;	//	Pointer to string A value
	const char* s = *B ;	//	Pointer to string B value

	if (!t)
		return (!s || !s[0]) ? 0 : -_tolower(*s) ;
	if (!s || !s[0])
		return _tolower(*t) ;

	for (; *t && *s && _tolower(*t) == _tolower(*s) ; t++, s++) ;
	return _tolower(*t) - _tolower(*s) ;
}

int32_t	StringCompareW	(const hzString& A, const hzString& B)
{
	//	Category:	Text Processing
	//
	//	Compares two hzString instances but ignores whitespace
	//
	//	Arguments:	1)	A	First test string
	//				2)	B	Second test string
	//
	//	Returns:	<0	If A is lexically less than B
	//				>0	If A is lexically more than B
	//				0	If A abs B are eqivelent.

	_hzfunc(__func__) ;

	return CstrCompareW(*A, *B) ;
}

int32_t	StringCompareF	(const hzString& a, const hzString& b)
{
	//	Category:	Text Processing
	//
	//	Fast String Compare.
	//
	//	Although normally one would expect a string compare function to correctly determine if one string was greater or less than another, this isn't
	//	nessesary in sets and maps if one is only concerned with lookup and not seeking to export the sets or maps in lexical order. String comparison
	//	can be speed up considerably by treating string values as arrays of 64 bit integers, rather than arrays of bytes.
	//
	//	Arguments:	1)	a	The 1st string
	//				2)	b	The 2nd string
	//
	//	Returns:	+1	If a > b
	//				-1	If a < b
	//				0	If a and b are equal.

	_hzfunc(__func__) ;

	return a.CompareF(b) ;
}
