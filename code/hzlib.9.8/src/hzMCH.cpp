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

#include "hzCtmpls.h"
#include "hzMCH.h"

using namespace std ;

/*
**	Definitions
*/

#define	HZ_MCH_SIZE_A	68
#define	HZ_MCH_SIZE_B	48
#define	HZ_MCH_SIZE_C	32
#define	HZ_MCH_SIZE_D	16

struct	_mch_blk_A
{
	//	Large block with next pointer
	uint32_t	next ;			//	Next block
	char		m_Data[68] ;	//	data area
} ;

struct	_mch_blk_B
{
	//	Small terminator block.
	char	m_Data[48] ;
} ;

struct	_mch_blk_C
{
	//	Small terminator block.
	char	m_Data[32] ;
} ;

struct	_mch_blk_D
{
	//	Small terminator block.
	char	m_Data[16] ;
} ;

//	SUPERBLOCKS

struct	_mch_sblk_A	{ _mch_blk_A	m_blocks[1024] ; } ;
struct	_mch_sblk_B	{ _mch_blk_B	m_blocks[1024] ; } ;
struct	_mch_sblk_C	{ _mch_blk_C	m_blocks[1024] ; } ;
struct	_mch_sblk_D	{ _mch_blk_D	m_blocks[1024] ; } ;

static	hzArray<_mch_sblk_A*>	s_super_A ;
static	hzArray<_mch_sblk_B*>	s_super_B ;
static	hzArray<_mch_sblk_C*>	s_super_C ;
static	hzArray<_mch_sblk_D*>	s_super_D ;

static	uint32_t	s_free_A ;
static	uint32_t	s_free_B ;
static	uint32_t	s_free_C ;
static	uint32_t	s_free_D ;

static	hzLockS		s_free_lock_A ;
static	hzLockS		s_free_lock_B ;
static	hzLockS		s_free_lock_C ;
static	hzLockS		s_free_lock_D ;

/*
**	Memory management regime for chains
*/

static	hzLockRWD	s_mch_mutex("hzMCH mutex") ;	//	Protects the memory allocation

/*
**	Section 1A:	hzChain private functions
*/

static	void*	_mch_xlate	(uint32_t addr)
{
	//	Translates micro-chain address to data area within the located block.
	//
	//	Arguments:	addr	The micro-chain address
	//
	//	Returns:	Pointer to a micro chain block if the supplied address is valid
	//				NULL if the supplied address is 0
	//
	//	Note that a string number of 0 is invalid as the minimum usage is a block is 2 (sizeof the usage counter itself). Note also that the string returned is
	//	not byte aligned. This contrasts with hzString where the actual char* value is always aligned along 8 byte boundaries.

	_hzfunc("__func__") ;

	char*		ptr ;		//	Super block pointer
	uint32_t	blkNo ;		//	Block number
	uint32_t	nOset ;		//	Block offset

	if (!addr)
		return 0 ;

	blkNo = (addr & 0x3fffC000) >> 10 ;
	nOset = addr & 0x000003ff ;

	switch	((addr & 0xC0000000) >> 30)
	{
	case 0:	ptr = (char*) s_super_A[blkNo] ;	ptr += (nOset * 72) ;	break ;
	case 1:	ptr = (char*) s_super_B[blkNo] ;	ptr += (nOset * 48) ;	break ;
	case 2:	ptr = (char*) s_super_C[blkNo] ;	ptr += (nOset * 32) ;	break ;
	case 3:	ptr = (char*) s_super_D[blkNo] ;	ptr += (nOset * 16) ;	break ;
	}

	return (void*) ptr ;
}

static	uint32_t	_alloc_blkA	(void)
{
	_mch_sblk_A*	pS ;		//	Pointer to size A block
	_mch_blk_A*		pA ;		//	Pointer to size A block
	uint32_t*		pNext ;		//	Pointer to block data, case as a pointer to unit32_t for use as next pointer
	uint32_t		addr ;		//	Address of block allocated
	uint32_t		lim ;		//	Address of block allocated

	s_free_lock_A.Lock() ;

		if (!s_free_A)
		{
			//	Allocate a superblock of 1024 A blocks
			pS = new _mch_sblk_A() ;
			addr = s_super_A.Count() * 1024 ;
			s_super_A.Add(pS) ;

			for (lim = 1 ; lim < 1024 ; lim++)
			{
				pA = pS->m_blocks ;
				pA->next = addr + lim ;
			}
			pA->next = 0 ;

			s_free_A = addr ;
		}

		//	Then allocate from the free list
		addr = s_free_A ;
		pNext = (uint32_t*) _mch_xlate(addr) ;
		s_free_A = *pNext ;

	s_free_lock_A.Unlock() ;

	return addr ;
}

