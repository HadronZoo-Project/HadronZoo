//
//	File:	hzMemory.cpp
//
//	Desc:	1)	Optional overide of general case new and delete operators (when hzOveride.h is included in application's main .cpp file)
//			2)	Provision of memory use diagnostics
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

#include <sys/stat.h>
#include <malloc.h>

#include "hzBasedefs.h"
#include "hzDatabase.h"
#include "hzDate.h"
#include "hzDocument.h"
#include "hzUnixacc.h"

using namespace std ;

/*
**	Global Variables
*/

global	hzMeminfo	_hzGlobal_Memstats ;		//	Memory statistics

/*
**	SECTION 1:	Overide of global new and delete operator
*/

class _hz_heap
{
	struct	_rblk
	{
		//	RAM block.
		//
		//	The global new/delete overload regine allocates small objects (between 8 and 128 bytes) from one of 16 separate free list, each of which deals only
		//	with objects of a specific size. There is a free list for 8 byte objects, another for 16 byte objects and so on. When any of these free lists become
		//	empty and new RAM block is allocated from main memory to top them up.
		//
		//	The RAM blocks are 8 bytes smaller than 64K. There is no compelling reason for this odd choice of size, except that 8 bytes tends to be the overhead
		//	associated with memory allocations with system calls such as malloc. So the idea was that each block would consume exactly 64K of real memory. Given
		//	the RAM blocks are always the same size, RAM blocks assigned to the free list of 8 byte objects will contain twice the number of 'object slots' that
		//	RAM blocks assigned to the free list of 16 byte objects will have. The larger the fixed sized objects, the fewer each RAM block will hold. For some
		//	sizes, a small proportion of the RAM block is wasted. Upon allocation of a RAM block, as many slots that will fit for the intended size, are set up
		//	within the block so that one slot points to the next with the last one pointing to NULL. Then the whole lot is added to the free list for the object
		//	size.
		//
		//	The reason for allocating small objects in this way, is to eliminate the aforementioned 8 byte per allocation overhead. This module is a recent and
		//	experimental addition to the HadronZoo Class Library. Long before a global new/delete overload was attempted, a similar regime was introduced in the
		//	hzString module. Ultimately the global new/delete overload may replace that in the hzString module, however this is not a priority. Before this will
		//	come an overview of how memory is used within real life applications. Considerable evolution is anticipated!

		int32_t		m_size ;			//	If block is dedicated show the size, otherwise this will be 0
		int32_t		m_usage ;			//	If the block is not dedicated, this will be the next object address to allocate
		uint64_t	m_data[1200] ;		//	Data area
	} ;

	//	Mutexs
	hzLockRWD	m008 ;				//	Mutex for 8 byte allocations
	hzLockRWD	m016 ;				//	Mutex for 16 byte allocations
	hzLockRWD	m024 ;				//	Mutex for 24 byte allocations
	hzLockRWD	m032 ;				//	Mutex for 32 byte allocations
	hzLockRWD	m040 ;				//	Mutex for 40 byte allocations
	hzLockRWD	mAll ;				//	Mutex for all the above
	hzLockRWD	mSze ;				//	Mutex for oversized allocations

	//	Freelist pointers (last 3 digits denotes size)
	uint64_t*	fL008 ;				//	Freelist pointer for 8 byte objects
	uint64_t*	fL016 ;				//	Freelist pointer for 16 byte objects
	uint64_t*	fL024 ;				//	Freelist pointer for 24 byte objects
	uint64_t*	fL032 ;				//	Freelist pointer for 32 byte objects
	uint64_t*	fL040 ;				//	Freelist pointer for 40 byte objects

	_rblk*	_mkblk	(int32_t size) ;
public:
	void*	m_allBlocks[8192] ;		//	Pointers to all managed blocks (max of 8192 blocks)

	//	Number of used objects
	uint32_t	nU008 ;				//	In use counter for 8 byte objects
	uint32_t	nU016 ;				//	In use counter for 16 byte objects
	uint32_t	nU024 ;				//	In use counter for 24 byte objects
	uint32_t	nU032 ;				//	In use counter for 32 byte objects
	uint32_t	nU040 ;				//	In use counter for 40 byte objects

	//	Number of free objects
	uint32_t	nF008 ;				//	Free counter for 8 byte objects
	uint32_t	nF016 ;				//	Free counter for 16 byte objects
	uint32_t	nF024 ;				//	Free counter for 24 byte objects
	uint32_t	nF032 ;				//	Free counter for 32 byte objects
	uint32_t	nF040 ;				//	Free counter for 40 byte objects

	uint32_t	nCallsMalloc ;		//	Number of objects over 128 bytes
	uint32_t	nCallsFree ;		//	Number of objects over 128 bytes
	uint32_t	nOver ;				//	Number of objects over 128 bytes
	uint32_t	nOverSize ;			//	Number of bytes in objects over 128 bytes
	uint32_t	numAllBlks ;		//	Number of blocks

	uint32_t	m_allSizes[1001] ;	//	Numbers alocated for each object size.
	uint32_t	m_allSizeA[1001] ;	//	Number of alocations for each object size.
	uint32_t	m_allSizeF[1001] ;	//	Number of deletions for each object size.

	_hz_heap	(void)
	{
		m008.Setname("Mem_fl_008") ;
		m016.Setname("Mem_fl_016") ;
		m024.Setname("Mem_fl_024") ;
		m032.Setname("Mem_fl_032") ;
		m040.Setname("Mem_fl_040") ;

		fL008=0; fL016=0; fL024=0; fL032=0; fL040=0;

		nU008 = nU016 = nU024 = nU032 = nU040 = 0 ;
		nF008 = nF016 = nF024 = nF032 = nF040 = 0 ;

		nCallsMalloc = nCallsFree = nOver = nOverSize = numAllBlks = 0 ;
	}

	~_hz_heap	(void)
	{
		shutdown() ;
	}

	void*	allocate	(uint32_t size) ;
	void	release		(void* ptr) ;
	void	shutdown	(void) ;
};

/*
**	Variables for memory regime
*/

static	_hz_heap*	s_Heap = 0 ;		//	The global heap
static	hzChain		s_memReport ;		//	Chain for memory report compilation
static	uint32_t	s_nBytesInit ;		//	Total bytes allocated under new override
static	uint32_t	s_nBytesExit ;		//	Total bytes reserved under new override

//	The actual working functions are
static	void*	_regime_alloc_Init	(uint32_t size) ;		//	Only called ONCE with the first thing allocated in the program anywhere
static	void*	_regime_alloc_Syst	(uint32_t size) ;		//	Allocate using malloc
static	void*	_regime_alloc_Ovrd	(uint32_t size) ;		//	Allocate using hzHeap
static	void	_regime_free_Syst	(void* mem) ;			//	This frees objects
static	void	_regime_free_Ovrd	(void* mem) ;			//	This frees objects
static	void*	_regime_alloc_Exit	(uint32_t size) ;		//	Allocates memory during shutdown of the memory manager

//	Note that hz_mem_allocate() and hz_mem_release() just called according to function pointers
void*	(*fnptr_Alloc)	(uint32_t) = _regime_alloc_Init ;	//	Allocator for new/delete override
void	(*fnptr_Free)	(void*) = _regime_free_Syst ;		//	De-allocator for new/delete override

/*
**	Heap functions
*/

