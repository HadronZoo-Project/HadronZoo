//
//	File:	hzEmaddr.cpp
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
//	Implimentation of the hzEmaddr (email address) class.
//

#include <iostream>

#include <stdarg.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzProcess.h"

/*
**	Definitions
*/

struct	_ema_space
{
	//	Internal structure for email address stringspace. Note there is no constructor as string spaces are allocated by _strAlloc() from superblocks.

	uchar	m_copy ;		//	Copy counter
	uchar	m_lhs ;			//	Length left of the '@'
	uchar	m_rhs ;			//	Length right of the '@'
	char	m_data[5] ;		//	First part of data
} ;

#define EMA_FACTOR	4		//	This is added to the email string size to accomodate the copy count and the size of the strings to the left and right of the @,
							//	the @ itself and the null terminator.

/*
**	Prototypes
*/

uint32_t	_strAlloc	(uint32_t size) ;
void		_strFree	(uint32_t obj, uint32_t size) ;
void		_strTest	(uint32_t obj, uint32_t size) ;
void*		_strXlate	(uint32_t addr) ;

/*
**	Global constants
*/

global	const hzEmaddr	_hz_null_hzEmaddr ;		//	Null email address

/*
**	hzEmaddr public methods
*/

void	hzEmaddr::Clear	(void)
{
	//	Clear the contents
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzEmaddr::Clear") ;

	_ema_space*	thisCtl ;		//	This email address space
	uint32_t	nLen ;			//	Length

    if (m_addr)
    {
		thisCtl = (_ema_space*) _strXlate(m_addr) ;
		if (!thisCtl)
			hzexit(_fn, 0, E_CORRUPT, "Illegal string addressn %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

		nLen = thisCtl->m_lhs + thisCtl->m_rhs + 1 ;
		if (!nLen)
			hzexit(_fn, 0, E_CORRUPT, "Zero length email address %u:%u", (m_addr&0x7fff0000)>>16, m_addr&0xffff) ;

        if (_hzGlobal_MT)
        {
            __sync_add_and_fetch(&(thisCtl->m_copy), -1) ;

            if (!thisCtl->m_copy)
				_strFree(m_addr, nLen + EMA_FACTOR) ;
        }
        else
        {
            thisCtl->m_copy-- ;
            if (!thisCtl->m_copy)
				_strFree(m_addr, nLen + EMA_FACTOR) ;
        }

        m_addr = 0 ;
    }
}

hzString	hzEmaddr::Str	(void) const
{
	//	Create and populate a hzString with the email address value. Return this hzString by value.
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString by value being email address

	_hzfunc("hzEmaddr::Str") ;

	_ema_space*	thisCtl ;	//	This email address space
	hzString	S ;			//	Email address as string

	if (m_addr)
	{
		thisCtl = (_ema_space*) _strXlate(m_addr) ;
		if (!thisCtl)
			threadLog("%s: Ivalid string space %u:%u\n", *_fn, (m_addr&0x7fff000)>>16, m_addr&0xffff) ;
		else
			S = (char*) thisCtl->m_data ;
	}

	return S ;
}

uint32_t	hzEmaddr::Length	(void) const
{
	//	Return length in bytes of the whole email address

	_hzfunc("hzEmaddr::Length") ;

	_ema_space*	thisCtl ;	//	This email address space

	if (!m_addr)
		return 0 ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;
	if (!thisCtl)
		{ threadLog("%s: Ivalid string space %u:%u\n", *_fn, (m_addr&0x7fff000)>>16, m_addr&0xffff) ; return 0 ; }

	return thisCtl->m_lhs + thisCtl->m_rhs + 1 ;
}

uint32_t	hzEmaddr::LhsLen	(void) const
{
	//	Return length in bytes of the first part email address

	_hzfunc("hzEmaddr::LhsLen") ;

	_ema_space*	thisCtl ;	//	This email address space

	if (!m_addr)
		return 0 ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;
	if (!thisCtl)
		{ threadLog("%s: Ivalid string space %u:%u\n", *_fn, (m_addr&0x7fff000)>>16, m_addr&0xffff) ; return 0 ; }

	return thisCtl->m_lhs ;
}

uint32_t	hzEmaddr::DomLen	(void) const
{
	//	Return length in bytes of the domain part of the email address

	_hzfunc("hzEmaddr::DomLen") ;

	_ema_space*	thisCtl ;	//	This email address space

	if (!m_addr)
		return 0 ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;
	if (!thisCtl)
		{ threadLog("%s: Ivalid string space %u:%u\n", *_fn, (m_addr&0x7fff000)>>16, m_addr&0xffff) ; return 0 ; }

	return thisCtl->m_rhs ;
}

const char*	hzEmaddr::GetDomain	(void) const
{
	//	Return domain name as null terminated string

	_hzfunc("hzEmaddr::GetDomain") ;

	_ema_space*	thisCtl ;	//	This email address space

	if (!m_addr)
		return 0 ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;
	if (!thisCtl)
		{ threadLog("%s: Ivalid string space %u:%u\n", *_fn, (m_addr&0x7fff000)>>16, m_addr&0xffff) ; return 0 ; }

	return thisCtl->m_data + thisCtl->m_lhs + 1 ;
}

const char*	hzEmaddr::GetAddress	(void) const
{
	//	Return whole email address as null terminated string

	_hzfunc("hzEmaddr::GetAddress") ;

	_ema_space*	thisCtl ;	//	This email address space

	if (!m_addr)
		return 0 ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;
	if (!thisCtl)
		{ threadLog("%s: Ivalid string space %u:%u\n", *_fn, (m_addr&0x7fff000)>>16, m_addr&0xffff) ; return 0 ; }

	return thisCtl->m_data ;
}

hzEmaddr&	hzEmaddr::operator=	(const char* cpStr)
{
	//	Assign the hzEmaddr to an email address held in a character string
	//
	//	Argument:	cpStr	A null terminated string assumed to be an email address
	//
	//	Returns:	Reference to this email adress instance in all cases.
	//
	//	Note: This function will record an E_FORMAT error if the supplied cstr did not amount to an email address

	_hzfunc("hzEmaddr::operator=(cstr)") ;

	_ema_space*	destCtl ;		//	This email address space
	const char*	i ;				//	Email iterator
	char*		j ;				//	Email iterator
	uint32_t	nLhs = 0 ;		//	LHS part of email address
	uint32_t	nRhs = 0 ;		//	RHS part of email address

	Clear() ;

	if (!cpStr || !cpStr[0])
		return *this ;

	if (!IsEmaddr(cpStr))
	{
		hzerr(_fn, HZ_ERROR, E_FORMAT, "Cannot assign %s", cpStr) ;
		return *this ;
	}

	for (i = cpStr ; *i && *i != '@' ; nLhs++, i++) ;
	if (*i == '@')
		for (i++ ; *i ; nRhs++, i++) ;

	if (nLhs < 1 || nLhs > 192 || nRhs < 1 || nRhs > 63)
	{
		hzerr(_fn, HZ_ERROR, E_FORMAT, "Cannot assign %s", cpStr) ;
		return *this ;
	}

	m_addr = _strAlloc(nLhs + nRhs + 1 + EMA_FACTOR) ;
	if (!m_addr)
		hzexit(_fn, 0, E_MEMORY, "Cannot assign %s", cpStr) ;
	destCtl = (_ema_space*) _strXlate(m_addr) ;

	//	Assign the value
	destCtl->m_copy = 1 ;
	destCtl->m_lhs = nLhs & 0xff ;
	destCtl->m_rhs = nRhs & 0xff ;
	for (j = destCtl->m_data, i = cpStr ; *i ; *j++ = *i++) ;
	*j = 0 ;
	return *this ;
}

hzEmaddr&	hzEmaddr::operator=	(const hzString& S)
{
	//	Assign the hzEmaddr to an email address held in a hzString instance
	//
	//	Argument:	S	A string assumed to be an email address
	//
	//	Returns:	Reference to this email adress instance in all cases.
	//
	//	Note: This function will record an E_FORMAT error if the supplied cstr did not amount to an email address

	_hzfunc("hzEmaddr::operator=(hzStr)") ;

	Clear() ;
	return operator=(*S) ;
}

hzEmaddr&	hzEmaddr::operator=	(hzChain::Iter& ci)
{
	//	Determines if the supplied iterator is at the start of a valid email address and if it is, assignes this as the value to the calling instance.
	//
	//	Argument:	ci	Chain iterator
	//
	//	Returns:	Reference to this email address instance. This will be empty if the input did not amount to an email address

	_hzfunc("hzEmaddr::operator=(chIter)") ;

	hzChain::Iter	xi ;		//	External chain iterator

	_ema_space*	thisCtl ;		//	This email address space
	char*		i ;				//	For populating string
	int32_t		at = 0 ;		//	The @ has been encountered (later used as a counter)
	uchar		nLhs = 0 ;		//	Chars before the @
	uchar		nRhs = 0 ;		//	Chars after the @
	uchar		nPeriod = 0 ;	//	No of periods on RHS
	char		lCh = 0 ;		//	Last char processed

	Clear() ;
	if (ci.eof())
		return *this ;

	//	Process the string and set char incidence aggregates
	for (xi = ci ; !xi.eof() && *xi > CHAR_SPACE ; lCh = *xi, xi++)
	{
		if (*xi == CHAR_AT)
		{
			if (at)
				return *this ;
			if (!lCh || lCh == CHAR_PERIOD)
				return *this ;
			at = 1 ;
			continue ;
		}

		if (*xi == CHAR_PERIOD)
		{
			if (!lCh || lCh == CHAR_PERIOD || lCh == CHAR_AT)
				return *this ;
			if (at)
				{ nRhs++ ; nPeriod++ ; }
			else
				nLhs++ ;
			continue ;
		}

		if (at)
		{
			if (IsUrlnorm(*xi))
			{
				nRhs++ ;
				if (nRhs > 63)
					return *this ;
				continue ;
			}
		}
		else
		{
			if (IsUrlnorm(*xi) || *xi=='!' || *xi=='#' || *xi=='$' || *xi=='%' || *xi=='&' || *xi=='\'' || *xi=='*' || *xi=='+'
				|| *xi=='/' || *xi=='=' || *xi=='?' || *xi=='^' || *xi=='`' || *xi=='{' || *xi=='|' || *xi=='}' || *xi=='~' || *xi=='.')
			{
				nLhs++ ;
				if (nLhs > 192)
					return *this ;
				continue ;
			}
		}

		break ;
	}

	if (lCh == CHAR_PERIOD || !at || !nPeriod || !nLhs || nRhs < 2)
	{
		hzerr(_fn, HZ_ERROR, E_FORMAT, "Cannot assign") ;
		return *this ;
	}

	if (lCh == CHAR_PERIOD)	return *this ;
	if (!at)				return *this ;
	if (!nPeriod)			return *this ;
	if (!nLhs || nRhs < 2)	return *this ;

	//m_cpBuf = new uchar[nLhs + nRhs + 5] ;
	m_addr = _strAlloc(nLhs + nRhs + 1 + EMA_FACTOR) ;
	if (!m_addr)
		hzexit(_fn, 0, E_MEMORY, "Cannot assign %d bytes", nLhs + nRhs + 5) ;
	thisCtl = (_ema_space*) _strXlate(m_addr) ;

	i = (char*) thisCtl->m_data ;

	for (xi = ci, at = 0 ; at < (nLhs + nRhs + 1) ; at++, xi++)
		i[at] = *xi ;
	i[at] = 0 ;

	thisCtl->m_copy = 1 ;
	thisCtl->m_lhs = nLhs ;
	thisCtl->m_rhs = nRhs ;

	return *this ;
}

hzEmaddr&	hzEmaddr::operator=	(const hzEmaddr& E)
{
	//	Assign the hzEmaddr to an email address held in another hzEmaddr instance
	//
	//	Arguments:	1)	E	The supplied email address as a hzEmail instance
	//
	//	Returns:	Reference to this hzEmail instance

	//	It this internal pointer and that of the operand already point to the same space in memory, do nothing

	_hzfunc("hzEmaddr::operator=") ;

	_ema_space*	suppCtl ;		//	Supplied email address space

    if (m_addr == E.m_addr)
        return *this ;

	Clear() ;

	if (E.m_addr)
	{
		//_copyadd() ;
		suppCtl = (_ema_space*) _strXlate(E.m_addr) ;

    	if (_hzGlobal_MT)
        	__sync_add_and_fetch(&(suppCtl->m_copy), 1) ;
    	else
        	suppCtl->m_copy++ ;

		m_addr = E.m_addr ;
	}

	return *this ;
}

/*
**	Compare operators
*/

static	int32_t	_lhscompare	(const char* a, const char* b)
{
	//	Compare only upto the @ in what is assumend to be two email addresses
	//
	//	Arguments:	1)	a	First email address
	//				2)	b	Second email address
	//
	//	Returns:	-1	If the LHS of a < LHS of b
	//				+1	If the LHS of a > LHS of b
	//				0	If the LHS of a = LSH of b

	_hzfunc(__func__) ;

	for (; *a && *a != '@' && *a == *b ; a++, b++) ;

	if (*a == '@' || *a == 0)
	{
		if (*b == '@' || *b == 0)
			return 0 ;
		return -1 ;
	}

	if (*b == '@' || *b == 0)
		return 1 ;

	return *a > *b ? 1 : -1 ;
}

bool	hzEmaddr::operator==	(const hzEmaddr& E) const
{
	//	Test for equality between this hzEmaddr and an operand instance
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this addesss is equal to the supplied test email address
	//				False	Otherwise

	_hzfunc("hzEmaddr::operator==") ;

	_ema_space*	thisCtl ;		//	This email address space
	_ema_space*	suppCtl ;		//	Supplied email address space

	if (m_addr == E.m_addr)			return true ;
	if (m_addr && E.m_addr == 0)	return false ;
	if (E.m_addr && m_addr == 0)	return false ;

	//	Compare domains first then LHS
	thisCtl = (_ema_space*) _strXlate(m_addr) ;
	suppCtl = (_ema_space*) _strXlate(E.m_addr) ;

	if (!strcmp(thisCtl->m_data + thisCtl->m_lhs + 1, suppCtl->m_data + suppCtl->m_lhs + 1))
	{
		if (!_lhscompare((char*) thisCtl->m_data, (char*) suppCtl->m_data))
			return true ;
	}

	return false ;
}

bool	hzEmaddr::operator<		(const hzEmaddr& E) const
{
	//	Return true if this hzEmaddr instance is lexically less than the operand. Note that comparison is first done on the domain part (the RHS
	//	of the @) and then done on the address part (the LHS of the @)
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this addesss is lexically less than the supplied test email address
	//				False	Otherwise

	_hzfunc("hzEmaddr::operator<") ;

	int32_t	res ;

	//	Are this and the operand connected?
	if (m_addr == E.m_addr)			return false ;
	if (m_addr && E.m_addr == 0)	return false ;
	if (E.m_addr && m_addr == 0)	return true ;

	//	Compare domains first
	res = strcmp(GetDomain(), E.GetDomain()) ;
	if (res < 0)
		return true ;
	if (res > 0)
		return false ;

	//	Domains are equal so compare LHS
	res = _lhscompare(GetAddress(), *E) ;
	return res < 0 ? true : false ;
}

bool	hzEmaddr::operator<=	(const hzEmaddr& E) const
{
	//	Return true if this hzEmaddr instance is lexically less than or equal to the operand. Note that comparison is first done on the domain
	//	part (the RHS of the @) and then done on the address part (the LHS of the @)
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this addesss is lexically less than or equal to the supplied test email address
	//				False	Otherwise

	_hzfunc("hzEmaddr::operator<=") ;

	int32_t		res ;		//	Comparison result

	//	Are this and the operand connected?
	if (m_addr == E.m_addr)			return true ;
	if (m_addr && E.m_addr == 0)	return false ;
	if (E.m_addr && m_addr == 0)	return true ;

	//	Compare domains first
	res = strcmp(GetDomain(), E.GetDomain()) ;
	if (res < 0)
		return true ;
	if (res > 0)
		return false ;

	//	Domains are equal so compare LHS
	res = _lhscompare(GetAddress(), E) ;
	return res <= 0 ? true : false ;
}

bool	hzEmaddr::operator>		(const hzEmaddr& E) const
{
	//	Return true if this hzEmaddr instance is lexically greater than the operand. Note that comparison is first done on the domain
	//	part (the RHS of the @) and then done on the address part (the LHS of the @)
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this addesss is lexically greater than the supplied test email address
	//				False	Otherwise

	_hzfunc("hzEmaddr::operator>") ;

	int32_t		res ;		//	Comparison result

	//	Are this and the operand connected?
	if (m_addr == E.m_addr)			return false ;
	if (m_addr && E.m_addr == 0)	return true ;
	if (E.m_addr && m_addr == 0)	return false ;

	//	Compare domains first
	res = strcmp(GetDomain(), E.GetDomain()) ;
	if (res > 0)
		return true ;
	if (res < 0)
		return false ;

	//	Domains are equal so compare LHS
	res = _lhscompare(GetAddress(), E) ;
	return res > 0 ? true : false ;
}

bool	hzEmaddr::operator>=	(const hzEmaddr& E) const
{
	//	Return true if this hzEmaddr instance is lexically greater than or equal to the operand. Note that comparison is first done on the domain
	//	part (the RHS of the @) and then done on the address part (the LHS of the @)
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this addesss is lexically greater than or equal to the supplied test email address
	//				False	Otherwise

	_hzfunc("hzEmaddr::operator>=") ;

	int32_t		res ;		//	Comparison result

	//	Are this and the operand connected?
	if (m_addr == E.m_addr)			return true ;
	if (m_addr && E.m_addr == 0)	return true ;
	if (E.m_addr && m_addr == 0)	return false ;

	//	Compare domains first
	res = strcmp(GetDomain(), E.GetDomain()) ;
	if (res > 0)
		return true ;
	if (res < 0)
		return false ;

	//	Domains are equal so compare LHS
	res = _lhscompare(GetAddress(), E) ;
	return res >= 0 ? true : false ;
}

bool	hzEmaddr::operator==	(const hzString& S) const
{
	//	Test for equality between this hzEmaddr and an email address held in a hzString
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this addesss is lexically equal to the supplied test email address
	//				False	Otherwise

	_hzfunc("hzEmaddr::operator==") ;

	_ema_space*	thisCtl ;		//	This email address space

	if (!S && !m_addr)	return true ;
	if (!S)				return false ;
	if (!m_addr)		return false ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;

	return CstrCompare((char*) thisCtl->m_data, *S) == 0 ? true : false ;
}

bool	hzEmaddr::operator!=	(const hzString& S) const
{
	//	Test for inequality between this hzEmaddr and an email address held in a hzString
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this addesss is not lexically equal to the supplied test email address
	//				False	Otherwise

	_hzfunc("hzEmaddr::operator!=") ;

	_ema_space*	thisCtl ;		//	This email address space

	if (!S && !m_addr)	return false ;
	if (!S)				return true ;
	if (!m_addr)		return true ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;

	return CstrCompare((char*) thisCtl->m_data, *S) == 0 ? false : true ;
}

bool	hzEmaddr::operator==	(const char* cpStr) const
{
	//	Test for equality between this hzEmaddr and an email address held in a character string
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this addesss is lexically equal to the supplied test email address
	//				False	Otherwise

	_hzfunc("hzEmaddr::operator==") ;

	_ema_space*	thisCtl ;		//	This email address space

	if ((!cpStr || !cpStr[0]) && !m_addr)
		return true ;
	if (!cpStr || !cpStr[0])
		return false ;
	if (!m_addr)
		return false ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;

	return CstrCompare((char*) thisCtl->m_data, cpStr) == 0 ? true : false ;
}

bool	hzEmaddr::operator!=	(const char* cpStr) const
{
	//	Test for inequality between this hzEmaddr and an email address held in a character string
	//
	//	Arguments:	1)	E	Test email address
	//
	//	Returns:	True	If this email addesss is not lexically equal to the supplied
	//				False	If this email address has the same value as the supplied

	_hzfunc("hzEmaddr::operator!=") ;

	_ema_space*	thisCtl ;		//	This email address space

	if ((!cpStr || !cpStr[0]) && !m_addr)
		return false ;
	if (!cpStr || !cpStr[0])
		return true ;
	if (!m_addr)
		return true ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;

	return CstrCompare((char*) thisCtl->m_data, cpStr) == 0 ? false:true ;
}

const char*	hzEmaddr::operator*	(void) const
{
	//	Returns the URL data (a null terminated string)
	//
	//	Arguments:	None
	//	Returns:	Content as null terminated string

	_hzfunc("hzEmaddr::operator*") ;

	_ema_space*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return 0 ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;
	return thisCtl->m_data ;
}

hzEmaddr::operator const char*    (void) const
{
	//	Returns the string data (a null terminated string)
	//
	//	Arguments:	None
	//	Returns:	Content as null terminated string

	_hzfunc("hzEmaddr::operator const char*") ;

	_ema_space*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return 0 ;

	thisCtl = (_ema_space*) _strXlate(m_addr) ;
	return thisCtl->m_data ;
}

/*
**	Stream operator
*/

std::ostream&	operator<<	(std::ostream& os, const hzEmaddr& obj)
{
	//	Category:	Data Output
	//
	//	Friend function to hzEmaddr class to stream out the email address.
	//
	//	Arguments:	1)	os		Reference to output stream
	//				2)	ema		Const reference to an email address
	//
	//	Returns:	Reference to the supplied output stream

	_hzfunc("operator<<(ostream,hzEmaddr)") ;

	os << *obj ;
	return os ;
}

bool	IsEmaddr	(const char* cpStr)
{
	//	Category:	Text Processing
	//
	//	Tests if a supplied string (or text at a hzChain iterator) is of the form of an email address. To qualify, there must be a single occurence
	//	of '@' both preceeded and followed by strings of non-zero length whose characters are members of a set of permitted characters (see below).
	//	There also must be at least one period in the string following the @. Additionally the length of the whole must not exceed 255.
	//
	//	The first instance of a whitespace character (<= space) terminates the test. If the string up to but not including the whitespace character
	//	or null terminator, then the test is passed, otherwise it fails.
	//
	//	Permitted chars on the LHS of the '@' are: [a-z], [A-Z], [0-9], [!, #, $, %, &, ', *, +, -, /, =, ?, ^, _, `, {, |, }, ~] and the period (.)
	//	if it is not the last character in the string.
	//
	//	Permitted chars on the LHS of the '@' are: [a-z], [A-Z], [0-9] and the period (.) or minus sign (-) if they are not the last character in the
	//	string.
	//
	//	Arguments:	1)	cpStr	The char pointer to be tested
	//
	//	Returns:	True	If the supplied cstr sets the email address
	//				False	Otherwise

	_hzfunc("IsEmaddr") ;

	const char*	i ;					//	Source string iterator
	uint32_t	nLhs = 0 ;			//	Count to the left of the @
	uint32_t	nRhs = 0 ;			//	Count to the right of the @
	bool		bPeriod = false ;	//	Confirmed period

	if (!cpStr || !cpStr[0])
		return false ;

	//	Count chars upto the @
	for (i = cpStr ; *i > CHAR_SPACE ; i++)
	{
		if (*i == CHAR_AT)
			break ;

		if ((*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z') || (*i >= '0' && *i <= '9')
			|| *i=='!' || *i=='#' || *i=='$' || *i=='%' || *i=='&' || *i=='\'' || *i=='*' || *i=='+' || *i=='-'
			|| *i=='/' || *i=='=' || *i=='?' || *i=='^' || *i=='_' || *i=='`' || *i=='{' || *i=='|' || *i=='}' || *i=='~'
			|| (*i=='.' && i[1] > ' ' && i[1] != '@' && nLhs))
		{
			nLhs++ ;
			if (nLhs > 63)
				return false ;
			continue ;
		}

		break ;
	}

	if (*i != CHAR_AT || !nLhs)
		return false ;

	//	Count chars beyond @
	for (i++ ; *i > CHAR_SPACE ; i++)
	{
		if (*i == CHAR_AT)
			return false ;

		if (*i == CHAR_PERIOD && i[1] && i[1] != CHAR_PERIOD)
			{ bPeriod = true ; continue ; }

		if ((*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z') || (*i >= '0' && *i <= '9')
			|| (*i=='-' && i[1] > ' ' && nRhs)
			|| (*i=='.' && i[1] > ' ' && nRhs))
		{
			nRhs++ ;
			if (nRhs > 192)
				return false ;
			continue ;
		}

		break ;
	}

	if (bPeriod && nLhs && nRhs > 2)
		return true ;
	return false ;
}

bool	AtEmaddr	(hzEmaddr& ema, uint32_t& nLen, hzChain::Iter& ci)
{
	//	Category:	Text Processing
	//
	//	If the supplied iterator is at the start of a valid email address, then the supplied email address instance will be populated with this
	//	address as the value and the supplied length will be set.
	//
	//	Arguments:	1)	ema		Reference to an email address, populated by this function
	//				2)	nLen	Reference to an integer set to the email length
	//				3)	ci		Input chain iterator
	//
	//	Returns:	True	If the supplied cstr sets the email address
	//				False	Otherwise

	_hzfunc("AtEmaddr") ;

	ema.Clear() ;
	ema = ci ;
	if (!ema)
		return false ;
	nLen = ema.Length() ;
	return true ;
}
