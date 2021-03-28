//
//	File:	hdsGraph.cpp
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

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>

#include "hzErrcode.h"
//#include "hzFsTbl.h"
#include "hzDissemino.h"

using namespace std ;

/*
**	Graphics Entity Constructors/Destructors
*/

hdsGraphic::hdsGraphic	(void)
{
	type = HZSHAPE_NULL ; 
	fillcolor = linecolor = DS_NULL_COLOR ;
	thick = 1 ;
	northY = northX = eastY = eastX = southY = southX = westY = westX = text_y = text_x = DS_NULL_COORD ;
	width = height = top = bot = lft = rht = rad = width = from = to = DS_NULL_COORD ;
}

hdsDiagram::hdsDiagram	(hdsApp* pApp)
{
	InitVE(pApp) ;
	m_pApp = pApp ;
	m_fillColor = m_lineColor = DS_NULL_COLOR ;
	m_Height = m_Width = DS_NULL_COORD ;
}

hdsDiagram::~hdsDiagram (void)
{
	hdsGraphic*	ptr ;	//	Current shape pointer
	uint32_t	x ;		//	Shape iterator

	for (x = 0 ; x < m_Shapes.Count() ; x++)
	{
		ptr = m_Shapes[x] ;
		delete ptr ;
	}
}

/*
**	hdsText - please note, not a VE
*/

hdsText::hdsText	(void)
{
	Clear() ;
}

void	hdsText::Clear	(void)
{
	text = (char*) 0 ;
	linecolor = 0 ;
	ypos = xpos = DS_NULL_COORD ;
	alignCode = 0 ;
}

hdsText	hdsText::operator=	(const hdsText& op)
{
	text = op.text ;
	linecolor = op.linecolor ;
	ypos = op.ypos ;
	xpos = op.xpos ;
	alignCode = op.alignCode ;
	return *this ;
}

/*
**	hdsPieChart
*/

hdsPieChart::hdsPieChart (hdsApp* pApp)
{
	InitVE(pApp) ;
	m_Total = 0.0 ;
	m_Height = m_Width = 0 ;
	m_ccY = m_ccX = m_Rad = m_IdxY = m_IdxX = m_IdxW = 0 ;
}

hdsPieChart::~hdsPieChart	(void)
{
	hzList<_pie*>::Iter pi ;	//	Pie chart component iterator
	_pie*	ptr ;				//	Pie chart component

	for (pi = m_Parts ; pi.Valid() ; pi++)
	{
		ptr = pi.Element() ;
		delete ptr ;
	}
}

/*
**	hdsBarChart
*/

hdsBarChart::hdsBarChart (hdsApp* pApp)
{
	InitVE(pApp) ;
	m_xpad = 0 ;
	m_gap = 5 ;
	m_minX = m_maxX = m_minY = m_maxY = DS_NULL_COORD ;
	m_xpix = m_ypix = m_xTop = m_xBot = m_yLft = m_yRht = DS_NULL_COORD ;
	m_Height = m_Width = 0 ;
	m_marginLft = m_marginRht = 20 ;
	m_marginTop = m_marginBot = 15 ;
}

hdsBarChart::~hdsBarChart	(void)
{
	hzList<_bset*>::Iter	bi ;	//	Barchart dataset iterator
	_bset*	ptr ;					//	Barchart dataset

	for (bi = m_Sets ; bi.Valid() ; bi++)
	{
		ptr = bi.Element() ;
		delete ptr ;
	}
}

/*
**	hdsBarChart
*/

hdsStdChart::hdsStdChart (hdsApp* pApp)
{
	InitVE(pApp) ;
	m_xpad = 0 ;
	m_minX = m_maxX = m_minY = m_maxY = DS_NULL_COORD ;
	m_xpix = m_ypix = m_xTop = m_xBot = m_yLft = m_yRht = DS_NULL_COORD ;
	m_marginLft = m_marginRht = 20 ;
	m_marginTop = m_marginBot = 15 ;
}

hdsStdChart::~hdsStdChart	(void)
{
	hzList<_rset*>::Iter	ri ;	//	Line chart dataset iterator
	_rset*	ptr ;					//	Line chart dataset

	for (ri = m_Sets ; ri.Valid() ; ri++)
	{
		ptr = ri.Element() ;
		delete ptr ;
	}
}

/*
**	Graphics Config Functions
*/

