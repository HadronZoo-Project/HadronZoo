//
//	File:	hzTmplVect.h
//
//	Legal Notice: This file is part of the HadronZoo C++ Class Library.
//
//	Copyright 1998, 2021 HadronZoo Project (http://www.hadronzoo.com)
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

#ifndef hzTmplVect_h
#define hzTmplVect_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"
#include "hzIsamT.h"

/*
**	The Vector Template
*/

template	<class OBJ>	class	hzVect
{
	_hz_tmpl_ISAM	base ;			//	The base index
	OBJ				m_Null ;		//	NULL Object
	mutable OBJ		m_Default ;		//	Default (null) object

	//	Prevent copies
	hzVect<OBJ>	(const hzVect<OBJ>&) ;
	hzVect<OBJ>&	operator=	(const hzVect<OBJ>&) ;

public:
	hzVect	(void)
	{
		base.Start(sizeof(OBJ), 0) ;
		base.SetLock(HZ_NOLOCK) ;
		memset(&m_Null, 0, sizeof(OBJ)) ;
		_hzGlobal_Memstats.m_numVectors++ ;
	} 

	hzVect	(hzLockOpt eLock)
	{
		base.Start(sizeof(OBJ), 0) ;
		base.SetLock(eLock) ;
		memset(&m_Null, 0, sizeof(OBJ)) ;
		_hzGlobal_Memstats.m_numVectors++ ;
	} 

	hzVect	(const hzString& name)
	{
		base.Start(sizeof(OBJ), 0) ;
		base.SetLock(HZ_NOLOCK) ;
		base.SetName(name) ;
		memset(&m_Null, 0, sizeof(OBJ)) ;
		_hzGlobal_Memstats.m_numVectors++ ;
	}

	hzVect	(hzLockOpt eLock, const hzString& name)
	{
		base.Start(sizeof(OBJ), 0) ;
		base.SetLock(eLock) ;
		base.SetName(name) ;
		memset(&m_Null, 0, sizeof(OBJ)) ;
		_hzGlobal_Memstats.m_numVectors++ ;
	}

	~hzVect	(void)
	{
		_hzGlobal_Memstats.m_numVectors-- ;
	}

	//	Set functions
	void	SetLock		(hzLockOpt eLock)		{ base.SetLock(eLock) ; }
	void	SetName		(const hzString& name)	{ base.SetName(name) ; }
	void	LockRead	(void)					{ base.LockRead() ; }
	void	LockWrite	(void)					{ base.LockWrite() ; }
	void	Unlock		(void)					{ base.Unlock() ; }

	//	Get functions
	uint32_t	Count	(void) const	{ return base.Count() ; }
	uint32_t	Nodes	(void) const	{ return base.Nodes() ; }
	uint32_t	Level	(void) const	{ return base.Level() ; }
	uint32_t	IsamId	(void) const	{ return base.IsamId() ; }
	uint32_t	Name	(void) const	{ return base.Name() ; }

	//	Modify data functions
	hzEcode	Add		(OBJ key)
	{
		_hzfunc("hzVect::Add") ;

		_hz_set_bkt<OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base.InsertPosn(nSlot, Count()) ;
		if (pDN)
		{
			pBuck = (_hz_set_bkt<OBJ>*) pDN->m_pElements ;
			pBuck->m_Keys[nSlot] = key ;
			return E_OK ;
		}
		return E_CORRUPT ;
	}

	hzEcode	Insert	(OBJ key, uint32_t nPosn)
	{
		_hzfunc("hzVect::Insert") ;

		_hz_set_bkt<OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base.InsertPosn(nSlot, nPosn) ;
		if (pDN)
		{
			pBuck = (_hz_set_bkt<OBJ>*) pDN->m_pElements ;
			pBuck->m_Keys[nSlot] = key ;
			return E_OK ;
		}
		return E_CORRUPT ;
	}

	hzEcode	Delete	(uint32_t nPosn)
	{
		_hzfunc("hzVect::Delete") ;

		_hz_set_bkt<OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByPos(nSlot, nPosn, false) ;
		if (pDN)
		{
			pBuck = (_hz_set_bkt<OBJ>*) pDN->m_pElements ;
			pBuck->m_Keys[nSlot] = m_Null ;
		}

		return base.DeletePosn(nPosn) ;
	}

	void	Clear	(void)		{ base.Clear() ; }

	OBJ&	operator[]	(uint32_t nIndex) const
	{
		_hz_set_bkt<OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByPos(nSlot, nIndex, false) ;
		if (!pDN)
		{
			m_Default = m_Null ;
			return m_Default ;
		}

		pBuck = (_hz_set_bkt<OBJ>*) pDN->m_pElements ;
		return pBuck->m_Keys[nSlot] ;
	}
} ;

#endif	//	hzIsamVect_h
