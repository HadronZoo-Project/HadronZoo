//
//	File:	hdbClass.cpp
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

//	Group 1 Datatyeps: C++ Fundamental	
global	const hdbCpptype*	datatype_DOUBLE ;		//	64 bit floating point value
global	const hdbCpptype*	datatype_INT64 ;		//	64-bit Signed integer
global	const hdbCpptype*	datatype_INT32 ;		//	32-bit Signed integer
global	const hdbCpptype*	datatype_INT16 ;		//	16-bit Signed integer
global	const hdbCpptype*	datatype_BYTE ;			//	8-bit Signed integer
global	const hdbCpptype*	datatype_UINT64 ;		//	64-bit Positive integer
global	const hdbCpptype*	datatype_UINT32 ;		//	32-bit Positive integer
global	const hdbCpptype*	datatype_UINT16 ;		//	16-bit Positive integer
global	const hdbCpptype*	datatype_UBYTE ;		//	8-bit Positive integer
global	const hdbCpptype*	datatype_BOOL ;			//	either true or false

//	Group 2 Datatypes: HadronZoo Defined types (fixed size)
global	const hdbHzotype*	datatype_DOMAIN ;		//	Internet Domain
global	const hdbHzotype*	datatype_EMADDR ;		//	Email Address
global	const hdbHzotype*	datatype_URL ;			//	Universal Resource Locator
global	const hdbHzotype*	datatype_PHONE ;		//	Phone number (limited aphabet, standard form, likely to be unique to data object)
global	const hdbHzotype*	datatype_IPADDR ;		//	IP Address
global	const hdbHzotype*	datatype_TIME ;			//	No of seconds since midnight (4 bytes)
global	const hdbHzotype*	datatype_SDATE ;		//	No of days since Jan 1st year 0000
global	const hdbHzotype*	datatype_XDATE ;		//	Full date & time
global	const hdbHzotype*	datatype_STRING ;		//	Any string, treated as a single value
global	const hdbHzotype*	datatype_TEXT ;			//	Any string, treated as a series of words, stored on disk, frequent change
global	const hdbHzotype*	datatype_BINARY ;		//	File assummed to be un-indexable (eg image). Stored on disk, infrequent change.
global	const hdbHzotype*	datatype_TXTDOC ;		//	Document from which text can be extracted/indexed. Stored on disk, infrequent change.

extern	hzDeltaClient*	_hzGlobal_DeltaClient ;		//	Total current delta clients

/*
**	SECTION 1:	Miscellaneous Init Functions
*/

const char*	_hdb_showinitstate	(hdbIniStat nState)
{
	static	const char*	_istates[] =
	{
		"HDB_CLASS_INIT_NONE",
		"HDB_CLASS_INIT_PROG",
		"HDB_CLASS_INIT_DONE",
		"HDB_REPOS_INIT_PROG",
		"HDB_REPOS_INIT_DONE",
		"HDB_REPOS_OPEN",
		"HDB_REPOS_INIT_UNDEF",
		""
	} ;

	if (nState == HDB_CLASS_INIT_NONE)	return _istates[0] ;
	if (nState == HDB_CLASS_INIT_PROG)	return _istates[1] ;
	if (nState == HDB_CLASS_INIT_DONE)	return _istates[2] ;
	if (nState == HDB_REPOS_INIT_PROG)	return _istates[3] ;
	if (nState == HDB_REPOS_INIT_DONE)	return _istates[4] ;
	if (nState == HDB_REPOS_OPEN)		return _istates[5] ;

	return _istates[6] ;
}

void	_hdb_ck_initstate	(const hzFuncname& _fn, const hzString& objName, hdbIniStat eActual, hdbIniStat eExpect)
{
	//  Report cases of wrong initialization

	if (eActual == eExpect)
		return ;

	hzexit(_fn, objName, E_INITFAIL, "Target DB entity [%s] Actual init state %s. Expected %s\n",
		*objName, _hdb_showinitstate(eActual), _hdb_showinitstate(eExpect)) ;
}

/*
**	SECTION 2:	hdbEnum Functions
*/

hzEcode	hdbEnum::AddItem	(const hzString& strValue)
{
	//	Add a string value to the enumerated set of strings using the  default value system.

	uint32_t	strNo ;		//	String number

	if (!strValue)
		return E_ARGUMENT ;

	strNo = _hzGlobal_StringTable->Locate(*strValue) ;
	if (!strNo)
		strNo = _hzGlobal_StringTable->Insert(strValue) ;

	m_Items.Add(strNo) ;
	m_Values.Add(strNo) ;
}

hzEcode	hdbEnum::AddItem	(const hzString& strValue, uint32_t numValue)
{
	//	Add a string value to the enumerated set of strings but with a specified value.

	uint32_t	strNo ;		//	String number

	if (!strValue)
		return E_ARGUMENT ;

	strNo = _hzGlobal_StringTable->Locate(*strValue) ;
	if (!strNo)
		strNo = _hzGlobal_StringTable->Insert(strValue) ;

	m_Items.Add(strNo) ;
	m_Values.Add(numValue) ;
}

/*
**	SECTION 3:	hdbClass Functions
*/

hdbClass::hdbClass  (hdbADP& adp, hdbClsDgn designation)
{   
	m_pADP = &adp ;
	m_arrMembers.SetDefault((hdbMember*)0) ;
	//m_mapMembers.SetDefaultObj((hdbMember*)0) ;
	m_eClassInit = HDB_CLASS_INIT_NONE ;
	m_eDesignation = designation ;
	m_nMinSpace = 0 ; 
	m_nLitmusSize = 0 ; 
	m_ClassId = 0 ; 
	m_Basetype = BASETYPE_CLASS ;
}

hdbClass::~hdbClass (void)
{   
	_clear() ;
}

hzEcode	hdbMember::Init	(const hdbClass* pClass, const hdbDatatype* pType, const hzString& memName, uint32_t minPop, uint32_t maxPop, uint32_t nPosn)
{
	//	Initialize the data object class member with name and data type
	//
	//	Arguments:	1)	memName	The member name (this must be unique within the data object class)
	//				2)	type	The data type (must be a legal HadronZoo datatype)
	//				3)	minPop	Minimum number of values the member may have in a real class instance (for repository inserts)
	//				4)	maxPop	Maximum number of values
	//
	//	Returns:	E_ARGUMENT	If the name has not been supplied
	//				E_SETONCE	If we are attempting to rename the member
	//				E_OK		If operation was successful

	_hzfunc("hdbClass::Member::Init") ;

	//	Check if name supplied and that the current column is not already named
	if (!memName || !memName[0])
		return E_ARGUMENT ;
	if (m_Name)
		return E_SETONCE ;

	m_pClass = pClass ;
	m_Name = memName ;
	m_pType = pType ;
	m_popMin = minPop ;
	m_popMax = maxPop ;
	m_nPosn = nPosn ;

	return E_OK ;
}

void	hdbClass::_clear	(void)
{
	//	Private function to clear data object class descriptor. Note this is always assumed to be successful
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hdbClass::_clear") ;

	hdbMember*	pMbr ;	//	Member pointer
	uint32_t	x ;		//	Member iterator

	//	We only need this because the hzArray is of pointers

	for (x = 0 ; x < m_arrMembers.Count() ; x++)
	{
		pMbr = m_arrMembers[x] ;
		if (pMbr)
			delete pMbr ;
	}

	m_mapMembers.Clear() ;
	m_arrMembers.Clear() ;
}

hzEcode	hdbClass::InitStart	(const hzString& className)
{
	//	Init Start function. This must be called only on a hdbClass instance that is not initialized and which is not in the process of being
	//	initialized.
	//
	//	Arguments:	1)	className	The data class name
	//
	//	Returns:	E_ARGUMENT	No row class object named
	//				E_INITFAIL	Function called out of sequence: Must be first called and not repeated.
	//				E_MEMORY	Insufficient memory
	//				E_OK		Operation successful.

	_hzfunc("hdbClass::InitStart") ;

	//	initialize using a table config file

	if (!m_pADP)
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No Host ADP Found") ;

	if (!className)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No name supplied") ;

	if (m_pADP->GetPureClass(className))
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "hdbClass %s already exists", *className) ;

	if (m_eClassInit != HDB_CLASS_INIT_NONE)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Function called out of sequence: Must be first called and not repeated") ;

	m_Typename = className ;
	m_eClassInit = HDB_CLASS_INIT_PROG ;

	return E_OK ;
}

