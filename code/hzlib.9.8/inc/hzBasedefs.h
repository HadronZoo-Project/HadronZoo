//
//	File:	hzBasedefs.h
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

#ifndef hzBasedefs_h
#define hzBasedefs_h

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//	This header sets out 'basic definitions' in order to:-
//
//		1) Standardize common data type names.
//
//		2) Define sizes for common objects such as disk blocks.
//
//		3) Define functions to provide a top level wrapper for memory allocation/release so that overrides for global operator new/delete can be switched on if required.
//
//		4) Define an atomic value union.
//
//		5) Define various receptacle classes to recieve text output from translator fuctions. Included among receptacles is one for IP packets.
//
//		6) Define the hzMeminfo or memory information class used in memory usage reposrting.
//

/*
**	Unify common types
*/

#ifndef uchar_defined
#define uchar_defined
typedef	unsigned char	uchar ;
#endif

//	Define global to be empty string allowing its use alongside 'static' for asthetic purposes
#define global

//	Shorthand keywords
#define _r	register
#define	_m	mutable

//	HadronZoo sizes
#ifdef BITS32
#define HZ_ALIGNMENT	4
#else
#define HZ_ALIGNMENT	8
#endif

#define HZ_BLOCKSIZE	4096	//	Assumed size of disk block size
#define HZ_MAXPACKET	1460	//	Assumed size of IP packet
#define HZ_MAXFNAMELEN	 256	//	Max length of filename
#define HZ_MAXPATHLEN	 256	//	Max length of filepath

#define memalign(len)	len>0 ? (((len/HZ_ALIGNMENT)+1)*HZ_ALIGNMENT) : 0

/*
**	Debugging
*/

#define	HZ_DEBUG_CTMPLS		0x01	//	Switch on debugging within collection class templates
#define	HZ_DEBUG_MAILER		0x02	//	Switch on debugging within email functions
#define	HZ_DEBUG_DNS		0x04	//	Switch on debugging within DNS functions
#define	HZ_DEBUG_CLIENT		0x08	//	Switch on debugging within Internet client functions
#define	HZ_DEBUG_SERVER		0x10	//	Switch on debugging within Internet server functions

/*
**	Value manipulation
*/

union	_atomval
{
	//	Category:	Data
	//
	//	Union to store values for various data types of up to 64 bits in size.

	const void*	m_pVoid ;		//	Used to point to a string or text entity, a hdbEnum, hzEmaddr, hzUrl or hzChain instance
	double		m_Double ;		//	Double precision numbers
	int64_t		m_sInt64 ;		//	Signed 64 bit integer
	uint64_t	m_uInt64 ;		//	Unsigned 64 bit integer
	int32_t		m_sInt32 ;		//	Signed 32 bit integer
	uint32_t	m_uInt32 ;		//	Unsigned 32 bit integer
	int16_t		m_sInt16 ;		//	Signed 16 bit integer
	uint16_t	m_uInt16 ;		//	Unsigned 16 bit integer
	char		m_sByte ;		//	Signed 8 bit integer
	uchar		m_uByte ;		//	Unsigned 8 bit integer
	bool		m_Bool ;		//	True or false

	_atomval	(void)	{ m_uInt64 = 0 ; }
} ;

/*
**	Unique System ID
*/

class	hzSysID
{
	//	Category:	Data
	//
	//	A HadronZoo system id is just a 64-bit unique id, created as a class to protect it. The id value can be set, read and compared but not otherwise operated upon.

	uint64_t	m_Value ;	//	The id

public:
	hzSysID	(void)	{ m_Value = 0 ; }

	hzSysID&	operator=	(const hzSysID& op)		{ m_Value = op.m_Value ; }
	hzSysID&	operator=	(uint64_t val)			{ m_Value = val ; }

	bool	operator==	(const hzSysID& op) const	{ return m_Value == op.m_Value ? true:false ; }
	bool	operator<	(const hzSysID& op) const	{ return m_Value < op.m_Value ? true:false ; }
	bool	operator>	(const hzSysID& op) const	{ return m_Value > op.m_Value ? true:false ; }
	bool	operator!	(void) const				{ return m_Value ? false:true ; }

	operator uint64_t	(void) const	{ return m_Value ; }
} ;

/*
**	Recepticles
*/

//	Class Group:	Data Receptacles
//
//	Where classes have a fixed or limited length textual representation (text value), one way of obtaining this is have a member function populate a buffer or a
//	string. The former is effecient but carries the obvious risk of an oversight leading to buffer overflow. The later is safer but ineffecient. Such a function
//	has to create a string, populate the buffer, set the string value to the buffer and then return it. And the calling function has to destruct a string rather
//	than just a buffer. The string construction and destruction processes involve locking in a multithreaded environment which only adds to the cost.
//
//	Recepticles are the means by which the former method is made safer. A recepticle is a fixed size buffer defined as a class. The member function that exports
//	the text value is written so as to expect a reference to a recepticle rather than a char* pointer. All the recepticles are named 'hzRecep' but postfixed
//	with the buffer size.

struct	hzRecep04	{ char	m_buf[4] ; } ;
struct	hzRecep08	{ char	m_buf[8] ; } ;
struct	hzRecep16	{ char	m_buf[16] ; } ;
struct	hzRecep24	{ char	m_buf[24] ; } ;
struct	hzRecep32	{ char	m_buf[32] ; } ;
struct	hzRecep48	{ char	m_buf[48] ; } ;

