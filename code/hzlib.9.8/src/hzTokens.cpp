//
//	File:	hzTokens.cpp
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

#include <fstream>

#include <sys/stat.h>

#include "hzChars.h"
#include "hzCtmpls.h"
#include "hzProcess.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzDatabase.h"
#include "hzTokens.h"

/*
**	Section 1:	Non Member Functions
*/

bool	_testInteger	(hzString& S, chIter& ci)
{
	//	Return true if the supplied chain iterator is at a digit. Gather all consecutive digits into the supplied string. As this is only for tokenization, the
	//	value of the integer is not calcuated.
	//
	//	Arguments:	1)	S	String to contain the set of contigous digits
	//				2)	ci	The chain iterator
	//
	//	Returns:	True	If the supplied iterator is at a digit
	//				False	Otherwise

	_hzfunc("_testInteger") ;

	hzChain		W ;		//	Working chain for gathering a complete word
	chIter		x ;		//	Internal copy of supplied chain iterator
	hzEcode		rc ;	//	Return code

	S.Clear() ;
	for (x = ci ; !x.eof() ; x++)
	{
		if (!(chartype[*x] & CTYPE_DIGIT))
			break ;
		W.AddByte(*x) ;
	}

	if (!W.Size())
		return false ;

	S = W ;
	ci += S.Length() ;
	return true ;
}

bool	_testNumber	(hzString& S, chIter& ci)
{
	//	Return true if the sequence at the current supplied chain iterator amounts to a number. Gather all consecutive digits into the supplied string. As this
	//	is only for tokenization, the value of the integer is not calcuated.
	//
	//	Arguments:	1)	S	String to contain the set of contigous digits
	//				2)	ci	The chain iterator
	//
	//	Returns:	True	If the supplied iterator is at the start of a valid number
	//				False	Otherwise

	hzChain		W ;				//	Working chain for gathering a complete word
	chIter		x ;				//	Internal copy of supplied chain iterator
	uint32_t	nDigits = 0 ;	//	Number of digits found
	uint32_t	nBytes = 0 ;	//	Number of bytes found

	x = ci ;

	//	Deal with leading sign
	if (*x == CHAR_MINUS || *x == CHAR_PLUS)
		{ W.AddByte(*x) ; nBytes++ ; x++ ; }

	//	Expect a series of at least one digit
	for (nDigits = 0 ; IsDigit(*x) ; nBytes++, nDigits++, x++)
		W.AddByte(*x) ;
	if (!nDigits)
		return false ;

	//	Test for a period that is followed by at least one digit
	if (*x == CHAR_PERIOD)
	{
		x++ ;
		W.AddByte(*x) ;
		for (nDigits = 0 ; IsDigit(*x) ; nBytes++, nDigits++, x++)
			W.AddByte(*x) ;
		if (!nDigits)
			return false ;
	}

	//	Test for the 'e' followed by at least one digit or a +/- followed by at least one digit
	if (*x == 'e')
	{
		x++ ;
		W.AddByte(*x) ;
		if (*x == CHAR_MINUS || *x == CHAR_PLUS)
			{ W.AddByte(*x) ; nBytes++ ; x++ ; }

		for (nDigits = 0 ; IsDigit(*x) ; nBytes++, nDigits++, x++)
			W.AddByte(*x) ;
		if (!nDigits)
			return false ;
	}

	if (!nBytes)
		return false ;

	ci = x ;
	S = W ;
	return true ;
}

