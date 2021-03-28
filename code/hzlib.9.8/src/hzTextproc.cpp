//
//	File:	hzTextproc.cpp
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
//	Purpose:	General text processing functions.
//

#include <iostream>
#include <fstream>

#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzChain.h"
#include "hzDirectory.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Char type map
*/

int16_t	chartype[256] =
{
	/*	0x00 NULL		*/	CTYPE_BINARY,
	/*	0x01 CTRLA		*/	CTYPE_BINARY,
	/*	0x02 CTRLB		*/	CTYPE_BINARY,
	/*	0x03 CTRLC		*/	CTYPE_BINARY,
	/*	0x04 CTRLD		*/	CTYPE_BINARY,
	/*	0x05 CTRLE		*/	CTYPE_BINARY,
	/*	0x06 CTRLF		*/	CTYPE_BINARY,
	/*	0x07 CTRLG		*/	CTYPE_BINARY,
	/*	0x08 CTRLH		*/	CTYPE_BINARY,
	/*	0x09 TAB		*/	CTYPE_WHITE,
	/*	0x0a NEWLINE	*/	CTYPE_WHITE,
	/*	0x0b CTRLK		*/	CTYPE_BINARY,
	/*	0x0c CTRLL		*/	CTYPE_BINARY,
	/*	0x0d CTRLM		*/	CTYPE_WHITE,
	/*	0x0e CTRLN		*/	CTYPE_BINARY,
	/*	0x0f CTRLO		*/	CTYPE_BINARY,
	/*	0x10 CTRLP		*/	CTYPE_BINARY,
	/*	0x11 CTRLQ		*/	CTYPE_BINARY,
	/*	0x12 CTRLR		*/	CTYPE_BINARY,
	/*	0x13 CTRLS		*/	CTYPE_BINARY,
	/*	0x14 CTRLT		*/	CTYPE_BINARY,
	/*	0x15 CTRLU		*/	CTYPE_BINARY,
	/*	0x16 CTRLV		*/	CTYPE_BINARY,
	/*	0x17 CTRLW		*/	CTYPE_BINARY,
	/*	0x18 CTRLX		*/	CTYPE_BINARY,
	/*	0x19 CTRLY		*/	CTYPE_BINARY,
	/*	0x1a CTRLZ		*/	CTYPE_BINARY,
	/*	0x1b			*/	CTYPE_BINARY,
	/*	0x1c			*/	CTYPE_BINARY,
	/*	0x1d			*/	CTYPE_BINARY,
	/*	0x1e			*/	CTYPE_BINARY,
	/*	0x1f			*/	CTYPE_BINARY,
	/*	0x20 SPACE		*/	CTYPE_WHITE,
	/*	0x21 PLING		*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x22 QUOTE		*/	CTYPE_PUNCT,
	/*	0x23 HASH		*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x24 DOLLAR		*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x25 PERCENT	*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x26 AMPSAND	*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x27 SQUOTE		*/	CTYPE_PUNCT | CTYPE_URL_RESV,
	/*	0x28 OPENPAR	*/	CTYPE_PUNCT | CTYPE_URL_RESV,
	/*	0x29 CLOSEPAR	*/	CTYPE_PUNCT | CTYPE_URL_RESV,
	/*	0x2a ASTERISK	*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x2b PLUS		*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x2c COMMA		*/	CTYPE_PUNCT | CTYPE_URL_RESV,
	/*	0x2d MINUS		*/	CTYPE_PUNCT | CTYPE_URL_NORM,
	/*	0x2e PERIOD		*/	CTYPE_PUNCT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x2f FWSLASH	*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x30 DIGIT0		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x31 DIGIT1		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x32 DIGIT2		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x33 DIGIT3		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x34 DIGIT4		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x35 DIGIT5		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x36 DIGIT6		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x37 DIGIT7		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x38 DIGIT8		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x39 DIGIT9		*/	CTYPE_DIGIT | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x3a COLON		*/	CTYPE_PUNCT | CTYPE_URL_RESV,
	/*	0x3b SCOLON		*/	CTYPE_PUNCT | CTYPE_URL_RESV,
	/*	0x3c LESS		*/	CTYPE_SYMB,
	/*	0x3d EQUAL		*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x3e MORE		*/	CTYPE_SYMB,
	/*	0x3f QUERRY		*/	CTYPE_PUNCT | CTYPE_URL_RESV,
	/*	0x40 AT			*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x41 UC_A		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x42 UC_B		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x43 UC_C		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x44 UC_D		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x45 UC_E		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x46 UC_F		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x47 UC_G		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x48 UC_H		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x49 UC_I		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x4a UC_J		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x4b UC_K		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x4c UC_L		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x4d UC_M		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x4e UC_L		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x4f UC_M		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x50 UC_N		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x51 UC_O		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x52 UC_R		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x53 UC_S		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x54 UC_T		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x55 UC_U		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x56 UC_V		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x57 UC_W		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x58 UC_X		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x59 UC_Y		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x5a UC_Z		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x5b SQOPEN		*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x5c BWSLASH	*/	CTYPE_SYMB,
	/*	0x5d SQCLOSE	*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x5e HAT		*/	CTYPE_SYMB,
	/*	0x5f USCORE		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x60 BKQUOTE	*/	CTYPE_PUNCT,
	/*	0x61 LC_A		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x62 LC_B		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x63 LC_C		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x64 LC_D		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x65 LC_E		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_NUMCHAR | CTYPE_URL_NORM,
	/*	0x66 LC_F		*/	CTYPE_ALPHA | CTYPE_HEXDIGIT | CTYPE_URL_NORM,
	/*	0x67 LC_G		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x68 LC_H		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x69 LC_I		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x6a LC_J		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x6b LC_K		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x6c LC_L		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x6d LC_M		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x6e LC_N		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x6f LC_O		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x70 LC_P		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x71 LC_Q		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x72 LC_R		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x73 LC_S		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x74 LC_T		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x75 LC_U		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x76 LC_V		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x77 LC_W		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x78 LC_X		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x79 LC_Y		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x7a LC_Z		*/	CTYPE_ALPHA | CTYPE_URL_NORM,
	/*	0x7b CUROPEN	*/	CTYPE_SYMB,
	/*	0x7c OR			*/	CTYPE_SYMB,
	/*	0x7d CURCLSE	*/	CTYPE_SYMB,
	/*	0x7e TILDA		*/	CTYPE_SYMB  | CTYPE_URL_RESV,
	/*	0x7f BLOCK		*/	CTYPE_BINARY,
	/*	0x80 euro sign	*/	CTYPE_SYMB,
	/*	0x81			*/	CTYPE_BINARY,
	/*	0x82			*/	CTYPE_PUNCT,
	/*	0x83			*/	CTYPE_ALPHA,
	/*	0x84			*/	CTYPE_PUNCT,
	/*	0x85			*/	CTYPE_PUNCT,
	/*	0x86			*/	CTYPE_PUNCT,
	/*	0x87			*/	CTYPE_PUNCT,
	/*	0x88			*/	CTYPE_PUNCT,
	/*	0x89			*/	CTYPE_SYMB,
	/*	0x8A			*/	CTYPE_ALPHA,
	/*	0x8B			*/	CTYPE_PUNCT,
	/*	0x8C			*/	CTYPE_ALPHA,
	/*	0x8D			*/	CTYPE_BINARY,
	/*	0x8E			*/	CTYPE_ALPHA,
	/*	0x8F			*/	CTYPE_BINARY,
	/*	0x90			*/	CTYPE_BINARY,
	/*	0x91			*/	CTYPE_PUNCT,
	/*	0x92			*/	CTYPE_PUNCT,
	/*	0x93			*/	CTYPE_PUNCT,
	/*	0x94			*/	CTYPE_PUNCT,
	/*	0x95			*/	CTYPE_PUNCT,
	/*	0x96			*/	CTYPE_PUNCT,
	/*	0x97			*/	CTYPE_PUNCT,
	/*	0x98			*/	CTYPE_SYMB,
	/*	0x99			*/	CTYPE_SYMB,
	/*	0x9A			*/	CTYPE_SYMB,
	/*	0x9B			*/	CTYPE_SYMB,
	/*	0x9C			*/	CTYPE_ALPHA,
	/*	0x9D			*/	CTYPE_BINARY,
	/*	0x9E			*/	CTYPE_ALPHA,
	/*	0x9F			*/	CTYPE_ALPHA,
	/*	0xA0			*/	CTYPE_WHITE,
	/*	0xA1			*/	CTYPE_SYMB,
	/*	0xA2			*/	CTYPE_SYMB,
	/*	0xA3			*/	CTYPE_SYMB,
	/*	0xA4			*/	CTYPE_SYMB,
	/*	0xA5			*/	CTYPE_SYMB,
	/*	0xA6			*/	CTYPE_SYMB,
	/*	0xA7			*/	CTYPE_SYMB,
	/*	0xA8			*/	CTYPE_SYMB,
	/*	0xA9			*/	CTYPE_SYMB,
	/*	0xAA			*/	CTYPE_SYMB,
	/*	0xAB			*/	CTYPE_SYMB,
	/*	0xAC			*/	CTYPE_SYMB,
	/*	0xAD			*/	CTYPE_SYMB,
	/*	0xAE			*/	CTYPE_SYMB,
	/*	0xAF			*/	CTYPE_SYMB,
	/*	0xB0			*/	CTYPE_SYMB,
	/*	0xB1			*/	CTYPE_SYMB,
	/*	0xB2			*/	CTYPE_SYMB,
	/*	0xB3			*/	CTYPE_SYMB,
	/*	0xB4			*/	CTYPE_SYMB,
	/*	0xB5			*/	CTYPE_SYMB,
	/*	0xB6			*/	CTYPE_SYMB,
	/*	0xB7			*/	CTYPE_SYMB,
	/*	0xB8			*/	CTYPE_SYMB,
	/*	0xB9			*/	CTYPE_SYMB,
	/*	0xBA			*/	CTYPE_SYMB,
	/*	0xBB			*/	CTYPE_SYMB,
	/*	0xBC			*/	CTYPE_SYMB,
	/*	0xBD			*/	CTYPE_SYMB,
	/*	0xBE			*/	CTYPE_SYMB,
	/*	0xBF			*/	CTYPE_SYMB,
	/*	0xC0			*/	CTYPE_ALPHA,
	/*	0xC1			*/	CTYPE_ALPHA,
	/*	0xC2			*/	CTYPE_ALPHA,
	/*	0xC3			*/	CTYPE_ALPHA,
	/*	0xC4			*/	CTYPE_ALPHA,
	/*	0xC5			*/	CTYPE_ALPHA,
	/*	0xC6			*/	CTYPE_ALPHA,
	/*	0xC7			*/	CTYPE_ALPHA,
	/*	0xC8			*/	CTYPE_ALPHA,
	/*	0xC9			*/	CTYPE_ALPHA,
	/*	0xCA			*/	CTYPE_ALPHA,
	/*	0xCB			*/	CTYPE_ALPHA,
	/*	0xCC			*/	CTYPE_ALPHA,
	/*	0xCD			*/	CTYPE_ALPHA,
	/*	0xCE			*/	CTYPE_ALPHA,
	/*	0xCF			*/	CTYPE_ALPHA,
	/*	0xD0			*/	CTYPE_ALPHA,
	/*	0xD1			*/	CTYPE_ALPHA,
	/*	0xD2			*/	CTYPE_ALPHA,
	/*	0xD3			*/	CTYPE_ALPHA,
	/*	0xD4			*/	CTYPE_ALPHA,
	/*	0xD5			*/	CTYPE_ALPHA,
	/*	0xD6			*/	CTYPE_ALPHA,
	/*	0xD7			*/	CTYPE_SYMB,
	/*	0xD8			*/	CTYPE_ALPHA,
	/*	0xD9			*/	CTYPE_ALPHA,
	/*	0xDA			*/	CTYPE_ALPHA,
	/*	0xDB			*/	CTYPE_ALPHA,
	/*	0xDC			*/	CTYPE_ALPHA,
	/*	0xDD			*/	CTYPE_ALPHA,
	/*	0xDE			*/	CTYPE_ALPHA,
	/*	0xDF			*/	CTYPE_ALPHA,
	/*	0xE0			*/	CTYPE_ALPHA,
	/*	0xE1			*/	CTYPE_ALPHA,
	/*	0xE2			*/	CTYPE_ALPHA,
	/*	0xE3			*/	CTYPE_ALPHA,
	/*	0xE4			*/	CTYPE_ALPHA,
	/*	0xE5			*/	CTYPE_ALPHA,
	/*	0xE6			*/	CTYPE_ALPHA,
	/*	0xE7			*/	CTYPE_ALPHA,
	/*	0xE8			*/	CTYPE_ALPHA,
	/*	0xE9			*/	CTYPE_ALPHA,
	/*	0xEA			*/	CTYPE_ALPHA,
	/*	0xEB			*/	CTYPE_ALPHA,
	/*	0xEC			*/	CTYPE_ALPHA,
	/*	0xED			*/	CTYPE_ALPHA,
	/*	0xEE			*/	CTYPE_ALPHA,
	/*	0xEF			*/	CTYPE_ALPHA,
	/*	0xF0			*/	CTYPE_ALPHA,
	/*	0xF1			*/	CTYPE_ALPHA,
	/*	0xF2			*/	CTYPE_ALPHA,
	/*	0xF3			*/	CTYPE_ALPHA,
	/*	0xF4			*/	CTYPE_ALPHA,
	/*	0xF5			*/	CTYPE_ALPHA,
	/*	0xF6			*/	CTYPE_ALPHA,
	/*	0xF7			*/	CTYPE_SYMB,
	/*	0xF8			*/	CTYPE_ALPHA,
	/*	0xF9			*/	CTYPE_ALPHA,
	/*	0xFA			*/	CTYPE_ALPHA,
	/*	0xFB			*/	CTYPE_ALPHA,
	/*	0xFC			*/	CTYPE_ALPHA,
	/*	0xFD			*/	CTYPE_ALPHA,
	/*	0xFE			*/	CTYPE_ALPHA,
	/*	0xFF			*/	CTYPE_ALPHA
} ;

