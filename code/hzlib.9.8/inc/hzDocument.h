//
//	File:	hzDocument.h
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
//	The hzDocument class is the base for hzDocXml and hzDocHtml classes, instances of which would constitute a single XML or HTML document respectively.
//

#ifndef hzDocument_h
#define hzDocument_h

#include "hzErrcode.h"
#include "hzTmplArray.h"
#include "hzTmplList.h"
#include "hzTmplVect.h"
#include "hzTmplSet.h"
#include "hzTmplMapS.h"
#include "hzTmplMapM.h"
#include "hzEmaddr.h"
#include "hzUrl.h"
#include "hzStrRepos.h"
#include "hzProcess.h"

#define HDOC_ONLOAD_LINKS	0x01	//	Upon loading of HTML document, populate the m_setLinks and m_vecLinks with links found in page
#define HDOC_ONLOAD_FORMS	0x02	//	Upon loading of HTML document, populate the m_Forms with forms & thier fields found in page

/*
**	SECTION 1:	HTML Tags
*/

enum	hzDoctype
{
	//	Category:	Document
	//
	//	Enumeration for acceptable document types. This is a vastly cut down set compared to MIME types because it is limited to document structures
	//	that can be loaded into instances of a derivative of the hzDocument class.

	DOCTYPE_UNDEFINED,		//	Document type undefined
	DOCTYPE_HTML,			//	Document is HTML
	DOCTYPE_XML,			//	Document is XML
} ;

enum	hzHtagtype
{
	//	Category:	Document
	//
	//	Enumeration for all currently legal HTML5 tags.
	//
	//	Note that this enum, along with enum hzHtagclass, enum hzHtagrule and the hzHtagform class, are related matter, pertenant both to the generation of HTML
	//	in Dissemino and the parsing of any incoming HTML (web scraping)

	HTAG_NULL,			//	No valid tag

	//	PAGE STRUCTURE      Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_DOCTYPE,		//	<!DOCTYPE>   N N N  Defines the document type
	HTAG_HTML,			//	<html>       N N N  Defines an HTML document
	HTAG_HEAD,			//	<head>       N N N  Defines information about the document
	HTAG_TITLE,			//	<title>      N T N  Defines a title for the document
	HTAG_META,			//	<meta>       N N N  Defines metadata about an HTML document
	HTAG_BODY,			//	<body>       N N Y  Defines the document's body
	HTAG_BASE,			//	<base>       N N ?  Specifies the base URL/target for all relative URLs in a document
	HTAG_BASEFONT,		//	<basefont>   N N ?  (deprecated) Specifies a default color, size, and font for all text in a document
	HTAG_STYLE,			//	<style>      N N N  Marks a stylesheet

	//	FRAMES              Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_FRAME,			//	<frame>      N N Y  (deprecated) Defines a window (a frame) in a frameset
	HTAG_FRAMESET,		//	<frameset>   N N Y  (deprecated) Loads 2 or more frame elements which are separate document
	HTAG_IFRAME,		//	<iframe>     N N Y  (HTML5 vers) Defines an inline frame

	//	PROGRAMING          Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_PARAM,			//	<param>      N N ?  ???
	HTAG_SCRIPT,		//	<script>     N N N  Marks start of JavaScript (or other)
	HTAG_NOFRAMES,		//	<noframes>   N X N  (deprecated) Defines an alternate content for users that do not support frames
	HTAG_NOSCRIPT,		//	<noscript>   N X N  The content of a tag-antitag pair gives message if browser does not support scripts
	HTAG_APPLET,		//	<applet>     N T Y  Marks an applet

	//	DATA/LAYOUT         Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_TABLE,			//	<table>      N N Y  For starting a table
	HTAG_TCOL,			//	<col>        N Y Y  For defining alignment in a table's columns (saves doing it in the cols themselves)
	HTAG_TCOLGRP,		//	<colgroup>
	HTAG_TH,			//	<th>         N X N  Table heading
	HTAG_TR,			//	<tr>         N N Y  Table row marker
	HTAG_TBL_CEL,		//	<td>         N E Y  Table column cell (may either have a set of child tags OR have text content on the same basis as <p>)
	HTAG_TBODY,			//	<tbody>      N Y N  Uused to group the body content in an HTML table - used in conjunction with the thead and tfoot elements
	HTAG_THEAD,			//	<thead>      N Y N  See above
	HTAG_TFOOT,			//	<tfoot>      N Y N  See above
	HTAG_DIV,			//	<div>        N Y Y  Defines a functional section in a document
	HTAG_SPAN,			//	<span>       N N Y  Defines a section in a document
	HTAG_FIELDSET,		//	<fieldset>   N N Y  Tag is used to logically group together elements in a form. (draws a box around the related form elements)
	HTAG_LEGEND,		//	<legend>     N Y N  Tag defines a caption for the fieldset element.
	HTAG_MENU,			//	<menu>       N Y Y  Marks menu (of items)
	HTAG_DT,			//	<dt>         X X N  Definition term
	HTAG_DD,			//	<dd>         X X N  The definition (dd always follows a dt)
	HTAG_DFN,			//	<dfn>        X X N  This is another form of the definition term tag
	HTAG_DIR,			//	<dir>        N N Y  Directory list (of <li> tags)
	HTAG_DLIST,			//	<dl>         N N Y  Definition lists
	HTAG_OLIST,			//	<ol>         N N Y  Ordered list
	HTAG_ULIST,			//	<ul>         N N Y  Unordered list
	HTAG_ITEM,			//	<li>         N Y N  Xlat List item
	HTAG_TIME,			//	<time>       Y Y N  Cont Either attr of datetime=value or value found in the content

	//	LINKS               Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_ANCHOR,		//	<a>          X X N  Marks a link
	HTAG_NAV,			//	<nav>        X X N  Marks a link
	HTAG_LINK,			//	<link>       X X N  Marks a link

	//	INPUT/FORMS         Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_FORM,			//	<form>       N N Y  Marks start of a form
	HTAG_INPUT,			//	<input>      X X N  Covers button/checkbox/file/hidden/image/password/radio/reset/submit/text/value
	HTAG_TEXTAREA,		//	<textarea>   N X N  Marks an text area (multiline input box)
	HTAG_SELECT,		//	<select>     N N Y  Selector
	HTAG_OPTGROUP,		//	<optgroup>   N X Y  Groups of select options
	HTAG_OPTION,		//	<option>     N X N  Select options
	HTAG_BUTTON,		//	<button>     X X N  Alternative to <input type=submit ...>

