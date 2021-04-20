//
//	File:	hzIdset.cpp
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

#include "hzChars.h"
#include "hzDatabase.h"

using namespace std ;

/*
**	Variables
*/

static  char	s_BitArray	[256] =
{
	//	Array to speed up bit counting per byte

	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,		//	  0 -  15
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,		//	 16 -  31
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,		//	 32 -  47
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,		//	 48 -  63
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,		//	 64 -  79
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,		//	 80 -  95
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,		//	 96 - 111
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,		//	112 - 127
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,		//	128 - 143
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,		//	144 - 159
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,		//	160 - 175
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,		//	176 - 191
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,		//	192 - 207
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,		//	208 - 223
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,		//	224 - 239
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8		//	240 - 255
} ;

/*
**	SECTION 1:	Bitmap non member functions
*/

uint32_t	CountBits	(const void* buf, uint32_t nBytes)
{
	//	Category:	Bitwise
	//
	//	Count the total number of bits set in the supplied buffer, taken to be of the supplied size.
	//
	//	Arguments:	1)	buf		The const void* buffer
	//				2)	nBytes	The number of bytes
	//
	//	Returns:	Number of bits found to be set

	const uchar*	uber ;		//	Buffer as a series of chars
	uint32_t		nCount ;	//	Buffer iterator
	uint32_t		nBits ;		//	Cumulative bit counter

	if (!buf)
		return -1 ;
	uber = (uchar*) buf ;

	for (nBits = nCount = 0 ; nCount < nBytes ; nCount++)
		nBits += s_BitArray[uber[nCount]] ;

	return nBits ;
}

void	SetBits	(char* pBitbuf, uint32_t nOset, bool bValue)
{
	//	Category:	Bitwise
	//
	//	Set or clear the bit in the buffer at the given offset
	//
	//	Arguments:	1)	pBitbuf	The buffer
	//				2)	nOset	The offset (is divided by 8)
	//				2)	bValue	The offset (is divided by 8)
	//
	//	Returns:	True	If the requested bit number is set within the segment
	//				False	If the requested bit number is not set within the segment or it exceeds the segment range (0 - 4095)

	_hzfunc("__func__") ;

	if (pBitbuf)
	{
		if (bValue)
		{
			switch	(nOset % 8)
			{
			case 0:	pBitbuf[nOset/8] |= 0x80 ; break ;
			case 1:	pBitbuf[nOset/8] |= 0x40 ; break ;
			case 2:	pBitbuf[nOset/8] |= 0x20 ; break ;
			case 3:	pBitbuf[nOset/8] |= 0x10 ; break ;
			case 4:	pBitbuf[nOset/8] |= 0x08 ; break ;
			case 5:	pBitbuf[nOset/8] |= 0x04 ; break ;
			case 6:	pBitbuf[nOset/8] |= 0x02 ; break ;
			case 7:	pBitbuf[nOset/8] |= 0x01 ; break ;
			}
		}
		else
		{
			switch	(nOset % 8)
			{
			case 0:	pBitbuf[nOset/8] &= ~0x80 ; break ;
			case 1:	pBitbuf[nOset/8] &= ~0x40 ; break ;
			case 2:	pBitbuf[nOset/8] &= ~0x20 ; break ;
			case 3:	pBitbuf[nOset/8] &= ~0x10 ; break ;
			case 4:	pBitbuf[nOset/8] &= ~0x08 ; break ;
			case 5:	pBitbuf[nOset/8] &= ~0x04 ; break ;
			case 6:	pBitbuf[nOset/8] &= ~0x02 ; break ;
			case 7:	pBitbuf[nOset/8] &= ~0x01 ; break ;
			}
		}
	}
}

bool	GetBits	(const char* pBitbuf, uint32_t nOset)
{
	//	Category:	Bitwise
	//
	//	Determine if the bit in the buffer at the given offset is set
	//
	//	Arguments:	1)	pBitbuf	The buffer
	//				2)	nOset	The offset (is divided by 8)
	//
	//	Returns:	True	If the requested bit number is set within the segment
	//				False	If the requested bit number is not set within the segment or it exceeds the segment range (0 - 4095)

	_hzfunc("__func__") ;

	if (!pBitbuf)
		return false ;

	switch	(nOset%8)
	{
	case 0:	return pBitbuf[nOset/8] & 0x80 ? true : false ;
	case 1:	return pBitbuf[nOset/8] & 0x40 ? true : false ;
	case 2:	return pBitbuf[nOset/8] & 0x20 ? true : false ;
	case 3:	return pBitbuf[nOset/8] & 0x10 ? true : false ;
	case 4:	return pBitbuf[nOset/8] & 0x08 ? true : false ;
	case 5:	return pBitbuf[nOset/8] & 0x04 ? true : false ;
	case 6:	return pBitbuf[nOset/8] & 0x02 ? true : false ;
	case 7:	return pBitbuf[nOset/8] & 0x01 ? true : false ;
	}

	return false ;
}