/*
**	Character type functions
*/

//	FnSet:		Character-Type
//	Category:	Text Processing
//
//	The following two set of functions take either an int32_t or a char as the input character and return true if the character is of the implied
//	type. The set starting with 'I' take int32_t. The set starting with 'i' take char.
//
//	Func:	IsBinary(int32_t)
//	Func:	IsWhite(int32_t)
//	Func:	IsDigit(int32_t)
//	Func:	IsHex(int32_t)
//	Func:	IsAlpha(int32_t)
//	Func:	IsHyphen(int32_t)
//	Func:	IsAlphanum(int32_t)
//	Func:	IsPunct(int32_t)
//	Func:	IsSymb(int32_t)
//	Func:	IsNumchar(int32_t)
//	Func:	IsUrlnorm(int32_t)
//	Func:	IsUrlresv(int32_t)
//	Func:	IsTagchar(int32_t)
//	Func:	IsDomainChar(int32_t)

bool	IsBinary	(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_BINARY ? true : false ; }
bool	IsWhite		(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_WHITE ? true : false ; }
bool	IsDigit		(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_DIGIT ? true : false ; }
bool	IsHex		(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_HEXDIGIT ? true : false ; }
bool	IsAlpha		(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_ALPHA ? true : false ; }
bool	IsHyphen	(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_HYPEN ? true : false ; }
bool	IsAlphanum	(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & (CTYPE_ALPHA | CTYPE_DIGIT) ? true : false ; }
bool	IsPunct		(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_PUNCT ? true : false ; }
bool	IsSymb		(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_SYMB ? true : false ; }
bool	IsNumchar	(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_NUMCHAR ? true : false ; }
bool	IsUrlnorm	(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_URL_NORM ? true : false ; }
bool	IsUrlresv	(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & (CTYPE_URL_NORM | CTYPE_URL_RESV) ? true : false ; }
bool	IsTagchar	(int32_t c)	{ return c >= 0 && c < 256 && chartype[c] & CTYPE_HTX_TAG ? true : false ; }

/*
**	Functions
*/

uint32_t	CstrIncidence	(const char* str, char c)
{
	//	Category:	Text Processing
	//
	//	Count the incidence of a particular char in a source string
	//
	//	Arguments:	1) cpStr	The source string
	//				2) c		The char value being counted
	//
	//	Returns:	0+	Being the number of times the supplied test char occurs within the supplied cstr

	const char*	i ;			//	Input string iterator
	uint32_t	n = 0 ;		//	Incidence counter

	if (!str || !str[0] || !c)
		return 0 ;

	for (i = str ; *i ; i++)
	{
		if (*i == c)
			n++ ;
	}

	return n ;
}

void	SplitChain  (hzVect<hzString>& ar, hzChain& input, char cDelim)
{
	//	Category:	Text Processing
	//
	//	Splits an input chain into a series of strings on the basis of a single delimiting char.
	//
	//	Arguments:	1)	ar		The vector of strings that will be populated by this operation (note it is first cleared)
	//				2)	input	The input string or chain
	//				3)	cDelim	The delimitor (default of comma)
	//
	//	Returns:	None.

	hzChain		C ;		//	Temporary chain (for storing partial data)
	chIter		i ;		//	Itererator for input
	hzString	S ;		//	Set by temp chain upon delim

	ar.Clear() ;

	if (input.Size())
	{
		if (!cDelim)
			cDelim = CHAR_COMMA ;

		for (i = input ; !i.eof() ; i++)
		{
			if (*i == cDelim)
			{
				S = C ;
				ar.Add(S) ;
				C.Clear() ;
			}

			C.AddByte(*i) ;
		}

		if (C.Size())
		{
			S = C ;
			ar.Add(S) ;
		}
	}
}

void	SplitStrOnChar  (hzVect<hzString>& ar, hzString& input, char cDelim)
{
	//	Category:	Text Processing
	//
	//	Splits an input string into a series of strings on the basis of a single delimiting char.
	//
	//	Arguments:	1)	ar		The vector of strings that will be populated by this operation (note it is first cleared)
	//				2)	input	The input string or chain
	//				3)	cDelim	The delimitor (default of comma)
	//
	//	Returns:	None

	const char*	i ;			//	Input iterator
	hzString	S ;			//	Substring of input
	uint32_t	nRef = 0 ;	//	Substring end position (within input)
	uint32_t	nPos = 0 ;	//	Substring start position (within input)

	ar.Clear() ;

	if (input)
	{
		if (!cDelim)
			cDelim = 0 ;

		i = *input ;
		nRef = nPos = 0 ;

		for (;;)
		{
			if (i[nPos] == 0 || i[nPos] == cDelim)
			{
				S = input.SubString(nRef, nPos - nRef) ;
				if (S)
					ar.Add(S) ;
				if (i[nPos] == 0)
					break ;
				nRef = nPos + 1 ;
			}
			nPos++ ;
		}
	}
}

hzEcode SplitCstrOnChar  (hzVect<hzString>& ar, const char* input, char cDelim)
{
	//	Category:	Text Processing
	//
	//	Split a null terminated string using a single character as deliminator
	//
	//	Arguments:	1) ar		A vector of hzString that is populated by this operation
	//				2) input	The input null terminated string to be split
	//				3) cDelim	The delimiter
	//
	//	Returns:	E_ARGUMENT	If either the input or delimiter is not supplied
	//				E_OK		If the input was processed

	const char*	i ;			//	Iterator
	hzString	S ;			//	Value acceptor
	uint32_t	nRef = 0 ;	//	Reference position
	uint32_t	nPos = 0 ;	//	Position reached so far

	ar.Clear() ;

	if (!input || !input[0])
		return E_ARGUMENT ;
	if (!cDelim)
		cDelim = CHAR_COMMA ;

	for (i = input ;; i++)
	{
		if (*i == 0 || *i == cDelim)
		{
			if (nPos > nRef)
			{
				//j = S._blank(nPos - nRef) ;
				//memcpy(j, input + nRef, nPos - nRef) ;
				S.SetValue(input + nRef, nPos - nRef) ;
			}
			ar.Add(S) ;
			S.Clear() ;

			nRef = nPos + 1 ;
		}

		if (*i == 0)
			break ;
		nPos++ ;
	}

	return E_OK ;
}

hzEcode SplitCstrOnCstr  (hzVect<hzString>& ar, const char* input, const char* delim)
{
	//	Category:	Text Processing
	//
	//	Split a null terminated string using another null terminated string as deliminator
	//
	//	Arguments:	1) ar		A vector of hzString that is populated by this operation
	//				2) input	The input null terminated string to be split
	//				3) delim	The delimiter string
	//
	//	Returns:	E_ARGUMENT	If either the input or delimiter is not supplied
	//				E_OK		If the input was processed

	const char*	i ;			//	Iterator
	hzString	S ;			//	Value acceptor
	uint32_t	nRef = 0 ;	//	Reference position
	uint32_t	nPos = 0 ;	//	Position reached so far
	uint32_t	nLen ;		//	Delimiter length

	ar.Clear() ;

	if (!input || !input[0])	return E_ARGUMENT ;
	if (!delim || !delim[0])	return E_ARGUMENT ;

	nLen = strlen(delim) ;

	for (i = input ;;)
	{
		if (*i == 0)
		{
			if (nPos > nRef)
			{
				//j = S._blank(nPos - nRef) ;
				//memcpy(j, input + nRef, nPos - nRef) ;
				S.SetValue(input + nRef, nPos - nRef) ;
			}
			ar.Add(S) ;
			break ;
		}

		if (*i == delim[0])
		{
			if (CstrCompare(i, delim))
			{
				if (nPos > nRef)
				{
					//j = S._blank(nPos - nRef) ;
					//memcpy(j, input + nRef, nPos - nRef) ;
					S.SetValue(input + nRef, nPos - nRef) ;
				}
				ar.Add(S) ;
				S.Clear() ;

				nRef = nPos + nLen ;
				i += nLen ;
				nPos += nLen ;
				continue ;
			}
		}

		i++ ; nPos++ ;
	}

	return E_OK ;
}