_hz_heap::_rblk*	_hz_heap::_mkblk	(int32_t size)
{
	_rblk*		pBlk ;		//	Block pointer
	uint64_t*	ptr ;		//	Treat internal data as 8 byte ints
	int32_t		n ;			//	Data segment iterator
	int32_t		max ;		//	Max number of data segments

	//	Allocate the block
	pBlk= (_rblk*) malloc(sizeof(_rblk)) ;

	nCallsMalloc++;
	nOverSize += malloc_usable_size(pBlk) ;

	m_allBlocks[numAllBlks] = pBlk ;
	numAllBlks++ ;
	memset(pBlk, 0, sizeof(_rblk)) ;

	//	Partition the block
	switch	(size)
	{
	case 8:		max = 1200 ; nF008 += max ; break ;
	case 16:	max =  600 ; nF016 += max ; break ;
	case 24:	max =  400 ; nF024 += max ; break ;
	case 32:	max =  300 ; nF032 += max ; break ;
	case 40:	max =  240 ; nF040 += max ; break ;
	}

	size /= 8 ;

	ptr = pBlk->m_data ;
	ptr += size ;

	for (n = 0, max-- ; max ; max--, n += size)
	{
		pBlk->m_data[n] = (uint64_t) ptr ;
		ptr += size ;
	}
	pBlk->m_size = size * 8 ;

	return pBlk ;
}

void*	_hz_heap::allocate	(uint32_t size)
{
	//	Allocate memory from the memory regime if required size is 64 bytes or less and from the heap otherwise. Under the regime, allocation is
	//	always from a freelist of segments of 8,16,24, ... 128 bytes. If the appropriate freelist is empty, a new block is allocated and divided
	//	into multiple segments. Allocations of greater than 128 bytes are placed in a map for oversized objects.
	//
	//	Arguments:	1)	size	Number of bytes to allocate
	//
	//	Returns:	Pointer to the allocated memory

	_rblk*		pBlk = 0;	//	Fixed block allocated if any
	void*		pObj = 0;	//	Object allocated
	uint64_t*	ptr;		//	Intermeadiate pointer for free list navigation
	int32_t		nDiv;		//	Position devider

	nCallsMalloc++;
	nDiv = size%8;
		
	if (nDiv)
		size += (8-nDiv);

	switch	(size)
	{
	case 8:		m008.LockWrite(); if (!fL008) {pBlk=_mkblk(8);    fL008=pBlk->m_data;} nF008-- ; nU008++; ptr=fL008; fL008=(uint64_t*)*ptr; m008.Unlock(); break; 
	case 16:	m016.LockWrite(); if (!fL016) {pBlk=_mkblk(16);   fL016=pBlk->m_data;} nF016-- ; nU016++; ptr=fL016; fL016=(uint64_t*)*ptr; m016.Unlock(); break; 
	case 24:	m024.LockWrite(); if (!fL024) {pBlk=_mkblk(24);   fL024=pBlk->m_data;} nF024-- ; nU024++; ptr=fL024; fL024=(uint64_t*)*ptr; m024.Unlock(); break; 
	case 32:	m032.LockWrite(); if (!fL032) {pBlk=_mkblk(32);   fL032=pBlk->m_data;} nF032-- ; nU032++; ptr=fL032; fL032=(uint64_t*)*ptr; m032.Unlock(); break; 
	case 40:	m040.LockWrite(); if (!fL040) {pBlk=_mkblk(40);   fL040=pBlk->m_data;} nF040-- ; nU040++; ptr=fL040; fL040=(uint64_t*)*ptr; m040.Unlock(); break; 

	default:	//	Non-standard size.
		mSze.LockWrite();

			pObj = malloc(size);

			nDiv = malloc_usable_size(pObj) ;
			if (nDiv < 8192)
			{
				m_allSizes[nDiv/8]++ ;
				m_allSizeA[nDiv/8]++ ;
			}
			nOver++ ;
			nOverSize += nDiv ;

		mSze.Unlock();

		return pObj;
	}

	return (void*) ptr;
}

void	_hz_heap::release	(void* pObj)
{
	//	Places object in free list if it is of one of the precribed sizes, otherwise it frees it from the OS managed heap
	//
	//	Arguments:	1)	pObj	Pointer to previously decclard memory
	//	Returns:	None

	_rblk*		pBlk ;		//	Block in which object is found
	uint64_t*	ptr ;		//	Object as uint64_t*
	char*		pVoid ;		//	Start of block (for pointer comparison)
	char*		pLim ;		//	End of block (for pointer comparison)
	uint32_t	size = 0 ;	//	Size derived from block in which object is found
	uint32_t	nPos ;		//	Position in hzOrder
	uint32_t	nDiv ;		//	Position devider

	if (!pObj)
		return ;
	if (pObj == this)
		return ;

	nCallsFree++;
	ptr = (uint64_t*) pObj ;

	//	Size is not supplied so must be determined from the only thing that is supplied - the pointer to the object to be freed. From the
	//	address we can determine what block it is in and so the size.

	mAll.LockWrite() ;
		size = 1000000 ;
		for (nPos = 2 ; nPos < numAllBlks ; nPos *= 2) ;
		nDiv = nPos / 2 ;
		nPos-- ;

		for (;;)
		{
			//if (nPos >= m_allBlocks.Count())
			if (nPos >= numAllBlks)
				nPos -= nDiv ;
			else
			{
				//pLim = pVoid = (char*) m_allBlocks.GetObj(nPos) ;
				pLim = pVoid = (char*) m_allBlocks[nPos] ;
				pLim += sizeof(_rblk) ;

				if (pObj < pVoid)
					nPos -= nDiv ;
				else if (pObj > pLim)
					nPos += nDiv ;
				else
				{
					pBlk = (_rblk*) pVoid ;
					size = pBlk->m_size ;
					break ;
				}
			}

			if (!nDiv)
				break ;
			nDiv /= 2 ;
		}
	mAll.Unlock() ;

	//	The following sets a pointer to the start of the object (first 8 bytes). Then it sets the contents of the start of the object so that it points
	//	to the start of the freelist (for the object size). The new start of the freelist is then set to the pointer (ie it is set to the start of the
	//	freed object)

	switch	(size)
	{
	case 8:		m008.LockWrite(); nU008--; nF008++ ; ptr=(uint64_t*)pObj; *ptr=(uint64_t)fL008; fL008=ptr; m008.Unlock(); break;
	case 16:	m016.LockWrite(); nU016--; nF016++ ; ptr=(uint64_t*)pObj; *ptr=(uint64_t)fL016; fL016=ptr; m016.Unlock(); break;
	case 24:	m024.LockWrite(); nU024--; nF024++ ; ptr=(uint64_t*)pObj; *ptr=(uint64_t)fL024; fL024=ptr; m024.Unlock(); break;
	case 32:	m032.LockWrite(); nU032--; nF032++ ; ptr=(uint64_t*)pObj; *ptr=(uint64_t)fL032; fL032=ptr; m032.Unlock(); break;
	case 40:	m040.LockWrite(); nU040--; nF040++ ; ptr=(uint64_t*)pObj; *ptr=(uint64_t)fL040; fL040=ptr; m040.Unlock(); break;
	default:
		nOver-- ;
		nDiv = malloc_usable_size(pObj) ;
		if (nDiv < 8192)
		{
			m_allSizes[nDiv/8]-- ;
			m_allSizeF[nDiv/8]++ ;
		}
		nOverSize -= nDiv ;
		break ;
	}
}

void	_hz_heap::shutdown	(void)
{
	//	Shutdown the heap
	//
	//	Arguments:	None
	//	Returns:	None

	fnptr_Alloc = _regime_alloc_Exit ;
}

/*
**	Global memory management functions
*/

