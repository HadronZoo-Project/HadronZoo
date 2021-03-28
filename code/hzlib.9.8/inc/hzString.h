//
//	File:	hzString.h
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

#ifndef hzString_h
#define hzString_h

#include <string.h>

#include "hzErrcode.h"
#include "hzLock.h"

#define HZSTRING_MAXLEN		1048575	//	Max length of string
//#define HZSTRING_MAXLEN		65535	//	Max length of string
#define HZSTRING_OFFSET		8		//	Number of bytes in string control section
#define HZSTRING_FACTOR		9		//	Number of bytes in string control section plus null terminator

class	hzChain ;

class	hzString
{
	//	Category:	Data
	//
	//	hzString is for the creaton and manipulation of general string values.
	//
	//	Implementation and operation of hzString is described in the synopsys, chapter 1.2.

	uint32_t	m_addr ;	//	Internal address of string

	//	Private methods
	int32_t _cmp	(const hzString&) const ;		//	Case sensitive compare
	int32_t _cmpI	(const hzString&) const ;		//	Case insensitive compare
	int32_t _cmp	(const char*) const ;			//	Case sensitive compare
	int32_t _cmpI	(const char*) const ;			//	Case insensitive compare
	int32_t	_cmpF	(const hzString&) const ;		//	Fast compare
	bool	_feq	(const hzString&) const ;		//	Determine equality by fast compare technique

public:
	void	Clear		(void) ;

	//	Assignment operators
	hzString&	operator=	(const hzString& op) ;
	hzString&	operator=	(const hzChain& C) ;
	hzString&	operator=	(const char* pStr) ;

	//	Constructors
	hzString	(void) ;
	hzString	(const char* pStr) ;
	hzString	(const hzString& op) ;

	//	Destructor
	~hzString	(void) ;

	//	Internal use only
	uint32_t	_int_addr	(void) const	{ return m_addr ; }
	void		_int_clr	(void)			{ m_addr = 0 ; }

	//	Get functions
	uint32_t	Length		(void) const ;

	//	Set functions
	hzEcode		SetValue	(const char* cpStr, uint32_t nLen) ;
	hzEcode		SetValue	(const char* cpStr, const char* cpTerm) ;
	hzEcode		SetValue	(const char* cpStr, char termchar) ;
	hzString	SubString	(uint32_t nPosn, uint32_t nLen = -1) const ;

	//	Miscellaneous operations
	hzString&	ToLower			(void) ;
	hzString&	ToUpper			(void) ;
	hzString&	UrlEncode		(bool bResv = false) ;
	hzString&	UrlDecode		(void) ;
	hzString&	Reverse			(void) ;
	hzString&	Truncate		(uint32_t len) ;
	hzString&	TruncateUpto	(const char* testStr) ;
	hzString&	TruncateBeyond	(const char* testStr) ;
	hzString&	Replace			(const char* strA, const char* strB) ;

	hzString&	DelWhiteLead	(void) ;
	hzString&	DelWhiteTrail	(void) ;
	hzString&	TopAndTail		(void) ;

	//	Find first/last instance of a test char in this string (I denotes case insensitive)
	int32_t	First		(const char testChar) const ;		//	Find first intance of test char in this string
	int32_t	FirstI		(const char testChar) const ;		//	Find first intance of test char in this string
	int32_t	Last		(const char testChar) const ;		//	Find last intance of test char in this string
	int32_t	LastI		(const char testChar) const ;		//	Find last intance of test char in this string

	//	Find first/last instance of a test cstr in this string (I denotes case insensitive)
	int32_t	First		(const char* testCstr) const ;		//	Find first intance of test cstr in this string
	int32_t	FirstI		(const char* testCstr) const ;		//	Find first intance of test cstr in this string
	int32_t	Last		(const char* testCstr) const ;		//	Find last intance of test cstr in this string
	int32_t	LastI		(const char* testCstr) const ;		//	Find last intance of test cstr in this string

	//	Find first/last instance of a test string in this string (I denotes case insensitive)
	int32_t	First		(const hzString& testStr) const ;	//	Find first intance of test string in this string
	int32_t	FirstI		(const hzString& testStr) const ;	//	Find first intance of test string in this string
	int32_t	Last		(const hzString& testStr) const ;	//	Find last intance of test string in this string
	int32_t	LastI		(const hzString& testStr) const ;	//	Find last intance of test string in this string

	//	Get functions
	bool	Contains	(const char testChar) const ;
	bool	Contains	(const char* cpNeedle) const ;
	bool	ContainsI	(const char* cpNeedle) const ;
	bool	Equiv		(const char* testStr) const ;
	bool	Contains	(hzString& S) const	{ return Contains(*S) ; }
	bool	ContainsI	(hzString& S) const	{ return ContainsI(*S) ; }

