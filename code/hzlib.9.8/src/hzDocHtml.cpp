//
//	File:	hzDocHtml.cpp
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
//	Management of HTML documents
//

#include <fstream>

#include <sys/stat.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzDocument.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Variables
*/

static	hzMapS<hzString,hzHtagform>		s_htagNam ;		//	All HTML tags by name
static	hzMapS<hzHtagtype,hzHtagform>	s_htagTyp ;		//	All HTML tags by type

static	hzHtagform	s_tagformDuff ;						//	Null tag form
static	uint32_t	s_htagPop ;							//	This is set by InitHtml() to the number of HTML tags, to indicate that the tags have been set up.

/*
**	SECTION 1:	HTML Tag Types
*/

hzEcode	InitHtml	(void)
{
	//	Category:	Data Initialization
	//
	//	Populate the map of tag names to tag forms and the map of tag types to tag forms (see hzHtagform definition). This facilitates HTML tag lookup for such
	//	purposes as the import and processing of HTML documents.
	//
	//	Arguments:	None
	//
	//	Returns:	E_SETONCE	If the HTML maps are already populated
	//				E_OK		If the operation was successful

	_hzfunc(__func__) ;

	if (s_htagPop)
		return hzerr(_fn, HZ_WARNING, E_SETONCE) ;

	hzHtagform	t ;		//	Full tag info for insertion

	//	Default (invalid)
	t.klas=HTCLASS_NUL; t.rule=HTRULE_NULL; t.type=HTAG_NULL;		t.name=0;			s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Page structure tags
	t.klas=HTCLASS_HDR; t.rule=HTRULE_SINGLE; t.type=HTAG_DOCTYPE;		t.name="!DOCTYPE";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_HTML;			t.name="html";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_HEAD;			t.name="head";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_TITLE;		t.name="title";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_META;			t.name="meta";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_BODY;			t.name="body";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_BASE;			t.name="base";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_BASEFONT;		t.name="basefont";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_STYLE;		t.name="style";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

	//	Programing tags
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_SCRIPT;		t.name="script";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_NOFRAMES;		t.name="noframes";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_NOSCRIPT;		t.name="noscript";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_HDR; t.rule=HTRULE_PAIRED; t.type=HTAG_APPLET;		t.name="applet";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

	//	Frames 	 
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_FRAME;		t.name="frame";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_FRAMESET;		t.name="frameset";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_IFRAME;		t.name="iframe";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_SINGLE; t.type=HTAG_PARAM;		t.name="param";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

	//	System tags
	t.klas=HTCLASS_SYS; t.rule=HTRULE_PAIRED; t.type=HTAG_EMBED;		t.name="embed";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_SYS; t.rule=HTRULE_PAIRED; t.type=HTAG_NOEMBED;		t.name="noembed";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

	//	Font control or text tags - no content
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_BOLD;			t.name="b";			s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_ULINE;		t.name="u";			s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HATG_ITALIC;		t.name="i";			s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_EM;			t.name="em";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_STRONG;		t.name="strong";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_CENTER;		t.name="center";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_FONT;			t.name="font";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_BIG;			t.name="big";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_SMALL;		t.name="small";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_SINGLE; t.type=HATG_BR;			t.name="br";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_SINGLE; t.type=HTAG_HR;			t.name="hr";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

	//	Text description tags
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_HEADER;		t.name="header";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_FOOTER;		t.name="footer";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_SECTION;		t.name="section";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_ARTICLE;		t.name="article";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_ASIDE;		t.name="aside";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_DETAILS;		t.name="details";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_SUMMARY;		t.name="summary";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_DIALOG;		t.name="dialog";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Text grouping tags
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_STRIKE;		t.name="strike";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_S;			t.name="s";			s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_DEL;			t.name="del";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_INS;			t.name="ins";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_KBD;			t.name="kbd";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_TXT; t.rule=HTRULE_PAIRED; t.type=HTAG_SPAN;			t.name="span";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Text control tags
	t.klas=HTCLASS_DAT; t.rule=HTRULE_OPTION; t.type=HTAG_PARAG;		t.name="p";			s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_QUOTATION;	t.name="q";			s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_H1;			t.name="h1";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_H2;			t.name="h2";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_H3;			t.name="h3";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_H4;			t.name="h4";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_H5;			t.name="h5";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_H6;			t.name="h6";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HATG_TT;			t.name="tt";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_CODE;			t.name="code";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_SAMP;			t.name="samp";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_CITE;			t.name="cite";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_CAPTION;		t.name="caption";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_VAR;			t.name="var";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_PRE;			t.name="pre";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_BQ;			t.name="bq";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_BLOCKQUOTE;	t.name="blockquote";s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAB_BDO;			t.name="bdo";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_SUBSCRIPT;	t.name="sub";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_SUPERSCRIPT;	t.name="sup";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Data/layout tags
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_TABLE;		t.name="table";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_OPTION; t.type=HTAG_TCOL;			t.name="col";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_OPTION; t.type=HTAG_TCOLGRP;		t.name="colgroup";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_TH;			t.name="th";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_TR;			t.name="tr";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_TBL_CEL;		t.name="td";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_DIV;			t.name="div";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_TBODY;		t.name="tbody";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_THEAD;		t.name="thead";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_TFOOT;		t.name="tfoot";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_FIELDSET;		t.name="fieldset";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_LEGEND;		t.name="legend";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_MENU;			t.name="menu";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_DT;			t.name="dt";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_DD;			t.name="dd";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_DFN;			t.name="dfn";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_DIR;			t.name="dir";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_DLIST;		t.name="dl";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_OLIST;		t.name="ol";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_ULIST;		t.name="ul";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_ITEM;			t.name="li";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_HGROUP;		t.name="hgroup";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_DAT; t.rule=HTRULE_PAIRED; t.type=HTAG_TIME;			t.name="time";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Link tags
	t.klas=HTCLASS_LNK; t.rule=HTRULE_PAIRED; t.type=HTAG_ANCHOR;		t.name="a";			s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_LNK; t.rule=HTRULE_PAIRED; t.type=HTAG_NAV;			t.name="nav";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_LNK; t.rule=HTRULE_PAIRED; t.type=HTAG_LINK;			t.name="link";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Input/form tags
	t.klas=HTCLASS_INP; t.rule=HTRULE_PAIRED; t.type=HTAG_FORM;			t.name="form";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INP; t.rule=HTRULE_SINGLE; t.type=HTAG_INPUT;		t.name="input";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INP; t.rule=HTRULE_PAIRED; t.type=HTAG_TEXTAREA;		t.name="textarea";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INP; t.rule=HTRULE_PAIRED; t.type=HTAG_SELECT;		t.name="select";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INP; t.rule=HTRULE_PAIRED; t.type=HTAG_OPTGROUP;		t.name="optgroup";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INP; t.rule=HTRULE_PAIRED; t.type=HTAG_OPTION;		t.name="option";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INP; t.rule=HTRULE_PAIRED; t.type=HTAG_BUTTON;		t.name="button";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INP; t.rule=HTRULE_PAIRED; t.type=HTAG_LABEL;		t.name="label";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Information tags
	t.klas=HTCLASS_INF; t.rule=HTRULE_PAIRED; t.type=HTAG_ABBR;			t.name="abbr";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INF; t.rule=HTRULE_PAIRED; t.type=HTAG_ACRONYM;		t.name="acronym";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_INF; t.rule=HTRULE_PAIRED; t.type=HTAG_ADDRESS;		t.name="address";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Image tags
	t.klas=HTCLASS_IMG; t.rule=HTRULE_SINGLE; t.type=HTAG_IMG;			t.name="img";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_MAP;			t.name="map";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_AREA;			t.name="area";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_OBJECT;		t.name="object";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_MARQUEE;		t.name="marquee";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_CANVAS;		t.name="canvas";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_FIGCAPTION;	t.name="figcaption";s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_FIGURE;		t.name="figure";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_AUDIO;		t.name="audio";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_SOURCE;		t.name="source";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_TRACK;		t.name="track";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);
	t.klas=HTCLASS_IMG; t.rule=HTRULE_PAIRED; t.type=HTAG_VIDEO;		t.name="video";		s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

 	//	Third party tags
	t.klas=HTCLASS_3RD; t.rule=HTRULE_SINGLE; t.type=HTAG_FBLIKE;		t.name="fb:like";	s_htagTyp.Insert(t.type,t); s_htagNam.Insert(t.name,t);

	s_htagPop = s_htagNam.Count() ;
	return E_OK ;
}

const char* Doctype2Txt	(hzDoctype dtype)
{
	//	Category:	Diagnostics
	//
	//	Convert hzDoctype enum to text for diagnostics
	//
	//	Arguments:	1)	dtype	The enumerated document type (either HTML or XML)
	//
	//	Returns:	Pointer to the doctype text form

	static	const char*	strings	[] =
	{
		"DOCTYPE_UNDEFINED",
		"DOCTYPE_HTML",
		"DOCTYPE_XML",
		""
	} ;

	if (dtype < 0 || dtype >= DOCTYPE_XML)
		return strings[0] ;
	return strings[dtype] ;
}
	
hzString	Tagtype2Txt	(hzHtagtype type)
{
	//	Category:	Diagnostics
	//
	//	Convert a HTML tag type (enum) into a string naming the type
	//
	//	Arguments:	1)	dtype	The enumerated document type (either HTML or XML)
	//
	//	Returns:	Instance of hzString by value

	//	If tagmap not loaded, load it
	if (!s_htagNam.Count())
		InitHtml() ;

	if (type < HTAG_NULL)
		return s_tagformDuff.name ;

	if (s_htagTyp.Count() <= (uint32_t) type)
		return s_tagformDuff.name ;

	return s_htagTyp[type].name ;
}

hzHtagtype	Txt2Tagtype	(const hzString& htag)
{
	//	Category:	Config
	//
	//	Convert a string representing a HTML tag type, into the HTML tag type.
	//
	//	Arguments:	1)	htag	A string presumed to be one of the allowed HTML5 tags
	//
	//	Returns:	Enumerated hzHtagtype

	_hzfunc(__func__) ;

	hzHtagform	tf ;		//	HTML tag info
	hzString	S ;			//	HTML tag search string

	//	If tagmap not loaded, load it
	if (!s_htagPop)
		InitHtml() ;

	S = htag ;
	S.ToLower() ;

	tf = s_htagNam[S] ;

	return tf.type ;
}

const hzHtagform&	TagLookup	(const hzString& htag)
{
	//	Category:	Internet
	//
	//	Lookup and return the hzHtagform (tag function class). The search is by tagname.
	//
	//	Arguments:	1)	htag	A string presumed to be one of the allowed HTML5 tags
	//
	//	Returns:	Reference to the tag form for the tag

	//	If tagmap not loaded, load it
	if (!s_htagNam.Count())
		InitHtml() ;

	return s_htagNam[htag] ;
}

const hzHtagform&	TagLookup	(chIter& ci)
{
	//	Category:	Internet
	//
	//	Determine if the supplied chain iterator, is at the start of a legal HTML tag or anti-tag
	//
	//	Arguments:	1)	ci	A chain iterator to be tested to see if it is at the begening of an allowed HTML5
	//
	//	Returns:	Reference to the tag form for the tag

	hzChain		W ;			//	Working chain
	chIter		xi ;		//	Internal chain iterator
	hzString	word ;		//	Individual word

	//	If tagmap not loaded, load it
	if (!s_htagNam.Count())
		InitHtml() ;

	xi = ci ;
	if (*xi != CHAR_LESS)
		return s_tagformDuff ;
	xi++ ;
	if (*xi == CHAR_FWSLASH)
		xi++ ;

	for (;;)
	{
		if (*xi == CHAR_SPACE)	break ;
		if (*xi == CHAR_MORE)	break ;

		W.AddByte(*xi) ;
		xi++ ;
	}

	word = W ;
	word.ToLower() ;
	return s_htagNam[word] ;
}

/*
**	Tag cleanup
*/

hzHtagInd	AtHtmlTag	(hzString& tagseq, chIter& ci)
{
	//	Category:	Text Processing
	//
	//	Determines if the supplied chain iterator marks the start of a sequence that amounts to a legal HTML tag or anti-tag. If it does not 0 is returned and
	//	the supplied string will be empty. If the sequence has the right form, a case-insensitive lookup is performed to test the name part against all known
	//	HTML5 tags. If this finds a match the supplied string will be populated with the sequence (including the opening and closing angle brackets). The return
	//	value will then be either 1 for the tag or 2 for the anti-tag.
	//
	//	Arguments:	1)	tagseq	If a tag is found, this string reference will be populated by it.
	//				2)	ci		The test chain iterator
	//
	//	Returns:	HTRULE_NULL		If the sequence is not a known HTML tag or antitag.
	//				HTRULE_PAIRED	If the sequence is a HTML tag.
	//				HTRULE_SINGLE	If the sequence is a HTML antitag.
	//				HTRULE_OPTION	If the sequence is both a HTML tag and antitag (eg <br/>).

	_hzfunc(__func__) ;

	hzChain		W ;				//	For building tagname
	chIter		zi ;			//	Used to iterate whole tag sequence.
	hzHtagform	tf ;			//	The tag form for the found tag (if any).
	hzString	tagname ;		//	The tag name
	hzHtagInd	retval ;		//	Return value (0 invalid, 1 tag, 2 anti-tag)

	//	If tagmap not loaded, load it
	if (!s_htagNam.Count())
		InitHtml() ;

	//	Clear the supplied tag and set chain iter
	tagseq = 0 ;
	zi = ci ;

	if (*zi != CHAR_LESS)
		return HTAG_IND_NULL ;

	zi++ ;
	if (*zi == CHAR_FWSLASH)
		{ retval = HTAG_IND_ANTI ; zi++ ; }
	else
		retval = HTAG_IND_OPEN ;

	for (; !zi.eof() && IsAlpha(*zi) ; zi++)
		W.AddByte(*zi) ;
	if (!W.Size())
		return HTAG_IND_NULL ;

	tagname = W ;
	W.Clear() ;
	tagname.ToLower() ;
	tf = s_htagNam[tagname] ;
	if (tf.type == HTAG_NULL)
		return HTAG_IND_NULL ;

	//	We have a HTML tag so build the complete tag for populating tagseq
	for (zi = ci ; !zi.eof() ; zi++)
	{
		W.AddByte(*zi) ;

		if (*zi == CHAR_DQUOTE)
		{
			for (zi++ ; !zi.eof() ; zi++)
			{
				W.AddByte(*zi) ;

				if (*zi == CHAR_BKSLASH)
					{ zi++ ; W.AddByte(*zi) ; }

				if (*zi == CHAR_DQUOTE)
					break ;
			}
			continue ;
		}

		if (*zi == CHAR_FWSLASH)
		{
			if (zi == "/>")
				{ retval = HTAG_IND_SELF ; zi++ ; W.AddByte(*zi) ; }
		}

		if (*zi == CHAR_MORE)
			break ;
	}
			
	if (*zi != CHAR_MORE)
		return HTAG_IND_NULL ;

	tagseq = W ;
	return retval ;
}

