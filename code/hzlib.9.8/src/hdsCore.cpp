//
//	File:	hdsCore.cpp
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

//	Description: Dissemino Web-Masters Management Facility
//
//	Resources such as pages, articles and include blocks have inherently unique and human readable identifiers such as pathname and title but internal operation
//	is enhanced by allocation of a unique numeric id. This is best done internally as it is a nuicence to have to allocate ids to these entities in the configs.
//	Instead, the human operator only needs to set the version number of each resource. This will be ver="1" in all cases to begin with but will be increased by
//	1 if editied.
//
//	It is important to understand that Dissemino makes a distinction between resorces that are explicitly defined in the configs and those that are derived from
//	other sources such as uploads from users. While the latter may amount to whole pages or articles to Dissemino they are meaningless binaries to be stored in
//	the database and given addresses accordingly. The regime herein described applies only to config defined resources.
//
//	To this end, Dissemino manages a series of files in the document root as follows:-
//
//	1) website.res.xml: This lists all config defined resources and adds the internally assigned id, the date and time stamp and the version to the pathname and
//	title. The 
//
//	2) website.$$.txt:	Where $$ in this case is the default language code. This file will contain a series of strings extracted from the currently applicable
//						resources.
//
//	If ANY of the above files do not exist in the document root, Dissemino will take the configs as autorative and create a complete set of new files missing files from the configs
//	a veriety of reasons, it is these have practical limitations. In
//	particular, change over time

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>

#include "hzErrcode.h"
#include "hzTokens.h"
#include "hzTextproc.h"
#include "hzDissemino.h"

using namespace std ;

/*
**	Variables
*/

static	hdsTree		s_treeDataModel ;			//	Tree for object model display

extern	hzString	_dsmScript_tog ;			//	Navtree toggle script
extern	hzString	_dsmScript_loadArticle ;	//	Navtree load article script
extern	hzString	_dsmScript_navtree ;		//	Navtree script

/*
**	Dissemino Application Container
*/

Dissemino*	Dissemino::GetInstance	(hzLogger& pLog)
{
	//	Category:	Dissemino System Initialization
	//
	//	Obtain the singleton hdsApp instance
	//
	//	Arguments:	1)	pLog	Pointer to an established logger to log config errors and events
	//
	//	Returns:	Pointer to the singleton HTML application
	//				NULL if the logger is not supplied

	_hzfunc("hdsApp::GetInstance") ;

	if (_hzGlobal_Dissemino)
		hzerr(_fn, HZ_WARNING, E_SETONCE, "hdsApp is a singleton class") ;
	else
	{
		_hzGlobal_Dissemino = new Dissemino() ;
		_hzGlobal_Dissemino->m_pLog = &pLog ;
	}

	return _hzGlobal_Dissemino ;
}

hzEcode	Dissemino::SetCookieName	(const hzString& cookieBase)
{
	//	Category:	Dissemino System Initialization
	//
	//	Sets the application name and base directories for the Dissemino application. These are as per the following arguments:-
	//
	//	Arguments:	1)	appname	The application name. This must be unique as seen by the delta server
	//				2)	baseDir	The base directory. From this a number of directories including the document root, will be set
	//
	//	Returns:	E_SETONCE	If this function has already been called
	//				E_ARGUMENT	If any of the arguments are NULL
	//				E_NOTFOUND	If the base directory does not exist
	//				E_OK		If success

	_hzfunc("Dissemino::SetCookieName") ;

	if (!cookieBase)	return E_ARGUMENT ;
	if (m_CookieName)	return E_SETONCE ;

	hzChain		Z ;		//	For building cookie name
	const char*	i ;		//	Iterator

	Z << "_hz_" ;
	for (i = *cookieBase ; *i ; i++)
	{
		if (*i > CHAR_SPACE)
			Z.AddByte(*i) ;
	}
	m_CookieName = Z ;
	Z.Clear() ;

	return E_OK ;
}

hzEcode	Dissemino::AddApplication	(const hzString& domain, const hzString& baseDir, const hzString& rootFile, uint32_t bOpFlags, uint32_t nPortSTD, uint32_t nPortSSL)
{
	//	Category:	Dissemino System Initialization
	//
	//	Add a Dissemino application
	//
	//	Argument:	appHost	The application hostname e.g. www.mydomain.com. This will be supplied in the header of HTTP requests and is the means by which Dissemino identifies the
	//						application to send the request to.
	//
	//	Returns:	E_DUPLICATE	If the named application already exists
	//				E_OK		If the operation was successful

	_hzfunc("Dissemino::AddApplication") ;

	hdsApp*		pApp ;	//	Dissemino application visible entity belongs to
	hzEcode		rc ;	//	Return code

	if (!domain || !baseDir || !rootFile)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "domain=[%s] baseDir=[%s] rootFile=[%s]", *domain, *baseDir, *rootFile) ;

	if (m_AppsByHost.Exists(domain))
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE) ;

	pApp = hdsApp::GetInstance(*m_pLog) ;
	rc = pApp->Init(domain, baseDir, rootFile) ;

	if (rc == E_OK)
	{
		m_AppsByHost.Insert(domain, pApp) ;
		pApp->m_AppID = m_AppsByHost.Count() ;
		pApp->m_OpFlags |= bOpFlags ;
		pApp->m_nPortSTD = nPortSTD ;
		pApp->m_nPortSSL = nPortSSL ;
		m_AppsByID.Insert(pApp->m_AppID, pApp) ;
	}

	return rc ;
}

