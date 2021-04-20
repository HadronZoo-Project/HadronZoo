//
//	File:	hdbObject.cpp
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

//
//	Implimentation of the HadronZoo Proprietary Database Suite
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
#include "hzCodec.h"
#include "hzDocument.h"
#include "hzDirectory.h"
#include "hzDatabase.h"
#include "hzDelta.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Variables
*/

/*
**	hdbObject Functions
*/

hdbObject::hdbObject	(void)
{
	m_pClass = 0 ;
	m_ObjId = 0 ;
	m_ReposId = m_ClassId = m_ClassMax = 0 ;
	m_bInit = false ;
}

hdbObject::~hdbObject	(void)
{
	if (m_bInit)
		Clear() ;
}

hzEcode	hdbObject::Init	(const hzString& objKey, const hdbClass* pClass)
{
	//	Initialize a standalone object (hdbObject instance)
	//
	//	The standalone object must be named and assigned a data class. The name is required so that hdbObject instances residing within a Dissemino user session
	//	can be referenced by form handler commands - even though hdbObject has other application that do not require a name. The data class may either be simple
	//	or complex (using sub-class members).
	//
	//	This allocates a hzAtom instance for each member of the supplied data class.
	//
	//	Argument:	objClass	The data class
	//
	//	Returns:	E_CORRUPT	If called without a hdbObject instance
	//				E_ARGUMENT	If the class is not supplied
	//				E_NOINIT	If the supplied class is not initialized
	//				E_OK		If the hdbObject is initialized to the class

	_hzfunc("hdbObject::Init") ;

	const hdbMember*	pMbr ;	//	This class member

	uint32_t	nIndex ;	//	Member iterator
	hzEcode		rc = E_OK ;	//	Return code

	//	Check class and class init state
	if (!this)				return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No instance") ;
	if (!pClass)			return hzerr(_fn, HZ_ERROR, E_NOINIT, "No data class supplied") ;
	if (!pClass->IsInit())	return hzerr(_fn, HZ_ERROR, E_NOINIT, "Data class is not initialized") ;

	//	Go thru members
	for (nIndex = 0 ; rc == E_OK && nIndex < pClass->MbrCount() ; nIndex++)
	{
		pMbr = pClass->GetMember(nIndex) ;
		if (!pMbr)
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "No member in position %d", nIndex) ;

		if (!pMbr->TxtName())
			return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Member in position %d has no name", nIndex) ;
	}

	if (rc == E_OK)
	{
		m_pClass = pClass ;
		m_Key = objKey ;
		m_bInit = true ;
	}

	return rc ;
}

hzEcode	hdbObject::Clear	(void)
{
	//	Clear all hdbObject values
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	Object not initialized
	//				E_OK		Operation successful. All atoms at null values

	_hzfunc("hdbObject::Clear") ;

	if (!m_bInit)
		return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (!m_pClass)
		return hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	m_Values.Clear() ;

	return E_OK ;
}

bool	hdbObject::IsNull	(void) const
{
	//	Determine if the hdbObject is empty or null. An object is null only if all it's members (both atomic and composite) are null.
	//
	//	Arguments:	None
	//
	//	Returns:	True	If the object is empty
	//				False	If the object has a value

	if (!m_bInit)
		return true ;
	if (!m_pClass)
		return true ;
	if (!m_Values.Count())
		return true ;

	return false ;
}

//	Fnrp:	hdbObject::SetValue
//
//	Assign the value in the supplied atom to the member.
//
//	Arguments:	1)	mbrNo	The member number
//				2)	atom	The atom value the member will be set to
//
//	Returns:	E_NOINIT	If the object has not been initialized to a class
//				E_CORRUPT	If the supplied member number does not identify an object class member.
//				E_TYPE		If the object class member is not atomic (is another class) or if the supplie atom has the wrong type.
//				E_OK		If the object class member has been set to the supplied atom value.

hzEcode	hdbObject::SetValue	(uint32_t mbrNo, const hzAtom& atom)
{
	_hzfunc("hdbObject::SetValue(1)") ;

	const hdbMember*	pMbr ;	//	This class member

	hdbROMID	dc ;	//	Delta encoding
	_atomval	av ;	//	Value from atom
	uint32_t	val ;	//	Either a 32/16/8 bit value or offset into m_Strings
	hzEcode		rc ;	//	Return code

	//	Object has class?
	if (!m_bInit)	hzexit(_fn, 0, E_NOINIT, "Object not initialized") ;
	if (!m_pClass)	hzexit(_fn, 0, E_NOINIT, "Object has no class") ;

	//	Locate member and test if type is atomic
	if (mbrNo >= m_pClass->MbrCount())
		hzexit(_fn, m_Key, E_CORRUPT, "Class %s. No member %d", m_pClass->TxtTypename(), mbrNo) ;

	pMbr = m_pClass->GetMember(mbrNo) ;
	if (!pMbr)
		hzexit(_fn, m_Key, E_CORRUPT, "Class %s. No member %d", m_pClass->TxtTypename(), mbrNo) ;

	threadLog("%s. Inserting class %s, member %d, value [%s]\n", *_fn, m_pClass->TxtTypename(), mbrNo, *atom.Str()) ;
	if (atom.IsNull())
		return E_OK ;

	//	Set up ROMID
	dc.m_ClsId = m_pClass->ClassId() ;
	dc.m_MbrId = mbrNo ;

	switch	(pMbr->Basetype())
	{
    case BASETYPE_DOUBLE:
    case BASETYPE_INT64:
    case BASETYPE_UINT64:
    case BASETYPE_XDATE:	//	Extract 8 byte quantity from atom and add it to m_Large - The offset into m_Large goes in m_Values
							av = atom.Datum() ;
							val = m_Large.Count() ;
							m_Large.Add(av) ;
							m_Values.Insert(dc, val) ;
							break ;

    case BASETYPE_BOOL:		//	Excluded if false, the value part of m_Values is irrelevent.
							av = atom.Datum() ;
							if (av.m_Bool)
								m_Values.Insert(dc, 1) ;
							break ;

    case BASETYPE_TBOOL:	//	Always included so value part of m_Values is bool value.
							av = atom.Datum() ;
							m_Values.Insert(dc, av.m_Bool ? 1 : 0) ;
							break ;

    case BASETYPE_INT32:
    case BASETYPE_INT16:
    case BASETYPE_BYTE:
    case BASETYPE_UINT32:
    case BASETYPE_UINT16:
    case BASETYPE_UBYTE:
    case BASETYPE_IPADDR:
    case BASETYPE_TIME:
    case BASETYPE_SDATE:	//	These fir directly into value part of m_Values
							av = atom.Datum() ;
							val = av.m_uInt32 ;
							m_Values.Insert(dc, val) ;
							break ;

    case BASETYPE_BINARY:
    case BASETYPE_TXTDOC:	//	These are chains and are pointed to by the void* member of _atomval
							break ;

    case BASETYPE_ENUM:		//	These are either single or mutiple selection. In the single case they will fit into m_Values directly. In the multiple case they will only fit into
							//	m_Values if the set of enum values does not exceed 32.
							break ;

    case BASETYPE_CLASS:
    //	case BASETYPE_REPOS:	//  Not handled at the moment
							rc = E_TYPE ;
							break ;

	case BASETYPE_DOMAIN:
    case BASETYPE_EMADDR:
    case BASETYPE_URL:
    case BASETYPE_STRING:
    case BASETYPE_TEXT:
    case BASETYPE_APPDEF:	//	Extract string from atom and add it to m_Strings. Use the offset from m_Strings as the value to go in m_Values
							val = m_Strings.Count() ;
							m_Strings.Add(atom.Str()) ;
							m_Values.Insert(dc, val) ;
							return E_OK ;
	}

	return rc ;
}