void	XmlCleanHtags	(hzChain& output, const hzChain& input)
{
	//	Category:	Text Processing
	//
	//	Remove all instance of <, > and & and replace them with &lt;, &gt; and &amp; respectively
	//
	//	Arguments:	1)	output	The cleaned output
	//				2)	input	The unclean input
	//
	//	Returns:	None

	chIter		zi ;		//	Chain iterator
	uint32_t	ent ;		//	Entity value (needed by call to AtEntity)
	uint32_t	entLen ;	//	Entity value (needed by call to AtEntity)

	for (zi = input ; !zi.eof() ; zi++)
	{
		if (*zi == CHAR_LESS)
			output << "&lt;" ;
		else if (*zi == CHAR_MORE)
			output << "&gt;" ;
		else if (*zi == CHAR_AMPSAND)
		{
			if (AtEntity(ent, entLen, zi))
				output.AddByte(*zi) ;
			else
				output << "&amp;" ;
		}
		else
			output.AddByte(*zi) ;
	}
}

hzEcode	hzDocument::Init	(const hzUrl& url)
{
	//	Initialize a hzDocument with a URL
	//
	//	Arguments:	1)	url		The URL of the document
	//
	//	Returns:	E_INITDUP	If the document is already associated with a URL
	//				E_OK		If the document URL is set

	_hzfunc("hzDocument::Init") ;

	if (*m_Info.m_urlReq)
	{
		if (m_Info.m_urlReq == url)
			return hzerr(_fn, HZ_ERROR, E_INITDUP, "%s. Duplicate call. Address already set to %s\n", *_fn, *m_Info.m_urlReq) ;
		return hzerr(_fn, HZ_ERROR, E_INITDUP, "%s. Duplicate call. Addr=%s, arg=%s\n", *_fn, *m_Info.m_urlReq, *url) ;
	}

	m_Info.m_urlReq = url ;

	return E_OK ;
}

hzDocHtml::hzDocHtml   (void)
{
	m_pRoot = 0 ;
	m_pHead = 0 ;
	m_pBody = 0 ;
	_hzGlobal_Memstats.m_numDochtm++ ;
}

hzDocHtml::~hzDocHtml  (void)
{
	threadLog("Called hzDocHtml::~hzDocHtml") ;

	_hzGlobal_Memstats.m_numDochtm-- ;
	Clear() ;
}


hzHtmElem*	hzDocHtml::_proctag	(hzHtmElem* pParent, chIter& ci, hzHtagtype type)
{
	//	This assumes the chain iterator is currently at a '<' char and that this is the start of an HTML tag or ant-tag. To succeed the tag must be
	//	both a known HTML tag and of the correct form.
	//
	//	If successful, the iterator will be advanced to one place beyond the terminating '>'. If unsuccessful, the iterator will be left unchanged.
	//
	//	Arguments:	1)	The parent node
	//				2)	The iterator.
	//				3)	The current tag type. This determines how structural defects are to be handled.#
	//
	//	Returns:	Pointer to a new hzHtmElem if the operation was sussessful
	//				NULL if function could not identify a tag
	//
	//	Scope:		Private to the hzDocHtml class.

	_hzfunc("hzDocHtml::_proctag") ;

	hzChain			theTag ;		//	The full text of the tag
	hzChain			Z ;				//	For building param names and values
	hzAttrset		ai ;			//	Attribute iterator
	chIter			end ;			//	End of tag marker
	chIter			xi ;			//	Main operating chain iterator
	chIter			yi ;			//	Shadow chain iterator
	hzHtmElem*		pX ;			//	Parent element
	hzHtmElem*		pNewnode ;		//	Tag found (new copy created)
	hzUrl			link_url ;		//	URL for links
	hzNumPair		attr ;			//  Attribute name/value pair
	hzString		tnam ;			//	Tag name
	hzString		attrName ;		//	Attr name
	hzString		attrValue ;		//	Attr value
	hzString		S ;				//	Temporary string
	uint32_t		nLine ;			//	Line number of tag
	hzHtagtype		ptype ;			//	Parent tag's type

	//bool	bQuot = false ;
	bool	bError = false ;

	//	Check validity of call
	if (ci.eof())
		{ threadLog("%s. Invalid iterator\n", *_fn) ; return 0 ; }

	nLine = ci.Line() ;

	if (*ci != '<')
		{ threadLog("%s. Line %d Wrong call\n", *_fn, nLine) ; return 0 ; }

	switch	(type)
	{
	case HTAG_META:			//	Examininig a <META ...> tag
	case HTAG_STYLE:		//	Examininig a <META ...> tag
	case HTAG_SCRIPT:		//	Examininig a <SCRIPT .> tag
	case HTAG_LINK:			//	Examininig a <LINK ...> tag
	case HTAG_HTML:			//	Examininig a <HTML ...> tag (header)
	case HTAG_BODY:			//	Examininig a <BODY ...> tag (body)
		break ;
	default:
		break ;
	}

	//	Pre-process the tag and get tag name
	xi = ci ;
	xi++ ;

	if (!IsAlpha(*xi))
		{ threadLog("%s. Line %d Non-tag (< followed by non-alpha %d)\n", *_fn, nLine, *xi) ; return 0 ; }

	for (; !xi.eof() && (*xi == CHAR_COLON || IsAlphanum(*xi)) ; xi++)
		theTag.AddByte(*xi) ;

	if (!theTag.Size())
		{ threadLog("%s. Line %d Tag un-named\n", *_fn, nLine) ; return 0 ; }
	tnam = theTag ;

	//	Check if tag is known as a HTML tag
	if (type == HTAG_TABLE)
		pNewnode = new hzHtmTbl() ;
	else
		pNewnode = new hzHtmElem() ;
	pNewnode->Init(this, pParent, tnam, type, m_vecTags.Count(), ci.Line()) ;
	m_vecTags.Add(pNewnode) ;
	//threadLog("procTag: New tag %s at %p\n", *tnam, pNewnode) ;

	//	Collect tag attributes if any
	for (; !xi.eof() ;)
	{
		if (IsWhite(*xi))
			{ xi++ ; continue ; }

		if (*xi == CHAR_FWSLASH)
		{
			if (xi == "/>")
				{ pNewnode->_setanti(xi.Line()) ; xi++ ; end = xi ; break ; }
		}

		if (*xi == CHAR_MORE)
			{ end = xi ; break ; }

		//	Not at end of tag, so should have attr=value sequence (otherwise error)
		if (!IsAlpha(*xi))
			{ threadLog("%s. Line %d Error. Unexpected char is [%c]\n", *_fn, nLine, *xi) ; xi++ ; continue ; }

		Z.Clear() ;
		for (; !xi.eof() && (IsUrlnorm(*xi) || *xi == CHAR_COLON || *xi == CHAR_PERIOD || *xi == CHAR_MINUS || *xi == CHAR_USCORE) ; xi++)
			Z.AddByte(*xi) ;
		attrName = Z ;

		Z.Clear() ;
		attrValue = (char*)0 ;

		for (; !xi.eof() && IsWhite(*xi) ; xi++) ;

		if (*xi != CHAR_EQUAL)
		{
			//	Tag attribute does not have a value assignent part (="some_val"). This is an error although there are some slopy exceptions,
			//	eg 'allowfullscreen' in the <tframe> tag.

			if (pNewnode->Type() == HTAG_IFRAME || pNewnode->Type() == HTAG_TIME)
			{
				attrValue = attrName ;
				//pNewnode->AddAttr(attrName, attrValue) ;

				attr.m_snName = m_Dict.Insert(*attrName) ;
				attr.m_snValue = m_Dict.Insert(*attrValue) ;
				m_NodeAttrs.Insert(pNewnode->GetUid(), attr) ;

				//	threadLog("%s. Line %d Tag %s. attr=%s/value=%s. Char now [%c]\n", *_fn, nLine, *tnam, *attrName, *attrValue, *xi) ;
				continue ;
			}

			threadLog("%s. Line %d Tag %s param %s not assigned\n", *_fn, nLine, *tnam, *attrName) ;
			return 0 ;
		}

		//	Get attribute value
		for (xi++ ; !xi.eof() && IsWhite(*xi) ; xi++) ;

		Z.Clear() ;
		if (*xi == CHAR_DQUOTE)
		{
			for (xi++ ; !xi.eof() && *xi != CHAR_DQUOTE ; xi++)
				Z.AddByte(*xi) ;
			if (xi.eof())
				{ threadLog("%s. Line %d Double-quote non-closure disqualifies tag\n", *_fn, nLine) ; return 0 ; }
			xi++ ;
		}
		else if (*xi == CHAR_SQUOTE)
		{
			for (xi++ ; !xi.eof() && *xi != CHAR_SQUOTE ; xi++)
				Z.AddByte(*xi) ;
			if (xi.eof())
				{ threadLog("%s. Line %d Single-quote non-closure disqualifies tag\n", *_fn, nLine) ; return 0 ; }
			xi++ ;
		}
		else
		{
			for (; !xi.eof() && IsUrlresv(*xi) ; xi++)
			Z.AddByte(*xi) ;
		}
		attrValue = Z ;

		//	If the tag is a link/anchor and attr is named 'href' then add link to the list of links found in the page
		if ((pNewnode->Type() == HTAG_LINK || pNewnode->Type() == HTAG_ANCHOR) && attrName.Equiv("href"))
		{
			//threadLog("%s. Considering link %s\n", *_fn, *attrValue) ;

			//	Is the link a mailto ?
			if (!attrValue)
			{
				S = theTag ;
				threadLog("%s. Line %d null link in tag %s\n", *_fn, nLine, *S) ;
			}
			else
			{
				if (attrValue[0] != CHAR_HASH)
				{
					if (memcmp(*attrValue, "mailto:", 7) == 0)
					{
						S = *attrValue + 7 ;
						m_Emails.Insert(S) ;
						//threadLog("%s. emaddr of %s\n", *_fn, *S) ;
					}
					else
					{
						//	Add the link

						if (m_Base && attrValue[0] == CHAR_FWSLASH)
						{
							link_url.SetValue(m_Base, attrValue) ;
							if (!link_url)
								threadLog("not a link case 1: %s\n", *attrValue) ;
							//threadLog("%s. Setting link with base %s\n", *_fn, *m_Base) ;
						}
						else if (m_Info.Domain())
						{
							link_url.SetValue(m_Info.Domain(), attrValue) ;
							if (!link_url)
								threadLog("not a link case 2: %s\n", *attrValue) ;
							//threadLog("%s. Setting link with domain %s\n", *_fn, *m_Info.Domain()) ;
						}
						else
						{
							link_url = attrValue ;
							if (!link_url)
								threadLog("not a link case 3: %s\n", *attrValue) ;
							//threadLog("%s. Setting naken link %s\n", *_fn, *link_url) ;
						}

						if (!link_url.Domain())
							threadLog("not a link case 4: %s\n", *link_url) ;

						if (link_url)
						{
							S = *link_url ;

							attr.m_snName = m_Dict.Insert(*attrName) ;
							attr.m_snValue = m_Dict.Insert(*attrValue) ;
							m_NodeAttrs.Insert(pNewnode->GetUid(), attr) ;

							if (!m_setLinks.Exists(link_url))
							{
								m_setLinks.Insert(link_url) ;
								m_vecLinks.Add(link_url) ;
							}
						}
					}
				}
			}
		}
		else
		{
			//pNewnode->AddAttr(attrName, attrValue) ;

			attr.m_snName = m_Dict.Insert(*attrName) ;
			attr.m_snValue = m_Dict.Insert(*attrValue) ;
			m_NodeAttrs.Insert(pNewnode->GetUid(), attr) ;
		}

		//	threadLog("%s. Line %d Tag %s. attr=%s/value=%s. Char now [%c]\n", *_fn, nLine, *tnam, *attrName, *attrValue, *xi) ;
	}

	if (xi.eof())
		{ threadLog("%s. Line %d A. non-closure disqualifies tag\n", *_fn, nLine) ; return 0 ; }

	if (*xi != CHAR_MORE)
		{ S = theTag ; threadLog("%s. Line %d C. malformed tag <%s> pnam=%s, attrValue=%s [%c]\n", *_fn, nLine, *S, *attrName, *attrValue, *xi) ; return 0 ; }

	for (xi++ ; !xi.eof() && IsWhite(*xi) ; xi++) ;
	end = xi ;

	//	Check for correct parentage
	if (pParent)
	{
 		//	Some tag-type rules

		ptype = pParent->Type() ;

		if (type == HTAG_TBL_CEL)
		{
			if (ptype == HTAG_TBL_CEL)
			{
				//	This is where the author has forgotton to close a <td> and is now adding the next <td> in the row. We
				//	seek back to the <tr> (the true parent).

				threadLog("%s. WARNING: Missing </td> anti-tag\n", *_fn) ;

				pX = pParent->Parent() ;
				if (pX)
				{
					ptype = pX->Type() ;
					if (ptype != HTAG_TH || ptype != HTAG_TR)
						pParent = pX ;
				}
			}
		}

		if (bError)
			threadLog("%s. WARNING: New <%s> tag has parent of <%s>\n", *_fn, *Tagtype2Txt(type), *Tagtype2Txt(ptype)) ;
	}

	ci = end ;
	//m_mapTags.Insert(pNewnode->Name(), pNewnode) ;
	return pNewnode ;
}

