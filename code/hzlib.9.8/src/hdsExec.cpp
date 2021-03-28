//
//	File:	hdsExec.cpp
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

//	Description:
//
//	Implementation of Dissemino forms. All the important executive actions occur as a result of data submitted in forms so functions relating to Dissemino forms
//	have been groupd in this file. Included are the hdsApp member functions responsible for reading form definitons and commands from the configs.

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>

#include "hzErrcode.h"
//#include "hzFsTbl.h"
#include "hzTokens.h"
#include "hzDissemino.h"

using namespace std ;

/*
**	SECTION 1:	Exec/Form Config Functions
*/

const char*	Exec2Txt	(Exectype eType)
{
	//	Category:	Diagnostics

	static const char*	_exec_types	[] =
	{
		"EXEC_NULL",
		"EXEC_SENDEMAIL",
		"EXEC_SETVAR",
		"EXEC_ADDUSER",
		"EXEC_LOGON",
		"EXEC_TEST",
		"EXEC_EXTRACT",
		"EXEC_OBJ_TEMP",
		"EXEC_OBJ_START",
		"EXEC_OBJ_FETCH",
		"EXEC_OBJ_IMPORT",
		"EXEC_OBJ_EXPORT",
		"EXEC_OBJ_SETMBR",
		"EXEC_OBJ_COMMIT",
		"EXEC_OBJ_CLOSE",
		"EXEC_TREE_DCL",
		"EXEC_TREE_HEAD",
		"EXEC_TREE_ITEM",
		"EXEC_TREE_FORM",
		"EXEC_TREE_SYNC",
		"EXEC_TREE_DEL",
		"EXEC_TREE_EXP",
		"EXEC_TREE_CLR",
		"EXEC_SRCH_PAGES",
		"EXEC_SRCH_REPOS",
		"EXEC_FILESYS",
		"EXEC_INVALID"
	} ;

	switch	(eType)
	{
	case EXEC_NULL:			return _exec_types[0] ;
	case EXEC_SENDEMAIL:	return _exec_types[1] ;
	case EXEC_SETVAR:		return _exec_types[2] ;
	case EXEC_ADDUSER:		return _exec_types[3] ;
	case EXEC_LOGON:		return _exec_types[4] ;
	case EXEC_TEST:			return _exec_types[5] ;
	case EXEC_EXTRACT:		return _exec_types[6] ;
	case EXEC_OBJ_TEMP:		return _exec_types[7] ;
	case EXEC_OBJ_START:	return _exec_types[8] ;
	case EXEC_OBJ_FETCH:	return _exec_types[9] ;
	case EXEC_OBJ_IMPORT:	return _exec_types[10] ;
	case EXEC_OBJ_EXPORT:	return _exec_types[11] ;
	case EXEC_OBJ_SETMBR:	return _exec_types[12] ;
	case EXEC_OBJ_COMMIT:	return _exec_types[13] ;
	case EXEC_OBJ_CLOSE:	return _exec_types[14] ;
	case EXEC_TREE_DCL:		return _exec_types[15] ;
	case EXEC_TREE_HEAD:	return _exec_types[16] ;
	case EXEC_TREE_ITEM:	return _exec_types[17] ;
	case EXEC_TREE_FORM:	return _exec_types[18] ;
	case EXEC_TREE_SYNC:	return _exec_types[19] ;
	case EXEC_TREE_DEL:		return _exec_types[20] ;
	case EXEC_TREE_EXP:		return _exec_types[21] ;
	case EXEC_TREE_CLR:		return _exec_types[22] ;
	case EXEC_SRCH_PAGES:	return _exec_types[23] ;
	case EXEC_SRCH_REPOS:	return _exec_types[24] ;
	case EXEC_FILESYS:		return _exec_types[25] ;
	}

	return _exec_types[26] ;
}

hdsFormhdl::hdsFormhdl	(void)
{
	m_pFormdef = 0 ;
	m_pCompletePage = 0 ;
	m_pFailDfltPage = 0 ;
	m_Access = 0 ;
	m_flgFH = 0 ;
}

hdsFormhdl::~hdsFormhdl	(void)
{
	hzList<hdsExec*>::Iter	ei ;	//	Command iterator

	hdsExec*	pExec ;				//	Current instruction

	for (ei = m_Exec ; ei.Valid() ; ei++)
	{
		pExec = ei.Element() ;

		delete pExec ;
	}
}

#if 0
hzEcode	hdsApp::_readExec	(hzXmlNode* pN, hzList<hdsExec*>& execList, hdsPage* pPage, hdsFormhdl* pFhdl)
hzEcode	hdsApp::_readFormHdl	(hzXmlNode* pN)
hzEcode	hdsApp::_readFormDef	(hzXmlNode* pN)
hdsFormref*	hdsApp::_readFormDef	(hzXmlNode* pN, hdsResource* pLR)
hdsFormref*	hdsApp::_readFormRef	(hzXmlNode* pN, hdsResource* pPage)
hdsVE*	hdsApp::_readField	(hzXmlNode* pN, hdsFormdef* pForm)
hdsVE*	hdsApp::_readXhide	(hzXmlNode* pN, hdsFormdef* pForm)
hzEcode	hdsApp::_readResponse	(hzXmlNode* pN, hdsFormhdl* pFhdl, hzString& pageGoto, hdsResource** pPageGoto)
#endif

/*
**	SECTION 2:	Exec/Form Execution Functions
*/

hzEcode	hdsExec::SendEmail	(hzChain& errorReport, hzHttpEvent* pE)
{
	_hzfunc("hdsExec::SendEmail") ;

	hzEmail		msg ;		//	Email instance
	hzEmaddr	emaFrom ;	//	Formal email address
	hzEmaddr	emaTo ;		//	Formal email address
	hzString	param1 ;	//	Used for sender address
	hzString	param2 ;	//	Used for recipient address
	hzString	param3 ;	//	Used for message subject
	hzString	param4 ;	//	Used for message content
	hzString	S ;			//	Intermediate string
	hzEcode		rc ;		//	Return code

	param1 = m_pApp->m_ExecParams[m_FstParam] ;
	param2 = m_pApp->m_ExecParams[m_FstParam+1] ;
	param3 = m_pApp->m_ExecParams[m_FstParam+2] ;
	param4 = m_pApp->m_ExecParams[m_FstParam+3] ;

	S = m_pApp->ConvertText(param1, pE) ;
	if (!S)
		{ errorReport.Printf("%s. No sender supplied (%s)\n", *_fn, *param1) ; return E_NODATA ; }
	emaFrom = S ;
	if (!emaFrom)
		{ errorReport.Printf("%s. Bad sender address %s (%s)\n", *_fn, *S) ; return E_BADADDRESS ; }

	S = m_pApp->ConvertText(param2, pE) ;
	if (!S)
		{ errorReport.Printf("%s. No recipient supplied (%s)\n", *_fn, *param2) ; return E_NODATA ; }
	emaTo = S ;
	if (!emaTo)
		{ errorReport.Printf("%s. Bad recipient address %s (%s)\n", *_fn, *S) ; return E_BADADDRESS ; }

	rc = msg.SetSender(emaFrom) ;
	if (rc != E_OK)
		{ errorReport.Printf("%s. Could not set sender to %s, err=%s\n", *_fn, *emaFrom, Err2Txt(rc)) ; return rc ; }
	errorReport.Printf("%s. Set sender to %s\n", *_fn, *emaFrom) ;

	//	Establish recipient
	rc = msg.AddRecipient(emaTo) ;
	if (rc != E_OK)
		{ errorReport.Printf("%s. Could not set recipient to %s, err=%s\n", *_fn, *emaTo, Err2Txt(rc)) ; return rc ; }
	errorReport.Printf("%s. Set rcpt to %s\n", *_fn, *emaTo) ;

	//	Establish subject
	S = m_pApp->ConvertText(param3, pE) ;
	rc = msg.SetSubject(S) ;
	if (rc != E_OK)
		{ errorReport.Printf("%s. Could not set subject to %s, err=%s\n", *_fn, *S, Err2Txt(rc)) ; return rc ; }
	errorReport.Printf("%s. Set subject to %s (%s)\n", *_fn, *S, *param3) ;

	//	Establish message body
	msg.m_Text = m_pApp->ConvertText(param4, pE) ;
	errorReport.Printf("%s. Set message body (%d bytes)\n", *_fn, msg.m_Text.Size()) ;

	rc = msg.Compose() ;
	if (rc != E_OK)
		{ errorReport.Printf("%s. Could not compose message, err=%s\n", *_fn, Err2Txt(rc)) ; return rc ; }
	errorReport.Printf("%s. Composed message\n", *_fn) ;

	rc = msg.SendSmtp("127.0.0.1", *m_pApp->m_SmtpUser, *m_pApp->m_SmtpPass) ;
	return rc ;
}

