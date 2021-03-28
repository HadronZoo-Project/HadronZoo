//
//	File:	ceReport.cpp
//
//  Legal Notice: This file is part of the HadronZoo::Autodoc program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
//
//  Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//  HadronZoo::Autodoc is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or any later version.
//
//  The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//  as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//  HadronZoo::Autodoc is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with HadronZoo::Autodoc. If not, see http://www.gnu.org/licenses.
//

//	Purpose:	Module to write out HTML pages and report summary for autodoc

#include <sys/stat.h>
#include <iostream>
#include <fstream>

#include <unistd.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzCtmpls.h"

#include "enforcer.h"

using namespace std ;

hzEcode	BuildHtmlTree	(void) ;

/*
**	Variables
*/

hzSet<hzString>		s_described ;		//	Described functions
hzSet<hzString>		s_undescribed ;		//	Undescribed functions
hzSet<hzString>		s_html ;			//	Articles written

extern	hdsTree		g_treeNav ;			//  The tree.
extern	ceProject	g_Project ;			//	The autodoc project

/*
**	Functions
*/

const char*	ceTypedef::DefFname	(void) const	{ return m_pFileDef ? m_pFileDef->TxtName() : 0 ; }
const char*	ceDefine::DefFname	(void) const	{ return m_pFileDef ? m_pFileDef->TxtName() : 0 ; }
const char*	ceMacro::DefFname	(void) const	{ return m_pFileDef ? m_pFileDef->TxtName() : 0 ; }

const char*	ceFunc::DclFname	(void) const	{ return m_pFileDcl ? m_pFileDcl->TxtName() : 0 ; }
const char*	ceFunc::DefFname	(void) const	{ return m_pFileDef ? m_pFileDef->TxtName() : 0 ; }


void	Clense	(hzString& S, hzChain& input)
{
	chIter		zi ;		//	Chain iterator
	hzChain		Z ;			//	Chain output
	hzString	tag ;		//	HTML tag
	uint32_t	ent ;		//	Entity value (needed by call to AtEntity)
	uint32_t	entLen ;	//	Entity length (needed by call to AtEntity)

	for (zi = input ; !zi.eof() ;)
	{
		if (*zi == CHAR_LESS)
		{
			if (zi == "</xtreeItem>")
			{
				Z << "</xtreeItem>" ;
				zi += 11 ;
				continue ;
			}
			
			if (zi == "<xtreeItem")
			{
				for (; !zi.eof() ;)
				{
					Z.AddByte(*zi) ;
					if (*zi == CHAR_DQUOTE)
					{
						for (zi++ ; !zi.eof() ; zi++)
						{
							Z.AddByte(*zi) ;
							if (*zi == CHAR_BKSLASH)
								{ zi++ ; Z.AddByte(*zi) ; }
							if (*zi == CHAR_DQUOTE)
								break ;
						}
					}
					if (*zi == CHAR_MORE)
						{ zi++ ; break ; }
					zi++ ;
				}
				continue ;
			}
 
			if (AtHtmlTag(tag, zi) != HTAG_IND_NULL)
			{
				Z += tag ;
				zi += tag.Length() ;
				continue ;
			}

			Z << "&lt;" ;
		}
		else if (*zi == CHAR_MORE)
			Z << "&gt;" ;
		else if (*zi == CHAR_AMPSAND)
		{
			if (AtEntity(ent, entLen, zi))
				Z.AddByte(*zi) ;
			else
				Z << "&amp;" ;
		}
		else if (*zi == CHAR_PERCENT)
			Z << "&#37;" ;
		else
			Z.AddByte(*zi) ;
		zi++ ;
	}

	S = Z ;
}

void	SegmentDoc	(hzChain& Z, hzChain& input)
{
	//	Break up source file as a set of <pre> tags

	_hzfunc(__func__) ;

	chIter		zi ;	//	Input iterator

	Z.Clear() ;

	Z << "<pre>\n" ;
	for (zi = input ; !zi.eof() ; zi++)
	{
		if (*zi == CHAR_CR)
			zi++ ;
		if (*zi == CHAR_NL)
		{
			zi++ ;
			if (*zi == CHAR_CR)
				zi++ ;
			if (*zi == CHAR_NL)
			{
				for (zi++ ; !zi.eof() && *zi == CHAR_CR || *zi == CHAR_NL ; zi++) ;
				Z << "\n</pre>\n<pre>\n" ;
			}
			else
				Z.AddByte(CHAR_NL) ;
			//col = 0 ;
		}

		if (!zi.eof())
			Z.AddByte(*zi) ;
		//col++ ;
	}
	Z << "</pre>\n" ;
}

