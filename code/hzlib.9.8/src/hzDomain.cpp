//
//	File:	hzDomain.cpp
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
//	Implimentation of the hzDomain (domain name) class.
//

#include <iostream>

#include <stdarg.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzProcess.h"

/*
**	Definitions
*/

struct	_dom_space
{
	//	Internal structure for domain name stringspace. Note there is no constructor as string spaces are allocated by _strAlloc() from superblocks.

	uchar	m_copy ;		//	Copy counter
	uchar	m_len ;			//	Length
	uchar	m_tld ;			//	Offset to top-level domain (right of the last period)
	char	m_data[5] ;		//	First part of data
} ;

#define DOM_FACTOR	4		//	This is added to the domain string size to accomodate the copy count and the size of the domain name string plus the null terminator.

/*
**	Prototypes
*/

uint32_t	_strAlloc	(uint32_t size) ;
void		_strFree	(uint32_t obj, uint32_t size) ;
void*		_strXlate	(uint32_t addr) ;

/*
**	Global constants
*/

global	const hzDomain	_hz_null_hzDomain ;		//	Null domain name

/*
**	hzDomain public methods
*/

void	hzDomain::Clear	(void)
{
	//	Clear the contents
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzDomain::Clear") ;

	_dom_space*	thisCtl ;		//	This domain name space

    if (m_addr)
    {
		thisCtl = (_dom_space*) _strXlate(m_addr) ;
		if (!thisCtl)
			hzexit(_fn, 0, E_CORRUPT, "Illegal domain address %u:%u", (m_addr&0x7ffff0000)>>16, m_addr&0xffff) ;

		if (!thisCtl->m_len)
			hzexit(_fn, 0, E_CORRUPT, "Zero domain length %u:%u", (m_addr&0x7ffff0000)>>16, m_addr&0xffff) ;

        if (_hzGlobal_MT)
        {
            __sync_add_and_fetch(&(thisCtl->m_copy), -1) ;

            if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->m_len + DOM_FACTOR) ;
        }
        else
        {
            thisCtl->m_copy-- ;
            if (!thisCtl->m_copy)
				_strFree(m_addr, thisCtl->m_len + DOM_FACTOR) ;
        }

        m_addr = 0 ;
    }
}

hzString	hzDomain::Str	(void) const
{
	//	Create and populate a hzString with the domain name value. Return this hzString by value.
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString by value being domain name

	_hzfunc("hzDomain::Str") ;

	_dom_space*	thisCtl ;	//	This domain name space
	hzString	S ;			//	domain name as string

	if (m_addr)
	{
		thisCtl = (_dom_space*) _strXlate(m_addr) ;
		S = (char*) thisCtl->m_data ;
	}

	return S ;
}

uint32_t	hzDomain::Length	(void) const
{
	//	Return length in bytes of the whole domain name

	_hzfunc("hzDomain::Length") ;

	_dom_space*	thisCtl ;	//	This domain name space

	if (!m_addr)
		return 0 ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;
	return thisCtl->m_len ;
}

const char*	hzDomain::GetDomain	(void) const
{
	//	Return domain name as null terminated string

	_hzfunc("hzDomain::GetDomain") ;

	_dom_space*	thisCtl ;	//	This domain name space

	if (!m_addr)
		return 0 ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;
	return thisCtl->m_data ;
}

const char*	hzDomain::GetTLD	(void) const
{
	//	Return whole domain name as null terminated string

	_hzfunc("hzDomain::GetAddress") ;

	_dom_space*	thisCtl ;	//	This domain name space

	if (!m_addr)
		return 0 ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;
	return thisCtl->m_data + thisCtl->m_tld + 1 ;
}

