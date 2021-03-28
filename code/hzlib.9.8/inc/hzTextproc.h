//
//	File:	hzTextproc.h
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
//	Generalized text processing package.
//

#ifndef hzTextproc_h
#define hzTextproc_h

#include "hzBasedefs.h"
#include "hzString.h"
#include "hzChain.h"
#include "hzEmaddr.h"
#include "hzUrl.h"
#include "hzDate.h"
#include "hzErrcode.h"
#include "hzDatabase.h"
#include "hzCtmpls.h"

//	Math macros
#define minval(a,b)		(a < b ? a : b)
#define maxval(a,b)		(a > b ? a : b)
#define roundto(a,b)	(a%b==0? a:((a/b)+b)*b)

#define conv2upper(c)	(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c)
#define conv2lower(c)	(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c)

/*
**	Token Processing Prototypes
*/

//	Is object an integer (takes signed target operand)
bool		IsInteger		(int64_t& nValue, const char* cpStr) ;
bool		IsInteger		(int32_t& nValue, const char* cpStr) ;
bool		IsInteger		(int16_t& nValue, const char* cpStr) ;
bool		IsInteger		(char&  nValue, const char* cpStr) ;

//	Is object a positive integer (unsigned)
bool		IsPosint		(uint64_t& nValue, const char* cpStr) ;
bool		IsPosint		(uint32_t& nValue, const char* cpStr) ;
bool		IsPosint		(uint16_t& nValue, const char* cpStr) ;
bool		IsPosint		(uchar&  nValue, const char* cpStr) ;

//	Is object a hexadecimal integer (unsigned)
bool		IsHexnum		(uint64_t& nValue, const char* cpStr) ;
bool		IsHexnum		(uint32_t& nValue, const char* cpStr) ;
bool		IsHexnum		(uint16_t&	nValue, const char* cpStr) ;
bool		IsHexnum		(uchar& nValue, const char* cpStr) ;

//	Std number notation
bool		IsDouble		(double& nValue, const char* cpStr) ;

//	Tests for addresses and other entities
bool		IsIPAddr		(uint32_t& nIpValue, const char* cpIpAddr) ;
bool		IsIPAddr		(const char* cpIpAddr) ;
bool		AtIpaddr		(hzIpaddr& ipa, uint32_t& nLen, hzChain::Iter& ci) ;

//	Test for Domain
bool		IsDomain		(const char* cpStr) ;
bool		AtDomain		(hzEmaddr& ema, uint32_t& nLen, hzChain::Iter& ci) ;

//	Test for Email address
bool		IsEmaddr		(const char* cpStr) ;
bool		AtEmaddr		(hzEmaddr& ema, uint32_t& nLen, hzChain::Iter& ci) ;

//	Test for URL
bool		IsUrl			(const char* cpStr) ;
bool		AtUrl			(hzUrl& url, uint32_t& nLen, hzChain::Iter& ci) ;

//	Is token a numeric entity (&#x[hex number]; or &#[dec number];)
bool		IsEntity		(uint32_t& nEnt, uint32_t& nLen, const char* str) ;
bool		AtEntity		(uint32_t& uVal, uint32_t& nLen, hzChain::Iter& z) ;

//	Miscellaneous
bool		IsAllDigits		(const char* cpStr) ;
bool		IsAlphaNum		(const char* cpTok) ;
bool		AtUnicodeSeq	(unsigned int& seq, unsigned int& nLen, hzChain::Iter& ci) ;

//	Token testing
uint32_t	TestAlphanum	(hzString& word, hzChain::Iter& ci) ;

/*
**	General Prototypes
*/

hzEcode		SplitCSV		(hzVect<hzString>& ar, const char* csvline, char cDelim = 0) ;
hzEcode		SplitCSV		(char** ar, char* csvline, uint32_t arSize, char cDelim = 0) ;
void		SplitChain		(hzVect<hzString>& ar, hzChain& input, char cDelim = 0) ;
void		SplitStrOnChar	(hzVect<hzString>& ar, hzString& input, char cDelim = 0) ;
hzEcode		SplitCstrOnChar	(hzVect<hzString>& ar, const char* csvline, char cDelim = 0) ;
hzEcode		SplitCstrOnCstr	(hzVect<hzString>& ar, const char* csvline, const char* delimstr) ;

uint32_t	CstrIncidence	(const char* cpStr, char c) ;
uint32_t	CstrCopy		(char* cpDest, const char* cpSource, uint32_t nMaxlen = 0) ;
uint32_t	CstrOverwrite	(char* cpDest, const char* cpSource, uint32_t nMaxlen = 0) ;
int32_t		CstrCompare		(const char* pA, const char*pB, uint32_t nMaxlen = 0) ;
int32_t		CstrCompareI	(const char* pA, const char*pB, uint32_t nMaxlen = 0) ;
int32_t		CstrCompareW	(const char* pA, const char* pB) ;
bool		CstrContains	(const char* cpHaystack, const char* cpNeedle) ;
bool		CstrContainsI	(const char* cpHaystack, const char* cpNeedle) ;
int32_t		CstrFirst		(const char* cpHaystack, const char* cpNeedle) ;
int32_t		CstrFirstI		(const char* cpHaystack, const char* cpNeedle) ;
int32_t		CstrLast		(const char* cpHaystack, const char* cpNeedle) ;
int32_t		CstrLastI		(const char* cpHaystack, const char* cpNeedle) ;
int32_t		CstrFirst		(const char* cpString, char testChar) ;
int32_t		CstrFirstI		(const char* cpString, char testChar) ;
int32_t		CstrLast		(const char* cpString, char testChar) ;
int32_t		CstrLastI		(const char* cpString, char testChar) ;

//	UNIX/Windows conversion
hzEcode		DosifyChain		(hzChain& Z) ;
hzEcode		DosifyFile		(const hzString& tgt, const hzString& src) ;

//	Text substitution
uint32_t	Ersatz			(hzChain& Z, hzString& from, hzString& to, bool bCase=false) ;
uint32_t	Ersatz			(hzChain& Z, const char* from, const char* to, bool bCase=false) ;
hzString	Ersatz			(hzString& S, hzString& from, hzString& to, bool bCase=false) ;
hzString	Ersatz			(hzString& S, const char* from, const char* to, bool bCase=false) ;

//	Presentation
hzString	FormalNumber	(int64_t nValue, uint32_t nMaxlen = 0) ;
hzString	FormalNumber	(uint64_t nValue, uint32_t nMaxlen = 0) ;
hzString	FormalNumber	(int32_t nValue, uint32_t nMaxlen = 0) ;
hzString	FormalNumber	(uint32_t nValue, uint32_t nMaxlen = 0) ;
hzString	FormalMoney		(int32_t nValue) ;

void		SpeakNumber		(hzChain& Z, int32_t num) ;
hzString	SpeakNumber		(int32_t num) ;

bool		ReadHex			(uint32_t& nVal, const char* s) ;
uint32_t	StripCRNL		(char* cpLine) ;
hzString	EnEscape		(const hzString& x) ;
hzString	DeEscape		(const hzString& x) ;
bool		FormCheckCstr	(const char* cpTest, const char* cpCtrl) ;
uint32_t	FormCheckChain	(hzChain::Iter& ci, const char* cpCtrl) ;
bool		FormCheckDate	(hzXDate& date, const char* cpTest, const char* cpCtrl) ;
hzEcode		Grep			(hzChain& Zo, hzChain& Zi, const char* cpExp) ;

//	Initialization
void	InitMimes	(void) ;

#endif	//	hzTextproc_h
