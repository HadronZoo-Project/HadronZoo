//
//	File:	hzTmplList.h
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

#ifndef hzTmplList_h
#define hzTmplList_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"

/*
**	Prototypes
*/

void	Fatal	(const char* va_alist ...) ;

/*
**	Section 1	Simple Collections
*/

#define HZ_LIST_NODESIZE	8

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

	//	Prevent copies
	//	hzList<OBJ>		(const hzList<OBJ>&) ;
	//	hzList<OBJ>&	operator=	(const hzList<OBJ>&) ;

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

#endif	//	hzTmplList_h