hdsApp*	Dissemino::GetApplication	(const hzString& domain)
{
	_hzfunc("Dissemino::GetApplication(str)") ;

	return m_AppsByHost[domain] ;
}

hdsApp*	Dissemino::GetApplication	(uint32_t appId)
{
	_hzfunc("Dissemino::GetApplication(int)") ;

	return m_AppsByID[appId] ;
}

/*
**	Dissemino Applications
*/

hdsApp*	hdsApp::GetInstance	(hzLogger& pLog)
{
	//	Category:	Dissemino System Initialization
	//
	//	Obtain the singleton hdsApp instance
	//
	//	Arguments:	1)	pLog	Pointer to an established logger to log config errors and events
	//
	//	Returns:	Pointer to the singleton HTML application
	//				NULL if the logger is not supplied

	_hzfunc("hdsApp::GetInstance") ;

	hdsApp*		pApp ;	//	Dissemino application visible entity belongs to

	pApp = new hdsApp() ;
	pApp->m_pLog = &pLog ;
	return pApp ;
}

hzEcode	hdsApp::Init	(const hzString& domain, const hzString& baseDir, const hzString& rootFile)
{
	//	Category:	Dissemino System Initialization
	//
	//	Sets the application name, the domain and base directories for the Dissemino application. These are as per the following arguments:-
	//
	//	Arguments:	1)	appname	The application name. This must be unique as seen by the delta server
	//				2)	domain	The domain name the application will act as part or whole website for
	//				3)	baseDir	The base directory. From this a number of directories including the document root, will be set
	//
	//	Returns:	E_SETONCE	If this function has already been called
	//				E_ARGUMENT	If any of the arguments are NULL
	//				E_FORMAT	If the domain name is not a valid domain
	//				E_NOTFOUND	If the base directory does not exist
	//				E_OK		If success

	_hzfunc("hdsApp::Init") ;

	if (m_Domain)
		return E_SETONCE ;

	if (!domain || !baseDir || !rootFile)
		return E_ARGUMENT ;

	m_Appname = domain ;
	m_Domain = domain ;
	m_BaseDir = baseDir ;
	m_RootFile = rootFile ;

	return E_OK ;
}

/*
**	USL (Universal String Label)
*/

hzEcode	hdsUSL::SetBlock	(uint32_t blkNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetBlock") ;

	if (m_Value)		return hzerr(_fn, HZ_WARNING, E_SEQUENCE) ;
	if (blkNo > 255)	return hzerr(_fn, HZ_WARNING, E_RANGE) ;

	m_Value = USL_B_MARK ;
	m_Value |= (blkNo << 14) ;
	return E_OK ;
}

hzEcode	hdsUSL::SetGroup	(uint32_t grpNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetGroup") ;

	if (m_Value)		return hzerr(_fn, HZ_WARNING, E_SEQUENCE) ;
	if (grpNo > 15)		return hzerr(_fn, HZ_WARNING, E_RANGE) ;

	m_Value = USL_G_MARK ;
	m_Value |= (grpNo << 27) ;
	return E_OK ;
}

hzEcode	hdsUSL::SetArticle	(const hdsUSL base, uint32_t artNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetArticle") ;

	if (!(base.m_Value & USL_G_MARK))	return hzerr(_fn, HZ_WARNING, E_TYPE) ;
	if (m_Value)						return hzerr(_fn, HZ_WARNING, E_SEQUENCE) ;
	if (artNo > 16383)					return hzerr(_fn, HZ_WARNING, E_RANGE) ;

	m_Value = base.m_Value ;
	m_Value |= (artNo << 14) ;
	return E_OK ;
}

hzEcode	hdsUSL::SetPage	(uint32_t pageNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetPage") ;

	if (m_Value)		return hzerr(_fn, HZ_WARNING, E_SEQUENCE) ;
	if (pageNo > 16383)	return hzerr(_fn, HZ_WARNING, E_RANGE) ;

	m_Value = USL_P_MARK ;
	m_Value |= (pageNo << 14) ;
	return E_OK ;
}

hzEcode	hdsUSL::SetSubj	(uint32_t subjNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetSubj") ;

	if (m_Value)		return hzerr(_fn, HZ_WARNING, E_SEQUENCE) ;
	if (subjNo > 16383)	return hzerr(_fn, HZ_WARNING, E_RANGE) ;

	m_Value = USL_S_MARK ;
	m_Value |= (subjNo << 14) ;
	return E_OK ;
}

hzEcode	hdsUSL::SetBlockVE	(uint32_t veNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetBlockVE") ;

	if (!(m_Value & USL_B_MARK))	return hzerr(_fn, HZ_WARNING, E_TYPE) ;
	if (veNo & 0xfffffb00)			return hzerr(_fn, HZ_WARNING, E_RANGE) ;

	m_Value |= veNo ;
	return E_OK ;
}

hzEcode	hdsUSL::SetPageVE	(uint32_t veNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetPageVE") ;

	if (!(m_Value & USL_P_MARK))	return hzerr(_fn, HZ_WARNING, E_TYPE) ;
	if (veNo & 0xfffffb00)			return hzerr(_fn, HZ_WARNING, E_RANGE) ;

	m_Value |= veNo ;
	return E_OK ;
}