hzEcode	hdsApp::_readText	(hdsText& tx, hzXmlNode* pN)
{
	//	Read a HTML canvas text entity
	//
	//	Arguments:	1)	tx		The current text entity
	//				2)	pN		Current XML node
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If the text entity was set OK

	_hzfunc("hdsApp::_readText") ;

	hzAttrset		ai ;		//	Attribute iterator
	uint32_t		ln ;		//	Line number
	hzEcode			rc = E_OK ;	//	Return code

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("text"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <text>\n", *_fn, pN->TxtName()) ;
	ln = pN->Line() ;

	tx.Clear() ;
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("font"))			tx.font = ai.Value() ;
		else if (ai.NameEQ("str"))			tx.text = ai.Value() ;
		else if (ai.NameEQ("ypos"))			tx.ypos = ai.Value() ? atoi(ai.Value()) : -1 ;
		else if (ai.NameEQ("xpos"))			tx.xpos = ai.Value() ? atoi(ai.Value()) : -1 ;
		else if (ai.NameEQ("linecolor"))	IsHexnum(tx.linecolor, ai.Value()) ;
		else if (ai.NameEQ("align"))
		{
			if		(ai.ValEQ("left"))		tx.alignCode = 0 ;
			else if (ai.ValEQ("center"))	tx.alignCode = 1 ;
			else if (ai.ValEQ("right"))		tx.alignCode = 2 ;
			else
				{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <text> Bad alignment (%s)\n", *_fn, *pN->Filename(), ln, ai.Value()) ; break ; }
		}
		else
			{ rc = E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <text> Bad param (%s=%s)\n", *_fn, *pN->Filename(), ln, ai.Name(), ai.Value()) ; break ; }
	}

	if (tx.ypos == -1)	{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <text> No ypos supplied\n", *_fn, *pN->Filename(), ln) ; }
	if (tx.xpos == -1)	{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <text> No xpos supplied\n", *_fn, *pN->Filename(), ln) ; }
	if (!tx.text)		{ rc=E_SYNTAX ; m_pLog->Out("%s. File %s Line %d: <text> No string supplied\n", *_fn, *pN->Filename(), ln) ; }

	return rc ;
}

hdsVE*	hdsApp::_readPieChart	(hzXmlNode* pN)
{
	//	Read in configs for a pie chart. The current XML node is assumed to be at an <xpiechart> tag
	//
	//	Argument:	pN	The current XML node
	//
	//	Returns:	Pointer to the new pie chart as a visible entity
	//				NULL	If the any configuration errors occur

	_hzfunc("hdsApp::_readPieChart") ;

	hdsPieChart::_pie*	pPart = 0 ;	//	Pie chart part iterator
	hdsPieChart*		pPch ;		//	The chart

	hdsText			TX ;			//	Text item
	hzAttrset		ai ;			//	Attribute iterator
	hdsVE*			thisVE ;		//	Visible entity (pie chart)
	hzXmlNode*		pN1 ;			//	Subtag probe
	hzString		cfg ;			//	Config source file
	hzString		color ;			//	Color (hex string)
	hzString		typ ;			//	Basetype
	uint32_t		ln ;			//	Current line
	uint32_t		bErr = 0 ;		//	Error
	uint32_t		bSet = 0 ;		//	Parameter check list
	hzEcode			rc ;			//	Return code

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("xpiechart"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <xpiechart>\n", *_fn, pN->TxtName()) ;

	thisVE = pPch = new hdsPieChart(this) ;
	cfg = pN->Filename() ;
	thisVE->m_Line = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;

	//	All <xpiechart> tags have attrs of id, type, header and footer and bgcolor
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("id"))			pPch->m_Id = ai.Value() ;
		else if (ai.NameEQ("font"))			pPch->m_Font = ai.Value() ;
		else if (ai.NameEQ("dtype"))		typ = ai.Value() ;
		else if (ai.NameEQ("bgcolor"))		{ bSet |= 0x01 ; IsHexnum(pPch->m_BgColor, ai.Value()) ; }
		else if (ai.NameEQ("linecolor"))	{ bSet |= 0x02 ; IsHexnum(pPch->m_lineColor, ai.Value()) ; }
		else if (ai.NameEQ("fillcolor"))	{ bSet |= 0x04 ; IsHexnum(pPch->m_fillColor, ai.Value()) ; }
		else if (ai.NameEQ("height"))		{ bSet |= 0x08 ; pPch->m_Height = ai.Value() ? atoi(ai.Value()) : 0 ; }
		else if (ai.NameEQ("width"))		{ bSet |= 0x10 ; pPch->m_Width = ai.Value() ? atoi(ai.Value()) : 0 ; }
		else if (ai.NameEQ("rad"))			{ bSet |= 0x20 ; pPch->m_Rad = ai.Value() ? atoi(ai.Value()) : -1 ; }
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xpiechart> Bad param (%s=%s)\n", *_fn, *cfg, pN->Line(), ai.Name(), ai.Value()) ; }
	}

	//	Check values
	if (!pPch->m_Id)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xchart> Chart id not supplied\n", *_fn, *cfg, pN->Line()) ; }
	if (!pPch->m_Font)	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <params> No font\n", *_fn, *cfg, ln) ; }
	if (!(bSet & 0x08))	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <params> No canvas height\n", *_fn, *cfg, ln) ; }
	if (!(bSet & 0x10))	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <params> No canvas width\n", *_fn, *cfg, ln) ; }
	if (!(bSet & 0x20))	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <params> No radius\n", *_fn, *cfg, ln) ; }

	//	Get other params. All types have headers and footer but different graphic and other configs
	for (pN1 = pN->GetFirstChild() ; !bErr && pN1 ; pN1 = pN1->Sibling())
	{
		ln = pN1->Line() ;

		if (pN1->NameEQ("text"))
		{
			_readText(TX, pN1) ;
			pPch->m_Texts.Add(TX) ;
		}
		else if (pN1->NameEQ("part"))
		{
			pPart = new hdsPieChart::_pie() ;
			pPch->m_Parts.Add(pPart) ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("name"))		pPart->header = ai.Value() ;
				else if (ai.NameEQ("color"))	IsHexnum(pPart->color, ai.Value()) ;
				else if (ai.NameEQ("value"))	pPart->value = ai.Value() ? atoi(ai.Value()) : DS_NULL_GRAPH ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <part> Bad param (%s=%s)\n", *_fn, *cfg, pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!pPart->header)
				{ bErr=1; m_pLog->Out("%s. File %s Line %d: <part> No name\n", *_fn, *cfg, ln) ; }

			if (pPart->color == DS_NULL_COLOR)	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <part> Bad color\n", *_fn, *cfg, ln) ; }
			if (pPart->value == DS_NULL_GRAPH)	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <part> Bad value\n", *_fn, *cfg, ln) ; }
		}
		else if (pN1->NameEQ("index"))
		{
			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("type"))		{}
				else if (ai.NameEQ("top"))		pPch->m_IdxX = ai.Value() ? atoi(ai.Value()) : DS_NULL_COORD ;
				else if (ai.NameEQ("lft"))		pPch->m_IdxY = ai.Value() ? atoi(ai.Value()) : DS_NULL_COORD ;
				else if (ai.NameEQ("width"))	pPch->m_IdxW = ai.Value() ? atoi(ai.Value()) : DS_NULL_COORD ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <index> Bad param (%s=%s)\n", *_fn, *cfg, ln, ai.Name(), ai.Value()) ; }
			}
		}
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xpiechart> Bad subtag <%s>\n", *_fn, *cfg, ln, pN1->TxtName()) ; }
	}

	if (bErr)
		{ delete pPch ; return 0 ; }

	m_pLog->Out("%s. Declared xpiechart parmas\n", *_fn) ;
	return thisVE ;
}

