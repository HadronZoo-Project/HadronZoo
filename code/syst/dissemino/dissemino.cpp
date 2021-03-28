//
//	File:	dissemino.cpp
//
//	Desc:	The main file for the Dissemino Low Latency Website Engine.
//
//	Legal Notice: This file is part of the HadronZoo::Dissemino program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	HadronZoo::Dissemino is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
//	Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//	as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//	HadronZoo::Dissemino is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with HadronZoo::Dissemino. If not, see http://www.gnu.org/licenses.
//

#include <iostream>
#include <fstream>

using namespace std ;

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>

#include "hzBasedefs.h"
#include "hzDissemino.h"
#include "hzDelta.h"
#include "hzProcess.h"

/*
**	Variables
*/

bool	_hzGlobal_XM = false ;			//	Using/not using operator new/delete override
bool	_hzGlobal_MT = true ;			//	Program is multi-threaded

global	hzProcess		proc ;			//	hzProcess instance (all Hadronzoo based applications require this)
global	hzLogger		slog ;			//	Error logging for server threads
global	hzLogger		schlog ;		//	Error logging for schedule thread
global	hzLogger		svrstats ;		//	Server stats (duration of connections)

global	hzIpServer*	theServer ;			//	HadronZoo server regime

static	hzString	g_Version = "2.0" ;	//	Version 2.0
static	hzString	fileHits ;			//	Hits file
static	hzString	fileLogs ;			//	Logfile
static	hzString	fileStats ;			//	Server stats file
static	hzString	fileLang ;			//	Language support file (all strings in all languages)
static	bool		s_bExportStrings ;	//	Export language support file (in default language)
static	bool		s_bExportForms ;	//	Export language support file (in default language)
static	bool		s_bNewData ;		//	Import new data as directed by <initstate> tag.
static	bool		s_bMonitor ;		//	Monitor config files for change (default is OFF)

/*
**	Functions
*/

void*	ThreadServeRequests	(void* pVoid)
{
	hzProcess	_procA ;

	theServer->ServeRequests() ;

	pthread_exit(pVoid) ;
	return pVoid ;
}

void*	ThreadServeResponses	(void* pVoid)
{
	hzProcess	_procB ;

	theServer->ServeResponses() ;

	pthread_exit(pVoid) ;
	return pVoid ;
}