hzEcode	hdbClass::InitMember	(const hzString& mbrName, const hdbDatatype* pType, uint32_t minPop, uint32_t maxPop)
{
	//	Adds a member to the data class
	//
	//	Arguments:	1)	mbrName	Member name
	//				2)	type	The external data type
	//				3)	minPop	Minimum number of values the member may have in a real class instance (for repository inserts)
	//				4)	maxPop	Maximum number of values
	//
	//	Returns:	E_ARGUMENT	If no member name is supplied
	//				E_DUPLICATE	If a member of the supplied name already exists
	//				E_NOINIT	If the InitStart() function has not yet been called
	//				E_SEQUENCE	If the InitDone() function has been called
	//				E_OK		If the member is successfully added
	//
	//	Note:	This function will terminate execution if the member could not be allocated.

	_hzfunc("hdbClass::InitMember(1)") ;

	const hdbClass*		pSub ;	//	Sub-class if applicable
	hdbMember*			pMbr ;	//	The column

	hzString	S ;		//	Temp string
	hzEcode		rc ;	//	Return code

	//if (m_eClassInit < 1 || m_eClassInit > 2)
	if (m_eClassInit != HDB_CLASS_INIT_PROG)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Init state %d Called out of sequence (must be after InitStart() and before InitDone()", m_eClassInit) ;
	if (!m_pADP)
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No Host ADP Found") ;

	if (!mbrName)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No member name supplied") ;
	if (!pType)		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No member data type supplies for %s", *mbrName) ;

	if (GetMember(mbrName))
		return hzerr(_fn, HZ_ERROR, E_INITDUP, "Member (%s) already defined", *mbrName) ;

	pMbr = new hdbMember() ;
	if (!pMbr)
		return hzerr(_fn, HZ_ERROR, E_MEMORY, "Member allocation") ;

	//	Set up member
	rc = pMbr->Init(this, pType, mbrName, minPop, maxPop, m_arrMembers.Count()) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Please examine arguments for column [%s]", *mbrName) ;

	m_mapMembers.Insert(mbrName, pMbr) ;
	//pMbr->SetPosn(m_arrMembers.Count()) ;

	//	Assign member ID
	m_pADP->RegisterMember(pMbr) ;

	//	Put the member in the map and of columns
	m_mapMembers.Insert(mbrName, pMbr) ;
	m_arrMembers.Add(pMbr) ;

	//threadLog("%s. Added member %s (of %d) to class %s\n", *_fn, *mbrName, this->MbrCount(), this->TxtTypename()) ;
	threadLog("%s. Added member %s (of %d) to class %s\n", *_fn, *mbrName, MbrCount(), TxtTypename()) ;

	//	If member is a sub-class then put this in the m_mapSubs and all its subs
	if (pMbr->IsClass())
	{
		pSub = (const hdbClass*) pMbr->Datatype() ;

		m_pADP->NoteSub(StrTypename(), pSub) ;

		/*
		EXPAND LATER
		for (nC = 0 ; nC < pSub->m_mapSubs.Count() ; nC++)
		{
			S = pSub->m_mapSubs.GetKey(nC) ;
			pSubsub = pSub->m_mapSubs.GetObj(nC) ;

			m_mapSubs.Insert(S, pSubsub) ;
		}
		*/
	}

	m_eClassInit = HDB_CLASS_INIT_PROG ;
	return E_OK ;
}

hzEcode	hdbClass::InitMember	(const hzString& mbrName, const hzString& typeName, uint32_t minPop, uint32_t maxPop)
{
	_hzfunc("hdbClass::InitMember(2)") ;

	const hdbDatatype*	pDT ;	//	Data type

	if (m_eClassInit != HDB_CLASS_INIT_PROG)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Init state %d Called out of sequence (must be after InitStart() and before InitDone()", m_eClassInit) ;
	if (!m_pADP)
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No Host ADP Found") ;

	pDT = m_pADP->GetDatatype(typeName) ;

	if (!pDT)
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "No such data type as %s", *typeName) ;

	return InitMember(mbrName, pDT, minPop, maxPop) ;
}

hzEcode	hdbClass::InitDone	(void)
{
	//	Complete the initialization sequence.
	//
	//	This is a matter of calculating 'classwise' parameters and indicating that we have completed defining the class. The 
	//
	//	Arguments:	None
	//
	//	Returns:	E_SEQUENCE	If this function is not called after a successful InitStart() and InitMember()
	//				E_OK		If this function terminates an open initialization sequence

	_hzfunc("hdbClass::InitDone") ;

	const hdbMember*	pMbr ;		//	Member pointer
	uint32_t			mbrNo ;		//	Member number

	if (m_eClassInit != HDB_CLASS_INIT_PROG)
		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "Called out of sequence (must be after InitStart() and at least one call to InitMember()") ;

	//	Calculate which members need which storage provisions

	m_nMinSpace = m_arrMembers.Count()/4 ;
	if (m_arrMembers.Count()%4)
		m_nMinSpace++ ;

	for (mbrNo = 0 ; mbrNo < m_arrMembers.Count() ; mbrNo++)
	{
		pMbr = m_arrMembers[mbrNo] ;

		if (pMbr->Basetype() == BASETYPE_BOOL)
			continue ;

		if (pMbr->IsClass())
			continue ;

		m_nMinSpace += pMbr->MaxPop() ? 4 : pMbr->DataSize() ;
	}

	m_eClassInit = HDB_CLASS_INIT_DONE ;
	return E_OK ;
}

bool	hdbClass::operator==	(const hdbClass& op) const
{
	//	Test for equality between hdbClass instances (this row has same columns and column settings as the operand)
	//
	//	Arguments:	1)	op	The operand data class

	_hzfunc("hdbClass::operator==") ;

	const hdbMember*	pMA ;		//	Member pointer for this class
	const hdbMember*	pMB ;		//	Member pointer for other class
	uint32_t			nIndex ;	//	Member iterator

	if (MbrCount() != op.MbrCount())
		return false ;

	for (nIndex = 0 ; nIndex < m_arrMembers.Count() ; nIndex++)
	{
		pMA = m_arrMembers[nIndex] ;
		pMB = op.m_arrMembers[nIndex] ;

		if (pMA != pMB)
			return false ;
	}

	return true ;
}

/*
**	SECTION 5:	Application Delta Profile
*/

