//
//	File:	hzTmplMapL.h
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

#ifndef hzTmplMapL_h
#define hzTmplMapL_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"
#include "hzIsamT.h"

/*
**	The hzLookup template
*/

template<class OBJ> int32_t _fast_str_compare   (const void* pKey, const _hz_vn_Dat* pDN, uint32_t nSlot)
{
    //  Called internally by _hz_tmpl_ISAM::_findDnodeByKey in the case of a set (keys only).

    _hz_map_bkt<hzString,OBJ>*   pBuck ;

    hzString*	key ;

    key = (hzString*) pKey ;
    pBuck = (_hz_map_bkt<hzString,OBJ>*) pDN->m_pElements ;

    return *key > pBuck->m_Keys[nSlot] ? 1 : *key < pBuck->m_Keys[nSlot] ? -1 : 0 ;
}

template<class OBJ>	class	hzLookup
{
	//	Category:	Object Collection
	//
	//	The hzLookup template is a special case of a hzMapS providing a memory resident one to one map of strings (hzString) to objects. The strings are
	//	required to be unique and there may only be one object per key. Only the objects can be of any C++, HadronZoo or application specific type.
	//
	//	hzLookup is considered UNORDERED because it uses the fast string compare function (fscompare). This function compares strings as though they
	//	were arrays of int64_t and thus only gives a valid lexical comparison in big-endian CPU architectures.

	_hz_tmpl_ISAM	base ;			//	_hz_set_isam_Value ordered list by value
	hzString		m_NullKey ;		//	Null key
	_m hzString		m_DefaultKey ;	//	Default key (effectively NULL)
	OBJ				m_NullObj ;		//	Null key
	mutable OBJ		m_DefaultObj ;	//	Default key (effectively NULL)

	//	Prevent copies
	hzLookup<OBJ>	(const hzLookup<OBJ>&) ;
	hzLookup<OBJ>&	operator=	(const hzLookup<OBJ>&) ;

public:
	hzLookup	(void)
	{
		base.Start(sizeof(hzString),sizeof(OBJ)) ;
		base.SetLock(HZ_NOLOCK) ;
		base.m_compare = _fast_str_compare<OBJ> ;
		_hzGlobal_Memstats.m_numMmaps++ ;
	} 

	hzLookup	(hzLockOpt eLock)
	{
		base.Start(sizeof(hzString),sizeof(OBJ)) ;
		base.SetLock(eLock) ;
		base.m_compare = _fast_str_compare<OBJ> ;
		_hzGlobal_Memstats.m_numMmaps++ ;
	} 

	hzLookup	(const hzString& name)
	{
		base.Start(sizeof(hzString),sizeof(OBJ)) ;
		base.SetLock(HZ_NOLOCK) ;
		base.SetName(name) ;
		base.m_compare = _fast_str_compare<OBJ> ;
		_hzGlobal_Memstats.m_numMmaps++ ;
	}

	hzLookup	(hzLockOpt eLock, const hzString& name)
	{
		base.Start(sizeof(hzString),sizeof(OBJ)) ;
		base.SetLock(eLock) ;
		base.SetName(name) ;
		base.m_compare = _fast_str_compare<OBJ> ;
		_hzGlobal_Memstats.m_numMmaps++ ;
	}

	~hzLookup	(void)	{ _hzGlobal_Memstats.m_numSpmaps-- ; }

	//	Init functions
	void	SetName			(const hzString& name)	{ base.SetName(name) ; }
	void	SetLock			(hzLockOpt eLock)		{ base.SetLock(eLock) ; }

	//void	SetDefaultObj	(OBJ obj)	{ base.SetDefaultObj(obj) ; }

	void	Clear	(void)	{ base.Clear() ; }

	//	Insert and delete by key
	hzEcode	Insert	(const hzString& key, const OBJ& obj)
	{
		_hzfunc("hzMapS::Insert") ;

		_hz_map_bkt<hzString,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base.InsertKeyU(nSlot, &key) ;
		if (pDN)
		{
			pBuck = (_hz_map_bkt<hzString,OBJ>*) pDN->m_pElements ;
			pBuck->m_Keys[nSlot] = key ;
			pBuck->m_Objs[nSlot] = obj ;
			return E_OK ;
		}

		threadLog("%s: Failed to INSERT\n", *_fn) ;
		return E_CORRUPT ;
	}

	hzEcode	Delete	(const hzString& key)
	{
		_hzfunc("hzMapS:Delete") ;

		_hz_map_bkt<hzString,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (pDN)
			return E_NOTFOUND ;

		pBuck = (_hz_map_bkt<hzString,OBJ>*) pDN->m_pElements ;
		pBuck->m_Keys[nSlot] = m_NullKey ;
		pBuck->m_Objs[nSlot] = m_NullObj ;
		pDN = base.DeleteKey(nSlot, &key) ;

		return E_OK ;
	}

	//	Locate keys or objects by position
	OBJ&	GetObj	(int32_t nIndex) const
	{
		_hz_map_bkt<hzString,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByPos(nSlot, nIndex, false) ;
		if (!pDN)
		{
			m_DefaultObj = m_NullObj ;
			return m_DefaultObj ;
		}

		pBuck = (_hz_map_bkt<hzString,OBJ>*) pDN->m_pElements ;
		return pBuck->m_Objs[nSlot] ;
	}

	const hzString&	GetKey	(int32_t nIndex) const
	{
		_hz_map_bkt<hzString,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByPos(nSlot, nIndex, false) ;
		if (!pDN)
		{
			m_DefaultKey = m_NullKey ;
			return m_DefaultKey ;
		}

		pBuck = (_hz_map_bkt<hzString,OBJ>*) pDN->m_pElements ;
		return pBuck->m_Keys[nSlot] ;
	}

	//	Locate elements by value
	bool	Exists		(const hzString& key) const
	{
		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (!pDN)
			return false ;
		return true ;
	}

	OBJ&	operator[]	(const hzString& key) const
	{
		_hz_map_bkt<hzString,OBJ>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (!pDN)
		{
			m_DefaultObj = m_NullObj ;
			return m_DefaultObj ;
		}

		pBuck = (_hz_map_bkt<hzString,OBJ>*) pDN->m_pElements ;
		return pBuck->m_Objs[nSlot] ;
	}

	//	Diagnostics
	uint32_t	Nodes		(void) const	{ return base.Nodes() ; }
	uint32_t	Count		(void) const	{ return base.Count() ; }
	hzEcode		NodeErrors	(void) const	{ return base.NodeReport(true) ; }
	hzEcode		NodeReport	(void) const	{ return base.NodeReport(false) ; }
} ;

#endif	//	hzTmplMapL_h