	HTAG_LABEL,			//	<label>      N Y N  E.g. <form>
						//                           <label for="male">Male</label><input type="radio" name="sex" id="male" />
						//                           <label for="female">Female</label><input type="radio" name="sex" id="female" />
						//                           </form>

	//	INFORMATION         Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_ABBR,			//	<abbr>
	HTAG_ACRONYM,		//	<acronym>
	HTAG_ADDRESS,		//	<address>

	//	SYSTEM              Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_EMBED,			//	<embed>
	HTAG_NOEMBED,		//	<noembed>

	//	Font control tags. Note these cannot have meaningful content in thier own right and are instead considered part of the content of a tag
	//	that can.
	//	FONT CONTROL        Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_BOLD,			//	<b>          X X N  Bold text (must be closed)
	HTAG_ULINE,			//	<u>          X X N  Underline
	HATG_ITALIC,		//	<i>          X X N  Italics
	HTAG_EM,			//	<em>         X X N  Emphasize
	HTAG_STRONG,		//	<strong>     X X N  Emphasize
	HTAG_CENTER,		//	<center>     N X Y  Center
	HTAG_FONT,			//	<font>       X X Y  Font setting
	HTAG_BIG,			//	<big>        X X Y  Larger text
	HTAG_SMALL,			//	<small>      X X Y  Smaller text

	//	TEXT DESCRIPTION    Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_HEADER,		//	<header>     N X N  Defines a header for a document or section
	HTAG_FOOTER,		//	<footer>     N X N  Defines a footer for a document or section
	HTAG_SECTION,		//	<section>    N X N  Defines a section in a document
	HTAG_ARTICLE,		//	<article>    N X N  Defines an article
	HTAG_ASIDE,			//	<aside>      N X N  Defines content aside from the page content
	HTAG_DETAILS,		//	<details>    N X N  Defines additional details that the user can view or hide
	HTAG_SUMMARY,		//	<summary>    N X N  Defines a visible heading for a <details> element
	HTAG_DIALOG,		//	<dialog>     N X N  Defines a dialog box or window

	//	TEXT GROUPING       Tagname      P C S  Desc
	//	----------------------------------------------------
	//HTAG_FB,			//	<fb>         ? ? ?  Facebook tag - treak as a comment block and ignore
	//HTAG_GOOGLE,		//	<g>          ? ? ?  Google tag - treak as a comment block and ignore
	HTAG_MISC_ORG,		//	<x:y>        ? ? ?  Third party tag containing a colon
	HTAG_STRIKE,		//	<strike>     ? ? ?  Effectively a comment block
	HTAG_S,				//	<s>          ? ? ?  Same as <strike>
	HTAG_DEL,			//	<del>        X N N  Marks deleted text (contents to be ignored)
	HTAG_INS,			//	<ins>        ? ? ?  Marks an inserted part! (contents to be agregated to parent)
	HTAG_KBD,			//	<kbd>        ? ? ?  Keyboard text
	HTAG_QUOTATION,		//	<q>, </q>    ? ? ?  Paragragh (can be closed or be ended by next <p> or by data structure tags

 	//	TEXT CONTROL        Tagname      P C S  Desc
 	//	----------------------------------------------------
	HTAG_PARAG,			//	<p>, </p>    N X Y  Paragragh (can be closed or be ended by next <p> or by data structure tags)
	HTAG_H1,			//	<h1>         X X N  Size 1 heading
	HTAG_H2,			//	<h2>         X X N  Size 2 heading
	HTAG_H3,			//	<h3>         X X N  Size 3 heading
	HTAG_H4,			//	<h4>         X X N  Size 4 heading
	HTAG_H5,			//	<h5>         X X N  Size 5 heading
	HTAG_H6,			//	<h6>         X X N  Size 6 heading
	HATG_BR,			//	<br>         X X N  Non-breaking line (newline). This has no anti-tag
	HATG_HR,			//	<hr>         X X N  Defines a thematic change in the content. No anti-tag
	HATG_TT,			//	<tt>         X X N  Teletype
	HTAG_HGROUP,		//	<hgroup>     X X N  heading group
	HTAG_CODE,			//	<code>       X X N  For de-marking computer code
	HTAG_SAMP,			//	<samp>       X X N  For de-marking smaple computer code
	HTAG_CITE,			//	<cite>       X X N  For marking out text
	HTAG_CAPTION,		//	<caption>    X X N  For marking out text
	HTAG_VAR,			//	<var>        X X N  For marking out text
	HTAG_PRE,			//	<pre>        N T N  As-is text (allows multiple spaces and tags)
	HTAG_BQ,			//	<bq>         ? ? ?  ???
	HTAG_BLOCKQUOTE,	//	<blockquote> X X Y  Indents encased HTML block
	HTAB_BDO,			//	<bdo>        X X Y  Controls text direction (left-to-right instead of right-to-left)
	HTAG_SUBSCRIPT,		//	<sub>        X T N  Subscripts (eg chemical formula)
	HTAG_SUPERSCRIPT,	//	<sup>        X T N  For expressing powers

	//	IMAGE               Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_IMG,			//	<img>        X N N  Includes an image file
	HTAG_MAP,			//	<map>        X N N  Uses a series of <area> tags to mark out area of an image.
	HTAG_AREA,			//	<area>       ? ? ?  Used to mark out areas of an image
	HTAG_OBJECT,		//	<object>     X X Y  Includes objects such as images, audio, videos, Java applets, ActiveX, PDF, and Flash.
	HTAG_HR,			//	<hr>         X N N  Horizontal line
	HTAG_MARQUEE,		//	<marquee>    ? ? ?  Ticker
	HTAG_CANVAS,		//	<canvas>     N N Y  Used to draw graphics, on the fly, via scripting (usually JavaScript)
	HTAG_FIGCAPTION,	//	<figcaption> X X N  Defines a caption for a <figure> element
	HTAG_FIGURE,		//	<figure>     X X N  Specifies self-contained content

	//	AUDIO/VIDEO         Tagname      P C S  Desc
	//	----------------------------------------------------
	HTAG_AUDIO,			//	<audio>      X X N  Defines sound content
	HTAG_SOURCE,		//	<source>     X X N  Defines multiple media resources for media elements (<video> and <audio>)
	HTAG_TRACK,			//	<track>      X X N  Defines text tracks for media elements (<video> and <audio>)
	HTAG_VIDEO,			//	<video>      X X N  Defines a video or movie