void	hdbClass::DescClass	(hzChain& Z, uint32_t nIndent) const
{
	//	Export Class Description
	//
	//	Write out XML fragment describing the data class to the supplied chain. Note that as the class description is likely to be part of an Application Delta
	//	Profile, the supplied chain is not pre-cleared by this function.
	//
	//	Arguments:	1)	Z		The chain to aggregate the class description to
	//				2)	nIndent	The number of leading tabs to apply to each line
	//
	//	Returns:	None

	_hzfunc("hdbClass::DescClass") ;

	const hdbDatatype*	pType ;		//	Member data type
	const hdbClass*		pSubClass ;	//	Sub-class where member is BASETYPE_CLASS
	const hdbMember*	pMbr ;		//	Member pointer

	uint32_t	mbrNo ;		//	Member iterator
	uint32_t	n ;			//	Indent counter

	for (n = nIndent ; n > 0 ; n--)
		Z.AddByte(CHAR_TAB) ;

	switch	(m_eDesignation)
	{
	case HDB_CLASS_DESIG_SYS:	Z.Printf("<class id=\"%d\" desig=\"sys\" name=\"%s\">\n", m_ClassId, TxtTypename()) ;	break ;
	case HDB_CLASS_DESIG_USR:	Z.Printf("<class id=\"%d\" desig=\"usr\" name=\"%s\">\n", m_ClassId, TxtTypename()) ;	break ;
	case HDB_CLASS_DESIG_CFG:	Z.Printf("<class id=\"%d\" desig=\"cfg\" name=\"%s\">\n", m_ClassId, TxtTypename()) ;	break ;
	}

	for (mbrNo = 0 ; mbrNo < MbrCount() ; mbrNo++)
	{
		pMbr = GetMember(mbrNo) ;

		for (n = nIndent ; n > 0 ; n--)
			Z.AddByte(CHAR_TAB) ;

		pType = pMbr->Datatype() ;

		if (!pType)
		{
			Z.Printf("\t<member posn=\"%d\" uid=\"%d\" min=\"%d\" max=\"%d\" datatype=\"INVALID_DATA_TYPE\" name=\"%s\"/>\n",
				pMbr->Posn(), pMbr->DeltaId(), pMbr->MinPop(), pMbr->MaxPop(), pMbr->TxtName()) ;
			continue ;
		}

		if (!pMbr->IsClass())
		{
			Z.Printf("\t<member posn=\"%d\" uid=\"%d\" min=\"%d\" max=\"%d\" datatype=\"%s\" name=\"%s\"/>\n",
				pMbr->Posn(), pMbr->DeltaId(), pMbr->MinPop(), pMbr->MaxPop(), pType->TxtTypename(), pMbr->TxtName()) ;
			continue ;
		}

		if (pMbr->Basetype() == BASETYPE_CLASS)
		{
			Z.Printf("\t<member posn=\"%d\" uid=\"%d\" min=\"%d\" max=\"%d\" subclass=\"%s\" name=\"%s\">\n",
				pMbr->Posn(), pMbr->DeltaId(), pMbr->MinPop(), pMbr->MaxPop(), pType->TxtTypename(), pMbr->TxtName()) ;
			pSubClass = (hdbClass*) pMbr->Datatype() ;
			pSubClass->DescClass(Z, nIndent + 1) ;

			for (n = nIndent ; n > 0 ; n--)
				Z.AddByte(CHAR_TAB) ;
			Z << "\t</member>\n" ;
			continue ;
		}
	}

	for (n = nIndent ; n > 0 ; n--)
		Z.AddByte(CHAR_TAB) ;
	Z << "</class>\n" ;
}

hzEcode	hdbClass::DescCheck	(hzChain& report, hzChain& desc) const
{
	//	Check Class Description
	//
	//	With the class initialized, it is possible to check that a class description in a delta or other repository file, matches the class. The description is
	//	supplied as a hzChain which is obtained by reading the file or file header. This function loads the hzChain into an XML document and then extracts tags
	//	describing the class and its members.
	//
	//	Write out XML fragment describing the data class to the supplied chain. Note that as the class description is likely to be part of an Application Delta
	//	Profile, the supplied chain is not pre-cleared by this function.
	//
	//	Arguments:	1)	report	To report errors
	//				2)	desc	The supplied description
	//
	//	Returns:	E_FORMAT	If there are ANY descrepancies (details in report)
	//				E_OK		If the description matches the class
	//
	//	Read a <class> tag on behalf of the hdbADProfile::Import function.

	_hzfunc("hdbClass::DescCheck") ;

	const hdbDatatype*	pType ;		//	Data type

	hzDocXml		doc ;			//	XML document
	hzXmlNode*		pRoot ;			//	XML root node
	hzXmlNode*		pN2 ;			//	Second level node
	hzAttrset		ai ;			//	Attribute iterator
	hzString		cname ;			//	Class name
	hzString		str_id ;		//	Member id
	hzString		str_uid ;		//	Member uid
	hzString		str_min ;		//	Member minPop
	hzString		str_max ;		//	Member minPop
	hzString		str_typ ;		//	Member data type
	hzString		str_sub ;		//	Member sub class
	hzString		str_nam ;		//	Member name
	hzEcode			rc = E_OK ;		//	Return code
	int16_t			n ;				//	Integer test value

	//	Load description into XML doc
	rc = doc.Load(desc) ;
	if (rc != E_OK)
		{ report << "Could not load XML document with supplied description\n" ; return E_SYNTAX ; }

	pRoot = doc.GetRoot() ;
	if (!pRoot)
		{ report << "No XML root found in supplied description\n" ; return E_SYNTAX ; }

	if (!pRoot->NameEQ("class"))
		{ report << "Expected first tag of <class>\n" ; return E_SYNTAX ; }

	//	Obtain <class> attributes
	str_id = cname = 0 ;

	for (ai = pRoot ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("id"))	str_id = ai.Value() ;
		else if (ai.NameEQ("name"))	cname = ai.Value() ;
		else
			{ rc = E_SYNTAX ; report.Printf("Line %d: <class> bad param %s=%s\n", pRoot->Line(), ai.Name(), ai.Value()) ; break ; }
	}

	//	Check class name and id
	if (!cname)
		report << "No class name supplied\n" ;
	else
	{
		if (cname != m_Typename)
			report.Printf("Name mismatch. Class actual name is %s. Description names class as %s.\n", *m_Typename, *cname) ;
	}

	if (!str_id)
		report << "No class id supplied\n" ;
	else
	{
		if (!IsInteger(n, *str_id))
			report.Printf("Illegal Class ID: Must be integer. (%s)\n", *str_id) ;
		if (n != m_ClassId)
			report.Printf("Class Delta ID mismatch. Class actual %d: Description id is %s\n", m_ClassId, *str_id) ;
	}

	//	Read in members
	for (pN2 = pRoot->GetFirstChild() ; pN2 ; pN2 = pN2->Sibling())
	{
		if (!pN2->NameEQ("member"))
			{ report.Printf("<class> only <member> allowed. %s unexpected\n", pN2->TxtName()) ; continue ; }

		str_id = str_uid = str_min = str_max = str_typ = str_sub = str_nam = 0 ;

		//	Read in member parameters
		for (ai = pN2 ; ai.Valid() ; ai.Advance())
		{
			if		(ai.NameEQ("posn"))		str_id = ai.Value() ;
			else if (ai.NameEQ("uid"))		str_uid = ai.Value() ;
			else if (ai.NameEQ("min"))		str_min = ai.Value() ;
			else if (ai.NameEQ("max"))		str_max = ai.Value() ;
			else if (ai.NameEQ("datatype"))	str_typ = ai.Value() ;
			else if (ai.NameEQ("subclass"))	str_sub = ai.Value() ;
			else if (ai.NameEQ("name"))		str_nam = ai.Value() ;
			else
				report.Printf("Line %d: <member> bad param %s=%s\n", pN2->Line(), ai.Name(), ai.Value()) ;
		}

		if (!str_id)	report.Printf("No member position suplied (line %d)\n", pN2->Line()) ;
		if (!str_uid)	report.Printf("No member UID suplied (line %d)\n", pN2->Line()) ;
		if (!str_min)	report.Printf("No member min pop suplied (line %d)\n", pN2->Line()) ;
		if (!str_max)	report.Printf("No member max pop suplied (line %d)\n", pN2->Line()) ;
		if (!str_nam)	report.Printf("No member name suplied (line %d)\n", pN2->Line()) ;

		if (!IsInteger(n, *str_id))		report.Printf("Illegal member position: Must be integer 0+ (line %d)\n", pN2->Line()) ;
		if (!IsInteger(n, *str_uid))	report.Printf("Illegal member ID: Must be integer 0+ (line %d)\n", pN2->Line()) ;
		if (!IsInteger(n, *str_min))	report.Printf("Illegal member min-POP: Must be integer 0+ (line %d)\n", pN2->Line()) ;
		if (!IsInteger(n, *str_max))	report.Printf("Illegal member max-POP: Must be integer 0+ (line %d)\n", pN2->Line()) ;

		if (!str_typ)
			str_typ = str_sub ;

		pType = m_pADP->GetDatatype(str_typ) ;
		if (!pType)
			report.Printf("<member> No such data type or sub-class as %s\n", pN2->Line(), *str_typ) ;
	}

	if (report.Size())
		return E_FORMAT ;
	return E_OK ;
}