hzEcode	hzDocHtml::_htmPreproc	(hzChain& Z)
{
	//	Remove comments and non applicable conditional comments from HTML
	//
	//	Arguments:	1)	Reference to chain to be pre-processed
	//
	//	Returns:	E_FORMAT	If the HTML is malformed
	//				E_OK		If the HTML was successfully processed

	chIter	zi ;		//	Iterator of input
	hzChain	X ;			//	Target chain
	hzChain	word ;		//	Diagnostics chain
	bool	bIn ;		//	In a conditional comment

	if (Z.Size() == 0)
		return E_OK ;

	for (zi = Z ; !zi.eof() ;)
	{
		if (*zi != CHAR_LESS)
			{ X.AddByte(*zi) ; zi++ ; continue ; }

		if (zi == "<!-->")
			{ zi += 5 ; continue ; }

		//	Ignore deleted text within comment (<!-- and -->) tags. Note these cannot be nested
		bIn = false ;

		if (zi == "<!--[if")
			{ bIn = true ; zi += 7 ; }
		if (zi == "<![if")
			{ bIn = true ; zi += 5 ; }

		if (bIn)
		{
			for (; !zi.eof() && *zi <= CHAR_SPACE ;)
				zi++ ;

			if (zi == "!IE")
			{
				//	Specific non-IE comment. Content herein must be allowed through.
				//	threadLog("%s. Allowing non-IE cond comment line %d - ", __func__, zi.Line()) ;

				for (zi += 2 ; !zi.eof() && *zi != CHAR_MORE ; zi++) ;
				if (zi.eof())
				{
					threadLog("Unterminated conditional comment (line %d)\n", zi.Line()) ;
					return E_FORMAT ;
				}

				zi++ ;
				if (zi == "-->")
					zi += 3 ;

				for (; !zi.eof() ; zi++)
				{
					if (*zi == CHAR_LESS)
					{
						if (zi == "<![endif]>")			{ zi += 10 ; break ; }
						if (zi == "<![endif]-->")		{ zi += 12 ; break ; }
						if (zi == "<![endif]>-->")		{ zi += 13 ; break ; }
						if (zi == "<!--<![endif]-->")	{ zi += 16 ; break ; }
					}

					word.AddByte(*zi) ;
					X.AddByte(*zi) ;
				}

				//threadLog("word is %s\n", *word
				//m_Error << "\nword is: " << word ;
				//m_Error.AddByte(CHAR_NL) ;
				word.Clear() ;
				continue ;
			}

			if (zi == "!(")
				zi += 2 ;

			if (zi == "lte IE" || zi == "lt IE" || zi == "gte IE" || zi == "gt IE" || zi == "IE")
			{
				//	We are not and never will be IE so ignore conditional comment
				//	threadLog("%s. Stripping IE cond comment line %d - ", __func__, zi.Line()) ;

				for (zi += 2 ; !zi.eof() ; zi++)
				{
					if (zi == "<![endif]>")		{ zi += 10 ; break ; }
					if (zi == "<![endif]-->")	{ zi += 12 ; break ; }
				}
				continue ;
			}

			//	Include non IE stuff
			//	threadLog("%s. Stripping non-IE cond comment line %d - ", __func__, zi.Line()) ;

			for (zi += 2 ; !zi.eof() && *zi != CHAR_MORE ; zi++) ;
			if (zi.eof())
			{
				threadLog("%s. Unterminated conditional comment (line %d)\n", __func__, zi.Line()) ;
				return E_FORMAT ;
			}

			zi++ ;
			if (zi == "<!-->")
				zi += 5 ;
			if (zi == "-->")
				zi += 3 ;

			for (; !zi.eof() ; zi++)
			{
				if (*zi == CHAR_LESS)
				{
					if (zi == "<![endif]>")			{ zi += 10 ; break ; }
					if (zi == "<![endif]-->")		{ zi += 12 ; break ; }
					if (zi == "<![endif]>-->")		{ zi += 13 ; break ; }
					if (zi == "<!--<![endif]-->")	{ zi += 16 ; break ; }
				}

				word.AddByte(*zi) ;
				X.AddByte(*zi) ;
			}

			//m_Error << "\nword is: " << word ;
			//m_Error.AddByte(CHAR_NL) ;
			word.Clear() ;
			continue ;
		}

		if (zi == "<!--")
		{
			for (zi += 4 ; !zi.eof() ; zi++)
			{
				if (zi == "-->")
					{ zi += 3 ; break ; }
			}

			if (zi.eof())
			{
				threadLog("_htmPreproc. Unterminated normal comment starting on line %d\n", zi.Line()) ;
				return E_FORMAT ;
			}
			continue ;
		}

		X.AddByte(*zi) ;
		zi++ ;
	}

	if (X.Size() == Z.Size())
		return E_OK ;

	Z.Clear() ;
	Z = X ;
	return E_OK ;
}