/*
**	SECTION 2:	Bitmap segment allocation and freelist regime
*/

class	_bm_seg
{
	//	The _bm_seg (bitmap segment) class comprises a count, a segment number and 32 bytes of storage. Currently this storage space is used as a bitmap to hold
	//	upto 256 'numbers' on behalf of hdbIdset which can hold as many as 65,535 such segments.

	uint16_t	m_count ;		//	Number of bits set
	uint16_t	m_segno ;		//	Segment number
	uchar		m_work[32] ;	//	Data buffer

	//	Non-implimented copy constructor
	_bm_seg	(const _bm_seg& op) ;

	//	Non-implimented Assignment operator
	_bm_seg&	operator=	(const _bm_seg& op) ;

public:
	_bm_seg		(void) { m_count = m_segno = 0 ; }
	~_bm_seg	(void) {}

	//	Metadata
	uint32_t	Count	(void) const	{ return m_count & 0x0fff ; }		//	Return population count
	uint32_t	Segno	(void) const	{ return m_segno ; }				//	Return segment number
	bool		Perm	(void) const	{ return m_count & 0x8000 ? false : true ; }

	//	Clear all bits in the segment
	void	Clear	(void)
	{
		m_count = m_segno = 0 ;
		memset(m_work, 0, 32) ;
	}

	//	Add an element to the segment
	uint32_t	Insert	(const hzSet<uint32_t>& idset, uint32_t& nPosn) ;
	void		Insert	(uint32_t nElement) ;

	//	Remove an element from the segment
	uint32_t	Delete	(uint32_t nElement) ;
	uint32_t	Delete	(const hzSet<uint32_t>& idset, uint32_t& nPosn) ;

	//	Where a segment has a single value
	bool		IsSet	(uint32_t oset) const ;

	//uint32_t	Value	(void) const ;

	//	Fetch a number of elements from the segment
	int32_t		_fetch	(hzVect<uint32_t>& Result, uint32_t nBase, uint32_t nStart, uint32_t nReq) const ;

	//	Boolean operators
	_bm_seg&	operator|=	(const _bm_seg& ss) ;
	_bm_seg&	operator&=	(const _bm_seg& ss) ;

	//	Import and Export
	hzEcode	Import	(hzChain::Iter& ci) ;
	hzEcode	Export	(hzChain& C) const ;

	//	Diagnostics
	void	Show	(hzLogger& xlog) const ;
} ;

class	_bm_seg_bloc
{
	//	This holds blocks of bitmap segments as part of the 32-bit bitmap segment addressing regime, needed to keep down the size of non-leaf segments as these
	//	contain 8 segment addresses apiece.

public:
	_bm_seg		m_space[1024] ;

	_bm_seg_bloc	(void)	{ _hzGlobal_Memstats.m_numBitmapSB++ ; }
	~_bm_seg_bloc	(void)	{ _hzGlobal_Memstats.m_numBitmapSB-- ; }
} ;

hzArray<_bm_seg_bloc*>	s_bmseg_blocs ;		//	Holds all mass allocations of bitmap segments

uint32_t	s_sp_free ;			//	Free list pointer
uint32_t	s_nSegprocs ;		//	Total number of segprocs

_bm_seg*	_bmseg_xlate	(uint32_t nAddr)
{
	//	Convert a 32-bit bitmap segment address to a bitmap segment pointer
	//
	//	Argument:	nAddr	Bitmap segment address
	//
	//	Returns:	Pointer to bitmap segment
	//				NULL if the address does not exists

	_bm_seg_bloc*	pBlk ;		//	Bitmap segemnt block pointer
	_bm_seg*		pSeg ;		//	Bitmap segment pointer
	uint32_t		blocNo ;	//	Block number
	uint32_t		blocOset ;	//	Block offset

	blocNo = nAddr / 1024 ;
	blocOset = nAddr % 1024 ;

	if (blocNo > s_bmseg_blocs.Count())
		return 0 ;
	pBlk = s_bmseg_blocs[blocNo] ;
	pSeg = pBlk->m_space + blocOset ;

	return pSeg ;
}