hdsVE*	hdsApp::_readBarChart	(hzXmlNode* pN)
{
	//	Category:	HtmlGeneration
	//
	//	Read in parameters for displaying a standard (bar or line) chart (graph)
	//
	//	Argument:	pN	The current XML node
	//
	//	Returns:	Pointer to the new bar chart as a visible entity
	//				NULL	If the any configuration errors occur

	_hzfunc("hdsApp::_readBarChart") ;

	static	hzString	dfltFont = "10px Arial" ;	//	Default font settings

	hdsBarChart*	pBch ;		//	This chart

	hdsBarChart::_bset*	pSet ;	//	Dataset pointer

	hdsText			TX ;		//	Text item
	chIter			zi ;		//	For reading comma separated values
	hzAttrset		ai ;		//	Attribute iterator
	hzXmlNode*		pN1 ;		//	Subtag probe
	hdsVE*			thisVE ;	//	Visible entity/active entity
	double			yval ;		//	For reading numeric values
	double			xval ;		//	For reading numeric values
	const char*		j ;			//	For reading comma separated values
	hzString		tmp ;		//	Temp string (for comma separated values)
	hzString		cfg ;		//	Config source file
	hzString		typ ;		//	X/Y axis datatype
	hzString		min ;		//	X/Y axis min value
	hzString		max ;		//	X/Y axis max value
	hzString		mark ;		//	X/Y axis line marker
	hzString		count ;		//	X/Y axis value marker
	uint32_t		ln ;		//	Line in file
	hdbBasetype		eType ;		//	Basetype
	uint32_t		bErr = 0 ;	//	Error

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("xbarchart"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <xbarchart>\n", *_fn, pN->TxtName()) ;

	pSet = 0 ;
	thisVE = pBch = new hdsBarChart(this) ;
	cfg = pN->Filename() ;
	thisVE->m_Line = ln = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;

	//	All <xbarchart> tags have attrs of id, type
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("id"))			pBch->m_Id = ai.Value() ;
		else if (ai.NameEQ("bgcolor"))		IsHexnum(pBch->m_BgColor, ai.Value()) ;
		else if (ai.NameEQ("fgcolor"))		IsHexnum(pBch->m_FgColor, ai.Value()) ;
		else if (ai.NameEQ("linecolor"))	IsHexnum(pBch->m_lineColor, ai.Value()) ;
		else if (ai.NameEQ("fillcolor"))	IsHexnum(pBch->m_fillColor, ai.Value()) ;
		else if (ai.NameEQ("height"))		pBch->m_Height = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("width"))		pBch->m_Width = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("font"))			pBch->m_Font = ai.Value() ;
		else if (ai.NameEQ("gap"))			pBch->m_gap = ai.Value() ? atoi(ai.Value()) : 0 ;
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xbarchart> Bad param (%s=%s)\n", *_fn, *cfg, pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!pBch->m_Id)		{ bErr=1; m_pLog->Out("%s. File %s Line %d: <xbarchart> No id\n", *_fn, *cfg, ln) ; }
	if (!pBch->m_Font)		{ bErr=1; m_pLog->Out("%s. File %s Line %d: <xbarchart> No font\n", *_fn, *cfg, ln) ; }
	if (!pBch->m_Height)	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <xbarchart> No canvas height\n", *_fn, *cfg, ln) ; }
	if (!pBch->m_Width)		{ bErr=1; m_pLog->Out("%s. File %s Line %d: <xbarchart> No canvas width\n", *_fn, *cfg, ln) ; }

	m_pLog->Out("%s. Declaring xbarchart (%s)\n", *_fn, *pBch->m_Id) ;

	//	Get other params. All types have headers and footer but different graphic and other configs
	for (pN1 = pN->GetFirstChild() ; !bErr && pN1 ; pN1 = pN1->Sibling())
	{
		ln = pN1->Line() ;

		if (pN1->NameEQ("text"))
		{
			_readText(TX, pN1) ;
			pBch->m_Texts.Add(TX) ;
		}
		else if (pN1->NameEQ("xaxis"))
		{
			m_pLog->Out("%s. Doing xbarchart x-axis\n", *_fn) ;
			typ.Clear() ;
			min.Clear() ;
			max.Clear() ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("header"))	pBch->m_HdrX = ai.Value() ;
				else if (ai.NameEQ("dtype"))	typ = ai.Value() ;
				else if (ai.NameEQ("min"))		pBch->m_minX = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("max"))		pBch->m_maxX = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("pix"))		pBch->m_xpix = ai.Value() ? atoi(ai.Value()) : 0 ;
				else if (ai.NameEQ("step"))		pBch->m_xdiv = ai.Value() ? atoi(ai.Value()) : 0 ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> Bad param (%s=%s)\n", *_fn, *cfg, ln, ai.Name(), ai.Value()) ; }
			}

			if (!pBch->m_HdrX)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No x-axis header\n", *_fn, *cfg, ln) ; }
			if (!pBch->m_xpix)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No pixel depth set (xpix)\n", *_fn, *cfg, ln) ; }
			if (!pBch->m_xdiv)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No marker depth set (step)\n", *_fn, *cfg, ln) ; }

			if (!typ)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No datatype set (dtype)\n", *_fn, *cfg, ln) ; }

			if (pBch->m_minX == -1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No minimum value set\n", *_fn, *cfg, ln) ; }
			if (pBch->m_maxX == -1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No maximum value set\n", *_fn, *cfg, ln) ; }

			if (bErr)
				break ;

			eType = Str2Basetype(typ) ;
			if (eType == BASETYPE_UNDEF)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> Illegal datatype (%s)\n", *_fn, *cfg, ln, *typ) ; break ; }
			pBch->m_ratX = (pBch->m_maxX - pBch->m_minX)/pBch->m_xpix ;

			m_pLog->Out("%s. Declared xbarchart x-axis range of (%f - %f) over %d pixels with steps of %f (xrat %f)\n",
				*_fn, pBch->m_minX, pBch->m_maxX, pBch->m_xpix, pBch->m_xdiv, pBch->m_ratX) ;
		}
		else if (pN1->NameEQ("yaxis"))
		{
			m_pLog->Out("%s. Doing xbarchart y-axis\n", *_fn) ;
			typ.Clear() ;
			min.Clear() ;
			max.Clear() ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("header"))	pBch->m_HdrY = ai.Value() ;
				else if (ai.NameEQ("dtype"))	typ = ai.Value() ;
				else if (ai.NameEQ("min"))		pBch->m_minY = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("max"))		pBch->m_maxY = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("pix"))		pBch->m_ypix = ai.Value() ? atoi(ai.Value()) : 0 ;
				else if (ai.NameEQ("step"))		pBch->m_ydiv = ai.Value() ? atoi(ai.Value()) : 0 ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> Bad param (%s=%s)\n", *_fn, *cfg, pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!pBch->m_HdrY)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No y-axis header\n", *_fn, *cfg, pN1->Line()) ; }
			if (!pBch->m_ypix)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No pixel depth set (xpix)\n", *_fn, *cfg, pN1->Line()) ; }
			if (!pBch->m_ydiv)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No marker depth set (step)\n", *_fn, *cfg, pN1->Line()) ; }

			if (!typ)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No datatype set (dtype)\n", *_fn, *cfg, pN1->Line()) ; }
			if (pBch->m_minY == -1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No minimum value set\n", *_fn, *cfg, pN1->Line()) ; }
			if (pBch->m_maxY == -1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No maximum value set\n", *_fn, *cfg, pN1->Line()) ; }

			if (bErr)
				break ;

			eType = Str2Basetype(typ) ;
			if (eType == BASETYPE_UNDEF)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> Illegal datatype (%s)\n", *_fn, *cfg, pN1->Line(), *typ) ; break ; }
			//pBch->m_ypop = ((pBch->m_maxY - pBch->m_minY)/pBch->m_ydiv)+1 ;
			pBch->m_ratY = (pBch->m_maxY - pBch->m_minY)/pBch->m_ypix ;

			//	Place y-axis values in the m_vals array
			for (yval = pBch->m_minY ; yval <= pBch->m_maxY ; yval += pBch->m_ydiv)
				pBch->m_vals.Add(yval) ;

			m_pLog->Out("%s. Declared xbarchart y-axis range of (%f - %f) over %d pixels with steps of %f (yrat %f)\n",
				*_fn, pBch->m_minY, pBch->m_maxY, pBch->m_ypix, pBch->m_ydiv, pBch->m_ratY) ;
		}
		else if (pN1->NameEQ("component"))
		{
			//	The <component> tag names the dataset and sets a color for the trace. The values are provided as a simple list of x-values. There must be one
			//	x-value per y-axis marker.

			m_pLog->Out("%s. Doing <component>\n", *_fn) ;

			pSet = new hdsBarChart::_bset() ;
			pSet->start = pBch->m_vals.Count() ;
			pBch->m_Sets.Add(pSet) ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("color"))	IsHexnum(pSet->color, ai.Value()) ;
				else if (ai.NameEQ("name"))		pSet->header = ai.Value() ;
				else if (ai.NameEQ("values"))
				{
					//	break up param into separate values
					for (j = ai.Value() ; *j ; j++)
					{
						for (; *j <= CHAR_SPACE ; j++) ;

						//	Expect a digit
						if (!IsDigit(*j))
							{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> Bad value\n", *_fn, *cfg, ln) ; break ; }
						for (xval = 0.0 ; *j >= '0' && *j <= '9' ; j++)
							{ xval *= 10 ; xval += *j - '0' ; }
						pBch->m_vals.Add(xval) ;
						pSet->popl++ ;

						if (*j != CHAR_COMMA)
							continue ;
						if (!*j)
							break ;

						//	Expect a comma
						if (*j != CHAR_COMMA)
							{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> unexpected char [%c]\n", *_fn, *cfg, ln, *j) ; break ; }
					}
				}
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> Bad param (%s=%s)\n", *_fn, *cfg, pN1->Line(), ai.Name(), ai.Value()) ; }
			}
			m_pLog->Out("%s. Done <component> popl %d have now %d values\n", *_fn, pSet->popl, pBch->m_vals.Count()) ;
		}
		else if (pN1->NameEQ("index"))
		{
			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("type"))	{}
				else if (ai.NameEQ("top"))		pBch->m_IdxX = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("lft"))		pBch->m_IdxY = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("width"))	pBch->m_IdxY = ai.Value() ? atoi(ai.Value()) : -1 ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <index> Bad param (%s=%s)\n", *_fn, *cfg, pN1->Line(), ai.Name(), ai.Value()) ; }
			}
		}
		else
		{
			bErr=1 ;
			m_pLog->Out("%s. File %s Line %d: <xbarchart> Bad subtag %s\n", *_fn, *cfg, pN1->Line(), pN1->TxtName()) ;
		}
	}

	//	Calculate dimensions
	if (bErr)
		{ delete pBch ; return 0 ; }

	pBch->m_xTop = 40 ;
	pBch->m_yLft = 50 ;
	pBch->m_yRht = 50 + pBch->m_ypix ;
	pBch->m_xBot = pBch->m_xTop + pBch->m_xpix ;

	m_pLog->Out("%s. Declared xbarchart parmas\n", *_fn) ;
	return thisVE ;
}

