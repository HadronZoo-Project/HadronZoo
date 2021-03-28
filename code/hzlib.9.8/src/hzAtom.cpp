//
//	File:	hzAtom.cpp
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

#include <iostream>
#include <fstream>
#include <pthread.h>

#include "hzTextproc.h"
#include "hzChars.h"
#include "hzDatabase.h"

using namespace std ;

/*
**	Functions to read atom values
*/

hzString	hzAtom::Str	(void) const
{
	//	Purpose:	Formulate a textual representation of the atom value
	//
	//	Arguments:	1)	eFmt	Format (optional)
	//
	//	Returns:	Instance of hzString by value being atom value in text form.

	_hzfunc("hzAtom::Str") ;

	hzXDate		fd ;			//	Recepticle for hzXDate data
	hzEmaddr	ema ;			//	Recepticle for hzEmaddr data
	hzUrl		url ;			//	Recepticle for hzUrl data
	hzIpaddr	ipa ;			//	Recepticle for hzIpaddr data
	hzSDate		sd ;			//	Recepticle for hzSDate data
	hzTime		ti ;			//	Recepticle for hzTime data
	char		buf	[40] ;		//	Recepticle for null terminated string

	if (!(m_eStatus & ATOM_SET))
	{
		m_Str = 0 ;
		return m_Str ;
	}

	switch (m_eType)
	{
	case BASETYPE_UNDEF:		m_Str = 0 ;
								return m_Str ;

	case BASETYPE_CPP_UNDEF:	m_Str = "CPP_UNDEF" ;
								return m_Str ;

	case BASETYPE_DOUBLE:		sprintf(buf, "%f", m_Data.m_Double) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_INT64:		sprintf(buf, "%ld", m_Data.m_sInt64) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_INT32:		sprintf(buf, "%d", m_Data.m_sInt32) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_INT16:		sprintf(buf, "%d", m_Data.m_sInt16) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_BYTE:			sprintf(buf, "%d", m_Data.m_sByte) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_UINT64:		sprintf(buf, "%lu", m_Data.m_uInt64) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_UINT32:		sprintf(buf, "%u", m_Data.m_uInt32) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_UINT16:		sprintf(buf, "%u", m_Data.m_uInt16) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_UBYTE:		sprintf(buf, "%u", m_Data.m_uByte) ; 	m_Str = buf ;	return m_Str ;

	case BASETYPE_BOOL:			m_Str = m_Data.m_Bool ? "true" : "false" ;
								return m_Str ;

	case BASETYPE_HZO_UNDEF:	m_Str = "HZO_UNDEF" ;
								return m_Str ;

	case BASETYPE_STRING:
	case BASETYPE_DOMAIN:
	case BASETYPE_EMADDR:
	case BASETYPE_URL:			return m_Str ;

	case BASETYPE_IPADDR:		memcpy(&ipa, &m_Data.m_uInt32, 4) ;	m_Str = ipa.Str() ;	return m_Str ;
	case BASETYPE_TIME:			memcpy(&ti, &m_Data.m_uInt32, 4) ;	m_Str = ti.Str() ;	return m_Str ;
	case BASETYPE_SDATE:		memcpy(&sd, &m_Data.m_uInt32, 4) ;	m_Str = sd.Str() ;	return m_Str ;
	case BASETYPE_XDATE:		memcpy(&fd, &m_Data.m_uInt64, 8) ;	m_Str = fd.Str() ;	return m_Str ;

   	case BASETYPE_TEXT:			sprintf(buf, "TXT ref=%x",	m_Data.m_uInt32) ;	m_Str = buf ;	return m_Str ;
   	case BASETYPE_BINARY:		sprintf(buf, "BIN ref=%x",	m_Data.m_uInt32) ;	m_Str = buf ;	return m_Str ;
   	case BASETYPE_TXTDOC:		sprintf(buf, "DOC ref=%x",	m_Data.m_uInt32) ;	m_Str = buf ;	return m_Str ;
	case BASETYPE_ENUM:			sprintf(buf, "%u",			m_Data.m_sInt32) ;	m_Str = buf ;	return m_Str ;
   	case BASETYPE_APPDEF:		sprintf(buf, "APP ref=%x",	m_Data.m_uInt32) ;	m_Str = buf ;	return m_Str ;
   	case BASETYPE_CLASS:		sprintf(buf, "OBJ ref=%x",	m_Data.m_uInt32) ;	m_Str = buf ;	return m_Str ;
	}

	m_Str = 0 ;
	return m_Str ;
}

