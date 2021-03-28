//
//	File:	hzDatabase.h
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

#ifndef hzDatabase_h
#define hzDatabase_h

/*
**	Other includes
*/

#include "hzBasedefs.h"
#include "hzDate.h"
#include "hzChain.h"
#include "hzCtmpls.h"
#include "hzIpaddr.h"
#include "hzEmaddr.h"
#include "hzUrl.h"
#include "hzMimetype.h"
#include "hzDocument.h"
#include "hzProcess.h"

/*
**	Definitions
*/

#define HZ_MAXOBJECT	16000000	//	Largets possible binary object

/*
**	SECTION 1:	Classes for data structure/definition.
*/

enum	hdbBasetype
{
	//	Category:	Data Definition
	//
	//	The hdbBasetype of a HadronZoo data type (hdbDatatype) is described in Synopsis HDB-1 SECTION 1, as having the following purpose:-
	//
	//	In the case of hdbCpptype, the hzBasetype is used to determine the size of datum. In the case of hdbHzotype it determines which process should apply. In
	//	the case of hdbRgxtype, hdbEnum and hdbClass it exists for consistency and serves only to identify that the type is a hdbRgxtype, hdbEnum or a hdbClass.
	//
	//	NOTE that in the case of hdbClass, we have BASETYPE_CLASS (the constructor default) and BASETYPE_REPOS. This relates to how instances of a sub-class are
	//	manifest within an instance of the host class, i.e. by reference or by value. If in the configs, a member of the host class names a (predefined) class,
	//	then the member will have the named class as its data type but will store values (instance of the named class) INTERNALLY by value. Alternatively, if in
	//	the configs, a member of the host class names a (predefined) repository, then the member will have the class of the repository as its data type. And as
	//	a repository has been named as the data sink, member values (instance of the named repository class), will be stored EXTERNALLY by reference, in the
	//	named repository.
 
	BASETYPE_UNDEF	= 0,		//	Null data type, not grouped

	//	Group 1: Fundamental C++ types (fixed size)
	BASETYPE_CPP_UNDEF	= 0x0100,	//	C++ type but not yet defined
	BASETYPE_DOUBLE		= 0x0101,	//	64 bit floating point value
	BASETYPE_INT64		= 0x0102,	//	64-bit Signed integer
	BASETYPE_INT32		= 0x0103,	//	32-bit Signed integer
	BASETYPE_INT16		= 0x0104,	//	16-bit Signed integer
	BASETYPE_BYTE		= 0x0105,	//	8-bit Signed integer
	BASETYPE_UINT64		= 0x0106,	//	64-bit Positive integer
	BASETYPE_UINT32		= 0x0107,	//	32-bit Positive integer
	BASETYPE_UINT16		= 0x0108,	//	16-bit Positive integer
	BASETYPE_UBYTE		= 0x0109,	//	8-bit Positive integer
	BASETYPE_BOOL		= 0x010A,	//	Either true or false, cannot be empty or have mutiple values

	//	Group 2: HadronZoo Defined types (fixed size)
	BASETYPE_HZO_UNDEF	= 0x0200,	//	HadronZoo inbuilt type but not yet defined
	BASETYPE_TBOOL		= 0x0201,	//	Either true, false or don't know (empty). Cannot have mutiple values
	BASETYPE_DOMAIN		= 0x0202,	//	Internet domain
	BASETYPE_EMADDR		= 0x0203,	//	Email Address
	BASETYPE_URL		= 0x0204,	//	Universal Resource Locator
	BASETYPE_IPADDR		= 0x0205,	//	IP Address
	BASETYPE_TIME		= 0x0206,	//	No of seconds since midnight (4 bytes)
	BASETYPE_SDATE		= 0x0207,	//	No of days since Jan 1st year 0000
	BASETYPE_XDATE		= 0x0208,	//	Full date & time
	BASETYPE_STRING		= 0x0209,	//	Any string, treated as a single value
	BASETYPE_TEXT		= 0x020A,	//	Any string, treated as a series of words and indexable. Cannot have multiple values.
	BASETYPE_BINARY		= 0x020B,	//	Binary object, assummed to be un-indexable (e.g. image). No multiple values. Disk only.
	BASETYPE_TXTDOC		= 0x020C,	//	Document from which text can be extracted and so indexed. No multiple values. Disk only.

	//	Group 3: Application defined data enumerations
	BASETYPE_ENUM		= 0x1001,	//	Data enumeration

	//	Group 4: Application defined special text types
	BASETYPE_APPDEF		= 0x2000,	//	String subject to special interpretation by the application (e.g. serial numbers).

	//	Group 5: Application defined data class
	BASETYPE_CLASS		= 0x4000,	//	Sub-class (instances stored as part of host class instance).
} ;

hdbBasetype	Str2Basetype	(const hzString& type) ;
const char*	Basetype2Txt	(hdbBasetype dt) ;

/*
**	Application Delta Profile
*/

//	Reserved system class ids
#define HZ_ADP_CLS_SUBSCRIBER	   1	//	Subscriber class ID
#define HZ_ADP_CLS_SITEINDEX	   2	//	Site index class ID
#define HZ_ADP_CLS_FIN_CRCY		   3	//	Financial Currency class ID
#define HZ_ADP_CLS_FIN_CAT		   4	//	Financial Category class ID
#define HZ_ADP_CLS_FIN_ACC		   5	//	Financial Account class ID
#define HZ_ADP_CLS_FIN_TRNS		   6	//	Financial Transaction class ID

//	Other class and member ids
#define HZ_ADP_CLS_RNG_USER		  21	//	User data classes have IDs from 21 to 50
#define HZ_ADP_CLS_RNG_APPL		  51	//	Application data classes have IDs from at 51 to 1000
#define HZ_ADP_CLS_RNG_CONTEXT	1001	//	Sub-class combinations have IDs starting at 1001
#define	HZ_ADP_MBR_RNG_SYSTEM	   1	//	System data classes have members from 1 to 500
#define HZ_ADP_MBR_RNG_USER		 501	//	User data classes have members from 501 to 1000
#define HZ_ADP_MBR_RNG_APPL		1001	//	Application data classes have members startig at 1001

class	hdbDatatype ;
class	hdbClass ;
class	hdbMember ;
class	hdbEnum ;
class	hdbObjRepos ;
class	hdbBinRepos ;
class	hdbRgxtype ;

class	hdbADP
{
	//	Category:	Database
	//
	//	hdbADP - Application Delta Profile
	//
	//	The ADP is the entire set of database entities and their delta assignments for an application. Most programs will only have a single ADP but in the case of Dissemino, there
	//	can be multiple ADP instances. Dissemino is routinely configured to listen on the standard ports for HTTP and HTTPS on behalf of multiple hosts (web applications), most of
	//	which require some form of database functionality. Any that do, get an ADP. Because of this, there is no global ADP and programs must declare ADP instances as required.

	hzMapM	<hzString,const hdbClass*>		m_mapSubs ;				//	Cumulative map of all sub-classes
	hzMapS	<hzString,const hdbEnum*>		m_mapEnums ;			//	Total data enums (selectors)
	hzMapS	<hzString,const hdbClass*>		m_mapClasses ;			//	All defined data classes
	hzMapS	<hzString,const hdbDatatype*>	m_mapDatatypes ;		//	Complete set of names of all legal data types including enums and classes

	hzMapS	<hzString,hdbObjRepos*>			m_mapRepositories ;		//	All declared data repositories
	hzMapS	<hzString,hdbBinRepos*>			m_mapBinRepos ;			//	All hdbBinRepos instances

	hzMapS	<uint16_t,const hdbClass*>		m_mapClsCtxDtId ;		//	Map of data class delta ids to data classes
	hzMapS	<hzString,uint16_t>				m_mapClsCtxName ;		//	Map of classname and classname.membername (if of a data class), to data class delta ids
	hzMapS	<uint16_t,const hdbMember*>		m_mapMembers ;			//	Map of member IDs to members (all classes)

	hzArray	<hdbObjRepos*>		m_arrRepositories ;		//	Repositories by Delta id

	hzString	m_appName ;					//	Application name
	hzString	m_Datadir ;					//	Directory of binary core regime and repository deltas
	uint32_t	m_nsqClsUsr ;				//	Data class sequence for user classes (11-40)
	uint32_t	m_nsqClsCfg ;				//	Data class sequence for classes declared within the application configs (41-1000)
	uint32_t	m_nsqClsCtx ;				//	Data class sequence for Sub-class combinations (1001 upwards)
	uint32_t	m_nsqMbrSys ;				//	Data class member sequence for in-built classes (1-500)
	uint32_t	m_nsqMbrUsr ;				//	Data class member sequence for user classes (501-1000)
	uint32_t	m_nsqMbrCfg ;				//	Data class member sequence for all other config classes (1001 upwards)

	hzEcode	_rdClass	(hzXmlNode* pN) ;

	//	Prevent copies
	hdbADP	(const hdbADP&) ;
	hdbADP&	operator=	(const hdbADP&) ;

public:
	//	Constructor and destructors
	hdbADP	(void)
	{
		//m_nsqClsSys = 1 ;			//	System data classes have IDs starting at 1
		m_nsqClsUsr = HZ_ADP_CLS_RNG_USER ;
		m_nsqClsCfg = HZ_ADP_CLS_RNG_APPL ;
		m_nsqClsCtx = HZ_ADP_CLS_RNG_CONTEXT ;
		m_nsqMbrSys = HZ_ADP_MBR_RNG_SYSTEM ;
		m_nsqMbrUsr = HZ_ADP_MBR_RNG_USER ;
		m_nsqMbrCfg = HZ_ADP_MBR_RNG_APPL ;
	}
	~hdbADP	(void)	{}

	//	Init fuction - adds in-built data types
	hzEcode	InitStandard	(const hzString& appName) ;
	hzEcode	InitSubscribers	(const hzString& dataDir) ;
	hzEcode	InitSiteIndex	(const hzString& dataDir) ;
	hzEcode	InitFinancials	(const hzString& dataDir) ;
	hzEcode	InitBlockedIPs	(const hzString& dataDir) ;
	//hzEcode	InitSearch		(void) ;