uint32_t	segAlloc	(void)
{
	//	Category:	Bitwise
	//
	//	Allocates an open segment processing region
	//
	//	Arguments:	None
	//	Returns:	Address of allocated segment
	//
	//	Note: Insufficient memory currently causes a program exit

	_hzfunc("segAlloc") ;

	_bm_seg_bloc*	pBlk ;		//	Segment bloc pointer
	_bm_seg*		pSeg ;		//	Segment pointer
	uint32_t*		pNum ;		//	Pointer to segment recast
	uint32_t		segAddr ;	//	Segment address
	uint32_t		n ;			//	Segment iterator

	if (s_sp_free)
	{
		segAddr = s_sp_free ;
		pSeg = _bmseg_xlate(segAddr) ;
		pNum = (uint32_t*) pSeg ;

		s_sp_free = *pNum ;
		pSeg->Clear() ;
		return segAddr ;
	}

	//	No segments free. Allocate a complete fresh block. Set all 1024 segments in the block to address the next block up. Then set the free pointer to the 2nd
	//	segment in the block. The 1st segment in the block is the one allocated.

	segAddr = s_bmseg_blocs.Count() ;

	if (!segAddr)
	{
		//	If there is nothing in the array of blocks, put in a null entry (this will make minimum segment address of 1024)
		s_bmseg_blocs.Add(0) ;
		segAddr++ ;
	}

	pBlk = new _bm_seg_bloc() ;
	if (!pBlk)
		hzexit(_fn, 0, E_MEMORY, "Could not allocate a bitmap segment bloc") ;
	s_bmseg_blocs.Add(pBlk) ;

	//	Now multiply by 1024
	segAddr *= 1024 ;

	pSeg = pBlk->m_space ;
	for (n = 0 ; n < 1024 ; n++)
	{
		pNum = (uint32_t*) pSeg ;
		*pNum = segAddr + n ;
	}

	return segAddr ;
}

void	segFree	(uint32_t segAddr)
{
	//	Category:	Bitwise
	//
	//	Frees a segment processing region
	//
	//	Argument:	segAddr	The address of segment to be freed
	//
	//	Returns:	None

	_bm_seg*	pSeg ;		//	Pointer to segment
	uint32_t*	pNum ;		//	Pointer to segment recast

	if (!s_sp_free)
		s_sp_free = segAddr ;
	else
	{
		//	Find the segment to be freed and set its pointer to the free list address. Then set the free list address to the segment being freed
		pSeg = _bmseg_xlate(segAddr) ;
		if (!pSeg)
			return ;

		pNum = (uint32_t*) pSeg ;
		*pNum = s_sp_free ;

		s_sp_free = segAddr ;
	}
}

/*
**	SECTION 3:	_bm_seg member functions
*/

uint32_t	_bm_seg::Insert	(const hzSet<uint32_t>& idset, uint32_t& nPosn)
{
	//	Insert multiple elements into the segment (set a bit in the segment). This takes the element in the set at the given position
	//	and assesses the segment number. This element and all other elements falling within the same segment number are inserted. The
	//	process stops when an element is encountered that belongs in a higher segment.
	//
	//	Arguments:	1)	idset	The set of object ids (all of which must have values between 0 and 4095)
	//				2)	nPosn	The iterator into the set of object ids.
	//
	//	Returns:	Number of bits set (document ids added) by the operation, 0 if nothing was added.

	_hzfunc("_bm_seg::Insert(set)") ;

	uint32_t	docId ;			//	The number from the idset
	uint32_t	segNo ;			//	Segment number implied by the docId
	uint32_t	nIndex ;		//	Offset into data
	uint32_t	noset ;			//	Total number of docIds added

	//	Check if there is anything to do
	if (!idset.Count())
		return 0 ;
	if (nPosn > idset.Count())
		return 0 ;

	//	Go thru set of object ids until we reach one that is out of range or we run out
	for (noset = 0 ; nPosn < idset.Count() ; nPosn++)
	{
		docId = idset.GetObj(nPosn) ;
		segNo = docId/BITMAP_SEGSIZE ;
		nIndex = docId % BITMAP_SEGSIZE ;

		switch	(nIndex % 8)
		{
		case 0:	m_work[nIndex/8] |= 0x80 ; break ;
		case 1:	m_work[nIndex/8] |= 0x40 ; break ;
		case 2:	m_work[nIndex/8] |= 0x20 ; break ;
		case 3:	m_work[nIndex/8] |= 0x10 ; break ;
		case 4:	m_work[nIndex/8] |= 0x08 ; break ;
		case 5:	m_work[nIndex/8] |= 0x04 ; break ;
		case 6:	m_work[nIndex/8] |= 0x02 ; break ;
		case 7:	m_work[nIndex/8] |= 0x01 ; break ;
		}

		noset++ ;
	}

	return noset ;
}

void	_bm_seg::Insert	(uint32_t docId)
{
	//	Insert an element into the segment (set a bit in the segment). Note this may change the length of the segment's data and if
	//	this is the case the segment data will have to be reallocated if this change of size also changes the size as a multiple of
	//	the alignment factor (8 bytes in the case of a 64 bit implimentation).
	//
	//	Arguments:	1)	docId	The element (expressed as a relative number between 0 and 4095)
	//
	//	Returns:	None

	_hzfunc("_bm_seg::Insert(int)") ;

	uint32_t	segNo ;		//	Segment number (for docId)
	uint32_t	segVl ;		//	Segment offset (for docId)
	uint32_t	nIndex ;		//	Offset into data

	segNo = docId/BITMAP_SEGSIZE ;
	segVl = docId%BITMAP_SEGSIZE ;
	nIndex = docId % BITMAP_SEGSIZE ;

	switch	(nIndex % 8)
	{
	case 0:	m_work[nIndex/8] |= 0x80 ; break ;
	case 1:	m_work[nIndex/8] |= 0x40 ; break ;
	case 2:	m_work[nIndex/8] |= 0x20 ; break ;
	case 3:	m_work[nIndex/8] |= 0x10 ; break ;
	case 4:	m_work[nIndex/8] |= 0x08 ; break ;
	case 5:	m_work[nIndex/8] |= 0x04 ; break ;
	case 6:	m_work[nIndex/8] |= 0x02 ; break ;
	case 7:	m_work[nIndex/8] |= 0x01 ; break ;
	}
}