	//	THIRD PARTY
	//	----------------------------------------------------
	HTAG_FBLIKE,		//	<fb:like>	Defines the Facebook like icon

	HTAG_UNKNOWN		//	Invalid tag
} ;

enum	hzHtagclass
{
	//	Category:	Document
	//
	//	HTML Tag groupings by function
	//
	//	Note that this enum, along with enum hzHtagtype, enum hzHtagrule and the hzHtagform class, are related matter, pertenant both to the generation of HTML
	//	in Dissemino and the parsing of any incoming HTML (web scraping)

	HTCLASS_NUL,		//	No valid class
	HTCLASS_HDR,		//	Page structure tags
	HTCLASS_DAT,		//	Data/layout tags
	HTCLASS_LNK,		//	Link tags
	HTCLASS_INP,		//	Input/form tags. Attrs only, no content
	HTCLASS_INF,		//	Information tags
	HTCLASS_SYS,		//	System tags (embed directives)

	HTCLASS_TXT,		//	Font control tags. Note these cannot have meaningful content in thier own right and are instead treated as part of the content of a
						//	parent tag that can. This also means that during data extraction from a HTML page, these tags are ignored.

	HTCLASS_IMG,		//	Image tags (no text content)
	HTCLASS_3RD			//	Third party tag (no text content, ignored)
} ;

enum	hzHtagrule
{
	//	Category:	Document
	//
	//	Rules concerning opening and closing of tags
	//
	//	Note that this enum, along with enum hzHtagtype, enum hzHtagclass and the hzHtagform class, are related matter, pertenant both to the generation of HTML
	//	in Dissemino and the parsing of any incoming HTML (web scraping)

	HTRULE_NULL,		//	No valid rule
	HTRULE_PAIRED,		//	HTML Tag must be closed with either the <... /> notation or with the anti-tag
	HTRULE_SINGLE,		//	HTML Tag is not closed as it is its own anti-tag
	HTRULE_OPTION		//	HTML Tag closure is optional (anti-tag exists but is not required)
} ;

class	hzHtagform
{
	//	Category:	Document
	//
	//	Used in the tag lookup table to tie together the name, type, class and rule for each HTML tag
	//
	//	hzTagform is the item of interest when establishing if a tag is a legal HTML(5) tag. It comprises the tag name/type both in text and enumerated form and
	//	the applicable tag class and rule. The function HtmlInit() establishes a lookup table of all known HTML5 tags for the benefit of the HTML parser.

public:
	hzString	name ;	//	Tag name
	hzHtagtype	type ;	//	Tag type
	hzHtagclass	klas ;	//	Tag class
	hzHtagrule	rule ;	//	Tag rule

	hzHtagform&	operator=	(const hzHtagform& op)
	{
		name = op.name ;
		type = op.type ;
		klas = op.klas ;
		rule = op.rule ;

		return *this ;
	}
} ;

class	hzDocHtml ;

class	hzDocMeta
{
	//	Category:	Document
	//
	//  A document description, commonly used as a 'page marker' for webpages in web-scraping programs.
	//
	//  This will have the URL, the scrambled filename for storing the page in the repository and both the date last fetched and the expiry date.

public:
	hzXDate		m_Download ;	//	Date and time of last download
	hzXDate		m_Modified ;	//	Last modified date according to page header
	hzXDate		m_Expires ;		//	When page falls out of date
	hzUrl		m_urlReq ;		//	Requested URL
	hzUrl		m_urlAct ;		//	Actual location of page
	hzString	m_Title ;		//	Title of page (or sub-RSS)
	hzString	m_Desc ;		//	Description (RSS only)
	hzString	m_Filename ;	//	Filename in repository
	hzString	m_Etag ;		//	Entity tag if supplied
	uint32_t	m_Id ;			//	Assigned by webscrape to track order
	hzDoctype	m_Doctype ;		//	Document type (XML/HTML)

	hzDocMeta	(void)
	{
		m_Id = 0 ;
	}

	void	Clear	(void)
	{
		m_urlReq = (char*) 0 ;
		m_urlAct = (char*) 0 ;
		m_Title = (char*) 0 ;
		m_Desc = (char*) 0 ;
		m_Filename = (char*) 0 ;
		m_Etag = (char*) 0 ;
		m_Modified.Clear() ;
		m_Expires.Clear() ;
		m_Doctype = DOCTYPE_UNDEFINED ;
	}

	hzDocMeta&	operator=	(const hzDocMeta& op)
	{
		m_urlReq = op.m_urlReq ;
		m_urlAct = op.m_urlAct ;
		m_Title = op.m_Title ;
		m_Desc = op.m_Desc ;
		m_Filename = op.m_Filename ;
		m_Etag = op.m_Etag ;
		m_Modified = op.m_Modified ;
		m_Expires = op.m_Expires ;
		m_Doctype = op.m_Doctype ;
		m_Id = op.m_Id ;

		return *this ;
	}

	hzUrl&	Locale	(void)
	{
		if (*m_urlAct)
			return m_urlAct ;
		return m_urlReq ;
	}

	hzString	Domain	(void)
	{
		if (*m_urlAct)
			return m_urlAct.Domain() ;
		return m_urlReq.Domain() ;
	}
} ;

class	hzDocument
{
	//	Category:	Document
	//
	//	Pure virtual base class for the HTML document (hzDocHtml) and the XML document (hzDocXml). These two classes were tied together only because web scrape
	//	tools could find themselves dowloading both but all were considered documents.

protected:
	hzStrRepos	m_Dict ;	//	All strings
	hzDocMeta	m_Info ;	//	Metadata
	hzChain		m_Error ;	//	Error reporting

public:
	hzMapM	<uint32_t,hzNumPair>	m_NodeAttrs ;	//	All node attributes


	//	Constructor/Destructor
	hzDocument			(void)	{}
	virtual	~hzDocument	(void)	{}

	hzEcode	Init	(const hzUrl& url) ;

	const char*	Xlate	(uint32_t strNo) const	{ return m_Dict.Xlate(strNo) ; }

	void		SetMeta	(const hzDocMeta& dm)	{ m_Info = dm ; }

	const hzChain&	Error	(void)	{ return m_Error ; }

	virtual	hzEcode		Load	(hzChain& Z) = 0 ;
	virtual	hzDoctype	Whatami	(void) const = 0 ;
} ;

class	hzDocHtml ;

