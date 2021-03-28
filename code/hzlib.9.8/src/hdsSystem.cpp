//
//	File:	hdsSystem.cpp
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
//	Dissemino HTML Generation
//

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>
#include <sys/stat.h>
#include <openssl/ssl.h>

#include "hzChars.h"
#include "hzString.h"
#include "hzChain.h"
#include "hzCtmpls.h"
#include "hzCodec.h"
#include "hzDate.h"
#include "hzDatabase.h"
#include "hzDocument.h"
#include "hzTextproc.h"
#include "hzTokens.h"
#include "hzHttpServer.h"
#include "hzMailer.h"
#include "hzProcess.h"
#include "hzDissemino.h"

using namespace std ;

/*
**	Variables
*/

global	Dissemino*	_hzGlobal_Dissemino ;	//	Holds all live applications

//	Standard strings. Please note that it is forbidden to use the values set here as names for other fields in your forms!
static	const hzString	g_Errmsg = "errmsg" ;			//	The name of the event variable used on form redisplay to convey why form is being redisplayed.
static	const hzString	s_articleTitle = "x-title" ;	//	Name of HTTP header supplied upon request of an article

/*
**	Functions
*/

void	hdsApp::SetStdTypeValidations	(void)
{
	//	Attaches Validation JavaScript to the standard data types. For this to succeed, the standard data types have to be present in the ADP. This function must therefore, only be
	//	called after a call to m_ADP.InitStandard(). Execution is terminated if any of the standard types expected are missing. 
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hdsApp::SetStdTypeValidations") ;

	const hdbDatatype*	dt ;			//	Pointer to Cpp data type
	const hdbHzotype*	ht ;			//	Pointer to Cpp data type
	const hzString		jsAlpha ;		//	Javascript for alpha
	const hzString		jsAlphnum ;		//	Javascript for alphanumeric
	const hzString		jsIpaddr ;		//	Javascript for IP addresses
	const hzString		jsDomain ;		//	Javascript for Email addresses
	const hzString		jsEmaddr ;		//	Javascript for Email addresses
	const hzString		jsURL ;			//	Javascript for URLs
	const hzString		jsXdate ;		//	Javascript for URLs
	const hzString		jsSdate ;		//	Javascript for URLs
	const hzString		jsTime ;		//	Javascript for URLs

	//InitDatabase() ;

	//	Add JS to Group 2

	dt = m_ADP.GetDatatype("ipaddr") ;
	ht = dynamic_cast<const hdbHzotype*>(dt) ;
	if (ht)
		ht->m_valJS=jsIpaddr;
	else
		hzexit(_fn, m_Appname, E_NOTFOUND, "No ipaddr datatype declared") ;

	dt = m_ADP.GetDatatype("domain") ;
	ht = dynamic_cast<const hdbHzotype*>(dt) ;
	if (ht)
		ht->m_valJS=jsDomain;
	else
		hzexit(_fn, m_Appname, E_NOTFOUND, "No domain datatype declared") ;

	dt = m_ADP.GetDatatype("emaddr") ;
	ht = dynamic_cast<const hdbHzotype*>(dt) ;
	if (ht)
		ht->m_valJS=jsEmaddr;
	else
		hzexit(_fn, m_Appname, E_NOTFOUND, "No emaddr datatype declared") ;

	dt = m_ADP.GetDatatype("url") ;
	ht = dynamic_cast<const hdbHzotype*>(dt) ;
	if (ht)
		ht->m_valJS=jsURL;
	else
		hzexit(_fn, m_Appname, E_NOTFOUND, "No url datatype declared") ;

	dt = m_ADP.GetDatatype("time") ;
	ht = dynamic_cast<const hdbHzotype*>(dt) ;
	if (ht)
		ht->m_valJS=jsTime;
	else
		hzexit(_fn, m_Appname, E_NOTFOUND, "No time datatype declared") ;

	dt = m_ADP.GetDatatype("sdate") ;
	ht = dynamic_cast<const hdbHzotype*>(dt) ;
	if (ht)
		ht->m_valJS=jsSdate ;
	else
		hzexit(_fn, m_Appname, E_NOTFOUND, "No sdate datatype declared") ;

	dt = m_ADP.GetDatatype("xdate") ;
	ht = dynamic_cast<const hdbHzotype*>(dt) ;
	if (ht)
		ht->m_valJS=jsXdate ;
	else
		hzexit(_fn, m_Appname, E_NOTFOUND, "No xdate datatype declared") ;
}

bool	hdsApp::IsPcEnt	(hzString& pcntEnt, const char* input)
{
	//	Determine if the supplied cstr is a percent entity and if it is, populate the supplied string with the entity
	//
	//	Arguments:	1)	pcntEnt	The hzString to contain the entity
	//				2)	input	The input text
	//
	//	Returns:	True	If the supplied cstr is a percent entity
	//				False	Otherwise

	_hzfunc("hdsApp::IsPcEnt") ;

	hzChain		W ;		//	Chain to build entity
	uint32_t	c ;		//	Char count

	pcntEnt.Clear() ;
	if (!input)
		return false ;

	if (input[0] != CHAR_PERCENT)
		return false ;

	if (input[1] != 'x' && input[1] != 'u' && input[1] != 's' && input[1] != 'e' && input[1] != 'i' && input[1] != 'v')
		return false ;

	if (input[2] != CHAR_COLON)
		return false ;

	//	Get the variable type and name component
	W.AddByte(CHAR_PERCENT) ;
	W.AddByte(input[1]) ;
	W.AddByte(CHAR_COLON) ;

	for (input += 3, c = 0 ; *input && *input != CHAR_SCOLON && IsAlpha(*input) ; c++, input++)
		W.AddByte(*input) ;

	if (c && *input == CHAR_PERIOD)
	{
		W.AddByte(*input) ;
		for (input++ ; *input && *input != CHAR_SCOLON && IsAlpha(*input) ; input++)
			W.AddByte(*input) ;
	}

	if (c && *input == CHAR_SCOLON)
		W.AddByte(*input) ;
	else
		c = 0 ;

	if (c)
		pcntEnt = W ;
	W.Clear() ;
	return c ? true : false ;
}

bool	hdsApp::AtPcEnt	(hzString& pcntEnt, chIter& input)
{
	//	Determine if the supplied chain iterator is at the start of a percent entity and if it is, populate the supplied string (arg 1) with the entity
	//
	//	Arguments:	1)	pcntEnt	The hzString to contain the entity
	//				2)	input	The input text
	//
	//	Returns:	True	If a percent entity is found
	//				False	Otherwise

	_hzfunc("hdsApp::AsPcEnt") ;

	hzChain		W ;			//	Chain to build entity
	chIter		zi ;		//	Temp iterator
	uint32_t	c = 0 ;		//	Char count

	pcntEnt.Clear() ;
	zi = input ;
	if (*zi != CHAR_PERCENT)
		return false ;
	W.AddByte(CHAR_PERCENT) ;
	zi++ ;

	if (*zi != 'x' && *zi != 'e' && *zi != 's' && *zi != 'u')
		return false ;
	W.AddByte(*zi) ;
	zi++ ;

	if (*zi != CHAR_COLON)
		return false ;
	W.AddByte(CHAR_COLON) ;

	//	Get the variable type and name component
	for (zi++ ; !zi.eof() && *zi != CHAR_SCOLON && IsAlpha(*zi) ; c++, zi++)
		W.AddByte(*zi) ;

	if (c && *zi == CHAR_PERIOD)
	{
		W.AddByte(*zi) ;
		for (zi++ ; !zi.eof() && *zi != CHAR_SCOLON && IsAlpha(*zi) ; zi++)
			W.AddByte(*zi) ;
	}

	if (c && *zi == CHAR_SCOLON)
		W.AddByte(*zi) ;
	else
		c = 0 ;

	if (c)
		pcntEnt = W ;
	W.Clear() ;
	return c ? true : false ;
}

hdbBasetype	hdsApp::PcEntTest	(hzString& err, hdsFormdef* pFormdef, const hdbClass* pHost, const hzString& pcntEnt)
{
	//	Test the validity of the supplied percent entity, within the context of the configs. This function should not be called once the configs have been read.
	//	Validity is determined firstly by a test of format and then by a test of scope. Percent entities begin with a percent followed by a lower case letter to
	//	indicate the percent entity class (either X, U, S, E, I or V), a colon, an entity name and finally a semi-colon.
	//
	//	The scope tests differ by percent entity class as follows:-
	//
	//		X-class	(standard system values) the entity either exists and is valid or does not exist and is invalid.
	//
	//		U-class (standard user values) must also exist but fail the scope test if they exist within a page or a form within a page, whose access settings
	//				exclude logged in users.
	//
	//		S-class (independent user session variables). As these can be set up in any form handler, not necessarily related to the form in which the S-class
	//				percent entity appears, these are currently not tested.
	//
	//		E-class (event values). These are tested against the fields known to exist in the supplied form.
	//
	//		I-class (in-situ values). These are valid if they name members of the applicable class and not otherwise. The applicable class is the host class of
	//				the supplied form. If no form is supplied, these are invalid.
	//
	//		V-class (session object values). These are valid if the name members of a session object that an earlier <procdata> command has brought into focus.
	//				Currently this is not tested.
	//
	//	Arguments:	1)	err			The error report string to be populated in the event of an error.
	//				2)	pFormdef	The host form where the percent entity is in (if any)
	//				3)	pHost		Applicable data class. If this is not supplied it will be the host class of the supplied form (if any)
	//				4)	pcntEnt		The string to tbe tested for validity as a percent entity
	//
	//	Returns		1)	E_SYNTAX	The entity is malformed
	//				2)	E_NOTFOUND	The entity or a component thereof, refers to a non existant class or variable
	//				3)	E_RANGE		The entity is not an available option
	//				4)	E_TYPE		The datatype of the entity is not as required (if set in arg2)
	//				5)	E_OK		The entity is valid

	_hzfunc("hdsApp::PcEntTest") ;

	const hdbClass*		pUC ;		//	User class (for checking %u: user entities
	const hdbMember*	pMbr ;		//	Class member

	hdbObjRepos*	pRepos = 0 ;	//	Relevent cache (either the cache for the logged in user or the cache containing current object of interest)
	hdsPage*		pPage = 0 ;		//	Hostpage of form
	hdsField*		pFld ;			//	Field within the form definition
	hzChain			W ;				//	Variable name buffer
	hzChain			erep ;			//	Error report
	hzAtom			atom ;			//	For litteral values and type testing
	const char*		i ;				//	Used to iterate input value
	hzString		w ;				//	Constructed variable name
	hzString		extn ;			//	Variable extension for binaries and documents (eg ->addr, ->data)
	hzString		val ;			//	Variable value
	//hdsUsertype		ut ;			//	User class
	uint32_t		srcInd ;		//	Source indicator
	uint32_t		n ;				//	General iterator
	hdbBasetype		dt ;			//	Actual datatype of entity
	hzEcode			rc = E_OK ;		//	Return code

	err = 0 ;
	if (!pcntEnt)
		{ err = "null entity" ; return BASETYPE_UNDEF ; }

	i = *pcntEnt ;
	if (*i != CHAR_PERCENT)
	{
		erep.Printf("Literal value %s not allowed", *pcntEnt) ;
		err = erep ;
		return BASETYPE_UNDEF ;
	}

	i++ ;
	srcInd = *i ;
	if (srcInd != 'x' && srcInd != 'u' && srcInd != 's' && srcInd != 'e' && srcInd != 'i' && srcInd != 'v')
		{ err = "Invalid percent entity class" ; return BASETYPE_UNDEF ; }

	i++ ;
	if (*i != CHAR_COLON)
		{ err = "Missing colon after percent entity class" ; return BASETYPE_UNDEF ; }

	//	Get the variable type and name component
	for (i++ ; *i && *i != CHAR_SCOLON && IsAlphanum(*i) ; i++)
		W.AddByte(*i) ;
	w = W ;
	W.Clear() ;

	if (*i == CHAR_PERIOD)
	{
		for (i++ ; *i && *i != CHAR_SCOLON && IsAlphanum(*i) ; i++)
			W.AddByte(*i) ;
		extn = W ;
		W.Clear() ;
	}

	if (*i != CHAR_SCOLON)
		{ erep.Printf("PcEntTest: Missing semicolon after percent entity variable name (%s), char is [%c]", *pcntEnt, *i) ; err = erep ; return BASETYPE_UNDEF ; }
	i++ ;

	//	Lookup variable and determine its value
	dt = BASETYPE_UNDEF ;

	if (!pHost)
	{
		if (pFormdef)
			pHost = pFormdef->m_pClass ;
	}

	switch (srcInd)
	{
	case 'x':	//	System variables
		if (w == "count" || w == "objid")
		{
			//	Count is of objects in a class
			pRepos = m_ADP.GetObjRepos(extn) ;
			if (!pRepos)
				{ rc = E_NOTFOUND ; erep.Printf("%s is not a declared repository", *extn) ; }
			else
				dt = BASETYPE_INT32 ;
			break ;
		}

		if (w == "date")		{ dt = BASETYPE_SDATE ;		break ; }
		if (w == "time")		{ dt = BASETYPE_TIME ;		break ; }
		if (w == "datetime")	{ dt = BASETYPE_XDATE ;		break ; }
		if (w == "referer")		{ dt = BASETYPE_URL ;		break ; }
		if (w == "usecs")		{ dt = BASETYPE_UINT32 ;	break ; }
		if (w == "random")		{ dt = BASETYPE_STRING ;	break ; }
		if (w == "uid")			{ dt = BASETYPE_INT32 ;		break ; }
		if (w == "user")		{ dt = BASETYPE_STRING ;	break ; }

		//	Also permitted are ....

		rc = E_RANGE ;
		erep.Printf("Invalid system entity (%s)", *w) ;
		break ;

	case 'u':	//	U-Class Percent Entities: For these to evaluate, the user must be logged in and the entity must name an atomic member of the applicable user class. However only
				//	the validity is being tested here, so user status is not important.

		if (!pFormdef)
			{ rc = E_ARGUMENT ; erep.Printf("All user variables must occur within a form lying in a page accessable to logged on users") ; break ; }
		//			if (!(pPage = pFormdef->m_pHostpage))
		//				{ rc = E_ARGUMENT ; erep.Printf("Cannot establish hostpage for form %s", *pFormdef->m_pRefname) ; break ; }
		//			if (!(pPage->m_Access & ACCESS_MASK))
		//				{ rc = E_ARGUMENT ; erep.Printf("User variable %s not accessible in page %s as it is not viewable by non users", *w, *pPage->m_Title) ; break ; } 

		//	Now go through the subscriber class and all user classes to which the access rules apply. If the access rules allow more than one class of
		//	user but the member named in the entity is not a member of every allowed class, then this is an error.

		if (!m_Allusers)
			{ rc = E_RANGE ; erep.Printf("Subscriber Cache not invoked - No users\n") ; break ; }

		pUC = m_Allusers->Class() ;
		pMbr = pUC->GetMember(w) ;
		if (pMbr)
			{ dt = pMbr->Basetype() ; break ; }

		for (n = 0 ; rc == E_OK && n < m_UserTypes.Count() ; n++)
		{
			val = m_UserTypes.GetKey(n) ;
			pUC = m_ADP.GetPureClass(val) ;
			if (!pUC)
				Fatal("%s. No such user class as %s\n", *_fn, *val) ;

			if (pPage->m_Access & m_UserTypes.GetObj(n))
			{
				pMbr = pUC->GetMember(w) ;
				if (pMbr)
					dt = pMbr->Basetype() ;
				else
				{
					rc = E_ARGUMENT ;
					erep.Printf("User variable %s not a member of user class %s and yet this class of user can access the page", *w, pUC->TxtTypename()) ;
				}
			}
		}

		break ;

	case 's':	//	Session variables (explicitly set as part of this or a parent form) constrained by the same set as event variables

		if (!m_tmpVarsSess.Exists(w))
			{ rc = E_RANGE ; erep.Printf("%s is not in scope", *w) ; }
		dt = m_tmpVarsSess[w] ;
		break ;

	case 'e':	//	Event variables. The entity is legal if the refered to variable is in scope (is within the supplied set of available variables)
		if (pFormdef)
		{
			if (!pFormdef->m_mapFlds.Exists(w))
				erep.Printf("Variable %s is not incident in form %s (%p)", *w, *pFormdef->m_Formname, pFormdef) ;
			else
			{
				pFld = pFormdef->m_mapFlds[w] ;
				if (pFld)
					{ dt = pFld->m_Fldspec.m_pType->Basetype() ; break ; }
			}
		}
		if (pHost)
		{
			pMbr = pHost->GetMember(w) ;
			if (pMbr)
				dt = pMbr->Basetype() ;
			else
				{ rc = E_RANGE ; erep.Printf("%s is not a member of class %s", *w, pHost->TxtTypename()) ; }
			break ;
		}

		if (!pFormdef && pHost)
			erep.Printf("Variable %s cannot be tested as there is neither a host form or class", *w) ;
		break ;

	case 'v':	//	Current object variables require the page they appear in to have a current object. This means there must be a current object class
				//	and a current object ID.

		if (!pHost)
			{ rc = E_ARGUMENT ; erep.Printf("All user variables must occur within the context of a host user class") ; break ; }
		pMbr = pHost->GetMember(w) ;
		if (!pMbr)
			{ rc = E_RANGE ; erep.Printf("%s is not a member of class %s and nor is it a freely declared variable", *w, pHost->TxtTypename()) ; }
		else
			dt = pMbr->Basetype() ;
		break ;
	}

	err = erep ;
	return dt ;
}

