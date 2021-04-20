//
//	File:	hzTmplArray.h
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

#ifndef hzTmplArray_h
#define hzTmplArray_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"

/*
**	Prototypes
*/

void	Fatal	(const char* va_alist ...) ;

#define HZ_ARRAY_NODESIZE	8

template<class OBJ> class hzArray
{
	//	Category:	Object Collection
	//
	//	The hzArray class template facilitates an array of objects. The objects are not stored in an actual array (of continuous memory) but in a tree of blocks
	//	based on absolute positioning. On the highest level the data blocks comprise only an array of HZ_ARRAY_NODESIZE objects while on the inner levels the index
	//	blocks comprise nothing but HZ_ARRAY_NODESIZE pointers to the blocks above. With a population of 0 the root of the tree is null and the hzArray has a level
	//	of zero. With a population between 1 and HZ_ARRAY_NODESIZE, the root points to a single data block and the hzArray now has 1 level. As the population rises
	//	above HZ_ARRAY_NODESIZE the level goes up to 2 and from now on the root is an index node. A new level is created when the root splits into two nodes, which
	//	happens when the population goes above HZ_ARRAY_NODESIZE to the power of the current level.
	//
	//	Note that hzArray does not support an Insert() or a Delete() method. It is intended only to be aggregated to by a series of appends and then accesses by
	//	the [] operator. The hzArray is thus an unordered collection.

	struct	_hz_ar_data
	{
		//	Block of HZ_ARRAY_NODESIZE objects

		OBJ	m_Objs[HZ_ARRAY_NODESIZE] ;		//	Objects held in the array
	} ;

	struct	_hz_ar_indx
	{
		//	Array of pointers (to either index or data blocks)

		void*	m_Ptrs[HZ_ARRAY_NODESIZE] ;	//	The pointers
	} ;

	struct	_array_ca
	{
		//	Array control area

		hzLockRW	m_Lock ;		//	Read/Write lock
		//hzLocker*	m_pLock ;		//	Locking (off by default)
		void*		m_pRoot ;		//	Root node
		uint32_t	m_nFactor ;		//	1 for level 0, HZ_ARRAY_NODESIZE for level 1, HZ_ARRAY_NODESIZE squared for 2 etc
		uint32_t	m_nNoDN ;		//	Number of data nodes
		uint32_t	m_nNoIN ;		//	Number of index nodes
		uint32_t	m_nCount ;		//	Number of items
		uint32_t	m_nLevel ;		//	The level of the array
		_m uint32_t	m_nCopy ;		//	Number of copies
		OBJ			m_Default ;		//	Default (empty) element

		//_array_ca	(void)	{ m_pRoot = 0 ; m_pLock = 0 ; m_nLevel = m_nNoDN = m_nNoIN = m_nCount = m_nCopy = 0 ; m_nFactor = 1 ; }
		_array_ca	(void)	{ m_pRoot = 0 ; m_nLevel = m_nNoDN = m_nNoIN = m_nCount = m_nCopy = 0 ; m_nFactor = 1 ; }
		~_array_ca	(void)	{}

		/*
		void	SetLock	(hzLockOpt eLock)
		{
			if (eLock == HZ_ATOMIC)	m_pLock = new hzLockRW() ;
			if (eLock == HZ_MUTEX)	m_pLock = new hzLockRWD() ;
		}
		*/

		//void	Lock	(void)	{ if (m_pLock) m_pLock->LockWrite() ; }
		//void	Unlock	(void)	{ if (m_pLock) m_pLock->Unlock() ; }
		void	Lock	(void)	{ m_Lock.LockWrite() ; }
		void	Unlock	(void)	{ m_Lock.Unlock() ; }
	} ;

	void	_clear	(void* pN, uint32_t nLevel, uint32_t maxLevel)
	{
		_hz_ar_indx*	pIdx ;	//	Current index block pointer
		_hz_ar_data*	pDN ;	//	Current object block pointer
		uint32_t		n ;		//	Object block iterator

		if (!mx)
			return ;
		if (nLevel < maxLevel)
		{
			pIdx = (_hz_ar_indx*) pN ;
			for (n = 0 ; n < HZ_ARRAY_NODESIZE && pIdx->m_Ptrs[n] ; n++)
				_clear(pIdx->m_Ptrs[n], nLevel+1, maxLevel) ;
		}
		else
		{
			pDN = (_hz_ar_data*) pN ; //pIdx->m_Ptrs[n] ;
			delete pDN ;
		}
	}

	_array_ca*	mx ;		//	The actual array

	//	Prevent copies
	hzArray<OBJ>	(hzArray<OBJ>& op) ;
	hzArray<OBJ>&	operator=	(const hzArray<OBJ>& op) ;

public:
	//hzArray	(hzLockOpt eLock = HZ_NOLOCK)
	hzArray	(void)
	{
		_hzGlobal_Memstats.m_numArrays++ ;
		mx = new _array_ca() ;
		//mx->SetLock(eLock) ;
	}
		
	~hzArray	(void)
	{
		mx->Lock() ;
		if (mx->m_nCopy)
		{
			mx->m_nCopy-- ;
			mx->Unlock() ;
		}
		else
		{
			//Clear() ;
			if (mx->m_pRoot)
				_clear(mx->m_pRoot, 1, mx->m_nLevel) ;

			_hzGlobal_Memstats.m_numArrayDA-- ;
			mx->Unlock() ;
			delete mx ;
		}
		_hzGlobal_Memstats.m_numArrays-- ;
	}

	void	SetDefault	(OBJ dflt)		{ mx->m_Default = dflt ; }

	void	Clear	(void)
	{
		_hzfunc("hzArray::Clear") ;

		//	Clear all nodes
		mx->Lock() ;
			if (mx->m_pRoot)
				_clear(mx->m_pRoot, 1, mx->m_nLevel) ;

			mx->m_pRoot = 0 ;
			mx->m_nLevel = 0 ;
			mx->m_nFactor = 1 ;
		mx->Unlock() ;
	}

	hzEcode	Add	(const OBJ& obj)
	{
		//	Add (append) an object to the end of the collection
		//
		//	Arguments:	1)	obj	Object to add

		_hzfunc("hzArray::Add") ;

		_hz_ar_data*	pDN ;	//	Data node
		_hz_ar_indx*	pIdx ;	//	Index node

		uint32_t	x ;			//	Position so far
		uint32_t	n ;			//	Offest at current level
		uint32_t	f ;			//	Division factor
		uint32_t	dnAddr ;	//	Slot on higest index node for target data node
		uint32_t	nSlot ;		//	Target slot on data node

		if (!mx)
			mx = new _array_ca() ;

		mx->Lock() ;

		//	If no root
		if (!mx->m_pRoot)
		{
			mx->m_nNoDN = 1 ;
			pDN = new _hz_ar_data() ;
			pDN->m_Objs[0] = obj ;
			mx->m_pRoot = pDN ;
			mx->m_nLevel = 1 ;
			mx->m_nCount = 1 ;
			mx->Unlock() ;
			return E_OK ;
		}

		if (mx->m_nCount < HZ_ARRAY_NODESIZE)
		{
			//	If root is a data node
			pDN = (_hz_ar_data*) mx->m_pRoot ;
			pDN->m_Objs[mx->m_nCount] = obj ;
			mx->m_nCount++ ;
			mx->Unlock() ;
			return E_OK ;
		}

		//	Do we need a new level?
		if (mx->m_nCount == (mx->m_nFactor * HZ_ARRAY_NODESIZE))
		{
			mx->m_nNoIN++ ;
			pIdx = new _hz_ar_indx() ;
			pIdx->m_Ptrs[0] = mx->m_pRoot ;
			mx->m_pRoot = pIdx ;
			mx->m_nLevel++ ;
			mx->m_nFactor *= HZ_ARRAY_NODESIZE ;
		}

		//	Go to highest data node
		pIdx = (_hz_ar_indx*) mx->m_pRoot ;

		for (x = mx->m_nCount, f = mx->m_nFactor ; f >= (HZ_ARRAY_NODESIZE * HZ_ARRAY_NODESIZE) ;)
		{
			n = x/f ; x %= f ; f /= HZ_ARRAY_NODESIZE ;

			if (!pIdx->m_Ptrs[n])
				{ mx->m_nNoIN++ ; pIdx->m_Ptrs[n] = new _hz_ar_indx() ; }
			pIdx = (_hz_ar_indx*) pIdx->m_Ptrs[n] ;
		}

		//	Data node 'address' is 
		dnAddr = x/f ;
		if (!pIdx->m_Ptrs[dnAddr])
			{ mx->m_nNoDN++ ; pIdx->m_Ptrs[dnAddr] = new _hz_ar_data() ; }
		pDN = (_hz_ar_data*) pIdx->m_Ptrs[dnAddr] ;

		//	Data node slot is remainder
		nSlot = x%f ;
		pDN->m_Objs[nSlot] = obj ;
		mx->m_nCount++ ;
		mx->Unlock() ;
		return E_OK ;
	}

	OBJ&	operator[]	(uint32_t nPosn) const
	{
		_hzfunc("hzArray::operator[]") ;

		_hz_ar_indx*	pIdx ;	//	Index node pointer
		_hz_ar_data*	pDN ;	//	Index node pointer

		uint32_t	x ;			//	Position reached so far
		uint32_t	n ;			//	Offest at current level
		uint32_t	f ;			//	Division factor

		if (nPosn >= mx->m_nCount)
			return mx->m_Default ;

		f = mx->m_nFactor ;

		if (f == 1)
		{
			pDN = (_hz_ar_data*) mx->m_pRoot ;
			n = nPosn ;
			return pDN->m_Objs[nPosn] ;
		}

		pIdx = (_hz_ar_indx*) mx->m_pRoot ;

		for (x = nPosn, f = mx->m_nFactor ; f >= (HZ_ARRAY_NODESIZE * HZ_ARRAY_NODESIZE) ;)
		{
			n = x/f ; x %= f ; f /= HZ_ARRAY_NODESIZE ;
			pIdx = (_hz_ar_indx*) pIdx->m_Ptrs[n] ;
		}
		pDN = (_hz_ar_data*) pIdx->m_Ptrs[x/f] ;
		return pDN->m_Objs[x%f] ;
	}

	OBJ*	InSitu	(uint32_t nPosn) const
	{
		_hzfunc("hzArray::InSitu") ;

		_hz_ar_indx*	pIdx ;	//	Index node pointer
		_hz_ar_data*	pDN ;	//	Index node pointer
		OBJ*			pObj ;	//	Object pointer

		uint32_t	x ;			//	Position reached so far
		uint32_t	n ;			//	Offest at current level
		uint32_t	f ;			//	Division factor

		if (nPosn >= mx->m_nCount)
			return 0 ;	//mx->m_Default ;

		f = mx->m_nFactor ;

		if (f == 1)
		{
			pDN = (_hz_ar_data*) mx->m_pRoot ;
			pObj = (OBJ*) pDN->m_Objs ;
			return pObj + nPosn ;
		}

		pIdx = (_hz_ar_indx*) mx->m_pRoot ;

		for (x = nPosn, f = mx->m_nFactor ; f >= (HZ_ARRAY_NODESIZE * HZ_ARRAY_NODESIZE) ;)
		{
			n = x/f ; x %= f ; f /= HZ_ARRAY_NODESIZE ;
			pIdx = (_hz_ar_indx*) pIdx->m_Ptrs[n] ;
		}
		pDN = (_hz_ar_data*) pIdx->m_Ptrs[x/f] ;
		pObj = (OBJ*) pDN->m_Objs ;
		return pObj + (x%f) ;
	}

	//	Diagnostics
	uint32_t	Level	(void) const	{ return mx ? mx->m_nLevel : 0 ; }
	uint32_t	Factor	(void) const	{ return mx ? mx->m_nFactor : 0 ; }
	uint32_t	dBlocks	(void) const	{ return mx ? mx->m_nNoDN : 0 ; }
	uint32_t	iBlocks	(void) const	{ return mx ? mx->m_nNoIN : 0 ; }
	uint32_t	Count	(void) const	{ return mx ? mx->m_nCount : 0 ; }
} ;

#endif	//	hzTmplArray_h