void*	hz_mem_allocate	(uint32_t size)
{
	//	Category:	Memory
	//
	//	This function is ALWAYS called whenever operator new or new[] is called on ANY object. It will invoke whater function fnptr_Alloc points to
	//
	//	Arguments:	1)	size	Requested number of bytes for allocation
	//	Returns:	Pointer to the allocated memory

	return fnptr_Alloc(size) ;
}

void	hz_mem_release	(void* ptr)
{
	//	Category:	Memory
	//
	//	This function is ALWAYS called whenever operator delete or delete[] is called on ANY object. It will invoke whater function fnptr_Free points to
	//
	//	Arguments:	1)	ptr	Pointer to object to be freed
	//	Returns:	None

	fnptr_Free(ptr) ;
}

static	void*	_regime_alloc_Init	(uint32_t size)
{
	//	Category:	Memory
	//
	//	Since fnptr_Alloc is initially set to this function, the first allocation ever called by the program calls this function. So before allocating anything
	//	the global boolean _hzGlobal_XM is checked. If true the general new/delete override regime will be used for ALL memory allocation and deletion, if false
	//	malloc and free will be used instead.
	//
	//	Facilitating this is quite elaborate however. Operator new and delete always call _hz_mem_allocate and _hz_mem_release respectively. These call whatever
	//	function fnptr_Alloc and fnptr_Free point to. In the current version, fnptr_Free always points to _regime_free_norm which always tests _hzGlobal_XM in
	//	order to decide how to delete memory. But fnptr_Alloc is initially set to call this function.
	//
	//	The first memory allocated for the application is allocated via this function purely so _hzGlobal_XM can be tested before ANY further allocations. This
	//	function decides to set fnptr_Alloc to either _regime_alloc_Syst() for malloc/free and _regime_alloc_Ovrd for the override - then calls the selected
	//	function to allocate the requested memory. Subsequent allocations will be from whichever funtion fnptr_Alloc is set to.
	//
	//	Arguments:	1)	size	Number of bytes to allocate
	//	Returns:	Pointer to allocated memory block

	static	bool	bBeenHere = false ;

	void*	ptr ;		//	New object space

	if (_hzGlobal_XM)
	{
		if (bBeenHere)
			{ printf("_regime_alloc_Init: Duplicate Call\n") ; exit(1) ; }
		bBeenHere = true ;

		//	printf("Allocating the heap %p %p\n", *fnptr_Alloc, *fnptr_Free) ; fflush(stdout) ;
		s_Heap = (_hz_heap*) malloc(sizeof(_hz_heap)) ;
		memset(s_Heap, 0, sizeof(_hz_heap)) ;

		s_Heap->nCallsMalloc++ ;
		s_Heap->nOverSize += sizeof(_hz_heap) ;

		fnptr_Alloc = _regime_alloc_Ovrd ;
		fnptr_Free = _regime_free_Ovrd ;
	}
	else
	{
		fnptr_Alloc = _regime_alloc_Syst ;
		fnptr_Free = _regime_free_Syst ;
	}

	//	printf("Now Allocating the memory using approved fn\n") ; fflush(stdout) ;
	ptr = fnptr_Alloc(size) ;
	return ptr ;
}
                    
static	void*	_regime_alloc_Syst	(uint32_t size)
{
	//	Category:	Memory
	//
	//	Only called during new-overide initialization
	//
	//	Arguments:	1)	size	Bytes to allocate
	//	Returns:	Pointer to the allocated block

	void*	ptr ;		//	New object space

	s_nBytesInit += size ;
	ptr = malloc(size) ;

	return ptr ;
}

static	void*	_regime_alloc_Ovrd	(uint32_t size)
{
	//	Category:	Memory
	//
	//	This will allocate an object of the required size from a managed block if the size is small and by a direct call to malloc otherwise. In the
	//	latter case, the object will be placed in the map of outsized objects.
	//
	//	Arguments:	1)	size	Number of bytes requested
	//	Returns:	Pointer to the allocated block

	if (!s_Heap)
	{
		//	Fatal("No heap initialized\n") ;
		printf("No heap initialized\n") ; fflush(stdout) ;
		exit(1) ;
	}

	//	printf("_regime_alloc_Ovrd %d bytes\n", size) ; fflush(stdout) ;
	return s_Heap->allocate(size) ;
}

static	void	_regime_free_Ovrd	(void* ptr)	{ s_Heap->release(ptr) ; }
static	void	_regime_free_Syst	(void* ptr)	{ free(ptr) ; }

static	void*	_regime_alloc_Exit	(uint32_t size)
{
	//	Category:	Memory
	//
	//	Only called to allocate memory during shutdown (generally for diagnostic pursose only)
	//
	//	Arguments:	1)	size	Bytes to allocate
	//	Returns:	Pointer to the allocated block

	void*	ptr ;		//	New object space

	s_nBytesExit += size ;
	ptr = malloc(size) ;
	memset(ptr, 0, size) ;

	return ptr ;
}

/*
**	SECTION 2:	Memory use dianostics
*/

static char*	_sn	(char* buf, uint32_t nValue)
{
	//	Support function for HadronZooMemoryReport(). Essentially it is the same as the FormalNumber() function in hzTextproc.cpp except that it places the text
	//	form of the supplied value in a supplied buffer rather than returning it as a string. Although trivial, calling the standard FormalNumber function would
	//	alter memory usage.
	//	
	//	Arguments:	1)	buf		A pre-allocated buffer to recive the text form of the supplied value
	//				2)	nValue	The supplied value
	//				3)	bSign	True if the value can be -ve
	//
	//	Returns:	Pointer to the supplied buffer in all cases. 

	int32_t		B ;				//	Number of billions
	int32_t		M ;				//	Number of millions
	int32_t		T ;				//	Number of thousands
	int32_t		U ;				//	Number of units

	B = (nValue / 1000000000) ;
	M = (nValue % 1000000000) / 1000000 ;
	T = (nValue % 1000000) / 1000 ;
	U = (nValue % 1000) ;

	if (B)
		sprintf(buf, "%d,%03d,%03d,%03d", B, M, T, U) ;
	else if (M)
		sprintf(buf, "%d,%03d,%03d", M, T, U) ;
	else if (T)
		sprintf(buf, "%d,%03d", T, U) ;
	else
		sprintf(buf, "%d", U) ;

	return buf ;
}

/*
**	RAM Reports
*/

static	void	_report_mem_item3	(hzChain& Z, const char* title, uint32_t vA, uint32_t vB, uint32_t vC)
{
	//	Reports 3 values (e.g allocated and in use, free reserves and total)

	char	a[32] ;	//	Working buffer param 1
	char	b[32] ;	//	Working buffer param 1
	char	c[32] ;	//	Working buffer param 1

	Z.Printf("\t<tr align=\"right\"><td align=\"left\">%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n", title, _sn(a,vA), _sn(b,vB), _sn(c,vC)) ;
}

static	void	_report_mem_itemA	(hzChain& Z, const char* title, uint32_t curr, uint32_t prev, uint32_t sum)
{
	//	Reports 3 values (e.g allocated and in use, free reserves and total)

	char	b1[32] ;	//	Working buffer param 1
	char	b2[32] ;	//	Working buffer param 2
	char	b3[32] ;	//	Working buffer param 3

	Z << "<tr align=\"right\"><td align=\"left\">" ;
	if (!title)
		Z << "untitled" ;
	else
		Z << title ;
	Z << "</td><td></td><td></td><td></td><td></td>" ;

	Z.Printf("<td>%s</td><td>%s</td><td>%s</td></tr>\n", _sn(b1,curr), _sn(b2,prev), _sn(b3,sum)) ;
}