	//	Insert new data type
	hzEcode	RegisterDataClass	(const hdbClass* pClass) ;
	hzEcode	RegisterComposite	(hzString& context, const hdbClass* pClass)	;
	hzEcode	RegisterMember		(const hdbMember* pMbr) ;
	hzEcode	RegisterDataEnum	(const hdbEnum* pEnum) ;
	hzEcode	RegisterRegexType	(const hdbRgxtype* pRgx) ;

	//	Register repositories
	hzEcode	RegisterObjRepos	(hdbObjRepos* pRepos) ;
	hzEcode	RegisterBinRepos	(hdbBinRepos* pRepos) ;

	//	Obtain data class by data class delta
	const hdbDatatype*	GetDatatype		(const hzString& tname) const	{ return m_mapDatatypes[tname] ; }
	const hdbClass*		GetPureClass	(const hzString& cname) const	{ return m_mapClasses[cname] ; }
	const hdbClass*		GetDataClass	(uint32_t clsId) const			{ return m_mapClsCtxDtId[clsId] ; }
	const hdbEnum*		GetDataEnum		(const hzString& ename) const	{ return m_mapEnums[ename] ; }
	const hdbEnum*		GetDataEnum		(uint32_t n) const				{ return m_mapEnums.GetObj(n) ; }
	uint32_t			CountDataClass	(void) const					{ return m_mapClasses.Count() ; }
	uint32_t			CountDataEnum	(void) const					{ return m_mapEnums.Count() ; }

	//	Get object repository
	hdbObjRepos*	GetObjRepos		(const hzString& rname) const		{ return m_mapRepositories[rname] ; }
	hdbObjRepos*	GetObjRepos		(uint32_t n) const					{ return m_mapRepositories.GetObj(n) ; }
	uint32_t		CountObjRepos	(void) const						{ return m_mapRepositories.Count() ; }
	uint32_t		CountAllMembers	(void) const						{ return m_mapMembers.Count() ; }

	//	Get binary repository
	hdbBinRepos*	GetBinRepos		(const hzString& dsName) const		{ return m_mapBinRepos[dsName] ; }
	uint32_t		CountBinRepos	(void) const						{ return m_mapBinRepos.Count() ; }

	hzEcode	NoteSub		(const hzString& clsName, const hdbClass* pSub)	{ return m_mapSubs.Insert(clsName, pSub) ; }
	bool	IsSubClass	(const hdbClass* pMain, const hdbClass* pSub) ;

	//	Obtain class delta id from either the pure classname or in the case of composite members, classname.membername
	uint16_t	GetDataClassID	(const hzString& clsName) const	{ return m_mapClsCtxName[clsName] ; }

	hzEcode	Export	(void) ;
	hzEcode	Import	(const hzString& appName) ;
	void	Report	(hzChain& Z) ;

	hzEcode	DeltaInit   (const char* dir, const char* app, const char* arg, const char* ver, bool bMustHave) ;
} ;

class	hdbDatatype
{
	//	Category:	Data Definition
	//
	//	hdbDatatype unifies five quite different groups of data types that members of data classes can assume. There is a derivative for each group as follows:-
	//
	//		1)	hdbCpptype	C++ fundamental data types
	//		2)	hdbHzotype	HadronZoo in-built data types
	//		3)	hdbRgxtype	Application specific data types validated by a regular expression
	//		4)	hdbEnum		Application specific data enumeration (validation list)
	//		5)	hdbClass	Application specific data class
	//
	//	The hdbDatatype ensures each data type has a name and a base data type. All data types known to an application are held in _hzGlobal_Datatypes, declared
	//	as a hzMapS<hzString,hdbDatatype*>. This maps data type name to data type and ensures the data type name is unique across all data types, not just those
	//	within the same group.
	//
	//	The data type used to initialize a data class member or other data entity must always be supplied as a pointer to one of the derived classes. Use of the
	//	base class is meaningless. For this reason the hdbDatatype constructor is protected.
	//
	//	Note that InitDatabase() pre-loads both the C++ fundamental and the HadronZoo in-built data types. Any application specific data types are added as they
	//	are encountered, usually in the application config files. 

protected:
	hdbDatatype	(void)
	{
		m_Basetype = BASETYPE_UNDEF ;
	}

	hzString	m_Typename ;	//	Type name
	hdbBasetype	m_Basetype ;	//	Real (internal) data type
	uint32_t	m_Resv ;		//	Reserved

public:
	virtual	~hdbDatatype	(void)	{}

	hzEcode	SetTypename	(const hzString& name)
	{
		if (m_Typename)
			return E_SEQUENCE ;
		m_Typename = name ;
		return E_OK ;
	}

	hzEcode	SetBasetype	(hdbBasetype bt)
	{
		if (m_Basetype != BASETYPE_UNDEF && m_Basetype != BASETYPE_CPP_UNDEF && m_Basetype != BASETYPE_HZO_UNDEF)
			return E_SEQUENCE ;
		m_Basetype = bt ;
		return E_OK ;
	}

	hdbBasetype		Basetype	(void) const	{ return m_Basetype ; }
	const hzString&	StrTypename	(void) const	{ return m_Typename ; }
	const char*		TxtTypename	(void) const	{ return *m_Typename ; }
} ;

class	hdbCpptype : public hdbDatatype
{
	//	Category:	Data Definition
	//
	//	Data type comprising a fundamental C++ type

public:
	hdbCpptype	(void)
	{
		m_Basetype = BASETYPE_CPP_UNDEF ;
	}

	~hdbCpptype	(void)	{}
} ;

class	hdbHzotype : public hdbDatatype
{
	//	Category:	Data Definition
	//
	//	Data type inbuilt with the HadronZoo library

public:
	mutable hzString	m_valJS ;		//	Validation JavaScript (for front-end presentation)

	hdbHzotype	(void)
	{
		m_Basetype = BASETYPE_HZO_UNDEF ;
	}

	~hdbHzotype	(void)	{}
} ;

class	hdbRgxtype : public hdbDatatype
{
	//	Category:	Data Definition
	//
	//	Data type comprising a regular expression controlled text data type

public:
	hzString	m_valJS ;		//	Validation JavaScript (for front-end presentation)

	hdbRgxtype	(void)
	{
		m_Basetype = BASETYPE_APPDEF ;
	}

	~hdbRgxtype	(void)	{}

	hzString	m_Regex ;		//	Regular expression
} ;

class	hdbEnum : public hdbDatatype
{
	//	Category:	Data Definition
	//
	//	The hdbEnum class is an enumerated data type. hdbEnum uses a single instance of hzArray<uint32_t> to effect a one-to-one mapping between pre-set strings
	//	(items) and numerical values (item numbers). The values in the array are string numbers and so are translated to/from null terminated strings (items) by
	//	the global string table. The items numbers are simply the positions of the items in the array. The hdbEnum class is important in Dissemino applications
	//	where datum of an enumerated data type are manifest as HTML selectors, sets of radio buttons or sets of checkboxes.
	//
	//	The string numbers in the global string table (see the hzFsTbl class) are RUIDs (run-time unique identifiers). This is perfectly good for RAM based
	//	data repositories (caches) as these are loaded on startup from their delta files which always contain the full string values. In effect, the delta files
	//	are the authorative record precisely because they are on disk and don't use RUIDs.
	//
	//	In the simple case where the data member using the hdbEnum as its type has a maximum population of 1 (only one item can be selected), the member's value
	//	will be stored on disk as the full string. In a RAM based data cache this can be manifest either as a 4-byte string number or to save space, as an item
	//	number which in cases where the list has less than 256 items, will take only 1 byte. The hdbEnum thus has a variable base data type.
	//
	//	In the more complex case where multiple items can be selected, there are two methods for holding the member's value in RAM. For small hdbEnum lists, the
	//	value can be a stright bitmap whose size is directly related to the list size. For longer hdbEnum lists an encoded bitmap is used. Here the size is more
	//	closely related to the numbers of items actually selected so in general, this is space efficient - although it does involve the overhead of storing data
	//	outside the fixed memory area of the cache.
	//
	//	On disk though, member values in which many items were selected from a large list, would generally be manifest as a verbose entity. Disk space might be
	//	cheap but there is always merit in avoiding verbosity. The verbosity would increase read/write times and would slow transmission of data transactions to
	//	the backup delta servers.
	//
	//	Of course, in the general case, long item lists can be too unweildy in the front end. Selecting your country from a select list being an obvious case in
	//	point. A drill-down approach can ease this problem but one must bear in mind that for now, item lists are not hierarchical.

	//	Prevent copies
	hdbEnum	(const hdbEnum&) ;
	hdbEnum&	operator=	(const hdbEnum&) ;

public:
	hzArray	<uint32_t>	m_Items ;	//	String numbers
	hzArray	<uint32_t>	m_Values ;	//	String values

	uint32_t	m_Default ;		//	Position of default item (commonly 0)
	uint32_t	m_nMax ;		//	Length of longest item

	hdbEnum	(void)
	{
		m_Basetype = BASETYPE_ENUM ;
		m_Default = 0 ;
		m_nMax = 0 ;
	}

	~hdbEnum	(void)
	{
	}

	hzEcode	AddItem	(const hzString& strValue) ;
	hzEcode	AddItem	(const hzString& strValue, uint32_t numValue) ;
} ;

/*
**	SECTION 1:	Classes for data structure/definition.
*/

enum	hdbIniStat
{
	//	Category:	Initialization
	//
	//	Class/Repository Initialization states
	//
	//	Note that classes (hdbClass instances) are constructed with an initialization state of HDB_CLASS_INIT_NONE. There is an initialization sequence which is
	//	strictly enforced. This begins with a call to InitStart() which takes the initialization state to HDB_CLASS_INIT_PROG. Then for each class member, there
	//	is a call to InitMember(). This function expects an initialization state of HDB_CLASS_INIT_PROG otherwise it will terminate execution. InitMember() does
	//	not alter the initialization state. Finally there is a call to InitDone(). This also expects an initialization state of HDB_CLASS_INIT_PROG otherwise it
	//	will terminate execution. InitDone() sets the initialization state of the hdbClass instance to HDB_CLASS_INIT_DONE which is the end of the process for a
	//	data class.
	//
	//	Repositories are also constructed with an initialization state of HDB_CLASS_INIT_NONE and have a similar initialization sequence to classes. InitStart()
	//	for a repository takes an already initialized class so the class members are all known. The functions InitMbrIndex() and InitMbrStore() can be called to
	//	add an index or define how a member is stored but these are optional. As with classes, InitDone() completes the initialization sequence. The repository
	//	can then be opened, operated upon and closed.