bool	IsHexValue	(uint32_t& nLen, const char* pStr)
{
	//	Category:	Text Processing
	//
	//	Determine if supplied char string (arg 2) amounts to a hexadecimal number. This may optionally be preceeded by # or 0x. If the string is
	//	a hexadecimal number true is returned and the supplied uint32_t reference (arg 1) is set to the value.
	//
	//	Arguments:	1)	nLen	Set by the operation as the length of the hexadecimal number if found
	//				2)	pStr	The pointer into the test string
	//
	//	Returns:	True	If the supplied cstr amounts to a hexidecimal number
	//				False	Otherwise

	const char*	i = pStr ;		//	Input string iterator
	uint32_t	nBytes = 0 ;
	uint32_t	nHex = 0 ;

	nLen = 0 ;
	if (!i)
		return false ;

	if (*i == CHAR_HASH && IsHex(i[1]))
		{ nBytes++ ; i++ ; }

	if (*i == '0' && i[1] == 'x')
		{ nBytes += 2 ; i += 2 ; }

	for (; IsHex(*i) ; nHex++, nBytes++, i++) ;

	if (!nHex)
		return false ;

	nLen = nBytes ;
	return true ;
}

bool	_testHexnum	(hzString& S, chIter& ci)
{
	//	Arguments:	1)	S	Reference to string to hold discovered hex number
	//				2)	ci	Chain iterator of ongoing input
	//
	//	Returns:	True	If the supplied cstr amounts to a hexidecimal number
	//				False	Otherwise

	_hzfunc("IsHexValue") ;

	hzChain	W ;
	chIter	x = ci ;
	uint32_t	nSize = 0 ;

	if (x == CHAR_HASH)
		{ W.AddByte(*x) ; x++ ; }
	else if (x == "0x" || x == "0X")
		{ W << "0x" ; x += 2 ; }
	else
		return false ;

	for (; IsHex(*x) ; nSize++, x++)
		W.AddByte(*x) ;

	if (!W.Size())
		return false ;

	S = W ;
	ci = x ;
	return true ;
}

/*
**	Tokenize Fuctions
*/

hzEcode	TokenizeWords	(hzVect<hzToken>& toks, hzChain& C)
{
	//	Category:	Text Processing
	//
	//	Tokenize into words and numbers only, ignoring all punctuation. This is suitable for indexation of documents although it is not
	//	suitable for querying a document index as it does not produce boolean expressions of words.
	//
	//	Arguments:	1)	toks	The vector of tokens found in the input
	//				2)	C		The input chain
	//
	//	Returns:	E_NODATA	If the supplied chain is empty
	//				E_OK		If the supplied chain is tokenized

	_hzfunc("TokenizeWords") ;

	chIter		ci ;		//	For iteration of input
	hzChain		word ;		//	For building token
	hzToken		T ;			//	Token
	hzString	S ;			//	Temp string
	uint32_t	nLine ;		//	For assigning line numbers to tokens

	toks.Clear() ;
	if (!C.Size())
		return E_NODATA ;

	for (ci = C ; !ci.eof() ;)
	{
		if (*ci <= CHAR_SPACE)
			{ ci++ ; continue ; }

		if (*ci == CHAR_DQUOTE)
		{
			S = 0 ;
			word.Clear() ;
			nLine = ci.Line() ;

			for (ci++ ; !ci.eof() ; ci++)
			{
				if (*ci == CHAR_DQUOTE)
					{ ci++ ; break ; }

				word.AddByte(*ci) ;
			}

			if (word.Size())
			{
				S = word ;
				T.Init(S, nLine, TOKEN_ALPHANUM) ;
				toks.Add(T) ;
			}

			continue ;
		}

		if (ci == "/*")
		{
			for (ci += 2 ; !ci.eof() ; ci++)
			{
				if (ci == "*/")
					{ ci += 2 ; break ; }
			}
			continue ;
		}

		if (IsAlphanum(*ci))
		{
			S = 0 ;
			word.Clear() ;
			nLine = ci.Line() ;

			word.AddByte(*ci) ;
			for (ci++ ; !ci.eof() && IsAlphanum(*ci) ; ci++)
				word.AddByte(*ci) ;

			if (word.Size())
			{
				S = word ;
				T.Init(S, nLine, TOKEN_ALPHANUM) ;
				toks.Add(T) ;
			}

			continue ;
		}

		ci++ ;
	}

	return E_OK ;
}

