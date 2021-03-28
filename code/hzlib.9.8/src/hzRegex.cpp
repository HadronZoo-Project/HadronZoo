//
//	File:	hzRegex.cpp
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
//	Impliments regular expression matches for filenames or other purpose
//

#include <iostream>

#include <stdarg.h>

#include "hzBasedefs.h"
#include "hzChars.h"
#include "hzTextproc.h"
#include "hzChain.h"
#include "hzDate.h"
#include "hzProcess.h"

/*
**	Functions
*/

static	bool	_checkformpart	(const char** test, const char** part)
{
	//	Check current part of test string matches part

	const char*	c ;			//	For processing ctrl string into parts
	const char*	s ;			//	Progressive ptr for test string

	for (c = *part, s = *test ;;)
	{
		if (*c == CHAR_QUERY)
			{ c++ ; s++ ; continue ; }

		if (*c == CHAR_SQOPEN)
		{
			if (memcmp(c, "[0-9]", 5) == 0 && *s >= '0' && *s <= '9')	{ c += 5 ; s++ ; continue ; }
			if (memcmp(c, "[a-z]", 5) == 0 && *s >= 'a' && *s <= 'z')	{ c += 5 ; s++ ; continue ; }
			if (memcmp(c, "[A-Z]", 5) == 0 && *s >= 'A' && *s <= 'Z')	{ c += 5 ; s++ ; continue ; }
		}

		if (*c == CHAR_ASTERISK)
		{
			*part = c ;
			*test = s ;
			return true ;
		}

		if (*c == 0 && *s == 0)
		{
			*part = c ;
			*test = s ;
			return true ;
		}

		if (*s == *c)
			{ s++ ; c++ ; continue ; }
		break ;
	}

	return false ;
}

bool	FormCheckCstr	(const char* cpTest, const char* cpCtrl)
{
	//	Category:	Regular Expression
	//
	//	Checks if a test string is of the form specified by a control string.
	//
	//	The method is aimed primarily at filtering filenames and is not intended as a formal regular expression interpreter. The following sequences appearing in the control string
	//	are treated as wildcards as follows:-
	//
	//		*		One or more consequtive asterisks match to a series of zero or more characters of any form
	//		?		Matches to one character
	//		[0-9]	Matches to any digit 0-9
	//		[a-z]	Matches to any lower case letter
	//		[A-Z]	Matches to any upper case letter
	//
	//	The control string is firstly broken into parts by an asterisk (or a series of consequtive asterisks), appearing anywhere within it. Each part either ends with an asterisk
	//	or a null terminator. If the control string starts with an asterisk, the first part (part 0) will just comprise the asterisk and be of zero length. Likewise if the control
	//	string ends with an asterisk, the last part will just comprise the asterisk and be of zero length. The test for a match is passed if the test string contains each part and
	//	in the order of appreance in the control string. The test string automatically matches a part of zero length.
	//
	//	Arguments:	1) cpTest	The test string
	//				2) cpCtrl	The control string
	//
	//	Returns:	True	If the test string is of the form specified by the control
	//				False	Otherwise.

	_hzfunc(__func__) ;

	const char*		c ;			//	For processing ctrl string into parts
	const char*		s ;			//	Progressive ptr for test string
	uint32_t		nParts ;	//	Total number of parts
	uint32_t		nChars ;	//	Total number of chars to be matched
	uint32_t		curPart ;	//	Current part
	bool			bMatch ;	//	Match by _checkformpart()

	//	If no control string there is no criteria so an automatic pass
	if (!cpCtrl || !cpCtrl[0])
		return true ;

	//	If no test string this is invalid so an automatic fail
	if (!cpTest || !cpTest[0])
		return false ;

	//	Break up the control string
	for (c = cpCtrl ; *c ;)
	{
		if (*c == CHAR_ASTERISK)
			{ nParts++ ; for (; *c == CHAR_ASTERISK ; c++) ; }
		else if (*c == CHAR_SQOPEN)
		{
			if (memcmp(c, "[0-9]", 5))	{ c += 5 ; nChars++ ; continue ; }
			if (memcmp(c, "[a-z]", 5))	{ c += 5 ; nChars++ ; continue ; }
			if (memcmp(c, "[A-Z]", 5))	{ c += 5 ; nChars++ ; continue ; }
		}
		else
			{ c++ ; nChars++ ; }
	}

	//	Process the CTRL string
	for (curPart = 0, c = cpCtrl, s = cpTest ; *c && *s ;)
	{
		bMatch = _checkformpart(&s, &c) ;
		if (!curPart && !bMatch)
			return false ;

		if (!bMatch)
			s++ ;
		else
		{
			if (*c == CHAR_ASTERISK)
			{
				//	Advance the control until no longer on an asterisk. Then look for the first occurence of the control char in the test
				curPart++ ;
				for (; *c == CHAR_ASTERISK ; c++) ;
			}
		}
	}

	if (*c == 0 && *s == 0)
		return true ;
	return false ;
}