hdsVE*	hdsApp::_readStdChart	(hzXmlNode* pN)
{
	//	Category:	HtmlGeneration
	//
	//	Read in parameters for displaying a standard (bar or line) chart (graph)
	//
	//	Argument:	pN	The current XML node
	//
	//	Returns:	Pointer to the new standard chart as a visible entity
	//				NULL	If the any configuration errors occur

	_hzfunc("hdsApp::_readStdChart") ;

	static	hzString	dfltFont = "10px Arial" ;	//	Default font settings

	hdsStdChart*	pChart ;	//	This chart

	hdsStdChart::_rset*	pSet ;	//	Dataset pointer

	hdsText			TX ;		//	Text item
	chIter			zi ;		//	For reading comma separated values
	hzChain			tmpChain ;	//	For processing explicit datasets
	hzAttrset		ai ;		//	Attribute iterator
	hzXmlNode*		pN1 ;		//	Subtag probe
	hdsVE*			thisVE ;	//	Visible entity/active entity
	double			yval ;		//	For reading numeric values
	double			xval ;		//	For reading numeric values
	hzString		tmp ;		//	Temp string (for comma separated values)
	hzString		cfg ;		//	Config source file
	hzString		typ ;		//	X/Y axis datatype
	hzString		min ;		//	X/Y axis min value
	hzString		max ;		//	X/Y axis max value
	hzString		mark ;		//	X/Y axis line marker
	hzString		count ;		//	X/Y axis value marker
	uint32_t		ln ;		//	Line in file
	uint32_t		bErr = 0 ;	//	Error
	hdbBasetype		eType ;		//	Basetype

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("xstdchart"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <xstdchart>\n", *_fn, pN->TxtName()) ;

	pSet = 0 ;
	thisVE = pChart = new hdsStdChart(this) ;
	cfg = pN->Filename() ;
	thisVE->m_Line = ln = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;

	//	All <xstdchart> tags have attrs of id, type
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("id"))			pChart->m_Id = ai.Value() ;
		else if (ai.NameEQ("bgcolor"))		IsHexnum(pChart->m_BgColor, ai.Value()) ;
		else if (ai.NameEQ("fgcolor"))		IsHexnum(pChart->m_FgColor, ai.Value()) ;
		else if (ai.NameEQ("linecolor"))	IsHexnum(pChart->m_lineColor, ai.Value()) ;
		else if (ai.NameEQ("fillcolor"))	IsHexnum(pChart->m_fillColor, ai.Value()) ;
		else if (ai.NameEQ("height"))		pChart->m_Height = ai.Value() ? atoi(ai.Value()) : -1 ;
		else if (ai.NameEQ("width"))		pChart->m_Width = ai.Value() ? atoi(ai.Value()) : -1 ;
		else if (ai.NameEQ("font"))			pChart->m_Font = ai.Value() ;
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xstdchart> Bad param (%s=%s)\n", *_fn, *cfg, pN->Line(), ai.Name(), ai.Value()) ; }
	}

	if (!pChart->m_Id)		{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xstdchart> No canvas id supplied\n", *_fn, *cfg, ln) ; }
	if (!pChart->m_Height)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xstdchart> No canvas height\n", *_fn, *cfg, ln) ; }
	if (!pChart->m_Width)		{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xstdchart> No canvas width\n", *_fn, *cfg, ln) ; }

	if (!pChart->m_Font)
		pChart->m_Font = dfltFont ;

	m_pLog->Out("%s. Declaring xstdchart (%s)\n", *_fn, *pChart->m_Id) ;

	//	Get other params. All types have headers and footer but different graphic and other configs
	for (pN1 = pN->GetFirstChild() ; !bErr && pN1 ; pN1 = pN1->Sibling())
	{
		ln = pN1->Line() ;

		if (pN1->NameEQ("text"))
		{
			_readText(TX, pN1) ;
			pChart->m_Texts.Add(TX) ;
		}
		else if (pN1->NameEQ("xaxis"))
		{
			m_pLog->Out("%s. Doing xstdchart x-axis\n", *_fn) ;
			typ.Clear() ;
			min.Clear() ;
			max.Clear() ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("header"))	pChart->m_HdrX = ai.Value() ;
				else if (ai.NameEQ("dtype"))	typ = ai.Value() ;
				else if (ai.NameEQ("min"))		pChart->m_minX = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("max"))		pChart->m_maxX = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("ypos"))		pChart->m_yLft = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("xpos"))		pChart->m_xTop = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("pix"))		pChart->m_xpix = ai.Value() ? atoi(ai.Value()) : 0 ;
				else if (ai.NameEQ("step"))		pChart->m_xdiv = ai.Value() ? atoi(ai.Value()) : 0 ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> Bad param (%s=%s)\n", *_fn, *cfg, ln, ai.Name(), ai.Value()) ; }
			}

			if (!pChart->m_HdrX)		{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No x-axis header\n", *_fn, *cfg, ln) ; }
			if (!pChart->m_xpix)		{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No pixel depth set (xpix)\n", *_fn, *cfg, ln) ; }
			if (!pChart->m_xdiv)		{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No marker depth set (step)\n", *_fn, *cfg, ln) ; }
			if (pChart->m_yLft == -1)	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <xaxis> No y-coord\n", *_fn, *cfg, ln) ; }
			if (pChart->m_xTop == -1)	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <xaxis> No top x-coord\n", *_fn, *cfg, ln) ; }

			if (!typ)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No datatype set (dtype)\n", *_fn, *cfg, ln) ; }

			if (pChart->m_minX == -1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No minimum value set\n", *_fn, *cfg, ln) ; }
			if (pChart->m_maxX == -1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> No maximum value set\n", *_fn, *cfg, ln) ; }

			if (bErr)
				break ;

			eType = Str2Basetype(typ) ;
			if (eType == BASETYPE_UNDEF)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> Illegal datatype (%s)\n", *_fn, *cfg, ln, *typ) ; break ; }
			//pChart->m_xpop = ((pChart->m_maxX - pChart->m_minX)/pChart->m_xdiv)+1 ;
			pChart->m_ratX = (pChart->m_maxX - pChart->m_minX)/pChart->m_xpix ;

			m_pLog->Out("%s. Declared xstdchart x-axis range of (%f - %f) over %d pixels with steps of %f (xrat %f)\n",
				*_fn, pChart->m_minX, pChart->m_maxX, pChart->m_xpix, pChart->m_xdiv, pChart->m_ratX) ;
		}
		else if (pN1->NameEQ("yaxis"))
		{
			m_pLog->Out("%s. Doing xstdchart y-axis\n", *_fn) ;
			typ.Clear() ;
			min.Clear() ;
			max.Clear() ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("header"))	pChart->m_HdrY = ai.Value() ;
				else if (ai.NameEQ("dtype"))	typ = ai.Value() ;
				else if (ai.NameEQ("min"))		pChart->m_minY = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("max"))		pChart->m_maxY = ai.Value() ? atoi(ai.Value()) : -1 ;
				//else if (ai.NameEQ("xpos")	pChart->m_ypix = ai.Value() ? atoi(ai.Value()) : 0 ;
				//else if (ai.NameEQ("ypos")	pChart->m_ypix = ai.Value() ? atoi(ai.Value()) : 0 ;
				else if (ai.NameEQ("pix"))		pChart->m_ypix = ai.Value() ? atoi(ai.Value()) : 0 ;
				else if (ai.NameEQ("step"))		pChart->m_ydiv = ai.Value() ? atoi(ai.Value()) : 0 ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> Bad param (%s=%s)\n", *_fn, *cfg, pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			if (!pChart->m_HdrY)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No y-axis header\n", *_fn, *cfg, pN1->Line()) ; }
			if (!pChart->m_ypix)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No pixel depth set (xpix)\n", *_fn, *cfg, pN1->Line()) ; }
			if (!pChart->m_ydiv)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No marker depth set (step)\n", *_fn, *cfg, pN1->Line()) ; }

			if (!typ)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No datatype set (dtype)\n", *_fn, *cfg, pN1->Line()) ; }
			if (pChart->m_minY == -1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No minimum value set\n", *_fn, *cfg, pN1->Line()) ; }
			if (pChart->m_maxY == -1)	{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> No maximum value set\n", *_fn, *cfg, pN1->Line()) ; }

			if (bErr)
				break ;

			eType = Str2Basetype(typ) ;
			if (eType == BASETYPE_UNDEF)
				{ bErr = 1 ; m_pLog->Out("%s. File %s Line %d: <yaxis> Illegal datatype (%s)\n", *_fn, *cfg, pN1->Line(), *typ) ; break ; }
			//pChart->m_ypop = ((pChart->m_maxY - pChart->m_minY)/pChart->m_ydiv)+1 ;
			pChart->m_ratY = (pChart->m_maxY - pChart->m_minY)/pChart->m_ypix ;

			m_pLog->Out("%s. Declared xstdchart y-axis range of (%f - %f) over %d pixels with steps of %f (yrat %f)\n",
				*_fn, pChart->m_minY, pChart->m_maxY, pChart->m_ypix, pChart->m_ydiv, pChart->m_ratY) ;
		}
		else if (pN1->NameEQ("dataset"))
		{
			//	The <dataset> tag names the dataset and sets a color for the trace. The values are provided as a simple list of x-values. There need not be any
			//	correlation between the number of y,x value pairs and the number of y-axis markers. There must be at least two y,x value pairs. The value pairs
			//	will be of the form {y,x}

			m_pLog->Out("%s. Doing <dataset>\n", *_fn) ;
			pSet = new hdsStdChart::_rset() ;
			pSet->start = pChart->m_vals.Count() ;
			pChart->m_Sets.Add(pSet) ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("color"))	IsHexnum(pSet->color, ai.Value()) ;
				else if (ai.NameEQ("name"))		pSet->header = ai.Value() ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xaxis> Bad param (%s=%s)\n", *_fn, *cfg, pN1->Line(), ai.Name(), ai.Value()) ; }
			}

			//	if (pN1->m_tmpContent.Size())
			//		zi = pN1->m_tmpContent ;
			//	else
			//		pN1->m_tmpContent = pN1->m_fixContent ;

			tmpChain = pN1->m_fixContent ;

			for (zi = tmpChain ; !zi.eof() ; zi++)
			{
				for (; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

				if (*zi != '{')
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <dataset> Expected an opening brace (got %c)\n", *_fn, *cfg, ln, *zi) ; break ; }
				zi++ ;

				//	Expect a number
				if (!IsDigit(*zi))
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <dataset> Bad value\n", *_fn, *cfg, ln) ; break ; }
				for (yval = 0.0 ; *zi >= '0' && *zi <= '9' ; zi++)
					{ yval *= 10 ; yval += *zi - '0' ; }
				pChart->m_vals.Add(yval) ;
				pSet->popl++ ;

				//	Expect a comma
				if (*zi != CHAR_COMMA)
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <dataset> Expected a comma (got %c)\n", *_fn, *cfg, ln, *zi) ; break ; }
				zi++ ;

				//	Expect a number
				if (!IsDigit(*zi))
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <dataset> Bad value\n", *_fn, *cfg, ln) ; break ; }
				for (xval = 0.0 ; *zi >= '0' && *zi <= '9' ; zi++)
					{ xval *= 10 ; xval += *zi - '0' ; }
				pChart->m_vals.Add(xval) ;
				pSet->popl++ ;

				//	Expect a closing '}'
				if (*zi != '}')
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <dataset> Expected a closing brace (got %c)\n", *_fn, *cfg, ln, *zi) ; break ; }
				zi++ ;
				if (*zi == CHAR_COMMA)
					zi++ ;
			}
			m_pLog->Out("%s. Done <dataset> popl %d have now %d values\n", *_fn, pSet->popl, pChart->m_vals.Count()) ;
		}
		else if (pN1->NameEQ("index"))
		{
			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("type"))		{}
				else if (ai.NameEQ("top"))		pChart->m_IdxX = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("lft"))		pChart->m_IdxY = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("width"))	pChart->m_IdxY = ai.Value() ? atoi(ai.Value()) : -1 ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <index> Bad param (%s=%s)\n", *_fn, *cfg, pN1->Line(), ai.Name(), ai.Value()) ; }
			}
		}
		else
		{
			bErr=1 ;
			m_pLog->Out("%s. File %s Line %d: <xstdchart> Bad subtag %s\n", *_fn, *cfg, pN1->Line(), pN1->TxtName()) ;
		}
	}

	//	Calculate dimensions
	if (bErr)
		{ delete pChart ; return 0 ; }

	//	pChart->m_xTop = 40 ;
	//	pChart->m_yLft = 50 ;
	pChart->m_yRht = 50 + pChart->m_ypix ;
	pChart->m_xBot = pChart->m_xTop + pChart->m_xpix ;

	m_pLog->Out("%s. Declared xchart parmas\n", *_fn) ;
	return thisVE ;
}