hzEcode	hzDocHtml::Load	(hzChain& Z)
{
	//	Populate the hzDocHtml object with HTML source code in the supplied chain.
	//
	//	Two scenarios are permitted - Full or Partial as follows:-
	//		1)	Full:		If the HTML source has the <html> as its first tag it will be considered as a full page and tested as such.
	//						It will be expected to have the standard sub-tags of <head> and <body> and thier corresponding anti-tags.
	//						If either of these are missing or in error (malformed or containing unxpected or malformed tags) the HTML
	//						source code is deemed to be syntactically in error and the load fails.
	//
	//		2)	Partial:	If the opening tag of the HTML source code is not the <html> tag it is viable only if it would be viable as
	//						a HTML fragment that could be seemlessly inserted into the <body> part of a whole HTML page. This is to say
	//						that all it's tags must be legal sub-tags of <body> and not of <head> and nor must the <body> or <head> tag
	//						or anti-tag be present.
	//
	//	In either case, tags are loaded into a tree of nodes (tags). The nodes/tags may be searched for and examined. 
	//
	//	Arguments:	1)	Z	The chain containing the HTML document
	//
	//	Returns:	E_FORMAT	If the HTML was rejected by the the HTML pre-processor _htmlPreproc() OR if any tags could not be processed by _proctag()
	//				E_OK		If the HTML was loaded successfully
	//
	//	Note:	Unlike XML where tags are named so that content in the tree can be searched directly, the nodes in HTML are not named
	//	named and so cannot be definitely referenced (they only have type). Some other process must apply application specific criteria
	//	to read meaning into the data.

	_hzfunc("hzDocHtml::Load") ;

	hzChain			nc ;			//	Node content
	hzChain			T ;				//	For token building
	hzChain			W ;				//	For token building
	chIter			zi ;			//	Chain iterator
	chIter			tw_start ;		//	Start of tagword marker
	chIter			tmp ;			//	Start of tagword marker
	chIter			limit ;			//	End of tag marker - Protection against malformed tags (NLA style)
	hzHtmElem*		pCN = 0 ;		//	Current HTML node
	hzHtmElem*		pNN ;			//	New HTML node
	hzHtmElem*		pX ;			//	HTML node for diagnostics
	hzHtmElem*		pCurForm = 0 ;	//	HTML node for diagnostics
	hzAttrset		ai ;			//	Attribute iterator
	hzHtmForm*		pForm = 0 ;		//	Form found in page
	hzPair			P ;				//	Name value pair (for forms and fields)
	hzString		strval ;		//	To test if current tag is being closed
	hzString		tagword ;		//	From MakeTag - just the tagname.
	hzString		wholetag ;		//	From MakeTag - the entire opening sequence if applicable
	hzString		anam ;			//	Attribute name
	hzString		aval ;			//	Attribute value
	hzHtagform		tf ;			//	Tag form
	uint32_t		nX ;			//	For nesting levels/general iteration
	uint32_t		nColon ;		//	Does the tagname contain a colon (3rd party tag)
	uint32_t		nLine ;			//	Line number for errors
	uint32_t		quote ;			//	Are we in a quoted string
	bool			bAnti ;			//	Tag is an anti-tag
	int32_t			cDelim ;		//	Delimiting char (single/double quote)
	hzEcode			rc = E_OK ;		//	return code

	Clear() ;
	//m_Error.Clear() ;

	//	Pre-process the HTML
	rc = _htmPreproc(Z) ;
	if (rc != E_OK)
		return rc ;

	m_Content = Z ;

	//	Make sure the HTML tags are loading into the lookup table
	if (!s_htagNam.Count())
		InitHtml() ;

	//	Init the iterator
	zi = Z ;
	zi.Skipwhite() ;

	//	Bypass the doctype if present
	if (zi.Equiv("<!DOCTYPE"))
	{
		quote = 0 ;
		for (zi += 9 ; !zi.eof() ; zi++)
		{
			if (quote)
			{
				if (*zi == CHAR_DQUOTE)
					quote = 0 ;
				continue ;
			}

			if (*zi == CHAR_MORE)
				{ zi++ ; break ; }

			if (*zi == CHAR_DQUOTE)
				quote = 1 ;
		}

		zi.Skipwhite() ;
	}

	//	Look for the opening <html>
	for (; !zi.eof() ; zi++)
	{
		if (zi.Equiv("<html"))
		{
			m_pRoot = _proctag(0, zi, HTAG_HTML) ;
			if (!m_pRoot)
				{ threadLog("Could not establist root node (the <html> tag)\n") ; return E_FORMAT ; }
			break ;
		}
	}

	if (!m_pRoot)
	{
		threadLog("%s. No valid contents found before expected <html> tag - assuming a partial page\n", *_fn) ;
		zi = Z ;
		zi.Skipwhite() ;
		pCN = new hzHtmElem() ;
		pCN->Init(this, 0, tagword, HTAG_NULL, m_vecTags.Count(), zi.Line()) ;
		m_vecTags.Add(pCN) ;
	}
	else
	{
		//	A <html> tag has been found so this is a full page. Look for <head> next
		for (; !zi.eof() ;)
		{
			if (zi.Equiv("<head"))
			{
				m_pHead = _proctag(m_pRoot, zi, HTAG_HEAD) ;
				if (!m_pHead)
					{ threadLog("%s. Could not process <head> tag\n", *_fn) ; return E_FORMAT ; }
				break ;
			}
			zi++ ;
		}

		if (!m_pHead)
			{ threadLog("%s. Expected a <head> tag\n", *_fn) ; return E_FORMAT ; }
		pCN = m_pHead ;

		//	Now get the subtags of <head>
		for (; rc == E_OK && pCN && !zi.eof() ;)
		{
			//	Handle tag content
			if (*zi != CHAR_LESS)
			{
				//	Ignore certain constructs
				if (zi == "//")
				{
					for (zi += 2 ; !zi.eof() && *zi != CHAR_NL ; zi++) ;
					continue ;
				}

				//	If not part of a construct, just agregate the char to the current tag's content, striping leading whitespace
				if (*zi <= CHAR_SPACE && pCN->m_tmpContent.Size() == 0)
					{ zi++ ; continue ; }
				pCN->m_tmpContent.AddByte(*zi) ;
				zi++ ;
				continue ;
			}

			//	Ignore deleted text within comment (<!-- and -->) tags. Note these cannot be nested
			nLine = zi.Line() ;

			if (zi == "<!--[if")
			{
				for (zi += 7 ; !zi.eof() ; zi++)
				{
					if (zi == "<![endif]>")		{ zi += 10 ; break ; }
					if (zi == "<![endif]-->")	{ zi += 12 ; break ; }
				}
				continue ;
			}

			if (zi == "<![if")
			{
				for (zi += 5 ; !zi.eof() ; zi++)
				{
					if (zi == "<![endif]>")		{ zi += 10 ; break ; }
					if (zi == "<![endif]-->")	{ zi += 12 ; break ; }
				}
				continue ;
			}

			if (zi == "<!--")
			{
				for (zi += 4 ; !zi.eof() ; zi++)
				{
					if (zi == "-->")
						{ zi += 3 ; break ; }
				}
				continue ;
			}

			//  Handle <![CDATA[...]]> block by converting the innards to straight data (apparently CDATA now legal in HTML)
			if (zi == "<![CDATA[")
			{
				for (zi += 9 ; !zi.eof() ; zi++)
				{
					if (zi == "]]>")
						{ zi += 3 ; break ; }
					pCN->m_tmpContent.AddByte(*zi) ;
				}
				continue ;
			}

			//	Eliminate <noscript> tags from header (we don't use them)
			if (zi == "<noscript")
			{
				for (zi += 9 ; !zi.eof() ; zi++)
				{
					if (zi == "</noscript>")
						{ zi += 11 ; break ; }
				}
				if (zi.eof())
					{ threadLog("%s. Unclosed <noscript> block\n", *_fn) ; rc = E_FORMAT ; break ; }
				continue ;
			}

 			//	At this point we have the '<' start of tag char. Establish whole and tagword of possible HTML tag

			wholetag = 0 ;
			tagword = 0 ;
	
			limit = zi ;
			limit++ ;
			W.AddByte(CHAR_LESS) ;
			bAnti = false ;
			if (*limit == CHAR_FWSLASH)
				{ W.AddByte(CHAR_FWSLASH) ; bAnti = true ; limit++ ; }

			nColon = 0 ;
			for (tw_start = limit ; !limit.eof() ; limit++)
			{
				if (*limit == CHAR_COLON || IsAlphanum(*limit))
				{
					if (*limit == CHAR_COLON)
						nColon++ ;

					T.AddByte(*limit) ;
					W.AddByte(*limit) ;
					continue ;
				}
				break ;
			}
			tagword = T ;
			T.Clear() ;
		
			for (; !limit.eof() ;)
			{
				W.AddByte(*limit) ;

				if (*limit == CHAR_DQUOTE || *limit == CHAR_SQUOTE)
				{
					cDelim = *limit ;

					for (limit++ ; !limit.eof() ; limit++)
					{
						if (*limit == CHAR_BKSLASH)
						{
							limit++ ;
							if (*limit == cDelim)
								continue ;
						}
						if (*limit == cDelim)
							break ;
					}
				}

				if (*limit == CHAR_MORE)
					break ;
				limit++ ;
			}

			wholetag = W ;
			W.Clear() ;

			if (*limit != CHAR_MORE)
			{
				threadLog("%s. Malformed tag (%s)\n", *_fn, *wholetag) ;
				zi = limit ;
				continue ;
			}
			limit++ ;

			//tagword.ToLower() ;

			if (nColon)
			{
				if (!s_htagNam.Exists(tagword))
				{
					tf.klas = HTCLASS_3RD ;
					tf.rule = HTRULE_OPTION ;
					tf.name = tagword ;
					s_htagTyp.Insert(tf.type, tf) ;
					s_htagNam.Insert(tf.name, tf) ;
					threadLog("%s. Inserted 3rd party HTML tag %s\n", *_fn, *tagword) ;
				}
			}

			if (!s_htagNam.Exists(tagword))
			{
				if (bAnti)
					threadLog("%s. Line %d case 1 Unknown lookup anti-tag </%s> (%s)\n", *_fn, zi.Line(), *tagword, *wholetag) ;
				else
					//threadLog("%s. Line %d Case 1 Unknown lookup tag <%s> (%s)\n", *_fn, zi.Line(), *tagword, *wholetag) ;
					threadLog("%s. Line %d Case 1 Unknown lookup tag <%s> (%d bytes)\n", *_fn, zi.Line(), *tagword, wholetag.Length()) ;

				pCN->m_tmpContent << wholetag ;
				zi = limit ;
				continue ;
			}

			tf = s_htagNam[tagword] ;

			if (tf.type == HTAG_NULL)
			{
				if (bAnti)
					threadLog("%s. Line %d case 2 Unknown lookup anti-tag </%s> (%s)\n", *_fn, zi.Line(), *tagword, *wholetag) ;
				else
					//threadLog("%s. Line %d Case 2 Unknown lookup tag <%s> (%s)\n", *_fn, zi.Line(), *tagword, *wholetag) ;
					threadLog("%s. Line %d Case 2 Unknown lookup tag <%s> (%d bytes)\n", *_fn, zi.Line(), *tagword, wholetag.Length()) ;

				pCN->m_tmpContent << wholetag ;
				zi = limit ;
				continue ;
			}

			//	Obtain tag name
			if (bAnti == false)
			{
				if (zi.Equiv("<title>"))
				{
					pCN = _proctag(m_pHead, zi, HTAG_TITLE) ;
					if (!pCN)
						{ rc = E_FORMAT ; threadLog("%s. Line %d Could not process <meta> tags\n", *_fn, zi.Line()) ; }
				}
				else if (zi.Equiv("<meta"))
				{
					pCN = _proctag(m_pHead, zi, HTAG_META) ;
					if (!pCN)
						{ rc = E_FORMAT ; threadLog("%s. Line %d Could not process <meta> tags\n", *_fn, zi.Line()) ; }
				}
				else if (zi.Equiv("<style"))
				{
					pCN = _proctag(m_pHead, zi, HTAG_STYLE) ;
					if (!pCN)
						{ rc = E_FORMAT ; threadLog("%s. Line %d Could not process <style> tags\n", *_fn, zi.Line()) ; }
				}
				else if (zi.Equiv("<script"))
				{
					pCN = _proctag(m_pHead, zi, HTAG_SCRIPT) ;
					if (!pCN)
						{ rc = E_FORMAT ; threadLog("%s. Line %d Could not process <script> tags\n", *_fn, zi.Line()) ; }
				}
				else if (zi.Equiv("<link"))
				{
					pCN = _proctag(m_pHead, zi, HTAG_LINK) ;
					if (!pCN)
						{ rc = E_FORMAT ; threadLog("%s. Line %d Could not process <link> tags\n", *_fn, zi.Line()) ; }
				}
				else if (zi.Equiv("<base"))
				{
					pCN = _proctag(m_pHead, zi, HTAG_BASE) ;
					if (!pCN)
						{ rc = E_FORMAT ; threadLog("%s. Line %d Could not process <link> tags\n", *_fn, zi.Line()) ; }
					//	Set m_Base
					if (pCN->m_tmpContent.Size())
						m_Base = pCN->m_tmpContent ;
					else
					{
						//	set the m_Base to the first param
						ai = pCN ;
						if (ai.Value())
							m_Base = ai.Value() ;
						//	pAttr = pCN->GetFirstAttr() ;
						//	if (pAttr)
						//		m_Base = pAttr->value ;
					}
				}
				else
					{ rc = E_FORMAT ; threadLog("%s. Line %d Could not process <%s> tag within <head>\n", *_fn, zi.Line(), *tagword) ; }

				continue ;
			}

			//	Handle antitag
			if (bAnti)
			{
				if (zi.Equiv("</head>"))
					{ zi += 7 ; break ; }

				//	Inactive (text rendering only) anti-tags
				if (tf.klas == HTCLASS_TXT)
					{ pCN->m_tmpContent << wholetag ; zi = limit ; continue ; }

				//	{ zi = limit ; continue ; }

				zi = limit ;

				if (pCN->Type() == tf.type || tf.rule == HTRULE_SINGLE)
					pCN = pCN->Parent() ;
				else
				{
					threadLog("%s. case 1 Tag mis-match. Current highest tag is <%s id=%d, level=%d> but on line %d we have an anti-tag for %s\n",
						*_fn, *Tagtype2Txt(pCN->Type()), pCN->GetUid(), pCN->Level(), zi.Line(), *Tagtype2Txt(tf.type)) ;

					if (tf.rule == HTRULE_SINGLE)
					{
						//pCN = pX ;
						pCN = pCN->Parent() ;
						threadLog("%s. Case 2 Corrected by allowing last tag as anti-tag\n", *_fn) ;
					}

					if (pCN->Type() == HTAG_TBL_CEL && tf.type == HTAG_TR)
					{
						for (pX = pCN ; pX ; pX = pX->Parent())
						{
							if (pX->Type() == tf.type)
							{
								pCN = pX ;
								threadLog("%s. Corrected by decending to level %d\n", *_fn, pCN->Level()) ;
								break ;
							}
						}
					}
				}
				continue ;
			}

			//	If none of the above just advance
			zi++ ;
		}

 		//	Advance to the <body> tag
		for (; !zi.eof() ;)
		{
			if (zi.Equiv("<body"))
			{
				m_pBody = _proctag(m_pRoot, zi, HTAG_BODY) ;
				if (!m_pBody)
					{ threadLog("%s. Expected an actual body\n", *_fn) ; return E_FORMAT ; }
				break ;
			}
			zi++ ;
		}

		if (!m_pBody)
			{ threadLog("%s. Expected a <body> tag\n", *_fn) ; return E_FORMAT ; }
		pCN = m_pBody ;
	}

	//
	//	Process document body. Here everything is either a tag, an anti-tag or it is tag-content. Both tags and antitags begin with a '<' so the
	//	raw HTML is iterated and whenever the < is found, it is tested for a known tag/antitag. In the general case of "<tag>content</tag>", the
	//	process is to call _procTag() to parse the tag, garner the attributes and to create a new element (which the current element is then set
	//	to). Bytes after the tag are agregated to the current element's content until the antitag occurs (at which point the current element is
	//	then set back to the parent tag).
	//
	//	The exceptions to the general case:-
	//
	//	1)	Paragraph tags can be left open (antitag omited). These tags are closed by the parent antitag or by another paragraph tag.
	//
	//	2)	Print control tags which are completely ignored. These can never become the current tag so any content they have is aggregated to
	//		their parent tag.
	//
	//	3)	Links which do become current, but will have thier content aggregated to the parent tag.
	//

	for (; pCN && !zi.eof() ;)
	{
		//	Handle tag content
		if (*zi != CHAR_LESS)
		{
			if (pCN->Type() != HTAG_ANCHOR)
			{
				if (*zi <= CHAR_SPACE && pCN->m_tmpContent.Size() == 0)
					{ zi++ ; continue ; }
				pCN->m_tmpContent.AddByte(*zi) ;
			}
			else
			{
				if (pCN->Parent())
					pCN->Parent()->m_tmpContent.AddByte(*zi) ;
			}

			zi++ ;
			continue ;
		}

		//	Ignore deleted text within <strike></strike> tags
		nLine = zi.Line() ;

		if (zi == "<strike>")
		{
			for (zi += 8 ; !zi.eof() ; zi++)
			{
				if (zi == "</strike>")
					{ zi += 9 ; break ; }
			}
			if (zi.eof())
				{ threadLog("%s. Unclosed comment block\n", *_fn) ; rc = E_FORMAT ; break ; }
			continue ;
		}

		if (zi == "<fb:like>")
		{
			for (zi += 9 ; !zi.eof() ; zi++)
			{
				if (zi == "</fb:like>")
					{ zi += 10 ; break ; }
			}
			if (zi.eof())
				{ threadLog("%s. Facebook special\n", *_fn) ; rc = E_FORMAT ; break ; }
			continue ;
		}

		if (zi == "<g:plusone>")
		{
			for (zi += 11 ; !zi.eof() ; zi++)
			{
				if (zi == "</g:plusone>")
					{ zi += 12 ; break ; }
			}
			if (zi.eof())
				{ threadLog("%s. Google special\n", *_fn) ; rc = E_FORMAT ; break ; }
			continue ;
		}

		//	Ignore deleted text within comment (<!-- and -->) tags
		if (zi == "<!--[if")
		{
			for (zi += 7 ; !zi.eof() ; zi++)
			{
				if (zi == "<![endif]>")		{ zi += 10 ; break ; }
				if (zi == "<![endif]-->")	{ zi += 12 ; break ; }
			}
			if (zi.eof())
				{ threadLog("%s. Unterminated <!--[if cond]..> tag starting line %d\n", *_fn, nLine) ; rc = E_FORMAT ; break ; }
			continue ;
		}

		if (zi == "<![if")
		{
			for (zi += 5 ; !zi.eof() ; zi++)
			{
				if (zi == "<![endif]>")		{ zi += 10 ; break ; }
				if (zi == "<![endif]-->")	{ zi += 12 ; break ; }
			}
			if (zi.eof())
				{ threadLog("%s. Unterminated <![if cond]..> tag starting line %d\n", *_fn, nLine) ; rc = E_FORMAT ; break ; }
			continue ;
		}

		if (zi == "<!--")
		{
			for (zi += 4 ; !zi.eof() ; zi++)
			{
				if (zi == "-->")
					{ zi += 3 ; break ; }
			}
			if (zi.eof())
				{ threadLog("%s. Unterminated <!--> tag starting line %d\n", *_fn, nLine) ; rc = E_FORMAT ; break ; }
			continue ;
		}

		/*
 		**	At this point we have the '<' start of tag char. Establish whole and tagword of possible HTML tag
		*/

		wholetag = 0 ;
		tagword = 0 ;
	
		limit = zi ;
		limit++ ;
		W.AddByte(CHAR_LESS) ;
		bAnti = false ;
		if (*limit == CHAR_FWSLASH)
			{ W.AddByte(CHAR_FWSLASH) ; bAnti = true ; limit++ ; }

		nColon = 0 ;
		for (tw_start = limit ; !limit.eof() ; limit++)
		{
			if (*limit == CHAR_COLON || IsAlphanum(*limit))
			{
				if (*limit == CHAR_COLON)
					nColon++ ;

				T.AddByte(*limit) ;
				W.AddByte(*limit) ;
				continue ;
			}
			break ;
		}
		tagword = T ;
		T.Clear() ;
		
		for (; !limit.eof() ;)
		{
			W.AddByte(*limit) ;

			if (*limit == CHAR_DQUOTE || *limit == CHAR_SQUOTE)
			{
				cDelim = *limit ;

				for (limit++ ; !limit.eof() ; limit++)
				{
					if (*limit == CHAR_BKSLASH)
					{
						limit++ ;
						if (*limit == cDelim)
							continue ;
					}
					if (*limit == cDelim)
						break ;
				}
			}

			if (*limit == CHAR_MORE)
				break ;
			limit++ ;
		}

		wholetag = W ;
		W.Clear() ;

		if (*limit != CHAR_MORE)
		{
			threadLog("%s. Malformed tag (%s)\n", *_fn, *wholetag) ;
			zi = limit ;
			continue ;
		}

		tagword.ToLower() ;

		if (nColon)
		{
			if (!s_htagNam.Exists(tagword))
			{
				tf.klas=HTCLASS_3RD ;
				tf.rule=HTRULE_OPTION ;
				tf.name = tagword ;
				s_htagTyp.Insert(tf.type, tf) ;
				s_htagNam.Insert(tf.name, tf) ;
				threadLog("%s. Inserted 3rd party HTML tag %s\n", *_fn, *tagword) ;
			}
		}

		//	if (bAnti)
		//		threadLog("%s. Case 2 line %d Doing antitag %s\n", *_fn, zi.Line(), *tagword) ;
		//	else
		//		threadLog("%s. Case 2 line %d Doing tag %s\n", *_fn, zi.Line(), *tagword) ;

		tf = s_htagNam[tagword] ;

		if (tf.type == HTAG_NULL)
		{
			//	Unrecognized tags are just made part of the content of the currently applicable tag

			if (bAnti)
				threadLog("%s. Line %d Unknown lookup anti-tag </%s> (%s)\n", *_fn, zi.Line(), *tagword, *wholetag) ;
			else
				//threadLog("%s. Line %d Case 3 Unknown lookup tag <%s> (%s)\n", *_fn, zi.Line(), *tagword, *wholetag) ;
				threadLog("%s. Line %d Case 3 Unknown lookup tag <%s> (%d bytes)\n", *_fn, zi.Line(), *tagword, wholetag.Length()) ;

			pCN->m_tmpContent << wholetag ;
			zi = limit ;
			continue ;
		}

		if (bAnti == false)
		{
			//	Ignore graphic tags
			if (tf.klas == HTCLASS_IMG)
				{ zi = limit ; continue ; }

			//	Ignore self-closed 'system' tags
			if (tf.klas == HTCLASS_SYS)
			{
				if (tf.type == HTAG_EMBED)
					pCN->m_tmpContent << "<embed/>" ;

				if (tf.type == HTAG_NOEMBED)
					pCN->m_tmpContent << "<noembed/>" ;

				for (; !zi.eof() ; zi++)
				{
					if (*zi == CHAR_MORE)
						{ zi++ ; break ; }
				}

				threadLog("%s. Line %d Bypassed system tag <%s> (%s)\n", *_fn, zi.Line(), *tagword, *wholetag) ;
				zi = limit ;
				continue ;
			}

			//	Handle HTCLASS_TXT 'in-content' tags. We just copy these through, complete with tag, antitag and content, to the content of the
			//	current tag. However these tags should still be placed in the m_mapTags and m_vecTags member.

			if (tf.klas == HTCLASS_TXT)	// || tf.type == HTAG_ANCHOR)
			{
				pCN->m_tmpContent << wholetag ; zi = limit ;
				continue ;
			}

			//	If we are suppressing anchors, we only want the content of a <a href=...>...</a> sequence.
			//if (m_bOpflags & HDOC_SUPPRESS_LINKS && tf.klas == HTCLASS_LNK && tf.type == HTAG_ANCHOR)
			//if (bFlags & HDOC_ONLOAD_LINKS && tf.klas == HTCLASS_LNK && tf.type == HTAG_ANCHOR)
			//	{ zi = limit ; continue ; }

			//	Eliminate scripts (may revisit)
			if (zi.Equiv("<script"))
			{
				//	plog->Out("%s. ignoring a script tag ...\n", __FUNCTION__) ;

				for (tmp = zi ; !tmp.eof() ; tmp++)
				{
					if (tmp.Equiv("</script>"))
						{ tmp += 9 ; zi = tmp ; break ; }
				}
				if (zi.eof())
					{ threadLog("%s. Unclosed script tag\n", *_fn) ; rc = E_FORMAT ; break ; }
				continue ;
			}

			/*
 			**	Process 'data structure' tags into nodes. These are tables (with there rows and columns) but also menus
			**	and ordered and unordered lists.
			*/

			pNN = 0 ;
			pNN = _proctag(pCN, zi, tf.type) ;

			if (!pNN)
			{
				threadLog("%s. No node allocated for tag <%s>\n", *_fn, *Tagtype2Txt(tf.type)) ;
				return E_FORMAT ;
			}

			pCN = pNN ;

			zi = limit ;

			/*
			**	Handle the <input> tag. As this is it's own anti-tag it has no content, only parameters. We need to include the tag
			**	in the tree as it is active, but we need to effect the anti-tag aspect as well (so the level is not raised)
			*/

			if (tf.type == HTAG_INPUT)
				pCN = pCN->Parent() ;

			continue ;
		}

 		//	Handle anti-tags
		if (bAnti)
		{
			//	Inactive (text rendering only) anti-tags
			if (tf.klas == HTCLASS_TXT)	// || tf.type == HTAG_ANCHOR)
				{ pCN->m_tmpContent << wholetag ; zi = limit ; continue ; }

			//	Ignore self-closed 'system' tags
			if (tf.klas == HTCLASS_SYS)
			{
				if (tf.type == HTAG_EMBED)
					pCN->m_tmpContent << "</embed>" ;

				if (tf.type == HTAG_NOEMBED)
					pCN->m_tmpContent << "</noembed>" ;

				for (; !zi.eof() ; zi++)
				{
					if (*zi == CHAR_MORE)
						{ zi++ ; break ; }
				}

				threadLog("%s. Line %d Bypassed system anti-tag <%s> (%s)\n", *_fn, zi.Line(), *tagword, *wholetag) ;
				zi = limit ;
				continue ;
			}

			zi = limit ;

			if (pCN->Type() == tf.type || tf.rule == HTRULE_SINGLE)
				pCN = pCN->Parent() ;
			else
			{
				threadLog("%s. case 2 Tag mis-match. Current highest tag is <%s id=%d, level=%d> but on line %d we have an anti-tag for %s\n",
					*_fn, *Tagtype2Txt(pCN->Type()), pCN->GetUid(), pCN->Level(), zi.Line(), *Tagtype2Txt(tf.type)) ;

				if (tf.rule == HTRULE_SINGLE)
				{
					//pCN = pX ;
					pCN = pCN->Parent() ;
					threadLog("%s. Case 1 Corrected by allowing last tag as anti-tag\n", *_fn) ;
				}

				if (pCN->Type() == HTAG_TBL_CEL && tf.type == HTAG_TR)
				{
					for (pX = pCN ; pX ; pX = pX->Parent())
					{
						if (pX->Type() == tf.type)
						{
							pCN = pX ;
							threadLog("%s. Corrected by decending to level %d\n", *_fn, pCN->Level()) ;
							break ;
						}
					}
				}
			}
			continue ;
		}

		threadLog("HANDLING ABD %s (%s)\n", *tagword, *wholetag) ;
	}

	if (pCN)
		threadLog("%s. End of file encountered whilst inside tag definition\n", *_fn) ;

	//	Move thru the tags in thier order of appearence and reduce where appropriate, the tag content held in chains to strings. Place forms in
	//	the list of forms and place form field tags with thier host forms.

	for (nX = 0 ; nX < m_vecTags.Count() ; nX++)
	{
		pX = m_vecTags[nX] ;

		if (pX->Type() == HTAG_FORM)
		{
			//	Add the form to to m_Forms and set this to the current form
			pCurForm = pX ;
			pForm = new hzHtmForm() ;
			m_Forms.Add(pForm) ;
			continue ;
		}

		if (pCurForm)
		{
			if (pX->Type() == HTAG_INPUT)
			{
				//	Add this field to the current form (report error if not in a current form)
				if (pX->Line() < pCurForm->Anti())
				{
					P.name = pX->Name() ;

					//	for (pAttr = pX->GetFirstAttr() ; pAttr ; pAttr = pAttr->next)
					//	{
					//		if (pAttr->name == "value")
					//			{ P.value = pAttr->value ; break ; }
					//	}
					for (ai = pX ; ai.Valid() ; ai.Advance())
					{
						anam = ai.Name() ; aval = ai.Value() ;

						if (anam == "value")
							{ P.value = aval ; break ; }
					}

					pForm->fields.Add(P) ;
				}
				continue ;
			}

			if (pX->Line() > pCurForm->Anti())
				pCurForm = 0 ;
		}
	}

	threadLog("%s. END OF LOAD page has %d links\n", *_fn, m_vecLinks.Count()) ;

	return rc ;
}