hzEmaddr	hzAtom::Emaddr	(void) const
{
	//	Retrieve email address from this atom. This will be populated if the hzAtom has a value and the datatype is BASETYPE_EMADDR and be empty otherwise
	//
	//	Arguments:	None
	//	Returns:	Instance of hzEmail by value.

	hzEmaddr*	pEma ;		//	Interim pointer
	hzEmaddr	ema ;		//	Return value

	if (m_eStatus == ATOM_SET && m_eType == BASETYPE_EMADDR)
	{
		pEma = (hzEmaddr*) m_Data.m_pVoid ;
		ema = *pEma ;
	}

	return ema ;
}

hzUrl	hzAtom::Url	(void) const
{
	//	Arguments:	None
	//	Returns:	Instance of hzUrl by value. This will be populated if the hzAtom has a value and the datatype is BASETYPE_URL and be empty otherwise.

	hzUrl*	pUrl ;		//	Interim pointer
	hzUrl	url ;		//	Return value

	if (m_eStatus == ATOM_SET && m_eType == BASETYPE_URL)
	{
		pUrl = (hzUrl*) m_Data.m_pVoid ;
		url = *pUrl ;
	}

	return url ;
}

hzXDate	hzAtom::XDate	(void) const
{
	//	Arguments:	None
	//	Returns:	Instance of hzXDate by value. This will be populated if the hzAtom has a value and the datatype is BASETYPE_XDATE and be empty otherwise.

	hzXDate	x ;		//	Return value

	if (m_eStatus == ATOM_SET && m_eType == BASETYPE_XDATE)
		memcpy(&x, &m_Data, sizeof(hzXDate)) ;
	return x ;
}

hzSDate	hzAtom::SDate	(void) const
{
	//	Arguments:	None
	//	Returns:	Instance of hzSDate by value. This will be populated if the hzAtom has a value and the datatype is BASETYPE_SDATE and be empty otherwise.

	hzSDate	x ;		//	Return value

	if (m_eStatus == ATOM_SET && m_eType == BASETYPE_SDATE)
		memcpy(&x, &m_Data.m_uInt32, sizeof(hzSDate)) ;
	return x ;
}

hzTime	hzAtom::Time	(void) const
{
	//	Arguments:	None
	//	Returns:	Instance of hzTime by value. This will be populated if the hzAtom has a value and the datatype is BASETYPE_TIME and be empty otherwise.

	hzTime	x ;		//	Return value

	if (m_eStatus == ATOM_SET && m_eType == BASETYPE_TIME)
		memcpy(&x, &m_Data.m_uInt32, sizeof(hzTime)) ;
	return x ;
}

hzIpaddr	hzAtom::Ipaddr	(void) const
{
	//	Arguments:	None
	//	Returns:	Instance of hzIpaddr by value. This will be populated if the hzAtom has a value and the datatype is BASETYPE_IPADDR and be empty otherwise.

	hzIpaddr	x ;		//	Return value

	if (m_eStatus == ATOM_SET && m_eType == BASETYPE_IPADDR)
		memcpy(&x, &m_Data.m_uInt32, sizeof(hzIpaddr)) ;
	return x ;
}

/*
**	Functions to set hzAtom Values
*/

