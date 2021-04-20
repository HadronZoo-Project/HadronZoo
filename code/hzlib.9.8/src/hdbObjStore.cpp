//
//	File:	hdbObjStore.cpp
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
//	Implimentation of the hdbObjStore (class object store) class as part of the HadronZoo Database Suite
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

extern	hzDeltaClient*	_hzGlobal_DeltaClient ;					//	Total of current delta clients

/*
**	Prototypes
*/

const char*	_hds_showinitstate	(hdbIniStat nState) ;
void		_hdb_ck_initstate	(const hzFuncname& _fn, const hzString& objName, hdbIniStat eActual, hdbIniStat eExpect) ;

/*
**	Functions
*/

#if 0
void	hdbObjStore::_initerr	(const hzFuncname& _fn, uint32_t nExpect)
{
	//	Report cases of wrong initialization

	if (nExpect == CLASS_REPOS_INIT_NONE)
	{
		//	The only function that should expect this is InitStart()
		if (m_nInitState)
			hzexit(_fn, E_SEQUENCE, "Object Store %s is already at init state %d. Cannot restart initialization", *m_Name, m_nInitState) ;
		return ;
	}

	if (nExpect == CLASS_REPOS_INIT_START)
	{
		//	The only functions that should expect this are the InitMember() and InitStore(). With this expectation, the flag CLASS_REPOS_INIT_START must be set,
		//	the flag CLASS_REPOS_INIT_INPROG can be set, all others must be clear.

		if (!m_nInitState)
			hzexit(_fn, E_SEQUENCE, "Cannot modify class before calling InitStart()") ;

		if (m_nInitState >= CLASS_REPOS_INIT_DONE)
			hzexit(_fn, E_SEQUENCE, "Cannot modify class after InitDone()") ;
		return ;
	}

	if (nExpect == CLASS_REPOS_INIT_INPROG)
	{
		//	The only function that should expect this is InitDone(). The flag CLASS_REPOS_INIT_START can be set, the flag CLASS_REPOS_INIT_INPROG can be set,
		//	all others must be clear

		if (!m_nInitState)
			hzexit(_fn, E_SEQUENCE, "Cannot call InitDone() before calling InitStart()") ;

		//	if (m_nInitState < CLASS_REPOS_INIT_INPROG)
		//		hzexit(_fn, E_SEQUENCE, "Cannot call InitDone() on unmodified repository %s", *m_Name) ;

		if (m_nInitState >= CLASS_REPOS_INIT_DONE)
			hzexit(_fn, E_SEQUENCE, "Class %s: Cannot call InitDone more than once!", *m_Name) ;
		return ;
	}

	if (nExpect == CLASS_REPOS_INIT_DONE)
	{
		//	The only function that should expect this is Open()
		if (m_nInitState < CLASS_REPOS_INIT_DONE)
		{
			if (m_Name)
				hzexit(_fn, E_SEQUENCE, "Object Store %s not initialized (init state %d)", *m_Name, m_nInitState) ;
			else
				hzexit(_fn, E_SEQUENCE, "Object Store (unnamed) not initialized (init state %d)", m_nInitState) ;
		}

		if (m_nInitState & HDB_REPOS_OPEN)
			hzexit(_fn, E_SEQUENCE, "Object Store %s already open (init state %d)", *m_Name, m_nInitState) ;
		return ;
	}

	if (nExpect == HDB_REPOS_OPEN)
	{
		//	Expect open state
		if (!(m_nInitState & HDB_REPOS_OPEN))
		{
			if (m_Name)
				hzexit(_fn, E_SEQUENCE, "Object Store %s is not open (init state %d). Cannot access", *m_Name, m_nInitState) ;
			else
				hzexit(_fn, E_SEQUENCE, "Object Store (unnamed) is not open (init state %d). Cannot access", m_nInitState) ;
		}

		return ;
	}

	hzexit(_fn, E_CORRUPT, "Illegal Init State Expectation") ;
}
#endif

