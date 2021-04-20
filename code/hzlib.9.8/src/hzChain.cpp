//
//	File:	hzChain.cpp
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
#include <iostream>

#include <stdarg.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzChain.h"
#include "hzIpaddr.h"
#include "hzEmaddr.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Definitions
*/

#define	ZBLKSIZE	1460

struct	_zblk
{
	//	Category:	Data
	//
	//	Data container for hzChain

	_zblk*		prev ;				//	prev block in list
	_zblk*		next ;				//	next block in list
	uint16_t	xize ;				//	bytes used
	uint16_t	m_Id ;				//	Used to check corruption (all blocks in a chain have to bear the same id as the chain)
	char		m_Data[ZBLKSIZE] ;	//	data area

	void	clear	(void)
	{
		//	Clear chain block content
		//
		//	Arguments:	None
		//	Returns:	None

		prev = next = 0 ;
		xize = 0 ;
		memset(m_Data, 0, ZBLKSIZE) ;
	}

	_zblk	(void)
	{
		m_Id = 0xffff ;
		clear() ;
	}
} ;

/*
**	Memory management regime for chains
*/

static	hzLockRWD	s_chain_mutex("hzChain mutex") ;	//	Protects the memory allocation

static	_zblk*	s_zblk_free ;				//	Free list pointer

/*
**	Section 1A:	hzChain private functions
*/

_zblk*	_zblk_alloc	(void)
{
	//	Allocate a chain block. Either draw from a list of free blocks or create a new block
	//
	//	Arguments:	1)	id	The chain id	
	//
	//	Returns:	Pointer to a usable z-block

	_hzfunc("_zblk_alloc") ;

	_zblk*	zp = 0 ;	//	Pointer to a real block (8 bytes)

	s_chain_mutex.LockWrite() ;

		if (s_zblk_free)
		{
			//	Set allocation address to the freelist, set the freelist to the next in the list.
			zp = s_zblk_free ;
			s_zblk_free = zp->next ;

			_hzGlobal_Memstats.m_numChainBF-- ;
		}
		else
		{
			//	No free blocks so allocate a new one
			zp = new _zblk() ;

			if (zp)
				_hzGlobal_Memstats.m_numChainBlks++ ;
		}

	//	Return the address
	s_chain_mutex.Unlock() ;

	if (!zp)
		Fatal("%s. No chain block allocated\n", *_fn) ;

	//	Clear the free block's values to ready it for use
	zp->clear() ;
	//zp->m_Id = id ;

	return zp ;
}

#if 0
uint16_t	_setchainid	(void)
{
	//	Set chain id (for diagnostics)
	//
	//	Arguments:	None
	//	Returns:	Number being id for the new chain

	static	uint16_t	s_SeqChain = 0 ;	//	Sequence for chain id

	if (s_SeqChain == 0xfffe)
		s_SeqChain = 0 ;

	return ++s_SeqChain ;
}
#endif

int32_t	hzChain::_compare	(const hzChain& op) const
{
	//	Lexical compare
	//
	//	Arguments:	1)	op	The other chain to which this chain is compared
	//
	//	Returns:	>0	If this chain is lexically greater than the supplied chain
	//				<0	If this chain is lexically lesser than the suplied chain
	//				0	If the two chains are the same

	_hzfunc("hzChain::_compare") ;

	chIter	a ;		//	Chain iterator this
	chIter	b ;		//	Chain iterator supplied

	int32_t	res ;	//	Comparison result

	for (a = *this, b = op ; !a.eof() && !b.eof() ; a++, b++)
	{
		if (*a != *b)
			break ;
	}

	if (a.eof() && b.eof())
		return 0 ;

	if (a.eof())
		res = *b ;
	else if (b.eof())
		res = *a ;
	else
		res = *a - *b ;

	return res ;
}

/*
**	Section 1B:	hzChain public functions
*/

void	hzChain::Clear	(void)
{
	//	Clears all content held by the hzChain.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzChain::Clear") ;

	_chain*		cx ;		//	Temp pointer to control
	_zblk*		zp ;		//	Block pointer
	uint32_t	count ;		//	Number of blocks in chain

	//	Already no content - no action required.
	if (!mx)
		return ;

	//	If this chain is one of many pointing to the same contents, decrement the copy count and detach this chain from the contents.
	cx = mx ;
	mx = 0 ;

    if (_hzGlobal_MT)
    {
        __sync_add_and_fetch((uint32_t*)&(cx->m_copy), -1) ;

        if (cx->m_copy)
			return ;
    }
    else
    {
        cx->m_copy-- ;

        if (cx->m_copy)
			return ;
    }


	if (!cx->m_Begin)
	{
		delete cx ;
		return ;
	}

	//	This chain is the sole owner of the contents so delete by returning all blocks to the free list of blocks. As the blocks are linked, we
	//	only need to set the next pointer in the last block to the free list and then set the free list to the first block.

	s_chain_mutex.LockWrite() ;

		//	Check for corruption
		if (!cx->m_End)
			hzexit(_fn, 0, E_CORRUPT, "The internal begin/end blocks are %p and %p", cx->m_Begin, cx->m_End) ;

		zp = (_zblk*) cx->m_End ;
		zp->next = s_zblk_free ;
		s_zblk_free = (_zblk*) cx->m_Begin ;
		cx->m_Begin = cx->m_End = 0 ;
		cx->m_nSize = 0 ;

		count = cx->m_nSize / ZBLKSIZE ;
		if (cx->m_nSize % ZBLKSIZE)
			count++ ;

		_hzGlobal_Memstats.m_numChainBF += count ;

	s_chain_mutex.Unlock() ;

	delete cx ;
}

hzChain::hzChain	(void)
{
	//  Construct an empty hzChain instance. Increment the global count of currently allocated hzChain instances for memory use reporting purposes.

	_hzGlobal_Memstats.m_numChain++ ;
	mx = 0 ;
}

hzChain::hzChain	(const hzChain& op)
{
	//	Copy constructor

	if (op.mx)
	{
    	if (_hzGlobal_MT)
        	__sync_add_and_fetch((uint32_t*)&(op.mx->m_copy), 1) ;
    	else
        	op.mx->m_copy++ ;
	}

	mx = op.mx ;
}

hzChain::~hzChain	(void)
{
	//  Delete this hzChain instance. Decrement the global count of currently allocated hzChain instances for memory use reporting purposes.

	_hzGlobal_Memstats.m_numChain-- ;
	Clear() ;
	delete mx ;
}

hzChain::_chain::_chain	(void)
{
	//	Allocate the hzChain Data Container

	m_Begin = m_End = 0 ;
	m_nSize = 0 ;
	m_copy = 1 ;
	_hzGlobal_Memstats.m_numChainDC++ ;
}

hzChain::_chain::~_chain	(void)
{
	_hzGlobal_Memstats.m_numChainDC-- ;
}