hzEcode	hzAtom::SetValue	(hdbBasetype eType, const hzString& S)
{
	//	Set the atom to the supplied data type and value.
	//
	//	Arguments:	1)	eType	The datatype
	//				2)	s		The string that either is or contains the value
	//
	//	Returns:	E_TYPE		If the anticipated data type is not specified or conflicts with current type
	//				E_BADVALUE	If the supplied string does not represent a valid value for the anticipated data type.
	//				E_OK		If the operation was successful.
	//

	_hzfunc("hzAtom::SetValue(hzString)") ;

	const char*	j ;					//	Storage for hzEmaddr data
	uint64_t	x ;					//	Storage for integer data
	hzXDate		fd ;				//	Storage for hzXDate data
	hzSDate		sd ;				//	Storage for hzSDate data
	hzTime		ti ;				//	Storage for hzTime data
	hzIpaddr	ipa ;				//	Storage for hzIpaddr data
	bool		bMinus = false ;	//	Negation indicator
	hzEcode		rc = E_BADVALUE ;	//	Return code

	//	Clear atom first
	Clear() ;

	//	If no value, just return
	if (!S)
		return E_OK ;

	m_eType = eType ;

	/*
	**	HadronZoo string-like types
	*/

	if (m_eType == BASETYPE_STRING)
		{ m_Str = S ; m_eStatus = ATOM_SET ; return E_OK ; }

	if (m_eType == BASETYPE_DOMAIN)
	{
		//hzDomain	dom ;		//	Temp email address

		//dom = S ;
		if (IsDomain(*S))
			{ m_eStatus = ATOM_SET ; m_Str = S ; return E_OK ; }
		return E_BADVALUE ;
	}

	if (m_eType == BASETYPE_EMADDR)
	{
		//hzEmaddr	ema ;		//	Temp email address

		if (IsEmaddr(*S))
			//ema = S ;
		//if (ema)
			{ m_eStatus = ATOM_SET ; m_Str = S ; return E_OK ; }
		return E_BADVALUE ;
	}

	if (m_eType == BASETYPE_URL)
	{
		hzUrl	url ;		//	Temp URL

		url = S ;
		if (url)
			{ m_eStatus = ATOM_SET ; m_Str = S ; return E_OK ; }
		return E_BADVALUE ;
	}

	/*
	**	HadronZoo types without smart pointers
	*/

	if (m_eType == BASETYPE_IPADDR)
	{
		ipa = *S ;
		if (ipa)
			{ m_eStatus = ATOM_SET ; memcpy(&m_Data, &ipa, 4) ; return E_OK ; }
		return E_BADVALUE ;
	}

	if (m_eType == BASETYPE_XDATE)
	{
		rc = fd.SetDateTime(S) ;
		if (rc == E_OK)
			{ m_eStatus = ATOM_SET ; memcpy(&m_Data, &fd, 8) ; return E_OK ; }
		return E_BADVALUE ;
	}

	if (m_eType == BASETYPE_SDATE)
	{
		rc = sd.SetDate(S) ;
		if (rc == E_OK)
			{ m_eStatus = ATOM_SET ; memcpy(&m_Data, &sd, 4) ; return E_OK ; }
		return E_BADVALUE ;
	}

	if (m_eType == BASETYPE_TIME)
	{
		rc = ti.SetTime(S) ;
		if (rc == E_OK)
			{ m_eStatus = ATOM_SET ; memcpy(&m_Data, &ti, 4) ; return E_OK ; }
		return E_BADVALUE ;
	}

	/*
	**	Numeric types (no smart pointers)
	*/

	if (m_eType == BASETYPE_DOUBLE)
	{
		if (IsDouble(m_Data.m_Double, *S))
			{ m_eStatus = ATOM_SET ; return E_OK ; }
		return E_BADVALUE ;
	}

	if (m_eType == BASETYPE_BOOL)
	{
		if (S == "true" || S == "yes" || S == "y" || S == "1")
			{ m_Data.m_Bool = true ; m_eStatus = ATOM_SET ; return E_OK ; }
		if (S == "false" || S == "no" || S == "n" || S == "0")
			{ m_Data.m_Bool = false ; m_eStatus = ATOM_SET ; return E_OK ; }
		return E_BADVALUE ;
	}

	//	Must be number or number equiv
	j = *S ;
	if (*j == CHAR_MINUS)
		{ j++ ; bMinus = true ; }
	for (x = 0 ; IsDigit(*j) ; j++)
		{ x *= 10 ; x += (*j - '0') ; }
	if (*j)
		return E_BADVALUE ;

	switch	(m_eType)
	{
	case BASETYPE_BYTE:		if (x > 0x7f)
								return E_BADVALUE ;
							m_Data.m_sByte = x & 0x7f ;
							if (bMinus)
								m_Data.m_sByte *= -1 ;
							break ;

	case BASETYPE_INT16:	if (x > 0x7fff)
								return E_BADVALUE ;
							m_Data.m_sInt16 = x & 0x7fff ;
							if (bMinus)
								m_Data.m_sInt16 *= -1 ;
							break ;

	case BASETYPE_INT32:	if (x > 0x7fffffff)
								return E_BADVALUE ;
							m_Data.m_sInt32 = x & 0x7fffffff ;
							if (bMinus)
								m_Data.m_sInt32 *= -1 ;
							break ;

	case BASETYPE_INT64:	m_Data.m_sInt64 = x ;
							if (bMinus)
								m_Data.m_sInt64 *= -1 ;
							break ;

	case BASETYPE_UBYTE:	if (x & 0xffffffffffffff00)
								return E_BADVALUE ;
							m_Data.m_uByte = (x & 0xff) ;
							break ;

	//case BASETYPE_ENUM2:
	case BASETYPE_UINT16:	if (x & 0xffffffffffff0000)
								return E_BADVALUE ;
							m_Data.m_uInt16 = (x & 0xffff) ;
							break ;

	case BASETYPE_UINT32:	if (x & 0xffffffff00000000)
								return E_BADVALUE ;
							m_Data.m_uInt32 = (x & 0xffffffff) ;
							break ;

	//case BASETYPE_UUID:
	case BASETYPE_UINT64:	m_Data.m_uInt64 = x ;
							break ;
	}

	m_eStatus = ATOM_SET ;
	return E_OK ;
}

