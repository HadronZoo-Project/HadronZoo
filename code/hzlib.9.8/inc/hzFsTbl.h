//
//	File:	hzFsTbl.h
//
//	Legal Notice: This file is part of the HadronZoo C++ Class Library.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	The HadronZoo C++ Class Library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
//	as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
//	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with the HadronZoo C++ Class Library. If not, see
//	http://www.gnu.org/licenses.
//

#ifndef hzFsTbl_h
#define hzFsTbl_h

#include "hzBasedefs.h"
#include "hzCtmpls.h"
#include "hzString.h"

#define	HZ_STRTBL_BLKSZE	65534	//	Space in a string table block (64K -1)

class	hzFsTbl
{
	//	Category:	Data
	//
	//	The hzFsTbl (unique string table) class, manages large numbers of cstr (null terminated strings), held in a series of 64K blocks. In the envisiaged
	//	operating environment, as part of the HadronZoo Database Suite, the strings are commonly whole words, hence the name.
	//
	//	hzFsTbl follows the 'always append never delete' maxim. As new strings are added, they are always placed, along with the null teminator, at the end
	//	of the data space in the last block in the series. When the last block has insufficient space to store the new string, a new block is allocated and this
	//	becomes the last block in the series. Although this is potentially wasteful, because the strings are generally small the wastage will be small. Current
	//	policy therefore, is not to complicate processing by attempting to find space on other blocks.
	//
	//	There may be a maximum of 65,536 blocks allowing the total capacity of the dictionary to approach 4Gb (more than sufficient for most applications). More
	//	importantly, the strings have 32 bit addresses which in a 64 bit environment (these days the entry level default), is half the size of a pointer. Of the
	//	32 bits, the most significant 16 identify the block while the least significant 16 give the offset to the start of the string within the block.
	//
	//	Strings managed by hzFsTbl may be passed or soft copied in the application by passing or copying the string address. Since the string refered to by
	//	the address is constant, there is no notion of a smart pointer with a copy count as there is with hzString. This is particularly useful in multithreaded
	//	applications as only the introduction of new strings needs to be made threadsafe. 
	//
	//	As the strings are never deleted, or for that matter modified during runtime, the strings are stored in order of incidence - the order in which they are
	//	encountered in the application. Clearly the order of incidence is useless for the purposes of lookup by value. To recify this, hzFsTbl maintains an
	//	instance of hzVect<uint32_t> to hold the string addresses in order of string value. Lookup by value is then only a matter of binary choping this vector
	//	and comparing the string found with the test string with each chop.
	//
	//	A hzFsTbl instance may be optionally configured to back up its contents to a file. To operate the dictionary in this mode, call the member function
	//	Import() directly after construction and before any strings have been added.

	class	_fxstr_blk
	{
		//	String table block

	public:
		uint16_t	m_Usage ;					//	Bytes used (next free position)
		char		m_Space[HZ_STRTBL_BLKSZE] ;	//	String space

		_fxstr_blk	()	{ m_Space[0] = 0 ; m_Usage = 0 ; }
	} ;

	hzVect<_fxstr_blk*>		m_Blocks ;			//	The 64K string storage blocks
	hzVect<uint32_t>		m_Strings ;			//	The string addresses in order of value

	_fxstr_blk*		m_pLastBlock ;				//	Block to put new strings
	std::ofstream	m_Outstream ;				//	Output stream if file backup applies
	hzString		m_Filename ;				//	Backup filename set by Import()
	uint32_t		m_nInserts ;				//	Number of insert calls

	//	Load backup file on startup
	hzEcode		_loadStrings	(const hzString& backupFilename) ;

	//	Prevent copies
	hzFsTbl	(const hzFsTbl&) ;
	hzFsTbl&	operator=	(const hzFsTbl&) ;

public:
	hzFsTbl		(void) ;	//	Default constructor: Call to obtain a standalone dictionary (for XML/HTML document loading)
	~hzFsTbl	(void) ;	//	Default destructor (all cases)

	//	Singleton constructor (sets _hzGlobal_StringTable)
	static	hzFsTbl*	StartStrings	(const hzString& backupFilename = 0) ;
	static	hzFsTbl*	StartDomains	(const hzString& backupFilename = 0) ;
	static	hzFsTbl*	StartEmaddrs	(const hzString& backupFilename = 0) ;

	void	Clear() ;

	//	Insert hzString if the string does not exist or increment usage value if it does. Returns string number but does not indicate if string is new.
	uint32_t	Insert	(const char* str) ;
	uint32_t	Insert	(const hzString& S)	{ return Insert(*S) ; }

	//	Convert string number to null terminated string
	const char*	Xlate	(uint32_t strNo) const ;

	//	Convert hzString to string number. A zero return indicates string not found.
	uint32_t	operator[]	(const hzString& S) ;
	uint32_t	Locate	(const char* str) const ;

	//	Reporting
	const char*	GetStr	(uint32_t posn) ;
	uint32_t	Blocks	(void) const	{ return m_Blocks.Count() ; }
	uint32_t	Count	(void) const	{ return m_Strings.Count() ; }