void	hdbObjRepos::DescRepos	(hzChain& Z, uint32_t nIndent) const
{
	//	Export Repository Description
	//
	//	Write out XML fragment describing the data repository to the supplied chain. Note that as the class description is likely to be part of an Application
	//	Delta Profile, the supplied chain is not pre-cleared by this function.
	//
	//	Arguments:	1)	Z		The chain to aggregate the class description to
	//				2)	nIndent	The number of leading tabs to apply to each line
	//
	//	Returns:	None

	_hzfunc("hdbObjRepos::DescRepos") ;
}

hzEcode	hdbADP::Export	(void)
{
	//	Export the Application Delta Profile (ADP)
	//
	//	The current ADP for any application using the HadronZoo Database Suite (HDB), will be in a file 'appname.adp' in the /etc/hzDelta.d directory.

	_hzfunc("hdbADP::Export") ;

	hzMapS	<uint32_t,const hdbClass*>	classesById ;	//	For ordering classes by ID

	ofstream		os ;			//	Output stream
	ifstream		is ;			//	Input stream for previous ADP if applicable
	hzChain			Z ;				//	For building the ADP
	hzChain			Y ;				//	Previous ADP
	const hdbEnum*	pEnum ;			//	Data enum
	const hdbClass*	pClass ;		//	Data class
	hzString		fname ;			//	Filename
	hzString		bkfile ;		//	Filename
	uint32_t		nC ;			//	Data class iterator
	hzEcode			rc = E_OK ;		//	Return code

	fname = "/etc/hzDelta.d/" + m_appName + ".adp" ;
	bkfile = "/etc/hzDelta.d/" + m_appName + ".bak" ;

	threadLog("Exporting ADP to %s\n", *fname) ;

	if (TestFile(*fname) == E_OK)
	{
		is.open(*fname) ;
		Y << is ;
		is.close() ;
	}

	Z.Printf("<AppDeltaProfile app=\"%s\">\n", *m_appName) ;

	for (nC = 0 ; nC < m_mapEnums.Count() ; nC++)
	{
		pEnum = m_mapEnums.GetObj(nC) ;

		Z.Printf("\t<enum name=\"%s\"/>\n", pEnum->TxtTypename()) ;
	}

	for (nC = 0 ; nC < m_mapClasses.Count() ; nC++)
	{
		pClass = m_mapClasses.GetObj(nC) ;

		classesById.Insert(pClass->ClassId(), pClass) ;
	}

	for (nC = 0 ; nC < classesById.Count() ; nC++)
	{
		pClass = classesById.GetObj(nC) ;

		pClass->DescClass(Z, 1) ;
	}

	Z << "</AppDeltaProfile>\n" ;

	if (Y.Size())
	{
		if (Y == Z)
			{ threadLog("ADP is an exact match\n") ; return E_OK ; }
		threadLog("Backing up ADP\n") ;
		Filecopy(*bkfile, *fname) ;
	}

	threadLog("Exporting current ADP\n") ;
	os.open(*fname) ;
	os << Z ;
	os.close() ;

	return rc ;
}

hzEcode	hdbADP::_rdClass	(hzXmlNode* pN)
{
	//	Read a <class> tag on behalf of the hdbADProfile::Import function.

	_hzfunc("hdbADProfile::_readClass") ;

	const hdbDatatype*	pType ;		//	Data type

	hdbClass*		pClass ;		//	Data class
	hzXmlNode*		pN2 ;			//	Second level node
	hzAttrset		ai ;			//	Attribute iterator
	hzString		cname ;			//	Class name
	hzString		desig ;			//	Class designation
	hzString		str_id ;		//	Member id
	hzString		str_uid ;		//	Member uid
	hzString		str_min ;		//	Member minPop
	hzString		str_max ;		//	Member minPop
	hzString		str_typ ;		//	Member data type
	hzString		str_sub ;		//	Member sub class
	hzString		str_nam ;		//	Member name
	uint32_t		minPop ;		//	Minimum population
	uint32_t		maxPop ;		//	Maximum population
	hzEcode			rc = E_OK ;		//	Return code

	if (!pN->NameEQ("class"))
		return E_SYNTAX ;

	str_id = cname = 0 ;

	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("id"))		str_id = ai.Value() ;
		else if (ai.NameEQ("desig"))	desig = ai.Value() ;
		else if (ai.NameEQ("name"))		cname = ai.Value() ;
		else
			{ rc = E_SYNTAX ; threadLog("Line %d: <class> bad param %s=%s\n", pN->Line(), ai.Name(), ai.Value()) ; break ; }
	}

	if (!cname)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No class name supplied") ;
	if (!desig)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No class designation supplied") ;

	if		(desig == "sys")	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	else if (desig == "usr")	pClass = new hdbClass(*this, HDB_CLASS_DESIG_USR) ;
	else if (desig == "cfg")	pClass = new hdbClass(*this, HDB_CLASS_DESIG_CFG) ;
	else
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Invalid class designation supplied (%s). Must be sys|usr|cfg") ;

	pClass->InitStart(cname) ;

	threadLog("%s - Reading class %s\n", *_fn, *cname) ;
	m_mapDatatypes.Insert(cname, pClass) ;
	m_mapClasses.Insert(cname, pClass) ;

	//	Read in member parameters
	for (pN2 = pN->GetFirstChild() ; pN2 ; pN2 = pN2->Sibling())
	{
		if (!pN2->NameEQ("member"))
			{ rc = E_SYNTAX ; threadLog("Line %d: <class> only <member> allowed. %s unexpected\n", pN2->Line(), pN2->TxtName()) ; break ; }

		str_id = str_uid = str_min = str_max = str_typ = str_sub = str_nam = 0 ;

		for (ai = pN2 ; ai.Valid() ; ai.Advance())
		{
			if		(ai.NameEQ("posn"))		str_id = ai.Value() ;
			else if (ai.NameEQ("uid"))		str_uid = ai.Value() ;
			else if (ai.NameEQ("min"))		str_min = ai.Value() ;
			else if (ai.NameEQ("max"))		str_max = ai.Value() ;
			else if (ai.NameEQ("datatype"))	str_typ = ai.Value() ;
			else if (ai.NameEQ("subclass"))	str_sub = ai.Value() ;
			else if (ai.NameEQ("name"))		str_nam = ai.Value() ;
			else
				{ rc = E_SYNTAX ; threadLog("Line %d: <member> bad param %s=%s\n", pN2->Line(), ai.Name(), ai.Value()) ; break ; }
		}

		if (!str_typ)
			str_typ = str_sub ;

		pType = m_mapDatatypes[str_typ] ;
		if (!pType)
			{ rc = E_NOTFOUND ; threadLog("Line %d: <member> No such data type as %s\n", pN2->Line(), *str_typ) ; break ; }

		minPop = str_min ? atoi(*str_min) : 0 ;
		maxPop = str_max ? atoi(*str_max) : 0 ;

		pClass->InitMember(str_nam, pType, minPop, maxPop) ;
	}

	pClass->InitDone() ;

	return rc ;
}