hzEcode	hzAtom::SetValue	(hdbBasetype eType, const hzChain& Z)
{
	_hzfunc("hzAtom::SetValue(hzChain)") ;

	if (eType != BASETYPE_BINARY && eType != BASETYPE_TXTDOC)
		return hzerr(_fn, HZ_WARNING, E_TYPE, "May only set BINARY and TXTDOC values") ;

	Clear() ;
	if (Z.Size())
	{
		m_eType = eType ;
		m_Chain = Z ;
		m_eStatus = ATOM_SET | ATOM_CHAIN ;
	}

	return E_OK ;
}

hzEcode	hzAtom::SetValue	(hdbBasetype eType, const _atomval& av)
{
	_hzfunc("hzAtom::SetValue(atomval)") ;

	//	If the current and supplied data type are the same and the current m_Data equal to the supplied _atomval, do nothing
	if (m_eType == eType && m_Data.m_uInt64 == av.m_uInt64)
		return E_OK ;

	//	Clear the atom (in all cases)
	Clear() ;
	if (!av.m_uInt64)
		return E_OK ;

	m_eType = eType ;
	m_Data = av ;
	m_eStatus = ATOM_SET ;

	if (m_eType == BASETYPE_BINARY || m_eType == BASETYPE_TXTDOC)
		m_eStatus |= ATOM_CHAIN ;
	return E_OK ;
}