hzEcode	hdsApp::PcEntScanStr	(hzString& err, hdsFormdef* pFormdef, hdbClass* pHost, const hzString& input)
{
	//	Scans a string looking for percent entities to test. If none found then the supplied text is automatically deemed valid. If there are percent entities,
	//	these are tested for valididity by calling PcEntTest(). If any of these are invalid or out of scope the supplied string is deemed invalid.
	//
	//	Arguments:	1)	err		The error report string.
	//				2)	pFormdef	The currently applicable form (if any)
	//				3)	pHost	The currently applicable class. If this is NULL the class will be that of the supplied form (if any)
	//				4)	input	The input text
	//
	//	Returns:	E_SYNTAX	If part of the text appears to be a malformed percent entity.
	//				E_OK		If the supplied input contain no percent entities or valid percent entities.

	_hzfunc("hdsApp::PcEntScanStr") ;

	hzChain			erep ;			//	For error report
	const char*		i ;				//	Input iterator
	hzString		pcntEnt ;		//	Current percent entity
	hzString		cerr ;			//	Current entity error
	hdbBasetype		peType ;		//	Returned datatype
	hzEcode			rc = E_OK ;		//	Return code

	for (i = *input ; *i ;)
	{
		if (*i == CHAR_PERCENT)
		{
			if (i[1] == CHAR_PERCENT)
				{ i += 2 ; continue ; }

			if (IsAlpha(i[1]) && i[2] == CHAR_COLON && IsAlpha(i[3]))
			{
				if (IsPcEnt(pcntEnt, i))
				{
					peType = PcEntTest(cerr, pFormdef, pHost, pcntEnt) ;
					if (peType == BASETYPE_UNDEF)
						{ rc = E_SYNTAX ; erep.Printf("(%s %s)", *pcntEnt, *cerr) ; }
					else
						{ i += pcntEnt.Length() ; continue ; }
				}
				else
				{
					//	Sonething like %a:a but is not actually a percent entity is not allowed
					rc = E_SYNTAX ;
					erep.Printf("(%c%c%c%c - malformed percent entity)", i[0], i[1], i[2], i[3]) ;
				}
			}
		}
		i++ ;
	}

	err = erep ;
	return rc ;
}

hzEcode	hdsApp::PcEntScanChain	(hzString& err, hdsFormdef* pFormdef, hdbClass* pHost, const hzChain& input)
{
	//	Scans a chain (of HTML) looking for percent entities to test. If none found then the supplied text is automatically deemed valid. If there are percent
	//	entities, these are tested for valididity by calling PcEntTest(). If any of these are invalid or out of scope the supplied text is deemed invalid.
	//
	//	Arguments:	1)	err		The error report string.
	//				2)	pFormdef	The currently applicable form (if any)
	//				3)	pHost	The currently applicable class (if any)
	//				4)	input	The input text
	//
	//	Returns:	E_SYNTAX	If part of the text appears to be a malformed percent entity.
	//				E_OK		If the supplied input contain no percent entities or valid percent entities.

	_hzfunc("hdsApp::PcEntScanChain") ;

	hzChain			erep ;			//	For error report
	chIter			zi ;			//	Input iterator
	hzString		pcntEnt ;		//	Current percent entity
	hzString		cerr ;			//	Current entity error
	hdbBasetype		peType ;		//	Returned datatype
	hzEcode			rc = E_OK ;		//	Return code

	for (zi = input ; !zi.eof() ;)
	{
		if (*zi == CHAR_PERCENT)
		{
			if (zi[1] == CHAR_PERCENT)
				{ zi += 2 ; continue ; }

			if (IsAlpha(zi[1]) && zi[2] == CHAR_COLON && IsAlpha(zi[3]))
			{
				if (AtPcEnt(pcntEnt, zi))
				{
					peType = PcEntTest(cerr, pFormdef, pHost, pcntEnt) ;
					if (peType == BASETYPE_UNDEF)
						{ rc = E_SYNTAX ; erep.Printf("(%s %s)", *pcntEnt, *cerr) ; }
					else
						{ zi += pcntEnt.Length() ; continue ; }
				}
				else
				{
					//	Sonething like %a:a but is not actually a percent entity is not allowed
					rc = E_SYNTAX ;
					erep.Printf("(%c%c%c%c - malformed percent entity)", zi[0], zi[1], zi[2], zi[3]) ;
				}
			}
		}
		zi++ ;
	}

	err = erep ;
	return rc ;
}