hzEcode SplitCSV  (hzVect<hzString>& ar, const char* line, char cDelim)
{
	//	Category:	Text Processing
	//
	//	Purpose:	Splits a line from a .csv file into it's fields. Copes with quoted values automatically, removes double-quote pairs
	//				if present and de-escapes sequences.
	//
	//	Arguments:	1)	ar		Either an array of char* or an array of hzString
	//				2)	line	The line to be split (char*)
	//				3)	cDelim	Delimitor char
	//
	//	Returns:	E_ARGUMENT	If either the target array or the line is empty
	//				E_FORMAT	If the number of fields does not match expected number
	//				E_OK		If successful

	hzChain		C ;		//	Working chain
	const char*	i ;		//	Input iterator
	hzString	S ;		//	Data field

	ar.Clear() ;
	i = line ;
	if (!i || !i[0])
		return E_ARGUMENT ;

	if (!cDelim || cDelim == CHAR_DQUOTE)
		cDelim = CHAR_COMMA ;

	for (;;)
	{
		if (*i == CHAR_DQUOTE)
		{
			for (i++ ; *i ; i++)
			{
				if (*i == CHAR_BKSLASH)
				{
					if (i[1] == 'r')	{ i++ ; C.AddByte(CHAR_CR) ; continue ; }
					if (i[1] == 'n')	{ i++ ; C.AddByte(CHAR_NL) ; continue ; }
					if (i[1] == 't')	{ i++ ; C.AddByte(CHAR_TAB) ; continue ; }
					if (i[1] == '"')	{ i++ ; C.AddByte(CHAR_DQUOTE) ; continue ; }
				}

				if (i[0] == CHAR_DQUOTE && i[1] == CHAR_DQUOTE)
					{ i++ ; C.AddByte(CHAR_DQUOTE) ; continue ; }

				if (*i == CHAR_DQUOTE)
					break ;
				C.AddByte(*i) ;
			}

			if (*i == CHAR_DQUOTE)
				i++ ;
			else
				break ;
		}

		if (*i == 0 || *i == cDelim)
		{
			S = C ; ar.Add(S) ; C.Clear() ; S = 0 ;

			if (*i == 0)
				break ;
			i++ ;
			continue ;
		}

		//	Just add char to chain
		if (*i == CHAR_BKSLASH)
		{
			if (i[1] == 'r')	{ i++ ; C.AddByte(CHAR_CR) ; continue ; }
			if (i[1] == 'n')	{ i++ ; C.AddByte(CHAR_NL) ; continue ; }
			if (i[1] == 't')	{ i++ ; C.AddByte(CHAR_TAB) ; continue ; }
			if (i[1] == '"')	{ i++ ; C.AddByte(CHAR_DQUOTE) ; continue ; }
		}

		C.AddByte(*i) ;
		i++ ;
	}

	return E_OK ;
}

hzEcode SplitCSV  (char** ar, char* line, uint32_t arSize, char cDelim)
{
	//	Category:	Text Processing
	//
	//	Purpose:	Splits a line from a .csv file into it's fields. Copes with quoted values automatically, removes double-quote pairs
	//				if present. Note this function does not de-escapes sequences.
	//
	//	Arguments:	1)	ar		Either an array of char* or an array of hzString
	//				2)	line	The line to be split (char*)
	//				3)	cDelim	Delimitor char
	//
	//	Returns:	E_ARGUMENT	If either the target array or the line is not supplied or the array size is 0
	//				E_FORMAT	If the number of fields does not match expected number
	//				E_OK		If successful

	char*		i ;		//	Input iterator
	uint32_t	nPos ;	//	Position within array

	if (!ar || !arSize)
		return E_ARGUMENT ;
	nPos = 0 ;

	if (!line || !line[0])
		return E_ARGUMENT ;

	if (!cDelim || cDelim == CHAR_DQUOTE)
		cDelim = CHAR_COMMA ;

	for (i = line ; *i && nPos < arSize ; i++)
	{
		if (*i == CHAR_DQUOTE)
		{
			//	Handle quoted CSV entry
			i++ ;
			ar[nPos++] = i ;

			for (; *i ; i++)
			{
				if (*i == CHAR_DQUOTE)
					{ *i++ = 0 ; break ; }
			}

			//	Expect delimiter
			if (*i && *i != cDelim)
				return E_FORMAT ;
			continue ;
		}

		//	Handle unquoted CSV entry
		ar[nPos++] = i ;

		for (; *i ; i++)
		{
			if (*i == cDelim)
				{ i++ ; break ; }
		}
	}

	return E_OK ;
}

hzEcode	DosifyChain	(hzChain& Z)
{
	//	Category:	Text Processing
	//
	//	'Dosify' a chain (convert instances of newline into carriage return newline. Note that chains cannot be dosified more than once
	//	by mistake. The instances of newline are not converted to CR-NL unless they lack the preceeding CR
	//
	//	Arguments:	1)	Z	The input chain that will be dosified by this operation
	//
	//	Returns:	E_NODATA	If the source file has not been supplied
	//				E_ARGUMENT	If the target file has not been supplied
	//				E_NOTFOUND	If the source file cannot be found
	//				E_OPENFAIL	If the source file cannot be opened
	//				E_READFAIL	If the source file cannot be read
	//				E_WRITEFAIL	If the target file cannot be created or written to
	//				E_OK		If the operation was successful

	hzChain		F ;		//	Resulting chain
	chIter		zi ;	//	Input chain iterator

	if (!Z.Size())
		return E_NODATA ;

	for (zi = Z ; !zi.eof() ; zi++)
	{
		if (*zi == CHAR_CR)
		{
			F.AddByte(*zi) ;
			zi++ ;

			if (*zi == CHAR_NL)
			{
				F.AddByte(*zi) ;
				continue ;
			}
		}

		if (*zi == CHAR_NL)
		{
			F.AddByte(CHAR_CR) ;
			F.AddByte(CHAR_NL) ;
			continue ;
		}

		F.AddByte(*zi) ;
	}

	Z.Clear() ;
	Z = F ;

	return E_OK ;
}

hzEcode	DosifyFile	(const hzString& tgt, const hzString& src)
{
	//	Category:	Text Processing
	//
	//	'Dosify' a file by converting all instances of newline (with or without the preceeding carriage return) into the carriage return newline sequence.
	//
	//	Arguments:	1)	tgt		The pathname of the target file
	//				2)	src		The pathname of the source file
	//
	//	Returns:	E_NODATA	If the source file has not been supplied
	//				E_ARGUMENT	If the target file has not been supplied
	//				E_NOTFOUND	If the source file cannot be found
	//				E_OPENFAIL	If the source file cannot be opened
	//				E_READFAIL	If the source file cannot be read
	//				E_WRITEFAIL	If the target file cannot be created or written to
	//				E_OK		If the operation was successful

	_hzfunc(__func__) ;

	ifstream	is ;			//	Input stream
	ofstream	os ;			//	Output stream
	hzString	target ;		//	Intermeadiate filename
	char		buf [1024] ;	//	Working buffer
	bool		bSame ;			//	Target/Source match indicator
	hzEcode		rc = E_OK ;		//	Return code

	if (!src)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No source file specified") ;
	if (!tgt)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No target file specified") ;

	//	If source and target file are the same
	if (tgt == src)
		{ bSame = true ; target = tgt + ".x" ; }
	else
		{ bSame = false ; target = tgt ; }

	//	Seek to open source file
	rc = OpenInputStrm(is, src, *_fn) ;
	if (rc != E_OK)
		return rc ;

	//	Open target for writing
	os.open(*target) ;
	if (os.fail())
		return E_WRITEFAIL ;

	for (; rc == E_OK ;)
	{
		is.getline(buf, 1024) ;
		if (!is.gcount())
			break ;

		if (is.fail())
			{ rc = E_READFAIL ; break ; }

		if (!buf[0])
			continue ;

		if (is.gcount() == 1024)
		{
			if (buf[1023] == CHAR_CR)
			{
				buf[1023] = 0 ;
				os << buf << "\r\n" ;
				continue ;
			}

			os.write(buf, 1024) ;
			continue ;
		}

		os << buf << "\r\n" ;
		if (os.fail())
			rc = E_WRITEFAIL ;
	}

	is.close() ;
	os.close() ;

	if (bSame)
	{
		//	Lose the source file and rename the target back to the original
		unlink(*src) ;
		rename(*target, *tgt) ;
	}

	return rc ;
}

uint32_t	CstrCopy	(char* cpDest, const char* cpSource, uint32_t nMaxlen)
{
	//	Category:	Text Processing
	//
	//	Copy a string (and handle null pointers without crashing)
	//
	//	Arguments:	1)	cpDest		Destination string
	//				2)	cpSource	Source string
	//				3)	nMaxlen		Max length to copy or 0 for no whole string copy
	//
	//	Returns:	0	If either the desination or the source are not supplied
	//				0+	The number of characters copied

	_hzfunc("CstrCopy_a") ;

	uint32_t	nCount = 0 ;	//	Byte counter

	if (!cpDest)		return 0 ;
	if (!cpSource)		return 0 ;
	if (!cpSource[0])	return 0 ;

	if (nMaxlen)
		for (; *cpSource && nCount < nMaxlen ; *cpDest++ = *cpSource++, nCount++) ;
	else
		for (; *cpSource ; *cpDest++ = *cpSource++, nCount++) ;
	*cpDest = 0 ;

	return nCount ;
}

uint32_t	CstrOverwrite	(char* cpDest, const char* cpSource, uint32_t nMaxlen)
{
	//	Category:	Text Processing
	//
	//	Overwite the character string (arg 1) with that provided (arg 2) and optionally limit the number of bytes (arg 3)
	//
	//	Arguments:	1)	cpDest		Destination string
	//				2)	cpSource	Source string
	//				3)	nMaxlen		Max length to copy or 0 for no whole string copy
	//
	//	Returns:	0	If either the desination or the source are not supplied
	//				0+	The number of characters overwritten

	_hzfunc("CstrOverwrite_a") ;

	uint32_t	nCount = 0 ;	//	Byte counter

	if (!cpDest)		return 0 ;
	if (!cpSource)		return 0 ;
	if (!cpSource[0])	return 0 ;

	if (nMaxlen)
		for (; *cpSource && nCount < nMaxlen ; *cpDest++ = *cpSource++, nCount++) ;
	else
		for (; *cpSource ; *cpDest++ = *cpSource++, nCount++) ;

	return nCount ;
}

int32_t	CstrCompareI	(const char* pA, const char* pB, uint32_t nMaxlen)
{
	//	Category:	Text Processing
	//
	//	Purpose:	Compare two strings on a case insensitive basis. This does not crash when
	//				given null arguments.
	//
	//	Arguments:	1)	pA		First string
	//				2)	pB		Second string
	//				3)	nMaxlen	Max length to compare (default 0 for no maximum)
	//
	//	Returns:	+1	If pA is lexically greater than pB within the specified length
	//				-1	If pA is lexically less than pB within the specified length
	//				0	If pA and pB are equivelent within the specified length

	_hzfunc("CstrCompareI") ;

	if (!pA || !pA[0])
	{
		if (!pB)	return 1000 ;
		if (!pB[0])	return 0 ;
		return conv2lower(*pB) ;
	}

	if (!pB || !pB[0])
		return conv2lower(*pA) ;

	if (!nMaxlen)
	{
		for (; *pA && *pB ; pA++, pB++)
		{
			if (conv2lower(*pA) != conv2lower(*pB))
				return conv2lower(*pA) - conv2lower(*pB) ;
		}
	}
	else
	{
		for (uint32_t nCount = 0 ; nCount < nMaxlen ; nCount++, pA++, pB++)
		{
			if (conv2lower(*pA) != conv2lower(*pB))
				return conv2lower(*pA) - conv2lower(*pB) ;
		}
	}

	return 0 ;
}