hzEcode	hzAtom::SetNumber	(const char* s)
{
	//	Set atom to a numeric data type and value if the supplied string amounts to a numberic value, i.e. is of the form
	//	[sign] digits [[.] digits] [[e][sign]digits]
	//
	//	Arguments:	1)	s	The supplied string.
	//
	//	Returns:	E_NOINIT	If the anticipated data type is not specified.
	//				E_BADVALUE	If the supplied string does not represent a valid value for the anticipated data type.
	//				E_OK		If the operation was successful.

	_hzfunc("hzAtom::SetNumber") ;

	const char*	i ;					//	Iterator
	uint64_t	valA ;				//	For digits past the decimal point
	double		valD ;				//	For double value
	uint32_t	valB ;				//	For digits past the decimal point
	uint32_t	valE ;				//	For digits past the exponent
	uint32_t	nDigits = 0 ;		//	Digit counter
	int32_t		nBytes = 0 ;		//	Byte count
	bool		bNeg = false ;		//	Minus operator indicator

	Clear() ;
	i = s ;
	if (!i)
		return E_NODATA ;

	//	Deal with leading sign
	if (*i == CHAR_MINUS)
		{ bNeg = true ; i++ ; }
	else if (*i == CHAR_PLUS)
		i++ ;
	else if (i[0] == '0' && (i[1] == 'x' || i[1] == 'X'))
	{
		//	Whole token must be hex and limited to 16 bytes - otherwise fail.

		for (i += 2 ; IsHex(*i) ; nBytes++, i++)
		{
			valA *= 16 ;

			if (*i >= '0' && *i <= '9') { valA += (*i - '0') ; continue ; }
			if (*i >= 'A' && *i <= 'F') { valA += 10 ; valA += (*i - 'A') ; continue ; }
			if (*i >= 'a' && *i <= 'f') { valA += 10 ; valA += (*i - 'a') ; continue ; }

			break ;
		}

		if (*i)
			{ m_eType = BASETYPE_UNDEF ; return E_FORMAT ; }
		if (!nBytes || nBytes > 16)
			{ m_eType = BASETYPE_UNDEF ; return E_FORMAT ; }

		if		(nBytes > 8)	{ m_eType = BASETYPE_UINT64 ; m_Data.m_uInt64 = valA ; }
		else if (nBytes > 4)	{ m_eType = BASETYPE_UINT32 ; m_Data.m_uInt32 = valA & 0xffffffff ; }
		else if (nBytes > 2)	{ m_eType = BASETYPE_UINT16 ; m_Data.m_uInt16 = valA & 0xffff ; }
		else
			{ m_eType = BASETYPE_UBYTE ; m_Data.m_uByte = valA & 0xff ; }
		return E_OK ;
	}

	//	Expect a series of at least one digit
	for (nDigits = 0 ; IsDigit(*i) ; nBytes++, nDigits++, i++)
	{
		valA *= 10 ;
		if (*i >= '0' && *i <= '9')
			{ valA += (*i - '0') ; continue ; }
		break ;
	}

	if (!nDigits)
		return BASETYPE_UNDEF ;

	//	Test for a period that is followed by at least one digit
	if (*i == CHAR_PERIOD)
	{
		i++ ;
		for (nDigits = 0 ; IsDigit(*i) ; nBytes++, nDigits++, i++)
		{
			valD *= 10.0 ;
			if (*i >= '0' && *i <= '9')
				{ valB += (*i - '0') ; continue ; }
			break ;
		}
		if (!nDigits)
			return BASETYPE_UNDEF ;

		for (; nBytes ; nBytes--)
			valD /= 10.0 ;
		valD += (double) valA ;

		//	Test for the 'e' followed by at least one digit or a +/- followed by at least one digit
		if (*i == 'e')
		{
			i++ ;
			if (*i == CHAR_MINUS || *i == CHAR_PLUS)
				{ nBytes++ ; i++ ; }

			for (nDigits = 0 ; IsDigit(*i) ; nBytes++, nDigits++, i++)
			{
				valE *= 10 ;
				if (*i >= '0' && *i <= '9')
					{ valE += (*i - '0') ; continue ; }
				break ;
			}

			if (!nDigits)
				return BASETYPE_UNDEF ;
		}

		m_Data.m_Double = valD ;
		m_eType = BASETYPE_DOUBLE ;
		return E_OK ;
	}

	//	Not a double

	if (nDigits > 8)
	{
		if (bNeg)
			{ m_Data.m_sInt64 = valA ; m_eType = BASETYPE_INT64 ; }
		else
			{ m_Data.m_uInt64 = valA ; m_eType = BASETYPE_UINT64 ; }
	}
	else if (nDigits > 4)
	{
		if (bNeg)
			{ m_Data.m_sInt32 = valA ; m_eType = BASETYPE_INT32 ; }
		else
			{ m_Data.m_uInt32 = valA ; m_eType = BASETYPE_UINT32 ; }
	}
	else if (nDigits > 2)
	{
		if (bNeg)
			{ m_Data.m_sInt16 = valA ; m_eType = BASETYPE_INT16 ; }
		else
			{ m_Data.m_uInt16 = valA ; m_eType = BASETYPE_UINT16 ; }
	}
	else
	{
		if (bNeg)
			{ m_Data.m_sByte = valA ; m_eType = BASETYPE_BYTE ; }
		else
			{ m_Data.m_uByte = valA ; m_eType = BASETYPE_UBYTE ; }
	}

	m_eStatus = ATOM_SET ;
	return E_OK ;
}