hzEcode	hdsExec::Adduser	(hzChain& errorReport, hzHttpEvent* pE)
{
	//	Explicitly create a new user as an executive step.
	//
	//	This will insert information for a new user into both the specified user repository and the inbult subscriber repository. The sequence is as follows:-
	//
	//	1)	The proposed username must be unique across all users in all user classes. The username must not already exist in the subscriber class.
	//	2)	The address of the subsriber entry is established in order to be inserted in the user class
	//	3)	The user info that belongs in the user class repository is added
	//	4)	The user info that belongs in the subscriber repository is added
	//
	//	If this action fails on the grounds of poor setup, this is a fatal error.
	//	If this action fails on the grounds of insufficient user information, this is a config error as it should not be possible to enter an incomplete user
	//	registration form.
	//	If this action fails on the grounds of username already in existance, send the error page.
	//
	//	The execAdduser instance is created by the <addUser> tag in the configs and should only ever appear in the form handler of a form reserved for
	//	the express purpose of registering new users! Note that adding new users must not be attempted by a series of execCommit directives.
	//
	//	Arguments:	1)	error	The error report chain
	//				2)	pE		The HTTP event pointer

	_hzfunc("hdsExec::Adduser") ;

	hzList	<hzPair>::Iter	pi ;	//	member name/value pairs for user-class object

	const hdbClass*		pSubsClass ;	//	User class
	const hdbClass*		pUserClass ;	//	User class
	hdbObject			subsObj ;		//	Subscriber object
	hdbObject			userObj ;		//	User class object
	hzAtom				atom ;			//	Atom for loading objects
	hzChain				err ;			//	Error report
	hzLogger*			pLog ;			//	Thread logger
	hdbObjCache*		pSubsRepos ;	//	User class repository
	hdbObjCache*		pUserRepos ;	//	User class repository
	hzPair				pair ;			//	Member name/value pair for user-class object
	hzString			uclassName ;	//	Name of user class
	hzString			username ;		//	Extracted/converted username
	hzString			password ;		//	Extracted/converted password
	hzString			emaddress ;		//	Email address
	hzString			S ;				//	Value string
	uint32_t			subsObjId ;		//	New user object id in subscriber cache
	uint32_t			userObjId ;		//	New user object id in user-class cache
	hzEcode				rc = E_OK ;		//	Return code

	/*
	**	Check basics
	*/

	uclassName	= m_pApp->m_ExecParams[m_FstParam] ;
	username	= m_pApp->m_ExecParams[m_FstParam+1] ;
	password	= m_pApp->m_ExecParams[m_FstParam+2] ;
	emaddress	= m_pApp->m_ExecParams[m_FstParam+3] ;

	if (!uclassName)	return hzerr(_fn, HZ_ERROR, E_NOINIT, "No user class specified") ;
	if (!username)		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No user name specified") ;
	if (!password)		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No user password specified") ;
	if (!emaddress)		return hzerr(_fn, HZ_ERROR, E_NOINIT, "No user email specified") ;

	pSubsClass = m_pApp->m_Allusers->Class() ;	//m_ADP.GetPureClass(m_pApp->m_UserBase) ;
	pSubsRepos = m_pApp->m_Allusers ;

	pUserClass = m_pApp->m_ADP.GetPureClass(uclassName) ;
	pUserRepos = (hdbObjCache*) m_pApp->m_ADP.GetObjRepos(uclassName) ;

	if (!pSubsClass)	return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "No subscriber class set up\n") ;
	if (!pSubsRepos)	return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "No subscriver repository\n") ;

	if (!pUserClass)	return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "No user class: Class %s not known as a class\n", *uclassName) ;
	if (!pUserRepos)	return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "No user repository for class %s\n", *uclassName) ;

	subsObj.Init(S, pSubsClass) ;
	userObj.Init(S, pUserClass) ;

	/*
	**	Set up SUBSCRIBER object
	*/

	//	Step 1: Establish username
	rc = m_pApp->PcEntConv(atom, username, pE) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_WARNING, rc, "Error during conversion of username %s", *username) ;
	username = atom.Str() ;

	//	Check it does not already exist
	S = "userName" ;
	rc = m_pApp->m_Allusers->Identify(subsObjId, S, username) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_WARNING, rc, "Error during lookup of user %s", *username) ;

	if (subsObjId)
		return hzerr(_fn, HZ_WARNING, E_DUPLICATE, "User %s already exists", *username) ;

	pLog = GetThreadLogger() ;

	//	Set the username in the subscriber object
	subsObj.SetValue(SUBSCRIBER_USERNAME, atom) ;
	pLog->Log(_fn, "Set subscriber username to %s\n", *atom.Str()) ;

	//	Step 2: Establish email address
	rc = m_pApp->PcEntConv(atom, emaddress, pE) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_WARNING, rc, "Error during conversion of password %s", *emaddress) ;
	subsObj.SetValue(SUBSCRIBER_EMAIL, atom) ;
	pLog->Log(_fn, "Set subscriber email %s to %s\n", *emaddress, *atom.Str()) ;

	//	Step 3: Establish password
	rc = m_pApp->PcEntConv(atom, password, pE) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_WARNING, rc, "Error during conversion of password %s", *password) ;
	subsObj.SetValue(SUBSCRIBER_PASSWORD, atom) ;
	pLog->Log(_fn, "Set subscriber password %s to %s\n", *password, *atom.Str()) ;

	//	Step 4: Set userUID. This is always the current population of susbsriber cache
	subsObjId = pSubsRepos->Count() + 1 ;
	atom = subsObjId ;
	subsObj.SetValue(SUBSCRIBER_USER_UID, atom) ;
	pLog->Log(_fn, "subscriber UID = %s\n", *atom.Str()) ;

	//	Step 5: Set user adderess - This is always the current population of the user cache
	userObjId = pUserRepos->Count() + 1 ;
	atom = userObjId ;
	subsObj.SetValue(SUBSCRIBER_USERADDR, atom) ;
	pLog->Log(_fn, "user object id = %s\n", *atom.Str()) ;

	//	Step 6: Set user type (user class id)
	atom.SetValue(BASETYPE_STRING, uclassName) ;
	subsObj.SetValue(SUBSCRIBER_USERTYPE, atom) ;
	pLog->Log(_fn, "user class %s\n", *atom.Str()) ;

	/*
	**	Now create USER object
	*/

	atom = subsObjId ;
	userObj.SetValue(0, atom) ;

	/*
	MUST FIX
	for (pi = m_Steps ; pi.Valid() ; pi++)
	{
		pair = pi.Element() ;

		//	Establish member value
		rc = m_pApp->PcEntConv(atom, pair.value, pE) ;
		if (rc != E_OK)
			return hzerr(_fn, HZ_WARNING, rc, "Error during conversion of member value %s %s", *pair.name, *pair.value) ;

		//	Set member value
		pLog->Log(_fn, "%s: Set user object member %s to %s\n", *_fn, *pair.name, *atom.Str()) ;
		userObj.SetValue(pair.name, atom.Str()) ;
	}
	*/

	/*
	**	Now do the inserts
	*/

	rc = pUserRepos->Insert(userObjId, userObj) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_WARNING, rc, "Error during insert of user %s", *username) ;
	pLog->Log(_fn, "%s: Inserted user object member objid %d\n", *_fn, userObjId) ;

	rc = pSubsRepos->Insert(subsObjId, subsObj) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_WARNING, rc, "Error during insert of subscriber %s", *username) ;
	pLog->Log(_fn, "%s: Inserted subs object member objid %d\n", *_fn, subsObjId) ;

	return E_OK ;
}