uint32_t	FormCheckChain	(hzChain::Iter& ci, const char* cpCtrl)
{
	//	Category:	Regular Expression
	//
	//	This determines if the implied string (at the current supplied chain iterator) is of the supplied form
	//
	//	Arguments:	1)	ci		The input chain as chain iterator
	//				2)	cpCtrl	The control string
	//
	//	Returns:	Number of chars in the chain that comprise the construct of the supplied form

	_hzfunc(__func__) ;

	chIter		z ;			//	For processing ctrl string into parts
	const char*	c ;			//	Progressive ptr for test string
	uint32_t	len = 0 ;	//	Length of construct

	if (!cpCtrl || !cpCtrl[0])
		return true ;

	if (ci.eof())
		return false ;

	//	Process the CTRL string
	for (c = cpCtrl, z = ci ; *c && !z.eof() ;)
	{
		if (*c == CHAR_QUERY)
			{ c++ ; len++ ; z++ ; continue ; }

		if (*c == CHAR_SQOPEN)
		{
			if (memcmp(c, "[0-9]", 5) == 0 && *z >= '0' && *z <= '9')	{ c += 5 ; len++ ; z++ ; continue ; }
			if (memcmp(c, "[a-z]", 5) == 0 && *z >= 'a' && *z <= 'z')	{ c += 5 ; len++ ; z++ ; continue ; }
			if (memcmp(c, "[A-Z]", 5) == 0 && *z >= 'A' && *z <= 'Z')	{ c += 5 ; len++ ; z++ ; continue ; }
		}

		if (*c == CHAR_ASTERISK)
		{
			//	Advance the control until no longer on an asterisk. Then look for the first occurence of the control char in the test
			for (; *c == CHAR_ASTERISK ; c++) ;
			for (; !z.eof() && *z != *c ; len++, z++) ;
			continue ;
		}

		if (*c == *z)
			{ c++ ; len++ ; z++ ; continue ; }
		break ;
	}

	if (*c == 0)
		return len ;
	return 0 ;
}

/*
**	Filename or Glob Filtering with support for HadronZoo date forms
*/

struct	_psudoDate
{
	//	Non-compact date for lexical-date comparisons

	int16_t	Y ;			//	Year
	char	M ;			//	Month
	char	D ;			//	Day
	char	h ;			//	Hour
	char	m ;			//	Minute
	char	s ;			//	Second
	char	resv ;		//	Reserved

	_psudoDate	()	{ _clear() ; }

	void	_clear	()	{ Y = 0 ; M = D = h = m = s = resv = 0 ; }
} ;

bool	_set_psudo_date	(_psudoDate& pd, const char** test, const char** ctrl)
{
	//	Support function for FormCheckDate
	//
	//	Returns:	True	If the psudo date is set
	//				False	Otherwise.

	_hzfunc(__func__) ;

	const char*	t = *test ;
	const char*	c = *ctrl ;

	for (c++ ; *c && *t ;)
	{
		if (*c == '}')
			break ;

		if (c[0] == 'Y' && c[1] == 'Y' && c[2] == 'Y' && c[3] == 'Y' && IsDigit(t[0]) && IsDigit(t[1]) && IsDigit(t[2]) && IsDigit(t[3]))
			{ pd.Y = ((t[0]-'0')*1000) + ((t[1]-'0')*100) + ((t[2]-'0')*10) + (t[3]-'0') ; t += 4 ; c += 4 ; continue ; }

		if (c[0] == 'Y' && c[1] == 'Y' && IsDigit(t[0]) && IsDigit(t[1]))
			{ pd.Y = ((t[0]-'0')*10) + (t[1]-'0') + 2000 ; t += 2 ; c += 2 ; continue ; }

		if (c[0] == 'M' && c[1] == 'M' && IsDigit(t[0]) && IsDigit(t[1]))
		{
			pd.M = ((t[0]-'0')*10) + (t[1]-'0') ;
			if (pd.M > 0 && pd.M < 13)
				{ t += 2 ; c += 2 ; continue ; }
			return false ;
		}

		if (c[0] == 'D' && c[1] == 'D' && IsDigit(t[0]) && IsDigit(t[1]))
		{
			pd.D = ((t[0]-'0')*10) + (t[1]-'0') ;
			if (pd.D < 32)
				{ t += 2 ; c += 2 ; continue ; }
			return false ;
		}

		if (c[0] == 'h' && c[1] == 'h' && IsDigit(t[0]) && IsDigit(t[1]))
		{
			pd.h = ((t[0]-'0')*10) + (t[1]-'0') ;
			if (pd.h < 24)
				{ t += 2 ; c += 2 ; continue ; }
			return false ;
		}

		if (c[0] == 'm' && c[1] == 'm' && IsDigit(t[0]) && IsDigit(t[1]))
		{
			pd.m = ((t[0]-'0')*10) + (t[1]-'0') ;
			if (pd.m < 60)
				{ t += 2 ; c += 2 ; continue ; }
			return false ;
		}

		if (c[0] == 's' && c[1] == 's' && IsDigit(t[0]) && IsDigit(t[1]))
		{
			pd.s = ((t[0]-'0')*10) + (t[1]-'0') ;
			if (pd.s < 60)
				{ t += 2 ; c += 2 ; continue ; }
			return false ;
		}

		//	No other formats accepted yet
		return false ;
	}

	if (*c != '}')
		return false ;
	c++ ;

	*test = t ;
	*ctrl = c ;
	return true ;
}