class	hzHtmElem
{
	//	Category:	Document
	//
	//	hzHtmlElem is the internal manifestation of a HTML tag appearing in a HTML document.
	//
	//	Note the use of 32-bit string numbers instead of pointers for the parent, first child and next sibling. It is for the resolution of these by means of a
	//	hzStrRepos, that each HTML tag contains a pointer back to the host document which holds the dictionary.

protected:
	hzDocHtml*	m_pHostDoc ;		//	Host document
	hzString	m_Name ;			//	Name of this tag
	uint32_t	m_Parent ;			//	Parent node (for root this is a hzDocHtml, all other nodes this is a hzHtmElem)
	uint32_t	m_Children ;		//	Sub nodes of this node
	uint32_t	m_Sibling ;			//	Next node (in the series m_Chridren belonging to the parent of this)
	uint32_t	m_Uid ;				//	Unique id (within page)
	uint32_t	m_nLine ;			//	Line number of tag in the page source
	uint32_t	m_nAnti ;			//	Line number of anti-tag in the page source
	uint16_t	m_nLevel ;			//	Level of node (root node is 0)
	uint16_t	m_nAttrs ;			//	Number of parameters (not set until page load complete)
	uint32_t	m_nSubnodes ;		//	Number of sub-nodes (not set until page load complete)
	hzHtagtype	m_Type ;			//	Type of HTML tag

	//	Adding subnodes
	hzEcode		_addnode	(hzHtmElem* pNode) ;
	uint32_t	_testnode	(hzVect<hzHtmElem*>& ar, const char* srchExp, uint32_t& nLimit, uint32_t nLevel, bool bLog = false) ;

public:
	hzChain		m_tmpContent ;		//	Content of this tag
	hzString	m_fixContent ;		//	Contents of the tag (after loading if n_tempContent is small)

	//	Constructors/Destructors
	hzHtmElem	(void)
	{
		m_Parent = m_Children = m_Sibling = 0 ;
		m_Uid = -1 ;
		m_nLine = 0 ;
		m_nAnti = 0 ;
		m_nLevel = 0 ;
		m_nAttrs = m_nSubnodes = 0 ; 
		m_Type = HTAG_NULL ;
	}

	virtual	~hzHtmElem	(void)	{}

	//	Initialization
	hzEcode		Init		(hzDocHtml* pRoot, hzHtmElem* pParent, hzString& tagname, hzHtagtype type, uint32_t id, uint32_t line) ;
	void		_setanti	(uint32_t line)	{ m_nAnti = line ; }

	//	Geting subnodes and params
	void		FindSubnodes	(hzVect<hzHtmElem*>& result, const char* srchExp, bool bLog = false) ;

	hzHtmElem*	GetFirstChild	(void) const ;
	hzHtmElem*	Sibling			(void) const ;
	hzHtmElem*	Parent			(void) const ;

	const hzDocHtml*	GetHostDoc	(void) const	{ return m_pHostDoc ; }

	//	Get other node info
	hzDocHtml*	GetTree		(void) ;
	hzString	Name		(void) const	{ return m_Name ; }
	hzHtagtype	Type		(void) const	{ return m_Type ; }
	uint32_t	Level		(void) const	{ return m_nLevel ; }
	uint32_t	Line		(void) const	{ return m_nLine ; }
	uint32_t	Anti		(void) const	{ return m_nAnti ; }
	uint32_t	GetUid		(void) const	{ return m_Uid ; }
} ;

class	hzHtmCol	: public hzHtmElem
{
	//	Category:	Document
	//
	//	Column in a table

public:
	hzString	m_Title ;		//	To appear above each row
	hzString	m_HdrRef ;		//	Link from heading if any
	hzString	m_Value ;		//	Cell value (must update this for each cell value)
	hzString	m_ValRef ;		//	Link from cell if any
	hzString	m_Class ;		//	Style-sheet class (if set this will be default for the table's <td> tags)
	uint32_t	m_BgColor ;		//	Background color (if not set use table value)
	uint32_t	m_FgColor ;		//	Foreground color (if not set use table value)
	uint16_t	m_Margin ;		//	Width of preceeding spaces
	uint16_t	m_Width ;		//	Width in pixels

	hzHtmCol	(void)
	{
		m_BgColor = 0x000000 ;
		m_BgColor = 0xffffff ;
		m_Margin = 5 ;
		m_Width = 10 ;
	}
} ;

class	hzHtmTbl	: public hzHtmElem
{
	//	Category:	Document
	//
	//	In HTML tables have sub-tags only of <tr> (table row). Columns are effected by the <tr> sub-tags. In the first row the columns can be named by the <th>
	//	(table heading) tag, although this is frequently done with the <td> tag instead - meaning that 
	//
	//	When querying a table for a cell value, we have to specify the column-name and the row number. If the table has <th> tags in
	//	the first row, the values of these will be compared to the supplied column name. If the table does not have <th> tags in the
	//	first row and instead has <td> tags, the values of these will be used instead.

	hzVect<hzHtmCol*>	m_Cols ;	//	Columns

	hzString	m_Title ;			//	Title of table
	hzString	m_Class ;			//	Style sheet class
	hzString	m_Url ;				//	To be pre-pending for links
	hzString	m_Empty ;			//	Msg to be displayed when DoFooter() is called with no rows done
	uint32_t	m_BgColor ;			//	Background color
	uint32_t	m_FgColor ;			//	Foreground color
	uint16_t	m_Height ;			//	Total hieght in pixels
	uint16_t	m_Width ;			//	Total width in pixels
	uint16_t	m_Border ;			//	Border width
	uint16_t	m_Cellspace ;		//	Cell spacing
	uint16_t	m_Cellpad ;			//	Cell pading
	uint16_t	m_nCols ;			//	Number of columns
	uint16_t	m_nRows ;			//	Number of data rows

public:
	hzHtmTbl	(void)
	{
		m_BgColor = 0xffffff ;
		m_FgColor = 0x000000 ;
		m_Height = 500 ;
		m_Width = 800 ;
		m_Border = 0 ;
		m_Cellspace = 0 ;
		m_Cellpad = 0 ;
		m_nCols = m_nRows = 0 ;
	}

	~hzHtmTbl	(void)
	{
	}

	//	Initialization
	void	AddColumn	(hzHtmCol* col)
	{
		m_Cols.Add(col) ;
	}