#define xnumlen64(v)	(v >= 0x1000000000000000 ? 16	\
						:v >= 0x100000000000000 ? 15	\
						:v >= 0x10000000000000 ? 14		\
						:v >= 0x1000000000000 ? 13		\
						:v >= 0x100000000000 ? 12		\
						:v >= 0x10000000000 ? 11		\
						:v >= 0x1000000000 ? 10			\
						:v >= 0x100000000 ? 9			\
						:v >= 0x10000000 ? 8			\
						:v >= 0x1000000 ? 7		\
						:v >= 0x100000 ? 6		\
						:v >= 0x10000 ? 5		\
						:v >= 0x1000 ? 4		\
						:v >= 0x100 ? 3			\
						:v >= 0x10 ? 2 : 1)

uint32_t	_numlen32	(int32_t v)
{
	return v>=1000000000 ? 10:v>=100000000 ? 9:v>=10000000 ? 8:v>=1000000 ? 7:v>=100000 ? 6:v>=10000 ? 5:v>=1000 ? 4:v>=100 ? 3 :v>=10 ? 2:1 ;
}

uint32_t	_numlen32	(uint32_t v)
{
	return v>=1000000000 ? 10:v>=100000000 ? 9:v>=10000000 ? 8:v>=1000000 ? 7:v>=100000 ? 6:v>=10000 ? 5:v>=1000 ? 4:v>=100 ? 3 :v>=10 ? 2:1 ;
}

uint32_t	_numlen64	(int64_t v)
{
	if (v >= 1000000000000)
		{ v /= 1000000000000 ; return v>=10000000 ? 20 : v>=1000000 ? 19 : v>=100000 ? 18 : v>=10000 ? 17 : v>=1000 ? 16 : v>=100 ? 15 : v>=10 ? 14 : 13 ; }

	if (v >= 1000000)
		{ v /= 1000000 ; return v>=100000 ? 12 : v>=10000 ? 11 : v>=1000 ? 10 : v>=100 ? 9 : v>=10 ? 8 : 7 ; }

	return v>=100000 ? 6 : v>=10000 ? 5 : v>=1000 ? 4 : v>=100 ? 3 : v>=10 ? 2 : 1 ;
}

uint32_t	_numlen64	(uint64_t v)
{
	if (v >= 1000000000000)
		{ v /= 1000000000000 ; return v>=10000000 ? 20 : v>=1000000 ? 19 : v>=100000 ? 18 : v>=10000 ? 17 : v>=1000 ? 16 : v>=100 ? 15 : v>=10 ? 14 : 13 ; }

	if (v >= 1000000)
		{ v /= 1000000 ; return v>=100000 ? 12 : v>=10000 ? 11 : v>=1000 ? 10 : v>=100 ? 9 : v>=10 ? 8 : 7 ; }

	return v>=100000 ? 6 : v>=10000 ? 5 : v>=1000 ? 4 : v>=100 ? 3 : v>=10 ? 2 : 1 ;
}