hzEcode	hdbADP::Import	(const hzString& appName)
{
	//	Import the Application Delta Profile (ADP), of the named application.
	//
	//	The current ADP for any application using the HadronZoo Database Suite (HDB), will be in a file 'appname.adp' in the /etc/hzDelta.d directory.

	_hzfunc("hdbADP::Import") ;

	hzArray	<hzString>	ar ;		//	Enum values

	ifstream		is ;			//	Input stream for previous ADP if applicable
	hzDocXml		docADP ;		//	XML document
	hzChain			Z ;				//	For building the ADP
	hzChain			Y ;				//	Previous ADP
	hzXmlNode*		pRoot ;			//	Document root node
	hzXmlNode*		pN ;			//	First level node
	hzAttrset		ai ;			//	Attribute iterator
	hdbEnum*		pEnum ;			//	Data enum
	hzString		fname ;			//	Filename
	hzString		bkfile ;		//	Filename
	hzString		cname ;			//	Class name
	hzString		str_id ;		//	Member id
	hzString		str_uid ;		//	Member uid
	hzString		str_min ;		//	Member minPop
	hzString		str_max ;		//	Member minPop
	hzString		str_typ ;		//	Member data type
	hzString		str_sub ;		//	Member sub class
	hzString		str_nam ;		//	Member name
	hzString		S ;				//	Temp string (enum values)
	uint32_t		n ;				//	Enum value iterator
	uint32_t		strNo ;			//	String number
	hzEcode			rc = E_OK ;		//	Return code

	fname = "/etc/hzDelta.d/" + appName + ".adp" ;

	if (TestFile(*fname) == E_OK)
	{
		is.open(*fname) ;
		Y << is ;
		is.close() ;
	}

	//threadLog("%s. Loading ADP for %s\n", *_fn, *appName) ;

	rc = docADP.Load(*fname) ;
	if (rc != E_OK)
	{
		threadLog("%s. Could not load ADP document (%s)\n", *_fn, *fname) ;
		threadLog(docADP.Error()) ;
		return rc ;
	}

	pRoot = docADP.GetRoot() ;
	if (!pRoot)
	{
		threadLog("%s. ADP document (%s) has no route\n", *_fn, *fname) ;
		return E_NOINIT ;
	}

	for (pN = pRoot->GetFirstChild() ; pN ; pN = pN->Sibling())
	{
		if (pN->NameEQ("enum"))
		{
			for (ai = pN ; ai.Valid() ; ai.Advance())
			{
				if (ai.NameEQ("name"))
					str_nam = ai.Value() ;
				else
					{ rc = E_SYNTAX ; threadLog("Line %d: <enum> bad param %s=%s\n", pN->Line(), ai.Name(), ai.Value()) ; break ; }
			}

			pEnum = new hdbEnum() ;
			pEnum->SetTypename(str_nam) ;

			m_mapDatatypes.Insert(str_nam, pEnum) ;
			m_mapEnums.Insert(str_nam, pEnum) ;

			SplitCSV(ar, pN->m_fixContent) ;

			for (n = 0 ; n < ar.Count() ; n++)
			{
				S = ar[n] ;

				if (S.Length() > pEnum->m_nMax)
					pEnum->m_nMax = S.Length() ;

				strNo = _hzGlobal_StringTable->Locate(*S) ;
				if (!strNo)
					strNo = _hzGlobal_StringTable->Insert(*S) ;

				pEnum->m_Items.Add(strNo) ;
			}
		}
		else if (pN->NameEQ("class"))
		{
			rc = _rdClass(pN) ;
			if (rc != E_OK)
				break ;
		}
	}

	return rc ;
}

hzEcode	hdbADP::InitStandard	(const hzString& appName)
{
	//	Add the fundamental C++ and the HadronZoo in-built data types to the global map of datatypes _hzGlobal_Datatypes.
	//
	//	Arguments:	None
	//
	//	Returns:	E_SEQUENCE	If this function has already been called
	//				E_OK		Otherwise.

	_hzfunc("hdbADP::InitStandard") ;

	hdbCpptype*		ct ;	//	Pointer to Cpp data type
	hdbHzotype*		ht ;	//	Pointer to Cpp data type

	if (!this)
		Fatal("No ADP Instance") ;
	//if (m_mapDatatypes.Count())
		//return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "This function has already been called setting app name to %s", *m_appName) ;
	if (m_appName)
		Fatal("This function has already been called setting app name to ") ;
	if (!appName)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No application name supplied") ;
	m_appName = appName ;

	//	Group 1:	C++ Fundamental types
	datatype_DOUBLE	= ct = new hdbCpptype() ;	ct->SetTypename("double");	ct->SetBasetype(BASETYPE_DOUBLE);	m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_INT64	= ct = new hdbCpptype() ;	ct->SetTypename("int64");	ct->SetBasetype(BASETYPE_INT64);	m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_INT32	= ct = new hdbCpptype() ;	ct->SetTypename("int32");	ct->SetBasetype(BASETYPE_INT32);	m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_INT16	= ct = new hdbCpptype() ;	ct->SetTypename("int16");	ct->SetBasetype(BASETYPE_INT16);	m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_BYTE	= ct = new hdbCpptype() ;	ct->SetTypename("byte");	ct->SetBasetype(BASETYPE_BYTE);		m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_UINT64	= ct = new hdbCpptype() ;	ct->SetTypename("uint64");	ct->SetBasetype(BASETYPE_UINT64);	m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_UINT32	= ct = new hdbCpptype() ;	ct->SetTypename("uint32");	ct->SetBasetype(BASETYPE_UINT32);	m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_UINT16	= ct = new hdbCpptype() ;	ct->SetTypename("uint16");	ct->SetBasetype(BASETYPE_UINT16);	m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_UBYTE	= ct = new hdbCpptype() ;	ct->SetTypename("ubyte");	ct->SetBasetype(BASETYPE_UBYTE);	m_mapDatatypes.Insert(ct->StrTypename(),ct);
	datatype_BOOL	= ct = new hdbCpptype() ;	ct->SetTypename("bool");	ct->SetBasetype(BASETYPE_BOOL);		m_mapDatatypes.Insert(ct->StrTypename(),ct);

	//	Group 2:	HadronZoo in-built
	datatype_DOMAIN	= ht = new hdbHzotype() ;	ht->SetTypename("domain");	ht->SetBasetype(BASETYPE_DOMAIN);	m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_EMADDR	= ht = new hdbHzotype() ;	ht->SetTypename("emaddr");	ht->SetBasetype(BASETYPE_EMADDR);	m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_URL	= ht = new hdbHzotype() ;	ht->SetTypename("url");		ht->SetBasetype(BASETYPE_URL);		m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_IPADDR	= ht = new hdbHzotype() ;	ht->SetTypename("ipaddr");	ht->SetBasetype(BASETYPE_IPADDR);	m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_TIME	= ht = new hdbHzotype() ;	ht->SetTypename("time");	ht->SetBasetype(BASETYPE_TIME);		m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_SDATE	= ht = new hdbHzotype() ;	ht->SetTypename("sdate");	ht->SetBasetype(BASETYPE_SDATE);	m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_XDATE	= ht = new hdbHzotype() ;	ht->SetTypename("xdate");	ht->SetBasetype(BASETYPE_XDATE);	m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_STRING	= ht = new hdbHzotype() ;	ht->SetTypename("string");	ht->SetBasetype(BASETYPE_STRING);	m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_TEXT	= ht = new hdbHzotype() ;	ht->SetTypename("text");	ht->SetBasetype(BASETYPE_TEXT);		m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_BINARY	= ht = new hdbHzotype() ;	ht->SetTypename("binary");	ht->SetBasetype(BASETYPE_BINARY);	m_mapDatatypes.Insert(ht->StrTypename(),ht);
	datatype_TXTDOC	= ht = new hdbHzotype() ;	ht->SetTypename("txtdoc");	ht->SetBasetype(BASETYPE_TXTDOC);	m_mapDatatypes.Insert(ht->StrTypename(),ht);

	return E_OK ;
}