uint32_t	_bm_seg::Delete	(const hzSet<uint32_t>& idset, uint32_t& nPosn)
{
	//	Delete multiple elements from the segment. This takes the element in the set at the given position
	//	and assesses the segment number. This element and all other elements falling within the same segment number are inserted. The
	//	process stops when an element is encountered that belongs in a higher segment.
	//
	//	Arguments:	1)	idset	The set of object ids (all of which must have values between 0 and 4095)
	//				2)	nPosn	The iterator into the set of object ids.
	//
	//	Returns:	1	If the operation was successful. This is also returned when the operand value is already set within the segment.
	//				0	If nothing was added.
	//
	//	Note: In tht event of either lack of memory or data corruption, the program will exit.

	_hzfunc("_bm_seg::Delete(set)") ;

	uint32_t	docId ;			//	The number from the idset
	uint32_t	segNo ;			//	Segment number implied by the docId
	uint32_t	nIndex ;		//	Offset into data
	uint32_t	nodel ;			//	Total number of docIds deleted

	//	Check if there is anything to do
	if (!idset.Count())
		return 0 ;

	//	Go thru set of object ids until we reach one that is out of range or we run out
	for (nodel = 0 ; nPosn < idset.Count() ; nPosn++)
	{
		docId = idset.GetObj(nPosn) ;
		segNo = docId / BITMAP_SEGSIZE ;
		nIndex = docId % BITMAP_SEGSIZE ;

		switch	(nIndex % 8)
		{
		case 0:	m_work[nIndex/8] &= ~0x80 ; break ;
		case 1:	m_work[nIndex/8] &= ~0x40 ; break ;
		case 2:	m_work[nIndex/8] &= ~0x20 ; break ;
		case 3:	m_work[nIndex/8] &= ~0x10 ; break ;
		case 4:	m_work[nIndex/8] &= ~0x08 ; break ;
		case 5:	m_work[nIndex/8] &= ~0x04 ; break ;
		case 6:	m_work[nIndex/8] &= ~0x02 ; break ;
		case 7:	m_work[nIndex/8] &= ~0x01 ; break ;
		}

		if (segNo != m_segno)
			break ;

		nodel++ ;
	}

	return nodel ;
}

uint32_t	_bm_seg::Delete	(uint32_t docId)
{
	//	Delete an element from the segment (clear a bit in the segment)
	//
	//	Arguments:	1)	docId	The element (expressed as a relative number between 0 and 4095)
	//
	//	Returns:	1	If the operation was successful. This is also returned when the operand value is already set within the segment.
	//				0	If nothing was deleted.
	//
	//	Note: In tht event of either lack of memory or data corruption, the program will exit.

	_hzfunc("_bm_seg::Delete") ;

	uint32_t	segNo ;			//	Segment number implied by the docId
	uint32_t	nIndex ;		//	Offset into data

	if (docId > 256)
		hzexit(_fn, 0, E_RANGE, "Deletes from segments must be 0 to 4095") ;

	segNo = docId / BITMAP_SEGSIZE ;
	nIndex = docId % BITMAP_SEGSIZE ;

	switch	(nIndex % 8)
	{
	case 0:	m_work[nIndex/8] &= ~0x80 ; break ;
	case 1:	m_work[nIndex/8] &= ~0x40 ; break ;
	case 2:	m_work[nIndex/8] &= ~0x20 ; break ;
	case 3:	m_work[nIndex/8] &= ~0x10 ; break ;
	case 4:	m_work[nIndex/8] &= ~0x08 ; break ;
	case 5:	m_work[nIndex/8] &= ~0x04 ; break ;
	case 6:	m_work[nIndex/8] &= ~0x02 ; break ;
	case 7:	m_work[nIndex/8] &= ~0x01 ; break ;
	}

	return 1 ;
}