hzEcode	hdbObject::SetValue	(uint32_t mbrNo, const hzString& value)
{
	//	Assign the supplied string value to the member as identified by the member number.
	//
	//	This is a matter of first setting an atom to have the member data type and the value in or implied in, the supplied string. Then the atom value is added
	//	to the object's m_Values collection using the member number as key.
	//
	//	Values for populating hdbObject members are commonly supplied in string form as this is how they arise in form submissions in a website.
	//
	//	Arguments:	1)	name	The member name
	//				2)	value	The string value the member will be set to
	//
	//	Returns:	E_NOINIT	If the object has not been initialized to a class
	//				E_CORRUPT	If the supplied member number does not identify an object class member.
	//				E_TYPE		If the object class member is not atomic (is another class) or if the supplie atom has the wrong type.
	//				E_OK		If the object class member has been set to the supplied atom value.

	_hzfunc("hdbObject::SetValue(2)") ;

	const hdbMember*	pMbr ;		//	Class member

	hdbROMID	dc ;	//	Delta encoding
	hzAtom		atom ;	//	Atom instance

	//	Object has class?
	if (!m_bInit)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (!m_pClass)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	//	Locate member and test if type is atomic
	if (mbrNo >= m_pClass->MbrCount())
		return hzerr(_fn, HZ_ERROR, E_CORRUPT) ;

	pMbr = m_pClass->GetMember(mbrNo) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT) ;

	if (pMbr->Basetype() == BASETYPE_CLASS)
		return E_TYPE ;

	atom.SetValue(pMbr->Basetype(), value) ;
	SetValue(mbrNo, atom) ;

	threadLog("%s. Inserting class %s, member %d, value %s\n", *_fn, m_pClass->TxtTypename(), mbrNo, *atom.Str()) ;
	return E_OK ;
}

hzEcode	hdbObject::SetValue	(const hzString& name, const hzString& value)
{
	//	Assign the supplied string value to the member as identified by the member name.
	//
	//	This is a matter of first setting an atom to have the member data type and the value in or implied in, the supplied string. Then the atom value is added
	//	to the object's m_Values collection using the member number as key.
	//
	//	Values for populating hdbObject members are commonly supplied in string form as this is how they arise in form submissions in a website.
	//
	//	Arguments:	1)	name	The member name
	//				2)	value	The string value the member will be set to
	//
	//	Returns:	E_NOINIT	If the object has not been initialized to a class
	//				E_CORRUPT	If the supplied member number does not identify an object class member.
	//				E_TYPE		If the object class member is not atomic (is another class) or if the supplie atom has the wrong type.
	//				E_OK		If the object class member has been set to the supplied atom value.

	_hzfunc("hdbObject::SetValue(3)") ;

	const hdbMember*	pMbr ;	//	Member pointer

	if (!m_bInit)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (!m_pClass)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	pMbr = m_pClass->GetMember(name) ;
	if (!pMbr)
		return E_CORRUPT ;

	return SetValue(pMbr->Posn(), value) ;
}

hzEcode	hdbObject::SetBinary	(const hzString& member, const hzChain& binary)
{
	//	Assign the supplied value to the named document or binary. This presumes the named member if of type BINARY and fails
	//	if this is not the case.
	//
	//	Arguments:	1)	member	Member name
	//				2)	binary	The data supplied as chain
	//				3)	docname	The original resouce name
	//				4)	mt		The MIME type
	//
	//	Returns:	E_NOINIT	If the object has not been initialized to a class
	//				E_CORRUPT	If the supplied member number does not identify an object class member.
	//				E_TYPE		If the object class member is not atomic (is another class) or if the supplie atom has the wrong type.
	//				E_OK		If the object class member has been set to the supplied atom value.

	_hzfunc("hdbObject::SetBinary") ;

	const hdbMember*	pMbr ;		//	Member pointer

	//	hzValset*	pVSet ;		//	Member's atom with this object

	if (!m_bInit)
		return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (!m_pClass)
		return hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	pMbr = m_pClass->GetMember(member) ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, m_Key, "Class %s has no member %s", m_pClass->TxtTypename(), *member) ;

	if (pMbr->Basetype() != BASETYPE_BINARY)
		return hzerr(_fn, HZ_ERROR, E_TYPE, m_Key, "%s->%s is not of binary data type", m_pClass->TxtTypename(), *member) ;

	//	pVSet->SetBinary(binary) ;
	return E_OK ;
}