bool	FormCheckDate	(hzXDate& xdate, const char* cpTest, const char* cpCtrl)
{
	//	Category:	Regular Expression
	//
	//	Purpose:	Check if a test string contains a date and is of the form implied by a control string
	//
	//	The control string is interpreted as a simplfied regular expression. The method is aimed primarily at filtering filenames which are expected to contain
	//	dates and is not intended as a formal regular expression interpreter. The ? will match to any one character and the * will match to a series of zero or
	//	more characters as per the standards. However treatment of the [] constructs is more limited.
	//
	//	Arguments:	1)	xdate	The hzXDate instance to be populated in the event of HadronZoo date forms in the control being met in the test.
	//				2)	cpTest	The test string
	//				3)	cpCtrl	The control or 'form' string
	//
	//	Returns:	True	If the test string is of the form specified in the control
	//				False	Otherwise. Note that this may be because the test string contains an invalid date or partial date.
	//

	_hzfunc(__func__) ;

	_psudoDate	pd ;		//	For showing that anticipated date or partial date in the test string are valid
	const char*	ctrl ;		//	For processing ctrl string into parts
	const char*	test ;		//	Progressive ptr for test string
	uint32_t	val ;		//	For Hex number read

	xdate.Clear() ;

	if (!cpCtrl || !cpCtrl[0])	return true ;
	if (!cpTest || !cpTest[0])	return false ;

	test = cpTest ;
	ctrl = cpCtrl ;

	//	Perform the test
	for (; *test && *ctrl ;)
	{
		if (*ctrl == '*')
		{
			//	Because we don't know how many chars this pertains to, we recursivly call this function in a loop. We start with test
			//	where it is and advance it one place for each call. The control string for all these calls is set to one place beyond
			//	the asterisk(s).

			for (; *ctrl == '*' ; ctrl++) ;

			for (; *test ; test++)
			{
				if (FormCheckDate(xdate, test, ctrl))
					return true ;
			}

			return false ;
		}

		if (*ctrl == '?')
			{ test++ ; ctrl++ ; continue ; }

		if (*ctrl == '{')
		{
			if (!_set_psudo_date(pd, &test, &ctrl))
				return false ;
			continue ;

		}

		if (*ctrl == CHAR_BKSLASH)
		{
			//	This will be treated only as a backslash if followed by another backslash
			if (ctrl[1] == CHAR_BKSLASH)
				if (*test == CHAR_BKSLASH)
					{ test++ ; ctrl += 2 ; continue ; }

			if (ctrl[1] == 'n')
				if (*test == CHAR_NL)
					{ test++ ; ctrl += 2 ; continue ; }

			//	Whitespace
			if (ctrl[1] == 's')
				if (*test == CHAR_CTRLL || *test == CHAR_NL || *test == CHAR_CR || *test == CHAR_TAB || *test == CHAR_CTRLK)
					{ test++ ; ctrl += 2 ; continue ; }

			//	Non white space
			if (ctrl[1] == 'S')
				if (*test > CHAR_SPACE)
					{ test++ ; ctrl += 2 ; continue ; }

			if (ctrl[1] == 'c')
			{
				//	\cx
				//	Matches the control character indicated by x. For example, \cM matches a Control-M or carriage return character.
				//	The value of x must be in the range of A-Z or a-z. If not, c is assumed to be a literal c character.
				if (ctrl[2] >= 'a' && ctrl[2] <= 'z')
					if (*test == (ctrl[2]-'a'))
						{ test++ ; ctrl += 3 ; continue ; }

				if (ctrl[2] >= 'A' && ctrl[2] <= 'Z')
					if (*test == (ctrl[2]-'A'))
						{ test++ ; ctrl += 3 ; continue ; }

				if (*test == 'c')
					{ test++ ; ctrl += 2 ; continue ; }
			}

			if (ctrl[1] == 'x')
				if (IsHexnum(val, ctrl+1))
					if (*test == (uchar) (val & 0xff))
						{ test++ ; ctrl += 4 ; continue ; }

			if (ctrl[1] == 't')
				if (*test == CHAR_TAB)
					{ test++ ; ctrl += 2 ; continue ; }

			//	Cope with vertical tab
			if (ctrl[1] == 'v')
				if (*test == CHAR_CTRLI)
					{ test++ ; ctrl += 2 ; continue ; }

			//	Cope with \f, \r, \n
			if (*test == ctrl[1])
				{ test++ ; ctrl += 2 ; continue ; }

			//	Drop thru and just test for the backslash
		}

		//	No we have ordinary char in ctrl
		if (*test == *ctrl)
			{ test++ ; ctrl++ ; continue ; }
		return false ;
		
	}

	if (pd.Y || pd.M || pd.D)
	{
		if ( pd.Y &&  pd.M &&  pd.D)
			xdate.SetDate(pd.Y, pd.M, pd.D) ;

		else if ( pd.Y &&  pd.M && !pd.D)
			xdate.SetDate(pd.Y, pd.M, 1) ;

		else if ( pd.Y && !pd.M &&  pd.D)	xdate.Clear() ;	//invalid
		else if ( pd.Y && !pd.M && !pd.D)	xdate.Clear() ;	//invalid

		else if (!pd.Y &&  pd.M &&  pd.D)
		{
			if (xdate.IsSet())
				pd.Y = xdate.Year() ;
			else
				{ xdate.SysDateTime() ; pd.Y = xdate.Year() ; }
			xdate.SetDate(pd.Y, pd.M, pd.D) ;
		}

		else if (!pd.Y &&  pd.M && !pd.D)	xdate.Clear() ;	//invalid
		else if (!pd.Y && !pd.M &&  pd.D)	xdate.Clear() ;	//invalid
		else if (!pd.Y && !pd.M && !pd.D)	xdate.Clear() ;	//invalid

		//	if (!pd.Y || !pd.M || !pd.D)
		//		xdate.SysDateTime() ;

		//	if (!pd.Y)	pd.Y = xdate.Year() ;
		//	if (!pd.M)	pd.M = xdate.Month() ;
		//	if (!pd.D)	pd.D = xdate.Day() ;

		//	xdate.SetDate(pd.Y, pd.M, pd.D) ;

		if (!xdate.IsSet())
			return false ;
	}

	return true ;
}