int32_t	_bm_seg::_fetch	(hzVect<uint32_t>& Result, uint32_t segNo, uint32_t nStart, uint32_t nReq) const
{
	//	Obtain a number of ids from the segment.
	//
	//	Arguments:	1)	Result	The vector of uint32_ts to store the result
	//				2)	segNo	The segment number (effects values of doc ids placed in result)
	//				3)	nStart	The number of ids to ignore
	//				4)	nReq	The number of ids required
	//
	//	Returns:	Number of numbers added to the result by this function call

	_hzfunc("_bm_seg::_fetch") ;

	uint32_t	nPos ;		//	Bit iterator
	uint32_t	nBase ;		//	Base value of segment (from segment number)
	uint32_t	nFound ;	//	Bits found to be set
	uint32_t	nCount ;	//	Segment buffer iterator
	hzEcode		rc ;		//	Return code

	Result.Clear() ;

	//	This should never happen
	if (nStart > 255)
		return -1 ;

	//	If requested start is beyond the population count no values can be added to the vector so just return
	if (nStart >= Count())
		return 0 ;

	//	Bypass bytes until we reach start
	for (nCount = 0 ; nCount < 32 ; nCount++)
	{
		//	Add up bits found
		nFound += s_BitArray[m_work[nCount]] ;
		if (nFound >= nStart)
			break ;
	}

	nCount *= 8 ;
	nFound = 0 ;

	//	Add the rest of the results upto the upper limit to the vector
	nBase = segNo * BITMAP_SEGSIZE ;
	for (nFound = nPos = 0 ; nPos < 256 && nFound < nReq ; nPos++)
	{
		if (IsSet(nPos))
		{
			if (nCount >= nStart)
			{
				Result.Add(nBase + nPos) ;
				nFound++ ;
				if (nFound == nReq)
					break ;
			}
			nCount++ ;
		}
	}

	return nFound ;
}

bool	_bm_seg::IsSet	(uint32_t nIndex) const
{
	//	Apply the [] operator to this segment
	//
	//	Arguments:	1)	segVal	The bit number to test for
	//
	//	Returns:	True	If the requested bit number is set within the segment
	//				False	If the requested bit number is not set within the segment or it exceeds the segment range (0 - 4095)

	_hzfunc("_bm_seg::IsSet") ;

	if (nIndex > 256)
		return false ;

	switch	(nIndex%8)
	{
	case 0:	return m_work[nIndex/8] & 0x80 ? true : false ;
	case 1:	return m_work[nIndex/8] & 0x40 ? true : false ;
	case 2:	return m_work[nIndex/8] & 0x20 ? true : false ;
	case 3:	return m_work[nIndex/8] & 0x10 ? true : false ;
	case 4:	return m_work[nIndex/8] & 0x08 ? true : false ;
	case 5:	return m_work[nIndex/8] & 0x04 ? true : false ;
	case 6:	return m_work[nIndex/8] & 0x02 ? true : false ;
	case 7:	return m_work[nIndex/8] & 0x01 ? true : false ;
	}

	return false ;
}

//	Boolean operators
_bm_seg&	_bm_seg::operator|=	(const _bm_seg& ss)
{
	//	ORs this segment with the operand
	//
	//	Arguments:	1)	ss	The other segment
	//
	//	Returns:	Reference to this segment

	_hzfunc("_bm_seg::operator|=") ;

	//	Anything to do?
	if (!ss.Count())
		return *this ;

	//	OK do it

	return *this ;
}

_bm_seg&	_bm_seg::operator&=	(const _bm_seg& ss)
{
	//	ANDs this segment with the operand
	//
	//	Arguments:	1)	ss	The other segment
	//
	//	Returns:	Reference to this segment

	_hzfunc("_bm_seg::operator&=") ;

	//	Anything to do?
	if (!Count())
		return *this ;

	if (!ss.Count())
		{ Clear() ; return *this ; }

	//	OK do it
	return *this ;
}

hzEcode	_bm_seg::Export	(hzChain& C) const
{
	//	Export segment to a chain that can be written to a file. The format is 'human readable' with each byte of the data expanded out
	//	to a pair of hexadecimal digits and the whole enclosed in square brackets. In the export of a bitmap these forms appear one per
	//	line and are preceeded by a decimal segment number.
	//
	//	Arguments:	1)	C	Aggregation chain
	//
	//	Returns:	E_CORRUPT	If anything is wrong with the segment
	//				E_OK		If the operation was successful

	uint32_t	val ;			//	Used to extract segment number
	uint32_t	byte ;			//	Used to extract most and then least significant 4 bits from encoded segment byte
	uint32_t	nCount ;		//	Number of bytes written

	if (!nCount)
		return E_NODATA ;

	for (nCount = 0 ; nCount < val ; nCount++)
	{
		byte = (m_work[nCount] & 0xf0) >> 4 ;
		C.AddByte(byte > 9 ? byte - 10 + 'a' : byte + '0') ;

		byte = (m_work[nCount] & 0x0f) ;
		C.AddByte(byte > 9 ? byte - 10 + 'a' : byte + '0') ;
	}

	C.AddByte(CHAR_SQCLOSE) ;
	return E_OK ;
}