/*
NEED TO USE THIS AGAIN
static	void	_report_mem_itemB	(hzChain& Z, const char* title, uint32_t A, uint32_t B, uint32_t C, uint32_t D, uint32_t sum)
{
	//	Reports 5 values

	char	b1[32] ;	//	Working buffer param 1
	char	b2[32] ;	//	Working buffer param 2
	char	b3[32] ;	//	Working buffer param 3
	char	b4[32] ;	//	Working buffer param 4
	char	b5[32] ;	//	Working buffer param 5

	Z << "<tr align=\"right\"><td align=\"left\">" ;
	if (!title)
		Z << "untitled" ;
	else
		Z << title ;

	Z.Printf("</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n", _sn(b1, A), _sn(b2, B), _sn(b3, C), _sn(b4, D), _sn(b5,sum)) ;
}
*/

static	void	_report_mem_itemC	(hzChain& Z, const char* title, uint32_t A, uint32_t B, uint32_t C, uint32_t D, uint32_t curr, uint32_t prev, uint32_t sum)
{
	//	Reports 7 values

	char	b1[32] ;	//	Working buffer param 1
	char	b2[32] ;	//	Working buffer param 2
	char	b3[32] ;	//	Working buffer param 3
	char	b4[32] ;	//	Working buffer param 4
	char	b5[32] ;	//	Working buffer param 5
	char	b6[32] ;	//	Working buffer param 6
	char	b7[32] ;	//	Working buffer param 7

	Z << "<tr align=\"right\"><td align=\"left\">" ;
	if (!title)
		Z << "untitled" ;
	else
		Z << title ;

	Z.Printf("</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
		_sn(b1, A), _sn(b2, B), _sn(b3, C), _sn(b4, D), _sn(b5,curr), _sn(b6,prev), _sn(b7,sum)) ;
}

