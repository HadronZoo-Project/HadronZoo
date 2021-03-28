//
//	File:	hdbObjCache.cpp
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

#include <iostream>
#include <fstream>
#include <cstdio>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hzBasedefs.h"
#include "hzString.h"
#include "hzChars.h"
#include "hzChain.h"
#include "hzDate.h"
#include "hzTextproc.h"
#include "hzCtmpls.h"
#include "hzCodec.h"
#include "hzDocument.h"
#include "hzDirectory.h"
#include "hzDatabase.h"
//#include "hzFsTbl.h"
#include "hzDelta.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Definitions
*/

#define	CACHE_LITMUS	0x01	//	The member is using a litmus bit (via m_Info -> m_nLitmus)
#define	CACHE_FIXED		0x02	//	The member is using fixed space (via m_Info -> m_nOset)
#define	CACHE_LIST		0x04	//	The member is using fixed space (via m_Info -> m_nList)

/*
**	Variables
*/

extern	hzDeltaClient*	_hzGlobal_DeltaClient ;		//	Total of current delta clients

/*
**	Prototypes
*/

uint32_t	Datatype2Size		(hdbBasetype eType) ;
const char*	_hds_showinitstate	(hdbIniStat nState) ;
void		_hdb_ck_initstate	(const hzFuncname& _fn, const hzString& objName, hdbIniStat eActual, hdbIniStat eExpect) ;

/*
**	SECTION 1: hdbObjCache Functions
*/

void	hdbObjCache::_cache_blk::_clear	(void)
{
	//	Clear entire matrix (and thus cache)
	//
	//	Arguments:	None
	//	Returns:	None

	for (uint32_t n = 0 ; n < m_Ram.Count() ; n++)
		delete [] (char*) m_Ram[n] ;

	m_Ram.Clear() ;
}

hzEcode	hdbObjCache::_cache_blk::InitMatrix	(const hdbClass* pClass)
{
	_hzfunc("hdbObjCache::_cache_blk::InitMatrix") ;

	const hdbMember*	pMbr ;		//	Member pointer
	uint32_t		mbrNo ;		//	Member number
	uint32_t		oset ;		//	Offset counter
	uint32_t		nLitmus ;	//	Litmus allocator
	uint16_t		nFixed ;	//	List allocator
	uint16_t		scen ;		//	Pop Scenario flag
	bool			bComp ;		//	Compulsory
	bool			bMult ;		//	Muktiple
	hzEcode			rc ;		//	Returm code

	//	Check class initstate
	if (!pClass)
		hzexit(_fn, 0, E_ARGUMENT, "No class supplied") ;

	if (!pClass->IsInit())
		hzexit(_fn, 0, E_ARGUMENT, "Class not initialized") ;

	m_pClass = pClass ;

	m_Info	= new _mbr_data[m_pClass->MbrCount()] ;
	memset(m_Info, 0, sizeof(_mbr_data) * m_pClass->MbrCount()) ;

	nLitmus = 1 ;
	nFixed = 0 ;
	m_nBlklists = 0 ;

	//	Consider members
	for (mbrNo = 0 ; mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		pMbr = m_pClass->GetMember(mbrNo) ;

		//	Population secanario number
		if (pMbr->MinPop() == 1 && pMbr->MaxPop() == 1)	{ scen = 1 ; bComp = true ;  bMult = false ; }
		if (pMbr->MinPop() == 1 && pMbr->MaxPop() > 1)	{ scen = 2 ; bComp = true ;  bMult = true ; }
		if (pMbr->MinPop() == 0 && pMbr->MaxPop() == 1)	{ scen = 3 ; bComp = false ; bMult = false ; }
		if (pMbr->MinPop() == 0 && pMbr->MaxPop() > 1)	{ scen = 4 ; bComp = false ; bMult = true ; }

		switch	(pMbr->Basetype())
		{
		//	Group 1:
		case BASETYPE_DOUBLE:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 8 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;
		case BASETYPE_INT64:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 8 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;
		case BASETYPE_INT32:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;

		case BASETYPE_INT16:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (!bMult)		m_Info[mbrNo].m_nSize = 2 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;
		case BASETYPE_BYTE:		m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (!bMult)		m_Info[mbrNo].m_nSize = 1 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;

		case BASETYPE_UINT64:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 8 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;
		case BASETYPE_UINT32:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;

		case BASETYPE_UINT16:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (!bMult)		m_Info[mbrNo].m_nSize = 2 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;
		case BASETYPE_UBYTE:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (!bMult)		m_Info[mbrNo].m_nSize = 1 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;

		case BASETYPE_BOOL:		m_Info[mbrNo].m_nLitmus = nLitmus++ ;	m_Info[mbrNo].m_nSize = 0 ;	break ;

		//	Group 2:
		case BASETYPE_DOMAIN:	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;
		case BASETYPE_EMADDR:	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;
		case BASETYPE_URL:		if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;

		case BASETYPE_IPADDR:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;
		case BASETYPE_TIME:		m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;
		case BASETYPE_SDATE:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;
		case BASETYPE_XDATE:	m_Info[mbrNo].m_nLitmus = nLitmus++ ;	if (scen == 1)	m_Info[mbrNo].m_nSize = 8 ;	else m_Info[mbrNo].m_nList = m_nBlklists++ ;	break ;

		case BASETYPE_STRING:	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;
		case BASETYPE_TEXT:		if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;
		case BASETYPE_BINARY:	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ; else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;
		case BASETYPE_TXTDOC:	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ; else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;

		//	Enums
		case BASETYPE_ENUM:		m_Info[mbrNo].m_nSize = 4 ;

		//	App specific
		case BASETYPE_APPDEF:	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;

		//	Classes
		case BASETYPE_CLASS:	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;
		//case BASETYPE_REPOS:	if (scen == 1)	m_Info[mbrNo].m_nSize = 4 ;	else { m_Info[mbrNo].m_nLitmus = nLitmus++ ; m_Info[mbrNo].m_nList = m_nBlklists++ ; }	break ;
		}
	}

	/*
	**	Calculate block size and offsets
	*/

	//	The litmus bits come first and there is always at least one for the delete flags
	oset = nLitmus * 8 ;

	for (mbrNo = 0 ; mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		pMbr = m_pClass->GetMember(mbrNo) ;

		if (m_Info[mbrNo].m_nSize)
		{
			//	A member with a list cannot have space in the fixed area
			if (!m_Info[mbrNo].m_nList)
			{
				m_Info[mbrNo].m_nOset = oset ;
				oset += (m_Info[mbrNo].m_nSize * 64) ;
			}
		}

		threadLog("%s: MEMBER %d litmus %d list %d oset %04d size %d (%s)\n",
			*_fn, mbrNo, m_Info[mbrNo].m_nLitmus, m_Info[mbrNo].m_nList, m_Info[mbrNo].m_nOset, m_Info[mbrNo].m_nSize, pMbr->TxtName()) ;
	}

	m_nBlksize = oset ;
	threadLog("%s. CLASS REPORT %s Totals: Members %d Litmus %d Fixed %d Lists %d Space %d\n",
		*_fn, m_pClass->TxtTypename(), m_pClass->MbrCount(), nLitmus, nFixed, m_nBlklists, m_nBlksize) ;
}

hzEcode	hdbObjCache::_cache_blk::AssignSlot	(uint32_t& nSlot, uint32_t objId, uint32_t mode)
{
	//	Assign space for the object at the given object id (in effect an object address). Only when the requested object id exceeds the total objects supported
	//	by currently allocated blocks is a new block allocated. This function returns a pointer to the memory block, not a space for the object to be stored. It
	//	set the slot number to indicate where the object members are to be positioned within the block. 
	//
	//	Arguments:	1)	nSlot	Reference to the assigned slot
	//				2)	objId	The object id
	//
	//	Returns:	Pointer to destination block
	//				NULL if not object id is supplied

	_hzfunc("_cache_blk::AssignSlot") ;

	//_c64_blk*	pBloc ;		//	The data block the fixed part of the object will live in. This will be the last block.
	void*		pBloc ;		//	The data block the fixed part of the object will live in. This will be the last block.
	uint32_t	blkNo ;		//	Block iterator

	//	Validate
	if (!this)		hzexit(_fn, 0, E_CORRUPT, "No class instance") ;
	if (!m_pClass)	hzexit(_fn, 0, E_NOINIT, "Matrix not initialized") ;

	nSlot = 0 ;
	if (!objId)
		return E_NOTFOUND ;

	if (mode > 2)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Illegal Mode %d. Only 0/1/2 is permitted", mode) ;

	if (mode == 2)
	{
		//	This can only be invoked by hdbObjCache::Open which can encounter deltas for objects that as of yet, have not been loaded
		if (objId > m_nCachePop)
			m_nCachePop = objId ;
	}

	if (mode == 1)
	{
		//	This is invoked by hdbObjCache::Insert: The supplied object id MUST be EXACTLY 1 greater then the count of objects, otherwise this is a sequence error

		if (objId > (m_nCachePop + 1))
			return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Insert of object %d illegal. Only %d is permitted", objId, m_nCachePop) ;
		if (objId == (m_nCachePop + 1))
			m_nCachePop++ ;
	}

	if (mode == 0)
	{
		//	This is invoked by lookups: The supplied object id MUST exist
		if (objId > m_nCachePop)
			return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Lookup: Object %d does not exist. Max is %d", objId, m_nCachePop) ;
	}

	//	Object ids start at 1 so to get actual position in the bloc we deduct 1
	objId-- ;
	blkNo = objId/64 ;
	nSlot = objId%64 ;

	if (blkNo == m_Ram.Count())
	{
		pBloc = new char[m_nBlksize] ;
		memset(pBloc, 0, m_nBlksize) ;
		m_Ram.Add(pBloc) ;
	}
	/*
	if (!m_Ram.Exists(blkNo))
	{
		//	Get a new fixed length space
		//pBloc = new _c64_blk() ;
		pBloc = new char[m_nBlksize] ;

		//pBloc->m_pFixSpace = new char[m_nBlksize] ;
		//memset(pBloc->m_pFixSpace, 0, m_nBlksize) ;
		memset(pBloc, 0, m_nBlksize) ;

		//	if (m_nBlklists)
		//	{
		//		pBloc->m_pVarSpace = new hzMCH[m_nBlklists] ;
		//	}

		m_Ram.Insert(blkNo, pBloc) ;
	}
	*/

	//	Return fixed length space
	//	return (char*) pBloc->m_pFixSpace ;
	return E_OK ;
}

