//
//	File:	hzEmaddr.h
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

#ifndef hzEmaddr_h
#define hzEmaddr_h

//	Other Includes
#include "hzBasedefs.h"
#include "hzString.h"
#include "hzChain.h"

class	hzDomain
{
	//	Category:	Data
	//
	//	The hzDomain class imposes a data type of domain name. Domain names are a special case of a string with a defined form namely:-
	//
	//	hzDomain can only accept string values that comply with this form

	uint32_t	m_addr ;		//	Encoded address of data space and control

public:
	//	Set methods
	void		Clear		(void) ;
	hzDomain&	operator=	(const char* cpStr) ;
	hzDomain&	operator=	(const hzString& S) ;
	hzDomain&	operator=	(const chIter& ci) ;
	hzDomain&	operator=	(const hzDomain& E) ;

	//	Constructors
	hzDomain	(void)					{ m_addr = 0 ; }
	hzDomain	(const hzDomain& dom)	{ m_addr = 0 ; operator=(dom) ; }
	hzDomain	(const char* cpEMAddr)	{ m_addr = 0 ; operator=(cpEMAddr) ; }
	hzDomain	(const hzString& dom)	{ m_addr = 0 ; operator=(dom) ; }

	//	Destructor
	~hzDomain	(void)	{ Clear() ; }

	//	Internal use only
	uint32_t	_int_addr	(void) const	{ return m_addr ; }
	void		_int_clr	(void)			{ m_addr = 0 ; }

	//	Direct compare operators (compare domains first)
	bool	operator==	(const hzDomain& E) const ;
	bool	operator<	(const hzDomain& E) const ;
	bool	operator<=	(const hzDomain& E) const ;
	bool	operator>	(const hzDomain& E) const ;
	bool	operator>=	(const hzDomain& E) const ;

	//	Normal string compares
	bool	operator==	(const char* cpStr) const ;
	bool	operator!=	(const char* cpStr) const ;
	bool	operator==	(const hzString& S) const ;
	bool	operator!=	(const hzString& S) const ;

	/*
	**	Get value methods
	*/

	uint32_t	Length	(void) const ;		//	Return total length of email address

	//	Get part or whole value
	const char*	GetDomain	(void) const ;
	const char*	GetTLD		(void) const ;

	hzString	Str	(void) const ;

	//	Casting Operators
	bool		operator!	(void) const	{ return m_addr ? false : true ; }
	const char*	operator*	(void) const ;
	operator	const char*	(void) const ;

	//	Friend operator functions
	friend	std::ostream&	operator<<	(std::ostream& os, const hzDomain& ema) ;
} ;

/*
**	Email addresses
*/

class	hzEmaddr
{
	//	Category:	Data
	//
	//	The hzEmaddr class imposes a data type of email address. It is a form of string that can only accept values that are of the form of an
	//	email address. As such, no operators apply to hzEmaddr. They may only be set and compared.

	uint32_t	m_addr ;		//	Encoded address of data space and control

public:
	//	Set methods
	void		Clear		(void) ;
	hzEmaddr&	operator=	(const char* cpStr) ;
	hzEmaddr&	operator=	(const hzString& S) ;
	hzEmaddr&	operator=	(const chIter& ci) ;
	hzEmaddr&	operator=	(const hzEmaddr& E) ;

	//	Constructors
	hzEmaddr	(void)					{ m_addr = 0 ; }
	hzEmaddr	(const hzEmaddr& ema)	{ m_addr = 0 ; operator=(ema) ; }
	hzEmaddr	(const char* cpEMAddr)	{ m_addr = 0 ; operator=(cpEMAddr) ; }
	hzEmaddr	(hzString& ema)			{ m_addr = 0 ; operator=(ema) ; }

	//	Destructor
	~hzEmaddr	(void)	{ Clear() ; }

	//	Internal use only
	uint32_t	_int_addr	(void) const	{ return m_addr ; }
	void		_int_clr	(void)			{ m_addr = 0 ; }

	//	Direct compare operators (compare domains first)
	bool	operator==	(const hzEmaddr& E) const ;
	bool	operator<	(const hzEmaddr& E) const ;
	bool	operator<=	(const hzEmaddr& E) const ;
	bool	operator>	(const hzEmaddr& E) const ;
	bool	operator>=	(const hzEmaddr& E) const ;

	//	Normal string compares
	bool	operator==	(const char* cpStr) const ;
	bool	operator!=	(const char* cpStr) const ;
	bool	operator==	(const hzString& S) const ;
	bool	operator!=	(const hzString& S) const ;

	/*
	**	Get value methods
	*/

	uint32_t	Length	(void) const ;		//	Return total length of email address
	uint32_t	LhsLen	(void) const ;		//	Return length before the @
	uint32_t	DomLen	(void) const ;		//	Return length after the @

	//	Get part or whole value
	const char*	GetDomain	(void) const ;
	const char*	GetAddress	(void) const ;

	hzString	Str	(void) const ;

	//	Casting Operators
	bool		operator!	(void) const	{ return m_addr ? false : true ; }
	const char*	operator*	(void) const ;
	operator	const char*	(void) const ;

	//	Friend operator functions
	friend	_atomval	atomval_hold	(const hzEmaddr& ema) ;
	friend	std::ostream&	operator<<	(std::ostream& os, const hzEmaddr& ema) ;
} ;

//	Convienent globals always at null
extern	const	hzEmaddr	_hz_null_hzEmaddr ;		//	Null email address

#endif	//	hzEmaddr_h