hzEcode	hdsApp::PcEntConv	(hzAtom& atom, const hzString& v, hzHttpEvent* pE)
{
	//	Convert percent entities to set the type and value of the supplied atom. Here the percent entity is supplied in isolation. The ConvertText function
	//	(which calls this function), is used where percent entities can appear as part of a larger string (that is otherwise to be used verbatim).
	//
	//	Arguments:	1)	atom	The hzAtom to be set with the variable's value
	//				2)	v		The percent entity
	//				3)	pE		HTTP event pointer
	//	
	//	Returns:	E_SYNTAX	If the supplied percent entity string is invalid
	//				E_NOTFOUND	If the supplied percent entity is not matched
	//				E_OK		If the supplied percent entity string is valid

	_hzfunc("hdsApp::PcEntConv") ;

	const hdbMember*	pMbr ;		//	Class member

	hzChain			W ;				//	Variable name buffer
	hdsInfo*		pInfo ;			//	User session if applicable
	hdsFormref*		pFormref ;		//	From the event context
	hdsFormdef*		pFormdef ;		//	From the form reference
	hdsField*		pFld ;			//	Selected by name from the form
	const hdbEnum*	pSlct ;			//	Selector (for conversion of ints to strings)
	hdbObjRepos*	pRepos = 0 ;	//	Repository
	hdbObjCache*	pCache = 0 ;	//	Relevent cache (either the cache for the logged in user or the cache containing current object of interest)
	const char*		i ;				//	Used to iterate input value
	hzString		w_val ;			//	Constructed variable name
	hzString		extn ;			//	Variable extension for binaries and documents (eg ->addr, ->data)
	hzString		val ;			//	Variable value
	uint32_t		nTest ;			//	Category of percent directive
	uint32_t		strNo ;			//	Dictionary string number
	char			buf[12] ;		//	Buffer for random init code
	hzEcode			rc = E_OK ;		//	Return code

	if (!pE)
		hzexit(_fn, m_Appname, E_ARGUMENT, "No HTTP event supplied for input=%s", *v) ;

	atom.Clear() ;
	if (!v)
		return E_OK ;

	m_pLog->Log(_fn, "PcEntConv processing %s\n", *v) ;

	//	If nothing to convert
	if (v[0] != CHAR_PERCENT)
	{
		atom = v ;
		return E_OK ;
	}

	//	All percent entities must have colon after the percent and the source directive
	if (v[2] != CHAR_COLON)
		return E_SYNTAX ;

	//	Get the variable type and name component
	i = *v ;
	for (i += 3 ; *i && *i != CHAR_SCOLON && IsAlpha(*i) ; i++)
		W.AddByte(*i) ;
	w_val = W ;
	W.Clear() ;

	if (*i == CHAR_PERIOD)
	{
		for (i++ ; *i && *i != CHAR_SCOLON && IsAlpha(*i) ; i++)
			W.AddByte(*i) ;
		extn = W ;
		W.Clear() ;
	}

	if (*i == CHAR_SCOLON)
		i++ ;

	//	If there is a sesssion - this affects many percent entities
	pInfo = (hdsInfo*) pE->Session() ;

	//	Lookup variable and determine its value
	switch (v[1])
	{
	case 'x':	//	SYSTEM VALUES

		if (w_val == "count")
		{
			//	Count is of objects in a class
			pRepos = m_ADP.GetObjRepos(extn) ;
			if (!pRepos)
				{ rc = E_NOTFOUND ; m_pLog->Out("%s. Cannot Xlate. No such cache\n", *_fn) ; }
			else
				atom = pRepos->Count() ;
		}

		else if (w_val == "objid")
		{
			m_pLog->Out("%s. Translating objid: Cache %s\n", *_fn, *extn) ;
			pRepos = m_ADP.GetObjRepos(extn) ;
			if (!pRepos)
				{ rc = E_NOTFOUND ; m_pLog->Out("%s. Cannot Xlate. No such cache\n", *_fn) ; }
			else
			{
				if (!pE->m_ObjIds.Exists(extn))
					{ rc = E_NOTFOUND ; m_pLog->Out("%s. Cannot Xlate. No event entry\n", *_fn) ; }
				else
				{
					atom = pE->m_ObjIds[extn] ;
					m_pLog->Out("%s. Xlate OK. Cache %s has objid of %d\n", *_fn, *extn, pE->m_ObjIds[extn]) ;
				}
			}
		}

		else if (w_val == "date")		atom = pE->m_Occur.Date() ;
		else if (w_val == "time")		atom = pE->m_Occur.Time() ;
		else if (w_val == "datetime")	atom = pE->m_Occur ;
		else if (w_val == "referer")	atom.SetValue(BASETYPE_STRING, pE->Referer().Resource()) ;
		else if (w_val == "usecs")		atom = pE->m_Occur.uSec() ;
		else if (w_val == "uid")		atom = (uint32_t) (pInfo ? pInfo->m_UserId : 0) ;
		else if (w_val == "random")		{ sprintf(buf, "%06d", pE->m_Occur.uSec()) ; atom.SetValue(BASETYPE_STRING, buf) ; }
		else
			rc = E_NOTFOUND ;

		m_pLog->Out("%s. Have now value of %s = %s\n", *_fn, *w_val, *atom.Str()) ;
		break ;

	case 'e':	//	HTTP EVENT VARIABLES

		pSlct = m_ADP.GetDataEnum(w_val) ;
		if (pSlct)
		//if (m_ADP.mapEnums.Exists(w_val))
		{
			//	If a selector, values returned in form submissions will be integers
			val = pE->m_mapStrings[w_val] ;

			if (IsPosint(nTest, *val))
			{
				//pSlct = m_ADP.mapEnums[w_val] ;
				strNo = pSlct->m_Items[nTest] ;
				
				rc = atom.SetValue(BASETYPE_STRING, _hzGlobal_StringTable->Xlate(strNo)) ;
				break ;
			}
		}

		if (pE->m_Uploads.Exists(w_val))
		{
			//	Find an entry in m_Files paart of the HTTP event - then give either the file name or the file data

			hzHttpFile&	hf = pE->m_Uploads[w_val] ;
			if (extn == "name")
				rc = atom.SetValue(BASETYPE_STRING, hf.m_filename) ;
			else if (extn == "data")
			{
				m_pLog->Out("%s. Have file %s of %d bytes\n", *_fn, *hf.m_filename, hf.m_file.Size()) ;
				if (hf.m_file.Size())
					{ rc = E_OK ; atom = hf.m_file ; }
				else
					rc = E_NODATA ;
				m_pLog->Out("%s. Have file %s of %d bytes\n", *_fn, *hf.m_filename, hf.m_file.Size()) ;	//atom.Chain().Size()) ;
			}
			else
				rc = E_BADVALUE ;
			break ;
		}

		if (w_val == g_Errmsg)
		{
			if (pE->m_appError)
				atom = pE->m_appError ;
			break ;
		}

		if (w_val == "resarg")
		{
			atom.SetValue(BASETYPE_STRING, pE->m_Resarg) ;
			break ;
		}

		if (pE->m_mapStrings.Exists(w_val))
		{
			val = pE->m_mapStrings[w_val] ;

			if (pE->m_pContextForm)
			{
				pFormref = (hdsFormref*) pE->m_pContextForm ;
				pFormdef = m_FormDefs[pFormref->m_Formname] ;

				if (!pFormdef->m_mapFlds.Exists(w_val))
				{
					m_pLog->Out("%s. case 1: Using string context value %s = %s\n", *_fn, *w_val, *val) ;
					rc = atom.SetValue(BASETYPE_STRING, val) ;
				}
				else
				{
					pFld = pFormdef->m_mapFlds[w_val] ;
					if (pFld)
					{
						//	m_pLog->Out("%s. case 2: Using string context value %s = %s %s\n", *_fn, *w_val, Basetype2Txt(pFld->m_Fldspec.m_pFldtype->dtype), *val) ;
						rc = atom.SetValue(pFld->m_Fldspec.m_pType->Basetype(), val) ;
					}
					else
					{
						m_pLog->Out("%s. case 3: Using string context value %s = %s\n", *_fn, *w_val, *val) ;
						rc = atom.SetValue(BASETYPE_STRING, val) ;
					}
				}
			}
			else
			{
				m_pLog->Out("%s. Using context-less value %s\n", *_fn, *val) ;
				rc = atom.SetValue(BASETYPE_STRING, val) ;
			}
			break ;
		}

		if (pE->m_mapChains.Exists(w_val))
		{
			val = pE->m_mapChains[w_val] ;

			if (pE->m_pContextForm)
			{
				pFormref = (hdsFormref*) pE->m_pContextForm ;
				pFormdef = m_FormDefs[pFormref->m_Formname] ;

				if (!pFormdef->m_mapFlds.Exists(w_val))
				{
					m_pLog->Out("%s. case 1: Using chain context value %s = %s\n", *_fn, *w_val, *val) ;
					rc = atom.SetValue(BASETYPE_STRING, val) ;
				}
				else
				{
					m_pLog->Out("%s. case 2: Using chain context value %s = %s\n", *_fn, *w_val, *val) ;
					pFld = pFormdef->m_mapFlds[w_val] ;
					rc = atom.SetValue(pFld->m_Fldspec.m_pType->Basetype(), val) ;
				}
			}
			else
			{
				m_pLog->Out("%s. Using context-less value %s\n", *_fn, *val) ;
				rc = atom.SetValue(BASETYPE_STRING, val) ;
			}
			break ;
		}

		rc = E_NOTFOUND ;
		break ;

	case 's':	//	SESSION VARIABLES

		rc = E_NOTFOUND ;
		if (pInfo && pInfo->m_Sessvals.Count())
		{
			if (pInfo->m_Sessvals.Exists(w_val))
			{
				atom = pInfo->m_Sessvals[w_val] ;
				rc = E_OK ;
			}
			break ;
		}
		m_pLog->Log(_fn, "Warning: No such session value [%s]\n", *w_val) ;
		break ;

	case 'i':	//	IN-SITU OBJECT MEMBERS

		rc = E_NOTFOUND ;
		if (!pInfo)
			break ;
		if (!pInfo->m_CurrRepos)
			break ;
		if (!pInfo->m_CurrObj)
			break ;

		//pRepos = m_ADP.m_arrRepositories[pInfo->m_CurrRepos] ;
		pRepos = m_ADP.GetObjRepos(pInfo->m_CurrRepos) ;
		if (pRepos)
			pCache = dynamic_cast<hdbObjCache*>(pRepos) ;
		if (pCache)
			pMbr = pCache->GetMember(w_val) ;

		if (pMbr)
		{
			if (pMbr->Basetype() == BASETYPE_BINARY || pMbr->Basetype() == BASETYPE_TXTDOC)
			{
				if (extn == "addr")
				{
					m_pLog->Out("Fetching address %s\n", *w_val) ;
					rc = pCache->Fetchval(atom, w_val, pInfo->m_CurrObj) ;
				}
				if (extn == "data")
				{
					m_pLog->Out("Fetching binary body %s\n", *w_val) ;
					rc = pCache->Fetchbin(atom, w_val, pInfo->m_CurrObj) ;
				}
			}
			else
			{
				m_pLog->Out("Fetching ord %s\n", *w_val) ;
				rc = pCache->Fetchval(atom, w_val, pInfo->m_CurrObj) ;
			}
		}
		break ;

	case 'v':	//	CURRENT OBJECT MEMBERS
		break ;

	case 'u':	//	CURRENT USER-INFO MEMBERS (also user dirs and files)

		rc = E_NOTFOUND ;
		if (!pInfo)
			break ;

		m_pLog->Out("%s. Fetching item %s for subscriber id %d\n", *_fn, *w_val, pInfo->m_SubId) ;

		if (w_val == "username")
		{
			if (m_Allusers)
				rc = m_Allusers->Fetchval(atom, SUBSCRIBER_USERNAME, pInfo->m_SubId) ;
		m_pLog->Out("%s. Fetched item %s\n", *_fn, *atom.Str()) ;
			break ;
		}

		if (pInfo->m_CurrRepos && pInfo->m_UserId > 0)
		{
			//pRepos = m_ADP.m_arrRepositories[pInfo->m_CurrRepos] ;
			pRepos = m_ADP.GetObjRepos(pInfo->m_CurrRepos) ;
			if (pRepos)
				pCache = dynamic_cast<hdbObjCache*>(pRepos) ;
			if (pCache)
				pMbr = pCache->GetMember(w_val) ;

			if (!pMbr)
				break ;

			if (pMbr->Basetype() == BASETYPE_BINARY || pMbr->Basetype() == BASETYPE_TXTDOC)
			{
				if (extn == "addr")
				{
					m_pLog->Out("Fetching address %s\n", *w_val) ;
					rc = pCache->Fetchval(atom, w_val, pInfo->m_UserId) ;
				}
				if (extn == "data")
				{
					m_pLog->Out("Fetching binary body %s\n", *w_val) ;
					rc = pCache->Fetchbin(atom, w_val, pInfo->m_UserId) ;
				}
			}
			else
			{
				m_pLog->Out("Fetching ord %s\n", *w_val) ;
				rc = pCache->Fetchval(atom, w_val, pInfo->m_UserId) ;
			}
		}
		break ;

	default:
		rc = hzerr(_fn, HZ_ERROR, E_SYNTAX, "Invalid Percent Entity Category (%c)", v[1]) ;
	}

	return rc ;
}

void	hdsApp::ConvertText	(hzChain& Z, hzHttpEvent* pE)
{
	//	Process the supplied data (assumed to be text) by replacing any incident percent entities by thier 'here and now' values. The data is supplied
	//	as a chain which remains in situ and will be unchanged if there are no succesful percent entity conversions.
	//
	//	Arguments:	1)	Z	Data to be treated as text which may contain one or more percent entities.
	//				2)	pE	The HTTP event from which the non system and non user variables can be garnered or from which the user can be established.
	//
	//	Returns:	None

	_hzfunc("hdsApp::ConvertText(1)") ;

	hzChain		W ;				//	Variable name buffer
	hzChain		C ;				//	Final value
	chIter		zi ;			//	Interation of contnt
	hzAtom		atom ;			//	User profile record variable
	hzString	pcntEnt ;		//	Constructed variable name
	hzEcode		rc ;			//	Variable location. 1 system, 2 event, 3 session, 4 user value (must be logged in with session & cookie)

	if (!Z.Size())
		return ;

	for (zi = Z ; !zi.eof() ;)
	{
		if (*zi == CHAR_AT)
		{
			if (zi == "@resarg;")
			{
				if (pE->m_Resarg)
					C << pE->m_Resarg ;
				zi += 8 ;
				continue ;
			}
		}

		if (*zi == CHAR_PERCENT)
		{
			if (zi[1] == CHAR_PERCENT)
				{ C << "%%" ; zi += 2 ; continue ; }

			if (IsAlpha(zi[1]) && zi[2] == CHAR_COLON && IsAlpha(zi[3]))
			{
				if (AtPcEnt(pcntEnt, zi))
				{
					rc = PcEntConv(atom, pcntEnt, pE) ;

					if (atom.Type() != BASETYPE_UNDEF)
					{
						if (atom.IsSet())
							C << atom.Str() ;
					}
					else
					{
						m_pLog->Log(_fn, "Atom type unknown %s\n", *pcntEnt) ;
						C << pcntEnt ;
					}

					zi += pcntEnt.Length() ;
					continue ;
				}
			}
		}

		C.AddByte(*zi) ;
		zi++ ;
	}

	Z.Clear() ;
	Z = C ;
}

hzString	hdsApp::ConvertText	(const hzString& str, hzHttpEvent* pE)
{
	//	Process the supplied data (assumed to be text) by replacing any incident percent entities by thier 'here and now' values. The data is supplied
	//	as a string which remains in situ and will be unchanged if there are no succesful percent entity conversions.
	//
	//	Arguments:	1)	S	Data to be treated as text which may contain one or more percent entities.
	//				2)	pE	The HTTP event from which the non system and non user variables can be garnered or from which the user can be established.
	//
	//	Returns:	None

	_hzfunc("hdsApp::ConvertText(2)") ;

	hzChain		Z ;		//	Temporary chain
	hzString	S ;		//	Resultant string

	Z = str ;
	ConvertText(Z, pE) ;
	S = Z ;

	return S ;
}

const char*	Exec2Txt	(Exectype eType) ;

/*
**	Main Section: Request Processing
*/

