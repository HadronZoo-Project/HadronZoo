//
//	File:	hzIsamT.h
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

#ifndef hzIsamT_h
#define hzIsamT_h

/*
**	Definitions
*/

#define HZ_T_ISAMNODE_FULL	8			//	Maximum node population
#define HZ_T_ISAMNODE_HALF	4			//	Maximum node population
#define HZ_T_ISAMNODE_LOW	5			//	Minimum node population
#define HZ_T_BADSLOT		0xffffffff	//	Invalid node slot

enum	_hz_sbias
{
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

class	_hz_vn_Idx ;		//	Index node

class	_hz_vn_Any
{
	//	Base class for vector index and data nodes

public:
	virtual	~_hz_vn_Any	(void)	{}

	virtual	_hz_vn_Idx*	Parent		(void) = 0 ;
	virtual	_hz_vn_Any*	Infra		(void) = 0 ;
	virtual	_hz_vn_Any*	Ultra		(void) = 0 ;
	virtual	uint32_t	Cumulative	(void) = 0 ;
	virtual	uint32_t	Level		(void) = 0 ;
	virtual	uint32_t	IsamId		(void) = 0 ;
	virtual	int32_t		Usage		(void) = 0 ;
	virtual	void		SetParent	(_hz_vn_Idx*) = 0 ;
} ;

class	_hz_vn_Idx	: public _hz_vn_Any
{
	//	Vector index nodes

	/*
	NOTE the future version: All nodes (data & index), and index buckets, will be 64 bytes and reside in a dedicated area with 32-bit addressing.
 
	_tnodeAddr	parent ;				//	Parent node
	_tnodeAddr	infra ;					//	Infra adjacent
	_tnodeAddr	ultra ;					//	Ultra adjacent
	hzLockS		m_lock ;				//	Data node lock
	uint32_t	m_nCumulative ;			//	Number of elements ultimately pointed to by this block
	uint16_t	m_isamId ;				//	Ties the node to the ISAM
	uint8_t		usage ;					//	Number of elements in this block
	uint8_t		level ;					//	Level of this block in the tree
	_hz_vn_Any*	m_Buckets[5] ;			//	Pointers either to index or data nodes
	uint32		m_BktCumulatives[5] ;	//	Cumulatives for each bucket
	*/
public:
	_hz_vn_Idx*	parent ;						//	Parent node
	_hz_vn_Idx*	infra ;							//	Infra adjacent
	_hz_vn_Idx*	ultra ;							//	Ultra adjacent
	_hz_vn_Any*	m_Ptrs[HZ_T_ISAMNODE_FULL] ;	//	Pointers either to index or data nodes
	uint32_t	m_nCumulative ;					//	Number of elements ultimately pointed to by this block
	uint16_t	m_isamId ;						//	Ties the node to the ISAM
	uint16_t	level ;							//	Level of this block in the tree
	int16_t		usage ;							//	Number of elements in this block
	uint16_t	resv ;							//	Reserved

	_hz_vn_Idx	(void)
	{
		_hzGlobal_Memstats.m_numIsamIndx++ ;
		m_Ptrs[0] = m_Ptrs[1] = m_Ptrs[2] = m_Ptrs[3] = m_Ptrs[4] = m_Ptrs[5] = m_Ptrs[6] = m_Ptrs[7] = 0 ; 
		parent = infra = ultra = 0 ;
		m_nCumulative = 0 ;
		m_isamId = usage = level = resv = 0 ;
	}

	~_hz_vn_Idx	(void)
	{
		_hzGlobal_Memstats.m_numIsamIndx-- ;
	}

	//	Set methods
	void	SetCumulatives	(void) ;

	void	SetParent	(_hz_vn_Idx* par)	{ parent = par ; }
	void	SetLevel	(int32_t v)			{ level = v ; }

	//	Get methods
	_hz_vn_Idx*	Parent	(void)	{ return parent ; }
	_hz_vn_Idx*	Infra	(void)	{ return infra ; }
	_hz_vn_Idx*	Ultra	(void)	{ return ultra ; }