hzAtom&	hzAtom::operator=	(const hzAtom& a)
{
	//	Set atom type and value to that of another atom
	//
	//	Argument:	a	The operand atom
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(hzAtom)") ;

	Clear() ;

	m_eType = a.m_eType ;
	m_eStatus = a.m_eStatus ;
	m_Data = a.m_Data ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzChain& Z)
{
	//	Set atom value to a chain.
	//
	//	Argument:	Z	The data as chain
	//	Returns:	Reference to this atom
	
	_hzfunc("hzAtom::operator=(hzChain)") ;

	Clear() ;
	if (Z.Size())
		{ m_eType = BASETYPE_BINARY ; m_Chain = Z ; m_eStatus = ATOM_SET | ATOM_CHAIN ; }
	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzString& s)
{
	//	Set atom value to a string.
	//
	//	Argument:	s	The data as string
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(hzString)") ;

	Clear() ;
	if (s)
		{ m_eType = BASETYPE_STRING ; m_Str = s ; m_eStatus = ATOM_SET ; }
	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzDomain& dom)
{
	_hzfunc("hzAtom::operator=(hzDomain)") ;

	Clear() ;
	if (dom)
		{ m_eType = BASETYPE_DOMAIN ; m_Str = *dom ; m_eStatus = ATOM_SET ; }
	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzEmaddr& ema)
{
	//	Set atom value to an email address.
	//
	//	Argument:	ema		The data as email address
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(hzEmaddr)") ;

	Clear() ;
	if (ema)
		{ m_eType = BASETYPE_EMADDR ; m_Str = *ema ; m_eStatus = ATOM_SET ; }
	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzUrl& url)
{
	//	Set atom value to a URL
	//
	//	Argument:	url	The data as URL
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(hzUrl)") ;

	Clear() ;
	if (url)
		{ m_eType = BASETYPE_URL ; m_Str = *url ; m_eStatus = ATOM_SET ; }
	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzIpaddr& ipa)
{
	//	Set atom value to an IP address.
	//
	//	Argument:	ipa		The data as IP address
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(hzIpaddr)") ;

	Clear() ;

	m_eType = BASETYPE_IPADDR ;

	if (!ipa)
		{ memset(&m_Data, 0, 8) ; m_eStatus = ATOM_CLEAR ; }
	else
		{ memcpy(&m_Data, &ipa, sizeof(hzIpaddr)) ; m_eStatus = ATOM_SET ; }

	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzXDate& fd)
{
	//	Set atom value to a full date.
	//
	//	Argument:	fd	The data as full date & time
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(hzXDate)") ;

	Clear() ;

	m_eType = BASETYPE_XDATE ;
	memcpy(&m_Data, &fd, 8) ;

	if (m_Data.m_uInt64)
		m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzSDate& sd)
{
	//	Set atom value to a short date.
	//
	//	Argument:	sd	The data as short form date
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(hzSDate)") ;

	Clear() ;

	m_eType = BASETYPE_SDATE ;

	if (!sd)
		{ m_Data.m_sInt64 = 0 ; m_eStatus = ATOM_CLEAR ; }
	else
		{ m_Data.m_uInt32 = sd.NoDays() ; m_eStatus = ATOM_SET ; }

	return *this ;
}

hzAtom&	hzAtom::operator=	(const hzTime& time)
{
	//	Set atom value to a time. Note that this function will have no effect unless the atom has type of undefined or of
	//	BASETYPE_TIME
	//
	//	Argument:	tim	Time of day
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(hzTime)") ;

	Clear() ;

	m_eType = BASETYPE_TIME ;

	if (!time)
		{ m_Data.m_sInt64 = 0 ; m_eStatus = ATOM_CLEAR ; }
	else
		{ m_Data.m_uInt32 = time.NoSecs() ; m_eStatus = ATOM_SET ; }

	return *this ;
}