hzEcode	hdsExec::Logon	(hzChain& errorReport, hzHttpEvent* pE)
{
	//	Log a user in as admin. This is done without password checking. As the supplied m_Uname parameter is always a percent entity, this must be evaluated
	//	before use.
	//
	//	Arguments:	1)	error	The error report chain
	//				2)	pE		The HTTP event pointer
	//
	//	Returns:	E_NOTFOUND	If there is no session or if the username is not established
	//				E_OK		If the username is established

	//	QUERY THIS
	_hzfunc("hdsExec::Logon") ;

	hzAtom			atom ;		//	Atom for fetching values
	//hdsUsertype		utype ;		//	User type (for loging in)
	hdsInfo*		pInfo ;		//	Session
	hdbObjCache*	pCache ;	//	User class repos
	hzString		tmpStr ;	//	Temp string
	hzString		m_Uname ;	//	Username resolved form percent entity
	hzString		unam ;		//	Username resolved form percent entity
	uint32_t		bAccess ;	//	Object id in the base user cache
	uint32_t		objId ;		//	Object id in the base user cache
	hzEcode			rc ;		//	Return code

	m_Uname	= m_pApp->m_ExecParams[m_FstParam] ;

	pInfo = (hdsInfo*) pE->Session() ;
	if (pInfo)
		{ errorReport.Printf("%s. Cannot logon user implied by %s as there is already a session in place\n", *_fn, *m_Uname) ; return E_NOTFOUND ; }

	if (m_Uname[0] != CHAR_PERCENT)
		unam = m_Uname ;
	else
	{
		rc = m_pApp->PcEntConv(atom, m_Uname, pE) ;
		if (rc != E_OK)
			{ errorReport.Printf("%s. Cannot logon user as %s does not evaluate\n", *_fn, *m_Uname) ; return E_NOTFOUND ; }
		unam = atom.Str() ;
	}

	errorReport.Printf("%s. Trying username %s\n", *_fn, *unam) ;
	rc = m_pApp->m_Allusers->Identify(objId, SUBSCRIBER_USERNAME, unam) ;
	if (rc != E_OK)
		{ errorReport.Printf("%s. User %s not found\n", *_fn, *unam) ; return E_NOTFOUND ; }
	if (!objId)
		{ errorReport.Printf("%s. User %s not found case 2\n", *_fn, *unam) ; return E_NOTFOUND ; }

	if (objId > 0)
	{
		pInfo->m_SubId = objId ;

		//	Log user in as a user of the appropriate class

		m_pApp->m_SessCookie.Insert(pE->Cookie(), pInfo) ;

		rc = m_pApp->m_Allusers->Fetchval(atom, SUBSCRIBER_USERTYPE, objId) ;

		if (rc != E_OK)
			errorReport.Printf("%s. Could not fetch subscriber user type with obj id %d\n", *_fn, objId) ;
		else
		{
			tmpStr = atom.Str() ;
			bAccess = m_pApp->m_UserTypes[tmpStr] ;
			pCache = (hdbObjCache*) m_pApp->m_ADP.GetObjRepos(tmpStr) ;

			if (!pCache)
				{ errorReport.Printf("%s. User type [%s] not located\n", *_fn, *tmpStr) ; return E_NOTFOUND ; }
			pInfo->m_UserRepos = pCache->DeltaId() ;

			rc = m_pApp->m_Allusers->Fetchval(atom, SUBSCRIBER_USERADDR, objId) ;
			if (rc != E_OK)
				errorReport.Printf("%s. Could not fetch subscriber user address with obj id %d\n", *_fn, objId) ;
			else
				pInfo->m_UserId = atom.Int32() ;

			if (rc == E_OK)
			{
				pInfo->m_Access &= ACCESS_ADMIN ;
				pInfo->m_Access |= bAccess ;
			}
		}
	}

	return rc ;
}

hzEcode	hdsExec::Extract	(hzChain& errorReport, hzHttpEvent* pE)
{
	//	Perform a text extraction. The scenario will be that the input will be a file bound to the event (an uploaded file in a form submission). The
	//	m_Input member will name the field carrying the file. ...
	//
	//	Arguments:	1)	error	The error report chain
	//				2)	pE		The HTTP event pointer
	//
	//	Returns:	E_NOTFOUND	If there is no session or if the username is not established
	//				E_OK		If the username is established

	_hzfunc("hdsExec::Extract") ;

	hzVect<hzXmlNode*>			vx ;	//	Nodes containing text
	hzList<hzHttpFile>::Iter	fx ;	//	Iterator of submitted files

	hzDocXml		xdoc ;		//	Unzipped docx file loader
	hzHttpFile		hf ;		//	File meta data from event
	hzChain			Z ;			//	Unzipped docx file
	hzChain			D ;			//	Part of docx within and including <w:document> tags
	hzChain			E ;			//	For export of XML doc
	hzChain			T ;			//	Extracted Text
	chIter			zi ;		//	For iteration (aim to ignore larges parts of MicroSoft formats)
	hzXmlNode*		pN ;		//	Node pointer
	const char*		i ;			//	For derivation
	hzString		m_Input ;	//	Name of field carrying the submitted file
	hzString		m_Target ;	//	Name of field carrying the target file
	hzString		fldname ;	//	Name of field carrying the submitted file
	uint32_t		n ;			//	XML Node counter
	hzEcode			rc ;		//	Return code

	m_Input = m_pApp->m_ExecParams[m_FstParam] ;
	m_Target = m_pApp->m_ExecParams[m_FstParam+1] ;

	errorReport.Printf("%s Called with %s and %s\n", *_fn, *m_Input, *m_Target) ;

	i = *m_Input ;
	if (memcmp(i, "%e:", 3))
	{
		errorReport.Printf("%s. Not an identifiable input\n", *_fn) ;
		return E_NOTFOUND ;
	}

	fldname = i + 3 ;
	if (pE->m_Uploads.Exists(fldname))
	{
		hf = pE->m_Uploads[fldname] ;

		errorReport.Printf("%s. Found input (fld %s file %s of %d bytes) mime=%d\n", *_fn, *hf.m_fldname, *hf.m_filename, hf.m_file.Size(), hf.m_mime) ;

		if (hf.m_mime == HMTYPE_APP_OPEN_DOCX)
		{
			Punzip(Z, hf.m_file) ;

			for (zi = Z ; !zi.eof() ; zi++)
			{
				if (*zi != CHAR_LESS)
					continue ;

				if (zi == "<w:document")
				{
					D << "<w:document" ;
					for (zi += 11 ; !zi.eof() ; zi++)
					{
						if (*zi == CHAR_LESS)
						{
							D.AddByte(CHAR_NL) ;
	
							if (zi == "</w:document>")
							{
								D << "</w:document>\n" ;
								break ;
							}
						}
						D.AddByte(*zi) ;

						if (*zi == CHAR_MORE)
							D.AddByte(CHAR_NL) ;
					}
					break ;
				}
			}

			rc = xdoc.Load(D) ;
			errorReport.Printf("%s. XML load status err=%s\n", __func__, Err2Txt(rc)) ;

			if (rc == E_OK)
			{
				xdoc.FindNodes(vx, "w:t") ;
				for (n = 0 ; n < vx.Count() ; n++)
				{
					pN = vx[n] ;

					E += "<p>\n" ;
					//E << pN->m_tmpContent ;
					E << pN->m_fixContent ;
					E += "\n</p>\n" ;
				}
			}

			errorReport.Printf("%s. Setting var %s with chain of %d bytes\n", *_fn, *m_Target, E.Size()) ;
			pE->SetVarChain(m_Target, E) ;
			//pE->m_mapChains.Insert(m_Target, E) ;
		}
	}

	return rc ;
}