hzEcode	TokenizeFreetext	(hzVect<hzToken>& toks, hzChain& C)
{
	//	Category:	Text Processing
	//
	//	Tokenize into words and numbers in accordance with the rules for freetext indexation. This, like the TokenizeWords() function,
	//	ignors all punctuation between words although it does process punctuation within words.
	//
	//	Words are first split on the basis of whitespace only (although trailing punctuation chars are ignored). This produces 'raw'
	//	words from which other words may be derived. As the raw words are built up, a count of each character type is kept. This helps
	//	drive the word derivation process. The output then consists of the raw word (if it survives) followed by any derived word.
	//
	//	Examples:-
	//	1)	$4million	-> dollars 4 million
	//	2)	bee-keeper	-> bee-keeper beekeeper bee keeper.
	//
	//	Arguments:	1)	toks	The vector of tokens found in the input
	//				2)	C		The input chain
	//
	//	Returns:	E_NODATA	If the supplied chain is empty
	//				E_OK		If the supplied chain is tokenized

	_hzfunc("TokenizeFreetext") ;

	hzChain		raw ;		//	For building raw word
	chIter		ci ;		//	For iteration of input
	chIter		ri ;		//	For iteration of raw word
	hzToken		T ;			//	Token
	hzString	word ;		//	Word (from raw)
	uint32_t	ucVal ;		//	Value of unicode char
	uint32_t	nPunct ;	//	Count of punctuation chars in raw word
	uint32_t	nSymb ;		//	Count of symbol chars in raw word
	uint32_t	nDigit ;	//	Count of digits in raw word
	uint32_t	nAlpha ;	//	Count of alphas in raw word
	uint32_t	nWeird ;	//	Count of alphas in raw word
	uint32_t	ucLen ;		//	Length of unicode char
	uint32_t	nLine ;		//	Line number (at start of raw sequence)

	toks.Clear() ;
	if (!C.Size())
		return E_NODATA ;

	ci = C ;
	ci.Line(1) ;
	for (; !ci.eof() ;)
	{
		//	Ignore leading spaces and other non alphnumerics
		if (!IsAlphanum(*ci))
			{ ci++ ; continue ; }

		//	Now we only have to deal with chars that are part of the raw word
		raw.Clear() ;
		nPunct = nSymb = nDigit = nAlpha = nWeird = 0 ;

		nLine = ci.Line() ;
		for (; !ci.eof() && IsAlphanum(*ci) ;)
		{
			if (IsPunct(*ci))
			{
				if (ci[1] <= CHAR_SPACE)
					{ ci++ ; break ; }

				nPunct++ ;
				raw.AddByte(*ci) ;
				ci++ ;
				continue ;
			}

			if (IsSymb(*ci))	{ nSymb++ ;		raw.AddByte(*ci) ;	ci++ ;	continue ; }
			if (IsDigit(*ci))	{ nDigit++ ;	raw.AddByte(*ci) ;	ci++ ;	continue ; }
			if (IsAlpha(*ci))	{ nAlpha++ ;	raw.AddByte(*ci) ;	ci++ ;	continue ; }

			//	Do we have a unicode sequence?
			if (AtUnicodeSeq(ucVal, ucLen, ci))
			{
				if (ucVal > 255)
					{ nWeird++ ; ci++ ; }
				else
					{ raw.AddByte(ucVal & 0xff) ; ci += ucLen ; }
				continue ;
			}

			raw.AddByte(*ci) ;
		}

		//	Now we have a raw word, do we add it to the token list as-is or do we spawn derivatives?

		if (!raw.Size())
			continue ;

		if (nAlpha)
		{
			if (!nDigit && !nPunct && !nSymb && !nWeird)
			{
				word = raw ;
				word.ToLower() ;
				T.Init(word, nLine, TOKEN_ALPHANUM) ;
				toks.Add(T) ;
			}

			//	Now must fill in what happens when we have digits, hyphens and what have you.
		}
	}

	return E_OK ;
}