hzEcode	hdbObjCache::_cache_blk::_setMbrValue	(uint32_t objId, uint32_t mbrNo, _atomval value)
{
	//	This will set the bit in the litmus for the member. The 64-bit number storing the member litmus bit on behalf of all objects in the block, is found from
	//	the menber's litmus number. 
	//
	//	Arguments:	1)	objId	The object id
	//				2)	mbrNo	Member number
	//				3)	bValue	Boolean value to set
	//
	//	Returns:	E_NOTFOUND	If the object id does not exist
	//				E_CORRUPT	If the member does not exists
	//				E_OK		If the operation was successful

	//	const hdbClass::_mbr*	pMbr ;	//	Member pointer

	//_c64_blk*	pBloc ;		//	Block pointer
	void*		pBloc ;		//	Block pointer
	char*		ptr ;		//	Pointer in litmus blocks
	_r uint32_t	blkNo ;		//	Block iterator
	_r uint32_t	nSlot ;		//	Block iterator
	_r uint32_t	nSize ;		//	Size

	//	Check validity
	if (!objId || objId > m_nCachePop)
		return E_NOTFOUND ;

	if (mbrNo >= m_pClass->MbrCount())
		return E_CORRUPT ;

	//	if (m_Info[mbrNo].m_nLitmus)
	//		return E_TYPE ;

	//	Get block and slot
	objId-- ;
	blkNo = objId/64 ;
	nSlot = objId%64 ;

	pBloc = m_Ram[blkNo] ;
	if (!pBloc)
		return E_CORRUPT ;

	nSize = m_Info[mbrNo].m_nSize ;

	//	Either the core data goes in fixed or var space
	if (m_Info[mbrNo].m_Flag & CACHE_LIST)
	{
	}
	else
	{
		//	Fixed space
		//ptr = (char*) pBloc->m_pFixSpace ;
		ptr = (char*) pBloc ;
		ptr += m_Info[mbrNo].m_nOset ;
		ptr += (nSlot * nSize) ;

		switch	(nSize)
		{
		case 8:	memcpy(ptr, &value.m_uInt64, 8) ;	break ;
		case 4:	memcpy(ptr, &value.m_uInt32, 4) ;	break ;
		case 2:	memcpy(ptr, &value.m_uInt16, 2) ;	break ;
		case 1:	*ptr = value.m_uByte ;	break ;
		}
	}

	return E_OK ;
}

hzEcode	hdbObjCache::_cache_blk::_setMbrBool	(uint32_t objId, uint32_t mbrNo, bool bValue)
{
	//	This will set the bit in the litmus for the member. The 64-bit number storing the member litmus bit on behalf of all objects in the block, is found from
	//	the menber's litmus number. 
	//
	//	Arguments:	1)	objId	The object id
	//				2)	mbrNo	Member number
	//				3)	bValue	Boolean value to set
	//
	//	Returns:	E_NOTFOUND	If the object id does not exist
	//				E_CORRUPT	If the member does not exists
	//				E_TYPE		If the member does not have a litmus bit
	//				E_OK		If the operation was successful

	//_c64_blk*	pBloc ;		//	Block pointer
	void*		pBloc ;		//	Block pointer
	char*		ptr ;		//	Pointer in litmus blocks
	uint32_t	blkNo ;		//	Block iterator
	uint32_t	nSlot ;		//	Block iterator

	//	Check state
	if (!objId || objId > m_nCachePop)	return E_NOTFOUND ;
	if (mbrNo >= m_pClass->MbrCount())	return E_CORRUPT ;
	if (!m_Info[mbrNo].m_nLitmus)		return E_TYPE ;

	objId-- ;
	blkNo = objId/64 ;
	nSlot = objId%64 ;

	pBloc = m_Ram[blkNo] ;
	if (!pBloc)
		return E_CORRUPT ;

	//ptr = (char*) pBloc->m_pFixSpace ;
	ptr = (char*) pBloc ;
	ptr += (m_Info[mbrNo].m_nLitmus * 8) ;

	if (bValue)
		ptr[nSlot/8] |= (0x01 << (nSlot % 8)) ;
	else
		ptr[nSlot/8] &= ~(0x01 << (nSlot % 8)) ;
	/*
		switch	(nSlot % 8)
		{
		case 0:	ptr[nSlot/8] |= 0x01 ;
		case 1:	ptr[nSlot/8] |= 0x02 ;
		case 2:	ptr[nSlot/8] |= 0x04 ;
		case 3:	ptr[nSlot/8] |= 0x08 ;
		case 4:	ptr[nSlot/8] |= 0x10 ;
		case 5:	ptr[nSlot/8] |= 0x20 ;
		case 6:	ptr[nSlot/8] |= 0x40 ;
		case 7:	ptr[nSlot/8] |= 0x80 ;
		}
	else
		switch	(nSlot % 8)
		{
		case 0:	ptr[nSlot/8] &= ~0x01 ;
		case 1:	ptr[nSlot/8] &= ~0x02 ;
		case 2:	ptr[nSlot/8] &= ~0x04 ;
		case 3:	ptr[nSlot/8] &= ~0x08 ;
		case 4:	ptr[nSlot/8] &= ~0x10 ;
		case 5:	ptr[nSlot/8] &= ~0x20 ;
		case 6:	ptr[nSlot/8] &= ~0x40 ;
		case 7:	ptr[nSlot/8] &= ~0x80 ;
		}
	*/
	return E_OK ;
}

hzEcode	hdbObjCache::_cache_blk::_setMbrLitmus	(uint32_t objId, bool bValue)
{
	//	This will set the bit in the litmus for the member. The 64-bit number storing the member litmus bit on behalf of all objects in the block, is found from
	//	the menber's litmus number. 
	//
	//	Arguments:	1)	objId	The object id
	//				2)	mbrNo	Member number
	//				3)	bValue	Boolean value to set
	//
	//	Returns:	E_NOTFOUND	If the object id does not exist
	//				E_CORRUPT	If the member does not exists
	//				E_TYPE		If the member does not have a litmus bit
	//				E_OK		If the operation was successful

	//_c64_blk*	pBloc ;		//	Block pointer
	void*		pBloc ;		//	Block pointer
	char*		ptr ;		//	Pointer in litmus blocks
	uint32_t	blkNo ;		//	Block iterator
	uint32_t	nSlot ;		//	Block iterator

	//	Check state
	if (!objId || objId > m_nCachePop)
		return E_NOTFOUND ;

	objId-- ;
	blkNo = objId/64 ;
	nSlot = objId%64 ;

	pBloc = m_Ram[blkNo] ;
	if (!pBloc)
		return E_CORRUPT ;

	//ptr = (char*) pBloc->m_pFixSpace ;
	ptr = (char*) pBloc ;

	if (bValue)
		ptr[nSlot/8] |= (0x01 << (nSlot % 8)) ;
	else
		ptr[nSlot/8] &= ~(0x01 << (nSlot % 8)) ;

	return E_OK ;
}

hzEcode	hdbObjCache::_cache_blk::GetVal	(_atomval& value, uint32_t objId, uint32_t mbrNo) const
{
	//	This will set the bit in the litmus for the member. The 64-bit number storing the member litmus bit on behalf of all objects in the block, is found from
	//	the menber's litmus number. 
	//
	//	Arguments:	1)	objId	The object id
	//				2)	mbrNo	Member number
	//				3)	bValue	Boolean value to set
	//
	//	Returns:	E_NOTFOUND	If the object id does not exist
	//				E_CORRUPT	If the member does not exists
	//				E_OK		If the operation was successful

	_hzfunc("hdbObjCache::_cache_blk::GetVal") ;

	//_c64_blk*	pBloc ;		//	Block pointer
	void*		pBloc ;		//	Block pointer
	char*		ptr ;		//	Pointer in litmus blocks
	_r uint32_t	blkNo ;		//	Block iterator
	_r uint32_t	nSlot ;		//	Block iterator
	_r uint32_t	nSize ;		//	Size
	_r uint32_t	nOset ;		//	Offset within block

	//	Check validity
	value.m_uInt64 = 0 ;

	if (!objId || objId > m_nCachePop)
		return E_NOTFOUND ;

	if (mbrNo >= m_pClass->MbrCount())
		return E_CORRUPT ;

	//	if (m_Info[mbrNo].m_nLitmus)
	//		return E_TYPE ;

	//	Get block and slot
	objId-- ;
	blkNo = objId/64 ;
	nSlot = objId%64 ;

	pBloc = m_Ram[blkNo] ;
	if (!pBloc)
		return E_CORRUPT ;

	nSize = m_Info[mbrNo].m_nSize ;

	//	Either the core data goes in fixed or var space
	if (m_Info[mbrNo].m_Flag & CACHE_LIST)
	{
		threadLog("%s. CACHE_LIST\n", *_fn) ;
	}
	else
	{
		//	Fixed space
		//ptr = (char*) pBloc->m_pFixSpace ;
		ptr = (char*) pBloc ;
		nOset = m_Info[mbrNo].m_nOset + (nSlot * nSize) ;
		ptr += nOset ;

		switch	(nSize)
		{
		case 8:	memcpy(&value.m_uInt64, ptr, 8) ;	break ;
		case 4:	memcpy(&value.m_uInt32, ptr, 4) ;	break ;
		case 2:	memcpy(&value.m_uInt16, ptr, 2) ;	break ;
		case 1:	value.m_uByte = *ptr ;	break ;
		}
	}

	return E_OK ;
}

hzEcode	hdbObjCache::_cache_blk::GetBool	(bool& bValue, uint32_t objId, uint32_t mbrNo) const
{
	//	This will set the bit in the litmus for the member. The 64-bit number storing the member litmus bit on behalf of all objects in the block, is found from
	//	the menber's litmus number. 
	//
	//	Arguments:	1)	bValue	Boolean value set by this call
	//				2)	objId	The object id
	//				3)	mbrNo	Member number
	//
	//	Returns:	E_NOTFOUND	If the object id does not exist
	//				E_CORRUPT	If the member does not exists
	//				E_TYPE		If the member does not have a litmus bit
	//				E_OK		If the operation was successful

	//_c64_blk*	pBloc ;		//	Block pointer
	void*		pBloc ;		//	Block pointer
	char*		ptr ;		//	Pointer in litmus blocks
	uint32_t	blkNo ;		//	Block iterator
	uint32_t	nSlot ;		//	Block iterator

	//	Check state
	if (!objId || objId > m_nCachePop)	return E_NOTFOUND ;
	if (mbrNo >= m_pClass->MbrCount())	return E_CORRUPT ;
	if (!m_Info[mbrNo].m_nLitmus)		return E_TYPE ;

	objId-- ;
	blkNo = objId/64 ;
	nSlot = objId%64 ;

	pBloc = m_Ram[blkNo] ;
	if (!pBloc)
		return E_CORRUPT ;

	//ptr = (char*) pBloc->m_pFixSpace ;
	ptr = (char*) pBloc ;
	ptr += (m_Info[mbrNo].m_nLitmus * 8) ;

	//	bValue = GetBits(ptr, mbrNo) ;
	switch	(nSlot % 8)
	{
	case 0:	return ptr[nSlot/8] & 0x01 ? 1 : 0 ;
	case 1:	return ptr[nSlot/8] & 0x02 ? 1 : 0 ;
	case 2:	return ptr[nSlot/8] & 0x04 ? 1 : 0 ;
	case 3:	return ptr[nSlot/8] & 0x08 ? 1 : 0 ;
	case 4:	return ptr[nSlot/8] & 0x10 ? 1 : 0 ;
	case 5:	return ptr[nSlot/8] & 0x20 ? 1 : 0 ;
	case 6:	return ptr[nSlot/8] & 0x40 ? 1 : 0 ;
	case 7:	return ptr[nSlot/8] & 0x80 ? 1 : 0 ;
	}

	return E_OK ;
}