#if 0
void	ExportForms	(hdsApp* pApp)
{
	//	Export data classes found in the configs as XML fragments that define default forms and form-handlers. This facility is provided as a development aid in
	//	order to save time. The output is to file name "classforms.def" in the current directory which should always be the development config directory for the
	//	website. The facility is invoked by calling Dissemino with "-forms" supplied as an argument. Dissemino will load the configs, write the classforms.def
	//	file and then exit.
	//
	//	There will be one form and one form-handler generated for each class found in the configs, including those for which form(s) currently exists. Forms in
	//	HTML cannot be nested so in cases where class members are composite (have a data type which is another class), the form produced will contain an extra
	//	button for each composite member. The class which a member has as its data type will have to have been declare beforehand (otherwise the configs cannot
	//	be read in), and this will have a form and form handler. The ...

	_hzfunc(__func__) ;

	const hdsFldspec*	pSpec ;		//	Field spec
	const hdbClass*		pClass ;	//	Data class pointer
	const hdbMember*	pMem ;		//	Data class member pointer

	ofstream		os ;			//	Output file
	hzChain			Z ;				//	Output chain
	hdsBlock*		pIncl ;			//	Pointer to default include block
	hzString		dfltInclude ;	//	Use a <xinclude> block in pages?
	const char*		cname ;			//	Class name
	const char*		mname ;			//	Member name
	const char*		sname ;			//	Fldspec reference name
	uint32_t		nC ;			//	Data class iterator
	uint32_t		nM ;			//	Data class member iterator
	bool			bUser ;			//	True is class is a user class
	char			fname[80] ;		//	Filename
	char			pebuf[4] ;		//	Percent entity form

	slog.Out("%s. Have %d classes and %d repositories\n", *_fn, pApp->m_ADP.CountDataClass(), pApp->m_ADP.CountObjRepos()) ;

	pebuf[0] = CHAR_PERCENT ;
	pebuf[1] = 's' ;
	pebuf[2] = ':' ;
	pebuf[3] = 0 ;

	if (pApp->m_Includes.Count())
	{
		pIncl = pApp->m_Includes.GetObj(0) ;
		dfltInclude = pIncl->m_Refname ;
	}

	for (nC = 0 ; nC < classes.Count() ; nC++)
	{
		cname = classes.GetObj(nC) ;
		pClass = pApp->m_ADP.GetPureClass(cname) ;

		if (!pClass)
		{
			slog.Out("No such class as %s\n", *cname) ;
			continue ;
		}

		if (pClass->StrTypename() == "subscriber")
			{ slog.Out("%s Ignoring subscriber class\n", *_fn) ; continue ; }

		slog.Out("%s. Doing class %s with %d members\n", *_fn, cname, pClass->MbrCount()) ;

		bUser = false ;
		if (pApp->m_UserTypes.Exists(cname))
			bUser = true ;

		//	Start the XML output
		Z << "<disseminoInclude>\n" ;

		/*
		**	Write out a basic list page for the class
		*/

		Z.Printf("<xpage path=\"/list_%s\" subject=\"%s\" title=\"List %s\" access=\"any\" bgcolor=\"eeeeee\" ops=\"noindex\">\n",
			cname, *pClass->m_Category, cname) ;
		Z << "\t<desc>None</desc>\n" ;
		if (dfltInclude)
			Z.Printf("\t<xblock name=\"%s\"/>\n", *dfltInclude) ;
		Z.AddByte(CHAR_NL) ;

		Z.Printf("\t<xtable repos=\"%s\" criteria=\"all\" rows=\"10000\" width=\"90\" height=\"500\" css=\"main\">\n", cname) ;
		Z <<
		"\t\t<ifnone><p>Sorry no records ...</p></ifnone>\n"
		"\t\t<header>Table header ...</header>\n"
		"\t\t<footer>Table footer ...</footer>\n" ;

		for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
		{
			pMem = pClass->GetMember(nM) ;
			mname = pMem->TxtName() ;

			Z.Printf("\t\t<xcol member=\"%s\" title=\"%s\"/>\n", mname, mname) ;
		}

		Z << "\t</xtable>\n" ;
		Z << "</xpage>\n" ;

		/*
		**	Write out a basic host page for the form for the class
		*/

		Z.Printf("<xpage path=\"/add_%s\" subject=\"%s\" title=\"Add %s\" access=\"any\" bgcolor=\"eeeeee\" ops=\"noindex\">\n",
			cname, *pClass->m_Category, cname) ;
		Z << "\t<desc>None</desc>\n" ;
		if (dfltInclude)
			Z.Printf("\t<xblock name=\"%s\"/>\n", *dfltInclude) ;
		Z.AddByte(CHAR_NL) ;

		Z <<
		"<style>\n"
		".tooltip { position: relative; display: inline-block; border-bottom: 1px dotted black; }\n"
		".tooltip .tooltiptext { visibility: hidden; width: 120px; background-color: black; color: #fff; text-align: center; padding: 5px 0; border-radius: 6px; position: absolute; z-index: 1; }\n"
		".tooltip:hover .tooltiptext { visibility: visible; }\n"
		"</style>\n\n" ;

		Z <<
		"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
		"\t\t<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"\t\t<tr><td class=\"title\" align=\"center\">Title: Add a " << cname << "</td></tr>\n"
		"\t\t<tr>\n"
		"\t\t<td valign=\"top\" align=\"center\" class=\"main\">\n"
		"\t\t<p>\n"
		"\t\tUse this form to create an instance of " << cname << "\n"
		"\t\t</p>\n"
		"\t\t</td>\n"
		"\t</tr>\n"
		"\t<xdiv user=\"public\">\n"
		"\t<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"\t<tr>\n"
		"\t\t<td height=\"20\" align=\"center\" class=\"main\">\n"
		"\t\tPlease be aware that by submitting this form, you are consenting to the use of cookies. Please read our cookie policy to <a href=\"/cookies\">find out more</a>\n"
		"\t\t</td>\n"
		"\t</tr>\n"
		"\t</xdiv>\n"
		"\t<tr><td height=\"40\">&nbsp;</td></tr>\n"
		"\t</table>\n\n" ;

		//	Now write out the form itself
		if (bUser)
			Z.Printf("\t<xform name=\"form_reg_%s\" action=\"fhdl_phase1_%s\" class=\"%s\">\n", cname, cname, cname) ;
		else
			Z.Printf("\t<xform name=\"form_add_%s\" action=\"fhdl_add_%s\" class=\"%s\">\n", cname, cname, cname) ;
		Z << "\t<xhide name=\"lastpage\" value=\"%x:referer;\"/>\n\n" ;

		Z << "\t<table width=\"56%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

		for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
		{
			pMem = pClass->GetMember(nM) ;
			mname = pMem->TxtName() ;

			pSpec = pMem->GetSpec() ;

			if (!pSpec)
				{ slog.Out("%s. Warning No Field specification for member %s\n", *_fn, mname) ; continue ; }

			if (pSpec->m_Desc)
				Z.Printf("\t\t<tr height=\"18\"><td class=\"fld\"><div class=\"tooltip\">%s<span class=\"tooltiptext\">%s</span></div>:</td>\t<td><xfield member=\"%s\"",
					*pSpec->m_Title, *pSpec->m_Desc, mname) ;
			else
				Z.Printf("\t\t<tr height=\"18\"><td class=\"fld\">%s:</td>\t<td><xfield member=\"%s\"", *pSpec->m_Title, mname) ;

			/*
			if (pSpec->m_Refname)
			{
				sname = *pSpec->m_Refname ;
				slog.Out("Doing member %s (fld spec %s)\n", mname, sname) ;
				Z.Printf("\tfldspec=\"%s\"", sname) ;
			}
			else
			{
				slog.Out("Doing member %s (fld spec %p)\n", mname, pSpec) ;
				Z << "\tfldspec=\"UNNAMED\"" ;
			}
			*/

			Z << "\tdata=\"" ; Z << pebuf ; Z << mname << "\"" ;

			Z << "\tflags=\"req\"/></td></tr>\n" ;
		}
		Z << "\t</table>\n" ;

		//	Write out a rudimentry page footer
		Z <<
		"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n"
		"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Abort\"/> <xbutton show=\"Submit\"/></td></tr>\n"
		"\t\t<tr height=\"50\" bgcolor=\"#CCCCCC\"><td class=\"vb10blue\"><center>Site Name</center></td></tr>\n"
		"\t</table>\n" ;

		Z << "\t</xform>\n" ;
		Z << "</xpage>\n\n" ;

		//	Write form-handler for class

		if (bUser)
		{
			//	Write out user registration form-handler. This will be the first form-handler (with name of the form fhdl_classname), with a sendemail step to
			//	conduct email verification and reference to a second form-handler. This second form handler will also be produced and will commit a new user.

			Z.Printf("<formhdl name=\"fhdl_phase1_%s\" form=\"form_reg_%s\" ops=\"cookie\">\n", cname, cname) ;
			Z << "\t<procdata>\n" ;

			//	Do the <setvar tags for all the members
			Z << "\t\t<setvar name=\"iniCode\" type=\"uint32_t\" value=\"%x:usecs;\"/>\n" ;
			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("\t\t<setvar name=\"%s\" type=\"%s\" value=\"%ce:%s;\"/>\n", mname, pSpec->m_pType->TxtTypename() , CHAR_PERCENT, mname) ;
			}

			//	Do the send email code thing

			Z.Printf("\t\t<sendemail from=\"%s\" to=\"%ce:email;\" smtpuser=\"%s\" smtppass=\"%s\" subject=\"Membership Application\">\n",
				*pApp->m_SmtpAddr, CHAR_PERCENT, *pApp->m_SmtpUser, *pApp->m_SmtpPass) ;

			//Dear %e:salute; %e:fname; %e:lname; of %e:orgname;

			Z.Printf("Thank you for registering with %s. Your initial login code is %cs:iniCode;i\n\n", *pApp->m_Domain, CHAR_PERCENT) ;
			Z << "The code is good for 1 hour. Enter this code instead of your selected password in the login screen.\n\n" ;

			Z << "This email has been generated by a submission to the Registration form at " ;
			Z << pApp->m_Domain ;
			Z << ". If you have recived this in error, please ignore\n" ;
			Z << "\t\t</sendemail>\n" ;
			Z << "\t</procdata>\n" ;

			Z << "\t<error goto=\"/register\"/>\n\n" ;

			Z.Printf("\t<response name=\"form_reg_%s\" bgcolor=\"eeeeee\">\n", cname) ;
			Z << "\t\t<xblock name=\"gdInclude\"/>\n" ;
			Z.Printf("<xform name=\"formCkCorp1\" action=\"hdlCkCorp\">\n") ;

			Z <<
			"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t<tr><td align=center class=title>Email Verification</td></tr>\n"
			"\t\t<tr height=\"250\">\n"
			"\t\t<td class=\"main\">\n"
			"\t\tThank you %s:fname; %s:lname; for your registration on belhalf of %s:orgname; to the gdpr360 XLS document processing service.\n"
			"\t\t</td>\n"
			"\t\t</tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\" class=\"fld\">Code:</td><td><xfield fldspec=\"fs_Usec\" var=\"testCode\" flags=\"req\"/></td></tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Complete Registration\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t</xform>\n"
			"\t</response>\n"
			"</formhdl>\n\n" ;

			Z.Printf("<formhdl name=\"fhdl_phase2_%s\" form=\"form_reg_%s\" ops=\"cookie\">\n", cname, cname) ;
			Z <<
			"\t<procdata>\n"
			"\t\t<testeq param1=\"%e:testCode;\" param2=\"%s:iniCode;\">\n"
			"\t\t\t<error name=\"InvalidCode\" bgcolor=\"eeeeee\">\n"
			"\t\t\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t\t\t<xform name=\"formCkCorp2\" action=\"hdlCkCorp\">\n"
			"\t\t\t\t\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t\t\t\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t\t\t\t\t<tr><td align=center class=title>Oops! Wrong Code!</td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"250\"><td class=\"main\">Please check the email again and type in or cut and paste the code.</td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"50\"><td align=\"center\" class=\"fld\">Code:</td><td><xfield fldspec=\"fs_Usec\" var=\"testCode\" flags=\"req\"/></td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Complete Registration\"/></td></tr>\n"
			"\t\t\t\t\t</table>\n"
			"\t\t\t\t</xform>\n"
			"\t\t\t</error>\n"
			"\t\t</testeq>\n" ;

			Z.Printf("<addSubscriber class=\"%s\" userName=\"%css:orgname;\" userPass=\"%css:password;\" userEmail=\"%css:email;\">\n",
				cname, CHAR_PERCENT, CHAR_PERCENT, CHAR_PERCENT) ;

			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("\t\t\t<seteq member=\"%s\" input=\"%cs%s\"/>\n", mname, CHAR_PERCENT, mname) ;
			}

			Z <<
			"\t\t</addSubscriber>\n"
			"\t\t<logon user=\"%s:orgname;\"/>\n"
			"\t</procdata>\n" ;

			Z <<
			"\t<error name=\"RegistrationError\" bgcolor=\"eeeeee\">\n"
			"\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t<div class=\"stdpage\">\n"
			"\t\t<table bgcolor=\"#FFDDDD\" width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t<tr><td align=center class=title>Internal Error</td></tr>\n"
			"\t\t<tr height=\"160\"><td>&nbsp;</td></tr>\n"
			"\t\t<tr>\n"
			"\t\t<td>An Internal error has occured and system administration has been notified. Please try again later.</td>\n"
			"\t\t</tr>\n"
			"\t\t<tr height=\"160\"><td>&nbsp;</td></tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Return to Site\" goto=\"/index.html\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t\t</div>\n"
			"\t</error>\n" ;

			Z <<
			"\t<response name=\"CorpCodeOK\" bgcolor=\"eeeeee\">\n"
			"\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t\t<tr><td align=center class=title>Registration Complete</td></tr>\n"
			"\t\t\t<tr height=\"250\">\n"
			"\t\t\t\t<td class=\"main\">\n"
			"\t\t\t\tYour registration as %u:orgname; is now complete. You are logged in and may now use the document processing facility.\n"
			"\t\t\t\t</td>\n"
			"\t\t\t</tr>\n"
			"\t\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Continue\" goto=\"/menu\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t</response>\n"
			"</formhdl>\n" ;
		}
		else
		{
			//	Write out standard commit form-handler. (fhdl_classname)
			Z.Printf("<formhdl name=\"fhdl_add_%s\" form=\"form_add_%s\">\n", cname, cname) ;
			Z << "\t<procdata>\n" ;
			Z.Printf("<commit class=\"%s\">\n", cname) ;

			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("<seteq member=\"%s\" input=\"%ce:%s;\"/>\n", mname, CHAR_PERCENT, mname) ;
			}

			Z <<
			"\t\t</commit>\n"
			"\t</procdata>\n\n" ;

			Z.Printf("<error goto=\"/add_%s\"/>\n", cname) ;

			Z.Printf("<response name=\"%s\" bgcolor=\"eeeeee\">\n", cname) ;

			Z <<
			"\t</response>\n"
			"</formhdl>\n" ;
		}

		Z << "</disseminoInclude>\n" ;

		//	Commit the pro-forma page, form and form-handlers to a file called 'classname.forms' This can then be adapted and included in the main XML file for
		//	the site
		if (Z.Size())
		{
			sprintf(fname, "%s/%s.forms", *pApp->m_Configdir, cname) ;

			slog.Out("%s. Writing form of %d bytes to %s\n", *_fn, Z.Size(), fname) ;

			os.open(fname) ;
			if (os.fail())
				threadLog("Could not open export forms file (classforms.dat)\n") ;
			else
			{
				os << Z ;
				if (os.fail())
					threadLog("Could not write export forms file (classforms.dat)\n") ;
				os.close() ;
			}
			os.clear() ;
		}

		Z.Clear() ;
	}
}
#endif