	//	Set functions
	void		SetTitle	(const char* title)		{ m_Title = title ; }
	void		SetClass	(const char* cls)		{ m_Class = cls ; }
	void		SetUrl		(const char* url)		{ m_Url = url ; }
	void		SetEmpty	(const char* empty)		{ m_Empty = empty ; }
	void		SetBgColor	(uint32_t color)		{ m_BgColor = color ; }
	void		SetFgColor	(uint32_t color)		{ m_FgColor = color ; }
	void		SetHeight	(uint32_t h)			{ m_Height = h ; }
	void		SetWidth	(uint32_t w)			{ m_Width = w ; }

	//	Get functions
	uint32_t	Colcount	(void) ;
	uint32_t	Rowcount	(void) ;

	hzString&	GetUrl		(void)	{ return m_Url ; }

	hzString	GetColl		(uint32_t nCol) ;
	hzString	GetCell		(uint32_t nRow, uint32_t nCol) ;
} ;

/*
**	Section 3:	Entities that comprise a series of HTML tags - The hzDocHtml class
*/

class	hzHtmForm
{
	//	Category:	Document
	//
	//	hzHtmForm is analogous to the hzwForm (web form) class except that the latter is for page generation and so has 'rendering control' data
	//	to enable HTML formation. The hzHtmForm class marks a form found in a downloaded page and notes only the form name and a list of fields
	//	manifest as name-value pairs (name of field plus a pre-set value if provided).

public:
	hzList<hzPair>	fields ;	//	Incident fields
	hzString		name ;		//	Form name

	~hzHtmForm	(void)
	{
		fields.Clear() ;
	}
} ;

class	hzDocHtml	: public hzDocument
{
	//	Category:	Document
	//
	//	A whole or partial HTML Page or Document

	hzHtmElem*	m_pRoot ;			//	All tags found on level 0
	hzHtmElem*	m_pHead ;			//	All tags found in the header (head is level 1)
	hzHtmElem*	m_pBody ;			//	All tags found in the body (body is level 1)

	hzString	m_CookieSess ;		//	Set by HTML header upon Browse() or LoadHtml() or LoadFile()
	hzString	m_CookiePath ;		//	Set by HTML header upon Browse() or LoadHtml() or LoadFile()
	hzString	m_Title ;			//	Will be filename on export
	hzString	m_EntityTag ;		//	Entity tag from the header if given

	//	Documet building functions
	hzHtmElem*	_proctag	(hzHtmElem* pParent, hzChain::Iter& cur, hzHtagtype type) ;
	hzEcode		_htmPreproc	(hzChain& Z) ;

	//	Reporting and export
	void		_report		(hzLogger& xlog, hzHtmElem* node) ;
	hzEcode		_xport		(hzChain& Z, hzHtmElem* node) ;

	//	Support for element selection
	hzEcode		_selectTag	(hzSet<hzHtmElem*>& parents, hzSet<hzHtmElem*>& elements, const hzString& tagspec) ;
	hzEcode		_selectTerm	(hzSet<hzHtmElem*>& elements, const hzString& term) ;
	hzEcode		_selectExp	(hzSet<hzHtmElem*>& elements, const hzString& exp) ;

public:
	hzMapM	<hzString,hzHtmElem*>	m_mapTags ;	//	All nodes within document

	hzArray	<hzHtmElem>		m_arrNodes ;		//	All nodes within document
	hzSet	<hzUrl>			m_setLinks ;		//	Links to other pages occuring in this page's body
	hzSet	<hzEmaddr>		m_Emails ;			//	Email addresses occuring in this page's body
	hzVect	<hzUrl>			m_vecLinks ;		//	All elements in the order they appear
	hzVect	<hzHtmElem*>	m_vecTags ;			//	All elements in the order they appear
	hzVect	<hzString>		m_vecText ;			//	All text sections found in page
	hzList	<hzHtmForm*>	m_Forms ;			//	List of forms appearing in the page (if any)

	hzChain		m_Content ;		//	Full content of web-page
	hzString	m_Base ;		//	Base for URLs begining with /

	hzDocHtml	(void) ;
	~hzDocHtml	(void) ;

	hzDoctype	Whatami	(void) const	{ return DOCTYPE_HTML ; }

	hzEcode	Load	(hzChain& Z) ;				//	Load HTML document from a hzChain instance
	hzEcode	Load	(const char* cpFilename) ;	//	Load HTML document from a file
	hzEcode	Import	(const hzString& filepath) ;
	hzEcode	Export	(const hzString& filepath) ;
	void	Report	(hzLogger& xlog) ;
	void	Clear	(void) ;

	//	Get functions
	hzHtmElem*	GetRoot		(void)	{ return m_pRoot ; }
	hzString&	CookieSess	(void)	{ return m_CookieSess ; }
	hzString&	CookiePath	(void)	{ return m_CookiePath ; }

	//	Obtain a vector of elements according to tagname and attribute incidence
	hzEcode		FindElements	(hzVect<hzHtmElem*>& elements, hzString& tagname, hzString& attrName, hzString& attrValue) ;

	//	Obtain a vector of elements according to filtering criteria
	hzEcode		FindElements	(hzVect<hzHtmElem*>& elements, const char* srchExp) ;
	hzEcode		FindElements	(hzVect<hzHtmElem*>& elements, const hzString& srchExp) ;

	//	Extract links according to filtering criteria, from a page either as basic (URLs only) or URLs plus tag content
	uint32_t	ExtractLinksBasic	(hzVect<hzUrl>&links, const hzSet<hzString>& domains, const hzString& criteria) ;
	uint32_t	ExtractLinksContent	(hzMapS<hzUrl,hzString>&links, const hzSet<hzString>& domains, const hzString& criteria) ;
} ;

#define INIT_START	0	//	Nothing happened yet

/*
**	Section 1:	Classes concerning XML formatting issues
**
**	XML, DTD, XLST and HadronZoo Records.
**
**	XML is a heirarchical data form in which data is held in tags. The data itself boils down to a series of name-value pairs which are 
**	related by virtual of thier position. The values are typeless strings although XML does give direction as to how these strings are
**	to be parsed.
**
**	DTD (Document Type Definition) puts constraints on the XML which will be regared by the XML parser as invalid unless it conforms
**	to the form implied in the DTD.
**
**	XLST (Extensible Language Stylesheet Transformations) are applied to XML documents so that they may be transformed from one form
**	(in one DTD) into another (in a second DTD). XLST describes the steps that must be taken to achieve this and this includes function
**	calls and data typing.
**
**	HadronZoo Records (hzRecord) are like XML, a heirarchical data form except that the values in the name-value pairs are strongly
**	type controlled. The form of the records is strictly defined by hzRecfmt which is alogous to DTD except that is contains typing
**	directives. To populate records from an XML source will require some form of an XLST.
*/