static	void	_report_objects	(hzChain& Z, const hzMeminfo& cms, const hzMeminfo& pms, uint32_t& total)
{
	//	Category:	Diagnostics
	//
	//	Support function to ReportMemoryUsage(). Provides Report on strings and chain memory usage by the application.
	//
	//	Arguments:	1)	Z		The hzChain instance to receive the report
	//				2)	cms		Current memory stats
	//				3)	pms		Previous memory stats
	//				4)	total	Running total RAM
	//
	//	Returns:	None

	static int32_t	prev_StrTbl = 0 ;	//	Previous count of strings held in string table

	uint32_t	U ;			//	Memory used by given object class
	char		b1[32] ;	//	Working buffer param 1

	Z <<
	"<table width=\"750\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" "
		"style=\"text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000;\">\n"
	"<tr>\n"
	"\t<th width=\"90\">Entity</th>\n"
	"\t<th width=\"90\">Assigned</th>\n"
	"\t<th width=\"90\">Prev</th>\n"
	"\t<th width=\"90\">Free</th>\n"
	"\t<th width=\"90\">Prev</th>\n"
	"\t<th width=\"90\">Total</th>\n"
	"\t<th width=\"90\">Prev</th>\n"
	"\t<th width=\"110\">Total RAM</th>\n"
	"</tr>\n" ;

	//	LISTS
	U = cms.m_numLists * 8 ;
	U += (cms.m_numListDC * 32) ;
	total += U ;
	_report_mem_itemC(Z, "Lists", cms.m_numListDC, pms.m_numListDC, cms.m_numLists - cms.m_numListDC, pms.m_numLists - pms.m_numListDC, cms.m_numLists, pms.m_numLists, U) ;

	//	CHAINS
	U = cms.m_numChain * sizeof(hzChain) ;
	U += (cms.m_numChainDC * 32) ;
	total += U ;
	_report_mem_itemC(Z, "Chains", cms.m_numChainDC, pms.m_numChainDC, cms.m_numChain - cms.m_numChainDC, pms.m_numChain - pms.m_numChainDC, cms.m_numChain, pms.m_numChain, U) ;

	//	CHAIN BLOCKS. Note that count of allocated blocks is the total number of blocks created minus the count of those in the freelist
	U = cms.m_numChainBlks * 1480 ;
	total += U ;
	_report_mem_itemC(Z, "Chain Blks", cms.m_numChainBlks - cms.m_numChainBF, pms.m_numChainBlks - pms.m_numChainBF, cms.m_numChainBF, pms.m_numChainBF, cms.m_numChainBlks, pms.m_numChainBlks, U) ;

 	//	STRINGS
	if (_hzGlobal_StringTable)
	{
		U = _hzGlobal_StringTable->Blocks() * 65536 ;
		total += U ;
		_report_mem_itemA(Z, "String Table", _hzGlobal_StringTable->Count(), prev_StrTbl, U) ;
	}

	//	Report number of string superblocks
	//	_report_mem_itemA(Z, "String Superblocks", cms.m_numSblks, pms.m_numSblks, cms.m_numSblks * 65536) ;

	//	U = cms.m_strSm_u[0] * 16 ;		total+=U ; _report_mem_itemB(Z, "Strings (008)", cms.m_strSm_u[0],	pms.m_strSm_u[0],	cms.m_strSm_f[0],	pms.m_strSm_f[0],	U) ;
	//	U = cms.m_strSm_u[1] * 16 ;		total+=U ; _report_mem_itemB(Z, "Strings (016)", cms.m_strSm_u[1],	pms.m_strSm_u[1],	cms.m_strSm_f[1],	pms.m_strSm_f[1],	U) ;
	//	U = cms.m_strSm_u[2] * 24 ;		total+=U ; _report_mem_itemB(Z, "Strings (024)", cms.m_strSm_u[2],	pms.m_strSm_u[2],	cms.m_strSm_f[2],	pms.m_strSm_f[2],	U) ;
	//	U = cms.m_strSm_u[3] * 32 ;		total+=U ; _report_mem_itemB(Z, "Strings (032)", cms.m_strSm_u[3],	pms.m_strSm_u[3],	cms.m_strSm_f[3],	pms.m_strSm_f[3],	U) ;
	//	U = cms.m_strSm_u[4] * 40 ;		total+=U ; _report_mem_itemB(Z, "Strings (040)", cms.m_strSm_u[4],	pms.m_strSm_u[4],	cms.m_strSm_f[4],	pms.m_strSm_f[4],	U) ;
	//	U = cms.m_strSm_u[5] * 48 ;		total+=U ; _report_mem_itemB(Z, "Strings (048)", cms.m_strSm_u[5],	pms.m_strSm_u[5],	cms.m_strSm_f[5],	pms.m_strSm_f[5],	U) ;
	//	U = cms.m_strSm_u[6] * 56 ;		total+=U ; _report_mem_itemB(Z, "Strings (056)", cms.m_strSm_u[6],	pms.m_strSm_u[6],	cms.m_strSm_f[6],	pms.m_strSm_f[6],	U) ;
	//	U = cms.m_strSm_u[7] * 64 ;		total+=U ; _report_mem_itemB(Z, "Strings (064)", cms.m_strSm_u[7],	pms.m_strSm_u[7],	cms.m_strSm_f[7],	pms.m_strSm_f[7],	U) ;
	//	U = cms.m_strSm_u[8] * 72 ;		total+=U ; _report_mem_itemB(Z, "Strings (072)", cms.m_strSm_u[8],	pms.m_strSm_u[8],	cms.m_strSm_f[8],	pms.m_strSm_f[8],	U) ;
	//	U = cms.m_strSm_u[9] * 80 ;		total+=U ; _report_mem_itemB(Z, "Strings (080)", cms.m_strSm_u[9],	pms.m_strSm_u[9],	cms.m_strSm_f[9],	pms.m_strSm_f[9],	U) ;
	//	U = cms.m_strSm_u[10] * 88 ;	total+=U ; _report_mem_itemB(Z, "Strings (088)", cms.m_strSm_u[10],	pms.m_strSm_u[10],	cms.m_strSm_f[10],	pms.m_strSm_f[10],	U) ;
	//	U = cms.m_strSm_u[11] * 96 ;	total+=U ; _report_mem_itemB(Z, "Strings (096)", cms.m_strSm_u[11],	pms.m_strSm_u[11],	cms.m_strSm_f[11],	pms.m_strSm_f[11],	U) ;
	//	U = cms.m_strSm_u[12] * 104 ;	total+=U ; _report_mem_itemB(Z, "Strings (104)", cms.m_strSm_u[12],	pms.m_strSm_u[12],	cms.m_strSm_f[12],	pms.m_strSm_f[12],	U) ;
	//	U = cms.m_strSm_u[13] * 112 ;	total+=U ; _report_mem_itemB(Z, "Strings (112)", cms.m_strSm_u[13],	pms.m_strSm_u[13],	cms.m_strSm_f[13],	pms.m_strSm_f[13],	U) ;
	//	U = cms.m_strSm_u[14] * 120 ;	total+=U ; _report_mem_itemB(Z, "Strings (120)", cms.m_strSm_u[14],	pms.m_strSm_u[14],	cms.m_strSm_f[14],	pms.m_strSm_f[14],	U) ;
	//	U = cms.m_strSm_u[15] * 128 ;	total+=U ; _report_mem_itemB(Z, "Strings (128)", cms.m_strSm_u[15],	pms.m_strSm_u[15],	cms.m_strSm_f[15],	pms.m_strSm_f[15],	U) ;
	//	U = cms.m_strSm_u[16] * 136 ;	total+=U ; _report_mem_itemB(Z, "Strings (136)", cms.m_strSm_u[16],	pms.m_strSm_u[16],	cms.m_strSm_f[16],	pms.m_strSm_f[16],	U) ;
	//	U = cms.m_strSm_u[17] * 144 ;	total+=U ; _report_mem_itemB(Z, "Strings (144)", cms.m_strSm_u[17],	pms.m_strSm_u[17],	cms.m_strSm_f[17],	pms.m_strSm_f[17],	U) ;
	//	U = cms.m_strSm_u[18] * 152 ;	total+=U ; _report_mem_itemB(Z, "Strings (152)", cms.m_strSm_u[18],	pms.m_strSm_u[18],	cms.m_strSm_f[18],	pms.m_strSm_f[18],	U) ;
	//	U = cms.m_strSm_u[19] * 160 ;	total+=U ; _report_mem_itemB(Z, "Strings (160)", cms.m_strSm_u[19],	pms.m_strSm_u[19],	cms.m_strSm_f[19],	pms.m_strSm_f[19],	U) ;
	//	U = cms.m_strSm_u[20] * 168 ;	total+=U ; _report_mem_itemB(Z, "Strings (168)", cms.m_strSm_u[20],	pms.m_strSm_u[20],	cms.m_strSm_f[20],	pms.m_strSm_f[20],	U) ;
	//	U = cms.m_strSm_u[21] * 176 ;	total+=U ; _report_mem_itemB(Z, "Strings (176)", cms.m_strSm_u[21],	pms.m_strSm_u[21],	cms.m_strSm_f[21],	pms.m_strSm_f[21],	U) ;
	//	U = cms.m_strSm_u[22] * 184 ;	total+=U ; _report_mem_itemB(Z, "Strings (184)", cms.m_strSm_u[22],	pms.m_strSm_u[22],	cms.m_strSm_f[22],	pms.m_strSm_f[22],	U) ;
	//	U = cms.m_strSm_u[23] * 192 ;	total+=U ; _report_mem_itemB(Z, "Strings (192)", cms.m_strSm_u[23],	pms.m_strSm_u[23],	cms.m_strSm_f[23],	pms.m_strSm_f[23],	U) ;
	//	U = cms.m_strSm_u[24] * 200 ;	total+=U ; _report_mem_itemB(Z, "Strings (200)", cms.m_strSm_u[24],	pms.m_strSm_u[24],	cms.m_strSm_f[24],	pms.m_strSm_f[24],	U) ;
	//	U = cms.m_strSm_u[25] * 208 ;	total+=U ; _report_mem_itemB(Z, "Strings (208)", cms.m_strSm_u[25],	pms.m_strSm_u[25],	cms.m_strSm_f[25],	pms.m_strSm_f[25],	U) ;
	//	U = cms.m_strSm_u[26] * 216 ;	total+=U ; _report_mem_itemB(Z, "Strings (216)", cms.m_strSm_u[26],	pms.m_strSm_u[26],	cms.m_strSm_f[26],	pms.m_strSm_f[26],	U) ;
	//	U = cms.m_strSm_u[27] * 224 ;	total+=U ; _report_mem_itemB(Z, "Strings (224)", cms.m_strSm_u[27],	pms.m_strSm_u[27],	cms.m_strSm_f[27],	pms.m_strSm_f[27],	U) ;
	//	U = cms.m_strSm_u[28] * 232 ;	total+=U ; _report_mem_itemB(Z, "Strings (232)", cms.m_strSm_u[28],	pms.m_strSm_u[28],	cms.m_strSm_f[28],	pms.m_strSm_f[28],	U) ;
	//	U = cms.m_strSm_u[29] * 240 ;	total+=U ; _report_mem_itemB(Z, "Strings (240)", cms.m_strSm_u[29],	pms.m_strSm_u[29],	cms.m_strSm_f[29],	pms.m_strSm_f[29],	U) ;
	//	U = cms.m_strSm_u[30] * 248 ;	total+=U ; _report_mem_itemB(Z, "Strings (248)", cms.m_strSm_u[30],	pms.m_strSm_u[30],	cms.m_strSm_f[30],	pms.m_strSm_f[30],	U) ;
	//	U = cms.m_strSm_u[31] * 256 ;	total+=U ; _report_mem_itemB(Z, "Strings (256)", cms.m_strSm_u[31],	pms.m_strSm_u[31],	cms.m_strSm_f[31],	pms.m_strSm_f[31],	U) ;

	//	Oversized strings
	//	U = cms.m_ramStrOver ; total += U ; _report_mem_itemA(Z, "Strings (other)", cms.m_numStrOver, pms.m_numStrOver, U) ;

	//	Report number of string (total)
	//	U = cms.m_numStrings * sizeof(hzString) ;
	//	total += U ;
	//_report_mem_itemC(Z, "Strings Total", cms.m_numStrings - cms.m_numStrNull, pms.m_numStrings - pms.m_numStrNull, cms.m_numStrNull, pms.m_numStrNull, cms.m_numStrings, pms.m_numStrings, U) ;
	_report_mem_itemA(Z, "Strings Total", cms.m_numStrings, pms.m_numStrings, U) ;

	//	Report number of memory blocks for strings (A-E)
	//	U = cms.m_numMemblkA * 4096 ; total += U ; _report_mem_itemA(Z, "memblk16", cms.m_numMemblkA, pms.m_numMemblkA, U) ;
	//	U = cms.m_numMemblkB * 4096 ; total += U ; _report_mem_itemA(Z, "memblk24", cms.m_numMemblkB, pms.m_numMemblkB, U) ;
	//	U = cms.m_numMemblkC * 4096 ; total += U ; _report_mem_itemA(Z, "memblk32", cms.m_numMemblkC, pms.m_numMemblkC, U) ;
	//	U = cms.m_numMemblkD * 4096 ; total += U ; _report_mem_itemA(Z, "memblk48", cms.m_numMemblkD, pms.m_numMemblkD, U) ;
	//	U = cms.m_numMemblkE * 4096 ; total += U ; _report_mem_itemA(Z, "memblk64", cms.m_numMemblkE, pms.m_numMemblkE, U) ;

	//	COLLECTIONS
	U = cms.m_numSmaps * 64 ;		total += U ;	_report_mem_itemA(Z, "Maps(S)",			cms.m_numSmaps,		pms.m_numSmaps,		U) ;
	U = cms.m_numMmaps * 64 ;		total += U ;	_report_mem_itemA(Z, "Maps(M)",			cms.m_numMmaps,		pms.m_numMmaps,		U) ;
	U = cms.m_numSets * 64 ;		total += U ;	_report_mem_itemA(Z, "Sets",			cms.m_numSets,		pms.m_numSets,		U) ;
	U = cms.m_numArrays * 64 ;		total += U ;	_report_mem_itemA(Z, "Arrays",			cms.m_numArrays,	pms.m_numArrays,	U) ;
	U = cms.m_numVectors * 64 ;		total += U ;	_report_mem_itemA(Z, "Vectors",			cms.m_numVectors,	pms.m_numVectors,	U) ;
	U = cms.m_numIsams * 64 ;		total += U ;	_report_mem_itemA(Z, "ISAMS",			cms.m_numIsams,		pms.m_numIsams,		U) ;
	U = cms.m_numIsamIndx * 64 ;	total += U ;	_report_mem_itemA(Z, "ISAM Blk Index",	cms.m_numIsamIndx,	pms.m_numIsamIndx,	U) ;
	U = cms.m_numIsamData * 64 ;	total += U ;	_report_mem_itemA(Z, "ISAM Blk Data",	cms.m_numIsamData,	pms.m_numIsamData,	U) ;
	U = cms.m_numBitmaps * 64 ;		total += U ;	_report_mem_itemA(Z, "Bitmaps",			cms.m_numBitmaps,	pms.m_numBitmaps,	U) ;
	U = cms.m_numBitmapSB * 64 ;	total += U ;	_report_mem_itemA(Z, "Bitmap Segments",	cms.m_numBitmapSB,	pms.m_numBitmapSB,	U) ;
	U = cms.m_numDochtm * 64 ;		total += U ;	_report_mem_itemA(Z, "Doc HTML",		cms.m_numDochtm,	pms.m_numDochtm,	U) ;
	U = cms.m_numDocxml * 64 ;		total += U ;	_report_mem_itemA(Z, "Doc XML",			cms.m_numDocxml,	pms.m_numDocxml,	U) ;
	U = cms.m_numBincron * 64 ;		total += U ;	_report_mem_itemA(Z, "BinCrons",		cms.m_numBincron,	pms.m_numBincron,	U) ;
	U = cms.m_numBinstore * 64 ;	total += U ;	_report_mem_itemA(Z, "BinStores",		cms.m_numBinstore,	pms.m_numBinstore,	U) ;

	//	TOTAL
	Z << "<tr><td>TOTAL EST:</td><td></td><td></td><td></td><td></td><td></td><td></td>" ;
	Z.Printf("<td align=\"right\">%s</td></tr>\n", _sn(b1,total)) ;
	Z << "</table>\n\n" ;
}