void	AssembleSynop	(hzChain& X, const hzChain& Z)
{
	//	Assemble a synopsis.
	//
	//	Converts comment text into a series of paragraphs. Tabs after a newline determine indentation level but tabs occuring mid-text indicate a table is intended. On encountering
	//	a mid-text tab, the current paragraph is concluded, a new paragrah started directly after the tab or tabs, and both are marked as table cells. Marking a paragraph as a cell
	//	is a matter of increasing the cell count (normally 0). In the process the first paragraph marked by the a mid-text tab has a cell count of 1 while the next has a cell count
	//	of 2.
	//
	//	Once the comment has been broken into paragraphs and cells, it is assembled into HTML. Paragraph indentation gives rise to blockquote tags, unless any paragraph is deemed a
	//	cell in which case a table with rows and comments is constructed. Without a mid-text tab paragraphs are marked by a double newline or end of comment.

	_hzfunc("AssembleSynop") ;

	hzArray	<cePara*>	paras ;	//	Array of paragraphs in comment

	hzChain		pcon ;			//	For building paragraphs
	cePara*		pPara ;			//	Current paragraph
	chIter		zi ;			//	Chain iterator
	uint32_t	baseCol ;		//	Controls <blockquote>
	uint32_t	currCol ;		//	Controls <blockquote>
	uint32_t	nP ;			//	Paragraph loop counter
	uint32_t	nCell ;			//	Cell number
	bool		bTable ;		//	<pre> tag in force

	if (!Z.Size())
		return ;

	//	Process input into paragraphs
	zi = Z ;
	for (baseCol = 0 ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++)
		baseCol++ ;
	if (*zi == CHAR_NL)
	{
		zi++ ;
		for (baseCol = 0 ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++)
			baseCol++ ;
	}

	pPara = new cePara() ;
	pPara->m_nLine = zi.Line() ;
	pPara->m_nCol = baseCol ;

	for (; !zi.eof() ;)
	{
		if (*zi == CHAR_NL)
		{
			if (zi == "\n\n")
			{
				pPara->m_Content = pcon ;
				pcon.Clear() ;
				paras.Add(pPara) ;
				g_Project.m_AllParagraphs.Insert(pPara->m_Content, pPara) ;
				nCell = 0 ;

				for (; *zi == CHAR_NL ; zi++) ;
				for (currCol = 0 ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++)
					currCol++ ;

				if (zi == "<pre")
				{
					//	Process the <pre> block as a single paragraph

					pPara = new cePara() ;
					pPara->m_nLine = zi.Line() ;
					pPara->m_nCol = currCol ;

					pcon << "<pre" ;
					for (zi += 4 ; !zi.eof() ; zi++)
					{
						if (zi == CHAR_LESS)
						{
							if (zi == "</pre>\n")
								{ zi += 7 ; pcon << "</pre>\n" ; break ; }
						}
						pcon.AddByte(*zi) ;
					}

					pPara->m_Content = pcon ;
					pcon.Clear() ;
					paras.Add(pPara) ;
					g_Project.m_AllParagraphs.Insert(pPara->m_Content, pPara) ;

					for (; *zi == CHAR_NL ; zi++) ;
					for (currCol = 0 ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++)
						currCol++ ;
				}

				//	Line up next paragraph
				if (!zi.eof())
				{
					pPara = new cePara() ;
					pPara->m_nLine = zi.Line() ;
					pPara->m_nCol = currCol ;
				}
				continue ;
			}

			for (zi++ ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;
			if (!zi.eof())
				pcon.AddByte(CHAR_NL) ;
			continue ;
		}

		if (*zi == CHAR_TAB)
		{
			//	A tab in mid line means paragraph is a table cell

			nCell++ ;
			pPara->m_Content = pcon ;
			pPara->m_nCell = nCell ;
			pcon.Clear() ;
			paras.Add(pPara) ;
			g_Project.m_AllParagraphs.Insert(pPara->m_Content, pPara) ;

			for (; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;
			if (!zi.eof())
			{
				pPara->next = new cePara() ;
				pPara = pPara->next ;
				pPara->m_nCell = nCell + 1 ;
				pPara->m_nLine = zi.Line() ;
				pPara->m_nCol = zi.Col() ;
				//pPara->m_File = fname ;
			}
			continue ;
		}

		pcon.AddByte(*zi) ;
		zi++ ;
	}

	if (pcon.Size())
	{
		pPara->m_Content = pcon ;
		pcon.Clear() ;
		paras.Add(pPara) ;
		g_Project.m_AllParagraphs.Insert(pPara->m_Content, pPara) ;
	}

	//	Assemble the HTML
	pPara = paras[0] ;
	bTable = false ;
	currCol = baseCol ;

	for (nP = 0 ; nP < paras.Count() ; nP++)
	{
		pPara = paras[nP] ;

		if (pPara->m_nCell)
		{
			if (!bTable)
			{
				X << "<table>\n" ;
				X << "\t<tr>\n" ;
				bTable = true ;
				nCell = 1 ;
			}

			if (pPara->m_nCell == 1 && nCell > 1)
			{
				X << "\t</tr>\n\t<tr>\n" ;
				nCell = 1 ;
			}
			nCell = pPara->m_nCell ;

			X << "\t\t<td valign=\"top\">" << pPara->m_Content << "</td>\n" ;
		}
		else
		{
			if (bTable)
			{
				X << "\t</tr>\n</table>\n\n" ;
				bTable = false ;
			}

			for (; pPara->m_nCol > currCol ;)
			{
				currCol += 4 ;
				//X << "<blockquote>\n" ;
			}

			for (; pPara->m_nCol < currCol ;)
			{
				currCol -= 4 ;
				//X << "</blockquote>\n" ;
			}

			X << "<p>\n" ;
			X << pPara->m_Content ;
			X << "\n</p>\n" ;
		}
	}

	//	Clean up state
	for (; currCol > baseCol ;)
	{
		currCol-- ;
		//X << "</blockquote>\n" ;
	}

	if (bTable)
	{
		X << "\t</tr>\n</table>\n\n" ;
		bTable = false ;
	}
}

/*
**	Make Page functions
*/

void	DescribeX	(hzChain& C, hzString& desc)
{
	const char*	z ;

	if (!desc)
		C << "<p><b>Description:</b> Not available</p>\n" ;
	else
	{
		C << "<p><b>Description:</b></p>\n" ;

		C << "<p>\n" ;
		for (z = *desc ; *z ; z++)
		{
			if (*z == CHAR_LESS)
				C << "&lt;" ;
			else if (*z == CHAR_MORE)
				C << "&gt;" ;
			else if (*z == CHAR_NL && z[1] == CHAR_NL)
				{ z++ ; C << "\n</p>\n<p>\n" ; }
			else
				C.AddByte(*z) ;
		}
		C << "\n</p>\n" ;
	}
}

hdsArticleStd* ceComp::MakeProjFile	(void)
{
	//	Write out page describing project component

	_hzfunc("ceComp::MakeProjFile") ;

	hzChain			C ;			//	Chain for building output
	hdsArticleStd*	pt ;		//	Node added

	pt = g_treeNav.AddItem(0, m_Docname, m_Title, true) ;
	if (!m_Desc)
	{
		slog.Out("%s. WARNING article id %s is empty\n", *_fn, *m_Name) ;
		return pt ;
	}

	slog.Out("%s. Adding desc of %d bytes to articles sum\n", *_fn, m_Desc.Length()) ;

	//	Write the <xtreeitem> tag
	pt->m_Content = m_Desc ;
	return pt ;
}

hzEcode	ceComp::MakePageFile	(hdsArticleStd* addPt, ceFile* pFile)
{
	//	Create HTML page for a listing of a header or source file

	_hzfunc("ceComp::MakePageFile") ;

	hzChain			C ;		//	Chain for building output
	hzChain			X ;		//	Chain for building output
	hdsArticleStd*	pt ;	//	Tree item added

	if (!pFile)	Fatal("%s. No file supplied\n", *_fn) ;
	if (!addPt)	Fatal("%s. No tree add point supplied\n", *_fn) ;

	if (!pFile->m_Content)
		{ slog.Out("%s. WARNING file %s is empty\n", *_fn, *pFile->StrName()) ; return E_OK ; }

	if (s_html.Exists(pFile->m_Docname))
		slog.Out("%s. DUPLICATE_ARTICLE %s\n", *_fn, *pFile->m_Docname) ;
	s_html.Insert(pFile->m_Docname) ;

	//	Note as a file cannot have child nodes we don't need to keep the pointer to the tree item
	pt = g_treeNav.AddItem(addPt, pFile->m_Docname, pFile->StrName(), false) ;

	//	Write the <xtreeitem> tag
	XmlCleanHtags(C, pFile->m_Content) ;
	SegmentDoc(X, C) ;
	Clense(pt->m_Content, X) ;

	return E_OK ;
}

void	_mkfuncProto	(hzChain& Z, ceFunc* pFunc)
{
	//	Support function to MakePageClass. Appends the supplied chain with HTML showing the prototypes for functions in a function group as part of the HTML describing the class.
	//
	//	Args:	1)	The build chain
	//			2)	The function group
	//			3)	The <div> counter
	//			4)	Expand only if the function has a page in its own right

	_hzfunc(__func__) ;

	hzList<ceVar*>::Iter	vi ;	//	Argument iterator

	ceVar*		pArg ;				//	Argument iterator
	uint32_t	n ;					//	Arg counter
	bool		bPrint = false ;	//	True if printable

	if (!pFunc)
		Fatal("%s: No function provided", __func__) ;

	g_EC[_hzlevel()].Clear() ;

	if (!pFunc->ParComp())
	{
		g_EC[_hzlevel()].Printf("%s: NO_COMP: No parent project component provided for %s %s id=%u\n", __func__, pFunc->EntDesc(), *pFunc->Fqname(), pFunc->GetUEID()) ;
		slog.Out(g_EC[_hzlevel()]) ;
		return ;
	}
	//slog.Out("%s: Making prototype for function %s\n", __func__, *pFunc->Fqname()) ;

	//	First show return and name of function

	if (pFunc->Artname() && pFunc->GetAttr() & EATTR_PRINTABLE)
	{
		bPrint = true ;

		Z << "<tr>" ;
		Z.Printf("<td><a href=\"#\" onclick=\"loadArticle('%s-%s');\">%s</a></td>", *pFunc->ParComp()->StrName(), *pFunc->Artname(), *pFunc->Return().Str()) ;
		Z.Printf("<td><a href=\"#\" onclick=\"loadArticle('%s-%s');\">%s</a></td>", *pFunc->ParComp()->StrName(), *pFunc->Artname(), *pFunc->StrName()) ;
		Z.Printf("<td><a href=\"#\" onclick=\"loadArticle('%s-%s');\">(", *pFunc->ParComp()->StrName(), *pFunc->Artname()) ;
	}
	else
	{
		Z << "<tr>" ;
		Z.Printf("<td>%s</td>", *pFunc->Return().Str()) ;
		Z.Printf("<td>%s</td>", *pFunc->StrName()) ;
		Z << "<td>(" ;
	}

	n = pFunc->m_Args.Count() ;
	if (!n)
		Z << "void)" ;
	else
	{
		for (n--, vi = pFunc->m_Args ; vi.Valid() ; n--, vi++)
		{
			pArg = vi.Element() ;

			Z << pArg->Typlex().Str() ;
			if (pArg->StrName())
				Z << " " << pArg->StrName() ;

			Z << (n ? ", " : ")") ;
		}
	}

	if (bPrint)
		Z << "</a>" ;
	Z.Printf("</td><td>%s</td></tr>\n", *pFunc->StrDesc()) ;
}

hzEcode	ceComp::MakePageCtmpl	(hdsArticleStd* addPt, ceClass* pCtmpl)
{
	//	Make a HTML page describing a class template.
	//
	//	In a class template, methods are all declared and defined in the same file - so the test for this is diabled. Only the usual filter of 'internal' is applied.

	_hzfunc("ceComp::MakePageCtmpl") ;

	hzList<ceFunc*>::Iter	fi ;	//	Iterator for member functions

	hzList	<ceFunc*>	con ;		//	List of constructors
	hzList	<ceFunc*>	des ;		//	List of destructors
	hzList	<ceFunc*>	nor ;		//	List of normal methods
	hzList	<ceFunc*>	ops ;		//	List of operator functions
	hzList	<ceFunc*>	sta ;		//	List of static functions
	hzList	<ceFunc*>	fri ;		//	List of friend functions

	hzChain			C ;				//	Page buffer
	ceEntity*		pE ;			//	Function pointer
	hdsArticleStd*	pt1 ;			//	Tree item pointer for level+1 items
	ceClass*		pSubClass ;		//	Sub-class if any
	ceFunc*			pFunc ;			//	Function pointer
	ceVar*			pVar ;			//	Variable pointer
	hzString		Title ;			//	For incl struct/class in title
	uint32_t		nIndex ;		//	General iterator

	if (!pCtmpl)	Fatal("%s. No class template supplied\n", *_fn) ;
	if (!addPt)		Fatal("%s. No tree add point supplied\n", *_fn) ;

	/*
 	**	Exclude classes which are internal and without a category
 	*/

	if (pCtmpl->GetAttr() & EATTR_INTERNAL)
	{
		if (!pCtmpl->Category())
			{ slog.Out("%s. NOTE: Class template %s is internal and not categorized - NO_PAGE!\n", *_fn, *pCtmpl->StrName()) ; return E_OK ; }
	}

	if (!pCtmpl->Category())
		slog.Out("%s. WARNING: Class template %s is not categorized\n", *_fn, *pCtmpl->StrName()) ;

	if (s_html.Exists(pCtmpl->Artname()))
		slog.Out("%s. DUPLICATE_ARTICLE %s\n", *_fn, *pCtmpl->Artname()) ;
	s_html.Insert(pCtmpl->Artname()) ;

	pt1 = g_treeNav.AddItem(addPt, pCtmpl->Artname(), pCtmpl->StrName(), false) ;

	//	Deal with sub-classes first by recursion
	for (nIndex = 0 ; (pE = pCtmpl->GetEntity(nIndex)) ; nIndex++)
	{
		//	Bypass non-class entities in this class scope
		if (pE->Whatami() != ENTITY_CLASS)
			continue ;

		//	Bypass the entry for this class in it's own scope
		if (pE == pCtmpl)
			continue ;

		//	if (pE->Component() != this)
		//		continue ;

		pSubClass = (ceClass *) pE ;
		MakePageClass(pt1, pSubClass) ;
	}

	//	Now proceed to process the supplied class

	Title = pCtmpl->GetAttr() & CL_ATTR_STRUCT ? "struct " : "class " ;
	Title += pCtmpl->StrName() ;

	C << "<p><b>Defined and implimented in file:</b> " << pCtmpl->DefFile()->StrName() << "</p>\n" ;

	C << pCtmpl->StrDesc() ;

	/*
	**	Sort class/struct methods into categories
	*/

	//	Group functions according to general type
	for (nIndex = 0 ; (pE = pCtmpl->GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->Whatami() != ENTITY_FUNCTION)
			continue ;

		pFunc = (ceFunc*) pE ;

		if (pFunc->GetAttr() & FN_ATTR_CONSTRUCTOR)	{ con.Add(pFunc) ; continue ; }
		if (pFunc->GetAttr() & FN_ATTR_DESTRUCTOR)	{ des.Add(pFunc) ; continue ; }
		if (pFunc->GetAttr() & FN_ATTR_FRIEND)		{ fri.Add(pFunc) ; continue ; }
		if (pFunc->GetAttr() & FN_ATTR_OPERATOR)	{ ops.Add(pFunc) ; continue ; }
		if (pFunc->GetAttr() & FN_ATTR_STATIC)		{ sta.Add(pFunc) ; continue ; }

		nor.Add(pFunc) ;
		MakePageFunc(pt1, pFunc) ;
	}

	/*
	**	Table of member functions
	*/

	//	Constructors/Destructors
	C << "<p><b>Constructors/Destructors</b></p>\n" ;
	C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;

	if (con.Count() == 0)
	{
		C.Printf("<tr><td>Default constructor</td><td>%s()</td><td>Not specified in code. Default applies</td></tr>\n", *pCtmpl->StrName()) ;
		C.Printf("<tr><td>Copy constructor</td><td>%s(%s&amp;)</td><td>Not specified in code. Default applies</td></tr>\n", *pCtmpl->StrName()) ;
	}
	else
	{
		slog.Out("%s: Have %d constructors for ctmpl %s\n", *_fn, con.Count(), *pCtmpl->StrName()) ;
		for (fi = con ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
	}

	if (des.Count() == 0)
		C.Printf("<tr><td>Destructor</td><td>%s()</td><td>Not specified in code. Default applies</td></tr>\n", *pCtmpl->StrName()) ;
	else
	{
		slog.Out("%s: Have %d de-structors for ctmpl %s\n", *_fn, des.Count(), *pCtmpl->StrName()) ;
		for (fi = des ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
	}
	C << "</table>\n" ;

	//	Member functions
	if (nor.Count())
	{
		slog.Out("%s: Have %d std methods  for ctmpl %s\n", *_fn, nor.Count(), *pCtmpl->StrName()) ;
		C << "<p><b>Methods (member functions):</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		for (fi = nor ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	//	operators
	if (ops.Count())
	{
		slog.Out("%s: Have %d operators    for ctmpl %s\n", *_fn, ops.Count(), *pCtmpl->StrName()) ;
		C << "<p><b>Overloaded operators:</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		for (fi = ops ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	//	Static functions if any
	if (sta.Count())
	{
		slog.Out("%s: Have %d static funcs for ctmpl %s\n", *_fn, sta.Count(), *pCtmpl->StrName()) ;
		C << "<p><b>Static functions:</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		for (fi = sta ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	//	Friend functions if any
	if (fri.Count())
	{
		slog.Out("%s: Have %d friend funcs for ctmpl %s\n", *_fn, fri.Count(), *pCtmpl->StrName()) ;
		C << "<p><b>Friend functions:</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		for (fi = fri ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	/*
	**	List member variables
	*/

	hzVect<ceVar*>	tmpAr ;

	for (nIndex = 0 ; (pE = pCtmpl->GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->Whatami() != ENTITY_VARIABLE)
			continue ;

		pVar = (ceVar*) pE ;
		tmpAr.Add(pVar) ;
	}

	if (!tmpAr.Count())
		C << "<p>This class has no member variables.</p>\n" ;
	else
	{
		C <<
		"<p>Member Variables</p>\n"
		"<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		
		for (nIndex = 0 ; nIndex < tmpAr.Count() ; nIndex++)
		{
			pVar = tmpAr[nIndex] ;

			C.Printf("<tr><td width=\"100\">%s</td><td width=\"100\">%s</td>", *pVar->Typlex().Str(), *pVar->StrName()) ;
			if (pVar->StrDesc())
				C << "<td width=\"10\"></td><td width=\"400\">" << *pVar->StrDesc() << "</td></tr>\n" ;
			else
				C << "<td width=\"10\"></td><td width=\"400\">No description</td></tr>\n" ;
		}

		C << "</table>\n\n" ;
	}

	//	Write out article
	Clense(pt1->m_Content, C) ;

	return E_OK ;
}

hzEcode	ceComp::MakePageClass	(hdsArticleStd* addPt, ceClass* pClass)
{
	//	Make a HTML page describing a class

	_hzfunc("ceComp::MakePageClass") ;

	hzMapS	<hzString,ceFngrp*>	todo ;	//	List of functions to make page for

	hzList	<ceClass*>::Iter	si ;	//	Sub-class iterator
	hzList	<ceFunc*>::Iter		fi ;	//	Iterator for member functions

	hzVect	<ceVar*>	tmpAr ;			//	For collating variables
	hzList	<ceClass*>	subs ;			//	List of sub-classes
	hzList	<ceFunc*>	con ;			//	List of private methods
	hzList	<ceFunc*>	des ;			//	List of private methods
	hzList	<ceFunc*>	pri ;			//	List of private methods
	hzList	<ceFunc*>	hin ;			//	List of methods defined 'inline' within class def
	hzList	<ceFunc*>	nor ;			//	List of normal methods
	hzList	<ceFunc*>	ops ;			//	List of operator functions
	hzList	<ceFunc*>	sta ;			//	List of static functions
	hzList	<ceFunc*>	fri ;			//	List of friend functions

	hzChain			X ;					//	Page buffer
	hzChain			C ;					//	Page buffer
	hzString		Title ;				//	Struct/Class thing
	hdsArticleStd*	pt1 = 0 ;			//	Tree item pointer for level+1 items
	ceEntity*		pE ;				//	Function pointer
	ceClass*		pSubClass ;			//	Sub-class if any
	ceFngrp*		pGrp ;				//	Function group pointer
	ceFunc*			pFunc ;				//	Function pointer
	ceVar*			pVar ;				//	Variable pointer
	uint32_t		nCount ;			//	Counter
	uint32_t		nIndex ;			//	General iterator

	if (!pClass)	Fatal("%s. No class supplied\n", *_fn) ;
	if (!addPt)		Fatal("%s. No tree add point supplied\n", *_fn) ;

 	//	Exclude classes which are internal and without a category
	if (pClass->GetAttr() & EATTR_INTERNAL)
	{
		if (!pClass->Category())
			{ slog.Out("%s. NOTE Class %s is internal and not categorized - NO_PAGE!\n", *_fn, *pClass->StrName()) ; return E_OK ; }
	}

	if (!pClass->Category())
		slog.Out("%s. SEVERE WARNING Class %s is not categorized\n", *_fn, *pClass->StrName()) ;

	//	Provide Class Intro, state sources and base class if any
	if (s_html.Exists(pClass->Artname()))
		slog.Out("%s. DUPLICATE_ARTICLE %s\n", *_fn, *pClass->Artname()) ;
	s_html.Insert(pClass->Artname()) ;

	slog.Out("%s. ADDING CLASS TREE ITEM WITH REFNAME %s HDLN %s\n", *_fn, *pClass->Artname(), *pClass->StrName()) ;
	pt1 = g_treeNav.AddItem(addPt, pClass->Artname(), pClass->StrName(), false) ;

	Title = pClass->GetAttr() & CL_ATTR_STRUCT ? "struct " : "class " ;
	if (pClass->GetAttr() & CL_ATTR_TEMPLATE)
		Title += "template " ;
	Title += pClass->StrName() ;

	if (pClass->GetAttr() & CL_ATTR_TEMPLATE)
		C << "<p><b>Templated class, defined and implimented in file:</b> " << pClass->DefFile()->StrName() ;
	else
		C << "<p><b>Defined in file:</b> " << pClass->DefFile()->StrName() ;
	C << "</p>\n" ;

	if (pClass->Base())
		C << "<p><b>Derivative of:</b> " << pClass->Base()->StrName() << "</p>\n\n" ;
	C << "<p>\n" << pClass->StrDesc() << "</p>\n" ;

	/*
	**	Deal with sub-classes. In theory a sub-class could itself have sub-classes although there are no known instances of this in the HadronZoo C++
	**	class library or any of the applications accompanying its release. So we ignore this and just generate HTML for the sub-class in the same page
	**	as the host class. We first iterate the entities to see if there are subclasses and if the are we add some blurb about 'this class employs the
	**	following sub-class(es)'
	*/

	for (nIndex = 0 ; (pE = pClass->GetEntity(nIndex)) ; nIndex++)
	{
		//	Bypass non-class entities in this class scope and the entry for this class in it's own scope
		if (pE->Whatami() != ENTITY_CLASS)
			continue ;
		if (pE == pClass)
			continue ;

		//	Count sub-classes
		pSubClass = (ceClass *) pE ;

		if (pSubClass->GetAttr() & EATTR_INTERNAL)
			{ subs.Add(pSubClass) ; continue ; }

		if (pSubClass->GetScope() != SCOPE_PUBLIC)
			{ subs.Add(pSubClass) ; continue ; }

		MakePageClass(pt1, pSubClass) ;
	}

	if (subs.Count())
	{
		if (subs.Count() > 1)
			C << "<p>This class employes the folowing private sub-classes</p>\n" ;
		else
			C.Printf("<p>This class employes the private sub-class %s as follows:-</p>\n", *pSubClass->StrName()) ;

		//	Make HTML for non-public sub-classes
		for (si = subs ; si.Valid() ; si++)
		{
			pSubClass = si.Element() ;
			C.Printf("<p>%s</p>\n", *pSubClass->StrName()) ;
		}
	}

	/*
	**	Table of member functions
	*/

	//	Sort class/struct methods into categories
	for (nIndex = 0 ; (pE = pClass->GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->Whatami() != ENTITY_FN_OVLD)
			continue ;

		pGrp = (ceFngrp*) pE ;

		//	Count out number printable
		nCount = 0 ;
		for (fi = pGrp->m_functions ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;

			//	Divide the functions by attribute
			if		(pFunc->GetAttr() & FN_ATTR_CONSTRUCTOR)	con.Add(pFunc) ;
			else if (pFunc->GetAttr() & FN_ATTR_DESTRUCTOR)		des.Add(pFunc) ;
			else if (pFunc->GetAttr() & FN_ATTR_FRIEND)			fri.Add(pFunc) ;
			else if (pFunc->GetAttr() & FN_ATTR_OPERATOR)		ops.Add(pFunc) ;
			else if (pFunc->GetAttr() & FN_ATTR_STATIC)			sta.Add(pFunc) ;
			else
			{
				if (pFunc->GetScope() == SCOPE_PRIVATE)
					pri.Add(pFunc) ;
				else
					nor.Add(pFunc) ;
			}

			//	Count printable
			if (pFunc->GetAttr() & EATTR_INTERNAL)
				continue ;
			if (pFunc->DefFname() && pFunc->DefFname() == pFunc->DclFname())
				continue ;
			pFunc->SetAttr(EATTR_PRINTABLE) ;
			nCount++ ;
		}

		if (!nCount)
			continue ;

		if (nCount > 1)
				pGrp->SetAttr(EATTR_GRPSOLO) ;
		pGrp->SetAttr(EATTR_PRINTABLE) ;
		todo.Insert(pGrp->StrName(), pGrp) ;
	}

	//	Divide the functions by attribute
	for (nIndex = 0 ; (pE = pClass->GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->Whatami() != ENTITY_FN_OVLD)
			continue ;

		pGrp = (ceFngrp*) pE ;
		if (!(pGrp->GetAttr() & EATTR_PRINTABLE))
			continue ;

		if (pGrp->GetAttr() & EATTR_GRPSOLO)
		{
			for (fi = pGrp->m_functions ; fi.Valid() ; fi++)
			{
				pFunc = fi.Element() ;
				if (pFunc->GetAttr() & EATTR_PRINTABLE)
				{
					MakePageFunc(pt1, pFunc) ;
					pFunc->SetAttr(EATTR_PRINTDONE) ;
					break ;
				}
			}
		}
		else
		{
			for (fi = pGrp->m_functions ; fi.Valid() ; fi++)
			{
				pFunc = fi.Element() ;
				if (pFunc->GetAttr() & EATTR_PRINTABLE)
				{
					MakePageFunc(pt1, pFunc) ;
					pFunc->SetAttr(EATTR_PRINTDONE) ;
				}
			}
		}
	}

	/*
	**	Table of member functions
	*/

	//	Constructors/Destructors
	C << "<p><b>Constructors/Detructors</b></p>\n" ;
	C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;

	if (!con.Count())
	{
		C.Printf("<tr><td>Default constructor</td><td>%s()</td><td>Not specified in code. Default applies</td></tr>\n", *pClass->StrName()) ;
		C.Printf("<tr><td>Copy constructor</td><td>%s(&amp;)</td><td>Not specified in code. Default applies</td></tr>\n", *pClass->StrName()) ;
	}
	else
	{
		slog.Out("%s: Have %d constructors for class %s\n", *_fn, con.Count(), *pClass->StrName()) ;
		for (fi = con ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
	}

	if (!des.Count())
		C.Printf("<tr><td>Default constructor</td><td>%s()</td><td>Not specified in code. Default applies</td></tr>\n", *pClass->StrName()) ;
	else
	{
		slog.Out("%s: Have %d de-structors for class %s\n", *_fn, des.Count(), *pClass->StrName()) ;
		for (fi = des ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
	}
	C << "</table>\n" ;

	//	Private Member functions
	if (pri.Count())
	{
		slog.Out("%s: Have %d pri methods  for class %s\n", *_fn, pri.Count(), *pClass->StrName()) ;
		C << "<p><b>Private Member Functions</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;

		for (fi = pri ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	//	Public Member functions
	if (nor.Count())
	{
		slog.Out("%s: Have %d std methods  for class %s\n", *_fn, nor.Count(), *pClass->StrName()) ;
		C << "<p><b>Public Methods:</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;

		for (fi = nor ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	//	operators
	if (ops.Count())
	{
		slog.Out("%s: Have %d operators    for class %s\n", *_fn, ops.Count(), *pClass->StrName()) ;
		C << "<p><b>Overloaded operators:</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		for (fi = ops ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	//	Static functions if any
	if (sta.Count())
	{
		slog.Out("%s: Have %d static funcs for class %s\n", *_fn, sta.Count(), *pClass->StrName()) ;
		C << "<p><b>Static functions:</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		for (fi = sta ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	//	Friend functions if any
	if (fri.Count())
	{
		slog.Out("%s: Have %d friend funcs for class %s\n", *_fn, fri.Count(), *pClass->StrName()) ;
		C << "<p><b>Friend functions:</b></p>\n" ;
		C << "<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		for (fi = fri ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;
			_mkfuncProto(C, pFunc) ;
		}
		C << "</table>\n" ;
	}

	/*
	**	Table/list of member variables
	*/

	for (nIndex = 0 ; (pE = pClass->GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->Whatami() != ENTITY_VARIABLE)
			continue ;

		pVar = (ceVar*) pE ;
		tmpAr.Add(pVar) ;
	}

	if (!tmpAr.Count())
		C << "<p><b>Member Variables:</b> None.</p>\n" ;
	else
	{
		C <<
		"<p><b>Member Variables:</b></p>\n"
		"<table width=\"96%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"fld\">\n" ;
		
		for (nIndex = 0 ; nIndex < tmpAr.Count() ; nIndex++)
		{
			pVar = tmpAr[nIndex] ;

			C.Printf("<tr><td width=\"100\">%s</td><td width=\"100\">%s</td>", *pVar->Typlex().Str(), *pVar->StrName()) ;
			if (pVar->StrDesc())
				C << "<td width=\"10\"></td><td width=\"400\">" << pVar->StrDesc() << "</td></tr>\n" ;
			else
				C << "<td width=\"10\"></td><td width=\"400\">No description</td></tr>\n" ;
		}

		C << "</table>\n\n" ;
	}

	//	Write out the <xtreeitem> tag
	Clense(pt1->m_Content, C) ;

	return E_OK ;
}

hzEcode	ceComp::MakePageEnum	(hdsArticleStd* addPt, ceEnum* pEnum)
{
	//	Make a HTML page describing an enum

	_hzfunc("ceComp::MakePageEnum") ;

	hzChain			X ;				//	For building HTML page
	hzChain			C ;				//	For building HTML main content (article)
	hzString		Filename ;
	hzString		Title ;
	ceEnumval*		pEV ;
	hdsArticleStd*	pt ;			//	Tree item added
	uint32_t		nIndex ;

	if (!pEnum)	Fatal("%s. No enum supplied\n", *_fn) ;
	if (!addPt)	Fatal("%s. No tree add point supplied\n", *_fn) ;

	/*
 	**	Exclude classes which are internal and without a category
 	*/

	if (pEnum->GetAttr() & EATTR_INTERNAL)
	{
		if (!pEnum->Category())
			{ slog.Out("%s. NOTE Enum %s is internal and not categorized - NO_PAGE!\n", *_fn, *pEnum->StrName()) ; return E_OK ; }
	}

	if (!pEnum->Category())
		slog.Out("%s. WARNING: Enum %s is not categorized\n", *_fn, *pEnum->StrName()) ;

	if (!pEnum->DefFile())
	{
		slog.Out("%s. SEVERE: Enum %s does not hava a file in which it is defined\n", *_fn, *pEnum->StrName()) ;
		return E_NODATA ;
	}

	if (s_html.Exists(pEnum->Artname()))
		slog.Out("%s. DUPLICATE_ARTICLE %s\n", *_fn, *pEnum->Artname()) ;
	s_html.Insert(pEnum->Artname()) ;

	pt = g_treeNav.AddItem(addPt, pEnum->Artname(), pEnum->StrName(), false) ;

	Title = "enum " ;
	Title += pEnum->StrName() ;

	/*
	**	Begin HTML
	*/

	if (pEnum->DefFile())
		C << "<p>Defined in header file: " << pEnum->DefFile()->TxtName() << "</p>\n" ;
	C << "<p>\n" << pEnum->StrDesc() << "</p>\n" ;
	C << "<p><b>Enum definition:</b></p>\n" ;
	C << "<p>enum &nbsp; &nbsp;" << pEnum->StrName() << "<br/>{</p>\n" ;

	C << "<table class=\"fld\">\n" ;
	for (nIndex = 0 ; nIndex < pEnum->m_ValuesByNum.Count() ; nIndex++)
	{
		pEV = pEnum->m_ValuesByNum.GetObj(nIndex) ;
		if (!pEV)
			Fatal("%s. Error: No enum value for enum %s, position %d\n", *_fn, *pEnum->StrName(), nIndex) ;

		if (!pEV->StrName())
		{
			slog.Out("%s. Error: No enum value name for enum %s, position %d\n", *_fn, *pEnum->StrName(), nIndex) ;
			continue ;
		}

		C << "<tr><td width=\"9\"></td><td>" << pEV->StrName() << "</td><td>" << pEV->m_txtVal << "</td><td>" ;

		if (!pEV->StrDesc())
			C << "No description" ;
		else
			C << pEV->StrDesc() ;

		C << "</td></tr>\n" ;
	}
	C << "</table>\n" ;
	C << "<p>} ;</p>\n" ;

	//	Write the <xtreeitem> tag
	Clense(pt->m_Content, C) ;

	return E_OK ;
}

/*
hzEcode	ceFunc::_addProgsteps	(hzChain& C, ceStmt* pExec)
{
	_hzfunc(__func__) ;

	for (; pExec ; pExec = pExec->next)
	{
		if (pExec->m_Pretext)
			{ C << "<p>\n" ; C << pExec->m_Pretext ; C << "\n</p>\n" ; }
		if (pExec->m_Comment)
			{ C << "<p>\n" ; C << pExec->m_Comment ; C << "\n</p>\n" ; }

		if (pExec->child)
			_addProgsteps(C, pExec->child) ;
	}
}
*/

hzEcode	ceComp::MakePageFunc	(hdsArticleStd* addPt, ceFunc* pFunc)
{
	//	Make a page for a function group. If a page is part of a set call MakePageFnset.

	_hzfunc("ceComp::MakePageFunc") ;

	hzList<ceFunc*>::Iter	fi ;	//	Iterator for member functions
	hzList<ceVar*>::Iter	ai ;	//	Arguments iterator
	hzList<hzPair>::Iter	di ;	//	Arguments description iterator

	hzChain			C ;				//	Build basic HTML main content
	ceToken			T ;				//	Current token
	ceFile*			pFile ;			//	File in which function is defined
	ceStmt			Stmt ;			//	Current statement
	ceVar*			pVar ;			//	Variable instance (for func args etc)
	hzPair			currArg ;		//	Current arg desc
	hzString		pgTitle ;		//	Title of page (fqname of fn-group)
	hdsArticleStd*	pt ;			//	Tree item added
	const char*		i ;				//	For processing comment blocks
	uint32_t		nIndex ;		//	Token iterator
	uint32_t		nCol ;			//	Column indicator
	uint32_t		curln ;			//	Current line
	uint32_t		nEnd ;			//	End of tokens
	uint32_t		nArg ;			//	Argument iterator

	if (!pFunc)
		Fatal("%s. No function supplied\n", *_fn) ;

	if (pFunc->m_pSet)
		return MakePageFnset(addPt, pFunc->m_pSet) ;

	/*
 	**	Exclude classes which are internal and without a category
 	*/

	if (pFunc->GetAttr() & EATTR_INTERNAL)
	{
		if (!pFunc->GetCategory())
			{ slog.Out("%s. NOTE Function %s is internal and not categorized - NO_PAGE!\n", *_fn, *pFunc->StrName()) ; return E_OK ; }
	}

	if (!pFunc->GetCategory())
		slog.Out("%s. WARNING: Function %s is not categorized\n", *_fn, *pFunc->Fqname()) ;

	if (!pFunc->DefFile())
	{
		slog.Out("%s. SEVERE: Function %s does not hava a file in which it is defined\n", *_fn, *pFunc->Fqname()) ;
		return E_NODATA ;
	}

	if (!pFunc->Artname())
		{ slog.Out("%s. Function group %s has no HTML setting\n", *_fn, *pFunc->StrName()) ; return E_OK ; }
	slog.Out("%s. Called on fn %s\n", *_fn, *pFunc->Fqname()) ;

	//	Define page title
	if (!pFunc->ParKlass())
		pgTitle = "Global function: " ;
	else
		pgTitle = "Member function: " ;
	pgTitle += pFunc->Fqname() ;

	if (s_html.Exists(pFunc->Artname()))
		slog.Out("%s. DUPLICATE_ARTICLE %s\n", *_fn, *pFunc->Artname()) ;
	s_html.Insert(pFunc->Artname()) ;

	if (addPt)
		pt = g_treeNav.AddItem(addPt, pFunc->Artname(), pFunc->StrName(), false) ;

	/*
	**	Rationalize argument descriptions
	*/

	if (pFunc->m_Args.Count())
	{
		di = pFunc->m_Argdesc ;
		ai = pFunc->m_Args ;

		slog.Out("Rationalizing argument descriptions for %s (%d, %d)\n", *pFunc->StrName(), pFunc->m_Argdesc.Count(), pFunc->m_Args.Count()) ;

		for (; ai.Valid() && di.Valid() ; ai++, di++)
		{
			pVar = ai.Element() ;
			if (pVar->StrDesc())
				continue ;
			currArg = di.Element() ;
			pVar->SetDesc(currArg.value) ;
		}
	}

	/*
	**	Begin HTML
	*/

	if (pFunc->StrDesc())
		C << "<p>\n" << pFunc->StrDesc() << "</p>\n" ;

	C << "<table border=\"1\">\n" ;
	C << "<tr><th>Return Type</th><th>Function name</th><th>Arguments</th></tr>\n" ;

	C << "<tr><td>" ;
	C << pFunc->Return().Str() ;
	C << "</td><td>" ;
	C << pFunc->Fqname() ;
	C << "</td><td>(" ;
	if (!pFunc->m_Args.Count())
		C << "void" ;
	else
	{
		for (ai = pFunc->m_Args ; ai.Valid() ; ai++)
		{
			pVar = ai.Element() ;
			C << pVar->Typlex().Str() ;
			C.AddByte(CHAR_COMMA) ;
		}
	}
	C << ")</td></tr>\n" ;
	C << "</table>\n" ;

	//	State in which file functions are declared and defined. The HTML will be of the form 'declared in file.h' and 'defined in file.cpp' or where
	//	the function is both declared and defined in the same file write out 'declared and defined in file.h/.cpp'

	if (!pFunc->DefFile())
	{
		//	No definition
		if (pFunc->GetAttr() & FN_ATTR_VIRTUAL)
			slog.Out("%s. VIRT func %s (dcl=%s, No definition)\n", *_fn, *pFunc->Fqname(), pFunc->DclFname()) ;
		else
			slog.Out("%s. FAIL func %s (dcl=%s, No definition)\n", *_fn, *pFunc->Fqname(), pFunc->DclFname()) ;
	}

	//	Warn if no description
	if (pFunc->StrDesc())
		slog.Out("%s. PASS func %s (dcl=%s, def=%s)\n", *_fn, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname()) ;
	else if (pFunc->DefFile() == pFunc->DclFile())
		slog.Out("%s. TRIV func %s (dcl=%s, def=%s)\n", *_fn, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname()) ;
	else if (pFunc->StrDesc())
		slog.Out("%s. DESC func %s (dcl=%s, def=%s)\n", *_fn, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname()) ;
	else
		slog.Out("%s. WARN func %s (dcl=%s, def=%s) No description\n", *_fn, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname()) ;

	//	Produce text for dcl and def filenames
	if (pFunc->DclFile() && pFunc->DefFile())
	{
		if (pFunc->DclFile() == pFunc->DefFile())
			C << "<p>Declared and defined in file: " << pFunc->DefFname() << "</p>\n" ;
		else
			C.Printf("<p>\nDeclared in file: %s<br/>\nDefined in file : %s\n</p>\n", pFunc->DclFname(), pFunc->DefFname()) ;
	}
	else if (!pFunc->DefFile() && pFunc->DclFile())
		C << "<p><b>Declared in file:</b> " << pFunc->DclFname() << "</p>\n" ;
	else if (!pFunc->DclFile() && pFunc->DefFile())
		C << "<p><b>Defined in file:</b> " << pFunc->DefFname() << "</p>\n" ;
	else
		C << "<p>Not publicly declared or defined</p>\n" ;

	//	Description
	//pFunc->Describe(C) ;

	//	Body
	pFile = pFunc->DefFile() ;

	if (!pFile)
	{
		C.Printf("<p>Function %s does not have a file in which it is defined</p>\n", *pFunc->Fqname()) ;
		slog.Out("CODING STANDARDS: Function %s (id %u) does not have a file in which it is defined\n", *pFunc->Fqname(), pFunc->GetUEID()) ;
		return E_OK ;
		//continue ;
	}

	//	Description based on executable code in function body
	if (pFunc->m_Execs.Count())
	{
		C << "<p><b>Function Logic:</b></p>\n" ;
		//	Check if there are prinatble statements
		for (nIndex = 0 ; nIndex < pFunc->m_Execs.Count() ; nIndex++)
		{
			Stmt = pFunc->m_Execs[nIndex] ;
			if (Stmt.m_Pretext)
				break ;
			if (Stmt.m_Comment)
				break ;
		}
		if (nIndex < pFunc->m_Execs.Count())
		{
			C << "<p><b>Function Logic:</b></p>\n" ;

			for (nIndex = 0 ; nIndex < pFunc->m_Execs.Count() ; nIndex++)
			{
				Stmt = pFunc->m_Execs[nIndex] ;

				if (Stmt.m_Pretext)
					{ C << "<p>\n" ; C << Stmt.m_Pretext ; C << "\n</p>\n" ; }
				if (Stmt.m_Comment)
					{ C << "<p>\n" ; C << Stmt.m_Comment ; C << "\n</p>\n" ; }
			}
		}
	}

	/*
	**	Render the function
	*/

	C <<
	"<p><b>Function body:</b></p>\n"
	"<pre>\n" ;

	//	First show return and name of function
	C << pFunc->Return().Str() ;
	C.AddByte(CHAR_TAB) ;
	C << pFunc->Fqname() ;
	C << "\t(" ;

	nArg = pFunc->m_Args.Count() ;

	if (!nArg)
		C << "void)" ;
	else
	{
		for (nArg--, ai = pFunc->m_Args ; ai.Valid() ; nArg--, ai++)
		{
			pVar = ai.Element() ;

			if (pFunc->DefFile() != pFunc->DclFile())
			{
				if (!pVar->StrDesc())
					slog.Out("%s. MISSING ARG DESC func %s (dcl=%s, def=%s) Argument %s\n",
						*_fn, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname(), *pVar->StrName()) ;
			}

			C << pVar->Typlex().Str() ;
			if (pVar->StrName())
				C << " " << pVar->StrName() ;

			C << (nArg ? ", " : ")") ;
		}
	}
	C.AddByte(CHAR_NL) ;

	//	Now print tokens

	nIndex = pFunc->GetDefStart() ;
	T = pFile->X[nIndex] ;
	nIndex = T.orig ;

	nEnd = pFunc->GetDefEnd() ;
	T = pFile->X[nEnd] ;
	nEnd = T.orig ;

	for (; nIndex < nEnd ; nIndex++)
	{
		if (pFile->P[nIndex].type == TOK_CURLY_OPEN)
			break ;
	}
		
	curln = pFile->P[nIndex].line ;
	nCol = 1 ;

	for (; nIndex <= nEnd ; nIndex++)
	{
		T = pFile->P[nIndex] ;

		//	If the token has a different (higher) line number than the previous token, add newlines here
		if (curln < T.line)
		{
			if ((T.line - curln) > 3)
			{
				slog.Out("tok %d has line of %d, curr line is %d\n", nIndex, T.line, curln) ;
				C.AddByte(CHAR_NL) ;
				curln = T.line ;
			}
			else
			{
				for (; curln < T.line ; curln++)
					C.AddByte(CHAR_NL) ;
			}
			nCol = 1 ;
		}

		//	If the token has a higher column number than the current column value, add spaces here
		for (; nCol < T.col ; nCol++)
			C.AddByte(CHAR_SPACE) ;

		//	Generate text from the token
		switch	(T.type)
		{
		case TOK_CURLY_OPEN:	C << "{" ; nCol++ ; break ;
		case TOK_CURLY_CLOSE:	C << "}" ; nCol++ ; break ;
		case TOK_ROUND_OPEN:	C << "(" ; nCol++ ; break ;
		case TOK_ROUND_CLOSE:	C << ")" ; nCol++ ; break ;
		case TOK_END:			C << ";" ; nCol++ ; break ;

		case TOK_COMMENT:

			if (T.t_cmtf & TOK_FLAG_COMMENT_LINE)
				C << "//  " ;
			else
			{
				//	Block comment so will open with slash/asterisk a newline and then tabs to a double asterisk
				C << "/*\n" ;
				for (nCol = 1 ; nCol < T.col ; nCol++)
					C.AddByte(CHAR_SPACE) ;
				C << "**  " ;
			}

			if (T.value)
			{
				for (i = *T.value ; *i ; i++)
				{
					if (*i == CHAR_NL)
					{
						//	Do the indent
						C.AddByte(CHAR_NL) ;
						for (nCol = 1 ; nCol < T.col ; nCol++)
							C.AddByte(CHAR_SPACE) ;

						//	And follow with either the '**' or the '*/'
						if (T.t_cmtf & TOK_FLAG_COMMENT_LINE)
							C << "//  " ;
						else
							C << "**  " ;
						continue ;
					}

					if		(*i == CHAR_LESS)	C << "&lt;" ;
					else if (*i == CHAR_MORE)	C << "&gt;" ;
					else
						C.AddByte(*i) ;
				}
			}

			//	Terminate comment
			if (!(T.t_cmtf & TOK_FLAG_COMMENT_LINE))
			{
				for (nCol = 1 ; nCol < T.col ; nCol++)
					C.AddByte(CHAR_SPACE) ;
				C << "*/\n" ;
			}
			nCol = 1 ;
			break ;

		case TOK_QUOTE:

			C.AddByte(CHAR_DQUOTE) ;
			if (T.value)
			{
				for (i = *T.value ; *i ; i++)
				{
					if		(*i == CHAR_LESS)	C << "&lt;" ;
					else if (*i == CHAR_MORE)	C << "&gt;" ;
					else
						C.AddByte(*i) ;
				}
			}
			C.AddByte(CHAR_DQUOTE) ;
			nCol += (T.value.Length() + 2) ;
			break ;

		case TOK_SCHAR:

			C.AddByte(CHAR_SQUOTE) ;
			if (T.value)
			{
				for (i = *T.value ; *i ; i++)
				{
					if		(*i == CHAR_LESS)	C << "&lt;" ;
					else if (*i == CHAR_MORE)	C << "&gt;" ;
					else
						C.AddByte(*i) ;
				}
			}
			C.AddByte(CHAR_SQUOTE) ;
			nCol += (T.value.Length() + 2) ;
			break ;

		case TOK_CMD_IF:		nCol += 2 ;	C << "if" ;		break ;
		case TOK_CMD_ELSE:		nCol += 4 ; C << "else" ;	break ;
		case TOK_CMD_FOR:		nCol += 3 ;	C << "for" ;	break ;
		case TOK_CMD_DO:		nCol += 2 ; C << "do" ;		break ;

		case TOK_CMD_WHILE:		nCol += 5 ;	C << "while" ;		break ;
		case TOK_CMD_BREAK:		nCol += 5 ;	C << "break" ;		break ;
		case TOK_CMD_CONTINUE:	nCol += 8 ;	C << "continue" ;	break ;
		case TOK_CMD_GOTO:		nCol += 4 ;	C << "goto" ;		break ;
		case TOK_CMD_RETURN:	nCol += 6 ;	C << "return" ;		break ;

		case TOK_OP_LSHIFTEQ:	nCol += 3 ;	C << "&lt;&lt;&eq;" ;	break ;
		case TOK_OP_LSHIFT:		nCol += 2 ;	C << "&lt;&lt;" ;		break ;
		case TOK_OP_LESSEQ:		nCol += 2 ;	C << "&lt;&eq;" ;		break ;
		case TOK_OP_LESS:		nCol += 1 ;	C << "&lt;" ;			break ;
		case TOK_OP_RSHIFTEQ:	nCol += 3 ;	C << "&gt;&gt;&eq;" ;	break ;
		case TOK_OP_RSHIFT:		nCol += 2 ;	C << "&gt;&gt;" ;		break ;
		case TOK_OP_MOREEQ:		nCol += 2 ;	C << "&gt;&eq;" ;		break ;
		case TOK_OP_MORE:		nCol += 1 ;	C << "&gt;" ;			break ;

		default:	if (T.value)
					{
						for (i = *T.value ; *i ; i++)
						{
							if		(*i == CHAR_LESS)	C << "&lt;" ;
							else if (*i == CHAR_MORE)	C << "&gt;" ;
							else
								C.AddByte(*i) ;
						}
						nCol += T.value.Length() ;
					}
					break ;
		}
	}
	C << "\n</pre>\n" ;

	//	Write the <xtreeitem> tag
	Clense(pt->m_Content, C) ;

	return E_OK ;
}

hzEcode	ceComp::MakePageFnset	(hdsArticleStd* addPt, ceFnset* pFs)
{
	//	Create a HTML page describing all the functions in the set.

	_hzfunc("ceComp::MakePageFnset") ;

	hzList<ceFunc*>::Iter	fi ;	//	Iterator for member functions
	hzList<ceVar*>::Iter	ai ;	//	Arguments iterator

	hzList<ceVar>::Iter	V ;			//	Iterator for member variables
	hzList<hzString>::Iter	I ;		//	Iterator for member variables

	hzChain			C ;				//	Build basic HTML main content
	ceFunc*			pFunc ;			//	Function pointer
	ceVar*			pVar ;			//	Variable instance (for func args etc)
	hdsArticleStd*	pt ;			//	Tree item added
	int32_t			nArg ;			//	Argument iterator

	if (!pFs)	Fatal("%s. No function set suplied\n", *_fn) ;
	if (!addPt)	Fatal("%s. No tree add point supplied\n", *_fn) ;

	if (!pFs->Artname())
		{ slog.Out("%s. Function set %s has no HTML setting\n", *_fn, *pFs->m_Title) ; return E_OPENFAIL ; }

	if (pFs->m_bDone)
	{
		slog.Out("%s. Page for function set %s (%s) - DONE\n", *_fn, *pFs->m_Title, *pFs->Artname()) ;
		return E_OK ;
	}

	slog.Out("%s. Page for function set %s (%s)\n", *_fn, *pFs->m_Title, *pFs->Artname()) ;
	pFs->m_bDone = true ;

	//	Define page title
	//	if (pFs->Fqname() == pFs->m_Title)
	//		pgTitle = "Global function: " ;
	//	else
	//		pgTitle = "Member function: " ;
	//	pgTitle += pFs->Fqname() ;

	if (s_html.Exists(pFs->Artname()))
		slog.Out("%s. DUPLICATE_ARTICLE %s\n", *_fn, *pFs->Artname()) ;
	s_html.Insert(pFs->Artname()) ;

	pt = g_treeNav.AddItem(addPt, pFs->Artname(), pFs->m_Title, false) ;

	/*
	**	Begin HTML
	*/

	if (pFs->m_grpDesc)
		DescribeX(C, pFs->m_grpDesc) ;

	if (pFs->m_functions.Count() > 1)
	{
		//	Set out a table of the functions.
		C << "<table border=\"1\">\n" ;
		C << "<tr><th>Return Type</th><th>Function name</th><th>Arguments</th><th>Declared</th><th>Description</th></tr>\n" ;

		for (fi = pFs->m_functions ; fi.Valid() ; fi++)
		{
			pFunc = fi.Element() ;

			C << "<tr><td>" << pFunc->Return().Str() << "</td><td>" << pFunc->Fqname() << "</td><td>(" ;
			if (!pFunc->m_Args.Count())
				C << "void" ;
			else
			{
				for (nArg = 0, ai = pFunc->m_Args ; ai.Valid() ; nArg++, ai++)
				{
					if (nArg)
						C << ", " ;

					pVar = ai.Element() ;
					C << pVar->Typlex().Str() ;
					C.AddByte(CHAR_SPACE) ;
					C << pVar->StrName() ;
				}
			}
			C.Printf(")</td><td>%s</td><td>%s</td></tr>\n", pFunc->DclFname(), *pFunc->StrDesc()) ;
		}
		C << "</table>\n" ;
	}

	//	State where functions are declared and defined. All functions in a group should be declared in a single header file. This will
	//	be the case where they are class members as they are declared as part of the class definition which must nessesarily be in one
	//	header file. It is possible though (albeit bad practice) for non-member functions of the same name to be declared in different
	//	header files. It is also common for functions (member or non-member) to be defined in different source files. What we want to
	//	do here is present a summary of where functions are defined as follow:-
	//
	//	If there is only one function in the group then we will write out 'declared in file.h' and 'defined in file.cpp' or where the
	//	function is both declared and defined in the same file write out 'declared and defined in file.h/.cpp'
	//
	//	If there is more than one function but all are declared in the same header file write out 'declared in file.h', and if all are
	//	declared in one file then also write out 'defined in file ...' and if all are declared and defined in one file then this too
	//	will be as above. However if the functions are defined in different files then we need a table column to state where functions
	//	are defined. If more than one header is implicated we need a column for those also!

	for (fi = pFs->m_functions ; fi.Valid() ; fi++)
	{
		pFunc = fi.Element() ;

		if (!pFunc->DefFile())
		{
			//	No definition
			if (pFunc->GetAttr() & FN_ATTR_VIRTUAL)
				slog.Out("%s. VIRT grp %s func %s (dcl=%s, No definition)\n", *_fn, *pFs->m_Title, *pFunc->Fqname(), pFunc->DclFname()) ;
			else
				slog.Out("%s. FAIL grp %s func %s (dcl=%s, No definition)\n", *_fn, *pFs->m_Title, *pFunc->Fqname(), pFunc->DclFname()) ;
		}

		//	Warn if no description
		if (pFunc->StrDesc())
			slog.Out("%s. PASS grp %s, func %s (dcl=%s, def=%s)\n", *_fn, *pFs->m_Title, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname()) ;
		else if (pFunc->DefFile() == pFunc->DclFile())
			slog.Out("%s. TRIV grp %s, func %s (dcl=%s, def=%s)\n", *_fn, *pFs->m_Title, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname()) ;
		else if (pFs->m_grpDesc)
			slog.Out("%s. DESC grp %s, func %s (dcl=%s, def=%s)\n", *_fn, *pFs->m_Title, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname()) ;
		else
			slog.Out("%s. WARN grp %s, func %s (dcl=%s, def=%s) No description\n",
				*_fn, *pFs->m_Title, *pFunc->Fqname(), pFunc->DclFname(), pFunc->DefFname()) ;
	}

	//	Write the <xtreeitem> tag
	Clense(pt->m_Content, C) ;

	return E_OK ;
}

/*
**	Section X:	Component Report
*/

hzEcode	_reportClass	(hzChain& Z, ceClass* pClass)
{
	//	Support function called by part 1 of ceComp::Report to write out the class name and a list of functions to the all classes file. It is not to be confused with MakePageClass
	//	which actually generates a HTML page describing a class.

	_hzfunc(__func__) ;

	hzList<ceFunc*>::Iter	fi ;

	ceEntity*	pE ;		//	Entity pointer
	ceFunc*		pFunc ;		//	Function pointer
	int32_t		nIndex ;	//	Entity iterator

	if (!pClass)
		Fatal("%s. No class supplied\n", *_fn) ;

	if (pClass->GetAttr() & CL_ATTR_STRUCT)
		Z << "struct\t" ;
	else
		Z << "class\t" ;

	if (pClass->GetAttr() & CL_ATTR_TEMPLATE)
		Z << "(templated)\t" ;
	if (pClass->GetAttr() & CL_ATTR_ABSTRACT)
		Z << "(abstract)\t" ;

	Z << pClass->StrName() << " defined in " << pClass->DefFile()->StrName() ;
	Z.AddByte(CHAR_NL) ;

	for (nIndex = 0 ; (pE = pClass->GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->Whatami() != ENTITY_FUNCTION)
			continue ;

		//	Indicate if funcs are described or undescribed
		pFunc = (ceFunc*) pE ;

		if (pFunc->StrDesc())
			s_described.Insert(pFunc->Extname()) ;
		else
		{
			//	The function itself does not need a description if the group to which it belongs does, or if it belongs to a function set which does.
			if (pFunc->m_pSet && pFunc->m_pSet->m_grpDesc)
				s_described.Insert(pFunc->Extname()) ;
			else
			{
				s_undescribed.Insert(pFunc->Extname()) ;
			}
		}

		Z.Printf("    function:\t%s\n", *pFunc->Fqname()) ;
	}
}

hzEcode	ceComp::Statements	(const hzString& htmlDir) const
{
	_hzfunc("ceComp::Statements") ;

	ofstream	os ;			//	For file writes
	hzChain		C ;				//	Output chain
	ceStmt		theStmt ;		//	Statement
	hzString	filename ;		//	Current filename
	ceFile*		pFile ;			//	Autodoc header/source file
	ceEntity*	pE ;			//	Entity (from root)
	ceEntity*	pX ;			//	Entity (from class)
	ceClass*	pClass ;		//	Class
	ceFunc*		pFunc ;			//	Function
	uint32_t	nIndex ;		//	Iterator root entities
	uint32_t	nCount ;		//	Iterator statments
	uint32_t	nCE ;			//	Iterator class entities
	uint32_t	x ;				//	Iterator

	slog.Out("%s: Called\n", *_fn) ;

	filename = htmlDir + "/" + m_Name + "_statements.xml" ;

	C.Printf("<Component name=\"%s\">\n", *m_Name) ;
	for (nIndex = 0 ; nIndex < m_Headers.Count() ; nIndex++)
	{
		pFile = m_Headers.GetObj(nIndex) ;
		C.Printf("\t<header name=\"%s\">\n", *pFile->StrName()) ;

		for (nCount = 0 ; nCount < pFile->m_Statements.Count() ; nCount++)
		{
			theStmt = pFile->m_Statements[nCount] ;

			for (x = theStmt.m_nLevel ; x ; x--)
				C.AddByte(CHAR_TAB) ;
			C.Printf("\t%d  line%4d  start%5d  end%5d  %s obj=%s\n",
				theStmt.m_nLevel, theStmt.m_nLine, theStmt.m_Start, theStmt.m_End, Statement2Txt(theStmt.m_eSType), *theStmt.m_Object) ;
		}
		C << "\t</header>\n" ;
	}

	C.Printf("\nSource files (total %d)\n", m_Sources.Count()) ;
	for (nIndex = 0 ; nIndex < m_Sources.Count() ; nIndex++)
	{
		pFile = m_Sources.GetObj(nIndex) ;
		C.Printf("\t<source name=\"%s\">\n", *pFile->StrName()) ;

		for (nCount = 0 ; nCount < pFile->m_Statements.Count() ; nCount++)
		{
			theStmt = pFile->m_Statements[nCount] ;

			for (x = theStmt.m_nLevel ; x ; x--)
				C.AddByte(CHAR_TAB) ;
			C.Printf("\t%d  line%4d  start%5d  end%5d  %s obj=%s\n",
				theStmt.m_nLevel, theStmt.m_nLine, theStmt.m_Start, theStmt.m_End, Statement2Txt(theStmt.m_eSType), *theStmt.m_Object) ;
		}
		C << "\t</source>\n" ;
	}

	slog.Out("%s: Done headers and source files\n", *_fn) ;

	C << "\t<Classes>\n" ;
	for (nIndex = 0 ; (pE = g_Project.m_RootET.GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->ParComp() != this)
			continue ;
		if (pE->Whatami() != ENTITY_CLASS)
			continue ;

		pClass = (ceClass*) pE ;
		C.Printf("\t\t<Class name=\"%s\">\n", *pClass->StrName()) ;

		for (nCount = 0 ; nCount < pClass->m_Statements.Count() ; nCount++)
		{
			theStmt = pClass->m_Statements[nCount] ;

			C.Printf("\t\t%d ", theStmt.m_nLine) ;

			for (x = theStmt.m_nLevel ; x ; x--)
				C.AddByte(CHAR_TAB) ;
			C.Printf("%s %s\n", Statement2Txt(theStmt.m_eSType), *theStmt.m_Object) ;
		}

		//	Do class functions
		for (nCE = 0 ; (pX = pClass->GetEntity(nCE)) ; nCE++)
		{
			if (pX->Whatami() != ENTITY_FUNCTION)
				continue ;

			pFunc = (ceFunc*) pX ;
			C.Printf("\t\t\t<Function name=\"%s\">\n", *pFunc->Fqname()) ;

			for (nCount = 0 ; nCount < pFunc->m_Execs.Count() ; nCount++)
			{
				theStmt = pFunc->m_Execs[nCount] ;

				C.Printf("\t\t\t%d ", theStmt.m_nLine) ;
				for (x = theStmt.m_nLevel ; x ; x--)
					C.AddByte(CHAR_TAB) ;

				C.Printf("%s %s\n", Statement2Txt(theStmt.m_eSType), *theStmt.m_Object) ;
			}

			C << "\t\t\t</Function>\n" ;
		}

		C << "\t\t</Class>\n" ;
	}
	C << "\t</Classes>\n" ;

	C << "</Component>\n" ;
	slog.Out("%s: Done classes and functions\n", *_fn) ;

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open sources file for export\n", *_fn) ; return E_OPENFAIL ; }

	os << C ;
	os.close() ;
	os.clear() ;
	C.Clear() ;
}

hzEcode	ceComp::Report	(const hzString& htmlDir)	//const
{
	//	Create HTML report for project component.
	//
	//	Work through the entire entity table (starting from the global scope as root) and export as HTML, information on each key entity.

	_hzfunc("ceComp::Report") ;

	hzList<ceClass*>::Iter		cl ;	//	Class iterator
	hzList<ceFunc*>::Iter		fi ;	//	Function iterator
	hzList<ceEnum*>::Iter		ei ;	//	Enum iterator
	hzList<ceVar*>::Iter		vi ;	//	Global variables iterator
	hzList<ceDefine*>::Iter		di ;	//	Global applicable hash defines
	hzList<ceMacro*>::Iter		mi ;	//	Global applicable macros
	hzList<ceEntity*>::Iter		en ;	//	General entity iterator
	hzList<ceVar*>::Iter		ai ;	//	Function arg iterator

	hzList<ceEntity*>	lstCtmpls ;		//	All function groups and sets found within scope
	hzList<ceEntity*>	lstClasses ;	//	All function groups and sets found within scope
	hzList<ceEntity*>	lstFuncs ;		//	All function groups found within scope
	hzList<ceEntity*>	lstFnsets ;		//	All function sets found within scope
	hzList<ceEntity*>	lstEnums ;		//	All enums found within scope
	hzList<ceEntity*>	lstVars ;		//	All variables found within scope
	hzList<ceEntity*>	lstDefines ;	//	All hash defines found within scope
	hzList<ceEntity*>	lstMacros ;		//	All macros found within scope

	hzSet<hzString>	clcats ;			//  Set of class category names found so far

	ofstream		os ;				//	For file writes
	hzChain			C ;					//	Output chain
	hzChain			H ;
	hzString		S ;					//	Temp string
	hzString		T ;					//	Temp string
	hzString		pgTitle ;
	hzString		last_file ;
	hzString		filename ;			//	Current filename
	hzString		gname ;				//	Class/func group name (for categorizations)
	ceToken			token ;
	hdsArticleStd*	pt0 ;				//	Tree item pointer for this level items
	hdsArticleStd*	pt1 ;				//	Tree item pointer for level 1 items
	hdsArticleStd*	pt2 ;				//	Tree item pointer for level 2 items
	ceSynp*			pSynp ;
	ceFile*			pFile ;				//	Autodoc header/source file
	ceEntity*		pE ;
	ceClass*		pClass ;
	ceFunc*			pFunc ;
	ceVar*			pVar ;				//	Pointer to variable
	ceEnum*			pEnum ;
	ceEnumval*		pEV ;
	ceMacro*		pMacro ;			//	Pointer to macro instance
	ceDefine*		pDefine ;			//	Pointer to define instance
	uint32_t		nNaked ;			//	Count of uncategorized classes/functions
	uint32_t		nIndex ;			//	Iterator
	uint32_t		nCount ;			//	Iterator
	uint32_t		x ;					//	Iterator
	uint32_t		nG ;				//	Function group counter
	uint32_t		nE ;				//	Ersatz counter
	char			buf	[160] ;
	hzEcode			rc ;

	//	Check for illegal repeat call
	if (m_bComplete)
		Fatal("%s. Repeat call for component %s\n", *_fn, *m_Name) ;
	m_bComplete = true ;

	/*
	**	Collect applicable objects
	*/

	for (nCount = nIndex = 0 ; (pE = g_Project.m_RootET.GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->ParComp() != this)
			continue ;

		if (pE->Whatami() == ENTITY_CLASS)
		{
			if (pE->GetAttr() & CL_ATTR_TEMPLATE)
				lstCtmpls.Add(pE) ;
			else
				lstClasses.Add(pE) ;
		}

		if (pE->Whatami() == ENTITY_FUNCTION)	lstFuncs.Add(pE) ;
		if (pE->Whatami() == CE_UNIT_FUNC_GRP)	lstFnsets.Add(pE) ;
		if (pE->Whatami() == ENTITY_ENUM)		lstEnums.Add(pE) ;
		if (pE->Whatami() == ENTITY_DEFINE)		lstDefines.Add(pE) ;
		if (pE->Whatami() == ENTITY_MACRO)		lstMacros.Add(pE) ;
		if (pE->Whatami() == ENTITY_VARIABLE)	lstVars.Add(pE) ;
	}

	/*
	**	Compile summary of statements in header and source files
	*/

	Statements(htmlDir) ;

	/*
	**	Compile Summary of Objects
	*/
	slog.Out("%s We have %d templates\n",	*_fn, lstCtmpls.Count()) ;
	slog.Out("%s We have %d classes\n",		*_fn, lstClasses.Count()) ;
	slog.Out("%s We have %d functions\n",	*_fn, lstFuncs.Count()) ;
	slog.Out("%s We have %d func sets\n",	*_fn, lstFnsets.Count()) ;
	slog.Out("%s We have %d enums\n",		*_fn, lstEnums.Count()) ;
	slog.Out("%s We have %d variables\n",	*_fn, lstVars.Count()) ;
	slog.Out("%s We have %d # defines\n",	*_fn, lstDefines.Count()) ;
	slog.Out("%s We have %d macros\n",		*_fn, lstMacros.Count()) ;

	//	Compile list of source and header files
	filename = htmlDir + "/" + m_Name + "_sources.txt" ;
	C.Printf("Header files (total %d)\n", m_Headers.Count()) ;
	for (x = 0 ; x < m_Headers.Count() ; x++)
	{
		pFile = m_Headers.GetObj(x) ;
		C.Printf("\t%s\n", *pFile->StrName()) ;
	}

	C.Printf("\nSource files (total %d)\n", m_Sources.Count()) ;
	for (x = 0 ; x < m_Sources.Count() ; x++)
	{
		pFile = m_Sources.GetObj(x) ;
		C.Printf("\t%s\n", *pFile->StrName()) ;
	}

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open sources file for export\n", *_fn) ; return E_OPENFAIL ; }

	os << C ;
	os.close() ;
	os.clear() ;
	C.Clear() ;

	//	Compile class template pages
	filename = htmlDir + "/" + m_Name + "_ctmpls.txt" ;
	C.Printf("All class templates (total %d)\n", nCount) ;

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Component %s. Could not open class templates file for export\n", *_fn, *m_Name) ; return E_OPENFAIL ; }
	os << C ;
	os.close() ;
	os.clear() ;
	slog.Out("%s. Component %s. Exported %d Class Templates\n", *_fn, *m_Name, nCount) ;
	C.Clear() ;

	//	Compile class pages
	filename = htmlDir + "/" + m_Name + "_classlist.txt" ;
	C.Printf("All Classes (total %d)\n", nCount) ;

	for (en = lstClasses ; en.Valid() ; en++)
	{
		pE = en.Element() ;
		pClass = (ceClass *) pE ;
		C << pClass->StrName() << " defined in " << pClass->DefFile()->StrName() ;
		C.AddByte(CHAR_NL) ;
	}

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open classes file for export\n", *_fn) ; return E_OPENFAIL ; }
	os << C ;
	os.close() ;
	os.clear() ;
	C.Clear() ;

	//	Then produce list of all classes in the component but with member functions
	filename = (char*) 0 ;
	filename = htmlDir + "/" + m_Name + "_classfull.txt" ;

	C.Clear() ;
	C.Printf("All Classes (total %d)\n", nCount) ;

	for (en = lstClasses ; en.Valid() ; en++)
	{
		pE = en.Element() ;
		pClass = (ceClass *) pE ;
		_reportClass(C, pClass) ;
		nCount++ ;
	}

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open classes file for export\n", *_fn) ; return E_OPENFAIL ; }
	os << C ;
	os.close() ;
	os.clear() ;
	slog.Out("%s. Component %s. Exported %d Classes\n", *_fn, *m_Name, nCount) ;
	C.Clear() ;

	//	Compile function pages
	filename = htmlDir + "/" + m_Name + "_functions.txt" ;
	C.Printf("Non-member functions (total %d)\n", lstFuncs.Count()) ;

	for (en = lstFuncs ; en.Valid() ; en++)
	{
		pE = en.Element() ;
		pFunc = (ceFunc*) pE ;

		sprintf(buf, "    %-15s %-31s ", *pFunc->Return().Str(), *pFunc->Fqname()) ;
		C << buf ;

		if (pFunc->m_Args.Count() == 0)
			C << "(void)\n" ;
		else
		{
			C << "(" ;
			for (ai = pFunc->m_Args ;;)
			{
				pVar = ai.Element() ;
				C << pVar->Typlex().Str() ;
				ai++ ;
				if (!ai.Valid())
					break ;
				C << ", " ;
			}
			C << ")\n" ;
		}
	}

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open functions file for export\n", *_fn) ; return E_OPENFAIL ; }
	os << C ;
	os.close() ;
	os.clear() ;
	C.Clear() ;

	//	Show enums
	filename = htmlDir + "/" + m_Name + "_enums" ;
	C.Printf("All Enums (total %d)\n", nCount) ;

	for (en = lstEnums ; en.Valid() ; en++)
	{
		pE = en.Element() ;
		pEnum = (ceEnum *) pE ;

		if (pEnum->DefFile())
			C << pEnum->StrName() << " defined in " << pEnum->DefFile()->TxtName() << "\n" ;

		for (x = 0 ; x < pEnum->m_ValuesByNum.Count() ; x++)
		{
			pEV = pEnum->m_ValuesByNum.GetObj(x) ;
			sprintf(buf, "\t%03d ", x) ;
			C << buf << pEV->StrName() << "\n" ;
		}
	}

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open enums file for export\n", *_fn) ; return E_OPENFAIL ; }
	os << C ;
	os.close() ;
	os.clear() ;
	C.Clear() ;

	//	Show Hash Defines
	filename = htmlDir + "/" + m_Name + "_defines.txt" ;
	C.Printf("Hash Defines (total %d)\n", nCount) ;

	for (en = lstDefines ; en.Valid() ; en++)
	{
		pE = en.Element() ;
		pDefine = (ceDefine *) pE ;

		if (pDefine->DefFname() != last_file)
		{
			last_file = pDefine->DefFname() ;
			C << "In file " << pDefine->DefFname() << "\n" ;
		}
		C << "\t" << pDefine->StrName() << ":\t-->\t" ;

		for (nE = 0 ; nE <= pDefine->m_Ersatz.Count() ; nE++)
			C << " " << pDefine->m_Ersatz[nE].value ;
		C << "\n" ;
	}
	last_file = 0 ;
	slog.Out("%s. Component %s. Exported %d hash-defines\n", *_fn, *m_Name, nCount) ;

	C << "\n#macros:\n" ;
	for (en = lstMacros ; en.Valid() ; en++)
	{
		pE = en.Element() ;
		pMacro = (ceMacro *) pE ;

		if (pMacro->DefFname() != last_file)
		{
			last_file = pMacro->DefFname() ;
			C << "In file " << pMacro->DefFname() << "\n" ;
		}

		if (!pMacro->m_Ersatz.Count())
			C << pMacro->StrName() << "\n" ;
		else
		{
			C << pMacro->StrName() << ":\t" ;
			for (nE = 0 ; nE < pMacro->m_Ersatz.Count() ; nE++)
				C << " " << pMacro->m_Ersatz[nE].value ;
			C << "\n" ;
		}
	}

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open defines file for export\n", *_fn) ; return E_OPENFAIL ; }
	os << C ;
	os.close() ;
	os.clear() ;
	C.Clear() ;

	//	Show global variables
	filename = htmlDir + "/" + m_Name + "_globvars.txt" ;
	C.Printf("Global variables (total %d)\n", nCount) ;

	for (en = lstVars ; en.Valid() ; en++)
	{
		pE = en.Element() ;
		pVar = (ceVar*) pE ;
		C << pVar->StrName() << "\n" ;
	}

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open globvar file for export\n", *_fn) ; return E_OPENFAIL ; }
	os << C ;
	os.close() ;
	os.clear() ;
	C.Clear() ;

	/*
	**	Show all described and undescribed entities
	*/

	os.open("entity.described") ;
	if (os.fail())
		{ slog.Out("%s. Could not open globvar file for export\n", *_fn) ; return E_OPENFAIL ; }

	//for (nIndex = 0 ; (pE = g_Project.m_ET.GetEntity(nIndex)) ; nIndex++)
	for (nIndex = 0 ; (pE = g_Project.m_RootET.GetEntity(nIndex)) ; nIndex++)
	{
		if (!pE->StrDesc())
			continue ;
		os << pE->StrName() << ":\t" << pE->StrDesc() << endl ;
	}

	os << "Described (general)\n" ;
	for (nIndex = 0 ; nIndex < s_described.Count() ; nIndex++)
	{
		os << s_described.GetObj(nIndex) << endl ;
	}

	os.close() ;
	os.clear() ;

	os.open("entity.undescribed") ;
	if (os.fail())
		{ slog.Out("%s. Could not open globvar file for export\n", *_fn) ; return E_OPENFAIL ; }

	//for (nIndex = 0 ; (pE = g_Project.m_ET.GetEntity(nIndex)) ; nIndex++)
	for (nIndex = 0 ; (pE = g_Project.m_RootET.GetEntity(nIndex)) ; nIndex++)
	{
		if (pE->StrDesc())
			continue ;
		os << pE->StrName() << endl ;
	}

	os << "#\n#\tUndescribed functions\n#\n" ;
	//for (nG = 0 ; nG < g_Project.m_ET.Count() ; nG++)
	for (nG = 0 ; nG < g_Project.m_RootET.Count() ; nG++)
	{
		//pE = g_Project.m_ET.GetEntity(nG) ;
		pE = g_Project.m_RootET.GetEntity(nG) ;
		if (pE->Whatami() != ENTITY_FUNCTION)
			continue ;

		pFunc = (ceFunc*) pE ;

		if (pFunc->StrDesc())
			continue ;
		os << pFunc->StrName() << endl ;
	}

	os << "#\n#\tUndescribed (general)\n#\n" ;
	for (nIndex = 0 ; nIndex < s_undescribed.Count() ; nIndex++)
	{
		os << s_undescribed.GetObj(nIndex) << endl ;
	}

	os.close() ;
	os.clear() ;

	/*
	**	Write out the HTML
	*/

	if (chdir(*htmlDir) == -1)
	{
		slog.Out("%s. Could not change to data directory\n", *_fn) ;
		return E_OPENFAIL ;
	}
	slog.Out("%s. Changed to HTML dir %s\n", *_fn, *htmlDir) ;

	//	Create main page for Project Component
	pt0 = MakeProjFile() ;

	//	Write out synopsis pages
	if (m_arrSynopsis.Count())
	{
		S = "synop" ;
		T = "Synopses" ;
		pt1 = (hdsArticleStd*) g_treeNav.AddHead(pt0, S, T, false) ;

		for (nIndex = 0 ; nIndex < m_arrSynopsis.Count() ; nIndex++)
		{
			pSynp = m_arrSynopsis[nIndex] ;
			slog.Out("REPORT SYNOP %s\n", *pSynp->m_Docname) ;
		}

		for (nIndex = 0 ; nIndex < m_arrSynopsis.Count() ; nIndex++)
		{
			pSynp = m_arrSynopsis[nIndex] ;

			if (!pSynp->m_Content.Size())
			{
				if (!pSynp->m_Docname.Contains(CHAR_PERIOD))
					pt2 = (hdsArticleStd*) g_treeNav.AddHead(pt1, pSynp->m_Docname, pSynp->StrName(), false) ;
				else
					pt2 = (hdsArticleStd*) g_treeNav.AddHead(pSynp->m_Parent, pSynp->m_Docname, pSynp->StrName(), false) ;
				continue ;
			}

			if (!pSynp->m_Docname.Contains(CHAR_PERIOD))
				pt2 = g_treeNav.AddItem(pt1, pSynp->m_Docname, pSynp->StrName(), false) ;
			else
				pt2 = g_treeNav.AddItem(pSynp->m_Parent, pSynp->m_Docname, pSynp->StrName(), false) ;

			//Paragraphize(C, pSynp->m_Content) ;
			AssembleSynop(C, pSynp->m_Content) ;
			pt2->m_Content = C ;
			C.Clear() ;
		}
	}

	//	Write out pages for header files
	if (m_Headers.Count())
	{
		if (m_Headers.Count() > 1)
		{
			S = "hdrs" ;
			T = "Header Files" ;
			pt1 = (hdsArticleStd*) g_treeNav.AddHead(pt0, S, T, false) ;
			for (nIndex = 0 ; nIndex < m_Headers.Count() ; nIndex++)
			{
				pFile = m_Headers.GetObj(nIndex) ;
				MakePageFile(pt1, pFile) ;
			}
		}
		else
		{
			pFile = m_Headers.GetObj(0) ;
			MakePageFile(pt0, pFile) ;
		}
		slog.Out("%s. Component %s. Made pages for %d header files\n", *_fn, *m_Name, nIndex) ;
	}

	//	Write out pages for source files
	if (m_Sources.Count())
	{
		if (m_Sources.Count() > 1)
		{
			S = "srcs" ;
			pt1 = g_treeNav.AddItem(pt0, S, "Source Files", false) ;
			for (nIndex = 0 ; nIndex < m_Sources.Count() ; nIndex++)
			{
				pFile = m_Sources.GetObj(nIndex) ;
				MakePageFile(pt1, pFile) ;
			}
		}
		else
		{
			pFile = m_Sources.GetObj(0) ;
			MakePageFile(pt0, pFile) ;
		}
	}
	slog.Out("%s. Component %s. Made pages for %d source files\n", *_fn, *m_Name, nIndex) ;

	//	Write out pages for class templates
	if (lstCtmpls.Count())
	{
		if (lstCtmpls.Count() > 1)
		{
			S = "ctmpls" ;
			pt1 = g_treeNav.AddItem(pt0, S, "Class Templates", false) ;

			for (en = lstCtmpls ; en.Valid() ; en++)
			{
				pE = en.Element() ;
				pClass = (ceClass*) pE ;
				MakePageCtmpl(pt1, pClass) ;
			}
		}
		else
		{
			en = lstCtmpls ;
			pE = en.Element() ;
			pClass = (ceClass*) pE ;
			MakePageCtmpl(pt0, pClass) ;
		}
	}

	//	Write out pages for classes
	if (lstClasses.Count())
	{
		if (lstClasses.Count() > 1)
		{
			//	Categorize
			clcats.Clear() ;
			nNaked = 0 ;
			for (en = lstClasses ; en.Valid() ; en++)
			{
				pE = en.Element() ;
				pClass = (ceClass*) pE ;

		   		if (!pClass->Category())
					nNaked++ ;
				else
				{
					if (!clcats.Exists(pClass->Category()))
					{
						slog.Out("Inserting Class Category of %s\n", *pClass->Category()) ;
						clcats.Insert(pClass->Category()) ;
					}
				}
			}

			S = "cls" ;
			pt1 = (hdsArticleStd*) g_treeNav.AddHead(pt0, S, "Classes", false) ;

			if (nNaked)
			{
				//	Do the ungrouped classes first
				for (en = lstClasses ; en.Valid() ; en++)
				{
					pE = en.Element() ;
					pClass = (ceClass*) pE ;

					if (pClass->ParKlass())
						continue ;
					if (pClass->Category())
						continue ;

					MakePageClass(pt1, pClass) ;
				}
			}

			if (nNaked < lstClasses.Count())
			{
				//	There are grouped (non-naked) classes so do these by group)

				for (nG = 0 ; nG < clcats.Count() ; nG++)
				{
					gname = clcats.GetObj(nG) ;
					S = gname + " Classes" ;
					pt2 = g_treeNav.AddItem(pt1, S, S, false) ;

					for (en = lstClasses ; en.Valid() ; en++)
					{
						pE = en.Element() ;
						pClass = (ceClass *) pE ;

						if (pClass->ParKlass())
							continue ;
						if (pClass->Category() != gname)
							continue ;

						MakePageClass(pt2, pClass) ;
					}
				}
			}
		}
		else
		{
			en = lstClasses ;
			pE = en.Element() ;
			pClass = (ceClass*) pE ;
			MakePageClass(pt0, pClass) ;
		}
	}

	//	Write out pages for Enums
	if (lstEnums.Count())
	{
		if (lstEnums.Count() > 1)
		{
			//	Categorize
			clcats.Clear() ;
			nNaked = 0 ;
			for (en = lstEnums ; en.Valid() ; en++)
			{
				pE = en.Element() ;
				pEnum = (ceEnum*) pE ;

		   		if (!pEnum->Category())
					nNaked++ ;
				else
				{
					if (!clcats.Exists(pEnum->Category()))
					{
						slog.Out("Inserting Enum Category of %s\n", *pEnum->Category()) ;
						clcats.Insert(pEnum->Category()) ;
					}
				}
			}

			S = "enums" ;
			pt1 = (hdsArticleStd*) g_treeNav.AddHead(pt0, S, "Enums", false) ;

			if (nNaked)
			{
				//	Do the ungrouped classes first
				for (en = lstEnums ; en.Valid() ; en++)
				{
					pE = en.Element() ;
					pEnum = (ceEnum*) pE ;

					if (pEnum->Category())
						continue ;
					MakePageEnum(pt1, pEnum) ;
				}
			}

			if (nNaked < lstEnums.Count())
			{
				//	There are grouped (non-naked) classes so do these by group)

				for (nG = 0 ; nG < clcats.Count() ; nG++)
				{
					gname = clcats.GetObj(nG) ;
					S = gname + " Enums" ;
					pt2 = g_treeNav.AddItem(pt1, S, _hzGlobal_nullString, false) ;

					for (en = lstEnums ; en.Valid() ; en++)
					{
						pE = en.Element() ;
						pEnum = (ceEnum *) pE ;

						if (pEnum->Category() != gname)
							continue ;
						MakePageEnum(pt2, pEnum) ;
					}
				}
			}
		}
		else
		{
			en = lstEnums ;
			pE = en.Element() ;
			pEnum = (ceEnum*) pE ;
			MakePageEnum(pt0, pEnum) ;
		}
	}

	//	Write out pages for non member (global) fuctions
	if (lstFuncs.Count())
	{
		if (lstFuncs.Count() > 1)
		{
			//	Categorize
			clcats.Clear() ;
			nNaked = 0 ;

			for (en = lstFuncs ; en.Valid() ; en++)
			{
				pE = en.Element() ;
				pFunc = (ceFunc*) pE ;

		   		if (!pFunc->GetCategory())
					nNaked++ ;
				else
				{
					if (!clcats.Exists(pFunc->GetCategory()))
					{
						slog.Out("Inserting Fngrp Category of %s\n", *pFunc->GetCategory()) ;
						clcats.Insert(pFunc->GetCategory()) ;
					}
				}
			}

			S = "globfn" ;
			pt1 = (hdsArticleStd*) g_treeNav.AddHead(pt0, S, "Global Functions", false) ;

			if (nNaked)
			{
				//	Do the ungrouped classes first
				S = "Uncategorized Functions" ;
				pt2 = g_treeNav.AddItem(pt1, S, S, false) ;

				for (en = lstFuncs ; en.Valid() ; en++)
				{
					pE = en.Element() ;
					pFunc = (ceFunc*) pE ;

		   			if (pFunc->GetCategory())
						continue ;
					MakePageFunc(pt2, pFunc) ;
				}
			}

			if (nNaked < lstFuncs.Count())
			{
				//	There are grouped (non-naked) classes so do these by group)

				for (nG = 0 ; nG < clcats.Count() ; nG++)
				{
					gname = clcats.GetObj(nG) ;
					S = gname + " Functions" ;
					pt2 = g_treeNav.AddItem(pt1, S, S, false) ;

					for (en = lstFuncs ; en.Valid() ; en++)
					{
						pE = en.Element() ;
						pFunc = (ceFunc*) pE ;

						if (pFunc->GetCategory() != gname)
							continue ;
						MakePageFunc(pt2, pFunc) ;
					}
				}
			}
		}
		else
		{
			en = lstFuncs ;
			pE = en.Element() ;
			pFunc = (ceFunc*) pE ;
			MakePageFunc(pt0, pFunc) ;
		}
	}

	//	Do function sets
	//	for (nG = 0 ; nG < g_Project.m_ET.Count() ; nG++)
	//	{
		//	pE = g_Project.m_ET.GetEntity(nG) ;

		//	if (pE->ParComp() != this)
			//	continue ;

		//	if (pE->Whatami() == CE_UNIT_FUNC_GRP)
		//	{
			//	pFs = (ceFnset*) pE ;
			//	MakePageFnset(pt1, pFs) ;
		//	}
	//	}

	slog.Out(" -- End of REPORT for Component %s --\n", *m_Name) ;
	return E_OK ;
}

/*
**	Module to write out HTML tree
*/

hzEcode	BuildHtmlTree	(void)
{
	//	Build a HTML/JavaScript hierarchical tree.
	//
	//	Modus Operandi:	At the root is the title of the whole which is always visible followed by the first level nodes. The first level nodes are:-
	//
	//	<body class="hztree">
	//		<div id="hztreeroot">
	//			The tree title
	//			<div style="display: block;">
	//				Either a ref to a single page (an image of |-) and a document
	//				Or an image of a boxed + sign plus an image of a closed book (folder) followed by
	//				a series set. This would begin with an image of a boxed - sign plus an image of an
	//				open book and would be followed by the series of items in the folder. Each of these
	//				would be as follows:-
	//				<div id="folder_id">
	//					Ref to single page or a new series. In the latter case all entries would be
	//					preceeded by a series of | images (depending on the level)
	//				</div>
	//			</div>
	//		</div>
	//	</body>

	_hzfunc(__func__) ;

	ofstream	os ;
	hzChain		Z ;		//	For adding tree items to a tree
	hzChain		J ;		//	For entries to a javascript array that navtree.js will turn into tree items
	chIter		z ;

	if (g_treeNav.Count())
		slog.Out("Building Navigation Tree\n") ;
	else
	{
		slog.Out("Cannot Build Navigation Tree\n") ;
		return E_NODATA ;
	}

	g_treeNav.ExportDataScript(J) ;

	os.open("atree.js") ;
	if (os.fail())
		{ slog.Out("Could not access menu script file\n") ; return E_OPENFAIL ; }
	os << J ;
	os.close() ;
	os.clear() ;

	slog.Out("Built Navigation Tree\n") ;

	return E_OK ;
}

hzEcode	ceProject::Report	(void)
{
	//	Recurse thru components calling Report

	_hzfunc("ceProject::Report") ;

	hzList<ceComp*>::Iter	ci ;

	ofstream	os ;			//	For output
	hzChain		Zr ;			//	For building output
	hzChain		Xr ;			//	For building output
	ceComp*		pComp ;			//	Project component
	ceEntbl*	pET ;			//	Entity Table
	cePara*		pPara ;			//	Paragraph
	hzString	filename ;		//	Output filenames
	hzString	S ;				//	Temp string
	uint32_t	nC ;			//	Loop control for Libs then Progs
	hzEcode		rc ;			//	Return code

	//	Set directory to the HTML target
	if (chdir(*m_HtmlDir) == -1)
	{
		slog.Out("%s. Could not change to data directory\n", *_fn) ;
		return E_READFAIL ;
	}

	//	Init Tree: If the project has multiple components we want a tree with the project name at the root and all the components at level 1, we set the tree root here, before the
	//	components are called. Then the Report function for each component sees there is a root and adds to it. If however, there is only a single component, leave the root unset.
	//	Then when the Report function for the component sees there is no root it will set the root with the component name.

	//s_pNavTree = new hdsTree() ;

	if ((m_Libraries.Count() + m_Programs.Count()) > 1)
	{
		if (m_Name)
			//g_treeNav.m_Root = g_treeNav.AddItem(0, 0, m_Docname, m_Name, false) ;
			g_treeNav.AddItem(0, m_Docname, m_Name, false) ;
		else
		{
			S = "untitled" ;
			//g_treeNav.m_Root = g_treeNav.AddItem(0, 0, S, S, false) ;
			g_treeNav.AddItem(0, S, S, false) ;
		}
	}

	//	Report all componenets in the order given in the config
	for (nC = 0 ; nC < 2 ; nC++)
	{
		if (nC == 0)
			ci = m_Libraries ;
		else
			ci = m_Programs ;

		for (; rc == E_OK && ci.Valid() ; ci++)
		{
			pComp = ci.Element() ;

			slog.Out("%s. Calling component report\n", *_fn) ;
			rc = pComp->Report(m_HtmlDir) ;
			slog.Out("%s. DONE component report\n", *_fn) ;

			/*
			filename = pComp->StrName() + ".entx" ;
			os.open(*filename) ;
			if (os.fail())
				slog.Out("%s. Could not open file %s for writing\n", *_fn, *filename) ;
			else
			{
				slog.Out("%s. Calling entity report\n", *_fn) ;
				Zr.Clear() ;
				//g_Project.m_ET.EntityExport(Z, pComp, 0) ;
				g_Project.m_RootET.EntityExport(Zr, pComp, 0) ;
				os << Zr ;
				os.close() ;
				Zr.Clear() ;
			}
			os.clear() ;
			slog.Out("%s. DONE entity report\n", *_fn) ;
			*/

			if (pComp->m_Error.Size())
			{
				filename = pComp->StrName() + ".errata" ;
				os.open(*filename) ;
				if (os.fail())
					slog.Out("%s. Could not open file %s for writing\n", *_fn, *filename) ;
				else
				{
					os << pComp->m_Error ;
					os.close() ;
				}
				os.clear() ;
			}
		}
	}

	//	Report all entities
	filename = g_Project.m_Name + ".entx" ;
	os.open(*filename) ;
	if (os.fail())
		slog.Out("%s. Could not open file %s for writing\n", *_fn, *filename) ;
	for (nC = 0 ; nC < g_Project.m_EntityTables.Count() ; nC++)
	{
		pET = g_Project.m_EntityTables.GetObj(nC) ;
		pET->EntityExport(Zr, 0, 0) ;
		os << Zr ;
		Zr.Clear() ;
	}
	g_Project.m_RootET.EntityExport(Zr, pComp, 0) ;
	os << Zr ;
	Zr.Clear() ;
	os.close() ;
	os.clear() ;
	slog.Out("%s. DONE entity report\n", *_fn) ;

	//	Report paragraphs
	filename = "paragraphs.txt" ;
	os.open(*filename) ;
	if (os.fail())
		slog.Out("%s. Could not open file %s for writing\n", *_fn, *filename) ;
	else
	{
		for (nC = 0 ; nC < m_AllParagraphs.Count() ; nC++)
		{
			pPara = m_AllParagraphs.GetObj(nC) ;
			if (!pPara->m_nCell)
				os << "PARA: " << pPara->m_File << " line " << pPara->m_nLine << " col [" << pPara->m_nCol << pPara->m_Content ;
			else
				os << "CELL: " << pPara->m_nCell << ") " << pPara->m_File << " line " << pPara->m_nLine << " col [" << pPara->m_nCol << pPara->m_Content ;
			os << "]\n" ;
		}
		os.close() ;
	}
	os.clear() ;

	//g_treeNav.m_Root->m_bHide = false ;

	//	Assign pages to tree items
	slog.Out("%s. Building HTML tree\n", *_fn) ;
	//rc = BuildHtmlTree() ;
	//if (rc != E_OK)
	//	return rc ;
	//slog.Out("%s. Built HTML tree\n", *_fn) ;

	//	Write out the articles file
	filename = "articles.xml" ;
	filename = g_Project.m_Name + ".articles.xml" ;

	os.open(*filename) ;
	if (os.fail())
		{ slog.Out("%s. Could not open file %s for writing\n", *_fn, *filename) ; return E_OPENFAIL ; }
	Zr.Clear() ;
	Zr << "<passiveFiles>\n" ;
	Zr.Printf("\n<xtreeDcl name=\"%s\" page=\"%s\">\n", *m_Name, *m_Page) ;
	g_treeNav.ExportArticleSet(Zr) ;
	os << Zr ;
	os << "</xtreeDcl>\n" ;
	os << "</passiveFiles>\n" ;
	os.close() ;
	os.clear() ;

	return E_OK ;
}