	//	Export string to file other than the designated backup
	hzEcode		Export	(const hzString& fullpath) ;
} ;

class	hzNumPair
{
	//	Category:	Data
	//
	//	Pair of string numbers for hzFsTbl use outside _hzGlobal_StringTable

public:
	uint32_t	m_snName ;		//	Name string number
	uint32_t	m_snValue ;		//	Value string number
} ;

class	hzFixStr
{
	//	Category:	Data
	//
	//	hzFixStr is a fixed string entered in _hzGlobal_StringTable and no other hzFsTbl instance.

	uint32_t	m_strno ;	//	The string number

public:
	hzFixStr	(void)
	{
		m_strno = 0 ;
	}

	hzFixStr	(const hzFixStr& op)
	{
		m_strno = op.m_strno ;
	}

	~hzFixStr	(void)	{}

	//	Assignment
	hzFixStr	operator=	(const char* cstr) ;
	hzFixStr	operator=	(const hzString& str)	{ return operator=(*str) ; }
	hzFixStr	operator=	(const hzFixStr& op)	{ m_strno = op.m_strno ; }

	//	To extract actual string value
	const char*	operator*	(void) const ;

	//	Equality and compare operators
	bool	operator==	(const hzFixStr& op)	{ return m_strno == op.m_strno ? true : false ; }
	bool	operator!=	(const hzFixStr& op)	{ return m_strno == op.m_strno ? false : true ; }
	bool	operator<	(const hzFixStr& op) ;
	bool	operator<=	(const hzFixStr& op) ;
	bool	operator>	(const hzFixStr& op) ;
	bool	operator>=	(const hzFixStr& op) ;

	//	Casting operator
	operator const char*	(void) const	{ return operator*() ; }
} ;

class	hzFixPair
{
	//	Category:	Data
	//
	//	A pair of string table references to act as name/value pair

public:
	hzFixStr	name ;		//	The name part
	hzFixStr	value ;		//	The value part
} ;

class	hzFxDomain
{
	//	Category:	Data
	//
	//	hzFxDomain is a fixed string entered in _hzGlobal_FST_Domains and no other hzFsTbl instance.

	uint32_t	m_strno ;	//	The string number

public:
	hzFxDomain	(void)
	{
		m_strno = 0 ;
	}

	hzFxDomain	(const hzFxDomain& op)
	{
		m_strno = op.m_strno ;
	}

	~hzFxDomain	(void)	{}

	//	Assignment
	hzFxDomain	operator=	(const char* cstr) ;
	hzFxDomain	operator=	(const hzString& str)	{ return operator=(*str) ; }
	hzFxDomain	operator=	(const hzFxDomain& op)	{ m_strno = op.m_strno ; }

	//	To extract actual string value
	const char*	operator*	(void) const ;

	//	Equality and compare operators
	bool	operator==	(const hzFxDomain& op)	{ return m_strno == op.m_strno ? true : false ; }
	bool	operator!=	(const hzFxDomain& op)	{ return m_strno == op.m_strno ? false : true ; }
	bool	operator<	(const hzFxDomain& op) ;
	bool	operator<=	(const hzFxDomain& op) ;
	bool	operator>	(const hzFxDomain& op) ;
	bool	operator>=	(const hzFxDomain& op) ;

	//	Casting operator
	operator const char*	(void) const	{ return operator*() ; }
} ;

class	hzFxEmaddr
{
	//	Category:	Data
	//
	//	hzFxEmaddr is a fixed string entered in _hzGlobal_FST_Emaddr and no other hzFsTbl instance.

	uint32_t	m_strno ;	//	The string number

public:
	hzFxEmaddr	(void)
	{
		m_strno = 0 ;
	}

	hzFxEmaddr	(const hzFxEmaddr& op)
	{
		m_strno = op.m_strno ;
	}

	~hzFxEmaddr	(void)	{}

	//	Assignment
	hzFxEmaddr	operator=	(const char* cstr) ;
	hzFxEmaddr	operator=	(const hzString& str)	{ return operator=(*str) ; }
	hzFxEmaddr	operator=	(const hzFxEmaddr& op)	{ m_strno = op.m_strno ; }

	//	To extract actual string value
	const char*	operator*	(void) const ;

	//	Equality and compare operators
	bool	operator==	(const hzFxEmaddr& op)	{ return m_strno == op.m_strno ? true : false ; }
	bool	operator!=	(const hzFxEmaddr& op)	{ return m_strno == op.m_strno ? false : true ; }
	bool	operator<	(const hzFxEmaddr& op) ;
	bool	operator<=	(const hzFxEmaddr& op) ;
	bool	operator>	(const hzFxEmaddr& op) ;
	bool	operator>=	(const hzFxEmaddr& op) ;

	//	Casting operator
	operator const char*	(void) const	{ return operator*() ; }
} ;

extern	hzFsTbl*	_hzGlobal_StringTable ;
extern	hzFsTbl*	_hzGlobal_FST_Domain ;
extern	hzFsTbl*	_hzGlobal_FST_Emaddr ;

#endif	//	hzFsTbl_h