uint32_t	hzChain::_vainto	(const char* fmt, va_list ap)
{
	//	This is the 'backbone' of the Printf function. It is not to be called at the application level.
	//
	//	Arguments:	1)	fmt		Formated text input
	//				2)	ap		Variable arguments list
	//
	//	Returns:	Number of bytes written

	_hzfunc("hzChain::_vainto") ;

	hzChain			C ;					//	For processing hzChain instances
	const char*		i ;					//	For processing format string
	const char*		j ;					//	For processing arguments
	_atomval		val ;				//	Accommodate different value sizes
	_atomval		div ;				//	Accommodate different value sizes
	hzString		S ;					//	For processing hzString instances
	uint32_t		nCount = 0 ;		//	Number of bytes added to chain
	uint32_t		x ;					//	Offset needed to bypass format command
	uint32_t		nLenObj ;			//	Length of textualized object
	uint32_t		nLenMax ;			//	Length specified in format command
	uint32_t		nGap ;				//	Number of pad chars to print
	int32_t			res ;				//	Result of division by divider
	bool			bLeft = false ;		//	Force left justify
	char			cPad ;				//	Padding char, either space or zero

	for (i = fmt ; *i ; i++)
	{
		//	Parse thru text as is until a percent directive
		if (*i != '%')
		{
			AddByte(*i) ;
			nCount++ ;
			continue ;
		}

		//	The double percent is a percent
		if (i[1] == '%')
		{
			AddByte(*i) ;
			nCount++ ;
			i++ ;
			continue ;
		}

		//	We have a percent directive
		nLenObj = nLenMax = 0 ;
		x = 1 ;

		if (i[x] == '-')
			{ bLeft = true ; x++ ; }

		if (i[x] >= '0' && i[x] <= '9')
		{
			//	This means we have to insert padding
			cPad = i[x] == '0' ? '0' : ' ' ;

			for (nLenMax = 0 ; i[x] >= '0' && i[x] <= '9' ; x++)
			{
				nLenMax *= 10 ;
				nLenMax += i[x] - '0' ;
			}
		}

		val.m_uInt64 = 0 ;

		switch (i[x])
		{
		case 'c':	//	Single char
					val.m_sInt32 = va_arg(ap, int) ;
					AddByte(val.m_sInt32 & 0xff) ;
					nCount++ ;
					i += x ;
					break ;

		case 's':	//	Normal string
					j = va_arg(ap, const char*) ;
					if (!j)
					{
						i++ ;
						break ;
					}
					if (bLeft)
					{
						nLenObj = strlen(j) ;
						for (; *j ; j++)
							{ AddByte(*j) ; nCount++ ; }
						if (nLenMax > nLenObj)
							for (nGap = nLenMax - nLenObj ; nGap > 0 ; nGap--)
								{ AddByte(CHAR_SPACE) ; nCount++ ; }
					}
					else
					{
						if (nLenMax)
						{
							nLenObj = strlen(j) ;
							if (nLenMax > nLenObj)
								for (nGap = nLenMax - nLenObj ; nGap > 0 ; nGap--)
									{ AddByte(CHAR_SPACE) ; nCount++ ; }
						}
						for (; *j ; j++)
							{ AddByte(*j) ; nCount++ ; }
					}
					i += x ;
					break ;

		case 'd':	//	int32_t value
					val.m_sInt32 = (int32_t) va_arg(ap, int) ;
					if (val.m_sInt32 < 0)
						{ AddByte('-') ; nCount++ ; val.m_sInt32 *= -1 ; nLenObj++ ; }

					nLenObj += _numlen32(val.m_sInt32) ;
					if (nLenMax > nLenObj)
						for (nGap = nLenMax - nLenObj ; nGap > 0 ; nGap--)
							{ AddByte(cPad) ; nCount++ ; }

					for (div.m_sInt32 = 1000000000 ; div.m_sInt32 > 1 && div.m_sInt32 > val.m_sInt32 ; div.m_sInt32 /= 10) ;

					for (; div.m_sInt32 ; div.m_sInt32 /= 10)
						{ res = val.m_sInt32/div.m_sInt32 ; AddByte('0' + (res & 0x7f)) ; nCount++ ; val.m_sInt32 -= (res * div.m_sInt32) ; }
					i += x ;
					break ;

		case 'u':	//	uint32_t value
					val.m_uInt32 = (uint32_t) va_arg(ap, int) ;
					nLenObj += _numlen32(val.m_uInt32) ;
					if (nLenMax > nLenObj)
						for (nGap = nLenMax - nLenObj ; nGap > 0 ; nGap--)
							{ AddByte(cPad) ; nCount++ ; }

					for (div.m_uInt32 = 1000000000 ; div.m_uInt32 > 1 && div.m_uInt32 > val.m_uInt32 ; div.m_uInt32 /= 10) ;

					for (; div.m_uInt32 ; div.m_uInt32 /= 10)
						{ res = val.m_uInt32/div.m_uInt32 ; AddByte('0' + (res & 0x7f)) ; nCount++ ; val.m_uInt32 -= (res * div.m_uInt32) ; }
					i += x ;
					break ;

		case 'l':	//	int64_t value
					val.m_sInt64 = (int64_t) va_arg(ap, int64_t) ;
					if (val.m_sInt64 < 0)
						{ AddByte('-') ; nCount++ ; val.m_sInt64 *= -1 ; nLenObj++ ; }

					nLenObj += _numlen64(val.m_sInt64) ;
					if (nLenMax > nLenObj)
						for (nGap = nLenMax - nLenObj ; nGap > 0 ; nGap--)
							{ AddByte(cPad) ; nCount++ ; }

					for (div.m_sInt64 = 1000000000000000000 ; div.m_sInt64 > 1 && div.m_sInt64 > val.m_sInt64 ; div.m_sInt64 /= 10) ;

					for (; div.m_sInt64 ; div.m_sInt64 /= 10)
						{ res = val.m_sInt64/div.m_sInt64 ; AddByte('0' + (res & 0x7f)) ; nCount++ ; val.m_sInt64 -= (res * div.m_sInt64) ; }
					i += x ;
					break ;

		case 'L':	//	uint64_t value
					val.m_uInt64 = (uint64_t) va_arg(ap, uint64_t) ;
					nLenObj += _numlen64(val.m_uInt64) ;
					if (nLenMax > nLenObj)
						for (nGap = nLenMax - nLenObj ; nGap > 0 ; nGap--)
							{ AddByte(cPad) ; nCount++ ; }

					for (div.m_sInt64 = 1000000000000000000 ; div.m_uInt64 > 1 && div.m_uInt64 > val.m_uInt64 ; div.m_uInt64 /= 10) ;

					for (; div.m_uInt64 ; div.m_uInt64 /= 10)
						{ res = val.m_uInt64/div.m_uInt64 ; AddByte('0' + (res & 0x7f)) ; nCount++ ; val.m_uInt64 -= (res * div.m_uInt64) ; }
					i += x ;
					break ;

		case 'x':	val.m_uInt32 = (uint32_t) va_arg(ap, int) ;
					nLenObj += xnumlen64(val.m_uInt32) ;
					if (nLenMax > nLenObj)
						for (nGap = nLenMax - nLenObj ; nGap > 0 ; nGap--)
							{ AddByte(cPad) ; nCount++ ; }

					for (div.m_uInt32 = 0x10000000 ; div.m_uInt32 > 1 && div.m_uInt32 > val.m_uInt32 ; div.m_uInt32 /= 16) ;

					for (; div.m_uInt32 ; div.m_uInt32 /= 16)
					{
						res = val.m_uInt32/div.m_uInt32 ;
						AddByte(res < 10 ? ('0'+res) : ('a'+(res-10))) ;
						nCount++ ;
						val.m_uInt32 -= (res * div.m_uInt32) ;
					}
					i += x ;
					break ;

		case 'X':
		case 'p':	val.m_uInt64 = (uint64_t) va_arg(ap, uint64_t) ;
					nLenObj += xnumlen64(val.m_uInt64) ;
					if (nLenMax > nLenObj)
						for (nGap = nLenMax - nLenObj ; nGap > 0 ; nGap--)
							{ AddByte(cPad) ; nCount++ ; }

					for (div.m_uInt64 = 0x1000000000000000 ; div.m_uInt64 > 1 && div.m_uInt64 > val.m_uInt64 ; div.m_uInt64 /= 16) ;

					for (; div.m_uInt64 ; div.m_uInt64 /= 16)
					{
						res = val.m_uInt64/div.m_uInt64 ;
						AddByte(res < 10 ? ('0'+res) : ('a'+(res-10))) ;
						nCount++ ;
						val.m_uInt64 -= (res * div.m_uInt64) ;
					}
					i += x ;
					break ;

		default:	AddByte(*i) ;
					nCount++ ;
		}
	}

	return nCount ;
}

uint32_t	hzChain::Append		(const void* vpStr, uint32_t nBytes)
{
	//	Appends the chain with the first nBytes nB (void*) buffer of given size. This operation makes no assumptions about buffer content and so the operation is
	//	not null terminated.
	//
	//	Arguments:	1)	vpStr	The buffer address as void*
	//				2)	nBytes	The number of bytes from the buffer to append to the chain
	//
	//	Returns:	Number of bytes added

	_hzfunc("hzChain::Append(1)") ;

	_zblk*		curBlk ;			//	Working block pointer
	_zblk*		newBlk ;			//	Working block pointer
	const char*	i ;					//	Input iterator
	uint32_t	nCan ;				//	Max number of bytes that can be written to current block
	uint32_t	nWritten = 0 ;		//	Limiter

	if (!nBytes)
		return nBytes ;

	//	If nothing in chain, create the first block
	if (!mx)
		mx = new _chain() ;
	if (!mx->m_Begin)
		mx->m_Begin = mx->m_End = _zblk_alloc() ;

	curBlk = (_zblk*) mx->m_End ;
	if (!curBlk)
		Fatal("%s. Chain %p has no end block\n", *_fn, this) ;	//mx->m_Id) ;

	i = (char*) vpStr ;
	for (; nWritten < nBytes ;)
	{
		if (curBlk->xize == ZBLKSIZE)
		{
			//	Out of space - make new block

			newBlk = _zblk_alloc() ;
			if (!newBlk)
				Fatal("%s. No allocation (case 2)\n", *_fn) ;
			newBlk->prev = (_zblk*) mx->m_End ;
			curBlk->next = newBlk ;
			mx->m_End = newBlk ;
			curBlk = newBlk ;
		}

		nCan = ZBLKSIZE - curBlk->xize ;

		if ((nWritten + nCan) > nBytes)
			nCan = nBytes - nWritten ;

		//	Add bytes to current block
		memcpy(curBlk->m_Data + curBlk->xize, i, nCan) ;
		curBlk->xize += nCan ;
		i += nCan ;
		mx->m_nSize += nCan ;
		nWritten += nCan ;
	}

	return nWritten ;
}

