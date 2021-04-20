//
//	File:	ceComment.cpp
//
//  Legal Notice: This file is part of the HadronZoo::CodeEnforcer program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
//
//  Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//  HadronZoo::CodeEnforcer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or any later version.
//
//  The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//  as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//  HadronZoo::CodeEnforcer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with HadronZoo::CodeEnforcer. If not, see http://www.gnu.org/licenses.
//

//	This module comprises functions to lift entity descriptions from comments in the project code base. HadronZoo coding standards are assumed so comments are expected to appear:-
//
//		a) After each #define except those used to limit the recursion of #includes. These should be line comments (with //) and begin on the same line.
//
//		b) After each variable declaration (line comment)
//
//		c) Directly after the opening curly brace of a data type definition, so a class or struct, class or struct template, union or enum. This can be a block comment using either
//		the /* and */ opening and closing sequences or multiple lines begining with //. The comment is presumed to describe the data type and should state the category to which it
//		belongs. Categories are decided by the developer but are important as they improve document navigation.
//
//		d) With the exception of constructors and destructors and trivial functions that are defined inline, directly after the opening curly brace of a function. This is expected
//		to be a block comment stating the category and describing the arguments and the set of possible returns.
//
//	Note that categories are stated by a dedicated line starting with "Category:" followed by a tab. The rest of the line is the category name. Arguments are each assigned a line,
//	the first of which will either begin with "Argument:" (singular) or "Arguments:" (multiple). In the singular case this is followed by the name and description. In the multiple
//	case by 'n)' where 'n' is the argument number, followed by name and description. Tabs are used as the seperator. Something similar applies to the set of possible returns. 
//
//	Note also that data type and function definitions can be described by a very similar comment block appearing outside the definition. These however, must explicity identify the
//	described entity with an opening section indicator line.
//
//	It is recommended that external comment blocks are used to describe a group of overloaded functions. The comment can specify arguments and returns, which then serve as defaults
//	if these are not specified in the opening comment of the function. The functions should still have an opening comment if only to state why the varient exists.
//
//	External comments can also be used to direct grouping of functions or classes that are related in some way other than overloading. The comment must contain either the "FnSet:"
//	or "ClSet:" directive to state if it is functions or classes that are grouped. The functions or classes must then be listed, oe per line, within the comment.

#include <iostream>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <execinfo.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzDocument.h"
#include "hzProcess.h"

#include "enforcer.h"

using namespace std ;

/*
**	Variables
*/

extern	hzLogger	slog ;				//	Logfile
extern	ceProject	g_Project ;			//	Project (current)

/*
**	Functions
*/

