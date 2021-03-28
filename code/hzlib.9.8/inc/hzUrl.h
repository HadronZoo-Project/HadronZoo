//
//	File:	hzUrl.h
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

#ifndef hzUrl_h
#define hzUrl_h

//	Other Includes
#include "hzBasedefs.h"
#include "hzString.h"

/*
**	Universal Resource Locator (URL)
*/

class	hzUrl
{
	//	Category:	Data
	//
	//	The hzUrl class is a HadronZoo 'string-like' data type which holds Internet URL (Universal Resource Locator) values. These are strings of the following
	//	form:-
	//
	//		1)	An optional protocol specifier eg 'http://' or 'https://'
	//
	//		2)	A mandatory domain specifier which may either be an IP address or a domain name (eg www.hadronzoo.com)
	//
	//		3)	An optional resource specifier being one or more strings separated by a forward slash representing 'directories' relative to the
	//		root for the domain.
	//
	//		4)	An optional resource qualifier consisting of one or more name-value pairs delimited by the query character.
	//
	//	In common with hzString, hzUrl has a single member being a pointer to an internal structure that incorporates both the value and control parameters.

	uint32_t	m_addr ;		//	Address of data and control space

public:
	//	Set methods (and validate). These accept both the standard forms and the filename form.
	void	Clear		(void) ;
	hzUrl&	SetValue	(const hzString domain, const hzString resource, bool secure = false, uint32_t port = 0) ;
	hzUrl&	operator=	(const char* cpStr) ;
	hzUrl&	operator=	(const hzString& S) ;
	hzUrl&	operator=	(const hzUrl& E) ;

	//	Constructors
	hzUrl	(void)					{ m_addr = 0 ; }
	hzUrl	(const hzUrl& op)		{ m_addr = 0 ; operator=(op) ; }
	hzUrl	(const char* cpUrl)		{ m_addr = 0 ; operator=(cpUrl) ; }
	hzUrl	(const hzString& url)	{ m_addr = 0 ; operator=(url) ; }

	//	Destructors
	~hzUrl	(void)	{ Clear() ; }

	//	Internal use only
	uint32_t	_int_addr	(void) const	{ return m_addr ; }
	void		_int_clr	(void)			{ m_addr = 0 ; }

	//	Get functions
	uint32_t	Length	(void) const ;	//	Return total length of URL (including the http:// or https:// sequence)
	uint32_t	Port	(void) const ;	//	Return port number
	bool		IsSSL	(void) const ;	//	Does the URL require SSL?

	//	Get value in a form suitable for a filename (for storage)
	hzString	Filename	(void) const ;

	//	Get part or whole value (not available in direct text due to internal format)
	hzString	Whole		(void) const ;
	hzString	Domain		(void) const ;
	hzString	Resource	(void) const ;

	/*
	**	Compare Operators
	*/

	//	These compare domains first
	bool	operator==	(const hzUrl& E) const ;
	bool	operator<	(const hzUrl& E) const ;
	bool	operator<=	(const hzUrl& E) const ;
	bool	operator>	(const hzUrl& E) const ;
	bool	operator>=	(const hzUrl& E) const ;

	//	Normal string compares
	bool	operator==	(const char* cpStr) const ;
	bool	operator!=	(const char* cpStr) const ;
	bool	operator==	(const hzString& S) const ;
	bool	operator!=	(const hzString& S) const ;

	//	Casting Operators
	bool		operator!	(void) const	{ return m_addr ? false : true ; }
	const char*	operator*	(void) const ;
	operator const char*	(void) const ;

	//	Friend operator functions
	friend	_atomval	atomval_hold	(const hzUrl& url) ;
	friend	std::ostream&	operator<<	(std::ostream& os, const hzUrl& url) ;
} ;

//	Convienent globals always at null
extern	const	hzUrl	_hz_null_hzUrl ;		//	Null URL

#endif	//	hzUrl_h