void	ExportForms	(hdsApp* pApp)
{
	//	Export data classes found in the configs as XML fragments that define default forms and form-handlers. This facility is provided as a development aid in
	//	order to save time. The output is to file name "classforms.def" in the current directory which should always be the development config directory for the
	//	website. The facility is invoked by calling Dissemino with "-forms" supplied as an argument. Dissemino will load the configs, write the classforms.def
	//	file and then exit.
	//
	//	There will be one form and one form-handler generated for each class found in the configs, including those for which form(s) currently exists. Forms in
	//	HTML cannot be nested so in cases where class members are composite (have a data type which is another class), the form produced will contain an extra
	//	button for each composite member. The class which a member has as its data type will have to have been declare beforehand (otherwise the configs cannot
	//	be read in), and this will have a form and form handler. The ...

	_hzfunc(__func__) ;

	const hdsFldspec*	pSpec ;		//	Field spec
	const hdbClass*		pClass ;	//	Data class pointer
	const hdbMember*	pMem ;		//	Data class member pointer

	ofstream	os ;				//	Output file
	hzChain		Z ;					//	Output chain
	hdsBlock*	pIncl ;				//	Pointer to default include block
	hzString	dfltInclude ;		//	Use a <xinclude> block in pages?
	hzString	cat ;				//	Category
	const char*	cname ;				//	Class name
	const char*	mname ;				//	Member name
	const char*	sname ;				//	Fldspec reference name
	uint32_t	nC ;				//	Data class iterator
	uint32_t	nM ;				//	Data class member iterator
	bool		bUser ;				//	True is class is a user class
	char		fname[80] ;			//	Filename
	char		pebuf[4] ;			//	Filename

	slog.Out("%s. Have %d classes and %d repositories\n", *_fn, pApp->m_ADP.CountDataClass(), pApp->m_ADP.CountObjRepos()) ;

	pebuf[0] = CHAR_PERCENT ;
	pebuf[1] = 's' ;
	pebuf[2] = ':' ;
	pebuf[3] = 0 ;

	if (pApp->m_Includes.Count())
	{
		pIncl = pApp->m_Includes.GetObj(0) ;
		dfltInclude = pIncl->m_Refname ;
	}

	for (nC = 0 ; nC < pApp->m_ADP.CountDataClass() ; nC++)
	{
		pClass = pApp->m_ADP.GetDataClass(nC) ;
		if (pClass->m_Category)
			cat = pClass->m_Category ;
		else
			cat = "Test" ;

		if (!pClass)
			Fatal("%s. No class found at posn %d\n", *_fn, nC) ;

		if (pClass->StrTypename() == "subscriber")
			{ slog.Out("%s Ignoring subscriber class\n", *_fn) ; continue ; }

		cname = pClass->StrTypename() ;
		slog.Out("%s. Doing class %s with %d members\n", *_fn, cname, pClass->MbrCount()) ;

		bUser = false ;
		if (pApp->m_UserTypes.Exists(cname))
			bUser = true ;

		//	Start the XML output
		Z << "<disseminoInclude>\n" ;

		/*
		**	Write out a basic list page for the class
		*/

		Z.Printf("<xpage path=\"/list_%s\" subject=\"%s\" title=\"List %s\" access=\"any\" bgcolor=\"eeeeee\" ops=\"noindex\">\n", cname, *cat, cname) ;
		Z << "\t<desc>None</desc>\n" ;
		if (dfltInclude)
			Z.Printf("\t<xblock name=\"%s\"/>\n", *dfltInclude) ;
		Z.AddByte(CHAR_NL) ;

		//	???
		Z <<
		"<style>\n"
		".tooltip { position: relative; display: inline-block; border-bottom: 1px dotted black; }\n"
		".tooltip .tooltiptext { visibility: hidden; width: 120px; background-color: black; color: #fff; text-align: center; padding: 5px 0; border-radius: 6px; position: absolute; z-index: 1; }\n"
		".tooltip:hover .tooltiptext { visibility: visible; }\n"
		"</style>\n\n" ;

		Z.Printf("\t<xtable repos=\"%s\" criteria=\"all\" rows=\"10000\" width=\"90\" height=\"500\" css=\"main\">\n", cname) ;
		Z <<
		"\t\t<ifnone><p>Sorry no records ...</p></ifnone>\n"
		"\t\t<header>Table header ...</header>\n"
		"\t\t<footer>Table footer ...</footer>\n" ;

		for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
		{
			pMem = pClass->GetMember(nM) ;
			mname = pMem->TxtName() ;

			Z.Printf("\t\t<xcol member=\"%s\" title=\"%s\"/>\n", mname, mname) ;
		}

		Z << "\t</xtable>\n" ;
		Z << "</xpage>\n" ;

		/*
		**	Write out a basic host page for the form for the class
		*/

		Z.Printf("<xpage path=\"/add_%s\" subject=\"%s\" title=\"Add %s\" access=\"any\" bgcolor=\"eeeeee\" ops=\"noindex\">\n", cname, *cat, cname) ;
		Z << "\t<desc>None</desc>\n" ;
		if (dfltInclude)
			Z.Printf("\t<xblock name=\"%s\"/>\n", *dfltInclude) ;
		Z.AddByte(CHAR_NL) ;

		Z <<
		"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
		"\t\t<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"\t\t<tr><td class=\"title\" align=\"center\">Title: Add a " << cname << "</td></tr>\n"
		"\t\t<tr>\n"
		"\t\t<td valign=\"top\" align=\"center\" class=\"main\">\n"
		"\t\t<p>\n"
		"\t\tUse this form to create an instance of " << cname << "\n"
		"\t\t</p>\n"
		"\t\t</td>\n"
		"\t</tr>\n"
		"\t<xdiv user=\"public\">\n"
		"\t<tr><td height=\"20\">&nbsp;</td></tr>\n"
		"\t<tr>\n"
		"\t\t<td height=\"20\" align=\"center\" class=\"main\">\n"
		"\t\tPlease be aware that by submitting this form, you are consenting to the use of cookies. Please read our cookie policy to <a href=\"/cookies\">find out more</a>\n"
		"\t\t</td>\n"
		"\t</tr>\n"
		"\t</xdiv>\n"
		"\t<tr><td height=\"40\">&nbsp;</td></tr>\n"
		"\t</table>\n\n" ;

		//	Now write out the form itself
		if (bUser)
			Z.Printf("\t<xform name=\"form_reg_%s\" action=\"fhdl_phase1_%s\" class=\"%s\">\n", cname, cname, cname) ;
		else
			Z.Printf("\t<xform name=\"form_add_%s\" action=\"fhdl_add_%s\" class=\"%s\">\n", cname, cname, cname) ;
		Z << "\t<xhide name=\"lastpage\" value=\"%x:referer;\"/>\n\n" ;

		Z << "\t<table width=\"56%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"main\">\n" ;

		for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
		{
			pMem = pClass->GetMember(nM) ;
			mname = pMem->TxtName() ;

			pSpec = pMem->GetSpec() ;

			if (!pSpec)
				{ slog.Out("%s. Warning No Field specification for member %s\n", *_fn, mname) ; continue ; }

			Z.Printf("\t\t<tr height=\"18\"><td class=\"fld\">%s:</td>\t<td><xfield member=\"%s\"", mname, mname) ;

			//	???
			if (pSpec->m_Desc)
				Z.Printf("\t\t<tr height=\"18\"><td class=\"fld\"><div class=\"tooltip\">%s<span class=\"tooltiptext\">%s</span></div>:</td>\t<td><xfield member=\"%s\"",
					*pSpec->m_Title, *pSpec->m_Desc, mname) ;
			else
				Z.Printf("\t\t<tr height=\"18\"><td class=\"fld\">%s:</td>\t<td><xfield member=\"%s\"", *pSpec->m_Title, mname) ;

			/*
			if (pSpec->m_Refname)
			{
				sname = *pSpec->m_Refname ;
				slog.Out("Doing member %s (fld spec %s)\n", mname, sname) ;
				Z.Printf("\tfldspec=\"%s\"", sname) ;
			}
			else
			{
				slog.Out("Doing member %s (fld spec %p)\n", mname, pSpec) ;
				Z << "\tfldspec=\"UNNAMED\"" ;
			}
			*/

			Z << "\tdata=\"" ; Z << pebuf ; Z << mname << "\"" ;

			Z << "\tflags=\"req\"/></td></tr>\n" ;
		}
		Z << "\t</table>\n" ;

		//	Write out a rudimentry page footer
		Z <<
		"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n"
		"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Abort\"/> <xbutton show=\"Submit\"/></td></tr>\n"
		"\t\t<tr height=\"50\" bgcolor=\"#CCCCCC\"><td class=\"vb10blue\"><center>Site Name</center></td></tr>\n"
		"\t</table>\n" ;

		Z << "\t</xform>\n" ;
		Z << "</xpage>\n\n" ;

		//	Write form-handler for class

		if (bUser)
		{
			//	Write out user registration form-handler. This will be the first form-handler (with name of the form fhdl_classname), with a sendemail step to
			//	conduct email verification and reference to a second form-handler. This second form handler will also be produced and will commit a new user.

			Z.Printf("<formhdl name=\"fhdl_phase1_%s\" form=\"form_reg_%s\" ops=\"cookie\">\n", cname, cname) ;
			Z << "\t<procdata>\n" ;

			//	Do the <setvar tags for all the members
			Z << "\t\t<setvar name=\"iniCode\" type=\"uint32_t\" value=\"%x:usecs;\"/>\n" ;
			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("\t\t<setvar name=\"%s\" type=\"%s\" value=\"%ce:%s;\"/>\n", mname, pSpec->m_pType->TxtTypename() , CHAR_PERCENT, mname) ;
			}

			//	Do the send email code thing

			Z.Printf("\t\t<sendemail from=\"%s\" to=\"%ce:email;\" smtpuser=\"%s\" smtppass=\"%s\" subject=\"Membership Application\">\n",
				*pApp->m_SmtpAddr, CHAR_PERCENT, *pApp->m_SmtpUser, *pApp->m_SmtpPass) ;

			//Dear %e:salute; %e:fname; %e:lname; of %e:orgname;

			Z.Printf("Thank you for registering with %s. Your initial login code is %cs:iniCode;i\n\n", *pApp->m_Domain, CHAR_PERCENT) ;
			Z << "The code is good for 1 hour. Enter this code instead of your selected password in the login screen.\n\n" ;

			Z << "This email has been generated by a submission to the Registration form at " ;
			Z << pApp->m_Domain ;
			Z << ". If you have recived this in error, please ignore\n" ;
			Z << "\t\t</sendemail>\n" ;
			Z << "\t</procdata>\n" ;

			Z << "\t<error goto=\"/register\"/>\n\n" ;

			Z.Printf("\t<response name=\"form_reg_%s\" bgcolor=\"eeeeee\">\n", cname) ;
			Z << "\t\t<xblock name=\"gdInclude\"/>\n" ;
			Z.Printf("<xform name=\"formCkCorp1\" action=\"hdlCkCorp\">\n") ;

			Z <<
			"\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t<tr><td align=center class=title>Email Verification</td></tr>\n"
			"\t\t<tr height=\"250\">\n"
			"\t\t<td class=\"main\">\n"
			"\t\tThank you %s:fname; %s:lname; for your registration on belhalf of %s:orgname; to the gdpr360 XLS document processing service.\n"
			"\t\t</td>\n"
			"\t\t</tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\" class=\"fld\">Code:</td><td><xfield fldspec=\"fs_Usec\" var=\"testCode\" flags=\"req\"/></td></tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Complete Registration\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t</xform>\n"
			"\t</response>\n"
			"</formhdl>\n\n" ;

			Z.Printf("<formhdl name=\"fhdl_phase2_%s\" form=\"form_reg_%s\" ops=\"cookie\">\n", cname, cname) ;
			Z <<
			"\t<procdata>\n"
			"\t\t<testeq param1=\"%e:testCode;\" param2=\"%s:iniCode;\">\n"
			"\t\t\t<error name=\"InvalidCode\" bgcolor=\"eeeeee\">\n"
			"\t\t\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t\t\t<xform name=\"formCkCorp2\" action=\"hdlCkCorp\">\n"
			"\t\t\t\t\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t\t\t\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t\t\t\t\t<tr><td align=center class=title>Oops! Wrong Code!</td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"250\"><td class=\"main\">Please check the email again and type in or cut and paste the code.</td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"50\"><td align=\"center\" class=\"fld\">Code:</td><td><xfield fldspec=\"fs_Usec\" var=\"testCode\" flags=\"req\"/></td></tr>\n"
			"\t\t\t\t\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Complete Registration\"/></td></tr>\n"
			"\t\t\t\t\t</table>\n"
			"\t\t\t\t</xform>\n"
			"\t\t\t</error>\n"
			"\t\t</testeq>\n" ;

			Z.Printf("<addSubscriber class=\"%s\" userName=\"%css:orgname;\" userPass=\"%css:password;\" userEmail=\"%css:email;\">\n",
				cname, CHAR_PERCENT, CHAR_PERCENT, CHAR_PERCENT) ;

			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("\t\t\t<seteq member=\"%s\" input=\"%cs%s\"/>\n", mname, CHAR_PERCENT, mname) ;
			}

			Z <<
			"\t\t</addSubscriber>\n"
			"\t\t<logon user=\"%s:orgname;\"/>\n"
			"\t</procdata>\n" ;

			Z <<
			"\t<error name=\"RegistrationError\" bgcolor=\"eeeeee\">\n"
			"\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t<div class=\"stdpage\">\n"
			"\t\t<table bgcolor=\"#FFDDDD\" width=\"100%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t<tr><td align=center class=title>Internal Error</td></tr>\n"
			"\t\t<tr height=\"160\"><td>&nbsp;</td></tr>\n"
			"\t\t<tr>\n"
			"\t\t<td>An Internal error has occured and system administration has been notified. Please try again later.</td>\n"
			"\t\t</tr>\n"
			"\t\t<tr height=\"160\"><td>&nbsp;</td></tr>\n"
			"\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Return to Site\" goto=\"/index.html\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t\t</div>\n"
			"\t</error>\n" ;

			Z <<
			"\t<response name=\"CorpCodeOK\" bgcolor=\"eeeeee\">\n"
			"\t\t<xblock name=\"gdInclude\"/>\n"
			"\t\t<table width=\"96%\" align=\"center\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"\t\t\t<tr><td width=5 height=25>&nbsp;</td></tr>\n"
			"\t\t\t<tr><td align=center class=title>Registration Complete</td></tr>\n"
			"\t\t\t<tr height=\"250\">\n"
			"\t\t\t\t<td class=\"main\">\n"
			"\t\t\t\tYour registration as %u:orgname; is now complete. You are logged in and may now use the document processing facility.\n"
			"\t\t\t\t</td>\n"
			"\t\t\t</tr>\n"
			"\t\t\t<tr height=\"50\"><td align=\"center\"><xbutton show=\"Continue\" goto=\"/menu\"/></td></tr>\n"
			"\t\t</table>\n"
			"\t</response>\n"
			"</formhdl>\n" ;
		}
		else
		{
			//	Write out standard commit form-handler. (fhdl_classname)
			Z.Printf("<formhdl name=\"fhdl_add_%s\" form=\"form_add_%s\">\n", cname, cname) ;
			Z << "\t<procdata>\n" ;
			Z.Printf("<commit class=\"%s\">\n", cname) ;

			for (nM = 0 ; nM < pClass->MbrCount() ; nM++)
			{
				pMem = pClass->GetMember(nM) ;
				mname = pMem->TxtName() ;

				Z.Printf("<seteq member=\"%s\" input=\"%ce:%s;\"/>\n", mname, CHAR_PERCENT, mname) ;
			}

			Z <<
			"\t\t</commit>\n"
			"\t</procdata>\n\n" ;

			Z.Printf("<error goto=\"/add_%s\"/>\n", cname) ;

			Z.Printf("<response name=\"%s\" bgcolor=\"eeeeee\">\n", cname) ;

			Z <<
			"\t</response>\n"
			"</formhdl>\n" ;
		}

		Z << "</disseminoInclude>\n" ;

		//	Commit the pro-forma page, form and form-handlers to a file called 'classname.forms' This can then be adapted and included in the main XML file for
		//	the site
		if (Z.Size())
		{
			sprintf(fname, "%s/%s.forms", *pApp->m_Configdir, cname) ;

			slog.Out("%s. Writing form of %d bytes to %s\n", *_fn, Z.Size(), fname) ;

			os.open(fname) ;
			if (os.fail())
				threadLog("Could not open export forms file (classforms.dat)\n") ;
			else
			{
				os << Z ;
				if (os.fail())
					threadLog("Could not write export forms file (classforms.dat)\n") ;
				os.close() ;
			}
			os.clear() ;
		}

		Z.Clear() ;
	}
}