/*
**	Inline grep for statistical analysis etc
*/

hzEcode	Grep	(hzChain& Zo, hzChain& Zi, const char* exp)
{
	//	Category:	Regular Expression
	//
	//	Effect a grep on the supplied exp to the input chain. Place all matching lines in the output chain
	//
	//	Arguments:	1)	Zo	The output chain being a list of matching lines
	//			2)	Zi	The input chain
	//			3)	exp	The test expression
	//
	//	Returns:	E_ARGUMENT	If no search expression is supplied
	//				E_OK		In all other circumstances 

	_hzfunc(__func__) ;

	hzChain::Iter	zi ;	//	For iteration of the input
	hzChain			L ;		//	For isolating a line. This is then tested for the expression. If it passes, it is added as is to the output.
	hzString		S ;		//	Temp string

	Zo.Clear() ;
	if (!Zi.Size())
		return E_OK ;

	if (!exp || !exp[0])
		return E_ARGUMENT ;

	for (zi = Zi ; !zi.eof() ; zi++)
	{
		if (*zi != CHAR_NL)
			{ L.AddByte(*zi) ; continue ; }

		//	Now have a line in L
		S = L ;
		L.Clear() ;
		if (!FormCheckCstr(*S, exp))
			continue ;

		//	Line has passed so add
		Zo << S ;
		Zo.AddByte(CHAR_NL) ;
		L.Clear() ;
	}

	return E_OK ;
}