	HDB_CLASS_INIT_NONE,	//	No initialization has begun
	HDB_CLASS_INIT_PROG,	//	The hdbClass member function InitStart() has been called and so the class definition is in progress, members can be added.
	HDB_CLASS_INIT_DONE,	//	The hdbClass member function InitDone() has been called and so the class definition is complete. No new members can be added.
	HDB_REPOS_INIT_PROG,	//	The repository member function InitStart() has been called so initialization steps relating to members can be effected.
	HDB_REPOS_INIT_DONE,	//	The repository member function InitDone() has been called. No further initialization is permitted.
	HDB_REPOS_OPEN			//	The repository is open to transactions. If the repository is closed the state reverts to HDB_REPOS_INIT_DONE.
} ;

class	hdsFldspec ;	//	Dissemino guest class

enum	hdbClsDgn
{
	//	Data Class Designation: Data classes are either:-

	HDB_CLASS_DESIG_SYS,	//	In-built data class
	HDB_CLASS_DESIG_USR,	//	User data class
	HDB_CLASS_DESIG_CFG		//	General data class
} ;

class	hdbMember
{
	//	The class memeber

private:
	const	hdbClass*		m_pClass ;		//	Data class of which this is member
	const	hdbDatatype*	m_pType ;		//	Data type
	mutable hdsFldspec*		m_pSpec ;		//	Dissemino field specification for rendering HTML (not for internal db consideration)
	mutable	uint16_t		m_DeltaMbrId ;	//	Unique member delta ID

	uint16_t	m_popMax ;			//	Maximum number of values (or 0 for unlimited)
	uint16_t	m_popMin ;			//	Minimum number of values (0 for can be empty, 1 for required)
	int16_t		m_nPosn ;			//	Position within class (effectively member id)
	int16_t		m_nDataSize ;		//	Data size (size of value will depend on data type)
	hzString	m_Name ;			//	Member name, must be unique within the data class

	//	Prevent copying
	hdbMember	(const hdbMember&) ;
	hdbMember&	operator=	(const hdbMember&) ;

public:
	mutable	hzString	m_Desc ;			//	For application specific purposes, data members may have an optional description
	mutable	hzString	m_dsmTabSubject ;	//	Dissemino tab section subject (not for internal db consideration)

	hdbMember	(void)
	{
		m_pClass = 0 ;
		m_pType = 0 ;
		m_pSpec = 0 ;
		m_DeltaMbrId = 0 ;
		m_popMax = 1 ;
		m_popMin = 0 ;
		m_nPosn = -1 ;
		m_nDataSize = -1 ;
	}
	~hdbMember	(void)	{}

	//	Set functions
	hzEcode	Init	(const hdbClass* pClass, const hdbDatatype* pType, const hzString& memberName, uint32_t minPop, uint32_t maxPop, uint32_t nPosn) ;
	void	_setId	(uint32_t mbrId) const		{ m_DeltaMbrId = mbrId ; }
	void	SetSpec	(hdsFldspec* pSpec) const	{ m_pSpec = pSpec ; }

	//	Get functions
	const hdbDatatype*	Datatype	(void) const	{ return m_pType ; }
	const hdsFldspec*	GetSpec		(void) const	{ return m_pSpec ; }

	bool			IsClass		(void) const	{ return !m_pType ? false : m_pType->Basetype() == BASETYPE_CLASS ? true : false ; }
	hzString		StrName		(void) const	{ return m_Name ; }
	const char*		TxtName		(void) const	{ return *m_Name ; }
	hzString		StrDesc		(void) const	{ return m_Desc ; }
	const char*		TxtDesc		(void) const	{ return *m_Desc ; }
	const hdbClass*	Class		(void) const	{ return m_pClass ; }
	hdbBasetype		Basetype	(void) const	{ return m_pType ? m_pType->Basetype() : BASETYPE_UNDEF ; }

	uint32_t	Posn		(void) const	{ return m_nPosn ; }
	uint32_t	MinPop		(void) const	{ return m_popMin ; }
	uint32_t	MaxPop		(void) const	{ return m_popMax ; }
	bool		IsMandatory	(void) const	{ return m_popMin ? true : false ; }
	bool		IsMultiple	(void) const	{ return m_popMax ? false : true ; }
	uint32_t	DataSize	(void) const	{ return (uint32_t) m_nDataSize ; }
	uint16_t	DeltaId		(void) const	{ return m_DeltaMbrId ; }

	//	Compare
	bool	operator==	(const hdbMember& op) const ;
	bool	operator!=	(const hdbMember& op) const	{ return !operator==(op) ; }
} ;

class	hdbClass : public hdbDatatype
{
	//	Category:	Data Definition
	//
	//	hdbClass defines a data class as a set of data class members with each having a name (unique within the class) and a predefined data type. In addition, members must specify
	//	a minimum population and depending on the data type, a maximum population. The minimum population can only be 0 or 1 and states if the member is compulsory. If set to 1, an
	//	object of the data class would be invalid without the member having a value. The maximum population is in theory, a positive integer or 0 for unlimited. In practice it may
	//	only be 1 for single-value members and 0 for members that allow multiple values. There was a plan to allow members to be limited to a specific number of values but this was
	//	never implemented.
	//
	//	Since data classes are themselves data types, the data type a member may be of includes previously defined data classes. Thus, within a data object of a main class, members
	//	can hold data objects of a sub-class. By this means, hdbClass supports full hierarchy.
	//
	//	Defining a data class is a matter of initializing a hdbClass instance. There is an initialization sequence which is strictly enforced. This begins with a call to InitStart,
	//	then for each member a call to InitMember, then finally, a call to InitDone. If this sequence is violated, the program exits! Data classes must first be defined before they
	//	can be used to initialize the repositories that will store objects of the given data class. This is directly akin to firstly defining C++ classes in header files, and later
	//	declaring instances of them in a source file. A class can exist without an instance but an instance cannot exist without a class! Since data classes must be defined before
	//	use, the HDB whilst hierarchichal, is not fully extensible. No data classs can aquire additional members once defined.
	//
	//	Data Class Member Delta IDs:
	//
	//	Please be aware of the data class id regime. All data classes have a 'pure' id but are generally referred to by their delta id which depends on the context of their use. If
	//	the data class has no sub-class members, the two ids will always be the same. However where a class is used as a sub-class, its delta id will differ from the pure id. This
	//	regime is described in the HadronZoo Library Overview.

private:
	hzMapS	<hzString,hdbMember*>		m_mapMembers ;	//	Members by member name
	hzArray	<hdbMember*>				m_arrMembers ;	//	Members in order of incidence

	hdbADP*		m_pADP ;			//	Host ADP
	hdbIniStat	m_eClassInit ;		//	Init state
	hdbClsDgn	m_eDesignation ;	//	Class designated as system, user or general
	uint16_t	m_nMinSpace ;		//	Minimum size of object, given litmus bits and members with minimum populations
	uint16_t	m_nLitmusSize ;		//	Size of litmus in bytes (no of member rounded up to xple of 8, then divide by 8)
	_m uint16_t	m_ClassId ;			//	Unique class id

	//	Prevent copies
	hdbClass	(hdbClass& op) ;
	hdbClass&	operator=	(hdbClass& op) ;

	void	_clear	(void) ;

public:
	hzString	m_Category ;	//	This has no database purpose but is helpful for Dissemino

	hdbClass	(hdbADP& adp, hdbClsDgn designation) ;
	~hdbClass	(void) ;

	//	Get Member inlines
	const hdbMember*	GetMember	(const hzString& mbr) const	{ return m_mapMembers[mbr] ; } 
	const hdbMember*	GetMember	(uint32_t mbrNo) const		{ return mbrNo < m_arrMembers.Count() ? m_arrMembers[mbrNo] : 0 ; }

	uint32_t	MemberNo	(const hzString& mbr) const
	{
		const hdbMember*	pMem ;

		pMem = m_mapMembers[mbr] ;
		return pMem ? pMem->Posn() : 0xffffffff ;
	}

	//	Init from DB method.
	hzEcode		InitStart	(const hzString& name) ;
	hzEcode		InitMember	(const hzString& name, const hdbDatatype* type, uint32_t minPop, uint32_t maxPop) ;
	hzEcode		InitMember	(const hzString& name, const hzString& type, uint32_t minPop, uint32_t maxPop) ;
	hzEcode		InitDone	(void) ;

	//	Diagnostics
	void	DescClass	(hzChain& Z, uint32_t nIndent) const ;
	hzEcode	DescCheck	(hzChain& report, hzChain& desc) const ;

	//	Get functions
	const hdbADP*	GetADP	(void) const	{ return m_pADP ; }

	hdbClsDgn	Designation	(void) const	{ return m_eDesignation ; }
	uint32_t	MinSpace	(void) const	{ return m_nMinSpace ; }
	uint32_t	LitmusSize	(void) const	{ return (m_arrMembers.Count()%8) ? 1 + (m_arrMembers.Count()/8) : (m_arrMembers.Count()/8) ; }
	uint32_t	MbrCount	(void) const	{ return m_arrMembers.Count() ; }
	uint32_t	ClassId		(void) const	{ return m_ClassId ; }
	bool		IsInit		(void) const	{ return m_eClassInit == HDB_CLASS_INIT_DONE ? true : false ; }

	//	Test sub class
	//	const hdbClass*	SubClass	(const hzString& subClassname) const ;

	bool	operator==	(const hdbClass& op) const ;
	bool	operator!=	(const hdbClass& op) const	{ return !operator==(op) ; }

	void	_setId	(uint32_t id) const	{ m_ClassId = id ; }
} ;

//#define	clsMbr	hdbClass::Member

/*
**	Data Class Instances: hdbObject and hzAtom
*/

//	Atom status
#define ATOM_CLEAR		0x00	//	Atom is clear
#define ATOM_SET		0x01	//	value set OK
#define ATOM_CHAIN		0x02	//	Atom holding hzChain
#define ATOM_STRNO		0x04	//	This applies only to string-like data types. If set, the value is a string number.