hzEcode hzDocHtml::Load	(const char* fpath)
{
	//	Loads an XML document into a tree of XML nodes
	//
	//	Arguments:	1)	fpath	Source file of HTML document
	//
	//	Returns:	E_ARGUMENT	If no file path is supplied
	//				E_NOTFOUND	If the file does not exist
	//				E_NODATA	If the file is empty
	//				E_OPENFAIL	If the file cannot be read
	//				E_FORMAT	If a format error caused the file load to fail
	//				E_OK		If the operation is successful

	_hzfunc("hzDocXml::Load") ;

	ifstream	is ;	//	Input stream
	hzChain		Z ;		//	Chain for holding file content
	hzEcode		rc ;	//	Return code

	//	Load document into a working chain
	rc = OpenInputStrm(is, fpath, *_fn) ;
	if (rc == E_OK)
	{
		Z << is ;
		is.close() ;
		rc = Load(Z) ;
	}

	return rc ;
}

hzHtmElem*  hzHtmElem::GetFirstChild	(void) const
{
	_hzfunc("hzHtmElem::GetFirstChild") ;

	if (!m_pHostDoc)
		hzexit(_fn, m_Name, E_NOINIT, "Node has no host document") ;

	if (!m_Children)
		return 0 ;

	return m_pHostDoc->m_arrNodes.InSitu(m_Children-1) ;
}

hzHtmElem*  hzHtmElem::Sibling  (void) const
{
	_hzfunc("hzHtmElem::Sibling") ;

	if (!m_pHostDoc)
		hzexit(_fn, m_Name, E_NOINIT, "Node has no host document") ;

	if (!m_Sibling)
		return 0 ;

	return m_pHostDoc->m_arrNodes.InSitu(m_Sibling-1) ;
}

hzHtmElem*  hzHtmElem::Parent   (void) const
{
	_hzfunc("hzHtmElem::Parent") ;

	if (!m_pHostDoc)
		hzexit(_fn, m_Name, E_NOINIT, "Node has no host document") ;

	if (!m_Parent)
		return 0 ;

	return m_pHostDoc->m_arrNodes.InSitu(m_Parent-1) ;
}

hzDocHtml*	hzHtmElem::GetTree	(void)
{
	//	Return the HTML document whose tree of HTML elemnents this hzHtmElem is a part. We start at the current node and follow the parentage all the way back
	//	to the base of the tree.
	//
	//	Arguments:	None
	//	Returns:	Pointer to root node of the tree to which the current node (element) belongs

	hzHtmElem*	pN ;	//	Current tree node

	if (!m_Parent)
		Fatal("hzHtmElem::GetTree. 1. Tag %s (line %d, level %d) has no parent\n", *m_Name, m_nLine, m_nLevel) ;

	for (pN = this ; pN->m_nLevel ; pN = pN->Parent()) ;
		if (!pN->m_Parent)
			Fatal("hzHtmElem::GetTree. 2. Tag %s (line %d, level %d) has no parent\n", *pN->m_Name, pN->m_nLine, pN->m_nLevel) ;

	return (hzDocHtml*) pN->Parent() ;
}

uint32_t	hzHtmElem::_testnode	(hzVect<hzHtmElem*>& tmpResult, const char* srchExp, uint32_t& nLimit, uint32_t nLevel, bool bLog)
{
	//	Recursive support function to the non-recursive FindSubnodes function.
	//
	//	Split up first part of search expression (up to first period or null terminator), to a node/tag name and if present, a content speciifer
	//	(="some_value"), an attribute name (->"attr_name") an attribute content specifer.
	//
	//	We now apply the test to the current node and when required, to the children. We do not operate where nodes are at a higher
	//	level than the limit. This is because the FindSubnodes function is looking for the set of nodes matching the search expression that are
	//	found at the lowest level
	//
	//	Arguments:	1)	tmpResult	Vector of HTML elements this function will add to
	//				2)	srchExp		HTML element selection criteria
	//				3)	nLimit		Depth limit for probing of child nodes
	//				4)	nLevel		Depth level of this HTML element
	//				5)	bLog		Print log flag
	//
	//	Returns:	Number of elements added during this call on this element

	_hzfunc("hzHtmElem::_testnode") ;

	hzChain			Z ;					//	For extracting search expression components
	hzHtmElem*		pNode ;				//	Node to be returned
	const char*		i ;					//	Search expression iterator
	const char*		cpNext = 0 ;		//	Next part of search expression if present
	hzAttrset		ai ;				//	Attribute iterator
	hzString		cont ;				//	Convert elemnet's content to temp string
	hzString		reqNode_name ;		//	Required name of node
	hzString		reqChild_name ;		//	Required name of node child
	hzString		reqNode_cont ;		//	Required content of node
	hzString		reqAttr_name ;		//	Required name of attribute
	hzString		reqAttr_value ;		//	Required value of attribute
	hzString		anam ;				//	Attribute name
	hzString		aval ;				//	Attribute value
	uint32_t		nTotal ;			//	Total nodes found matching search expression
	bool			bFound ;			//	Does this node pass this part of search expression

	//	If we are already at too high a level, return
	if (nLimit && (m_nLevel > nLimit))
	{
		if (bLog)
			threadLog("\t-> Out of range, returning 0\n") ;
		return 0 ;
	}

	//	Get required name of node
	for (i = srchExp ; IsAlpha(*i) ; i++)
		Z.AddByte(*i) ;
	reqNode_name = Z ;
	Z.Clear() ;

	if (*i == CHAR_PERIOD)
	{
		i++ ;
		if (!IsAlpha(*i))
		{
			if (bLog)
				threadLog("%s. Malformed criteria (%s)\n", *_fn, srchExp) ;
			return 0 ;
		}

		cpNext = i ;
		for (; IsAlpha(*i) ; i++)
			Z.AddByte(*i) ;
		reqChild_name = Z ;
		Z.Clear() ;
	}

	//	Get name of attribute if applicable
	if (i[0] == CHAR_MINUS && i[1] == CHAR_MORE)
	{
		for (i += 2 ; IsUrlnorm(*i) ; i++)
			Z.AddByte(*i) ;
		reqAttr_name = Z ;
		Z.Clear() ;
	}

	//	An equal sign after the tag name specifies what the tag contents must be for the tag to qualify
	if (*i == CHAR_EQUAL)
	{
		for (i += 2 ; *i != CHAR_DQUOTE ; i++)
			Z.AddByte(*i) ;
		reqAttr_value = Z ;
		Z.Clear() ;
	}

	/*
	if (bLog)
	{
		threadLog("On-node [%s] (%d) Testing node with reqNode_name=%s, reqChild_name=%s, reqAttr_name=%s, reqAttr_value=%s level=%d, slct=%s\n",
			*Lineage(), m_nLevel, *reqNode_name, *reqChild_name, *reqAttr_name, *reqAttr_value, nLevel, srchExp) ;
		for (pNode = m_Children ; pNode ; pNode = pNode->m_Sibling)
			threadLog("\t-> child: %s\n", *pNode->m_Name) ;
	}
	*/

	//	Now we have the first part of the search expression, we test to see if this node meets this. If it does we still have to establish if
	//	the remainder of the search expression (if it exists) is satisfied.

	//pAttr = 0 ;
	bFound = false ;

	if (m_Name == reqNode_name)
	{
		//	We are on the specified node so if the value is not right, any named attribute does not exist or it does but with the
		//	wrong value, we return a zero (to end the examination of this branch of nodes)

		bFound = true ;

		if (!reqChild_name)
		{
			//	No child node has been specified so this node must be the last to check

			if (reqNode_cont)
			{
				cont = m_tmpContent ;
				if (reqNode_cont != cont)
					return 0 ;
			}

			if (bFound && reqAttr_name)
			{
				//	See if we can find an attribute of the requrired name on this node
				for (ai = this ; ai.Valid() ; ai.Advance())
				{
					anam = ai.Name() ; aval = ai.Value() ;

					threadLog("Compare attr names (%s to param->name of %s)\n", *reqAttr_name, *anam) ;
					if (anam == reqAttr_name)
					{
						threadLog("Found a attr name match ") ;
						if (reqAttr_value)
						{
							if (reqAttr_value != aval)
							{
								threadLog("but not a pvalue match (%s not param->val of %s)\n", *reqAttr_value, *aval) ;
								continue ;
							}
						}
						threadLog(" - bingo\n") ;
						break ;
					}
				}

				//	if (!pAttr)
				//		{ threadLog("Oops - run out of params\n") ; return 0 ; }
			}
		}
	}

	if (bFound)
	{
		/*
		**	Now we have passed the first part of the search expression, we can add this node to the results if there is no furthur search expression. But
		**	if there is, we have to establish if the remainder of the search expression is satisfied. This will nessesitate a recursive call of
		**	this function for each and every child of this node with the search expression pointer advanced. Only if at least one of these calls
		**	succeeds (returns a positive integer for nodes added to the result), can this call succeed.
		*/

		if (!cpNext)
		{
			//threadLog("\tMatched. Adding %s at level %d and position %d to array\n", *Lineage(), m_nLevel, tmpResult.Count()) ;

			nLimit = m_nLevel ;
			tmpResult.Add(this) ;
			return 1 ;
		}

		//	Test children on the further search expression
		nTotal = 0 ;
		for (pNode = GetFirstChild() ; pNode ; pNode = pNode->Sibling())
		{
			//	if (!pNode->IsAncestor(this))
			//		Fatal("Case 2: Proported child failes to be ancestor of this\n") ;

			if (nLimit && (pNode->m_nLevel > nLimit))
				continue ;

			nTotal += pNode->_testnode(tmpResult, cpNext, nLimit, nLevel + 1, bLog) ;
		}
		return nTotal ;
	}

	/*
	**	This node does not have the required name and so does not meet the first part of the search expression. However a child might meet the
	**	search expression so we try each in turn.
	*/

	nTotal = 0 ;
	for (pNode = GetFirstChild() ; pNode ; pNode = pNode->Sibling())
	{
		if (nLimit && (pNode->m_nLevel > nLimit))
			continue ;

		if (pNode->Name() == reqNode_name)
			nTotal += pNode->_testnode(tmpResult, srchExp, nLimit, nLevel + 1, bLog) ;
	}

	return nTotal ;
}

void	hzHtmElem::FindSubnodes	(hzVect<hzHtmElem*>& result, const char* srchExp, bool bLog)
{
	//	From the current node (the node used to call this member function), find all sub-nodes matching the supplied search expression.
	//
	//	This function does not simply locate nodes that are children of the calling node whose name matches the supplied search expression. The aim is
	//	to locate descenant nodes, however far down the tree they are.
	//
	//	Note:	The search expression will be of the form of one or more name-value pairs as follows:-
	//
	//		1)	name="some_name";		- Only applies if the element is given an id which is often not the case
	//		2)	type="html_tagtype";	- The element is of the right type, eg <table>
	//		3)	class="class_value";	- The element has the given class value
	//		4)	pname="param_name";		- The element has the parameter
	//		4)	pvalue="param_value";	- The element has the parameter value
	//		6)	cont="content_value";	- The element has contents of the given value
	//
	//	Arguments:	1)	elements	The vector of elements found and in thier actual order of incidence.
	//				2)	srchExp		Search expression
	//				3)	bLog		Set if detailed logging is required
	//
	//	Returns:	None

	hzDocHtml*	pTree ;			//	The Tree holding this node
	uint32_t	nLimit = 0 ;	//	Level limit

	//	Check we have a tree
	pTree = GetTree() ;
	if (!pTree)
		Fatal("No tree - aborting\n") ;

	//	Recursively call _testnode
	result.Clear() ;
	_testnode(result, srchExp, nLimit, 0, bLog) ;
	threadLog("hzHtmElem::FindSubnodes: found %d results, set limit to %d\n", result.Count(), nLimit) ;
}

uint32_t	hzDocHtml::ExtractLinksBasic	(hzVect<hzUrl>& links, const hzSet<hzString>& domains, const hzString& form)
{
	//	Find all links on a page lying within a set of acceptable domains and matching any supplied criteria. These are aggregated to the supplied vector of link
	//	URLs. If no domains or criteria are supplied, all the links in the page will be aggregated.
	//
	//	Note the links in a page are established in the Load() function. This function meerly filters them. It does not read the page content.
	//
	//	Arguments:	1)	links:		The vector or set of URLs (links) found in the document
	//				2)	domains:	The set of domains that links must belong to in order to be included
	//				3)	form:		The search criteria is any
	//
	//	Returns:	Number of links that meet the supplied criteria

	hzUrl		link ;			//	URL of link
	uint32_t	nIndex ;		//	Links iterator

	links.Clear() ;

	for (nIndex = 0 ; nIndex < m_vecLinks.Count() ; nIndex++)
	{
		link = m_vecLinks[nIndex] ;

		//	Ignore empty links (should not be any)
		if (!link)
			continue ;

		//	Ignore links to domains not on the list of acceptable domains (usually the website domain only)
		if (domains.Count())
		{
			if (!domains.Exists(link.Domain()))
				continue ;
		}

		//	Now apply criteria
		if (form)
		{
			if (!FormCheckCstr(*link, *form))
				continue ;
		}

		links.Add(link) ;
	}

	return links.Count() ;
}