hzEcode	hdsUSL::SetVE_Pretext	(const hdsUSL base, uint32_t veNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetVE_Pretext") ;

	if (!base.m_Value)	return hzerr(_fn, HZ_WARNING, E_NOINIT) ;
	if (veNo > 8191)
	{
		hzRecep32	r32 ;
		return hzerr(_fn, HZ_WARNING, E_RANGE, "Attempting a VE number of %u on base of %s", veNo, base.Txt(r32)) ;
	}

	m_Value = base.m_Value ;
	m_Value |= (veNo << 1) ;
	m_Value |= USL_X_MASK ;
	return E_OK ;
}

hzEcode	hdsUSL::SetVE_Content	(const hdsUSL base, uint32_t veNo)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsUSL::SetVE_Content") ;

	if (!base.m_Value)	return hzerr(_fn, HZ_WARNING, E_NOINIT) ;
	if (veNo > 8191)
	{
		hzRecep32	r32 ;
		return hzerr(_fn, HZ_WARNING, E_RANGE, "Attempting a VE number of %u on base of %s", veNo, base.Txt(r32)) ;
	}

	m_Value = base.m_Value ;
	m_Value |= (veNo << 1) ;
	return E_OK ;
}

hzEcode	hdsUSL::SetByText	(const char* usl)
{
	//	Category:	Dissemino System Initialization
	//
	//	Set a USL by text value. The only source of the text value anticipated is during mass string import after an external language translation.

	_hzfunc("hdsUSL::SetByText") ;

	return E_OK ;
}

char*	hdsUSL::Txt (hzRecep32& r) const
{
	uint32_t	co ;	//	Container number (from bits 08 thru 21 inclusive)
	uint32_t	ve ;	//	Visual entity number (from bits 22 thru 30 inclusive)

	co = (m_Value & USL_C_MASK) >> 14 ;
	ve = (m_Value & USL_V_MASK) >> 1 ;

	if (m_Value & USL_G_MARK)
	{
		if (m_Value & USL_X_MASK)
			sprintf(r.m_buf, "g%u.%u.p%u", (m_Value & USL_G_MASK)>>27, co, ve) ;
		else
			sprintf(r.m_buf, "g%u.%u.c%u", (m_Value & USL_G_MASK)>>27, co, ve) ;
		return r.m_buf ;
	}

	if (m_Value & USL_P_MARK)
	{
		if (!ve)
			sprintf(r.m_buf, "p%u", co) ;
		else
		{
			if (m_Value & USL_X_MASK)
				sprintf(r.m_buf, "p%u.p%u", co, ve) ;
			else
				sprintf(r.m_buf, "p%u.c%u", co, ve) ;
		}
		return r.m_buf ;
	}

	if (m_Value & USL_S_MARK)
	{
		sprintf(r.m_buf, "s%u", co) ;
		return r.m_buf ;
	}

	if (m_Value & USL_B_MARK)
	{
		if (!ve)
			sprintf(r.m_buf, "b%u", co) ;
		else
		{
			if (m_Value & USL_X_MASK)
				sprintf(r.m_buf, "b%u.p%u", co, ve) ;
			else
				sprintf(r.m_buf, "b%u.c%u", co, ve) ;
		}
		return r.m_buf ;
	}

	sprintf(r.m_buf, "?%u.%u", co, ve) ;
	return r.m_buf ;
}

/*
**	Visual Entities
*/

static	uint32_t	s_abs_coll = 0 ;		//	Absoute collection id

hdsVE::hdsVE	(void)
{
	m_pApp = 0 ;
	m_Children = m_Sibling = 0 ;
	m_BgColor = DS_NULL_COLOR ;
	m_FgColor = DS_NULL_COLOR ;
	m_Access = 0 ;
	m_Line = 0 ;
	m_Indent = 0 ;
	m_nAttrs = 0 ;
	m_nChildren = 0 ;
	m_flagVE = 0 ;
	m_VID = 0 ;
}

hdsBlock::hdsBlock	(hdsApp* pApp)
{
	//	m_pRoot = 0 ;
	InitVE(pApp) ;
	m_EID = ++s_abs_coll ;
	m_bScriptFlags = 0 ;
}

hdsPage::hdsPage	(hdsApp* pApp)
{
	//m_pParentForm = 0 ;
	m_pApp = pApp ;
	m_EID = ++s_abs_coll ;
	m_Access = 0 ;
	m_flagVE = 0 ;
	m_HitCount = 0 ;
	m_bScriptFlags = 0 ;
	m_BgColor = 0 ;
	m_Line = 0 ;
	m_Width = m_Height = m_Left = m_Top = 0 ;
}

hzEcode hdsVE::InitVE	(hdsApp* pApp)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsVE::InitVE") ;

	if (!pApp)
		//return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No Application supplied") ;
		hzexit(_fn, *m_Tag, E_CORRUPT, "No Application supplied") ;
	if (!pApp->m_AppID)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Supplied application has no app ID") ;

	if (m_pApp)
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Duplicate call %p but already at %p", pApp, m_pApp) ;
		//return E_DUPLICATE ;

	m_pApp = pApp ;
	m_pApp->m_arrVEs.Add(this) ;
	m_VID = m_pApp->m_arrVEs.Count() ;

	return E_OK ;
}