uint32_t	hzChain::AppendSub		(hzChain& Z, uint32_t nStart, uint32_t nBytes)
{
	//	Append this chain with a sub-chain
	//
	//	Arguments:	1)	Z		Sub-chain input
	//				2)	nStart	Start position within sub-chain
	//				3)	nBytes	No of bytes to write from sub-chain	
	//
	//	Returns:	Number of bytes added

	_hzfunc("hzChain::Append(hzChain,Start,noBytes)") ;

	chIter		zi ;					//	For iteration of operand chain
	_zblk*		curBlk ;				//	Working block pointer
	_zblk*		newBlk ;				//	Working block pointer
	uint32_t	nBytesWritten = 0 ;		//	Total bytes written

	if (!Z.Size())
		return 0 ;

	//	If nothing in this chain, create the first block
	if (!mx)
		mx = new _chain() ;
	if (!mx->m_Begin)
		mx->m_Begin = mx->m_End = _zblk_alloc() ;

	curBlk = (_zblk*) mx->m_End ;
	if (!curBlk)
		Fatal("%s. Chain %p has no end block\n", *_fn, this) ;	//mx->m_Id) ;

	for (zi = Z, zi += nStart ; !zi.eof() && nBytesWritten < nBytes ; zi++)
	{
		if (curBlk->xize == ZBLKSIZE)
		{
			//	Out of space - make new block

			newBlk = _zblk_alloc() ;
			if (!newBlk)
				Fatal("%s. No allocation (case 2)\n", *_fn) ;
			newBlk->prev = (_zblk*) mx->m_End ;
			curBlk->next = newBlk ;	//bloc_addr ;
			mx->m_End = newBlk ;	//bloc_addr ;

			curBlk = newBlk ;
		}

		curBlk->m_Data[curBlk->xize] = *zi ;
		curBlk->xize++ ;
		mx->m_nSize++ ;
		nBytesWritten++ ;
	}

	return nBytesWritten ;
}

//	End of hzChain Append functions

hzEcode	hzChain::AddByte	(const char C)
{
	//	Appends chain with a single byte. Takes the single arg as the char to append, returns either E_OK if successful but E_MEMORY
	//	if not. The latter is a fatal condition.
	//
	//	Arguments:	1)	C	Char to add
	//
	//	Returns:	E_OK

	_hzfunc("hzChain::AddByte") ;

	//	Place char C on the end of the chain. Must detect physical end of 
	//	the current block and add new block

	_zblk*	curBlk ;		//	Working block pointer
	_zblk*	newBlk ;		//	Working block pointer

	if (!mx)
		mx = new _chain() ;
	if (!mx->m_Begin)
		mx->m_Begin = mx->m_End = _zblk_alloc() ;

	curBlk = (_zblk*) mx->m_End ;
	if (!curBlk)
		Fatal("%s. Chain %p has no end block\n", *_fn, this) ;	//mx->m_Id) ;

	if (curBlk->xize == ZBLKSIZE)
	{
		//	Out of space - make new block

		newBlk = _zblk_alloc() ;
		if (!newBlk)
			Fatal("%s. No allocation (case 2)\n", *_fn) ;

		newBlk->prev = (_zblk*) mx->m_End ;
		curBlk->next = newBlk ;
		mx->m_End = newBlk ;

		curBlk = newBlk ;
	}

	curBlk->m_Data[curBlk->xize] = C ;
	curBlk->xize++ ;
	mx->m_nSize++ ;

	return E_OK ;
}

uint32_t	hzChain::Printf	(const char* va_alist ...)
{
	//	Write to the chain using the printf method. Uses varargs.
	//
	//	Arguments:	1)	va_alist	The format string
	//
	//	Returns:	Number of bytes written

	_hzfunc("hzChain::Printf") ;

	va_list		ap ;	//	Stack list

	va_start(ap, va_alist) ;

	return _vainto(va_alist, ap) ;
}

hzChain&	hzChain::operator=		(const char* s)
{
	//	Makes this chain equal to the supplied char string operand. Any pre-existing contents are disregarded.
	//
	//	Arguments:	1)	s	The supplied null terminated string
	//
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator=(cstr)") ;

	Clear() ;
	operator+=(s) ;
	return *this ;
}

hzChain&	hzChain::operator=		(const hzString& S)
{
	//	Makes this chain equal to the supplied string operand. Any pre-existing contents are disregarded.
	//
	//	Arguments:	1)	S	The supplied string
	//
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator=(hzString)") ;

	Clear() ;
	operator+=(S) ;
	return *this ;
}

hzChain&	hzChain::operator=		(const hzChain& op)
{
	//	Makes this chain equal to the supplied chain operand. Any pre-existing contents are disregarded.
	//
	//	Arguments:	1)	op	The supplied chain
	//
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator=(hzChain)") ;

	Clear() ;

	if (op.mx)
	{
    	if (_hzGlobal_MT)
        	__sync_add_and_fetch((uint32_t*)&(op.mx->m_copy), 1) ;
    	else
        	op.mx->m_copy++ ;
	}
    mx = op.mx ;

	return *this ;
}

hzChain&	hzChain::operator+=	(const hzChain& op)
{
	//	Append this chain with the supplied chain. This is done as a series of memcpy calls.
	//
	//	Arguments:	1)	supp	The supplied chain
	//
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator+=(hzChain&)") ;

	_zblk*		curBlk ;		//	Block pointer for this chain
	_zblk*		newBlk ;		//	Working block pointer
	_zblk*		srcBlk ;		//	Block pointer for the operand chain
	uint32_t	nCan ;			//	Available space on this chain's current (last) block
	uint32_t	srcOset = 0 ;	//	Offset into source block

	if (!op.mx)
		return *this ;

	//	If nothing in current chain, create the first block
	if (!mx)
		mx = new _chain() ;
	if (!mx->m_End)
		mx->m_Begin = mx->m_End = _zblk_alloc() ;

	srcBlk = (_zblk*) op.mx->m_Begin ;
	curBlk = (_zblk*) mx->m_End ;
	if (!curBlk)
		Fatal("%s. Chain %p has no end block\n", *_fn, this) ;	//, mx->m_Id) ;

	for (;;)
	{
		if (curBlk->xize == ZBLKSIZE)
		{
			newBlk = _zblk_alloc() ;
			if (!newBlk)
				Fatal("%s. No allocation (case 2)\n", *_fn) ;
			newBlk->prev = (_zblk*) mx->m_End ;
			curBlk->next = newBlk ;
			mx->m_End = newBlk ;

			curBlk = newBlk ;
		}

		nCan = ZBLKSIZE - curBlk->xize ;
		if (nCan > (srcBlk->xize - srcOset))
			nCan = (srcBlk->xize - srcOset) ;

		memcpy(curBlk->m_Data + curBlk->xize, srcBlk->m_Data + srcOset, nCan) ;
		curBlk->xize += nCan ;
		srcOset += nCan ;

		if (srcOset >= srcBlk->xize)
		{
			if (!srcBlk->next)
				break ;
			srcBlk = srcBlk->next ;
			srcOset = 0 ;
		}
	}

	mx->m_nSize += op.mx->m_nSize ;

	return *this ;
}