uint32_t	hzDocHtml::ExtractLinksContent	(hzMapS<hzUrl,hzString>& links, const hzSet<hzString>& domains, const hzString& criteria)
{
	//	Find all links on a page lying within a set of acceptable domains and matching any supplied criteria. These are aggregated to the supplied map of link
	//	URLs to link content. If no domains or criteria are supplied, all the links in the page will be aggregated.
	//
	//	Note the links in a page are established in the Load() function. This function meerly filters them. It does not read the page content.
	//
	//	Arguments:	1) links:	The vector or set of URLs (links) found in the document
	//				2) domains:	The set of domains that links must belong to in order to be included
	//				3) form:	The search criteria is any
	//
	//	Returns:	Number of links that meet the supplied criteria

	hzHtmElem*		pElement ;	//	HTML node
	hzAttrset		ai ;		//	Attribute iterator
	hzString		anam ;		//	Attribute name
	hzString		S ;			//	Content of link node
	hzUrl			link ;		//	URL of link
	uint32_t		nIndex ;	//	Links iterator

	links.Clear() ;

	for (nIndex = 0 ; nIndex < m_vecTags.Count() ; nIndex++)
	{
		pElement = m_vecTags[nIndex] ;

		if (pElement->Type() != HTAG_ANCHOR)
			continue ;

		//for (pm = pElement->GetFirstAttr() ; pm ; pm = pm->next)
		for (ai = pElement ; ai.Valid() ; ai.Advance())
		{
			anam = ai.Name() ;

			if (anam.Equiv("href"))
			{
				link = ai.Value() ;

				//	Ignore empty links (should not be any)
				if (!link)
					continue ;

				//	Ignore links to domains not on the list of acceptable domains (usually the website domain only)
				if (domains.Count())
				{
					if (!domains.Exists(link.Domain()))
						continue ;
				}

				//	Enforce limiting criteria
				if (criteria)
				{
					if (!FormCheckCstr(*link, *criteria))
						continue ;
				}

				S = pElement->m_tmpContent ;
				links.Insert(link, S) ;
			}
		}
	}

	return links.Count() ;
}

hzEcode	hzDocHtml::Import	(const hzString& path)
{
	//	Loads an HTML document into a tree of HTML nodes
	//
	//	Arguments:	1)	path	The full pathname of the file to load
	//
	//	Returns:	E_ARGUMENT	If no file path is supplied
	//				E_NOTFOUND	If the file does not exist
	//				E_NODATA	If the file is empty
	//				E_OPENFAIL	If the file cannot be read
	//				E_FORMAT	If a format error caused the file load to fail
	//				E_OK		If the operation is successful

	_hzfunc("hzDocHtml::Import") ;

	ifstream	is ;	//	Input stream
	hzChain		Z ;		//	Chain for holding file content
	hzEcode		rc ;	//	Return code

	//	Check path and load document
	rc = OpenInputStrm(is, path, *_fn) ;
	if (rc == E_OK)
	{
		Z << is ;
		is.close() ;

		rc = Load(Z) ;
	}

	return rc ;
}

void	hzDocHtml::_report	(hzLogger& xlog, hzHtmElem* node)
{
	//	Category:	Diagnostics
	//
	//	Recursive suport function for non-recursive hzDocHtml::Report
	//
	//	Arguments:	1)	xlog	The logfile to write report to
	//				2)	node	The starting node
	//
	//	Returns:	None

	hzHtmElem*		pSub ;		//	Subnodes
	hzChain			ult ;		//	Final version of node contents
	chIter			x ;			//	Content iterator
	hzAttrset		ai ;		//	Attribute iterator
	int				n ;			//	Level iterator

	if (!node)
		{ xlog.Out("hzDocHtml::_report: ERROR No HTML element suppled\n") ; return ; }

	/*
 	**	Write out the opening of the tag
	*/

	xlog.Out("%2d: ", node->Level()) ;
	for (n = node->Level() ; n ; n--)
		xlog << ". " ;

	xlog.Out("<%s", *Tagtype2Txt(node->Type())) ;

	for (ai = node ; ai.Valid() ; ai.Advance())
		xlog.Out(" %s=\"%s\"", ai.Name(), ai.Value()) ;

	xlog << ">\n" ;

	/*
 	**	First visit higher level tags if any
	*/

	//pSub = node->FirstSubnode() ;
	pSub = node->GetFirstChild() ;
	if (pSub)
	{
		//for (; pSub ; pSub = pSub->NextSubnode())
		for (; pSub ; pSub = pSub->Sibling())
			_report(xlog, pSub) ;
	}

	/*
 	**	Then do content
	*/

	if (node->m_tmpContent.Size())
	{
		for (x = node->m_tmpContent ; !x.eof() ; x++)
		{
			if (*x <= CHAR_SPACE)
				continue ;
			break ;
		}
		for (; !x.eof() ; x++)
		{
			if (x == "\r\n")
				{ x++ ; continue ; }
			ult.AddByte(*x) ;
		}

		if (ult.Size())
		{
			xlog.Out("%2d: ", node->Level()) ;
			for (n = node->Level() ; n ; n--)
				xlog << "  " ;

			xlog << "[" << ult << "]\n" ;
		}
	}

	/*
 	**	Write out the closing of the tag
	*/

	xlog.Out("%2d: ", node->Level()) ;
	for (n = node->Level() ; n ; n--)
		xlog << ". " ;
	xlog.Out("</%s>\n", *Tagtype2Txt(node->Type())) ;
}

void	hzDocHtml::Report	(hzLogger& xlog)
{
	//	Show list of nodes plus content
	//
	//	Arguments:	1)	xlog	The logfile to write report to
	//	Returns:	None

	_hzfunc("hzDocHtml::Report") ;

	hzHtmElem*	pE ;			//	Current node
	hzString	S ;				//	Tag content holder
	uint32_t	nIndex ;		//	Document tag iterator

	if (!m_vecTags.Count())
		xlog.Out("PAGE is EMPTY - No nodes in Vector\n") ;
	else
	{
		for (nIndex = 0 ; nIndex < m_vecTags.Count() ; nIndex++)
		{
			pE = m_vecTags[nIndex] ;

			S = pE->m_tmpContent ;

			xlog.Out("id=%d par=%d subs=%d nxt=%d lev=%d: %s [%s]\n",
				pE->GetUid(),
				pE->Parent() ? pE->Parent()->GetUid() : 0,
				pE->GetFirstChild() ? pE->GetFirstChild()->GetUid() : 0,
				pE->Sibling() ? pE->Sibling()->GetUid() : 0,
				pE->Level(),
				*Tagtype2Txt(pE->Type()),
				*S) ;
		}
	}

	//	Show tree of nodes plus content
	if (!m_pRoot)
		xlog.Out("PAGE is EMPTY - No subnodes of root\n") ;
	else
		_report(xlog, m_pRoot) ;
}

hzEcode	hzDocHtml::_xport	(hzChain& Z, hzHtmElem* node)
{
	//	Recursive support function for hzDocHtml::Export. It exports the full tag (including attributes and content) of the supplied node and all
	//	subnodes, to the supplied chain.
	//
	//	Arguments:	1)	Z		The output chain
	//				2)	node	The current node
	//
	//	Returns:	E_ARGUMENT	If no HTML element is supplied
	//				E_OK		If the operation was successful
	//
	//	Note this is a support function for hzDocHtml::Export

	hzChain			ult ;		//	Final version of node contents
	chIter			x ;			//	Content iterator
	hzHtmElem*		pSub ;		//	Subnodes
	hzAttrset		ai ;		//	Attribute iterator
	int				n ;			//	Level iterator

	if (!node)
		return E_ARGUMENT ;

 	//	Write out the opening of the tag
	Z.Printf("%2d: ", node->Level()) ;
	for (n = node->Level() ; n ; n--)
		Z << ". " ;
	Z.Printf("<%s", *Tagtype2Txt(node->Type())) ;

	for (ai = node ; ai.Valid() ; ai.Advance())
		Z.Printf(" %s=\"%s\"", ai.Name(), ai.Value()) ;
	Z << ">\n" ;

 	//	Then do content
	if (node->m_tmpContent.Size())
	{
		for (x = node->m_tmpContent ; !x.eof() ; x++)
		{
			if (*x <= CHAR_SPACE)
				continue ;
			break ;
		}
		for (; !x.eof() ; x++)
		{
			if (x == "\r\n")
				{ x++ ; continue ; }
			ult.AddByte(*x) ;
		}

		if (ult.Size())
		{
			Z.Printf("%2d: ", node->Level()) ;
			for (n = node->Level() ; n ; n--)
				Z << "  " ;

			Z.AddByte('[') ;
			Z << ult ;
			Z.AddByte(']') ;
			Z.AddByte(CHAR_NL) ;
		}
	}

 	//	First visit higher level tags if any
	//pSub = node->FirstSubnode() ;
	pSub = node->GetFirstChild() ;
	if (pSub)
	{
		//for (; pSub ; pSub = pSub->NextSubnode())
		for (; pSub ; pSub = pSub->Sibling())
			_xport(Z, pSub) ;
	}

 	//	Write out the closing of the tag
	Z.Printf("%2d: ", node->Level()) ;
	for (n = node->Level() ; n ; n--)
		Z << ". " ;
	Z.Printf("</%s>\n", *Tagtype2Txt(node->Type())) ;
	return E_OK ;
}

hzEcode	hzDocHtml::Export	(const hzString& filepath)
{
	//	Exports a HTML page to a file named as per the supplied file path.
	//
	//	Arguments:	1)	filepath	The file to export the HTML document to
	//
	//	Returns:	E_ARGUMENT	If no export file path is supplied
	//				E_NODATA	If there is no HTML elements in the document
	//				E_OPENFAIL	If the supplied 
	//				E_WRITEFAIL	If a write file occurs during export
	//				E_OK		If the export ran to completion

	_hzfunc("hzDocHtml::Export") ;

	ofstream	os ;		//	Output stream
	hzChain		Z ;			//	Working chain for output construction
	hzEcode		rc = E_OK ;	//	Return code

	if (!filepath)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No pathname supplied") ;

	if (!m_pRoot)
	{
		if (!m_Content.Size())
			{ threadLog("%s. Empty page (no root node). Nothing written to file %s\n", *_fn, *filepath) ; return E_NODATA ; }
	}

 	//	Dump out to file
	os.clear() ;
	os.open(*filepath) ;
	if (os.fail())
	{
		threadLog("%s. Could not open file %s\n", *_fn, *filepath) ;
		return E_OPENFAIL ;
	}

	if (m_Info.m_urlReq)
		Z.Printf("URL (req): %s\n", *m_Info.m_urlReq) ;
	if (*m_Info.m_urlAct)
		Z.Printf("URL (act): %s\n", *m_Info.m_urlAct) ;
	os << Z ;
	if (os.fail())
		rc = E_WRITEFAIL ;
	Z.Clear() ;

	if (rc == E_OK)
	{
		if (m_pRoot)
			rc = _xport(Z, m_pRoot) ;
		else
			Z = m_Content ;

		os << Z ;
		if (os.fail())
			rc = E_WRITEFAIL ;
	}

	os.close() ;
	return rc ;
}

void	hzDocHtml::Clear	(void)
{
	//	Recursively clear the tree of nodes
	//
	//	Arguments:	None
	//	Returns:	None

	hzHtmElem*	pNode ;		//	Node pointer
	uint32_t	nIndex ;	//	Document tags iterator

	for (nIndex = 0 ; nIndex < m_vecTags.Count() ; nIndex++)
	{
		pNode = m_vecTags[nIndex] ;
		delete pNode ;
	}

	threadLog("Clearing %d vec_tags\n", m_vecTags.Count()) ;
	threadLog("Clearing %d set_links\n", m_setLinks.Count()) ;
	threadLog("Clearing %d m_emails\n", m_Emails.Count()) ;

	m_vecTags.Clear() ;
	m_vecLinks.Clear() ;
	m_setLinks.Clear() ;
	m_Emails.Clear() ;

	m_pRoot = 0 ;
	m_pHead = 0 ;
	m_pBody = 0 ;
}

hzEcode	hzDocHtml::FindElements	(hzVect<hzHtmElem*>& elements, hzString& htag, hzString& attrName, hzString& attrValue)
{
	//	Find all elements in a page with the given tag name and/or attribute and value.
	//
	//	Arguments:	1)	elements	Elements found in order of incidence in this document matching on tag type and on attribute name and value if supplied.
	//				2)	htag		The tag type. This is compulsory and matches only elements of the given type.
	//				3)	aname		The attribute name. This is optional but if supplied, will require elements to have an attribute of the supplied name
	//				4)	avalue		The attribute value. Also optional but if supplied, will require elements to have an attribute of the supplied name
	//
	//	Returns:	E_NOTFOUND	If no elements matched
	//				E_OK		If elements matched

	hzHtmElem*		pElement ;	//	HTML node
	hzAttrset		ai ;		//	Attribute iterator
	hzString		anam ;		//	Attribute name
	hzString		aval ;		//	Attribute value
	hzString		S ;			//	Content of link node
	hzUrl			link ;		//	URL of link
	uint32_t		Lo ;		//	First element in m_mapTags to investigate
	uint32_t		Hi ;		//	Last element in m_mapTags to investigate
	uint32_t		nIndex ;	//	Links iterator
	bool			bOk ;		//	OK to insert the element

	elements.Clear() ;

	Lo = 0 ;
	Hi = m_mapTags.Count() - 1 ;

	if (htag)
	{
		//	A tagname has been supplied so limit the investigation to tags with the tagname
		Lo = m_mapTags.First(htag) ;
		if (Lo < 0)
			return E_NOTFOUND ;
		Hi = m_mapTags.Last(htag) ;
	}

	//	Investigate elements
	for (nIndex = Lo ; nIndex <= Hi ; nIndex++)
	{
		pElement = m_mapTags.GetObj(nIndex) ;

		bOk = false ;

		if (attrName)
		{
			//	An attrubute name has been supplied so the element must have this attribute
			for (ai = pElement ; ai.Valid() ; ai.Advance())
			{
				anam = ai.Name() ; aval = ai.Value() ;

				if (anam == attrName)
				{
					if (!attrValue)
						bOk = true ;
					else
					{
						if (aval == attrValue)
							bOk = true ;
					}
				}
			}
		}
		else
		{
			if (attrValue)
			{
				//	An attribute value ...
				for (ai = pElement ; ai.Valid() ; ai.Advance())
				{
					anam = ai.Name() ; aval = ai.Value() ;

					if (aval == attrValue)
						bOk = true ;
				}
			}
		}

		if (bOk)
			//elements.Insert(pElement) ;
			elements.Add(pElement) ;
	}

	return E_OK ;
}