int32_t	CstrCompare	(const char* pA, const char* pB, uint32_t nMaxlen)
{
	//	Category:	Text Processing
	//
	//	Purpose:	Compare two strings (cstr) on a case sensitive basis. This does not crash when given null arguments.
	//
	//	Arguments:	1)	pA		First string
	//				2)	pB		Second string
	//				3)	nMaxlen	Max length to compare (default 0 for no maximum)
	//
	//	Returns:	+1	If pA is lexically greater than pB within the specified length
	//				-1	If pA is lexically less than pB within the specified length
	//				0	If pA and pB are equal within the specified length

	_hzfunc("CstrCompare") ;

	uint32_t	nCount ;	//	Byte counter

	if (!pA || !pA[0])
	{
		if (!pB)	return 0 ;
		if (!pB[0])	return 0 ;
		return *pB ;
	}

	if (!pB || !pB[0])
		return *pA ;

	if (!nMaxlen)
		for (; *pA && *pB && *pA == *pB ; pA++, pB++) ;
	else
		for (nCount = 0 ; nCount < nMaxlen && *pA && *pB && *pA == *pB ; nCount++, pA++, pB++) ;

	return *pA - *pB ;
}

bool	CstrContains	(const char* cpHaystack, const char* cpNeedle)
{
	//	Category:	Text Processing
	//
	//	Determine if the haystack string contains the needle string
	//
	//	Arguments:	1)	cpHaystack	The source string
	//				2)	cpNeedle	The being sought
	//
	//	Returns:	True	If the needle string occurs in the haystack string
	//				False	Otherwise

	const char*	i ;		//	Input string iterator

	if (!cpHaystack || !cpHaystack[0] || !cpNeedle || !cpNeedle[0])
		return false ;

	for (i = cpHaystack ; *i ; i++)
	{
		if (i[0] != cpNeedle[0])
			continue ;

		if (!CstrCompare(i, cpNeedle))
			return true ;
	}

	return false ;
}

bool	CstrContainsI	(const char* cpHaystack, const char* cpNeedle)
{
	//	Category:	Text Processing
	//
	//	Determine if the supplied sub-string is contained within the supplied string on a case-insensitive basis
	//
	//	Arguments:	1)	cpHaystack	The source string
	//				2)	cpNeedle	The being sought
	//
	//	Returns:	True	If the needle string occurs in the haystack string
	//				False	Otherwise

	const char*	i ;			//	Input string iterator
	uint32_t	len ;		//	Test string length
	char		lower ;		//	Lower case of first char of needle
	char		upper ;		//	Upper case of first char of needle

	if (!cpHaystack || !cpHaystack[0] || !cpNeedle || !cpNeedle[0])
		return false ;

	lower = tolower(cpNeedle[0]) ;
	upper = toupper(cpNeedle[0]) ;

	len = strlen(cpNeedle) ;

	for (i = cpHaystack ; *i ; i++)
	{
		if (*i != lower && *i != upper)
			continue ;

		if (!CstrCompareI(i, cpNeedle, len))
			return true ;
	}

	return false ;
}

int32_t	CstrFirst	(const char* cpStr, char testChar)
{
	//	Category:	Text Processing
	//
	//	This returns the position of the first instance of c in str. It has the advantage that it won't crash when given duff input
	//
	//	Arguments:	1)	cpStr		The string to test
	//				2)	testChar	The char to test for
	//
	//	Returns:	-1	If the test char does not appear in the string
	//				0+	Being the position of the first occurence of the test char in the string

	int32_t	nPosn = 0 ;		//	Input string position

	if (!cpStr || !cpStr[0] || !testChar)
		return -1 ;

	for (nPosn = 0 ; cpStr[nPosn] ; nPosn++)
	{
		if (cpStr[nPosn] == testChar)
			return nPosn ;
	}

	return -1 ;
}

int32_t	CstrFirstI	(const char* cpStr, char testChar)
{
	//	Category:	Text Processing
	//
	//	This returns the position of the first instance of c in str. The comparison is case insensitive
	//
	//	Arguments:	1)	cpStr		The string to test
	//				2)	testChar	The char to test for
	//
	//	Returns:	-1	If the test char does not appear in the string
	//				0+	Being the position of the first occurence of the test char in the string

	int32_t	nPosn = 0 ;		//	Input string position

	if (!cpStr || !cpStr[0] || !testChar)
		return -1 ;

	for (nPosn = 0 ; cpStr[nPosn] ; nPosn++)
	{
		if (_tolower(cpStr[nPosn]) == _tolower(testChar))
			return nPosn ;
	}

	return -1 ;
}

int32_t	CstrFirst	(const char* cpHaystack, const char* cpNeedle)
{
	//	Category:	Text Processing
	//
	//	Determine if the supplied sub-string is contained within the supplied string on a case-sensitive basis
	//
	//	Arguments:	1)	cpHaystack	The source string
	//				2)	cpNeedle	The string being sought
	//
	//	Returns:	Position	If the sub-string occurs in the string
	//				0			Otherwise

	uint32_t	n ;		//	String iterator (position)

	if (!cpHaystack || !cpHaystack[0] || !cpNeedle || !cpNeedle[0])
		return false ;

	for (n = 0 ; cpHaystack[n] ; n++)
	{
		if (cpHaystack[n] != cpNeedle[0])
			continue ;

		if (!CstrCompare(cpHaystack + n, cpNeedle))
			return n ;
	}

	return -1 ;
}

int32_t	CstrFirstI	(const char* cpHaystack, const char* cpNeedle)
{
	//	Category:	Text Processing
	//
	//	Determine if the supplied sub-string is contained within the supplied string on a case-sensitive basis
	//
	//	Arguments:	1)	cpHaystack	The source string
	//				2)	cpNeedle	The string being sought
	//
	//	Returns:	Position	If the sub-string occurs in the string
	//				0			Otherwise

	uint32_t	n ;			//	String iterator (position)
	char		lower ;		//	Lower case of first char of needle
	char		upper ;		//	Upper case of first char of needle

	if (!cpHaystack || !cpHaystack[0] || !cpNeedle || !cpNeedle[0])
		return false ;

	lower = tolower(cpNeedle[0]) ;
	upper = toupper(cpNeedle[0]) ;

	for (n = 0 ; cpHaystack[n] ; n++)
	{
		if (cpHaystack[n] != lower && cpHaystack[n] != upper)
			continue ;

		if (!CstrCompareI(cpHaystack + n, cpNeedle))
			return n ;
	}

	return -1 ;
}

int32_t	CstrLast	(const char* cpStr, char testChar)
{
	//	Category:	Text Processing
	//
	//	This returns the position of the last instance of c in str. It has the advantage that it won't crash when given duff input
	//
	//	Arguments:	1)	cpStr		The string to test
	//				2)	testChar	The char to test for
	//
	//	Returns:	-1	If the test char does not appear in the string
	//				0+	Being the position of the last occurence of the test char in the string

	int32_t	nPosn = 0 ;		//	Input string position
	int32_t	nLast = -1 ;	//	Last position found

	if (!cpStr || !cpStr[0] || !testChar)
		return -1 ;

	for (nPosn = 0 ; cpStr[nPosn] ; nPosn++)
	{
		if (cpStr[nPosn] == testChar)
			nLast = nPosn ;
	}

	return nLast ;
}

int32_t	CstrLastI	(const char* cpStr, char testChar)
{
	//	Category:	Text Processing
	//
	//	This returns the position of the last instance of c in str. The comparison is case insensitive
	//
	//	Arguments:	1)	cpStr		The source string
	//				2)	testChar	The char to test for
	//
	//	Returns:	-1	If the test char does not appear in the string
	//				0+	Being the position of the last occurence of the test char in the string

	int32_t	nPosn = 0 ;		//	Input string position
	int32_t	nLast = -1 ;	//	Last position found

	if (!cpStr || !cpStr[0] || !testChar)
		return -1 ;

	for (nPosn = 0 ; cpStr[nPosn] ; nPosn++)
	{
		if (cpStr[nPosn] == testChar)
			nLast = nPosn ;
	}

	return nLast ;
}

int32_t	CstrLast	(const char* cpHaystack, const char* cpNeedle)
{
	//	Category:	Text Processing
	//
	//	Determine if the haystack string contains the needle string
	//
	//	Arguments:	1)	cpHaystack	The source string
	//				2)	cpNeedle	The being sought
	//
	//	Returns:	True	If the needle string occurs in the haystack string
	//				False	Otherwise

	int32_t	n ;				//	String iterator (position)
	int32_t	posn = -1 ;		//	Last position needle found

	if (!cpHaystack || !cpHaystack[0] || !cpNeedle || !cpNeedle[0])
		return -1 ;

	for (n = 0 ; cpHaystack[n] ; n++)
	{
		if (cpHaystack[n] != cpNeedle[0])
			continue ;

		if (!CstrCompare(cpHaystack + n, cpNeedle))
			posn = n ;
	}

	return posn ;
}

int32_t	CstrLastI	(const char* cpHaystack, const char* cpNeedle)
{
	//	Category:	Text Processing
	//
	//	Determine if the supplied sub-string is contained within the supplied string on a case-sensitive basis
	//
	//	Arguments:	1)	cpHaystack	The source string
	//				2)	cpNeedle	The string being sought
	//
	//	Returns:	Position	If the sub-string occurs in the string
	//				-1			Otherwise

	int32_t	n ;				//	String iterator (position)
	int32_t	posn = -1 ;		//	Last position needle found
	char	lower ;			//	Lower case of first char of needle
	char	upper ;			//	Upper case of first char of needle

	if (!cpHaystack || !cpHaystack[0] || !cpNeedle || !cpNeedle[0])
		return -1 ;

	lower = tolower(cpNeedle[0]) ;
	upper = toupper(cpNeedle[0]) ;

	for (n = 0 ; cpHaystack[n] ; n++)
	{
		if (cpHaystack[n] != lower && cpHaystack[n] != upper)
			continue ;

		if (!CstrCompareI(cpHaystack + n, cpNeedle))
			posn = n ;
	}

	return posn ;
}

//	FnGrp:		FormalNumber
//	Category:	Text Presentation
//
//	Present a number formaly with commas every three digits
//
//	Variations:	1)	For large (64 bit) numbers (Maxlen 16 including a - sign)
//				2)	For int32_t (32 bit) numbers (Maxlen 27 including a - sign)
//
//	Arguments:	1)	nValue	The number to print
//				2)	nMaxlen	The size of the output (max depends on value range)
//
//	Returns:	Instance of string by value being the text representation of the supplied value
//
//	Func:	FormalNumber(int64_t,uint32_t)
//	Func:	FormalNumber(uint64_t,uint32_t)
//	Func:	FormalNumber(int32_t,uint32_t)
//	Func:	FormalNumber(uint32_t,uint32_t)