hzChain&	hzChain::operator+=	(const hzString& s)
{
	//	Append chain with supplied string
	//
	//	Arguments:	1)	s	The supplied string
	//
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator+=(hzString&)") ;

	//const char*	i ;
	//uint32_t	len ;

	if (!this)
		hzexit(_fn, E_CORRUPT, "No instance") ;

	//len = s.Length() ;
	//i = *s ;

	if (!s)
		return *this ;

	Append(*s, s.Length()) ;
	return *this ; 
}

hzChain&	hzChain::operator+=	(std::ifstream& is)
{
	//	Append the current chain with the operand stream. The stream is read until no more bytes are available.
	//
	//	Arguments:	1)	s	The supplied stream
	//
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator+=(ifstream&)") ;

	//	Read from a stream directly into the end of the chain

	_zblk*		curBlk ;	//	Block pointer for this chain
	_zblk*		newBlk ;	//	Working block pointer
	uint32_t	nAvail ;	//	Available space within current block
	uint32_t	nRead ;		//	Actual bytes read with stream read call
	char*		buf ;		//	Read buffer - need this to avoid advanced allocation of blocks which could subsequently remain empty

	/*
 	**	If nothing in chain, create the first block
	*/

	if (!mx)
		mx = new _chain() ;
	if (!mx->m_End)
		mx->m_Begin = mx->m_End = _zblk_alloc() ;

	curBlk = (_zblk*) mx->m_End ;
	if (!curBlk)
		Fatal("%s. Chain %p has no end block\n", *_fn, this) ;	//mx->m_Id) ;

	/*
 	**	If the last block is already partly full, use first read to fill it up
	*/

	if (curBlk->xize < ZBLKSIZE)
	{
		nAvail = (ZBLKSIZE - curBlk->xize) ;

		is.read(curBlk->m_Data + curBlk->xize, nAvail) ;

		nRead = is.gcount() ;
		if (!nRead)
			return *this ;

		curBlk->xize += (int16_t) nRead ;
		mx->m_nSize += nRead ;

		if (nRead < nAvail)
			return *this ;
	}

	/*
 	**	Repeatedly read into buffer and then allocate and populate blocks
	*/

	buf = new char[ZBLKSIZE + 4] ;

	for (;;)
	{
		is.read(buf, ZBLKSIZE) ;
		nRead = is.gcount() ;

		if (!nRead)
			break ;

		//	Allocate the block, set the begin, current and end if these are null.
		newBlk = _zblk_alloc() ;
		if (!newBlk)
			Fatal("%s. No allocation (case 2)\n", *_fn) ;

		newBlk->prev = (_zblk*) mx->m_End ;
		curBlk->next = newBlk ;
		mx->m_End = newBlk ;

		curBlk = newBlk ;

		//	Write data into block
		memcpy(curBlk->m_Data, buf, nRead) ;
		curBlk->xize += (int16_t) nRead ;
		mx->m_nSize += nRead ;

		//	If the number of bytes read was less than asked, we are at end of file
		if (nRead < ZBLKSIZE)
			break ;
	}

	delete buf ;
	return *this ;
}

hzChain&	hzChain::operator+=	(const char* s)
{
	//	Append the current chain with the supplied char string operand.
	//
	//	Arguments:	1)	s	The null terminated string to add to the chain
	//
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator+=(char*)") ;

	_zblk*		curBlk ;		//	Block pointer
	_zblk*		newBlk ;		//	Working block pointer
	const char*	i ;				//	Supplied string iterator

	if (!s || !s[0])
		return *this ;

	//	If nothing in chain, create the first block
	if (!mx)
		mx = new _chain() ;
	if (!mx->m_Begin)
		mx->m_Begin = mx->m_End = _zblk_alloc() ;

	curBlk = (_zblk*) mx->m_End ;
	if (!curBlk)
		Fatal("%s. Chain %p has no end block\n", *_fn, this) ;	//, mx->m_Id) ;

	for (i = s ; *i ; i++)
	{
		if (curBlk->xize == ZBLKSIZE)
		{
			//	Out of space - make new block

			newBlk = _zblk_alloc() ;
			if (!newBlk)
				Fatal("%s. No allocation (case 2)\n", *_fn) ;

			newBlk->prev = (_zblk*) mx->m_End ;
			curBlk->next = newBlk ;
			mx->m_End = newBlk ;

			curBlk = newBlk ;
		}

		curBlk->m_Data[curBlk->xize] = *i ;
		curBlk->xize++ ;
		mx->m_nSize++ ;
	}

	return *this ;
}

hzChain&	hzChain::operator<<	(const hzChain& C)
{
	//	Append chain with supplied chain
	//
	//	Arguments:	1)	C	The chain to add to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(hzChain&)") ;

	return operator+=(C) ;
}

hzChain&	hzChain::operator<<	(const hzEmaddr& e)
{
	//	Append chain with supplied Email address
	//
	//	Arguments:	1)	C	The chain to add to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(hzEmaddr&)") ;

	return operator+=(*e) ;
}

hzChain&	hzChain::operator<<	(const hzIpaddr& i)
{
	//	Append chain with supplied IP address
	//
	//	Arguments:	1)	i	The IP address to add to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(hzIpaddr&)") ;

	return operator+=(i.Str()) ;
}

hzChain&	hzChain::operator<<	(const hzString& s)
{
	//	Append chain with supplied string
	//
	//	Arguments:	1)	s	The string to add to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(hzString&)") ;

	return operator+=(s) ;
}

hzChain&	hzChain::operator<<	(const char* s)
{
	//	Append chain with supplied char string
	//
	//	Arguments:	1)	s	The null terminated string to add to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(char*)") ;

	return operator+=(s) ;
}

hzChain&	hzChain::operator<<	(uint32_t nValue)
{
	//	Append the current chain with the text equivelent of the supplied integer operand.
	//
	//	Arguments:	1)	nValue	The value to be appended as text to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(uint32)") ;

	char	cvNum[12] ;		//	Working buffer

	sprintf(cvNum, "%u", nValue) ;
	return operator+=(cvNum) ;
}

hzChain&	hzChain::operator<<	(uint64_t nValue)
{
	//	Append the current chain with the text equivelent of the supplied integer operand.
	//
	//	Arguments:	1)	nValue	The value to be appended as text to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(uint64)") ;

	char	cvNum[12] ;		//	Working buffer

	sprintf(cvNum, "%lu", nValue) ;
	return operator+=(cvNum) ;
}