class	hzPacket
{
	//	Category:	Internet
	//
	//	This only comprises a buffer sufficient to hold an IP packet. It is fashioned as a class for the avoidance of error. Passing a pointer to a buffer is
	//	asking for trouble. Passing a reference of one of these isn't.
	//
	//	Each hzPacket has a buffer capable of holding the data in a single IP packet, plus a pointer to the next instance. hzPacket is intended to form links in a
	//	rolling chain, in effect a queue. Two such rolling chains are operated by the hzTcpConnex class, an instance of which exists for each connected client.

public:
	hzPacket*	next ;					//	Next in series

	uint32_t	m_msgId ;				//	Message ID
	uint32_t	m_seq ;					//	Position of packet in data stream
	uint32_t	m_size ;				//	No of bytes in this packet
	char		m_data[HZ_MAXPACKET] ;	//	Buffer for writing

	hzPacket (void)	{ next = 0 ; m_msgId = m_seq = m_size = 0 ; }
} ;

/*
**	Memory regime for overide of operator new etc
**
**	Here we declare the memory allocation function _hz_mem_allocate and the memory deletion function _hz_mem_free and then redefine operator new, new[], delete
**	and delete[] as inlines that call these functions. 
**
**	The functions hz_mem_allocate and hz_mem_release are defined in hzMemory.cpp
*/

void*	hz_mem_allocate	(uint32_t size) ;
void	hz_mem_release	(void* ptr) ;

inline	void*	operator new		(size_t size)	{ return hz_mem_allocate((uint32_t) size) ; }
inline	void*	operator new[]		(size_t size)	{ return hz_mem_allocate((uint32_t) size) ; }

inline	void	operator delete		(void* ptr)		{ hz_mem_release(ptr) ; }
inline	void	operator delete[]	(void* ptr)		{ hz_mem_release(ptr) ; }

/*
**	Memory use reporting
*/

class	hzMeminfo
{
	//	Category:	Diagnostics
	//
	//	In order to keep track of the memory use of a HadronZoo based application, the key parameters are held in a single class for convienence.

public:
	//	Micro chains
	uint32_t	m_numMCH ;				//	Total number of hzMCH instances (with or without data)
	uint32_t	m_numMCH_D ;			//	Number of hzMCH instances with data

	//	Standard chains
	uint32_t	m_numChain ;			//	Total number of hzChain instances (with or without data container)
	uint32_t	m_numChainDC ;			//	Number of hzChain instances with data container
	uint32_t	m_numChainBlks ;		//	Total number of hzChain blocks
	uint32_t	m_numChainBF ;			//	Number of hzChain blocks in free list

	//	Strings
	uint32_t	m_strSm_u[32] ;			//	Small string spaces (8 to 256 bytes), in use
	uint32_t	m_strSm_f[32] ;			//	Small string spaces (8 to 256 bytes), free
	uint32_t	m_numSblks ;			//	Number of string superblocks
	uint32_t	m_numStrOver ;			//	Number of hzString instances (oversize)
	uint32_t	m_ramStrOver ;			//	Total memory allocated to oversized hzStrings
	uint32_t	m_numStrings ;			//	Number of hzString instances

	//	Operator new/delete override
	uint32_t	m_numMemblkA ;			//	Number of type A memblk instances in RAM (for size A 16 byte objects)
	uint32_t	m_numMemblkB ;			//	Number of type B memblk instances in RAM (for size B 24 byte objects)
	uint32_t	m_numMemblkC ;			//	Number of type C memblk instances in RAM (for size C 32 byte objects)
	uint32_t	m_numMemblkD ;			//	Number of type D memblk instances in RAM (for size D 48 byte objects)
	uint32_t	m_numMemblkE ;			//	Number of type E memblk instances in RAM (for size E 64 byte objects)

	//	Common objects
	uint32_t	m_numIsams ;			//	Number of ISAM collections
	uint32_t	m_numIsamIndx ;			//	Number of ISAM index blocks
	uint32_t	m_numIsamData ;			//	Number of ISAM data blocks
	uint32_t	m_numArrays ;			//	Number of hzArray instances
	uint32_t	m_numArrayDA ;			//	Number of hzArray instances with data area
	uint32_t	m_numLists ;			//	Number of hzList instances
	uint32_t	m_numListDC ;			//	Number of hzList instances with data area
	uint32_t	m_numQues ;				//	Number of hzQue instances
	uint32_t	m_numStacks ;			//	Number of hzStack instances
	uint32_t	m_numSmaps ;			//	Number of hzMapS instances
	uint32_t	m_numMmaps ;			//	Number of hzMapM instances
	uint32_t	m_numSpmaps ;			//	Number of hzLookup instances
	uint32_t	m_numSets ;				//	Number of hzSet instances
	uint32_t	m_numVectors ;			//	Number of hzVect instances
	uint32_t	m_numBitmaps ;			//	Number of hzBitmap instances
	uint32_t	m_numBitmapSB ;			//	Number of hzBitmap 'segment block' instances
	uint32_t	m_numMCHB ;				//	Number of 'micro chain' blocks.
	uint32_t	m_numDochtm ;			//	Number of hzDocHtm instances
	uint32_t	m_numDocxml ;			//	Number of hzDocXml instances
	uint32_t	m_numBincron ;			//	Number of hdbBinCron instances
	uint32_t	m_numBinstore ;			//	Number of hdbBinStore instances
} ;

/*
**	Externals
*/

extern	hzMeminfo	_hzGlobal_Memstats ;	//	Memory statistics
extern	uint32_t	_hzGlobal_Debug ;		//	Global debug flags
extern	bool		_hzGlobal_XM ;			//	Global new/delete overload switch

#endif	//	hzBasedefs_h