hzEcode	hdbObjStore::InitStart	(const hdbClass* pObjClass, const hzString& reposName, const hzString& workdir)
{
	//	Begin initialization of the object store with the supplied class definition. This checks the initialization states of both this cache and the
	//	class definition. The class definition must be initialized, the store must not be.
	//
	//	The initialization process will set up the format of the (fixed length) core file based on the class definition. This process assigns positions for data class
	//	members that can be accomodated as fixed length objects (numeric data). These members will then always be located at these positions within rows held in the
	//	fixed length core.
	//
	//	The process then goes on to assign core positions for the offsets and sizes within the variable length part, for members that cannot be accomodated directly
	//	in the core.
	//
	//	Arguments:	1)	objClass	The data class
	//				2)	reposName		The repository name
	//				3)	workdir		The operaional directory
	//
	//	Returns:	E_INITDUP	If there has been a previous attempt to initialize the store
	//				E_ARGUMENT	If no data class OR no repository name OR wording directory is supplied
	//				E_NOINIT	If the supplied class is not initialized (has no members)

	_hzfunc("hdbObjStore::InitStart") ;

	//const hdbMember*	pMem ;	//	Member pointer (from initializing hdbClass)

	uint32_t	nIndex ;		//	Member iterator
	hzEcode		rc ;			//	Return code

	//	Check initialization state
	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_CLASS_INIT_NONE) ;

	if (!pObjClass)		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No data class supplied") ;
	if (!reposName)		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No repository name supplied") ;
	if (!workdir)		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No repository working directory supplied") ;

	if (!pObjClass->MbrCount())
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "Supplied class has no members") ;

	if (m_pADP->GetObjRepos(reposName))
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Repository %s already exists", *reposName) ;

	m_pClass = pObjClass ;
	m_Name = reposName ;

	rc = AssertDir(*workdir, 0777) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Cannot assert working directory %s\n", *workdir) ;
	m_Workdir = workdir ;

	//	Set up m_Binaries to be all nulls. Later real hdbBinRepos instances will be created as required.
	m_Binaries = new hdbBinRepos*[m_pClass->MbrCount()] ;
	if (!m_Binaries)
		hzexit(_fn, m_Name, E_MEMORY, "Cannot allocate set of index pointers") ;

	for (nIndex = 0 ; nIndex < m_pClass->MbrCount() ; nIndex++)
		m_Binaries[nIndex] = 0 ;


	//	Set up index array to be all nulls. Later the AddIndex function will insert real indexes where requested
	m_Indexes = new hdbIndex*[m_pClass->MbrCount()] ;
	if (!m_Indexes)
		hzexit(_fn, m_Name, E_MEMORY, "Cannot allocate set of index pointers") ;

	for (nIndex = 0 ; nIndex < m_pClass->MbrCount() ; nIndex++)
	{
		m_Indexes[nIndex] = 0 ;

		//pMem = m_pClass->GetMember(nIndex) ;
	}

	m_eReposInit = HDB_REPOS_INIT_PROG ;

	rc = m_pADP->RegisterObjRepos(this) ;
	return rc ;
}

hzEcode	hdbObjStore::InitMbrIndex	(const hzString& memberName, bool bUnique)
{
	//	Add an index based on the supplied member name. Find the member in the class and from the datatype, this will determine which sort of index
	//	should be set up.
	//
	//	Arguments:	1)	mbrName	Name of member index shall apply to
	//				2)	bUnique	Flag if value uniqness applies

	_hzfunc("hdbObjStore::AddIndex") ;

	//_member*	p_mem ;			//	Named class member
	const hdbMember*	pMem ;	//	Member

	hdbIndex*	pIdx = 0 ;		//	The index to be added
	uint32_t	mbrNo ;			//	Member number

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_INIT_PROG) ;

	//	mbrNo = m_Class.MemberPosn(memberName) ;
	//	if (mbrNo < 0)
	//		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "No such member as %s\n", *memberName) ;
	//	p_mem = m_members[mbrNo] ;
	pMem = m_pClass->GetMember(memberName) ;

	//	Member's datatype will determine the type of index
	switch	(pMem->Basetype())
	{
	case BASETYPE_STRING:	if (bUnique)
								pIdx = new hdbIndexUkey() ;
							else
								pIdx = new hdbIndexText() ;
							break ;

	case BASETYPE_EMADDR:
	case BASETYPE_URL:
	case BASETYPE_IPADDR:
	case BASETYPE_TIME:
	case BASETYPE_SDATE:
	case BASETYPE_XDATE:
	case BASETYPE_DOUBLE:
	case BASETYPE_INT64:
	case BASETYPE_INT32:
	case BASETYPE_INT16:
	case BASETYPE_BYTE:
	case BASETYPE_UINT64:
	case BASETYPE_UINT32:
	case BASETYPE_UINT16:
	case BASETYPE_UBYTE:	pIdx = new hdbIndexUkey() ;
							break ;

	case BASETYPE_ENUM:		pIdx = new hdbIndexEnum() ;
							break ;

	case BASETYPE_TEXT:
	case BASETYPE_TXTDOC:	pIdx = new hdbIndexText() ;
							break ;
	default:
		pIdx = 0 ;
		break ;
	}

	m_Indexes[mbrNo] = pIdx ;
	m_eReposInit = HDB_REPOS_INIT_PROG ;

	return E_OK ;
}

