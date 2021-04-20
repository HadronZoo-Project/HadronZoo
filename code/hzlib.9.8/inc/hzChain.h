//
//	File:	hzChain.h
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

#ifndef hzChain_h
#define hzChain_h

#include <iostream>
#include <fstream>

#include <stdarg.h>

#include "hzString.h"
//#include "hzIpaddr.h"
//#include "hzEmaddr.h"

class	hzEmaddr ;
class	hzIpaddr ;

/*
**	Section 2:	The hzChain class
*/

class	hzChain
{
	//	Category:	Data
	//
	//	hzChain effects an unlimited buffer intended for the assembly of lengthy string values. The class has both append functions and a printf type function. Content is held in a
	//	double linked list of fixed sized blocks rather than a contiguous block of memory. This approach avoids reallocation and copying during the string value build but rules out
	//	direct casting to null terminated character strings. Instead hzChain instances are expected to be iterated from begining to end. An iterator is provided as a sub-class for
	//	this purpose.
	//
	//	The current size of the chain blocks is 1,460 bytes - the same size as an IP packet. This is deliberate as a key use of hzChain is recipt of server requests and assembly of
	//	server responses. hzChain class also acts as a queue. All operations to add content will append the chain. There are no insert operations.
	//
	//	And although the double linked list allows iterators to decrement, in general data is always drawn from the 'front'. In general, hzChains are considered
	//	to hold a single atomic datum. However, by allowing blocks to be deleted from the front, hzChain can be deployed to process data streams. Incomming data
	//	and requests from connected clients and the server responses, are both processed as data streams. Once the iterator processing the incomming data passes
	//	onto the next block, the first block is discarded. Likewise once blocks in the outgoing data have been written to the socket, they are discarded. Doing
	//	this prevents chains becoming ever larger in cases where clients remain connected and make a large number of requests.
	//
	//	Note that while hzChain is good for string assembly, the block size makes it a poor choice for string storage. Once assembled, strings should be stored
	//	using the hzString class.

	struct _chain
	{
		//	hzChain control area

		uint32_t	m_copy ;		//	Copy count
		uint32_t	m_nSize ;		//	Total size in bytes
		void*		m_Begin ;		//	Address of first block in chain
		void*		m_End ;			//	Address of last block in chain
		//hzLockS		m_Lock ;		//	Atomic spinlock
		//uint32_t	m_Resv ;		//	Reserved

	 	_chain	(void) ;
	 	~_chain	(void) ;
	} ;

	_chain*		mx ;	//	Smart pointer to contents

	int32_t		_compare	(const hzChain& op) const ;

public:
	struct	BlkIter
	{
		//	Block iterator used for writing out whole chains to sockets

		void*	m_block ;		//	Current block address

		BlkIter	(void)	{ m_block = 0 ; }

		//	Operators to set iterator position
		BlkIter&	operator=    (const hzChain& I)
		{
			//	Set a block iterator current block pointer to the current block in the chain
			if (I.mx)
				m_block = I.mx->m_Begin ;
			else
				m_block = 0 ;
			return *this ;
		}

		BlkIter&	operator=    (const BlkIter& I)
		{
			//	Set this block iterator to another
			m_block = I.m_block ;
			return *this ;
		}

		//	Advance the block
		BlkIter&	Advance	(void) ;

		//	Advance the block and remove previous
		BlkIter&	Roll	(void) ;

		//	Return block internals, data and data size
		void*		Data	(void) ;
		uint32_t	Size	(void) ;
	} ;

	struct	Iter
	{
		//	Standard bytewise chain iterator

		void*		m_block ;		//	Current block address
		uint32_t	m_nLine ;		//	For tracking line numbers
		uint32_t	m_nCol ;		//	Column number
		uint16_t	m_nOset ;		//	Current offset within block
		char		m_cDefault ;	//	Default char
		char		m_Reserved ;	//	Not used at present

		Iter	(void)
		{
			//	Construct and init the chain iterator
			m_block = 0 ;
			m_nLine = 1 ;
			m_nCol = 0 ;
			m_nOset = 0 ;
			m_cDefault = 0 ;
		}

		//	Read unicode char
		hzEcode	ReadUnicodeChar	(uint32_t& uniVal) ;

		//	Skip whitespace
		Iter&		Skipwhite	(void) ;

		//	Misc functions
		uint32_t	Line	(void)			{ return m_nLine ; }
		uint32_t	Col		(void)			{ return m_nCol ; }
		void		Line	(uint32_t n)	{ m_nLine = n ; }

		char	current		(void) const ;
		bool	eof			(void) const ;

		//	Operators to set iterator position
		Iter&	operator=    (const hzChain& I)
		{
			//	Set this chain iterator to the start of the supplied chain
			if (I.mx)
				m_block = I.mx->m_Begin ;
			else
				m_block = 0 ;

			m_nLine = 1 ;
			m_nCol = 0 ;
			m_nOset = 0 ;

			return *this ;
		}

		Iter&	operator=    (const Iter& I)
		{
			//	Set this chain iterator to another
			m_block = I.m_block ;
			m_nLine = I.m_nLine ;
			m_nOset = I.m_nOset ;
			m_cDefault = I.m_cDefault ;

			return *this ;
		}

		//	Write out block of chars to buffer. Return bytes written out but don't increment iterator (do that in app with the return value)
		uint32_t	Write	(void* pBuf, uint32_t maxBytes) ;

		bool	operator==   (const Iter& I) const	{ return m_block == I.m_block && m_nOset == I.m_nOset ? true : false ; }
		bool	operator!=   (const Iter& I) const	{ return m_block == I.m_block && m_nOset == I.m_nOset ? false : true ; }