hzString	FormalNumber	(int64_t nValue, uint32_t nMaxlen)
{
	_hzfunc("FormalNumber_a") ;

	char*		i ;			//	Working buffer pointer
	hzString	v ;			//	Target string
	uint32_t	a ;			//	Number of billion billions
	uint32_t	b ;			//	Number of million billions
	uint32_t	c ;			//	Number of million millions
	uint32_t	B ;			//	Number of billions
	uint32_t	M ;			//	Number of millions
	uint32_t	T ;			//	Number of thousands
	uint32_t	U ;			//	Number of units
	char		buf[32] ;	//	Working buffer

	i = buf ;
	if (nValue < 0)
	{
		*i++ = CHAR_MINUS ;
		nValue *= -1 ;
	}

	a = (uint32_t) ((nValue / 1000000000000000000)) & 0xffff ;
	b = (uint32_t) ((nValue % 1000000000000000000) / 1000000000000000) & 0xffff ;
	c = (uint32_t) ((nValue % 1000000000000000) / 1000000000000) & 0xffff ;
	B = (uint32_t) ((nValue % 1000000000000) / 1000000000) & 0xffff ;

	M = (uint32_t) ((nValue % 1000000000) / 1000000) & 0xffff ;
	T = (uint32_t) ((nValue % 1000000) / 1000) & 0xffff ;
	U = (uint32_t) ((nValue % 1000)) & 0xffff ;

	if (nMaxlen > 0)
	{
		if (nMaxlen > 27)
			nMaxlen = 27 ;
		if (i[0] == CHAR_MINUS)
			nMaxlen-- ;

		if		(a > 9)		nMaxlen -= 26 ;
		else if (a)			nMaxlen -= 25 ;
		else if (b > 99)	nMaxlen -= 23 ;
		else if (b > 9)		nMaxlen -= 22 ;
		else if (b)			nMaxlen -= 21 ;
		else if (c > 99)	nMaxlen -= 19 ;
		else if (c > 9)		nMaxlen -= 18 ;
		else if (c)			nMaxlen -= 17 ;
		else if (B > 99)	nMaxlen -= 15 ;
		else if (B > 9)		nMaxlen -= 14 ;
		else if (B)			nMaxlen -= 13 ;
		else if (M > 99)	nMaxlen -= 11 ;
		else if (M > 9)		nMaxlen -= 10 ;
		else if (M)			nMaxlen -= 9 ;
		else if (T > 99)	nMaxlen -= 7 ;
		else if (T > 9)		nMaxlen -= 6 ;
		else if (T)			nMaxlen -= 5 ;
		else if (U > 99)	nMaxlen -= 3 ;
		else if (U > 9)		nMaxlen -= 2 ;
		else
			nMaxlen-- ;

		for (; nMaxlen > 0 ; nMaxlen--)
			*i++ = CHAR_SPACE ;
	}
	if		(a)	sprintf(i, "%d,%03d,%03d,%03d,%03d,%03d,%03d", a, b, c, B, M, T, U) ;
	else if (b)	sprintf(i, "%d,%03d,%03d,%03d,%03d,%03d", b, c, B, M, T, U) ;
	else if (c)	sprintf(i, "%d,%03d,%03d,%03d,%03d", c, B, M, T, U) ;
	else if (B)	sprintf(i, "%d,%03d,%03d,%03d", B, M, T, U) ;
	else if (M)	sprintf(i, "%d,%03d,%03d", M, T, U) ;
	else if (T)	sprintf(i, "%d,%03d", T, U) ;
	else		sprintf(i, "%d", U) ;

	//	correct font problem

	v = buf ;
	return v ;
}

hzString	FormalNumber	(uint64_t nValue, uint32_t nMaxlen)
{
	_hzfunc("FormalNumber(u64)") ;

	char*		i ;			//	Working buffer pointer
	hzString	v ;			//	Target string
	uint32_t	a ;			//	Number of billion billions
	uint32_t	b ;			//	Number of million billions
	uint32_t	c ;			//	Number of million millions
	uint32_t	B ;			//	Number of billions
	uint32_t	M ;			//	Number of millions
	uint32_t	T ;			//	Number of thousands
	uint32_t	U ;			//	Number of units
	char		buf [32] ;	//	Working buffer

	i = buf ;

	if (nValue < 0)
	{
		*i++ = CHAR_MINUS ;
		nValue *= -1 ;
	}

	a = (uint32_t) ((nValue / 1000000000000000000)) & 0xffff ;
	b = (uint32_t) ((nValue % 1000000000000000000) / 1000000000000000) & 0xffff ;
	c = (uint32_t) ((nValue % 1000000000000000) / 1000000000000) & 0xffff ;
	B = (uint32_t) ((nValue % 1000000000000) / 1000000000) & 0xffff ;

	M = (uint32_t) ((nValue % 1000000000) / 1000000) & 0xffff ;
	T = (uint32_t) ((nValue % 1000000) / 1000) & 0xffff ;
	U = (uint32_t) ((nValue % 1000)) & 0xffff ;

	if (nMaxlen > 0)
	{
		if (nMaxlen > 27)
			nMaxlen = 27 ;
		if (i[0] == CHAR_MINUS)
			nMaxlen-- ;

		if		(a > 9)		nMaxlen -= 26 ;
		else if (a)			nMaxlen -= 25 ;
		else if (b > 99)	nMaxlen -= 23 ;
		else if (b > 9)		nMaxlen -= 22 ;
		else if (b)			nMaxlen -= 21 ;
		else if (c > 99)	nMaxlen -= 19 ;
		else if (c > 9)		nMaxlen -= 18 ;
		else if (c)			nMaxlen -= 17 ;
		else if (B > 99)	nMaxlen -= 15 ;
		else if (B > 9)		nMaxlen -= 14 ;
		else if (B)			nMaxlen -= 13 ;
		else if (M > 99)	nMaxlen -= 11 ;
		else if (M > 9)		nMaxlen -= 10 ;
		else if (M)			nMaxlen -= 9 ;
		else if (T > 99)	nMaxlen -= 7 ;
		else if (T > 9)		nMaxlen -= 6 ;
		else if (T)			nMaxlen -= 5 ;
		else if (U > 99)	nMaxlen -= 3 ;
		else if (U > 9)		nMaxlen -= 2 ;
		else
			nMaxlen-- ;

		for (; nMaxlen > 0 ; nMaxlen--)
			*i++ = CHAR_SPACE ;
	}
	if		(a)	sprintf(i, "%d,%03d,%03d,%03d,%03d,%03d,%03d", a, b, c, B, M, T, U) ;
	else if (b)	sprintf(i, "%d,%03d,%03d,%03d,%03d,%03d", b, c, B, M, T, U) ;
	else if (c)	sprintf(i, "%d,%03d,%03d,%03d,%03d", c, B, M, T, U) ;
	else if (B)	sprintf(i, "%d,%03d,%03d,%03d", B, M, T, U) ;
	else if (M)	sprintf(i, "%d,%03d,%03d", M, T, U) ;
	else if (T)	sprintf(i, "%d,%03d", T, U) ;
	else		sprintf(i, "%d", U) ;

	//	correct font problem

	v = buf ;
	return v ;
}

hzString	FormalNumber	(int32_t nValue, uint32_t nMaxlen)
{
	_hzfunc("FormalNumber(i32)") ;

	char*		i ;			//	Working buffer pointer
	hzString	v ;			//	Target string
	uint32_t	B ;			//	Number of billions
	uint32_t	M ;			//	Number of millions
	uint32_t	T ;			//	Number of thousands
	uint32_t	U ;			//	Number of units
	char		buf [32] ;	//	Working buffer

	i = buf ;

	if (nValue < 0)
	{
		*i++ = CHAR_MINUS ;
		nValue *= -1 ;
	}

	B = (nValue / 1000000000) ;
	M = (nValue % 1000000000) / 1000000 ;
	T = (nValue % 1000000) / 1000 ;
	U = (nValue % 1000) ;

	if (nMaxlen > 0)
	{
		if (nMaxlen > 16)
			nMaxlen = 16 ;
		if (i[0] == CHAR_MINUS)
			nMaxlen-- ;

		if		(B)			nMaxlen -= 13 ;
		else if (M > 99)	nMaxlen -= 11 ;
		else if (M > 9)		nMaxlen -= 10 ;
		else if (M)			nMaxlen -= 9 ;
		else if (T > 99)	nMaxlen -= 7 ;
		else if (T > 9)		nMaxlen -= 6 ;
		else if (T)			nMaxlen -= 5 ;
		else if (U > 99)	nMaxlen -= 3 ;
		else if (U > 9)		nMaxlen -= 2 ;
		else
			nMaxlen-- ;

		for (; nMaxlen > 0 ; nMaxlen--)
			*i++ = CHAR_SPACE ;
	}

	if (B)		sprintf(i, "%d,%03d,%03d,%03d", B, M, T, U) ;
	else if (M)	sprintf(i, "%d,%03d,%03d", M, T, U) ;
	else if (T)	sprintf(i, "%d,%03d", T, U) ;
	else		sprintf(i, "%d", U) ;

	v = buf ;
	return v ;
}

hzString	FormalNumber	(uint32_t nValue, uint32_t nMaxlen)
{
	_hzfunc("FormalNumber(u32)") ;

	char*		i ;			//	Working buffer pointer
	hzString	v ;			//	Target string
	uint32_t	B ;			//	Number of billions
	uint32_t	M ;			//	Number of millions
	uint32_t	T ;			//	Number of thousands
	uint32_t	U ;			//	Number of units
	char		buf [32] ;	//	Working buffer

	i = buf ;

	if (nValue < 0)
	{
		*i++ = CHAR_MINUS ;
		nValue *= -1 ;
	}

	B = (nValue / 1000000000) ;
	M = (nValue % 1000000000) / 1000000 ;
	T = (nValue % 1000000) / 1000 ;
	U = (nValue % 1000) ;

	if (nMaxlen > 0)
	{
		if (nMaxlen > 16)
			nMaxlen = 16 ;
		if (i[0] == CHAR_MINUS)
			nMaxlen-- ;

		if		(B)			nMaxlen -= 13 ;
		else if (M > 99)	nMaxlen -= 11 ;
		else if (M > 9)		nMaxlen -= 10 ;
		else if (M)			nMaxlen -= 9 ;
		else if (T > 99)	nMaxlen -= 7 ;
		else if (T > 9)		nMaxlen -= 6 ;
		else if (T)			nMaxlen -= 5 ;
		else if (U > 99)	nMaxlen -= 3 ;
		else if (U > 9)		nMaxlen -= 2 ;
		else
			nMaxlen-- ;

		for (; nMaxlen > 0 ; nMaxlen--)
			*i++ = CHAR_SPACE ;
	}

	if (B)		sprintf(i, "%d,%03d,%03d,%03d", B, M, T, U) ;
	else if (M)	sprintf(i, "%d,%03d,%03d", M, T, U) ;
	else if (T)	sprintf(i, "%d,%03d", T, U) ;
	else		sprintf(i, "%d", U) ;

	v = buf ;
	return v ;
}

hzString	FormalMoney	(int32_t nValue)
{
	//	Category:	Text Presentation
	//
	//	Present a sum of money formaly with either a minus or a space, commas every three digits a period and two digits for cents. 
	//
	//	Argument:	nValue	The number to print
	//
	//	Returns:	Instance of string by value being the text representation of the supplied value

	_hzfunc("FormalMoney") ;

	char*		i ;			//	Working buffer pointer
	hzString	v ;			//	Target string
	uint32_t	M ;			//	Number of millions (value/100,000,000 cents)
	uint32_t	T ;			//	Number of thousands (value%100,000,000/100,000)
	uint32_t	U ;			//	Number of units
	uint32_t	C ;			//	Number of cents
	uint32_t	pad ;		//	For padding calculation
	char		buf[16] ;	//	Working buffer

	i = buf ;

	i[0] = CHAR_SPACE ;
	if (nValue < 0)
	{
		i[0] = CHAR_MINUS ;
		nValue *= -1 ;
	}

	M = (nValue / 100000000) ;
	T = (nValue % 100000000) / 100000 ;
	U = (nValue % 100000) / 100 ;
	C = (nValue % 100) ;

	if		(M > 9)		pad = 0 ;
	else if (M)			pad = 1 ;
	else if (T > 99)	pad = 3 ;
	else if (T > 9)		pad = 4 ;
	else if (T)			pad = 5 ;
	else if (U > 99)	pad = 7 ;
	else if (U > 9)		pad = 8 ;
	else
		pad = 9 ;

	for (i++ ; pad > 0 ; pad--)
		*i++ = CHAR_SPACE ;

	if		(M)	sprintf(i, "%d,%03d,%03d.%02d", M, T, U, C) ;
	else if (T)	sprintf(i, "%d,%03d.%02d", T, U, C) ;
	else if (U)	sprintf(i, "%d.%02d", U, C) ;
	else		sprintf(i, "0.%02d", C) ;

	v = buf ;
	return v ;
}