hzEcode	hdbObjCache::_cache_blk::GetObject	(bool& bValue, uint32_t objId) const
{
	//	This will set the bit in the litmus for the member. The 64-bit number storing the member litmus bit on behalf of all objects in the block, is found from
	//	the menber's litmus number. 
	//
	//	Arguments:	1)	bValue	Boolean value set by this call
	//				2)	objId	The object id
	//				3)	mbrNo	Member number
	//
	//	Returns:	E_NOTFOUND	If the object id does not exist
	//				E_CORRUPT	If the member does not exists
	//				E_TYPE		If the member does not have a litmus bit
	//				E_OK		If the operation was successful

	//_c64_blk*	pBloc ;		//	Block pointer
	void*		pBloc ;		//	Block pointer
	char*		ptr ;		//	Pointer in litmus blocks
	uint32_t	blkNo ;		//	Block iterator
	uint32_t	nSlot ;		//	Block iterator

	//	Check state
	if (!objId || objId > m_nCachePop)	return E_NOTFOUND ;

	objId-- ;
	blkNo = objId/64 ;
	nSlot = objId%64 ;

	pBloc = m_Ram[blkNo] ;
	if (!pBloc)
		return E_CORRUPT ;

	//ptr = (char*) pBloc->m_pFixSpace ;
	ptr = (char*) pBloc ;

	//	bValue = GetBits(ptr, mbrNo) ;
	switch	(nSlot % 8)
	{
	case 0:	return ptr[nSlot/8] & 0x01 ? 1 : 0 ;
	case 1:	return ptr[nSlot/8] & 0x02 ? 1 : 0 ;
	case 2:	return ptr[nSlot/8] & 0x04 ? 1 : 0 ;
	case 3:	return ptr[nSlot/8] & 0x08 ? 1 : 0 ;
	case 4:	return ptr[nSlot/8] & 0x10 ? 1 : 0 ;
	case 5:	return ptr[nSlot/8] & 0x20 ? 1 : 0 ;
	case 6:	return ptr[nSlot/8] & 0x40 ? 1 : 0 ;
	case 7:	return ptr[nSlot/8] & 0x80 ? 1 : 0 ;
	}

	return E_OK ;
}

hzEcode	hdbObjCache::InitStart	(const hdbClass* pObjClass, const hzString& rname, const hzString& workdir)
{
	//	Begin initialization of an object cache with the supplied class definition, name and working directory. The initialization states of both this cache and
	//	the supplied class definition are checked. The class definition must be completely initialized. The cache must be completely unitialialized.
	//
	//	The initialization process sets up the internal memory system of the object cache to receive and store objects conforming to the format dictated by the
	//	class definition. Any boolean class members are held in thier own idset (hdbIdset) and since bitmaps are their own index, these are directly pointed to
	//	by the members index pointer.
	//
	//	All other class members occupy fixed size slots in memory managed by the hdbObjCache::_matrix subclass. The slots can be either 8, 4, 2 or 1 byte long,
	//	depending on the data type and whether or not the member allows multiple values. The fixed size is facilitated in the following way:-
	//
	//	1)	Class members that are singular and whose data types are fixed size, may be stored as-is.
	//
	//	2)	Class members that are string like (either strings, email addresses or URLs) are fixed size becuase the actual strings are stored in a string table.
	//		Strings in the string use a 32-bit identifier so within the object caches a string is always 4 bytes long.
	//
	//	3)	Class members that are documents or binary objects are also fixed size as they are also identifiers to resorces that lay outside the object cache.
	//
	//	4)	Class members that are of another class, are fixed because they are just addresses to objects in another object repository.
	//
	//	5)	Class members that support multiple values, regarless of type are also arranged to be of fixed size in the matrix memory. The entry for the member
	//		will always point to the first in a series of address/value pairs, held in an auxilliary matrix maintained in the same object cache.
	//
	//	Once this function has completed, it may be followed by calls to InitIndex() to define indexes for the cache. After that it must be followed by a call
	//	to InitDone() to complete the process. Note that it is nessesary to have an initialization sequence rather than a single call to a single initialization
	//	function because indexes are not and cannot be part of a data class.
	//
	//	Arguments:	1)	pObjClass	The data class
	//				2)	rname		The repository name
	//				3)	workdir		The operaional directory
	//
	//	Returns:	E_ARGUMENT	If no data class is supplied
	//				E_NOINIT	If no class members have been defined
	//				E_INITDUP	If this is a repeat call
	//				E_DUPLICATE	If the cache already exists
	//				E_OK		If the operation was successful

	_hzfunc("hdbObjCache::InitStart") ;

	uint32_t	nIndex ;	//	Member iterator
	hzEcode		rc ;		//	Return code

	//	Check init state and state of supplied class
	_hdb_ck_initstate(_fn, rname, m_eReposInit, HDB_CLASS_INIT_NONE) ;

	//	Check data class has been supplied and is initialized
	if (!pObjClass)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No data class supplied") ;

	if (!pObjClass->IsInit())
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "Supplied class (%s) is not initialized", pObjClass->TxtTypename()) ;

	//	Ensure Repository Name is Unique
	if (!rname)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No name supplied") ;

	if (m_pADP->GetObjRepos(rname))
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Cache %s already exists", *rname) ;

	//	Ensure working directory is given and operational
	if (!workdir)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No working directory supplied for object cache %s", *rname) ;

	rc = AssertDir(*workdir, 0777) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Cannot assert working directory %s for object cache %s\n", *workdir, *rname) ;

	//	Proceed with initialization
	m_pClass = pObjClass ;
	m_Name = rname ;
	m_Workdir = workdir ;
	
	/*
	**	Set up the controller(s) for the class
	*/

	m_pMain = new _cache_blk() ;
	m_pMain->InitMatrix(m_pClass) ;

	//	Set up m_Binaries to be all nulls. Later real hdbBinRepos instances will be created as required.
	m_Binaries = new hdbBinRepos*[m_pClass->MbrCount()] ;
	if (!m_Binaries)
		hzexit(_fn, m_Name, E_MEMORY, "Cannot allocate set of index pointers") ;

	for (nIndex = 0 ; nIndex < m_pClass->MbrCount() ; nIndex++)
		m_Binaries[nIndex] = 0 ;

	//	Set up index array to be all nulls. Later the AddIndex function will insert real indexes where requested
	m_Indexes = new hdbIndex*[pObjClass->MbrCount()] ;
	if (!m_Indexes)
		hzexit(_fn, m_Name, E_MEMORY, "Cannot allocate set of index pointers") ;

	for (nIndex = 0 ; nIndex < pObjClass->MbrCount() ; nIndex++)
		m_Indexes[nIndex] = 0 ;

	//	Set init state
	m_eReposInit = HDB_REPOS_INIT_PROG ;

	//	Insert the repository
	m_pADP->RegisterObjRepos(this) ;
	return E_OK ;
}

hzEcode	hdbObjCache::InitMbrIndex	(const hzString& mbrName, bool bUnique)
{
	//	Add an index based on the supplied member name. Find the member in the class and from the datatype, this will determine which sort of index
	//	should be set up.
	//
	//	Arguments:	1)	mbrName	Name of member index shall apply to
	//				2)	bUnique	Flag if value uniqness applies
	//
	//	Returns:	E_NOINIT	If the cache initialization sequence has not been started
	//				E_ARGUMENT	If the member name is not supplied
	//				E_SEQUENCE	If the cache initialization has been completed by InitDone()
	//				E_NOTFOUND	If the named member is not found in the cache class
	//				E_TYPE		If the named member is of a type that cannot accept an index
	//				E_OK		If the index is successfully created

	_hzfunc("hdbObjCache::InitMbrIndex") ;

	const hdbMember*	pMbr ;		//	Named class member
	hdbIndex*		pIdx ;		//	The index to be added
	hzString		iname ;		//	Index name of the form repos::member
	hzEcode			rc = E_OK ;	//	Return code

	//	Check init state
	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_INIT_PROG) ;

	if (!m_pClass)	return hzerr(_fn, HZ_ERROR, E_NOINIT, "No cache class set up") ;
	if (!mbrName)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No member name supplied") ;

	pMbr = m_pClass->GetMember(mbrName) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "No such member as %s in class %s\n", *mbrName, m_pClass->TxtTypename()) ;

	if (pMbr->Posn() < 0 || pMbr->Posn() >= m_pClass->MbrCount())
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Member %s has no defined position within the class\n", *mbrName) ;

	//	Member's datatype will determine the type of index
	pIdx = 0 ;
	switch	(pMbr->Basetype())
	{
	case BASETYPE_EMADDR:
	case BASETYPE_URL:		pIdx = new hdbIndexUkey() ;
							break ;

	case BASETYPE_STRING:	//	if (bUnique)
							//		pIdx = new hdbIndexUkey() ;
							//	else
							//		pIdx = new hdbIndexText() ;
							//	break ;

	case BASETYPE_IPADDR:
	case BASETYPE_TIME:
	case BASETYPE_SDATE:
	case BASETYPE_XDATE:
	case BASETYPE_DOUBLE:
	case BASETYPE_INT64:
	case BASETYPE_INT32:
	case BASETYPE_UINT64:
	case BASETYPE_UINT32:
	case BASETYPE_UINT16:
	case BASETYPE_UBYTE:	pIdx = new hdbIndexUkey() ;
							break ;

	case BASETYPE_INT16:
	case BASETYPE_BYTE:
	case BASETYPE_ENUM:		pIdx = new hdbIndexEnum() ;
							break ;

	case BASETYPE_TEXT:
	case BASETYPE_TXTDOC:	pIdx = new hdbIndexText() ;
							break ;

	case BASETYPE_CLASS:
	//case BASETYPE_REPOS:
	case BASETYPE_BINARY:	rc = hzerr(_fn, HZ_ERROR, E_TYPE, "Invalid member type. No Index allowed") ;
							break ;

	default:
		break ;
	}

	m_Indexes[pMbr->Posn()] = pIdx ;
	m_eReposInit = HDB_REPOS_INIT_PROG ;

	//	iname = m_Name + "::" + pMbr->StrName() ;

	//	if (pMbr->Basetype() == BASETYPE_STRING || pMbr->Basetype() == BASETYPE_EMADDR || pMbr->Basetype() == BASETYPE_URL)
	//		rc = pIdx->Init(iname, BASETYPE_UINT32) ;
	//	else
	////			rc = pIdx->Init(iname, pMbr->Basetype()) ;

	return rc ;
}

hzEcode	hdbObjCache::InitMbrStore	(const hzString& mbrName, const hzString& storeName, bool bStore)
{
	//	Direct a member of the cache classs to be stored in either a hdbBinCron or hdbBinStore and if an externally declared hdbBinCron or hdbBinStore is to be used
	//	provide the name so it can be selected from _hzGlobal_BinDataCrons or _hzGlobal_BinDataStores.
	//
	//	Arguments:	1)	mbrName		The member name. The named member must exist and be of base data type TEXT, TXTDOC or BINARY.
	//				2)	storeName	The name of the externally declared hdbBinCron or hdbBinStore.
	//				3)	bStore		Set if a hdbBinStore is to be used. The default is to use a hdbBinCron for TEXT and TXTDOC members.
	//
	//	Returns:	E_NOTFOUND	If the named member does not exist.
	//				E_TYPE		If the named member is not of base data type TEXT, TXTDOC or BINARY.
	//				E_OK		If the operation was successful.

	_hzfunc("hdbObjCache::InitMbrStore") ;

	const hdbBinRepos*	pRepos ;	//	Binary datum repository
	const hdbMember*	pMbr ;		//	Named class member

	uint32_t	mbrNo ;				//	Member number
	hzEcode		rc ;				//	Returm code

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_INIT_PROG) ;
	//if (!m_eReposInit)		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Cannot add any members before InitStart()") ;
	//if (m_eReposInit > 2)	return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Cannot add any members after InitDone()") ;
	//if (!m_pClass)			return hzerr(_fn, HZ_ERROR, E_NOINIT, "No cache class set up") ;
	//if (!mbrName)			return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No member name supplied") ;

	pMbr = m_pClass->GetMember(mbrName) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "No such member as %s in class %s\n", *mbrName, m_pClass->TxtTypename()) ;
	mbrNo = pMbr->Posn() ;

	if (mbrNo < 0 || mbrNo >= m_pClass->MbrCount())
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Member %s has no defined position within class %s", *mbrName, m_pClass->TxtTypename()) ;

	if (pMbr->Basetype() != BASETYPE_BINARY && pMbr->Basetype() != BASETYPE_TEXT && pMbr->Basetype() != BASETYPE_TXTDOC)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Member %s has non binary base type", *mbrName) ;

	if (storeName)
	{
		//	A pre-existing hdbBinCron or hdbBinStore has been named so the member will use this.
		if (bStore)
		{
			pRepos = m_pADP->GetBinRepos(storeName) ;
			if (pRepos)
				m_Binaries[mbrNo] = (hdbBinRepos*) pRepos ;
			else
				return hzerr(_fn, m_Name, HZ_ERROR, E_NOTFOUND, "Binary datum store %s does not exist", *storeName) ;
		}
		/*
		else
		{
			pRepos = m_pADP->GetBinRepos(storeName) ;
			if (pRepos)
				m_Binaries[mbrNo] = (hdbBinCron*) pRepos ;
			else
				return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Binary datum datacron %s does not exist", *storeName) ;
		}
		*/
	}
	else
	{
		//	No hdbBinCron or hdbBinStore has been named so the member will use the hzObjCache's default hdbBinCron or hdbBinStore.
		if (!m_pDfltBinRepos)
		{
			m_pDfltBinRepos = new hdbBinRepos(*m_pADP) ;
			rc = m_pDfltBinRepos->Init(m_Name, m_Workdir) ;
			if (rc != E_OK)
				return hzerr(_fn, HZ_ERROR, rc, "Failed to initialize binary data store %s (%s)\n", *m_Name, *m_Workdir) ;
		}
		m_Binaries[mbrNo] = m_pDfltBinRepos ;
	}

	return E_OK ;
}