	//	Arithmetic operators (only addition allowed)
	hzString&	operator+=	(const hzString& S) ;
	hzString&	operator+=	(const char* cpStr) ;

	//	Index operator
	const char	operator[]	(uint32_t nIndex) const ;

	//	Equality and test operators
	bool	operator==	(const char* s) const		{ return _cmp(s)==0?true:false ; }
	bool	operator!=	(const char* s) const		{ return _cmp(s)!=0?true:false ; }
	bool	operator<	(const char* s) const		{ return _cmp(s)< 0?true:false ; }
	bool	operator<=	(const char* s) const		{ return _cmp(s)<=0?true:false ; }
	bool	operator>	(const char* s) const		{ return _cmp(s)> 0?true:false ; }
	bool	operator>=	(const char* s) const		{ return _cmp(s)>=0?true:false ; }
	bool	operator==	(const hzString& s) const	{ return _feq(s) ; }
	bool	operator!=	(const hzString& s) const	{ return !_feq(s) ; }
	bool	operator<	(const hzString& s) const	{ return m_addr==s.m_addr ? false : _cmp(*s)< 0?true:false ; }
	bool	operator<=	(const hzString& s) const	{ return m_addr==s.m_addr ? true  : _cmp(*s)<=0?true:false ; }
	bool	operator>	(const hzString& s) const	{ return m_addr==s.m_addr ? false : _cmp(*s)> 0?true:false ; }
	bool	operator>=	(const hzString& s) const	{ return m_addr==s.m_addr ? true  : _cmp(*s)>=0?true:false ; }
	bool	operator!	(void) const				{ return m_addr ? false : true ; }

	//	Case sensitive, case insensitive and fast compare
	int32_t	Compare		(const hzString& s) const	{ return _cmp(s) ; }
	int32_t	CompareI	(const hzString& s) const	{ return _cmpI(s) ; }
	int32_t	CompareF	(const hzString& s) const	{ return _cmpF(s) ; }
	bool	Equiv		(hzString& S) const			{ return Equiv(*S) ; }

	//	Casting operators
	const char*	operator*	(void) const ;
	operator const char*	(void) const ;

	//	Friend operators
	friend	hzString	operator+	(const hzString S, const hzString S2) ;
	friend	hzString	operator+	(const hzString S, const char* s) ;
	friend	hzString	operator+	(const char* s, const hzString S) ;

	//	Stream interface
	friend	std::istream&	operator>>	(std::istream& is, hzString& obj) ;
	friend	std::ostream&	operator<<	(std::ostream& os, const hzString& obj) ;
} ;

/*
**	Class hzPair (name-value pair)
*/

class	hzPair
{
	//	Category:	Data
	//
	//	Name-Value pair
public:
	hzString	name ;			//	fld name
	hzString	value ;			//	text content

	void	Clear	(void)	{ name = value = (char*) 0 ; }	//	Clear name value pair

	//	Set (with const operand)
	void	SetName		(const char* cpName)	{ name = cpName ; }
	void	SetValue	(const char* cpValue)	{ value = cpValue ; }
	void	Name		(const hzString& S)		{ name = S ; }
	void	Value		(const hzString& S)		{ value = S ; }
	void	operator=	(const hzPair& op)		{ name = op.name ; value = op.value ; }
	void	operator=	(const char* cpValue)	{ value = cpValue ; }

	//	Get
	hzString&	Name	(void)	{ return name ; }
	hzString&	Value	(void)	{ return value ; }

	//	Conditional operators
	bool	operator==	(const char* cpStr)	{ return value == cpStr ; }
	bool	operator!=	(const char* cpStr)	{ return value != cpStr ; }
	bool	operator==	(hzString& S)	{ return value == S ; }
	bool	operator!=	(hzString& S)	{ return value != S ; }
} ;

/*
**	Externals
*/

extern	const hzString	_hzGlobal_nullString ;	//	Empty hzString (guarenteed)

/*
**	Prototypes
*/

int32_t	StringCompare	(const hzString& A, const hzString& B) ;	//	Compare two strings
int32_t	StringCompareI	(const hzString& A, const hzString& B) ;	//	Compare two strings, ignore case
int32_t	StringCompareW	(const hzString& A, const hzString& B) ;	//	Compare two strings, apriori whitespace
int32_t	StringCompareF	(const hzString& A, const hzString& B) ;	//	Compare two strings with the fast method

#endif	//	hzString_h