hzEcode	hdbObjStore::InitMbrStore	(const hzString& mbrName, const hzString& binaryName, bool bStore)
{
	//	Direct a member of the data classs to be stored in either a hdbBinCron or hdbBinStore. The data member must be of base data type BINARY or TXTDOC. Note
	//	that if an externally declared hdbBinCron or hdbBinStore is to be used, it must be named in argument 2 so it can be selected from the ADP maps
	//	mapBinDataCrons or mapBinDataStores. If this is left blank, a binary store (either a hdbBinCron or a hdbBinStore) will be assigned to the object stored.
	//
	//	Arguments:	1)	mbrName		The member name. The named member must exist and be of base data type TEXT, TXTDOC or BINARY.
	//				2)	binaryName	The name of the externally declared hdbBinCron or hdbBinStore, if applicable.
	//				3)	bStore		Set if a hdbBinStore is to be used. The default is to use a hdbBinCron for TEXT and TXTDOC members.
	//
	//	Returns:	E_NOTFOUND	If the named member does not exist.
	//				E_TYPE		If the named member is not of base data type TXTDOC or BINARY.
	//				E_OK		If the operation was successful.

	_hzfunc("hdbObjStore::InitStore") ;

	const hdbMember*	pMem ;	//	Named class member

	uint32_t	mbrNo ;			//	Member number
	hzEcode		rc ;			//	Returm code

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_INIT_PROG) ;

	if (!m_pClass)	return hzerr(_fn, HZ_ERROR, E_NOINIT, "No cache class set up") ;
	if (!mbrName)	return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No member name supplied") ;

	pMem = m_pClass->GetMember(mbrName) ;
	if (!pMem)
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "No such member as %s\n", *mbrName) ;
	mbrNo = pMem->Posn() ;

	if (mbrNo < 0 || mbrNo >= m_pClass->MbrCount())
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "Member %s has no defined position within the class", *mbrName) ;

	if (pMem->Basetype() != BASETYPE_BINARY && pMem->Basetype() != BASETYPE_TEXT && pMem->Basetype() != BASETYPE_TXTDOC)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Member %s has non binary base type", *mbrName) ;

	if (binaryName)
	{
		//	A pre-existing hdbBinCron or hdbBinStore has been named so the member will use this.
		m_Binaries[mbrNo] = (hdbBinRepos*) m_pADP->GetBinRepos(binaryName) ;
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

hzEcode	hdbObjStore::InitDone	(void)
{
	//	Conclude the initialization process and set init status to allow no further indexes to be added.
	//	deal with files if the working directory has been supplied. If a file of the cache's name exists in the working directory, this is assumed
	//	to be the data file and will be read in to populate the cache. The file must begin with a header which must match the class definition so this
	//	is checked before loading the remaining data. If a file of the cache's name does not exist in the working directory, it will be created and a
	//	header will be written.
	//
	//	If a backup directory has been specified and a file of the cache's name exists in this directory, the header will be checked and assuming this
	//	is OK, the length of the file will aslo be checked (should match with that in the work directory)

	_hzfunc("hdbObjStore::InitDone") ;

	const hdbMember*	pMbr ;		//	Data class member

	ifstream		is ;		//	For reading in description file
	ofstream		os ;		//	For writing out description file
	FSTAT			fs ;		//	File status
	hzChain			report ;	//	Needed for description check
	hzChain			cdescNew ;	//	For writing class description header from class
	hzChain			cdescPre ;	//	For writing class description header from data file
	hzAtom			atom ;		//	For setting member values
	hdbIndex*		pIdx ;		//	The index to be added
	hdbIndexUkey*	pIdxU ;		//	The index to be added
	hzString		S ;			//	Temp string holding memeber data
	uint32_t		nLine ;		//	Line number for reporting file errors
	uint32_t		mbrNo ;		//	Member number
	char			buf [204] ;	//	For getline
	hzEcode			rc ;		//	Return code

	//	Check initialization and if there are some members added
	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_INIT_PROG) ;

	if (!m_pClass->MbrCount())	return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Class %s: Appears to have no members!", *m_Name) ;

	m_Workpath = m_Workdir + "/" + m_Name + ".store" ;
	m_Descpath = m_Workdir + "/" + m_Name + ".desc" ;

	m_pClass->DescClass(cdescNew, 0) ;
	if (!cdescNew.Size())
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Class %s: Failed to write description", *m_Name) ;

	if (lstat(*m_Descpath, &fs) < 0)
	{
		//	Working data file does not exist or is empty
		os.open(*m_Descpath) ;
		if (os.fail())
			return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Class %s Cannot open description file %s in write mode", *m_Name, *m_Descpath) ;

		os << cdescNew ;
		if (os.fail())
		{
			os.close() ;
			return hzerr(_fn, HZ_ERROR, rc, "Class %s Cannot write description file %s", *m_Name, *m_Descpath) ;
		}
		os.flush() ;
		os.close() ;
		os.clear() ;
	}

	//	Working data file does exist and has content so read it in. Start with header
	is.open(*m_Descpath) ;
	if (is.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Class %s data file %s exists but cannot be read in", *m_Name, *m_Descpath) ;

	for (nLine = 1 ;; nLine++)
	{
		is.getline(buf, 200) ;
		if (!buf[0])
			break ;

		cdescPre << buf ;
		cdescPre.AddByte(CHAR_NL) ;

		if (!strcmp(buf, "</class>"))
			break ;
	}
	is.close() ;

	//	Compare class description header from file to that of the class
	rc = m_pClass->DescCheck(report, cdescPre) ;
	if (rc != E_OK)
	{
		threadLog(report) ;
		return hzerr(_fn, HZ_ERROR, rc) ;
	}

	if (cdescNew.Size() != cdescPre.Size())
	{
		is.close() ;
		threadLog("%s. Class %s Format error in data file [%s]", *_fn, *m_Name, *m_Workpath) ;
		threadLog(cdescPre) ;
		threadLog(cdescNew) ;
		return E_FORMAT ;
	}

	//	Initialize all allocated indexes
	for (mbrNo = 0 ; mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		//	First check if a member gives rise to an hdbBinCron instance. This is needed if there are one or more members with a binary datatype requiring a hdbBinCron
		//	but also (since hdbObjStore is disk bound), one or more members that are multiple or string like. (note it is almost certainly needed!)

		pMbr = m_pClass->GetMember(mbrNo) ;

		if (pMbr->Basetype() == BASETYPE_BINARY || pMbr->Basetype() == BASETYPE_TXTDOC)
		{
			if (!m_Binaries[mbrNo])
			{
				if (!m_pDfltBinRepos)
				{
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
			S = pMbr->StrName() ;

			rc = pIdxU->Init(this, S, pMbr->Basetype()) ;
			if (rc != E_OK)
				return hzerr(_fn, HZ_ERROR, rc, "Failed to initialize unique-key index %s (%s)\n", *m_Name, *m_Workdir) ;
		}
	}

	//	Now create the core data binary store
	m_pCore = new hdbBinRepos(*m_pADP) ;

	if (rc == E_OK)
		m_eReposInit = HDB_REPOS_INIT_DONE ;
	return rc ;
}

hzEcode	hdbObjStore::Open	(void)
{
	//	Open the hdbObjStore. This is a matter of opening the core data hdbBinStore (for the fixed length part of objects) and any other hdbBinCron/hdbBinStores
	//	used to hold data members that are of base data types TXTDOC or BINARY.
	//
	//	The core data hdbBinStore will always be 'native' to the specific hdbObjStore instance and so this will be opened with the hdbBinStore::Init() function.
	//	Any TXTDOC/BINARY data members may use either a hdbBinCron or a hdbBinStore and this may either be native to the hdbObjStore or be a pre-declared global
	//	entity. If native it will be opened in this function, if global this function will error if this is not already open. The policy is that all global data
	//	storing resources are opened at startup.
	//
	//	All native hdbBinCron/hdbBinStores will store data in files that appear in the working directory specific to the hdbObjStore. The 'base' of the filename
	//	will be the name of the hdbObjStore.
	//
	//	Arguments:	None

	_hzfunc("hdbObjStore::Open") ;

	hzEcode	rc = E_OK ;		//	Return code

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_INIT_DONE) ;

	rc = m_pCore->Init(m_Name, m_Workdir) ;
	if (rc != E_OK)
		hzexit(_fn, m_Name, rc, "Could not INIT (OPEN) core data binary") ;

	if (rc == E_OK)
		m_eReposInit = HDB_REPOS_OPEN ;

	return rc ;
}

hzEcode	hdbObjStore::Close	(void)
{
	//	Close the hdbObjStore. This is a matter of closing the core data hdbBinStore (for the fixed length part of objects), together with any native hdbBinCron
	//	or hdbBinStores used to hold TXTDOC/BINARY data members. If any data members are using a pre-declared global hdbBinCron or hdbBinStore, this will remain
	//	open.
	//
	//	Arguments:	None

	_hzfunc("hdbObjStore::Close") ;

	hzEcode	rc = E_OK ;		//	Return code

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	//	Close core data binary and any binaries kept on behalf of member
	rc = m_pCore->Close() ;
	m_eReposInit = HDB_REPOS_INIT_DONE ;

	return rc ;
}

hzEcode	hdbObjStore::Insert		(uint32_t& objId, const hdbObject& obj)
{
	//	Insert a new object of the host class, into the object store.
	//
	//	The INSERT operation always creates a new object so a new object address is assigned and the delta output will contain an entry for each member that has
	//	a value. The object id will be the address of the object in the store. If the object is less than half a block in size, the entire object will reside in
	//	a tail block and the address top bit will be set to indicate the. If the object exceeds this size, the address will not have the top bit set meaning the
	//	first block is a whole block. The object ids are always written in hex and as the deltas contain the object id, this is reflected in the delta file.
	//
	//	The INSERT operation is subject to all the normal conditions concerning object validity. If a member defined as unique, has a value that already exists,
	//	or if any member's min and max population rules or exceeded, the insert will fail.
	//
	//	The INSERT operation is a matter only of assigning blocks.

	_hzfunc("hdbObjStore::Insert") ;

	const hdbMember*	pMem ;	//	Class member

	hzChain			Z ;				//	Chain for building core data
	hzChain			X ;				//	Chain for any binary member values
	hdbObjStore*	pStore ;		//	Secondary hdbObjStore if applicable
	//hzValset*		pElem ;			//	Object node (atom of secondary class object)
	hdbObject*		pObj2 ;			//	Secondary object
	hzAtom*			pAtom ;			//	Atom from member of populating object
	hdbIndex*		pIdx ;			//	Index pointer
	hdbIndexUkey*	pIdxU ;			//	Index pointer
	hdbIndexEnum*	pIdxE ;			//	Index pointer
	hzString		S ;				//	Temp string
	uint32_t		mbrNo ;			//	Member number
	uint32_t		secId = 0 ;		//	Secondary object id
	hzEcode			rc = E_OK ;		//	Return code
	//hzRecep32		r32 ;			//	Time/date buffer

	//	Check Init state
	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	if (!obj.IsInit())
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "Supplied object is not initialized") ;

	if (obj.Classname() != m_pClass->StrTypename())
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Supplied object is of class %s while this cache is of class %s", obj.Classname(), m_pClass->TxtTypename()) ;

	threadLog("%s. Called\n", *_fn) ;
	objId = 0 ;

	//	Go thru all members and check if any of them specify that all values must be unique (any that are will have an index based on a one to one
	//	mapping of the member type to object id). We look up if the value of the member in the new object already exists and if it does the INSERT
	//	operation is aborted. Only an UPDATE operation would work in this event.

	for (mbrNo = 0 ; mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		pIdx = m_Indexes[mbrNo] ;
		if (!pIdx)
			continue ;

		//	pElem = obj.Node(mbrNo) ;
		//	if (!pElem->IsAtom())
		//		continue ;
		//	pAtom = (hzAtom*) pElem ;

		if (pIdx->Whatami() == HZINDEX_UKEY)
		{
			pIdxU = (hdbIndexUkey*) pIdx ;

			rc = pIdxU->Select(objId, *pAtom) ;
			if (rc != E_OK)
				return rc ;

			if (objId)
				return E_DUPLICATE ;
		}
	}

	//	At this point it is known that the new object does not conflict with an existing one and so insertation can proceed. The objId is assigned as
	//	the number of existing objects + 1
	objId = m_nObjs + 1 ;
	threadLog("%s called on store %s (assigned objId is %d)\n", *_fn, obj.Classname(), objId) ;

	//	Get a new fixed length space
	//pBase = m_pMain->AssignSlot(nSlot, objId) ;

	/*
	**	Now go thru object's class members filling in the allocated fixed length space
	*/

	for (mbrNo = 0 ; rc == E_OK && mbrNo < m_pClass->MbrCount() ; mbrNo++)
	{
		pMem = m_pClass->GetMember(mbrNo) ;

		//	pElem = obj.Node(mbrNo) ;
		//	if (pAtom->IsNull())
		//		continue ;

		threadLog("doing member %d %s\n", mbrNo, pMem->TxtName()) ;

		//threadLog("doing member %d with base at %p and oset at %p (%u)\n", mbrNo, pBase, pVar, (pVar - pBase) & 0xffffffff) ;

		//if (!pElem->IsAtom())
		if (pMem->Basetype() == BASETYPE_CLASS)
		{
			//	Insert the secondary class object but then the address of the secondary class object has to be placed in the fixed length area part
			//	for the secondary class member

			threadLog("NON ATOM\n") ;

			//pObj2 = (hdbObject*) pElem ;
			pObj2 = (hdbObject*) pAtom->Binary() ;
			pStore = (hdbObjStore*) m_pADP->GetObjRepos(pObj2->Classname()) ;	//	Not right. Can't assume this will be the repository used

			if (!pStore)
				return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Cannot locate cache for composite member %s", pObj2->Classname()) ;
			threadLog("%s. Inserting secondary into cache %s\n", *_fn, pObj2->Classname()) ;

			secId = 0 ;
			rc = pStore->Insert(secId, *pObj2) ;
			threadLog("%s. Returned from secondary insert with objId %d and err %s\n", *_fn, secId, Err2Txt(rc)) ;

			//	Now write out the secondary address in delta notation
			Z.Printf("@%d.%d=%d\n", objId, mbrNo, secId) ;

			continue ;
		}

		//pAtom = (hzAtom*) pElem ;
		threadLog("%s. DOING member %d %s (val %s)\n", *_fn, mbrNo, pMem->TxtName(), *pAtom->Str()) ;

		if (pMem->Basetype() == BASETYPE_BINARY)
		{
			X = pAtom->Chain() ;

			if (X.Size())
			{
				threadLog("%s. Written Binary of %d bytes. Err=%s\n", *_fn, X.Size(), Err2Txt(rc)) ;
				//rc = m_Binaries[mbrNo]->Insert(secId, X, pAtom->Mime()) ;
				rc = m_Binaries[mbrNo]->Insert(secId, X) ;
				Z.Printf("@%d.%d=%u\n", objId, mbrNo, secId) ;
			}
		}

		else if (pMem->Basetype() == BASETYPE_TXTDOC)
		{
			X = pAtom->Chain() ;

			if (X.Size())
			{
				threadLog("%s. Written Document of %d bytes. Err=%s\n", *_fn, X.Size(), Err2Txt(rc)) ;
				//rc = m_Binaries[mbrNo]->Insert(secId, X, pAtom->Mime()) ;
				rc = m_Binaries[mbrNo]->Insert(secId, X) ;
				Z.Printf("@%d.%d=%u\n", objId, mbrNo, secId) ;
			}
		}

		else if (pMem->Basetype() == BASETYPE_STRING || pMem->Basetype() == BASETYPE_EMADDR || pMem->Basetype() == BASETYPE_URL)
		{
			//	Handle string type datums. Only update and copy to the backup file if the string has changed

			S = pAtom->Str() ;

			//	Now write out the string in delta notation
			threadLog("%s. Writing string %s\n", *_fn, *S) ;
			Z.Printf("@%d.%d=%s\n", objId, mbrNo, *S) ;
		}
		else
		{
			//	Handle numeric type datums.

			S = pAtom->Str() ;
			threadLog("%s. Writing numval %s val=[%s]\n", *_fn, pMem->TxtName(), *S) ;

			switch	(pMem->Basetype())
			{
			case BASETYPE_IPADDR:	Z.Printf("@%d.%d=%s\n", objId, mbrNo, *pAtom->Ipaddr().Str()) ;	break ;
			case BASETYPE_TIME:		Z.Printf("@%d.%d=%s\n", objId, mbrNo, *pAtom->Time().Str()) ;	break ;
			case BASETYPE_SDATE:	Z.Printf("@%d.%d=%s\n", objId, mbrNo, *pAtom->SDate().Str()) ;	break ;
			case BASETYPE_XDATE:	Z.Printf("@%d.%d=%s\n", objId, mbrNo, *pAtom->XDate().Str()) ;	break ;
			case BASETYPE_DOUBLE:	Z.Printf("@%d.%d=%f\n", objId, mbrNo, pAtom->Double()) ;		break ;
			case BASETYPE_INT64:	Z.Printf("@%d.%d=%l\n", objId, mbrNo, pAtom->Int64()) ;			break ;
			case BASETYPE_INT32:	Z.Printf("@%d.%d=%d\n", objId, mbrNo, pAtom->Int32()) ;			break ;
			case BASETYPE_INT16:	Z.Printf("@%d.%d=%d\n", objId, mbrNo, pAtom->Int16()) ;			break ;
			case BASETYPE_BYTE:		Z.Printf("@%d.%d=%d\n", objId, mbrNo, pAtom->Byte()) ;			break ;
			case BASETYPE_UINT64:	Z.Printf("@%d.%d=%u\n", objId, mbrNo, pAtom->Unt64()) ;			break ;
			case BASETYPE_UINT32:	Z.Printf("@%d.%d=%u\n", objId, mbrNo, pAtom->Unt32()) ;			break ;
			case BASETYPE_UINT16:	Z.Printf("@%d.%d=%u\n", objId, mbrNo, pAtom->Unt16()) ;			break ;
			case BASETYPE_UBYTE:	Z.Printf("@%d.%d=%u\n", objId, mbrNo, pAtom->UByte()) ;			break ;
			case BASETYPE_ENUM:		Z.Printf("@%d.%d=%u\n", objId, mbrNo, pAtom->UByte()) ;			break ;
			case BASETYPE_BOOL:		Z.Printf("@%d.%d=%s\n", objId, mbrNo, pAtom->Bool() ? "true":"false") ;
									break ;
			}
		}

		pIdx = m_Indexes[mbrNo] ;

		if (pIdx)
		{
			if (pIdx->Whatami() == HZINDEX_UKEY)
			{
				pIdxU = (hdbIndexUkey*) pIdx ;
				rc = pIdxU->Insert(*pAtom, objId) ;
			}

			if (pIdx->Whatami() == HZINDEX_ENUM)
			{
				pIdxE = (hdbIndexEnum*) pIdx ;
				rc = pIdxE->Insert(objId, *pAtom) ;
			}
		}
	}

	threadLog("%s. Done members\n", *_fn) ;

	//	Now write out new object

	if (rc == E_OK && Z.Size())
	{
		if (_hzGlobal_DeltaClient)
			_hzGlobal_DeltaClient->DeltaWrite(Z) ;
	}

	//rc = m_pCore->Insert(objId, Z, mt) ;
	rc = m_pCore->Insert(objId, Z) ;

	return rc ;
}

hzEcode	hdbObjStore::Update		(hdbObject& obj, uint32_t objId)
{
	//	Overwrite the object with the supplied object id. This operation will overwrite the data in the core file and create a new entry in the hdbBinCron if this is
	//	active.

	_hzfunc("hdbObjStore::Update") ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;
}

hzEcode	hdbObjStore::Fetch		(hdbObject& obj, uint32_t objId)
{
	//	Loads an object from the cache into the supplied object container. This first reads the fixed length part of the object from the core file and then, assuming
	//	there is a variable length part, reads this from the hdbBinCron and/or hdbBinStore. 
	//
	//	Arguments:	1)	obj		The object
	//				2)	objId	The object id to fetch
	//
	//	Returns:	E_RANGE		The requested row is out of range
	//				E_OK		Operation success

	_hzfunc("hdbObjStore::Fetch") ;

	const hdbMember*	pMem ;	//	Member pointer

	hzChain		Z ;				//	Used to retrieve from non-binary from core
	hzChain		X ;				//	Used to retrieve individual values from core
	chIter		zi ;			//	For iterating the encoded core data
	hzString	S ;				//	Used to get value from X
	uint32_t	mbrNo ;			//	Member and index iterator
	uint32_t	addr ;			//	Address of either sub-class instance or a binary
	//	hzMimetype	mt ;			//	Only needed for the Fetch from core
	hzEcode		rc = E_OK ;		//	Return code

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	//	Clear all object members
	obj.Clear() ;
	if (objId < 0 || objId > m_nObjs)
		return E_RANGE ;

	//	Now read in the rest of the data (in delta notation)
	//m_pCore->Fetch(Z, mt, objId) ;
	m_pCore->Fetch(Z, objId) ;

	for (zi = Z ; !zi.eof() ; zi++)
	{
		if (*zi != CHAR_AT)
			continue ;

		for (zi++, mbrNo = 0 ; IsDigit(*zi) ; zi++)
			{ mbrNo *= 10 ; mbrNo += (*zi - '0') ; }

		pMem = m_pClass->GetMember(mbrNo) ;
		if (!pMem)
		{
			rc = hzerr(_fn, HZ_ERROR, E_FORMAT, "ObjID %d: Member %d does not exist in class %s\n", objId, mbrNo, m_pClass->TxtTypename()) ;
			break ;
		}

		if (*zi == CHAR_EQUAL)
		{
		}


		//	Now either the value is an address or as-is
		if (pMem->Basetype() == BASETYPE_CLASS || pMem->Basetype() == BASETYPE_TXTDOC || pMem->Basetype() == BASETYPE_BINARY)
		{
			for (zi++, mbrNo = 0 ; IsDigit(*zi) ; zi++)
				{ addr *= 10 ; addr += (*zi - '0') ; }

			//	Obtain the binary in a separate chain (X)
			//	obj.SetValue(pMem->Basetype(), X) ;
		}
		else
		{
			for (zi++ ; *zi != CHAR_NL ; zi++)
				X.AddByte(*zi) ;

			if (!X.Size())
				continue ;
			S = X ;
			X.Clear() ;

			obj.SetValue(mbrNo, S) ;
		}
	}

	return E_OK ;
}

hzEcode	hdbObjStore::Delete		(uint32_t objId)
{
	//	Standard delete operation, fails only if the stated object id does not exist

	_hzfunc("hdbObjStore::Delete") ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	return E_OK ;
}

hzEcode	hdbObjStore::Clear		(void)
{
	//	Empty the Cache - except that we never do this within an application

	_hzfunc("hdbObjStore::Clear") ;

	_hdb_ck_initstate(_fn, Name(), m_eReposInit, HDB_REPOS_OPEN) ;

	return E_OK ;
}