enum	XmlType
{
	//	Category:	Document
	//
	//	Controls the type of data held by a tag

	XML_TYPE_UNDEF,		//	Undefined (default)
	XML_CDATA,			//	Interpret node contents as pure character data requiring no processing or interpretation
	XML_PCDATA			//	Parsed char data - Change chars to entities where appropriate and treat <p> ect as HTML
						//	tags rather than XML tags (nodes) 
} ;

enum	hzXOccur
{
	//	Category:	Document
	//
	//	This is a quantifier (given by a single character in the DTD) that immediately follows the specified item to which it applies,
	//	to restrict the number of successive occurrences of these items at the specified position in the content of the element; it
	//	and may be either:

	XML_INCID_PLUS,		//	(+)	Specifying that there must be one or more occurrences of the item.
	XML_INCID_STAR,		//	(*) Specifying that any number (zero or more) of occurrences is allowed, the item is thus optional.
	XML_INCID_MAKR,		//	(?) Specifying that there must not be more than one occurrence, the item is optional.
	XML_INCID_DFLT		//	( ) If there is no quantifier, the specified item must occur exactly once at the specified position in the content of the element.
} ;

enum	hzHtagInd
{
	//	Category:	Document
	//
	//	This is returned by the AtHtmlTag function which determines if the supplied chain iterator is at either:-

	HTAG_IND_NULL,		//	Not at a HTML tag or anti-tag
	HTAG_IND_OPEN,		//	At the start of an opening HTML tag
	HTAG_IND_ANTI,		//	At the start of an HTML anti-tag
	HTAG_IND_SELF		//	At the start of a self closing HTML tag
} ;

struct	hzAttrSet
{
	//	Category:	Document
	//
	//	This defines what attributes are allowed within an XML tag

	hzAttrSet*	next ;			//	Next attribute if applicable
	hzString	m_Name ;		//	Name of attribute
	hzXOccur	m_Incidence ;	//	Quantity control

	hzAttrSet	(void)
	{
		next = 0 ;
		m_Incidence = XML_INCID_DFLT ;
	}
} ;

struct	hzTagCtrl
{
	//	Category:	Document
	//
	//	This defines what form of data and attributes nodes of this tag can have. hzTagCtrl is part
	//	of the doctype for the hzDocXml class.

	hzAttrSet*	m_pAttrs ;		//	Allowed attributes
	hzString	m_Name ;		//	Name of tag (copy of that in doctype map)
	hzString	m_FQN ;			//	Fully qualified name of tag (copy of that in doctype map)
	XmlType		m_Type ;		//	Type eg CDATA
	hzXOccur	m_Incidence ;	//	Quantity control

	hzTagCtrl	(void)
	{
		m_pAttrs = 0 ;
		m_Type = XML_TYPE_UNDEF ;
		m_Incidence = XML_INCID_DFLT ;
	}
} ;

class	hzDocCtrl
{
	//	Category:	Document
	//
	//	Document controller: This is the internal manifestation of a document type definition (DTD) and defines an object model to which a hzDocuement
	//	instance must conform to be valid.

public:
	hzMapM<hzString,hzTagCtrl*>	m_TagsByName ;	//	All tags within document (by tag name only)
	hzMapS<hzString,hzTagCtrl*>	m_TagsByFQN ;	//	All tags within document (by fully qualified tag name)

	hzTagCtrl*	m_pRoot ;		//	The root tag, the document has this tag at it's root
	hzString	m_Name ;		//	Name of the doctype

	hzDocCtrl	(void)
	{
		m_pRoot = 0 ;
	}
} ;

/*
**	SECTION 2:	Classes storing XML data
*/

class	hzDocXml ;

class	hzXmlNode
{
	//	Category:	Document
	//
	//	These form the nodes in the hzXmlObj tree (the XML document)

	hzDocXml*	m_pHostDoc ;	//	Host XML document
	uint32_t	m_Parent ;		//	Parent node - Note in the root node this will point to the document. Where applicable, this must be cast to hzDocXml.
	uint32_t	m_Children ;	//	Sub nodes of this node
	uint32_t	m_Sibling ;		//	Next node (in the series m_Chridren belonging to the parent of this)
	uint32_t	m_Uid ;			//	Unique id
	uint32_t	m_snPtxt ;		//	String number for pre-text
	uint32_t	m_snName ;		//	String number for Name
	uint32_t	m_nLine ;		//	Line number of tag
	uint32_t	m_nAnti ;		//	Line number of anti-tag
	uint16_t	m_nLevel ;		//	Level of node (root node is 0)
	uint16_t	m_nCol ;		//	Column of node within line wthin document
	uint16_t	m_bXmlesce ;	//	Node may contain HTML tags as part of the content
	uint16_t	m_nAttrs ;		//	Number of attributes

	hzXmlNode*	_findsubnode	(bool& bMatch, const hzString& name, const hzString& attr, const hzString& value) ;
	uint32_t	_testnode		(hzVect<hzXmlNode*>& ar, const char* srchExp, uint32_t& nLimit) ;

public:
	hzString	m_fixContent ;	//	Contents of the tag (after loading if m_tmpContent is small)

	hzXmlNode	(void)
	{
		m_pHostDoc = 0 ;
		m_Parent = m_Children = m_Sibling = 0 ;
		m_Uid = m_snPtxt = m_snName = m_nLine = m_nAnti = 0 ;
		m_nLevel = m_nCol = m_bXmlesce = m_nAttrs = 0 ;
	}

	~hzXmlNode	(void)	{}

	hzXmlNode&	operator=	(const hzXmlNode& op)
	{
		m_pHostDoc = op.m_pHostDoc ;
		m_Parent = op.m_Parent ;
		m_Children = op.m_Children ;
		m_Sibling = op.m_Sibling ;
		m_Uid = op.m_Uid ;
		m_snPtxt = op.m_snPtxt ;
		m_snName = op.m_snName ;
		m_nLine = op.m_nLine ;
		m_nAnti = op.m_nAnti ;
		m_nLevel = op.m_nLevel ;
		m_nCol = op.m_nCol ;
		m_bXmlesce = op.m_bXmlesce ;
		m_nAttrs = op.m_nAttrs ;
	} 

	hzXmlNode*	Init	(hzDocXml* pHostDoc, hzXmlNode* pParent, uint32_t snName, uint32_t nLineNo, uint32_t nCol, bool bXmlesce = false) ;