hzEcode	TokenizeBool	(hzVect<hzToken>& toks, hzChain& C)
{
	//	Category:	Text Processing
	//
	//	Tokenize supplied chain into tokens expected to form a boolean expression
	//
	//	Arguments:	1)	toks	The vector of tokens found in the input
	//				2)	C		The input chain
	//
	//	Returns:	E_NODATA	If the supplied chain is empty
	//				E_OK		If the supplied chain is tokenized

	_hzfunc("hzTokenlist::TokenizeBool") ;

	hzChain		W ;				//	For building tokens
	chIter		ci ;			//	For iteration of input
	hzToken		T ;				//	Token
	hzString	S ;				//	For assembling the token value
	uint32_t	nLine ;			//	For assigning line numbers to tokens
	char		tmp	[4] ;		//	For operator

	toks.Clear() ;
	if (!C.Size())
		return E_NODATA ;

	ci = C ;
	nLine = 1 ;
	for (; !ci.eof() ;)
	{
		//	Increment line number when newlines are encountered
		if (*ci == CHAR_NL)
			{ nLine++ ; ci++ ; continue ; }

		//	Strip whitespace and other non-printable chars
		if (IsBinary(*ci) || IsWhite(*ci))
			{ ci++ ; continue ; }

		//	Eliminate comments
		if (ci == "/*")
		{
			for (ci += 2 ; !ci.eof() && ci != "*/" ; ci++) ;
			ci += 2 ;
			continue ;
		}

		if (ci == "//")
		{
			for (ci += 2 ; !ci.eof() && *ci != CHAR_NL ; ci++) ;
			ci++ ;
			continue ;
		}

		//	Assume we are at the start of a token - Check for quoted string
		if (*ci == CHAR_DQUOTE)
		{
			for (ci++ ; !ci.eof() && *ci != CHAR_DQUOTE ; ci++)
				W.AddByte(*ci) ;
			S = W ;
			W.Clear() ;
			T.Init(S, ci.Line(), TOKEN_STRING) ;
			toks.Add(T) ;
			ci++ ;
			continue ;
		}

		//	Check for valid hexadecimal value
		if (_testHexnum(S, ci))
		{
			T.Init(S, ci.Line(), TOKEN_STRING) ;
			toks.Add(T) ;
			continue ;
		}

		//	Check for integer
		if (_testInteger(S, ci))
		{
			T.Init(S, ci.Line(), TOKEN_INTEGER) ;
			toks.Add(T) ;
			continue ;
		}

		//	Check for number (std form)
		if (_testNumber(S, ci))
		{
			T.Init(S, ci.Line(), TOKEN_NUMBER) ;
			toks.Add(T) ;
			continue ;
		}

		//	Check for seperator
		if (IsPunct(*ci))
		{
			tmp[0] = *ci ;
			tmp[1] = 0 ;
			ci++ ;
			T.Init(tmp, nLine, TOKEN_SEPARATOR) ;
			toks.Add(T) ;
			continue ;
		}

		//	Check for operator
		if (IsSymb(*ci))
		{
			for (; !ci.eof() && IsSymb(*ci) ; ci++)
				W.AddByte(*ci) ;
			S = W ;
			W.Clear() ;
			T.Init(S, nLine, TOKEN_OPERATOR) ;
			toks.Add(T) ;
			continue ;
		}

		//	Not an operator or separator - must be general entitiy
		//	We have a rule that productions must be written out
		//	in full. E.g. 2X must be written as 2*X. The system
		//	will interpret 2X as an alpha numeric quantity

		for (; !ci.eof() && IsAlphanum(*ci) ; ci++)
			W.AddByte(*ci) ;
		S = W ;
		W.Clear() ;
		T.Init(S, nLine, TOKEN_OPERATOR) ;
		toks.Add(T) ;
	}

	return E_OK ;
}