/*
**	Section X: Number to text conversion
*/

void	_speakdigit	(hzChain& Z, int32_t num)
{
	//	Category:	Text Presentation
	//
	//	Converts digits 0-9 to thier text equivelent and aggregates this to the supplied chain
	//
	//	Arguments:	1)	Z	The chain to add to
	//				2)	num	The digig to convert to word
	//
	//	Returns:	None

	switch	(num)
	{
	case 0:	Z << "zero" ;
	case 1:	Z << "one" ;
	case 2:	Z << "two" ;
	case 3:	Z << "three" ;
	case 4:	Z << "four" ;
	case 5:	Z << "five" ;
	case 6:	Z << "six" ;
	case 7:	Z << "seven" ;
	case 8:	Z << "eight" ;
	case 9:	Z << "nine" ;
	}
}

void	_speakno	(hzChain& Z, int32_t num)
{
	//	Category:	Text Presentation
	//
	//	Converts the supplied int32_t number (arg 2) to the textual equivelent and aggregates this to the supplied chain
	//
	//	Arguments:	1)	Z	The chain to add to
	//				2)	num	The digig to convert to word
	//
	//	Returns:	None

	int32_t	h ;		//	Hundreds
	int32_t	t ;		//	Tens

	if (num > 99)
	{
		h = num / 100 ;
		num -= (h * 100) ;

		_speakdigit(Z, h) ;
		if (num)
			Z << " hundred and " ;
		else
		{
			Z << " hundred" ;
			return ;
		}
	}

	if (num > 19)
	{
		t = num / 10 ;
		num -= (t * 10) ;

		switch	(t)
		{
		case 2:	Z << "twenty" ;
		case 3:	Z << "thirty" ;
		case 4:	Z << "fourty" ;
		case 5:	Z << "fifty" ;
		case 6:	Z << "sixty" ;
		case 7:	Z << "seventy" ;
		case 8:	Z << "eighty" ;
		case 9:	Z << "ninety" ;
		}

		if (!num)
			return ;
	}

	if (num > 9)
	{
		switch	(num)
		{
		case 10: Z << "ten" ;
		case 11: Z << "eleven" ;
		case 12: Z << "twelve" ;
		case 13: Z << "thirteen" ;
		case 14: Z << "fourteen" ;
		case 15: Z << "fifteen" ;
		case 16: Z << "sixteen" ;
		case 17: Z << "seventeen" ;
		case 18: Z << "eighteen" ;
		case 19: Z << "nineteen" ;
		}
	}

	if (num)
		_speakdigit(Z, num) ;
}

void	SpeakNumber	(hzChain& Z, int32_t nValue)
{
	//	Category:	Text Presentation
	//
	//	Converts the supplied int32_t number (arg 2) to the textual equivelent and aggregates this to the supplied chain
	//
	//	Arguments:	1)	Z		Output chain aggregated by this operation
	//				2)	nValue	The numeric value
	//
	//	Returns:	None

	_hzfunc("SpeakNumber") ;

	hzString	v ;		//	To be returned
	int32_t		B ;		//	Billions
	int32_t		M ;		//	Millions
	int32_t		T ;		//	Thousands
	int32_t		U ;		//	Units (0-999)

	if (nValue < 0)
	{
		Z << "Minus " ;
		nValue *= -1 ;
	}

	B = (nValue / 1000000000) ;
	M = (nValue % 1000000000) / 1000000 ;
	T = (nValue % 1000000) / 1000 ;
	U = (nValue % 1000) ;

	if (B)
	{
		nValue -= (B * 1000000000) ;
		_speakno(Z, B) ;
		if (nValue)
			Z << " billion, " ;
		else
		{
			Z << " billion" ;
			return ;
		}
	}

	if (M)
	{
		nValue -= (M * 1000000) ;
		_speakno(Z, M) ;
		if (nValue)
			Z << " million, " ;
		else
		{
			Z << " million" ;
			return ;
		}
	}

	if (T)
	{
		nValue -= (T * 1000000) ;
		_speakno(Z, T) ;
		if (nValue)
			Z << " thousand and " ;
		else
		{
			Z << " thousand" ;
			return ;
		}
	}

	if (U)
		_speakno(Z, U) ;
}

hzString	SpeakNumber	(int32_t num)
{
	//	Category:	Text Presentation
	//
	//	Converts the supplied int32_t number (arg 1) to the textual equivelent (as the number would be spoken)
	//
	//	Arguments:	1)	nValue	The numeric value
	//
	//	Returns:	Instance of hzString by value being text equivelent of supplied number

	hzChain		Z ;		//	Working chain
	hzString	S ;		//	Target string

	SpeakNumber(Z, num) ;
	S = Z ;
	return S ;
}

bool	ReadHex		(int32_t& nVal, const char* s)
{
	//	Category:	Text Processing
	//
	//	Assumes the supplied character string pointer is the start of a hexadecimal number and reads the value. If there are 8 chars that match
	//	[0-9] or [a-f] or [A-F], true is returned and arg1 (int32_t&) is set to the hex value.
	//
	//	Arguments:	1)	nVal	The int32_t reference to be populated
	//				2)	s		The char string assumed to be at the start of a hex number
	//
	//	Returns:	True	If the supplied cstr amounted to a hex value
	//				False	Otherwise

	_hzfunc("ReadHex") ;

	char*	i = (char*) s ;	//	Need this because tolower violates const
	int32_t	v = 0 ;			//	Value being read
	int32_t	n = 0 ;			//	Number of chars processed

	nVal = 0 ;
	if (!i || !i[0])
		return false ;

	for (; *i && n < 8 ; i++, n++)
	{
		if (chartype[*i] & CTYPE_DIGIT)
		{
			v *= 16 ;
			v += (*i - CHAR_0) ;
			continue ;
		}

		//*i = conv2lower(*i) ;
		*i = tolower(*i) ;
		if (*i < 'a' || *i > 'f')
			return false ;

		v *= 16 ;
		v += 10 ;
		v += (*i - 'a') ;
	}

	nVal = v ;
	return true ;
}

uint32_t	StripCRNL	(char* cpLine)
{
	//	Category:	Text Processing
	//
	//	Purpose:	Strip any carriage return and newline from input.
	//
	//	Arguments:	1)	cpLine	Input line (char*)
	//
	//	Returns:	Value being length of the remaining string.

	char*	i = cpLine ;	//	Line iterator
	uint32_t	nCount ;	//	Remaining length

	if (!i)		return 0 ;
	if (!i[0])	return 0 ;

	for (nCount = 0 ; *i ; i++, nCount++)
	{
		if (*i == '\r')
		{
			if (i[1] == CHAR_NL && i[2] == 0)
			{
				i[0] = i[1] = 0 ;
				break ;
			}
		}

		if (*i == '\r' || *i == CHAR_NL)
		{
			if (i[1] == 0)
			{
				i[0] = 0 ;
				break ;
			}
		}
	}

	return nCount ;
}

hzString	EnEscape	(const hzString& x)
{
	//	Category:	Text Processing
	//
	//	Replaces real values with thier escape sequences where approapriate. Eg "Hello" on one line followed by
	//	"World" on another will become "Hello\nWorld".
	//
	//	Arguments:	1)	x	Input string
	//
	//	Returns:	Instance of hzString by value being the escaped translation of the supplied string

	_hzfunc(__func__) ;

	hzChain		ult ;		//	Working chain
	const char*	i ;			//	Input string iterator
	hzString	result ;	//	Output string

	if (!x.Length())
		return result ;

	for (i = *x ; *i ; i++)
	{
		if (*i == CHAR_CR)	{ ult.AddByte(CHAR_BKSLASH) ; ult.AddByte('r') ; continue ; }
		if (*i == CHAR_NL)	{ ult.AddByte(CHAR_BKSLASH) ; ult.AddByte('n') ; continue ; }
		if (*i == CHAR_TAB)	{ ult.AddByte(CHAR_BKSLASH) ; ult.AddByte('t') ; continue ; }

		if (*i < 27)
		{
			ult.AddByte(CHAR_HAT) ;
			ult.AddByte((*i + 'A') - 1) ;
			continue ;
		}

		ult.AddByte(*i) ;
	}

	result = ult ;
	return result ;
}

hzString	DeEscape	(const hzString& x)
{
	//	Category:	Text Processing
	//
	//	Replaces escape sequences with the real vales. Eg "Hello\nWorld" will have the '\n' replaced by the ASCII value for a newline. Handles \r, \t
	//
	//	Arguments:	1)	x	Input string
	//
	//	Returns:	Instance of hzString by value being the unescaped translation of the supplied string

	_hzfunc(__func__) ;

	char*		buf ;		//	Working buffer
	char*		j ;			//	Working buffer iterator
	const char*	i ;			//	Input string iterator
	hzString	result ;	//	Output string

	if (!x.Length())
		return result ;

	j = buf = new char[x.Length() + 1] ;

	for (i = *x ; *i ; i++)
	{
		if (*i == CHAR_BKSLASH)
		{
			i++ ;
			if (*i == 'r')	{ *j++ = CHAR_CR ; continue ; }
			if (*i == 'n')	{ *j++ = CHAR_NL ; continue ; }
			if (*i == 't')	{ *j++ = CHAR_TAB ; continue ; }
			if (*i == 'v')	{ *j++ = CHAR_CTRLI ; continue ; }
			if (*i == 'f')	{ *j++ = CHAR_CTRLL ; continue ; }
			if (*i == '"')	{ *j++ = CHAR_DQUOTE ; continue ; }

			if (*i == CHAR_BKSLASH)
				{ *j++ = CHAR_BKSLASH ; continue ; }

			i-- ;
		}
		if (*i == CHAR_HAT)
		{
			i++ ;
			if (*i >= 'A' && *i <= 'Z')
				{ *j++ = ((*i - 'A') + 1) ; continue ; }
			i-- ;
		}
		*j++ = *i ;
	}
	*j = 0 ;

	result = buf ;
	delete buf ;
	return result ;
}

/*
**	Tokenization functions
*/