hzEcode	_bm_seg::Import	(hzChain::Iter& ci)
{
	//	Import segment from a chain. The first two bytes are assumed to be the segment id, the next two the byte count which will be
	//	how many further bytes to read in. The whole will be stored in the _segdata regime.
	//
	//	Note that numerous regimes may apply regarding segment metadata. The copy count is not needed for import/export purposes. The
	//	segment number used in the importing process may differ from that assigned by the exporting process. However it is exported
	//	just in case and by convention appears as a decimal (of variable length) before the segment data enclosed in the square [].
	//	In the future the length (first two bytes in []) will be removed but for now it is simply ignored. It is read in but regardless
	//	of it's value the real length is derived by counting bytes within the [] and adding 2 for the copy count and 2 more for the
	//	segment number.
	//
	//	Arguments:	1)	ci	Chain iterator (advanced by the import operation)
	//
	//	Returns:	E_CORRUPT	If anything is wrong with the segment
	//				E_OK		If the operation was successful

	_hzfunc("_bm_seg::Import") ;

	uint32_t	xlen ;			//	Memory aligned segment length
	uint32_t	len ;			//	Segment length
	uint32_t	valA ;			//	Working byte in working buffer
	uint32_t	segNo ;			//	Segment number
	uchar		buf	[520] ;		//	Working buffer

	Clear() ;

	if (!IsDigit(*ci))
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Expected a segment number") ;

	for (len = 0 ; IsDigit(*ci) ; len++, ci++)
		buf[len] = *ci ;
	buf[len] = 0 ;
	segNo = atoi((char*)buf) ;

	if (*ci != CHAR_SQOPEN)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "After segment number, expected an opening square bracket") ;

	for (len = 0, ci++ ; IsHex(*ci) ; ci++)
	{
		valA = *ci >= 'a' ? *ci - 'a' + 10 : *ci - '0' ;
		valA *= 16 ;
		ci++ ;
		if (!IsHex(*ci))
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "2nd part of byte pair is not hexadecimal (%c)", *ci) ;
		valA += (*ci >= 'a' ? *ci - 'a' + 10 : *ci - '0') ;

		buf[len] = (char) valA ;
		len++ ;
	}

	if (*ci != CHAR_SQCLOSE)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "After segment data, expected a closing square bracke") ;
	ci++ ;

	memcpy(m_work, buf, xlen - 6) ;
	return E_OK ;
}

/*
**	SECTION 4:	hdbIdset Member Functions
*/

hdbIdset::hdbIdset	(const hdbIdset& op)
{
	mx = op.mx ;
	if (mx)
		mx->m_copies++ ;
	_hzGlobal_Memstats.m_numBitmaps++ ;
}

hdbIdset::hdbIdset	(void)
{
	mx = 0 ;
	_hzGlobal_Memstats.m_numBitmaps++ ;
}

hdbIdset::~hdbIdset   (void)
{
	Clear() ;
	_hzGlobal_Memstats.m_numBitmaps-- ;
}

void	hdbIdset::Clear   (void)
{
	//	Clears the bitmap (removes all segments)
	//
	//	Arguments:	None
	//	Returns:	None

	if (mx)
	{
		if (mx->m_copies)
			mx->m_copies-- ;
		else
		{
			mx->m_segments.Clear() ;
			delete mx ;
		}
		mx = 0 ;
	}
}

uint32_t	hdbIdset::Insert	(uint32_t docId)
{
	//	Insertation of a document id is effected by first establishing the number of the segment the document id would be in and then seeing if that
	//	segment actually exists. If it does not it is created. Then the Insert method is called on the segment to insert the document id.
	//
	//	Arguments:	1)	docId	The document id to insert
	//
	//	Returns:	1	If the supplied id did not exist in the bitmap prior to this call
	//				0	If the supplied id already existed in the bitmap

	_hzfunc("hdbIdset::Insert") ;

	_bm_seg*	pSeg ;		//	Target segment
	uint32_t	segNo ;		//	Target segment number
	uint32_t	segOset ;	//	Target segment offset
	uint32_t	segAddr ;	//	Target segment number

	segNo = docId / 256 ;
	segOset = docId % 256 ;

	if (!mx)
		mx = new _bitmap_ca() ;

	if (!mx->m_segments.Exists(segNo))
	{
		//	Segment does not exist so create
		segAddr = segAlloc() ;
		pSeg = _bmseg_xlate(segAddr) ;
		pSeg->Insert(segOset) ;

		mx->m_segments.Insert(segNo, segAddr) ;
		mx->m_Total++ ;
		return 1 ;
	}

	//	Segment already exists
	segAddr = mx->m_segments[segNo] ;
	pSeg = _bmseg_xlate(segAddr) ;

	if (pSeg->IsSet(segOset))
	{
		//	Value is new
		pSeg->Insert(segOset) ;
		mx->m_Total++ ;
		return 1 ;
	}

	//	Value already exists
	return 0 ;
}