void	ReportStringAllocations	(hzChain& Z)
{
	//	Category:	Diagnostics

	hzMeminfo	cms ;		//	Current snapshot
	uint32_t	N ;			//	Number of string spaces used (for a given size)
	uint32_t	U ;			//	Memory used by N
	uint32_t	F ;			//	Number of string spaces free (for a given size)
	uint32_t	V ;			//	Memory used by F

	uint32_t	nss = 0 ;	//	Number of string spaces in use
	uint32_t	fss = 0 ;	//	Number of string spaces free

	uint32_t	mcu ;		//	Total memory consumed by used string spaces
	uint32_t	mcf ;		//	Total memory consumed by free string spaces
	//uint32_t	fre ;		//	Total memory held in free lists

	cms = _hzGlobal_Memstats ;

	//	Report number of string superblocks
	_report_mem_item3(Z, "512K Superblocks", cms.m_numSblks, 0, cms.m_numSblks * 589832) ;

	N = cms.m_strSm_u[0] ;  nss+=N ; U=N*8 ;	mcu+=U ;	F = cms.m_strSm_f[0] ;	fss+=F ; V=F*9 ;   mcf+=V ; _report_mem_item3(Z, "Strings (008)", N, F, U) ;
	N = cms.m_strSm_u[1] ;  nss+=N ; U=N*16 ;	mcu+=U ;	F = cms.m_strSm_f[1] ;	fss+=F ; V=F*16 ;  mcf+=V ; _report_mem_item3(Z, "Strings (016)", N, F, U) ;
	N = cms.m_strSm_u[2] ;  nss+=N ; U=N*24 ;	mcu+=U ;	F = cms.m_strSm_f[2] ;	fss+=F ; V=F*24 ;  mcf+=V ; _report_mem_item3(Z, "Strings (024)", N, F, U) ;
	N = cms.m_strSm_u[3] ;  nss+=N ; U=N*32 ;	mcu+=U ;	F = cms.m_strSm_f[3] ;	fss+=F ; V=F*32 ;  mcf+=V ; _report_mem_item3(Z, "Strings (032)", N, F, U) ;
	N = cms.m_strSm_u[4] ;  nss+=N ; U=N*40 ;	mcu+=U ;	F = cms.m_strSm_f[4] ;	fss+=F ; V=F*40 ;  mcf+=V ; _report_mem_item3(Z, "Strings (040)", N, F, U) ;
	N = cms.m_strSm_u[5] ;  nss+=N ; U=N*48 ;	mcu+=U ;	F = cms.m_strSm_f[5] ;	fss+=F ; V=F*48 ;  mcf+=V ; _report_mem_item3(Z, "Strings (048)", N, F, U) ;
	N = cms.m_strSm_u[6] ;  nss+=N ; U=N*56 ;	mcu+=U ;	F = cms.m_strSm_f[6] ;	fss+=F ; V=F*56 ;  mcf+=V ; _report_mem_item3(Z, "Strings (056)", N, F, U) ;
	N = cms.m_strSm_u[7] ;  nss+=N ; U=N*64 ;	mcu+=U ;	F = cms.m_strSm_f[7] ;	fss+=F ; V=F*64 ;  mcf+=V ; _report_mem_item3(Z, "Strings (064)", N, F, U) ;
	N = cms.m_strSm_u[8] ;  nss+=N ; U=N*72 ;	mcu+=U ;	F = cms.m_strSm_f[8] ;	fss+=F ; V=F*72 ;  mcf+=V ; _report_mem_item3(Z, "Strings (072)", N, F, U) ;
	N = cms.m_strSm_u[9] ;  nss+=N ; U=N*80 ;	mcu+=U ;	F = cms.m_strSm_f[9] ;	fss+=F ; V=F*80 ;  mcf+=V ; _report_mem_item3(Z, "Strings (080)", N, F, U) ;
	N = cms.m_strSm_u[10] ; nss+=N ; U=N*88 ;	mcu+=U ;	F = cms.m_strSm_f[10] ;	fss+=F ; V=F*88 ;  mcf+=V ; _report_mem_item3(Z, "Strings (088)", N, F, U) ;
	N = cms.m_strSm_u[11] ; nss+=N ; U=N*96 ;	mcu+=U ;	F = cms.m_strSm_f[11] ;	fss+=F ; V=F*96 ;  mcf+=V ; _report_mem_item3(Z, "Strings (096)", N, F, U) ;
	N = cms.m_strSm_u[12] ; nss+=N ; U=N*104 ;	mcu+=U ;	F = cms.m_strSm_f[12] ;	fss+=F ; V=F*104 ; mcf+=V ; _report_mem_item3(Z, "Strings (104)", N, F, U) ;
	N = cms.m_strSm_u[13] ; nss+=N ; U=N*112 ;	mcu+=U ;	F = cms.m_strSm_f[13] ;	fss+=F ; V=F*112 ; mcf+=V ; _report_mem_item3(Z, "Strings (112)", N, F, U) ;
	N = cms.m_strSm_u[14] ; nss+=N ; U=N*120 ;	mcu+=U ;	F = cms.m_strSm_f[14] ;	fss+=F ; V=F*120 ; mcf+=V ; _report_mem_item3(Z, "Strings (120)", N, F, U) ;
	N = cms.m_strSm_u[15] ; nss+=N ; U=N*128 ;	mcu+=U ;	F = cms.m_strSm_f[15] ;	fss+=F ; V=F*128 ; mcf+=V ; _report_mem_item3(Z, "Strings (128)", N, F, U) ;
	N = cms.m_strSm_u[16] ; nss+=N ; U=N*136 ;	mcu+=U ;	F = cms.m_strSm_f[16] ;	fss+=F ; V=F*136 ; mcf+=V ; _report_mem_item3(Z, "Strings (136)", N, F, U) ;
	N = cms.m_strSm_u[17] ; nss+=N ; U=N*144 ;	mcu+=U ;	F = cms.m_strSm_f[17] ;	fss+=F ; V=F*144 ; mcf+=V ; _report_mem_item3(Z, "Strings (144)", N, F, U) ;
	N = cms.m_strSm_u[18] ; nss+=N ; U=N*152 ;	mcu+=U ;	F = cms.m_strSm_f[18] ;	fss+=F ; V=F*152 ; mcf+=V ; _report_mem_item3(Z, "Strings (152)", N, F, U) ;
	N = cms.m_strSm_u[19] ; nss+=N ; U=N*160 ;	mcu+=U ;	F = cms.m_strSm_f[19] ;	fss+=F ; V=F*160 ; mcf+=V ; _report_mem_item3(Z, "Strings (160)", N, F, U) ;
	N = cms.m_strSm_u[20] ; nss+=N ; U=N*168 ;	mcu+=U ;	F = cms.m_strSm_f[20] ;	fss+=F ; V=F*168 ; mcf+=V ; _report_mem_item3(Z, "Strings (168)", N, F, U) ;
	N = cms.m_strSm_u[21] ; nss+=N ; U=N*176 ;	mcu+=U ;	F = cms.m_strSm_f[21] ;	fss+=F ; V=F*176 ; mcf+=V ; _report_mem_item3(Z, "Strings (176)", N, F, U) ;
	N = cms.m_strSm_u[22] ; nss+=N ; U=N*184 ;	mcu+=U ;	F = cms.m_strSm_f[22] ;	fss+=F ; V=F*184 ; mcf+=V ; _report_mem_item3(Z, "Strings (184)", N, F, U) ;
	N = cms.m_strSm_u[23] ; nss+=N ; U=N*192 ;	mcu+=U ;	F = cms.m_strSm_f[23] ;	fss+=F ; V=F*192 ; mcf+=V ; _report_mem_item3(Z, "Strings (192)", N, F, U) ;
	N = cms.m_strSm_u[24] ; nss+=N ; U=N*200 ;	mcu+=U ;	F = cms.m_strSm_f[24] ;	fss+=F ; V=F*200 ; mcf+=V ; _report_mem_item3(Z, "Strings (200)", N, F, U) ;
	N = cms.m_strSm_u[25] ; nss+=N ; U=N*208 ;	mcu+=U ;	F = cms.m_strSm_f[25] ;	fss+=F ; V=F*208 ; mcf+=V ; _report_mem_item3(Z, "Strings (208)", N, F, U) ;
	N = cms.m_strSm_u[26] ; nss+=N ; U=N*216 ;	mcu+=U ;	F = cms.m_strSm_f[26] ;	fss+=F ; V=F*216 ; mcf+=V ; _report_mem_item3(Z, "Strings (216)", N, F, U) ;
	N = cms.m_strSm_u[27] ; nss+=N ; U=N*224 ;	mcu+=U ;	F = cms.m_strSm_f[27] ;	fss+=F ; V=F*224 ; mcf+=V ; _report_mem_item3(Z, "Strings (224)", N, F, U) ;
	N = cms.m_strSm_u[28] ; nss+=N ; U=N*232 ;	mcu+=U ;	F = cms.m_strSm_f[28] ;	fss+=F ; V=F*232 ; mcf+=V ; _report_mem_item3(Z, "Strings (232)", N, F, U) ;
	N = cms.m_strSm_u[29] ; nss+=N ; U=N*240 ;	mcu+=U ;	F = cms.m_strSm_f[29] ;	fss+=F ; V=F*240 ; mcf+=V ; _report_mem_item3(Z, "Strings (240)", N, F, U) ;
	N = cms.m_strSm_u[30] ; nss+=N ; U=N*248 ;	mcu+=U ;	F = cms.m_strSm_f[30] ;	fss+=F ; V=F*248 ; mcf+=V ; _report_mem_item3(Z, "Strings (248)", N, F, U) ;
	N = cms.m_strSm_u[31] ; nss+=N ; U=N*256 ;	mcu+=U ;	F = cms.m_strSm_f[31] ;	fss+=F ; V=F*256 ; mcf+=V ; _report_mem_item3(Z, "Strings (256)", N, F, U) ;

	//	Oversized strings
	U = cms.m_ramStrOver ; mcu += U ; _report_mem_item3(Z, "Strings (ovr)", cms.m_numStrOver, 0, U) ;

	//	Report number of string (use)
	nss += cms.m_numStrings ;
	U = cms.m_numStrings * sizeof(hzString) ;
	mcu += U ;
	_report_mem_item3(Z, "Population (incl copies)", cms.m_numStrings, 0, U) ;
	_report_mem_item3(Z, "Total string spaces", nss, fss, mcu) ;
}