static	uint32_t	_alloc_blkB	(void)
{
	_mch_sblk_B*	pS ;		//	Pointer to size A block
	_mch_blk_B*		pB ;		//	Pointer to size A block
	uint32_t*		pNext ;		//	Pointer to block data, case as a pointer to unit32_t for use as next pointer
	uint32_t		addr ;		//	Address of block allocated
	uint32_t		lim ;		//	Address of block allocated

	s_free_lock_B.Lock() ;

		if (!s_free_B)
		{
			//	Allocate a superblock of 1024 A blocks
			pS = new _mch_sblk_B() ;
			addr = s_super_B.Count() * 1024 ;
			s_super_B.Add(pS) ;

			for (lim = 1 ; lim < 1024 ; lim++)
			{
				pB = pS->m_blocks ;
				pB->m_Data[0] = addr + lim ;
			}
			pB->m_Data[0] = 0 ;

			s_free_B = addr ;
		}

		//	Then allocate from the free list
		addr = s_free_B ;
		pNext = (uint32_t*) _mch_xlate(addr) ;
		s_free_B = *pNext ;

	s_free_lock_B.Unlock() ;

	return addr ;
}

static	uint32_t	_alloc_blkC	(void)
{
	_mch_sblk_C*	pS ;		//	Pointer to size A block
	_mch_blk_C*		pC ;		//	Pointer to size A block
	uint32_t*		pNext ;		//	Pointer to block data, case as a pointer to unit32_t for use as next pointer
	uint32_t		addr ;		//	Address of block allocated
	uint32_t		lim ;		//	Address of block allocated

	s_free_lock_C.Lock() ;

		if (!s_free_C)
		{
			//	Allocate a superblock of 1024 A blocks
			pS = new _mch_sblk_C() ;
			addr = s_super_C.Count() * 1024 ;
			s_super_C.Add(pS) ;

			for (lim = 1 ; lim < 1024 ; lim++)
			{
				pC = pS->m_blocks ;
				pC->m_Data[0] = addr + lim ;
			}
			pC->m_Data[0] = 0 ;

			s_free_C = addr ;
		}

		//	Then allocate from the free list
		addr = s_free_C ;
		pNext = (uint32_t*) _mch_xlate(addr) ;
		s_free_C = *pNext ;

	s_free_lock_C.Unlock() ;

	return addr ;
}

static	uint32_t	_alloc_blkD	(void)
{
	_mch_sblk_D*	pS ;		//	Pointer to size A block
	_mch_blk_D*		pD ;		//	Pointer to size A block
	uint32_t*		pNext ;		//	Pointer to block data, case as a pointer to unit32_t for use as next pointer
	uint32_t		addr ;		//	Address of block allocated
	uint32_t		lim ;		//	Address of block allocated

	s_free_lock_D.Lock() ;

		if (!s_free_D)
		{
			//	Allocate a superblock of 1024 A blocks
			pS = new _mch_sblk_D() ;
			addr = s_super_D.Count() * 1024 ;
			s_super_D.Add(pS) ;

			for (lim = 1 ; lim < 1024 ; lim++)
			{
				pD = pS->m_blocks ;
				pD->m_Data[0] = addr + lim ;
			}
			pD->m_Data[0] = 0 ;

			s_free_D = addr ;
		}

		//	Then allocate from the free list
		addr = s_free_D ;
		pNext = (uint32_t*) _mch_xlate(addr) ;
		s_free_D = *pNext ;

	s_free_lock_D.Unlock() ;

	return addr ;
}

static	void	_mch_free	(uint32_t addr)
{
	//	This takes the current address which does not necessarily mean the start of the series in the chain, then runs to the end. Clear() will simply call this
	//	from the starting address. Write may call it further in. Either way always run to the end.

	_mch_blk_A*	pBlk ;		//	Large block pointer
	uint32_t*	pAddr ;		//	Address of block we are to free
	uint32_t*	pFree ;		//	Pointer to free list address
	uint32_t	curr ;		//	Starting point
	uint32_t	type ;		//	Block type

	if (!addr)
		return ;
	curr = addr ;

	//	Run to end of large blocks
	for (;;)
	{
		type = (curr & 0xC0000000) >> 30 ;

		if (type)
			break ;

		pBlk = (_mch_blk_A*) _mch_xlate(curr) ;
		curr = pBlk->next ;
	}

	//	Now delete the small block if there is one
	pAddr = (uint32_t*) _mch_xlate(curr) ;

	if (type == 1)	{ pFree = (uint32_t*) s_free_B ;	*pFree = curr ;	*pAddr = s_free_B ; }
	if (type == 2)	{ pFree = (uint32_t*) s_free_C ;	*pFree = curr ;	*pAddr = s_free_C ; }
	if (type == 3)	{ pFree = (uint32_t*) s_free_D ;	*pFree = curr ;	*pAddr = s_free_D ; }

	//	Now free the large block
}