hzChain&	hzChain::operator<<	(int32_t nValue)
{
	//	Append the current chain with the text equivelent of the supplied integer operand.
	//
	//	Arguments:	1)	nValue	The value to be appended as text to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(int32)") ;

	char	cvNum[12] ;		//	Working buffer

	sprintf(cvNum, "%d", nValue) ;
	return operator+=(cvNum) ;
}

hzChain&	hzChain::operator<<	(int64_t nValue)
{
	//	Append the current chain with the text equivelent of the supplied integer operand.
	//
	//	Arguments:	1)	nValue	The value to be appended as text to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(int64)") ;

	char	cvNum[12] ;		//	Working buffer

	sprintf(cvNum, "%ld", nValue) ;
	return operator+=(cvNum) ;
}

hzChain&	hzChain::operator<<	(double nValue)
{
	//	Append the current chain with the text equivelent of the supplied numeric operand.
	//
	//	Arguments:	1)	nValue	The value to be appended as text to this chain
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(double)") ;

	char	cvNum[24] ;		//	Working buffer

	sprintf(cvNum, "%f", nValue) ;
	return operator+=(cvNum) ;
}

hzChain&	hzChain::operator<<	(std::ifstream& is)
{
	//	Append the current chain with the operand stream. The stream is read until no more bytes are available.
	//
	//	Arguments:	1)	is	The input stream
	//	Returns:	Reference to this chain

	_hzfunc("hzChain::operator<<(stream)") ;

	*this += is ;
	return *this ;
}

//	Stream operator
std::istream&	operator>>	(std::istream& is, hzChain& Z)
{
	//	Category:	Data Input
	//
	//	Append chain by reading from a stream. Note this continues until no more bytes are available from the stream.
	//	
	//	Arguments:	1)	is	The input stream
	//				2)	Z	The chain populated by this operation
	//
	//	Returns:	Reference to the input stream

	_hzfunc("std::istream& operator>> hzChain&") ;

	char	cvBuf	[516] ;	//	Working buffer

	for (;;)
	{
		is.read(cvBuf, 512) ;
		if (!is.gcount())
			break ;
		cvBuf[is.gcount()] = 0 ;
		Z += cvBuf ;
	}

	return is ;
}

std::ostream&	operator<<	(std::ostream& os, const hzChain& Z)
{
	//	Category:	Data Output
	//
	//	Write out (whole) chain content to a stream.
	//	
	//	Arguments:	1)	os	The output stream
	//				2)	Z	The chain to be written out
	//
	//	Returns:	Reference to the input stream

	_hzfunc("std::istream& operator<< hzChain&") ;

	_zblk*		zb ;			//	Working block pointer

	if (!Z.mx)
		return os ;

	for (zb = (_zblk*) Z.mx->m_Begin ; zb ; zb = zb->next)
		os.write(zb->m_Data, zb->xize) ;

	return os ;
}

/*
**	Section 2:	hzChain Iterator functions
*/

hzChain::BlkIter&	hzChain::BlkIter::Advance	(void)
{
	//	Advance the hzChain block iterator by one chain block
	//
	//	Arguments:	None
	//	Returns:	Reference to this chain block iterator

	_hzfunc("hzChain::BlkIter::Advance") ;

	_zblk*	zp ;	//	Block pointer

	zp = (_zblk*) m_block ;
	m_block = zp = zp->next ;
	return *this ;
}

hzChain::BlkIter&	hzChain::BlkIter::Roll	(void)
{
	//	Advance the hzChain block iterator by one chain block
	//
	//	Arguments:	None
	//	Returns:	Reference to this chain block iterator

	_hzfunc("hzChain::BlkIter::Roll") ;

	_zblk*	zp ;	//	Block pointer

	zp = (_zblk*) m_block ;
	m_block = zp = zp->next ;
	return *this ;
}

void*	hzChain::BlkIter::Data	(void)
{
	//	Return the inner data area of the current chain block
	//
	//	Arguments:	None
	//
	//	Returns:	Pointer to the block iterator's current block data space
	//				NULL if the block iterator is at the end of the chain or the chain is empty

	_hzfunc("hzChain::BlkIter::Data") ;

	_zblk*	zp ;	//	Block pointer

	zp = (_zblk*) m_block ;
	if (zp)
		return zp->m_Data ;
	return 0 ;
}

uint32_t	hzChain::BlkIter::Size	(void)
{
	//	Return the number of bytes in the data area of the current chain block
	//
	//	Arguments:	None
	//
	//	Returns:	>0	The number of bytes held in (the usage of) the block iterator's current block data space
	//				0	If the block iterator is at the end of the chain or the chain is empty

	_hzfunc("hzChain::BlkIter::Size") ;

	_zblk*	zp ;	//	Block pointer

	zp = (_zblk*) m_block ;
	if (zp)
		return zp->xize ;
	return 0 ;
}

char	hzChain::Iter::current	(void) const
{
	//	Return the value of the char currently pointed to by the iterator
	//
	//	Arguments:	None
	//
	//	Returns:	>0	The current character of the chain iterator
	//				0	If the chain iterator is at the end of the chan or the chain is empty

	_hzfunc("hzChain::Iter::current") ;

	_zblk*	zp ;	//	Block pointer

	if (!m_block)
		return m_cDefault ;

	zp = (_zblk*) m_block ;
	return zp->m_Data[m_nOset] ;
}

char	hzChain::Iter::operator*	(void) const
{
	//	Return the value of the char currently pointed to by the iterator
	//
	//	Arguments:	None
	//
	//	Returns:	>0	The current character of the chain iterator
	//				0	If the chain iterator is at the end of the chan or the chain is empty

	_hzfunc("hzChain::Iter::operator*") ;

	_zblk*	zp ;	//	Block pointer

	if (!m_block)
		return m_cDefault ;

	zp = (_zblk*) m_block ;
	if (!zp)
		Fatal("%s. Cannot access block %d\n", *_fn, m_block) ;

	if (!zp->next && m_nOset == zp->xize)
		return m_cDefault ;

	if (m_nOset >= zp->xize)
		Fatal("%s. Have block %d next %d oset %d size %d\n", *_fn, zp->next, m_block, m_nOset, zp->xize) ;

	return zp->m_Data[m_nOset] ;
}

bool	hzChain::Iter::eof	(void) const
{
	//  Rteurns true if the iterator is at EOF. Returns false otherwise.
	//
	//  Arguments:	None
	//
	//  Returns:	True	If the iterator is at EOF
	//  			False	If the iterator is still valid

	_hzfunc("hzChain::Iter::eof") ;

	_zblk*	zp ;	//	Block pointer

	if (!m_block)
		return true ;

	zp = (_zblk*) m_block ;
	if (!zp)
		Fatal("%s. Cannot access block %d\n", *_fn, m_block) ;

	if (zp->next)
		return false ;
	return m_nOset < zp->xize ? false : true ;
}