hzEcode hdsVE::SetSibling	(hdsVE* pSib)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsVE::SetSibling") ;

	if (pSib == this)	hzexit(_fn, *m_Tag, E_CORRUPT, "Attempt to set sibling of this to this") ;
	if (m_Sibling)		hzexit(_fn, *m_Tag, E_CORRUPT, "Already have sibling") ;

	m_Sibling = pSib->m_VID ;
}

hzEcode hdsVE::AddChild	(hdsVE* pChild)
{
	//	Category:	Dissemino System Initialization
	//
	//	Add a child to this visible entity.
	//
	//	Arguments:	1)	pChild	The pointer to the child visible entity
	//
	//	Returns:	E_OK

	_hzfunc("hdsVE::AddChild") ;

	hdsVE*	pVE ;	//	Visible entity pointer

	if (!pChild)
		hzexit(_fn, *m_Tag, E_ARGUMENT, "Null child supplied to Visible Entity") ;

	if (m_flagVE & VE_COMPLETE)
		hzexit(_fn, *m_Tag, E_SEQUENCE, "Visible Entity is already complete, cannot add child %s", *pChild->m_Tag) ;

	if (pChild == this)
		hzexit(_fn, *m_Tag, E_CORRUPT, "Child is same as parent") ;

	//	if (pChild->Children())
	//		threadLog("hdsVE::AddChild. Child %s (line %d, vid %d) being added to vis-ent %s (line %d, vid %d) is alredy a parent of %d children\n",
	//			*pChild->m_Tag, pChild->m_Line, pChild->m_VID, *m_Tag, m_Line, m_VID, pChild->m_Children) ;
	//	else
	//		threadLog("hdsVE::AddChild. Child %s (line %d, vid %d) being added to vis-ent %s (line %d, vid %d) OK\n",
	//			*pChild->m_Tag, pChild->m_Line, pChild->m_VID, *m_Tag, m_Line, m_VID) ;

	if (!m_Children)
		m_Children = pChild->m_VID ;
	else
	{
		for (pVE = Children() ; pVE->Sibling() ; pVE = pVE->Sibling())
		{
			if (pVE == pChild)
			{
				threadLog("%s. Have duplicate\n", *_fn) ;
				return E_OK ;
			}
		}
		pVE->SetSibling(pChild) ;
	}

	m_nChildren++ ;
	return E_OK ;
}

hdsVE*	hdsVE::Children	(void) const
{
	if (!m_Children)
		return 0 ;
	if (!m_pApp)
		return 0 ;

	return m_pApp->m_arrVEs[m_Children-1] ;
}

hdsVE*	hdsVE::Sibling	(void) const
{
	if (!m_Sibling)
		return 0 ;
	if (!m_pApp)
		return 0 ;

	return m_pApp->m_arrVEs[m_Sibling-1] ;
}

hzEcode hdsVE::AddAttr	(const hzString& name, const hzString& value)
{
	//	Category:	Dissemino System Initialization
	//
	//	Add an attribute to the visible entity
	//

	_hzfunc("hdsVE::AddAttr") ;

	const char*		i ;				//	Attribute value iterator
	hzString		pcntEnt ;		//	Percent entity
	hzFixPair		pa ;			//	Attribute
	hzEcode			rc = E_OK ;		//	Return code

	if (!m_pApp)
		return E_CORRUPT ;

	//	Check attribute for impact on flags: Does it make the tag active?
	for (i = *value ; *i ; i++)
	{
		if (*i != CHAR_PERCENT)
			continue ;

		if (i[1] == CHAR_PERCENT)
			{ i++ ; continue ; }

		if (IsAlpha(i[1]) && i[2] == CHAR_COLON)
		{
			if (m_pApp->IsPcEnt(pcntEnt, i))
				m_flagVE |= VE_AT_ACTIVE ;
			else
				{ rc = E_SYNTAX ; break ; }
			i += 2 ;
		}
	}

	//	Add the attribute
	pa.name = name ;
	pa.value = value ;
	m_nAttrs++ ;

	rc = m_pApp->m_VE_attrs.Insert(m_VID, pa) ;
	return rc ;
}

hzEcode	hdsBlock::AddVisent	(hdsVE* pChild)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsBlock::AddVisent") ;

	uint32_t	nV ;	//	VE iterator

	if (!pChild)					hzexit(_fn, m_Refname, E_ARGUMENT, "Null child supplied to block") ;
	if (m_flagVE & VE_COMPLETE)		hzexit(_fn, m_Refname, E_SEQUENCE, "Block is already complete, cannot add more entities") ;
	if (pChild == this)				hzexit(_fn, m_Refname, E_CORRUPT, "Case 1 Child is include block pointing to itself") ;
	if (m_VID == pChild->m_VID)		hzexit(_fn, m_Refname, E_CORRUPT, "Case 2 Child is include block pointing to itself") ;

	//	Run through array looking for duplicates
	for (nV = 0 ; nV < m_VEs.Count() ; nV++)
	{
		if (m_VEs[nV] == pChild)
			hzexit(_fn, m_Refname, E_CORRUPT, "Child is already in include block") ;
	}

	//	threadLog("%s. ADDED CHILD %s TO XINCLUDE %p %s\n", *_fn, *pChild->m_Tag, this, *m_Refname) ;
	m_VEs.Add(pChild) ;

	return E_OK ;
}