hzEcode	hzDocHtml::FindElements	(hzVect<hzHtmElem*>& elements, const char* srchExp)
{
	//	Find all tags meeting the supplied criteria and place pointers to the tags in the supplied results vector.
	//
	//	Note:	The criteria will be of the form of one or more name-value pairs as follows:-
	//
	//		1)	name="some_name";		- Only applies if the element is given an id which is often not the case
	//		2)	type="html_tagtype";	- The element is of the right type, eg <table>
	//		3)	class="class_value";	- The element has the given class value
	//		4)	pname="param_name";		- The element has the parameter
	//		4)	pvalue="param_value";	- The element has the parameter value
	//		6)	cont="content_value";	- The element has contents of the given value
	//
	//	Arguments:	1)	elements	The vector of elements found and in thier actual order of incidence.
	//				2)	srchExp		Search expression
	//
	//	Returns:	E_NOTFOUND	If no elements matched
	//				E_OK		If elements matched

	_hzfunc("hzDocHtml::FindElements") ;

	hzVect<hzString>	list ;		//	List of tagnames forming required nod ancestry
	hzVect<hzHtmElem*>	found ;		//	Nodes matching this

	hzChain			Z ;				//	For extracting tagnames etc
	hzHtmElem*		pN ;			//	Element
	hzHtmElem*		pK ;			//	Element child
	hzAttrset		ai ;			//	Attribute iterator
	const char*		i ;				//	For processing criteria
	hzString		tnam ;			//	Tagname
	hzString		knam ;			//	Child tagname (if any)
	hzString		reqAttr_name ;	//	Attribute name (if any)
	hzString		reqAttr_value ;	//	Attribute value (if any)
	uint32_t		Lo ;			//	1st element to investigate
	uint32_t		Hi ;			//	Lst element to investigate
	uint32_t		x ;				//	Element iterator
	uint32_t		v ;				//	Element iterator
	uint32_t		anc ;			//	Ancestry level

	elements.Clear() ;

	//	Find node by name required name of node
	for (i = srchExp ; IsAlphanum(*i) ; i++)
		Z.AddByte(*i) ;
	tnam = Z ;
	Z.Clear() ;
	list.Add(tnam) ;

	for (; *i == CHAR_PERIOD ;)
	{
		i++ ;
		if (!IsAlpha(*i))
		{
			threadLog("%s. Malformed criteria (%s)\n", *_fn, srchExp) ;
			return E_FORMAT ;
		}

		for (; IsAlphanum(*i) ; i++)
			Z.AddByte(*i) ;
		tnam = Z ;
		Z.Clear() ;
		list.Add(tnam) ;
	}

	//	Get name of attribute if applicable
	if (i[0] == CHAR_MINUS && i[1] == CHAR_MORE)
	{
		for (i += 2 ; IsUrlnorm(*i) ; i++)
			Z.AddByte(*i) ;
		reqAttr_name = Z ;
		Z.Clear() ;
	}

	//	An equal sign after the tag name specifies what the tag contents must be for the tag to qualify
	if (*i == CHAR_EQUAL)
	{
		for (i += 2 ; *i != CHAR_SQUOTE ; i++)
			Z.AddByte(*i) ;
		reqAttr_value = Z ;
		Z.Clear() ;
	}

	anc = list.Count() ;
	if (anc)
	{
		//	Look up the last tag in the m_mapTags
		anc-- ;
		tnam = list[anc] ;

		Lo = m_mapTags.First(tnam) ;
		if (Lo < 0)
			return E_OK ;
		Hi = m_mapTags.Last(tnam) ;

		threadLog("node (%d - %d) %s a=%s v=%s", Lo, Hi, *tnam, *reqAttr_name, *reqAttr_value) ;

		for (x = Lo ; x <= Hi ; x++)
		{
			pN = m_mapTags.GetObj(x) ;

			if (!anc)
				found.Add(pN) ;
			else
			{
				//	Progress thru ancestry
				pK = pN->Parent() ;
				for (v = anc-1 ; pK && v >= 0 ; pK = pK->Parent(), v--)
				{
					threadLog("<- %s ", *pK->Name()) ;
					if (pK->Name() != list[v])
						break ;
				}
				if (v < 0)
				{
					found.Add(pN) ;
					threadLog("OK ") ;
				}
			}
		}

		//	Check all found nodes for attribute criiteria
		for (x = 0 ; x < found.Count() ; x++)
		{
			pN = found[x] ;

			if (!reqAttr_name && !reqAttr_value)
				elements.Add(pN) ;
			else
			{
				for (ai = pN ; ai.Valid() ; ai.Advance())
				{
					if (reqAttr_name && reqAttr_name != ai.Name())
					{
						threadLog("-1 ") ;
						continue ;
					}
					if (reqAttr_value && reqAttr_value != ai.Value())
					{
						threadLog("-2 ") ;
						continue ;
					}
					elements.Add(pN) ;
					threadLog("+ ") ;
					break ;
				}
			}
		}

		threadLog("done\n") ;
	}
	else
	{
		//	Check all the nodes for attribute criteria
		for (x = 0 ; x < m_mapTags.Count() ; x++)
		{
			pN = m_mapTags.GetObj(x) ;

			if (!reqAttr_name && !reqAttr_value)
				elements.Add(pN) ;
			else
			{
				for (ai = pN ; ai.Valid() ; ai.Advance())
				{
					if (reqAttr_name && reqAttr_name != ai.Name())
						continue ;
					if (reqAttr_value && reqAttr_value != ai.Value())
						continue ;
					elements.Add(pN) ;
					break ;
				}
			}
		}
	}

	return E_OK ;
}

hzEcode	hzDocHtml::_selectTag	(hzSet<hzHtmElem*>& parents, hzSet<hzHtmElem*>& elements, const hzString& tagspec)
{
	//	Finds the set of tags meeting the supplied tag specifier.
	//
	//	Arguments:	1)	parents		Set of parent tags
	//				2)	elements	Set of selected tags
	//				3)	tagspec		Tag selection criteria
	//
	//	Returns:	E_SYNTAX	If the tag is malformed or illegal
	//				E_OK		If the tag is correct, even if no instances are found

	_hzfunc("hzDocHtml::_selectTag") ;

	hzMapS<hzString,hzString>	pairs ;		//	List of attrs and attr values the tag must possess (if any)

	hzChain			word ;			//	Word extraction
	hzAttrset		ai ;			//	Attribute iterator
	hzHtmElem*		pE ;			//	HTML element (tag)
	hzHtmElem*		pAnc ;			//	HTML element (tag)
	const char*		i ;				//	For processing term
	hzString		tagname ;		//	Name of tag sought
	hzString		pnam ;			//	Name of attr sought
	hzString		pval ;			//	Value of attr sought
	hzString		anam ;			//	Attribute name
	hzString		aval ;			//	Attribute value
	uint32_t		nP ;			//	Name-value pair iterator
	uint32_t		Lo ;			//	First incidence of tagname
	uint32_t		Hi ;			//	Last incidence of tagname
	uint32_t		nIndex ;		//	Tag iterator
	uint32_t		nFound ;		//	All attributes found
	bool			bFound ;		//	Ancestry test
	hzEcode			rc = E_OK ;		//	Return code

	elements.Clear() ;

	/*
	**	Get tag name from the search criteria
	*/

	i = *tagspec ;

	if (i[0] != CHAR_LESS)
		{ threadLog("%s Term does not begin with an opening '<' char\n", *_fn) ; return E_SYNTAX ; }

	for (i++ ; IsAlphanum(*i) ; i++)
		word.AddByte(*i) ;
	tagname = word ;
	word.Clear() ;

	if (!tagname)
		{ threadLog("%s No tagname supplied\n") ; return E_SYNTAX ; }

	/*
	**	Get attribute requirements from the search criteria
	*/

	for (; *i == CHAR_SPACE ;)
	{
		for (i++ ; *i && *i <= CHAR_SPACE ; i++) ;
		pnam = pval = 0 ;

		for (; IsAlphanum(*i) ; i++)
			word.AddByte(*i) ;
		pnam = word ;
		word.Clear() ;

		if (!pnam)
			{ rc = E_SYNTAX ; threadLog("%s Attr name not supplied\n", *_fn) ; break ; }
		if (*i != CHAR_EQUAL)
			{ rc = E_SYNTAX ; threadLog("%s Attr name not followed by an assignment operator\n", *_fn) ; break ; }

		i++ ;
		if (*i == CHAR_ASTERISK)
		{
			i++ ;
			pval = "*" ;
			pairs.Insert(pnam, pval) ;
			continue ;
		}

		if (*i != CHAR_SQUOTE)
			{ rc = E_SYNTAX ; threadLog("%s Attr has no opening single quote\n", *_fn) ; break ; }
		for (i++ ; *i && *i != CHAR_SQUOTE ; i++)
			word.AddByte(*i) ;
		if (*i != CHAR_SQUOTE)
			{ rc = E_SYNTAX ; threadLog("%s Attr has no closing single quote\n", *_fn) ; break ; }
		i++ ;
		pval = word ;
		word.Clear() ;

		pairs.Insert(pnam, pval) ;
	}

	if (rc != E_OK)
		return rc ;

	if (*i != CHAR_MORE)
		{ threadLog("%s Term does not end with a closing '<' char\n", *_fn) ; return E_SYNTAX ; }

	threadLog("%s. Examining %d tags for tagnam=%s", *_fn, m_vecTags.Count(), *tagname) ;
	for (nP = 0 ; nP < pairs.Count() ; nP++)
	{
		pnam = pairs.GetKey(nP) ;
		pval = pairs.GetObj(nP) ;
		threadLog(" - with %s=%s", *pnam, *pval) ;
	}
	threadLog("\n") ;

	/*
	**	Get all tags in document with the tagname. It is not a failure if none found.
	*/

	Lo = m_mapTags.First(tagname) ;
	if (Lo < 0)
		threadLog("%s. No matching tags for <%s>\n", *_fn, *tagname) ;
	else
	{
		Hi = m_mapTags.Last(tagname) ;

		for (nIndex = Lo ; nIndex <= Hi ; nIndex++)
		{
			pE = m_mapTags.GetObj(nIndex) ;

			//	Exclude elements with the wrong parent
			if (parents.Count())
			{
				bFound = false ;
				for (pAnc = pE->Parent() ; pAnc ; pAnc = pAnc->Parent())
				{
					if (parents.Exists(pAnc))
					{
						bFound = true ;
						threadLog("%s. Found parent of %p\n", *_fn, pAnc) ;
						break ;
					}

					threadLog("%s. No such parent as %p\n", *_fn, pAnc) ;
				}

				if (!bFound)
					continue ;
			}

			//if (parents.Count() && !parents.Exists(pE->Parent()))
			//	continue ;

			//	No attribute/value pairs specified so the tag is added to the list
			if (!pairs.Count())
				{ elements.Insert(pE) ; continue ; }

			nFound = 0 ;
			for (ai = pE ; ai.Valid() ; ai.Advance())
			{
				anam = ai.Name() ; aval = ai.Value() ;

				if (!pairs.Exists(anam))
					continue ;

				pval = pairs[anam] ;

				if (pval == "*")
					{ nFound++ ; continue ; }
				if (pval == aval)
					nFound++ ;
			}

			//	If there is a match on every attribute/value pair specified, add to the list
			if (nFound == pairs.Count())
				elements.Insert(pE) ;
		}
	}

	threadLog("%s. Found %d tags for tagspec=[%s]\n", *_fn, elements.Count(), *tagspec) ;

	return rc ;
}

hzEcode	hzDocHtml::_selectTerm	(hzSet<hzHtmElem*>& elements, const hzString& term)
{
	//	A 'term' within the context of HTML document tag selection, can be a specification of a single tag or it can specifiy multiple tags. In the latter case,
	//	where multiple tag specifiers are concatenated, hierarchy is implied.
	//
	//	Selection works on the basis of more detail, more tests. For example, the term <div> will populate the set of elements found with every <div> tag in the
	//	document. The term <div class> will only find div tags with an attribute of 'class' while the term <div class="body"> will only find div tags that have
	//	an attribute of class whose value is 'body'. It should be noted however, that tags are selected if they have what is asked for in the term. There is not
	//	presently, any means to exclude tags if they have something we don't want them to have.
	//
	//	A hierarchical concatenated term such as <div class='body'><p> will find every paragraph tag in the document whose parent tag is a div with an attribute
	//	of class whose value is 'body'. If no div tags meet that criteria nothing will be selected. Likewise if div tags do meet the <div class="body"> test but
	//	are not followed directly by the <p> tag, nothing is selected.
	//
	//	Note that multiple tag terms are implemented by multiples calls to _selectTag, with the selection of tags found being reduced by each call. 
	//
	//	Arguments:	1)	elements	Set of lements selected by this function
	//				2)	term		Tag selection criteria
	//
	//	Returns:	E_SYNTAX	If the tag is malformed or illegal
	//				E_OK		If the tag is correct, even if no instances are found

	_hzfunc("hzDocHtml::_selectTerm") ;

	hzSet<hzHtmElem*>	parents ;	//	Parents
	hzArray<hzString>	ar ;		//	Array of terms

	uint32_t	x ;		//	For populating reducedSet
	uint32_t	t ;		//	Term count
	hzEcode		rc ;	//	Return code

	SplitCSV(ar, *term, CHAR_PLUS) ;

	if (!ar.Count())
		{ threadLog("%s. No tag specifiers found in term\n", *_fn) ; return E_SYNTAX ; }
	threadLog("%s. Term is %s (%d) components\n", *_fn, *term, ar.Count()) ;

	for (t = 0 ; t < ar.Count() ; t++)
	{
		threadLog("%s. Term component %d: %s\n", *_fn, t, *ar[t]) ;
	}

	if (ar.Count() == 1)
	{
		//	Call the _selectTag function once with the document's m_vecTags vector as the reduced set
		rc = _selectTag(parents, elements, ar[0]) ;
		return rc ;
	}

	//	There is more than one tag. Call the _selectTag function with no parents listed to start with and then repeatedly with the elements
	//	found acting as the list of valid parents for the next call.
	rc = _selectTag(parents, elements, ar[0]) ;
	if (rc == E_OK)
	{
		if (elements.Count())
		{
			for (t = 1 ; rc == E_OK && t < ar.Count() ; t++)
			{
				//	Parents is the last tag's haul
				parents.Clear() ;
				for (x = 0 ; x < elements.Count() ; x++)
					parents.Insert(elements.GetObj(x)) ;
				rc = _selectTag(parents, elements, ar[t]) ;
			}
		}
	}

	threadLog("%s. Found %d tags for term=[%s]\n", *_fn, elements.Count(), *term) ;
	return rc ;
}