static	void	_report_heap	(hzChain& Z, const hzMeminfo& cms, const hzMeminfo& pms, uint32_t& total)
{
	//	Category:	Diagnostics
	//
	//	Support function to ReportMemoryUsage(). Provides Report on global new/delete override by the application, if applicable
	//
	//	Argument:	Z	The hzChain instance to receive the report
	//	Returns:	None

	uint32_t	usage ;		//	Memory used by given object class
	uint32_t	nA ;		//	Iterator for all sizes

	char	b1[32] ;		//	Working buffer param 1
	char	b3[32] ;		//	Working buffer param 3
	char	b5[32] ;		//	Working buffer param 5
	char	b7[32] ;		//	Working buffer param 7

	Z <<
	"<div class=\"stdpg\">\n"
	"<table width=\"506\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" "
		"style=\"text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000;\">\n"
	"<tr>\n"
	"\t<th width=\"120\">HadronZoo Entity</th>\n"
	"\t<th width=\"90\">Assigned</th>\n"
	"\t<th width=\"90\">Free</th>\n"
	"\t<th width=\"90\">Total</th>\n"
	"\t<th width=\"110\">Total RAM</th>\n"
	"</tr>\n" ;

	//	Managed
	Z << "<tr align=\"right\"><td align=\"left\">Heap   8-byte objects</td>" ;
	usage = (s_Heap->nU008 + s_Heap->nF008) * 8 ;
	Z.Printf("<td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
		_sn(b1, s_Heap->nU008), _sn(b3, s_Heap->nF008), _sn(b5, s_Heap->nU008 + s_Heap->nF008),	_sn(b7, usage)) ; 

	Z << "<tr align=\"right\"><td align=\"left\">Heap  16-byte objects</td>" ;
	usage = (s_Heap->nU016 + s_Heap->nF016) * 16 ;
	Z.Printf("<td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
		_sn(b1, s_Heap->nU016), _sn(b3, s_Heap->nF016), _sn(b5, s_Heap->nU016 + s_Heap->nF016),	_sn(b7, usage)) ; 

	Z << "<tr align=\"right\"><td align=\"left\">Heap  24-byte objects</td>" ;
	usage = (s_Heap->nU024 + s_Heap->nF024) * 24 ;
	Z.Printf("<td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
		_sn(b1, s_Heap->nU024), _sn(b3, s_Heap->nF024), _sn(b5, s_Heap->nU024 + s_Heap->nF024),	_sn(b7, usage)) ; 

	Z << "<tr align=\"right\"><td align=\"left\">Heap  32-byte objects</td>" ;
	usage = (s_Heap->nU032 + s_Heap->nF032) * 32 ;
	Z.Printf("<td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
		_sn(b1, s_Heap->nU032), _sn(b3, s_Heap->nF032), _sn(b5, s_Heap->nU032 + s_Heap->nF032),	_sn(b7, usage)) ; 

	Z << "<tr align=\"right\"><td align=\"left\">Heap  40-byte objects</td>" ;
	usage = (s_Heap->nU040 + s_Heap->nF040) * 40 ;
	Z.Printf("<td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
		_sn(b1, s_Heap->nU040), _sn(b3, s_Heap->nF040), _sn(b5, s_Heap->nU040 + s_Heap->nF040),	_sn(b7, usage)) ; 

	//	Print report for oversized objects for each multiple of 8 bytes for which there are outstanding allocatons
	Z << "<tr><td>Oversize (48+ bytes)</td><td></td><td></td><td></td></tr>\n" ;

	for (nA = 0 ; nA < 1001 ; nA++)
	{
		if (!s_Heap->m_allSizes[nA] && !s_Heap->m_allSizeF[nA])
			continue ;

		Z.Printf("<tr align=\"right\"><td align=\"left\">Obj size %d</td>", nA * 8) ;
		usage = s_Heap->m_allSizes[nA] * nA * 8 ;
		Z.Printf("<td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
			_sn(b1, s_Heap->m_allSizeA[nA]), _sn(b3, s_Heap->m_allSizeF[nA]), _sn(b5, s_Heap->m_allSizes[nA]),	_sn(b7, usage)) ; 
	}

	Z << "<tr align=\"right\"><td align=\"left\">All objects</td>" ;
	Z.Printf("<td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
		_sn(b1,s_Heap->nCallsMalloc),
		_sn(b3,s_Heap->nCallsFree),
		_sn(b3,s_Heap->nCallsMalloc - s_Heap->nCallsFree),
		_sn(b7,s_Heap->nOverSize)) ;

	Z <<
	"</table>\n"
	"</div>\n" ;
}