hzEcode	hdbObject::GetValue	(hzAtom& atom, const hdbROMID& romid) const
{
	//	Set the supplied atom with the member value given by the supplied ROMID. This function will use the ROMID to identify the member and thus member data type. This determines
	//	if the value lays directly within m_Values, or if the m_Values value is to be used as offset to either m_Large or m_Strings
	//
	//	Arguments:	1)	atom	Target hzAtom instance
	//				2)	romid	Real object member identifier
	//
	//	Returns:	E_NOINIT	If the object has not been initialized to a class
	//				E_CORRUPT	If the supplied member number does not identify an object class member.
	//				E_TYPE		If the object class member is not atomic (is another class) or if the supplie atom has the wrong type.
	//				E_OK		If the object class member has been set to the supplied atom value.

	_hzfunc("hdbObject::GetValue") ;

	const hdbMember*	pMbr ;	//	This class member
	const hdbClass*		pCls ;	//	Applicable class

	_atomval	av ;	//	Value from atom
	uint32_t	val ;	//	Either a 32/16/8 bit value or offset into m_Strings
	hzEcode		rc ;	//	Return code

	//	Object has class?
	if (!m_bInit)	hzexit(_fn, 0, E_NOINIT, "Object not initialized") ;
	if (!m_pClass)	hzexit(_fn, 0, E_NOINIT, "Object has no class") ;

	pCls = m_pClass->GetADP()->GetDataClass(romid.m_ClsId) ;
	if (!pCls)
		pCls = m_pClass ;
	if (!pCls)
		hzexit(_fn, m_Key, E_CORRUPT, "No such class delta id %d", romid.m_ClsId) ;

	//	Locate member and test if type is atomic
	if (romid.m_MbrId >= pCls->MbrCount())
		hzexit(_fn, m_Key, E_CORRUPT, "Class %s has only %d members. No member %d", pCls->TxtTypename(), pCls->MbrCount(), romid.m_MbrId) ;

	pMbr = pCls->GetMember(romid.m_MbrId) ;
	if (!pMbr)
		hzexit(_fn, m_Key, E_CORRUPT, "Class %s. No member %d", pCls->TxtTypename(), romid.m_MbrId) ;

	//	Clear atom
	atom.Clear() ;

	//	Return if ROMID not found
	if (!m_Values.Exists(romid))
		return E_OK ;

	val = m_Values[romid] ;
	av.m_uInt64 = 0 ;

	switch	(pMbr->Basetype())
	{
    case BASETYPE_DOUBLE:
    case BASETYPE_INT64:
    case BASETYPE_UINT64:
    case BASETYPE_XDATE:	//	Extract 8 byte quantity from atom and add it to m_Large - The offset into m_Large goes in m_Values
							av = m_Large[val] ;
							atom.SetValue(pMbr->Basetype(), av) ;
							break ;

    case BASETYPE_BOOL:		//	Excluded if false, the value part of m_Values is irrelevent.
							atom = (bool) true ;
							break ;

    case BASETYPE_TBOOL:	//	Always included so value part of m_Values is bool value.
							atom = (bool) val ? true : false ;
							break ;

	//	32-bit entities
    case BASETYPE_INT32:	av.m_sInt32 = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;
    case BASETYPE_INT16:	av.m_sInt16 = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;
    case BASETYPE_BYTE:		av.m_sByte = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;
    case BASETYPE_UINT32:	av.m_uInt32 = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;
    case BASETYPE_UINT16:	av.m_uInt16 = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;
    case BASETYPE_UBYTE:	av.m_uByte = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;
    case BASETYPE_IPADDR:	av.m_uInt32 = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;
    case BASETYPE_TIME:		av.m_uInt32 = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;
    case BASETYPE_SDATE:	av.m_uInt32 = val ; atom.SetValue(pMbr->Basetype(), av) ;	break ;

    case BASETYPE_BINARY:
    case BASETYPE_TXTDOC:	//	These are chains and are pointed to by the void* member of _atomval
							break ;

    case BASETYPE_ENUM:		//	These are either single or mutiple selection. In the single case they will fit into m_Values directly. In the multiple case they will only fit into
							//	m_Values if the set of enum values does not exceed 32.
							break ;

    case BASETYPE_CLASS:
    //case BASETYPE_REPOS:	//  Not handled at the moment
							rc = E_TYPE ;
							break ;

    case BASETYPE_EMADDR:
    case BASETYPE_URL:
    case BASETYPE_STRING:
    case BASETYPE_TEXT:
    case BASETYPE_APPDEF:	//	Extract string from atom and add it to m_Strings. Use the offset from m_Strings as the value to go in m_Values
							atom = m_Strings[val] ;
							break ;
	}

	return rc ;
}

/*
**	_hdb_s_obj Import/Export Functions
*/