#if ZZZ
ActState	ProcessAction	(hzsAction* pAct)
{
}

void*	DoTheTask	(void * pVoid)
{
	//	Runs a single publication issue task (collect, process, deliver) or an incremental collector. This will run in a separate thread. The
	//	hatIdesc instance (task controller) is passed in via pVoid.

	_hzfunc("DoTheTask") ;

	hzProcess	_procTask ;		//	hzProcess instance

	FSTAT		fs ;
	hzLogger	logAct ;		//	Log channel for this action only
	hzEmaddr	emaddr ;		//	Email addr (of admin in event of failure)
	hzEmail		E ;				//	Email class (for email construction)
	hzEmail		F ;				//	Email class (in event of failure)
	hzChain		embody ;		//	Email body
	hzChain		emreport ;		//	Record of SMTP conversation
	hzChain		memReport ;		//	Chain for memory report
	hzString	server ;		//	Email server
	hzString	sender ;		//	Sender (HATS)
	hzString	announce ;		//	Sended apparent name
	hzString	subject ;		//	Email subject (task has failed)
	hzString	logpath ;		//	Full pathname of logfile
	hzXDate		expires ;		//	Time job ends
	hzSDate		issueDate ;		//	Issue date (from the task from the act)
	hatIdesc	task ;			//	Task (from the act)
	hzsAction*	pAct ;			//	The scheduled task
	hatPubl*	pub ;			//	The parent publication
	hatIctl*	ivk ;			//	Invokation
	hatColl*	pColl ;			//	Invokations's collector
	hatProc*	pProc ;			//	Invokations's processor
	hatDlvr*	pDlvr ;			//	Invokations's deliverer
	int32_t		dlvrFlags ;		//	Delivery flags - Mostly to exclude emails
	int32_t		nFail = 0 ;		//	Set if task fails and an email needs to be sent
	ActState	as ;			//	Action status
	hzEcode		rc ;			//	Return codes ...

	//	Set signal to capture segmentation errors
	signal(SIGSEGV, CatchSegVio);

	//	Get the action, publication and invokation
	pAct = (hzsAction *) pVoid ;
	if (!pAct)
	{
		pthread_exit(pVoid) ;
		return pVoid ;
	}

	pub = pAct->GetPub() ;
	ivk = pAct->GetIctl() ;

	if (!pub)	schlog.Out("%s No publication in supplied action (%s) (machine addr %p)\n", *_fn, *pAct->Str(), pAct) ;
	if (!ivk)	schlog.Out("%s No invokation in supplied action (%s) (machine addr %p)\n", *_fn, *pAct->Str(), pAct) ;

	if (pAct->CurrStatus() == HZ_ACTION_NULL)
		schlog.Out("%s Action is not initialized (%s) (machine addr %p)\n", *_fn, *pAct->Str(), pAct) ;

	if (!pAct || !pub || !ivk)
	{
		pthread_exit(pVoid) ;
		return pVoid ;
	}

	pColl = ivk->m_pColl ;
	pProc = ivk->m_pProc ;
	pDlvr = ivk->m_pDlvr ;

	if (!pColl && !pProc && !pDlvr)
	{
		schlog.Out("%s Action invokation has no components (%s) (machine addr %p)\n", *_fn, *pAct->Str(), pAct) ;
		pthread_exit(pVoid) ;
		return pVoid ;
	}

	//	Increment thread counter and set action to running
	//SetFred(true) ;
	pAct->m_bRunning = true ;

	//	Obtain the issue date (even staging invokations have this in order to regulate filenames)
	task = pAct->Id() ;
	issueDate = task.GetDate() ;	//.SetDate(task.cstrDate()) ;

	/*
	**	Open task logfile
	*/

	logpath = g_LogsDir + "/" + pAct->Str() ;
	logAct.OpenFile(*logpath, LOGROTATE_NEVER) ;

	expires.SysDateTime() ;
	logAct.Out("#\n#\tTask %s Commences at %04d:%02d:%02d %02d:%02d:%02d\n#\n", *pAct->Str(),
		expires.Year(), expires.Month(), expires.Day(), expires.Hour(), expires.Min(), expires.Sec()) ;

	schlog.Record("Task %s commenced: current status %s\n", *pAct->Str(), *ActState2Str(pAct->CurrStatus())) ;

	/*
	**	Eliminate completed of failed tasks
	*/

	/*
	if (act->CurrStatus() == HZ_ACTION_QUE4DLVR)	{ logAct.Out("Nothing to do: Task %s is already in que\n", *act->Str()) ; goto doneproc ; }
	if (act->CurrStatus() == HZ_ACTION_COMPLETE)	{ logAct.Out("Nothing to do: Task %s is complete\n", *act->Str()) ; goto doneproc ; }
	if (act->CurrStatus() > HZ_ACTION_FAILCOLLECT)	{ logAct.Out("Nothing to do: Task %s has failed\n", *act->Str()) ; goto doneproc ; }
	*/

	if (pAct->CurrStatus() == HZ_ACTION_PENDING || pAct->CurrStatus() == HZ_ACTION_STARTED)
	{
		/*
		**	Run collection
		*/

		logAct.Record("Status=%s: Invoking Collection stage\n", *ActState2Str(pAct->CurrStatus())) ;

		//	if (pAct->CurrStatus() == HZ_ACTION_PENDING)
		//		pAct->StatusEvent(HZ_ACTION_STARTED) ;

		schlog.Out("PRE-COLLECT STATS\n") ;
		ReportMemoryUsage(memReport, false) ;
		schlog.Out(memReport) ;
		as = CollectingHat(pAct) ;
		pAct->StatusEvent(as) ;
		schlog.Out("POST-COLLECT STATS\n") ;
		ReportMemoryUsage(memReport, false) ;
		schlog.Out(memReport) ;

		/*
		**	Update action status and report
		*/

		if (pAct->CurrStatus() == HZ_ACTION_COLLECTED)
			logAct.Record("The collection was a success\n") ;
		else if (pAct->CurrStatus() == HZ_ACTION_FAILCOLLECT)
		{
			logAct.Record("The collection has failed\n") ;
			nFail = 1 ;
		}
		else
		{
			pAct->UpdateRuntime(900) ;

			if (pAct->Run() > pAct->Expires())
			{
				logAct.Record("Task %s has expired. (issue=%s, run=%s, expired=%s) Marked as failed\n",
					*task.Str(), *issueDate.Str(), *pAct->Run().Str(), *expires.Str()) ;
				pAct->StatusEvent(HZ_ACTION_FAILCOLLECT) ;
				nFail = 2 ;
			}

			logAct.Record("The collection needs to be redone\n") ;
		}
	}

	if (pAct->CurrStatus() == HZ_ACTION_COLLECTED)
	{
		//	Run the process

		if (!ivk->m_pProc)
		{
			logAct.Out("%s. This publication has no processor for this invokation\n", *_fn) ;
			pAct->SetStatus(HZ_ACTION_PROCESSED) ;
		}
		else
		{
			logAct.Record("INVOKING DATA PROCESSING\n") ;

			schlog.Out("PRE-PROCESS STATS\n") ;
			ReportMemoryUsage(memReport, false) ;
			schlog.Out(memReport) ;

			as = ProcessAction(pAct) ;
			pAct->StatusEvent(as) ;

			schlog.Out("POST-PROCESS STATS\n") ;
			ReportMemoryUsage(memReport, false) ;
			schlog.Out(memReport) ;

			if (pAct->CurrStatus() == HZ_ACTION_PROCESSED)
				logAct.Record("The data processing was a success\n") ;

			if (pAct->CurrStatus() == HZ_ACTION_FAILPROCESS)
			{
				logAct.Record("The data processing has failed\n") ;
				nFail = 3 ;
			}
		}
	}

	if (pAct->CurrStatus() == HZ_ACTION_PROCESSED)
	{
		schlog.Out("PRE-DELIVERY STATS\n") ;
		ReportMemoryUsage(memReport, false) ;
		schlog.Out(memReport) ;

		if (ActHistoric(pAct->Id()))
			as = DeliveringHat(pAct, DLVR_UP_ALLES | DLVR_PG_ALLES) ;
		else
		{
			dlvrFlags = DLVR_UP_ALLES + DLVR_PG_ALLES + DLVR_EMAIL_DOC + DLVR_EMAIL_TXT + DLVR_EMAIL_HTM ;
			dlvrFlags &= pDlvr->m_TodoFlags ;
			as = DeliveringHat(pAct, dlvrFlags) ;
		}
		pAct->StatusEvent(as) ;

		schlog.Out("POST-DELIVERY STATS\n") ;
		ReportMemoryUsage(memReport, false) ;
		schlog.Out(memReport) ;

		//if (pAct->CurrStatus() == HZ_ACTION_COMPLETE)
	}

	/*
	*/

	/*
	**	Send email to administrator if task has either finished or failed
	*/

	if (pAct->CurrStatus() == HZ_ACTION_EMAILED)
		pAct->SetStatus(HZ_ACTION_COMPLETE) ;

	if (pAct->CurrStatus() >= HZ_ACTION_FAILCOLLECT)
	{
		embody << "Doing task " << pAct->Str() << "\n" ;

		if (nFail == 1)
			embody.Printf("The collection has failed\n") ;

		if (nFail == 2)
			embody.Printf("Task %s has expired. (run=%s, expired=%s) Marked as failed\n",
				*task.Str(), *pAct->Run().Str(), *expires.Str()) ;

		if (nFail == 3)
			embody.Printf("The data processing has failed\n") ;

		if (nFail == 4)
			embody.Printf("QueDeliver failed. Task %s has thus expired in delivery (error=%s)\n",
				*pAct->Str(), Err2Txt(rc)) ;

		subject = "Task " ;
		subject += pAct->Str() ;
		subject += " Status = " ;
		subject += ActState2Str(pAct->CurrStatus()) ;

		rc = E.SetSender("tnauk2@tnauk.org.uk", "HATS FAILURE ALERT") ;
		if (rc != E_OK)
			logAct.Record("Failed to set sender to hats@tnauk.org.uk (HATS FAILURE ALERT)\n") ;

		if (rc == E_OK)
		{
			rc = E.SetSubject(*subject) ;
			if (rc != E_OK)
				logAct.Record("Failed to set subject to %s\n", *subject) ;
		}

		if (rc == E_OK)
		{
			rc = E.AddBody(embody) ;
			if (rc != E_OK)
				logAct.Record("Failed to set add email body\n") ;
		}

		if (rc == E_OK)
		{
			rc = E.Compose() ;
			if (rc != E_OK)
				logAct.Record("Failed to set compose email\n") ;
		}

		if (rc == E_OK)
			rc = E.SendSmtp("127.0.0.1", g_emsysUser, g_emsysPass) ;

		if (rc != E_OK)
			logAct.Record("Failed to send email to administrator\n") ;
		else
			logAct.Record("Sent email to administrator\n") ;

		embody.Clear() ;
	}

doneproc:
	pAct->m_bRunning = false ;

	logAct.Record("jobid %s - terminating thread\n", *pAct->Str()) ;
	logAct.Close() ;
	//SetFred(false) ;

	pthread_exit(pVoid) ;
	return pVoid ;
}