hzEcode	hzDocHtml::_selectExp	(hzSet<hzHtmElem*>& elements, const hzString& srchExp)
{
	//	Recursive support function for hzDocHtml::SelectElements (see below)
	//
	//	Breaks up the expression into a term or 'term op expression' and calls _selectTerm to find the set of tags for each term. The terms can
	//	be enclosed in parenthesis but individually, they take the form of tags enclosed in a <> block. The tag name is the first and often only
	//	part but optionally after that, attributes may be specified.
	//
	//	Arguments:	1)	elements	The set of elements elected (in order of tag type)
	//				2)	srchExp		Search expression
	//
	//	Returns:	E_SYNTAX	If the expression is malformed
	//				E_OK		If the operation was successful (it still may have found no elements)

	_hzfunc("hzDocHtml::_selectExp") ;

	hzSet<hzHtmElem*>	setA ;		//	Element set for first term
	hzSet<hzHtmElem*>	setB ;		//	Element set for second term

	hzChain			word ;			//	Individual word
	hzHtmElem*		pE ;			//	HTML element
	const char*		i ;				//	For processing criteria
	hzString		termA ;			//	First term
	hzString		termB ;			//	Remainder of epression
	hzString		expA ;			//	First term
	hzString		expB ;			//	Remainder of epression
	uint32_t		op ;			//	1 for OR and 2 for AND
	uint32_t		n ;				//	Counter
	uint32_t		level ;			//	Parenthesis
	hzEcode			rc = E_OK ;		//	Return code

	/*
	**	Get 1st term
	*/

	for (i = *srchExp ; *i && *i <= CHAR_SPACE ; i++) ;

	if (*i == '(')
	{
		level = 1 ;
		for (i++ ; level && *i >= CHAR_SPACE ; i++)
		{
			if (*i == '(')	level++ ;
			if (*i == ')')	level-- ;

			if (level)
				word.AddByte(*i) ;
		}

		expA = word ;
	}
	else if (*i == CHAR_LESS)
	{
		for (; *i == CHAR_LESS ;)
		{
			for (; *i != CHAR_MORE ; i++)
				word.AddByte(*i) ;
			word.AddByte(CHAR_MORE) ;
			i++ ;
			if (*i == CHAR_PLUS)
				{ word.AddByte(CHAR_PLUS) ; i++ ; }
		}

		termA = word ;
	}
	else
	{
		threadLog("%s. Expected an opening '<'\n", *_fn) ;
		rc = E_SYNTAX ;
	}

	if (rc != E_OK)
		return rc ;

	if (*i == 0)
	{
		//	No further terms so populate element list with setA
		threadLog("%s. Calling _selectTerm with a single exp [%s] term [%s]\n", *_fn, *srchExp, *termA) ;
		if (expA)
			rc = _selectExp(elements, termA) ;
		if (termA)
			rc = _selectTerm(elements, termA) ;
		//for (n = 0 ; n < setA.Count() ; n++)
		//	elements.Insert(setA.GetObj(n)) ;
		threadLog("%s. case 1 Found %d tags for term=[%s]\n", *_fn, elements.Count(), *srchExp) ;
		return rc ;
	}

	/*
	**	Get operator
	*/

	for (; *i && *i <= CHAR_SPACE ; i++) ;

	if (!CstrCompareI(i, "or"))
		{ i += 2 ; op = 1 ; }
	else if (!CstrCompareI(i, "and"))
		{ i += 3 ; op = 2 ; }
	else
		{ threadLog("%s. Illegal operator [%s]\n", *_fn, i) ; return E_SYNTAX ; }

	/*
	**	Get remainder of expression as second term
	*/

	for (; *i && *i <= CHAR_SPACE ; i++) ;
	word.Clear() ;

	if (*i == '(')
	{
		level = 1 ;
		for (i++ ; level && *i >= CHAR_SPACE ; i++)
		{
			if (*i == '(')	level++ ;
			if (*i == ')')	level-- ;

			if (level)
				word.AddByte(*i) ;
		}

		expB = word ;
	}
	else if (*i == CHAR_LESS)
	{
		for (; *i == CHAR_LESS ;)
		{
			for (; *i != CHAR_MORE ; i++)
				word.AddByte(*i) ;
			word.AddByte(CHAR_MORE) ;
			i++ ;
			if (*i == CHAR_PLUS)
				{ word.AddByte(CHAR_PLUS) ; i++ ; }
		}

		termB = word ;
	}
	else
	{
		threadLog("%s. Expected an opening '<'\n", *_fn) ;
		rc = E_SYNTAX ;
	}

	if (rc != E_OK)
		return rc ;

	/*
	**	Apply operator
	*/

	threadLog("%s. Calling _selectTerm with terms [%s:%s] and [%s:%s]\n", *_fn, *expA, *termA, *expB, *termB) ;

	if (expA)
		rc = _selectExp(setA, termA) ;
	if (termA)
		rc = _selectTerm(setA, termA) ;

	if (expB)
		rc = _selectExp(setB, expB) ;
	if (termB)
		rc = _selectTerm(setB, termB) ;

	if (op == 1)
	{
		threadLog("%s. OR'ing", *_fn) ;

		for (n = 0 ; n < setA.Count() ; n++)
			elements.Insert(setA.GetObj(n)) ;

		threadLog(" to") ;

		for (n = 0 ; n < setB.Count() ; n++)
			elements.Insert(setB.GetObj(n)) ;

		threadLog(" (total %d)\n", elements.Count()) ;
	}
	else
	{
		threadLog("%s. AND'ing") ;
		for (n = 0 ; n < setA.Count() ; n++)
		{
			pE = setA.GetObj(n) ;
			if (setB.Exists(pE))
				elements.Insert(pE) ;
		}
		threadLog("\n") ;
	}

	threadLog("%s. Found %d tags for term=[%s]\n", *_fn, elements.Count(), *srchExp) ;
	return rc ;
}

hzEcode	hzDocHtml::FindElements	(hzVect<hzHtmElem*>& elements, const hzString& srchExp)
{
	//	Select elements from this document according to the supplied search expression
	//
	//	Webpages (HTML documents) commonly contain a lot of supurfluous matter whilst confining most information content to a limited set of elements (tags). If
	//	it is known which element(s) contain what information (eg title, author, body content etc), FindElements can be used to select these element(s) and from
	//	there, data can be efficiently extracted.
	//
	//	Arguments:	1)	The vector of HTML elements to be populated by this query. A vector is used in preference to a set as this ensures that
	//					the elements found will be in the order of thier incidence in the HTML document.
	//				2)	The criteria as a boolean expression of one or more terms, where each term specifies how elements are to be selected.
	//
	//	Returns:	E_SYNTAX	If the expression is malformed
	//				E_OK		If the operation was successful (it still may have found no elements)
	//
	//	Support functions:
	//
	//	SelectElements() itself calls the private member function _selectExp to do the selecting. This places selected elements in a hzSet ordered
	//	by their RAM address (this ensures tags are only counted once). SelectElements() then re-orders the elements from the hzSet into a hzVect.
	//
	//	_selectExp	(hzSet<hzHtmElem*>& elements, const hzString& exp) simply breaks up the expression into a term or 'term op expression' and calls
	//	the second fupport function _selectTerm() to find the set of tags for each term.
	//
	//	_selectTerm	(hzSet<hzHtmElem*>& elements, const hzString& exp) deals only with terms designed to specify elements. Each term consists of one or
	//	more tag specifiers, which when multiple, are separated by a + sign. A single tag specifier will identify a list of one or more tags within
	//	the document. Subsequent tag specifiers will do the same but will limit the search to descendents of the tags found under the previous tag
	//	specifier. The _selectTerm() calls the third support function _selectTag() on each tag specifier in turn, to actually do the selecting.
	//
	//	_selectTag	(hzSet<hzHtmElem*>& parents, hzSet<hzHtmElem*>& elements, const hzString& exp) uses a single tag specifier to select tags from the
	//	HTML document and then if a list of parents (previously found tags) is supplied the selected tags are tested to ensure they have an ancestor
	//	among the list of parents.
	//
	//	Each tag specifier will be encased in a <> block and be of the general form <tagname attr1='value1' attr2='value2' ...> where either the tag
	//	name or at least one attribute must exist. If an attribute is specified the tag must match on the attribute be be selected. Wildcards can be
	//	used as well.

	hzMapS<uint32_t,hzHtmElem*>	ord ;	//	Ordered set
	hzSet<hzHtmElem*>			res ;	//	Results

	hzHtmElem*	pE ;	//	The HTML element (tag)
	uint32_t	x ;		//	Result set iterator
	hzEcode		rc ;	//	Return code

	elements.Clear() ;
	if (!srchExp)
		return E_OK ;

	//	Get expression 
	rc = _selectExp(res, srchExp) ;
	if (rc != E_OK)
	{
		threadLog("%s. Failed\n", __func__) ;
		return rc ;
	}

	//	Assemble results
	for (x = 0 ; x < res.Count() ; x++)
	{
		pE = res.GetObj(x) ;
		ord.Insert(pE->GetUid(), pE) ;
	}

	for (x = 0 ; x < res.Count() ; x++)
	{
		pE = ord.GetObj(x) ;
		elements.Add(pE) ;
	}

	threadLog("%s. Got %d elements\n", __func__, res.Count()) ;
	return E_OK ;
}

/*
**	Section 2:	hzHtmElem members
*/

hzEcode hzHtmElem::Init	(hzDocHtml* pRoot, hzHtmElem* pParent, hzString& tagname, hzHtagtype type, uint32_t id, uint32_t line)
{
	//	Initialize a HTML element (tag) to the parent element (if any), the tag type. Set also the id and line number (within the HTML
	//	in question)
	//
	//	Arguments:	1)	pRoot	Pointer to the HTML document root
	//				2)	pParent	Pointer to the parent element of this
	//				3)	tagname	The name of this tag
	//				4)	htag	HTML Tag type
	//				5)	id		Numeric identifier
	//				6)	line	Line number of tag in the source HTML file
	//
	//	Returns:	E_ARGUMENT	If no root is supplied
	//				E_OK		If the HTML element was initialized

	_hzfunc("hzHtmElem::Init") ;

	if (!pRoot)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No root supplied") ;

	if (!pParent)
	{
		m_Parent = 0 ;
		m_nLevel = 0 ;
	}
	else
	{
		m_Parent = pParent->GetUid() ;
		m_nLevel = pParent->m_nLevel + 1 ;
		pParent->_addnode(this) ;
	}

	m_Name = tagname ;
	m_Type = type ;
	m_Uid = id ;
	m_nLine = line ;

	m_Children = 0 ;
	m_Sibling = 0 ;

	return E_OK ;
}

hzEcode	hzHtmElem::_addnode	(hzHtmElem* pNode)
{
	//	Adds an element as a subnode of this. Subnodes are always appended.
	//
	//	Arguments:	1)	pNode	Element to add as child of this element
	//
	//	Returns:	E_ARGUMENT	If no element is supplied
	//				E_DUPLICATE	If the supplied element is actually this element
	//				E_OK		If the element is added as child

	_hzfunc("hzHtmElem::_addnode") ;

	hzHtmElem*	p_temp ;	//	Current node pointer

	if (!pNode)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Attempt to add a null node") ;
	if (pNode == this)
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Attempt to add a node to itself (%s)", *m_Name) ;

	if (!m_Children)
		m_Children = pNode->GetUid() ;
	else
	{
		for (p_temp = GetFirstChild() ; p_temp->m_Sibling ; p_temp = p_temp->Sibling())
		{
			if (pNode == p_temp)
				return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Attempt to add an already existing node to %s", *m_Name) ;
		}
		p_temp->m_Sibling = pNode->GetUid() ;
	}

	m_nSubnodes++ ;
	return E_OK ;
}

/*
**	Section 2A:	hzHtmlTable members
*/

uint32_t	hzHtmTbl::Colcount	(void)
{
	//	Establishes the number of column headers. If there are no <th> headers there will still be columns.
	//
	//	Method is to check if there has been an edit (any additional tags) since the last report (of either row or column count). If not then the value held in
	//	m_NoCols is returned. Otherwise the columns are counted explicitly. In the absence of the row of table headers, the column count will be the row with
	//	the maximum number of columns.
	//
	//	Arguments:	None
	//	Returns:	Number of columns

	hzHtmElem*	pE ;		//	Table row tags
	hzHtmElem*	pC ;		//	Columns

	if (!m_nCols)
	{
		pE = GetFirstChild() ; 

		for (pC = pE->GetFirstChild() ; pC ; pC = pC->Sibling())
		{
			//	plog->Out("Colcount - looking at tag %s\n", *Tagtype2Txt(pC->Type())) ;

			if (pC->Type() != HTAG_TH)
				continue ;
			m_nCols++ ;
		}
	}
	
	return m_nCols ;
}

uint32_t	hzHtmTbl::Rowcount	(void)
{
	//	Returns the number of rows. This will not include the row of headers.
	//
	//	Arguments:	None
	//	Returns:	Number of rows in the table

	if (!m_nSubnodes)
	{
		threadLog("Table is empty\n") ;
		return 0 ;
	}

	if (!m_nCols)
	{
		if (!m_nRows)
			Colcount() ;

		if (!m_nCols)
			m_nRows = m_nSubnodes ;
		else
			m_nRows = m_nSubnodes - 1 ;
	}
	
	return m_nRows ;
}

hzString	hzHtmTbl::GetColl	(uint32_t nCol)
{
	//	Return the value (string) of the requested column
	//
	//	In the case of a table, the only allowed sub-nodes are <tr> nodes. The columns for the table are all under the table's first <tr> sub-node as <th> nodes.
	//
	//	Arguments:	1)	nCol	The column number
	//
	//	Returns:	Instance of hzString by value - of the table row as a concatenated series of <td>content</td>

	hzHtmElem*	pE ;		//	Table row tags
	hzHtmElem*	pC ;		//	Columns
	hzString	S ;			//	Target string
	uint32_t	nIndex ;	//	Column iterator

	if (!m_Children)
		return S ;
	pE = GetFirstChild() ; 

	if (!pE->GetFirstChild())
		return S ;

	nIndex = 0 ;
	for (pC = pE->GetFirstChild() ; pC ; pC = pC->Sibling())
	{
		if (pC->Type() != HTAG_THEAD)
			continue ;

		if (nIndex == nCol)
		{
			S = pC->m_tmpContent ;
			break ;
		}

		nIndex++ ;
	}

	return S ;
}

hzString	hzHtmTbl::GetCell	(uint32_t nRow, uint32_t nCol)
{
	//	Return the cell from the supplied row and column.
	//
	//	Method is to move thru the table's <tr> subnodes to get to the row, then move thur that row's <td> (or equivelent) tags to get to the column within the row (the cell).
	//
	//	Arguments:	1)	nRow	The row number
	//				2)	nCol	The column number
	//
	//	Returns:	Instance of hzString by value - of the table cell

	hzHtmElem*	pR ;		//	Table row tags
	hzHtmElem*	pC ;		//	Columns
	hzString	S ;			//	Target string
	uint32_t	row = -1 ;	//	Row counter
	uint32_t	col = 0 ;	//	Column counter

	if (!m_Children)	{ S = "No child nodes" ; return S ; }
	if (!m_nCols)		{ S = "No columns" ; return S ; }

	for (pR = GetFirstChild() ; row <= nRow && pR ; row++, pR = pR->Sibling())
	{
		if (row < nRow)
			continue ;

		for (pC = pR->GetFirstChild() ; col <= nCol && pC ; col++, pC = pC->Sibling())
		{
			if (col < nCol)
				continue ;

			S = pC->m_tmpContent ;
			break ;
		}
		break ;
	}

	return S ;
}

/*
**	Non-member functions
*/

hzDoctype	DeriveDoctype	(hzChain& Z)
{
	//	Category:	Text Processing
	//
	//	Rudimentary check to determine if the document is HTML or XML.
	//
	//	Argument:	Z	Input document
	//
	//	Returns:	The doctype

	chIter	zi ;		//	Chain iterator

	for (zi = Z ; !zi.eof() && *zi != CHAR_LESS ; zi++) ;

	if (zi.Equiv("<html"))
		return DOCTYPE_HTML ;

	if (zi.Equiv("<!DOCTYPE "))
	{
		zi += 10 ;
		if (zi.Equiv("html"))
			return DOCTYPE_HTML ;
		if (zi.Equiv("xml"))
			return DOCTYPE_XML ;
	}

	if (zi.Equiv("<?xml"))
		return DOCTYPE_XML ;
		
	return DOCTYPE_UNDEFINED ;
}