hzEcode	hdsArticleStd::AddVisent	(hdsVE* pChild)
{
	//	Category:	Dissemino System Initialization
	//
	_hzfunc("hdsArticle::AddVisent") ;

	uint32_t	nV ;	//	VE iterator

	if (!pChild)
		hzexit(_fn, m_Refname, E_ARGUMENT, "Null child supplied to article") ;

	if (m_flagVE & VE_COMPLETE)
		hzexit(_fn, m_Refname, E_SEQUENCE, "Article is already complete, cannot add %s", *pChild->m_Tag) ;

	//	Run through array looking for duplicates
	for (nV = 0 ; nV < m_VEs.Count() ; nV++)
	{
		if (m_VEs[nV] == pChild)
			hzexit(_fn, m_Refname, E_CORRUPT, "Child %s is already in include block", *pChild->m_Tag) ;
	}

	//	threadLog("%s. ADDED CHILD %s TO ARTICLE %p %s\n", *_fn, *pChild->m_Tag, this, *m_Refname) ;
	m_VEs.Add(pChild) ;

	return E_OK ;
}

hzEcode	hdsArticleStd::AddFormref	(hdsFormref* pFR)
{
	//	Category:	Dissemino System Initialization
	//
	if (!m_pFormref)
		{ m_pFormref = pFR ; m_flagVE |= VE_CT_ACTIVE ; return E_OK ; }
	if (m_pFormref != pFR)
		return E_DUPLICATE ;
	return E_OK ;
}

void	hdsApp::_assignveids_r	(hdsVE* pVE, const hdsUSL& base, uint32_t& flags, uint32_t& nId)
{
	//	Category:	Dissemino System Initialization
	//
	//	Recursive support function to non recursive AssignVEids() function
	//	This will assign numeric 'positional' identifiers to a VE according to incidence within a page, article or block
	//
	//	Arguments:	1)	pVE		Pointer to the visible entity
	//				2)	base	HTML/Internet application base name
	//
	//	Returns:	None

	_hzfunc("hdsApp::_assignveids_r") ;

	hdsVE*	pSub ;		//	Visible entity (child)
	hzEcode	rc ;		//	Return code

	if (!pVE)
		hzexit(_fn, 0, E_CORRUPT, "No VE supplied") ;

	//	Set flags so if any VE active, host becomes active
	if (pVE->m_flagVE & VE_AT_ACTIVE)	flags |= VE_AT_ACTIVE ;
	if (pVE->m_flagVE & VE_PT_ACTIVE)	flags |= VE_PT_ACTIVE ;
	if (pVE->m_flagVE & VE_CT_ACTIVE)	flags |= VE_CT_ACTIVE ;

	//	Ensure no repeat
	if (pVE->m_flagVE & VE_EVALUATED)
		return ;
	pVE->m_flagVE |= VE_EVALUATED ;

	//	Assing uid
	nId++ ;

	//	Handle pre-text
	if (pVE->m_strPretext)
		pVE->m_strPretext.DelWhiteTrail() ;
	if (pVE->m_strContent)
		pVE->m_strContent.DelWhiteTrail() ;

	if (pVE->m_strPretext)
	{
		rc = pVE->m_usiPretext.SetVE_Pretext(base, nId) ;
		if (rc != E_OK)
			threadLog("%s. Failed to set USL for %s String Pretext [%s]\n", *_fn, *pVE->m_Tag, *pVE->m_strPretext) ;
		else
		{
			if (m_pDfltLang->m_LangStrings.Exists(pVE->m_usiPretext))
				threadLog("%s. Duplicate String Pretext [%s]\n", *_fn, *pVE->m_strPretext) ;
			else
				m_pDfltLang->m_LangStrings.Insert(pVE->m_usiPretext, pVE->m_strPretext) ;
		}
	}

	//	Handle content
	if (pVE->m_strContent)
	{
		rc = pVE->m_usiContent.SetVE_Content(base, nId) ;
		if (rc != E_OK)
			threadLog("%s. Failed to set USL for %s String Content [%s]\n", *_fn, *pVE->m_Tag, *pVE->m_strContent) ;
		else
		{
			if (m_pDfltLang->m_LangStrings.Exists(pVE->m_usiContent))
				threadLog("%s. Duplicate String Content [%s]\n", *_fn, *pVE->m_strContent) ;
			else
				m_pDfltLang->m_LangStrings.Insert(pVE->m_usiContent, pVE->m_strContent) ;
		}
	}

	//	Recurse
	for (pSub = pVE->Children() ; pSub ; pSub = pSub->Sibling())
	{
		nId++ ;
		_assignveids_r(pSub, base, flags, nId) ;
	}
}