hzEcode	hdbIdset::Delete	(uint32_t docId)
{
	//	Delete a document/object id from the bitmap
	//
	//	Argument:	docId	The document/object id
	//
	//	Returns:	E_NOTFOUND	If the document/object id was not in the bitmap
	//				E_OK		If it was and now isn't

	_hzfunc("hdbIdset::Delete") ;

	_bm_seg*	pSeg ;		//	Target segment
	uint32_t	segNo ;		//	Target segment number
	uint32_t	segOset ;	//	Target segment offset
	uint32_t	segAddr ;	//	Target segment number

	if (!mx)
		return E_NOTFOUND ;

	segNo = docId / 256 ;
	segOset = docId % 256 ;

	if (!mx->m_segments.Exists(segNo))
		return E_NOTFOUND ;

	segAddr = mx->m_segments[segNo] ;
	pSeg = _bmseg_xlate(segAddr) ;

	if (!pSeg->IsSet(segOset))
		return E_NOTFOUND ;

	//	Clear the bit
	pSeg->Delete(segOset) ;
	return E_OK ;
}

uint32_t	hdbIdset::DelRange	(uint32_t lo, uint32_t hi)
{
	//	Delete a whole segment from the bitmap. This is useful in regimes where there is a correlation between document ids and say dates and after a while the
	//	document are archived off and drop out of the main system and thus the index.
	//
	//	Arguments:	1)	lo	The lowest number to be deleted
	//				2)	hi	The highest number to be deleted
	//
	//	Returns:	Number actually deleted

	//	STUB: Not needed yet
	return 0 ;
}

void	hdbIdset::Export	(hzChain& Z) const
{
	//	Export the bitmap and it's segments to the supplied chain. Be aware this can produce chains of several megabytes.
	//
	//	Arguments:	1)	Z	The aggregation chain
	//
	//	Returns:	None

	_hzfunc("hdbIdset::Export") ;

	_bm_seg*	pSeg ;			//	Segment
	uint32_t	segAddr ;		//	Segment address
	uint32_t	nIndex ;		//	Iterator

	//	Is there anything to do?
	if (Count())
	{
		//	Easiest way is to agregate the segments from both this bitmap and the operand, to a set of segments
		for (nIndex = 0 ; nIndex < mx->m_segments.Count() ; nIndex++)
		{
			segAddr = mx->m_segments.GetObj(nIndex) ;
			pSeg = _bmseg_xlate(segAddr) ;

			pSeg->Export(Z) ;
			Z.AddByte(CHAR_NL) ;
		}
	}
}

hzEcode	hdbIdset::Import	(hzChain::Iter& ci)
{
	//	Import (Load) a bitmap from the supplied chain iterator
	//
	//	Arguments:	1)	ci	The chain iterator
	//
	//	Returns:	E_SYNTAX	If there is an error in processing. This will leave the bitmap in an undefined state
	//				E_OK		If the operation is successful

	_hzfunc("hdbIdset::Import") ;

	//	STUB
	return E_OK ;
}

uint32_t	hdbIdset::Fetch	(hzVect<uint32_t>& Result, uint32_t nStart, uint32_t nReq) const
{
	//	Purpose:	Fetch ids from the bitmap into an id buffer
	//
	//	Arguments:	1)	Result	The id buffer (type uint32_t)
	//				2)	nStart	The start position (ignore all ids before this)
	//				3)	nReq	The number to fetch
	//
	//	Returns:	Number of ids fetched

	_hzfunc("hdbIdset::Fetch") ;

	hzVect<uint32_t>	temp ;	//	For querying segments

	_bm_seg*	pSeg ;			//	Segment (to process)
	uint32_t	nIndex ;		//	Iterator
	uint32_t	nPosn = 0 ;		//	Position (in the bitmap so far) attained last segment
	uint32_t	nMax = 0 ;		//	Maximum id this segment
	uint32_t	segNo ;			//	Segment number
	uint32_t	segAddr ;		//	Segment address
	uint32_t	nSegStart ;		//	Starting position within a segment
	uint32_t	x ;				//	For iterating resut of segment fetch
	uint32_t	v ;				//	For value

	Result.Clear() ;

	if (nStart >= mx->m_Total)
		return 0 ;

	//if (nStart < 0)	return hzerr(_fn, HZ_ERROR, E_RANGE, "Negaive start positions not permited") ;
	//if (nReq < 0)	return hzerr(_fn, HZ_ERROR, E_RANGE, "Negaive required values not permited") ;

	//	Now we have the range of segments for the bitmap.
	for (nIndex = 0 ; nIndex < mx->m_segments.Count() ; nIndex++)
	{
		segAddr = mx->m_segments.GetObj(nIndex) ;
		segNo = mx->m_segments.GetKey(nIndex) ;

		pSeg = _bmseg_xlate(segAddr) ;

		nMax += pSeg->Count() ;

		if (nMax > nStart)
		{
			//	Get ids from the segment until the result buffer is full
			if (nStart > nPosn)
				nSegStart = nStart - nPosn ;
			else
				nSegStart = 0 ;

			//	Tricky bit is fething from the segment
			pSeg->_fetch(temp, segNo, nSegStart, nReq - Result.Count()) ;

			for (x = 0 ; x < temp.Count() ; x++)
			{
				v = temp[x] ;
				Result.Add(v) ;
			}
		}

		nPosn = nMax ;
	}

	return Result.Count() ;
}