hzEcode	hdbADP::InitSubscribers	(const hzString& dataDir)
{
	//	Create the subscriber data class and repository

	_hzfunc("hdbADP::InitSubscribers") ;

	//static	bool	bBeenHere = false ;		//	Set once run

	hdbClass*		pClass ;		//	Subscriber class pointer
	hdbObjCache*	pRepos ;		//	Subscriber repository pointer
	hzString		S ;				//	Temporary string
	hzEcode			rc ;			//	Return code

	S = "subscriber" ;
	if (m_mapRepositories.Exists(S))
		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "This function has already been called") ;
	//bBeenHere = true ;

	if (!dataDir)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No application data directory") ;

	//	Allocate and initialize subscriber class
	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	rc = pClass->InitStart(S) ;

	if (rc == E_OK)	{ S = "username" ;	rc = pClass->InitMember(S, datatype_STRING, 1, 1) ; }
	if (rc == E_OK)	{ S = "userEmail" ;	rc = pClass->InitMember(S, datatype_EMADDR, 1, 1) ; }
	if (rc == E_OK)	{ S = "userpass" ;	rc = pClass->InitMember(S, datatype_STRING, 1, 1) ; }
	if (rc == E_OK)	{ S = "userUID" ;	rc = pClass->InitMember(S, datatype_UINT32, 1, 1) ; }
	if (rc == E_OK)	{ S = "userAddr" ;	rc = pClass->InitMember(S, datatype_UINT32, 1, 1) ; }
	if (rc == E_OK)	{ S = "userType" ;	rc = pClass->InitMember(S, datatype_STRING, 1, 1) ; }
	if (rc == E_OK)
		rc = pClass->InitDone() ;

	//	Resister subscriber class
	if (rc == E_OK)
		RegisterDataClass(pClass) ;

	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber class") ;

	//	Create and initialize susscriber repository
	pRepos = new hdbObjCache(*this) ;
	if (!pRepos)
		hzexit(_fn, 0, E_MEMORY, "No subsciber cache allocated") ;

	rc = pRepos->InitStart(pClass, pClass->StrTypename(), dataDir) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos") ;

	S = "username" ;
	rc = pRepos->InitMbrIndex(S, true) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos index") ;

	rc = pRepos->InitDone() ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not complete subsriber repos initialization") ;

	//	Resister subscriber repository
	rc = m_mapRepositories.Insert(pRepos->Name(), pRepos) ;
	return rc ;
}

hzEcode	hdbADP::InitSiteIndex	(const hzString& dataDir)
{
	//	Create the sitepage data class and repository

	_hzfunc("hdbADP::InitSiteIndex") ;

	//static	bool	bBeenHere = false ;		//	Set once run

	hdbClass*		pClass ;		//	Subscriber class pointer
	hdbObjCache*	pRepos ;		//	Subscriber repository pointer
	hzString		S ;				//	Temporary string
	hzEcode			rc ;			//	Return code

	S = "siteindex" ;
	if (m_mapRepositories.Exists(S))
		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "This function has already been called") ;
	//bBeenHere = true ;

	if (!dataDir)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No application data directory") ;

	//	Allocate and initialize subscriber class
	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	rc = pClass->InitStart(S) ;

	if (rc == E_OK)	{ S = "pageUrl" ;	rc = pClass->InitMember(S, datatype_STRING, 1, 1) ; }
	if (rc == E_OK)	{ S = "PageTitle" ;	rc = pClass->InitMember(S, datatype_STRING, 1, 1) ; }
	if (rc == E_OK)
		rc = pClass->InitDone() ;

	//	Resister subscriber class
	if (rc == E_OK)
		RegisterDataClass(pClass) ;

	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber class") ;

	//	Create and initialize susscriber repository
	pRepos = new hdbObjCache(*this) ;
	if (!pRepos)
		hzexit(_fn, 0, E_MEMORY, "No subsciber cache allocated") ;

	rc = pRepos->InitStart(pClass, pClass->StrTypename(), dataDir) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos") ;

	S = "pageUrl" ;
	rc = pRepos->InitMbrIndex(S, true) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos index") ;

	rc = pRepos->InitDone() ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not complete subsriber repos initialization") ;

	//	Resister subscriber repository
	rc = m_mapRepositories.Insert(pRepos->Name(), pRepos) ;
	return rc ;
}

#if 0
hzEcode	hdbADP::InitSearch	(void)
{
	//	Create the search data class

	_hzfunc("hdbADP::InitSearch") ;

	static	bool	bBeenHere = false ;		//	Set once run

	hdbClass*		pClass ;		//	Subscriber class pointer
	hzString		S ;				//	Temporary string
	hzEcode			rc ;			//	Return code

	if (bBeenHere)
		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "This function has already been called") ;
	bBeenHere = true ;

	//	Allocate and initialize subscriber class
	S = "sitesrch" ;
	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	rc = pClass->InitStart(S) ;

	if (rc == E_OK)
		{ S = "criteria" ;	rc = pClass->InitMember(S, datatype_STRING, 1, 1) ; }
	if (rc == E_OK)
		rc = pClass->InitDone() ;

	//	Resister subscriber class
	if (rc == E_OK)
		RegisterDataClass(pClass) ;

	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init sitesrch class") ;
	return E_OK ;
}
#endif

hzEcode	hdbADP::InitFinancials	(const hzString& dataDir)
{
	_hzfunc("hdbADP::InitFinancials") ;

	hdbEnum*		pEnum ;		//	Enum pointer
	hdbClass*		pClass ;	//	Data class pointer
	hdbObjCache*	pRepos ;	//	Repository pointer
	hzString		cname ;		//	Current data class name
	hzString		mname ;		//	Current member name
	hzEcode			rc ;		//	Return code

	cname = "Currency" ;
	if (m_mapRepositories.Exists(cname))
		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "This function has already been called") ;

	if (!dataDir)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No application data directory") ;

	//	Setup the 'Account Type' enum
	mname = "enumAccType" ;
	pEnum = new hdbEnum() ;
	pEnum->SetTypename(mname) ;

	pEnum->AddItem("ACC_NULL",		0x0000) ;
	pEnum->AddItem("ACC_ASSET",		0x0001) ;
	pEnum->AddItem("ACC_BANK",		0x0002) ;
	pEnum->AddItem("ACC_CASH",		0x0004) ;
	pEnum->AddItem("ACC_DIRECTOR",	0x0008) ;
	pEnum->AddItem("ACC_FOREX",		0x0010) ;
	pEnum->AddItem("ACC_GOV",		0x0020) ;
	pEnum->AddItem("ACC_SHARE",		0x0040) ;
	pEnum->AddItem("ACC_STOCK",		0x0080) ;
	pEnum->AddItem("ACC_TRADE",		0x0100) ;

	RegisterDataEnum(pEnum) ;

	/*
	**	Currencies
	*/

	//	Set up Currency data class
	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	rc = pClass->InitStart(cname) ;

	if (rc == E_OK)	{ mname = "Name" ;		rc = pClass->InitMember(mname, "string", 1, 1) ; }
	if (rc == E_OK)	{ mname = "Symbol" ;	rc = pClass->InitMember(mname, "string", 1, 1) ; }

	rc = pClass->InitDone() ;
	if (rc == E_OK)
		RegisterDataClass(pClass) ;

	//	Create and initialize Currency repository
	pRepos = new hdbObjCache(*this) ;
	if (!pRepos)
		hzexit(_fn, 0, E_MEMORY, "No Currency cache allocated") ;

	rc = pRepos->InitStart(pClass, pClass->StrTypename(), dataDir) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos") ;

	mname = "Name" ;
	rc = pRepos->InitMbrIndex(mname, true) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos index") ;

	rc = pRepos->InitDone() ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not complete subsriber repos initialization") ;

	rc = m_mapRepositories.Insert(pRepos->Name(), pRepos) ;
	pRepos->Open() ;

	/*
	**	Categories
	*/

	//	Set up Currency data class
	cname = "Category" ;
	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	rc = pClass->InitStart(cname) ;

	if (rc == E_OK)	{ mname = "Code" ;	rc = pClass->InitMember(mname, "string", 1, 1) ; }
	if (rc == E_OK)	{ mname = "Desc" ;	rc = pClass->InitMember(mname, "string", 1, 1) ; }

	rc = pClass->InitDone() ;
	if (rc == E_OK)
		RegisterDataClass(pClass) ;

	//	Create and initialize Currency repository
	pRepos = new hdbObjCache(*this) ;
	if (!pRepos)
		hzexit(_fn, 0, E_MEMORY, "No Currency cache allocated") ;

	rc = pRepos->InitStart(pClass, pClass->StrTypename(), dataDir) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos") ;

	mname = "Code" ;
	rc = pRepos->InitMbrIndex(mname, true) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos index") ;

	rc = pRepos->InitDone() ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not complete subsriber repos initialization") ;

	rc = m_mapRepositories.Insert(pRepos->Name(), pRepos) ;
	pRepos->Open() ;

	/*
	**	Accounts
	*/

	//	Set up Account data class
	cname = "Account" ;
	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	rc = pClass->InitStart(cname) ;

	if (rc == E_OK)	{ mname = "Opened" ;	rc = pClass->InitMember(mname, "sdate",			1, 1) ; }
	if (rc == E_OK)	{ mname = "Closed" ;	rc = pClass->InitMember(mname, "sdate",			1, 1) ; }
	if (rc == E_OK)	{ mname = "Currency" ;	rc = pClass->InitMember(mname, "string",		1, 1) ; }
	if (rc == E_OK)	{ mname = "Code" ;		rc = pClass->InitMember(mname, "string",		1, 1) ; }
	if (rc == E_OK)	{ mname = "Desc" ;		rc = pClass->InitMember(mname, "string",		1, 1) ; }
	if (rc == E_OK)	{ mname = "InitBal" ;	rc = pClass->InitMember(mname, "int32",			1, 1) ; }
	if (rc == E_OK)	{ mname = "Type" ;		rc = pClass->InitMember(mname, "enumAccType",	1, 1) ; }

	rc = pClass->InitDone() ;
	if (rc == E_OK)
		RegisterDataClass(pClass) ;

	//	Create and initialize Account repository
	pRepos = new hdbObjCache(*this) ;
	if (!pRepos)
		hzexit(_fn, 0, E_MEMORY, "No Currency cache allocated") ;

	rc = pRepos->InitStart(pClass, pClass->StrTypename(), dataDir) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos") ;

	mname = "Code" ;
	rc = pRepos->InitMbrIndex(mname, true) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos index") ;

	rc = pRepos->InitDone() ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not complete subsriber repos initialization") ;

	rc = m_mapRepositories.Insert(pRepos->Name(), pRepos) ;
	pRepos->Open() ;

	/*
	**	Transactions
	*/

	//	Set up Transaction data class
	cname = "Transaction" ;
	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	rc = pClass->InitStart(cname) ;

	if (rc == E_OK)	{ mname = "Date" ;		rc = pClass->InitMember(mname, "sdate",		1, 1) ; }
	if (rc == E_OK)	{ mname = "Currency" ;	rc = pClass->InitMember(mname, "string",	1, 1) ; }
	if (rc == E_OK)	{ mname = "Category" ;	rc = pClass->InitMember(mname, "string",	1, 1) ; }
	if (rc == E_OK)	{ mname = "From" ;		rc = pClass->InitMember(mname, "string",	1, 1) ; }
	if (rc == E_OK)	{ mname = "To" ;		rc = pClass->InitMember(mname, "string",	1, 1) ; }
	if (rc == E_OK)	{ mname = "Desc" ;		rc = pClass->InitMember(mname, "string",	1, 1) ; }
	if (rc == E_OK)	{ mname = "Note" ;		rc = pClass->InitMember(mname, "string",	1, 1) ; }
	if (rc == E_OK)	{ mname = "Qty" ;		rc = pClass->InitMember(mname, "string",	1, 1) ; }
	if (rc == E_OK)	{ mname = "Value" ;		rc = pClass->InitMember(mname, "int32",		1, 1) ; }
	if (rc == E_OK)	{ mname = "Type" ;		rc = pClass->InitMember(mname, "int32",		1, 1) ; }

	rc = pClass->InitDone() ;
	if (rc == E_OK)
		RegisterDataClass(pClass) ;

	//	Create and initialize Transaction repository
	pRepos = new hdbObjCache(*this) ;
	if (!pRepos)
		hzexit(_fn, 0, E_MEMORY, "No Currency cache allocated") ;

	rc = pRepos->InitStart(pClass, pClass->StrTypename(), dataDir) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos") ;

	mname = "Date" ;
	rc = pRepos->InitMbrIndex(mname, true) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos index") ;

	mname = "From" ;
	rc = pRepos->InitMbrIndex(mname, true) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos index") ;

	rc = pRepos->InitDone() ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not complete subsriber repos initialization") ;

	rc = m_mapRepositories.Insert(pRepos->Name(), pRepos) ;
	pRepos->Open() ;

	return rc ;
}