void*	ScheduleTasks	(void* pVoid)
{
	//	This is the 'main' function of the scheduling thread. It wakes up every 10 seconds or so to check for three classes of possible actions becoming due or
	//	necessary. These are:-
	//
	//		1)	Asynchonous tasks such as data collection from vendors which are time consuming and may require retries 
	//		2)	Tasks that can be performed on the basis of memory resident data and so are not exected to require retries or consume much time
	//		3)	Tasks that react to conditions such as a config file change or a problem with the server thread.
	//
	//	It loops round all active tasks to kick them off.
	//	This runs in its own thread leaving the main thread to serve web pages.

	_hzfunc("ScheduleTasks") ;

	static	int32_t	lastY = -1 ;		//	Last tested year
	static	int32_t	lastM = -1 ;		//	Last tested month
	static	int32_t	lastD = -1 ;		//	Last tested day

	hzProcess	_theSched ;				//	Thread initializer

	hzVect<hzsAction*>	results ;		//	Actions to consider
	hzVect<hzsAction*>	toRun ;			//	Actions to run
	hzVect<hatIdesc>	tasks ;			//	Issues to enact (populated by TestTriggers)
	hzVect<hatPubl*>	allpubs ;		//	All pubs (by number as it happens)
	hzVect<hatIdesc>	newtasks ;		//	Actions for each publication

	pthread_attr_t	tattr ;				//	Thread attribute (to support thread invokation)
	pthread_t		tid ;				//	Thread identitiy (to support thread invokation)

	hatIdesc	task ;					//	OMG!
	hzXDate		now ;					//	The time now ...
	hzXDate		nex ;					//	Next time to check file triggers
	hzXDate		doplan ;				//	Next time to run make plans
	hzXDate		dd ;					//	Date/time action fell due (var decl for diags)
	hzXDate		dr ;					//	Date/time action will run (var decl for diags)
	hzXDate		first ;
	hzString	path ;					//	Build path of logfile
	hatPubl*	pub ;					//	Publication of task
	hzsAction*	pAct ;					//	Action to consider
	int32_t		nIndex ;				//	For actions iteration
	int32_t		prc ;					//	pthread return value
	int32_t		interval ;				//	Number of seconds to sleep
	int32_t		nNewacts ;				//	Total of new actions added to db
	bool		bRun ;					//	Set if action runnable
	bool		bError ;				//	Error in formation of action
	hzEcode		rc = E_OK ;

	/*
	**	Open the logfile and set up signal handlers
	*/

	path = g_LogsDir + "/hschd" ;
	if (!path)
		Fatal("No logfile for scheduler\n") ;
	if (schlog.OpenFile(*path, LOGROTATE_NEVER) != E_OK)
		Fatal("Cannot open logfile %s\n", *path) ;

	signal(SIGSEGV, CatchSegVio);

	/*
	**	Loop round calling MakePlans function
	*/

	now.SysDateTime() ;
	doplan.SetDate(2000, 1, 1) ;
	doplan.SetTime(0, 0, 0) ;
	nex = doplan ;

	for (;;)
	{
		/*
		**	Purge directories and cycle the active publications to generate actions upon startup and every midnight
		*/

		now.SysDateTime() ;
		results.Clear() ;
		toRun.Clear() ;

		if (now.Year() != lastY || now.Month() != lastM || now.Day() != lastD)
		{
			/*
			**	Day has changed so calculate which actions should be performed over the coming 24 hours
			*/

			schlog.Record("Generating actions for all publications\n") ;

			schlog << "Doing Schedule\n{\n" ;

			PubSelect(allpubs, 0, ORD_ASC_NUMBER, true) ;

			for (nIndex = 0 ; nIndex < allpubs.Count() ; nIndex++)
			{
				pub = allpubs[nIndex] ;

				if (pub->m_Ictls.Count())
				{
					rc = pub->GenerateActions(newtasks, schlog) ;
					if (rc != E_OK)
						schlog.Record(" - FAILED to generate actions for %s\n\n", *pub->m_Pubname) ;
				}
			}

			schlog.Record("Now have total of %d potentially new actions\n", newtasks.Count()) ;

			//	Go thru actions and compile g_Actions etc

			first.SysDateTime() ;

			for (nNewacts = nIndex = 0 ; nIndex < newtasks.Count() ; nIndex++)
			{
				task = newtasks[nIndex] ;

				bError = false ;
				if (!task)
					{ bError = true ; schlog.Record("Excl act: Task not set\n") ; }

				if (bError)
				{
					schlog.Record("%s: Irradicated invalid action %s\n", __func__, *task.Str()) ;
					continue ;
				}

				rc = ActInject(task, TRIGGER_TIME) ;
				if (rc != E_OK)
					schlog.Record("%s: Could not insert action %s - error %s\n", __func__, *task.Str(), Err2Txt(rc)) ;
				else
					nNewacts++ ;
			}

			schlog.Record("\n}\nNow have total of %d new actions\n\n", nNewacts) ;

			lastY = now.Year() ;
			lastM = now.Month() ;
			lastD = now.Day() ;

			now.SysDateTime() ;
		}

		if (now > nex)
		{
			/*
			**	Check file triggers
			*/

			schlog.Out("%s. Examining Input Directories at %s\n", *_fn, *now.Str()) ;

			TestTriggers(tasks, schlog) ;

			schlog.Out("%s. Injecting %d tasks into job que\n{\n", *_fn, tasks.Count()) ;

			for (nIndex = 0 ; nIndex < tasks.Count() ; nIndex++)
			{
				task = tasks[nIndex] ;

				if (ActHistoric(task))
					{ schlog.Out("\tTask %s - Historic\n", *task.Str()) ; continue ; }

				pub = task.GetPub() ;
				if (!pub->m_OpStatus)
					{ schlog.Out("\tTask %s - Suspended\n", *task.Str()) ; continue ; }

				pAct = ActLookup(task) ;

				rc = ActInject(task, TRIGGER_FILE) ;
				if (rc != E_OK)
					Fatal("%s: Could not insert action %s - error %s\n", *_fn, *task.Str(), Err2Txt(rc)) ;

				if (pAct)
					schlog.Out("\tTask %s - has status %s\n", *task.Str(), *ActState2Str(pAct->CurrStatus())) ;
				else
					schlog.Out("\tTask %s - New Scheduled\n", *task.Str()) ;
			}

			//	Set up next time to run
			now.SysDateTime() ;
			interval = 600 - (now.AsEpoch() % 600) ;
			nex = now ;
			nex.altdate(SECOND, interval) ;

			schlog.Out("%s. Current time [%s] Next run will be at %s (%d seconds from now)\n", *_fn, *now.Str(), *nex.Str(), interval) ;
		}

		if (now > doplan)	// && GetFred() < g_nThreadLimit)
		{
			/*
			**	Make plans - Obtain all actions and derive a list of those due to run.
			*/

			ActSelect(results, 0, ORD_DEC_DATE, true) ;

			if (results.Count())
			{
				pAct = results[0] ;
				pAct = results[results.Count()-1] ;
			}

			schlog.Out("\nASSESSING %d ACTIONS to see if due\n", results.Count()) ;

			for (nIndex = 0 ; nIndex < results.Count() ; nIndex++)
			{
				pAct = results[nIndex] ;

				//trigType = pAct->IsReady() ;
				dd = pAct->Due() ;
				dr = pAct->Run() ;

				schlog.Out(" - [%02d:%02d:%02d] action %s (due %s, run %s)", now.Hour(), now.Min(), now.Sec(), *pAct->Str(), *dd.Str(), *dr.Str()) ;

				//	States requiring no further action
				if (pAct->CurrStatus() == HZ_ACTION_COMPLETE)		{ schlog.Out(" -> completed/delivered\n") ; continue ; }
				if (pAct->CurrStatus() == HZ_ACTION_FAILCOLLECT)	{ schlog.Out(" -> failed to collect\n") ; continue ; }
				if (pAct->CurrStatus() == HZ_ACTION_FAILPROCESS)	{ schlog.Out(" -> failed to process\n") ; continue ; }
				if (pAct->CurrStatus() == HZ_ACTION_EXPIRED)		{ schlog.Out(" -> failed to deliver\n") ; continue ; }

				//	Incomplete states
				if (pAct->CurrStatus() == HZ_ACTION_PENDING)		schlog.Out(" $pending") ;
				if (pAct->CurrStatus() == HZ_ACTION_STARTED)		schlog.Out(" $started (retry collect)") ;
				if (pAct->CurrStatus() == HZ_ACTION_COLLECTED)		schlog.Out(" $data collected") ;
				if (pAct->CurrStatus() == HZ_ACTION_PROCESSED)		schlog.Out(" $data processed") ;
				if (pAct->CurrStatus() == HZ_ACTION_PUBLISHED)		schlog.Out(" $data published") ;
				//if (pAct->CurrStatus() == HZ_ACTION_EMAILED)		schlog.Out(" $emailed DOC & HTML") ;
				//if (pAct->CurrStatus() == ACTION_EM_DOCZ)		schlog.Out(" $emailed (DOC)") ;
				//if (pAct->CurrStatus() == ACTION_EM_HTML)		schlog.Out(" $emailed (SHTML)") ;

				if (pAct->m_bRunning)
					{ schlog.Out(" $running\n") ; continue ; }

				bRun = true ;

				if (pAct->Due() > now)	{ schlog.Out(" $not-due") ; bRun = false ; }
				if (pAct->Run() > now)	{ schlog.Out(" $runs-soon") ; bRun = false ; }
				if (!pAct->ReadyTime())	{ schlog.Out(" $wait-time") ; /*bRun = false ;*/ }
				if (!pAct->ReadyData())	{ schlog.Out(" $wait-file") ; bRun = false ; }

				//	The action is not completed, due and not yet running so run it

				if (bRun)
				{
					toRun.Add(pAct) ;
					schlog.Out(" $qued-exec") ;
				}
				schlog.Out("\n") ;
			}

			schlog.Out("Extending do-plan by default 300 seconds\n") ;
			doplan = now ;
			doplan.altdate(SECOND, 300) ;
			schlog.Record("Done MakePlans: Will re-assess tasks at %s\n", *doplan.Str()) ;

			/*
			**	If there were actions due to run, go thru list kicking off each in turn
			*/

			schlog.Record("Got %d tasks qued for execution\n", toRun.Count()) ;

			pthread_attr_init(&tattr) ;
			pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED) ;

			for (nIndex = 0 ; nIndex < toRun.Count() ; nIndex++)
			{
				pAct = toRun[nIndex] ;

				//	for (; GetFred() >= g_nThreadLimit ;)
				//	sleep(2) ;

				if (pAct->CurrStatus() == HZ_ACTION_COMPLETE)
					{ schlog.Out("Nothing to do: Task %s is complete\n", *pAct->Str()) ; continue ; }
				if (pAct->CurrStatus() >= HZ_ACTION_FAILCOLLECT)
					{ schlog.Out("Nothing to do: Task %s has failed\n", *pAct->Str()) ; continue ; }

				schlog.Record("Starting jobid %s - ", *pAct->Str()) ;

				tid = 0 ;
				prc = pthread_create(&tid, &tattr, &DoTheTask, (void *) pAct) ;

				if (prc == 0)
					schlog.Out("Now running in thread %u\n", tid) ;
				else
					schlog.Out("Failed to start (error=%d)\n", prc) ;
			}
		}

		//	Have checked generate tasks, check triggers and do the plan. Sleep for 10 seconds and go round again
		schlog.Out(".") ;
		sleep(5) ;
	}

	schlog.Record("WEIRD ERROR: Exiting scheduler for some strange reason\n") ;
	schlog.Close() ;
	pthread_exit(pVoid) ;
	return pVoid ;
}
#endif