hzEcode	hdsApp::_readShapes	(hzXmlNode* pN, hdsDiagram* pDiag)
{
	//	Read in a single diagram component
	//
	//	Arguments:	1)	pN		The current XML node expected to be a <shapes> tag
	//				2)	pDiag	The current diagram
	//
	//	Returns:	E_SYNTAX	If there is a syntax error
	//				E_OK		If no errors occured

	_hzfunc("hdsApp::_readShape") ;

	hzMapS<hzString,hdsGraphic*>	tmp ;	//	Map of graphics (for connectors)

	hdsGraphic*		pShape = 0 ;			//	Diagram component (new)
	hdsGraphic*		pShape_x = 0 ;			//	Diagram component (previously existing)

	hzAttrset		ai ;					//	Attribute iterator
	hzXmlNode*		pN1 ;					//	Subtag probe
	hzString		cfg ;					//	Config source file
	hzString		name ;					//	Node name
	hzString		from ;					//	For connector from (shape id)
	hzString		to ;					//	For connector to (shape id)
	uint32_t		dfltFillColor = -1 ;	//	Default fill color
	uint32_t		ln ;					//	Line number
	uint32_t		flag ;					//	Flags to control which attributes apply to which shape types
	uint32_t		maxY = 0 ;				//	Highest y-coord
	uint32_t		maxX = 0 ;				//	Highest x-coord
	uint32_t		bErr = 0 ;				//	Error if set

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("shapes"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <shapes>\n", *_fn, pN->TxtName()) ;
	if (!pDiag)
		Fatal("%s. No diagram supplied\n", *_fn) ;
	cfg = pN->Filename() ;

	//	Process params here
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if (ai.NameEQ("fillcolor"))
			IsHexnum(dfltFillColor, ai.Value()) ;
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <%s> Bad param (%s=%s)\n", *_fn, *cfg, ln, pN->TxtName(), ai.Name(), ai.Value()) ; }
	}

	//	Process child nodes (shapes)
	for (pN1 = pN->GetFirstChild() ; pN1 ; pN1 = pN1->Sibling())
	{
		ln = pN1->Line() ;
		name = pN1->TxtName() ;

		pShape = new hdsGraphic() ;
		pShape->nid = pDiag->m_Shapes.Count() ;
		pDiag->m_Shapes.Add(pShape) ;

		if		(name == "line")		{ flag = 0x04FF ; pShape->type = HZSHAPE_LINE ; }
		else if (name == "arrow")		{ flag = 0x0CFF ; pShape->type = HZSHAPE_ARROW ; }
		else if (name == "diamond")		{ flag = 0x04FF ; pShape->type = HZSHAPE_DIAMOND ; }
		else if (name == "rect")		{ flag = 0x04FF ; pShape->type = HZSHAPE_RECT ; }
		else if (name == "rrect")		{ flag = 0x04FF ; pShape->type = HZSHAPE_RRECT ; }
		else if (name == "stadium")		{ flag = 0x04FF ; pShape->type = HZSHAPE_STADIUM ; }
		else if (name == "connect")		{ flag = 0x3CFF ; pShape->type = HZSHAPE_CONNECT ; }
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xdiagram> Bad subtag <%s>\n", *_fn, *cfg, ln, *name) ; }

		for (ai = pN1 ; ai.Valid() ; ai.Advance())
		{
			if		(flag & 0x0001 && ai.NameEQ("id"))		pShape->uid = ai.Value() ;
			else if	(flag & 0x0002 && ai.NameEQ("text"))	pShape->text = ai.Value() ;
			else if	(flag & 0x0003 && ai.NameEQ("txY"))		pShape->text_y = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if	(flag & 0x0008 && ai.NameEQ("txX"))		pShape->text_x = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if	(flag & 0x0010 && ai.NameEQ("top"))		pShape->top = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if (flag & 0x0020 && ai.NameEQ("bot"))		pShape->bot = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if (flag & 0x0040 && ai.NameEQ("lft"))		pShape->lft = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if (flag & 0x0080 && ai.NameEQ("rht"))		pShape->rht = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if (flag & 0x0100 && ai.NameEQ("rad"))		pShape->rad = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if (flag & 0x0200 && ai.NameEQ("width"))	pShape->width = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if (flag & 0x0400 && ai.NameEQ("thick"))	pShape->thick = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if (flag & 0x0800 && ai.NameEQ("head"))	pShape->head = ai.Value() ? atoi(ai.Value()) : -1 ;
			else if (flag & 0x1000 && ai.NameEQ("from"))	from = ai.Value() ;
			else if (flag & 0x2000 && ai.NameEQ("to"))		to = ai.Value() ;
			else
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <%s> Bad param (%s=%s)\n", *_fn, *cfg, ln, *name, ai.Name(), ai.Value()) ; }
		}

		if (!pShape->uid)
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <%s> No shape id supplied\n", *_fn, *cfg, ln, *name) ; }
		else
		{
			if (tmp.Exists(pShape->uid))
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <%s> Duplicate shape id (%s)\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }
			else
				tmp.Insert(pShape->uid, pShape) ;
		}

		if (pShape->type == HZSHAPE_CONNECT)
		{
			if (!from)
				{ bErr=1; m_pLog->Out("%s. File %s Line %d: <%s> No from shape id supplied\n", *_fn, *cfg, ln, *name) ; }
			if (!to)
				{ bErr=1; m_pLog->Out("%s. File %s Line %d: <%s> No from shape id supplied\n", *_fn, *cfg, ln, *name) ; }

			pShape_x = tmp[pShape->uid] ;
			if (!pShape_x)
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <%s> Cannot locate shape %s\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }
			else
				pShape->from = pShape_x->nid ;

			pShape_x = tmp[pShape->uid] ;
			if (!pShape_x)
				{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <%s> Cannot locate shape %s\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }
			else
				pShape->to = pShape_x->nid ;

			from = (char*) 0 ;
			to = (char*) 0 ;
			continue ;
		}

		if (pShape->lft == -1)			{ bErr=1; m_pLog->Out("%s. File %s Line %d: <%s> No Lft coord supplied\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }
		if (pShape->top == -1)			{ bErr=1; m_pLog->Out("%s. File %s Line %d: <%s> No Top coord supplied\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }
		if (pShape->rht == -1)			{ bErr=1; m_pLog->Out("%s. File %s Line %d: <%s> No Rht coord supplied\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }
		if (pShape->bot == -1)			{ bErr=1; m_pLog->Out("%s. File %s Line %d: <%s> No Bot coord supplied\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }
		if (pShape->top > pShape->bot)	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <%s> Top/Bot inversion\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }
		if (pShape->lft > pShape->rht)	{ bErr=1; m_pLog->Out("%s. File %s Line %d: <%s> Lft/Rht reversal\n", *_fn, *cfg, ln, *name, *pShape->uid) ; }

		//	Set canvas limits
		if (pShape->bot > maxX)	maxX = pShape->bot ;
		if (pShape->rht > maxY)	maxY = pShape->rht ;

		//	Set width and height
		pShape->width = pShape->rht - pShape->lft ;
		pShape->height = pShape->bot - pShape->top ;

		//	Set connection points
		pShape->northY = pShape->lft + (pShape->width/2) ;
		pShape->northX = pShape->top ;

		pShape->eastY = pShape->rht ;
		pShape->eastX = pShape->top + (pShape->height/2) ;

		pShape->southY = pShape->lft + (pShape->width/2) ;
		pShape->southX = pShape->bot ;

		pShape->westY = pShape->lft ;
		pShape->westX = pShape->top + (pShape->height/2) ;

		if (pShape->type == HZSHAPE_STADIUM)
		{
			pShape->rad = (pShape->bot - pShape->top)/2 ;
			pShape->text_y = pShape->lft + pShape->rad ;
			pShape->text_x = pShape->top + pShape->rad ;
		}

		if (pShape->fillcolor == DS_NULL_COLOR)
		{
			if (dfltFillColor == DS_NULL_COLOR)
				pShape->fillcolor = 0 ;
			else
				pShape->fillcolor = dfltFillColor ;
		}
	}

	return bErr ? E_SYNTAX : E_OK ;
}

hdsVE*	hdsApp::_readDiagram	(hzXmlNode* pN)
{
	//	Read in diagram components.
	//
	//	<xdiagram id="mydiagram" header="flowchart">
	//		<stadium top="20" bot="70" lft="50" rht="150" text="Start"/>
	//		<arrow from="70,100" to"120,100" width="2" head="10" color="0"/> 
	//		<diamond top="120" bot="220" lft="50" rht="150" text="Are we ready?"/>
	//		<arrow from="170,150" to"70,100" width="2" head="10" color="0" text="N"/> 
	//		<arrow from="220,100" to"270,100" width="2" head="10" color="0" text="Y"/> 
	//		<stadium top="270" bot="320" lft="50" rht="150" text="Go!"/>
	//	</xdiagram>
	//
	//	Argument:	pN	The current XML node expected to be a <xdiagram> tag
	//
	//	Returns:	Pointer to diagram visual entity

	_hzfunc("hdsApp::_readDiagram") ;

	hdsText			TX ;		//	Diagram Text component
	hdsLine			LN ;		//	Diagram Line component
	hzAttrset		ai ;		//	Attribute iterator
	hzXmlNode*		pN1 ;		//	Subtag probe
	hdsDiagram*		pDiag ;		//	This diagram
	hdsVE*			thisVE ;	//	Visible entity/active entity
	hzString		cfg ;		//	Config source file
	uint32_t		ln ;		//	Line number
	uint32_t		bErr = 0 ;	//	Error if set

	if (!pN)
		Fatal("%s. No node supplied\n", *_fn) ;
	if (!pN->NameEQ("xdiagram"))
		Fatal("%s. Incorrect node (%s) supplied. Must be <xdiagram>\n", *_fn, pN->TxtName()) ;

	thisVE = pDiag = new hdsDiagram(this) ;
	cfg = pN->Filename() ;
	thisVE->m_Line = ln = pN->Line() ;
	thisVE->m_Indent = pN->Level() ;

	//	Get diagram params
	for (ai = pN ; ai.Valid() ; ai.Advance())
	{
		if		(ai.NameEQ("id"))			pDiag->m_Id = ai.Value() ;
		else if (ai.NameEQ("bgcolor"))		IsHexnum(pDiag->m_BgColor, ai.Value()) ;
		else if (ai.NameEQ("fgcolor"))		IsHexnum(pDiag->m_FgColor, ai.Value()) ;
		else if (ai.NameEQ("linecolor"))	IsHexnum(pDiag->m_lineColor, ai.Value()) ;
		else if (ai.NameEQ("fillcolor"))	IsHexnum(pDiag->m_fillColor, ai.Value()) ;
		else if (ai.NameEQ("height"))		pDiag->m_Height = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("width"))		pDiag->m_Width = ai.Value() ? atoi(ai.Value()) : 0 ;
		else if (ai.NameEQ("font"))			pDiag->m_Font = ai.Value() ;
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xdiagram> Bad param (%s=%s)\n", *_fn, *cfg, ln, ai.Name(), ai.Value()) ; }
	}

	//	Check values
	if (!pDiag->m_Id)		{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xdiagram> No canvas id\n", *_fn, *cfg, ln) ; }
	if (!pDiag->m_Height)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xdiagram> No canvas height\n", *_fn, *cfg, ln) ; }
	if (!pDiag->m_Width)	{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xdiagram> No canvas width\n", *_fn, *cfg, ln) ; }

	if (bErr)
		{ delete pDiag ; return 0 ; }

	//	Get components
	for (pN1 = pN->GetFirstChild() ; pN1 ; pN1 = pN1->Sibling())
	{
		ln = pN1->Line() ;

		if (pN1->NameEQ("shapes"))
		{
			if (_readShapes(pN1, pDiag) != E_OK)
				bErr = 1 ;
		}
		else if (pN1->NameEQ("line"))
		{
			LN.Clear() ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("linecolor"))	IsHexnum(LN.linecolor, ai.Value()) ;
				else if (ai.NameEQ("y1"))			LN.y1 = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("x1"))			LN.x1 = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("y2"))			LN.y2 = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("x2"))			LN.x2 = ai.Value() ? atoi(ai.Value()) : -1 ;
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <line> Bad param (%s=%s)\n", *_fn, *cfg, ln, ai.Name(), ai.Value()) ; }
			}

			pDiag->m_Lines.Add(LN) ;
		}
		else if (pN1->NameEQ("text"))
		{
			TX.Clear() ;

			for (ai = pN1 ; ai.Valid() ; ai.Advance())
			{
				if		(ai.NameEQ("str"))			TX.text = ai.Value() ;
				else if (ai.NameEQ("ypos"))			TX.ypos = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("xpos"))			TX.xpos = ai.Value() ? atoi(ai.Value()) : -1 ;
				else if (ai.NameEQ("linecolor"))	IsHexnum(TX.linecolor, ai.Value()) ;
				else if (ai.NameEQ("align"))
				{
					if (ai.ValEQ("left"))	TX.alignCode = 0 ;
					if (ai.ValEQ("center"))	TX.alignCode = 1 ;
					if (ai.ValEQ("right"))	TX.alignCode = 2 ;
				}
				else
					{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <text> Bad param (%s=%s)\n", *_fn, *cfg, ln, ai.Name(), ai.Value()) ; }
			}

			pDiag->m_Texts.Add(TX) ;
		}
		else
			{ bErr=1 ; m_pLog->Out("%s. File %s Line %d: <xdiagram> Bad subtag <%s>\n", *_fn, *cfg, pN1->Line(), pN1->TxtName()) ; }
	}

	//	Calculate dimensions
	if (bErr)
		{ delete pDiag ; return 0 ; }

	m_pLog->Out("%s. Declared xchart parmas\n", *_fn) ;
	return thisVE ;
}