hzEcode	hdbADP::InitBlockedIPs	(const hzString& dataDir)
{
	//	Create the Blocked-IP data class and repository

	_hzfunc("hdbADP::InitBlockedIPs") ;

	static	bool	bBeenHere = false ;		//	Set once run

	hdbClass*		pClass ;		//	Subscriber class pointer
	hdbObjCache*	pRepos ;		//	Subscriber repository pointer
	hzString		S ;				//	Temporary string
	hzEcode			rc ;			//	Return code

	if (bBeenHere)
		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "This function has already been called") ;
	bBeenHere = true ;

	if (!dataDir)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No application data directory") ;

	//	Allocate and initialize subscriber class
	S = "blockedIP" ;
	pClass = new hdbClass(*this, HDB_CLASS_DESIG_SYS) ;
	rc = pClass->InitStart(S) ;

	if (rc == E_OK)	{ S = "ipa" ;	rc = pClass->InitMember(S, datatype_IPADDR, 1, 1) ; }
	if (rc == E_OK)
		rc = pClass->InitDone() ;

	//	Resister subscriber class
	if (rc == E_OK)
		RegisterDataClass(pClass) ;

	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init blockedIP class") ;

	//	Create and initialize susscriber repository
	pRepos = new hdbObjCache(*this) ;
	if (!pRepos)
		hzexit(_fn, 0, E_MEMORY, "No blockedIP cache allocated") ;

	rc = pRepos->InitStart(pClass, pClass->StrTypename(), dataDir) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init blockedIP repos") ;

	S = "ipa" ;
	rc = pRepos->InitMbrIndex(S, true) ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not init subsriber repos index") ;

	rc = pRepos->InitDone() ;
	if (rc != E_OK)
		hzexit(_fn, 0, rc, "Could not complete subsriber repos initialization") ;

	//	Resister subscriber repository
	rc = m_mapRepositories.Insert(pRepos->Name(), pRepos) ;
	return rc ;
}

void	hdbADP::Report	(hzChain& Z)
{
	//	Write out ADP
	//
	//	Diagnostics
	//
	//	Argument:	Z	Output chain
	//
	//	Returns:	None

	_hzfunc("hdbADP::Report") ;

	const hdbDatatype*	pDT ;		//	Data type
	const hdbClass*		pClass ;	//	Data class
	const hdbMember*	pMbr ;		//	Data class member
	hdbEnum*			pSlct ;		//	Data enum

	uint32_t	x ;			//	General iterator
	uint32_t	y ;			//	General iterator

	Z.Printf("APPLICATION DELTA PROFILE\n") ;

	//	Z.Printf("User Categories\n") ;
	//	for (x = 0 ; x < m_UserTypes.Count() ; x++)
	//		{ ut = m_UserTypes.GetObj(x) ; Z.Printf(" -- %s\n", *ut.m_Refname) ; }

	Z.Printf("Data types\n") ;
	Z.Printf(" -- Validation formats\n") ;
	for (x = 0 ; x < m_mapDatatypes.Count() ; x++)
	{
		pDT = m_mapDatatypes.GetObj(x) ;

		if (pDT->Basetype() != BASETYPE_APPDEF)
			continue ;

		Z.Printf("\t -- %s\n", pDT->TxtTypename()) ;
	}

	Z.Printf(" -- Selectors\n") ;
	for (x = 0 ; x < m_mapDatatypes.Count() ; x++)
	{
		pDT = m_mapDatatypes.GetObj(x) ;

		if (pDT->Basetype() != BASETYPE_ENUM)
			continue ;

		pSlct = (hdbEnum*) pDT ;
		Z.Printf("\t -- %s\n", pSlct->TxtTypename()) ;
	}

	Z.Printf(" -- Clases\n") ;
	for (x = 0 ; x < m_mapClasses.Count() ; x++)
	{
		pClass = m_mapClasses.GetObj(x) ;
		Z.Printf("\t -- %s\n", pClass->TxtTypename()) ;

		for (y = 0 ; y < pClass->MbrCount() ; y++)
		{
			pMbr = pClass->GetMember(y) ;
			Z.Printf("\t\t -- %s [%s]\n", pMbr->TxtName(), Basetype2Txt(pMbr->Basetype())) ;
		}
	}

	Z.Printf(" -- Class Delta ID assignments\n") ;
	for (x = 0 ; x < m_mapClsCtxDtId.Count() ; x++)
	{
		y = m_mapClsCtxDtId.GetKey(x) ;
		pClass = m_mapClsCtxDtId.GetObj(x) ;

		Z.Printf("\t\t -- [%d] %s\n", y, pClass->TxtTypename()) ;
	}
}