void*	ScheduleTasks	(void* pVoid)
{
	//	This is the 'main' function of the scheduling thread. It wakes up every 10 seconds or so to check for three classes of possible actions becoming due or
	//	necessary. These are:-
	//
	//		1)	Asynchonous tasks such as data collection from vendors which are time consuming and may require retries 
	//		2)	Tasks that can be performed on the basis of memory resident data and so are not exected to require retries or consume much time
	//		3)	Tasks that react to conditions such as a config file change or a problem with the server thread.
	//
	//	It loops round all active tasks to kick them off.
	//	This runs in its own thread leaving the main thread to serve web pages.

	_hzfunc("ScheduleTasks") ;

	hzVect<hzDirent>	Files ;			//	All files in the config directory

	static	int32_t	lastY = -1 ;		//	Last tested year
	static	int32_t	lastM = -1 ;		//	Last tested month
	static	int32_t	lastD = -1 ;		//	Last tested day

	hzProcess	_theSched ;				//	Thread initializer

	FSTAT		fs ;				//	File/dir status
	hzDirent	de ;				//	Directory entry
	hzXmlNode*	pRoot ;				//	Root node of document
	hdsApp*		pApp ;				//	Application
	hzString	fpath ;				//	Full file path of changed config file
	int32_t		prc ;				//	pthread return value
	int32_t		interval ;			//	Number of seconds to sleep
	int32_t		nA ;				//	Application iterator
	int32_t		n ;					//	loop iterator
	bool		bRun ;				//	Set if action runnable
	bool		bError ;			//	Error in formation of action
	hzEcode		rc = E_OK ;

	for (nA = 1 ; nA <= _hzGlobal_Dissemino->Count() ; nA++)
	{
		pApp = _hzGlobal_Dissemino->GetApplication(nA) ;

		if (!pApp->m_Configdir)
		{
			pthread_exit(pVoid) ;
			return pVoid ;
		}

		lstat(pApp->m_Configdir, &fs) ;
		pApp->m_LastCfgEpoch = fs.st_mtime ;
	}

	for (;;)
	{
		sleep(20) ;

		for (nA = 1 ; nA <= _hzGlobal_Dissemino->Count() ; nA++)
		{
			pApp = _hzGlobal_Dissemino->GetApplication(nA) ;

			//	Check if the configs directory has been updated

			lstat(pApp->m_Configdir, &fs) ;
			if (fs.st_mtime == pApp->m_LastCfgEpoch)
				continue ;

			if (fs.st_mtime < pApp->m_LastCfgEpoch)
			{
				pApp->m_pLog->Log(_fn, "Directory has regressed\n") ;
				continue ;
			}

			//	Directory has been apdate. Check for updated files
			Files.Clear() ;
			ReadDir(Files, pApp->m_Configdir, "*.xml") ;
			pApp->m_pLog->Log(_fn, "Config dir change among %d total files\n", Files.Count()) ;

			for (n = 0 ; n < Files.Count() ; n++)
			{
				de = Files[n] ;
				if (de.Mtime() > pApp->m_LastCfgEpoch)
				{
					pApp->m_pLog->Log(_fn, "NOTE: File %s has changed\n", *de.Name()) ;

					/*
					fpath = pApp->m_Configdir + "/" + de.Name() ;

					rc = doc.Load(fpath) ;
					if (rc != E_OK)
					{
						pApp->m_pLog->Log(_fn, "NOTE: Config %s did not load\n", *fpath) ;
						pApp->m_pLog->Out(doc.m_Error) ;
						continue ;
					}

					pApp->m_pLog->Log(_fn, "Loaded Config %s\n", *fpath) ;

					pRoot = doc.GetRoot() ;
					if (!pRoot)
					{
						pApp->m_pLog->Log(_fn, "NOTE: Config %s Document has no root\n", *fpath) ;
						//pApp->m_pLog->Out(m_cfgErr) ;
						//rc = E_NODATA ;
						continue ;
					}
			
					pApp->m_pLog->Log(_fn, "Parsed Config %s\n", *fpath) ;
					*/
 
					if (s_bMonitor)
					{
						rc = pApp->ReloadConfig(de.Name()) ;
						if (rc != E_OK)
						{
							pApp->m_pLog->Log(_fn, "NOTE: Failed to accept changed config %s\n", *fpath) ;
							continue ;
						}

						pApp->m_pLog->Log(_fn, "NOTE: Accepted config %s\n", *fpath) ;
					}
				}
			}

			pApp->m_LastCfgEpoch = fs.st_mtime ;
		}
	}

	pthread_exit(pVoid) ;
	return pVoid ;
}