hzEcode	ProcArgDesc	(hzList<hzPair>& argdesc, chIter& ci, const char* fname, uint32_t line)
{
	//	Process the arguments list section of the comment block. The argument list is indicated by the argument directive "Argument:" or "Arguments:" as described above.
	//
	//	Arguments:	1)	argdesc	List of name/value pairs being argname/description
	//				2)	ci		Comment content chain iterator
	//				3)	fname	Filename of comment
	//				4)	line	Line in file
	//
	//	Returns:	E_OK

	_hzfunc(__func__) ;

	chIter		zi ;			//	For comment block iteration
	chIter		yi ;			//	For checking if we have a continuation line, a new argument  etc
	hzChain		W ;				//	Used to build text for argument/return description
	hzPair		currArg ;		//	The current arg description
	uint32_t	expect_n = 1 ;	//	Expected argument/return number
	uint32_t	n ;				//	Argument/return number
	bool		bSingle ;		//	Sole argument
	hzEcode		rc = E_OK ;		//	Return code

	zi = ci ;

	if (zi == "Arguments:")
		bSingle = false ;
	else if (zi == "Argument:")
		bSingle = true ;
	else
		{ slog.Out("%s. Wrong Call\n", *_fn) ; return E_NOTFOUND ; }

	for (zi += 10 ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;

	//	Cope with the 'None' option
	if (zi == "None")
	{
		for (zi += 4 ; *zi && *zi != CHAR_NL ; zi++)
		ci = zi ;
		return E_OK ;
	}

	for (; !zi.eof() ;)
	{
		if (zi == "Returns:")
			break ;

		if (!bSingle)
		{
			//	Test for 'num)' sequence. If at this sequence we are at the start of an arg description and we advance the iterator beyond this to garner the
			//	name and description. If we are not at the start of a new arg desc but the continuation of the current, the pointer is not advanced
			if (IsDigit(*zi))
			{
				yi = zi ;
				for (n = 0 ; IsDigit(*yi) ; yi++)
					{ n *= 10 ; n += (*yi - '0') ; }
				if (*yi == ')')
				{
					//	At a new arg description
					if (n != expect_n)
						slog.Out("%s. WRONG_ARG Bad argument sequence file %s line %d\n", *_fn, fname, line) ;
					expect_n++ ;
					yi++ ;
					if (W.Size())
						{ currArg.value = W ; W.Clear() ; argdesc.Add(currArg) ; }
					zi = yi ;
				}
			}

			//	Pass whitespace after the n) and before the text. Add a space to the current if this is a continuation
			if (*zi == CHAR_SPACE || *zi == CHAR_TAB)
			{
				if (W.Size())
					W.AddByte(CHAR_SPACE) ;
				for (zi++ ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;
			}
		}

		//	First part should be arg name
		for (; *zi > CHAR_SPACE ; zi++)
			W.AddByte(*zi) ;
		currArg.name = W ;
		W.Clear() ;
		for (zi++ ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;

		//	Gather text upto eol
		for (; *zi && *zi != CHAR_NL ; zi++)
			W.AddByte(*zi) ;

		//	terminate if blank line
		zi++ ;
		if (*zi == CHAR_NL)
			break ;

		//	Skip whitespace to take us to the 'n)' sequence
		for (; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;
	}

	if (W.Size())
		{ currArg.value = W ; argdesc.Add(currArg) ; }

	if (!argdesc.Count())
		{ slog.Out("%s. WRONG_ARG Bad Format file %s line %d\n", *_fn, fname, line) ; return E_FORMAT ; }

	ci = zi ;
	return rc ;
}

static	hzString	str_lt_zero = "<0" ;
static	hzString	str_gt_zero = ">0" ;
static	hzString	str_plus_one = "1" ;
static	hzString	str_minus_one = "-1" ;
static	hzString	str_zero = "0" ;
static	hzString	str_positive = "0+" ;
static	hzString	str_true = "True" ;
static	hzString	str_false = "False" ;

hzEcode	ProcRetDesc	(hzList<hzPair>& retdesc, ceFunc* pFunc, chIter& ci, const char* fname, uint32_t ln)
{
	//	Process the returns list section of the comment block. The allowed formats differ depending on the return type of the function as follows:-
	//
	//	1)	Void:		Must have an entry starting with 'None' or 'None.'. This may be followed by a tab and then further commentary.
	//
	//	2)	Boolean:	Must have an entry starting with'True:' and an entry starting with 'False:'. Further commentry may follow after a tab and until the end
	//					of the line.
	//
	//	3)	Ref:		Function may be of any type but have no indirection and the attribute DT_ATTR_REFERENCE set. Only one entry required since any function
	//					returning a reference to anything, must always return a reference.
	//
	//	4)	Pointer:	Function returns a pointer to something (any type and level of indirection). There must be an entry starting with 'Pointer' and an entry
	//					starting with 'NULL'.
	//
	//	5)	Numeric:	Function returns by value, any of the fundamental signed or unsigned 64, 32, 16 or 8 bit types or a double. In the signed case there can
	//					be an entry starting with '<0' but in all other cases there will be an entry starting with '>0' and '0'. All are to be followed by a tab
	//					and a description of the applicable conditions.
	//
	//	6)	Class:		Function returns a class instance by value. Like reference, only one line.
	//
	//	Arguments:	1)	retlist	List of name/value pairs being title/description of outcome
	//

	_hzfunc(__func__) ;

	ceTyplex	ret ;			//	Return value of function if known
	chIter		zi ;			//	For comment block iteration
	chIter		yi ;			//	For checking if we have a continuation line, a new argument  etc
	hzChain		W ;				//	Used to build text for argument/return description
	hzPair		currArg ;		//	The current arg description
	uint32_t	n ;				//	Argument/return number
	hzEcode		rc = E_OK ;		//	Return code

	zi = ci ;

	if (zi != "Returns:")
		{ slog.Out("%s. Wrong Call\n", *_fn) ; return E_NOTFOUND ; }

	for (zi += 8 ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;

	//	Cope with the 'None' option
	if (zi == "None")
	{
		//	Must have return type of void with no indirection and no reference
		if (pFunc)
		{
			ret = pFunc->Return() ;

			if (ret.Type() != g_cpp_void || ret.m_Indir || ret.m_dt_attrs & DT_ATTR_REFERENCE)
			{
				slog.Out("%s %s line %d Function %s: WRONG_RET None only allowed for void\n", *_fn, fname, ln, *pFunc->Fqname()) ;
				rc = E_FORMAT ;
			}
		}

		for (zi += 4 ; *zi && *zi != CHAR_NL ; zi++)
		ci = zi ;
		return E_OK ;
	}

	if (pFunc)
	{
		ret = pFunc->Return() ;

		if (ret.m_Indir)
		{
			//	Pointer - There must be an entry starting with 'Pointer' and an entry starting with 'NULL'.
			if (zi != "Pointer" && zi != "NULL")
			{
				slog.Out("%s %s line %d Function %s: WRONG_RET func returning pointer must have entry starting with 'Pointer' or 'NULL'\n",
					*_fn, fname, ln, *pFunc->Fqname()) ;
				goto normal ;
			}
			goto conclude ;
		}

		if (ret.m_dt_attrs & DT_ATTR_REFERENCE)
		{
			//	Reference - Only one entry required since any function returning a reference to anything, must always return a reference.
			if (zi != "Reference" && zi != "Const reference")
			{
				slog.Out("%s %s line %d Function %s: WRONG_RET func returning reference must have entry starting with 'Reference' or 'Const reference'\n",
					*_fn, fname, ln, *pFunc->Fqname()) ;
				goto normal ;
			}

			for (; !zi.eof() && *zi != CHAR_NL ; zi++)
				W.AddByte(*zi) ;
			if (W.Size())
				{ currArg.value = W ; retdesc.Add(currArg) ; }
			goto conclude ;
		}

		if (ret.Type() == g_cpp_bool)
		{
			//	Boolean - Must have an entry starting with'True:' and an entry starting with 'False:'.
			if (zi != "True" && zi != "False")
			{
				slog.Out("%s %s line %d Function %s: WRONG_RET func returning bool must have entry starting with 'True' or 'False'\n",
					*_fn, fname, ln, *pFunc->Fqname()) ;
				goto normal ;
			}

			for (; !zi.eof() ; zi++)
			{
				if (*zi == CHAR_TAB || *zi == CHAR_SPACE)
					continue ;

				if		(zi == "True")	{ zi += 4 ; currArg.name = str_true ; }
				else if (zi == "False")	{ zi += 5 ; currArg.name = str_false ; }
				else
					slog.Out("%s %s line %d Function %s: WRONG_RET func returning bool must have entry starting with 'True' or 'False'\n",
						*_fn, fname, ln, *pFunc->Fqname()) ;

				for (; !zi.eof() && *zi == CHAR_TAB || *zi == CHAR_SPACE ; zi++) ;
				for (; !zi.eof() && *zi != CHAR_NL ; zi++)
					W.AddByte(*zi) ;
				if (W.Size())
					{ currArg.value = W ; W.Clear() ; retdesc.Add(currArg) ; }

				if (zi.eof())
					break ;
				if (zi == "\n\n")
					break ;
			}
			goto conclude ;
		}

		if (ret.Type() == g_cpp_void)
		{
			//	Void - Must have an entry starting with 'None'
			if (zi != "None")
			{
				slog.Out("%s %s line %d Function %s: WRONG_RET func returning void must have entry starting with 'None'\n",
					*_fn, fname, ln, *pFunc->Fqname()) ;
				goto normal ;
			}
			goto conclude ;
		}

		if (ret.Basis() == ADT_DOUBLE || ret.Basis() & CPP_DT_INT || ret.Basis() & CPP_DT_UNT)
		{
			//	Numeric - Can eithe be one entry begining with 'Number' or else an entry starting with '<0' (for signed numbers) and an entry starting with '>0' and '0'.
			if (zi == "Number" || zi == "Total" || zi == "Value" || zi == "Length" || zi == "Address")
			{
				for (; !zi.eof() && *zi != CHAR_NL ; zi++)
					W.AddByte(*zi) ;
				if (W.Size())
					{ currArg.value = W ; retdesc.Add(currArg) ; }
				goto conclude ;
			}

			if (zi != "<0" && zi != "-1" && zi != ">0" && zi != "0+" && zi != "+1" && *zi != '1' && *zi != '0')
			{
				slog.Out("%s line %d Function %s: WRONG_RET 1 func returning numeric must have entry starting with -1, <0, 1, >0 or 0\n",
					fname, ln, *pFunc->Fqname()) ;
				goto normal ;
			}

			for (; !zi.eof() ; zi++)
			{
				if (*zi == CHAR_TAB || *zi == CHAR_SPACE)
					continue ;

				if		(zi == "<0")	{ zi += 2 ; currArg.name = str_lt_zero ; }
				else if (zi == "-1")	{ zi += 2 ; currArg.name = str_minus_one ; }
				else if (zi == ">0")	{ zi += 2 ; currArg.name = str_gt_zero ; }
				else if (zi == "0+")	{ zi += 2 ; currArg.name = str_positive ; }
				else if (zi == "+1")	{ zi += 2 ; currArg.name = str_plus_one ; }
				else if (*zi == '1')	{ zi += 1 ; currArg.name = str_plus_one ; }
				else if (*zi == '0')	{ zi += 1 ; currArg.name = str_zero ; }
				else
					slog.Out("%s line %d Function %s: WRONG_RET 2 func returning numeric must have entry starting with -1, <0, 1, >0 or 0\n",
						fname, ln, *pFunc->Fqname()) ;

				for (; !zi.eof() && *zi == CHAR_TAB || *zi == CHAR_SPACE ; zi++) ;
				for (; !zi.eof() && *zi != CHAR_NL ; zi++)
					W.AddByte(*zi) ;
				if (W.Size())
					{ currArg.value = W ; W.Clear() ; retdesc.Add(currArg) ; }

				if (zi.eof())
					break ;
				if (zi == "\n\n")
					break ;
			}

			goto conclude ;
		}

		if (ret.Basis() == ADT_CLASS)
		{
			//	Handle the special case of hzEcode
			if (ret.Type()->StrName() == "hzEcode")
			{
				if (zi != "E_" && zi != "Enum")
				{
					slog.Out("%s %s line %d Function %s: WRONG_RET func returning hzEcode must have entry starting with 'E_'\n",
						*_fn, fname, ln, *pFunc->Fqname()) ;
					goto normal ;
				}
					goto normal ;
			}

			//	Class instance by value. Like reference, only one line.
			if (zi != "Instance")
			{
				slog.Out("%s %s line %d Function %s: WRONG_RET func returning class instance by vale must have entry starting with 'Instance'\n",
					*_fn, fname, ln, *pFunc->Fqname()) ;
				goto normal ;
			}

			for (; !zi.eof() && *zi != CHAR_NL ; zi++)
				W.AddByte(*zi) ;
			if (W.Size())
				{ currArg.value = W ; retdesc.Add(currArg) ; }
			goto conclude ;
		}
	}

normal:
	for (; !zi.eof() ;)
	{
		//	Test for 'num)' sequence. If at this sequence we are at the start of an arg description and we advance the iterator beyond this to garner
		//	the content. If we are not at the start of a new arg desc but the continuation of the current, the pointer is not advanced
		yi = zi ;
		if (IsDigit(*yi))
		{
			for (n = 0 ; IsDigit(*yi) ; yi++)
				{ n *= 10 ; n += (*yi - '0') ; }
			if (*yi == ')')
			{
				//	At a new arg description
				yi++ ;
				if (W.Size())
					{ currArg.value = W ; W.Clear() ; retdesc.Add(currArg) ; }
				zi = yi ;
			}
		}

		//	Pass whitespace after the n) and before the text. Add a space to the current if this is a continuation
		if (*zi == CHAR_SPACE || *zi == CHAR_TAB)
		{
			if (W.Size())
				W.AddByte(CHAR_SPACE) ;
			for (zi++ ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;
		}

		//	First part should be arg name
		for (; *zi > CHAR_SPACE ; zi++)
			W.AddByte(*zi) ;
		currArg.name = W ;
		W.Clear() ;
		for (zi++ ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;

		//	Gather text upto eol
		for (; *zi && *zi != CHAR_NL ; zi++)
			W.AddByte(*zi) ;

		//	terminate if blank line
		zi++ ;
		if (*zi == CHAR_NL)
			break ;

		//	Skip whitespace to take us to the 'n)' sequence
		for (; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;
	}

	if (W.Size())
		{ currArg.value = W ; retdesc.Add(currArg) ; }
	if (!retdesc.Count())
		{ slog.Out("%s. Bad Format\n", *_fn) ; return E_FORMAT ; }

conclude:
	ci = zi ;
	return rc ;
}

hzEcode	ceFile::ProcExtComment	(uint32_t origPosn)
{
	//	Examines an external comment block (one outside of an entity definition body) to see if it is a description of an entity or a grouping
	//	directive. In the former case, the entity description is updated. In the latter case we first look for the group name, then the category
	//	(if given) and then a series of function statements. These are of the form 'Func:' funcname function_description.
	//
	//	Arguments:	1)	origPosn	The original token in file array P

	_hzfunc("ceFile::ProcExtComment") ;

	hzList<ceFunc*>::Iter	fi ;	//	Functions iterator

	hzList<hzPair>	argdesc ;		//	List of argument description
	hzList<hzPair>	retdesc ;		//	List of return values

	hzChain::Iter	zi ;
	hzChain::Iter	zname ;

	hzChain			cmt ;			//	Comment body
	hzChain			fin ;			//	Result which will become entity description
	hzChain			W ;				//	Used to build current word
	hzString		tag ;			//	HTML5 tag/anti-tag
	hzString		order ;			//	Synopsis order
	hzString		name ;			//	Entity being commented upon
	hzString		categ ;			//	Category
	ceKlass*		pHost = 0 ;		//	Applicable class
	ceEntity*		pEnt ;			//	The entity named in the comment block
	ceFnset*		pFS ;			//	Function set
	ceFngrp*		pFgrp ;			//	Function group
	ceFunc*			pFunc ;			//	Function
	ceSynp*			pSynp ;			//	Synopsis (if applicable)
	uint32_t		ln ;			//	Line number of comment
	ceEntType		subject ;		//	Subject of comment eg class or function
	hzEcode			rc = E_OK ;

	//	Set up chain with comment content
	cmt.Clear() ;
	cmt << P[origPosn].value ;
	ln = P[origPosn].line ;
	subject = CE_UNIT_UNKNOWN ;
	zi = cmt ;
	zi.Skipwhite() ;

	//	slog.Out("%s. File %s Line %d EXT COMMENT (of %d bytes)\n[\n%s\n]\n",  *_fn, *StrName(), ln, cmt.Size(), *P[origPosn].value) ;
	//	slog.Out("%s. File %s Line %d EXT COMMENT (of %d bytes)\n",  *_fn, *StrName(), ln, cmt.Size()) ;

	//	Establish the comment subject
	if		(zi.Equiv("Class:"))		{ zi += 6 ; subject = ENTITY_CLASS ; }
	else if (zi.Equiv("Struct:"))		{ zi += 7 ; subject = ENTITY_CLASS ; }
	else if (zi.Equiv("Union:"))		{ zi += 6 ; subject = ENTITY_UNION ; }
	else if (zi.Equiv("Function:"))		{ zi += 9 ; subject = ENTITY_FUNCTION ; }
	else if (zi.Equiv("FnGrp:"))		{ zi += 8 ; subject = ENTITY_FN_OVLD ; }
	else if (zi.Equiv("FnSet:"))		{ zi += 8 ; subject = CE_UNIT_FUNC_GRP ; }
	else if (zi.Equiv("Enum:"))			{ zi += 5 ; subject = ENTITY_ENUM ; }
	else if (zi.Equiv("Synopsis:"))		{ zi += 9 ; subject = CE_UNIT_SYNOPSIS ; }
	else if (zi.Equiv("ClassGroup:"))	{ zi += 8 ; subject = CE_UNIT_CLASS_GRP ; }
	else
	{
		//	Comment has no formal subject directive and so is not processed. This is OK
		return E_OK ;
	}

	//	If the subject (entity type) is found, expect the next text to be the (fully qualified) entity name. We break this up into its parts as
	//	the scoping operator (::)

	/*
	zi.Skipwhite() ;
	for (; !zi.eof() && *zi != CHAR_NL ; zi++)
		W.AddByte(*zi) ;
	name = W ;
	W.Clear() ;
	zi.Skipwhite() ;

	if (!name)
	{
		slog.Out("%s. %s line %d: ERROR - No entity name\n", *_fn, *StrName(), ln) ;
		return E_NOTFOUND ;
	}
	*/

	/*
	**	Now process comment subject
	*/

	if (subject == CE_UNIT_SYNOPSIS)
	{
		//	Establish synopsis content (rest of lines). Note this will allow through HTML5 tags which will ultimately be treated as HTML5 tags. Other sequences
		//	that appear to be XML tags and the angle brackets are converted to HTML entities.

		//	Get synopsis order
		slog.Out("DOING SYNOP 1\n") ;
		zi.Skipwhite() ;
		for (; !zi.eof() && *zi > CHAR_SPACE ; zi++)
			W.AddByte(*zi) ;
		slog.Out("DOING SYNOP 2\n") ;
		order = W ;
		W.Clear() ;
		slog.Out("DOING SYNOP 3\n") ;

		if (!order)
			{ slog.Out("%s. %s line %d: SYNOP ERROR - No synopsis order\n", *_fn, *StrName(), zi.Line()) ; return E_NOTFOUND ; }
		slog.Out("DOING SYNOP 4\n") ;

		//	Get synopsis name (title)
		zi.Skipwhite() ;
		for (; !zi.eof() && *zi != CHAR_NL ; zi++)
			W.AddByte(*zi) ;
		slog.Out("DOING SYNOP 6\n") ;
		name = W ;
		W.Clear() ;
		zi.Skipwhite() ;
		slog.Out("DOING SYNOP 6\n") ;

		if (!name)
			{ slog.Out("%s. %s line %d: SYNOP ERROR - No synopsis name (order %s)\n", *_fn, *StrName(), zi.Line(), *order) ; return E_NOTFOUND ; }
		slog.Out("DOING SYNOP 7 %s %p\n", *name, m_pParComp) ;

		if (m_pParComp->m_mapSynopsis.Exists(name))
			{ slog.Out("%s. SYNOP ERROR REPEAT SYNOPSIS %s\n", *_fn, *name) ; return E_DUPLICATE ; }
		slog.Out("DOING SYNOP 8\n") ;

		pSynp = new ceSynp() ;
		slog.Out("DOING SYNOP 9\n") ;
		pSynp->SynopInit(this, order, name) ;
		slog.Out("DOING SYNOP 10\n") ;
		m_pParComp->m_mapSynopsis.Insert(pSynp->m_Docname, pSynp) ;
		slog.Out("DOING SYNOP 11\n") ;
		m_pParComp->m_arrSynopsis.Add(pSynp) ;

		slog.Out("DOING SYNOP %p %s [%s]\n", pSynp, *order, *name) ;

		for (; *zi && !zi.eof() ;)
		{
			//	Within a comment block, only certain HTML tags are allowed as is
			if (*zi == CHAR_LESS)
			{
				if (zi == "<b>")	{ zi += 3 ; pSynp->m_Content += "<b>" ; continue ; }
				if (zi == "<i>")	{ zi += 3 ; pSynp->m_Content += "<i>" ; continue ; }

				if (zi == "</b>")	{ zi += 4 ; pSynp->m_Content += "</b>" ; continue ; }
				if (zi == "</i>")	{ zi += 4 ; pSynp->m_Content += "</i>" ; continue ; }

				pSynp->m_Content << "&lt;" ;
				zi++ ;
				continue ;
			}

			if (*zi == CHAR_MORE)
			{
				pSynp->m_Content << "&gt;" ;
				zi++ ;
				continue ;
			}

			if (zi == "$:")
			{
				//	Heading within synopsis
				for (zi += 2 ; !zi.eof() && *zi > CHAR_SPACE ; zi++)
					W.AddByte(*zi) ;
				order = W ;
				W.Clear() ;

				if (!order)
					{ slog.Out("%s. %s line %d: case 1 SYNOP ERROR - No synopsis order\n", *_fn, *StrName(), zi.Line()) ; return E_NOTFOUND ; }

				//	Get name of sub-section
				zi.Skipwhite() ;
				for (; !zi.eof() && *zi != CHAR_NL ; zi++)
					W.AddByte(*zi) ;
				name = W ;
				W.Clear() ;

				if (!name)
					{ slog.Out("%s. %s line %d: case 2 SYNOP ERROR - No synopsis name (order %s)\n", *_fn, *StrName(), zi.Line(), *order) ; return E_NOTFOUND ; }

				pSynp->m_Content.Printf("<p id=\"%s\"><b>%s</b></p>\n", *order, *name) ;
				continue ;
			}

			if (zi == "@:")
			{
				//	Sub-section to be inserted underneath the main synopsis in accordamce with the numeric order specified

				//	Get synopsis order
				for (zi += 2 ; !zi.eof() && *zi > CHAR_SPACE ; zi++)
					W.AddByte(*zi) ;
				order = W ;
				W.Clear() ;

				if (!order)
					{ slog.Out("%s. %s line %d: case 3 SYNOP ERROR - No synopsis order\n", *_fn, *StrName(), zi.Line()) ; return E_NOTFOUND ; }

				//	Get name of sub-section
				zi.Skipwhite() ;
				for (; !zi.eof() && *zi != CHAR_NL ; zi++)
					W.AddByte(*zi) ;
				name = W ;
				W.Clear() ;

				if (!name)
					{ slog.Out("%s. %s line %d: case 4 SYNOP ERROR - No synopsis name (order %s)\n", *_fn, *StrName(), zi.Line(), *order) ; return E_NOTFOUND ; }

				if (m_pParComp->m_mapSynopsis.Exists(name))
					{ slog.Out("%s. SYNOP ERROR REPEAT SYNOPSIS %s\n", *_fn, *name) ; return E_DUPLICATE ; }

				slog.Out("DONE SYNOP %p %d bytes\n", pSynp, pSynp->m_Content.Size()) ;

				pSynp = new ceSynp() ;
				pSynp->SynopInit(this, order, name) ;
				m_pParComp->m_mapSynopsis.Insert(pSynp->m_Docname, pSynp) ;
				m_pParComp->m_arrSynopsis.Add(pSynp) ;

				slog.Out("DOING SYNOP SUB %p %s [%s]\n", pSynp, *order, *name) ;
				continue ;
			}

			pSynp->m_Content.AddByte(*zi) ;
			zi++ ;
		}

		P[origPosn].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		return E_OK ;
	}

	//	Get name
	zi.Skipwhite() ;
	for (; !zi.eof() && *zi != CHAR_NL ; zi++)
		W.AddByte(*zi) ;
	name = W ;
	W.Clear() ;
	zi.Skipwhite() ;

	if (!name)
	{
		slog.Out("%s. %s line %d: ERROR - No entity name\n", *_fn, *StrName(), ln) ;
		return E_NOTFOUND ;
	}

	//	Everything other than a synopsis may have a category
	if (zi == "Category:")
	{
		for (zi += 9 ; !zi.eof() && (*zi == CHAR_TAB || *zi == CHAR_SPACE) ; zi++) ;
		for (; !zi.eof() && *zi != CHAR_NL ; zi++)
			W.AddByte(*zi) ;
		categ = W ;
		W.Clear() ;
		zi.Skipwhite() ;
		slog.Out("%s. %s line %d set Category to [%s]\n", *_fn, *StrName(), ln, *categ) ;
	}

	if (subject == CE_UNIT_FUNC_GRP)
	{
		//	Create function-set
		pFS = new ceFnset() ;
		pFS->InitFnset(0, name) ;
		pFS->SetCateg(categ) ;
		slog.Out("%s: %s line %d Doing FnSet [%s]\n", *_fn, *StrName(), ln, *name) ;

		//	Get function set description
		fin.Clear() ;
		fin << "<p>\n" ;

		for (pEnt = 0 ; !zi.eof() ; zi++)
		{
			if (*zi == CHAR_NL)
			{
				if (zi == "\n\n")
				{
					fin << "</p>\n" ;
					zi.Skipwhite() ;
				}
			}

			if (zi == "Func:")
				break ;

			fin.AddByte(*zi) ;
		}

		//	Establish function set name, category, description and list of functions
		for (pEnt = 0 ; !zi.eof() ; zi++)
		{
			if (zi == "Arguments:" || zi == "Argument:")
				rc = ProcArgDesc(pFS->m_Argdesc, zi, *name, ln) ;

			if (zi == "Returns:")
				rc = ProcRetDesc(pFS->m_Retdesc, pFunc, zi, *name, ln) ;

			if (zi == "Func:")
			{
				//	Get function name (including the argument set)
				for (zi += 5 ; !zi.eof() && (*zi == CHAR_TAB || *zi == CHAR_SPACE) ; zi++) ;

				for (; !zi.eof() ; zi++)
				{
					if (*zi >= CHAR_SPACE)
						W.AddByte(*zi) ;
					if (*zi == CHAR_PARCLOSE)
						{ zi++ ; break ; }
				}
				//	for (; !zi.eof() && *zi >= CHAR_SPACE ; zi++)
				//		W.AddByte(*zi) ;
				name = W ;
				W.Clear() ;

				if (!name)
				{
					slog.Out("%s. %s line %d: No entity name\n", *_fn, *StrName(), ln) ;
					continue ;
					//return E_OK ;
				}

				pEnt = LookupString(pHost, name, *_fn) ;
				if (!pEnt)
				{
					slog.Out("%s. %s line %d: No such entity as %s\n", *_fn, *StrName(), ln, *name) ;
					continue ;
					//return E_OK ;
				}

				//	Set the function's set pointer to the ceFnset
				if (pEnt->Whatami() == ENTITY_FUNCTION)
				{
					pFunc = (ceFunc*) pEnt ;

					pFunc->m_pSet = pFS ;
					pFS->m_functions.Add(pFunc) ;
					slog.Out("%s. Added func %s to func_set %s\n", *_fn, *pFunc->Fqname(), *pFS->m_Title) ;
				}

				continue ;
			}

			if (*zi == CHAR_LESS)
				fin << "&lt;" ;
			else if (*zi == CHAR_MORE)
				fin << "&gt;" ;
			else
				fin.AddByte(*zi) ;
		}

		pFS->m_grpDesc = fin ;
		fin.Clear() ;
		P[origPosn].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
		return E_OK ;
	}

	//	Use name to lookup entity
	pEnt = LookupString(pHost, name, *_fn) ;
	if (!pEnt)
	{
		slog.Out("%s. %s line %d: No such entity as %s\n", *_fn, *StrName(), ln, *name) ;
		return E_OK ;
	}

	//	Does the entity match the subject?
	if (pEnt->Whatami() != subject)
	{
		slog.Out("%s. %s line %d: Entity %s is a %s instead of a %s\n",
			*_fn, *StrName(), ln, *name, Entity2Txt(pEnt->Whatami()), Entity2Txt(subject)) ;
		return E_OK ;
	}

	slog.Out("%s. %s line %d: Entity %s is a %s\n", *_fn, *StrName(), ln, *name, Entity2Txt(pEnt->Whatami())) ;

	if (subject == ENTITY_FN_OVLD)
	{
		//	After category we have the description and list of arguments and returns - just as with a function.
		pFgrp = (ceFngrp*) pEnt ;
		pFgrp->SetCateg(categ) ;

		for (; !zi.eof() ;)
		{
			if (zi == "Arguments:" || zi == "Argument:")
				rc = ProcArgDesc(pFgrp->m_Argdesc, zi, *name, ln) ;

			if (zi == "Returns:")
				rc = ProcRetDesc(pFgrp->m_Retdesc, 0, zi, *name, ln) ;

			if (*zi == CHAR_NL)
			{
				fin.AddByte(CHAR_NL) ;
				for (zi++ ; *zi == CHAR_SPACE ; zi++) ;
				for (; *zi == CHAR_FWSLASH || *zi == CHAR_ASTERISK ; zi++) ;
				for (; *zi == CHAR_SPACE ; zi++) ;
				continue ;
			}

			if (*zi)
				fin.AddByte(*zi) ;
			zi++ ;
		}
	}

	if (pEnt->Whatami() == ENTITY_FUNCTION)
	{
		pFunc = (ceFunc*) pEnt ;
		pFunc->SetCategory(categ) ;

		for (; !zi.eof() ;)
		{
			if (zi == "Arguments:" || zi == "Argument:")
				rc = ProcArgDesc(pFunc->m_Argdesc, zi, *name, ln) ;

			if (zi == "Returns:")
				rc = ProcRetDesc(pFunc->m_Retdesc, pFunc, zi, *name, ln) ;

			if (*zi == CHAR_NL)
			{
				fin.AddByte(CHAR_NL) ;
				for (zi++ ; *zi == CHAR_SPACE ; zi++) ;
				for (; *zi == CHAR_FWSLASH || *zi == CHAR_ASTERISK ; zi++) ;
				for (; *zi == CHAR_SPACE ; zi++) ;
				continue ;
			}

			if (*zi)
				fin.AddByte(*zi) ;
			zi++ ;
		}
	}

	pEnt->SetDesc(fin) ;
	//	slog.Out("%s. %s line %d: Entity %s is described as (%d bytes %d) [%s]\n", *_fn, *StrName(), ln, *pEnt->Name(), fin.Size(), cmt.Size(), *pEnt->Desc()) ;

	P[origPosn].t_cmtf |= TOK_FLAG_COMMENT_PROC ;
	return E_OK ;
}

hzEcode	ceFile::ProcCommentFunc	(ceFunc* pFunc, uint32_t pt)
{
	//	Examines a comment occuring inside a function code body. This copes with the following constructs:-
	//
	//		1)	Category:	The category (by functionality)
	//		2)	Arguments:	The list of arguments. This may be 'None' followed by optional text or if there are arguments, by one or more entries of the form:-
	//
	//			argNo) argName description ...
	//
	//		3)	Returns:	The list of possible return values. This may be 'None' in the case of void functions. Or it may be just a description where this or
	//						*this or where a reference to something is always returned. Or there can be one or more entries of the form:-
	//
	//			retNo) description OR
	//			retNo) value description
	//
	//	Arguments:	1)	pFunc	The applicable function
	//				2)	pt		The comment itself (position within array P)
	//
	//	Returns:

	_hzfunc("ceFile::ProcCommentFunc") ;

	hzList<hzPair>	arglist ;		//	Argument list
	hzList<hzPair>	retlist ;		//	Argument list

	hzList<hzPair>::Iter	pi ;	//	Arg/return list iterator

	hzPair		Pair ;				//	Arg/return list item
	ceTyplex	retTpx ;			//	Function return typlex
	hzChain		Z ;					//	For processing input
	hzChain		W ;					//	For building category
	chIter		zi ;				//	For iterating input
	hzString	S ;					//	Temp string
	uint32_t	line ;				//	Line nunber
	bool		bSuppress ;			//	Suppress messages if function is constructor/destructor/operator
	hzEcode		a_rc = E_NODATA ;	//	Return code from processing arglist
	hzEcode		r_rc = E_NODATA ;	//	Return code from processing returns

	if (!pFunc)
		hzexit(_fn, 0, E_ARGUMENT, "No function supplied") ;

	if (P[pt].type != TOK_COMMENT)
	{
		slog.Out("%s. File %s WRONG_ARG Function %s token is not a comment\n", __func__, *StrName(), *pFunc->Fqname()) ;
		return E_NODATA ;
	}

	if (!P[pt].value)
		hzerr(_fn, HZ_WARNING, E_NODATA, "No comment supplied") ;

	bSuppress = (pFunc->GetAttr() & (FN_ATTR_CONSTRUCTOR | FN_ATTR_DESTRUCTOR | FN_ATTR_OPERATOR)) ? true : false ;

	line = P[pt].line ;
	Z = P[pt].value ;
	zi = Z ;
	zi.Skipwhite() ;

	for (; !zi.eof() ;)
	{
		if (zi == "Category:")
		{
			if (pFunc->GetCategory())
				hzerr(_fn, HZ_WARNING, E_DUPLICATE, "Already have function category") ;

			for (zi += 9 ; !zi.eof() && (*zi == CHAR_TAB || *zi == CHAR_SPACE) ; zi++) ;
			for (; *zi && *zi != CHAR_NL ; zi++)
				W.AddByte(*zi) ;
			S = W ;
			pFunc->SetCategory(S) ;
			W.Clear() ;
			zi.Skipwhite() ;
		}

		if (zi == "Arguments:" || zi == "Argument:")
			a_rc = ProcArgDesc(arglist, zi, StrName(), P[pt].line) ;

		if (zi == "Returns:")
			r_rc = ProcRetDesc(retlist, pFunc, zi, StrName(), P[pt].line) ;

		if (*zi == CHAR_LESS)
			W << "&lt;" ;
		else if (*zi == CHAR_MORE)
			W << "&gt;" ;
		else
			W.AddByte(*zi) ;
		zi++ ;
	}

	S = W ;
	pFunc->SetDesc(S) ;

	/*
	**	Validate the arguments list
	*/

	if (a_rc == E_NODATA)
	{
		//	No arguments have been described in this function header comment. This could be because they have already been described in the function declaration
		//	uing a one argument per line format with comments describing the argument at the end of each line. If this has been done, the function's m_Argdesc
		//	will aready be populated.
		//
		//	Another reason may be that argument descriptions could reside with either the function group, or where applicable, the function set. The latter if
		//	it exists, takes precedence and must have argument descriptions. The function group will always exist but not nessesarily have argument descriptions

		if (!pFunc->m_pSet)
		{
			//if (pFunc->m_Args.Count() && pFunc->m_pGrp->m_Argdesc.Count() == 0 && pFunc->m_Argdesc.Count() == 0)
			if (!bSuppress)
			{
				if (pFunc->m_Args.Count() && pFunc->m_Argdesc.Count() == 0)
				{
					//	The function has arg but no arg description in this header comment - check if the function group has them and the names match
					if (pFunc->m_pGrp->m_Argdesc.Count() == 0)
						slog.Out("%s. %s line %d WRONG_ARG Function %s has no argument desc list\n", __func__, *StrName(), line, *pFunc->Fqname()) ;
				}

				if (pFunc->m_Args.Count() == 0)
					slog.Out("%s. %s line %d WRONG_ARG Function %s should state 'arguments: None'\n", __func__, *StrName(), line, *pFunc->Fqname()) ;
			}
		}
	}
	else
	{
		//	There is an argument list. Test if each one matches the actual arguments on name
		if (pFunc->m_Args.Count() == 0 && arglist.Count() > 0)
			slog.Out("%s. %s line %d WRONG_ARG Function %s has no args but %d have been described\n", __func__, *StrName(), line, *pFunc->Fqname(), arglist.Count()) ;

		if (pFunc->m_Args.Count())
		{
			if (arglist.Count() != pFunc->m_Args.Count())
			{
				slog.Out("%s. %s line %d WRONG_ARG Function %s has %d args and yet %d arg descriptions!\n",
					__func__, *StrName(), line, *pFunc->Fqname(), pFunc->m_Args.Count(), arglist.Count()) ;	//pFunc->m_Argdesc.Count()) ;

				//	for (pi = arglist ; pi.Valid() ; pi++)
				//	{
				//		Pair = pi.Element() ;
				//		slog.Out(" -> WRONG_ARG name/desc: %s/%s\n", *Pair.name, *Pair.value) ;
				//	}
			}
		}
	}

	/*
	**	Validate the returns list
	*/

	if (r_rc == E_NODATA)
	{
		if (!bSuppress)	//(pFunc->GetAttr() & (FN_ATTR_CONSTRUCTOR | FN_ATTR_DESTRUCTOR | FN_ATTR_OPERATOR)))
		{
			if (pFunc->Return().Type() == g_cpp_void && pFunc->Return().m_Indir == 0)
				slog.Out("%s. %s line %d WRONG_RET Function %s should state 'returns: None'\n", __func__, *StrName(), line, *pFunc->Fqname()) ;
			else
				slog.Out("%s. %s line %d WRONG_RET Function %s has no returns list\n", __func__, *StrName(), line, *pFunc->Fqname()) ;
		}
	}

	return E_OK ;
}

hzEcode	ceFile::ProcCommentClas	(ceClass* pClass, uint32_t pt)
{
	//	Examines an internal comment block (the first comment inside an entity definition body) to derive the category (if there is one) and the
	//	entity description. The category and description are supplied as args 1 & 2. The source comment is arg 3.

	_hzfunc("ceFile::ProcIntComment1") ;

	hzList	<hzPair>	argdesc ;	//	List of arguments

	hzChain		Z ;				//	For processing input
	hzChain		W ;				//	For building category
	chIter		zi ;			//	For iterating input
	hzString		S ;				//	Temp string
	hzEcode		rc = E_OK ;		//	Return code

	if (!pClass)
		hzexit(_fn, 0, E_ARGUMENT, "No class supplied") ;

	Z = P[pt].value ;
	zi = Z ;
	zi.Skipwhite() ;

	if (zi == "Category:")
	{
		for (zi += 9 ; !zi.eof() && (*zi == CHAR_TAB || *zi == CHAR_SPACE) ; zi++) ;
		for (; *zi && *zi != CHAR_NL ; zi++)
			W.AddByte(*zi) ;
		S = W ;
		W.Clear() ;
		zi.Skipwhite() ;
		pClass->SetCateg(S) ;
	}

	for (; !zi.eof() ;)
	{
		if (zi == "Arguments:" || zi == "Argument:")
			rc = ProcArgDesc(argdesc, zi, StrName(), P[pt].line) ;

		if (*zi == CHAR_LESS)
			W << "&lt;" ;
		else if (*zi == CHAR_MORE)
			W << "&gt;" ;
		else
			W.AddByte(*zi) ;
		zi++ ;
	}

	S = W ;
	pClass->SetDesc(S) ;
	return rc ;
}

hzEcode	ceFile::ProcCommentEnum	(ceEnum* pEnum, uint32_t pt)
{
	//	Examines an internal comment block (the first comment inside an entity definition body) to derive the category (if there is one) and the
	//	entity description. The category and description are supplied as args 1 & 2. The source comment is arg 3.

	_hzfunc("ceFile::ProcIntComment2") ;

	hzChain	Z ;			//	For processing input
	hzChain	W ;			//	For building category
	chIter	zi ;		//	For iterating input
	hzString	S ;			//	Temp string

	if (!pEnum)
		hzexit(_fn, 0, E_ARGUMENT, "No enum supplied") ;

	Z = P[pt].value ;
	zi = Z ;
	zi.Skipwhite() ;

	if (zi == "Category:")
	{
		for (zi += 9 ; !zi.eof() && (*zi == CHAR_TAB || *zi == CHAR_SPACE) ; zi++) ;
		for (; *zi && *zi != CHAR_NL ; zi++)
			W.AddByte(*zi) ;
		S = W ;
		pEnum->SetCateg(S) ;
		W.Clear() ;
		zi.Skipwhite() ;
	}

	for (; !zi.eof() ;)
	{
		if (*zi == CHAR_LESS)
			W << "&lt;" ;
		else if (*zi == CHAR_MORE)
			W << "&gt;" ;
		else
			W.AddByte(*zi) ;
		zi++ ;
	}

	S = W ;
	pEnum->SetDesc(S) ;
	return E_OK ;
}


const char*	Statement2Txt	(SType eS)
{
	static hzString	statement_illegal = "STMT_ILLEGAL" ;

	static hzString	statement_types[] =
	{
		"STMT_NULL",
		"USING",
		"NAMESPACE",
		"TYPEDEF  ",
		"CLASS_DCL",
		"CLASS_DEF",
		"CTMPL_DEF",
		"UNION_DCL",
		"UNION_DEF",
		"ENUM_DCL ",
		"ENUM_DEF ",
		"FUNC_DCL ",
		"FUNC_DEF ",
		"FTMPL_DEF",
    	"VARDCL_FNPTR",
    	"VARDCL_FNASS",
    	"VARDCL_STD  ",
    	"VARDCL_ASSIG",
    	"VARDCL_ARRAY",
    	"VARDCL_ARASS",
    	"VARDCL_CONS ",
    	"IF",
    	"ELSE",
    	"ELSEIF",
    	"FOR",
    	"DOWHILE",
    	"WHILE  ",
    	"SWITCH",
    	"CASE",
    	"VAR_INCA",
    	"VAR_INCB",
    	"VAR_DECA",
    	"VAR_DECB",
    	"VAR_ASSIGN",
    	"VAR_MATH",
    	"FUNC_CALL",
    	"DELETE",
		"CONTINUE",
		"BREAK",
		"GOTO",
		"RETURN"
	} ;

	switch	(eS)
	{
	case STMT_NULL:				return *statement_types[0] ;
	case STMT_USING:			return *statement_types[1] ;
	case STMT_NAMESPACE:		return *statement_types[2] ;
	case STMT_TYPEDEF:			return *statement_types[3] ;
	case STMT_CLASS_DCL:		return *statement_types[4] ;
	case STMT_CLASS_DEF:		return *statement_types[5] ;
	case STMT_CTMPL_DEF:		return *statement_types[6] ;
	case STMT_UNION_DCL:		return *statement_types[7] ;
	case STMT_UNION_DEF:		return *statement_types[8] ;
	case STMT_ENUM_DCL:			return *statement_types[9] ;
	case STMT_ENUM_DEF:			return *statement_types[10] ;
	case STMT_FUNC_DCL:			return *statement_types[11] ;
	case STMT_FUNC_DEF:			return *statement_types[12] ;
	case STMT_FTMPL_DEF:		return *statement_types[13] ;
    case STMT_VARDCL_FNPTR:		return *statement_types[14] ;
    case STMT_VARDCL_FNASS:		return *statement_types[15] ;
    case STMT_VARDCL_STD:		return *statement_types[16] ;
    case STMT_VARDCL_ASSIG:		return *statement_types[17] ;
    case STMT_VARDCL_ARRAY:		return *statement_types[18] ;
    case STMT_VARDCL_ARASS:		return *statement_types[19] ;
    case STMT_VARDCL_CONS:		return *statement_types[20] ;
    case STMT_BRANCH_IF:		return *statement_types[21] ;
    case STMT_BRANCH_ELSE:		return *statement_types[22] ;
    case STMT_BRANCH_ELSEIF:	return *statement_types[23] ;
    case STMT_BRANCH_FOR:		return *statement_types[24] ;
    case STMT_BRANCH_DOWHILE:	return *statement_types[25] ;
    case STMT_BRANCH_WHILE:		return *statement_types[26] ;
    case STMT_BRANCH_SWITCH:	return *statement_types[27] ;
    case STMT_BRANCH_CASE:		return *statement_types[28] ;
    case STMT_VAR_INCA:			return *statement_types[29] ;
    case STMT_VAR_INCB:			return *statement_types[30] ;
    case STMT_VAR_DECA:			return *statement_types[31] ;
    case STMT_VAR_DECB:			return *statement_types[32] ;
    case STMT_VAR_ASSIGN:		return *statement_types[33] ;
    case STMT_VAR_MATH:			return *statement_types[34] ;
    case STMT_FUNC_CALL:		return *statement_types[35] ;
    case STMT_DELETE:			return *statement_types[36] ;
	case STMT_CONTINUE:			return *statement_types[37] ;
	case STMT_BREAK:			return *statement_types[38] ;
	case STMT_GOTO:				return *statement_types[39] ;
	case STMT_RETURN:			return *statement_types[40] ;
	}

	return *statement_illegal ;
}