	uint32_t	Cumulative	(void)	{ return m_nCumulative ; }
	uint32_t	Level		(void)	{ return level ; }
	uint32_t	IsamId		(void)	{ return m_isamId ; }
	int32_t		Usage		(void)	{ return usage ; }
} ;

class	_hz_vn_Dat	: public _hz_vn_Any
{
	//	Vector data nodes

	/*
	NOTE the future version: All nodes (data & index), and index buckets, will be 64 bytes and reside in a dedicated area with 32-bit addressing. Data buckets will be as they are
	now so the data buckets require normal 64-bit pointers. This leaves space for 5 buckets (40 data objects) as set out below
 
	_tnodeAddr	parent ;			//	Parent node
	_tnodeAddr	infra ;				//	Infra adjacent
	_tnodeAddr	ultra ;				//	Ultra adjacent
	hzLockS		m_lock ;			//	Data node lock
	uint32_t	m_Id ;				//	Unique node id (for diagnostics)
	uint16_t	m_isamId ;			//	Ties node to ISAM
	uint16_t	usage ;				//	Number of elements in this block
	uchar*		m_pElements[5] ;	//	Pointers to bucket of elements
	*/

public:
	_hz_vn_Idx*	parent ;		//	Parent node
	_hz_vn_Dat*	infra ;			//	Infra adjacent
	_hz_vn_Dat*	ultra ;			//	Ultra adjacent
	uchar*		m_pElements ;	//	Pointers to bucket of elements
	uint32_t	m_Id ;			//	Unique node id (for diagnostics)
	uint16_t	m_isamId ;		//	Ties node to ISAM
	int16_t		usage ;			//	Number of elements in this block

	_hz_vn_Dat	(void)
	{
		m_Id = ++_hzGlobal_Memstats.m_numIsamData ;
		parent = 0 ;
		infra = ultra = 0 ;
		m_pElements = 0 ;
		m_isamId = usage = 0 ;
	}

	~_hz_vn_Dat	(void)
	{
		if (m_pElements)
			delete[] m_pElements ;
		_hzGlobal_Memstats.m_numIsamData-- ;
	}

	//	Get values
	_hz_vn_Idx*	Parent		(void)	{ return parent ; }
	_hz_vn_Dat*	Infra		(void)	{ return infra ; }
	_hz_vn_Dat*	Ultra		(void)	{ return ultra ; }

	uint32_t	Cumulative	(void)	{ return usage ; }
	uint32_t	Level		(void)	{ return 0 ; }
	uint32_t	IsamId		(void)	{ return m_isamId ; }
	uint32_t	Ident		(void)	{ return m_Id ; }
	uint32_t	InfId		(void)	{ return infra ? infra->m_Id : 0 ; }
	uint32_t	UltId		(void)	{ return ultra ? ultra->m_Id : 0 ; }
	int32_t		Usage		(void)	{ return usage ; }

	void*	Keys		(void)	{ return m_pElements ; }

	//	Set values
	void	SetParent	(_hz_vn_Idx* par)	{ parent = par ; }
	void	SetInfra	(_hz_vn_Dat* pInf)	{ infra = (_hz_vn_Dat*) pInf ; }
	void	SetUltra	(_hz_vn_Dat* pUlt)	{ ultra = (_hz_vn_Dat*) pUlt ; }

	void	IncUsage	(void)			{ usage++ ; }
	void	DecUsage	(void)			{ usage-- ; }
	void	SetUsage	(uint32_t u)	{ usage = u ; }
} ;

/*
**	The Vector Template itself!
*/

class	_hz_tmpl_ISAM
{
	//	Category:	Object Collection
	//
	//	The hzVect class template provides a form of vector or an array. The elements are accessible via the [] operator but are held in an _hz_isam_vect ISAM rather
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