hzEcode	hdsApp::_SubscriberAuthenticate	(hzHttpEvent* pE)
{
	//	Handle login Submissions
	//
	//	Note all user are authenticated against the subscriber repository

	_hzfunc("hdsApp::_SubscriberAuthenticate") ;

	hzAtom			atom ;			//	Atom for fetching values
	hdsInfo*		pInfo ;			//	Session
	hdbObjCache*	pCache ;		//	User repository
	hzSysID			newCookie ;		//	For cookie generation
	hzString		unam ;			//	Username (if form is login)
	hzString		pass ;			//	Password (if form is login)
	hzString		S ;				//	Temp string
	hzString		utName ;		//	User type name
	hzSDate			expires ;		//	For cookie expiry
	hzIpaddr		ipa ;			//	Client IP address
	uint32_t		objId ;			//	For user lookup
	uint32_t		utAccess ;		//	User type access flags
	hzEcode			rc = E_OK ;		//	Return code

	pInfo = (hdsInfo*) pE->Session() ;
	ipa = pE->ClientIP() ;

	if (!pInfo)
	{
		newCookie = MakeCookie(ipa, pE->EventNo()) ;
		expires.SysDate() ;
		expires += 365 ;

		pE->SetSessCookie(newCookie) ;
		pInfo = new hdsInfo() ;
		pE->SetSession(pInfo) ;
		m_SessCookie.Insert(newCookie, pInfo) ;
	}

	if (!pE->m_mapStrings.Exists(m_UsernameFld) || !pE->m_mapStrings.Exists(m_UserpassFld))
	{
		pE->m_appError = "Error: Form has login status but not the expected fields" ;
		m_pLog->Log(_fn, "%s. Error: Form has login status but not the expected fields (%s and %s)\n", *_fn, *m_UsernameFld, *m_UserpassFld) ;
		return E_NOTFOUND ;
	}

	unam = pE->m_mapStrings[m_UsernameFld] ;
	pass = pE->m_mapStrings[m_UserpassFld] ;

	m_pLog->Log(_fn, "username fld = %s\n", *m_UsernameFld) ;
	m_pLog->Log(_fn, "userpass fld = %s\n", *m_UserpassFld) ;
	m_pLog->Log(_fn, "Master user = %s\n", *m_MasterUser) ;
	m_pLog->Log(_fn, "Master pass = %s\n", *m_MasterPass) ;
	m_pLog->Log(_fn, "Trying username (%s) password (%s)\n", *unam, *pass) ;

	if (m_MasterPage && unam == m_MasterUser && pass == m_MasterPass)
	{
		//	Log user in as administrator with admin access rights 
		m_pLog->Log(_fn, "ADMIN PAGE !!!\n") ;
		pInfo->m_Access |= ACCESS_ADMIN ;
		if (!_hzGlobal_StatusIP.Exists(ipa))
			SetStatusIP(ipa, HZ_IPSTATUS_WHITE_HTTP, 900) ;
		m_MasterPage->Display(pE) ;
		return E_OK ;
	}

	//	The username (which could be an email address) is used to look up the user in the 'allusers' cache. The class of the actual user will
	//	be derived from this.

	rc = m_Allusers->Identify(objId, m_UsernameFld, unam) ;
	m_pLog->Out("%s. Login atempt %s=%s, obj=%d rc=%s\n", *_fn, *m_UserpassFld, *pass, objId, Err2Txt(rc)) ;

	if (objId >= 0)
	{
		m_pLog->Out("%s. Trying %s=%s, obj=%d\n", *_fn, *m_UserpassFld, *pass, objId) ;
		rc = m_Allusers->Fetchval(atom, SUBSCRIBER_PASSWORD, objId) ;

		if (rc != E_OK)
			m_pLog->Out("%s. No match on subscriber. Fetchval returns %s\n", *_fn, Err2Txt(rc)) ;
		else
		{
			pInfo->m_SubId = objId ;

			if (atom.Str() != pass)
			{
				m_pLog->Out("%s. No match on stored %s to supplied %s\n", *_fn, *atom.Str(), *pass) ;
				rc = E_NOTFOUND ;
			}
			else
			{
				//	Log user in as a user of the appropriate class
				m_SessCookie.Insert(pE->Cookie(), pInfo) ;
				rc = m_Allusers->Fetchval(atom, SUBSCRIBER_USERTYPE, objId) ;

				if (rc != E_OK)
					m_pLog->Out("%s. Could not fetch value for subscriber user type with object id %d\n", *_fn, objId) ;
				else
				{
					S = atom.Str() ;
					if (S)
					{
						utAccess = m_UserTypes[S] ;
						pCache = (hdbObjCache*) m_ADP.GetObjRepos(S) ;
						if (pCache)
							pInfo->m_UserRepos = pCache->DeltaId() ;
					}

					rc = m_Allusers->Fetchval(atom, SUBSCRIBER_USERADDR, objId) ;
					if (rc != E_OK)
						m_pLog->Out("%s. Could not fetch value for subscriber address with object id %d\n", *_fn, objId) ;
					else
					{
						pInfo->m_Username = unam ;
						pInfo->m_UserId = atom.Int32() ;
						pInfo->m_Access &= ACCESS_ADMIN ;
						pInfo->m_Access |= utAccess ;
						m_pLog->Out("%s. Fetched value for subscriber address with object id %d %d usrType %s and access %d\n",
							*_fn, objId, pInfo->m_UserId, *S, pInfo->m_Access) ;
					}
				}
			}
		}

		if (rc != E_OK)
			pE->m_appError = "Please check your username and password and try again" ;
	}

	return rc ;
}

hzSysID	hdsApp::MakeCookie	(const hzIpaddr& ipa, uint32_t eventNo)
{
	//	Category:	Internet
	//
	//	HadronZoo::Dissemino cookies have names based on fixed names Make a unique cookie for your internet aplication
	//
	//	Arguments:	1)	Cookie	The target where the cookie will be stored (hzString reference)
	//				2)	ipa		The browser ip address
	//				3)	appname	The application name
	//
	//	Returns:	E_MEMORY	If there was insufficient memory to complete the operation
	//				E_OK		If the operation was successful

	_hzfunc(__func__) ;

	hzSysID		cookie ;		//	The cookie to be
	uint64_t	seed ;			//	The cookie value to be
	time_t		pt ;			//	Current system epoch time stamp

	seed = (uint64_t) 0 ;
	pt = time(&pt) ;

	seed = (pt << 32) ;
	seed &= 0xffffffff00000000 ;
	seed |= (uint32_t) ipa ;

	cookie = seed ;
	return cookie ;
}

void	hdsApp::ProcForm	(hzHttpEvent* pE, hdsFormref* pFormref, hdsFormhdl* pFhdl)
{
	//	Execute all commands specified within a form handler and then formulate the HTTP response.
	//
	//	This runs each command in the form-handler, in the order set out in the configs. The only scenario in which a form handler is invoked is when a form is submitted and in all
	//	cases, there must be a HTTP response. Execution of the commands will halt on the first error.
	//
	//	Most commands either succeed or fail and where they fail, the matter can either be resolved by the visitor or user submitting the form or it cannot be. A resource not found
	//	because of a typo .....
	//
	//	The errors vary but form a narrow set overall. A resource is either granted or it is not. In the general case where a form is submitted by a human using
	//	a browser, format errors are resolved by JavaScript in the forms themselves. However this cannot be assumed so format errors have to be coped with. They
	//	are trapped server side when they fail to set variables and are always treated as critical.
	//
	//	The send email command can be inconclusive. An email address could be malformed, the implied domain may not exist, may not have any mail servers and may
	//	even have an invalid DNS entry. Or the email could be rejected by the destination server. All of these are critical errors and so will stop execution of
	//	the whole form handler. But if the DNS is down or busy, or if the destination server is busy, the emails are not sent directly to the destination server
	//	are are queued instead to be sent out via the local SMTP server. Under these circumstances it is not possible for the Dissemino application to discover
	//	the eventual outcome. But since it is highly probably the email will arrive it would be rather silly not to proceed with the rest of the form handler's
	//	commands. It may be worth advising the user that the email was queued in the form response though.
	//
	//	While command outcomes are limited to either a fail, a success or a presumed success, remedial action for a failure will depend on the command. In cases
	//	where the original form can be modified to correct the error, the HTTP response should be to re-display the original form. Where the error cannot be
	//	corrected by the user the HTTP response will ask the user to type again later. The way this is handled is that all commands should be supplied with an
	//	error directive. Then at the end of the form-handler a <response> tag will specify the HTTP response on success.
	//
	//	The responses must be either a PAGE or a C-Interface FUNCTION. They cannot be a fixed HTML page or an article.
	//
	//	Arguments:	1)	pE			The HTTP event pointer
	//				2)	pFormref	Form reference
	//				3)	pFhdl		Form handler
	//
	//	Returns:	None

	_hzfunc("hdsApp::ProcForm") ;

	hzList<hdsExec*>::Iter	ei ;	//	Command iterator

	hzChain			Z ;				//	For formulating response
	hzChain			errorReport ;	//	For formulating response
	hzPair			P ;				//	For checking presence of submitted data
	hdsInfo*		pInfo ;			//	Session
	hdsFormdef*		pFormdef ;		//	Form
	hdsField*		pFld ;			//	Field pointer
	hdsExec*		pExec ;			//	Exec function
	hdsResource*	pResponse ;		//	Resource to respond with
	hdsPage*		pPage = 0 ;		//	Page to respond with
	hdsCIFunc*		pFunc = 0 ;		//	C-Interface function to respond with
	hzAtom			atom ;			//	Atom for fetching values
	hzSysID			newCookie ;		//	For cookie generation
	hzXDate			now ;			//	Time now
	hzString		S ;				//	Temp string
	hzString		unam ;			//	Username (if form is login)
	hzString		pass ;			//	Password (if form is login)
	hzIpaddr		ipa ;			//	Client IP address
	hzSDate			expires ;		//	For cookie expiry
	uint32_t		nIndex ;		//	Name value pair iterator
	//hdsUsertype		usrType ;		//	User type (for loging in)
	hzEcode			rc = E_OK ;		//	Return code

	pInfo = (hdsInfo*) pE->Session() ;
	ipa = pE->ClientIP() ;
	now.SysDateTime() ;

	//	Check for form reference and form handler
	if (!pFormref)	Fatal("%s. No form reference supplied\n", *_fn) ;
	if (!pFhdl)		Fatal("%s. No form handler supplied\n", *_fn) ;

	m_pLog->Out("%s. Processing form reference %p flags %x handler %p and info %p\n", *_fn, pFormref, pFormref->m_flagVE, pFhdl, pInfo) ;

	pFormdef = pFhdl->m_pFormdef ;
	if (!pFormdef)
		Fatal("%s. Form handler (%s) has no form\n", *_fn, *pFhdl->m_Refname) ;

	m_pLog->Out("%s. Processing form reference %p and info %p\n", *_fn, pFormref, pInfo) ;
	m_pLog->Out("%s. Processing form %p %s with %d fields and info %p\n", *_fn, pFormdef, *pFormdef->m_Formname, pFormdef->m_vecFlds.Count(), pInfo) ;
	pE->m_pContextForm = pFormref ;

	//	Check if there is input
	if (!pE->Inputs())
	{
		rc = E_NODATA ;
		m_pLog->Out("%s. Form has no data\n", *_fn) ;
		goto respond ;
	}

	//	Check if input actually fits the expected format. Each field in the form ...
	for (nIndex = 0 ; nIndex < pE->m_Inputs.Count() ; nIndex++)
	{
		P = pE->m_Inputs[nIndex] ;
			m_pLog->Out("%s. Doing field %s %s\n", *_fn, *P.name, *P.value) ;
		pFld = pFormdef->m_mapFlds[P.name] ;
		if (!pFld)
		{
			m_pLog->Out("%s. No such field as %s\n", *_fn, *P.name) ;
			rc = E_NOTFOUND ;
		}
	}

	if (rc != E_OK)
		goto respond ;

	//	for (nIndex = 0 ; nIndex < pFormdef->m_mapFlds.Count() ; nIndex++)
	//	{
	//		pFld = pFormdef->m_mapFlds.GetObj(nIndex) ;
	//	}

	//	See if a session is needed
	if (pFhdl->m_flgFH & VE_COOKIES)
	{
		if (!pInfo)
		{
			newCookie = MakeCookie(ipa, pE->EventNo()) ;
			expires.SysDate() ;
			expires += 365 ;

			pE->SetSessCookie(newCookie) ;
			pInfo = new hdsInfo() ;
			pE->SetSession(pInfo) ;
			m_SessCookie.Insert(newCookie, pInfo) ;

			m_pLog->Out("%s: %s New cookie %016x and session %p\n", *_fn, *now.Str(), newCookie, pInfo) ;
		}
	}

	//	Perform exec functions
	for (ei = pFhdl->m_Exec ; ei.Valid() ; ei++)
	{
		m_pLog->Out("%s CASE 5\n", *_fn) ;

		pExec = ei.Element() ;
		if (!pExec)
		{
			m_pLog->Out("%s. ERROR. Null exec at posn %d\n", *_fn, nIndex) ;
			continue ;
		}
		m_pLog->Out("%s. HAVE EXEC %s\n", *_fn, Exec2Txt(pExec->m_Command)) ;

		rc = pExec->Exec(errorReport, pE) ;
		m_pLog->Out(errorReport) ;
		errorReport.Clear() ;

		if (rc == E_OK)
		{
			m_pLog->Out("%s. Sucess exec %s at posn %d (err=%s)\n", *_fn, Exec2Txt(pExec->Whatami()), nIndex, Err2Txt(rc)) ;
			continue ;
		}

		//	Failed
		m_pLog->Out("%s. Failed exec %s at posn %d (err=%s)\n", *_fn, Exec2Txt(pExec->Whatami()), nIndex, Err2Txt(rc)) ;

		if (pExec->m_FailGoto || pExec->m_pFailResponse)
		{
			//	The exec step has a fail directive

			if (errorReport.Size())
				pE->m_appError = errorReport ;

			if (pExec->m_pFailResponse)
				pResponse = pExec->m_pFailResponse ;
			else
			{
				if (pExec->m_FailGoto[0] == CHAR_PERCENT)
				{
					S = ConvertText(pExec->m_FailGoto, pE) ;
					m_pLog->Out("%s. Converted error page directive %s to %s\n", *_fn, *pExec->m_FailGoto, *S) ;
					pResponse = m_ResourcesName[S] ;
					if (!pResponse)
						pResponse = m_ResourcesPath[S] ;
				}
				else
				{
					m_pLog->Out("%s. Using named error page %s\n", *_fn, *pExec->m_FailGoto) ;
					pResponse = m_ResourcesName[pExec->m_FailGoto] ;
					if (!pResponse)
						pResponse = m_ResourcesPath[pExec->m_FailGoto] ;
				}
			}

			if (!pResponse)
				SendErrorPage(pE, HTTPMSG_OK, *_fn, "Could not find an error page for exec %s in form %s", Exec2Txt(pExec->Whatami()), *pFhdl->m_Refname) ;
			else
			{
				pPage = dynamic_cast<hdsPage*>(pExec->m_pFailResponse) ;
				if (pPage)
				{
					m_pLog->Out("%s. Using error page %s\n", *_fn, *pPage->m_Title) ;
					pPage->Display(pE) ;
				}
				else
				{
					pFunc = dynamic_cast<hdsCIFunc*>(pResponse) ;
					pFunc->m_pFunc(pE) ;
				}
			}
			return ;
		}
	}

	//	Formulate response. This is a case of running the process associated with form and/or the button used to submit the form. And
	//	then either presenting the results or going to a specified page.

respond:
	if (rc == E_OK)
	{
		if (pFhdl->m_pCompletePage)
			pResponse = pFhdl->m_pCompletePage ;
		else
		{
			if (pFhdl->m_CompleteGoto[0] == CHAR_PERCENT)
			{
				S = ConvertText(pFhdl->m_CompleteGoto, pE) ;
				m_pLog->Out("%s. Converted response page directive %s to %s\n", *_fn, *pFhdl->m_CompleteGoto, *S) ;
				pResponse = m_ResourcesName[S] ;
				if (!pResponse)
					pResponse = m_ResourcesPath[S] ;
			}
			else
			{
				m_pLog->Out("%s. Using named response page %s\n", *_fn, *pFhdl->m_CompleteGoto) ;
				pResponse = m_ResourcesName[pFhdl->m_CompleteGoto] ;
				if (!pResponse)
					pResponse = m_ResourcesPath[pFhdl->m_CompleteGoto] ;
			}
		}

		if (!pResponse)
			SendErrorPage(pE, HTTPMSG_NOTFOUND, *_fn, "Could not find a response page for form %s", *pFhdl->m_Refname) ;
		else
		{
			pPage = dynamic_cast<hdsPage*>(pResponse) ;
			if (pPage)
			{
				m_pLog->Out("%s. Using response page %s\n", *_fn, *pPage->m_Title) ;
				pPage->Display(pE) ;
			}
			else
			{
				pFunc = dynamic_cast<hdsCIFunc*>(pResponse) ;
				pFunc->m_pFunc(pE) ;
			}
		}
	}
	else
	{
		if (errorReport.Size())
			pE->m_appError = errorReport ;

		if (pFhdl->m_pFailDfltPage)
			pResponse = pFhdl->m_pFailDfltPage ;
		else
		{
			if (pFhdl->m_FailDfltGoto[0] == CHAR_PERCENT)
			{
				S = ConvertText(pFhdl->m_FailDfltGoto, pE) ;
				m_pLog->Out("%s. Converted error page directive %s to %s\n", *_fn, *pFhdl->m_FailDfltGoto, *S) ;
				pResponse = m_ResourcesName[S] ;
				if (!pResponse)
					pResponse = m_ResourcesPath[S] ;
			}
			else
			{
				m_pLog->Out("%s. Using named error page %s\n", *_fn, *pFhdl->m_FailDfltGoto) ;
				pResponse = m_ResourcesName[pFhdl->m_FailDfltGoto] ;
				if (!pResponse)
					pResponse = m_ResourcesPath[pFhdl->m_FailDfltGoto] ;
			}
		}

		if (!pResponse)
			SendErrorPage(pE, HTTPMSG_OK, *_fn, "Could not find an error page for form %s", *pFhdl->m_Refname) ;
		else
		{
			pPage = dynamic_cast<hdsPage*>(pFhdl->m_pFailDfltPage) ;
			if (pPage)
			{
				m_pLog->Out("%s. Using error page %s\n", *_fn, *pPage->m_Title) ;
				pPage->Display(pE) ;
			}
			else
			{
				pFunc = dynamic_cast<hdsCIFunc*>(pResponse) ;
				pFunc->m_pFunc(pE) ;
			}
		}
	}

	m_pLog->Out("%s. Processed form %p %s with %d fields and info %p\n", *_fn, pFormdef, *pFormdef->m_Formname, pFormdef->m_vecFlds.Count(), pInfo) ;
}