#if 0
uint32_t	_allocate	(uint32_t oldSize, uint32_t newSize)
{
	//	Allocate a chain block. Either draw from a list of free blocks or create a new block
	//
	//	Arguments:	1)	id	The chain id	
	//
	//	Returns:	Pointer to a usable z-block

	_hzfunc("hzMCH::_allocate") ;

	_mch_blk_A*	pA ;	//	Pointer to size A block
	_mch_blk_A*	pB ;	//	Pointer to size A block
	_mch_blk_A*	pC ;	//	Pointer to size A block
	_mch_blk_A*	pD ;	//	Pointer to size A block

	if (oldSize == newSize)
		return 0 ;

	s_mch_mutex.LockWrite() ;

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
	s_mch_mutex.Unlock() ;

	if (!zp)
		Fatal("%s. No chain block allocated\n", *_fn) ;

	//	Clear the free block's values to ready it for use
	zp->clear() ;
	//zp->m_Id = id ;

	return zp ;
}
#endif

/*
**	Section 2:	hzMCH public functions
*/

hzMCH::hzMCH	(void)
{
	//	Construct an empty hzChain instance. Increment the global count of currently allocated hzChain instances for memory use reporting purposes.

	_hzGlobal_Memstats.m_numMCH++ ;
	m_Addr = 0 ;
}

hzMCH::~hzMCH	(void)
{
	//	Delete this hzChain instance. Decrement the global count of currently allocated hzChain instances for memory use reporting purposes.

	Clear() ;
	_hzGlobal_Memstats.m_numMCH-- ;
}

void	hzMCH::Clear	(void)
{
	//	Clears all content held by the hzMCH.
	//
	//	If the micro chain address is that of a small block, the block will be the only block. In this case, clearing the content is only a matter of returning
	//	the block to the applicable free list. Where the address is that of a large block, there could be many such blocks and a trainling small block. All the
	//	blocks have to be iterated in order to ensure the trailing block is freed properly.
	//
	//	This can be avoided if it is known there are no trailing blocks. This would be easy to deduce froom the total size. If, after dividing the content size
	//	by 68, there either was no remainder or the remainder exceeded 48, there would be no trailing small block. The problem is the size is NOT available. It
	//	is not written in as the first part of the comtent. Therefor the ...
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzMCH::Clear") ;

	if (!m_Addr)
		return ;

	_mch_free(m_Addr) ;
}

/*
**	Section 2:	hzChain Iterator functions
*/

void	hzMCH::Write	(const hzChain& Z)
{
	//	Commit the supplied content to the micro chain.
	//
	//	If the curent address is zero, this is a matter of allocating as the input is iterated.
	//
	//	Write out to the supplied buffer, from the current position, upto maxBytes. Do not increment the iterator.
	//
	//	Arguments:	1)	pBuf		The output buffer
	//				2)	maxBytes	The maximum number to write (size of buffer)
	//
	//	Returns:	Number of bytes written to the buffer

	_hzfunc("hzMCH::Write") ;

	hzChain::Iter	zi ;	//	Chain iterator
	_mch_blk_A*		pLB ;	//	Working block pointer

	char*		i ;			//	Input iterator
	uint32_t	newAddr ;	//	Address of new block
	uint32_t	toGo ;		//	Bytes still to be written
	uint32_t	nCount ;	//	Count of bytes written to this block

	zi = Z ;
	pLB = 0 ;

	if (!m_Addr)
	{
		//	Iterate chain, allocating as required
		toGo = 4 + Z.Size() ;
		for (; toGo > 48 ;)
		{
			//	Alloc large block
			newAddr = _alloc_blkA() ;
			if (pLB)
				pLB->next = newAddr ;
			pLB = (_mch_blk_A*) _mch_xlate(newAddr) ;
			pLB->m_Data[0] = Z.Size() ;
			i = (char*) pLB ;

			//	Copy accross upto 64 bytes
			nCount = toGo > 64 ? 64 : toGo ;
			toGo -= nCount ;

			for (i += 8 ; nCount ; nCount--)
				*i++ = *zi++ ;
		}

		//	Now allocate tail
		if		(toGo > 32)	newAddr = _alloc_blkB() ;
		else if (toGo > 16)	newAddr = _alloc_blkC() ;
		else
			newAddr = _alloc_blkD() ;

		if (pLB)
			pLB->next = newAddr ;

		//	Write bytes to tail
		nCount = toGo ;
		i = (char*) _mch_xlate(newAddr) ;

		for (; nCount ; nCount--)
			*i++ = *zi++ ;
	}
}