	_hz_vn_Any*	m_pRoot ;		//	Start of index
	hzLocker*	m_pLock ;		//	Locking (off by default)
	hzString	m_Name ;		//	Optional ISAM name (all ISAM dependent templates use this)
	uint32_t	m_isamId ;		//	For diagnostics
	uint32_t	m_nNodes ;		//	Number of nodes (also as node id allocator)
	uint32_t	m_nElements ;	//	Number of data objects
	uint16_t	m_nSizeKey ;	//	Key size
	uint16_t	m_nSizeObj ;	//	Object size

	void	_clearnode	(_hz_vn_Any* pBlk)
	{
		//	Recursively clear supplied node and any child nodes

		_hz_vn_Idx*	pIdx ;	//	Current ISAM block pointer
		int32_t		n ;		//	ISAM block selector

		if (!pBlk->Level())
			delete pBlk ;
		else
		{
			pIdx = (_hz_vn_Idx*) pBlk ;

			for (n = 0 ; n < pIdx->usage ; n++)
				_clearnode(pIdx->m_Ptrs[n]) ;
			delete pIdx ;
		}
	}

	void	_clear	(void)
	{
		//	Clear the map of all keys and elements. Calls recursive _clearnode() function on the root node.

		if (!m_pRoot)
			return ;
		_clearnode(m_pRoot) ;
		m_nElements = 0 ;
		m_pRoot = 0 ;
	}

	//	Prevent copies
	_hz_tmpl_ISAM	(const _hz_tmpl_ISAM&) ;
	_hz_tmpl_ISAM&	operator=	(const _hz_tmpl_ISAM&) ;

public:
	int32_t		(*m_compare)	(const void* pKey, const _hz_vn_Dat* pDN, uint32_t nSlot) ;

	_hz_tmpl_ISAM	(void)
	{
		m_pRoot = 0 ;
		m_pLock = 0 ;
		m_nNodes = m_nElements = 0 ;
		m_nSizeKey = m_nSizeObj = 0 ;
		m_isamId = ++_hzGlobal_Memstats.m_numIsams ;
	}

	~_hz_tmpl_ISAM	(void)	{ _hzGlobal_Memstats.m_numIsams-- ; }

	//	Initialization
	void	Start		(uint32_t nSizeKey, uint32_t nSizeObj)
	{
		m_nSizeKey = nSizeKey ;
		m_nSizeObj = nSizeObj ;
	}

	void	SetName		(const hzString& name)	{ m_Name = name ; }

	void	SetLock	(hzLockOpt eLock)
	{
		if (eLock == HZ_ATOMIC)
			m_pLock = new hzLockRW() ;
		else if (eLock == HZ_MUTEX)
			m_pLock = new hzLockRWD() ;
		else
			m_pLock = 0 ;
	}

	void	Clear	(void)
	{
		//	Clear the map of all keys and elements: Begin at root and take the least pointer out to the lowest data node and pass through the list of data nodes deleting them. Then
		//	drop back one level to delete the index nodes level by level 
		//
		//	Arguments:	None
		//	Returns:	None

		LockWrite() ;
			_clear() ;
		Unlock() ;
	}

	void	LockRead	(void)	{ if (m_pLock) m_pLock->LockRead() ; }
	void	LockWrite	(void)	{ if (m_pLock) m_pLock->LockWrite() ; }
	void	Unlock		(void)	{ if (m_pLock) m_pLock->Unlock() ; }

	//	Internal functions
	void		_moveElement	(_hz_vn_Dat* pNodeTgt, _hz_vn_Dat* pNodeSrc, uint32_t tgtSlot, uint32_t srcSlot) ;
	_hz_vn_Dat*	_allocDataSlot	(_hz_vn_Dat* pDN, int32_t& nSlot) ;
	_hz_vn_Dat*	_expelDataSlot	(_hz_vn_Dat* pDN, int32_t nSlot) ;
	_hz_vn_Dat*	_findDnodeByPos	(int32_t& nSlot, uint32_t nPosn, bool bExpand) const ;
	_hz_vn_Dat*	_findDnodeByKey	(int32_t& nSlot, const void* pKey, _hz_sbias eBias) const ;
	_hz_vn_Dat*	_findAllByKey	(int32_t& nSlot, uint32_t& nPosn, const void* pKey, _hz_sbias eBias) const ;
	hzEcode		_allocIndxSlot	(_hz_vn_Any* pOld, _hz_vn_Any* pNew) ;
	void		_expelIndxSlot	(_hz_vn_Idx* pCur, _hz_vn_Any* pChild) ;