void	hdsApp::InPageQuery	(hzHttpEvent* pE)
{
	//	Respond to an in-page query (Dissemino AJAX)
	//
	//	An in-page query is an AJAX request to a URL of the form '/ck-repos.member.value' - to which the response is a blank page with a HTTP return code of either 200 (OK) or 404
	//	(NOTFOUND). A code of 200 indicates that the repository exists, is populated and is of a class which possesses the named member.
	//
	//	The purpose of an in-page query is to support form validation. Fields which must contain a unique value such as an email address or a username (a common requirement in site
	//	membership application/registration forms), can be validated by these AJAX calls. 
	//
	//	Argument:	pE	The HTTP event pointer
	//
	//	Returns:	None

	_hzfunc("hdsApp::InPageQuery") ;

	hzChain			Z ;				//	For arg processing
	hdbObjCache*	pCache ;		//	Use class (of desired document)
	const char*		r ;				//	Pointer into event resource
	hzString		S ;				//	Temp string
	hzString		val ;			//	Value
	uint32_t		objId ;			//	Object found
	hzEcode			rc = E_OK ;		//	Return code

	//	Get class/cache name
	r = pE->GetResource() ;
	for (r += 4 ; IsAlpha(*r) ; r++)
		Z.AddByte(*r) ;
	S = Z ;
	Z.Clear() ;
	pCache = (hdbObjCache*) m_ADP.GetObjRepos(S) ;
	if (!pCache)
		{ rc = E_NOTFOUND ; goto fail ; }

	if (*r != CHAR_PERIOD)
		{ rc = E_FORMAT ; goto fail ; }

	//	Get member name & value
	for (r++ ; IsAlpha(*r) ; r++)
		Z.AddByte(*r) ;
	S = Z ;
	Z.Clear() ;

	if (*r != CHAR_PERIOD)
		{ rc = E_FORMAT ; goto fail ; }
	val = ++r ;

	m_pLog->Out("Doing Binary fetch on %s %s\n", *S, *val) ;
	rc = pCache->Identify(objId, S, val) ;
	if (rc != E_OK)
		{ m_pLog->Out("Binary fech function failed. Err=%s\n", Err2Txt(rc)) ; goto fail ; }

	//	Serve it
	m_pLog->Out("%s. Located record %d\n", __func__, objId) ;
	if (objId)
	{
		pE->SendAjaxResult(HTTPMSG_OK) ;
		return ;
	}

fail:
	pE->SendAjaxResult(HTTPMSG_NOTFOUND) ;
	return ;
}

void	hdsApp::SendDocument	(hzHttpEvent* pE)
{
	//	Send a document to the client browser.
	//
	//	This function is called to respond to requests for URLs begining with '/docs' which Dissemino reserves for documents or other binary objects. Typically,
	//	these would be such things as .pdf files, commonly but not always uploaded by users.
	//
	//	The general form of the URL would be is "/docs/repository_name.membername.objid" as this will uniquely identify the document. This function will produce
	//	a 404 HTML response if the remainder of the URL is malformed, the repository name does not exist as a repository, the named member does not exist within
	//	the repository class or is not of a document or binary datatype or the given object id is not found in the repository.
	//
	//	Arguments:	1)	pE	The HTTP event pointer
	//
	//	Returns:	None

	_hzfunc("hdsApp::SendDocument") ;

	hzChain			E ;				//	Error report
	hzChain			X ;				//	Recepticle
	hdsInfo*		pInfo = 0 ;		//	Session
	hdbObjCache*	pCache ;		//	Use class (of desired document)
	const char*		r ;				//	Pointer into event resource
	hzString		S ;				//	Temp string
	uint32_t		docid = 0 ;		//	Object ID
	hzMimetype		mt ;			//	Mimetype
	hzEcode			rc = E_OK ;		//	Return codee

	pInfo = m_SessCookie[pE->Cookie()] ;

	//	Get class/cache name
	r = pE->GetResource() ;
	for (r += 6 ; IsAlpha(*r) ; r++)
		X.AddByte(*r) ;
	S = X ;
	X.Clear() ;
	pCache = (hdbObjCache*) m_ADP.GetObjRepos(S) ;

	if (!pCache)
		{ E.Printf("Dissemino could not locate the document cache") ; goto fail ; }
	if (*r != CHAR_PERIOD)
		{ E.Printf("Bad format for document request") ; goto fail ; }

	//	Get member name
	for (r++ ; IsAlpha(*r) ; r++)
		X.AddByte(*r) ;
	S = X ;
	E.Clear() ;

	if (*r != CHAR_PERIOD)
		{ E.Printf("Bad format for document request") ; goto fail ; }

	for (r++ ; IsDigit(*r) ; r++)
		{ docid *= 10 ; docid += (*r - '0') ; }
	if (!docid)
		{ E.Printf("Invalid document ID") ; goto fail ; }

	m_pLog->Out("%s. Class %s, member %s, docid %d\n", __func__, *pCache->Name(), *S, docid) ;

	//	Get the document/binary
	if (pInfo->m_UserRepos != pCache->DeltaId())
		{ E.Printf("User permission failure. Err=%s\n", Err2Txt(rc)) ; goto fail ; }

	rc = pCache->Fetchbin(X, S, docid) ;
	if (rc != E_OK)
		{ E.Printf("Binary fech function failed. Err=%s\n", Err2Txt(rc)) ; goto fail ; }

	//	Serve it
	rc = pE->SendRawChain(HTTPMSG_OK, mt, X, 90, false) ;
	return ;

fail:
	S = E ;
	SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, *S) ;
	return ;
}

/*
**	Common URL request strings
*/

//	Master presets
const hzString	preset_master_proc_auth		= "/master" ;		//	Fast compare for master proc auth
//	const hzString	preset_masterResList		= "/masterResList" ;		//	Fast compare for master Resources list
//	const hzString	preset_masterVisList		= "/masterVisList" ;		//	Fast compare for master Visitors list
//	const hzString	preset_masterBanned			= "/masterBanned" ;			//	Fast compare for master Blacklist
//	const hzString	preset_masterDomain			= "/masterDomain" ;			//	Fast compare for master String list
//	const hzString	preset_masterEmaddr			= "/masterEmaddr" ;			//	Fast compare for master String list
//	const hzString	preset_masterStrFix			= "/masterStrFix" ;			//	Fast compare for master String list
//	const hzString	preset_masterStrGen			= "/masterStrGen" ;			//	Fast compare for master String list
//	const hzString	preset_masterFileList		= "/masterFileList" ;		//	Fast compare for master Config ile list
const hzString	preset_masterFileEdit		= "/masterFileEdit" ;		//	Fast compare for master edit resource page
const hzString	preset_masterFileEdit_hdl	= "/masterFileEditHdl" ;	//	Fast compare for master edit resource page
//	const hzString	preset_masterCfgList		= "/masterCfgList" ;		//	Fast compare for master list resource page
//	const hzString	preset_masterCfgEdit		= "/masterCfgEdit" ;		//	Fast compare for master edit resource page
const hzString	preset_masterCfgEdit_hdl1	= "/masterCfgEditHdl1" ;	//	Fast compare for master edit resource page
//	const hzString	preset_masterCfgEdit_hdl2	= "/masterCfgEditHdl2" ;	//	Fast compare for master edit resource page
//	const hzString	preset_masterCfgEdit_hdl3	= "/masterCfgEditHdl3" ;	//	Fast compare for master edit resource page

//	Non-master presets
const hzString	preset_usr_proc_auth	= "/usrProcAuth" ;		//	Fast compare for user proc auth
const hzString	preset_jsc				= "jsc" ;				//	Fast compare for javascript location "jsc"
const hzString	preset_js				= "js" ;				//	Fast compare for javascript location "js"
const hzString	preset_img				= "img" ;				//	Fast compare for images location "img"
const hzString	preset_userdir			= "userdir" ;			//	Fast compare for user directory keyword "userdir"
const hzString	preset_textpg			= "textpg" ;			//	Fast compare for common literal "textpg"
const hzString	preset_docs				= "docs" ;				//	Fast compare for document location "docs"
const hzString	preset_robots			= "/robots.txt" ;		//	Fast compare for common literal "/robots.txt"
const hzString	preset_favicon			= "/favicon.ico" ;		//	Fast compare for common literal "/favicon.ico"
const hzString	preset_index			= "/index.html" ;		//	Fast compare for common literal "/index.html"
const hzString	preset_sitemap_txt		= "/sitemap.txt" ;		//	Fast compare for common literal "/sitemap.txt"
const hzString	preset_sitemap_xml		= "/sitemap.xml" ;		//	Fast compare for common literal "/sitemap.xml"
const hzString	preset_siteguide		= "/siteguide.html" ;	//	Fast compare for common literal "/siteguide.html"
const hzString	gen_noref				= "no-ref" ;			//	Fast compare for common literal "no-ref"

