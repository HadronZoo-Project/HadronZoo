//
//	File:	hzTmplQue.h
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

#ifndef hzTmplQue_h
#define hzTmplQue_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"

/*
**	Prototypes
*/

void	Fatal	(const char* va_alist ...) ;

template <class OBJ>	class	_hz_queitem
{
	//	Category:	Collection Support
	//
	//	List element containing an element and a pointer to the next element. For use in the unordered list class templates, hzList, hzQue and hzStack.

public:
	_hz_queitem*	next ;		//	Next item in list
	OBJ				m_Obj ;		//	Object in list

	_hz_queitem	(OBJ obj)	{ next = 0 ; m_Obj = obj ; }
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

		_hz_queitem<OBJ>*	m_pList ;	//	Start of que
		_hz_queitem<OBJ>*	m_pLast ;	//	End of que

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
		_hz_queitem<OBJ>*	pC ;	//	Curreent list item
		_hz_queitem<OBJ>*	pN ;	//	Next list item

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

		_hz_queitem<OBJ>*	pTL ;	//	List item to accomodate new object

		if (!(pTL = new _hz_queitem<OBJ>(obj)))
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

		_hz_queitem<OBJ>*	pTL ;	//	Object-link iterator

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

		_hz_queitem<OBJ>*	pTL ;	//	Current link
		_hz_queitem<OBJ>*	pPL ;	//	Previous link

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

#endif	//	hzTmplQue_h
