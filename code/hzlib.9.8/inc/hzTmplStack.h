//
//	File:	hzTmplStack.h
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

#ifndef hzTmplStack_h
#define hzTmplStack_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"

/*
**	Prototypes
*/

void	Fatal	(const char* va_alist ...) ;

/*
**	Definitions
*/

template <class OBJ>	class	_hz_stackitem
{
	//	Category:	Collection Support
	//
	//	List element containing an element and a pointer to the next element. For use in the unordered list class templates, hzList, hzQue and hzStack.

public:
	_hz_stackitem*	next ;		//	Next item in list
	OBJ				m_Obj ;		//	Object in list

	_hz_stackitem	(OBJ obj)	{ next = 0 ; m_Obj = obj ; }
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

		_hz_stackitem<OBJ>*	m_pList ;	//	Start of list of objects in stack
		_hz_stackitem<OBJ>*	m_pLast ;	//	Pointer to last object in the stack

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

		_hz_stackitem<OBJ>*	pC ;	//	Current object pointer
		_hz_stackitem<OBJ>*	pN ;	//	Next object in series

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

		_hz_stackitem<OBJ>*	pTL ;	//	New object pointer

		if (!(pTL = new _hz_stackitem<OBJ>(obj)))
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

		_hz_stackitem<OBJ>*	pTL ;	//	Object-link iterator

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

		_hz_stackitem<OBJ>*	pTL ;	//	Current link
		_hz_stackitem<OBJ>*	pPL ;	//	Previous link

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

#endif	//	hzTmplStack_h