bool	IsEntity	(uint32_t& uVal, uint32_t& nLen, const char* tok)
{
	//	Category:	Text Processing
	//
	//	Purpose:	Determine if a token could represent an entity of the form &#d..d; or &#xd..d;
	//				Note that because of the anticipated context in which the function will be used,
	//				the 
	//
	//	Arguments:	1)	uVal	A refernece to the result (int32_t)
	//				2)	nLen	The length of the token if confirmed as an entity.
	//				3)	tok		The token (const char*)
	//
	//	Returns:	True	If token could be a valid large integer
	//				False	If string of zero length or contains non numeric chars

	uint32_t	v = 0 ;		//	Value
	int32_t		c = 0 ;		//	Iterator
	const char*	i ;			//	Pointer into token

	uVal = 0 ;
	nLen = 0 ;

	//	First char must be an ampersand
	if (!tok)			return false ;
	if (tok[0] != '&')	return false ;

	//	Second char could be start of a non-numeric entity
	i = tok + 1 ;
	if (*i == 'a' && memcmp(i, "amp;", 4) == 0)		{ uVal = 38 ; nLen = 5 ; return true ; }
	if (*i == 'g' && memcmp(i, "gt;", 3) == 0)		{ uVal = 62 ; nLen = 4 ; return true ; }
	if (*i == 'l' && memcmp(i, "lt;", 3) == 0)		{ uVal = 60 ; nLen = 4 ; return true ; }
	if (*i == 'n' && memcmp(i, "nbsp;", 5) == 0)	{ uVal = 32 ; nLen = 6 ; return true ; }

	//	Now we only consider numeric entities and for these the second char must be a hash
	if (tok[1] != '#')	return false ;

	if (tok[2] == 'x')
	{
		//	Suspect a hexadecimal entity

		for (c = 3, i = tok + c ; IsHex(*i) ; c++, i++)
		{
			v *= 16 ;

			if (*i >= '0' && *i <= '9') { v += (*i - '0') ; continue ; }
			if (*i >= 'A' && *i <= 'F') { v += 10 ; v += (*i - 'A') ; continue ; }
			if (*i >= 'a' && *i <= 'f') { v += 10 ; v += (*i - 'a') ; continue ; }

			break ;
		}
	}
	else
	{
		//	Suspect a decimal entity

		for (c = 2, i = tok + c ; IsDigit(*i) ; c++, i++)
		{
			v *= 10 ;
			v += (*i - '0') ;
		}
	}

	if (*i == ';')
	{
		//	Entity confirmed

		uVal = v ;
		nLen = c + 1 ;
		return true ;
	}

	return false ;
}

bool	AtEntity	(uint32_t& uVal, uint32_t& entLen, hzChain::Iter& zi)
{
	//	Category:	Text Processing
	//
	//	Determine if the supplied chain iterator is at the start of a numeric entity of the form &#x[hex number]; or &#[dec number];
	//
	//	Arguments:	1)	uVal	The value of the entity.
	//				2)	entLen	The length of the entity
	//				3)	zi		The chain iterator.
	//
	//	Returns:	True	If the chain iterator is at the start of a valid entity
	//				False	Otherwise
	//
	//	Note:		This function always leaves the supplied iterator unchanged in accordance with the HadronZoo text-processing rules.

	_hzfunc(__func__) ;

	chIter		xi ;		//	Input chain iterator
	uint32_t	nLen ;		//	Length of entity

	uVal = entLen = 0 ;
	nLen = 0 ;

	//	First char must be an ampersand
	xi = zi ;
	if (*xi != CHAR_AMPSAND)
		return false ;

	//	Second char could be start of a non-numeric entity
	xi++ ;
	if (*xi == 'a' && xi == "amp;")		{ uVal = 38 ; return 5 ; }
	if (*xi == 'g' && xi == "gt;")		{ uVal = 62 ; return 4 ; }
	if (*xi == 'l' && xi == "lt;")		{ uVal = 60 ; return 4 ; }
	if (*xi == 'n' && xi == "nbsp;")	{ uVal = 32 ; return 6 ; }

	//	Now we only consider numeric entities and for these the second char must be a hash
	if (*xi != CHAR_HASH)
		return false ;

	xi++ ;
	if (*xi == 'x')
	{
		//	Examining a suspected hex entity

		nLen = 4 ;
		for (xi++ ; !xi.eof() && IsHex(*xi) ; nLen++, xi++)
		{
			uVal *= 16 ;

			if (*xi >= '0' && *xi <= '9') { uVal += (*xi - '0') ; continue ; }
			if (*xi >= 'A' && *xi <= 'F') { uVal += 10 ; uVal += (*xi - 'A') ; continue ; }
			if (*xi >= 'a' && *xi <= 'f') { uVal += 10 ; uVal += (*xi - 'a') ; continue ; }

			break ;
		}
	}
	else
	{
		//	Expecting a decimal entity

		nLen = 3 ;
		for (; !xi.eof() && IsDigit(*xi) ; nLen++, xi++)
		{
			uVal *= 10 ;
			uVal += (*xi - '0') ;
		}
	}

	if (*xi == ';')
	{
		//	Entity confirmed

		entLen = nLen ;
		return true ;
	}

	uVal = 0 ;
	return false ;
}

/*
**	Tokens: Unicode Sequences
*/

bool	AtUnicodeSeq	(uint32_t& uVal, uint32_t& nLen, hzChain::Iter& zi)
{
	//	Category:	Text Presentation
	//
	//	Determines if the supplied chain iterator is at the begining of a unicode sequence. If it is then both the value and size are determined.
	//
	//	Note to be a UTF-8 sequence, the first byte must be either:-
	//		a)	110xxxxx for a 2-byte sequence with the next byte being of the form 10xxxxxx 
	//		b)	1110xxxx for a 3-byte sequence with the next 2 bytes being of the form 10xxxxxx
	//		c)	11110xxx for a 4-byte sequence with the next 3 bytes being of the form 10xxxxxx
	//		d)	111110xx for a 5-byte sequence with the next 4 bytes being of the form 10xxxxxx
	//		e)	1111110x for a 6-byte sequence with the next 5 bytes being of the form 10xxxxxx
	//
	//	Arguments:	1)	uVal	The unicode value (set by this function)
	//				2)	nLen	The length of the unicode sequence encountered (set by this function)
	//				3)	zi		The chain or char sting iterator
	//
	//	Returns:	True	If the supplied chain iterator is at the begining of a unicode sequence
	//				False	Otherwise

	_hzfunc(__func__) ;

	chIter	xi ;		//	Input chain iterator
	uchar	ubuf[8] ;	//	Unicode buffer

	uVal = 0 ;
	nLen = 0 ;

	if (zi.eof())
		return false ;

	if (!(*zi & 0x80))
		return false ;

	//	Get first two bytes
	xi = zi ;
	ubuf[0] = (uchar) *xi ;
	xi++ ;
	ubuf[1] = (uchar) *xi ;

	//	If 2nd byte is not 0x80 or greater then we have a single byte unicode sequence
	if (!(ubuf[1] & 0x80))
		{ nLen = 1 ; uVal = ubuf[0] ; return false ; }

	//	If this is a utf-8 sequence then between 1 and 5 subsequent bytes will have a value between 128 and 191. If this is
	//	the case we compute a value for the sequence and then look up this value for a printable form.

	if ((ubuf[0] & 0xE0) == 0xC0)
	{
		//	First 2 bits set, 3rd bit clear; Utf-8 sequence is this and next byte
		if ((ubuf[1] & 0xC0) == 0x80)
			{ nLen = 2 ; uVal = ((ubuf[0] & 0x1F) << 6) + (ubuf[1] & 0x3F) ; return true ; }
		return false ;
	}

	if ((ubuf[0] & 0xF0) == 0xE0)
	{
		//	First 3 bits set, 4th bit clear; Utf-8 sequence is this and next 2 bytes
		xi++ ; ubuf[2] = (uchar) *xi ;

		if ((ubuf[1] & 0xC0) == 0x80 && (ubuf[2] & 0xC0) == 0x80)
			{ nLen = 3 ; uVal = ((ubuf[0] & 0x0F) << 12) + ((ubuf[1] & 0x3F) << 6) + (ubuf[2] & 0x3F) ; return true ; }
		return false ;
	}

	if ((ubuf[0] & 0xF8) == 0xF0)
	{
		//	First 4 bits set, 5th bit clear; Utf-8 sequence is this and next 3 bytes
		xi++ ; ubuf[2] = (uchar) *xi ;
		xi++ ; ubuf[3] = (uchar) *xi ;

		if ((ubuf[1] & 0xC0) == 0x80 && (ubuf[2] & 0xC0) == 0x80 && (ubuf[3] & 0xC0) == 0x80)
		{
			uVal = ((ubuf[0] & 0x07) << 18) + ((ubuf[1] & 0x3F) << 12) + ((ubuf[2] & 0x3F) <<  6) + (ubuf[3] & 0x3F) ;
			nLen = 4 ;
			return true ;
		}

		return false ;
	}

	if ((ubuf[0] & 0xFC) == 0xF8)
	{
		//	First 5 bits set, 6th bit clear; Utf-8 sequence is this and next 4 bytes
		xi++ ; ubuf[2] = (uchar) *xi ;
		xi++ ; ubuf[3] = (uchar) *xi ;
		xi++ ; ubuf[4] = (uchar) *xi ;

		if ((ubuf[1] & 0xC0) == 0x80 && (ubuf[2] & 0xC0) == 0x80 && (ubuf[3] & 0xC0) == 0x80 && (ubuf[4] & 0xC0) == 0x80)
		{
			uVal = ((ubuf[0] & 0x03) << 24) + ((ubuf[1] & 0x3F) << 18) + ((ubuf[2] & 0x3F) << 12) + ((ubuf[3] & 0x3F) <<  6) + (ubuf[4] & 0x3F) ;
			nLen = 5 ;
			return true ;
		}

		return false ;
	}

	if ((ubuf[0] & 0xFE) == 0xFC)
	{
		//	First 6 bits set, 7th bit clear; Utf-8 sequence is this and next 5 bytes
		xi++ ; ubuf[2] = (uchar) *xi ;
		xi++ ; ubuf[3] = (uchar) *xi ;
		xi++ ; ubuf[4] = (uchar) *xi ;
		xi++ ; ubuf[5] = (uchar) *xi ;

		if ((ubuf[1] & 0xC0) == 0x80 && (ubuf[2] & 0xC0) == 0x80 && (ubuf[3] & 0xC0) == 0x80 && (ubuf[4] & 0xC0) == 0x80 && (ubuf[5] & 0xC0))
		{
			uVal = ((ubuf[0] & 0x01) << 30) + ((ubuf[1] & 0x3F) << 24) + ((ubuf[2] & 0x3F) << 18) + ((ubuf[3] & 0x3F) << 12)
				+ ((ubuf[4] & 0x3F) <<  6) + (ubuf[5] & 0x3F) ;
			nLen = 6 ;
			return true ;
		}
	}

	return false ;
}

