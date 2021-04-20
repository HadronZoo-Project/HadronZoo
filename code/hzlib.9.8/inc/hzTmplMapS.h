//
//	File:	hzTmplMapS.h
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

#ifndef hzTmplMapS_h
#define hzTmplMapS_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"
#include "hzIsamT.h"

/*
**	The formal hzMapS template
*/

template<class KEY, class OBJ>	class	hzMapS
{
	//	Category:	Object Collection
	//
	//	The hzMapS template provides a memory resident one to one map of keys to objects. The keys are required to be unique and there may only be one
	//	object per key. Both the keys and the objects can be of any C++, HadronZoo or application specific type.

	_hz_tmpl_ISAM	base ;			//	_hz_set_isam_Value ordered list by value
	KEY				m_NullKey ;		//	Null key
	OBJ				m_NullObj ;		//	Null key
	mutable KEY		m_DefaultKey ;	//	Default key (effectively NULL)
	mutable OBJ		m_DefaultObj ;	//	Default key (effectively NULL)

	//	Prevent copies
	hzMapS<KEY,OBJ>		(const hzMapS<KEY,OBJ>&) ;
	hzMapS<KEY,OBJ>&	operator=	(const hzMapS<KEY,OBJ>&) ;

public:
	hzMapS	(void)
	{
		base.Start(sizeof(KEY), sizeof(OBJ)) ;
		base.SetLock(HZ_NOLOCK) ;
		base.m_compare = _tmpl_map_compare<KEY,OBJ> ;
		memset(&m_NullKey, 0, sizeof(KEY)) ;
		memset(&m_NullObj, 0, sizeof(OBJ)) ;
		_hzGlobal_Memstats.m_numSmaps++ ;
	} 

	hzMapS	(hzLockOpt eLock)
	{
		base.Start(sizeof(KEY), sizeof(OBJ)) ;
		base.SetLock(eLock) ;
		base.m_compare = _tmpl_map_compare<KEY,OBJ> ;
		memset(&m_NullKey, 0, sizeof(KEY)) ;
		memset(&m_NullObj, 0, sizeof(OBJ)) ;
		_hzGlobal_Memstats.m_numSmaps++ ;
	} 

	hzMapS	(const hzString& name)
	{
		base.Start(sizeof(KEY), sizeof(OBJ)) ;
		base.SetLock(HZ_NOLOCK) ;
		base.SetName(name) ;
		base.m_compare = _tmpl_map_compare<KEY,OBJ> ;
		memset(&m_NullKey, 0, sizeof(KEY)) ;
		memset(&m_NullObj, 0, sizeof(OBJ)) ;
		_hzGlobal_Memstats.m_numSmaps++ ;
	}

	hzMapS	(hzLockOpt eLock, const hzString& name)
	{
		base.Start(sizeof(KEY), sizeof(OBJ)) ;
		base.SetLock(eLock) ;
		base.SetName(name) ;
		base.m_compare = _tmpl_map_compare<KEY,OBJ> ;
		memset(&m_NullKey, 0, sizeof(KEY)) ;
		memset(&m_NullObj, 0, sizeof(OBJ)) ;
		_hzGlobal_Memstats.m_numSmaps++ ;
	}

	~hzMapS	(void)	{ _hzGlobal_Memstats.m_numSmaps-- ; }

	//	Init functions
	void	SetName	(const hzString& name)	{ base.SetName(name) ; }
	void	Clear	(void)					{ base.Clear() ; }

	//	Insert and delete by key
	hzEcode	Insert	(const KEY& key, const OBJ& obj)
	{
		_hzfunc("hzMapS::Insert") ;

		_hz_map_bkt<KEY,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base.InsertKeyU(nSlot, &key) ;
		if (pDN)
		{
			pBuck = (_hz_map_bkt<KEY,OBJ>*) pDN->m_pElements ;
			pBuck->m_Keys[nSlot] = key ;
			pBuck->m_Objs[nSlot] = obj ;
			return E_OK ;
		}
		threadLog("%s: Failed to INSERT\n", *_fn) ;
		return E_CORRUPT ;
	}

	hzEcode	Delete	(const KEY& key)
	{
		_hzfunc("hzMapS:Delete") ;

		_hz_map_bkt<KEY,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (pDN)
			return E_NOTFOUND ;

		pBuck = (_hz_map_bkt<KEY,OBJ>*) pDN->m_pElements ;
		pBuck->m_Keys[nSlot] = m_NullKey ;
		pBuck->m_Objs[nSlot] = m_NullObj ;
		pDN = base.DeleteKey(nSlot, &key) ;

		return E_OK ;
	}

	//	Locate keys or objects by position
	OBJ&	GetObj	(uint32_t nIndex) const
	{
		_hz_map_bkt<KEY,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByPos(nSlot, nIndex, false) ;
		if (!pDN)
		{
			m_DefaultObj = m_NullObj ;
			return m_DefaultObj ;
		}

		pBuck = (_hz_map_bkt<KEY,OBJ>*) pDN->m_pElements ;
		return pBuck->m_Objs[nSlot] ;
	}

	KEY&	GetKey	(uint32_t nIndex) const
	{
		_hz_map_bkt<KEY,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByPos(nSlot, nIndex, false) ;
		if (!pDN)
		{
			m_DefaultKey = m_NullKey ;
			return m_DefaultKey ;
		}

		pBuck = (_hz_map_bkt<KEY,OBJ>*) pDN->m_pElements ;
		return pBuck->m_Keys[nSlot] ;
	}

	//	Locate elements by value
	bool	Exists		(const KEY& key) const
	{
		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (!pDN)
			return false ;
		return true ;
	}

	OBJ&	operator[]	(const KEY& key) const
	{
		_hz_map_bkt<KEY,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (!pDN)
		{
			m_DefaultObj = m_NullObj ;
			return m_DefaultObj ;
		}

		pBuck = (_hz_map_bkt<KEY,OBJ>*) pDN->m_pElements ;
		return pBuck->m_Objs[nSlot] ;
	}

	int32_t	First		(const KEY key) const
	{
		uint32_t	nPosn ;
		int32_t		nSlot ;

		if (base._findAllByKey(nSlot, nPosn, key, HZ_ISAMSRCH_LO))
			return 0x7fffffff & nPosn ;
		return -1 ;
	}

	int32_t	Last		(const KEY key) const
	{
		uint32_t	nPosn ;
		int32_t		nSlot ;

		if (base._findAllByKey(nSlot, nPosn, key, HZ_ISAMSRCH_HI))
			return 0x7fffffff & nPosn ;
		return -1 ;
	}

	int32_t	Target		(const KEY key) const
	{
		uint32_t	nPosn ;
		int32_t		nSlot ;

		if (base._findAllByKey(nSlot, nPosn, key, HZ_ISAMSRCH_END))
			return 0x7fffffff & nPosn ;
		return -1 ;
	}

	//	Diagnostics
	uint32_t	Nodes		(void) const	{ return base.Nodes() ; }
	uint32_t	Count		(void) const	{ return base.Count() ; }
	hzEcode		NodeErrors	(void) const	{ return base.NodeReport(true) ; }
	hzEcode		NodeReport	(void) const	{ return base.NodeReport(false) ; }
} ;

#endif	//	hzTmplMapS_h
