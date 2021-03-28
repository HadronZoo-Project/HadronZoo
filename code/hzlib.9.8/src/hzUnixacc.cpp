//
//	File:	hzUnixDB.cpp
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
//	A set of functions to interigate UNIX system files to search for and provide information on users and user groups.
//

#include <fstream>

#include "hzCtmpls.h"
#include "hzProcess.h"
#include "hzUnixacc.h"

/*
**	Static Variables
*/

global	hzMapS<hzString,hzUserinfo*>	_hzGlobal_Userlist ;		//	All UNIX Users
global	hzMapS<hzString,hzGroupinfo*>	_hzGlobal_Grouplist ;		//	All UNIX User groups

/*
**	hzUserinfo functions
*/

hzEcode	hzUserinfo::Load	(void)
{
	//	Build system user information database form /etc/passwd
	//
	//	Arguments:	None.
	//
	//	Returns:	E_OPENFAIL	If the file /etc/passwd could not be opened for reading.
	//				E_OK		If the /etc/passwd file was read.

	_hzfunc("hzUserinfo::Load") ;

	std::ifstream	is ;		//	Input stream

	hzUserinfo*	pUI ;			//	Unix user meta data
	char*		i ;				//	String iterator
	char*		j ;				//	Reference iterator
	char		cvLine[256] ;	//	Getline Buffer

	is.open("/etc/passwd") ;

	if (is.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open passwd file") ;

	for (;;)
	{
		is.getline(cvLine, 256) ;
		if (!is.gcount())
			break ;

		i = cvLine ;

		pUI = new hzUserinfo() ;
		if (!pUI)
			hzexit(_fn, 0, E_MEMORY, "No memory for system user DB") ;

		for (j = i ; *i && *i != ':' ; i++) ;	*i++ = 0 ; pUI->m_Username = j ;
		for (j = i ; *i && *i != ':' ; i++) ;	*i++ = 0 ;
		for (j = i ; *i && *i != ':' ; i++) ;	*i++ = 0 ; pUI->m_nUserID = atoi(j) ;
		for (j = i ; *i && *i != ':' ; i++) ;	*i++ = 0 ; pUI->m_nGroupID = atoi(j) ;
		for (j = i ; *i && *i != ':' ; i++) ;	*i++ = 0 ; pUI->m_Geninfo = j ;
		for (j = i ; *i && *i != ':' ; i++) ;	*i++ = 0 ; pUI->m_Homedir = j ;
		for (j = i ; *i && *i != ':' ; i++) ;	*i++ = 0 ; pUI->m_Shell = j ;

		_hzGlobal_Userlist.Insert(pUI->m_Username, pUI) ;
	}

	is.close() ;
	is.clear() ;

	return E_OK ;
}

hzUserinfo*	hzUserinfo::Locate	(hzString& uname)
{
	//	Locate UNIX user by username
	//
	//	Arguments:	1)	uname	The UNIX/Linux username
	//
	//	Returns:	Pointer to the usr info

	return _hzGlobal_Userlist[uname] ;
}

hzUserinfo*	hzUserinfo::Locate	(uid_t nUserID)
{
	//	Locate UNIX user by user id
	//
	//	Arguments:	1)	nUserID	The UNIX user number
	//
	//	Returns:	Pointer to the usr info

	hzUserinfo*	pUI ;		//	UNIX user pointer
	uint32_t	nIndex ;	//	UNIX user iterator

	for (nIndex = 0 ; nIndex < _hzGlobal_Userlist.Count() ; nIndex++)
	{
		pUI = _hzGlobal_Userlist.GetObj(nIndex) ;

		if (pUI->m_nUserID == nUserID)
			return pUI ;
	}

	return 0 ;
}

/*
**	hzGroupinfo functions
*/

hzEcode	hzGroupinfo::Load	(void)
{
	//	Load user groups
	//
	//	Arguments:	None
	//
	//	Returns:	E_OPENFAIL	If the file /etc/group could not be opened
	//				E_OK		If the user groups have been read in

	_hzfunc("hzGroupinfo::Load") ;

	std::ifstream	is ;		//	Input stream
	hzGroupinfo*	pGI ;		//	UNIX group pointer

	char*	i ;					//	Line buffer iterator
	char*	cpGroupname ;		//	User group partial string
	char*	cpGroupID ;			//	User group partial string
	char	cvLine[256] ;		//	Line buffer

	is.open("/etc/group") ;

	if (is.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open group file") ;

	for (;;)
	{
		is.getline(cvLine, 256) ;
		if (!is.gcount())
			break ;

		pGI = new hzGroupinfo() ;
		if (!pGI)
			hzexit(_fn, 0, E_MEMORY, "No memory for system user DB") ;

		i = cvLine ;

		for (cpGroupname = i ; *i && *i != ':' ; i++) ;	*i++ = 0 ;
		for (cpGroupID = i ;  *i && *i != ':' ; i++) ;	*i++ = 0 ;

		pGI->m_Groupname = cpGroupname ;
		pGI->m_nGroupID = atoi(cpGroupID) ;

		_hzGlobal_Grouplist.Insert(pGI->m_Groupname, pGI) ;
	}

	is.close() ;
	return E_OK ;
}

hzGroupinfo*	hzGroupinfo::Locate	(hzString& Groupname)
{
	//	Locate user group by name
	//
	//	Arguments:	1)	Groupname	UNIX user group name
	//
	//	Returns:	Pointer to user group info

	return _hzGlobal_Grouplist[Groupname] ;
}

hzGroupinfo*	hzGroupinfo::Locate	(gid_t nGroupID)
{
	//	Locate user group by number
	//
	//	Arguments:	1)	nGroupID	UNIX user group number
	//
	//	Returns:	Pointer to user group info

	hzGroupinfo*	pGI ;		//	UNIX group pointer
	uint32_t		nIndex ;	//	UNIX group iterator

	for (nIndex = 0 ; nIndex < _hzGlobal_Grouplist.Count() ; nIndex++)
	{
		pGI = _hzGlobal_Grouplist.GetObj(nIndex) ;

		if (pGI->m_nGroupID == nGroupID)
			return pGI ;
	}

	return 0 ;
}