hzEcode	hdsArticleCIF::Run	(hzHttpEvent* pE)
{
	//	Runs the C-Interface article function
	//
	//	Arguments:	1)	pE	The HTTP event pointer
	//
	//	Returns:	Enum TCP return code

	_hzfunc("hdsArticleCIF::Run") ;

	hzChain			Z ;		//	Outgoing chain

	if (!this)
		Fatal(_fn, "No C-Interface Article") ;

	m_pFunc(Z, this, pE) ;
	pE->m_Error.Printf("AJAX case 8\n") ;
	return pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 43200, false) ;
}

hzTcpCode	hdsApp::ProcHTTP	(hzHttpEvent* pE)
{
	//	Process all HTTP requests and form submissions
	//
	//	This is the core of web server operation. 
	//
	//	Arguments:	1)	pE	The HTTP event pointer
	//
	//	Returns:	Enum TCP return code

	_hzfunc("hdsApp::ProcHTTP") ;

	hzChain			Z ;					//	Outgoing chain
	hzIpConnex*		pConnex ;			//	Client Connection
	hdsProfile*		pVP ;				//	Visitor profle
	hzAtom			sv ;				//	Session value
	hdsInfo*		pInfo = 0 ;			//	Session
	hdsLang*		pLang ;				//	Language selected
	hdsLang*		pLang_tmp ;			//	For language change
	hdsResource*	pResource ;			//	Resource to respond with
	hdsFile*		pFix ;				//	Page to generate
	hdsPage*		pPage ;				//	Page to generate
	hdsCIFunc*		pCIF ;				//	C-Interface function
	hdsFormref*		pFormref ;			//	Form reference
	hdsFormhdl*		pFhdl ;				//	Form handler
	hdsTree*		pAG = 0 ;			//	Article group
	hdsArticle*		pArt = 0 ;			//	Generic article
	hdsArticleStd*	pArtStd = 0 ;		//	Standard article
	hdsArticleCIF*	pArtCIF = 0 ;		//	C-Interface article
	const char*		pReq ;				//	Requested resource
	const char*		i ;					//	Resource iterator
	hzIpinfo		ipi ;				//	IP test address (blocking)
	hzSysID			cookie ;			//	The cookie
	hzString		iplocn ;			//	IP location of client
	hzString		currRef ;			//	Referer
	hzString		reqPath ;			//	Requested page/resource
	hzString		argA ;				//	1st argument (e.g. lang subdir or article group name)
	hzString		argB ;				//	2nd argument (e.g. article name)
	hzString		hname ;				//	Form handler name
	uint32_t		n ;					//	Resource iterator limit
	uint32_t		nAgt ;				//	User agent number
	hzIpaddr		ipa ;				//	Client IP address
	hzEcode			rc ;				//	Function returns codes
	hzTcpCode		trc ;				//	TCP return code (of this function)
	char			argbuf[100] ;		//	For language codes
	hzRecep32		r32 ;				//	For USL text value

	/*
	**	Check connection is valid
	*/

	if (!this)
		Fatal(_fn, "No Dissemino Application") ;

	pConnex = pE->GetConnex() ;
	if (!pConnex)
		Fatal(_fn, "No Client Connection") ;

	/*
	**	Check request is valid
	*/

	rc = E_FORMAT ;

	switch	(pE->Method())
	{
	case HTTP_GET:		rc = E_OK ;
						pConnex->m_Track.Printf("START ProcHTTP (GET) %s\n", pE->Hostname()) ;
						break ;

	case HTTP_HEAD:		rc = E_OK ;
						pConnex->m_Track.Printf("START ProcHTTP (HEAD) %s\n", pE->Hostname()) ;
						break ;

	case HTTP_POST:		rc = E_OK ;
						pConnex->m_Track.Printf("START ProcHTTP (POST) %s\n", pE->Hostname()) ;
						break ;

	case HTTP_OPTIONS:	pConnex->m_Track.Printf("START ProcHTTP (OPTIONS) %s\n", pE->Hostname()) ;	break ;
	case HTTP_PUT:		pConnex->m_Track.Printf("START ProcHTTP (PUT) %s\n", pE->Hostname()) ;		break ;
	case HTTP_DELETE:	pConnex->m_Track.Printf("START ProcHTTP (DELETE) %s\n", pE->Hostname()) ;	break ;
	case HTTP_TRACE:	pConnex->m_Track.Printf("START ProcHTTP (TRACE) %s\n", pE->Hostname()) ;	break ;
	case HTTP_CONNECT:	pConnex->m_Track.Printf("START ProcHTTP (CONNECT) %s\n", pE->Hostname()) ;	break ;
	case HTTP_INVALID:	pConnex->m_Track.Printf("START ProcHTTP (INVALID) %s\n", pE->Hostname()) ;	break ;
	}

	/*
	**	Check IP address is not blocked, determine location and if a user session applies
	*/

	ipa = pE->ClientIP() ;
	if (ipa == IPADDR_LOCAL || ipa == IPADDR_NULL || ipa == IPADDR_BAD)
	{
		SendErrorPage(pE, HTTPMSG_INTERNAL_SERVER_ERROR, __func__, "Could not dechiper IP address\n") ;
		return TCP_TERMINATE ;
	}

	if (_hzGlobal_StatusIP.Exists(ipa))
	{
		//	Banned IP? Kill connection
		ipi = _hzGlobal_StatusIP[ipa] ;

		if (ipi.m_bInfo & HZ_IPSTATUS_BLACK_HTTP)
		{
			pConnex->m_Track.Printf("BLOCKED IP ADDRESS %s - Killing Connection\n", *ipa.Str()) ;
			pConnex->SendKill() ;
			return TCP_TERMINATE ;
		}
	}

	if (rc != E_OK)
	{
		pConnex->m_Track.Printf("BLOCKING IP ADDRESS %s - Killing Connection\n", *ipa.Str()) ;
		SetStatusIP(ipa, HZ_IPSTATUS_BLACK_PROT, 9000) ;
		pConnex->SendKill() ;
		return TCP_TERMINATE ;
	}

	iplocn = GetIpLocation(ipa) ;
	pE->m_pContextLang = pLang = m_pDfltLang ;

	//	Get keep-alive status, requested resource, cookie etc
	trc = pE->Connection() ? TCP_KEEPALIVE : TCP_TERMINATE ;

	pReq = pE->GetResource() ;
	currRef = pE->Referer() ;

	if (!pE->Cookie())
		pE->m_Report.Printf("Supplied Cookie: [0] info 0\n") ;
	else
	{
		//	There is a cookie so get the session
		pInfo = m_SessCookie[pE->Cookie()] ;
		pE->m_Report.Printf("Supplied Cookie: [%016X] info %p\n", pE->Cookie(), pInfo) ;

		if (!pInfo)
			pE->DelSessCookie(pE->Cookie()) ;
		else
		{
			pE->SetSession(pInfo) ;
			pE->m_Report.Printf("Access:        %d\n", pInfo->m_Access) ;
			pE->m_Report.Printf("Subscriber ID: %d\n", pInfo->m_SubId) ;
			pE->m_Report.Printf("User ID:       %d\n", pInfo->m_UserId) ;
			pE->m_Report.Printf("Curr Object:   %d\n", pInfo->m_CurrObj) ;

			for (n = 0 ; n < pInfo->m_Sessvals.Count() ; n++)
			{
				pE->m_Report.Printf("Sesion var: %s=%s\n", *pInfo->m_Sessvals.GetKey(n), *pInfo->m_Sessvals.GetObj(n).Str()) ;
			}
		}

		pConnex->m_Track << pE->m_Report ;
		pE->m_Report.Clear() ;
	}

	if (pE->UserAgent())
	{
		if (m_UserAgents.Exists(pE->UserAgent()))
			nAgt = m_UserAgents[pE->UserAgent()] ;
		else
		{
			nAgt = m_UserAgents.Count() + 1 ;
			m_UserAgents.Insert(pE->UserAgent(), nAgt) ;
		}
	}

	//	Obtain basic profile on visitor from IP address
	pVP = m_Visitors[ipa] ;
	if (!pVP)
	{
		pVP = new hdsProfile() ;
		pVP->m_addr = ipa ;
		m_Visitors.Insert(ipa, pVP) ;
	}

	/*
	**	POST Method: Process commands and submissions
	*/

	if (pE->Method() == HTTP_POST)
	{
		reqPath = pReq ;

		if (!pInfo)
		{
			if (reqPath == m_LoginPost)
			{
				rc = _SubscriberAuthenticate(pE) ;
				if (rc == E_OK)
					reqPath = m_LoginAuth ;
				else
					reqPath = m_LoginFail ;
				goto proc_std_request ;
			}
		}

		if (pInfo && pInfo->m_Access & ACCESS_ADMIN)
		{
			if (reqPath == preset_masterFileEdit_hdl)	{ _masterFileEditHdl(pE) ; return trc ; }
			if (reqPath == preset_masterCfgEdit_hdl1)	{ _masterCfgEditHdl(pE) ; return trc ; }
		}

		if (reqPath == preset_master_proc_auth && !pInfo)
			{ rc = _masterProcAuth(pE) ; return trc ; }

		if (reqPath == preset_usr_proc_auth && !pInfo)
			{ rc = _SubscriberAuthenticate(pE) ; return trc ; }

		//	Obtain form reference and form handler name from the submission URL, then obtain form handler from form handler name.
		pFormref = m_FormUrl2Ref[reqPath] ;
		hname = m_FormUrl2Hdl[reqPath] ;
		pFhdl = m_FormHdls[hname] ;

		if (!pFhdl)
		{
			pVP->m_P404++ ;
			SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such form handler as %s", *reqPath) ;

			if (pVP->m_P404 > 20)
			{
				pE->m_Report.Printf("Note: Blocking IP %s for illegal POST requests\n", *ipa.Str()) ;
				SetStatusIP(ipa, HZ_IPSTATUS_BLACK_PROT, 0) ;
			}
		}
		else
		{
			pVP->m_post++ ;
			ProcForm(pE, pFormref, pFhdl) ;
		}

		pE->m_Report.Printf("\n%s POST: %s (%s, %s) ", *pE->m_Occur.Str(), *ipa.Str(), *iplocn, *m_pDfltLang->m_code) ;
		pE->m_Report.Printf("[cook=%016X info=%p] (%s) %s\n", pE->Cookie(), pInfo, *currRef, pE->GetResource()) ;
		pE->m_Report.Printf("<rbot=%d fav=%d art=%d, pg=%d scr=%d img=%d sp=%d fix=%d post=%d G404=%d P404=%d>\n",
			pVP->m_robot, pVP->m_favicon, pVP->m_art, pVP->m_page, pVP->m_scr, pVP->m_img, pVP->m_spec, pVP->m_fix, pVP->m_post, pVP->m_G404, pVP->m_P404) ;
		pConnex->m_Track << pE->m_Report ;
		pE->m_Report.Clear() ;
		return trc ;
	}

	/*
 	**	Deal with GET and HEAD method requests: All mainstrean resource requests.
 	*/

	if (pE->Method() != HTTP_GET && pE->Method() != HTTP_HEAD)
	{
		//	This can't happen!
		SendErrorPage(pE, HTTPMSG_METHOD_NOT_ALLOWED, __func__, "Method not supported") ;
		return TCP_TERMINATE ;
	}

	//	Check for robot, favicon and sitemap
	if (preset_robots == pE->GetResource())
	{
		rc = pE->SendRawString(HTTPMSG_OK, HMTYPE_TXT_PLAIN, m_Robot, 43200, false) ;
		if (rc != E_OK)
			pConnex->m_Track << pE->m_Error ;
		pVP->m_robot++ ;
		goto get_end ;
	}
	if (preset_favicon == pE->GetResource())
	{
		rc = pE->SendPageE(*m_Images, pReq + 1, 86400, false) ;
		if (rc != E_OK)
			pConnex->m_Track << pE->m_Error ;
		pVP->m_favicon++ ;
		goto get_end ;
	}
	if (preset_sitemap_txt == pE->GetResource())
	{
		pVP->m_spec++ ;
		if (!m_rawSitemapTxt.Size())
			SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such page as %s", pE->GetResource()) ;
		else
		{
			if (pE->Zipped() && m_zipSitemapTxt.Size())
				rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_PLAIN, m_zipSitemapTxt, 43200, true) ;
			else
				rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_PLAIN, m_rawSitemapTxt, 43200, false) ;
		}
		goto get_end ;
	}
	if (preset_sitemap_xml == pE->GetResource())
	{
		pVP->m_spec++ ;
		if (!m_rawSitemapTxt.Size())
			SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such page as %s", pE->GetResource()) ;
		else
		{
			if (pE->Zipped() && m_zipSitemapXml.Size())
				rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_APP_XML, m_zipSitemapXml, 43200, true) ;
			else
				rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_APP_XML, m_rawSitemapXml, 43200, false) ;
		}
		goto get_end ;
	}
	if (preset_siteguide == pE->GetResource())
	{
		pVP->m_spec++ ;
		if (!m_rawSiteguide.Size())
			SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such page as %s", pE->GetResource()) ;
		else
		{
			if (pE->Zipped() && m_zipSiteguide.Size())
				rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, m_zipSiteguide, 43200, true) ;
			else
				rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, m_rawSiteguide, 43200, false) ;
		}
		goto get_end ;
	}

	//	Check for master pages
	if (pInfo && pInfo->m_Access & ACCESS_ADMIN)
	{
		if (reqPath == "/masterLogout")			{ rc = _masterLogout(pE) ; goto get_end ; }
		if (!memcmp(pReq, "/masterAction", 13))
		{
			MasterArticle(pE) ;
			goto get_end ;
		}
	}
	/*
	if (pInfo && pInfo->m_Access & ACCESS_ADMIN)
	{
		if (pReq[1] == 'm' && pReq[2] == 'a' && pReq[3] == 's' && pReq[4] == 't' && pReq[5] == 'e' && pReq[6] == 'r')
		{
			reqPath = pReq ;	//+ 1 ;

			if (reqPath == "/masterMainMenu")	{ rc = _masterMainMenu(pE) ;	goto get_end ; }
			if (reqPath == "/masterCfgList")	{ rc = _masterCfgList(pE) ;		goto get_end ; }
			if (reqPath == "/masterCfgEdit")	{ rc = _masterCfgEdit(pE) ;		goto get_end ; }
			if (reqPath == "/masterResList")	{ rc = _masterResList(pE) ;		goto get_end ; }
			if (reqPath == "/masterVisList")	{ rc = _masterVisList(pE) ;		goto get_end ; }
			if (reqPath == "/masterDomain")		{ rc = _masterDomain(pE) ;		goto get_end ; }
			if (reqPath == "/masterEmaddr")		{ rc = _masterEmaddr(pE) ;		goto get_end ; }
			if (reqPath == "/masterStrFix")		{ rc = _masterStrFix(pE) ;		goto get_end ; }
			if (reqPath == "/masterStrGen")		{ rc = _masterStrGen(pE) ;		goto get_end ; }
			if (reqPath == "/masterBanned")		{ rc = _masterBanned(pE) ;		goto get_end ; }
			if (reqPath == "/masterMemstat")	{ rc = _masterMemstat(pE) ;		goto get_end ; }
			if (reqPath == "/masterUSL")		{ rc = _masterUSL(pE) ;			goto get_end ; }
			if (reqPath == "/masterFileList")	{ rc = _masterFileList(pE) ;	goto get_end ; }
			if (reqPath == "/masterFileEdit")	{ rc = _masterFileEdit(pE) ;	goto get_end ; }
			if (reqPath == "/masterDataModel")	{ rc = _masterDataModel(pE) ;	goto get_end ; }
			if (reqPath == "/masterCfgRestart")	{ rc = _masterCfgRestart(pE) ;	goto get_end ; }
			if (reqPath == "/masterLogout")		{ rc = _masterLogout(pE) ;		goto get_end ; }
		}

		//	Master article - should this not simply be get article???
		if (!memcmp(pReq, "/master_", 8))
			{ MasterArticle(pE) ; goto get_end ; }
	}
	*/

	/*
 	**	Check for AJAX requests. These will not be the only source of requests in which there is one name-value pair submitted in the query part of the request URL, but requests to
	**	check a member value for availabilty (e.g. email address) and requests for articles are of this form. If the request is not for any of these, then drop through. 
 	*/

	if (pE->QueryLen() && pE->m_mapStrings.Count() == 1)
	{
		//	Check for an in-page value query on a class member
		if (!memcmp(pReq, "/ck-", 4))
		{
			InPageQuery(pE) ;
			goto get_end ;
		}

		//	What follows must be an article request of the form "/article_group-article_name"
		argA = pE->m_mapStrings.GetKey(0) ;
		argB = pE->m_mapStrings.GetObj(0) ;

		//	Check if the article group is public
		pAG = m_ArticleGroups[argA] ;
		if (!pAG)
		{
			//	Check if the request is for an article in a tree vested with the user session
			if (pInfo && pInfo->m_pTree && pInfo->m_pTree->m_Groupname == argA)
				pAG = pInfo->m_pTree ;
		}

		if (pAG)
			pArt = (hdsArticle*) pAG->Item(argB) ;
		else
			pE->m_Error.Printf("Cmd: Could not locate group %s for article %s\n", *argA, *argB) ;

		if (!pArt)
			pE->SendAjaxResult(HTTPMSG_NOTFOUND, "Out of Range") ;
		else
		{
			if (pArt->m_Access == ACCESS_PUBLIC
				|| (pArt->m_Access == ACCESS_NOBODY && (!pInfo || !(pInfo->m_Access & ACCESS_MASK)))
				|| (pInfo && (pInfo->m_Access & ACCESS_ADMIN || (pInfo->m_Access & ACCESS_MASK) & pArt->m_Access)))
			{
				pVP->m_art++ ;

				pArtStd = dynamic_cast<hdsArticleStd*>(pArt) ;
				if (pArtStd)
				{
					//	Standard article
					pE->SetHdr(s_articleTitle, pArt->m_Title) ;

					if (!pArtStd->m_USL)
					{
						if (pArtStd->m_Content)
						{
							Z = pArtStd->m_Content ;
							rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 43200, false) ;
						}
						else
							pE->SendAjaxResult(HTTPMSG_NOCONTENT, "Article is a heading only") ;
					}
					else
					{
						if (pArtStd->m_flagVE & VE_ACTIVE)
						{
							pArtStd->Generate(Z, pE) ;

							if (!Z.Size())
							{
								pE->m_Error.Printf("AJAX case 2\n") ;
								pE->SendAjaxResult(HTTPMSG_NOCONTENT, "Article is a heading only") ;
							}
							else
							{
								if (pE->Zipped())
									rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 0, true) ;
								else
									rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 0, false) ;
							}
						}
						else
						{
							if (pE->Zipped())
							{
								Z = pLang->m_zipItems[pArtStd->m_USL] ;

								if (Z.Size())
									rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 43200, true) ;
								else
								{
									pE->m_Error.Printf("Cmd: Could not locate article zip data %s\n", pArtStd->m_USL.Txt(r32)) ;
									pE->SendAjaxResult(HTTPMSG_NOCONTENT, "Article is a heading only") ;
								}
							}
							else
							{
								Z = pLang->m_rawItems[pArtStd->m_USL] ;

								if (Z.Size())
									rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 43200, false) ;
								else
								{
									pE->m_Error.Printf("Cmd: Could not locate article raw data %s\n", pArtStd->m_USL.Txt(r32)) ;
									pE->SendAjaxResult(HTTPMSG_NOCONTENT, "Article is a heading only") ;
								}
							}
						}
					}
				}

				pArtCIF = dynamic_cast<hdsArticleCIF*>(pArt) ;
				if (pArtCIF)
				{
					//	C-Interface article
					rc = pArtCIF->Run(pE) ;
				}
			}
			else
				pE->SendAjaxResult(HTTPMSG_FORBIDDEN, "No Access") ;
		}

		if (rc != E_OK || pE->m_Error.Size())
		{
			if (pE->m_Error.Size())
				pConnex->m_Track << pE->m_Error ;
			trc = TCP_TERMINATE ;
		}

		goto get_end ;
	}

	//	Capture any sub-directory directives
	if (pReq[1])
	{
		for (n = 0, i = pReq + 1 ; *i && n < 8 ; i++, n++)
		{
			argbuf[n] = *i ;

			if (*i == CHAR_FWSLASH)
			{
				//	We have a subdir directive. If this names a language, change the language and recheck if there is another subdir directive. Otherwise break to
				//	leave the directive for later interpretation.

				argbuf[n] = 0 ;
				argA = argbuf ;

				pLang_tmp = m_Languages[argA] ;
				if (!pLang_tmp)
					break ;
		
				pReq = i ;
				pE->m_pContextLang = pLang = pLang_tmp ;
				argA = (char*)0 ;
				n = -1 ;
			}
		}

		//	Handle requests for one of the scripts not embedded within a page
		if (argA)
		{
			if (argA == preset_jsc || argA == preset_js)
			{
				if (argA == preset_jsc)
					reqPath = pReq + preset_jsc.Length() + 2 ;
				else
					reqPath = pReq + preset_js.Length() + 2 ;

				pConnex->m_Track.Printf("Serving script [%s]\n", *reqPath) ;

				if (m_rawScripts.Exists(reqPath))
				{
					//	Try the global non-language dependent scripts first

					pVP->m_scr++ ;
					if (pE->Method() == HTTP_HEAD)
						pE->SendHttpHead(m_rawScripts[reqPath], HMTYPE_TXT_JS, 86400) ;
					else
					{
						if (pE->Zipped())
						{
							Z = m_zipScripts[reqPath] ;
							rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_JS, Z, 86400, true) ;
							if (rc != E_OK)
								pConnex->m_Track << pE->m_Error ;
						}
						else
						{
							Z = m_rawScripts[reqPath] ;
							rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_JS, Z, 86400, false) ;
							if (rc != E_OK)
							{
								pConnex->m_Track << pE->m_Error ;
								//m_pLog->Out("[\n") ;
								//m_pLog->Out(m_rawScripts[reqPath]) ;
								//m_pLog->Out("\n]\n") ;
							}
						}
					}
					goto get_end ;
				}

				if (pLang->m_rawScripts.Exists(reqPath))
				{
					//	Try the language dependent scripts

					pVP->m_scr++ ;
					if (pE->Method() == HTTP_HEAD)
						pE->SendHttpHead(pLang->m_rawScripts[reqPath], HMTYPE_TXT_JS, 86400) ;
					else
					{
						if (pE->Zipped())
						{
							Z = pLang->m_zipScripts[reqPath] ;
							rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_JS, Z, 86400, true) ;
							if (rc != E_OK)
								pConnex->m_Track << pE->m_Error ;
						}
						else
						{
							Z = pLang->m_rawScripts[reqPath] ;
							rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_JS, Z, 86400, false) ;
							if (rc != E_OK)
								pConnex->m_Track << pE->m_Error ;
								//{ m_pLog->Out(pE->m_Error) ; m_pLog->Out("[\n") ; m_pLog->Out(pLang->m_rawScripts[reqPath]) ; m_pLog->Out("\n]\n") ; }
						}
					}
					goto get_end ;
				}

				SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such script as %s (%s)", *reqPath, pE->GetResource()) ;
				goto get_end ;
			}

			//	Handle requests for image files (as opposed to images in a repository)
			if (argA == preset_img)
			{
				pVP->m_img++ ;
				pConnex->m_Track.Printf("Serving image [%s]\n", pReq + 5) ;

				if (pE->Method() == HTTP_HEAD)
					pE->SendFileHead(*m_Images, pReq + 5) ;
				else
					pE->SendPageE(*m_Images, pReq + 5, 86400, false) ;
				goto get_end ;
			}

			//	Send special text pages
			if (argA == preset_textpg)
			{
				pVP->m_spec++ ;
				reqPath = pReq + preset_textpg.Length() + 1 ;
				pConnex->m_Track.Printf("Serving textpg [%s]\n", *reqPath) ;
				pE->SendPageE(*m_Docroot, pReq, 43200, false) ;
				goto get_end ;
			}

			//	Handle document/binary requests
			if (argA == preset_docs)
			{
				pVP->m_spec++ ;
				SendDocument(pE) ;
				goto get_end ;
			}

			//	Problem now is argA is not a js or an img, what is it?
		}

		//	Handle request for the stylesheet
		if (m_namCSS == pReq+1)
		{
			pVP->m_scr++ ;
			if (pE->Method() == HTTP_HEAD)
				pE->SendHttpHead(m_txtCSS, HMTYPE_TXT_CSS, 86400) ;
			else
			{
				if (pE->Zipped())
					rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_CSS, m_zipCSS, 86400, true) ;
				else
					rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_CSS, m_txtCSS, 86400, false) ;
				if (rc != E_OK)
					pConnex->m_Track << pE->m_Error ;
			}
			goto get_end ;
		}

		if (pReq[1] == 'u' && !memcmp(pReq, "/userdir", 8))
		{
			//	Handle user-specific files
			if (!pInfo)
			{
				SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such page as %s (no-session-info)", pE->GetResource()) ;
				goto get_end ;
			}

			sv = pInfo->m_Sessvals[preset_userdir] ;
			if (sv.IsNull())
				SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such page as %s", pE->GetResource()) ;

			pVP->m_spec++ ;
			reqPath = m_Docroot + sv.Str() ;
			reqPath = ConvertText(reqPath, pE) ;

			pConnex->m_Track.Printf("Serving userfile [%s/%s]\n", *reqPath, pReq + preset_userdir.Length() + 2) ;
			rc = pE->SendFilePage(*reqPath, pReq + preset_userdir.Length() + 2, 0, false) ;
			if (rc != E_OK)
				SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such page as %s", pE->GetResource()) ;
			goto get_end ;
		}

		//	Process the request
		if (pReq[1] == 'm')
		{
			if (!memcmp(pReq, "/memstats", 10))
			{
				Z <<
				"<html>\n"
				"<head>\n"
				"<style>\n"
				".main  { text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000; }\n"
				".stdpg { height:600px; border:0px; margin-left:5px; overflow-x:auto; overflow-y:auto; }\n"
				"</style>\n"
				"</head>\n"
				"<body>\n" ;

				ReportMemoryUsage(Z) ;

				Z <<
				"</body>\n"
				"</html>\n" ;

				pE->SendAjaxResult(HTTPMSG_OK, Z) ;
				goto get_end ;
			}

			if (!memcmp(pReq, "/mutexes", 8))
				{ ReportMutexContention(Z) ; pE->SendAjaxResult(HTTPMSG_OK, Z) ; goto get_end ; }
		}
	}

	reqPath = pReq ;
	if (pReq[0] == CHAR_PERCENT)
	{
		//	Does this ever happen?
		reqPath = ConvertText(reqPath, pE) ;
	}

	if (!reqPath)
		reqPath = "/" ;

	if (reqPath.Length() > 1)
	{
		if (reqPath[reqPath.Length()-1] == CHAR_FWSLASH)
			reqPath.Truncate(reqPath.Length()-1) ;
	}

	if (reqPath == m_LogoutURL)
	{
		pVP->m_spec++ ;

		if (pInfo)
		{
			//	pInfo->m_Access &= ACCESS_ADMIN ;
			pConnex->m_Track.Printf("%s. Deleting cookie %016x\n", *_fn, pE->Cookie()) ;

			m_SessCookie.Delete(pE->Cookie()) ;
			pE->SetSession(0) ;
			pConnex->m_Track.Printf("%s. Deleting cookie %016x\n", *_fn, pE->Cookie()) ;
		}
	}

	//	See if requested resource matches any C-Interface functions

	/*
	pCIF = m_AltFuncs[reqPath] ;
	if (pCIF)
	{
		pConnex->m_Track.Printf("%s. Serving C-Interface function %s\n", *_fn, *reqPath) ;
		rc = pCIF->m_pFunc(pE) ;
		pConnex->m_Track.Printf("%s. Done C-Interface returned %s\n", *_fn, Err2Txt(rc)) ;
		goto get_end ;
	}

	pFix = m_Fixed[reqPath] ;
	if (pFix)
	{
		pConnex->m_Track.Printf("Serving fixed page [%s]\n", *reqPath) ;
		pVP->m_fix++ ;
		if (pE->Zipped() && pFix->m_zipValue.Size())
			rc = pE->SendRawChain(HTTPMSG_OK, pFix->m_Mimetype, pFix->m_zipValue, 86400, true) ;
		else
			rc = pE->SendRawChain(HTTPMSG_OK, pFix->m_Mimetype, pFix->m_rawValue, 86400, false) ;

		if (rc != E_OK)
			pConnex->m_Track << pE->m_Error ;
			//{ m_pLog->Out("[") ; m_pLog->Out(pE->m_Error) ; m_pLog->Out("]\n") ; }
		goto get_end ;
	}
	*/