void	ReportMemoryUsage	(hzChain& Z)
{
	//	Category:	Diagnostics
	//
	//	Provides Report on memory usage by the application.
	//
	//	Please note, the HTML generated by this function is partial and comprises only the table of interest.
	//
	//	Argument:	Z	The hzChain instance to receive the report
	//	Returns:	None

	static	hzMeminfo	pms = _hzGlobal_Memstats ;		//	Previous/initial memory statistics

	hzMeminfo	cms ;				//	Current memory stats
	hzXDate		now ;				//	Current date and time
	uint32_t	total = 0 ;			//	Total memory usage

	now.SysDateTime() ;

	//	Take current snapshot
	cms = _hzGlobal_Memstats ;

	Z.Printf("<p><center>Memory Report at %s</center></p>\n", *now.Str()) ;

	//	If no heap, this is presented as collections on the LHS and string/chains on the RHS in one 'row'. If there is a heap we have string/chains on the top
	//	LHS, collections on the bottom LHS and the heap activity on the RHS
	if (s_Heap)
	{
		//	Two reports on LHS
		Z <<
		"<table width=\"1400\" slign=\"center\">\n"
		"<tr>\n"
		"	<td valign=\"top\">\n"
		"	<table width=\"750\" slign=\"center\">\n"
		"	<tr>\n"
		"		<td>\n" ;

				_report_objects(Z, cms, pms, total) ;

		Z <<
		"		</td>\n"
		"	</tr>\n"
		"	</table>\n"
		"	</td>\n"
		"	<td width=\"15\">&nbsp;</td>\n"
		"	<td>\n"
		"	<table width=\"550\" slign=\"center\">\n"
		"	<tr>\n"
		"		<td>\n" ;

				//	Heap on RHS
				_report_heap(Z, cms, pms, total) ;

		Z <<
		"		</td>\n"
		"	</tr>\n"
		"	</table>\n"
		"	</td>\n"
		"</tr>\n"
		"</table>\n" ;
	}
	else
	{
		//	collection on LHS, string chans on RHS
		Z <<
		"<table width=\"1400\" slign=\"center\">\n"
		"<tr>\n"
		"	<td valign=\"top\">\n" ;

		_report_objects(Z, cms, pms, total) ;

		Z <<
		"	</td>\n"
		"</tr>\n"
		"</table>\n" ;
	}

	pms = cms ;
}

void	RecordMemoryUsage	(void)
{
	//	Category:	Diagnostics
	//
	//	Provides Report on memory usage by the application and writes it to the logfile for the current thread
	//
	//	Arguments:	None
	//	Returns:	None

	hzChain		Z ;		//	Report output chain
	hzLogger*	pLog ;	//	Current thread logger

	pLog = GetThreadLogger() ;
	if (!pLog)
		return ;

	pLog->Out("Start of Memory Report\n") ;
	ReportMemoryUsage(Z) ;
	pLog->Out(Z) ;
}

void	ReportIsamUsage	(hzChain& Z)
{
	//	Category:	Diagnostics
	//
	//	Provides Report on ISAM usage by the application.
	//
	//	Argument:	Z	The hzChain instance to receive the report
	//	Returns:	None

	hzXDate	now ;		//	Current date and time

	now.SysDateTime() ;

	Z.Clear() ;
	Z.Printf("<p><center>ISAM Report at %s</center></p>\n", *now.Str()) ;

	Z <<
	"<html>\n"
	"<head>\n"
	"<style>\n"
	".main	{ text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000; }\n"
	".stdpg	{ height:600px; border:0px; margin-left:5px; overflow-x:auto; overflow-y:auto; }\n"
	"</style>\n"
	"</head>\n"
	"<body>\n"
	"<div class=\"stdpg\">\n"
	"<table width=\"70%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" "
		"style=\"text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000;\">\n"
	"<tr>\n"
	"\t<th>Type</th>\n"
	"\t<th>Name</th>\n"
	"\t<th>Blocks</th>\n"
	"\t<th>Object Size</th>\n"
	"\t<th>Objects</th>\n"
	"\t<th>Total RAM</th>\n"
	"</tr>\n" ;

	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_BinDataCrons</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_BinDataStores</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_Datatypes</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_Enums</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_Classes</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_Repositories</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">s_SSIncludes</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">s_PageStore</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_blockedIPs</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">s_mimesFile</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">s_mimesDesc</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">s_mimesEnum</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_Userlist</td>" ;
	Z << "<tr align=\"right\"><td align=\"left\">hzMapS</td><td align=\"left\">_hzGlobal_Grouplist</td>" ;

	Z <<
	"</table>\n"
	"</div>\n"
	"</body>\n"
	"</html>\n" ;
}