class	hzAtom
{
	//	Category:	Database
	//
	//	As described in the Library Overview (2.3.h1 Data Classes), hzAtom holds a single datum of any atomic HadronZoo data type and so serves as a universal means of passing and
	//	holding atomic values.
	//
	//	hzAtom comprises an atomval union with members of types char*, double, 64, 32, 16 and 8-bit signed and unsigned integers and a bool - together with a hzBasetype indicator,
	//	a hzString, a hzChain and control flags. An atomval on its own cannot state which of its members applies and nor can it disambiguate zero, hence the control flags.
	//
	//	The hzString is used to directly hold the value where the type is BASETYPE_STRING and where the type is BASETYPE_DOMAIN, BASETYPE_EMADDR or BASETYPE_URL, to hold the value
	//	as a string form. The hzString also enables hzAtom to avail the text form of any of the non-binary types held by atomval. The hzChain holds datum of binary data types.

	hzChain		m_Chain ;		//	For binary datum
	_atomval	m_Data ;		//	Actual data or pointer to document/binary/start of list
	_m hzString	m_Str ;			//	For string values use this instead of m_Data
	uint16_t	m_eType ;		//	Data type adopted
	uint16_t	m_eStatus ;		//	Set on assignment, either NULL, OK or some error

public:
	hzAtom	(void)
	{
		//	Standard constructor. The atom will be empty and be of unknown type.
		m_Data.m_uInt64 = 0 ;
		m_eType = (uint16_t) BASETYPE_UNDEF;
		m_eStatus = ATOM_CLEAR ;
	}

	hzAtom	(const hzAtom& op)
	{
		//  Copy constructor. The atom will be of the type and value of the operand.
		m_Data = op.m_Data ;
		m_eType = op.m_eType ;
		m_eStatus = op.m_eStatus ;
	}

	~hzAtom	(void)
	{
		//  Destructor. This will delete any strings the atom had retained.
		Clear() ;
	}

	hdbBasetype	Type	(void) const	{ return (hdbBasetype) m_eType ; }				//	Get internal type of hzAtom

	bool	IsSet	(void) const	{ return m_eStatus == ATOM_SET ? true : false ; }
	bool	IsNull	(void) const	{ return m_eStatus == ATOM_SET ? false : true ; }

	//	Get functions
	hzString	Str		(void) const ;
	hzDomain	Domain	(void) const ;
	hzEmaddr	Emaddr	(void) const ;
	hzUrl		Url		(void) const ;
	hzXDate		XDate	(void) const ;
	hzSDate		SDate	(void) const ;
	hzTime		Time	(void) const ;
	hzIpaddr	Ipaddr	(void) const ;

	hzChain		Chain	(void) const	{ return m_Chain ; }
	_atomval	Datum	(void) const	{ return m_Data ; }
	const void*	Binary	(void) const	{ return &m_Data ; }
	double		Double	(void) const	{ return m_Data.m_Double ; }
	uint64_t	Unt64	(void) const	{ return m_Data.m_uInt64 ; }
	uint32_t	Unt32	(void) const	{ return m_Data.m_uInt32 ; }
	uint16_t	Unt16	(void) const	{ return m_Data.m_uInt16 ; }
	int64_t		Int64	(void) const	{ return m_Data.m_sInt64 ; }
	int32_t		Int32	(void) const	{ return m_Data.m_sInt32 ; }
	int16_t		Int16	(void) const	{ return m_Data.m_sInt16 ; }
	char		Byte	(void) const	{ return m_Data.m_sByte ; }
	uchar		UByte	(void) const	{ return m_Data.m_uByte ; }
	bool		Bool	(void) const	{ return m_Data.m_Bool ; }

	//	Casting operators (HadronZoo data types)
	operator const hzString	(void) const	{ return Str() ; }
	operator const hzChain	(void) const	{ return Chain() ; }
	operator const hzIpaddr	(void) const	{ return Ipaddr() ; }
	operator const hzDomain	(void) const	{ return Domain() ; }
	operator const hzEmaddr	(void) const	{ return Emaddr() ; }
	operator const hzUrl	(void) const	{ return Url() ; }
	operator const hzXDate	(void) const	{ return XDate() ; }
	operator const hzSDate	(void) const	{ return SDate() ; }
	operator const hzTime	(void) const	{ return Time() ; }

	//	Casting operators (fundamental data types)
	operator double		(void) const	{ return m_Data.m_Double ; }
	operator int64_t	(void) const	{ return m_Data.m_sInt64 ; }
	operator uint64_t	(void) const	{ return m_Data.m_uInt64 ; }
	operator int32_t	(void) const	{ return m_Data.m_sInt32 ; }
	operator uint32_t	(void) const	{ return m_Data.m_uInt32 ; }
	operator int16_t	(void) const	{ return m_Data.m_sInt16 ; }
	operator uint16_t	(void) const	{ return m_Data.m_uInt16 ; }
	operator char		(void) const	{ return m_Data.m_sByte ; }
	operator uchar		(void) const	{ return m_Data.m_uByte ; }

	/*
	**	Setting hzAtom values
	*/

	//	Set value by datatype and free format string
	hzEcode	SetValue	(hdbBasetype eType, const hzString& S) ;
	hzEcode	SetValue	(hdbBasetype eType, const hzChain& Z) ;
	hzEcode	SetValue	(hdbBasetype eType, const _atomval& av) ;
	hzEcode	SetNumber	(const char* s) ;
	hzEcode	SetNumber	(const hzString& s)	{ return SetNumber(*s) ; }

	//	Set values by types
	hzAtom&	operator=	(const hzAtom& a) ;
	hzAtom&	operator=	(const hzChain& Z) ;
	hzAtom&	operator=	(const hzString& s) ;
	hzAtom&	operator=	(const hzIpaddr& v) ;
	hzAtom&	operator=	(const hzDomain& v) ;
	hzAtom&	operator=	(const hzEmaddr& v) ;
	hzAtom&	operator=	(const hzUrl& v) ;
	hzAtom&	operator=	(const hzXDate& v) ;
	hzAtom&	operator=	(const hzSDate& v) ;
	hzAtom&	operator=	(const hzTime& v) ;
	hzAtom&	operator=	(double v) ;
	hzAtom&	operator=	(int64_t v) ;
	hzAtom&	operator=	(uint64_t v) ;
	hzAtom&	operator=	(int32_t v) ;
	hzAtom&	operator=	(uint32_t v) ;
	hzAtom&	operator=	(int16_t v) ;
	hzAtom&	operator=	(uint16_t v) ;
	hzAtom&	operator=	(char v) ;
	hzAtom&	operator=	(uchar v) ;
	hzAtom&	operator=	(bool b) ;

	//	Set value to null
	hzAtom&	Clear		(void) ;

	//	Stream integration
	friend	std::ostream&	operator<<	(std::ostream& os, const hzAtom& obj) ;
} ;

class	hdbROMID
{
	//	Category:	Database
	//
	//	hdbROMID is primarily a support class for the single object container class, hdbObject. It comprises a 32-bit object id, a 16-bit class id and a 16-bit member id. hdbObject
	//	is implemented as a 1:many map of hdbROMID instances to values. This structure allows hdbObject to accommodate complex objects i.e. objects with sub-class objects contained
	//	within them.
	//
	//	The hdbROMID name is controversial because the class does not hold a true ROMID (Real Object Member Identifier). As mentioned in the Library Overview, in HadronZoo parlance
	//	a data object becomes real when it is inserted into a data object repository and assigned an object id. The term 'real' in this context means 'can be addressed'. To address
	//	a member of a real data class object, a true ROMID will specify the repository, the class (as this may be a sub-class of the repository native class), the object id and the
	//	member id.

public:
	uint32_t	m_ObjId ;	//	Object number
	uint16_t	m_ClsId ;	//	Class Delta ID
	uint16_t	m_MbrId ;	//	Member number

	hdbROMID	(void)	{ m_ClsId = m_MbrId = 0 ; m_ObjId = 0 ; }

	bool	operator<	(const hdbROMID& op) const
	{
		if (m_ClsId < op.m_ClsId)	return true ;
		if (m_ClsId > op.m_ClsId)	return false ;
		if (m_ObjId < op.m_ObjId)	return true ;
		if (m_ObjId > op.m_ObjId)	return false ;
		if (m_MbrId < op.m_MbrId)	return true ;
		return false ;
	}

	bool	operator>	(const hdbROMID& op) const
	{
		if (m_ClsId > op.m_ClsId)	return true ;
		if (m_ClsId < op.m_ClsId)	return false ;
		if (m_ObjId > op.m_ObjId)	return true ;
		if (m_ObjId < op.m_ObjId)	return false ;
		if (m_MbrId > op.m_MbrId)	return true ;
		return false ;
	}
} ;

class	hdbObject
{
	//	Category:	Database
	//
	//	hdbObject is a single data object container. Once initialized to a data class, hdbObject can be populated by an object of that data class. hdbObject holds the object 'open'
	//	so that it can be directly operate upon by the application, and is the principle means by which data objects are moved in and out of repositories. The repository INSERT and
	//	MODIFY operations store a populated hdbObject instance while the FETCH operation populates a blank hdbObject instance.
	//
	//	hdbObject is implemented as a one to many mapping between hdbROMID (partial deltas and values. The partial delta (hdbObject::_deltaInd), comprises a class, object
	//	and member id. This is described in detail in Section 3 of the synopsis 'Application Specific Cloud Computing'.
	//	
	//	<b>Note on Import/Export of JSON</b>
	//
	//	JSON as is widely documented, is built on two structures as follows:-
	//
	//		1) An unordered collection of name/value pairs (object)
	//		2) An ordered list of values (list or array)
	//
	//	The format of a JSON object (unordered set of name/value pairs) begins/ends with opening/closing curly braces containing entries of the form name:value
	//	separated by a comma. The name refers to a data class member while the value must evaluate to a legal value of the member's data type. The format of an
	//	array begins/ends with opening/closing square brackets containing comma separated values. The values in both structures may be of the following types:-
	//
	//		1)	A string (sequence of 0 or more unicode characters) enclosed in double quotes
	//		2)	A number (decimal and standard form)
	//		3)	True/False
	//		4)	Null
	//		5)	An object (using {})
	//		6)	An array (using []) These structures can be nested.
	//
	//	From this description it is clear JSON itself, cannot support the range of data types used by the HDB. However, it is still possible to import JSON data
	//	as the IMPORT process can be guided by the data class. The member names do have to be right though. Any name value pair whose name is not a class member
	//	will be ignored. The same applies values in a JSON list, if these exceed the member's maximum population they are ignored. The same applies where types
	//	are not matched. A member expecting an email address can accept a JSON string but not a JSON number. And if it is an object in the JSON the member must
	//	be a class in the hdbObject. Export is a lot easier as all HDB data types can map unambiguously to JSON data types. 