hzEcode	hdbObjCache::InitDone	(void)
{
	//	Complete the initialization of the object cache.
	//
	//	Now deal with files if the working directory has been supplied. If a file of the cache's name exists in the working directory, this is assumed
	//	to be the data file and will be read in to populate the cache. The file must begin with a header which must match the class definition so this
	//	is checked before loading the remaining data. If a file of the cache's name does not exist in the working directory, it will be created and a
	//	header will be written.
	//
	//	If a backup directory has been specified and a file of the cache's name exists in this directory, the header will be checked and assuming this
	//	is OK, the length of the file will aslo be checked (should match with that in the work directory)
	//
	//	Arguments:	None

	_hzfunc("hdbObjCache::InitDone") ;

	const hdbMember*	pMbr ;		//	Member
	ifstream		is ;		//	For reading in working data file
	ofstream		os ;		//	For writing in working data file
	FSTAT			fs ;		//	File status
	hzChain			X ;			//	For writing class description header from class
	hzChain			E ;			//	Existing class description header from data file
	hzAtom			atom ;		//	For setting member values
	hdbIndex*		pIdx ;		//	The index to be added
	hdbIndexUkey*	pIdxU ;		//	The index to be added
	hzString		S1 ;		//	Temp string holding memeber data
	hzString		S2 ;		//	Temp string holding memeber data
	uint32_t		nLine ;		//	Line number for reporting file errors
	uint32_t		mbrNo ;		//	Member number
	char			buf [508] ;	//	For getline
	hzEcode			rc ;		//	Return code

	//	Check initialization and if there are some members added
	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_INIT_PROG) ;

	if (!m_pClass)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Cache %s has no data class", *m_Name) ;
	if (!m_pClass->MbrCount())
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Class %s: Appears to have no members!", *m_Name) ;

	m_pClass->DescClass(X, 0) ;
	if (!X.Size())
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Class %s: Failed to write description", *m_Name) ;

	m_Workpath = m_Workdir + "/" + m_Name + ".cache" ;
	threadLog("%s called with name %s and workpath %s\n", *_fn, *m_Name, *m_Workpath) ;

	if (lstat(*m_Workpath, &fs) < 0)
	{
		//	Working data file does not exist or is empty. Create a new one from the class description as per C++ calls
		os.open(*m_Workpath) ;
		if (os.fail())
			return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Class %s Cannot open data file %s in write mode", *m_Name, *m_Workpath) ;

		os << X ;
		if (os.fail())
		{
			os.close() ;
			os.clear() ;
			return hzerr(_fn, HZ_ERROR, rc, "Class %s Cannot write class description to data file %s", *m_Name, *m_Workpath) ;
		}
		os.flush() ;
		os.close() ;
		os.clear() ;
	}

	//	Working data file does exist and has content so read it in. Start with header
	is.open(*m_Workpath) ;
	if (is.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Class %s data file %s exists but cannot be read in", *m_Name, *m_Workpath) ;
	for (nLine = 1 ;; nLine++)
	{
		is.getline(buf, 500) ;
		if (!buf[0])
			break ;

		E << buf ;
		E.AddByte(CHAR_NL) ;

		if (!strcmp(buf, "</class>"))
			break ;
	}
	is.close() ;

	//	Compare class description header from file to that of the class
	if (X.Size() != E.Size())
	{
		S1 = E ;
		S2 = X ;
		return hzerr(_fn, HZ_ERROR, E_FORMAT, "Format error in data file %s. Existing description \n[\n%s\n]\nNew\n[\n%s\n]\n", *m_Workpath, *S1, *S2) ;
	}

	//	Initialize all allocated indexes
	for (mbrNo = 0 ; mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		//	First check if a member gives rise to an hdbBinCron instance. This is needed if there are one or more members with datatype of documents or
		//	binaries
		pMbr = m_pClass->GetMember(mbrNo) ;
		threadLog("%s. DOING member %s\n", *_fn, pMbr->TxtName()) ;

		if (pMbr->Basetype() == BASETYPE_BINARY || pMbr->Basetype() == BASETYPE_TXTDOC)
		{
			if (!m_Binaries[mbrNo])
			{
				if (!m_pDfltBinRepos)
				{
					threadLog("%s. Assigning a default datacron [%s]\n", *_fn, *m_Name) ;

					m_pDfltBinRepos = new hdbBinRepos(*m_pADP) ;
					rc = m_pDfltBinRepos->Init(m_Name, m_Workdir) ;
					if (rc != E_OK)
						return hzerr(_fn, HZ_ERROR, rc, "Failed to initialize binary datacron %s (%s)\n", *m_Name, *m_Workdir) ;
				}
				m_Binaries[mbrNo] = m_pDfltBinRepos ;
			}
		}

		//	Check if the member has an associated index
		pIdx = m_Indexes[mbrNo] ;
		if (!pIdx)
			continue ;

		if (pIdx->Whatami() == HZINDEX_UKEY)
		{
			pIdxU = (hdbIndexUkey*) pIdx ;

			rc = pIdxU->Init(this, pMbr->StrName(), pMbr->Basetype()) ;

			if (rc != E_OK)
				return hzerr(_fn, HZ_ERROR, rc, "Failed to initialize unique-key index %s (%s)\n", *m_Name, pMbr->TxtName()) ;
		}
	}

	//	Init matrix

	if (rc == E_OK)
		m_eReposInit = HDB_REPOS_INIT_DONE ;
	return rc ;
}