	void	_setanti	(uint32_t lineNo)	{ m_nAnti = lineNo ; }	//	Set line number of anti-tag (mark end of node)

	void	Clear	(void) ;

	//	Adding subnodes
	hzEcode	AddNode	(hzXmlNode* pNode) ;

	//	Set content
	void	SetCDATA	(hzChain& C) ;
	hzEcode	SetPretext	(hzChain& C) ;
	void	SetContent	(hzChain& C) ;

	//	Geting subnodes and params
	void		Export_r		(hzDocXml* pDoc, hzChain& Z, uint32_t& relLine) ;
	void		Export			(hzChain& Z) ;
	hzEcode		SelectSubnodes	(hzVect<hzXmlNode*>& result, hzMapM<hzString,hzXmlNode*>& allsubnodes, const char* srchExp) ;
	void		FindSubnodes	(hzVect<hzXmlNode*>& result, const char* srchExp) ;
	hzXmlNode*	FindSubnode		(const char* srchExp) ;
	bool		IsAncestor		(hzXmlNode* candidate) ;

	bool		IsXmlesce		(void) const	{ return m_bXmlesce == 1 ? true : false ; }
	hzXmlNode*	GetFirstChild	(void) const ;
	hzXmlNode*	Sibling			(void) const ;
	hzXmlNode*	Parent			(void) const ;

	const char*	Xlate			(uint32_t strNo) const ;

	uint32_t	GetNoAttrs		(void) const	{ return m_nAttrs ; }

	//	Get other node info
	//hzString	Lineage		(void) const ;
	hzString	Filename	(void) const ;
	const char*	Fname		(void) const ;

	const hzDocXml*	GetHostDoc	(void) const	{ return m_pHostDoc ; }

	uint32_t	GetUid		(void) const	{ return m_Uid ; }
	uint32_t	Line		(void) const	{ return m_nLine ; }
	uint32_t	Anti		(void) const	{ return m_nAnti ; }
	uint32_t	Level		(void) const	{ return m_nLevel ; }
	uint32_t	StrnoName	(void) const	{ return m_snName ; }
	uint32_t	StrnoPtxt	(void) const	{ return m_snPtxt ; }
	const char*	TxtName		(void) const ;
	const char*	TxtPtxt		(void) const ;
	bool		NameEQ		(const char* testname) const ;
} ;

class	hzAttrset
{
	//	Category:	Document
	//
	//	The hzAttrset class is really an convenient attribute iterator. The tag attributes are stored in the host document in a one-to-many map between tag uids
	//	and attribute, rather than in a list held by the host tag. Given this arrangement, iteration of tag attributes would normally require a first and a last
	//	position within the map, as well as another variable to iterate between the two. In previous version of the HadronZoo library, each tag had an attribute
	//	pointer and each attribute had a next pointer. Attributes could be iterated by control loops of the form:-
	//
	//		for (attr = first_attr ; attr ; attr = attr->next)	{}
	//
	//	The objective of the attribute iterator is to achieve a similar interface. The hzAttrset is initialized to a tag (node) which in turn points to the host
	//	document. The initialization does a lookup in the document map and the Valid() method returns true if there is an attribute. The Advance() method moves
	//	on to the next attribut in the tag so:-
	//
	//		for (attrset = node ; attrset.Valid() ; attrset.Advance())	{}

	//const hzXmlNode*	m_pHostNode ;	//	This is needed for the node's uid and the document hosting the map
	const hzDocument*	m_pHostDoc ;	//	This is needed for the node's uid and the document hosting the map

	hzNumPair	m_Pair ;		//	Name/value pair
	uint32_t	m_NodeUid ;		//	Node Uid (key to node/attr map)
	int32_t		m_Start ;		//	First attribute for the tag in the map
	int32_t		m_Final ;		//	Last attribute for the tag in the map
	int32_t		m_Current ;		//	Current attribute for the tag in the map

	//	Prevent copies
	hzAttrset	(const hzAttrset&) ;
	hzAttrset&	operator=	(const hzAttrset&) ;

public:
	hzAttrset		(void)
	{
		//m_pHostNode = 0 ;
		m_pHostDoc = 0 ;
		m_NodeUid = 0 ;
		m_Current = m_Start = m_Final = -1 ;
		m_Pair.m_snName = m_Pair.m_snValue = 0 ;
	}

	~hzAttrset	(void)	{}

	//	Set the attribute iterator to the start of the attributes for the XML node
	hzAttrset&	operator=	(hzXmlNode* pNode) ;
	hzAttrset&	operator=	(hzHtmElem* pElem) ;

	bool	Valid	(void) const	{ return m_pHostDoc && m_Pair.m_snName ? true : false ; }

	bool		NameEQ	(const char* cstr) const ;
	bool		ValEQ	(const char* cstr) const ;
	void		Advance	(void) ;
	const char*	Name	(void) const ;
	const char*	Value	(void) const ;
} ;

#define	XMLESCE_OFF		0	//	Upon load, assume all tags are XML even if they are known HTML tags.
#define	XMLESCE_ON		1	//	Upon load, treat all HTML subtags found within a non-HTML tag as part of the non-HTML tag's content
#define	XMLESCE_MIX		2	//	Upon load, where tag content text is mixed with subtags, treat the sections of text as nodes in their own right.

class	hzDocXml	: public hzDocument
{
	//	Category:	Document
	//
	//	The XML tree. This is usually populated by reading a single XML file

	hzSet<hzString>	m_Xmlesce ;	//	List of tags whose nodes are permitted to contain HTML data. The node may still contain normal XML subnodes but any tag with
								//	a name that is a legal HTML tag - will be treated as HTML.

	hzDocCtrl	m_Doctype ;		//	The doctype
	hzXmlNode*	m_pRoot ;		//	All tags found on level 0
	hzString	m_Filename ;	//	Full path of loaded file (optional)
	uint32_t	m_FileEpoch ;	//	Modified time of loaded file (optional)
	uint32_t	m_bXmlesce ;	//	Treat all legal HTML tag/antitags as part of node content

	//	Support functions
	int32_t		_proctagopen	(hzXmlNode** ppChild, hzXmlNode* pParent, hzChain::Iter& ci) ;

public:
	hzMapM	<uint32_t,uint32_t>		m_NodesName ;	//	1 to many string numbers (name) to node (uid)
	hzMapM	<uint32_t,uint32_t>		m_NodesPar ;	//	1 to many parent nodes (node uid) to child nodes (node uid)
	hzArray	<hzXmlNode>				m_arrNodes ;	//	All nodes within document
	
