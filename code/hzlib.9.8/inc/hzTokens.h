//
//	File:	hzTokens.h
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
//	See synopsis 	Tokenization
//
//	This package facilitates tokenization of strings into tokens classified as one of the following types:-
//
//	1)	Alphanumeric. The token may have chars A-Z, a-z, 0-9 or _
//	2)	Operator. The token is an arithmetic or logical operator.
//	3)	Value. This can be either of the following:-
//		1)	An integer. (has only chars 0-9)
//		2)	A Double. (has form of standard number)
//		3)	A String Value. (Must be enclosed in quotes)
//	4)	Separator. Punctuation. Always a single char.
//
//	The operation is as follows:-
//
//	1)	Create an instance of hzToken
//	2)	Call member Tokenize() passing string to be tokenized.
//	3)	Move thru the resulting hzVect of tokens
//

#ifndef hzTokens_h
#define hzTokens_h

#include "hzCtmpls.h"

#define MAXTOKENSIZE	256

/*
**	Special char types for tokenization purposes
*/

#define	NOTYPE		0x0000		//	undefined char type
#define BINARY		0x0001		//	unprintable, ctl etc
#define WHITE		0x0002		//	space, tab or newline
#define DIGIT		0x0004		//	0 - 9 only
#define HEXDIGIT	0x0008		//	0 - 9 only
#define ALPHA		0x0010		//	a - z and A - Z
#define HYPEN		0x0020		//	any symbol used as hyphen
#define PUNCT		0x0040		//	any punctuation char
#define	SYMB		0x0080		//	symbols, eg math operators
#define NUMCHAR		0x0100		//	any char used in a number

/*
**	Token types
*/

enum	hzTokenType
{
	//	Category:	Text Processing
	//
	//	Enumeration of token types

	TOKEN_ALPHANUM,				//	Token consists of string of [a-z] or [A-Z] or [0-9]
	TOKEN_OPERATOR,				//	Token is any of the standard operators
	TOKEN_SEPARATOR,			//	Token acts as a separator
	TOKEN_INTEGER,				//	Token is a string of [0-9]
	TOKEN_NUMBER,				//	Token is a number eg 10, 10.8, 1.08e-2 etc etc
	TOKEN_HEXVALUE,				//	Token is an integer expresed in hex, must begin with '0x'
	TOKEN_DOUBLE,				//	Token is a number of standard form
	TOKEN_STRING,				//	Token is a string
	TOKEN_COMMENT,				//	Token is comment. Either /* ... */ or //
	TOKEN_UNDEFINED				//	Token is none of the above
} ;

enum	hzTokMode
{
	//	Category:	Text Processing
	//
	//	Enumeration of different tokenization regimes

	TOK_MO_WHITE,				//	Split into words separated by whitespace
	TOK_MO_FTEXT,				//	Split into words according to free text rules
	TOK_MO_BOOL,				//	Tokenize into boolean expression of operators and operands
	TOK_MO_CPP					//	Tokenize into C++ classes, functions, operators and operands
} ;

/*
**	hzToken class
*/

class	hzToken
{
	//	Category:	Text Processing
	//
	//	Tokens are the meaningful units derived from the input text.

private:
	hzString	m_Tok ;			//	The token
	uint32_t	m_nLine ;		//	The line it was on in the file
	hzTokenType	m_eType ;		//	The token type

public:
	hzToken		(void)
	{
		m_nLine = 0 ;
		m_eType = TOKEN_UNDEFINED ;
	}

	~hzToken	(void)
	{
	}

	//	Set functions
	hzEcode	Init	(const char* txt, uint32_t line, hzTokenType type)	{ m_Tok = txt ; m_nLine = line ; m_eType = type ; }
	hzEcode	Init	(const hzString& txt, uint32_t line, hzTokenType type)	{ m_Tok = txt ; m_nLine = line ; m_eType = type ; }

	//	Get functions
	const hzString&	Value	(void) const	{ return m_Tok ; }
	uint32_t		LineNo	(void) const	{ return m_nLine ; }
	hzTokenType		Type	(void) const	{ return m_eType ; }

	//	Operators
	bool	operator=	(const hzToken& op)
	{
		m_Tok = op.m_Tok ;
		m_nLine = op.m_nLine ;
		m_eType = op.m_eType ;
		return m_Tok.Length() ? true : false ;
	}

	bool	operator==	(const hzToken& op)
	{
		if (m_nLine != op.m_nLine)  return false ;
		if (m_eType != op.m_eType)  return false ;
		return m_Tok == op.m_Tok ;
	}

	bool	operator==	(const char* pStr)	{ return m_Tok == pStr ? true : false ; }
	bool	operator!=	(const char* pStr)	{ return m_Tok == pStr ? false : true ; }

	bool	operator!	(void)	{ return m_Tok ? false : true ; }
} ;

/*
**	Prototypes
*/

hzEcode	TokenizeChain	(hzVect<hzToken>& Toklist, hzChain& C, hzTokMode eMode) ;
hzEcode	TokenizeString	(hzVect<hzToken>& Toklist, const char* pBuf, hzTokMode eMode) ;
hzEcode	TokenizeFile	(hzVect<hzToken>& Toklist, const char* filepath, hzTokMode eMode) ;

#endif	//	hzTokens_h