hzAtom&	hzAtom::operator=	(double val)
{
	//	If the current atom type is unknown or double, set the value
	//
	//	Argument:	val	Numeric value (double)
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(double)") ;

	Clear() ;
	m_eType = BASETYPE_DOUBLE ;
	m_Data.m_Double = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(int64_t val)
{
	//	If the current atom type is unknown or int64_t, set the value
	//
	//	Argument:	val	Numeric value (int64_t)
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(int64)") ;

	Clear() ;
	m_eType = BASETYPE_INT64 ;
	m_Data.m_sInt64 = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(uint64_t val)
{
	//	If the current atom type is unknown or unt64, set the value
	//
	//	Argument:	val	Unsigned numeric value (uint64_t)
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(uint64)") ;

	Clear() ;
	m_eType = BASETYPE_UINT64 ;
	m_Data.m_uInt64 = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(int32_t val)
{
	//	If the current atom type is unknown or int32_t, set the value
	//
	//	Argument:	val	The data as an int32_t
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(int32)") ;

	Clear() ;
	m_eType = BASETYPE_INT32 ;
	m_Data.m_sInt32 = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(uint32_t val)
{
	//	If the current atom type is unknown or uint32_t, set the value
	//
	//	Argument:	val	The data as an uint32_t
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(uint32)") ;

	Clear() ;
	m_eType = BASETYPE_UINT32 ;
	m_Data.m_uInt32 = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(int16_t val)
{
	//	If the current atom type is unknown or int16_t, set the value
	//
	//	Argument:	val	The data as an int16_t
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(int16)") ;

	Clear() ;
	m_eType = BASETYPE_INT16 ;
	m_Data.m_sInt16 = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(uint16_t val)
{
	//	If the current atom type is unknown or uint16_t, set the value
	//
	//	Argument:	val	The data as an uint16_t
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(uint16)") ;

	Clear() ;
	m_eType = BASETYPE_UINT16 ;
	m_Data.m_uInt16 = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(char val)
{
	//	Set the atom value to the supplied char. The type of the atom will need to be either undefined or one of the numeric types.
	//
	//	Argument:	val	The data as a single char
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(char)") ;

	Clear() ;
	m_eType = BASETYPE_BYTE ;
	m_Data.m_sByte = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(uchar val)
{
	//	Set the atom value to the supplied unsigned char. The type of the atom will need to be either undefined or one of the numeric types.
	//
	//	Argument:	val	The data as a single unsigned char
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(uchar)") ;

	Clear() ;
	m_eType = BASETYPE_UBYTE ;
	m_Data.m_uByte = val ;
	m_eStatus = ATOM_SET ;

	return *this ;
}

hzAtom&	hzAtom::operator=	(bool b)
{
	//	Set the atom value to the supplied boolean. The type of the atom will need to be either undefined or TYPE_BOOL
	//
	//	Argument:	b	The data as a boolean
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::operator=(bool)") ;

	Clear() ;
	m_eStatus = ATOM_SET ;
	m_eType = BASETYPE_BOOL ;
	m_Data.m_Bool = b ;

	return *this ;
}

hzAtom&	hzAtom::Clear	(void)
{
	//	Clear the atom value to null. Note this does not set the type to undefined.
	//
	//	Arguments:	None
	//	Returns:	Reference to this atom

	_hzfunc("hzAtom::Clear") ;

	m_Chain.Clear() ;
	m_Str = 0 ;
	m_Data.m_uInt64 = 0 ;
	m_eType = BASETYPE_UNDEF ;
	m_eStatus = ATOM_CLEAR ;
	return *this ;
}

std::ostream&	operator<<	(std::ostream& os, const hzAtom& obj)
{
	//	Category:	Data Output
	//
	//	Facilitates streaming (printing of value) of any atom.
	//
	//	Arguments:	1)	os	The output stream
	//				2)	obj	The atom to write out
	//
	//	Returns:	Reference to the supplied output stream

	_hzfunc("hzAtom::operator<<") ;

	switch (obj.Type())
	{
	case BASETYPE_DOMAIN:
	case BASETYPE_EMADDR:
	case BASETYPE_URL:
	case BASETYPE_STRING:	os << obj.m_Str ;
							break ;

	case BASETYPE_IPADDR:	os << *obj.Ipaddr().Str() ;	break ;
	case BASETYPE_XDATE:	os << *obj.XDate().Str() ;	break ;
	case BASETYPE_SDATE:	os << *obj.SDate().Str() ;	break ;
	case BASETYPE_TIME:		os << *obj.Time().Str() ;	break ;
	case BASETYPE_DOUBLE:	os << obj.Double() ;		break ;

	//	Kludge to fix no stream handling of _int64_t
	case BASETYPE_INT64:	os << FormalNumber(obj.Int64()) ;	break ;
	case BASETYPE_INT32:	os << FormalNumber(obj.Int32()) ;	break ;
	case BASETYPE_INT16:	os << obj.Int16() ;	break ;
	case BASETYPE_BYTE:		os << obj.Byte() ;	break ;

	case BASETYPE_BOOL:		if (obj.Bool())
								os << "true" ;
							else
								os << "false" ;
							break ;
	case BASETYPE_UNDEF:	os << "Unknown type" ;
							break ;
	}

	return os ;
}