hzEcode	hdbObjCache::Open	(void)
{
	//	Opening a hdbObjCache will load the data from the delta file if it exists and has content.
	//
	//	Now deal with files if the working directory has been supplied. If a file of the cache's name exists in the working directory, this is assumed
	//	to be the data file and will be read in to populate the cache. The file must begin with a header which must match the class definition so this
	//	is checked before loading the remaining data. If a file of the cache's name does not exist in the working directory, it will be created and a
	//	header will be written.
	//
	//	If a backup directory has been specified and a file of the cache's name exists in this directory, the header will be checked and assuming this
	//	is OK, the length of the file will aslo be checked (should match with that in the work directory)
	//
	//	Arguments:	None

	_hzfunc("hdbObjCache::Open") ;

	hzMapM<uint32_t,_atomval>	tmpVals ;	//	Set of temporary values

    hzMapM<hdbROMID,uint32_t>   m_Values ;      //  Map of class/object/member to either non string like values or offsets to values in m_Large or m_Strings
    hzArray<_atomval>           m_Large ;       //  All 64-bit values (including pointers to chains).
    hzArray<hzString>           m_Strings ;     //  All string values (including email addresses and URL's).


	hdbObject		tmpObj ;		//	Temporary data object
	const hdbMember*	pMbr ;			//	Member
	ifstream		is ;			//	For reading in working data file
	hzChain			X ;				//	For writing class description header from class
	hzChain			Y ;				//	For writing class description header from data file
	hzAtom			atom ;			//	For setting member values
	_atomval		av ;			//	Atom value
	hdbIndex*		pIdx ;			//	Index pointer
	hdbIndexUkey*	pIdxU ;			//	Index pointer
	hdbIndexEnum*	pIdxE ;			//	Index pointer
	char*			j ;				//	For buffer iteration
	hzString		S ;				//	Temp string holding memeber data
	uint32_t		nLine ;			//	Line number for reporting file errors
	uint32_t		nSlot ;			//	Slot number
	uint32_t		rpsId ;			//	Repos id
	uint32_t		clsId ;			//	Class id
	uint32_t		objId ;			//	Object id
	uint32_t		strNo ;			//	String number
	uint32_t		mbrNo ;			//	Member number
	char			buf [3004] ;	//	For getline
	hzEcode			rc = E_OK ;		//	Return code

	threadLog("Opening Cache %s\n", *m_Name) ;
	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_INIT_DONE) ;

	//	Bypass header
	is.open(*m_Workpath) ;
	for (nLine = 1 ;; nLine++)
	{
		is.getline(buf, 200) ;
		if (!buf[0])
			break ;
		Y << buf ;
		Y.AddByte(CHAR_NL) ;

		if (!strcmp(buf, "</class>"))
			break ;
	}

	//	Now read in the rest of the data (in delta notation)
	for (; rc == E_OK ; nLine++)
	{
		is.getline(buf, 3000) ;
		if (!is.gcount())
			break ;

		if (!buf[0])
			continue ;

		if (buf[0] == CHAR_AT)
		{
			j = buf + 1 ;

			//	Gather repos id
			if (*j == 'r')
			{
				for (j++, rpsId = 0 ; IsDigit(*j) ; j++)
					{ rpsId *= 10 ; rpsId += *j - '0' ; }

				if (*j != CHAR_PERIOD)
					{ rc = hzerr(_fn, HZ_ERROR, E_FORMAT, "File %s Line %d: Period expected after repos ID", *m_Workpath, nLine) ; break ; }
				j++ ;
			}

			//	Gather class id
			if (*j == 'c')
			{
				for (j++, clsId = 0 ; IsDigit(*j) ; j++)
					{ clsId *= 10 ; clsId += *j - '0' ; }

				if (*j != CHAR_PERIOD)
					{ rc = hzerr(_fn, HZ_ERROR, E_FORMAT, "File %s Line %d: Period expected after class ID", *m_Workpath, nLine) ; break ; }
				j++ ;
			}

			//	Gather and act on the object id in the delta
			if (*j == 'o')
			{
				for (j++, objId = 0 ; IsDigit(*j) ; j++)
					{ objId *= 10 ; objId += *j - '0' ; }

				if (*j != CHAR_PERIOD)
					{ rc = hzerr(_fn, HZ_ERROR, E_FORMAT, "File %s Line %d: Period and member ID expected after object ID", *m_Workpath, nLine) ; break ; }
				j++ ;
			}

			//	Gather and act on the member ID in the delta
			if (*j == 'm')
			{
				for (j++, mbrNo = 0 ; IsDigit(*j) ; j++)
					{ mbrNo *= 10 ; mbrNo += *j - '0' ; }

				pMbr = m_pClass->GetMember(mbrNo) ;
				if (!pMbr)
					{ rc = hzerr(_fn, HZ_ERROR, E_FORMAT, "File %s Line %d: Member %d does not exist in class %s\n", *m_Workpath, nLine, mbrNo, m_pClass->TxtTypename()) ; break ; }
			}

			//	Do we have this object id?
			rc = m_pMain->AssignSlot(nSlot, objId, 2) ;
			if (rc != E_OK)
				break ;

			if (*j == CHAR_EQUAL)
			{
				for (j++ ; *j ; j++)
				{
					if (*j == 0x01)
					{
						j++ ;
						if (*j == 0x01)	X.AddByte(0x01) ;
						if (*j == 0x02)	X.AddByte(CHAR_CR) ;
						if (*j == 0x03)	X.AddByte(CHAR_NL) ;
					}
					else
						X.AddByte(*j) ;
				}

				if (!X.Size())
					continue ;
				S = X ;
				X.Clear() ;

				//	If bool, just set the litmus
				if (pMbr->Basetype() == BASETYPE_BOOL)
				{
					if (S == "true")
						m_pMain->_setMbrBool(objId, mbrNo, true) ;
					else
						m_pMain->_setMbrBool(objId, mbrNo, false) ;
					continue ;
				}

				rc = atom.SetValue(pMbr->Basetype(), S) ;
				if (rc != E_OK)
				{
					threadLog("%s: ERROR: Object %d member %s value %s. Err=%s\n", *_fn, objId, pMbr->TxtName(), *S, Err2Txt(rc)) ;
					break ;
				}

				//	Set the bit in the litmus anyway, because there is a value
				m_pMain->_setMbrBool(objId, mbrNo, true) ;

				//	If string-like, use the string table
				if (pMbr->Basetype() == BASETYPE_STRING || pMbr->Basetype() == BASETYPE_DOMAIN || pMbr->Basetype() == BASETYPE_EMADDR || pMbr->Basetype() == BASETYPE_URL)
				{
					//	Obtain a string number for the string - then set the allocated space to the string number

					if (pMbr->Basetype() == BASETYPE_DOMAIN)
					{
						if (!IsDomain(*S))
							threadLog("%s: Object %d member %s Value %s Rejected\n", *_fn, objId, pMbr->TxtName(), *S) ;

						strNo = _hzGlobal_FST_Domain->Locate(*S) ;
						if (!strNo)
							strNo = _hzGlobal_FST_Domain->Insert(*S) ;
					}
					else if (pMbr->Basetype() == BASETYPE_EMADDR)
					{
						if (!IsEmaddr(*S))
							threadLog("%s: Object %d member %s Value %s Rejected\n", *_fn, objId, pMbr->TxtName(), *S) ;

						strNo = _hzGlobal_FST_Emaddr->Locate(*S) ;
						if (!strNo)
							strNo = _hzGlobal_FST_Emaddr->Insert(*S) ;
					}
					else
					{
						strNo = _hzGlobal_StringTable->Locate(*S) ;
						if (!strNo)
							strNo = _hzGlobal_StringTable->Insert(*S) ;
					}
					//threadLog("%s: Object %d member %s Have str no %d as %s\n", *_fn, objId, pMbr->TxtName(), strNo, *S) ;

					//	Set value in the cache
					av.m_uInt64 = 0 ;
					av.m_uInt32 = strNo ;
					rc = m_pMain->_setMbrValue(objId, mbrNo, av) ;
				}
				else
				{
					//threadLog("%s: Object %d member %s Have str value as %s\n", *_fn, objId, pMbr->TxtName(), *atom.Str()) ;
					rc = m_pMain->_setMbrValue(objId, mbrNo, atom.Datum()) ;
				}
				if (rc != E_OK)
				{
					threadLog("%s: ERROR: Cannot set value! Object %d member %s value %s. Err=%s\n", *_fn, objId, pMbr->TxtName(), *S, Err2Txt(rc)) ;
					break ;
				}

				pIdx = m_Indexes[mbrNo] ;
				if (pIdx)
				{
					if (pIdx->Whatami() == HZINDEX_UKEY)
					{
						pIdxU = (hdbIndexUkey*) pIdx ;
						rc = pIdxU->Insert(atom, objId) ;
						if (rc != E_OK)
							threadLog("%s. Mbr %s Failed on UKEY index insert. Atom %s Err=%s\n", *_fn, pMbr->TxtName(), *atom.Str(), Err2Txt(rc)) ;
					}

					if (pIdx->Whatami() == HZINDEX_ENUM)
					{
						pIdxE = (hdbIndexEnum*) pIdx ;
						rc = pIdxE->Insert(objId, atom) ;
						if (rc != E_OK)
							threadLog("%s. Mbr %s Failed on ENUM index insert. Atom %s\n", *_fn, pMbr->TxtName(), *atom.Str()) ;
					}
				}
			}

			continue ;
		}

		rc = hzerr(_fn, HZ_ERROR, E_SYNTAX, "File %s Line %d [%s]: Unknown instruction. Only @obj:, @del:, and @obj: allowed\n", *m_Workpath, nLine, buf) ;
		break ;
	}
	is.close() ;

	//	Now open file for writing
	if (rc == E_OK)
	{
		m_eReposInit = HDB_REPOS_OPEN ;
		m_os.open(*m_Workpath, ios::app) ;
		if (m_os.fail())
			return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Class %s Cannot open data file %s in write mode", *m_Name, *m_Workpath) ;
	}

	threadLog("%s: Population of objects in cache %s is %d. Delta file %s. Status is %s\n", *_fn, *Name(), Count(), *m_Workpath, Err2Txt(rc)) ;
	return rc ;
}

hzEcode	hdbObjCache::Clear	(void)
{
	//	Destroys all data in the Ram table and re-initializes everything.
	//
	//	Arguments:	None

	_hzfunc("hdbObjCache::Clear") ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	if (m_pMain)
		m_pMain->Clear() ;
	//	if (m_pAuxA)	m_pAuxA->Clear() ;
	//	if (m_pAuxB)	m_pAuxB->Clear() ;

	return E_OK ;
}