bool	hzChain::Iter::Equal	(const char c) const
{
	//	Determines if the supplied char (arg 1) is the same as the current char in the chain iterator.
	//
	//	- The comparison is case-sensitive.
	//	- Returns true/false
	//
	//	Arguments:	1)	c	The test character
	//
	//	Returns:	True	If the supplied char is equal to that of the current iterator char
	//				False	Otherwise

	_hzfunc("hzChain::Iter::Equal(char)") ;

	_zblk*	zp ;	//	Block pointer

	if (!m_block)
		return false ;

	zp = (_zblk*) m_block ;
	if (!zp)
		Fatal("%s. Cannot access block %d\n", *_fn, m_block) ;

	return zp->m_Data[m_nOset] == c ? true : false ;
}

bool	hzChain::Iter::Equal	(const char* s) const
{
	//	Determine if the supplied char sequence (arg 1) matches that at the current point in the chain iterator, return true if it does, false
	//	otherwise. The comparison is case-sensitive and the function does not alter (advance or retard) the iterator.
	//
	//	Arguments:	1)	s	The test char string
	//
	//	Returns:	True	If the supplied cstr matches that found at the current iterator position
	//				False	Otherwise

	_hzfunc("hzChain::Iter::Equal(char*)") ;
 
	_zblk*		zp ;		//	Chain block pointer
	const char*	i = s ;		//	Test string iterator
	uint32_t	nOset ;		//	Offset within block

	//	Dismiss the case where the iterator is null
	if (!m_block)
		return s && s[0] ? false : true ;

	zp = (_zblk*) m_block ;
	if (!zp)
		Fatal("%s. Cannot access block %d\n", *_fn, m_block) ;

	//	Dismiss the case where the operand is null
	if (!i)
		return zp->m_Data[nOset] ? false : true ;

	//	Test for equality
	for (nOset = m_nOset ; *i ; i++)
	{
		if (!zp)
			return false ;

		if (*i != zp->m_Data[nOset])
			return false ;

		nOset++ ;
		if (nOset >= zp->xize)
		{
			nOset = 0 ;
			zp = zp->next ;
		}
	}

	return true ;
}

bool	hzChain::Iter::Equiv	(const char* s) const
{
	//	Determines if the supplied char sequence (arg 1) is found at the current char in the chain iterator. Note this function does not advance
	//	the iterator. The comparison is case-insensitive.
	//
	//	Arguments:	1)	s	The test char string
	//
	//	Returns:	True	If the supplied cstr matches that found at the current iterator position
	//				False	Otherwise

	_hzfunc("hzChain::Iter::Equiv") ;
 
	_zblk*		zp ;		//	Chain block pointer
	const char*	i = s ;		//	Test string iterator
	uint32_t	nOset ;		//	Offset within block

	//	Dismiss the case where the iterator is null
	if (!m_block)
		return s && s[0] ? false : true ;

	zp = (_zblk*) m_block ;
	if (!zp)
		Fatal("%s. Cannot access block %d\n", *_fn, m_block) ;

	//	Dismiss the case where the operand is null
	if (!i)
		return zp->m_Data[nOset] ? false : true ;

	for (nOset = m_nOset ; *i ; i++)
	{
		if (!zp)
			return false ;

		if (_tolower(*i) != _tolower(zp->m_Data[nOset]))
			return false ;

		nOset++ ;
		if (nOset >= zp->xize)
		{
			nOset = 0 ;
			zp = zp->next ;
		}
	}

	return true ;
}

uint32_t	hzChain::Iter::Advance	(uint32_t nInc)
{
	//	Increments the current chain iterator by the requested length. Will set the iterator to the end of the chain if the requested
	//	increment is too great.
	//
	//	Arguments:	1)	nInc	Number of bytes to advance
	//
	//	Returns:	Number of bytes advanced

	_hzfunc("hzChain::Iter::Advance") ;

	_zblk*		zp ;			//	Chain block pointer
	uint32_t	nCan ;			//	Advance possibible within current block
	uint32_t	nAdv = 0 ;		//	Number of chars actully advanced

	if (nInc < 0)
		return 0 ;

	zp = (_zblk*) m_block ;
	if (!zp)
		return 0 ;

	for (; nAdv < nInc ;)
	{
		nCan = zp->xize - m_nOset ;
		
		if ((nCan + nAdv) > nInc)
			nCan = nInc - nAdv ;

		m_nOset += nCan ;
		nAdv += nCan ;

		if (m_nOset == zp->xize)
		{
			if (!zp->next)
				break ;
			m_nOset = 0 ;
			m_block = zp = zp->next ;
		}
	}

	return nAdv ;
}

hzChain::Iter&	hzChain::Iter::operator++	(void)
{
	//	Increments the current chain iterator if it can be incremented ie is not at the end of the chain. Note that the void argument means this
	//	is the 'post evaluation version' called when the code is 'iter++' rather than '++iter'.
	//
	//	Arguments:	None
	//	Returns:	Reference to this chain iterator

	_hzfunc("hzChain::Iter::operator++") ;

	_zblk*	zp ;	//	Chain block pointer

	if (m_block)
	{
		zp = (_zblk*) m_block ;

		if (zp->m_Data[m_nOset] == CHAR_NL)
			{ m_nCol = 0 ; m_nLine++ ; }

		if (zp->m_Data[m_nOset] == CHAR_TAB)
			m_nCol += (4-(m_nCol%4)) ;
		else
			m_nCol++ ;

		if (m_nOset < zp->xize)
			m_nOset++ ;

		if (m_nOset >= zp->xize)
		{
			if (zp->next)
			{
				m_block = zp->next ;
				m_nOset = 0 ;
			}
		}
	}

	return *this ;
}

hzChain::Iter&	hzChain::Iter::operator++	(int)
{
	//	Increments the current chain iterator if it can be incremented ie is not at the end of the chain. Note that the void argument means this
	//	is the 'pre evaluation version' called when the code is '++iter' rather than 'iter++'.
	//
	//	Arguments:	Nominal int argument. No actual argument.
	//	Returns:	Reference to this chain iterator

	_hzfunc("hzChain::Iter::operator++") ;

	_zblk*	zp ;	//	Chain block pointer

	if (m_block)
	{
		zp = (_zblk*) m_block ;

		if (zp->m_Data[m_nOset] == CHAR_NL)
			{ m_nCol = 0 ; m_nLine++ ; }

		if (zp->m_Data[m_nOset] == CHAR_TAB)
			m_nCol += (4-(m_nCol%4)) ;
		else
			m_nCol++ ;

		if (m_nOset < zp->xize)
			m_nOset++ ;

		if (m_nOset >= zp->xize)
		{
			if (zp->next)
			{
				m_block = zp->next ;
				m_nOset = 0 ;
			}
		}
	}

	return *this ;
}

