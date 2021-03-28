//
//	File:	hzCtmpls.h
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

#ifndef hzCtmpls_h
#define hzCtmpls_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"

/*
**	Prototypes
*/

void	Fatal	(const char* va_alist ...) ;

enum	hzLockOpt
{
	//	Category:	System
	//
	//	Note this is for simplification of arguments within collection class templates only

	HZ_NOLOCK,		//	No locking in use
	HZ_ATOMIC,		//	Atomic locks no contention reporting
	HZ_MUTEX		//	Atomic locks with contention reporting
} ;

/*
**	Section 1	Simple Collections
*/

#define CTMPL_NODESIZE	8

template<class OBJ> class hzArray
{
	//	Category:	Object Collection
	//
	//	The hzArray class template facilitates an array of objects. The objects are not stored in an actual array (of continuous memory) but in a tree of blocks
	//	based on absolute positioning. On the highest level the data blocks comprise only an array of CTMPL_NODESIZE objects while on the inner levels the index
	//	blocks comprise nothing but CTMPL_NODESIZE pointers to the blocks above. With a population of 0 the root of the tree is null and the hzArray has a level
	//	of zero. With a population between 1 and CTMPL_NODESIZE, the root points to a single data block and the hzArray now has 1 level. As the population rises
	//	above CTMPL_NODESIZE the level goes up to 2 and from now on the root is an index node. A new level is created when the root splits into two nodes, which
	//	happens when the population goes above CTMPL_NODESIZE to the power of the current level.
	//
	//	Note that hzArray does not support an Insert() or a Delete() method. It is intended only to be aggregated to by a series of appends and then accesses by
	//	the [] operator. The hzArray is thus an unordered collection.

	struct	_hz_ar_data
	{
		//	Block of CTMPL_NODESIZE objects

		OBJ	m_Objs[CTMPL_NODESIZE] ;		//	Objects held in the array
	} ;

	struct	_hz_ar_indx
	{
		//	Array of pointers (to either index or data blocks)

		void*	m_Ptrs[CTMPL_NODESIZE] ;	//	The pointers
	} ;

	struct	_array_ca
	{
		//	Array control area

		hzLocker*	m_pLock ;		//	Locking (off by default)
		void*		m_pRoot ;		//	Root node
		uint32_t	m_nFactor ;		//	1 for level 0, CTMPL_NODESIZE for level 1, CTMPL_NODESIZE squared for 2 etc
		uint32_t	m_nNoDN ;		//	Number of data nodes
		uint32_t	m_nNoIN ;		//	Number of index nodes
		uint32_t	m_nCount ;		//	Number of items
		uint32_t	m_nLevel ;		//	The level of the array
		_m uint32_t	m_nCopy ;		//	Number of copies
		OBJ			m_Default ;		//	Default (empty) element

		_array_ca	(void)	{ m_pRoot = 0 ; m_pLock = 0 ; m_nLevel = m_nNoDN = m_nNoIN = m_nCount = m_nCopy = 0 ; m_nFactor = 1 ; }
		~_array_ca	(void)	{}

		void	SetLock	(hzLockOpt eLock)
		{
			if (eLock == HZ_ATOMIC)	m_pLock = new hzLockRW() ;
			if (eLock == HZ_MUTEX)	m_pLock = new hzLockRWD() ;
		}

		void	Lock	(void)	{ if (m_pLock) m_pLock->LockWrite() ; }
		void	Unlock	(void)	{ if (m_pLock) m_pLock->Unlock() ; }
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
			for (n = 0 ; n < CTMPL_NODESIZE && pIdx->m_Ptrs[n] ; n++)
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
	hzArray	(hzLockOpt eLock = HZ_NOLOCK)
	{
		_hzGlobal_Memstats.m_numArrays++ ;
		mx = new _array_ca() ;
		mx->SetLock(eLock) ;
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
			Clear() ;
			_hzGlobal_Memstats.m_numArrayDA-- ;
			mx->Unlock() ;
			delete mx ;
		}
		_hzGlobal_Memstats.m_numArrays-- ;
	}

	void	SetDefault	(OBJ dflt)		{ mx->m_Default = dflt ; }

	void	Clear	(void)
	{
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

		_hzfunc_ct("hzArray::Add") ;

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

		if (mx->m_nCount < CTMPL_NODESIZE)
		{
			//	If root is a data node
			pDN = (_hz_ar_data*) mx->m_pRoot ;
			pDN->m_Objs[mx->m_nCount] = obj ;
			mx->m_nCount++ ;
			mx->Unlock() ;
			return E_OK ;
		}

		//	Do we need a new level?
		if (mx->m_nCount == (mx->m_nFactor * CTMPL_NODESIZE))
		{
			mx->m_nNoIN++ ;
			pIdx = new _hz_ar_indx() ;
			pIdx->m_Ptrs[0] = mx->m_pRoot ;
			mx->m_pRoot = pIdx ;
			mx->m_nLevel++ ;
			mx->m_nFactor *= CTMPL_NODESIZE ;
		}

		//	Go to highest data node
		pIdx = (_hz_ar_indx*) mx->m_pRoot ;

		for (x = mx->m_nCount, f = mx->m_nFactor ; f >= (CTMPL_NODESIZE * CTMPL_NODESIZE) ;)
		{
			n = x/f ; x %= f ; f /= CTMPL_NODESIZE ;

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

		for (x = nPosn, f = mx->m_nFactor ; f >= (CTMPL_NODESIZE * CTMPL_NODESIZE) ;)
		{
			n = x/f ; x %= f ; f /= CTMPL_NODESIZE ;
			pIdx = (_hz_ar_indx*) pIdx->m_Ptrs[n] ;
		}
		pDN = (_hz_ar_data*) pIdx->m_Ptrs[x/f] ;
		return pDN->m_Objs[x%f] ;
	}

	OBJ*	InSitu	(uint32_t nPosn) const
	{
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

		for (x = nPosn, f = mx->m_nFactor ; f >= (CTMPL_NODESIZE * CTMPL_NODESIZE) ;)
		{
			n = x/f ; x %= f ; f /= CTMPL_NODESIZE ;
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

template <class OBJ>	class	_hz_listitem
{
	//	Category:	Collection Support
	//
	//	List element containing an element and a pointer to the next element. For use in the unordered list class templates, hzList, hzQue and hzStack.

public:
	_hz_listitem*	next ;		//	Next item in list
	OBJ				m_Obj ;		//	Object in list

	_hz_listitem	(OBJ obj)	{ next = 0 ; m_Obj = obj ; }
} ;

template <class OBJ>	class	hzList
{
	//	Category:	Object Collection
	//
	//	Single Linked list
	//
	//	hzList is a single linked list with each element linked only to the next element. This saves space compared to a double linked list, but at the price of
	//	no reverse iteration. HadronZoo has not yet created a double linked variant as the experience so far has not suggested a pressing need for one. Thusfar,
	//	the usage pattern has been that lists are compiled, and then iterated from begining to end. At some point a double linked list wil be brought in.
	//
	//	hzList provides an interator - the subclass Iter (hzList::Iter). The list iterator can be directly assigned to the begining of a list OR to another list
	//	iterator. Iteration is via the ++ operator until Iter::Valid() is no longer true. Objects are available via the iterator's Element() method.
	//
	//	hzList does not itself provide methods of Insert() and Delete(), instead these are provided by the iterator. This is because there is no way to specify
	//	a position within a list except in terms of the 'current' element. The concept of a current element only exists within an iterator, not within a list.
	//
	//	<b>Usage Considerations:</b>
	//
	//	In common with other HadronZoo collection classes, hzList does not facilitate hard copies. Lists are intended to be short but potentially numerous. Many
	//	tree-like classes use lists as the means by which nodes manage sub-nodes. It is common for applications to have many thousands of lists and it is common
	//	for large numbers of these to be empty. So hzList only comprises a pointer to a 'data area' that is initialized to NULL in the constructor. Only when an
	//	object is added will the data area be created. The data area is deleted when the last object is deleted from the list.
	//
	//	Note that it is common for items to be deleted during iteration as it becomes clear to the iterating process, the item is no longer needed. This is safe
	//	as long as only one iterator is operating on the list but not safe otherwise. Although the deletion takes place within a lock there is no way of knowing
	//	if another iterator is pointing to the item in question, and no way for the other iterator to know its current item is defunct. A process using the other
	//	iterator may attempt to perform an operation on a defunct object or seek to advance the other iterator on the basis of a defunct pointer.

	struct	_list_ca
	{
		//	Struct to hold list contents. This is used to prevent lists from real copying.

		_hz_listitem<OBJ>*	m_pList ;		//	Ptr to first element in the list
		_hz_listitem<OBJ>*	m_pLast ;		//	Ptr to last element in the list

		hzLockS		m_Lock ;			//	Built in lock
		uint32_t	m_nCount ;			//	No of elements in the list
		uint32_t	m_nCopy ;			//	No of copies
		uint32_t	m_nIter ;			//	No of iterators of this list

		_list_ca	(void)	{ m_pList = m_pLast = 0 ; m_nCount = m_nCopy = m_nIter = 0 ; _hzGlobal_Memstats.m_numListDC++ ; }
		~_list_ca	(void)	{ Clear() ; _hzGlobal_Memstats.m_numListDC-- ; }

		void	Lock		(void)	{ if (_hzGlobal_MT) m_Lock.Lock() ; }
		void	Unlock		(void)	{ if (_hzGlobal_MT) m_Lock.Unlock() ; }

		void	Clear	(void)
		{
			//	Clear all list contents.

			_hz_listitem<OBJ>*	pC ;	//	List element pointer
			_hz_listitem<OBJ>*	pN ;	//	Next object iterator

			Lock() ;
				if (m_nCopy > 0)
					m_nCopy-- ;
				else
				{
					for (pC = m_pList ; pC ; pC = pN)
						{ pN = pC->next ; delete pC ; }
					m_pList = m_pLast = 0 ;
					m_nCount = 0 ;
				}
			Unlock() ;
		}

		hzEcode	_add	(const OBJ obj)
		{
			//	Add item to end of list
			//
			//	Arguments:	1)	obj	Object to add

			_hzfunc_ct("_list_ca::_add") ;

			_hz_listitem<OBJ>*	pTL ;	//	List element pointer

			if (!(pTL = new _hz_listitem<OBJ>(obj)))
				Fatal("_list_ca::_add. Link allocation failure") ;

			Lock() ;
				if (m_pList == 0)
					m_pList = m_pLast = pTL ;
				else
				{
					m_pLast->next = pTL ;
					m_pLast = pTL ;
				}
				m_nCount++ ;
			Unlock() ;
			return E_OK ;
		}

		hzEcode	_insert	(const OBJ obj, _hz_listitem<OBJ>* currlink, bool bAfter)
		{
			//	Insert item before/after supplied current link.
			//
			//	Argument:	1)	obj			The object to insert
			//				2)	currLink	The current link
			//				3)	bAfter		Indicate insertation should be after current link, not before
			//
			//	Returns:	E_ARGUMENT	If the current link is not supplied
			//				E_NOTFOUND	If the supplied current link isn't actually in the list
			//				E_OK		If the object was inserted

			_hzfunc_ct("_list_ca::_insert") ;

			_hz_listitem<OBJ>*	marker ;	//	List element pointer
			_hz_listitem<OBJ>*	newlink ;	//	List element pointer

			if (!(newlink = new _hz_listitem<OBJ>(obj)))
				Fatal("_list_ca::_add. Link allocation failure") ;

			Lock() ;
				if (m_pList == 0)
					m_pList = m_pLast = newlink ;
				else
				{
					if (!currlink)
						return E_NOINIT ;

					if (bAfter)
					{
						//	To insert after currlink
						newlink->next = currlink->next ;
						currlink->next = newlink ;
					}
					else
					{
						for (marker = m_pList ; marker ; marker = marker->next)
						{
							if (marker->next == currlink)
								break ;
						}

						if (!marker)
							return E_NOTFOUND ;

						newlink->next = currlink ;
						marker->next = newlink ;
					}
				}

				m_nCount++ ;
			Unlock() ;
			return E_OK ;
		}

		hzEcode	_delete	(_hz_listitem<OBJ>* pTL)
		{
			//	Find the requested element in the list and delete it. This will also set the previous element's next pointer to the next element.
			//
			//	Note that the practice of deleting elements is not fully recomended. The process is inefficient because in order to keep memory consumption low,
			//	list elements have no backward pointer. In order to find the element prior to the target element, this function must iterate from the start. For
			//	the most part, lists are intended to be relatively short so this is OK. However this is not the only issue. In general, Lists are intended to be
			//	processed by iterating from begining to end. If the element is deleted by a call to Delete() on the iterator itself (a common method), it should
			//	be noted the current element of the iterator then becomes the next element in the list. Control loops in which Delete() is conditionally called
			//	on the iterator, need to ensure the iterator is not simply incremented each time, otherwise elements will be missed.
			//
			//	Argument:	pTL		The list item to be deleted
			//
			//	Returns:	E_NOTFOUND	If the supplied item is not in the list
			//				E_OK		If the item is deleted

			_hz_listitem<OBJ>*	prev = 0 ;	//	Previous link

			Lock() ;
				if (pTL == m_pList)
					m_pList = m_pList->next ;
				else
				{
					//	Advance prev link to be pointing to the item to be deleted
					for (prev = m_pList ; prev ; prev = prev->next)
					{
						if (prev->next == pTL)
							break ;
					}

					//	Sanity check, if null prev then exit
					if (!prev)
						return E_NOTFOUND ;
				}

				//	If the item is the last
				if (pTL == m_pLast)
					m_pLast = prev ;

				//	tie adjacent items together
				if (prev)
					prev->next = pTL->next ;

				delete pTL ;
				m_nCount-- ;
			Unlock() ;
			return E_OK ;
		}
	} ;

	_list_ca*	mx ;		//	Pointer to internal list container

public:
	struct	Iter
	{
		//	Generic iterator for the hzList class template

		_hz_listitem<OBJ>*	m_pCurr ;	//	Current element
		_list_ca*			m_pHandle ;	//	Pointer to the content

		OBJ		dflt ;					//	Default (null) element

		Iter	(void)	{ m_pHandle = 0 ; m_pCurr = 0 ; }
		Iter	(OBJ x)	{ m_pHandle = 0 ; m_pCurr = 0 ; dflt = x ; }
		~Iter	(void)	{ m_pHandle = 0 ; m_pCurr = 0 ; }

		void	SetDefault	(OBJ df)	{ dflt = df ; }

		hzEcode	Insert	(OBJ obj, bool bAfter=false)
		{
			//	Insert an element into the actual list. By default this will be before the current position but if bAfter is set then
			//	the element will be placed after the current position.
			//
			//	Returns:	E_ARGUMENT	If the current link is not supplied
			//				E_NOTFOUND	If the supplied current link isn't actually in the list
			//				E_OK		If the object was inserted

			_hzfunc_ct("hzList::Iter::Insert") ;

			if (!m_pHandle || !m_pCurr)
				return E_NOINIT ;
			return mx->_insert(m_pCurr, bAfter) ;
		}

		hzEcode	Delete	(void)
		{
			//	Delete the iterator's current element from the list. This operation will fail if the iterator is not the only one using the list's internal data
			//	area. In the event the deletion is successful, as the curent element then ceases to exist the current element is set to the next element in the
			//	list. The process calling delete on the iterator, should check if the deletion took place and if so, refrain from incrementing the iterator at
			//	the top of the iteration loop.
			//
			//	Returns:	E_NOTFOUND	If the iterator is not pointing to a list internal area.
			//				E_CONFLICT	If there is more than one iterator, none can delete
			//				E_OK		If the object was inserted

			_hz_listitem<OBJ>*	marker ;	//	List element pointer

			hzEcode	rc ;	//	Return code

			if (!m_pHandle)
				return E_NOTFOUND ;
			if (m_pHandle->m_nIter > 1)
				return E_CONFLICT ;
			marker = m_pCurr->next ;
			rc = m_pHandle->_delete(m_pCurr) ;
			m_pCurr = marker ;
			return rc ;
		}

		//	Assignment operators
		hzList<OBJ>::Iter&	operator=	(const hzList<OBJ>& sup)
		{
			//	Make an iterator equal to the beginning of the supplied list

			m_pHandle = sup.mx ;
			m_pCurr = m_pHandle ? m_pHandle->m_pList : 0 ;
			return *this ;
		}

		hzList<OBJ>::Iter&	operator=	(const Iter& i)
		{
			//	Make an iterator equal to another

			m_pHandle = i.m_pHandle ;
			m_pCurr = i.m_pCurr ;
			return *this ;
		}

		void	operator++	(void)
		{
			//	Increment iterator

			if (m_pCurr)
				m_pCurr = m_pCurr->next ;
		}

		void	operator++	(int)
		{
			//	Increment iterator

			if (m_pCurr)
				m_pCurr = m_pCurr->next ;
		}

		OBJ&	Element	(void)
		{
			//	Return curent element

			if (!m_pCurr)
				return dflt ;
			return m_pCurr->m_Obj ;
		}

		bool	Valid	(void)	{ return m_pCurr ? true : false ; }
	} ;

	/*
	**	Public hzList functions
	*/

	hzList	(void)
	{
		mx = 0 ;
		_hzGlobal_Memstats.m_numLists++ ;
	}


	/*
	hzList<OBJ>	(const hzList<OBJ>& op)
	{
		mx = 0 ;

		if (op.mx)
		{
			mx->Lock() ;
			mx->m_nCopy++ ;
			mx = op.mx ;
			mx->Unlock() ;
		}
	}
	*/

	~hzList	(void)
	{
		if (mx)
		{
			if (mx->m_nCopy > 0)
				mx->m_nCopy-- ;
			else
			{
				_hzGlobal_Memstats.m_numLists-- ;
				delete mx ;
				mx = 0 ;
			}
		}
	}

	//	Assignment operator
	const hzList<OBJ>&	operator=	(const hzList<OBJ>& op)
	{
		if (mx && mx == op.mx)
			return *this ;

		if (mx)
		{
			//	If the list is populated, clear it. Note will not delete objects if the list is of pointers rather than real objects and
			//	will therefore be a memory leak.

			if (mx->m_nCopy)
				mx->m_nCopy-- ;
			else
				mx->Clear() ;
		}

		mx = op.mx ;
		if (mx)
			mx->m_nCopy++ ;
		return *this ;
	}

	uint32_t	Count	(void) const
	{
		//	Return the number of items in the list
		return mx ? mx->m_nCount : 0 ;
	}

	hzEcode	Add	(const OBJ obj)
	{
		//	Add an object to the end of the list

		_hzfunc_ct("hzList::Add") ;

		if (!mx)
		{
			if (!(mx = new _list_ca()))
				Fatal("hzList::Add. Container allocation failure") ;
		}
		return mx->_add(obj) ;
	}

	void	Clear	(void)
	{
		//	Clear:	Empty the list
		if (mx)
		{
			if (mx->m_nCopy > 0)
				mx->m_nCopy-- ;
			else
				mx->Clear() ;
		}
		mx = 0 ;
	}
} ;

template <class OBJ>	class	hzQue
{
	//	Category:	Object Collection
	//
	//	The hzQue class template faciliates a memory resident que of objects. A que is characterized by PUSH and POP operations in which new
	//	objects are injected at the back of the que (pushed) and objects are removed from the front of the que (pulled).

private:
	struct	_que_ca
	{
		//	Struct to hold actual content (avoid hard copying)

		_hz_listitem<OBJ>*	m_pList ;	//	Start of que
		_hz_listitem<OBJ>*	m_pLast ;	//	End of que

		hzLocker*		m_pLock ;		//	Locking (off by default)
		hzString		m_Name ;		//	Diagnostics & reports
		uint32_t		m_nCount ;		//	Population
		_m uint32_t		m_nCopy ;		//	Copy count
		OBJ				m_Default ;		//	Default (null) element

		_que_ca		(void)	{ memset(&m_Default, 0, sizeof(OBJ)) ; m_pList = m_pLast = 0 ; m_pLock = 0 ; m_nCount = m_nCopy = 0 ; }
		~_que_ca	(void)	{}

		void	SetLock	(hzLockOpt eLock)
		{
			if (eLock == HZ_ATOMIC)	m_pLock = new hzLockRW() ;
			if (eLock == HZ_MUTEX)	m_pLock = new hzLockRWD() ;
		}

		void	LockRead	(void)	{ if (m_pLock) m_pLock->LockRead() ; }
		void	LockWrite	(void)	{ if (m_pLock) m_pLock->LockWrite() ; }
		void	Unlock		(void)	{ if (m_pLock) m_pLock->Unlock() ; }
	} ;

	_que_ca*	mx ;	//	Pointer to internal content

	//	Prevent copies
	hzQue<OBJ>	(const hzQue<OBJ>&) ;
	hzQue<OBJ>&	operator=	(const hzQue<OBJ>&) ;

public:
	//	Constructors
	hzQue	(hzLockOpt eLock = HZ_NOLOCK)	{ mx = new _que_ca() ; mx->SetLock(eLock) ; }
	hzQue	(const hzString name)			{ mx = new _que_ca() ; mx->m_Name = name ; mx->SetLock(HZ_MUTEX) ; }

	//	Destructor
	~hzQue	(void)
	{
		if (mx->m_nCopy > 0)
			mx->m_nCopy-- ;
		else
		{
			Clear() ;
			delete mx ;
			mx = 0 ;
		}
	}

	void	Clear	(void)
	{
		_hz_listitem<OBJ>*	pC ;	//	Curreent list item
		_hz_listitem<OBJ>*	pN ;	//	Next list item

		for (pC = mx->m_pList ; pC ; pC = pN)
		{
			pN = pC->next ;
			delete pC ;
		}

		mx->m_pList = mx->m_pLast = 0 ;
		mx->m_nCount = 0 ;
	}

	//	Assignment operator
	/*
	const hzQue<OBJ>&	operator=	(const hzQue<OBJ>& op)
	{
		//	If this control area is the same as that of the operand, do nothing. Otherwise decouple from or clear the control area and adopt that of the operand.
		if (mx == op.mx)
			return *this ;

			//	If there is an existing list, clear it.
		if (mx->m_nCopy)
			mx->m_nCopy-- ;
		else
			mx->Clear() ;

		mx = op.mx ;
		mx->m_nCopy++ ;
		return *this ;
	}
	*/

	uint32_t	Count	(void)
	{
		//	Return the number of items in the queue
		return mx->m_nCount ;
	}

	hzEcode	Push	(const OBJ& obj)
	{
		//	Push supplied object onto the queue

		_hz_listitem<OBJ>*	pTL ;	//	List item to accomodate new object

		if (!(pTL = new _hz_listitem<OBJ>(obj)))
			Fatal("Could not allocate new que element") ;

		if (mx->m_pList == 0)
			mx->m_pList = mx->m_pLast = pTL ;
		else
		{
			//mx->m_pLast->m_pNext = pTL ;
			mx->m_pLast->next = pTL ;
			mx->m_pLast = pTL ;
		}

		mx->m_nCount++ ;
		return E_OK ;
	}

	OBJ&	Pull	(void)
	{
		//	Pull an object from the queue

		_hz_listitem<OBJ>*	pTL ;	//	Object-link iterator

		OBJ		obj ;				//	Current object-link

		if ((pTL = mx->m_pList))
		{
			obj = mx->m_pList->m_Obj ;
			//mx->m_pList = mx->m_pList->m_pNext ;
			mx->m_pList = mx->m_pList->next ;
			delete pTL ;
			mx->m_nCount-- ;
		}

		return mx->m_Default ;
	}

	hzEcode	Delete	(const OBJ& obj)
	{
		//	Delete the supplied object from the queue (on condition it is foundin the queue)

		_hz_listitem<OBJ>*	pTL ;	//	Current link
		_hz_listitem<OBJ>*	pPL ;	//	Previous link

		hzEcode	rc = E_NOTFOUND ;	//	Return code

		//	match obj pointer to one on list

		pPL = 0 ;
		for (pTL = mx->m_pList ; pTL ; pTL = pTL->m_pNext)
		{
			if (pTL->m_Obj == obj)
			{
				//	found item to delete
				if (pTL == mx->m_pList)
					//mx->m_pList = mx->m_pList->m_pNext ;
					mx->m_pList = mx->m_pList->next ;

				//	If the item is the last
				if (pTL == mx->m_pLast)
					mx->m_pLast = pPL ;

				//	tie adjacent items together
				if (pPL)
					//pPL->m_pNext = pTL->m_pNext;
					pPL->next = pTL->next;

				delete pTL ;
				mx->m_nCount-- ;
				rc = E_OK ;
				break ;
			}

			pPL = pTL ;
		}

		return rc ;
	}
} ;

template <class OBJ>	class	hzStack
{
	//	Category:	Object Collection
	//
	//	The hzStack class template faciliates a memory resident stack of objects. A stack is characterized by PUSH and PULL operations in which new
	//	objects are injected at the front of the stack (pushed) and objects are removed from the front of the stack (pulled). This provides a LIFO
	//	(last-in, first-out) operation. This differs from a que (hzQue) in which new objects are placed at the back of a que and popped in order.

private:
	struct	_stack_ca
	{
		//	Struct to hold actual content (avoid hard copying)

		_hz_listitem<OBJ>*	m_pList ;	//	Start of list of objects in stack
		_hz_listitem<OBJ>*	m_pLast ;	//	Pointer to last object in the stack

		hzLocker*		m_pLock ;		//	Locking (off by default)
		hzString		m_Name ;		//	Diagnostics & reports
		uint32_t		m_nCount ;		//	Population
		mutable int32_t	m_nCopy ;		//	Copy count
		OBJ				m_Default ;		//	Default (null) element

		_stack_ca	(void)	{ memset(&m_Default, 0, sizeof(OBJ)) ; m_pList = m_pLast = 0 ; m_pLock = 0 ; m_nCount = m_nCopy = 0 ; }
		~_stack_ca	(void)	{ delete m_pLock ; }

		void	SetLock	(hzLockOpt eLock)
		{
			if (eLock == HZ_ATOMIC)	m_pLock = new hzLockRW() ;
			if (eLock == HZ_MUTEX)	m_pLock = new hzLockRWD() ;
		}

		void	LockRead	(void)	{ if (m_pLock) m_pLock->LockRead() ; }
		void	LockWrite	(void)	{ if (m_pLock) m_pLock->LockWrite() ; }
		void	Unlock		(void)	{ if (m_pLock) m_pLock->Unlock() ; }
	} ;

	_stack_ca*	mx ;	//	Pointer to internal content

	//	Prevent copies
	hzStack<OBJ>	(const hzStack<OBJ>&) ;
	hzStack<OBJ>&	operator=	(const hzStack<OBJ>&) ;

public:
	//	Constructor
	hzStack	(hzLockOpt eLock = HZ_NOLOCK)	{ mx = new _stack_ca() ; mx->SetLock(eLock) ; }
	hzStack	(const hzString name)			{ mx = new _stack_ca() ; mx->m_Name = name ; mx->SetLock(HZ_MUTEX) ; }
	//hzStack	(const hzStack<OBJ>& op)		{ mx = op.mx ; mx->m_nCopy++ ; }

	//	Destructor
	~hzStack	(void)
	{
		if (mx->m_nCopy > 0)
			mx->m_nCopy-- ;
		else
		{
			Clear() ;
			delete mx ;
		}
	}

	uint32_t	Count	(void)	{ return mx->m_nCount ; }

	/*
	const hzStack<OBJ>&	operator=	(const hzStack<OBJ>& op)
	{
		//	If this control area is the same as that of the operand, do nothing. Otherwise decouple from or clear the control area and adopt that of the operand.
		if (mx == op.mx)
			return *this ;

		if (mx->m_nCopy)
			mx->m_nCopy-- ;
		else
			mx->Clear() ;

		mx = op.mx ;
		mx->m_nCopy++ ;
		return *this ;
	}
	*/

	void	Clear	(void)
	{
		//	Delete all objects from the stack

		_hz_listitem<OBJ>*	pC ;	//	Current object pointer
		_hz_listitem<OBJ>*	pN ;	//	Next object in series

		for (pC = mx->m_pList ; pC ; pC = pN)
		{
			pN = pC->next ;
			delete pC ;
		}

		mx->m_pList = mx->m_pLast = 0 ;
		mx->m_nCount = 0 ;
	}

	void	Push	(const OBJ obj)
	{
		//	Push an object onto the stack
		//
		//	Argument:	obj		Object to push
		//	Returns:	None

		_hzfunc_ct("hzStack::Push") ;

		_hz_listitem<OBJ>*	pTL ;	//	New object pointer

		if (!(pTL = new _hz_listitem<OBJ>(obj)))
			Fatal("hzStack::Push. Allocaction failure") ;

		if (mx->m_pList == 0)
			mx->m_pList = mx->m_pLast = pTL ;
		else
		{
			//mx->m_pLast->m_pNext = pTL ;
			mx->m_pLast->next = pTL ;
			mx->m_pLast = pTL ;
		}

		mx->m_nCount++ ;
	}

	//	Return element
	OBJ&	Pull	(void)
	{
		//	Pull an object from the stack
		//
		//	Arguments:	None
		//	Returns:	Referenece to last object placed on stack

		_hz_listitem<OBJ>*	pTL ;	//	Object-link iterator

		OBJ		obj ;				//	Target object

		if ((pTL = mx->m_pList))
		{
			obj = mx->m_pList->m_Obj ;
			//mx->m_pList = mx->m_pList->m_pNext ;
			mx->m_pList = mx->m_pList->next ;
			delete pTL ;
			mx->m_nCount-- ;
		}

		return mx->m_Default ;
	}

	hzEcode	Delete	(const OBJ& obj)
	{
		//	Delete the specified object from the stack
		//
		//	Argument:	obj		Object to be deleted
		//
		//	Returns:	E_NOTFOUND	If the object does not exist
		//				E_OK		If the object was deleted

		_hz_listitem<OBJ>*	pTL ;	//	Current link
		_hz_listitem<OBJ>*	pPL ;	//	Previous link

		hzEcode	rc = E_NOTFOUND ;	//	Return code

		//	match obj pointer to one on list

		pPL = 0 ;
		for (pTL = mx->m_pList ; pTL ; pTL = pTL->m_pNext)
		{
			if (pTL->m_Obj == obj)
			{
				//	found item to delete
				if (pTL == mx->m_pList)
					//mx->m_pList = mx->m_pList->m_pNext ;
					mx->m_pList = mx->m_pList->next ;

				//	If the item is the last
				if (pTL == mx->m_pLast)
					mx->m_pLast = pPL ;

				//	tie adjacent items together
				if (pPL)
					//pPL->m_pNext = pTL->m_pNext;
					pPL->next = pTL->next;

				delete pTL ;
				mx->m_nCount-- ;
				rc = E_OK ;
				break ;
			}

			pPL = pTL ;
		}

		return rc ;
	}
} ;

/*
**	Section 2	Function Templates and support classes and class templatess
*/

//	Compare
template<class X>	int	_tmpl_compare	(const X a, const X b)	{ return a>b ? 1 : a<b ? -1 : 0 ; }

//
//	The _hz_order ISAM support classes and class templates
//
//	In this implimentation three proto-collection class templates are provided as follows:-
//
//	1)	_hz_isam_Relpos<KEY>	Object list ordered only by position, provides the underlying functionality of a vector with relative positioning. The formal class
//								template hzVect has a _hz_isam_Relpos as a member.
//
//	2)	_hz_isam_Value<KEY>		Object list ordered by value, can be numerically iterated but does NOT allow inserts or deletes based on relative positions. Instead
//								inserts and deletes are strictly by value which must be unique (it is assumed pointless to multiple enter identical objects). As this
//								template only has a single argument it can only act as an object set. The formal class template hzSet<KEY> uses an _hz_isam_Value.
//
//	3)	_hz_isam_Pair<KEY,OBJ>	Key/Object pair list ordered by key, similar to _hz_isam_Value but has two arguments in order to effect a key/object map. Multiple
//								instances of keys are supported enabling _hz_isam_Pair to form the basis of both the formal class template hzMapS (1 to 1 mapping)
//								and hzMapM (1 to many). 
//
//	All three are based on _hz_isam_Base which holds the tree of index and data blocks, a method of accessing elements by relative position and diagnostic functions
//	indicating block occupancy. The original intention was that _hz_isam_Relpos and _hz_isam_Value would be derivatives of _hz_isam_Base and _hz_isam_Pair would be
//	a derivative of _hz_isam_Value. However for reasons that are not altogether clear, the gcc C++ compiler version 4.4.7 inter alia, won't wear it. So instead each
//	of the proto-collection class templates contains the _hz_isam_Base as a member.
//
//	In all cases the index nodes are identical but there are two types of data blocks, implimented as class templates (_hz_isam_dblkA and _hz_isam_dblkB). The latter
//	has arguments of KEY and OBJ while the former only has KEY. This is to avoid space waste by byte alignment in a 64 bit environment. In the common case where 32
//	bit keys map to 64 bit objects or 64 bit keys map to 32 bit objects, bytes are wasted by binding the key/object pairs in a single class. But with _hz_isam_dblkB
//	having a separate array for keys and objects this problem is avoided. The number of elements per node, be they keys, objects or key/object pairs is set to be a
//	multiple of 8 which avoids byte alignment wastage in each node.

#define HZ_ORDSZE	8
#define HZ_ISAM_LOW	5

class	_hz_isam_iblk ;

class	_hz_isam_block
{
	//	Category:	Collection Support
	//
	//	Pure virtual base class for index and data nodes

public:
	_hz_isam_iblk*		parent ;	//	Parent node

	virtual	~_hz_isam_block	(void)	{}

	virtual	_hz_isam_iblk*	Parent	(void) = 0 ;
	virtual	_hz_isam_block*	Infra	(void) = 0 ;
	virtual	_hz_isam_block*	Ultra	(void) = 0 ;

	virtual	uint32_t	Cumulative	(void) = 0 ;
	virtual	uint32_t	Level		(void) = 0 ;
	virtual	uint32_t	Usage		(void) = 0 ;
	virtual	uint32_t	IsamId		(void) = 0 ;
	virtual	uint32_t	Ident		(void) = 0 ;
	virtual	uint32_t	InfId		(void) = 0 ;
	virtual	uint32_t	UltId		(void) = 0 ;

	virtual	void	SetParent	(_hz_isam_iblk*) = 0 ;
} ;

class	_hz_isam_iblk	: public _hz_isam_block
{
	//	Category:	Collection Support
	//
	//	Common Index nodes
public:
	_hz_isam_iblk*	infra ;				//	Infra adjacent
	_hz_isam_iblk*	ultra ;				//	Ultra adjacent
	_hz_isam_block*	m_Ptrs[HZ_ORDSZE] ;	//	Pointers either to index or data nodes

	uint32_t	cumulative ;			//	Number of elements ultimately pointed to by this block
	uint32_t	m_Id ;					//	Unique node id (for diagnostics)
	uint16_t	m_isamId ;				//	Ties the node to the ISAM
	uint16_t	usage ;					//	Number of elements in this block
	uint16_t	level ;					//	Level of this block in the tree
	uint16_t	resv ;					//	Reserved

	_hz_isam_iblk	(void)
	{
		//printf("NEW ISAM BLOCK\n") ;
		m_Id = ++_hzGlobal_Memstats.m_numIsamIndx ;
		m_Ptrs[0] = m_Ptrs[1] = m_Ptrs[2] = m_Ptrs[3] = m_Ptrs[4] = m_Ptrs[5] = m_Ptrs[6] = m_Ptrs[7] = 0 ; 
		parent = infra = ultra = 0 ;
		cumulative = 0 ;
		m_isamId = usage = level = resv = 0 ;
	}

	~_hz_isam_iblk	(void)
	{
		_hzGlobal_Memstats.m_numIsamIndx-- ;
	}

	_hz_isam_iblk*	Parent	(void)	{ return parent ; }
	_hz_isam_iblk*	Infra	(void)	{ return infra ; }
	_hz_isam_iblk*	Ultra	(void)	{ return ultra ; }

	uint32_t	Cumulative	(void)	{ return cumulative ; }
	uint32_t	Usage		(void)	{ return usage ; }
	uint32_t	Level		(void)	{ return level ; }
	uint32_t	IsamId		(void)	{ return m_isamId ; }
	uint32_t	Ident		(void)	{ return m_Id ; }
	uint32_t	InfId		(void)	{ return infra ? infra->m_Id : 0 ; }
	uint32_t	UltId		(void)	{ return ultra ? ultra->m_Id : 0 ; }

	void	SetParent	(_hz_isam_iblk* par)	{ parent = par ; }
	void	SetLevel	(int32_t v)				{ level = v ; }
} ;

class	_hz_isam_dblk	: public _hz_isam_block
{
	//	Category:	Collection Support
	//
	//	Pure virtual base class for index and data nodes

public:
	virtual ~_hz_isam_dblk	(void)	{}

	virtual	_hz_isam_dblk*	allocUltra	(void) = 0 ;

	virtual	void*	Keys		(void) = 0 ;
	virtual	void	IncUsage	(void) = 0 ;
	virtual	void	DecUsage	(void) = 0 ;
	virtual	void	SetUsage	(uint32_t) = 0 ;
	virtual void	SetInfra	(_hz_isam_dblk*) = 0 ;
	virtual void	SetUltra	(_hz_isam_dblk*) = 0 ;
	virtual void	setfromCur	(int32_t slot, int32_t srcSlot) = 0 ;
	virtual void	setfromInf	(int32_t slot, int32_t srcSlot) = 0 ;
	virtual void	setfromUlt	(int32_t slot, int32_t srcSlot) = 0 ;
} ;

template <class KEY>	class	_hz_isam_dblkA	: public _hz_isam_dblk
{
	//	Category:	Collection Support
	//
	//	Single datum data blocks for use in the _hz_isam_Relpos and _hz_isam_Value ISAM templates.

public:
	_hz_isam_dblkA<KEY>*	infra ;		//	Infra adjacent
	_hz_isam_dblkA<KEY>*	ultra ;		//	Ultra adjacent

	KEY			m_Keys[HZ_ORDSZE] ;		//	Pointers either to mapentries or higher nodes
	uint32_t	m_Id ;					//	Unique node id (for diagnostics)
	uint16_t	m_isamId ;				//	Ties node to ISAM
	uint16_t	usage ;					//	Number of elements in this block

	_hz_isam_dblkA	(void)
	{
		//printf("NEW DATA A BLK %lu\n", sizeof(KEY)) ;
		m_Id = ++_hzGlobal_Memstats.m_numIsamData ;
		parent = 0 ;
		infra = ultra = 0 ;
		m_isamId = usage = 0 ;
		memset(m_Keys, 0, (HZ_ORDSZE * sizeof(KEY))) ;
	}

	~_hz_isam_dblkA	(void)
	{
		_hzGlobal_Memstats.m_numIsamData-- ;
	}

	_hz_isam_dblkA<KEY>*	allocUltra	(void)
	{
		//	Allocate ultra node to this
		//
		//	Arguments:	None
		//	Returns:	Pointer to the allocated ultra node

		_hz_isam_dblkA<KEY>*	pUlt ;	//	Ultra node

		pUlt = new _hz_isam_dblkA<KEY> ;

		if (ultra)
			ultra->infra = pUlt ;
		pUlt->ultra = ultra ;
		pUlt->infra = this ;
		pUlt->m_isamId = m_isamId ;
		ultra = pUlt ;
		return pUlt ;
	}

	//	Get values
	_hz_isam_iblk*	Parent	(void)	{ return parent ; }
	_hz_isam_dblk*	Infra	(void)	{ return infra ; }
	_hz_isam_dblk*	Ultra	(void)	{ return ultra ; }

	uint32_t	Cumulative	(void)	{ return usage ; }
	uint32_t	Usage		(void)	{ return usage ; }
	uint32_t	Level		(void)	{ return 0 ; }
	uint32_t	IsamId		(void)	{ return m_isamId ; }
	uint32_t	Ident		(void)	{ return m_Id ; }
	uint32_t	InfId		(void)	{ return infra ? infra->m_Id : 0 ; }
	uint32_t	UltId		(void)	{ return ultra ? ultra->m_Id : 0 ; }

	void*	Keys		(void)	{ return &m_Keys ; }

	//	Set values
	void	SetParent	(_hz_isam_iblk* par)	{ parent = par ; }
	void	SetInfra	(_hz_isam_dblk* pInf)	{ infra = (_hz_isam_dblkA<KEY>*) pInf ; }
	void	SetUltra	(_hz_isam_dblk* pUlt)	{ ultra = (_hz_isam_dblkA<KEY>*) pUlt ; }

	void	IncUsage	(void)			{ usage++ ; }
	void	DecUsage	(void)			{ usage-- ; }
	void	SetUsage	(uint32_t u)	{ usage = u ; }

	void	setfromCur	(int32_t slot, int32_t srcSlot)	{ m_Keys[slot] = m_Keys[srcSlot] ; }
	void	setfromInf	(int32_t slot, int32_t srcSlot)	{ m_Keys[slot] = infra->m_Keys[srcSlot] ; }
	void	setfromUlt	(int32_t slot, int32_t srcSlot)	{ m_Keys[slot] = ultra->m_Keys[srcSlot] ; }
} ;

template <class KEY, class OBJ>	class	_hz_isam_dblkB	: public _hz_isam_dblk
{
	//	Category:	Collection Support
	//
	//	Dual datum data blocks for use in the _hz_isam_Pair class.

public:
	_hz_isam_dblkB<KEY,OBJ>*	infra ;	//	Infra adjacent
	_hz_isam_dblkB<KEY,OBJ>*	ultra ;	//	Ultra adjacent

	OBJ			m_Objs[HZ_ORDSZE] ;		//	Pointers either to mapentries or higher nodes
	KEY			m_Keys[HZ_ORDSZE] ;		//	Pointers either to mapentries or higher nodes
	uint32_t	m_Id ;					//	Unique node id (for diagnostics)
	uint16_t	m_isamId ;				//	Ties node to ISAM
	uint16_t	usage ;					//	Number of elements in this block

	_hz_isam_dblkB	(void)
	{
		//printf("NEW DATA B BLK %lu %lu\n", sizeof(KEY), sizeof(OBJ)) ;
		m_Id = ++_hzGlobal_Memstats.m_numIsamData ;
		parent = 0 ;
		infra = ultra = 0 ;
		m_isamId = usage = 0 ;
		memset(m_Keys, 0, (HZ_ORDSZE * sizeof(KEY))) ;
		memset(m_Objs, 0, (HZ_ORDSZE * sizeof(OBJ))) ;
	}
	~_hz_isam_dblkB	(void)
	{
		_hzGlobal_Memstats.m_numIsamData-- ;
	}

	//	allocate ultra node
	_hz_isam_dblkB<KEY,OBJ>*	allocUltra	(void)
	{
		_hz_isam_dblkB<KEY,OBJ>*	pUlt ;	//	Superior node

		pUlt = new _hz_isam_dblkB<KEY,OBJ> ;

		if (ultra)
			ultra->infra = pUlt ;
		pUlt->ultra = ultra ;
		pUlt->infra = this ;
		pUlt->m_isamId = m_isamId ;
		ultra = pUlt ;
		return pUlt ;
	}

	//	Get values
	_hz_isam_iblk*	Parent	(void)	{ return parent ; }
	_hz_isam_dblk*	Infra	(void)	{ return infra ; }
	_hz_isam_dblk*	Ultra	(void)	{ return ultra ; }

	uint32_t	Cumulative	(void)	{ return usage ; }
	uint32_t	Usage		(void)	{ return usage ; }
	uint32_t	Level		(void)	{ return 0 ; }
	uint32_t	IsamId		(void)	{ return m_isamId ; }
	uint32_t	Ident		(void)	{ return m_Id ; }
	uint32_t	InfId		(void)	{ return infra ? infra->m_Id : 0 ; }
	uint32_t	UltId		(void)	{ return ultra ? ultra->m_Id : 0 ; }

	void*	Keys		(void)	{ return &m_Keys ; }

	//	Set values
	void	SetParent	(_hz_isam_iblk* par)	{ parent = par ; }
	void	SetInfra	(_hz_isam_dblk* pInf)	{ infra = (_hz_isam_dblkB<KEY,OBJ>*) pInf ; }
	void	SetUltra	(_hz_isam_dblk* pUlt)	{ ultra = (_hz_isam_dblkB<KEY,OBJ>*) pUlt ; }

	void	IncUsage	(void)			{ usage++ ; }
	void	DecUsage	(void)			{ usage-- ; }
	void	SetUsage	(uint32_t u)	{ usage = u ; }

	void	setfromCur	(int32_t slot, int32_t srcSlot)	{ m_Keys[slot] = m_Keys[srcSlot] ; m_Objs[slot] = m_Objs[srcSlot] ; }
	void	setfromInf	(int32_t slot, int32_t srcSlot)	{ m_Keys[slot] = infra->m_Keys[srcSlot] ; m_Objs[slot] = infra->m_Objs[srcSlot] ; }
	void	setfromUlt	(int32_t slot, int32_t srcSlot)	{ m_Keys[slot] = ultra->m_Keys[srcSlot] ; m_Objs[slot] = ultra->m_Objs[srcSlot] ; }
} ;

template <class KEY>	class	_hz_isam_Base
{
	//	Category:	Collection Support
	//
	//	The _hz_isam_Base is what holds the ISAM root (and thus the index and data) on behalf of ...

	class	_isam_ca
	{
		//	ISAM control area

	public:
		_hz_isam_block*	m_pRoot ;		//	Start of index
		hzLocker*		m_pLock ;		//	Locking (off by default)
		uint32_t		isamId ;		//	For diagnostics
		uint32_t		m_nNodes ;		//	Number of nodes (also as node id allocator)
		uint32_t		m_nElements ;	//	Number of data objects
		_m uint32_t		m_copy ;		//	Copy count
		KEY				m_Default ;		//	Default (null) object

		_isam_ca	(void)
		{
			m_pLock = 0 ;
			m_pRoot = 0 ;
			m_nNodes = m_nElements = m_copy = 0 ;
			memset(&m_Default, 0, sizeof(KEY)) ;
		}

		~_isam_ca	(void)	{ _clear() ; }

		void	_clearnode	(_hz_isam_block* pBlk)
		{
			//	Recursively clear supplied node and any child nodes
			//
			//	Argument:	pBlk	The supplied node
			//	Returns:	None

			_hz_isam_iblk*	pIdx ;	//	Current ISAM block pointer
			uint32_t		n ;		//	ISAM block selector

			if (!pBlk->Level())
				delete pBlk ;
			else
			{
				pIdx = (_hz_isam_iblk*) pBlk ;

				for (n = 0 ; n < pIdx->usage ; n++)
					_clearnode(pIdx->m_Ptrs[n]) ;
				delete pIdx ;
			}
		}

		void	_clear	(void)
		{
			//	Clear the map of all keys and elements
			//
			//	Begin at root and take the least pointer out to the lowest data node and pass through the list of data nodes deleting them. Then drop back one level
			//	to delete the index nodes level by level 
			//
			//	Arguments:	None
			//	Returns:	None

			if (!m_pRoot)
				return ;
			_clearnode(m_pRoot) ;
			m_nElements = 0 ;
			m_pRoot = 0 ;
		}

		void	SetLock	(hzLockOpt eLock)
		{
			if (eLock == HZ_ATOMIC)	m_pLock = new hzLockRW() ;
			if (eLock == HZ_MUTEX)	m_pLock = new hzLockRWD() ;
		}
	} ;

	_isam_ca*	mz ;		//	The internal ISAM structure
	hzString	m_Name ;	//	ISAM name (all ISAM dependent templates use this)

	//	Prevent copies
	_hz_isam_Base<KEY>	(const _hz_isam_Base<KEY>&) ;
	_hz_isam_Base<KEY>&	operator=	(const _hz_isam_Base<KEY>&) ;

public:

	_hz_isam_Base	(void)	{ mz = 0 ; }

	~_hz_isam_Base	(void)
	{
		LockWrite() ;
		if (mz->m_copy)
			{ mz->m_copy-- ; Unlock() ; }
		else
			{ mz->_clear() ; Unlock() ; delete mz ; }
		_hzGlobal_Memstats.m_numIsams-- ;
	}

	void	Start	(void)
	{
		mz = new _isam_ca() ;
		mz->isamId = ++_hzGlobal_Memstats.m_numIsams ;
		//printf("ALLOC ISAM INTERNAL %d\n", mz->isamId) ; fflush(stdout) ;
	}

	void	SetLock	(hzLockOpt eLock)
	{
		if (eLock == HZ_ATOMIC)
			mz->m_pLock = new hzLockRW() ;
		else if (eLock == HZ_MUTEX)
			mz->m_pLock = new hzLockRWD() ;
		else
			mz->m_pLock = 0 ;
	}

	void	SetName		(const hzString& name)	{ m_Name = name ; }
	void	SetDefault	(const KEY& key)		{ mz->m_Default = key ; }

	void	LockRead	(void)	{ if (mz->m_pLock) mz->m_pLock->LockRead() ; }
	void	LockWrite	(void)	{ if (mz->m_pLock) mz->m_pLock->LockWrite() ; }
	void	Unlock		(void)	{ if (mz->m_pLock) mz->m_pLock->Unlock() ; }

	void	Clear	(void)
	{
		//	Clear the map of all keys and elements
		//
		//	Begin at root and take the least pointer out to the lowest data node and pass through the list of data nodes deleting them. Then drop back one level
		//	to delete the index nodes level by level 
		//
		//	Arguments:	None
		//	Returns:	None

		LockWrite() ;
		mz->_clear() ;
		Unlock() ;
	}

	/*
	_hz_isam_Base<KEY>&	operator=	(const _hz_isam_Base<KEY>& op)
	{
		//	If this vector has the same internal data as the operand do nothing - otherwise clear this vector first and then assign to the operand.
		if (mz && mz == op.mz)
			return *this ;

		Clear() ;
		mz = op.mz ;
		m_Name = op.m_Name ;
		LockWrite() ;
		mz->m_copy++ ;
		Unlock() ;
		return *this ;
	}
	*/

	_hz_isam_dblk*	_nodetr	(uint32_t& nOset, uint32_t nPosn, bool bExpand) const
	{
		//	Purpose:	This function finds for an absolute element position, the data node and slot where the element resides. It is the first of three 'data-node
		//				locator' functions and the only one that can reside in a base ISAM as it does not rely on key comparison. These functions are critical since
		//				all access to elements depend on them.
		//
		//	Method:		Starting at the root we add up the cumulatives until they exceed the required position. Then go up to the next level and do the same.
		//				The exceptions to this are where there is no data and therefore no root or where the only node is the root.
		//
		//	Arguments:	1)	nOset	A reference to store the node offset of the element
		//				2)	nPosn	The required position
		//				3)	bExpand	Expand flag. If set function can find the end postion + 1 (set for insert, clear for lookup)
		//
		//	Returns:	Pointer to the data node for the element

		_hzfunc_ct("_hz_isam_Base::_nodetr") ;

		_hz_isam_iblk*	pIdx = 0 ;		//	Current index node
		_hz_isam_block*	pChild = 0 ;	//	Can be index node or outer level data node
		_hz_isam_dblk*	pDN = 0 ;		//	Target data node

		uint32_t	nStart = 0 ;		//	True starting position of current node as per cumulatives
		uint32_t	nMin = 0 ;			//	The real position within the collection of the current node's lowest element.
		uint32_t	nMax = 0 ;			//	The real position within the collection of the current node's lowest element plus current node usage
		uint32_t	n ;					//	Element iterator

		//	Check if there is a root
		if (!mz->m_pRoot)
			{ nOset = -1 ; return 0 ; }

		//	Check limits
		nMax = bExpand ? mz->m_pRoot->Cumulative() : mz->m_pRoot->Cumulative()-1 ;
		if (nPosn > nMax)
			{ nOset = -1 ; return 0 ; }

		//	If the root is the only node
		if (mz->m_pRoot->Level() == 0)
		{
			pDN = (_hz_isam_dblk*) mz->m_pRoot ;
			nOset = nPosn ;
			return pDN ;
		}

		//	Common case, start at root which is an index node
		pIdx = (_hz_isam_iblk*) mz->m_pRoot ;
		nMax = nMin = nStart = 0 ;

		for (nStart = 0 ; !pDN ;)
		{
			//	Look on the node for the offset/pointer to the next node or the
			nMin = nStart ;
			for (n = 0 ; n < pIdx->Usage() ; n++)
			{
				pChild = (_hz_isam_block*) pIdx->m_Ptrs[n] ;
				if (!pChild)
					Fatal("_hz_isam_Base::_nodetr: isam=%d (%s): Missing node pointer on non leaf node", mz->isamId, *m_Name) ;

				if (pChild->Parent() != pIdx)
					Fatal("_hz_isam_Base::_nodetr: isam=%d (%s): Child node %d (level %d) does not point back to current node %d (level %d)",
						mz->isamId, *m_Name, pChild->Ident(), pChild->Level(), pIdx->m_Id, pIdx->level) ;

				//	If the cumulative exceeds the desired posn we have the node
				nStart = nMin ;
				nMax = nMin + pChild->Cumulative() ;
				if (nPosn < nMax)
					break ;
				nMin = nMax ;
			}

			//	Go to next level or accept data node as target
			if (pChild->Level())
				pIdx = (_hz_isam_iblk*) pChild ;
			else
				pDN = (_hz_isam_dblk*) pChild ;
		}

		nOset = nPosn - nStart ;

		if (nOset > pDN->Usage())
			Fatal("_hz_isam_Base::_nodetr: isam=%d (%s): Node %d usage %u has offset %u", mz->isamId, *m_Name, pDN->Ident(), pDN->Usage(), nOset) ;

		return pDN ;
	}

	hzEcode	_acceptIndx	(_hz_isam_iblk* pCur, _hz_isam_block* pOld, _hz_isam_block* pNew)
	{
		//	This function is called on an index node which may have either data node pointers or index node pointers as elements. It is only called when a child node
		//	pOld spawns a ultra sibling pNew. Note that nodes never spawn infra siblings. The action is to find the slot occupied by pOld and place pNew in the
		//	next slot up. This is an insert action with all elements higher than pOld moving up one place to make room for pNew.
		//
		//	In the event the current node pCur is null this is because pOld is currently the root and has no parent. A new root is formed with pOld in slot 0 and pNew
		//	(which must be pOld's ultra node) in slot 1.
		//
		//	In the event the current node is full we first check if the infra exists and has space. If it does we move slot 0 to the top slot of the infra, move
		//	elements upto and including pOld down one place and put pNew in pOld's former slot. If the infra is full we see if a ultra has space. If not create
		//	a new ultra node. We then insert this node's top top element in the ultra's slot 0. In the special case where pOld is in the top slot of this node
		//	we just put pNew in the ultra slot 0.
		//
		//	Note that if a new ultra sibling is created a pointer to it must be entered in the parent node. _acceptIndx is called on the parent node with this node
		//	as pOld and the new sibling as pNew. If there is no parent a parent is created and both this node as the sibling added as children. This new parent node
		//	is returned. The calling function (_acceptData) then sets this as the new root to the ISAM.
		//
		//	Arguments:	1)	pCur	The pointer to the current node (parent to pOld and pNew)
		//				2)	pOld	The pointer to the pre-existing child node (prognitor)
		//				3)	pNew	The pointer to the newly created ultra sibling of the prognitor
		//
		//	Returns:	E_OK	Always

		_hzfunc_ct("_hz_isam_Base::_acceptIndx") ;

		_hz_isam_block*	pNode = 0 ;		//	Shift element
		_hz_isam_iblk*	pIdxNew = 0 ;	//	New ultra sibling
		_hz_isam_iblk*	pPar = 0 ;		//	Parent for cumulatives
		uint32_t		slot = 0 ;		//	Slot found for existing pOld
		uint32_t		n = 0 ;			//	Slot iterator
		uint32_t		za ;			//	Shift cumulative 1
		uint32_t		zb ;			//	Shift cumulative 2

		//	Verify arguments
		if (!pOld)	Fatal("_hz_isam_Base::_acceptIndx: isam=%d (%s): No pre-existing item supplied", mz->isamId, *m_Name) ;
		if (!pNew)	Fatal("_hz_isam_Base::_acceptIndx: isam=%d (%s): No new item supplied", mz->isamId, *m_Name) ;

		if (pNew->Infra() != pOld)
			Fatal("_hz_isam_Base::_acceptIndx: isam=%d (%s): new %d does not point back to old %d", mz->isamId, *m_Name, pNew->Ident(), pOld->Ident()) ;
		if (pOld->Ultra() != pNew)
			Fatal("_hz_isam_Base::_acceptIndx: isam=%d (%s): old %d does not point forward to new %d", mz->isamId, *m_Name, pOld->Ident(), pNew->Ident()) ;

		//	Do we need a new root?
		if (!pCur)
		{
			pCur = new _hz_isam_iblk() ;
			pCur->m_Ptrs[0] = pOld ;
			pCur->m_Ptrs[1] = pNew ;
			pOld->SetParent(pCur) ;
			pNew->SetParent(pCur) ;
			pCur->m_isamId = mz->isamId ;
			pCur->usage = 2 ;
			pCur->cumulative = pOld->Cumulative() + pNew->Cumulative() ;
			pCur->level = pOld->Level()+1 ;
			mz->m_pRoot = pCur ;
			return E_OK ;
		}

		//	Given we have a current node, further checks
		if ((pOld->Level()+1) != pCur->level)
			Fatal("_hz_isam_Base::_acceptIndx: isam=%d (%s): At level %d but old element level is %d", mz->isamId, *m_Name, pCur->level, pOld->Level()) ;
		if ((pNew->Level()+1) != pCur->level)
			Fatal("_hz_isam_Base::_acceptIndx: isam=%d (%s): At level %d but new element level is %d", mz->isamId, *m_Name, pCur->level, pNew->Level()) ;

		if (!pNew->Parent())
			pNew->SetParent(pCur) ;
		if (pNew->Parent() != pCur)
			Fatal("%s: isam=%d (%s): new %d does not have this node (%d) as its parent instad it has %d",
				mz->isamId, *m_Name, pNew->Ident(), pCur->m_Id, pNew->Parent()->Ident()) ;

		//	Find out which slot pOld is on
		for (slot = 0 ; slot < pCur->usage ; slot++)
		{
			if (pCur->m_Ptrs[slot] == pOld)
				break ;
		}

		if (slot == pCur->usage)
			Fatal("_hz_isam_Base::_acceptIndx: isam=%d (%s): On node %d: Child node %d not found", mz->isamId, *m_Name, pCur->m_Id, pOld->Ident()) ;
		slot++ ;

		//	If there is space on this node just insert the new element
		if (pCur->usage < HZ_ORDSZE)
		{
			for (n = pCur->usage ; n > slot ; n--)
				pCur->m_Ptrs[n] = pCur->m_Ptrs[n-1] ;
			pCur->m_Ptrs[slot] = pNew ;
			pCur->usage++ ;

			//	Update cumulatives
			pCur->cumulative += pNew->Cumulative() ;
			for (pPar = pCur->parent ; pPar ; pPar = pPar->parent)
				pPar->cumulative += pNew->Cumulative() ;
			return E_OK ;
		}

		//	This node is full so we first see if we have an infra and with space
		if (pCur->infra && pCur->infra->usage < HZ_ORDSZE)
		{
			//	Move element 0 to infra node
			pNode = pCur->m_Ptrs[0] ;
			za = pNode->Cumulative() ;
			pNode->SetParent(pCur->infra) ;
			pCur->infra->m_Ptrs[pCur->infra->usage] = pNode ;
			pCur->infra->usage++ ;

			//	Insert the new element
			slot-- ;
			for (n = 0 ; n < slot ; n++)
				pCur->m_Ptrs[n] = pCur->m_Ptrs[n+1] ;
			zb = pNew->Cumulative() ;
			pNew->SetParent(pCur) ;
			pCur->m_Ptrs[slot] = pNew ;

			//	Update cumulatives
			pCur->infra->cumulative += za ;
			pCur->cumulative += (zb - za) ;
			if (pCur->parent == pCur->infra->parent)
			{
				for (pPar = pCur->parent ; pPar ; pPar = pPar->parent)
					pPar->cumulative += zb ;
			}
			else
			{
				for (pPar = pCur->infra->parent ; pPar ; pPar = pPar->parent)
					pPar->cumulative += za ;
				for (pPar = pCur->parent ; pPar ; pPar = pPar->parent)
					pPar->cumulative += (zb - za) ;
			}
			return E_OK ;
		}

		//	No room on the infra so we try the ultra. If full or non-existant, create an empty one.
		if (!pCur->ultra || pCur->ultra->usage == HZ_ORDSZE)
		{
			pIdxNew = new _hz_isam_iblk ;
			if (!pIdxNew)
				Fatal("_hz_isam_Base::_acceptIndx: Could not allocate sibling\n") ;

			pIdxNew->m_isamId = mz->isamId ;
			pIdxNew->level = pCur->level ;
			if (pCur->ultra)
				pCur->ultra->infra = pIdxNew ;
			pIdxNew->ultra = pCur->ultra ;
			pIdxNew->infra = pCur ;
			pCur->ultra = pIdxNew ;

			//	The new sibling must be given a parent.
			_acceptIndx(pCur->parent, pCur, pIdxNew) ;
		}

		//	OK now we can boogie as there is a ultra with space and parent
		if (slot == HZ_ORDSZE)
		{
			//	pOld is in this node's top slot so pNew goes into slot 0 of ultra
			zb = pNew->Cumulative() ;
			for (n = pCur->ultra->usage ; n ; n--)
				pCur->ultra->m_Ptrs[n] = pCur->ultra->m_Ptrs[n-1] ;
			pCur->ultra->m_Ptrs[0] = pNew ;
			pNew->SetParent(pCur->ultra) ;
			pCur->ultra->usage++ ;
			pCur->ultra->cumulative += zb ;

			for (pPar = pCur->ultra->parent ; pPar ; pPar = pPar->parent)
				pPar->cumulative += zb ;
			return E_OK ;
		}

		//	Higest element of this node goes into slot 0 of ultra, then new element inserted into this node
		for (n = pCur->ultra->usage ; n ; n--)
			pCur->ultra->m_Ptrs[n] = pCur->ultra->m_Ptrs[n-1] ;
		pNode = pCur->m_Ptrs[pCur->usage-1] ;
		za = pNode->Cumulative() ;
		pNode->SetParent(pCur->ultra) ;
		pCur->ultra->m_Ptrs[0] = pNode ;
		pCur->ultra->usage++ ;
		pCur->ultra->cumulative += za ;

		//	Insert new element into this node
		zb = pNew->Cumulative() ;
		for (n = pCur->usage-1 ; n > slot ; n--)
			pCur->m_Ptrs[n] = pCur->m_Ptrs[n-1] ;
		pCur->m_Ptrs[slot] = pNew ;
		pCur->cumulative += (zb - za) ;

		if (pCur->parent == pCur->ultra->parent)
		{
			for (pPar = pCur->parent ; pPar ; pPar = pPar->parent)
				pPar->cumulative += zb ;
		}
		else
		{
			for (pPar = pCur->ultra->parent ; pPar ; pPar = pPar->parent)
				pPar->cumulative += za ;
			for (pPar = pCur->parent ; pPar ; pPar = pPar->parent)
				pPar->cumulative += (zb - za) ;
		}

		return E_OK ;
	}

	void	_expelIndx	(_hz_isam_iblk* pCur, _hz_isam_block* pChild)
	{
		//	Expel the defunct child from this node. If this action leaves the current node disposable (empty or sparse enough to migrate its elements to its siblings)
		//	the current node will be excluded from its peers by connecting the infra and ultra siblings to each other. In this event and if this node has a parent
		//	ExpelIndex is called for the parent with this node as the child to be disposed of.
		//
		//	Arguments:	1)	pCur	Parent ISAM block
		//				2)	pChild	Child ISAM block to be removed
		//
		//	Returns:	None

		_hzfunc_ct("_hz_isam_iblk::_expelIndx") ;

		_hz_isam_block*	pXfer ;		//	Element (block pointer) being moved
		_hz_isam_iblk*	pPar = 0 ;	//	Parent for cumulatives
		uint32_t		x = 0 ;		//	No of elements moved to infra
		uint32_t		y = 0 ;		//	Offset into current node for copying elements to ultra
		uint32_t		slot ;		//	Slot of element being deleted
		uint32_t		za = 0 ;	//	Shift cumulative 1
		uint32_t		zb = 0 ;	//	Shift cumulative 2

		if (!pCur)		Fatal("_hz_isam_iblk::_expelIndx: isam=%d (%s): No node supplied from which to expell", *m_Name, mz->isamId) ;
		if (!pChild)	Fatal("_hz_isam_iblk::_expelIndx: isam=%d (%s): No child node supplied to expell from node %d", mz->isamId, *m_Name, pCur->m_Id) ;

		for (slot = 0 ; slot < pCur->usage ; slot++)
		{
			if (pCur->m_Ptrs[slot] == pChild)
				break ;
		}
		if (slot == pCur->usage)
			Fatal("_hz_isam_iblk::_expelIndx: isam=%d (%s): No pointer to depricated child node found on this node (%d)", mz->isamId, *m_Name, pCur->m_Id) ;

		if (slot < 0 || slot >= pCur->usage)
			Fatal("_hz_isam_iblk::_expelIndx: isam=%d (%s): Node %d Illegal slot value (%d)", mz->isamId, *m_Name, pCur->m_Id, slot) ;

		pCur->usage-- ;
		for (x = slot, y = pCur->usage ; x < y ; x++)
			pCur->m_Ptrs[x] = pCur->m_Ptrs[x+1] ;
		pCur->m_Ptrs[pCur->usage] = 0 ;
		pCur->cumulative -= pChild->Cumulative() ;
		for (pPar = pCur->parent ; pPar ; pPar = pPar->parent)
			pPar->cumulative -= pChild->Cumulative() ;
		delete pChild ;

		if (pCur->usage && pCur->usage < HZ_ISAM_LOW)
		{
			//	Now that an entry has been removed, can we merge the current node with either or both adjacents?
			x = pCur->infra ? HZ_ORDSZE - pCur->infra->usage : 0 ;
			y = pCur->ultra ? HZ_ORDSZE - pCur->ultra->usage : 0 ;

			if ((x + y) >= pCur->usage)
			{
				//	Move all that we can to infra node first
				if (pCur->infra && pCur->infra->usage < HZ_ORDSZE)
				{
					for (x = 0 ; pCur->usage && pCur->infra->usage < HZ_ORDSZE ; x++)
					{
						pXfer = pCur->m_Ptrs[x] ;
						pXfer->SetParent(pCur->infra) ;

						pCur->infra->m_Ptrs[pCur->infra->usage] = pXfer ;
						za += pXfer->Cumulative() ;

						pCur->infra->usage++ ;
						pCur->usage-- ;
					}
					pCur->infra->cumulative += za ;

					for (pPar = pCur->infra->parent ; pPar ; pPar = pPar->parent)
						pPar->cumulative += za ;
				}

				if (pCur->ultra && pCur->ultra->usage < HZ_ORDSZE)
				{
					//	If any elements remain on the current block we need to shift upwards the elements on the ultra to accomodate them
					for (y = pCur->ultra->usage + pCur->usage - 1 ; y >= pCur->usage ; y--)
						pCur->ultra->m_Ptrs[y] = pCur->ultra->m_Ptrs[y - pCur->usage] ;

					//	Move remainder to ultra node
					for (y = 0 ; y < pCur->usage ; y++)
					{
						pXfer = pCur->m_Ptrs[x + y] ;
						pXfer->SetParent(pCur->ultra) ;

						pCur->ultra->m_Ptrs[y] = pXfer ;
						zb += pXfer->Cumulative() ;
					}
					pCur->usage = 0 ;

					pCur->ultra->cumulative += zb ;
					pCur->ultra->usage += y ;
					for (pPar = pCur->ultra->parent ; pPar ; pPar = pPar->parent)
						pPar->cumulative += zb ;
				}
			}
		}

		if (!pCur->usage)
		{
			//	Connect adjacents
			if (pCur->ultra)
				pCur->ultra->infra = pCur->infra ;
			if (pCur->infra)
				pCur->infra->ultra = pCur->ultra ;

			if (pCur->parent)
				_expelIndx(pCur->parent, pCur) ;
		}
	}

	_hz_isam_dblk*	_acceptData	(_hz_isam_dblk* pDN, const KEY& key, uint32_t& nSlot)
	{
		//	With a target data node and slot identified by _nodetr this funtion performs an insert. This has the following outcomes:-
		//
		//	1)	If the target is not full the new entry will be placed in the target at the designated slot, shifting entries above the slot up by one place.
		//	2)	If the target node is full but the target's infra node has space then one of two things will happen:-
		//		a)	If the designated slot is 0 then the new entry will be placed directly in the top slot of the infra node.
		//		b)	If the designated slot is above 0 then slot 0 of the target is moved to the top slot of the infra node, the designated slot is reduced by 1 and all
		//			slots lower than the new designated slot is shifted down one place.
		//	3)	If the target node is full and there is no room on an infra, there will always be room on an ultra node. This is because if the ultra does not exist or
		//		is full, a new ultra will be created. Then one of two things will happen:-
		//		a)	If the designated slot is HZ_ORDSZE (which is possible if _nodetr() is called with bExpand = true), then the new entry will be placed in slot 0 of
		//			the ultra node.
		//		b)	If the designated slot is < HZ_ORDSZE then the top slot of the target node is moved to slot 0 of the ultra and then the insert will proceed as in
		//			scenario (1)
		//
		//	It is essential that this function informs the called of the node and slot the new entry actually ended up in as only the key can be placed. If there are
		//	objects to be placed alongside the key, this will be done by the caller after this function has returned. To this end the actual data node is returned and
		//	nSlot, supplied as an int32_t& is set.
		//
		//	Arguments	1)	The target node identified by _nodetr()
		//				2)	The key value
		//				3)	The reference to target slot identified by _nodetr(), _findDnodeByKey() or _findAbsoluteByKey() and adjusted by this function.
		//
		//	Returns		Pointer to the actual node the new entry resides in
		//
		//	Note that the target slot can be set to HZ_ORDSZE by _nodetr(), _findDnodeByKey() or _findAbsoluteByKey() if and only if the target node is full and has
		//	no ultra. After this function the slot will always have a value between 0 and HZ_ORDSZE-1 

		_hzfunc_ct("_hz_isam_Base::_acceptData") ;

		_hz_isam_dblk*	pInfra = 0 ;	//	New data node
		_hz_isam_dblk*	pUltra = 0 ;	//	New data node
		_hz_isam_iblk*	pIdx ;			//	Index node (for updating cimulatives)

		KEY*		pKeys ;				//	Access to node's keys
		uint32_t	n ;					//	Loop controller

		//	Test inputs
		if (!pDN)
			Fatal("_hz_isam_Base::_acceptData: isam=%d (%s): No taget data node supplied", mz->isamId, *m_Name) ;

		n = pDN->Usage() ;
		if (n > HZ_ORDSZE)			Fatal("_hz_isam_Base::_acceptData: isam=%d (%s): Garbage usage value (%d)", mz->isamId, *m_Name, n) ;
		if (nSlot > HZ_ORDSZE)		Fatal("_hz_isam_Base::_acceptData: isam=%d (%s): Illegal slot value (%d)", mz->isamId, *m_Name, nSlot) ;
		if (nSlot > pDN->Usage())	Fatal("_hz_isam_Base::_acceptData: isam=%d (%s): Excessive slot value (%d)", mz->isamId, *m_Name, nSlot) ;

		mz->m_nElements++ ;

		//	If new element can be accomodated on this node then add it and return
		if (pDN->Usage() < HZ_ORDSZE)
		{
			for (n = pDN->Usage() ; n > nSlot ; n--)
				pDN->setfromCur(n, n-1) ;

			pKeys = (KEY*) pDN->Keys() ;
			pKeys[nSlot] = key ;
			pDN->IncUsage() ;

			for (pIdx = pDN->parent ; pIdx ; pIdx = pIdx->parent)
				pIdx->cumulative++ ;
			return pDN ;
		}

		//	This node is full so we first see if the first element can be moved to this node's infra, if it exists
		pInfra = (_hz_isam_dblk*) pDN->Infra() ;
		if (pInfra && pInfra->Usage() < HZ_ORDSZE)
		{
			if (nSlot == 0)
			{
				//	Store object direcly in top slot in infra node
				nSlot = pInfra->Usage() ;
				pKeys = (KEY*) pInfra->Keys() ;
				pKeys[nSlot] = key ;
				pInfra->IncUsage() ;
				for (pIdx = pInfra->parent ; pIdx ; pIdx = pIdx->parent)
					pIdx->cumulative++ ;
				return pInfra ;
			}

			//	Move element 0 to infra node, decrement slot move all elements upto slot down one place, store object in slot
			pInfra->setfromUlt(pInfra->Usage(), 0) ;
			pInfra->IncUsage() ;
			for (pIdx = pInfra->parent ; pIdx ; pIdx = pIdx->parent)
				pIdx->cumulative++ ;

			nSlot-- ;
			for (n = 0 ; n < nSlot ; n++)
				pDN->setfromCur(n, n+1) ;
			pKeys = (KEY*) pDN->Keys() ;
			pKeys[nSlot] = key ;
			return pDN ;
		}

		//	No room on the infra so we try the ultra. If there isn't one or there is but it is full, create an empty one. Note that if we create a new node at
		//	this point, we don't update the index cumulatives below. This is because we are going to return the new node so that it will be added to the parent and
		//	at that point the cumulatives on the parent index nodes will be adjusted.
		pUltra = (_hz_isam_dblk*) pDN->Ultra() ;
		if (!pUltra || pUltra->Usage() == HZ_ORDSZE)
		{
			pUltra = pDN->allocUltra() ;
			if (!pUltra)
				Fatal("_hz_isam_Base::_acceptData: Could not allocate sibling") ;

			//	Put this new node into the index
			_acceptIndx((_hz_isam_iblk*) pDN->Parent(), pDN, pUltra) ;
		}

		//	OK now we can boogie as there is a ultra with space
		for (n = pUltra->Usage() ; n > 0 ; n--)
			pUltra->setfromCur(n, n-1) ;

		if (nSlot == HZ_ORDSZE)
		{
			//	New element goes into slot 0 of ultra
			pKeys = (KEY*) pUltra->Keys() ;
			pKeys[0] = key ;
			pUltra->IncUsage() ;
			for (pIdx = pUltra->parent ; pIdx ; pIdx = pIdx->parent)
				pIdx->cumulative++ ;
			nSlot = 0 ;
			return pUltra ;
		}

		//	Higest element of this node goes into slot 0 of ultra, then new element inserted into this node
		pUltra->setfromInf(0, pDN->Usage()-1) ;
		pUltra->IncUsage() ;
		for (pIdx = pUltra->parent ; pIdx ; pIdx = pIdx->parent)
			pIdx->cumulative++ ;

		//	Insert new element into this node
		for (n = pDN->Usage()-1 ; n > nSlot ; n--)
			pDN->setfromCur(n, n-1) ;
		pKeys = (KEY*) pDN->Keys() ;
		pKeys[nSlot] = key ;
		return pDN ;
	}

	_hz_isam_dblk*	_expelData	(_hz_isam_dblk* pDN, uint32_t nSlot)
	{
		//	The node can be removed if empty or if all remaining elements can be migrated to its infra and ultra adjacents. By convention we migrate what
		//	we can to the infra adjacent first and then migrate any remaining to the ultra adjacent.
		//
		//	Arguments:	1)	pDN		The node from which an element is to be removed
		//				2)	nSlot	The slot number of the depricated element
		//
		//	Returns:	Pointer to the current node remanent
		//				NULL if the current node is emptied by this action
		//
		//	Note: In most cases, the return is ignored by the caller.

		_hzfunc_ct("_hz_isam_Base::_expelData") ;

		_hz_isam_iblk*	pPar ;		//	For setting cumulatives in index
		_hz_isam_dblk*	pInfra ;	//	For setting cumulatives in index
		_hz_isam_dblk*	pUltra ;	//	For setting cumulatives in index
		KEY*			pKeys ;		//	Keys on node
		uint32_t		x = 0 ;		//	Loop control, doubles as No of elements moved to infra
		uint32_t		y = 0 ;		//	Loop control, doubles as No of elements moved to ultra
		uint32_t		ino ;		//	ISAM number

		//	Test inputs
		if (!pDN)
			Fatal("_hz_isam_Base::_expelData: isam=%d (%s): No taget data node supplied", mz->isamId, *m_Name) ;
		ino = pDN->IsamId() ;

		x = pDN->Usage() ;
		if (x > HZ_ORDSZE)			Fatal("_hz_isam_Base::_expelData: isam=%d (%s): Garbage usage value (%d)", mz->isamId, *m_Name, x) ;
		if (nSlot > HZ_ORDSZE)		Fatal("_hz_isam_Base::_expelData: isam=%d (%s): Illegal slot value (%d)", mz->isamId, *m_Name, nSlot) ;
		if (nSlot >= pDN->Usage())	Fatal("_hz_isam_Base::_expelData: isam=%d (%s): Excessive slot value (%d)", mz->isamId, *m_Name, nSlot) ;

		mz->m_nElements-- ;

		//	Exclude the slot
		pDN->DecUsage() ;
		for (x = nSlot, y = pDN->Usage() ; x < y ; x++)
			pDN->setfromCur(x, x+1) ;
		pKeys = (KEY*) pDN->Keys() ;
		pKeys[pDN->Usage()] = mz->m_Default ;

		for (pPar = pDN->parent ; pPar ; pPar = pPar->parent)
			pPar->cumulative-- ;

		//	See if reduced node can be purged
		if (pDN->Usage() && pDN->Usage() < HZ_ISAM_LOW)
		{
			pInfra = (_hz_isam_dblk*) pDN->Infra() ;
			pUltra = (_hz_isam_dblk*) pDN->Ultra() ;

			x = pInfra ? HZ_ORDSZE - pInfra->Usage() : 0 ;
			y = pUltra ? HZ_ORDSZE - pUltra->Usage() : 0 ;

			if ((x + y) >= pDN->Usage())
			{
				//	All remaining slots on this node can be accomodated by adjacents. Begin by moving all that we can to infra node first
				x = y = 0 ;
				if (pInfra && pInfra->Usage() < HZ_ORDSZE)
				{
					for (x = 0 ; pDN->Usage() && pInfra->Usage() < HZ_ORDSZE ; x++)
					{
						pInfra->setfromUlt(pInfra->Usage(), x) ;
						pInfra->IncUsage() ;
						pDN->DecUsage() ;
					}

					for (pPar = pInfra->Parent() ; pPar ; pPar = pPar->parent)
						pPar->cumulative += x ;
					if (pInfra->IsamId() != ino || pInfra->Usage() < 0 || pInfra->Usage() > HZ_ORDSZE)
						Fatal("_hz_isam_Base::_expelData: isam=%d (%s): Infra %d Invalid: isam id (%d) usage (%d)",
							ino, *m_Name, pInfra->Ident(), pInfra->IsamId(), pInfra->Usage()) ;
				}

				if (pUltra && pDN->Usage() && pUltra->Usage() < HZ_ORDSZE)
				{
					//	If any elements remain on the current block we need to shift upwards the elements on the ultra to accomodate them
					for (y = pUltra->Usage() + pDN->Usage() - 1 ; y >= pDN->Usage() ; y--)
						pUltra->setfromCur(y, y - pDN->Usage()) ;
					pUltra->SetUsage(pUltra->Usage() + pDN->Usage()) ;

					//	Move remainder to ultra node
					for (y = 0 ; y < pDN->Usage() ; y++)
						pUltra->setfromInf(y, x + y) ;
					pDN->SetUsage(0) ;

					for (pPar = pUltra->Parent() ; pPar ; pPar = pPar->parent)
						pPar->cumulative += y ;
					if (pUltra->IsamId() != ino || pUltra->Usage() < 0 || pUltra->Usage() > HZ_ORDSZE)
						Fatal("_hz_isam_Base::_expelData: isam=%d (%s): Ultra %d Invalid: isam id (%d) usage (%d)",
							ino, *m_Name, pUltra->Ident(), pUltra->IsamId(), pUltra->Usage()) ;
				}

				x += y ;
				for (pPar = pDN->Parent() ; pPar ; pPar = pPar->parent)
					pPar->cumulative -= x ;
			}
		}

		if (pDN->Usage())
			return pDN ;

		//	Eliminate node: Connect adjacents
		pInfra = (_hz_isam_dblk*) pDN->Infra() ;
		pUltra = (_hz_isam_dblk*) pDN->Ultra() ;
		if (pUltra)
			pUltra->SetInfra(pInfra) ;
		if (pInfra)
			pInfra->SetUltra(pUltra) ;

		if (pDN->Parent())
			_expelIndx(pDN->Parent(), pDN) ;
		else
		{
			delete pDN ;
			mz->m_pRoot = 0 ;
		}

		return 0 ;
	}

	bool	_shownode	(_hz_isam_block* pNode, hzLogger* plog, bool bData) const
	{
		//	Support function to Show(). Prints out elements for a given node.
		//
		//	Arguments:	1)	pNode	The current node
		//				2)	plog	The logger to use for output
		//				3)	bData	If set, output data part of element
		//
		//	Returns:	True	If there were no integrity issues
		//				False	If there were

		_hz_isam_iblk*	pIdx = 0 ;		//	Index node
		_hz_isam_block*	pChild ;		//	Child from slot
		_hz_isam_block*	pPrev ;			//	Lower adjacent sibling
		_hz_isam_block*	pNext ;			//	Higher adjacent sibling

		uint32_t	nIndex ;			//	Iterator of children
		uint32_t	nTotal ;			//	Cumulative
		uint32_t	testA ;				//	Id of derived node
		uint32_t	testB ;				//	Id of derived node
		uint32_t	par ;				//	Parent node
		uint32_t	inf ;				//	Inferior node
		uint32_t	sup ;				//	superior node
		bool		bError = false ;	//	Error indicator

		par = pNode->Parent() ? pNode->Parent()->Ident() : -1 ;
		inf = pNode->Infra()? pNode->Infra()->Ident() : -1 ;
		sup = pNode->Ultra()? pNode->Ultra()->Ident() : -1 ;

		if (!pNode->Level())
		{
			if (bData)
				plog->Out("\treport isam=%d Data Node %d parent %d inf %d sup %d level %d, usage %2d\n",
					mz->isamId, pNode->Ident(), par, inf, sup, pNode->Level(), pNode->Usage()) ;
			return true ;
		}

		pIdx = (_hz_isam_iblk*) pNode ;
		if (bData)
			plog->Out("report isam=%d Index Node %d parent %d inf %d sup %d level %d, usage %2d cumulate %d\n",
				mz->isamId, pNode->Ident(), par, inf, sup, pNode->Level(), pNode->Usage(), pNode->Cumulative()) ;

		if (pNode->Level() < 0 || pNode->Level() > 10)
			{ plog->Out("\tOut of range level - no furthur analysis of node.\n") ; return false ; }

		if (pNode->Usage() < 0 || pNode->Usage() > HZ_ORDSZE)
			{ plog->Out("\tOut of range usage - no furthur analysis of node.\n") ; return false ; }

		//	Check pointers
		for (nIndex = 0 ; nIndex < pIdx->Usage() ; nIndex++)
		{
			pChild = pIdx->m_Ptrs[nIndex] ;
			if (!pChild)
			{
				bError = true ;
				plog->Out("\tInvalid pointer in posn %d\n", nIndex) ;
			}
		}

		//	Check cumulatives
		nTotal = 0 ;
		for (nIndex = 0 ; nIndex < pIdx->Usage() ; nIndex++)
		{
			pChild = (_hz_isam_block*) pIdx->m_Ptrs[nIndex] ;
			nTotal += pChild->Cumulative() ;
		}
		if (nTotal != pIdx->Cumulative())
		{
			bError = true ;
			plog->Out(" -- ERROR: Counted cumulative for index node %d is %d, but empirical cumulatives is %d\n", pIdx->m_Id, nTotal, pIdx->Cumulative()) ;
		}

		//	Recurse to higher nodes
		for (nIndex = 0 ; nIndex < pIdx->Usage() ; nIndex++)
		{
			pChild = (_hz_isam_block*) pIdx->m_Ptrs[nIndex] ;
			if (!_shownode(pChild, plog, bData))
				bError = true ;
		}

		//	Check integrity of this node in connection with its neighbours
		for (nIndex = 0 ; nIndex < pIdx->Usage() ; nIndex++)
		{
			pChild = (_hz_isam_block*) pIdx->m_Ptrs[nIndex] ;

			//	We are not on a data node

			if (pChild->Parent() != pNode)
			{
				bError = true ;
				plog->Out(" -- ERROR: Child in posn %d (%d) does not have this node (%d) as it's parent. Instead it has (%d)\n",
					nIndex, pChild->Ident(), pNode->Ident(), pChild->Parent()?pChild->Parent()->Ident():-1) ;
				continue ;
			}

			//	Test siblings

			if (nIndex == 0)
			{
				if (!pIdx->infra && pChild->Infra())
				{
					bError = true ;
					plog->Out(" -- ERROR: Node %d While there is no infra node to this, the zeroth child has an infra (%d)\n",
						pIdx->m_Id, pChild->Infra()->Ident()) ;
				}
				if (pIdx->infra && !pChild->Infra())
				{
					bError = true ;
					plog->Out(" -- ERROR: Node %d There is an infra to this (%d) but the zeroth child does not have an infra\n", pIdx->m_Id, pIdx->infra->m_Id) ;
				}
				if (pIdx->infra && pChild->Infra())
				{
					testA = pChild->Infra() && pChild->Infra()->Parent() ? pChild->Infra()->Parent()->Ident() : -1 ;
					testB = pIdx->infra ? pIdx->infra->m_Id : -1 ;

					if (testA != testB)
					{
						bError = true ;
						plog->Out(" -- ERROR: Node %d The parent of the zeroth child infra (%d) is not the infra of this (%d)\n",
							pIdx->m_Id, testA, testB) ;
					}
				}
			}
			else if (nIndex == (pIdx->Usage()-1))
			{
				if (!pIdx->ultra && pChild->Ultra())
					{ bError = true ; plog->Out(" -- ERROR: While there is no ultra node to this node, the highest child has a ultra\n") ; }
				if (pIdx->ultra && !pChild->Ultra())
					{ bError = true ; plog->Out(" -- ERROR: There is an ultra to this node but the highest child does not have a ultra\n") ; }
				if (pIdx->infra && pChild->Ultra())
				{
					if (pChild->Ultra()->Parent() != pIdx->ultra)
						{ bError = true ; plog->Out(" -- ERROR: The parent of the highest child ultra is not the ultra of this node\n") ; }
				}
			}
			else
			{
				pPrev = (_hz_isam_block*) pIdx->m_Ptrs[nIndex-1] ;
				pNext = (_hz_isam_block*) pIdx->m_Ptrs[nIndex+1] ;

				if (pChild->Infra() != pPrev)
					{ bError = true ; plog->Out(" -- ERROR: Child %d infra is not the same as child %d\n", nIndex, nIndex-1) ; }
				if (pChild->Ultra() != pNext)
					{ bError = true ; plog->Out(" -- ERROR: Child %d ultra is not the same as child %d\n", nIndex, nIndex+1) ; }
			}
		}

		return bError ? false : true ;
	}

	hzEcode	NodeReport	(bool bData = false) const
	{
		//	Category:	Diagnostics
		//
		//	Starting from the root node and moving out to data nodes, this checks that nodes do not contain null pointers, have correct parent pointers and have
		//	correct cumulative values
		//
		//	Arguments:	1)	bData	If set, print data object
		//
		//	Returns:	E_CORRUPT	If any node has integrity issues
		//				E_OK		If all nodes succeed

		hzLogger*	plog ;			//	Current thread logger
		hzEcode		rc = E_OK ;		//	Return code

		plog = GetThreadLogger() ;
		if (plog)
		{
			if (!mz->m_pRoot)
				{ plog->Out("The _hz_isam_Base object is empty\n") ; return E_OK ; }

			if (!_shownode(mz->m_pRoot, plog, bData))
				rc = E_CORRUPT ;
		}
		
		return rc ;
	}

	hzEcode	_integBase	(hzLogger* plog, _hz_isam_iblk* pN) const
	{
		//	Quick index node integrity test
		//
		//	Arguments:	1)	plog	Logger
		//				2)	pN		Current node
		//
		//	Returns:	E_CORRUPT	If node fails integrity test
		//				E_OK		If node passes

		_hz_isam_block*	pInf ;	//	Infra to the current node
		_hz_isam_block*	pUlt ;	//	Ultra to the current node
		_hz_isam_block*	pCA ;	//	Child node A
		_hz_isam_block*	pCB ;	//	Child node B
		_hz_isam_iblk*	par ;	//	Parent of test node

		uint32_t	n ;			//	Loop control
		uint32_t	err = 0 ;	//	Set if error

		pInf = pN->Infra() ;
		pUlt = pN->Ultra() ;

		//	Test if the infra and ultra pointers point back to current node
		if (pInf && (pInf->Ultra() != pN))	{ err=1 ; plog->Out("On node %d: Infra node %d does not point back to this node\n", pN->Ident(), pInf->Ident()) ; }
		if (pUlt && (pUlt->Infra() != pN))	{ err=1 ; plog->Out("On node %d: Infra node %d does not point back to this node\n", pN->Ident(), pInf->Ident()) ; }

		//	Test if the slot 0 child has an infra whose parent is this node's infra
		pCA = pN->m_Ptrs[0] ;
		if (pInf)
		{
			if (!pCA->Infra())
				{ err=1 ; plog->Out("On node %d: Child 0 (%d) has no infra while this node does\n", pN->Ident(), pCA->Ident()) ; }
			else
			{
				par = pCA->Infra()->Parent() ;
				if (!par)
					{ err=1 ; plog->Out("On node %d: Child 0 (%d) has parent -1 diff to the infra %d\n", pN->Ident(), pCA->Ident(), pInf->Ident()) ; }
				if (par && par != pInf)
					{ err=1 ; plog->Out("On node %d: Child 0 (%d) has parent %d diff to the infra %d\n", pN->Ident(), pCA->Ident(), par->Ident(), pInf->Ident()) ; }
			}
		}

		//	Test if the top child has an ultra whose parent is this node's ultra
		pCB = pN->m_Ptrs[pN->Usage()-1] ;
		if (pUlt)
		{
			if (!pCB->Ultra())
				{ err=1 ; plog->Out("On node %d: Child 0 (%d) has no infra while this node does\n", pN->Ident(), pCB->Ident()) ; }
			else
			{
				par = pCB->Ultra()->Parent() ;
				if (!par)
					{ err=1 ; plog->Out("On node %d: Child 0 (%d) has parent -1 diff to the infra %d\n", pN->Ident(), pCB->Ident(), pUlt->Ident()) ; }
				if (par && par != pUlt)
					{ err=1 ; plog->Out("On node %d: Child 0 (%d) has parent %d diff to the infra %d\n", pN->Ident(), pCB->Ident(), par->Ident(), pUlt->Ident()) ; }
			}
		}

		//	For each slot test if the child's infra points to the child of the next lower slot
		//	For each slot test if the child's ultra points to the child of the next higher slot
		for (n = pN->Usage()-1 ; n > 0 ; n--)
		{
			pCA = pN->m_Ptrs[n-1] ;
			if (pCA->Ultra() != pCB)
				{ err=1; plog->Out("On node %d: Child in slot %d has infra of %d but slot %d points to %d\n", pN->Ident(), n+1, pCA->InfId(), n, pCB->Ident()) ; }
			if (pCB->Infra() != pCA)
				{ err=1; plog->Out("On node %d: Child in slot %d has ultra of %d but slot %d points to %d\n", pN->Ident(), n, pCA->UltId(), n+1, pCA->Ident()) ; }
			pCB = pN->m_Ptrs[n-1] ;
		}


		//	Recurse to child nodes
		if (pN->Level() > 1)
		{
			for (n = 0 ; n < pN->Usage() ; n++)
			{
				if (_integBase(plog, (_hz_isam_iblk*) pN->m_Ptrs[n]) != E_OK)
					err=1;
			}
		}

		return err ? E_CORRUPT : E_OK ;
	}

	hzEcode	IntegBase	(void) const
	{
		//	This test that index node are consistent. For each node pointer in an index node, the node pointed to is tested to ensure it's adjacents agree with ..
		//
		//	Arguments:	None
		//
		//	Returns:	E_CORRUPT	If any ode fails integrity test
		//				E_OK		If all nodes pass

		_hz_isam_iblk*	pN ;	//	ISAM root
		hzLogger*		plog ;	//	Current thread logger

		if (!mz->m_pRoot)
			return E_OK ;
		if (!mz->m_pRoot->Level())
			return E_OK ;

		pN = (_hz_isam_iblk*) mz->m_pRoot ;
		plog = GetThreadLogger() ;
		return _integBase(plog, pN) ;
	}

	//	Set functions
	void	SetRoot		(_hz_isam_block* pN)
	{
		if (!mz)
			Fatal("ISAM %s has no root\n", *m_Name) ;
		if (mz->m_pRoot)
			Fatal("ISAM %s already has a root\n", *m_Name) ;

		mz->m_nElements = 1 ;
		mz->m_pRoot = pN ;
	}

	//void	SetInitPop	(void)	{ mz->m_nElements = 1 ; }

	//	Get functions
	KEY&		DefaultKey	(void) const	{ return mz->m_Default ; }

	_hz_isam_block*	Root	(void) const	{ return mz->m_pRoot ; }

	uint32_t	Nodes		(void) const	{ return mz->m_nNodes ; }
	uint32_t	Count		(void) const	{ return mz->m_pRoot ? mz->m_pRoot->Cumulative() : 0 ; }
	uint32_t	Cumulative	(void) const	{ return mz->m_pRoot ? mz->m_pRoot->Cumulative() : 0 ; }
	uint32_t	Population	(void) const	{ return mz->m_nElements ; }
	uint32_t	Level		(void) const	{ return mz->m_pRoot ? mz->m_pRoot->Level() : 0 ; }
	uint32_t	IsamId		(void) const	{ return mz->isamId ; }
	hzString	Name		(void) const	{ return m_Name ; }
	bool		IsInit		(void) const	{ return mz ? true : false ; }
} ;

template <class KEY>	class	_hz_isam_Relpos
{
	//	Category:	Collection Support
	//
	//	_hz_isam_Relpos	- Single datum ISAM proving insert, delete and lookup on the basis of relative positions only.

	_hz_isam_Base<KEY>	isam ;	//	Start of indes

	//	Prevent copies
	_hz_isam_Relpos<KEY>	(const _hz_isam_Relpos<KEY>&) ;
	_hz_isam_Relpos<KEY>&	operator=	(const _hz_isam_Relpos<KEY>&) ;

public:
	_hz_isam_Relpos		(void)	{}
	~_hz_isam_Relpos	(void)	{ Clear() ; }

	void	Start		(void)	{ isam.Start() ; }

	void	SetLock		(hzLockOpt eLock)		{ isam.SetLock(eLock) ; }
	void	SetName		(const hzString& name)	{ isam.SetName(name) ; }
	void	SetDefault	(const KEY& key)		{ isam.SetDefault(key) ; }

	void	Clear	(void)	{ isam.Clear() ; }

	/*
	_hz_isam_Relpos<KEY>&	operator=	(const _hz_isam_Relpos<KEY>& op)
	{
		isam = op.isam ;
		return *this ;
	}
	*/

	hzEcode	_insRP		(uint32_t nPosn, const KEY& key)
	{
		//	Purpose:	Insert an element at position.
		//
		//	Arguments:	1)	nPosn	Position to insert object
		//				2)	key		The key to insert
		//
		//	Returns:	E_OK		If operation successful
		//				E_RANGE		If position was invalid
		//				E_CORRUPT	If there were integrity issues

		_hzfunc_ct("_hz_isam_Relpos::_insRP") ;
		//	static	const char*	fn = "_hz_isam_Relpos::_insRP" ;

		_hz_isam_dblkA<KEY>*	pDN = 0 ;		//	Current data node

		uint32_t	nOset ;		//	Offset into node (set by nodetrace)
		hzEcode		rc ;		//	Return code

		//	Check args
		if (Count() != isam.Population())
			Fatal("_hz_isam_Relpos::_insRP: isam=%d Found to have wrong population: Abs %d, root cumulative %d", isam.IsamId(), isam.Population(), Count()) ;

		//	if (nPosn < 0)
		//		Fatal("%s: isam=%d: Attempt to add element at position %d. Illigal negative position", isam.IsamId(), nPosn) ;

		if (nPosn > isam.Population())
			Fatal("_hz_isam_Relpos::_insRP: isam=%d: Attempt to add element at position %d. Element set ends at %d", isam.IsamId(), nPosn, isam.Population()) ;

		//	If ordered list is empty
		if (!isam.Root())
		{
			pDN = new _hz_isam_dblkA<KEY> ;
			if (!pDN)
				Fatal("_hz_isam_Relpos::_insRP: isam=%d: Could not allocate root node", isam.IsamId()) ;

			pDN->m_Keys[0] = key ;
			pDN->m_isamId = isam.IsamId() ;
			pDN->usage = 1 ;
			isam.SetRoot(pDN) ;
			return E_OK ;
		}

		//	Locate the 'target' top level node
		pDN = (_hz_isam_dblkA<KEY>*) isam._nodetr(nOset, nPosn, true) ;
		if (!pDN)
			Fatal("_hz_isam_Relpos::_insRP: isam=%%d: No target data node found for posn %d (pop %d)", isam.IsamId(), nPosn, isam.Population()) ;
		if (nOset < 0 || nOset > HZ_ORDSZE)
			Fatal("_hz_isam_Relpos::_insRP: isam=%d: Illegal slot no %d for node %d", isam.IsamId(), nOset, pDN->m_Id) ;

		pDN = (_hz_isam_dblkA<KEY>*) isam._acceptData(pDN, key, nOset) ;

		if (Count() != isam.Population())
			Fatal("_hz_isam_Relpos::_insRP: isam=%d: Expected population %d, actual %d", isam.IsamId(), isam.Population(), Count()) ;

		return E_OK ;
	}

	hzEcode	_delRP		(uint32_t nPosn)
	{
		//	Purpose:	Delete an element at position.
		//
		//	Method:		This calls _nodetr to eastablish the target data node and slot for the requested position within the ISAM. It then calls _expelData() to delete
		//				the slot from the target node.
		//
		//	Arguments:	1)	nPosn	Position of object to delete
		//
		//	Returns:	E_OK		If operation successful
		//				E_RANGE		If position was invalid
		//				E_NODATA	If element was a null
		//				E_MEMORY	If there was insufficient memory

		_hzfunc_ct("_hz_isam_Relpos::_delRP") ;

		_hz_isam_dblkA<KEY>*	pTgt = 0 ;		//	Target data node (from which delete will occur)

		uint32_t	nOset ;		//	Position within node

		//	Check args
		if (!isam.Root())
			return E_NOTFOUND ;

		if (nPosn < 0 || nPosn >= isam.Cumulative())
			return E_RANGE ;

		//	Locate the 'target' top level node
		pTgt = (_hz_isam_dblkA<KEY>*) isam._nodetr(nOset, nPosn, false) ;
		if (!pTgt)
			Fatal("_hz_isam_Relpos::_delRP: isam=%d: No data node found in spite of position %d being within range %d",
				isam.IsamId(), nPosn, isam.Cumulative()) ;
		if (nOset >= HZ_ORDSZE)
			Fatal("_hz_isam_Relpos::_delRP: isam=%d: Illegal slot no %d for node %d", isam.IsamId(), nOset, pTgt->m_Id) ;

		//	Expel the entity from the node
		//isam.mx->m_nElements-- ;
		isam._expelData(pTgt, nOset) ;

		if (Count() != isam.Population())
			Fatal("_hz_isam_Relpos::_delRP: isam=%d: Expected population %d, actual %d", isam.IsamId(), isam.Population(), Count()) ;

		return E_OK ;
	}

	KEY&	_locatePos	(uint32_t nPosn) const
	{
		//	Arguments:	1)	nPosn	Absolute position of requested element
		//
		//	Returns:	Reference to the element at the given position or default element if the position is out of range

		_hzfunc_ct("_hz_isam_Relpos::_locatePos") ;
		//	static	const char*	fn = "_hz_isam_Relpos::_locatePos" ;

		_hz_isam_dblkA<KEY>*	pDN ;	//	Target ISAM data block
		uint32_t				nOset ;	//	Slot obtained from isam._nodetr()

		//if (!isam.mx->m_pRoot || nPosn >= isam.mx->m_pRoot->Cumulative())
		if (!isam.Root() || nPosn >= isam.Cumulative())
			//return isam.mx->m_Default ;
			return isam.DefaultKey() ;

		pDN = (_hz_isam_dblkA<KEY>*) isam._nodetr(nOset, nPosn, false) ;
		if (!pDN)
			Fatal("_hz_isam_Relpos::_locatePos: isam=%d: Data node not found for position %d", isam.IsamId(), nPosn) ;
		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Relpos::_locatePos: isam=%d: Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;
		return pDN->m_Keys[nOset] ;
	}

	const KEY&	_c_locatePos	(uint32_t nPosn) const
	{
		//	Arguments:	1)	nPosn	Absolute position of requested element
		//
		//	Returns:	Const reference to the element at the given position or default element if the position is out of range

		_hzfunc_ct("_hz_isam_Relpos::_c_locatePos") ;

		_hz_isam_dblkA<KEY>*	pDN ;	//	Target ISAM data block
		uint32_t				nOset ;	//	Slot obtained from isam._nodetr()

		if (!isam.Root() || nPosn >= isam.Cumulative())
			//return isam.mx->m_Default ;
			return isam.DefaultKey() ;

		pDN = (_hz_isam_dblkA<KEY>*) isam._nodetr(nOset, nPosn, false) ;
		if (!pDN)
			Fatal("_hz_isam_Relpos::_c_locatePos: isam=%d: Data node not found for position %d", isam.IsamId(), nPosn) ;
		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Relpos::_c_locatePos: isam=%d: Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;
		return pDN->m_Keys[nOset] ;
	}

	hzEcode	NodeReport		(bool bData)	{ isam.NodeReport(bData) ; }

	uint32_t	Count	(void) const	{ return isam.Count() ; }
	uint32_t	Level	(void) const	{ return isam.Level() ; }
	uint32_t	IsamId	(void) const	{ return isam.IsamId() ; }
	bool		IsInit	(void) const	{ return isam.IsInit() ; }
} ;

enum	_hz_sbias
{
	//	Category:	Collection Support
	//
	//	ISAM Search Bias
	//
	//	In an ISAM in which the keys must be unique, a simple binary chop comparison process will determine if any given key is present in the ISAM and if it is, give the location.
	//	However in an ISAM in which the same key can map to many objects, to iterate the objects found under the key, there has to be a method of identifying the first and the last
	//	object. Simply chopping and comparing until there is a match will not acheive this. Furthermore, in any ISAM regardless of whether keys are to be unique, the slot where the
	//	new key will go has to be identified and this can go wrong. For example, a new key could have a value that fell between that of slot 3 and 4. The comparison sequence would
	//	be 7, 3, 5 and then 4. This would be OK because slot 4 is where the new key will go with what was at slot 4 moving to slot 5. If however the new value fell between 2 and 3,
	//	the comparison sequence would be 7, 3, 1, and then 2. This is wrong as the new key needs to go in at slot 3.
	//
	//	To resolve the matter, extra steps are taken at the end of the binary chop. These differ depending on what is required. The _hz_sbias enum serves to specify the option.

	HZ_ISAMSRCH_LO,		//	Find the first instance of the key within the ISAM or fail
	HZ_ISAMSRCH_HI,		//	Find the last instance of the key within the ISAM or fail
	HZ_ISAMSRCH_END		//	Find the slot where where the key will reside. So if (b) target=(b+1) else target=(lowest slot exceeding the key).
} ;

template<class KEY>	static _hz_isam_dblk*	_findDnodeByKey
	(uint32_t& nSlot, const _hz_isam_Base<KEY>& isam, const KEY& val, int32_t (*cmpfn)(const KEY&,const KEY&), _hz_sbias eBias)
{
	//	Category:	Collection Support
	//
	//	This is the second data-node locator function and is best suited to unique key ISAMs. On the assumption the ISAM in question is in ascending
	//	order KEY value, this function supports searches of type HZ_ISAMSRCH_LO and HZ_ISAMSRCH_UIN.
	//
	//	Note this function impliments a binary chop using 'pointer forwarding'. The lookup begins with a binary chop on the root node but since the
	//	elements for comparison are found only on data nodes, when on an index node we recurively follow the pointers to the zeroth element on the
	//	next node up until we are on a data node. Then we do the compare as though the data node value was the value of the slot in the (current)
	//	index node. We then follow the winning slot up the next level and binary chop that. Eventually we get to binary chop a data node and get the
	//	actual position that either matches the key or is just above or just below the key.
	//
	//	Arguments:	1)	A reference to store the node offset of the element
	//				2)	The isam to be searched
	//				3)	The search key
	//				4)	The pointer to the compare function
	//				5)	The lookup bias (HZ_ISAMSRCH_LO/HZ_ISAMSRCH_HI)
	//
	//	Returns:	Pointer to the data node if the key is matched,
	//				NULL pointer otherwise.

	_hzfunc_ct("_findDnodeByKey") ;

	_hz_isam_dblk*	pTgt = 0 ;	//	Target data node
	_hz_isam_dblk*	pTst = 0 ;	//	Target data node
	_hz_isam_iblk*	pIdx = 0 ;	//	Current index node
	_hz_isam_iblk*	pFwd = 0 ;	//	Forward pointer

	KEY*		pKeys ;			//	Array of keys from data node
	int32_t		nRes ;			//	Comparison result
	uint32_t	nPos ;			//	Middle of index
	uint32_t	nDiv ;			//	Divider

	//	if (eBias != HZ_ISAMSRCH_LO && eBias != HZ_ISAMSRCH_UIN)
	//		Fatal("_findDnodeByKey: Illegal bias %d", eBias) ;

	if (!cmpfn)
		cmpfn = _tmpl_compare ;

	nSlot = -1 ;
	//if (!isam.mx->m_pRoot)
	if (!isam.Root())
		return 0 ;

	//	Find target data node
	//if (isam.mx->m_pRoot && isam.mx->m_pRoot->Level() == 0)
	if (isam.Root() && isam.Level() == 0)
		//pTgt = (_hz_isam_dblk*) isam.mx->m_pRoot ;
		pTgt = (_hz_isam_dblk*) isam.Root() ;
	else
	{
		//	Determine next level node
		//for (pIdx = (_hz_isam_iblk*) isam.mx->m_pRoot ; pIdx ;)
		for (pIdx = (_hz_isam_iblk*) isam.Root() ; pIdx ;)
		{
			for (nPos = 7, nDiv = 4 ;; nDiv /= 2)
			{
				if (nPos >= pIdx->usage)
					{ nPos -= nDiv ; continue ; }

				if (pIdx->level == 1)
					pTst = (_hz_isam_dblkA<KEY>*) pIdx->m_Ptrs[nPos] ;
				else
				{
					for (pFwd = (_hz_isam_iblk*) pIdx->m_Ptrs[nPos] ; pFwd->Level() > 1 ; pFwd = (_hz_isam_iblk*) pFwd->m_Ptrs[0]) ;
					pTst = (_hz_isam_dblk*) pFwd->m_Ptrs[0] ;
				}

				//	Now do the compare
				pKeys = (KEY*) pTst->Keys() ;
				nRes = cmpfn(val, pKeys[0]) ;

				if (!nRes)
					{ pTgt = pTst ; break ; }

				if (nRes < 0)
				{
					if (nPos == 1)
						{ nPos = 0 ; break ; }
					if (!nDiv)
						{ nPos-- ; break ; }
					nPos -= nDiv ;
				}
				else
				{
					if (pIdx->usage == (nPos+1))
						break ;
					if (!nDiv)
						break ;
					nPos += nDiv ;
				}
			}

			if (pTgt)
				break ;

			if (pIdx->Level() > 1)
				pIdx = (_hz_isam_iblk*) pIdx->m_Ptrs[nPos] ;
			else
				{ pTgt = (_hz_isam_dblk*) pIdx->m_Ptrs[nPos] ; break ; }
		}
	}

	if (!pTgt)
		return 0 ;

	//	Identify the offset
	pKeys = (KEY*) pTgt->Keys() ;

	for (nRes = 0, nSlot = 7, nDiv = 4 ;; nDiv /= 2)
	{
		//	Ignore positions higher than highest used slot
		if (nSlot >= pTgt->Usage())
		{
			nRes = -1 ;
			if (!nDiv)
				{ nSlot = pTgt->Usage() ; break ; }
			nSlot -= nDiv ;
			continue ;
		}

		//	Do the compare
		nRes = cmpfn(val, pKeys[nSlot]) ;
		if (nRes == 0)
			break ;

		if (!nDiv)
			break ;
		if (nRes > 0)
			nSlot += nDiv ;
		else
			nSlot -= nDiv ;
	}

	if (eBias == HZ_ISAMSRCH_LO)
	{
		//	Exact match
		if (nRes)
			return 0 ;

		if (nSlot >= pTgt->Usage())
			Fatal("_findDnodeByKey: isam=%d: Node %d usage %d has offset %d", isam.IsamId(), pTgt->Ident(), pTgt->Usage(), nSlot) ;
		return pTgt ;
	}

	//	Deal with HZ_ISAMSRCH_UIN for inserts.

	if (nRes > 0)
	{
		if (nSlot < pTgt->Usage())
			nSlot++ ;
		if (nSlot == HZ_ORDSZE && pTgt->Ultra())
		{
			pTgt = (_hz_isam_dblk*) pTgt->Ultra() ;
			nSlot = 0 ;
		}
	}

	if (nSlot > pTgt->Usage())
		Fatal("_findDnodeByKey: isam=%d: Node %d usage %d has offset %d", isam.IsamId(), pTgt->Ident(), pTgt->Usage(), nSlot) ;

	return pTgt ;
}

template<class KEY>	static _hz_isam_dblk*	_findAbsoluteByKey
	(uint32_t& nAbs, uint32_t& nSlot, const _hz_isam_Base<KEY>& isam, const KEY& val, int32_t (*cmpfn)(const KEY&, const KEY&), _hz_sbias eBias)
{
	//	Category:	Collection Support
	//
	//	This is the final data-node locator function. On the assumption the ISAM is in ascending order of KEY
	//	value, this function supports searches of types HZ_ISAMSRCH_LO, HZ_ISAMSRCH_HI and HZ_ISAMSRCH_END. It also provides the absolute position of
	//	the element in the ISAM in order to facilitate numeric iteration.
	//
	//	This function impliments a binary chop using relative positions. The ISAM is treated as an array and numerically chopped using _nodetr(). If
	//	matches are found the lowest and highest matches are recorded. The nature of the chop is such that, assuming the ISAM elements are actually
	//	ordered, once a match is found the final test position must be within one place of either the lowest or the highest match. On the first match,
	//	both the lowest and highest matches are set to the position and it is possible, if this occurs ...
	//
	//	Arguments:	1)	A reference to store the absolute position
	//				2)	A reference to store the node offset of the element
	//				3)	The isam to be searched
	//				4)	The search key
	//				5)	The pointer to the compare function
	//				6)	The lookup bias (HZ_ISAMSRCH_LO/HZ_ISAMSRCH_HI)
	//
	//	Returns:	Pointer to the data node if the key is matched,
	//				NULL pointer otherwise.
	//
	//	Note that numeric iterators are invalidated by insert and delete operations.

	_hzfunc_ct("_findAbsoluteByKey") ;

	_hz_isam_dblk*	pDN = 0 ;	//	Target data node
	_hz_isam_dblk*	pMK = 0 ;	//	Marked data node

	KEY*		pKeys ;			//	Array of keys from data node
	uint32_t	max ;			//	Top position
	uint32_t	posn ;			//	Test position
	uint32_t	posnMk ;		//	Marked position
	uint32_t	slot ;			//	Slot
	uint32_t	slotMk ;		//	Marked slot
	uint32_t	nDiv ;			//	divider
	int32_t		res ;			//	result of compare

	//	if (eBias != HZ_ISAMSRCH_LO && eBias != HZ_ISAMSRCH_HI && eBias != HZ_ISAMSRCH_END)
	//		Fatal("_findAbsoluteByKey: Illegal bias %d", eBias) ;

	nAbs = nSlot = posnMk = slotMk = 0xffffffff ;
	if (!isam.Root())
		return 0 ;

	//	Set up initial test position and the divider
	max = isam.Count()-1 ;
	for (posn = 2 ; posn < max ; posn *= 2) ;
	nDiv = posn / 2 ;
	posn-- ;

	//	Run up and down collection until we can divide no more
	for (;;)
	{
		//	Ignore positions higher than highest used slot
		if (posn > max)
		{
			res = -1 ;
			if (!nDiv)
				break ;
			posn -= nDiv ;
			nDiv /= 2 ;
			continue ;
		}

		//	Do the compare
		pDN = isam._nodetr(slot, posn, false) ;
		pKeys = (KEY*) pDN->Keys() ;
		res = cmpfn(val, pKeys[slot]) ;

		if (res == 0)
		{
			//	Have an exact match so mark it but is it the higest or lowest?
			pMK = pDN ;
			slotMk = slot ;
			posnMk = posn ;

			if (!nDiv)
				break ;

			//	After marking posn, go lower or higher according to bias
			if (eBias == HZ_ISAMSRCH_LO)
				posn -= nDiv ;
			else
				posn += nDiv ;
		}
		else
		{
			//	No match, go higher or lower if you can, otherwise fail
			if (!nDiv)
				break ;

			if (res > 0)
				posn += nDiv ;
			else
				posn -= nDiv ;
		}

		nDiv /= 2 ;
	}

	//	If looking for lowest or highest and nothing found, give up
	if (eBias == HZ_ISAMSRCH_LO || eBias == HZ_ISAMSRCH_HI)
	{
		if (!pMK)
			return 0 ;
		nAbs = posnMk ;
		nSlot = slotMk ;
		return pMK ;
	}

	//	For HZ_ISAMSRCH_END if we have found highest entry, go up one place
	if (pMK)
	{
		pDN = isam._nodetr(slot, posnMk+1, true) ;
		nAbs = posnMk+1 ;
		nSlot = slot ;
		return pDN ;
	}

	//	Otherwise go up one place ONLY if the last compare was > 0

	if (res > 0)
		posn++ ;
	if (posn > max)
		posn = max+1 ;
	pDN = isam._nodetr(slot, posn, true) ;
	nAbs = posn ;
	nSlot = slot ;
	return pDN ;
}

template <class KEY>	class	_hz_isam_Value
{
	//	Category:	Collection Support
	//
	//	_hz_isam_Value - Single datum ISAM proving insert, delete and lookup by value (effectively a set)

	_hz_isam_Base<KEY>	isam ;						//	Underlying ISAM

	int32_t	(*m_cmpfn)(const KEY&, const KEY&) ;	//	Compare function (if null this will be _tmpl_compare)

	//	Prevent copies
	_hz_isam_Value<KEY>		(const _hz_isam_Value<KEY>&) ;
	_hz_isam_Value<KEY>&	operator=	(const _hz_isam_Value<KEY>&) ;

public:
	_hz_isam_Value	(void)	{ m_cmpfn = _tmpl_compare ; }
	~_hz_isam_Value	(void)	{ Clear() ; }

	void	Start		(void)	{ isam.Start() ; }

	void	SetLock		(hzLockOpt eLock)		{ isam.SetLock(eLock) ; }
	void	SetName		(const hzString& name)	{ isam.SetName(name) ; }
	void	SetDefault	(const KEY& key)		{ isam.SetDefault(key) ; }

	void	SetCompare	(int32_t (*cmpfn)(const KEY&, const KEY&))
	{
		m_cmpfn = cmpfn ;
	}

	void	Clear	(void)	{ isam.Clear() ; }

	hzEcode	_insUV	(const KEY& key)
	{
		//	Purpose:	Insert an element by unique value.
		//
		//	Arguments:	1)	key		The key to insert
		//
		//	Returns:	E_OK		If operation successful
		//
		//	Exits:		E_CORRUPT	If a target data node and slot was not located (integrity) or a population test fails
		//				E_MEMORY	If there was insufficient memory

		_hzfunc_ct("_hz_isam_Value::_insUV") ;

		_hz_isam_dblkA<KEY>*	pDN = 0 ;		//	Current data node

		uint32_t	nOset ;			//	Offset into node (set by nodetrace)

		//	If ordered list is empty
		if (!isam.Root())
		{
			pDN = new _hz_isam_dblkA<KEY> ;
			if (!pDN)
				Fatal("_hz_isam_Value::_insUV: isam=%d: Could not allocate root node", isam.IsamId()) ;

			pDN->m_Keys[0] = key ;
			pDN->m_isamId = isam.IsamId() ;
			pDN->usage = 1 ;
			isam.SetRoot(pDN) ;
			return E_OK ;
		}

		//	Locate the 'target' top level node
		//pDN = (_hz_isam_dblkA<KEY>*) _findDnodeByKey(nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_UIN) ;
		pDN = (_hz_isam_dblkA<KEY>*) _findDnodeByKey(nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_END) ;
		if (!pDN)
			Fatal("_hz_isam_Value::_insUV: isam=%d: No data node found", isam.IsamId()) ;
		if (nOset > HZ_ORDSZE)
			Fatal("_hz_isam_Value::_insUV: isam=%d: Illegal slot no %d for node %d", isam.IsamId(), nOset, pDN->m_Id) ;

		//	If new element has identical key then ignore
		if (nOset < HZ_ORDSZE)
		{
			if (m_cmpfn(key, pDN->m_Keys[nOset]) == 0)
				return E_OK ;
		}

		//isam.mx->m_nElements++ ;
		pDN = (_hz_isam_dblkA<KEY>*) isam._acceptData(pDN, key, nOset) ;

		if (Count() != isam.Population())
			Fatal("_hz_isam_Value::_insUV: isam=%d: Expected population %d, actual %d ", isam.IsamId(), isam.Population(), Count()) ;

		if (isam.IntegBase() != E_OK)
			Fatal("_hz_isam_Value::_insUV: isam=%d: INTEGRITY FAILURE", isam.IsamId()) ;
		return E_OK ;
	}

	hzEcode	_delUV	(const KEY& key)
	{
		//	Arguments:	1)	key		The key be removed
		//
		//	Returns:	E_OK		If operation successful
		//				E_NOTFOUND	If the key was not found in the ISAM

		_hzfunc_ct("_hz_isam_Value::_delUV") ;

		_hz_isam_dblkA<KEY>*	pDN = 0 ;	//	Target data node (from which delete will occur)
		uint32_t	nOset ;					//	Position within node

		//	Check args
		if (!isam.Root())
			return E_NOTFOUND ;

		//	Locate the 'target' top level node
		pDN = (_hz_isam_dblkA<KEY>*) _findDnodeByKey(nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_LO) ;
		if (!pDN)
			return E_NOTFOUND ;

		if (nOset >= HZ_ORDSZE)
			Fatal("_hz_isam_Value::_delUV: isam=%d: Illegal slot no %d for node %d", isam.IsamId(), nOset, pDN->m_Id) ;

		//	Expel the entity from the node
		//isam.mx->m_nElements-- ;
		pDN = (_hz_isam_dblkA<KEY>*) isam._expelData(pDN, nOset) ;

		if (Count() != isam.Population())
			Fatal("_hz_isam_Value::_delUV: isam=%d: Expected population %d, actual %d ", isam.IsamId(), isam.Population(), Count()) ;
		return E_OK ;
	}

	KEY&	_locatePos	(uint32_t nPosn)
	{
		//	Arguments:	1)	nPosn	Absolute position of element
		//
		//	Returns:	Reference to the key part of the resident element (non-const) or the default key if the supplied position is out of range.

		_hzfunc_ct("_hz_isam_Value::_locatePos") ;

		_hz_isam_dblkA<KEY>*	pDN ;	//	Target ISAM data block
		uint32_t				nOset ;	//	Slot obtained from isam._nodetr()

		if (!isam.Root() || nPosn >= isam.Cumulative())
			//return isam.mx->m_Default ;
			return isam.DefaultKey() ;

		pDN = (_hz_isam_dblkA<KEY>*) isam._nodetr(nOset, nPosn, false) ;

		if (!pDN)
			Fatal("_hz_isam_Value::_locatePos: isam=%d: Data node not found for position %d", isam.IsamId(), nPosn) ;
		if (nOset < 0 || nOset >= pDN->Usage())
			Fatal("_hz_isam_Value::_locatePos: isam=%d: Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;

		return pDN->m_Keys[nOset] ;
	}

	const KEY&	_c_locatePos	(uint32_t nPosn) const
	{
		//	Arguments:	1)	nPosn	Absolute position of element
		//
		//	Returns:	Reference to the key part of the resident element (const) or the default key if the supplied position is out of range.

		_hzfunc_ct("_hz_isam_Value::_c_locatePos") ;

		_hz_isam_dblkA<KEY>*	pDN ;	//	Target ISAM data block
		uint32_t				nOset ;	//	Slot obtained from isam._nodetr()

		if (!isam.Root() || nPosn >= isam.Cumulative())
			//return isam.mx->m_Default ;
			return isam.DefaultKey() ;

		pDN = (_hz_isam_dblkA<KEY>*) isam._nodetr(nOset, nPosn, false) ;

		if (!pDN)
			Fatal("_hz_isam_Value::_c_locatePos: isam=%d: Data node not found for position %d", isam.IsamId(), nPosn) ;
		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Value::_c_locatePos: isam=%d: Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;

		return pDN->m_Keys[nOset] ;
	}

	KEY&	_locUV	(const KEY& key) const
	{
		//	Arguments:	1)	key		Key value to search for
		//
		//	Returns:	Reference to the key part of the resident element or the default key if the supplied key does not exist.

		_hzfunc_ct("_hz_isam_Value::_locUV") ;

		_hz_isam_dblkA<KEY>*	pDN ;	//	Target ISAM data block
		uint32_t				nOset ;	//	Slot obtained

		if (!isam.Root())
			//return isam.mx->m_Default ;
			return isam.DefaultKey() ;

		pDN = (_hz_isam_dblkA<KEY>*) _findDnodeByKey(nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_LO) ;
		if (!pDN)
			//return isam.mx->m_Default ;
			return isam.DefaultKey() ;

		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Value::_locUV: Isam %d Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;

		return pDN->m_Keys[nOset] ;
	}

	int32_t	_getPosByKey	(const KEY& key, _hz_sbias eBias) const
	{
		//	Arguments:	1)	key		Key to search for
		//				2)	eBias	Search bias
		//
		//	Returns:	Value being the current absolute position of the entry matching the supplied KEY and bias

		_hzfunc_ct("_hz_isam_Value::_getPosByKey") ;

		_hz_isam_dblk*	pDN ;	//	Target ISAM data block
		uint32_t		nOset ;	//	Slot obtained
		uint32_t		nAbs ;	//	Absolute position

		if (!isam.Root())
			return -1 ;

		pDN = _findAbsoluteByKey(nAbs, nOset, isam, key, m_cmpfn, eBias) ;
		if (!pDN)
			return -1 ;
		return nAbs ;
	}

	bool	_exists	(const KEY& key) const
	{
		uint32_t	nOset ;		//	Slot obtained

		return _findDnodeByKey(nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_LO) ? true : false ;
	}

	hzEcode	Integrity	(void) const
	{
		//	Test integrity of the ISAM. This was a relic of early development days but has been left in for the benefit of testing any future changes. There are
		//	no circumstances where an application should ever have to test ISAM integrity.
		//
		//	Arguments:	None
		//
		//	Returns:	E_CORRUPT	Integrity failure
		//				E_OK		Integrity test passed

		_hz_isam_dblkA<KEY>*	pA ;	//	Data block of lower position
		_hz_isam_dblkA<KEY>*	pB ;	//	Data block of higher position

		uint32_t	osetA ;				//	Data block offset of lower position
		uint32_t	osetB ;				//	Data block offset of higher position
		uint32_t	nItems ;			//	Number of elements
		uint32_t	nCount ;			//	Element iterator
		int32_t		res ;				//	Comparison result
		hzEcode		rc = E_OK ;			//	Return code

		nItems = Count() ;
		if (nItems < 2)
			return E_OK ;

		for (nCount = 1 ; rc == E_OK && nCount < nItems ; nCount++)
		{
			pA = (_hz_isam_dblkA<KEY>*) isam._nodetr(osetA, nCount-1, false) ;
			pB = (_hz_isam_dblkA<KEY>*) isam._nodetr(osetB, nCount, false) ;

			res = m_cmpfn(pB->m_Keys[osetB], pA->m_Keys[osetA]) ;

			if (!res)	{ threadLog("isam=%d pop %d - Integrity fails duplicate at posn %d\n", isam.IsamId(), Count(), nCount) ; rc = E_CORRUPT ; }
			if (res<0)	{ threadLog("isam=%d pop %d - Integrity fails order at posn %d\n", isam.IsamId(), Count(), nCount) ; rc = E_CORRUPT ; }
		}
		return rc ;
	}

	hzEcode	NodeReport	(bool bData) const	{ isam.NodeReport(bData) ; }

	uint32_t	Count	(void) const	{ return isam.Count() ; }
	uint32_t	Level	(void) const	{ return isam.Level() ; }
	uint32_t	IsamId	(void) const	{ return isam.IsamId() ; }
	bool		IsInit	(void) const	{ return isam.IsInit() ; }
} ;

template <class KEY, class OBJ>	class	_hz_isam_Pair
{
	//	Category:	Collection Support
	//
	//	_hz_isam_Pair dual datum ISAM proving insert, delete and lookup

	_hz_isam_Base<KEY>	isam ;						//	Start of index

	mutable OBJ		m_Default ;						//	Default (empty) object

	int32_t	(*m_cmpfn)(const KEY&, const KEY&) ;	//	Compare function (if null this will be _tmpl_compare)

	//	Prevent copies
	_hz_isam_Pair<KEY,OBJ>	(const _hz_isam_Pair<KEY,OBJ>&) ;
	_hz_isam_Pair<KEY,OBJ>&	operator=	(const _hz_isam_Pair<KEY,OBJ>&) ;

public:
	_hz_isam_Pair	(void)	{ memset(&m_Default, 0, sizeof(OBJ)) ; m_cmpfn = _tmpl_compare ; }
	~_hz_isam_Pair	(void)	{ Clear() ; }

	void	Start		(void)	{ isam.Start() ; }
	void	SetLock		(hzLockOpt eLock)		{ isam.SetLock(eLock) ; }
	void	SetName		(const hzString name)	{ isam.SetName(name) ; }

	void	SetCompare	(int32_t (*cmpfn)(const KEY&, const KEY&))
	{
		m_cmpfn = cmpfn ;
	}

	//void	SetDefaultKey	(const KEY& key)	{ isam.mx->m_Default = key ; }
	void	SetDefaultKey	(const KEY& key)	{ isam.SetDefault(key) ; }
	void	SetDefaultObj	(const OBJ& obj)	{ m_Default = obj ; }

	//	Clear the map of all keys and elements
	void	Clear	(void)	{ isam.Clear() ; }

	/*
	_hz_isam_Pair<KEY,OBJ>&	operator=	(const _hz_isam_Pair<KEY,OBJ>& op)
	{
		isam = op.isam ;
		m_cmpfn = op.m_cmpfn ;
		m_Default = op.m_Default ;
		return *this ;
	}
	*/

	hzEcode	_insPairU	(const KEY& key, const OBJ& obj)
	{
		//	Purpose:	Insert a unique key/object pair by key value.
		//
		//	Arguments:	1)	key		Key of object to insert
		//				2)	obj		The object
		//
		//	Returns:	E_OK		If operation successful
		//				E_CORRUPT	If some curruption occurred.

		_hzfunc_ct("_hz_isam_Pair::_insPairU") ;

		_hz_isam_dblkB<KEY,OBJ>*	pDN = 0 ;		//	Current data node

		uint32_t	nOset ;				//	Offset into node (set by nodetrace)

		//	If ordered list is empty
		if (!isam.Root())
		{
			pDN = new _hz_isam_dblkB<KEY,OBJ> ;
			if (!pDN)
				Fatal("_hz_isam_Pair::_insPairU: isam=%d: Could not allocate root node", isam.IsamId()) ;

			pDN->m_Keys[0] = key ;
			pDN->m_Objs[0] = obj ;
			pDN->m_isamId = isam.IsamId() ;
			pDN->usage = 1 ;
			isam.SetRoot(pDN) ;
			return E_OK ;
		}

		//	Locate the 'target' top level node
		pDN = (_hz_isam_dblkB<KEY,OBJ>*) _findDnodeByKey(nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_END) ;
		if (!pDN)
			Fatal("_hz_isam_Pair::_insPairU: isam=%d: No data node found", isam.IsamId()) ;

		if (nOset > HZ_ORDSZE)
			Fatal("_hz_isam_Pair::_insPairU: isam=%d: Illegal slot no %d for node %d", isam.IsamId(), nOset, pDN->m_Id) ;

		//	If new element has identical key then set the object only
		if (nOset < HZ_ORDSZE && m_cmpfn(key, pDN->m_Keys[nOset]) == 0)
		{
			pDN->m_Objs[nOset] = obj ;
			return E_OK ;
		}

		//isam.mx->m_nElements++ ;
		pDN = (_hz_isam_dblkB<KEY,OBJ>*) isam._acceptData(pDN, key, nOset) ;

		if (Count() != isam.Population())
			Fatal("_hz_isam_Pair::_insPairU: isam=%d: Expected population %d, actual %d ", isam.IsamId(), isam.Population(), Count()) ;

		pDN->m_Objs[nOset] = obj ;
		return E_OK ;
	}

	hzEcode	_insPairM	(const KEY& key, const OBJ& obj)
	{
		//	Purpose:	Insert a non-unique key/object pair by key value.
		//
		//	Arguments:	1)	key		Key of object to insert
		//				2)	obj		The object
		//
		//	Returns:	E_OK		If operation successful
		//				E_CORRUPT	If some curruption occurred.

		_hzfunc_ct("_hz_isam_Pair::_insPairM") ;

		_hz_isam_dblkB<KEY,OBJ>*	pDN = 0 ;	//	Current data node

		uint32_t	nAbs ;			//	Absolute position (multiple key operation only)
		uint32_t	nOset ;			//	Offset into node (set by nodetrace)

		//	If ordered list is empty
		if (!isam.Root())
		{
			pDN = new _hz_isam_dblkB<KEY,OBJ> ;
			if (!pDN)
				Fatal("_hz_isam_Pair::_insPairM: isam=%d: Could not allocate root node", isam.IsamId()) ;

			pDN->m_Keys[0] = key ;
			pDN->m_Objs[0] = obj ;
			pDN->usage = 1 ;
			isam.SetRoot(pDN) ;
			return E_OK ;
		}

		//	Locate the 'target' top level node
		pDN = (_hz_isam_dblkB<KEY,OBJ>*) _findAbsoluteByKey(nAbs, nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_END) ;

		if (!pDN)
			Fatal("_hz_isam_Pair::_insPairM: isam=%d: No data node found", isam.IsamId()) ;
		if (nOset < 0 || nOset > HZ_ORDSZE)
			Fatal("_hz_isam_Pair::_insPairM: isam=%d: Illegal slot no %d for node %d", isam.IsamId(), nOset, pDN->m_Id) ;

		//	Insert
		//isam.mx->m_nElements++ ;
		pDN = (_hz_isam_dblkB<KEY,OBJ>*) isam._acceptData(pDN, key, nOset) ;

		if (Count() != isam.Population())
			Fatal("_hz_isam_Pair::_insPairM: isam=%d: Expected population %d, actual %d ", isam.IsamId(), isam.Population(), Count()) ;

		pDN->m_Objs[nOset] = obj ;
		return E_OK ;
	}

	hzEcode	_delPairU	(const KEY& key)
	{
		//	Purpose:	Delete the supplied object.
		//
		//	Method:		This identifies the target data node and then calls the target node's Expel() method to delete the element. If this results in an empty
		//				node or a node which can have its remaining population accomodated in imeadiately adjacent nodes, then the node will be deleted. This in
		//				turn, unless the target data node is the root, requries Expel() to be called on the data node's parent index node. This could mean the
		//				parent node is deleted and Expel() being called on its parent etc etc. Note that if the root node's population falls to 1, it will be
		//				removed and the sole node it points to will become the new root.
		//
		//	Arguments:	1)	key		The key of the key/object pair to delete
		//
		//	Returns:	E_OK		If operation successful
		//				E_NOTFOUND	If position was invalid

		_hz_isam_dblkB<KEY,OBJ>*	pDN = 0 ;	//	Target data node (from which delete will occur)

		uint32_t	nOset ;			//	Position within node

		//	Check args
		if (!isam.Root())
			return E_NOTFOUND ;

		//	Locate the 'target' top level node
		pDN = (_hz_isam_dblkB<KEY,OBJ>*) _findDnodeByKey(nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_LO) ;
		if (!pDN)
			return E_NOTFOUND ;

		//	Expel the entity from the node
		pDN = (_hz_isam_dblkB<KEY,OBJ>*) isam._expelData(pDN, nOset) ;

		if (Count() != isam.Population())
			Fatal("_hz_isam_Pair::_delPairU: isam=%d (%s): Expected population %d, actual %d", isam.IsamId(), *isam.Name(), isam.Population(), Count()) ;

		return E_OK ;
	}

	hzEcode	_delPairByVal	(const KEY& key, const OBJ& obj)
	{
		//	STUB
		return E_OK ;
	}

	hzEcode	_delPairByPos	(int32_t nPosn)
	{
		//	STUB
		return E_OK ;
	}

	KEY&	_getkeyPos	(uint32_t nPosn)
	{
		//	Arguments:	1)	nPosn	Absolute position within collection
		//
		//	Returns:	Reference to the KEY of the element at the given position or the default key if supplied position is out of range

		_hzfunc_ct("_hz_isam_Pair::_getkeyPos") ;

		_hz_isam_dblkB<KEY,OBJ>*	pDN ;	//	Target ISAM data block
		uint32_t					nOset ;	//	Slot obtained

		if (!isam.Root())
			return isam.mx->m_Default ;
		if (nPosn >= isam.Cumulative())
			return isam.mx->m_Default ;

		pDN = (_hz_isam_dblkB<KEY,OBJ>*) isam._nodetr(nOset, nPosn, false) ;
		if (!pDN)
			Fatal("_hz_isam_Pair::_getkeyPos: isam=%d: Data node not found for position %d", isam.IsamId(), nPosn) ;

		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Pair::_getkeyPos: isam=%d: Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;

		return pDN->m_Keys[nOset] ;
	}

	const KEY&	_c_getkeyPos	(uint32_t nPosn) const
	{
		//	Arguments:	1)	nPosn	Absolute position within collection
		//
		//	Returns:	Reference to the KEY of the element at the given position or the default key if supplied position is out of range

		_hzfunc_ct("_hz_isam_Pair::_c_getkeyPos") ;

		_hz_isam_dblkB<KEY,OBJ>*	pDN ;	//	Target ISAM data block
		uint32_t					nOset ;	//	Slot obtained

		if (!isam.Root())
			//return isam.mx->m_Default ;
			return isam.DefaultKey() ;
		if (nPosn >= isam.Cumulative())
			//return isam.mx->m_Default ;
			return isam.DefaultKey() ;

		pDN = (_hz_isam_dblkB<KEY,OBJ>*) isam._nodetr(nOset, nPosn, false) ;
		if (!pDN)
			Fatal("_hz_isam_Pair::_c_getkeyPos: isam=%d: Data node not found for position %d", isam.IsamId(), nPosn) ;

		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Pair::_c_getkeyPos: isam=%d: Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;

		return pDN->m_Keys[nOset] ;
	}

	OBJ&	_locPairByPosn	(uint32_t nPosn)
	{
		//	Arguments:	1)	nPosn	Absolute position within collection
		//
		//	Returns:	Reference to the element at the position supplied. This will be the default object if the position exceeds the collection content.

		_hzfunc_ct("_hz_isam_Pair::_locPairByPosn") ;

		_hz_isam_dblkB<KEY,OBJ>*	pDN ;	//	Target ISAM data block
		uint32_t					nOset ;	//	Slot obtained

		if (!isam.Root() || nPosn >= isam.Cumulative())
			return m_Default ;

		pDN = (_hz_isam_dblkB<KEY,OBJ>*) isam._nodetr(nOset, nPosn, false) ;
		if (!pDN)
			Fatal("_hz_isam_Pair::_locPairByPosn: isam=%d: Data node not found for position %d", isam.IsamId(), nPosn) ;

		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Pair::_locPairByPosn: isam=%d: Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;

		return pDN->m_Objs[nOset] ;
	}

	const OBJ&	_c_locPairByPosn	(uint32_t nPosn) const
	{
		//	Arguments:	1)	nPosn	Absolute position within collection
		//
		//	Returns:	Reference to the element at the position supplied. This will be the default object if the position exceeds the collection content.

		_hzfunc_ct("_hz_isam_Pair::_c_locPairByPosn") ;

		_hz_isam_dblkB<KEY,OBJ>*	pDN ;	//	Target ISAM data block
		uint32_t					nOset ;	//	Slot obtained

		if (!isam.Root() || nPosn >= isam.Cumulative())
			return m_Default ;

		pDN = (_hz_isam_dblkB<KEY,OBJ>*) isam._nodetr(nOset, nPosn, false) ;
		if (!pDN)
			Fatal("_hz_isam_Pair::_c_locPairByPosn: isam=%d: Data node not found for position %d", isam.IsamId(), nPosn) ;

		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Pair::_c_locPairByPosn: isam=%d: Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;

		return pDN->m_Objs[nOset] ;
	}

	OBJ&	_locPairByKey	(const KEY& key, _hz_sbias eBias) const
	{
		//	Arguments:	1)	key		The test key
		//				2)	eBias	The lookup bias
		//
		//	Returns:	Reference to the element at the position found. This will be the default object if the key does not exist.

		_hzfunc_ct("_hz_isam_Pair::_locPairByKey") ;

		_hz_isam_dblkB<KEY,OBJ>*	pDN ;	//	Target ISAM data block
		uint32_t					nOset ;	//	Slot obtained

		if (!isam.Root())
			return m_Default ;

		pDN = (_hz_isam_dblkB<KEY,OBJ>*) _findDnodeByKey(nOset, isam, key, m_cmpfn, eBias) ;
		if (!pDN)
			return m_Default ;

		if (nOset >= pDN->Usage())
			Fatal("_hz_isam_Pair::_locPairByKey: Isam %d Data node %d: Offset is %d, usage is %d", isam.IsamId(), pDN->m_Id, nOset, pDN->Usage()) ;

		return pDN->m_Objs[nOset] ;
	}

	int32_t	_getPosByKey	(const KEY& key, _hz_sbias eBias) const
	{
		//	Arguments:	1)	key		The test key
		//				2)	eBias	The lookup bias
		//
		//	Returns:	Number being the current absolute position of the entry matching the supplied KEY and bias

		_hzfunc_ct("_hz_isam_Pair::_getPosByKey") ;

		_hz_isam_dblk*	pDN ;	//	Target ISAM data block
		uint32_t		nOset ;	//	Slot obtained
		uint32_t		nAbs ;	//	Absolute position

		if (!isam.Root())
			return -1 ;

		pDN = _findAbsoluteByKey(nAbs, nOset, isam, key, m_cmpfn, eBias) ;
		if (!pDN)
			return -1 ;
		return nAbs ;
	}

	bool	_exists	(const KEY& key) const
	{
		//	Determine if the key exists
		//
		//	Arguments:	1)	key		The test key
		//
		//	Returns:	True	If the key exists in the collection
		//				False	Otherwise

		uint32_t	nOset ;			//	Slot

		return _findDnodeByKey(nOset, isam, key, m_cmpfn, HZ_ISAMSRCH_LO) ? true : false ;
	}

	uint32_t	Count	(void) const	{ return isam.Count() ; }
	uint32_t	Level	(void) const	{ return isam.Level() ; }
	uint32_t	IsamId	(void) const	{ return isam.IsamId() ; }
	bool		IsInit	(void) const	{ return isam.IsInit() ; }

	hzEcode	NodeReport	(bool bData)	{ isam.NodeReport(bData) ; }

	hzEcode	Integrity	(void) const
	{
		//	Test integrity of the ISAM. This was a relic of early development days but has been left in for the benefit of testing any future changes. There are
		//	no circumstances where an application should ever have to test ISAM integrity.
		//
		//	Arguments:	None
		//
		//	Returns:	E_CORRUPT	Integrity failure
		//				E_OK		Integrity test passed

		_hz_isam_dblkB<KEY,OBJ>*	pA ;	//	Data block of lower position
		_hz_isam_dblkB<KEY,OBJ>*	pB ;	//	Data block of higher position

		uint32_t	osetA ;					//	Data block offset of lower position
		uint32_t	osetB ;					//	Data block offset of higher position
		uint32_t	nItems ;				//	Number of elements
		uint32_t	nCount ;				//	Element iterator
		int32_t		res ;					//	Comparison result
		hzEcode		rc = E_OK ;				//	Return code

		nItems = Count() ;
		if (nItems < 2)
			return E_OK ;

		for (nCount = 1 ; rc == E_OK && nCount < nItems ; nCount++)
		{
			pA = (_hz_isam_dblkB<KEY,OBJ>*) isam._nodetr(osetA, nCount-1, false) ;
			pB = (_hz_isam_dblkB<KEY,OBJ>*) isam._nodetr(osetB, nCount, false) ;

			res = m_cmpfn(pB->m_Keys[osetB], pA->m_Keys[osetA]) ;

			if (!res)	{ threadLog("isam=%d pop %d - Integrity fails duplicate at posn %d\n", isam.IsamId(), Count(), nCount) ; rc = E_CORRUPT ; }
			if (res<0)	{ threadLog("isam=%d pop %d - Integrity fails order at posn %d\n", isam.IsamId(), Count(), nCount) ; rc = E_CORRUPT ; }
		}
		return rc ;
	}
} ;

/*
**	Section 3	The Indexed Collection Class Templates
*/

template<class OBJ> class hzVect
{
	//	Category:	Object Collection
	//
	//	The hzVect class template provides a form of vector or an array. The elements are accessible via the [] operator but are held in an _hz_isam_Relpos ISAM rather
	//	than in a contiguous buffer. This confers the advantage of avoiding expensive reallocation in the general case where the number of elements is not known in
	//	advance but with the caveat that elements have relative rather then absolute addresses.
	//
	//	Note that as the [] operator returns a reference to an element, out of range calls must return something and to this end hzVect has a default element which is
	//	returned in this event. The default element is created by default hzVect construction and so is 'empty' but this is the only guide to the calling program that
	//	the requested element was out of range.
	//
	//	Note that in stark contrast to the vector class template of the STL, the [] operator NEVER creates elements. This must be done explicitly either by the Add()
	//	method which always places the new element at the end or the Insert() method which requires an additional argument to indicate the position.
	//
	//	Errors: The only relevant error that can occur is that of memory allocaton which currently causes imeadiate termination of the progrem.

	_hz_isam_Relpos<OBJ>	m_Data ;	//	Element store

	//	Prevent copies
	hzVect<OBJ>	(const hzVect<OBJ>&) ;
	hzVect<OBJ>&	operator=	(const hzVect<OBJ>&) ;

public:
	hzVect	(void)									{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; _hzGlobal_Memstats.m_numVectors++ ; } 
	hzVect	(hzLockOpt eLock)						{ m_Data.Start() ; m_Data.SetLock(eLock) ; _hzGlobal_Memstats.m_numVectors++ ; } 
	hzVect	(const hzString& name)					{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; m_Data.SetName(name) ; _hzGlobal_Memstats.m_numVectors++ ; }
	hzVect	(hzLockOpt eLock, const hzString& name)	{ m_Data.Start() ; m_Data.SetLock(eLock) ; m_Data.SetName(name) ; _hzGlobal_Memstats.m_numVectors++ ; }
	//	hzVect	(const hzVect<OBJ>& op)					{ m_Data = op.m_Data ; _hzGlobal_Memstats.m_numVectors++ ; }

	~hzVect	(void)	{ _hzGlobal_Memstats.m_numVectors-- ; }

	void	SetName		(const hzString& name)	{ m_Data.SetName(name) ; }
	void	SetLock		(hzLockOpt eLock)		{ m_Data.SetLock(eLock) ; }
	void	SetDefault	(OBJ dflt)	{ m_Data.SetDefault(dflt) ; }

	//	Modify data functions
	hzEcode	Add		(OBJ key)					{ return m_Data._insRP(m_Data.Count(), key) ; }
	hzEcode	Insert	(OBJ key, uint32_t nPosn)	{ return m_Data._insRP(nPosn, key) ; }
	hzEcode	Delete	(int32_t nPosn)				{ return m_Data._delRP(nPosn) ; }

	void		Clear	(void)			{ m_Data.Clear() ; }
	uint32_t	Count	(void) const	{ return m_Data.Count() ; }
	bool		IsInit	(void) const	{ return m_Data.IsInit() ; }

	const OBJ&	Element	(uint32_t nIndex) const	{ return m_Data._c_locatePos(nIndex) ; }
	OBJ&	operator[]	(uint32_t nIndex) const	{ return m_Data._locatePos(nIndex) ; }

	//	hzVect&	operator=	(const hzVect<OBJ>& op)	{ m_Data = op.m_Data ; return *this ; }
} ;

/*
**	The formal hzMapS template
*/

template<class KEY, class OBJ>	class	hzMapS
{
	//	Category:	Object Collection
	//
	//	The hzMapS template provides a memory resident one to one map of keys to objects. The keys are required to be unique and there may only be one
	//	object per key. Both the keys and the objects can be of any C++, HadronZoo or application specific type.

	_hz_isam_Pair<KEY,OBJ>	m_Data ;	//	Element store

	//	Prevent copies
	hzMapS<KEY,OBJ>		(const hzMapS<KEY,OBJ>&) ;
	hzMapS<KEY,OBJ>&	operator=	(const hzMapS<KEY,OBJ>&) ;

public:
	hzMapS	(void)									{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; _hzGlobal_Memstats.m_numSmaps++ ; } 
	hzMapS	(hzLockOpt eLock)						{ m_Data.Start() ; m_Data.SetLock(eLock) ; _hzGlobal_Memstats.m_numSmaps++ ; } 
	hzMapS	(const hzString& name)					{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; m_Data.SetName(name) ; _hzGlobal_Memstats.m_numSmaps++ ; }
	hzMapS	(hzLockOpt eLock, const hzString& name)	{ m_Data.Start() ; m_Data.SetLock(eLock) ; m_Data.SetName(name) ; _hzGlobal_Memstats.m_numSmaps++ ; }

	~hzMapS	(void)	{ _hzGlobal_Memstats.m_numSmaps-- ; }

	//	Init functions
	void	SetName		(const hzString& name)	{ m_Data.SetName(name) ; }
	void	SetLock		(hzLockOpt eLock)		{ m_Data.SetLock(eLock) ; }
	void	SetCompare	(int32_t (*cmpfn)(const KEY&, const KEY&))	{ m_Data.SetCompare(cmpfn) ; }
	void	SetDefaultObj	(OBJ obj)	{ m_Data.SetDefaultObj(obj) ; }
	void	SetDefaultKey	(KEY key)	{ m_Data.SetDefaultKey(key) ; }

	void	Clear	(void)								{ m_Data.Clear() ; }

	//	Insert and delete by key
	hzEcode	Insert	(const KEY& key, const OBJ& obj)	{ return m_Data._insPairU(key, obj) ; }
	hzEcode	Delete	(const KEY& key)					{ return m_Data._delPairU(key) ; }

	//	Locate keys or objects by position
	const OBJ&	GetObj	(int32_t nIndex) const	{ return m_Data._c_locPairByPosn(nIndex) ; }
	const KEY&	GetKey	(int32_t nIndex) const	{ return m_Data._c_getkeyPos(nIndex) ; }
	OBJ&		GetReal	(int32_t nIndex)		{ return m_Data._locPairByPosn(nIndex) ; }

	//	Locate elements by value
	bool	Exists		(const KEY& key) const	{ return m_Data._exists(key) ; }
	OBJ&	operator[]	(const KEY& key) const	{ return m_Data._locPairByKey(key, HZ_ISAMSRCH_LO) ; }

	int32_t	First		(const KEY key) const	{ return m_Data._getPosByKey(key, HZ_ISAMSRCH_LO) ; }
	int32_t	Last		(const KEY key) const	{ return m_Data._getPosByKey(key, HZ_ISAMSRCH_HI) ; }
	int32_t	Target		(const KEY key) const	{ return m_Data._getPosByKey(key, HZ_ISAMSRCH_END) ; }

	//	Diagnostics
	uint32_t	Nodes		(void) const	{ return m_Data.Nodes() ; }
	uint32_t	Count		(void) const	{ return m_Data.Count() ; }
	bool		IsInit		(void) const	{ return m_Data.IsInit() ; }
	hzEcode		Integrity	(void) const	{ return m_Data.Integrity() ; }
	hzEcode		NodeErrors	(void) const	{ return m_Data.NodeReport(true) ; }
	hzEcode		NodeReport	(void) const	{ return m_Data.NodeReport(false) ; }

	//	Assignement to another hzMap (not recomended)
	//	hzMapS&	operator=	(const hzMapS& op)	{ m_Data = op.m_Data ; return *this ; }
} ;

/*
**	The hzMapM template
*/

template<class KEY, class OBJ>	class	hzMapM
{
	//	Category:	Object Collection
	//
	//	The hzMapM template provides a memory resident one to many map of keys (class KEY) to objects (class OBJ). Although there can be many objects
	//	with the same key the hzMapM template uses the same _hz_isam_Pair ISAM as hzMapS - meaning that the key is repeated for each object associated
	//	with it. Note for any given key, objects are inserted in the order of incidence so the insert location is always one place after the last.

	_hz_isam_Pair<KEY,OBJ>	m_Data ;	//	Element store

	//	Prevent copies
	hzMapM<KEY,OBJ>		(const hzMapM<KEY,OBJ>&) ;
	hzMapM<KEY,OBJ>&	operator=	(const hzMapM<KEY,OBJ>&) ;

public:
	hzMapM	(void)									{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; _hzGlobal_Memstats.m_numMmaps++ ; } 
	hzMapM	(hzLockOpt eLock)						{ m_Data.Start() ; m_Data.SetLock(eLock) ; _hzGlobal_Memstats.m_numMmaps++ ; } 
	hzMapM	(const hzString& name)					{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; m_Data.SetName(name) ; _hzGlobal_Memstats.m_numMmaps++ ; }
	hzMapM	(hzLockOpt eLock, const hzString& name)	{ m_Data.Start() ; m_Data.SetLock(eLock) ; m_Data.SetName(name) ; _hzGlobal_Memstats.m_numMmaps++ ; }

	~hzMapM	(void)	{ _hzGlobal_Memstats.m_numMmaps-- ; }

	//	Init functions
	void	SetCompare		(int32_t (*cmpfn)(const KEY&, const KEY&))	{ m_Data.SetCompare(cmpfn) ; }

	void	SetName			(const hzString& name)	{ m_Data.SetName(name) ; }
	void	SetLock			(hzLockOpt eLock)		{ m_Data.SetLock(eLock) ; }

	void	SetDefaultObj	(OBJ obj)	{ m_Data.SetDefaultObj(obj) ; }
	void	SetDefaultKey	(KEY key)	{ m_Data.SetDefaultKey(key) ; }

	void	Clear	(void)	{ m_Data.Clear() ; }

	//	Insert and delete by key
	hzEcode	Insert	(const KEY& key, const OBJ& obj)	{ _hzfunc("hzMapM::Insert") ; return m_Data._insPairM(key, obj) ; }
	hzEcode	Delete	(const KEY& key, const OBJ& obj)	{ _hzfunc("hzMapM::Delete(1)") ; return m_Data._delPairByVal(key,obj) ; }
	hzEcode	Delete	(int32_t nPosn)						{ _hzfunc("hzMapM::Delete(2)") ; return m_Data._delPairByPos(nPosn) ; }
	//hzEcode	Insert	(const KEY& key, const OBJ& obj)	{ return m_Data._insPairM(key, obj) ; }
	//hzEcode	Delete	(const KEY& key, const OBJ& obj)	{ return m_Data._delPairByVal(key,obj) ; }
	//hzEcode	Delete	(int32_t nPosn)						{ return m_Data._delPairByPos(nPosn) ; }

	//	Locate keys or objects by position
	const OBJ&	GetObj	(int32_t nIndex) const	{ return m_Data._c_locPairByPosn(nIndex) ; }
	const KEY&	GetKey	(int32_t nIndex) const	{ return m_Data._c_getkeyPos(nIndex) ; }
	OBJ&		GetReal	(int32_t nIndex)		{ return m_Data._locPairByPosn(nIndex) ; }

	//	Locate elements by value
	bool	Exists		(const KEY& key) const	{ return m_Data._exists(key) ; }
	OBJ&	operator[]	(const KEY& key) const	{ return m_Data._locPairByKey(key, HZ_ISAMSRCH_LO) ; }

	//	Iteration support functions
	int32_t	First		(const KEY key) const	{ return m_Data._getPosByKey(key, HZ_ISAMSRCH_LO) ; }
	int32_t	Last		(const KEY key) const	{ return m_Data._getPosByKey(key, HZ_ISAMSRCH_HI) ; }
	int32_t	Target		(const KEY key) const	{ return m_Data._getPosByKey(key, HZ_ISAMSRCH_END) ; }

	//	Diagnostics
	uint32_t	Nodes		(void) const	{ return m_Data.Nodes() ; }
	uint32_t	Count		(void) const	{ return m_Data.Count() ; }
	bool		IsInit		(void) const	{ return m_Data.IsInit() ; }
	hzEcode		Integrity	(void) const	{ return m_Data.Integrity() ; }
	hzEcode		NodeErrors	(void) const	{ return m_Data.NodeReport(true) ; }
	hzEcode		NodeReport	(void) const	{ return m_Data.NodeReport(false) ; }
} ;

template<class OBJ>	class	hzLookup
{
	//	Category:	Object Collection
	//
	//	The hzLookup template is a special case of a hzMapS providing a memory resident one to one map of strings (hzString) to objects. The strings are
	//	required to be unique and there may only be one object per key. Only the objects can be of any C++, HadronZoo or application specific type.
	//
	//	hzLookup is considered UNORDERED because it uses the fast string compare function (fscompare). This function compares strings as though they
	//	were arrays of int64_t and thus only gives a valid lexical comparison in big-endian CPU architectures.

	_hz_isam_Pair<hzString,OBJ>	m_Data ;	//	Element store

	//	Prevent copies
	hzLookup<OBJ>	(const hzLookup<OBJ>&) ;
	hzLookup<OBJ>&	operator=	(const hzLookup<OBJ>&) ;

public:
	hzLookup	(void)									{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; m_Data.SetCompare(StringCompareF) ; _hzGlobal_Memstats.m_numMmaps++ ; } 
	hzLookup	(hzLockOpt eLock)						{ m_Data.Start() ; m_Data.SetLock(eLock) ; m_Data.SetCompare(StringCompareF) ; _hzGlobal_Memstats.m_numMmaps++ ; } 
	hzLookup	(const hzString& name)					{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; m_Data.SetName(name) ; m_Data.SetCompare(StringCompareF) ; _hzGlobal_Memstats.m_numMmaps++ ; }
	hzLookup	(hzLockOpt eLock, const hzString& name)	{ m_Data.Start() ; m_Data.SetLock(eLock) ; m_Data.SetName(name) ; m_Data.SetCompare(StringCompareF) ; _hzGlobal_Memstats.m_numMmaps++ ; }

	~hzLookup	(void)	{ _hzGlobal_Memstats.m_numSpmaps-- ; }

	//	Init functions
	void	SetName			(const hzString& name)	{ m_Data.SetName(name) ; }
	void	SetLock			(hzLockOpt eLock)		{ m_Data.SetLock(eLock) ; }

	void	SetDefaultObj	(OBJ obj)	{ m_Data.SetDefaultObj(obj) ; }

	void	Clear	(void)	{ m_Data.Clear() ; }

	//	Insert and delete by key
	hzEcode	Insert	(const hzString& key, const OBJ& obj)	{ return m_Data._insPairU(key, obj) ; }
	hzEcode	Delete	(const hzString& key)					{ return m_Data._delPairU(key) ; }

	//	Locate keys or objects by position
	const OBJ&		GetObj	(int32_t nIndex) const	{ return m_Data._c_locPairByPosn(nIndex) ; }
	const hzString	GetKey	(int32_t nIndex) const	{ return m_Data._c_getkeyPos(nIndex) ; }
	OBJ&			GetReal	(int32_t nIndex)		{ return m_Data._locPairByPosn(nIndex) ; }

	//	Locate elements by value
	bool	Exists		(const hzString& key) const	{ return m_Data._exists(key) ; }
	OBJ&	operator[]	(const hzString& key) const	{ return m_Data._locPairByKey(key, HZ_ISAMSRCH_LO) ; }

	//	Diagnostics
	uint32_t	Nodes		(void) const	{ return m_Data.Nodes() ; }
	uint32_t	Count		(void) const	{ return m_Data.Count() ; }
	bool		IsInit		(void) const	{ return m_Data.IsInit() ; }
} ;

/*
**	The hzSet template
*/

template<class KEY>	class	hzSet
{
	//	Category:	Object Collection
	//
	//	The hzSet template is a memory resident set of unique objects. It is similar to hzMapS except that the objects serve as thier own keys. Note that the objects
	//	must have the comparison operators implimented.

	_hz_isam_Value<KEY>	m_Data ;	//	_hz_isam_Value ordered list by value

	//	Prevent copies
	hzSet<KEY>	(const hzSet<KEY>&) ;
	hzSet<KEY>& operator=	(const hzSet<KEY>&) ;

public:
	hzSet	(void)									{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; _hzGlobal_Memstats.m_numSets++ ; } 
	hzSet	(hzLockOpt eLock)						{ m_Data.Start() ; m_Data.SetLock(eLock) ; _hzGlobal_Memstats.m_numSets++ ; } 
	hzSet	(const hzString& name)					{ m_Data.Start() ; m_Data.SetLock(HZ_NOLOCK) ; m_Data.SetName(name) ; _hzGlobal_Memstats.m_numSets++ ; }
	hzSet	(hzLockOpt eLock, const hzString& name)	{ m_Data.Start() ; m_Data.SetLock(eLock) ; m_Data.SetName(name) ; _hzGlobal_Memstats.m_numSets++ ; }

	~hzSet	(void)	{ _hzGlobal_Memstats.m_numSets-- ; }

	//	Inits
	void	SetCompare	(int32_t (*cmpfn)(const KEY&, const KEY&))	{ m_Data.SetCompare(cmpfn) ; }

	void	SetName		(const hzString& name)	{ m_Data.SetName(name) ; }
	void	SetLock		(hzLockOpt eLock)		{ m_Data.SetLock(eLock) ; }

	void	SetDefault	(KEY val)	{ m_Data.SetDefault(val) ; }

	//	Data operations
	void	Clear	(void)				{ m_Data.Clear() ; }
	//	hzEcode	Insert	(const KEY& key)	{ _hzfunc("hzSet::Insert") ; return m_Data._insUV(key) ; }
	//	hzEcode	Delete	(const KEY& key)	{ _hzfunc("hzSet::Insert") ; return m_Data._delUV(key) ; }
	hzEcode	Insert	(const KEY& key)	{ return m_Data._insUV(key) ; }
	hzEcode	Delete	(const KEY& key)	{ return m_Data._delUV(key) ; }

	//	Lookup
	const KEY&	GetObj		(uint32_t nIndex) const	{ return m_Data._c_locatePos(nIndex) ; }
	KEY&		GetReal		(uint32_t nIndex)		{ return m_Data._locatePos(nIndex) ; }
	bool		Exists		(const KEY& key) const	{ return m_Data._exists(key) ; }
	KEY&		operator[]	(const KEY& key) const	{ return m_Data._locUV(key) ; }

	//	Diagnostics
	uint32_t	Nodes		(void) const	{ return m_Data.Nodes() ; }
	uint32_t	Count		(void) const	{ return m_Data.Count() ; }
	bool		IsInit		(void) const	{ return m_Data.IsInit() ; }
	hzEcode		Integrity	(void) const	{ return m_Data.Integrity() ; }
	hzEcode		NodeErrors	(void) const	{ return m_Data.NodeReport(true) ; }
	hzEcode		NodeReport	(void) const	{ return m_Data.NodeReport(false) ; }
} ;

#endif	//	hzCtmpls_h