bool	IsUnicodeSeq	(uint32_t& nEnt, uint32_t& nLen, const char* zi)
{
	//	Category:	Text Presentation
	//
	//	Determine if the current char is the start of a unicode (UTF-8) sequence. If it is the value and the length (args 1 & 2) are set and
	//	true is returned. Otherwise the value is just the current char (either upper or lower ASCII), the length is 1 and false is returned.
	//
	//	Note to be a UTF-8 sequence, the first byte must be either:-
	//		a)	110xxxxx for a 2-byte sequence with the next byte being of the form 10xxxxxx 
	//		b)	1110xxxx for a 3-byte sequence with the next 2 bytes being of the form 10xxxxxx
	//		c)	11110xxx for a 4-byte sequence with the next 3 bytes being of the form 10xxxxxx
	//		d)	111110xx for a 5-byte sequence with the next 4 bytes being of the form 10xxxxxx
	//		e)	1111110x for a 6-byte sequence with the next 5 bytes being of the form 10xxxxxx
	//
	//	Arguments:	1)	uVal	The unicode value
	//				2)	nLen	The length of the unicode sequence encountered
	//				3)	zi		The chain or char sting iterator
	//
	//	Returns:	True	If the chain iterator is at the start of a unicode sequence
	//				False	Otherwise

	const uchar*	xi ;	//	Input chain iterator

	uchar		ubuf[8] ;	//	Unicode buffer
	uint32_t	value ;		//	Unicode value

	nLen = 0 ;
	value = 0 ;
	ubuf[0] = (uchar) *zi ;

	if (ubuf[0] >= 192)
	{
		//	If this is a utf-8 sequence then between 1 and 5 subsequent bytes will have a value between 128 and 191. If this is
		//	the case we compute a value for the sequence and then look up this value for a printable form.

		xi = (uchar*) zi ;

		if (ubuf[0] < 224)
		{
			//	Utf-8 sequence is this and next byte
			xi++ ; ubuf[1] = (uchar) *xi ;

			if (ubuf[1] > 127 && ubuf[1] < 192)
			{
				value =  ((ubuf[0] & 0x1F) << 6) ;
				value += (ubuf[1] & 0x3F) ;
				nLen = 2 ;
			}
		}
		else if (ubuf[0] < 240)
		{
			//	Utf-8 sequence is this and next 2 bytes
			xi++ ; ubuf[1] = (uchar) *xi ;
			xi++ ; ubuf[2] = (uchar) *xi ;

			if (ubuf[1] > 127 && ubuf[1] < 192 && ubuf[2] > 127 && ubuf[2] < 192)
			{
				value =  ((ubuf[0] & 0x0F) << 12) ;
				value += ((ubuf[1] & 0x3F) <<  6) ;
				value += (ubuf[2] & 0x3F) ;
				nLen = 3 ;
			}
		}
		else if (ubuf[0] < 248)
		{
			//	Utf-8 sequence is this and next 3 bytes
			xi++ ; ubuf[1] = (uchar) *xi ;
			xi++ ; ubuf[2] = (uchar) *xi ;
			xi++ ; ubuf[3] = (uchar) *xi ;

			if (ubuf[1] > 127 && ubuf[1] < 192 && ubuf[2] > 127 && ubuf[2] < 192 && ubuf[3] > 127 && ubuf[3])
			{
				value =  ((ubuf[0] & 0x07) << 18) ;
				value += ((ubuf[1] & 0x3F) << 12) ;
				value += ((ubuf[2] & 0x3F) <<  6) ;
				value += (ubuf[3] & 0x3F) ;
				nLen = 4 ;
			}
		}
		else if (ubuf[0] < 252)
		{
			//	Utf-8 sequence is this and next 4 bytes
			xi++ ; ubuf[1] = (uchar) *xi ;
			xi++ ; ubuf[2] = (uchar) *xi ;
			xi++ ; ubuf[3] = (uchar) *xi ;
			xi++ ; ubuf[4] = (uchar) *xi ;

			if (ubuf[1] > 127 && ubuf[1] < 192 && ubuf[2] > 127 && ubuf[2] < 192
				&& ubuf[3] > 127 && ubuf[3] && ubuf[4] > 127 && ubuf[4])
			{
				value =  ((ubuf[0] & 0x03) << 24) ;
				value += ((ubuf[1] & 0x3F) << 18) ;
				value += ((ubuf[2] & 0x3F) << 12) ;
				value += ((ubuf[3] & 0x3F) <<  6) ;
				value += (ubuf[4] & 0x3F) ;
				nLen = 5 ;
			}
		}
		else
		{
			//	Utf-8 sequence is this and next 5 bytes
			xi++ ; ubuf[1] = (uchar) *xi ;
			xi++ ; ubuf[2] = (uchar) *xi ;
			xi++ ; ubuf[3] = (uchar) *xi ;
			xi++ ; ubuf[4] = (uchar) *xi ;
			xi++ ; ubuf[5] = (uchar) *xi ;

			if (ubuf[1] > 127 && ubuf[1] < 192 && ubuf[2] > 127 && ubuf[2] < 192
				&& ubuf[3] > 127 && ubuf[3] && ubuf[4] > 127 && ubuf[4] && ubuf[5] > 127 && ubuf[5])
			{
				value =  ((ubuf[0] & 0x01) << 30) ;
				value += ((ubuf[1] & 0x3F) << 24) ;
				value += ((ubuf[2] & 0x3F) << 18) ;
				value += ((ubuf[3] & 0x3F) << 12) ;
				value += ((ubuf[4] & 0x3F) <<  6) ;
				value += (ubuf[5] & 0x3F) ;
				nLen = 6 ;
			}
		}
	}

	nEnt = value ;
	return nLen ? true : false ;
}

uint32_t	TestAlphanum	(hzString& word, hzChain::Iter& ci)
{
	//	Category:	Text Presentation
	//
	//	If the supplied chain iterator's current character is alphanumeric, place it and all subsequent alphanumic characters into the supplied string. Note the
	//	supplied iterator is not advanced by this function.
	//
	//	Arguments:	1)	word	The found alphanumeric sequence
	//				2)	ci		The input chain iterator
	//
	//	Returns:	Length of alphanumeric sequence

	word.Clear() ;

	if (ci.eof())
		return 0 ;

	if (!IsAlphanum(*ci))
		return 0 ;

	hzChain::Iter	zi ;	//	Forward iterator
	hzChain			W ;		//	For building word

	for (zi = ci ; !zi.eof() && IsAlpha(*zi) ; zi++)
		W.AddByte(*zi) ;

	if (!W.Size())
		return 0 ;

	word = W ;
	return word.Length() ;
}

int32_t	CstrCompareW	(const char* a, const char* b)
{
	//	Category:	Text Processing
	//
	//	Purpose:	Compare two strings on a word basis. This ignores case and considers both input strings as a series of alphanumeric
	//				words separated by whitespace. Adjacent whitespace characters are treated as a single space with leading & trailing
	//				whitespace ignored. All punctuation characters and all characters beyond the lower ASCII are ignored.
	//
	//	Arguments:	1)	a	First string
	//				2)	b	Second string
	//
	//	Returns:	+1	If a is lexically more than b
	//				-1	If a is lexically less than b
	//				0	If a and b are lexically equivelent

	const char*	pA = a ;	//	String A iterator
	const char*	pB = b ;	//	String B iterator

	uint32_t	entA ;		//	Value of char (be it ASCII, entity or unicode sequence)
	uint32_t	entB ;		//	Same for second input
	uint32_t	len ;		//	Length of sequence from which ent is derived

	if (pA)
		for (pA++ ; *pA && *pA <= CHAR_SPACE ; pA++) ;
	if (pB)
		for (pB++ ; *pB && *pB <= CHAR_SPACE ; pB++) ;

	for (;;)
	{
		entA = entB = len = 0 ;

		//	Get next value from A
		if (pA)
		{
			if (*pA && *pA <= CHAR_SPACE)
			{
				for (pA++ ; *pA <= CHAR_SPACE ; pA++) ;
				entA = CHAR_SPACE ;
			}
			else
			{
				for (; *pA ;)
				{
					if (IsEntity(entA, len, pA))
						pA += len ;
					else if (IsUnicodeSeq(entA, len, pA))
						pA += len ;
					else
						entA = *pA++ ;

					if (IsAlphanum(entA))
						{ entA = conv2lower(entA) ; break ; }
				}
			}

			if (*pA == 0 && entA == CHAR_SPACE)
				entA = 0 ;
		}

		if (pB)
		{
			if (*pB && *pB <= CHAR_SPACE)
			{
				for (pB++ ; *pB && *pB <= CHAR_SPACE ; pB++) ;
				entB = CHAR_SPACE ;
			}
			else
			{
				for (; *pB ;)
				{
					if (IsEntity(entB, len, pB))
						pB += len ;
					else if (IsUnicodeSeq(entB, len, pB))
						pB += len ;
					else
						entB = *pB++ ;

					if (IsAlphanum(entB))
						{ entB = conv2lower(entB) ; break ; }
				}
			}

			if (*pB == 0 && entB == CHAR_SPACE)
				entB = 0 ;
		}

		if (entA != entB)
			return entA - entB ;

		if (!entA)
			break ;
	}

	return 0 ;
}

/*
**	Section X:	String substitution
*/

//	FnGrp:		Ersatz
//	Category:	Text Processing
//
//	Substitute all instances of one string with another. The source of text can be either a hzChain or a hzString. This may either be applied on a
//	case sensitive or a case insensitive basis.
//
//	Substitute all instances found in hzChain Z (arg 1) of hzString 'from' (arg 2) with hzString 'to' (arg 3). Matches on OLD are case sensitive
//	by default but are case insensitive if arg 4 is true.
//
//	Arguments:	1)	Z		The chain containing text upon which the substitions are to be performed.
//				2)	from	The string for which all instances are to be replaced
//				3)	to		The string that will serve as the replacement.
//				4)	bCase	Boolean true for case insensitive. Default false for case sensitive
//
//	Returns		The number of substitutions that were performed
//
//	Func:	Ersatz(hzChain&,hzString&,hzString&,bool)
//	Func:	Ersatz(hzChain&,const char*,const char*,bool)
//	Func:	Ersatz(hzString&,hzString&,hzString&,bool)
//	Func:	Ersatz(hzString&,const char*,const char*,bool)

uint32_t	Ersatz	(hzChain& Z, hzString& from, hzString& to, bool bCase)
{
	_hzfunc("Ersatz1") ;

	chIter		zi ;			//	Chain iterator
	hzChain		F ;				//	Result
	uint32_t	nSubs = 0 ;		//	Number of substitutions

	if (!from)	return -1 ;
	if (!to)	return -1 ;

	if (!Z.Size())
		return 0 ;

	if (bCase)
	{
		for (zi = Z ; !zi.eof() ;)
		{
			if (zi == from)
				{ nSubs++ ; F << to ; zi += from.Length() ; continue ; }

			F.AddByte(*zi) ;
			zi++ ;
		}
	}
	else
	{
		for (zi = Z ; !zi.eof() ;)
		{
			if (zi.Equiv(from))
				{ nSubs++ ; F << to ; zi += from.Length() ; continue ; }

			F.AddByte(*zi) ;
			zi++ ;
		}
	}

	Z.Clear() ;
	Z = F ;

	return nSubs ;
}

uint32_t	Ersatz	(hzChain& Z, const char* from, const char* to, bool bCase)
{
	_hzfunc("Ersatz2") ;

	hzString	a = from ;	//	Cast to hzString
	hzString	b = to ;	//	Cast to hzString

	return Ersatz(Z, a, b, bCase) ;
}

hzString	Ersatz	(hzString& S, hzString& from, hzString& to, bool bCase)
{
	_hzfunc("Ersatz3") ;

	hzChain	ult ;		//	For building result
	const char*	i ;		//	Iterator

	if (!S)
		return S ;

	if (bCase)
	{
		for (i = *S ; *i ; i++)
		{
			if (CstrCompareI(i, *from, from.Length()) == 0)
			//if (strncasecmp(i, *from, from.Length()) == 0)
				ult << to ;
			else
				ult.AddByte(*i) ;
		}
	}
	else
	{
		for (i = *S ; *i ; i++)
		{
			if (memcmp(i, from, from.Length()) == 0)
				ult << to ;
			else
				ult.AddByte(*i) ;
		}
	}

	S.Clear() ;
	S = ult ;
	return S ;
}

hzString	Ersatz	(hzString& S, const char* from, const char* to, bool bCase)
{
	_hzfunc("Ersatz4") ;

	hzString	a = from ;	//	Cast to hzString
	hzString	b = to ;	//	Cast to hzString

	return Ersatz(S, a, b, bCase) ;
}