void	hdsApp::AssignVisentIDs	(hzArray<hdsVE*>& listVE, uint32_t& flags, hdsUSL& base)
{
	//	Category:	Dissemino System Initialization
	//
	//	As the assignment of positional identifiers to a VE is a recursive process, this function acts as the root.
	//
	//	Arguments:	1)	LVE		List of visual entities
	//				2)	flags	Cumulative page operational flags
	//				3)	base	HTML/Internet application base name
	//
	//	Returns:	None

	_hzfunc("hdsApp::AssignVEids(2)") ;

	hzList<hdsVE*>::Iter	vi ;	//	Visible entity iterator

	hdsVE*		pVE ;				//	Visible entity pointer
	uint32_t	n ;					//	Visible entity iterator
	uint32_t	nId = 0 ;			//	Visible entity id. Set by _assignveids()
	hzRecep32	r32 ;				//	For USL text value

	m_pLog->Out("%s. PROCESSING VISUAL ENTITY %s. flags %d\n", *_fn, base.Txt(r32), flags) ;

	for (n = 0 ; n < listVE.Count() ; n++)
	{
		pVE = listVE[n] ;

		_assignveids_r(pVE, base, flags, nId) ;
	}
}

hdsFldspec::hdsFldspec	(void)
{
	m_pType = 0 ;
	nSize = 0 ;
	nCols = nRows = 0 ;
	nFldSeq = nExpSeq = 0 ;
	//	_hzGlobal_numFldspecs++ ;
}

hdsFldspec::~hdsFldspec (void)
{
	//	_hzGlobal_numFldspecs-- ;
}

hdsFldspec&	hdsFldspec::operator=	(const hdsFldspec& op)
{
	m_pType = op.m_pType ;
	m_Refname = op.m_Refname ;
	m_Desc = op.m_Desc ;
	m_Default = op.m_Default ;
	valJS = op.valJS ;
	m_Source = op.m_Source ;
	htype = op.htype ;
	valJS = op.valJS ;
	nSize = op.nSize ;
	nCols = op.nCols ;
	nRows = op.nRows ;
	nFldSeq = op.nFldSeq ;
	nExpSeq = op.nExpSeq ;
	return *this ;
}

hdsHtag::hdsHtag		(hdsApp* pApp)	{ InitVE(pApp) ; }
hdsXtag::hdsXtag		(hdsApp* pApp)	{ InitVE(pApp) ; }
hdsXdiv::hdsXdiv		(hdsApp* pApp)	{ InitVE(pApp) ; }
hdsCond::hdsCond		(hdsApp* pApp)	{ InitVE(pApp) ; }
hdsNavbar::hdsNavbar	(hdsApp* pApp)	{ InitVE(pApp) ; }
hdsArtref::hdsArtref	(hdsApp* pApp)	{ InitVE(pApp) ; m_Show = 0 ; }

hdsField::hdsField		(hdsApp* pApp)
{
	_hzfunc("hdsField::hdsField") ;

	InitVE(pApp) ;
	m_pClass = 0 ;
	m_pMem = 0 ;
}

hdsField::~hdsField		(void)
{
	m_pClass = 0 ;
	m_pMem = 0 ;
}

hdsFormref::hdsFormref	(hdsApp* pApp)	{ InitVE(pApp) ; m_ClsId = 0 ; }
hdsFormref::~hdsFormref (void)			{}

hdsButton::hdsButton	(hdsApp* pApp)	{ InitVE(pApp) ; }
hdsButton::~hdsButton	(void)			{} 

hdsRecap::hdsRecap		(hdsApp* pApp)	{ InitVE(pApp) ; }

hdsDirlist::hdsDirlist	(hdsApp* pApp)
{
	InitVE(pApp) ;
	m_pNone = m_pHead = m_pFoot = 0 ;
	m_Width = m_Height = m_nRows = m_Order = 0 ;
}

hdsDirlist::~hdsDirlist (void)
{
	m_pNone = m_pHead = m_pFoot = 0 ;
}

hdsTable::hdsTable		(hdsApp* pApp)
{
	InitVE(pApp) ;
	m_pCache = 0 ;
	m_pNone = m_pHead = m_pFoot = 0 ;
	m_nWidth = m_nHeight = m_nRows = 0 ;
	m_bEdit = false ;
}

hdsTable::~hdsTable	(void)
{
	m_pCache = 0 ;
	m_pNone = m_pHead = m_pFoot = 0 ;
}

hzEcode	hdsApp::AddUserType	(const hzString& utname)
{
	//	Category:	Dissemino System Initialization
	//
	//	Reserve the user type name and set an access code mask for that user type.
	//
	//	Argument:	utname	The user type name
	//
	//	Returns:	E_DUPLICATE	If the user type name has already been added to the collection of known user types for the application (m_UserTypes).
	//				E_OK		If the new user type name is added and the aceess mask set.

	_hzfunc("hdsApp::AddUserType") ;

	//hdsUsertype		utype ;		//	User type
	uint32_t		utype ;		//	User type
	uint32_t		x ;			//	Indexes iterator

	if (m_UserTypes.Exists(utname))
		return E_DUPLICATE ;

	//	Set use access code
	//utype.m_Access = 1 ;
	utype = 1 ;
	for (x = 0 ; x < m_UserTypes.Count() ; x++)
		//utype.m_Access *= 2 ;
		utype *= 2 ;

	//utype.m_Refname = utname ;
	//m_UserTypes.Insert(utype.m_Refname, utype) ;
	m_UserTypes.Insert(utname, utype) ;
	//m_pLog->Out("%s. Added USER TYPE %s access %d\n", *_fn, *utype.m_Refname, utype.m_Access) ;
	m_pLog->Out("%s. Added USER TYPE %s access %d\n", *_fn, *utname, utype) ;
	return E_OK ;
}