//  Operators
hdbIdset&	hdbIdset::operator=	(const hdbIdset& op)
{
	//	Assign a hdbIdset instance to another. This will only facilitate a soft copy.
	//
	//	Arguments:	1)	The supplied bitmap
	//
	//	Returns:	Reference to this bitmap instance in all cases.

	_hzfunc("hdbIdset::operator=") ;

	if (mx == op.mx)
		return *this ;

	Clear() ;
	mx = op.mx ;
	if (mx)
		mx->m_copies++ ;

	return *this ;
}

hdbIdset&	hdbIdset::operator|=	(const hdbIdset& op)
{
	//	Run through the segments in the supplied operand bitmap and request each in turn. If there is no corresponding segment in this bitmap then insert a copy
	//	of the segment from the operand bitmap into this bitmap. If this bitmap does have the corresponding segment OR the two segments together.
	//
	//	Arguments:	1)	The supplied bitmap
	//
	//	Returns:	Reference to this bitmap instance in all cases.

	_hzfunc("hdbIdset::operator|=") ;

	_bm_seg*	pSegT ;		//	Segments from this bitmap
	_bm_seg*	pSegS ;		//	Segments from the supplied (operand) bitmap
	uint32_t	segAddrT ;	//	Segment address (this)
	uint32_t	segAddrS ;	//	Segment address (supplied)
	uint32_t	nIndex ;	//	Iterator
	uint32_t	segNo ;		//	Segment number

	//	Is there anything or OR with?
	if (!op.Count())
		return *this ;

	//	Easiest way is to agregate the segments from both this bitmap and the operand, to a set of segments
	for (nIndex = 0 ; nIndex < op.mx->m_segments.Count() ; nIndex++)
	{
		segAddrS = op.mx->m_segments.GetObj(nIndex) ;
		pSegS = _bmseg_xlate(segAddrS) ;

		segNo = op.mx->m_segments.GetKey(nIndex) ;

		if (mx->m_segments.Exists(segNo))
		{
			//	Does this bitmap have this segment number?
			segAddrT = mx->m_segments[segNo] ;
			pSegT = _bmseg_xlate(segAddrT) ;

			mx->m_Total -= pSegT->Count() ;
			*pSegT |= *pSegS ;
			mx->m_Total += pSegT->Count() ;
			continue ;
		}

		//	The segment is not present in this bitmap so add
		mx->m_segments.Insert(segNo, segAddrS) ;
		mx->m_Total += pSegS->Count() ;
	}

	return *this ;
}

hdbIdset&	hdbIdset::operator&=	(const hdbIdset& op)
{
	//	Starting with the lowest segments in both this bitmap and the operand, delete from this bitmap all segments that are not found
	//	in the operand. Each segment in this bitmap then undergoes an &= operation with the corresponding segment from the operand. At
	//	the end of this process run through the segments and add up the count.
	//
	//	Arguments:	1)	The supplied bitmap
	//
	//	Returns:	Reference to this bitmap instance in all cases.

	_hzfunc("hdbIdset::operator&=") ;

	_bm_seg*	pSegT ;		//	Segments from this bitmap
	_bm_seg*	pSegS ;		//	Segments from the supplied (operand) bitmap
	uint32_t	segAddrT ;	//	Segment address (this)
	uint32_t	segAddrS ;	//	Segment address (supplied)
	uint32_t	nIndex ;	//	Iterator
	uint32_t	segNo ;		//	Segment number

	//	Is there anything to AND with?
	if (!op.Count())
		{ Clear() ; return *this ; }

	//	If this map is empty an AND operation cannot populate it - just return
	if (!Count())
		return *this ;

	nIndex = 0 ;
	for (; nIndex < mx->m_segments.Count() ;)
	{
		segAddrT = mx->m_segments.GetObj(nIndex) ;
		pSegT = _bmseg_xlate(segAddrT) ;

		segNo = mx->m_segments.GetKey(nIndex) ;

		mx->m_Total -= pSegT->Count() ;

		if (!op.mx->m_segments.Exists(segNo))
		{
			mx->m_segments.Delete(segNo) ;
			continue ;
		}

		segAddrS = op.mx->m_segments[segNo] ;
		pSegS = _bmseg_xlate(segAddrS) ;

		*pSegT &= *pSegS ;

		if (!pSegT->Count())
		{
			mx->m_segments.Delete(segNo) ;
			continue ;
		}

		mx->m_Total += pSegT->Count() ;
		mx->m_segments.Insert(segNo, segAddrT) ;
		nIndex++ ;
	}

	return *this ;
}