hzEcode	hdbADP::RegisterDataClass	(const hdbClass* pClass)
{
	//	Insert a new data class into the ADP. This may only be done during initialization.
	//
	//	Argument:	pClass	The data class to be inserted

	_hzfunc("hdbADP::RegisterDataClass") ;

	if (!pClass)
		return E_ARGUMENT ;

	if (!pClass->StrTypename())
		return E_NOINIT ;

	threadLog("%s Registering class %s\n", *_fn, pClass->TxtTypename()) ;

	if (pClass->ClassId())
		threadLog("%s Data class %s already registered with id %d", *_fn, pClass->TxtTypename(), pClass->ClassId()) ;
		//hzexit(_fn, HZ_ERROR, "Data class %s already registered with id %d", pClass->TxtTypename(), pClass->ClassId()) ;

	if (m_mapDatatypes.Exists(pClass->StrTypename()))
	{
		threadLog("%s Already have class %s\n", *_fn, *pClass->StrTypename()) ;
		return E_DUPLICATE ;
	}

	//	Allocate the class id
	switch	(pClass->Designation())
	{
	case HDB_CLASS_DESIG_SYS:	if		(pClass->StrTypename() == "subscriber")		pClass->_setId(HZ_ADP_CLS_SUBSCRIBER) ;
								else if (pClass->StrTypename() == "siteindex")		pClass->_setId(HZ_ADP_CLS_SITEINDEX) ;
								else if (pClass->StrTypename() == "FinCurrency")	pClass->_setId(HZ_ADP_CLS_FIN_CRCY) ;
								else if (pClass->StrTypename() == "FinCategory")	pClass->_setId(HZ_ADP_CLS_FIN_CAT) ;
								else if (pClass->StrTypename() == "FinAccount")		pClass->_setId(HZ_ADP_CLS_FIN_ACC) ;
								else if (pClass->StrTypename() == "FinTransaction")	pClass->_setId(HZ_ADP_CLS_FIN_TRNS) ;
								else
									return hzerr(_fn, HZ_ERROR, E_BADVALUE, "Unrecognized System Class (%s)", pClass->TxtTypename()) ;
								break ;

	case HDB_CLASS_DESIG_USR:	pClass->_setId(m_nsqClsUsr++) ;
								break ;

	case HDB_CLASS_DESIG_CFG:	pClass->_setId(m_nsqClsCfg++) ;
								break ;
	}

	m_mapDatatypes.Insert(pClass->StrTypename(), pClass) ;
	m_mapClasses.Insert(pClass->StrTypename(), pClass) ;
	m_mapClsCtxName.Insert(pClass->StrTypename(), pClass->ClassId()) ;	//m_mapClasses.Count()) ;
	m_mapClsCtxDtId.Insert(pClass->ClassId(), pClass) ;
	threadLog("%s Inserted %s\n", *_fn, *pClass->StrTypename()) ;

	return E_OK ;
}

hzEcode	hdbADP::RegisterComposite	(hzString& context, const hdbClass* pClass)
{
	//	Insert a new data class context into the ADP. This may only be done during initialization.
	//
	//	Argument:	pClass	The data class to be inserted

	_hzfunc("hdbADP::RegisterComposite") ;

	if (!context || !pClass)
		return E_ARGUMENT ;

	m_mapClsCtxName.Insert(context, m_nsqClsCtx) ;
	m_mapClsCtxDtId.Insert(m_nsqClsCtx, pClass) ;
	m_nsqClsCtx++ ;

	return E_OK ;
}

hzEcode	hdbADP::RegisterMember	(const hdbMember* pMbr)
{
	_hzfunc("hdbADP::RegisterMember") ;

	const hdbClass*	pClass ;	//	Data class of member
	uint32_t		mbrId ;		//	Member ID to be

	pClass = pMbr->Class() ;
	if (!pClass)
		hzexit(_fn, E_NOINIT, "Member not initialized to its host data class") ;

	//	Assign member ID
	switch	(pClass->Designation())
	{
	case HDB_CLASS_DESIG_SYS:	mbrId = m_nsqMbrSys++ ;
								break ;

	case HDB_CLASS_DESIG_USR:	mbrId = m_nsqMbrUsr++ ;
								break ;

	case HDB_CLASS_DESIG_CFG:	mbrId = m_nsqMbrCfg++ ;
								break ;
	}
	m_mapMembers.Insert(mbrId, pMbr) ;
	pMbr->_setId(mbrId) ;
}

hzEcode	hdbADP::RegisterDataEnum	(const hdbEnum* pEnum)
{
	//	Insert a new data enum into the ADP. This may only be done during initialization.
	//

	_hzfunc("hdbADP::RegisterDataEnum") ;

	if (!pEnum)
		return E_ARGUMENT ;

	if (!pEnum->StrTypename())
		return E_NOINIT ;

	m_mapDatatypes.Insert(pEnum->StrTypename(), pEnum) ;
	m_mapEnums.Insert(pEnum->StrTypename(), pEnum) ;

	return E_OK ;
}

hzEcode	hdbADP::RegisterRegexType	(const hdbRgxtype* pRgx)
{
	_hzfunc("hdbADP::RegisterRegexType") ;

	if (!pRgx)
		return E_ARGUMENT ;
	if (!pRgx->StrTypename())
		return E_NOINIT ;

	return m_mapDatatypes.Insert(pRgx->StrTypename(), pRgx) ;
}

hzEcode	hdbADP::RegisterObjRepos	(hdbObjRepos* pRepos)
{
	_hzfunc("hdbADP::RegisterObjRepos") ;

	if (!pRepos)
		return E_ARGUMENT ;
	if (!pRepos->Name())
		return E_NOINIT ;

	return m_mapRepositories.Insert(pRepos->Name(), pRepos) ;
}

#if 0
hzEcode	hdbADP::RegisterDataCron	(hdbBinCron* pRepos)
{
	if (!pRepos)
		return E_ARGUMENT ;
	if (!pRepos->Name())
		return E_NOINIT ;

	return m_mapBinDataCrons.Insert(pRepos->Name(), pRepos) ;
}

hzEcode	hdbADP::RegisterDataStore	(hdbBinStore* pRepos)
{
	if (!pRepos)
		return E_ARGUMENT ;
	if (!pRepos->m_Name)
		return E_NOINIT ;

	return m_mapBinDataStores.Insert(pRepos->m_Name, pRepos) ;
}
#endif

hzEcode	hdbADP::RegisterBinRepos	(hdbBinRepos* pRepos)
{
	_hzfunc("hdbADP::RegisterBinRepos") ;

	if (!pRepos)
		return E_ARGUMENT ;
	if (!pRepos->Name())
		return E_NOINIT ;

	return m_mapBinRepos.Insert(pRepos->Name(), pRepos) ;
}

bool	hdbADP::IsSubClass	(const hdbClass* pMain, const hdbClass* pSub)
{
	//	Determine if the second data class is a sub-class of the first
	//
	//	Arguments:	1)	pMain	The main class
	//				2)	pSub	The test class
	//
	//	Returns:	True	If the test class is a sub-class of the main class
	//				False	Otherwise

	_hzfunc("hdbADP::IsSubClass") ;

	uint32_t	val_Lo ;		//	First item
	uint32_t	val_Hi ;		//	Last item

	val_Lo = m_mapSubs.First(pMain->StrTypename()) ;
	if (val_Lo >= 0)
	{
		val_Hi = m_mapSubs.Last(pMain->StrTypename()) ;

		for (; val_Lo <= val_Hi ; val_Lo++)
		{
			if (pSub == m_mapSubs.GetObj(val_Lo))
				return true ;
		}
	}

	return false ;
}