	hzDocXml	(void) ;
	~hzDocXml	(void) ;

	hzDoctype	Whatami		(void) const	{ return DOCTYPE_XML ; }
	hzXmlNode*	GetRoot		(void) const	{ return m_pRoot ; }
	hzString	Filename	(void) const	{ return m_Filename ; }
	const char*	Fname		(void) const	{ return *m_Filename ; }

	void		AddXmlesce	(hzString& tagname)	{ m_Xmlesce.Insert(tagname) ; } 
	void		SetXmlesce	(bool bXmlesce)		{ m_bXmlesce = bXmlesce ; }

	//	Load and Export
	hzEcode		Load		(hzChain& Z) ;
	hzEcode		Load		(const char* cpFilename) ;
	hzEcode		Export		(hzChain& e) ;
	hzEcode		Export		(const hzString& filepath) ;
	void		Clear		(void) ;
	uint32_t	InsertStr	(const char* cstr)		{ return m_Dict.Insert(cstr) ; }

	//	Naviagation
	void		listnodes	(void) ;
	hzEcode		FindNodes	(hzVect<hzXmlNode*>& Nodes, const char* srchExp) ;

	hzString	GetValue	(hzXmlNode* pBasenode, hzString& Nodename, hzString& Info) ;
} ;

class	hzXmlSlct
{
	//	Category:	Document
	//
	//	XML Selector - used to extract information from an XML document, such as the set of links in an RSS Feed page. The assumption being that the XML will be of a known form and
	//	that the information of interest will be found in known nodes (tags) - usually the case with subscription data feeds. hzXmlSlct comprises selection criteria to locate nodes
	//	of interest, together with a method which directs the extraction process.
	//
	//	Node location can be on node name alone, but only if the nodes of interest use a node name that is not used within the document for any other purpose. As this is not always
	//	the case it is often necessary to provide further qualification. This can be done by specifying node ancestry, by requiring particular attributes and if need be, requiring
	//	the attributes to have particular values. In the notation the $ sign is used to separate node names, the -> symbol to state a required attribute and the = sign to specify a
	//	value. Location criteria are thus of the form:-
	//
	//		[acestor1$]...[ancestorN$]node_name[->attr1[=val1]]...[->attrN[=valN]]
	//
	//	The information of interest is usually the content of the node(s) of interest, but it in rare cases it can lay in the attributes. Accordingly, the method of extraction will
	//	either be "node" to extract node content or a series of "->attr" to name the attribute(s), or it can be both.
	//
	//	The string values extracted will in the case of multiple nodes, be aggregated.

	//	Category:	Document
	//
	//	The hzXmlSlct or 'XML selector' class is a configuration device, used to direct extraction of information from XML documents. Instead of hard coding how
	//	an XML document is read (the approach always taken when reading config files themselves), a set of one or more XML selectors defined within the program
	//	configs are applied to acheive the same. Both apporaches have merit. The hard coding is intentionally rigid and will typically invalidate documents that
	//	contain superfluous nodes or attributes. The set of XML selectors will only look for nodes of interest and ignore the rest.
	//
	//	Either way the extraction process is a matter of looking for nodes matching particular criteria and pulling the node content and/or values of particular
	//	node attributes. The XML selector sets out the following:-  
	//
	//		1)	What the node must have:
	//		
	//			a)	The node name. The node must match on node name.
	//			b)	Expected attribtes. If any then the node must have all attributes listed.
	//			c)	Expecte attribute values. If a value as assigned to an expected attribute, then the node must have that attribute with that value.
	//			d)	Particular ancestry:
	//			e)	Particular subnodes: ...
	//
	//		2)	What the node must not have:
	//
	//			a)	Unexpected attributes: If the node has any of the attributes listed, it is excluded
	//			b)	Unexpected attribute values. If the node has any of the attributes listed and the values they are listed with, it is excluded.
	//
	//		3)	Method of extraction:
	//
	//			a)	Node: This meaans the content of the node
	//			b)	Attr: This is written as "->attr_name" and will return the value of the attrubute if the node has it
	//
	//	Note that where XML nodes may have content mixed with subnodes, particular care should be taken when specifying the extraction method. The HadronZoo XML
	//	parser assigns node content to nodes in two ways. Content appearing BEFORE the closing tag but AFTER ANY opening tag is treated as content. However, any
	//	content appearing AFTER the opening tag but BEFORE an opening sub-tag is not. Instead it is assigned as pre-text to the sub-tag. For example:-
	//
	//		<tagA>blurb about A<tagB>blurb about B</tagB>more blurb about A</tagA>
	//
	//	The content of the node named tagB is "blurb about B" as expected but the content of the node named tagA is just "more blurb about A". The first part of
	//	the tagA content has gone missing. It can only be found as the pre-text of the subnode.

public:
	hzString	m_Slct ;	//	Criteria needed to locate node
	hzString	m_Info ;	//	How to obtain data from the node (eg ->'Date' meaning value of an attr called 'Date')
	hzString	m_Filt ;	//	Filter to be applied to the extracted data.

	hzXmlSlct	(void)
	{
	}

	bool	IsValid	(void)	{ return (!m_Slct || !m_Info)? false : true ; }
	bool	IsNull	(void)	{ return (!m_Slct && !m_Info)? true : false ; }
	bool	IsPart	(void)	{ return (m_Slct || m_Info)? true : false ; }

	hzXmlSlct&	operator=	(const hzXmlSlct& op)
	{
		m_Slct = op.m_Slct ;
		m_Info = op.m_Info ;
		m_Filt = op.m_Filt ;
		return *this ;
	}
} ;

/*
**	Prototypes
*/

hzEcode		InitHtml		(void) ;

hzHtagInd	AtHtmlTag		(hzString& tag, hzChain::Iter& ci) ;
void		XmlCleanHtags	(hzChain& output, const hzChain& input) ;

hzDoctype	DeriveDoctype	(hzChain& Z) ;
const char*	Doctype2Txt		(hzDoctype) ;
hzString	Tagtype2Txt		(hzHtagtype type) ;
hzHtagtype	Txt2Tagtype		(const hzString& tagtype) ;

const hzHtagform&	TagLookup	(const hzString& htag) ;
const hzHtagform&	TagLookup	(hzChain::Iter& ci) ;

#endif	//	hzDocument_h