uint32_t	hdsApp::_calcAccessFlgs	(hzString& a)
{
	//	Category:	Dissemino System Initialization
	//
	//	Determine access flags on the basis of user. No user or a user of 'public' means in the case of a resource, that the resource will always be
	//	visible in the browser unless rendered invisible by other means. A user of 'any' means that the user has to be logged in but it does not in
	//	what capacity. Any other setting must nessesarily be a user type and in this case, only users logged in as that type would see the resource.
	//	If more than one type of user is to be authorized then this has to be stated as user="typeA|typeB"
	//
	//	The display logic as applied to resources is always as follows:-
	//		- If the access value of (the resource) is 0 then show
	//		- If the access value is non-zero then the access value of the user must match on a bitwise AND
	//
	//	To facilitate this public will produce an access value of 0, any (or all) an access value of 0x7fffffff and specific user type will each set
	//	one bit.
	//	
	//	Arguments:	1)	a	The access directive
	//
	//	Returns:	Number being the bitwise access flags

	_hzfunc("hdsApp::_calcAccessFlgs") ;

	hzVect<hzString>	types ;		//	User types in specification

	//hdsUsertype	ut ;			//	User types
	hzString	S ;				//	Working string
	uint32_t	x ;				//	User type selector
	uint32_t	z = 0 ;			//	Calculated net access flags

	if (a == "public")	return ACCESS_PUBLIC ;
	if (a == "none")	return ACCESS_NOBODY ;
	if (a == "admin")	return ACCESS_ADMIN ;
	if (a == "any")		return ACCESS_MASK ;

	if (a.Contains(CHAR_OR))
	{
		SplitStrOnChar(types, a, '|') ;

		for (z = x = 0 ; x < types.Count() ; x++)
		{
			S = types[x] ;
			if (!m_UserTypes.Exists(S))
				return 0xffffffff ;

			//ut = m_UserTypes[S] ;
			//z |= ut.m_Access ;

			z |= m_UserTypes[S] ;
		}
	}
	else
	{
		if (!m_UserTypes.Exists(a))
			return 0xffffffff ;

		//ut = m_UserTypes[a] ;
		//z = ut.m_Access ;

		z = m_UserTypes[a] ;
	}

	m_pLog->Out("set access for %s to %d\n", *a, z) ;
	return z ;
}

hzEcode	hdsApp::_insertPage	(hdsPage* pPage, const char* fn)
{
	//	Category:	Dissemino System Initialization
	//
	//	Unified insert page operation to add pages to various page collections belonging in the hdsApp
	//
	//	Arguments:	1)	pPage	The page to insert
	//				2)	fn		The caller function name
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_insertPage") ;

	hzChain		Z ;			//	For building page HTML (inactive pages only)
	hzChain		X ;			//	For zipping page HTML if applicable
	hdsUSL		usl ;		//	Temp string
	hzString	strVal ;	//	Temp string
	hzEcode		rc = E_OK ;	//	Return code
	hzRecep32	r32 ;		//	For USL text value

	//	By path
	if (rc == E_OK)
	{
		rc = m_ResourcesPath.Insert(pPage->m_Url, pPage) ;
		//rc = m_PagesPath.Insert(pPage->m_Url, pPage) ;
		if (rc != E_OK)
			m_pLog->Out("%s. Could not insert page uid=%s url=%s title=%s into m_ResourcesPath\n", fn, pPage->m_USL.Txt(r32), *pPage->m_Url, *pPage->m_Title) ;
	}

	//	By name
	if (rc == E_OK)
	{
		rc = m_ResourcesName.Insert(pPage->m_Title, pPage) ;
		//rc = m_PagesName.Insert(pPage->m_Title, pPage) ;
		if (rc != E_OK)
			m_pLog->Out("%s. Could not insert page uid=%s url=%s title=%s into m_ResourcesName\n", fn, pPage->m_USL.Txt(r32), *pPage->m_Url, *pPage->m_Title) ;
	}

	//usl = *pPage->m_USL ;
	usl = pPage->m_USL ;
	AssignVisentIDs(pPage->m_VEs, pPage->m_flagVE, usl) ;

	if (pPage->m_flagVE & VE_ACTIVE)
		m_pLog->Out("%s. Assigned VE ids. Page %s (%s, %s) deemed ACTIVE\n", *_fn, usl.Txt(r32), *pPage->m_Title, *pPage->m_Url) ;
	else
	{
		m_pLog->Out("%s. Assigned VE ids. Page %s (%s, %s) deemed INACTIVE\n", *_fn, usl.Txt(r32), *pPage->m_Title, *pPage->m_Url) ;
		pPage->EvalHtml(Z, m_pDfltLang) ;
		Gzip(X, Z) ;
		strVal = Z ; m_pDfltLang->m_rawItems.Insert(pPage->m_USL, strVal) ;
		strVal = X ; m_pDfltLang->m_zipItems.Insert(pPage->m_USL, strVal) ;
	}

	return rc ;
}

/*
**	User Sessions
*/

hdsInfo::hdsInfo	(void)
{
	m_pTree = 0 ;
	//m_pObjects = 0 ;
	m_pObj = 0 ;
	m_pMisc = 0 ;
	m_Access = 0 ;
	m_SubId = 0 ;
	m_UserId = 0 ;
	m_CurrObj = 0 ;
	m_UserRepos = 0 ;
	m_CurrRepos = 0 ;
	m_CurrClass = 0 ;
}