hzEcode	hdsExec::Filesys	(hzChain& errorReport, hzHttpEvent* pE)
{
	//	Execute a filesystem command to create, delete or list a directory or to write out or read in a binary object to/from a file - as opposed to storing it
	//	in a repository.

	_hzfunc("hdsExec::Filesys") ;

	FSTAT		fs ;			//	File/directory status
	hzAtom		atom ;			//	For obtaing file content
	hzChain		content ;		//	The file content (usually file upload)
	hzString	resource ;		//	The directory/file to operate on
	hzString	m_Action ;		//	The command specifier (mkdir|rmdir|save|delete)
	hzString	m_Resource ;	//	Directory resource to operate on
	hzString	m_Content ;		//	Save command only - specifies data to be saved
	hzEcode		rc = E_OK ;		//	Return value

	errorReport.Printf("%s called with action %s and resource %s\n", *_fn, *m_Action, *m_Resource) ;

	m_Action = m_pApp->m_ExecParams[m_FstParam] ;
	m_Resource = m_pApp->m_ExecParams[m_FstParam] ;
	m_Content = m_pApp->m_ExecParams[m_FstParam] ;

	if (m_Action == "mkdir")
	{
		resource = m_pApp->ConvertText(m_Resource, pE) ;
		rc = AssertDir(resource, 0777) ;
		if (rc != E_OK)
			errorReport.Printf("%s. Could not assert dir %s error is %s\n", *_fn, *resource, Err2Txt(rc)) ;
		return rc ;
	}

	if (m_Action == "rmdir")
	{
		//	Will work even if directory not empy
		resource = m_pApp->ConvertText(m_Resource, pE) ;
		rc = BlattDir(resource) ;
		if (rc != E_OK)
			errorReport.Printf("%s. Could not blatt dir %s error is %s\n", *_fn, *resource, Err2Txt(rc)) ;
		return rc ;
	}

	if (m_Action == "save")
	{
		//	Write out data to a file.

		//	Obtain file name
		resource = m_pApp->ConvertText(m_Resource, pE) ;

		//	Obtain file content
		rc = m_pApp->PcEntConv(atom, m_Content, pE) ;
		content = atom.Chain() ;

		if (!content.Size())
		{
			errorReport.Printf("%s. No data to write to file %s\n", *_fn, *resource) ;
			return E_NODATA ;
		}

		ofstream	os ;	//	Output stream

		os.open(*resource) ;
		if (os.fail())
			{ errorReport.Printf("%s. Could not open for write, file %s\n", *_fn, *resource) ; return E_WRITEFAIL ; }

		os << content ;
		if (os.fail())
			{ errorReport.Printf("%s. Could not write, file %s\n", *_fn, *resource) ; return E_WRITEFAIL ; }

		os.close() ;
		return E_OK ;
	}

	if (m_Action == "delete")
	{
		//	Delete a file
		resource = m_pApp->ConvertText(m_Resource, pE) ;
		if (lstat(*resource, &fs) < 0)
			{ errorReport.Printf("%s. Action DELETE Failed: File %s not found\n", *_fn, *resource) ; return E_NOTFOUND ; }

		if (ISDIR(fs.st_mode))
			{ errorReport.Printf("%s. Action DELETE Failed: %s is a directory\n", *_fn, *resource) ; return E_TYPE ; }

		if (unlink(*resource) < 0)
			{ errorReport.Printf("%s. Action DELETE Failed: File %s could not be deleted\n", *_fn, *resource) ; return E_WRITEFAIL ; }

		return E_OK ;
	}

	//	List entries in a directory

	errorReport.Printf("%s. Filesys command Failed: Action %s illegal\n", *_fn, *m_Action) ;
	return E_SYNTAX ;
}

hzEcode	hdsExec::SrchPages	(hzChain& errorReport, hzHttpEvent* pE)
{
	//	Conduct a document search and provide a list of document ids found. In all cases the search will either be of an inbuilt resource such as the
	//	site's 'indigionous' pages (those defined as <xpage> in the config plus any passive pages), or it will be of a specified class.
	//
	//	Arguments:	1)	error	The error report chain
	//				2)	pE		The HTTP event pointer
	//
	//	Returns:	E_NOTFOUND	If there is no session or if the username is not established
	//				E_OK		If the username is established

	_hzfunc("hdsExec::SrchPages") ;

	hzVect<hdsPage*>	Result ;	//	Pages found
	hzVect<uint32_t>	res ;		//	Results

	hzChain			Z ;				//	For formulating response
	hzToken			T ;				//	Tokenizer for search criteria
	hdbIdset		R ;				//	Final search result
	hdsResource*	pRes ;			//	Resource pointer
	hdsPage*		pPage ;			//	Page pointer
	hzString		m_Action ;		//	The value of event variable x-action must have (if specified)
	hzString		m_Source ;		//	Source to search from (eg indiginous web pages)
	hzString		m_Criteria ;	//	The field name containng the search criteria
	hzString		m_Count ;		//	Name of the variable used to display the count
	hzString		m_Found ;		//	Name of the variable used to display the result (chain of HTML of table defined in format)
	hzString		V ;				//	Temp string
	hzString		S ;				//	Temp string
	uint32_t		nCount ;		//	Loop counter
	uint32_t		nStart ;		//	Fetch counter
	hzEcode			bSelect ;		//	True if we already have one per token result
	hzEcode			rc = E_OK ;		//	Return code
	char			buf [20] ;		//	For spelling out numbers

	if (!pE)
		Fatal("%s. No HTTP event supplied\n", __func__) ;
	errorReport.Printf("%s. Called\n", *_fn) ;

	//	First tokenize the search criteria
	Result.Clear() ;

	m_Criteria = m_pApp->m_ExecParams[m_FstParam] ;

	V = "srch_pages_criteria" ;
	if (!pE->m_mapStrings.Exists(V))
	{
		errorReport.Printf("%s. No such field as srch_pages_criteria\n", __func__) ;
		pE->m_appError = "Due to an Internal Error (Field name mismatch) we could not process your request" ;
		return E_NOTFOUND ;
	}

	S = pE->m_mapStrings[V] ;
	//rc = pE->SetVarString(m_Criteria, S) ;

	errorReport.Printf("%s. Search page index for [%s]\n", __func__, *S) ;
	rc = m_pApp->m_PageIndex.Eval(R, S) ;
	if (rc != E_OK)
	{
		//pE->SetVar(g_Errmsg, "Due to an Internal Error (Evaluation of page index) we could not process your request\n") ;
		pE->m_appError = "Due to an Internal Error (Evaluation of page index) we could not process your request" ;
		return E_NOTFOUND ;
	}

	if (!R.Count())
	{
		Z.Printf("Your search for %s found 0 results. Please try again", *S) ;
		pE->m_appError = Z ;
		return E_NOTFOUND ;
	}

	errorReport.Printf("%s. Results are %d pages, error=%s\n", __func__, R.Count(), Err2Txt(rc)) ;

	//	Fetch records from R

	sprintf(buf, "%d", R.Count()) ;
	S = buf ;
	rc = pE->SetVarString(m_Count, S) ;
	if (rc != E_OK)
		errorReport.Printf("%s. Could not set var %s to %s\n", __func__, *m_Count, buf) ;

	Z << "<div id=\"stdlist\">\n" ;
	Z << "<table align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n" ;
	for (nStart = 0 ;;)
	{
		R.Fetch(res, nStart, 20) ;
		errorReport.Printf("%s: Fetched %d records (total)\n", __func__, res.Count()) ;

		//	Lookup titles of pages using numbers

		for (nCount = 0 ; nCount < res.Count() ; nCount++)
		{
			//pRes = m_pApp->m_PagesName.GetObj(res[nCount]) ;
			pRes = m_pApp->m_ResourcesName.GetObj(res[nCount]) ;
			pPage = dynamic_cast<hdsPage*>(pRes) ;
			if (!pPage)
				continue ;

			errorReport.Printf("%s: Page No %d is page %p\n", __func__, res[nCount], pPage) ;
			Result.Add(pPage) ;

			Z.Printf("<tr><td><a href=\"%s\">%s</a></td><td>&nbsp;</td><td><a href=\"%s\">%s</a></td></tr>\n",
				*pPage->m_Url, *pPage->m_Title, *pPage->m_Url, *pPage->m_Desc) ;
		}

		if (res.Count() < 20)
			break ;
		nStart += 20 ;
	}
	Z << "</table>\n" ;
	Z << "</div>\n" ;

	V = "srch_pages_result" ;
	rc = pE->SetVarChain(m_Found, Z) ;
	if (rc != E_OK)
		errorReport.Printf("%s. Could not set var %s to chain of %d bytes\n", __func__, *m_Found, Z.Size()) ;
	errorReport.Printf("%s: Site scanned. %d pages\n", __func__, Result.Count()) ;

	return rc ;
}