	const hdbClass*	m_pClass ;	//	Object class

	hzString	m_Key ;			//	Unique name within app
	uint32_t	m_ObjId ;		//	Current object id
	uint16_t	m_ReposId ;		//	Repository id
	uint16_t	m_ClassId ;		//	Host class delta id
	uint16_t	m_ClassMax ;	//	Highest class delta id of a host class sub-class (if none this will be equal to m_ClassId)
	bool		m_bInit ;		//	set if initialization successful

	//	Private support functions
	hzEcode		_import_json_r	(hzChain& error, hzChain::Iter& zi, const hdbClass* pClass) ;
	hzEcode		_export_json_r	(hzChain& json, const hdbClass* pClass, uint32_t nLevel) const ;

	//	Prevent copies
	hdbObject	(const hdbObject&) ;
	hdbObject&	operator=	(const hdbObject&) ;

public:
	hzMapM<hdbROMID,uint32_t>	m_Values ;		//	Map of class/object/member to either non string like values or offsets to values in m_Large or m_Strings
	hzArray<_atomval>			m_Large ;		//	All 64-bit values (including pointers to chains).
	hzArray<hzString>			m_Strings ;		//	All string values (including email addresses and URL's).

	hdbObject	(void) ;	//	Constructor
	~hdbObject	(void) ;	//	Destructor

	hzEcode	Init		(const hzString& objKey, const hdbClass* pObjClass) ;

	hdbObject*	Node	(uint32_t nIndex) const ;
	hzEcode		Clear	(void) ;

	void		SetId	(uint32_t objId)	{ m_ObjId = objId ; }

	const hdbClass*		Class	(void) const	{ return m_pClass ; }
	const hzString&		ObjKey	(void) const	{ return m_Key ; }

	const char*	Classname	(void) const	{ return m_pClass ? m_pClass->TxtTypename() : 0 ; }
	uint32_t	ObjId		(void) const	{ return m_ObjId ; }
	uint32_t	ReposId		(void) const	{ return m_ReposId ; }
	bool		IsInit		(void) const	{ return m_bInit ; }
	bool		IsNull		(void) const ;

	//	Set value, prefered method. Note when the member being set is a list, this will just add the value
	hzEcode		SetValue	(uint32_t memNo, const hzAtom& atom) ;
	hzEcode		SetValue	(uint32_t memNo, const hzString& value) ;
	hzEcode		SetValue	(const hzString& name, const hzString& value) ;
	hzEcode		SetBinary	(const hzString& name, const hzChain& value) ;

	//	Get value by ROMID
	hzEcode		GetValue	(hzAtom& atom, const hdbROMID& romid) const ;

	//	Import and Export
	hzEcode		ImportSOBJ	(const hzChain& J, uint32_t objId) ;
	hzEcode		ExportSOBJ	(const hzChain& J) const ;
	hzEcode		ImportJSON	(const hzChain& J) ;
	hzEcode		ExportJSON	(hzChain& J) const ;
	hzEcode		ListSubs	(hzArray<uint32_t>& list, uint32_t clsId) const ;
} ;

/*
**	SECTION 3: Data Object Repositories
*/

class	hdbBinRepos
{
	//	Category:	Database
	//
	//	The hdbBinRepos is described in the HadronZoo synopsis, chapter 3.7 "Binary Datum Repositories"

	class	_datum_hd
	{
		//	Datum header in index file
	public:
		hzXDate		m_DTStamp ;			//	Date time stamp
		uint64_t	m_Addr ;			//	Address within superblock 
		uint32_t	m_Size ;			//	Datum size in bytes
		uint32_t	m_Prev ;			//	Previous datum id (0 if new)
		uint32_t	m_Appnote1 ;		//	Value specified by application (if any)
		uint32_t	m_Appnote2 ;		//	Value specified by application (if any)
	} ;

	//	Operational parameters
	std::ofstream	m_WrI ;				//	Index file output stream for Insert/Update
	std::ofstream	m_WrD ;				//	Data file output stream for Insert/Update
	std::ifstream	m_RdI ;				//	Data file input stream for Fetch
	std::ifstream	m_RdD ;				//	Data file input stream for Fetch

	hdbADP*		m_pADP ;				//	Host ADP
	uint64_t	m_nSize ;				//	Size of data file
	hzString	m_Name ;				//	Name of object store file
	hzString	m_Workdir ;				//	Directory where object store file is
	hzString	m_FileData ;			//	Name of row data file
	hzString	m_FileIndx ;			//	Name of index file (addresses and sizes of rows)
	hzLockS		m_LockIrd ;				//	Lock on index file read
	hzLockS		m_LockIwr ;				//	Lock on index file write
	hzLockS		m_LockDrd ;				//	Lock on data file read
	hzLockS		m_LockDwr ;				//	Lock on data file write
	uint32_t	m_nDatum ;				//	Number of rows
	uint32_t	m_nInitState ;			//	Initialization state
	char		m_Data[HZ_BLOCKSIZE] ;	//	Internal buffer

	//	Write function
	void	_deltaWrite	(void) ;

	//	Prevent copies
	hdbBinRepos	(const hdbBinRepos&) ;
	hdbBinRepos&	operator=	(const hdbBinRepos&) ;

public:
	hzChain	m_Error ;		//	Error or report string

	hdbBinRepos		(hdbADP& adp) ;
	~hdbBinRepos	(void) ;

	//	Get functions
	hzString	Name	(void) const	{ return m_Name ; }
	bool		IState	(void) const	{ return m_nInitState ; }
	uint32_t	Count	(void) const	{ return m_nDatum ; }
	uint32_t	Size	(void) const	{ return m_nSize ; }

	//	Init & Halt
	hzEcode	Init	(const hzString& name, const hzString& opdir) ;
	hzEcode	Open	(void) ;
	hzEcode	Close	(void) ;
	hzEcode	Integ	(hzLogger& pLog) ;

	//	Data operations
	hzEcode	Insert	(uint32_t& datumId, const hzChain& datum) ;
	hzEcode	Insert	(uint32_t& datumId, const hzChain& datum, uint32_t an1, uint32_t an2) ;
	hzEcode	Update	(uint32_t& datumId, const hzChain& datum) ;
	hzEcode	Update	(uint32_t& datumId, const hzChain& datum, uint32_t an1, uint32_t an2) ;
	hzEcode	Delete	(uint32_t datumId) ;
	hzEcode	Fetch	(hzChain& datum, uint32_t datumId) ;
} ;

/*
**	Idset class
*/

//	Sizes
#define BITMAP_SEGSIZE	256		//	Range for segment
#define SEG_FULLSIZE	 32		//	Max size of seg bitmap data area

//	Operational flags for bitmap segments
//	#define BMSEG_FIXED		0x01	//	Segment is fixed (does not allow inserts or deletes)
//	#define BMSEG_INVERTED	0x02	//	Segment is inverted (used when majority of values are occupied)

//	Operational flags for bitmaps
//	#define BITMAP_FIXED	0x01	//	Bitmap is fixed (will ensure all its segments are also fixed)

class	hdbIdset
{
	//	Category:	Index
	//
	//	The hdbIdset is the fronline class used in applications and manages bitmap segments. In its initial state it has no segments
	//	Bitmap processor. These are bitmaps but are a more efficient format for processing. The price paid is space inefficiency as the
	//	segments are held in maps rather than concatenated as they are in the hzBitmapS (storage) class.

	struct	_bitmap_ca
	{
		//	Bitmap internal structure to facilitate soft copy

		hzMapS<uint16_t,uint32_t>	m_segments ;	//	Map of segmets by segment number

		uint32_t	m_Addr ;		//	Either the address of the root segment or the lone value if count is 1
		uint32_t	m_Total ;		//	Total of ids held in bitmap
		uint16_t	m_copies ;		//	Copy count
		uint16_t	m_resv ;		//	Reserved

		_bitmap_ca	(void)	{ m_Total = 0 ; m_copies = 0 ; }
	} ;

	_bitmap_ca*	mx ;	//	Internal instance

public:
	hdbIdset	(const hdbIdset& op) ;
	hdbIdset	(void) ;

	~hdbIdset	(void) ;

	uint32_t	Count		(void) const	{ return mx ? mx->m_Total : 0 ; }
	uint32_t	NoSegs		(void) const	{ return mx ? mx->m_segments.Count() : 0 ; }

	void		Clear		(void) ;
	uint32_t	Insert		(uint32_t docId) ;
	hzEcode		Delete		(uint32_t docId) ;
	uint32_t	DelRange	(uint32_t lo, uint32_t hi) ;
	uint32_t	Fetch		(hzVect<uint32_t>& Result, uint32_t nStart, uint32_t nReq) const ;

	//	Operators
	hdbIdset&	operator=	(const hdbIdset& op) ;
	hdbIdset&	operator|=	(const hdbIdset& op) ;
	hdbIdset&	operator&=	(const hdbIdset& op) ;

	//	Import and Export
	hzEcode	Import	(hzChain::Iter& ci) ;
	void	Export	(hzChain& C) const ;
} ;

/*
**	Idset Prototypes
*/

uint32_t	CountBits	(const void* pvBuffer, uint32_t nNoBytes) ;
void		SetBits		(char* pBitbuf, uint32_t nOset, bool bValue) ;
bool		GetBits		(const char* pBitbuf, uint32_t nOset) ;

/*
**	Class based Repositories
*/

class	hdbIndex ;