hzChain::Iter&	hzChain::Iter::operator--	(void)
{
	//	Decrements the current chain iterator if it can be incremented (is not at the end of the chain)
	//
	//	Arguments:	None
	//	Returns:	Reference to this chain iterator

	_hzfunc("hzChain::Iter::operator--") ;

	_zblk*	zp ;	//	Chain block pointer

	if (m_block)
	{
		zp = (_zblk*) m_block ;

		m_nOset-- ;
		if (m_nOset < 0)
		{
			m_nOset = 0 ;
			if (zp->prev)
			{
				zp = zp->prev ;
				if (zp)
					m_nOset = zp->xize - 1 ;
			}
		}
	}

	return *this ;
}

hzChain::Iter&	hzChain::Iter::operator--	(int)
{
	//	Decrements the current chain iterator if it can be incremented (is not at the end of the chain)
	//
	//	Arguments:	Nominal int argument. No actual argument.
	//	Returns:	Reference to this chain iterator

	_hzfunc("hzChain::Iter::operator--(int)") ;

	_zblk*	zp ;	//	Chain block pointer

	if (m_block)
	{
		zp = (_zblk*) m_block ;

		m_nOset-- ;
		if (m_nOset < 0)
		{
			m_nOset = 0 ;
			if (zp->prev)
			{
				zp = zp->prev ;
				if (zp)
					m_nOset = zp->xize - 1 ;
			}
		}
	}

	return *this ;
}

hzChain::Iter&	hzChain::Iter::operator+=	(uint32_t nInc)
{
	//	Increments the current chain iterator by the requested length. Will set the iterator to the end of the chain if the requested increment is too great.
	//
	//	Arguments:	1)	nInc	The number of bytes to increment the chain iterator by
	//
	//	Returns:	Reference to this chain iterator

	_hzfunc("hzChain::Iter::operator+=") ;

	_zblk*	zp ;	//	Chain block pointer

	zp = (_zblk*) m_block ;

	for (; zp && nInc > 0 ;)
	{
		if (zp->m_Data[m_nOset] == CHAR_NL)
			{ m_nCol = 0 ; m_nLine++ ; }

		if (zp->m_Data[m_nOset] == CHAR_TAB)
			m_nCol += (4-(m_nCol%4)) ;
		else
			m_nCol++ ;

		if (m_nOset < zp->xize)
			m_nOset++ ;

		if (m_nOset >= zp->xize)
		{
			if (zp->next)
			{
				m_nOset = 0 ;
				m_block = zp = zp->next ;
			}
		}
		nInc-- ;
	}

	return *this ;
}

hzChain::Iter&	hzChain::Iter::operator-=	(uint32_t nDec)
{
	//	Retards the current chain iterator by the requested length. Will set the iterator to the start of the chain if the requested decrement is too great.
	//
	//	Arguments:	1)	nInc	The number of bytes to decrement the chain iterator by
	//
	//	Returns:	Reference to this chain iterator

	_hzfunc("hzChain::Iter::operator-=") ;

	_zblk*	zp ;	//	Chain block pointer

	zp = (_zblk*) m_block ;

	for (; zp && nDec ;)
	{
		if (m_nOset >= nDec)
		{
			//	If we can decrement N places and still be on the same block, then just decrement the offset
			m_nOset -= nDec ;
			break ;
		}

		nDec -= m_nOset ;

		if (!zp->prev)
		{
			m_nOset = 0 ;
			break ;
		}

		m_block = zp = zp->prev ;

		if (zp)
			m_nOset = (zp->xize - 1) ;
		else
			m_nOset = 0 ;
	}

	return *this ;
}

char	hzChain::Iter::operator[]	(uint32_t nOset) const
{
	//	Returns the value of the character pointed to by the iterator plus the supplied offset. The offsets should preferably be small for reasons of efficiency
	//	as the method winds through the requested number of places each time. The envisiaged use is in tokenization or encoding where the percent sign followed
	//	by a percent sign is a percent but a percent sign followed by two hexidecimal characters is an 8-bit ASCII character. It is a convient means of a look
	//	ahead only.
	//
	//	Arguments:	1)	nOset	Look ahead offset
	//
	//	Returns:	>0	The character at the relative position
	//				0	If the current position plus the offset exeeds the chain length.

	_hzfunc("hzChain::Iter::operator[]") ;

	hzChain::Iter	ci ;	//	Chain iterator

	ci = *this ;
	ci += nOset ;
	return *ci ;
}

hzChain::Iter&	hzChain::Iter::Skipwhite	(void)
{
	//	Advance interator to next non-whitespace char. Unless the iterator is currently pointing at a whitespace char, it does nothing.
	//
	//	Arguments:	None
	//	Returns:	Reference to this iterator instance

	_hzfunc("hzChain::Iter::Skipwhite") ;

	_zblk*	zp ;	//	Chain block pointer

	zp = (_zblk*) m_block ;

	if (zp && m_nOset < zp->xize)
	{
		for (; zp && IsWhite(zp->m_Data[m_nOset]) ;)
		{
			if (zp->m_Data[m_nOset] == CHAR_NL)
				{ m_nCol = 0 ; m_nLine++ ; }

			if (zp->m_Data[m_nOset] == CHAR_TAB)
				m_nCol += (4-(m_nCol%4)) ;
			else
				m_nCol++ ;

			m_nOset++ ;
			if (m_nOset >= zp->xize)
			{
				if (zp->next)
				{
					m_block = zp->next ;
					m_nOset = 0 ;
					zp = zp->next ;
				}
			}
		}
	}

	return *this ;
}

uint32_t	hzChain::Iter::Write	(void* pBuf, uint32_t maxBytes)
{
	//	Write out to the supplied buffer, from the current position, upto maxBytes. Do not increment the iterator.
	//
	//	Arguments:	1)	pBuf		The output buffer
	//				2)	maxBytes	The maximum number to write (size of buffer)
	//
	//	Returns:	Number of bytes written to the buffer

	_hzfunc("hzChain::Iter::Write") ;

	_zblk*		zp ;				//	Working block pointer
	char*		i ;					//	Input iterator
	uint32_t	nOset ;				//	Current offset
	uint32_t	nAvail ;			//	Max number of bytes that can be written from current block
	uint32_t	nWritten = 0 ;		//	Limiter

	if (maxBytes < 0)
		return -1 ;

	zp = (_zblk*) m_block ;
	if (!zp)
		return -1 ;
	nOset = m_nOset ;

	i = (char*) pBuf ;
	for (; nWritten < maxBytes ;)
	{
		if (nOset == zp->xize)
		{
			//	At end of current block, move to next
			zp = zp->next ;
			if (!zp)
				break ;
			nOset = 0 ;
		}

		nAvail = zp->xize - nOset ;

		if ((nWritten + nAvail) > maxBytes)
			nAvail = maxBytes - nWritten ;

		//	Add bytes to current block
		memcpy(i, zp->m_Data + nOset, nAvail) ;
		nOset += nAvail ;
		i += nAvail ;
		nWritten += nAvail ;
	}

	return nWritten ;
}