hzEcode	hdbObject::ImportSOBJ	(const hzChain& Z, uint32_t objId)
{
	//	Strictly for the purposes of LOADING from a hdbObjCache data object - Populate the object with the supplied hzChain which is to be interpreted as a serialized data object
	//
	//	Argument:	Z	The input serialize object
	//
	//	Returns:	E_FORMAT	If the supplied chain does not comply with the expected format
	//				E_OK		If the operation is successful

	_hzfunc("hdbObject::ImportSOBJ") ;

	const hdbMember*	pMbr ;	//	This class member
	//const hdbClass*		pCls ;	//	Applicable class

	chIter		zi ;				//	Input iterator
	char*		i ;					//	Buffer iterator
	char*		mbrLitmus ;			//	Litmus buffer
	_atomval	av ;				//	For 64 and 32 bit values
	hdbROMID	di ;				//	Delta encoding
	uint32_t	mbrNo ;				//	Member number (position within class)
	uint32_t	nC ;				//	Counter
	hzEcode		rc ;				//	Return code
	char		vbuf[8] ;			//	For memcpy to atomval

	//	Object has class?
	if (!m_bInit)	hzexit(_fn, 0, E_NOINIT, "Object not initialized") ;
	if (!m_pClass)	hzexit(_fn, 0, E_NOINIT, "Object has no class") ;

	//	Grab the litmus bits, puting the two bits for each member into a separate byte with values 0-3
	nC = m_pClass->MbrCount() ;
	nC += (4-(nC%4)) ;
	mbrLitmus = new char[nC+1] ;
	nC /= 4 ;

	for (zi = Z, i = mbrLitmus ; nC ; i += 4, zi++, nC--)
	{
		i[0] = 0 ;
		if (*zi & 0x080)	i[0] |= 2 ;
		if (*zi & 0x040)	i[0] |= 1 ;
		if (*zi & 0x020)	i[1] |= 2 ;
		if (*zi & 0x010)	i[1] |= 1 ;
		if (*zi & 0x008)	i[2] |= 2 ;
		if (*zi & 0x004)	i[2] |= 1 ;
		if (*zi & 0x002)	i[3] |= 2 ;
		if (*zi & 0x001)	i[3] |= 1 ;
	}

	//	Count up how many entries the fixed part will have
	for (mbrNo = 0 ; mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		pMbr = m_pClass->GetMember(mbrNo) ;

		di.m_ClsId = m_pClass->ClassId() ;
		di.m_ObjId = objId ;
		di.m_MbrId = pMbr->DeltaId() ;

		if (mbrLitmus[mbrNo] == 0)
			continue ;

		if (pMbr->Basetype() == BASETYPE_BOOL || pMbr->Basetype() == BASETYPE_TBOOL)
			continue ;

		if (pMbr->Basetype() == BASETYPE_ENUM)
		{
			switch	(mbrLitmus[mbrNo])
			{
			case 1:	//	Single value
			case 2:	//	Multiple values as bits in-situ
			case 3:	//	Multiple values as bitmap in variable space
				break ;
			}
		}

		switch	(pMbr->Basetype())
		{
		//	64-bit entities
		case BASETYPE_DOUBLE:
		case BASETYPE_INT64:
		case BASETYPE_UINT64:
		case BASETYPE_XDATE:	for (nC = 0 ; nC < 8 ; zi++, nC++)
									vbuf[nC] = *zi ;
								memcpy(&av, vbuf, 8) ;
								m_Values.Insert(di, av.m_uInt32) ;
								break ;

		//	32-bit entities
		case BASETYPE_INT32:
		case BASETYPE_UINT32:
		case BASETYPE_BINARY:
		case BASETYPE_TXTDOC:
		case BASETYPE_IPADDR:
		case BASETYPE_TIME:
		case BASETYPE_SDATE:	for (nC = 0 ; nC < 4 ; zi++, nC++)
									vbuf[nC] = *zi ;
								memcpy(&av.m_uInt32, vbuf, 4) ;
								m_Values.Insert(di, av.m_uInt32) ;
								break ;

		//	Strings
		case BASETYPE_DOMAIN:
		case BASETYPE_EMADDR:
		case BASETYPE_URL:
		case BASETYPE_APPDEF:
		case BASETYPE_STRING:	for (nC = 0 ; nC < 4 ; zi++, nC++)
									vbuf[nC] = *zi ;
								memcpy(&av.m_uInt32, vbuf, 4) ;
								m_Values.Insert(di, av.m_uInt32) ;
								break ;

		//	Smaller entities
		case BASETYPE_INT16:
		case BASETYPE_UINT16:	for (nC = 0 ; nC < 2 ; zi++, nC++)
									vbuf[nC] = *zi ;
								memcpy(&av.m_uInt16, vbuf, 4) ;
								m_Values.Insert(di, av.m_uInt32) ;
								break ;

		case BASETYPE_BYTE:
		case BASETYPE_UBYTE:	av.m_uByte = *zi++ ;
								m_Values.Insert(di, av.m_uInt32) ;
								break ;

		//	Special case of string stored directly
		case BASETYPE_TEXT:		//	if (str_val)
								//		{ av.m_uInt32 = m_Strings.Count() ; m_Strings.Add(str_val) ; m_Values.Insert(di, av.m_uInt32) ; }
								break ;
		}
	}

	//	Load from the fixed part

	//	Load from the count part

	return rc ;
}

hzEcode	hdbObject::ExportSOBJ	(const hzChain& Z) const
{
	//	Strictly for the purposes of formulating an encoded data object suitable for storing in a hdbObjCache.
}

/*
**	hdbObject Import/Export JSON Functions
*/