hzEcode	hdbObjCache::Insert	(uint32_t& objId, const hdbObject& theObj)
{
	//	Insert a whole new object into the cache and update the indexes accordingly.
	//
	//	The INSERT operation adds a new data object to a repository and creates a new object id. If the repository has an index on any data class members, these
	//	will be updated using the new object id. The INSERT operation will fail if an index on a member requires the member values to be unique and the supplied
	//	data object contains a value for the member that already exists.
	//
	//	The INSERT process is essentially a matter of end to end iteration of the map of values of the supplied data object. The data object must be of the same
	//	class as the repository OR be of a class that has the repository class as a sub-class. If the data object and repository classes do match, the object id
	//	reference (argument 1), should be pre-set by the caller to zero although it is actually ignored. Where the classes do not match, the object id reference
	//	is used to extract the object of interest. This protocol allows INSERT of sub-class objects in another repository to be simplified. Instead of having to
	//	populate a sub-class object before calling Insert() on the sub-class repository, the sub-class Insert() call simply passes the object as is.
	//
	//	It should be noted that where objects do contain sub-class objects that are to be placed in another repository, these sub-class objects will also be new
	//	in their respective repositories and must satisfy any uniqueness conditions of that repository. If a sub-class insert() fails the whole operation fails.
	//	For this reason this function tests the indexes of all implied repositories against the values found in the supplied data object.
	//
	//	This function is also responsible for issuing deltas to the delta server.
	//
	//	Arguments:	1)	objId	The object id that will be assigned by this operation
	//				2)	pObj	Pointer to object to be inserted
	//
	//	Returns:	E_NOINIT	If either the cache or the supplied object is not initialized
	//				E_ARGUMENT	If the object is not supplied
	//				E_TYPE		If the object is not of the same data class as the cache
	//				E_DUPLICATE	If the object cannot be inserted because one or more members violate uniqueness
	//				E_WRITEFAIL	If the object deltas cannot be written
	//				E_OK		If the insert operation was successful

	_hzfunc("hdbObjCache::Insert") ;

	const hdbClass*		pCls ;		//	Class pointer
	const hdbMember*	pMbr ;		//	Member pointer

	hzChain			Z ;				//	For building output to data file
	hzChain			theChain ;		//	For extracting atom data stored as chains
	//	hdbObjCache*	pCache ;		//	Secondary cache if applicable
	//	hdbObject*		pObj2 ;			//	Secondary object
	hzAtom			atom ;			//	Atom from member of populating object
	_atomval		av ;			//	Atomic value
	hdbROMID		romidA ;		//	Delta indicator
	hdbROMID		romidB ;		//	Delta indicator
	hdbIndex*		pIdx ;			//	Index pointer
	hdbIndexUkey*	pIdxU ;			//	Index pointer
	hdbIndexEnum*	pIdxE ;			//	Index pointer
	const char*		i ; 			//	For compacting string in backup file
	hzString		strVal ;		//	Temp string
	uint32_t		mbrNo ;			//	Member number
	uint32_t		nSlot ;			//	To be added to relative offets
	uint32_t		strNo ;			//	String number (doubles as secondary object id)
	uint32_t		binAddr ;		//	Address of binary datum if applicable
	//uint32_t		nOset ;			//	Offset into either theObj m_Large/m_Strings arrays
	int32_t			val_Lo ;		//	First item in one-to-many lookup
	int32_t			val_Hi ;		//	Last item in one-to-many lookup
	hzEcode			rc = E_OK ;		//	Return value
	hzEcode			ic = E_OK ;		//	Return value

	//	Check Init state
	if (!this)
		Fatal("%s. No Object\n", *_fn) ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	if (!theObj.IsInit())
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "Supplied object is not initialized") ;

	//	Check object class is the same as the host class or contains a sub-class that is the same as the sub-class
	objId = 0 ;
	pCls = 0 ;

	if (theObj.Class() != m_pClass)
	{
		if (!m_pADP->IsSubClass(m_pClass, theObj.Class()))
			return hzerr(_fn, HZ_ERROR, E_TYPE, "Supplied object class %s not compatible with this cache class %s", theObj.Classname(), m_pClass->TxtTypename()) ;
	} 

	/*
	**	Go thru all members and check if any of them specify that all values must be unique. Any that are will have an index based on a one to one map of member
	**	values to object id. We look up if the value of the member in the new object already exists and if it does the INSERT operation is aborted. Note that in
	**	the case of string like data types, storage and indexation of values in an object cache is by string number whils in the supplied standalone object, the
	**	values are strings. If the string does not exist in the string table, it cannot exist in the cache. 
	*/

	romidA.m_ClsId = m_pClass->ClassId() ;

	for (mbrNo = 0 ; mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		pMbr = m_pClass->GetMember(mbrNo) ;

		pIdx = m_Indexes[mbrNo] ;
		if (!pIdx)
			continue ;

		romidA.m_MbrId = mbrNo ;

		if (pIdx->Whatami() == HZINDEX_UKEY)
		{
			pIdxU = (hdbIndexUkey*) pIdx ;

			val_Lo = theObj.m_Values.First(romidA) ;
			if (val_Lo < 0)
				continue ;

			val_Hi = theObj.m_Values.Last(romidA) ;

			for (; val_Lo <= val_Hi ; val_Lo++)
			{
				//	Get the value from the member in the supplied object into an atom
				theObj.GetValue(atom, romidA) ;

				strVal = atom.Str() ;

				if (pMbr->Basetype() == BASETYPE_DOMAIN)
				{
					strNo = _hzGlobal_FST_Domain->Locate(*strVal) ;
					if (!strNo)
						continue ;

					atom = strNo ;
					rc = pIdxU->Select(objId, atom) ;
					if (rc != E_OK)
						return hzerr(_fn, m_Name, HZ_ERROR, rc, "Could not select on index for member %s", pMbr->TxtName()) ;
				}
				else if (pMbr->Basetype() == BASETYPE_EMADDR)
				{
					strNo = _hzGlobal_FST_Emaddr->Locate(*strVal) ;
					if (!strNo)
						continue ;

					atom = strNo ;
					rc = pIdxU->Select(objId, atom) ;
					if (rc != E_OK)
						return hzerr(_fn, m_Name, HZ_ERROR, rc, "Could not select on index for member %s", pMbr->TxtName()) ;
				}
				else if (pMbr->Basetype() == BASETYPE_STRING || pMbr->Basetype() == BASETYPE_URL)
				{
					strNo = _hzGlobal_StringTable->Locate(*strVal) ;
					if (!strNo)
						continue ;

					atom = strNo ;
					rc = pIdxU->Select(objId, atom) ;
					if (rc != E_OK)
						return hzerr(_fn, m_Name, HZ_ERROR, rc, "Could not select on index for member %s", pMbr->TxtName()) ;
				}
				else
				{
					rc = pIdxU->Select(objId, atom) ;
					if (rc != E_OK)
						return hzerr(_fn, m_Name, HZ_ERROR, rc, "Could not select on index for member %s", pMbr->TxtName()) ;
				}

				if (objId)
					return hzerr(_fn, m_Name, HZ_ERROR, E_DUPLICATE, "Got objId of %d for member %s", objId, pMbr->TxtName()) ;
			}
		}
	}

	/*
	**	At this point it is known that the new object does not conflict with an existing one and so insertation can proceed. The objId is assigned as
	**	the number of existing objects + 1
	*/

	objId = m_pMain->Count() + 1 ;
	threadLog("%s called on cache %s (assigned objId is %d)\n", *_fn, theObj.Classname(), objId) ;

	//	Ensure there is object space
	m_pMain->AssignSlot(nSlot, objId, 1) ;
	threadLog("%s assigned slot %d for %d values\n", *_fn, nSlot, theObj.m_Values.Count()) ;

	/*
	**	Now go thru object's class members filling in the allocated fixed length space
	*/

	for (val_Lo = 0, val_Hi = theObj.m_Values.Count() ; val_Lo < val_Hi ; val_Lo++)
	{
		romidB = theObj.m_Values.GetKey(val_Lo) ;
		theObj.GetValue(atom, romidB) ;

		//av = theObj.m_Values.GetObj(val_Lo) ;

		pCls = m_pADP->GetDataClass(romidB.m_ClsId) ;
		if (!pCls)
			pCls = m_pClass ;
		if (!pCls)
			hzerr(_fn, HZ_ERROR, E_CORRUPT, "No such class delta id %d", romidB.m_ClsId) ;

		pMbr = pCls->GetMember(romidB.m_MbrId) ;

		//	Set the bit in the litmus if there is a value
		//	if (av.m_uInt64)
		//		m_pMain->_setMbrBool(objId, romidB.m_MbrId, true) ;

		if (pMbr->Basetype() == BASETYPE_BOOL)
		{
			if (atom.Bool())
				m_pMain->_setMbrBool(objId, romidB.m_MbrId, true) ;
			else
				m_pMain->_setMbrBool(objId, romidB.m_MbrId, false) ;
			continue ;
		}

		if (atom.IsNull())
		{
			threadLog("%s entry %d: class %d obj %d mbr %d - blank\n", *_fn, val_Lo, romidB.m_ClsId, romidB.m_ObjId, romidB.m_MbrId) ;
			continue ;
		}
			m_pMain->_setMbrBool(objId, romidB.m_MbrId, true) ;

		strVal.Clear() ;
		//atom.SetValue(pMbr->Basetype(), av) ;
		threadLog("%s entry %d: class %d obj %d mbr %d = %s\n", *_fn, val_Lo, romidB.m_ClsId, romidB.m_ObjId, romidB.m_MbrId, *atom.Str()) ;

		if (pMbr->Basetype() == BASETYPE_CLASS)
		{
			//	Insert the secondary class object but then the address of the secondary class object has to be placed in the fixed length area part
			//	for the secondary class member

			threadLog("NON ATOM\n") ;

			/*
			URGENT
			pObj2 = (hdbObject*) atom.Binary() ;
			pCache = (hdbObjCache*) m_pADP->mapRepositories[pObj2->Classname()] ;
			if (!pCache)
				return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Cannot locate cache for composite member %s", pObj2->Classname()) ;
			threadLog("%s. Inserting secondary into cache %s\n", *_fn, pObj2->Classname()) ;

			secId = 0 ;
			rc = pCache->Insert(secId, *pObj2) ;
			threadLog("%s. Returned from secondary insert with objId %d and err %s\n", *_fn, secId, Err2Txt(rc)) ;

			//memcpy(&strOrig, pVar, m_pMain->m_Sizes[romidB.m_MbrId]) ;
			m_pMain->GetVal(atom.m_Data, objId, romidB.m_MbrId) ;

			if (secId != atom.m_Data.m_uInt32)
			{
				//memcpy(pVar, &secId, m_pMain->m_Sizes[romidB.m_MbrId]) ;
				m_pMain->_setMbrValue(objId, romidB.m_MbrId, atom.m_Data) ;

				//	Now write out the string in delta notation
				Z.Printf("@c%d.o%d.m%d=%d\n", romidB.m_ClsId, objId, romidB.m_MbrId, secId) ;
			}
			*/

			continue ;
		}

		if (pMbr->Basetype() == BASETYPE_BINARY)
		{
			theChain = atom.Chain() ;

			if (theChain.Size())
			{
				threadLog("%s. Written Binary of %d bytes. Err=%s\n", *_fn, theChain.Size(), Err2Txt(rc)) ;
				rc = m_Binaries[romidB.m_MbrId]->Insert(binAddr, theChain) ;
				Z.Printf("@c%d.o%d.m%d=%u\n", romidB.m_ClsId, objId, romidB.m_MbrId, binAddr) ;
			}
		}
		else if (pMbr->Basetype() == BASETYPE_TXTDOC)
		{
			theChain = atom.Chain() ;

			if (theChain.Size())
			{
				threadLog("%s. Written Document of %d bytes. Err=%s\n", *_fn, theChain.Size(), Err2Txt(rc)) ;
				rc = m_Binaries[romidB.m_MbrId]->Insert(binAddr, theChain) ;
				Z.Printf("@c%d.o%d.m%d=%u\n", romidB.m_ClsId, objId, romidB.m_MbrId, binAddr) ;
			}
		}
		else if (pMbr->Basetype() == BASETYPE_STRING || pMbr->Basetype() == BASETYPE_DOMAIN || pMbr->Basetype() == BASETYPE_EMADDR || pMbr->Basetype() == BASETYPE_URL)
		{
			//	Handle strings and string-like datum. These are stored as string numbers.

			strVal = atom.Str() ;
			if (!strVal)
				continue ;

			threadLog("%s. A DOING member %d %s (type %s) val %s (type %s)\n",
				*_fn, romidB.m_MbrId, pMbr->TxtName(), Basetype2Txt(pMbr->Basetype()), *strVal, Basetype2Txt(atom.Type())) ;

			if (pMbr->Basetype() == BASETYPE_DOMAIN)
			{
				strNo = _hzGlobal_FST_Domain->Locate(*strVal) ;
				if (!strNo)
					strNo = _hzGlobal_FST_Domain->Insert(*strVal) ;
			}
			else if (pMbr->Basetype() == BASETYPE_EMADDR)
			{
				strNo = _hzGlobal_FST_Emaddr->Locate(*strVal) ;
				if (!strNo)
					strNo = _hzGlobal_FST_Emaddr->Insert(*strVal) ;
			}
			else
			{
				strNo = _hzGlobal_StringTable->Locate(*strVal) ;
				if (!strNo)
					strNo = _hzGlobal_StringTable->Insert(*strVal) ;
			}
			av.m_uInt32 = strNo ;
			m_pMain->_setMbrValue(objId, romidB.m_MbrId, av) ;

			//	Compose the delta
			Z.Printf("@c%d.o%d.m%d=", romidB.m_ClsId, objId, romidB.m_MbrId) ;

			for (i = *strVal ; *i ; i++)
			{
				if (*i == 0x01)		{ Z.AddByte(0x01) ; Z.AddByte(0x01) ; continue ; }
				if (*i == CHAR_CR)	{ Z.AddByte(CHAR_BKSLASH) ; Z.AddByte('r') ; continue ; }
				if (*i == CHAR_NL)	{ Z.AddByte(CHAR_BKSLASH) ; Z.AddByte('n') ; continue ; }

				Z.AddByte(*i) ;
			}

			Z.AddByte(CHAR_NL) ;
			//atom = strNo ;
		}
		else
		{
			//	Handle numeric type datums.
			strVal = atom.Str() ;
			threadLog("%s. B DOING member %d %s (type %s) val %s (type %s)\n",
				*_fn, romidB.m_MbrId, pMbr->TxtName(), Basetype2Txt(pMbr->Basetype()), *strVal, Basetype2Txt(atom.Type())) ;
			m_pMain->_setMbrValue(objId, romidB.m_MbrId, atom.Datum()) ;
			Z.Printf("@c%d.o%d.m%d=%s\n", romidB.m_ClsId, objId, romidB.m_MbrId, *strVal) ;
		}

		pIdx = m_Indexes[romidB.m_MbrId] ;

		if (pIdx)
		{
			if (pIdx->Whatami() == HZINDEX_UKEY)
			{
				pIdxU = (hdbIndexUkey*) pIdx ;
				ic = pIdxU->Insert(atom, objId) ;
				threadLog("%s. UKEY idx insert of %s returned err=%s\n", *_fn, *atom.Str(), Err2Txt(ic)) ;
			}

			if (pIdx->Whatami() == HZINDEX_ENUM)
			{
				pIdxE = (hdbIndexEnum*) pIdx ;
				ic = pIdxE->Insert(objId, atom) ;
				threadLog("%s. ENUM idx insert of %s returned err=%s\n", *_fn, *atom.Str(), Err2Txt(ic)) ;
			}
		}
	}

	threadLog("%s. Done member values\n", *_fn) ;

	//	Now write out new deltas
	if (rc == E_OK && Z.Size())
	{
		m_os << Z ;
		if (m_os.fail())
			{ rc = E_WRITEFAIL ; threadLog("%s. Failed to write total of %d bytes\n", *_fn, Z.Size()) ; }
		else
			{ m_os.flush() ; threadLog("%s. Written total of %d bytes\n", *_fn, Z.Size()) ; }

		if (_hzGlobal_DeltaClient)
			_hzGlobal_DeltaClient->DeltaWrite(Z) ;
	}

	return rc ;
}

hzEcode	hdbObjCache::Delete	(uint32_t objId)
{
	//	Mark as deleted, the object found at the supplied address. Write out an object deletion to the working and backup data files is they apply.
	//
	//	Arguments:	1)	objId	The target object id

	_hzfunc("hdbObjCache::Delete") ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	//	if (!mx->m_Objects.Exists(objId))
	//		return E_NOTFOUND ;

	if (m_bDeletes)
		return E_DELETE ;

	//	pOld = (char*) mx->m_Objects[objId] ;

	//	mx->m_Objects.Delete(objId) ;

	return E_OK ;
}