		//	Advance (increment without column/line accounting)
		uint32_t	Advance		(uint32_t nInc) ;

		//	Increment and decrement
		Iter&	operator++	(void) ;
		Iter&	operator++	(int) ;
		Iter&	operator--	(void) ;
		Iter&	operator--	(int) ;
		Iter&	operator+=	(uint32_t nInc) ;
		Iter&	operator-=	(uint32_t nDec) ;

		//	Case sensitive compare functions
		bool	Equal		(const char c) const ;
		bool	Equal		(const char* s) const ;
		bool	Equal		(const hzString& S) const	{ return Equal(*S) ; }

		//	Case in-sensitive compare functions
		bool	Equiv		(const char c) const ;
		bool	Equiv		(const char* s) const ;
		bool	Equiv		(const hzString& S) const	{ return Equiv(*S) ; }

		//	Compare operators
		bool	operator==	(const char c) const	{ return Equal(c) ; }
		bool	operator==	(const char* s) const	{ return Equal(s) ; }
		bool	operator==	(hzString& S) const		{ return Equal(S) ; }
		bool	operator==	(Iter& I) const			{ return m_block == I.m_block && m_nOset == I.m_nOset ? true : false ; }
		bool	operator!=	(const char c) const	{ return !Equal(c) ; }
		bool	operator!=	(const char* s) const	{ return !Equal(s) ; }
		bool	operator!=	(hzString& S) const		{ return !Equal(S) ; }
		bool	operator!=	(Iter& I) const			{ return m_block == I.m_block && m_nOset == I.m_nOset ? false : true ; }

		//	Get current or offset char
		char	operator[]	(uint32_t nOset) const ;
		char	operator*	(void) const ;
	} ;

	struct	Mark
	{
		//	Chain marker. Used to denote where a 'sub-chain' starts and ends. Used in document processing in which the whole document is held in a single chain
		//	but where parts of the document must be accessed separately by the application. Use of markers can eliminate the need to copy out parts of documents
		//	to strings. It also solves the albeit rare conundrum in which a document part may exceed the maximum allowed length of a string.
		//
		//	Chain markers are similar to chain iterators in that the position within a chain is given by a block address and offset. However 

		void*		m_block ;		//	Current block address
		uint32_t	m_nLen ;		//	Total length
		uint16_t	m_nOset ;		//	Current offset within block
		uint16_t	m_resv ;		//	Reserved
	} ;

	void	Clear	(void) ;

	//	Constructors/Destructors
	hzChain		(void) ;
	hzChain		(const hzChain& op) ;
	~hzChain	(void) ;

	//	Get hzChain attributes
	uint32_t	Size	(void) const	{ return mx ? mx->m_nSize : 0 ; }
	//uint16_t	GetID	(void) const	{ return mx ? mx->m_Id : 0xffff ; }

	//	Set chain contents
	hzChain&	operator=	(const hzChain& op) ;
	hzChain&	operator=	(const hzString& S) ;
	hzChain&	operator=	(const char* pStr) ;

	//	Test operator
	bool		operator!	(void)	{ return Size() > 0 ? false : true ; }

	//	Append void* data to the chain. Note as there is no null termination the number of bytes to append must be specified.
	uint32_t	Append		(const void* vpBuf, uint32_t nBytes) ;

	//	Append a sub-chain
	uint32_t	AppendSub	(hzChain& Z, uint32_t nStart, uint32_t nBytes) ;

	//	Append single character
	hzEcode		AddByte		(const char c) ;

	//	Append using varargs
	uint32_t	_vainto		(const char*, va_list) ;
	uint32_t	Printf		(const char* ...) ;

	//	Append whole entities by operators
	hzChain&	operator+=	(const char* s) ;
	hzChain&	operator+=	(const hzString& s) ;
	hzChain&	operator+=	(const hzChain& op) ;
	hzChain&	operator+=	(std::ifstream& ifs) ;

	hzChain&	operator<<	(const char* s) ;
	hzChain&	operator<<	(const hzString& s) ;

	hzChain&	operator<<	(const hzEmaddr& e) ;
	hzChain&	operator<<	(const hzIpaddr& i) ;

	hzChain&	operator<<	(uint32_t nValue) ;
	hzChain&	operator<<	(uint64_t nValue) ;
	hzChain&	operator<<	(int32_t nValue) ;
	hzChain&	operator<<	(int64_t nValue) ;
	hzChain&	operator<<	(double nValue) ;
	hzChain&	operator<<	(const hzChain& op) ;
	hzChain&	operator<<	(std::ifstream& is) ;

	//	Compare operators
	bool	operator==	(const hzChain& op) const	{ return _compare(op) == 0 ? true : false ; }
	bool	operator!=	(const hzChain& op) const	{ return _compare(op) != 0 ? true : false ; }
	bool	operator<	(const hzChain& op) const	{ return _compare(op) <  0 ? true : false ; }
	bool	operator<=	(const hzChain& op) const	{ return _compare(op) <= 0 ? true : false ; }
	bool	operator>	(const hzChain& op) const	{ return _compare(op) >  0 ? true : false ; }
	bool	operator>=	(const hzChain& op) const	{ return _compare(op) >= 0 ? true : false ; }

	//	Stream operators
	friend	std::istream&	operator>>	(std::istream& is, hzChain& obj) ;
	friend	std::ostream&	operator<<	(std::ostream& os, const hzChain& obj) ;
} ;

#define chIter	 hzChain::Iter
#define chMark	 hzChain::Mark

#endif	//	hzChain_h