class	hdbObjRepos
{
	//	Category:	Database
	//
	//	hdbObjRepos is a base class to unify its two derivatives, the RAM based object repository hdbObjCache and the disk based hdbObjStore. Repositories store
	//	data objects (instances of a data class). Like the relational database tables they replace, repositories support the operations of INSERT, UPDATE, FETCH
	//	and DELETE operations. Unlike the tables, repositories support hierarchy.
	//
	//	Object repositories require initialization by a predefined data class. In the HDB, you cannot infer data class definitions by creating repositories and
	//	adding members, in the same way that you can create a table and then add columns. This restriction has simplified the development of the HDB but it also
	//	serves other purposes. It encourages clearer database designs and is more in line with programing languages such as C++.
	//
	//	Although repository initialization is simplified by using a predefined class, there is still an initialization sequence. The class definition determines
	//	how an object of the class is built, not how collections of such objects are built. It is for the repository to decide if objects are to be indexed, and
	//	if so, in what way. And it is for the repository to decide if the objects are to be unique by what members.
	//
	//	InitStart() takes the pre-defined class, names the repository and thus the data file, and it names the working directory. After calling InitStart() and
	//	before calling InitDone() to conclude the initialization, you may call:-
	//
	//		1)	InitMbrIndex()	To add an index to a mamber - as long as the member is indexable member and does not already have an index.
	//
	//		2)	InitMbrStore()	Both OBJECT repositories store binary member data in a BINARY repository. This fn names which one.
	//
	//	As this is part of initialization, both these functions exit if called on members with incompatible types.

protected:
	const hdbClass*	m_pClass ;		//	Data object class
	hzString		m_Name ;		//	The unique repository name
	hzString		m_Workdir ;		//	The working directory for the spool file
	hzString		m_Workpath ;	//	The full path for the spool file (will be of same name as class + .cache)
	uint16_t		m_DeltaId ;		//	Repository id (posn in repositories array in ADP)
	uint16_t		m_bDeletes ;	//	0 means no deletes, 1 deletes are supported by means of m_Objects mapping
	hdbIniStat		m_eReposInit ;	//	Controlls initialization

public:
	hdbObjRepos	(void)
	{
		m_DeltaId = 0 ;
		m_pClass = 0 ;
		m_bDeletes = 0 ;
	}

	virtual	~hdbObjRepos	(void)
	{
		m_pClass = 0 ;
	}

	virtual	hzEcode	InitStart		(const hdbClass* pObjClass, const hzString& reposName, const hzString& workdir) = 0 ;
	virtual	hzEcode	InitMbrIndex	(const hzString& memberName, bool bUnique) = 0 ;
	virtual	hzEcode	InitMbrStore	(const hzString& memberName, const hzString& binaryName, bool bStore) = 0 ;
	virtual	hzEcode	InitDone		(void) = 0 ;

	//	Post Init methods
	virtual	hzEcode	Open		(void) = 0 ;									//	Load in data from backup file (only call once)
	virtual	hzEcode	Insert		(uint32_t& objId, const hdbObject& obj) = 0 ;	//	Standard insert operation, fails if members defined as unique are duplicated
	virtual	hzEcode	Update		(hdbObject& obj, uint32_t objId) = 0 ;			//	Standard modify operation, fails only if the stated object id does not exist
	virtual	hzEcode	Fetch		(hdbObject& obj, uint32_t objId) = 0 ;			//	Loads an object from the cache into the supplied object container
	virtual	hzEcode	Delete		(uint32_t objId) = 0 ;							//	Standard delete operation, fails only if the stated object id does not exist

	//hzEcode	Aggregate	(const hzChain& json) ;			//	Insert new or update existing object from supplied data in JSON format

	//	General Get funtions
	virtual	uint32_t	Count	(void) = 0 ;

	hdbIniStat		InitState	(void) const	{ return m_eReposInit ; }
	hzString		Name		(void) const	{ return m_Name ; }
	const hdbClass*	Class		(void) const	{ return m_pClass ; }
	const char*		Classname	(void) const	{ return m_pClass ? m_pClass->TxtTypename() : 0 ; }
	uint32_t		DeltaId		(void) const	{ return m_DeltaId ; }

	//	Import/Export
	void	DescRepos	(hzChain& Z, uint32_t nIndent) const ;
	hzEcode	LoadCSV		(const hdbClass* pSubclass, const char* filepath, const char* delim, bool bQuote) ;
} ;

class	hdbObjCache : public hdbObjRepos
{
	//	Category:	Database
	//
	//	hdbObjCache is a RAM based data object repository. As per article 3.6 of the Library overview.

	struct	_mbr_data
	{
		//	Extra member meta info

		uint16_t	m_nOset ;		//	Relative offset of fixed space data value
		uint16_t	m_nLitmus ;		//	Litmus bit offset if applicable
		uchar		m_nList ;		//	List number (var data only)
		uchar		m_nChannel ;	//	Index list or channel
		uchar		m_nSize ;		//	Data size (4 or 8 bytes or size of class)
		uchar		m_Flag ;		//	Set according to incidnce of litmus/list/fixed

		_mbr_data	()	{ m_nOset = m_nLitmus = 0 ; m_nList = m_nChannel = m_nSize = m_Flag = 0 ; }
	} ;

	class	_cache_blk
	{
		//	General framework for holding objects of a given class. There will one of these for the host class and every sub-class.

		hzArray	<void*>	m_Ram ;			//	The data object blocks

		const hdbClass*	m_pClass ;		//	The class of objets

		uint32_t	m_nBlksize ;		//	RAM block size
		uint32_t	m_nBlklists ;		//	Number of lists
		uint32_t	m_nMembers ;		//	Class member count
		uint32_t	m_nCachePop ;		//	Total number of objects allocated

		void	_clear	(void) ;		//	Clears all blocks

		//	Prevent copying
		_cache_blk		(const _cache_blk& op) ;
		_cache_blk&	operator=	(const _cache_blk& op) ;

	public:
		_mbr_data*	m_Info ;		//	Object member info

		_cache_blk		(void)
		{
			m_pClass = 0 ;
			m_nBlksize = m_nBlklists = m_nCachePop = 0 ;
		}

		~_cache_blk	(void)	{ _clear() ; }

		void		Clear	(void)	{ _clear() ; }
		uint32_t	Count	(void)	{ return m_nCachePop ; }

		hzEcode	InitMatrix	(const hdbClass* pClass) ;

		hzEcode	AssignSlot	(uint32_t& nSlot, uint32_t objId, uint32_t mode) ;

		char*	SetPointer	(char* ptr, uint32_t nSlot, uint32_t mbrNo) ;

		hzEcode	_setMbrValue	(uint32_t objId, uint32_t mbrNo, _atomval value) ;
		hzEcode	_setMbrBool		(uint32_t objId, uint32_t mbrNo, bool bValue) ;
		hzEcode	_setMbrLitmus	(uint32_t objId, bool bValue) ;

		hzEcode	GetVal		(_atomval& value, uint32_t objId, uint32_t mbrNo) const ;
		hzEcode	GetBool		(bool& bValue, uint32_t objId, uint32_t mbrNo) const ;
		hzEcode	GetObject	(bool& bValue, uint32_t objId) const ;
	} ;

	hzArray<_cache_blk*>	m_ClassCaches ;	//	Control block for each class

	std::ofstream	m_os ;				//	Output stream to data file
	hdbADP*			m_pADP ;			//	Host ADP
	hdbBinRepos*	m_pDfltBinRepos ;	//	Default hdbBinRepos instance (only set if used by and TEXT, TXTDOC or BINARY members)
	hdbBinRepos**	m_Binaries ;		//	Array of hdbBinRepos (Cron or Store, one per member even if not used).
	hdbIndex**		m_Indexes ;			//	Array of indexes (one per member even if not used).

	_cache_blk*		m_pMain ;			//	Main data RAM table (full width needed for all members)

	void	_blank	(void)
	{
		m_pDfltBinRepos = 0 ;
		m_Binaries = 0 ;
		m_Indexes = 0 ;
		m_pMain = 0 ;
		m_bDeletes = 0 ;
		m_eReposInit = HDB_CLASS_INIT_NONE ;
	}

	void	_clear	(void)
	{
		if (m_pMain)	{ m_pMain->Clear() ; delete m_pMain ; }
	}

	void	_initerr	(const hzFuncname& _fn, uint32_t nExpect) ;
	void	_deltaWrite	(void) ;

	//	Prevent copying
	hdbObjCache		(const hdbObjCache&) ;
	hdbObjCache&	operator=	(const hdbObjCache&) ;

public:
	hdbObjCache		(hdbADP& adp)	{ m_pADP = &adp ; _blank(); }
	~hdbObjCache	(void)			{ _clear() ; }

	//	Init sequence - from predefined class, then add indexes then finished.
	hzEcode	InitStart		(const hdbClass* pObjClass, const hzString& reposName, const hzString& workdir) ;
	hzEcode	InitMbrIndex	(const hzString& memberName, bool bUnique) ;
	hzEcode	InitMbrStore	(const hzString& memberName, const hzString& binaryName, bool bStore) ;
	hzEcode	InitDone		(void) ;

	//	Post Init methods
	hzEcode	Open		(void) ;									//	Load in data from backup file (only call once)
	hzEcode	Insert		(uint32_t& objId, const hdbObject& obj) ;	//	Standard insert operation, fails if members defined as unique are duplicated
	hzEcode	Update		(hdbObject& obj, uint32_t objId) ;			//	Standard modify operation, fails only if the stated object id does not exist
	hzEcode	Fetch		(hdbObject& obj, uint32_t objId) ;			//	Loads an object from the cache into the supplied object container
	hzEcode	Select		(hdbIdset& result, const char* exp) ;		//	Select using a SQL-esce criteria
	hzEcode	Delete		(uint32_t objId) ;							//	Standard delete operation, fails only if the stated object id does not exist
	hzEcode	Clear		(void) ;									//	empty the Cache

	//	Obtain info about class of objects being stored
	const hdbMember*	GetMember	(const hzString mname)	{ return m_pClass ? m_pClass->GetMember(mname) : 0 ; }

	//	Identifies a single object by member name and value, fails if no objects are found or if multiple objects are found
	hzEcode	Identify	(uint32_t& objId, uint32_t memNo, const hzAtom& atom) ;
	hzEcode	Identify	(uint32_t& objId, uint32_t memNo, const hzString& value) ;
	hzEcode	Identify	(uint32_t& objId, const hzString& member, const hzString& value) ;