hzEcode	hdbObject::_import_json_r	(hzChain& error, hzChain::Iter& zi, const hdbClass* pClass)
{
	//	Recursive import the hdbObject value as a JSON
	//
	//	Argument:	J	The hzChain to be populated by the JSON value

	_hzfunc("hdbObject::_import_json_r") ;

	const hdbMember*	pMbr ;	//	Member

	hzAtom			atom ;			//	Atom value
	hdbROMID		di ;			//	Delta encoding
	hdbClass*		pSubClass ;		//	Sub-class
	hzChain			W ;				//	For building values
	hzXDate			xdate ;			//	Long form date
	_atomval		av ;			//	Temp value
	hzString		mbrName ;		//	Member name
	hzString		str_val ;		//	Temp string
	hzString		num_val ;		//	Temp string
	hzEmaddr		ema ;			//	Email address
	hzUrl			url ;			//	URL value
	hzSDate			sdate ;			//	Short form date
	hzTime			tm ;			//	hzTime
	hzIpaddr		ipa ;			//	IP address
	uint32_t		objId ;			//	Object id (start at 1)
	uint32_t		oset ;			//	Offset into either m_Large or m_Strings
	//bool			bConfirm ;		//	Used for testing integers
	bool			bMultObj ;		//	Expecting multiple objects
	bool			bMultVal ;		//	Expecting multiple objects
	hzEcode			rc = E_OK ;		//	Return code

	if (!pClass)
	{
		error.Printf("No class supplied\n") ;
		return E_ARGUMENT ;
	}

	//	Lose leading whitespace and check for [] block
	for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

	bMultObj = false ;
	if (*zi == CHAR_SQOPEN)
		{ bMultObj = true ; zi++ ; }

	//	Go thru {} object body
	objId = 1 ;
	for (; !zi.eof() && rc == E_OK ;)
	{
		for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

		//	Expect to be at object open
		if (*zi != CHAR_CUROPEN)
		{
			rc = E_FORMAT ;
			error.Printf("line %d col %d. No opening curly brace for object %d (at %c)\n", zi.Line(), zi.Col(), objId, *zi) ;
			break ;
		}
		for (zi++ ; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

		//	Do the members
		for (; rc == E_OK ;)
		{
			//	Expect member name followed by a colon, whitespace and a value
			for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
			if (*zi != CHAR_DQUOTE)
			{
				if (*zi == CHAR_CURCLOSE)
					break ;
				rc = E_FORMAT ;
				error.Printf("line %d col %d. Expected a quoted member name (at %c)\n", zi.Line(), zi.Col(), *zi) ;
				break ;
			}

			//	Get quoted value (handle escapes)
			for (zi++ ;; zi++)
			{
				if (*zi == CHAR_BKSLASH)
				{
					zi++ ;
					switch	(*zi)
					{
					case CHAR_DQUOTE:	W.AddByte(CHAR_DQUOTE) ;	break ;
					case CHAR_LC_N:		W.AddByte(CHAR_NL) ;		break ;
					default:
						W.AddByte(*zi) ;
					}
				}
				if (*zi == CHAR_DQUOTE)
					{ zi++ ; break ; }
				W.AddByte(*zi) ;
			}

			if (!W.Size())
			{
				rc = E_FORMAT ;
				error.Printf("No data class memeber name found on line %d col %d\n", zi.Line(), zi.Col()) ;
				break ;
			}
			if (*zi != CHAR_COLON)
			{
				rc = E_FORMAT ;
				error.Printf("Expected colon after name on line %d col %d\n", zi.Line(), zi.Col()) ;
				break ;
			}
		
			mbrName = W ;
			W.Clear() ;
			for (zi++ ; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
		
			//	LOOKUP MEMBER
			pMbr = pClass->GetMember(mbrName) ;
			if (!pMbr)
			{
				rc = E_FORMAT ;
				error.Printf("Line %d col %d. Member %s not found in class %s\n", zi.Line(), zi.Col(), *mbrName, pClass->TxtTypename()) ;
				break ;
			}

			if (pMbr->Basetype() == BASETYPE_CLASS)
			{
				//	Recurse to handle sub-class
				pSubClass = (hdbClass*) pMbr->Datatype() ;
				rc = _import_json_r(error, zi, pSubClass) ;
				continue ;
			}

			di.m_ClsId = pClass->ClassId() ;
			di.m_ObjId = objId ;
			di.m_MbrId = pMbr->DeltaId() ;

			bMultVal = false ;
			if (*zi == CHAR_SQOPEN)
				{ bMultVal = true ; zi++ ; }
			for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

			//	Expect member value. Can be single value or array but not an object as these are dealt with by sub-class recursion
			for (;;)
			{
				//	Get value
				if (*zi == CHAR_DQUOTE)
				{
					//	Get quoted value (handle escapes)
					for (zi++ ;; zi++)
					{
						if (*zi == CHAR_BKSLASH)
						{
							zi++ ;
							switch	(*zi)
							{
							case CHAR_DQUOTE:	W.AddByte(CHAR_DQUOTE) ;	break ;
							case CHAR_LC_N:		W.AddByte(CHAR_NL) ;		break ;
							default:
								W.AddByte(*zi) ;
							}
						}
						if (*zi == CHAR_DQUOTE)
							{ zi++ ; break ; }
						W.AddByte(*zi) ;
					}
				}
				else
				{
					for (; *zi >= CHAR_SPACE && *zi != CHAR_SCOLON ; zi++)
						W.AddByte(*zi) ;
				}
				str_val = W ;
				W.Clear() ;

				if (str_val == "null")
					str_val.Clear() ;

				for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

				//	Set atom
				if (str_val)
				{
					//bConfirm = false ;

					switch	(pMbr->Basetype())
					{
					//	64-bit entities
					case BASETYPE_DOUBLE:	if (IsDouble(av.m_Double, *str_val))
												{ oset = m_Large.Count() ; m_Large.Add(av) ; m_Values.Insert(di, oset) ; }
											break ;

					case BASETYPE_INT64:	if (IsInteger(av.m_sInt64, *str_val))
												{ oset = m_Large.Count() ; m_Large.Add(av) ; m_Values.Insert(di, oset) ; }
											break ;

					case BASETYPE_UINT64:	if (IsPosint(av.m_uInt64, *str_val))
												{ oset = m_Large.Count() ; m_Large.Add(av) ; m_Values.Insert(di, oset) ; }
											break ;

					case BASETYPE_XDATE:	xdate = str_val ;
											atom = xdate ;
											av = atom.Datum() ;
											oset = m_Large.Count() ;
											m_Large.Add(av) ;
											m_Values.Insert(di, oset) ;
											break ;

					//	32-bit entities
					case BASETYPE_INT32:	if (IsInteger(av.m_sInt32, *str_val))
												m_Values.Insert(di, av.m_sInt32) ;
											break ;

					case BASETYPE_INT16:	if (IsInteger(av.m_sInt16, *str_val))
												m_Values.Insert(di, av.m_sInt32) ;
											break ;

					case BASETYPE_BYTE:		if (IsInteger(av.m_sByte, *str_val))
												m_Values.Insert(di, av.m_sInt32) ;
											break ;

					case BASETYPE_UINT32:	if (IsPosint(av.m_uInt32, *str_val))
												m_Values.Insert(di, av.m_sInt32) ;
											break ;

					case BASETYPE_UINT16:	if (IsPosint(av.m_uInt16, *str_val))
												m_Values.Insert(di, av.m_sInt32) ;
											break ;

					case BASETYPE_UBYTE:	if (IsPosint(av.m_uByte, *str_val))
												m_Values.Insert(di, av.m_sInt32) ;
											break ;

					case BASETYPE_BINARY:	if (IsInteger(av.m_sInt32, *str_val))
												m_Values.Insert(di, av.m_sInt32) ;
											break ;

					case BASETYPE_TXTDOC:	if (IsInteger(av.m_sInt32, *str_val))
												m_Values.Insert(di, av.m_sInt32) ;
											break ;

					//	case BASETYPE_REPOS:	if (IsInteger(av.m_sInt32, *str_val))
					//								m_Values.Insert(di, av.m_sInt32) ;
					//							break ;

					case BASETYPE_IPADDR:	ipa = str_val ;		atom = ipa ;	av = atom.Datum() ; m_Values.Insert(di, av.m_uInt32) ;	break ;
					case BASETYPE_TIME:		tm = str_val ;		atom = tm ;		av = atom.Datum() ; m_Values.Insert(di, av.m_uInt32) ;	break ;
					case BASETYPE_SDATE:	sdate = str_val ;	atom = sdate ;	av = atom.Datum() ; m_Values.Insert(di, av.m_uInt32) ;	break ;

					//	Booleans
					case BASETYPE_BOOL:		if (str_val == "true")
												av.m_Bool = true ;
											else if (str_val == "false")
												av.m_Bool = false ;
											else
												{ rc = E_FORMAT ; error.Printf("Member %s is boolean. Only true/false allowed. Line %d\n", zi.Line()) ; }
											break ;

					//	Strings
					case BASETYPE_EMADDR:	ema = str_val ;
											if (ema.Length())
												{ av.m_uInt32 = m_Strings.Count() ; m_Strings.Add(str_val) ; m_Values.Insert(di, av.m_uInt32) ; }
											break ;

					case BASETYPE_URL:		url = str_val ;
											if (url.Length())
												{ av.m_uInt32 = m_Strings.Count() ; m_Strings.Add(str_val) ; m_Values.Insert(di, av.m_uInt32) ; }
											break ;

					case BASETYPE_APPDEF:
					case BASETYPE_ENUM:
					case BASETYPE_STRING:
					case BASETYPE_TEXT:		if (str_val)
												{ av.m_uInt32 = m_Strings.Count() ; m_Strings.Add(str_val) ; m_Values.Insert(di, av.m_uInt32) ; }
											break ;
					}

					//	if (bConfirm)
					//		m_Values.Insert(di, av) ;
					//	atom.Clear() ;
					//memset(&atom, 0, sizeof(hzAtom)) ;
				}

				//	Continue if multiple else break
				if (bMultVal)
				{
					//	Expect either a comma or a closing square brace
					if (*zi == CHAR_COMMA)
						{ zi++ ; continue ; }
					for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
					if (*zi == CHAR_SQCLOSE)
						zi++ ;
				}
				for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
				if (*zi != CHAR_SCOLON)
				{
					rc = E_FORMAT ;
					error.Printf("Line %d Col %d Expected semi-colon to end value (at %c)\n", zi.Line(), zi.Col(), *zi) ;
					break ;
				}
				zi++ ;
				break ;
			}
		}

		if (rc != E_OK)
			break ;

		for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
		if (*zi != CHAR_CURCLOSE)
		{
			rc = E_FORMAT ;
			error.Printf("Line %d Col %d Expected closing curly brace to end object (at %c)\n", zi.Line(), zi.Col(), *zi) ;
			break ;
		}
		for (zi++ ; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

		//	Continue if multiple object else break
		if (bMultObj)
		{
			//	Expect either a comma or a closing square brace
			if (*zi == CHAR_COMMA)
				{ zi++ ; objId++ ; continue ; }
			for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;
			if (*zi != CHAR_SQCLOSE)
			{
				rc = E_FORMAT ;
				error.Printf("Line %d Col %d Expected closing square brace to end object array (at %c)\n", zi.Line(), zi.Col(), *zi) ;
				break ;
			}
			zi++ ;
		}
		break ;
	}

	return rc ;
}

hzEcode	hdbObject::ImportJSON	(const hzChain& J)
{
	//	Import the hdbObject value as a JSON
	//
	//	Arguments:	1)	J	The JSON as hzChain

	_hzfunc("hdbObject::ImportJSON") ;

	hzChain		error ;			//	Gather all errors
	chIter		zi ;	//	Chain iterator
	hzEcode		rc ;	//	Return code

	if (!m_bInit)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (!m_pClass)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	if (!J.Size())
		return E_NODATA ;

	zi = J ;
	if (*zi != CHAR_CUROPEN)
		return E_FORMAT ;

	rc = _import_json_r(error, zi, m_pClass) ;
	if (rc != E_OK)	// || error.Size())
	{
		threadLog("START IMPORT ERROR\n") ;
		threadLog(error) ;
		threadLog("END IMPORT ERROR\n") ;
	}
	return rc ;
}

hzEcode	hdbObject::_export_json_r	(hzChain& J, const hdbClass* pClass, uint32_t nLevel) const
{
	//	Recursive JSON export by class.
	//
	//	The effect of this function is to create a JSON object enclosed in curly braces or an array of JSON objects enclosed in square braces. The map of values
	//	is in ascending order of class id, then object id and lastly member id. Where a member allows and has multiple values and is of an atomic data type, the
	//	values are adjacent as they share the same key. This provides a means to determine if the member's set of values should be enclosed in square braces.
	//
	//	Where members are of a sub-class data type, this function recurses. In the case where the class at hand is the host class, there will only be one onject
	//	but for subclasses, there are often multiple objects. Determining this is a case of ...
	//
	//	Where the supplied class is the host class, this will be singular by
	//	definition and so the output will be enclosed in curly braces. If the supplied class is a subclass the output could be an array - but only if the parent
	//	object member holds multiple instances. As an array must be enclosed in square braces, the presence of multiple instances must be determined first.
	//
	//	Arguments:	1)	J		The hzChain to be populated by the JSON value
	//				2)	nLevel	The object level

	_hzfunc("hdbObject::_export_json_r") ;

	const hdbClass*		pSub ;		//	Sub-class
	const hdbMember*	pMbr ;		//	Member pointer

	hzAtom			atom ;			//	Extracted value
	_atomval		av ;			//	Extracted value
	hdbROMID		di ;			//	Delta encoding
	hzString		X ;				//	Test string
	hzUrl			U ;				//	Test URL
	hzIpaddr		ipa ;			//	IP address
	hzTime			tm ;			//	hzTime
	hzSDate			sdate ;			//	Short form date
	hzSDate			xdate ;			//	Long form date
	uint32_t		mbrNo ;			//	Member number
	uint32_t		objId ;			//	Object number
	uint32_t		fstObj ;		//	First object number
	uint32_t		lstObj ;		//	Last object number
	uint32_t		nIndent ;		//	Indent iterator
	int32_t			val_Lo ;		//	First value of member
	int32_t			val_Hi ;		//	Last value of member
	int32_t			nV ;			//	Value iterator
	int32_t			val ;			//	Value from m_Values (value or offset into m_Large or m_Strings)
	hzRecep16		r16 ;			//	Recepticle
	hzRecep32		r32 ;			//	Recepticle

	//	CALCULATE FIRST & LAST OBJECTS
	if (pClass == m_pClass)
	{
		//	Host class. Can only be one object id
		fstObj = lstObj = 1 ;
	}
	else
	{
		//	Sub-class. Determine if there are multiple objects
		di.m_ClsId = pClass->ClassId() ;
		di.m_ObjId = 0 ;
		di.m_MbrId = 0 ;

		val_Lo = m_Values.Target(di) ;
		if (val_Lo < 0)
			threadLog("%s. Error level %d no instance of class %d\n", *_fn, nLevel, di.m_ClsId) ;
		di.m_ClsId++ ;
		val_Hi = m_Values.Target(di) ;
		if (val_Hi < 0)
			threadLog("%s. Error level %d no instance of higher class %d\n", *_fn, nLevel, di.m_ClsId) ;

		if (val_Hi > val_Lo)
			val_Hi-- ;

		di = m_Values.GetKey(val_Lo) ;
		fstObj = di.m_ObjId ;
		di = m_Values.GetKey(val_Hi) ;
		lstObj = di.m_ObjId ;
		threadLog("%s. Pop %d Level %d start %d end %d fst %d lst %d\n", *_fn, m_Values.Count(), nLevel, val_Lo, val_Hi, fstObj, lstObj) ;
	}

	//	OPEN OBJECT SET
	if (fstObj != lstObj)
	{
		//	If multiple object add an opening square brace and then the opening curly brace for 1st object.
		for (nIndent = 0 ; nIndent < nLevel ; nIndent++)
			J.AddByte(CHAR_TAB) ;
		J << "[\n" ;
		for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
			J.AddByte(CHAR_TAB) ;
		J << "{\n" ;
	}
	else
	{
		//	Add the opening curly brace to mark start of object
		for (nIndent = 0 ; nIndent < nLevel ; nIndent++)
			J.AddByte(CHAR_TAB) ;
		J << "{\n" ;
	}
	
	//	EXPORT ALL IN CLASS
	di.m_ClsId = pClass->ClassId() ;

	for (objId = fstObj ; objId <= lstObj ; objId++)
	{
		for (mbrNo = 0 ; mbrNo < pClass->MbrCount() ; mbrNo++)
		{
			pMbr = pClass->GetMember(mbrNo) ;

			di.m_ObjId = objId ;
			di.m_MbrId = pMbr->DeltaId() ;

			//	Recurse if sub-class
			if (pMbr->Basetype() == BASETYPE_CLASS)
			{
				pSub = (const hdbClass*) pMbr->Datatype() ;

				for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
					J.AddByte(CHAR_TAB) ;
				J.Printf("\"%s\":\n", pSub->TxtTypename()) ;

				_export_json_r(J, pSub, nLevel + 1) ;
				continue ;
			}

			//	Not a sub-class
			val_Lo = m_Values.First(di) ;
			if (val_Lo < 0)
			{
				for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
					J.AddByte(CHAR_TAB) ;
				J.Printf("\"%s\": null;\n", pMbr->TxtName()) ;
				continue ;
			}
			val_Hi = m_Values.Last(di) ;

			//	OPEN MEMBER
			if (val_Hi > val_Lo)
			{
				for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
					J.AddByte(CHAR_TAB) ;
				J.Printf("\"%s\": ", pMbr->TxtName()) ;
				for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
					J.AddByte(CHAR_TAB) ;
				J << "[ " ;
			}
			else
			{
				for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
					J.AddByte(CHAR_TAB) ;
				J.Printf("\"%s\": ", pMbr->TxtName()) ;
			}

			//	Loop thru values for the mbr
			for (nV = val_Lo ; nV <= val_Hi ; nV++)
			{
				val = m_Values.GetObj(nV) ;
				//threadLog("%s. Doing class %s member %d type %s - ", *_fn, pClass->TxtTypename(), mbrNo, Basetype2Txt(pMbr->Basetype())) ;

				//	Export the value
				switch	(pMbr->Basetype())
				{
				//	64-bit entities
				case BASETYPE_DOUBLE:	av = m_Large[val] ;	J.Printf("%f",	av.m_Double) ;	break ;
				case BASETYPE_INT64:	av = m_Large[val] ;	J.Printf("%dl",	av.m_sInt64) ;	break ;
				case BASETYPE_UINT64:	av = m_Large[val] ;	J.Printf("%ul",	av.m_uInt64) ;	break ;

				case BASETYPE_XDATE:	av = m_Large[val] ;
										xdate = av.m_uInt64 ;
										J.Printf("\"%s\"", *xdate.Str()) ;
										break ;

				//	32-bit entities
				case BASETYPE_INT32:	av.m_sInt32 = val ;	J.Printf("%d",	av.m_sInt32) ;		break ;
				case BASETYPE_INT16:	av.m_sInt16 = val ;	J.Printf("%d",	av.m_sInt16) ;		break ;
				case BASETYPE_BYTE:		av.m_sByte = val ;	J.Printf("%d",	av.m_sByte) ;		break ;
				case BASETYPE_UINT32:	av.m_uInt32 = val ;	J.Printf("%u",	av.m_uInt32) ;		break ;
				case BASETYPE_UINT16:	av.m_uInt16 = val ;	J.Printf("%u",	av.m_uInt16) ;		break ;
				case BASETYPE_UBYTE:	av.m_uByte = val ;	J.Printf("%u",	av.m_uByte) ;		break ;

				case BASETYPE_IPADDR:	ipa = val ;		J.Printf("\"%s\"", ipa.Txt(r16)) ;		break ;
				case BASETYPE_TIME:		tm = val ;		J.Printf("\"%s\"", tm.Txt(r16)) ;		break ;
				case BASETYPE_SDATE:	sdate = val ;	J.Printf("\"%s\"", sdate.Txt(r32)) ;	break ;

				//	Boolean
				case BASETYPE_BOOL:		if (val)
											J << "true" ;
										else
											J << "false" ;
										break ;

				//	Strings
				case BASETYPE_EMADDR:
				case BASETYPE_URL:
				case BASETYPE_STRING:
				case BASETYPE_APPDEF:
				case BASETYPE_TEXT:		J.AddByte(CHAR_DQUOTE) ;
										J << m_Strings[val] ;
										J.AddByte(CHAR_DQUOTE) ;
										break ;

				case BASETYPE_BINARY:	//  File assummed to be un-indexable (eg image). Stored on disk, infrequent change.
				case BASETYPE_TXTDOC:	//  Document from which text can be extracted/indexed. Stored on disk, infrequent change.
										J.Printf("%u", val) ;
										break ;

				case BASETYPE_ENUM:		//	Kludge for now
				case BASETYPE_CLASS:	break ;

				//	case BASETYPE_REPOS:	//  Export sub-class instance address
										J.Printf("%u", val) ;
										break ;
				}

				if (nV < val_Hi)
					J << ", " ;
			}

			//	CLOSE MEMBER
			if (val_Hi > val_Lo)
				J << "] ;\n" ;
			else
				J << ";\n" ;
		}

		if (objId < lstObj)
		{
			for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
				J.AddByte(CHAR_TAB) ;
			J << "},\n" ;
			for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
				J.AddByte(CHAR_TAB) ;
			J << "{\n" ;
		}
	}

	//	CLOSE OBJECT
	if (fstObj != lstObj)
	{
		for (nIndent = 0 ; nIndent <= nLevel ; nIndent++)
			J.AddByte(CHAR_TAB) ;
		J << "}\n" ;
		for (nIndent = 0 ; nIndent < nLevel ; nIndent++)
			J.AddByte(CHAR_TAB) ;
		J << "]\n" ;
	}
	else
	{
		for (nIndent = 0 ; nIndent < nLevel ; nIndent++)
			J.AddByte(CHAR_TAB) ;
		J << "}\n" ;
	}

	//threadLog("%s. UGH Now have %d bytes\n", *_fn, J.Size()) ;
	return E_OK ;
}

hzEcode	hdbObject::ExportJSON	(hzChain& J) const
{
	//	Export the hdbObject value as a JSON
	//
	//	Argument:	J	The hzChain to be populated by the JSON value

	_hzfunc("hdbObject::ExportJSON") ;

	if (!m_bInit)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;
	if (!m_pClass)	return hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	J.Clear() ;
	threadLog("START EXPORT\n") ;
	_export_json_r(J, m_pClass, 0) ;
	threadLog("DONE EXPORT\n") ;
}

hzEcode	hdbObject::ListSubs	(hzArray<uint32_t>& list, uint32_t clsId) const
{
	//	Find all sub-class objects for the given sub-class id.

	_hzfunc("hdbObject::ListSubs") ;

	hdbROMID	di ;			//	Delta encoding
	uint32_t	objId ;			//	Object number
	uint32_t	fstObj ;		//	First object number
	uint32_t	lstObj ;		//	Last object number
	int32_t		val_Lo ;		//	First value of member
	int32_t		val_Hi ;		//	Last value of member

	//	Sub-class. Determine if there are multiple objects
	di.m_ClsId = clsId ;
	di.m_ObjId = 0 ;
	di.m_MbrId = 0 ;

	list.Clear() ;

	val_Lo = m_Values.Target(di) ;
	if (val_Lo < 0)
		threadLog("%s. Error no instance of class %d\n", *_fn, di.m_ClsId) ;
	di.m_ClsId++ ;
	val_Hi = m_Values.Target(di) ;
	if (val_Hi < 0)
		threadLog("%s. Error no instance of higher class %d\n", *_fn, di.m_ClsId) ;

	if (val_Hi > val_Lo)
		val_Hi-- ;

	di = m_Values.GetKey(val_Lo) ;
	fstObj = di.m_ObjId ;
	di = m_Values.GetKey(val_Hi) ;
	lstObj = di.m_ObjId ;
	threadLog("%s. Pop %d start %d end %d fst %d lst %d\n", *_fn, m_Values.Count(), val_Lo, val_Hi, fstObj, lstObj) ;

	//	Now put object ids in the list
	for (objId = fstObj ; objId <= lstObj ; objId++)
	{
		list.Add(objId) ;
	}

	return E_OK ;
}