hzEcode	hdsExec::SrchRepos	(hzChain& errorReport, hzHttpEvent* pE)
{
	//	Conduct a document search and provide a list of document ids found. In all cases the search will either be of an inbuilt resource such as the
	//	site's 'indigionous' pages (those defined as <xpage> in the config plus any passive pages), or it will be of a specified class.
	//
	//	Arguments:	1)	error	The error report chain
	//				2)	pE		The HTTP event pointer
	//
	//	Returns:	E_NOTFOUND	If there is no session or if the username is not established
	//				E_OK		If the username is established

	_hzfunc("hdsExec::DocSearch") ;

	hzVect<hdsPage*>	Result ;	//	Pages found
	hzVect<uint32_t>	res ;		//	Results

	hzChain			Z ;				//	For formulating response
	hzToken			T ;				//	Tokenizer for search criteria
	hdbIdset		R ;				//	Final search result
	hdsResource*	pRes ;			//	Resource
	hdsPage*		pPage ;			//	Page pointer
	hdbObjCache*	pCache ;		//	Cache to look for records if we are not using pages
	hzString		m_Action ;		//	The value of event variable x-action must have (if specified)
	hzString		m_Source ;		//	Source to search from (eg indiginous web pages)
	hzString		m_Criteria ;	//	The field name containng the search criteria
	hzString		m_Count ;		//	Name of the variable used to display the count
	hzString		m_Found ;		//	Name of the variable used to display the result (chain of HTML of table defined in format)
	hzString		S ;				//	Temp string
	uint32_t		nCount ;		//	Loop counter
	uint32_t		nStart ;		//	Fetch counter
	hzEcode			bSelect ;		//	True if we already have one per token result
	hzEcode			rc = E_OK ;		//	Return code
	char			buf [20] ;		//	For spelling out numbers

	if (!pE)
		Fatal("%s. No HTTP event supplied\n", __func__) ;
	errorReport.Printf("%s. Called\n", *_fn) ;

	//	First tokenize the search criteria
	Result.Clear() ;

	m_Criteria = m_pApp->m_ExecParams[m_FstParam] ;

	if (!pE->m_mapStrings.Exists(m_Criteria))
	{
		errorReport.Printf("%s. No such field as %s\n", __func__, *m_Criteria) ;
		pE->m_appError = "Due to an Internal Error (Field name mismatch) we could not process your request" ;
		return E_NOTFOUND ;
	}

	S = pE->m_mapStrings[m_Criteria] ;
	//rc = pE->SetVarString(m_Criteria, S) ;

	if (m_Source == "pages")
	{
		errorReport.Printf("%s. Search page index for [%s]\n", __func__, *S) ;
		rc = m_pApp->m_PageIndex.Eval(R, S) ;
		if (rc != E_OK)
		{
			//pE->SetVar(g_Errmsg, "Due to an Internal Error (Evaluation of page index) we could not process your request\n") ;
			pE->m_appError = "Due to an Internal Error (Evaluation of page index) we could not process your request" ;
			return E_NOTFOUND ;
		}
	}
	else
	{
		//	The source must be a user defined class
		pCache = (hdbObjCache*) m_pApp->m_ADP.GetObjRepos(m_Source) ;
		if (!pCache)
		{
			//pE->SetVar(g_Errmsg, "Due to an Internal Error (Cache not located) we could not process your request\n") ;
			pE->m_appError = "Due to an Internal Error (Cache not located) we could not process your request" ;
			return E_NOTFOUND ;
		}

		rc = pCache->Select(R, *S) ;
		if (rc != E_OK)
		{
			//pE->SetVar(g_Errmsg, "Due to an Internal Error (Evaluation of select on cache) we could not process your request\n") ;
			pE->m_appError = "Due to an Internal Error (Evaluation of select on cache) we could not process your request" ;
			return E_NOTFOUND ;
		}
	}

	if (!R.Count())
	{
		Z.Printf("Your search for %s found 0 results. Please try again", *S) ;
		pE->m_appError = Z ;
		return E_NOTFOUND ;
	}

	errorReport.Printf("%s. Results are %d pages, error=%s\n", __func__, R.Count(), Err2Txt(rc)) ;

	//	Fetch records from R

	sprintf(buf, "%d", R.Count()) ;
	S = buf ;
	rc = pE->SetVarString(m_Count, S) ;
	if (rc != E_OK)
		errorReport.Printf("%s. Could not set var %s to %s\n", __func__, *m_Count, buf) ;

	Z << "<div id=\"stdlist\">\n" ;
	Z << "<table align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n" ;
	for (nStart = 0 ;;)
	{
		R.Fetch(res, nStart, 20) ;
		errorReport.Printf("%s: Fetched %d records (total)\n", __func__, res.Count()) ;

		//	Lookup titles of pages using numbers

		for (nCount = 0 ; nCount < res.Count() ; nCount++)
		{
			//pRes = m_pApp->m_PagesName.GetObj(res[nCount]) ;
			pRes = m_pApp->m_ResourcesName.GetObj(res[nCount]) ;
			pPage = dynamic_cast<hdsPage*>(pRes) ;
			if (!pPage)
				continue ;

			errorReport.Printf("%s: Page No %d is page %p\n", __func__, res[nCount], pPage) ;
			Result.Add(pPage) ;

			Z.Printf("<tr><td><a href=\"%s\">%s</a></td><td>&nbsp;</td><td><a href=\"%s\">%s</a></td></tr>\n",
				*pPage->m_Url, *pPage->m_Title, *pPage->m_Url, *pPage->m_Desc) ;
		}

		if (res.Count() < 20)
			break ;
		nStart += 20 ;
	}
	Z << "</table>\n" ;
	Z << "</div>\n" ;

	rc = pE->SetVarChain(m_Found, Z) ;
	if (rc != E_OK)
		errorReport.Printf("%s. Could not set var %s to chain of %d bytes\n", __func__, *m_Found, Z.Size()) ;
	errorReport.Printf("%s: Site scanned. %d pages\n", __func__, Result.Count()) ;

	return rc ;
}