hdsInfo::~hdsInfo	(void)
{
	delete m_pTree ;
	delete m_pObj ;
}

hzEcode	hdsInfo::ObjectAssert	(const hzString& objKey, const hdbClass* pClass)
{
	//	Category:	Dissemino Operation
	//
	//	Assert (create if not found), the required hdbObject
	//
	//	Arguments:	1)	objKey		The application name for the hdbObject instance
	//				2)	classname	The object host class
	//
	//	Returns:	E_CORRUPT	If the class does not exist or the objKey exists but for a different class
	//				E_OK		If the user session either has or can add the object

	hzList<hdbObject*>::Iter	objI ;	//	Object iterator

	//	const hdbClass*	pClass ;		//	Host class of object
	hdbObject*		pObj_test ;		//	Test object pointer

	if (!m_Objects.Count())
	{
		m_pObj = new hdbObject() ;
		m_pObj->Init(objKey, pClass) ;
		m_Objects.Add(m_pObj) ;
		return E_OK ;
	}

	m_pObj = 0 ;
	for (objI = m_Objects ; objI.Valid() ; objI++)
	{
		pObj_test = objI.Element() ;

		if (pObj_test->ObjKey() != objKey)
			continue ;

		m_pObj = pObj_test ;
		break ;
	}

	if (!m_pObj)
	{
		m_pObj = new hdbObject() ;
		m_pObj->Init(objKey, pClass) ;
		m_Objects.Add(m_pObj) ;
	}

	return E_OK ;
}

hdbObject*	hdsInfo::ObjectSelect	(const hzString& objKey)
{
	//	Category:	Dissemino Operation
	//
	//	Return a pointer to an already asserted hdbObject instance
	//
	//	Argument:	objKey	The application name for the hdbObject instance
	//
	//	Returns:	Pointer to the hdbObject if it has been asserted.
	//				Null otherwise.

	hzList<hdbObject*>::Iter	objI ;	//	Object iterator

	hdbObject*	pObj_test ;		//	Test object pointer

	m_pObj = 0 ;
	for (objI = m_Objects ; objI.Valid() ; objI++)
	{
		pObj_test = objI.Element() ;

		if (pObj_test->ObjKey() != objKey)
			continue ;

		m_pObj = pObj_test ;
		break ;
	}

	return m_pObj ;
}

hzEcode	hdsInfo::ObjectClose	(const hzString& objKey)
{
	//	Category:	Dissemino Operation
	//
	//	Remove the hdbObject from the user session
	//
	//	Argument:	objKey	The application name for the hdbObject instance
	//
	//	Returns:	E_NOTFOUND	If the named object does not exist in the user session
	//				E_OK		If the named object was removed
}

hzEcode	hdsApp::AddCIFunc	(hzEcode (*pFunc)(hzHttpEvent*), const hzString url, uint32_t access, HttpMethod eMethod)
{
	//	Category:	Dissemino System Initialization
	//
	//	Add a function that effects a Dissemino page to the application's resource map. In this case only a URL is available and not a resource name so only the m_ResourcesPath map
	//	is aggregated.
	//
	//	Arguments:	1)	pFunc	Pointer to the function. This must return hzEcode and accept a hzHttpEvent pointer as its only argument
	//				2)	url		The URL the function is associated with
	//				3)	access	The access level the website user is expected to have to access the resource
	//				4)	method	Either HTTP_GET (if the function shows a page) or HTTP_POST if the function acts as form handler 
	//
	//	Returns:	E_DUPLICATE	If the URL is already in use
	//				E_OK		If the operation was successful

	_hzfunc("hdsApp::AddCIFunc") ;

	hdsCIFunc*		pCIF ;			//	C-Interface function
	hdsResource*	pDupRes ;		//	For illegal dupliacte reporting, existing article by name

	if (m_ResourcesPath.Exists(url))
	{
		pDupRes = m_ResourcesPath[url] ;
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "%s Duplicate resource (%s) previous URL %s\n", *_fn, *url, pDupRes->TxtURL()) ;
	}

	pCIF = new hdsCIFunc() ;
	pCIF->m_Url = url ;
	pCIF->m_pFunc = pFunc ;
	pCIF->m_Access = access ;

	m_ResourcesPath.Insert(url, pCIF) ;
	return E_OK ;
}

hzEcode	hdsApp::SetLoginPost	(const hzString& post, const hzString& fail, const hzString& auth, const hzString& resume)
{
	//	Category:	Dissemino System Initialization
	//
	//	Add the login POST URL. This 

	m_LoginPost = post ;		//	The URL to which login form submissions must be posted
	m_LoginFail = fail ;		//	The URL the user will be directed to in event of username/password mismatch
	m_LoginAuth = auth ;		//	The URL the user will be directed to in event of successful authentication
	m_LoginResume = resume ;	//	The URL the user will be directed to in event of successful session resumption

	return E_OK ;
}
#if 0
hzEcode	hdsApp::SetLoginAJAX	(const hzString& cmd)
{
	m_LoginAJAX = cmd ;			//	The URL to which AJAX login form submissions must be posted

	return E_OK ;
}
#endif