hzEcode	hdbObjCache::Update	(hdbObject& obj, uint32_t objId)
{
	//	Overwrite the object found at the supplied address, with the supplied object and update any affected indexes accordingly
	//
	//	Arguments:	1)	obj		The new version of the data object
	//				2)	objId	The object id of the original version
	//
	//				DEPRECATED

	_hzfunc("hdbObjCache::Update") ;

	uint32_t	nIndex ;		//	Member and index iterator

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	if (objId >= m_pMain->Count())
		return E_RANGE ;

	//	Now go thru object's class members filling in the allocated fixed length space
	for (nIndex = 0 ;; nIndex++)
	{
	}

	return E_OK ;
}

hzEcode	hdbObjCache::Fetch	(hdbObject& obj, uint32_t objId)
{
	//	Fetch populates the supplied object recipticle (hdbObject instance) with the object identified by the supplied object id.
	//
	//	The supplied recepticle is cleared by this function. If the supplied object id is invalid, the recepticle is left blank
	//
	//	Arguments:	1)	obj		The object
	//				2)	objId	The object id to fetch
	//
	//	Returns:	E_RANGE		The requested row is out of range
	//				E_OK		Operation success

	_hzfunc("hdbObjCache::Fetch") ;

	const hdbMember*	pMbr ;			//	Member pointer
	hzAtom			atom ;			//	Atom
	_atomval		av ;			//	Value
	hdbROMID		delta ;			//	Delta indicator
	hzString		S ;				//	Temp string
	uint32_t		nSlot ;			//	Set by AssignSlot
	uint32_t		mbrNo ;			//	Member and index iterator
	hzEcode			rc = E_OK ;		//	Return code

	//	Check init state and that supplied object is of the same class as the cache
	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	if (obj.Class() != m_pClass)
		hzexit(_fn, m_Name, E_TYPE, "Cache is of class %s. Supplied object is of class %s", obj.Classname(), Classname()) ;

	//	Clear all object members
	obj.Clear() ;

	if (objId > m_pMain->Count())
		return E_NOTFOUND ;

	//	First find the fixed length object space

	//	Then memcpy each atom from the fixed length object space
	rc = m_pMain->AssignSlot(nSlot, objId, 0) ;
	if (rc != E_OK)
		return rc ;

	delta.m_ClsId = m_pClass->ClassId() ;

	for (mbrNo = 0 ; mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		pMbr = m_pClass->GetMember(mbrNo) ;
		//if (pMbr->Class())
		if (pMbr->Basetype() == BASETYPE_CLASS)
			continue ;
		delta.m_MbrId = mbrNo ;

		av.m_uInt64 = 0 ;
		m_pMain->GetVal(av, objId, mbrNo) ;
		//obj.m_Values.Insert(delta, av) ;
		obj.SetValue(mbrNo, atom) ;

		//	switch	(pMbr->Basetype())
		//	{
		//	case BASETYPE_STRING:
		//	case BASETYPE_EMADDR:
		//	case BASETYPE_URL:		//	The string-likes are always stored as a dictionary address
		//	case BASETYPE_TEXT:		//	Text can be stored as a 		p_mem->m_nSize = 4 ;	break ;
		//	case BASETYPE_IPADDR:
		//	case BASETYPE_TIME:
		//	case BASETYPE_SDATE:
		//	case BASETYPE_XDATE:
		//	case BASETYPE_ENUM:		//	The specific data types are always stored as-is
		//	case BASETYPE_BOOL:		//	Bool is always found in the bitmap section of the _cache_blk block
		//	case BASETYPE_DOUBLE:
		//	case BASETYPE_INT64:
		//	case BASETYPE_INT32:
		//	case BASETYPE_INT16:
		//	case BASETYPE_BYTE:
		//	case BASETYPE_UINT64:
		//	case BASETYPE_UINT32:
		//	case BASETYPE_UINT16:
		//	case BASETYPE_UBYTE:	//	The numeric data types are always stored as-is
		//	case BASETYPE_BINARY:	//	The binary is in a hdbBinCron/Store
		//	case BASETYPE_TXTDOC:	//  The binary is in a hdbBinCron/Store
		//	}
	}

	return E_OK ;
}

hzEcode	hdbObjCache::Select	(hdbIdset& result, const char* cpSQL)
{
	//	Select rows according to the supplied SQL-esce criteria (arg 2) and populate the supplied hdbIdset of row ids (arg 1) with the results. The
	//	criteria must explicitly name members in the conditions and where these named members are indexed, the index will be consulted. Where named
	//	members are not indexed, the objects in the cache are scanned.
	//
	//	For efficiency, if there are indexed members named and the applicable operator to the condition is AND, these are handled first. Then any
	//	scanning need only involve the surviving set. But this only applies where the index can be used. If the member is a STRING and the critiria
	//	specifies a range (especially an implied range by use of wildcard characters) then a formula is applied to determine if scanning is the
	//	better option.
	//
	//	Note that as strings are stored as string numbers (keys to a string table) within the cache, searching on a range of values for a STRING type
	//	members is further complicated by the fact that the string numbers bear no relationship to the string values (the string number is related to
	//	the order in which strings are encountered). So a wildcard value that resulted in 100 adjacent strings in the string table would produce 100
	//	strings that were very unlikely to be adjacent. Furthermore, since the string table is shared among all the caches, not all the string numbers
	//	would nessesarily apply to the current cache and yet all would have to be searched for.
	//
	//	Arguments:	1)	result	The bitmap of object ids identified by the select operation
	//				2)	cpSql	The SQL-esce search criteria

	_hzfunc("hdbObjCache::Select_a") ;

	//	Parse expression
	hzEcode		rc = E_OK ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	result.Clear() ;

	//	if (!pExp->Parse(cpSQL))
	//		hzerr(_fn, HZ_ERROR, E_PARSE) ;

	return rc ;
}

hzEcode	hdbObjCache::Identify	(uint32_t& objId, uint32_t mbrNo, const hzAtom& atom)
{
	//	Identify a single object by matching on the supplied member name and value.
	//
	//	Arguments:	1)	objId	The object id identified by the operation (0 if not object found)
	//				2)	member	The name of member to be tested
	//				3)	atom	The value the member must have to identify the object
	//
	//	Returns@	E_NOINIT	If the object cache is not initialized
	//				E_CORRUPT	If the member does not exist within the cache data class
	//				E_NODATA	If the member is not indexed
	//				E_OK		If no errors occured

	_hzfunc("hdbObjCache::Identify(0)") ;

	const hdbMember*	pMbr ;		//	Member pointer
	hdbIndex*		pIdx ;		//	Index pointer
	hdbIndexUkey*	pIdxU ;		//	Index pointer
	//uint32_t		strNo ;		//	String number
	hzEcode			rc ;		//	Return code

	threadLog("%s. mbrNo %d value %s\n", *_fn, mbrNo, *atom.Str()) ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	objId = 0 ;
	pMbr = m_pClass->GetMember(mbrNo) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No class member number %d in class %s", mbrNo, m_pClass->TxtTypename()) ;

	//	Check for index
	pIdx = m_Indexes[mbrNo] ;
	if (!pIdx)
		return hzerr(_fn, HZ_ERROR, E_NODATA, "No index on member %s in class %s", pMbr->TxtName(), m_pClass->TxtTypename()) ;

	/*
	if (pMbr->Basetype() == BASETYPE_STRING || pMbr->Basetype() == BASETYPE_EMADDR || pMbr->Basetype() == BASETYPE_URL)
	{
		strNo = _hzGlobal_StringTable->Locate(*value) ;
		atom = strNo ;
	}
	else
		atom.SetValue(pMbr->Basetype(), value) ;
	*/

	if (pIdx->Whatami() != HZINDEX_UKEY)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Wrong index on member %s in class %s", pMbr->TxtName(), m_pClass->TxtTypename()) ;

	pIdxU = (hdbIndexUkey*) pIdx ;
	rc = pIdxU->Select(objId, atom) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "ERROR: Index selection on member %s in class %s", pMbr->TxtName(), m_pClass->TxtTypename()) ;
	return rc ;
}

hzEcode	hdbObjCache::Identify	(uint32_t& objId, uint32_t mbrNo, const hzString& value)
{
	//	Identify a single object by matching on the supplied member name and value.
	//
	//	Arguments:	1)	objId	The object id identified by the operation (0 if not object found)
	//				2)	member	The name of member to be tested
	//				3)	value	The value the member must have to identify the object
	//
	//	Returns@	E_NOINIT	If the object cache is not initialized
	//				E_CORRUPT	If the member does not exist within the cache data class
	//				E_NODATA	If the member is not indexed
	//				E_OK		If no errors occured

	_hzfunc("hdbObjCache::Identify(1)") ;

	const hdbMember*	pMbr ;		//	Member pointer
	hzAtom			atom ;		//	Atom from member
	hdbIndex*		pIdx ;		//	Index pointer
	hdbIndexUkey*	pIdxU ;		//	Index pointer
	//uint32_t		strNo ;		//	String number
	hzEcode			rc ;		//	Return code

	if (!this)
		hzexit(_fn, E_CORRUPT, "No instance") ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	objId = 0 ;
	pMbr = m_pClass->GetMember(mbrNo) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No class member number %d in class %s", mbrNo, m_pClass->TxtTypename()) ;

	//	Check for index
	pIdx = m_Indexes[mbrNo] ;
	if (!pIdx)
		return hzerr(_fn, HZ_ERROR, E_NODATA, "No index on member %s in class %s", pMbr->TxtName(), m_pClass->TxtTypename()) ;

	atom.SetValue(pMbr->Basetype(), value) ;

	if (pIdx->Whatami() != HZINDEX_UKEY)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Wrong index on member %s in class %s", pMbr->TxtName(), m_pClass->TxtTypename()) ;

	pIdxU = (hdbIndexUkey*) pIdx ;
	rc = pIdxU->Select(objId, atom) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "ERROR: Index selection on member %s in class %s", pMbr->TxtName(), m_pClass->TxtTypename()) ;
	return rc ;
}

hzEcode	hdbObjCache::Identify	(uint32_t& objId, const hzString& member, const hzString& value)
{
	//	Identify a single object by matching on the supplied member name and value.
	//
	//	Arguments:	1)	objId	The object id identified by the operation (0 if not object found)
	//				2)	member	The name of member to be tested
	//				3)	value	The value the member must have to identify the object
	//
	//	Returns@	E_NOINIT	If the object cache is not initialized
	//				E_CORRUPT	If the member does not exist within the cache data class
	//				E_NODATA	If the member is not indexed
	//				E_OK		If no errors occured

	_hzfunc("hdbObjCache::Identify(2)") ;

	const hdbMember*	pMbr ;		//	Member pointer
	hzAtom			atom ;		//	Atom from member
	hdbIndex*		pIdx ;		//	Index pointer
	hdbIndexUkey*	pIdxU ;		//	Index pointer
	uint32_t		mbrNo ;		//	Member number
	hzEcode			rc ;		//	Return code

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	objId = 0 ;
	pMbr = m_pClass->GetMember(member) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No class member of %s in class %s", *member, m_pClass->TxtTypename()) ;

	mbrNo = pMbr->Posn() ;
	threadLog("%s. mbrNo %d value %s\n", *_fn, mbrNo, *value) ;

	//	Check for index
	pIdx = m_Indexes[mbrNo] ;
	if (!pIdx)
		return hzerr(_fn, HZ_ERROR, E_NODATA, "No index on member %s in class %s", *member, m_pClass->TxtTypename()) ;

	atom.SetValue(pMbr->Basetype(), value) ;

	if (pIdx->Whatami() != HZINDEX_UKEY)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Wrong index on member %s in class %s", *member, m_pClass->TxtTypename()) ;

	pIdxU = (hdbIndexUkey*) pIdx ;
	rc = pIdxU->Select(objId, atom) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "ERROR: Index selection on member %s in class %s", pMbr->TxtName(), m_pClass->TxtTypename()) ;
	return rc ;
}