proc_std_request:
	pResource = m_ResourcesPath[reqPath] ;
	pConnex->m_Track.Printf("Got resource %p from request %s\n", pResource, *reqPath) ;

	if (!pResource)
	{
		if (reqPath == "/")
			//pResource = m_PagesPath[preset_index] ;
			pResource = m_ResourcesPath[preset_index] ;
	}

	if (!pResource)
	{
		//pConnex->m_Track.Printf("No page case 2: Serving page with arg [%s]\n", *reqPath) ;

		if (!pInfo && m_MasterPath == pReq)
		{
			//	Supply login page
			rc = _masterLoginPage(pE) ;
			goto get_end ;
		}

		//	The page does not exist within the mormal Dissemino regime but it may exist as a page relative to the document root. If so and the flag for this is
		//	set, then stat and serve

		if (m_OpFlags & DS_APP_NORMFILE)
		{
			argB = m_Docroot + reqPath ;
			rc = TestFile(*argB) ;
			if (rc != E_OK)
				pConnex->m_Track.Printf("Cannot find normal page [%s]\n", *argB) ;
			else
			{
				pConnex->m_Track.Printf("Serving normal page [%s]\n", *argB) ;
				pE->SendFilePage(*m_Docroot, *reqPath, 0, false) ;
				goto get_end ;
			}
		}

		pConnex->m_Track.Printf("Non-exist page [%s]\n", *reqPath) ;
		pVP->m_G404++ ;
		if (pVP->m_G404 > 1000 || (!pVP->m_art && pVP->m_page && pVP->m_G404 > 20))
		{
			pE->m_Report.Printf("Note: Blocking IP %s for invalid GET requests\n", *ipa.Str()) ;
			SetStatusIP(ipa, HZ_IPSTATUS_BLACK_PROT, 9000) ;
		}

		if (pVP->m_robot)
			SendErrorPage(pE, HTTPMSG_GONE, __func__, "Page %s no longer exists", pE->GetResource()) ;
		else
			SendErrorPage(pE, HTTPMSG_NOTFOUND, __func__, "No such page as %s (info %p)", pE->GetResource(), pInfo) ;

		//pConnex->m_Track.Printf("Served non-exist page [%s]\n", *reqPath) ;
	}
	else
	{
		pVP->m_page++ ;

		if (pResource->m_Access == ACCESS_PUBLIC
			|| (pResource->m_Access == ACCESS_NOBODY && (!pInfo || !(pInfo->m_Access & ACCESS_MASK)))
			|| (pInfo && (pInfo->m_Access & ACCESS_ADMIN || (pInfo->m_Access & ACCESS_MASK) & pResource->m_Access)))
		{
			if ((pPage = dynamic_cast<hdsPage*>(pResource)))
			{
				pConnex->m_Track.Printf("Serving page [%s] (%s)\n", *reqPath, *pPage->m_Title) ;

				if (pE->Method() == HTTP_HEAD)
					pPage->Head(pE) ;
				else
				{
					pPage->m_HitCount++ ;

					if (pE->m_Resarg || pPage->m_flagVE & VE_ACTIVE)
					{
						pPage->Display(pE) ;
					}
					else
					{
						if (pE->Zipped())
						{
							Z = pLang->m_zipItems[pPage->m_USL] ;

							if (Z.Size())
								rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 43200, true) ;
							else
								pConnex->m_Track.Printf("Cmd: Could not locate page zip data %s\n", pPage->m_USL.Txt(r32)) ;
						}
						else
						{
							Z = pLang->m_rawItems[pPage->m_USL] ;

							if (Z.Size())
								rc = pE->SendRawChain(HTTPMSG_OK, HMTYPE_TXT_HTML, Z, 43200, false) ;
							else
								pConnex->m_Track.Printf("Cmd: Could not locate page raw data %s\n", pPage->m_USL.Txt(r32)) ;
						}
					}
				}
			}

			if ((pFix = dynamic_cast<hdsFile*>(pResource)))
			{
				pConnex->m_Track.Printf("Serving fixed page [%s]\n", *reqPath) ;
				pVP->m_fix++ ;
				if (pE->Zipped() && pFix->m_zipValue.Size())
					rc = pE->SendRawChain(HTTPMSG_OK, pFix->m_Mimetype, pFix->m_zipValue, 86400, true) ;
				else
					rc = pE->SendRawChain(HTTPMSG_OK, pFix->m_Mimetype, pFix->m_rawValue, 86400, false) ;

				if (rc != E_OK)
					pConnex->m_Track << pE->m_Error ;
					//{ m_pLog->Out("[") ; m_pLog->Out(pE->m_Error) ; m_pLog->Out("]\n") ; }
					//goto get_end ;
			}

			if ((pCIF = dynamic_cast<hdsCIFunc*>(pResource)))
			{
				pCIF = dynamic_cast<hdsCIFunc*>(pResource) ;
				if (pCIF)
				{
					pConnex->m_Track.Printf("Serving C-Interface Function [%s]\n", *reqPath) ;
					pCIF->m_pFunc(pE) ;
				}
				else
					pConnex->m_Track.Printf("NOT Serving C-Interface Function [%s]\n", *reqPath) ;
				//m_pLog->Out("Serving C-Interface Function [%s] (%s)\n", *reqPath, *pPage->m_Title) ;
			}
		}
		else
			SendErrorPage(pE, HTTPMSG_FORBIDDEN, __func__, "No access to page %s", pE->GetResource()) ;
	}

