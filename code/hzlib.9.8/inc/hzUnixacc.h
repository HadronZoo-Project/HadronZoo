//
//	File:	hzUnixacc.h
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

#ifndef hzUnixacc_h
#define hzUnixacc_h

#include "hzString.h"

class	hzUserinfo
{
	//	Category:	System
	//
	//	Information about a UNIX user account

	uid_t		m_nUserID ;			//	Numeric user ID.
	gid_t		m_nGroupID ;		//	Numeric groupd ID.
	time_t		m_nPwUpdate ;		//	Last Password change time.  
	time_t		m_nExpires ;		//	Account expiration time.  
	hzString	m_Username ;		//	System user's username
	hzString	m_Password ;		//	User's encrypted password.  
	hzString	m_Class ;			//	User's class or type (not implimented yet)
	hzString	m_Geninfo ;			//	General information about the user.  
	hzString	m_Homedir ;			//	User's home directory.
	hzString	m_Shell ;			//	Path of UNIX shell program.

public:
	hzUserinfo	(void)
	{
		m_nUserID = 0 ;
		m_nGroupID = 0 ;
		m_nPwUpdate = 0 ;
		m_nExpires = 0 ;
	}

	~hzUserinfo	(void)
	{
	}

	//	Get functions

	const char*	GetUsername	(void)	{ return *m_Username ; }
	const char*	GetPassword	(void)	{ return *m_Password ; }
	const char*	GetClass	(void)	{ return *m_Class ; }
	const char*	GetGeninfo	(void)	{ return *m_Geninfo ; }
	const char*	GetHomedir	(void)	{ return *m_Homedir ; }
	const char*	GetShell	(void)	{ return *m_Shell ; }
	uid_t		GetUserID	(void)	{ return m_nUserID ; }
	gid_t		GetGroupID	(void)	{ return m_nGroupID ; }
	time_t		GetPwUpdate	(void)	{ return m_nPwUpdate ; }
	time_t		GetExpires	(void)	{ return m_nExpires ; }

	//	Static functions

	static	hzEcode		Load	(void) ;
	static	hzUserinfo*	Locate	(hzString& Username) ;
	static	hzUserinfo*	Locate	(uid_t nUserID) ;
} ;

/*
**	Group info class
*/

class	hzGroupinfo
{
	//	Category:	System
	//
	//	Information about a group of UNIX users

	gid_t		m_nGroupID ;		//	Numeric groupd ID.
	hzString	m_Groupname ;		//	System user's username

public:
	hzGroupinfo	(void)
	{
		m_nGroupID = 0 ;
	}

	~hzGroupinfo	(void)
	{
	}

	//	Get functions

	const char*	GetGroupname	(void)	{ return *m_Groupname ; }
	gid_t		GetGroupID		(void)	{ return m_nGroupID ; }

	//	Static functions

	static	hzEcode			Load	(void) ;
	static	hzGroupinfo*	Locate	(hzString& Groupname) ;
	static	hzGroupinfo*	Locate	(gid_t nGroupID) ;
} ;

extern	hzMapS<hzString,hzUserinfo*>   _hzGlobal_Userlist ;        //  All UNIX Users
extern	hzMapS<hzString,hzGroupinfo*>  _hzGlobal_Grouplist ;       //  All UNIX User groups

#endif	//	hzUnixacc_h