static	uint32_t	_ckDomain	(uint32_t& lastPeriod, const char* cpStr)
{
	//	Tests if a supplied string (or text at a hzChain iterator) is of the form of an domain name.
	//
	//	Permitted chars are: [a-z], [A-Z], [0-9] and the period (.) or minus sign (-) if they are not the last character in the string.
	//
	//	Arguments:	1)	cpStr	The char pointer to be tested
	//
	//	Returns:	True	If the supplied cstr sets the domain name
	//				False	Otherwise

	_hzfunc("IsDomain") ;

	const char*	i ;			//	Source string iterator
	uint32_t	nC ;		//	Char count
	uint32_t	nPeriod ;	//	Confirmed period, last position

	if (!cpStr || !cpStr[0])
		return false ;

	//	Count chars beyond @
	for (nC = nPeriod = 0, i = cpStr ; *i && nC < 256 ; i++)
	{
		if (*i == CHAR_PERIOD)
			nPeriod = nC ;

		if ((*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z') || (*i >= '0' && *i <= '9')
			|| (*i == CHAR_MINUS && i[1] > CHAR_SPACE && nC) || (*i == CHAR_PERIOD && i[1] > CHAR_SPACE && nC))
		{
			nC++ ;
			continue ;
		}
		return 0 ;
	}

	if (nC < 2 || nC > 253)
		return 0 ;

	lastPeriod = nPeriod ;
	return nC ;
}

hzDomain&	hzDomain::operator=	(const char* cpStr)
{
	//	Assign the hzDomain to an domain name held in a character string
	//
	//	Arguments:	1)	cpStr	A null terminated string assumed to be an domain name
	//
	//	Returns:	Reference to this email adress instance in all cases.
	//
	//	Note: This function will record an E_FORMAT error if the supplied cstr did not amount to an domain name

	_hzfunc("hzDomain::operator=(cstr)") ;

	_dom_space*	destCtl ;		//	This domain name space
	const char*	i ;				//	Email iterator
	char*		j ;				//	Email iterator
	uint32_t	nLen = 0 ;		//	Length of domain name
	uint32_t	nLP = 0 ;		//	Offset to last period

	Clear() ;

	if (!cpStr || !cpStr[0])
		return *this ;

	nLen = _ckDomain(nLP, cpStr) ;
	if (!nLen)
	{
		hzerr(_fn, HZ_ERROR, E_FORMAT, "Cannot assign %s", cpStr) ;
		return *this ;
	}

	m_addr = _strAlloc(nLen + DOM_FACTOR) ;
	if (!m_addr)
		hzexit(_fn, 0, E_MEMORY, "Cannot assign %s", cpStr) ;
	destCtl = (_dom_space*) _strXlate(m_addr) ;

	//	Assign the value
	destCtl->m_copy = 1 ;
	destCtl->m_len = nLen & 0xff ;
	destCtl->m_tld = nLP & 0xff ;
	for (j = destCtl->m_data, i = cpStr ; *i ; *j++ = *i++) ;
	*j = 0 ;
	return *this ;
}

hzDomain&	hzDomain::operator=	(const hzString& S)
{
	//	Assign the hzDomain to an domain name held in a hzString instance
	//
	//	Arguments:	1)	S	A string assumed to be an domain name
	//
	//	Returns:	Reference to this email adress instance in all cases.
	//
	//	Note: This function will record an E_FORMAT error if the supplied cstr did not amount to an domain name

	_hzfunc("hzDomain::operator=(hzStr)") ;

	Clear() ;
	return operator=(*S) ;
}

hzDomain&	hzDomain::operator=	(const hzChain::Iter& ci)
{
	//	Determines if the supplied iterator is at the start of a valid domain name and if it is, assignes this as the value to the calling instance.
	//
	//	Arguments:	1)	ci	Chain iterator
	//
	//	Returns:	Reference to this domain name instance. This will be empty if the input did not amount to an domain name

	_hzfunc("hzDomain::operator=(chIter)") ;

	hzChain::Iter	xi ;		//	External chain iterator

	_dom_space*	thisCtl ;		//	This domain name space
	char*		i ;				//	For populating string
	uint32_t	n ;				//	Counter
	uchar		nC ;			//	Chars count
	uint32_t	nPeriod ;		//	Position of last period

	Clear() ;
	if (ci.eof())
		return *this ;

	//	Process the string and set char incidence aggregates
	for (nC = nPeriod = 0, xi = ci ; !xi.eof() && *xi > CHAR_SPACE & nC < 256 ; nC++, *xi, xi++)
	{
		if (*xi == CHAR_PERIOD)
			nPeriod = nC ;

		if (!((*xi >= 'a' && *xi <= 'z') || (*xi >= 'A' && *xi <= 'Z') || (*xi >= '0' && *xi <= '9')
				|| (*xi == CHAR_MINUS && i[1] > CHAR_SPACE && nC) || (*xi == CHAR_PERIOD && i[1] > CHAR_SPACE && nC)))
			break ;
	}

	if (*xi > CHAR_SPACE || !nPeriod || nC > 253 || nC < 2)
	{
		hzerr(_fn, HZ_ERROR, E_FORMAT, "Cannot assign") ;
		return *this ;
	}

	m_addr = _strAlloc(nC + DOM_FACTOR) ;
	if (!m_addr)
		hzexit(_fn, 0, E_MEMORY, "Cannot assign %d bytes", nC + 5) ;
	thisCtl = (_dom_space*) _strXlate(m_addr) ;

	i = (char*) thisCtl->m_data ;

	for (xi = ci, n = 0 ; n <= nC ; n++, xi++)
		i[n] = *xi ;
	i[n] = 0 ;

	thisCtl->m_copy = 1 ;
	thisCtl->m_len = nC & 0xff ;
	thisCtl->m_tld = nPeriod & 0xff ;

	return *this ;
}

hzDomain&	hzDomain::operator=	(const hzDomain& E)
{
	//	Assign the hzDomain to an domain name held in another hzDomain instance
	//
	//	Arguments:	1)	E	The supplied domain name as a hzEmail instance
	//
	//	Returns:	Reference to this hzEmail instance

	//	It this internal pointer and that of the operand already point to the same space in memory, do nothing

	_hzfunc("hzDomain::operator=(hzDomain)") ;

	_dom_space*	suppCtl ;		//	Supplied domain name space

    if (m_addr == E.m_addr)
        return *this ;

	Clear() ;

	if (E.m_addr)
	{
		//_copyadd() ;
		suppCtl = (_dom_space*) _strXlate(E.m_addr) ;

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

bool	hzDomain::operator==	(const hzDomain& E) const
{
	//	Test for equality between this hzDomain and an operand instance
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this addesss is equal to the supplied test domain name
	//				False	Otherwise

	_hzfunc("hzDomain::operator==") ;

	_dom_space*	thisCtl ;		//	This domain name space
	_dom_space*	suppCtl ;		//	Supplied domain name space

	if (m_addr == E.m_addr)			return true ;
	if (m_addr && E.m_addr == 0)	return false ;
	if (E.m_addr && m_addr == 0)	return false ;

	//	Compare domains first then LHS
	thisCtl = (_dom_space*) _strXlate(m_addr) ;
	suppCtl = (_dom_space*) _strXlate(E.m_addr) ;

	if (!strcmp(thisCtl->m_data, suppCtl->m_data))
		return true ;
	return false ;
}

bool	hzDomain::operator<		(const hzDomain& E) const
{
	//	Return true if this hzDomain instance is lexically less than the operand. Note that comparison is first done on the domain part (the RHS
	//	of the @) and then done on the address part (the LHS of the @)
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this addesss is lexically less than the supplied test domain name
	//				False	Otherwise

	_hzfunc("hzDomain::operator<") ;

	int32_t	res ;

	//	Are this and the operand connected?
	if (m_addr == E.m_addr)			return false ;
	if (m_addr && E.m_addr == 0)	return false ;
	if (E.m_addr && m_addr == 0)	return true ;

	//	Compare domains first
	res = strcmp(GetDomain(), E.GetDomain()) ;
	if (res < 0)
		return true ;
	return false ;
}

bool	hzDomain::operator<=	(const hzDomain& E) const
{
	//	Return true if this hzDomain instance is lexically less than or equal to the operand. Note that comparison is first done on the domain
	//	part (the RHS of the @) and then done on the address part (the LHS of the @)
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this addesss is lexically less than or equal to the supplied test domain name
	//				False	Otherwise

	_hzfunc("hzDomain::operator<=") ;

	int32_t		res ;		//	Comparison result

	//	Are this and the operand connected?
	if (m_addr == E.m_addr)			return true ;
	if (m_addr && E.m_addr == 0)	return false ;
	if (E.m_addr && m_addr == 0)	return true ;

	//	Compare domains first
	res = strcmp(GetDomain(), E.GetDomain()) ;
	if (res > 0)
		return false ;
	return true ;
}

bool	hzDomain::operator>		(const hzDomain& E) const
{
	//	Return true if this hzDomain instance is lexically greater than the operand. Note that comparison is first done on the domain
	//	part (the RHS of the @) and then done on the address part (the LHS of the @)
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this addesss is lexically greater than the supplied test domain name
	//				False	Otherwise

	_hzfunc("hzDomain::operator>") ;

	int32_t		res ;		//	Comparison result

	//	Are this and the operand connected?
	if (m_addr == E.m_addr)			return false ;
	if (m_addr && E.m_addr == 0)	return true ;
	if (E.m_addr && m_addr == 0)	return false ;

	//	Compare domains first
	res = strcmp(GetDomain(), E.GetDomain()) ;
	if (res > 0)
		return true ;
	return false ;
}

bool	hzDomain::operator>=	(const hzDomain& E) const
{
	//	Return true if this hzDomain instance is lexically greater than or equal to the operand. Note that comparison is first done on the domain
	//	part (the RHS of the @) and then done on the address part (the LHS of the @)
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this addesss is lexically greater than or equal to the supplied test domain name
	//				False	Otherwise

	_hzfunc("hzDomain::operator>=") ;

	int32_t		res ;		//	Comparison result

	//	Are this and the operand connected?
	if (m_addr == E.m_addr)			return true ;
	if (m_addr && E.m_addr == 0)	return true ;
	if (E.m_addr && m_addr == 0)	return false ;

	//	Compare domains first
	res = strcmp(GetDomain(), E.GetDomain()) ;
	if (res < 0)
		return false ;
	return true ;
}

bool	hzDomain::operator==	(const hzString& S) const
{
	//	Test for equality between this hzDomain and an domain name held in a hzString
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this addesss is lexically equal to the supplied test domain name
	//				False	Otherwise

	_hzfunc("hzDomain::operator==") ;

	_dom_space*	thisCtl ;		//	This domain name space

	if (!S && !m_addr)	return true ;
	if (!S)				return false ;
	if (!m_addr)		return false ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;

	return CstrCompare((char*) thisCtl->m_data, *S) == 0 ? true : false ;
}

bool	hzDomain::operator!=	(const hzString& S) const
{
	//	Test for inequality between this hzDomain and an domain name held in a hzString
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this addesss is not lexically equal to the supplied test domain name
	//				False	Otherwise

	_hzfunc("hzDomain::operator!=") ;

	_dom_space*	thisCtl ;		//	This domain name space

	if (!S && !m_addr)	return false ;
	if (!S)				return true ;
	if (!m_addr)		return true ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;

	return CstrCompare((char*) thisCtl->m_data, *S) == 0 ? false : true ;
}

bool	hzDomain::operator==	(const char* cpStr) const
{
	//	Test for equality between this hzDomain and an domain name held in a character string
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this addesss is lexically equal to the supplied test domain name
	//				False	Otherwise

	_hzfunc("hzDomain::operator==") ;

	_dom_space*	thisCtl ;		//	This domain name space

	if ((!cpStr || !cpStr[0]) && !m_addr)
		return true ;
	if (!cpStr || !cpStr[0])
		return false ;
	if (!m_addr)
		return false ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;

	return CstrCompare((char*) thisCtl->m_data, cpStr) == 0 ? true : false ;
}

bool	hzDomain::operator!=	(const char* cpStr) const
{
	//	Test for inequality between this hzDomain and an domain name held in a character string
	//
	//	Arguments:	1)	E	Test domain name
	//
	//	Returns:	True	If this email addesss is not lexically equal to the supplied
	//				False	If this domain name has the same value as the supplied

	_hzfunc("hzDomain::operator!=") ;

	_dom_space*	thisCtl ;		//	This domain name space

	if ((!cpStr || !cpStr[0]) && !m_addr)
		return false ;
	if (!cpStr || !cpStr[0])
		return true ;
	if (!m_addr)
		return true ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;

	return CstrCompare((char*) thisCtl->m_data, cpStr) == 0 ? false:true ;
}

const char*	hzDomain::operator*	(void) const
{
	//	Returns the URL data (a null terminated string)
	//
	//	Arguments:	None
	//	Returns:	Content as null terminated string

	_hzfunc("hzDomain::operator*") ;

	_dom_space*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return 0 ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;
	return thisCtl->m_data ;
}

hzDomain::operator const char*    (void) const
{
	//	Returns the string data (a null terminated string)
	//
	//	Arguments:	None
	//	Returns:	Content as null terminated string

	_hzfunc("hzDomain::operator const char*") ;

	_dom_space*	thisCtl ;		//	This string's control area

	if (!m_addr)
		return 0 ;

	thisCtl = (_dom_space*) _strXlate(m_addr) ;
	return thisCtl->m_data ;
}

/*
**	Stream operator
*/

std::ostream&	operator<<	(std::ostream& os, const hzDomain& obj)
{
	//	Category:	Data Output
	//
	//	Friend function to hzDomain class to stream out the domain name.
	//
	//	Arguments:	1)	os		Reference to output stream
	//				2)	ema		Const reference to an domain name
	//
	//	Returns:	Reference to the supplied output stream

	_hzfunc("operator<<(ostream,hzDomain)") ;

	os << *obj ;
	return os ;
}

bool	IsDomain	(const char* cpStr)
{
	//	Category:	Text Processing
	//
	//	Tests if a supplied string (or text at a hzChain iterator) is of the form of an domain name.
	//
	//	Permitted chars are: [a-z], [A-Z], [0-9] and the period (.) or minus sign (-) if they are not the last character in the string.
	//
	//	Arguments:	1)	cpStr	The char pointer to be tested
	//
	//	Returns:	True	If the supplied cstr sets the domain name
	//				False	Otherwise

	_hzfunc("IsDomain") ;

	const char*	i ;					//	Source string iterator
	uint32_t	nC = 0 ;			//	Char count
	bool		bPeriod = false ;	//	Confirmed period

	if (!cpStr || !cpStr[0])
		return false ;

	//	Count chars beyond @
	for (i = cpStr ; *i ; i++)
	{
		if (*i == CHAR_PERIOD)
			bPeriod = true ;

		if ((*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z') || (*i >= '0' && *i <= '9')
			|| (*i == CHAR_MINUS && i[1] > CHAR_SPACE && nC) || (*i == CHAR_PERIOD && i[1] > CHAR_SPACE && nC))
		{
			nC++ ;
			continue ;
		}
		return false ;
	}

	if (bPeriod && nC > 2)
		return true ;
	return false ;
}

bool	AtDomain	(hzDomain& dom, uint32_t& nLen, hzChain::Iter& ci)
{
	//	Category:	Text Processing
	//
	//	If the supplied iterator is at the start of a valid domain name, then the supplied domain name instance will be populated with this
	//	address as the value and the supplied length will be set.
	//
	//	Arguments:	1)	dom		Reference to an domain name, populated by this function
	//				2)	nLen	Reference to an integer set to the dom length
	//				3)	ci		Input chain iterator
	//
	//	Returns:	True	If the supplied cstr sets the domain name
	//				False	Otherwise

	_hzfunc("AtDomain") ;

	dom.Clear() ;
	dom = ci ;
	if (!dom)
		return false ;
	nLen = dom.Length() ;
	return true ;
}