/*
**	Application level Tokenization Functions
*/

hzEcode	TokenizeChain	(hzVect<hzToken>& toks, hzChain& C, hzTokMode eMode)
{
	//	Category:	Text Processing
	//
	//	Populate supplied vector of tokens (arg 1) by tokenizing the supplied chain (arg 2) according to the modus operandi specified by arg 3
	//
	//	Arguments:	1)	toks	The vector of tokens found in the input
	//				2)	C		The input chain
	//				3)	eMode	The tokenization regime (either WHITE, FTEXT or BOOL)
	//
	//	Returns:	E_NODATA	If the supplied chain is empty
	//				E_RANGE		If the supplied mode is invalid
	//				E_OK		If the supplied chain is tokenized

	switch	(eMode)
	{
	case TOK_MO_WHITE:	return TokenizeWords(toks, C) ;
	case TOK_MO_FTEXT:	return TokenizeFreetext(toks, C) ;
	case TOK_MO_BOOL:	return TokenizeBool(toks, C) ;
	}

	return E_RANGE ;
}

hzEcode	TokenizeString	(hzVect<hzToken>& toks, const char* pBuf, hzTokMode eMode)
{
	//	Category:	Text Processing
	//
	//	Populate supplied vector of tokens (arg 1) by tokenizing the supplied char string (arg 2) according to the modus operandi specified by
	//	arg 3
	//
	//	Arguments:	1)	toks	The vector of tokens found in the input
	//				2)	C		The input chain
	//				3)	eMode	The tokenization regime (either WHITE, FTEXT or BOOL)
	//
	//	Returns:	E_NODATA	If the supplied chain is empty
	//				E_RANGE		If the supplied mode is invalid
	//				E_OK		If the supplied chain is tokenized

	_hzfunc("TokenizeString") ;

	hzChain	C ;		//	Working chain

	C = pBuf ;

	switch	(eMode)
	{
	case TOK_MO_WHITE:	return TokenizeWords(toks, C) ;
	case TOK_MO_FTEXT:	return TokenizeFreetext(toks, C) ;
	case TOK_MO_BOOL:	return TokenizeBool(toks, C) ;
	}

	return E_RANGE ;
}

hzEcode	TokenizeFile	(hzVect<hzToken>& toks, const char* fname, hzTokMode eMode)
{
	//	Category:	Text Processing
	//
	//	Populate supplied vector of tokens (arg 1) by tokenizing the supplied file (named in arg 2) according to the modus operandi specified by
	//	arg 3
	//
	//	Arguments:	1)	toks	The vector of tokens found in the input
	//				2)	fname	The input filename
	//				3)	eMode	The tokenization regime (either WHITE, FTEXT or BOOL)
	//
	//	Returns:	E_NODATA	If the supplied chain is empty
	//				E_RANGE		If the supplied mode is invalid
	//				E_OK		If the supplied chain is tokenized

	_hzfunc("TokenizeFile") ;

	/*
	**	Convert file into tokens
	*/

	std::ifstream	is ;	//	Input stream
	FSTAT			fs ;	//	File status
	hzChain			C ;		//	Working chain

	if (!fname || !fname[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Cannot tokenize unnamed file") ;

	if (stat(fname, &fs) == -1)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "File (%s) does not exist") ;

	is.open(fname) ;
	if (is.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "File %s", fname) ;

	C << is ;
	is.close() ;

	switch	(eMode)
	{
	case TOK_MO_WHITE:	return TokenizeWords(toks, C) ;
	case TOK_MO_FTEXT:	return TokenizeFreetext(toks, C) ;
	case TOK_MO_BOOL:	return TokenizeBool(toks, C) ;
	}

	return E_RANGE ;
}