hzEcode	hdsExec::Exec	(hzChain& errorReport, hzHttpEvent* pE)
{
	//	Run a single Exec command. Commands are embraced by a <procdata> tag and can appear in <xform> and <page>
	//
	//	Arguments:	1)	error	The error report chain
	//				2)	pE		The HTTP event pointer

	_hzfunc("hdsExec::Exec") ;

	const hdbClass*		pClass ;	//	Data class
	const hdbMember*	pMbr ;		//	Data class member

	ifstream		is ;			//	For data import
	hdbObject*		pCurObj ;		//	Single data class instance
	hzAtom			atom ;			//	Atom for fetching values
	hzChain			bval ;			//	Document or binary (if applicable)
	hdbObjRepos*	pRepos ;		//	Target repository
	hdsInfo*		pInfo ;			//	Session
	hdbObject*		pObj ;			//	Standalone object pointer (either that vested with the HTTP event or the user session)
	hdbROMID		di ;			//	Delta
	hzString		param1 ;		//	1st parameter
	hzString		param2 ;		//	2nd parameter
	hzString		param3 ;		//	3rd parameter
	hzString		param4 ;		//	4th parameter
	hzString		param5 ;		//	5th parameter
	hzString		param6 ;		//	6th parameter
	hzString		strVal ;		//	Used to derive real value of the step's input when specified as a variable name instead of a literal
	hzString		S ;				//	Temp string
	hzString		a ;				//	Content of parameter 1
	hzString		b ;				//	Content of parameter 2
	//hdsUsertype		utype ;			//	User type (for loging in)
	uint32_t		n ;				//	Object value iterator
	uint32_t		objId = 0 ;		//	Object id
	hzEcode			rc = E_OK ;		//	Return code

	if (!pE)
		Fatal("%s. No HTTP event supplied\n", __func__) ;

	//	Obtain user session if any
	pInfo = (hdsInfo*) pE->Session() ;
	threadLog("%s %s Commencing with info %p -> ", *_fn, Exec2Txt(m_Command), pInfo) ;
	errorReport.Printf("%s %s Commencing with info %p -> ", *_fn, Exec2Txt(m_Command), pInfo) ;

	switch	(m_Command)
	{
	case EXEC_SENDEMAIL:	//	Send an email. Params are sender, recipient, subject and msg.
							rc = SendEmail(errorReport, pE) ;
							break ;

	case EXEC_SETVAR:		if (!pInfo)
								{ errorReport.Printf("EXEC_SETVAR No session in place\n") ; return E_CORRUPT ; }

							//	Get the value
							param2 = m_pApp->m_ExecParams[m_FstParam+1] ;

							if (param2[0] == CHAR_PERCENT)
								rc = m_pApp->PcEntConv(atom, param2, pE) ;
							else
								rc = atom.SetValue(m_type, param2) ;

							//	Get the name
							param1 = m_pApp->m_ExecParams[m_FstParam] ;
							if (!pInfo->m_Sessvals.Exists(param1))
								pInfo->m_Sessvals.Insert(param1, atom) ;
							else
								pInfo->m_Sessvals[param1] = atom ;
							break ;

	case EXEC_ADDUSER:		//	Explicitly create a new user as an executive step. See hdsExec::Adduser()
							rc = Adduser(errorReport, pE) ;
							break ;

	case EXEC_LOGON:		//	Log a user in as admin. This is done without password checking. As the supplied m_Uname parameter is always a percent entity,
							//	this must be evaluated before use.
							rc = Logon(errorReport, pE) ;
							break ;

	case EXEC_TEST:			//	Tests if a condition is true. Currently limited to testing if two parameters are equal

							param1 = m_pApp->m_ExecParams[m_FstParam] ;
							param2 = m_pApp->m_ExecParams[m_FstParam+1] ;

							a = m_pApp->ConvertText(param1, pE) ;
							b = m_pApp->ConvertText(param2, pE) ;

							if (a == b)
								errorReport.Printf("exec test OK: a = %s, b = %s\n", *a, *b) ;
							else
							{
								errorReport.Printf("exec test FAIL: a = %s, b = %s\n", *a, *b) ;
								rc = E_NOTFOUND ;
							}
							break ;

	case EXEC_EXTRACT:		//	Perform a text extraction. The scenario will be that the input will be a file bound to the event (e.g. a file upload in a form
							//	submission). The m_Input member will name the field carrying the file.
							rc = Extract(errorReport, pE) ;
							break ;

	case EXEC_OBJ_TEMP:		//	Create a temporary hdbObject and vest with the HTTP event (m_pContextObj)

		param1 = m_pApp->m_ExecParams[m_FstParam] ;
		param2 = m_pApp->m_ExecParams[m_FstParam+1] ;
		threadLog("Got exec params of %s and %s\n", *param1, *param2) ;
		pClass = m_pApp->m_ADP.GetPureClass(param2) ;
		if (!pClass)
			{ errorReport.Printf("EXEC_OBJ_TEMP FAIL: No such class as %s\n", *param2) ; rc = E_NOTFOUND ; break ; }
		if (pE->m_pContextObj)
			{ errorReport.Printf("EXEC_OBJ_TEMP FAIL: Already a temp object\n") ; rc = E_NOTFOUND ; break ; }
		pE->m_pContextObj = pObj = new hdbObject() ;
		pObj->Init(param1, pClass) ;
		errorReport.Printf("EXEC_OBJ_TEMP: Allocated a temp object of %s\n", *param1) ;
		break ;

	case EXEC_OBJ_START:	//	Assert the current user session object. This either finds the current object as null and sets it, or finds the right object in
							//	place. If the current object is not the right one, this is an error.

		if (!pInfo)
			{ errorReport.Printf("EXEC_OBJ_START FAIL: No user session\n") ; rc = E_NOINIT ; break ; }
			
		threadLog("Got exec param of %d\n", m_FstParam) ;
		param1 = m_pApp->m_ExecParams[m_FstParam] ;
		threadLog("Got exec param of %d\n", m_FstParam) ;
		threadLog("Got exec param of %s\n", *param1) ;
		param2 = m_pApp->m_ExecParams[m_FstParam+1] ;
		threadLog("Got exec param of %s\n", *param2) ;
		pClass = m_pApp->m_ADP.GetPureClass(param2) ;
		pInfo->ObjectAssert(param1, pClass) ;	//m_pApp->m_SObj2Class[param1]) ;
		break ;

	case EXEC_OBJ_FETCH:	//	Load the current user session object from a repository
	case EXEC_OBJ_IMPORT:	//	Load the current user session object from a JSON file
	case EXEC_OBJ_EXPORT:	//	Save the current user session object to a JSON file
	case EXEC_OBJ_SETMBR:	//	Set a data class member within the current object. The params are repository name, class name, member name and the data source.
	case EXEC_OBJ_COMMIT:	//	Commit data in the named hdbObject instance (param1) to the named repository (param2). An object id of 0 effects an INSERT while
	case EXEC_OBJ_CLOSE:	//	Closes the current user session object.

		//	First obtain existing user session or temp object
		param1 = m_pApp->m_ExecParams[m_FstParam] ;		//	Object key
		if (pE->m_pContextObj)
			pCurObj = (hdbObject*) pE->m_pContextObj ;
		else
		{
			if (!pInfo)
				{ errorReport.Printf("%s FAIL: No user session\n", Exec2Txt(m_Command)) ; rc = E_NOINIT ; break ; }

			pCurObj = pInfo->ObjectSelect(param1) ;
			pCurObj = pInfo->m_pObj ;

			if (!pInfo->m_pObj)
				{ errorReport.Printf("%s FAIL: No current object\n", Exec2Txt(m_Command)) ; rc = E_NOINIT ; break ; }
		}

		//	Then switch again on these object commands
		switch	(m_Command)
		{
		case EXEC_OBJ_FETCH:	//	Load the current user session object from a repository
								break ;

		case EXEC_OBJ_IMPORT:	//	Load the current user session object from a JSON file

			param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Source file name
			strVal = m_pApp->ConvertText(param2, pE) ;
			if (rc != E_OK)
				{ errorReport.Printf("%s FAIL: String %s could not be converted\n", Exec2Txt(m_Command), *param2) ; rc = E_OPENFAIL ; break ; }
			is.open(*strVal) ;
			if (is.fail())
				{ errorReport.Printf("%s FAIL: File %s could not be opened\n", Exec2Txt(m_Command), *strVal) ; ; rc = E_OPENFAIL ; break ; }
			bval << is ;
			is.close() ;
			pCurObj->ImportJSON(bval) ;
			break ;

		case EXEC_OBJ_EXPORT:	//	Save the current user session object to a JSON file

			param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Traget file name
			break ;

		case EXEC_OBJ_SETMBR:	//	Set a data class member within the current object. The params are repository name, class name, member name and the data source.

			param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Class name
			param3 = m_pApp->m_ExecParams[m_FstParam+2] ;	//	Member name
			param4 = m_pApp->m_ExecParams[m_FstParam+3] ;	//	Data source

			pClass = m_pApp->m_ADP.GetPureClass(param2) ;
			if (!pClass)
				{ errorReport.Printf("EXEC_OBJ_SETMBR FAIL: No such class as %s\n", *param2) ; rc = E_NOTFOUND ; break ; }

			pMbr = pClass->GetMember(param3) ;
			if (!pMbr)
				{ errorReport.Printf("EXEC_OBJ_SETMBR FAIL: Class %s has no member of %s\n", *param2, *param3) ; rc = E_NOTFOUND ; break ; }

			switch	(pMbr->Basetype())
			{
			case BASETYPE_TXTDOC:
			case BASETYPE_BINARY:
				//	The step is to set the object's member to the file content. The filename is got from the step input (naming the event variable) and
				//	from there, we lookup the actual file content in the event

				if (pE->m_Uploads.Exists(param4))
				{
					hzHttpFile	hf ;	//	External document/binary file

					hf = pE->m_Uploads[param4] ;

					strVal = m_pApp->ConvertText(param4, pE) ;

					errorReport.Printf("%s. Step %s=%s (fld %s file %s of %d bytes) mime=%d\n",
						*_fn, *param3, *param4, *hf.m_fldname, *hf.m_filename, hf.m_file.Size(), hf.m_mime) ;
					rc = pCurObj->SetBinary(param3, hf.m_file) ;
				}

				if (pE->m_mapChains.Exists(param4))
				{
					bval = pE->m_mapChains[param4] ;
					strVal.Clear() ;

					errorReport.Printf("%s. Step %s=%s (chain %d bytes)\n", *_fn, *param3, *param4, bval.Size()) ;
					rc = pCurObj->SetBinary(param3, bval) ;
				}
				break ;

			case BASETYPE_EMADDR:
			case BASETYPE_URL:
			case BASETYPE_STRING:

				strVal = m_pApp->ConvertText(param4, pE) ;
				errorReport.Printf("%s. Strlike Step %s = %s (%s)\n", *_fn, *param3, *param4, *strVal) ;
				rc = pCurObj->SetValue(pMbr->Posn(), strVal) ;
				break ;

			default:	//	Deal with num-like or numeric values

				rc = m_pApp->PcEntConv(atom, param4, pE) ;
				errorReport.Printf("%s. Numlike Step %s = %s (%s)\n", *_fn, *param3, *param4, *atom.Str()) ;
				rc = pCurObj->SetValue(pMbr->Posn(), atom) ;
				break ;
			}
			break ;

		case EXEC_OBJ_COMMIT:	//	Commit data in the named hdbObject instance (param1) to the named repository (param2). An object id of 0 effects an INSERT while
								//	a non-zero object id effects an UPDATE. The object id is derived from @resarg and is foud at this juncture by ....

			if (!pCurObj)
				{ errorReport.Printf("EXEC_OBJ_COMMIT FAIL: No current object\n") ; rc = E_NOINIT ; break ; }

			for (n = 0 ; n < pCurObj->m_Values.Count() ; n++)
			{
				di = pCurObj->m_Values.GetKey(n) ;
				errorReport.Printf("EXEC_OBJ_COMMIT class %d mbr %d obj %d\n", di.m_ClsId, di.m_MbrId, di.m_ObjId) ;
			}

			//	param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Target repository
			//	param3 = m_pApp->m_ExecParams[m_FstParam+2] ;	//	Source of object id?

			//	pRepos = m_pApp->m_ADP.mapRepositories[param2] ;
			//pRepos = m_pApp->m_ADP.mapRepositories.GetObj(pCurObj->ReposId()) ;
			pRepos = m_pApp->m_ADP.GetObjRepos(pCurObj->ReposId()) ;

			if (!pRepos)
				{ errorReport.Printf("EXEC_OBJ_COMMIT FAIL: Object %s No repository\n", *param1) ; rc = E_NOINIT ; break ; }

			if (pCurObj->ObjId())
				rc = pRepos->Update(*pCurObj, pCurObj->ObjId()) ;
			else
				rc = pRepos->Insert(objId, *pCurObj) ;

			/*
			if (param3)
			{
				//	Modify object meeting with criteria
				m_pApp->PcEntConv(atom, param3, pE) ;
				if (atom.Type() != BASETYPE_INT32)
					errorReport.Printf("%s. IdNote (%s) returns wrong type (%s)\n", *_fn, *param3, Basetype2Txt(atom.Type())) ;
				else
				{
					objId = atom.Int32() ;
					errorReport.Printf("%s. Modify cache %s obj %d with pinfo %p\n", *_fn, *pRepos->Name(), objId, pInfo) ;
					if (objId > 0)
						rc = pRepos->Update(*pCurObj, objId) ;
					else
						rc = pRepos->Insert(objId, *pCurObj) ;
				}
			}
			else
			{
				//	Insert a new object
				errorReport.Printf("%s. Insert cache %s obj %d with pinfo %p\n", *_fn, *pRepos->Name(), objId, pInfo) ;
				rc = pRepos->Insert(objId, *pCurObj) ;
			}
			*/

			if (rc != E_OK)
				errorReport.Printf("EXEC_OBJ_COMMIT FAIL: Insert/Modify on repository of %s failed with error %s\n", *param2, Err2Txt(rc)) ;
			break ;

		case EXEC_OBJ_CLOSE:	//	Closes the current user session object.
								break ;
		}
		break ;

	//	Tree Operations

	case EXEC_TREE_DCL:		//	Declare a private tree and place it in the user session. This will have no effect if the user session already has the tree which
							//	is identified by name. If there is a different tree in the session, it will be deleted and replaced with the requested tree.

		if (!pInfo)
			{ errorReport.Printf("EXEC_TREE_DCL FAIL. No user session\n") ; return E_NOTFOUND ; }

		param1 = m_pApp->m_ExecParams[m_FstParam] ;		//	Tree id
		//param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Tree to copy (if applicable)

		if (pInfo->m_pTree)
		{
			if (pInfo->m_pTree->m_Groupname == param1)
				break ;
			delete pInfo->m_pTree ;
		}

		pInfo->m_pTree = new hdsTree() ;
		pInfo->m_pTree->m_Groupname = param1 ;
		threadLog("%s. SET tree %s in session %p\n", *_fn, *pInfo->m_pTree->m_Groupname, pInfo) ;
		break ;

	/*
	case EXEC_TREE_CPY:		//	If the current user session tree is empty, copy headings from the named public tree (if supplied) and copy entries from a source
							//	object (if supplied). This command can only apply to the current user session tree. If this is not present or does not have the
							//	supplied id, this is an error. The command will do nothing if the current user session tree is not empty. It is for initializing
							//	values only.

		if (!pInfo)
			{ errorReport.Printf("EXEC_TREE_CPY FAIL. No user session\n") ; return E_NOTFOUND ; }

		param1 = m_pApp->m_ExecParams[m_FstParam] ;		//	Tree id
		param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Tree to copy (if applicable)
		param3 = m_pApp->m_ExecParams[m_FstParam+2] ;	//	Object to copy (if applicable)

		//	Check source tree if supplied
		if (param2)
		{
			pTree = m_pApp->m_ArticleGroups[param2] ;
			if (!pTree)
				{ errorReport.Printf("EXEC_TREE_CPY FAIL. Named source tree %s not found\n", *param2) ; return E_NOTFOUND ; }
		}

		//	Check source object if supplied
		if (param3)
		{
		}

		if (pInfo->m_pTree->Count())
			break ;
	*/

	case EXEC_TREE_HEAD:		//	Add a heading to the tree if it does not already exist. A heading is an empty article with no link.

		if (!pE)
			Fatal("%s. No HTTP event supplied\n", *_fn) ;
		pInfo = (hdsInfo*) pE->Session() ;

		if (!pInfo)
			{ errorReport.Printf("EXEC_TREEOP FAIL. No user session\n") ; return E_NOTFOUND ; }

		if (!pInfo->m_pTree)
			{ errorReport.Printf("EXEC_TREEOP FAIL. No user tree\n") ; return E_NOTFOUND ; }

		param1 = m_pApp->m_ExecParams[m_FstParam] ;		//	Tree id
		param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Parent item id
		param3 = m_pApp->m_ExecParams[m_FstParam+2] ;	//	Refname
		param4 = m_pApp->m_ExecParams[m_FstParam+3] ;	//	Headline

		if (pInfo->m_pTree->m_Groupname != param1)
			{ errorReport.Printf("EXEC_TREEOP FAIL. Wrong tree\n") ; return E_NOTFOUND ; }

		//	If a parent tree item is stated, check it exists
		if (param2 && !pInfo->m_pTree->Item(param2))
			{ errorReport.Printf("EXEC_TREEOP FAIL. Stated parent (%s) does not exist\n", *param2) ; return E_NOTFOUND ; }

		//	Then add/delete the new item(s)
		pInfo->m_pTree->AddHead(param2, param3, param4, false) ;
		break ;

	case EXEC_TREE_ITEM:	//	Add an article to the tree if it does not already exist.

		if (!pE)
			Fatal("%s. No HTTP event supplied\n", *_fn) ;
		pInfo = (hdsInfo*) pE->Session() ;

		if (!pInfo)
			{ errorReport.Printf("EXEC_TREEOP FAIL. No user session\n") ; return E_NOTFOUND ; }

		if (!pInfo->m_pTree)
			{ errorReport.Printf("EXEC_TREEOP FAIL. No user tree\n") ; return E_NOTFOUND ; }

		param1 = m_pApp->m_ExecParams[m_FstParam] ;		//	Tree id
		param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Parent item id
		param3 = m_pApp->m_ExecParams[m_FstParam+2] ;	//	Refname
		param4 = m_pApp->m_ExecParams[m_FstParam+3] ;	//	Headline

		if (pInfo->m_pTree->m_Groupname != param1)
			{ errorReport.Printf("EXEC_TREEOP FAIL. Wrong tree\n") ; return E_NOTFOUND ; }

		//	If a parent tree item is stated, check it exists
		if (param2 && !pInfo->m_pTree->Item(param2))
			{ errorReport.Printf("EXEC_TREEOP FAIL. Stated parent (%s) does not exist\n", *param2) ; return E_NOTFOUND ; }


		//	Then add/delete the new item(s)
		//pArt = new hdsArticle() ;
		pInfo->m_pTree->AddItem(param2, param3, param4, false) ;
		break ;

	case EXEC_TREE_FORM:	//	Add a form to the tree as an article.

		if (!pE)
			Fatal("%s. No HTTP event supplied\n", *_fn) ;
		pInfo = (hdsInfo*) pE->Session() ;

		if (!pInfo)
			{ errorReport.Printf("EXEC_TREEOP FAIL. No user session\n") ; return E_NOTFOUND ; }

		if (!pInfo->m_pTree)
			{ errorReport.Printf("EXEC_TREEOP FAIL. No user tree\n") ; return E_NOTFOUND ; }

		param1 = m_pApp->m_ExecParams[m_FstParam] ;		//	Tree id
		param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Parent item id
		param3 = m_pApp->m_ExecParams[m_FstParam+2] ;	//	Refname
		param4 = m_pApp->m_ExecParams[m_FstParam+3] ;	//	Headline
		param5 = m_pApp->m_ExecParams[m_FstParam+4] ;	//	Form reference
		param6 = m_pApp->m_ExecParams[m_FstParam+5] ;	//	Data class context

		if (pInfo->m_pTree->m_Groupname != param1)
			{ errorReport.Printf("EXEC_TREEOP FAIL. Wrong tree\n") ; return E_NOTFOUND ; }

		//	First locate the parent item to which the new items will be added, then add
		pInfo->m_pTree->Item(param2) ;
		pInfo->m_pTree->AddForm(param2, param3, param4, param5, param6, false) ;
		break ;

	case EXEC_TREE_SYNC:	//	Sync the tree to a standalone object.

		/*
		if (!pE)
			Fatal("%s. No HTTP event supplied\n", *_fn) ;
		pInfo = (hdsInfo*) pE->Session() ;

		if (!pInfo)
			{ errorReport.Printf("EXEC_TREEOP FAIL. No user session\n") ; return E_NOTFOUND ; }

		if (!pInfo->m_pTree)
			{ errorReport.Printf("EXEC_TREEOP FAIL. No user tree\n") ; return E_NOTFOUND ; }

		param1 = m_pApp->m_ExecParams[m_FstParam] ;		//	Tree id
		param2 = m_pApp->m_ExecParams[m_FstParam+1] ;	//	Object id

		if (pInfo->m_pTree->m_Groupname != param1)
			{ errorReport.Printf("EXEC_TREEOP FAIL. Wrong tree\n") ; return E_NOTFOUND ; }

		//	Then obtain existing user session or temp object
		if (pE->m_pContextObj)
			pCurObj = (hdbObject*) pE->m_pContextObj ;
		else
		{
			if (!pInfo)
				{ errorReport.Printf("%s FAIL: No user session\n", Exec2Txt(m_Command)) ; rc = E_NOINIT ; break ; }

			pCurObj = pInfo->ObjectSelect(param2) ;
			pCurObj = pInfo->m_pObj ;

			if (!pInfo->m_pObj)
				{ errorReport.Printf("%s FAIL: No current object\n", Exec2Txt(m_Command)) ; rc = E_NOINIT ; break ; }
		}

		pInfo->m_pTree->Sync(*pCurObj) ;
		*/
		break ;

	case EXEC_TREE_DEL:	break ;
	case EXEC_TREE_EXP:	break ;
	case EXEC_TREE_CLR:	break ;

	case EXEC_SRCH_PAGES:	//	Conduct a document search and provide a list of document ids found. The search will either be of an inbuilt resource such as the
							//	site's 'indigionous' pages (those defined as <xpage> in the config plus any passive pages), or it will be of a specified class.
		rc = SrchPages(errorReport, pE) ;
		break ;

	case EXEC_SRCH_REPOS:	//	Conduct a search on a repository and provide a list of object ids found. The search will either be of an inbuilt resource such as the
							//	site's 'indigionous' pages (those defined as <xpage> in the config plus any passive pages), or it will be of a specified class.
		rc = SrchRepos(errorReport, pE) ;
		break ;

	case EXEC_FILESYS:		//	Execute a filesystem command to create, delete or list a directory or to write out or read in a binary object to/from a file (as
							//	opposed to storing it in a repository.
		rc = Filesys(errorReport, pE) ;
		break ;
	}

	return rc ;
}