get_end:
	if (pE->m_Report.Size())
	{
		pConnex->m_Track << pE->m_Report ;
		pE->m_Report.Clear() ;
	}

	pE->m_Report.Printf("%s ev=%d sk=%d ", *pE->m_Occur.Str(), pE->EventNo(), pE->CliSocket()) ;

	if (pE->Connection())
		pE->m_Report << "ka " ;
	else
		pE->m_Report << "cl " ;

	pE->m_Report.Printf("<bot=%d art=%d pg=%d scr=%d img=%d sp=%d fix=%d post=%d G404=%d P404=%d> ",
		pVP->m_robot, pVP->m_art, pVP->m_page, pVP->m_scr, pVP->m_img, pVP->m_spec, pVP->m_fix, pVP->m_post, pVP->m_G404, pVP->m_P404) ;

	if (pE->Method() == HTTP_HEAD)
		pE->m_Report << "HED: " ;
	else
		pE->m_Report << "GET: " ;

	pE->m_Report.Printf("%s lang %s %s vtotal=%u ", *iplocn, *pLang->m_code, *ipa.Str(), m_Visitors.Count()) ;

	if (pInfo)
		pE->m_Report.Printf("[cook=%016X info=%p] (%s) %s\n", pE->Cookie(), pInfo, *currRef, pE->GetResource()) ;
	else
		pE->m_Report.Printf("[cook=%016X info=0] (%s) %s\n", pE->Cookie(), *currRef, pE->GetResource()) ;
	pConnex->m_Track << pE->m_Report ;
	return trc ;
}

void	hdsApp::Shutdown	(void)
{
	//	Shutsdown Dissemino Application. Deallocates all allocated resources before desconstruction
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hdsApp::Shutdown") ;

	hdsArticle*		pArt ;			//  All articles
	hdsFormdef*		pForm ;			//  All known forms
	hdsFormhdl*		pFormhdl ;		//  All known form handlers
	hdsFormref*		pFormref ;		//  All known form handlers
	hdsBlock*		pBlock ;		//  All known forms
	hdsTree*		pAG ;			//  All article groups
	hdsResource*	pPage ;			//  All known pages by path
	hdsInfo*		pInfo ;			//	All sessions

	uint32_t	n ;		//	Iterator
	uint32_t	x ;		//	Iterator

	for (n = 0 ; n < m_FormDefs.Count() ; n++)
		{ pForm = m_FormDefs.GetObj(n) ; delete pForm ; }
	m_FormDefs.Clear() ;

	for (n = 0 ; n < m_FormHdls.Count() ; n++)
		{ pFormhdl = m_FormHdls.GetObj(n) ; delete pFormhdl ; }
	m_FormHdls.Clear() ;

	for (n = 0 ; n < m_FormUrl2Ref.Count() ; n++)
		{ pFormref = m_FormUrl2Ref.GetObj(n) ; delete pFormref ; }
	m_FormUrl2Ref.Clear() ;

	for (n = 0 ; n < m_Includes.Count() ; n++)
		{ pBlock = m_Includes.GetObj(n) ; delete pBlock ; }
	printf("Cleared %d Includes\n", m_Includes.Count()) ;
	fflush(stdout) ;
	m_Includes.Clear() ;

	for (n = 0 ; n < m_ArticleGroups.Count() ; n++)
	{
		pAG = m_ArticleGroups.GetObj(n) ;
		if (!pAG)
			continue ;

		for (x = 0 ; x < pAG->Count() ; x++)
		{
			pArt = (hdsArticle*) pAG->Item(x) ;
			delete pArt ;
		}
		printf("Cleared %d Articles\n", pAG->Count()) ;
		fflush(stdout) ;
		pAG->Clear() ;
	}

	//for (n = 0 ; n < m_Fixed.Count() ; n++)
	//	{ pFix = m_Fixed.GetObj(n) ; delete pFix ; }
	//printf("Cleared %d Fixed resources\n", m_Fixed.Count()) ;
	//fflush(stdout) ;
	//m_Fixed.Clear() ;

	for (n = 0 ; n < m_ResourcesPath.Count() ; n++)
		{ pPage = m_ResourcesPath.GetObj(n) ; delete pPage ; }
	printf("Cleared %d Pages\n", m_ResourcesPath.Count()) ;
	fflush(stdout) ;
	m_ResourcesPath.Clear() ;
	m_ResourcesName.Clear() ;

	for (n = 0 ; n < m_Responses.Count() ; n++)
		{ pPage = m_Responses.GetObj(n) ; delete pPage ; }
	printf("Cleared %d Formhdl response pages\n", m_Responses.Count()) ;
	fflush(stdout) ;
	m_Responses.Clear() ;

	for (n = 0 ; n < m_SessCookie.Count() ; n++)
		{ pInfo = m_SessCookie.GetObj(n) ; delete pInfo ; }
	printf("Cleared %d Sessions\n", m_SessCookie.Count()) ;
	fflush(stdout) ;
	m_SessCookie.Clear() ;

	printf("Cleared %d Styles\n", m_Styles.Count()) ;
	fflush(stdout) ;
	m_Styles.Clear() ;

	printf("Cleared %d Passives\n", m_Passives.Count()) ;
	fflush(stdout) ;
	m_Passives.Clear() ;

	printf("Cleared All\n") ;
	fflush(stdout) ;
}