	//	Fetches a single member of an object (note use Fetchbin if you need the binary body)
	hzEcode	Fetchval	(hzAtom& atom, uint32_t memNo, uint32_t objId) ;
	hzEcode	Fetchval	(hzAtom& atom, const hzString& member, uint32_t objId) ;
	hzEcode	Fetchbin	(hzAtom& atom, const hzString& member, uint32_t objId) ;
	hzEcode	Fetchbin	(hzChain& Z, const hzString& member, uint32_t objId) ;
	hzEcode	Fetchlist	(hzVect<uint32_t>& items, const hzString& member, uint32_t objId) ;

	//	General Get funtions
	uint32_t	Count	(void)	{ return m_pMain ? m_pMain->Count() : 0 ; }
	//hdbIniStat	IState	(void)	{ return m_eReposInit ; }
} ;

class	hdbObjStore : public hdbObjRepos
{
	//	Category:	Database
	//
	//	Like hdbObjCache, an hdbObjStore instance once initialized to a particular data class, can then contain multiple objects of that data class. But unlike
	//	hdbObjCache, it holds all its data on the disk. The objective being to store much greater volumes of data, albeit at a price of much slower access.
	//
	//	As such, hdbObjStore cannot take advantage of concepts such as the central string table as the string IDs are only good during runtime. And although the
	//	central string table could be synced to disk in much the same way as the hdbObjCache instances are, the anticipation of higher data volumes renders this
	//	idea impractical. So while data class members of numeric data types could hold values in fixed space as they do in hdbObjCache, members of string based
	//	data types cannot.
	//
	//	So members of data objects that under the hdbObjCache storage regime are held in fixed length space, are under hdbObjStore regime, grouped together as a
	//	single binary object - essentially a JSON, and stored in a hzBinStore. Any binary member values are stored in separate hzBinCron or hzBinStore - just as
	//	they are under hdbObjCache. The encoding used is HadronZoo Delta format.
	//
	//	Note that the object ID issued by the INSERT operation is exactly that issued by the INSERT on the core hzBinStore used to store the non-binary data. It
	//	follows that there must be at least one data class member that is non-binary. This limitation is unlikely to ever be a problem as binary objects should
	//	always be accompained by an ID or some form of description.
	//
	//	Note also that members of objects within a hdbObjStore cannot be accesses directly. The whole object must be loaded into a hdbObject instance before the
	//	member values can be written or read.

	hdbADP*			m_pADP ;			//	Host ADP
	hdbBinRepos*	m_pCore ;			//	Used to store all non-binary values in HadronZoo Delta format.
	hdbBinRepos*	m_pDfltBinRepos ;	//	Default hdbBinRepos instance (only set if used by and TEXT, TXTDOC or BINARY members)
	hdbBinRepos**	m_Binaries ;		//	Array of hdbBinRepos (Cron or Store, one per member even if not used).

	hdbIndex**		m_Indexes ;			//	Array of indexes (one per member even if not used).
	hzString		m_Descpath ;		//	The full path for the description file (will be of same name as class + .desc)
	int32_t			m_Core_fd ;			//	Open file for fixed area
	uint32_t		m_nLast ;			//	Last object id issued
	uint32_t		m_nObjs ;			//	Number of objects held
	char			buf [1024] ;		//	Core read/write buffer

	void	_blank	(void)
	{
		m_pCore = 0 ;
		m_pDfltBinRepos = 0 ;
		m_Binaries = 0 ;

		m_Indexes = 0 ;
		m_nLast = m_nObjs = 0 ;
		m_bDeletes = 0 ;
		m_eReposInit = HDB_CLASS_INIT_NONE ;
	}

	void	_clear	(void)
	{
	}

	void	_initerr	(const hzFuncname& _fn, uint32_t nExpect) ;
	void	_deltaWrite	(void) ;

	hdbObjStore	(const hdbObjStore& op)		{ _blank() ; }
	hdbObjStore&	operator=	(hdbObjStore& op)	{ _blank() ; return *this ; }

public:
	hdbObjStore	(hdbADP& adp)	{ m_pADP = &adp ; _blank() ; }
	~hdbObjStore	(void)	{ _clear() ; }

	//	Init sequence - from predefined class, then add indexes then finished.
	hzEcode	InitStart		(const hdbClass* pObjClass, const hzString& reposName, const hzString& workdir) ;
	hzEcode	InitMbrIndex	(const hzString& memberName, bool bUnique) ;
	hzEcode	InitMbrStore	(const hzString& memberName, const hzString& binaryName, bool bStore) ;
	hzEcode	InitDone		(void) ;

	//	Post Init methods
	hzEcode	Open		(void) ;									//	Open data files
	hzEcode	Close		(void) ;									//	Close data files
	hzEcode	Insert		(uint32_t& objId, const hdbObject& obj) ;	//	Standard insert operation, fails if members defined as unique are duplicated
	hzEcode	Update		(hdbObject& obj, uint32_t objId) ;			//	Standard modify operation, fails only if the stated object id does not exist
	hzEcode	Fetch		(hdbObject& obj, uint32_t objId) ;			//	Loads an object from the cache into the supplied object container
	hzEcode	Select		(hdbIdset& result, const char* exp) ;		//	Select using a SQL-esce criteria
	hzEcode	Delete		(uint32_t objId) ;							//	Standard delete operation, fails only if the stated object id does not exist
	hzEcode	Clear		(void) ;									//	empty the Cache

	//	General Get funtions
	uint32_t	Count	(void)	{ return m_nObjs ; }
	//hdbIniStat	IState	(void)	{ return m_eReposInit ; }

	//void	Desc	(hzChain& Z, uint32_t nIndent) const ;
} ;

/*
**	SECTION 4:	Indexation
*/ 

#define	ISAM_CACHE_OFF		0	//	Don't cache
#define	ISAM_CACHE_MIN		1	//	Cache only indicator keys in lower blocks
#define	ISAM_CACHE_IDX		2	//	Cache lower blocks (all higher block indicators)
#define	ISAM_CACHE_ALL		3	//	Cache all blocks

enum	hdbIdxtype
{
	//	Category:	Index
	//
	//	This is returned by the Whatami functions of the hdbIndex class family

	HZINDEX_NULL,	//	No index
	HZINDEX_ENUM,	//	Index is hdbIndexEnum
	HZINDEX_UKEY,	//	Index is hdbIndexUkey
	HZINDEX_TEXT,	//	Index is hdbIndexText
} ;

enum	hzSqlOp
{
	//	Category:	Index
	//
	//	Universal SQL operators (Applies to all member types)

	HZSQL_EQUAL,		//	Member equal to an operand

	//	Arithmetic SQL operators (Applies to dates and numeric members)
	HZSQL_LT,			//	Member less than operand
	HZSQL_GT,			//	Member greater than operand
	HZSQL_LTEQ,			//	Member less than or equal to operand
	HZSQL_GTEQ,			//	Member greater than or equal to operand
	HZSQL_BETWEEN,		//	Member is greater than lower operand, less than higher operand
	HZSQL_RANGE,		//	Member is >= lower operand, <= higher operand

	//	String SQL operators
	HZSQL_CONTAINS,		//	Member contains operand
} ;

class	hdbIsamfile
{
	//	Category:	Index
	//
	//	hdbIsamfile is a memory assisted disk based ordered collection class, designed to store large volumes of data either as a 1:1 or 1:many key-object map or as a key-only set.
	//	Both keys and objects are strings and are limited to 256 bytes. The collection is ordered by lexical value. It is anticipated that in most cases, both keys and objects will
	//	be small single values such as domain names. However, the strings can also be serialized objects. Epistula for example, uses a hdbIsamfile to store short form messages.
	//
	//	The keys or key-object pairs are held within a single large data file, in logical data blocks formed of a variable number of disk blocks. The index is a memory resident map
	//	which maps the LOWEST key from from each logical block to the logical block address, this being the position of its first disk block in the data file. The index is rendered
	//	persistent by means of a separate index file, which is operated on an always-append basis. The index file is completely read in during initialization.
	//
	//	With an SSD, there is no concern regarding read operations but write operations must be minimized to conserve the life of the device. With a standard hard disk all I/O is a
	//	performance issue. Either way, write operations in particular must be as few as possible. Clearly retrieval of an item (key or key-object pair), will read at least one disk
	//	block unless some form of cache is deployed and the item of interest happens to be in it. It is also clear that until an inserted item is committed to disk, it can be lost
	//	in the event of a program crash. However as files are buffered, not all write operations are 'hard'. Data is only written to a disk block when the buffer pointer equates to
	//	a block boundary OR a seek operation redeploys the buffer to another block, necessitating the saving of the current buffer content to the current disk block. If all inserts
	//	were written to the end of the data file, as the items are small most would not result in a hard write. As the whole point of an ISAM is that items are in key order in the
	//	data file, the advantage of file buffering is lost.
	//
	//	To overcome this difficulty, hdbIsamfile has an optional auxillary memory resident map for pending items which is backed up to file that is always appeneded. This auxillary
	//	will map items to the logical block addresses where they would be inserted. Then at a suitable point, the pending items are which directly maps keys  ....
	//

	hzMapM<hzString,uint32_t>	m_Index ;	//	Operaional map

	std::ofstream	m_WrI ;					//	Index file output stream for Insert/Update
	std::ofstream	m_WrD ;					//	Data file output stream for Insert/Update
	std::ifstream	m_RdD ;					//	Data file input stream for Fetch

	hzString	m_FileData ;				//	Name of row data file
	hzString	m_FileIndx ;				//	Name of index file (addresses and sizes of rows)
	hzString	m_Workdir ;					//	Working directory
	hzString	m_Name ;					//	Basename
	uint32_t	m_nElements ;				//	Total number of elements
	uint32_t	m_nBlocks ;					//	Total number of logical data blocks
	uint16_t	m_nKeyLimit ;				//	Max size of key (max 256 bytes)
	uint16_t	m_nObjLimit ;				//	Max size of object (max 256 bytes)
	uint16_t	m_nBlkSize ;				//	Logical data block size
	uint16_t	m_nInitState ;				//	Initialization state
	char		m_Buf[HZ_BLOCKSIZE] ;		//	Operational buffer

public:
	hzChain		m_Error ;					//	Error report
	hzEcode		m_Cond ;					//	Error condition