hzEcode	hdbObjCache::Fetchval	(hzAtom& atom, uint32_t mbrNo, uint32_t objId)
{
	//	Fetch into the supplied atom, a single atomic value from the numbered member of the supplied object (id)
	//
	//	Arguments:	1)	atom	The atom to be set to the member's value.
	//				2)	member	The name of member
	//				3)	objId	The id of the data object

	_hzfunc("hdbObjCache::Fetchval(mbrNo)") ;

	const hdbMember*	pMbr ;		//	Member pointer
	_atomval		av ;		//	Atom value
	hzString		S ;			//	Temp string
	uint32_t		nSlot ;		//	Set by AssignSlot
	uint32_t		strNo ;		//	String number
	hzEcode			rc ;

	atom.Clear() ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	pMbr = m_pClass->GetMember(mbrNo) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No class member identified class %s", m_pClass->TxtTypename()) ;

	//	Check the member is atomic
	//if (pMbr->Class())
	if (pMbr->Basetype() == BASETYPE_CLASS)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Member %s in class %s is composite", pMbr->TxtName(), m_pClass->TxtTypename()) ;

	if (objId < 1 || objId > m_pMain->Count())
		return hzerr(_fn, HZ_ERROR, E_RANGE, "Object id %d is out of range (pop is %d)", objId, m_pMain->Count()) ;

	//	Grab the value from this object element
	m_pMain->AssignSlot(nSlot, objId, 0) ;

	//	Get the value
	switch	(pMbr->Basetype())
	{
	case BASETYPE_DOMAIN:	rc = m_pMain->GetVal(av, objId, mbrNo) ;
							//threadLog("%s: Got an av of %u and ret of %s\n", *_fn, av.m_uInt32, Err2Txt(rc)) ;
							if (rc != E_OK)
								break ;
							strNo = av.m_uInt32 ;
							if (strNo)
								{ S = _hzGlobal_FST_Domain->Xlate(strNo) ; rc = atom.SetValue(pMbr->Basetype(), S) ; }
							break ;

	case BASETYPE_EMADDR:	rc = m_pMain->GetVal(av, objId, mbrNo) ;
							//threadLog("%s: Got an av of %u and ret of %s\n", *_fn, av.m_uInt32, Err2Txt(rc)) ;
							if (rc != E_OK)
								break ;
							strNo = av.m_uInt32 ;
							//threadLog("%s: Got a str no of %u which is %s\n", *_fn, strNo, _hzGlobal_FST_Emaddr->Xlate(strNo)) ;
							if (strNo)
								{ S = _hzGlobal_FST_Emaddr->Xlate(strNo) ; rc = atom.SetValue(pMbr->Basetype(), S) ; }
							break ;

	case BASETYPE_STRING:
	case BASETYPE_URL:		rc = m_pMain->GetVal(av, objId, mbrNo) ;
							//threadLog("%s: Got an av of %u and ret of %s\n", *_fn, av.m_uInt32, Err2Txt(rc)) ;
							if (rc != E_OK)
								break ;
							strNo = av.m_uInt32 ;
							if (strNo)
								{ S = _hzGlobal_StringTable->Xlate(strNo) ; rc = atom.SetValue(pMbr->Basetype(), S) ; }
							break ;

	case BASETYPE_TEXT:		//	Text can be stored as a hdbBinCron/Store
							break ;

	case BASETYPE_IPADDR:
	case BASETYPE_TIME:
	case BASETYPE_SDATE:
	case BASETYPE_XDATE:
	case BASETYPE_ENUM:		//	The specific data types are always stored as-is
							rc = m_pMain->GetVal(av, objId, mbrNo) ;
							break ;

	case BASETYPE_BOOL:		//	Bool is always found in the bitmap section of the _cache_blk block
							break ;

	case BASETYPE_DOUBLE:
	case BASETYPE_INT64:
							rc = m_pMain->GetVal(av, objId, mbrNo) ;
							break ;

	case BASETYPE_INT32:
	case BASETYPE_INT16:
	case BASETYPE_BYTE:
	case BASETYPE_UINT64:
	case BASETYPE_UINT32:
	case BASETYPE_UINT16:
	case BASETYPE_UBYTE:	//	The numeric data types are always stored as-is
							threadLog("%s - type num fetch on member %s\n", *_fn, pMbr->TxtName()) ;
							rc = m_pMain->GetVal(av, objId, mbrNo) ;
							break ;

	case BASETYPE_BINARY:	//	The binary is in a hdbBinCron/Store
							break ;

	case BASETYPE_TXTDOC:	//	The binary is in a hdbBinCron/Store
							break ;
	}

	return rc ;
}

hzEcode	hdbObjCache::Fetchval	(hzAtom& atom, const hzString& member, uint32_t objId)
{
	//	Fetch into the supplied atom, a single atomic value from the named member of the supplied object (id)
	//
	//	Arguments:	1)	atom	The atom to be set to the member's value.
	//				2)	member	The name of member
	//				3)	objId	The id of the data object

	_hzfunc("hdbObjCache::Fetchval(mbrName)") ;

	const hdbMember*	pMbr ;	//	Class member

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	pMbr = m_pClass->GetMember(member) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No class member of %s in class %s", *member, m_pClass->TxtTypename()) ;

	return Fetchval(atom, pMbr->Posn(), objId) ;
}

hzEcode	hdbObjCache::Fetchbin	(hzAtom& atom, const hzString& member, uint32_t objId)
{
	//	Fetch the actual binary content from a document/binary member. This is a two step process. Firstly in the object element itself, we have the
	//	address of the binary which will reside in a hdbBinCron or hdbBinStore instance. The Second part is lifting the actual value (reading in the
	//	binary)
	//
	//	Arguments:	1)	atom	The atom to be set to the member's value.
	//				2)	member	The name of member
	//				3)	objId	The id of the data object
	//
	//	Returns:	E_RANGE		If the object id is invalid
	//				E_TYPE		If the member is not BASETYPE_BINARY or BASETYPE_TXTDOC
	//				E_OK		If the operation was successful

	_hzfunc("hdbObjCache::Fetchbin") ;

	const hdbMember*	pMbr ;			//	Member pointer
	hzChain			Z ;				//	Chain to recieve the binary from the hdbBinCron
	_atomval		av ;			//	Atom value
	uint32_t		nSlot ;			//	Set by AssignSlot
	uint32_t		mbrNo ;			//	Member position
	uint32_t		addr ;			//	String number
	hzEcode			rc = E_OK ;		//	Return from hdbBinCron::Fetch

	atom.Clear() ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	pMbr = m_pClass->GetMember(member) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No class member of %s in class %s", *member, m_pClass->TxtTypename()) ;
	mbrNo = pMbr->Posn() ;

	if (objId < 1 || objId > m_pMain->Count())
		return E_RANGE ;

	//	And check member is a document or a binary
	if (pMbr->Basetype() != BASETYPE_BINARY && pMbr->Basetype() != BASETYPE_TXTDOC)
		return E_TYPE ;

	//	Grab the binary address from this object element
	m_pMain->AssignSlot(nSlot, objId, 0) ;

	m_pMain->GetVal(av, objId, mbrNo) ;
	addr = av.m_uInt32 ;

	//	Grab the binary value
	rc = m_Binaries[mbrNo]->Fetch(Z, addr) ;
	if (rc == E_OK)
		atom.SetValue(pMbr->Basetype(), Z) ;

	return rc ;
}

hzEcode	hdbObjCache::Fetchbin	(hzChain& Z, const hzString& member, uint32_t objId)
{
	//	Populate the supplied chain with the binary object held by the named member in the identified data object.
	//
	//	This assumes the named data class member holds binary objects and that the supplied data object id is valid. As object repositories do not directly hold
	//	binary values, the member value will be the address of the binary object residing in a separate binary repository. The address is then used in a Fetch()
	//	on the binary repository.
	//
	//	Arguments:	1)	atom	The atom to be set to the member's value.
	//				2)	member	The name of member
	//				3)	objId	The id of the data object
	//
	//	Returns:	E_RANGE		If the object id is invalid
	//				E_TYPE		If the member is not BASETYPE_BINARY or BASETYPE_TXTDOC
	//				E_OK		If the operation was successful

	_hzfunc("hdbObjCache::Fetchbin") ;

	const hdbMember*	pMbr ;		//	Member pointer
	_atomval		av ;			//	Atom value
	uint32_t		nSlot ;			//	Set by AssignSlot
	uint32_t		mbrNo ;			//	Member position
	uint32_t		addr ;			//	String number
	//hzMimetype	mt ;			//	For fetching mimetype from the hdbObjCron
	hzEcode			rc = E_OK ;		//	Return from hdbBinCron::Fetch

	//	Exit if cache not open
	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	pMbr = m_pClass->GetMember(member) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No class member of %s in class %s", *member, m_pClass->TxtTypename()) ;
	mbrNo = pMbr->Posn() ;

	if (objId < 1 || objId > m_pMain->Count())
		return E_RANGE ;

	//	And check member is a document or a binary
	if (pMbr->Basetype() != BASETYPE_BINARY && pMbr->Basetype() != BASETYPE_TXTDOC)
		return E_TYPE ;

	//	Grab the binary address from this object element
	m_pMain->AssignSlot(nSlot, objId, 0) ;

	m_pMain->GetVal(av, objId, mbrNo) ;
	addr = av.m_uInt32 ;

	//	Grab the binary value
	//rc = m_Binaries[mbrNo]->Fetch(Z, mt, addr) ;
	rc = m_Binaries[mbrNo]->Fetch(Z, addr) ;

	return rc ;
}

hzEcode	hdbObjCache::Fetchlist	(hzVect<uint32_t>& items, const hzString& member, uint32_t objId)
{
	//	For members that amount to lists (of either atomic values or class instances), fetch the list into the supplied vector (of uint32_t). The vector
	//	will then be used to iterate through real values for the member, stored in one of the auxillary RAM tables.
	//
	//	Arguments:	1)	items	The vector of items (uint32_t) which will serve as addresses
	//				2)	member	The class member (which must be defined as supporting multiple values)
	//				3)	objId	The data object id
	//
	//	Returns:	E_NOINIT	If the hdbObjCache is not fully initialized
	//				E_CORRUPT	If the named member does not exist
	//				E_RANGE		If the member is not a list (has max pop of 1)
	//				E_NOTFOUND	If the object id does not exist in the repository
	//				E_OK		If init state is full, member is a list and object exists - even if the member has no values.

	_hzfunc("hdbObjCache::Fetchlist") ;

	const hdbMember*	pMbr ;		//	Member pointer

	items.Clear() ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	pMbr = m_pClass->GetMember(member) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No class member of %s in class %s", *member, m_pClass->TxtTypename()) ;

	if (pMbr->MaxPop() == 1)
		return E_RANGE ;

	if (objId < 1 || objId > m_pMain->Count())
		return E_NOTFOUND ;

	//	if (!mx->m_Lists[pMbr->Posn()])
	//		return E_TYPE ;
	return E_OK ;
}
