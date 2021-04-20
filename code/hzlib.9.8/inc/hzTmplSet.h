//
//	File:	hzTmplSet.h
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

#ifndef hzTmplSet_h
#define hzTmplSet_h

#include "hzString.h"
#include "hzLock.h"
#include "hzProcess.h"
#include "hzIsamT.h"

/*
**	The hzSet template
*/

template<class KEY>	class	hzSet
{
	//	Category:	Object Collection
	//
	//	The hzSet template is a memory resident set of unique objects. It is similar to hzMapS except that the objects serve as thier own keys. Note that the objects
	//	must have the comparison operators implimented.

	_hz_tmpl_ISAM	base ;			//	_hz_set_isam_Value ordered list by value
	KEY				m_Null ;		//	Null key
	mutable KEY		m_Default ;		//	Default key (effectively NULL)

	//	Prevent copies
	hzSet<KEY>	(const hzSet<KEY>&) ;
	hzSet<KEY>& operator=	(const hzSet<KEY>&) ;

public:
	hzSet	(void)
	{
		base.Start(sizeof(KEY), 0) ;
		base.SetLock(HZ_NOLOCK) ;
		base.m_compare = _tmpl_set_compare<KEY> ;
		memset(&m_Null, 0, sizeof(KEY)) ;
		_hzGlobal_Memstats.m_numSets++ ;
	} 

	hzSet	(hzLockOpt eLock)
	{
		base.Start(sizeof(KEY), 0) ;
		base.SetLock(eLock) ;
		base.m_compare = _tmpl_set_compare<KEY> ;
		memset(&m_Null, 0, sizeof(KEY)) ;
		_hzGlobal_Memstats.m_numSets++ ;
	} 

	hzSet	(const hzString& name)
	{
		base.Start(sizeof(KEY), 0) ;
		base.SetLock(HZ_NOLOCK) ;
		base.SetName(name) ;
		base.m_compare = _tmpl_set_compare<KEY> ;
		memset(&m_Null, 0, sizeof(KEY)) ;
		_hzGlobal_Memstats.m_numSets++ ;
	}

	hzSet	(hzLockOpt eLock, const hzString& name)
	{
		base.Start(sizeof(KEY), 0) ;
		base.SetLock(eLock) ;
		base.SetName(name) ;
		base.m_compare = _tmpl_set_compare<KEY> ;
		memset(&m_Null, 0, sizeof(KEY)) ;
		_hzGlobal_Memstats.m_numSets++ ;
	}

	~hzSet	(void)	{ _hzGlobal_Memstats.m_numSets-- ; }

	//	Inits
	void	SetName		(const hzString& name)	{ base.SetName(name) ; }

	//	Data operations
	void	Clear	(void)	{ base.Clear() ; }

	hzEcode Insert	(const KEY& key)
	{
		_hzfunc("hzSet::Insert") ;

		_hz_set_bkt<KEY>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base.InsertKeyU(nSlot, &key) ;
		if (pDN)
		{
			pBuck = (_hz_set_bkt<KEY>*) pDN->m_pElements ;
			pBuck->m_Keys[nSlot] = key ;
			return E_OK ;
		}
		return E_CORRUPT ;
	}

	hzEcode Delete	(const KEY& key)
	{
		_hzfunc("hzSet::Delete") ;

		_hz_set_bkt<KEY>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (pDN)
			return E_NOTFOUND ;

		pBuck = (_hz_set_bkt<KEY>*) pDN->m_pElements ;
		pBuck->m_Keys[nSlot] = m_Null ;
		pDN = base.DeleteKey(nSlot, &key) ;

		return E_OK ;
	}

	//	Lookup
	bool	Exists	(const KEY& key) const
	{
		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (!pDN)
			return false ;
		return true ;
	}

	KEY&	GetObj	(uint32_t nIndex) const
	{
		_hz_set_bkt<KEY>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByPos(nSlot, nIndex, false) ;
		if (!pDN)
		{
			m_Default = m_Null ;
			return m_Default ;
		}

		pBuck = (_hz_set_bkt<KEY>*) pDN->m_pElements ;
		return pBuck->m_Keys[nSlot] ;
	}

	KEY&	operator[]	(const KEY& key) const
	{
		_hz_set_bkt<KEY>*	pBuck ;

		_hz_vn_Dat*	pDN ;
		int32_t		nSlot ;

		pDN = base._findDnodeByKey(nSlot, &key, HZ_ISAMSRCH_LO) ;
		if (!pDN)
		{
			m_Default = m_Null ;
			return m_Default ;
		}

		pBuck = (_hz_set_bkt<KEY>*) pDN->m_pElements ;
		return pBuck->m_Keys[nSlot] ;
	}

	//	Diagnostics
	uint32_t	Nodes		(void) const	{ return base.Nodes() ; }
	uint32_t	Count		(void) const	{ return base.Count() ; }
	hzEcode		NodeErrors	(void) const	{ return base.NodeReport(true) ; }
	hzEcode		NodeReport	(void) const	{ return base.NodeReport(false) ; }
} ;

#endif	//	hzTmplSet_h