	hdbIsamfile		(void) ;
	~hdbIsamfile	(void) ;

	hzEcode	Init	(hdbADP& adp, const hzString& workdir, const hzString& name, uint32_t keySize, uint32_t objSize, uint32_t blkSize=4) ;
	hzEcode	Open	(void) ;
	hzEcode	Close	(void) ;

	hzEcode	Insert	(const hzString& key, const hzString& obj) ;
	bool	Exists	(const hzString& key) ;
	hzEcode	Fetch	(hzArray<hzPair>& result, const hzString& keyA, const hzString& keyB) ;
	hzEcode	Delete	(const hzString& key) ;
} ;

class	hdbIndex
{
	//	Category:	Index
	//
	//	The hdbIndex pure virtual base class exists only to unify the different type of indexes.

	union	_idx_set
	{
		hzMapS	<uint32_t,hdbIdset>*	m_Maps ;	//	For Enums, 16-bit and 8-bit values only. These values have a narrow range so each value is likely to occur in many objects.
													//	Hence each value is represented by a bitmap to hold the object id of objects that hold the value.

		hzMapS	<hzString,hdbIdset>*	m_Keys ;	//	Free text index. Map of words to list of objects (records) containing the words
		hzMapS	<hzString,uint32_t>*	pStr ;		//	String index (not currently used)
		hzMapS	<uint64_t,uint32_t>*	pLu ;		//	64-but index
		hzMapS	<uint32_t,uint32_t>*	pSu ;		//	32-bit index
	} ;

protected:
	//const hdbObjRepos*	m_pRepos ;		//	Repository
	//const hdbMember*		m_pMbr ;		//	Class member

	hzString	m_Name ;	//	Name of index (usually that of the applicable class member)

	hdbIndex()	{}

public:
	virtual	~hdbIndex	(void)	{}

	virtual	hdbIdxtype	Whatami		(void) = 0 ;
} ;

class	hdbIndexEnum : public hdbIndex
{
	//	Category:	Index
	//
	//	hdbIndexEnum applies specifically to class members of BASETYPE_ENUM with maximum posible populations of either 255 or 65,355 values, with or without the
	//	option of multiple selected values.
	//
	//	These class members usually relate to a set of options availed as a HTML selector, a radio button set or a set of check-boxes in a HTML form. Given this
	//	general scenario, cases where the lower maximum population of 255 is exceeded are rare and cases where this is significantly exceeded very rare. They do
	//	exist however, particularly as a result of more complex HTML structures involving multiple selectors and so have to be considered. A good example would
	//	be 'preferred location' in a globally applicable website.
	//
	//	hdbIndexEnum is currently the only indexation method available for enumerated data class members. It consists of a map between each enumerated value and
	//	a bitmap. So for any given value allowed for the member, there is a set of ascending object numbers in which that member has that value.
	//
	//	Note that where a hdbObjCache is used to store objects, there is little to be gained from storing bitmaps arising from an enumerated index. The bitmaps
	//	are just as easy to populate during the hdbObjCache load process.

	hzMapS	<uint32_t,hdbIdset*>	m_Maps ;	//	Map of keys to lists of objects matching the key (held as bitmaps)

	//	Prevent copy construction
	hdbIndexEnum	(hdbIndexEnum& op)	{}

	//	Prevent direct copies
	hdbIndexEnum&	operator=	(hdbIndexEnum& op)
	{
		return *this ;
	}

public:
	hdbIndexEnum	(void)
	{
	}

	~hdbIndexEnum	(void)
	{
		Halt() ;
	}

	void	Halt	(void) ;
	hzEcode	Insert	(uint32_t nObjectId, const hzAtom& eVal) ;
	hzEcode	Delete	(uint32_t nObjectId, const hzAtom& eVal) ;
	hzEcode	Select	(hdbIdset& Result, const hzAtom& eVal) ;

	//	Diagnostics
	hzEcode	Dump	(const hzString& cpFilename, bool bFull) ;

	//	Get functions
	uint32_t	Count		(void)	{ return m_Maps.Count() ; }
	hdbIdxtype	Whatami		(void)	{ return HZINDEX_ENUM ; }
} ;

class	hdbIndexUkey : public hdbIndex
{
	//	Category:	Index
	//
	//	hdbIndexUkey (unique key index) ensures objects in a repository are unique with respect to the data member to which the index applies. hdbIndexUkey can be
	//	applied to class members of any string like, specific or numeric data type with a maximum population of 1. Indexes are implimented as direct, one to one
	//	mappings between values and object ids. For each value found in the index, there will be a single object in the repository that will have that value for
	//	the applicable data class member. hdbIndexUkey is commonly applied to usernames, email addresses and other inherently unique data such as social security
	//	numbers.
	//
	//	Note that as hdbIndexUkey can be applied to members with either string like or numeric data types, the mapping regime must be able to cope with this. By
	//	means of a union of map pointers, the maping can be between a key of string or a 64-bit or 32-bit number - and the 32-bit object id. 

	union	_ptrset
	{
		hzMapS	<hzString,uint32_t>*	pStr ;	//	String index (not currently used)
		hzMapS	<uint64_t,uint32_t>*	pLu ;	//	64-but index
		hzMapS	<uint32_t,uint32_t>*	pSu ;	//	32-bit index
	} ;

	_ptrset		m_keys ;		//	The key
	hdbBasetype	m_eBasetype ;	//	The type
	bool		m_bInit ;		//	Init state

public:
	hdbIndexUkey		(void)
	{
		m_keys.pStr = 0 ;
		m_eBasetype = BASETYPE_UNDEF ;
		m_bInit = false ;
	}

	~hdbIndexUkey	(void)
	{
	}

	hzEcode	Init	(const hdbObjRepos* pRepos, const hzString& mbrName, hdbBasetype eType) ;
	void	Halt	(void) ;

	hzEcode	Insert	(const hzAtom& A, uint32_t objId) ;
	hzEcode	Delete	(const hzAtom& A) ;
	hzEcode	Select	(uint32_t& objId, const hzAtom& key) ;

	hdbBasetype	Basetype	(void)	{ return m_eBasetype ; }
	hdbIdxtype	Whatami		(void)	{ return HZINDEX_UKEY ; }
} ;

class	hdbIndexText : public hdbIndex
{
	//	Category:	Index
	//
	//	hdbIndexText effects a free text index applicable class members of BASETYPE_TEXT and BASETYPE_TXTDOC only. Free text indexation is particularly profigate
	//	as it has a bitmap for each word occuring at least once in any document. In substantive collections of documents a count of a million unique words or so
	//	is unusual - and that is a lot of bitmaps! It is also not uncommon for the source documents to acheive large populations, leading to lenghty bitmaps.

	hzMapS	<hzString,hdbIdset>	m_Keys ;	//	Map of words to list of objects (records) containing the words

public:
	hzEcode	Init	(const hzString& name, const hzString& opdir, const hzString& backup, uint32_t cacheMode) ;
	hzEcode	Halt	(void) ;
	hzEcode	Insert	(const hzString& Word, uint32_t docId) ;
	hzEcode	Delete	(const hzString& Word, uint32_t docId) ;
	hzEcode	Clear	(void) ;
	hzEcode	Select	(hdbIdset& Result, const hzString& Word) ;
	hzEcode	Eval	(hdbIdset& Result, const hzString& Criteria) ;

	uint32_t	Count	(void)	{ return m_Keys.Count() ; }

	//	Diagnostics
	hzEcode		Export	(const hzString& filepath, bool bFull = true) ;

	hdbIdxtype	Whatami	(void)	{ return HZINDEX_TEXT ; }
} ;

/*
**	External variables for database package
*/

//	Group 1 Datatyeps: C++ Fundamentals
extern	const hdbCpptype*	datatype_DOUBLE ;		//	64 bit floating point value
extern	const hdbCpptype*	datatype_INT64 ;		//	64-bit Signed integer
extern	const hdbCpptype*	datatype_INT32 ;		//	32-bit Signed integer
extern	const hdbCpptype*	datatype_INT16 ;		//	16-bit Signed integer
extern	const hdbCpptype*	datatype_BYTE ;			//	8-bit Signed integer
extern	const hdbCpptype*	datatype_UINT64 ;		//	64-bit Positive integer
extern	const hdbCpptype*	datatype_UINT32 ;		//	32-bit Positive integer
extern	const hdbCpptype*	datatype_UINT16 ;		//	16-bit Positive integer
extern	const hdbCpptype*	datatype_UBYTE ;		//	8-bit Positive integer
extern	const hdbCpptype*	datatype_BOOL ;			//	either true or false

//	Group 2 Datatypes: HadronZoo Defined types (fixed size)
extern	const hdbHzotype*	datatype_DOMAIN ;		//	Internet Domain
extern	const hdbHzotype*	datatype_EMADDR ;		//	Email Address
extern	const hdbHzotype*	datatype_URL ;			//	Universal Resource Locator
extern	const hdbHzotype*	datatype_PHONE ;		//	Phone number (limited aphabet, standard form, likely to be unique to data object)
extern	const hdbHzotype*	datatype_IPADDR ;		//	IP Address
extern	const hdbHzotype*	datatype_TIME ;			//	No of seconds since midnight (4 bytes)
extern	const hdbHzotype*	datatype_SDATE ;		//	No of days since Jan 1st year 0000
extern	const hdbHzotype*	datatype_XDATE ;		//	Full date & time
extern	const hdbHzotype*	datatype_STRING ;		//	Any string, treated as a single value
extern	const hdbHzotype*	datatype_TEXT ;			//	Any string, treated as a series of words, stored on disk, frequent change
extern	const hdbHzotype*	datatype_BINARY ;		//	File assummed to be un-indexable (eg image). Stored on disk, infrequent change.
extern	const hdbHzotype*	datatype_TXTDOC ;		//	Document from which text can be extracted/indexed. Stored on disk, infrequent change.

/*
**	Prototypes
*/

//	hzEcode		InitDatabase	(const hzString& stringsFile = 0) ;
//	hzEcode		ExportADP		(void) ;

#endif	//	hzDatabase_h