	//	Data Operations
	_hz_vn_Dat*	InsertPosn	(int32_t& nSlot, uint32_t nPosn) ;
	hzEcode		DeletePosn	(uint32_t nPosn) ;
	_hz_vn_Dat*	InsertKeyU	(int32_t& nSlot, const void* pKey) ;
	_hz_vn_Dat*	InsertKeyM	(int32_t& nSlot, const void* pKey) ;
	_hz_vn_Dat*	DeleteKey	(int32_t& nSlot, const void* pKey) ;

	//	Diagnostics
	bool		_shownode	(_hz_vn_Any* pNode, hzLogger* plog, bool bData) const ;
	hzEcode		_integBase	(hzLogger* plog, _hz_vn_Idx* pN) const ;
	hzEcode		NodeReport	(bool bData = false) const ;
	hzEcode		IntegBase	(void) const ;

	//	Get info
	_hz_vn_Any*	Root		(void) const	{ return m_pRoot ; }

	uint32_t	Nodes		(void) const	{ return m_nNodes ; }
	uint32_t	Count		(void) const	{ return m_pRoot ? m_pRoot->Cumulative() : 0 ; }
	uint32_t	Cumulative	(void) const	{ return m_pRoot ? m_pRoot->Cumulative() : 0 ; }
	uint32_t	Population	(void) const	{ return m_nElements ; }
	uint32_t	Level		(void) const	{ return m_pRoot ? m_pRoot->Level() : 0 ; }
	uint32_t	IsamId		(void) const	{ return m_isamId ; }
	hzString	Name		(void) const	{ return m_Name ; }
} ;

/*
**	Standard Function Templates
*/

template <class KEY>	class _hz_set_bkt
{
	//	Data bucket, held by data nodes in a set of keys.

public:
	KEY		m_Keys[HZ_T_ISAMNODE_FULL] ;	//	Elements
} ;

template <class KEY, class OBJ>	class _hz_map_bkt
{
	//	Data bucket, held by data nodes in a map of key/object pairs.

public:
	KEY		m_Keys[HZ_T_ISAMNODE_FULL] ;	//	Keys
	OBJ		m_Objs[HZ_T_ISAMNODE_FULL] ;	//	Objects
} ;

template<class KEY> int32_t _tmpl_set_compare	(const void* pKey, const _hz_vn_Dat* pDN, uint32_t nSlot)
{
	//	Called internally by _hz_tmpl_ISAM::_findDnodeByKey in the case of a set (keys only).

	_hz_set_bkt<KEY>*	pBuck ;
	KEY*				key ;

	key = (KEY*) pKey ;
	pBuck = (_hz_set_bkt<KEY>*) pDN->m_pElements ;

	return *key > pBuck->m_Keys[nSlot] ? 1 : *key < pBuck->m_Keys[nSlot] ? -1 : 0 ;
}

template<class KEY, class OBJ> int32_t _tmpl_map_compare	(const void* pKey, const _hz_vn_Dat* pDN, uint32_t nSlot)
{
	//	Called internally by _hz_tmpl_ISAM::_findDnodeByKey in the case of a map (key/object pairs).

	_hz_map_bkt<KEY,OBJ>*	pBuck ;

	KEY*	key ;

	key = (KEY*) pKey ;
	pBuck = (_hz_map_bkt<KEY,OBJ>*) pDN->m_pElements ;

	return *key > pBuck->m_Keys[nSlot] ? 1 : *key < pBuck->m_Keys[nSlot] ? -1 : 0 ;
}

#endif	//	hzIsamT_h