hzTcpCode	ProcHTTP	(hzHttpEvent* pE)
{
	//	This function as it is, simply passes the supplied HTTP event through to hdsApp::ProcHTTP() to process. It exists as a function for two reason: Firstly it is needed so that
	//	hdsApp::AddPortHTTP() can be passed a pointer to it. This is because hdsApp::AddPortHTTP() expects a pointer to a function and NOT a pointer to a member function. Secondly,
	//	although no code is supplied here, code can be added to intercept particular HTTP events, either prior to being passed to hdsApp::ProcHTTP() or as an alternative to being
	//	passed to hdsApp::ProcHTTP().

	hdsApp*	pApp ;

	pApp = _hzGlobal_Dissemino->GetApplication(pE->Hostname()) ;
	if (!pApp)
	{
		threadLog("NO SUCH APP %s\n", pE->Hostname()) ;
		return TCP_TERMINATE ;
	}
	pE->m_pContextApp = pApp ;

	//	NOTE: Add any intercept code here!

	return pApp->ProcHTTP(pE) ;
}

hzTcpCode	UdpHandler	(hzChain& input, hzIpConnex*)
{
	hzString	S ;

	S = input ;
	slog.Out("NOTE: Accepted UDP message %s\n", *S) ;
}

int		main	(int argc, char ** argv)
{
	_hzfunc("dissemino::Main") ;

	FSTAT			fs ;				//	File status
	hdbObjRepos*	pRepos ;			//	Cache to look for records if we are not using pages
	hdsApp*			pApp ;				//	Dissemino application
	pthread_t		tid ;				//	Thread id
	hzString		appName ;			//	Project name for XML file
	hzString		fname ;				//	File name for XML file
	hzString		thisDir ;			//	Current directory (assumed to be where app's xml files are)
	int32_t			nArg ;				//	Argument iterator, then later use as application iterator
	int32_t			nI ;				//	Repos iterator
	bool			bDemon = false ;	//	Run as demon
	bool			bSyntax = false ;	//	Parse config then terminate
	bool			bCities = false ;	//	Load city-detail IP allocations rather than country-detail IP ranges.
	bool			bTest = false ;		//	Test mode - host the application
	bool			bDebug = false ;	//	Debug mode - extra verbose logging
	bool			bMT = false ;		//	Use multithreaded mode
	hzEcode			rc ;				//	Returns from various hdsApp function calls.

	//	Setup Signals
	signal(SIGHUP,	CatchCtrlC) ;
	signal(SIGINT,	CatchCtrlC) ;
	signal(SIGQUIT,	CatchCtrlC) ;
	signal(SIGILL,	CatchSegVio) ;
	signal(SIGTRAP,	CatchSegVio) ;
	signal(SIGABRT,	CatchSegVio) ;
	signal(SIGBUS,	CatchSegVio) ;
	signal(SIGFPE,	CatchSegVio) ;
	signal(SIGKILL,	CatchSegVio) ;
	signal(SIGUSR1,	CatchCtrlC) ;
	signal(SIGSEGV,	CatchSegVio) ;
	signal(SIGUSR2,	CatchCtrlC) ;
	signal(SIGPIPE,	SIG_IGN) ;
	signal(SIGALRM,	CatchCtrlC) ;
	signal(SIGTERM,	CatchCtrlC) ;
	signal(SIGCHLD,	SIG_IGN) ;
	signal(SIGCONT,	CatchCtrlC) ;
	signal(SIGSTOP,	CatchCtrlC) ;

	//	Process args
	if (argc == 1)
	{
		cout << "Usage: mkapp [-t(est)] [-d(emon)] [-s(yntax)] [-p(asiv)] [-r(ecord)] [-debug] -select/-epollST/-epollMT project_filename\n" ;
		return 101 ;
	}

	for (nArg = 1 ; nArg < argc ; nArg++)
	{
		if		(!strcmp(argv[nArg], "-t"))			bTest = true ;
		else if (!strcmp(argv[nArg], "-d"))			bDemon = true ;
		else if (!strcmp(argv[nArg], "-s"))			bSyntax = true ;
		else if (!strcmp(argv[nArg], "-c"))			bCities = true ;
		else if (!strcmp(argv[nArg], "-m"))			s_bMonitor = true ;
		else if (!strcmp(argv[nArg], "-strings"))	s_bExportStrings = true ;
		else if (!strcmp(argv[nArg], "-forms"))		s_bExportForms = true ;
		else if (!strcmp(argv[nArg], "-newData"))	s_bNewData = true ;
		else if (!strcmp(argv[nArg], "-debug"))		bDebug = true ;
		else if (!strcmp(argv[nArg], "-MT"))		bMT = true ;
		else
		{
			if (appName)
				cout << "Ignoring all arguments beyond the first [" << appName << "]\n" ;
			else
				appName = argv[nArg] ;
		}
	}

	cout << "collected args\n" ;

	if (!appName)
		{ cout << "mkapp: No project name supplied\n" ; return 102 ; }

	if (!appName.Contains(".xml"))
		fname = appName + ".xml" ;
	else
	{
		fname = appName ;
		appName.TruncateUpto(".xml") ;
	}

	cout << "appname is " << appName << endl ;
	cout << "config is " << fname << endl ;

	if (bSyntax)
		bDemon = false ;

	//fileLang = appName + ".lang" ;
	fileLang = appName ;

	if (bTest)
		{ fileLogs = appName + ".test" ; fileStats = appName + ".teststats" ; }
	else
		{ fileLogs = appName + ".norm" ; fileStats = appName + ".normstats" ; }

	cout << "logfile is " << fileLogs << endl ;

	if (lstat(*fname, &fs) == -1)
		{ cout << "mkapp: file [" << fname << "] does not exist\n" ; return 103 ; }

	//	Run as demon
	if (bDemon)
		Demonize() ;

	//	Open logs
	rc = slog.OpenFile(*fileLogs, LOGROTATE_NEVER) ;
	if (rc != E_OK)
		{ cout << "dissemino: logfile [" << fileLogs << "] could not be opened\n" ; return 104 ; }
	slog.Verbose(!bDemon) ;
	if (bDebug)
		_hzGlobal_Debug |= HZ_DEBUG_SERVER ;
	slog.Out("Starting Dissemino for project %s ...\n", *appName) ;

	rc = svrstats.OpenFile(*fileStats, LOGROTATE_NEVER) ;
	if (rc != E_OK)
		{ slog.Out("Server stats not opened (%s)\n", *fileStats) ; return 105 ; }

	/*
	**	Init IP Ranges
	*/

	cout << "Opening country codes\n" ;

	rc = InitCountryCodes() ;
	if (rc != E_OK)
		{ slog.Out("No Country Codes\n") ; return 105 ; }

	if (bCities)
	{
		rc = InitIpCity() ;
		if (rc != E_OK)
		{
			slog.Out("No City level IP Location table, trying basic level\n") ;
			bCities = false ;
		}
	}

	if (!bCities)
	{
		rc = InitIpBasic() ;
		if (rc != E_OK)
			slog.Out("No Basc level IP Location table\n") ;
	}

	//	Init the dictionary
	hzFsTbl::StartStrings() ;
	hzFsTbl::StartDomains() ;
	hzFsTbl::StartEmaddrs() ;
	if (!_hzGlobal_StringTable)
		{ slog.Out("Could not create string table\n") ; return 106 ; }

	//	Init MIME types
	HadronZooInitMimes() ;

	/*
	**	Init Dissemino Instance
	*/

	Dissemino::GetInstance(slog) ;
	if (!_hzGlobal_Dissemino)
		{ slog.Out("%s. No DISSEMINO\n", *_fn) ; return 109 ; }
	slog.Out("DISSEMINO Instance created\n") ;

	/*
	**	Load Dissemino Sphere
	*/

	rc = _hzGlobal_Dissemino->ReadSphere(*fname) ;
	if (rc != E_OK)
		{ slog.Out("Config failed\n") ; return 108 ; }
	slog.Out("DISSEMINO SPHERE LOADED\n") ;

	/*
	for (nArg = 1 ; nArg <= _hzGlobal_Dissemino->Count() ; nArg++)
	{
		pApp = _hzGlobal_Dissemino->GetApplication(nArg) ;

		pApp->m_ADP.InitStandard(appName) ;
		if (s_bIndex)
			pApp->m_ADP.InitSiteIndex("../data") ;
	}
	*/

	if (s_bExportStrings)
	{
		for (nArg = 1 ; nArg <= _hzGlobal_Dissemino->Count() ; nArg++)
		{
			pApp = _hzGlobal_Dissemino->GetApplication(nArg) ;

			rc = pApp->ExportStrings() ;
			if (rc != E_OK)
				{ slog.Out("%s. Failed to Export Strings (Language Support)\n", *_fn) ; return 110 ; }
			slog.Out("Language Strings Exported\n") ;
		}
	}

	for (nArg = 1 ; nArg <= _hzGlobal_Dissemino->Count() ; nArg++)
	{
		pApp = _hzGlobal_Dissemino->GetApplication(nArg) ;

		if (s_bNewData)
		{
			if (pApp->m_InitstateLoads.Count())
			{
				hzList<hdsLoad>::Iter	li ;

				hdsLoad		ld ;

				for (li = pApp->m_InitstateLoads ; li.Valid() ; li++)
				{
					ld = li.Element() ;
					rc = ld.m_pRepos->LoadCSV(ld.m_pClass, ld.m_Filepath, ",", false) ;
					if (rc != E_OK)
					{
						slog.Out("%s. Failed to Load CSV file %s (class %s) into repository %s\n",
							*_fn, *ld.m_Filepath, ld.m_pClass->TxtTypename(), *ld.m_pRepos->Name()) ;
						return 111 ;
					}
				}
			}
		}

		pApp->SetupScripts() ;
		if (rc != E_OK)
			{ slog.Out("%s. Standard Script Generation Failure\n", *_fn) ; return 113 ; }
		slog.Out("Scripts Formulated\n") ;
		/*
		if (s_bIndex)
		{
			rc = pApp->IndexPages() ;
			if (rc != E_OK)
				{ slog.Out("%s. Page Indexation Failure\n", *_fn) ; return 114 ; }
			slog.Out("Pages Indexed\n") ;
			pApp->m_PageIndex.Export("pageindex") ;
		}
		*/
	}

	//theApp->setupStdUsers() ;

	/*
	if (!theApp->m_txtCSS.Size())
		{ slog.Out("NO CSS\n") ; return 115 ; }
	*/

	for (nArg = 1 ; nArg <= _hzGlobal_Dissemino->Count() ; nArg++)
	{
		pApp = _hzGlobal_Dissemino->GetApplication(nArg) ;

		for (nI = 0 ; nI < pApp->m_ADP.CountObjRepos() ; nI++)
		{
			pRepos = pApp->m_ADP.GetObjRepos(nI) ;
			if (pRepos->InitState() == HDB_REPOS_OPEN)
				{ slog.Out("%s. Repository %s already OPEN\n", *_fn, *pRepos->Name()) ; continue ; }
			rc = pRepos->Open() ;
			if (rc != E_OK)
				{ slog.Out("%s. Could not OPEN repository %s\n", *_fn, *pRepos->Name()) ; return 116 ; }
			slog.Out("Opened Repository %s\n", *pRepos->Name()) ;
		}
		slog.Out("All Repositories Open\n") ;

		//pApp->m_ADP.appName = appName ;
		pApp->m_ADP.Export() ;
	}

	if (s_bExportForms)
	{
		for (nArg = 1 ; nArg <= _hzGlobal_Dissemino->Count() ; nArg++)
		{
			pApp = _hzGlobal_Dissemino->GetApplication(nArg) ;

			ExportForms(pApp) ;
		}
		goto endproc ;
	}

	if (bSyntax)
	{
		slog.Out("%s. Terminating after syntax only check\n", *_fn) ;
		goto endproc ;
	}

	if (bDemon)
	{
		for (nArg = 1 ; nArg <= _hzGlobal_Dissemino->Count() ; nArg++)
		{
			pApp = _hzGlobal_Dissemino->GetApplication(nArg) ;

			fileLogs = pApp->m_Logroot + "/" + appName ;

		slog.Out("Terminating logfile - Logging now in application log directories: %s\n", *fileLogs) ;
		slog.Close() ;
			slog.OpenFile(*fileLogs, LOGROTATE_DAY) ;
			slog.Verbose(!bDemon) ;
		}
	}

	if (bDebug)
	{
		_hzGlobal_StringTable->Export("allstrings") ;

		for (nArg = 1 ; nArg <= _hzGlobal_Dissemino->Count() ; nArg++)
		{
			pApp = _hzGlobal_Dissemino->GetApplication(nArg) ;

			pApp->m_PageIndex.Export("allindex") ;
		}
	}

	rc = InitIpInfo("/etc/hzDelta.d/ips") ;
	if (rc != E_OK)
		{ slog.Out("Could not init BW lists\n") ; return 118 ; }

	slog.Out("Starting Dissemino\n") ;
	slog.Out("Starting Dissemino for project %s ...\n", *appName) ;

	/*
	**	Either produce code or run test appication
	*/

	slog.Out("Project compiled successfully\n") ;

	slog.Out("Setting up Project Server\n") ;
	SetupHost() ;

	//	Init SSL
	rc = InitServerSSL(_hzGlobal_sslPvtKey, _hzGlobal_sslCert, _hzGlobal_sslCertCA, false) ;
	if (rc != E_OK)
		slog.Out("Failed to init SSL\n") ;

	//	Init MIME dataset
	//	InitMimes() ;

	//	Init connection to Delta Server
	//rc = DeltaInit(*thisDir, "dissemino", *appName, *g_Version, false) ;
	if (rc != E_OK)
		slog.Out("%s. Could not complete Delta authentication\n", *_fn) ;
	slog.Out("Delta Initialized\n") ;

	//	Create server
	theServer = hzIpServer::GetInstance(&slog) ;
	if (!theServer)
		Fatal("%s. No server\n", *_fn) ;
	slog.Out("Server obtained\n") ;
	theServer->SetStats(&svrstats) ;
	slog.Out("Stats set up\n") ;

	if (!_hzGlobal_Dissemino->m_nPortSTD)
	{
		//	Where ports are not specified for the Dissemino Sphere as a whole, the applications will specify ports individually.
		for (nArg = 1 ; nArg <= _hzGlobal_Dissemino->Count() ; nArg++)
		{
			pApp = _hzGlobal_Dissemino->GetApplication(nArg) ;

			if (theServer->AddPortHTTP(&ProcHTTP, 900, pApp->m_nPortSTD, 5, false) != E_OK)
				Fatal("%s. Could not init socket for port %d\n", *_fn, pApp->m_nPortSTD) ;
			slog.Out("HTTP Port added\n") ;

			if (theServer->AddPortHTTP(&ProcHTTP, 900, pApp->m_nPortSSL, 5, true) != E_OK)
				Fatal("%s. Could not init socket for port %d\n", *_fn, pApp->m_nPortSSL) ;
			slog.Out("HTTPS Port added\n") ;
		}
	}
	else
	{
		//	Where ports are specified for the Dissemino Sphere as a whole, all applications will use the same ports.
		if (theServer->AddPortHTTP(&ProcHTTP, 900, 80, 5, false) != E_OK)
			Fatal("%s. Could not init socket for port 80\n", *_fn) ;
		slog.Out("HTTP Port added\n") ;

		if (theServer->AddPortHTTP(&ProcHTTP, 900, 443, 5, true) != E_OK)
			Fatal("%s. Could not init socket for port 443\n", *_fn) ;
		slog.Out("HTTPS Port added\n") ;
	}

	//	if (theServer->AddPortUDP(&UdpHandler, 0, 0, 900, 20088, 5, false) != E_OK)
	//		Fatal("%s. Could not init UDP socket for port 20088\n", *_fn) ;
	//	slog.Out("UDP Port added\n") ;

	pthread_create(&tid, 0, &ScheduleTasks, 0) ;
	slog.Record("Started thread A %u\n", tid) ;

	//	Activate Server
	if (theServer->Activate() != E_OK)
	{
		slog.Out("Server class could not activate\n") ;
		return 118 ;
	}
	slog.Out("DISSEMINO SERVER ACTIVATED\n") ;

	if (!bMT)
		theServer->ServeEpollST() ;
	else
	{
		pthread_create(&tid, 0, &ThreadServeRequests, 0) ;
		slog.Record("Started thread A %u\n", tid) ;

		pthread_create(&tid, 0, &ThreadServeResponses, 0) ;
		slog.Record("Started thread B %u\n", tid) ;

		theServer->ServeEpollMT() ;
	}

endproc:
	slog.Out("HadronZoo::Dissemino SHUTDOWN\n") ;
	return 0 ;
}
