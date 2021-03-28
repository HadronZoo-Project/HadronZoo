//
//	File:	hzDissemino.h
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

#ifndef hzDissemino_h
#define hzDissemino_h

#include <sys/stat.h>

#include "hzChars.h"
#include "hzChain.h"
#include "hzCtmpls.h"
#include "hzCodec.h"
#include "hzDate.h"
#include "hzFsTbl.h"
#include "hzDirectory.h"
#include "hzDatabase.h"
#include "hzDocument.h"
#include "hzTextproc.h"
#include "hzHttpServer.h"
#include "hzMailer.h"
#include "hzProcess.h"

/*
**	Definitions
*/

//	Access control flags
#define ACCESS_PUBLIC	0x00000000	//	Access by anyone (login status ignored)
#define ACCESS_NOBODY	0x20000000	//	Access by anyone who is NOT logged in
#define ACCESS_ADMIN	0x40000000	//	Access by site administrators only
#define ACCESS_MASTER	0x80000000	//	Access by web-master only
#define ACCESS_MASK		0x1fffffff	//	Bits 0 - 29 reserved for up to 30 types of users

//	Null values
#define DS_NULL_GRAPH	0x80000000	//	Null graph value
#define DS_NULL_COLOR	0x80000000	//	Null color
#define DS_NULL_COORD	0x8000		//	Null coordinate

//	Subscriber repostiory members
#define SUBSCRIBER_USERNAME	0		//	Unique unsername for all users
#define SUBSCRIBER_EMAIL	1		//	Unique primary email address for user account
#define SUBSCRIBER_PASSWORD	2		//	Password for user account
#define SUBSCRIBER_USER_UID	3		//	Unique user id (oject id in subscriber repository)
#define SUBSCRIBER_USERADDR	4		//	Address of user account (object id in the user repository)
#define SUBSCRIBER_USERTYPE	5		//	The user repository for the user

/*
**	HTML generation
*/

enum	hzHtmltype
{
	//	Category:	Dissemino HTML Generation
	//
	//	Html types apply only to the rendering. The only constraint is that the Htnl type can accomodate the filed's data type.

	HTMLTYPE_NULL,			//	Initial (invalid) value
	HTMLTYPE_TEXT,			//	Full range of printable chars
	HTMLTYPE_PASSWORD,		//	As text but char's won't print
	HTMLTYPE_TEXTAREA,		//	As text but a text area is described
	HTMLTYPE_SELECT,		//	A HTML selector
	HTMLTYPE_CHECKBOX,		//	A HTML checkbox
	HTMLTYPE_RADIO,			//	A HTML radio button set
	HTMLTYPE_FILE,			//	File uploaded
	HTMLTYPE_HIDDEN,		//	Hidden field
} ;

const char* Htmltype2Txt	(hzHtmltype ht) ;

class	hdsFldspec
{
	//	Category:	Dissemino Config
	//
	//	One golden rule is that a database should not know or care how the data it stores and retrieves is presented or collected. However as a development aid, Dissemino generates
	//	default forms and form handlers directly from data class definitions. From the perspective of the database, each data class member need only a name, a data type, a flag to
	//	state if a value is compulsory and another flag to state if multiple values are allowed. By contrast, for a member to appear as a field in a HTML form, other parameters are
	//	required such as the following:-
	//
	//		1)	An optional description.
	//		2)	Optional validation Javascript.
	//		3)	Max number of chars that may be entered
	//		4)	Visible width in chars
	//		5)	Number of rows (only applies to textarea and select fields)
	//		6)	HTML type (eg Text, Select)
	//
	//	The hdsFldspec 'field specification' class is intended as an innocuous means of associating HTML parameters with a data class member. hdsFldspec instances are created in
	//	response to <fldspec> tags in the configs. These must be defined before being used as part of data member declarations in a data class definition. 

public:
	const hdbDatatype*	m_pType ;	//	Datatype

	hzString	m_Refname ;		//	Ref name
	hzString	m_Title ;		//	Title (of data entry field)
	hzString	m_Desc ;		//	Description (often presented as popup on mouseover event)
	hzString	m_Default ;		//	Default value
	hzString	m_Tab ;			//	Page/form tab (for auto forms)
	hzString	valJS ;			//	Javascript to validate data
	hzString	m_Source ;		//	If the form percent entity referers to a different class/memeber
	uint32_t	nSize ;			//	Max number of chars that may be entered
	uint16_t	nCols ;			//	Visible width in chars
	uint16_t	nRows ;			//	Only applies to textarea and select fields
	uint16_t	nFldSeq ;		//	Field display order
	uint16_t	nExpSeq ;		//	Data export order
	hzHtmltype	htype ;			//	HTML type (eg Text, Select)

	hdsFldspec	(void) ;
	~hdsFldspec	(void) ;

	hdsFldspec&	operator=	(const hdsFldspec& op) ;

	hzEcode		Validate	(hzLogger* pLog, const hzString& cfg, const char* caller, uint32_t ln) ;
} ;

//	ClassGroup:	Dissemino Language Support
//
//	Category:	Dissemino Config
//
//	Class:	hdsUSL
//	Class:	hdsLang
//
//	To properly represent a website in multiple languages, there needs to be a different version of each page or article for each language and these need to be writen by competent
//	interpreters. However, where the resources for such an approach are not available, there are always translation programs! Dissemino language support is aimed at supporting the
//	latter without undermining the former.
//
//	Although not entirely satisfactory in all cases, for the most part strings in an original language can be directly mapped to strings in another. What consitutes a paragraph in
//	a webpage in the original language is likely to consitute a paragraph in a target language. The shorter the strings, the better the correlation. With page titles and nav-tree
//	heading for example, there is less to go wrong.
//
//	Translation programs usually leave alphanumeric sequences unchanged, so something like "p1.1.4 Hello World" through an English to German translator is returned as "p1.1.4 Hallo
//	Welt". So Dissemino assigns each string occuring anywhere in the website, a USL (Universal String Lable), which is an id of this alphanumeric and hierarchical form. During HTML
//	generation, the USL is used to look up the actual string value in a map in an instance of hdsLang - for which there is one for every language the website supports.
//
//	This arrangement allows whole sets of strings to be mass exported, sent for translation (by program or human) and then re-imported. It is not automated but automation would not
//	be a difficult step. For the purpose of HTML generation, only a 32-bit number is required. However by building in notational hierarchy, USLs make it easier to translate page by
//	page and are a more effective aid to diagnostics. The entities covered by USLs are visual entities and containers of visual entities, such as include blocks, pages and articles
//	but also trees.
//
//	In the case of a visual entity container, the text form of a USL will be a letter to indicate container type and a number. USLs are run time ids based on order of instantiation
//	in the configs. So the first page declared in the configs has a USL of p1. Include blocks are given a 'b' prefix so the first include block declared in the configs has a USL of
//	b1. The visual entities within the containers have USLs of the form "container-USL.N' where N is the order in which the visual entity appears in the container definition in the
//	configs. The first visual entity in the first page will thus be p1.1. The most complex USL are those of visual entities within articles that are members of an article group. An
//	article has a prefix of 'a' so a visual entity in a lone article would have a USL of the form aX.Y. A visual entity in a article in a group would have a USL of the form gX.Y.Z.
//	Note only one prefix is used as it is obvious the second number indicated the article.
//
//	Because there won't be too many containers and generally very few visual entities within a container, USLs are encoded into 32-bit numbers to save space.

#define	USL_G_MARK	0x80000000		//	USL is that of an article group
#define	USL_P_MARK	0x04000000		//	USL is that of a page
#define	USL_S_MARK	0x02000000		//	USL is that of a page subject
#define	USL_B_MARK	0x01000000		//	USL is that of an include block

#define USL_G_MASK	0x78000000		//	Bits 01 thru 04 inclusive, identify article group (if applicable)
#define USL_C_MASK	0x07ffc000		//	Bits 05 thru 17 inclusive, identify visual entity container
#define USL_V_MASK	0x00003ffe		//	Bits 18 thru 30 inclusive, identify the visual entity within a container
#define USL_X_MASK	0x00000001		//	Bit 31 indicates the visual entity as pretext, rather then content

class	hdsUSL
{
	//	Dissemino Universal String Label
	//
	//	hdsUSL exists primarily for language support which is implemented by 

	uint32_t	m_Value ;
public:
	hdsUSL	(void)	{ m_Value = 0 ; }
	~hdsUSL	(void)	{}

	void	Clear	(void)	{ m_Value = 0 ; }

	hzEcode	SetBlock		(uint32_t blkNo) ;
	hzEcode	SetGroup		(uint32_t grpNo) ;
	hzEcode	SetArticle		(const hdsUSL base, uint32_t artNo) ;
	hzEcode	SetPage			(uint32_t pageNo) ;
	hzEcode	SetSubj			(uint32_t subjNo) ;

	hzEcode	SetBlockVE		(uint32_t veNo) ;
	hzEcode	SetPageVE		(uint32_t veNo) ;
	hzEcode	SetVE_Pretext	(const hdsUSL base, uint32_t veNo) ;
	hzEcode	SetVE_Content	(const hdsUSL base, uint32_t veNo) ;
	hzEcode	SetByText		(const char* usl) ;

	bool	operator<	(const hdsUSL& op) const	{ return (m_Value < op.m_Value) ; }
	bool	operator>	(const hdsUSL& op) const	{ return (m_Value > op.m_Value) ; }

	operator uint32_t	(void) const	{ return m_Value ; }
	operator bool   	(void) const	{ return m_Value ? true : false ; }

	char*	Txt	(hzRecep32& r) const ;
} ;

class	hdsLang
{
	//	Category:	Dissemino Config
	//

public:
	hzMapS	<hdsUSL,hzString>	m_LangStrings ;		//	String reference code to string content
	hzMapS	<hdsUSL,hzString> 	m_rawItems ;		//	Fixed (non active) HTML from pages, articles and blocks (unzipped)
	hzMapS	<hdsUSL,hzString> 	m_zipItems ;		//	Fixed (non active) HTML from pages, articles and blocks (zipped)

	hzMapS	<hzString,hzString>	m_rawScripts ;		//	Scripts
	hzMapS	<hzString,hzString>	m_zipScripts ;		//	Scripts zipped

	hzString	m_code ;	//	Language code
	hzString	m_flag ;	//	URL of the flag image
	hzString	m_name ;	//	Name of language (in the default language)
	hzString	m_natv ;	//	Name of language (in the target language)

	hdsLang	(void)	{}
} ;

/*
**	Visual Entities
*/

enum	xTag
{
	//	Category:	Dissemino Config
	//
	//	HTML visible entity types

	HZ_VISENT_NULL,			//	Undefined
	HZ_VISENT_FIELD,		//	The entity is a field
	HZ_VISENT_FORM,			//	The entity is a form (containing fields)
	HZ_VISENT_BUTTON,		//	The entity is a button
	HZ_VISENT_RECAP,		//	The entity is a human test icon (google recaptcha)
	HZ_VISENT_LOOP,			//	The entity is an loop controller
	HZ_VISENT_HTAG,			//	Other HTML tag not serving as an active entity
	HZ_VISENT_XTAG,			//	Language support tag
	HZ_VISENT_HBLOCK,		//	Block of HTML tags not serving as an active entity
	HZ_VISENT_ARTREF,		//	Block of CDATA HTML tags not serving as an active entity
	HZ_VISENT_XDIV,			//	Block of HTML tags depending on who is logged in
	HZ_VISENT_XCOND,		//	Block of HTML tags depending on a variable's value
	HZ_VISENT_TXTVAL,		//	Block of text forming part of the value of a node
	HZ_VISENT_NAVBAR,		//	Navigation pull-down menu
	HZ_VISENT_PIECHART,		//	The entity is a pie chart
	HZ_VISENT_BARCHART,		//	The entity is a bar chart
	HZ_VISENT_STDCHART,		//	The entity is a std chart
	HZ_VISENT_DIAGRAM,		//	The entity is a chart
} ;

//	Operational flags for hdsVE class
#define	VE_COMPLETE		0x0001	//	No more subtags can be added
#define	VE_EVALUATED	0x0002	//	No more subtags can be added
#define VE_LANG			0x0004	//	Language support on (text-bearing tags only)
#define VE_AT_ACTIVE	0x0008	//	Has an active value in the tag attributes
#define VE_CT_ACTIVE	0x0010	//	Has an active value in the tag contents
#define VE_PT_ACTIVE	0x0020	//	Has an active value in the tag pretext
#define VE_MULTIPLE		0x0040	//	Where applicable (eg <select>), values can be multiple
#define VE_DISABLED		0x0080	//	Tag will appear in page as greyed out
#define	VE_MULTIPART	0x0100	//	Set only in hdsForm and only where the form accepts a file upload
#define	VE_COOKIES		0x0200	//	Set only in hdsForm and will issue a session (and so a cookie) upon form submission
#define VE_COMPULSORY	0x0400	//	Set only in hdsField to indicate the field is compulsory
#define VE_UNIQUE		0x0800	//	Set only in hdsField to indicate the field (eg an email address) must be unique

#define VE_ACTIVE	(VE_AT_ACTIVE | VE_CT_ACTIVE | VE_PT_ACTIVE)

#define	DS_INH_FORM		0x01	//	The VE is a form or a subtag of a form, so this VE or any of its children cannot be a form
#define	DS_INH_TEXT		0x02	//	The VE is text-bearing and thus may only have children that cannot themselves have children.
#define	DS_INH_NOCHILD	0x04	//	The VE may not have children.
#define	DS_INH_SERIES	0x08	//	The VE has no text content and must be comprised of a series of children (e.g. <tr>)
#define	DS_INH_HYBRID	0x10	//	The VE may have text content (childless children allowed) OR be comprised of a series of children (e.g. <td>)

class	hdsApp ;

class	hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	The hdsVE (visible entity) class gives rise to HTML output. To be clear, the term visible does not mean that a hdsVE instance will always be manifest as
	//	a visible object when rendered by the browser. It strictly means that HTML tags will be generated in the output. hdsVE has numerous derivatives, some of
	//	which are directly associated with a HTML tag. Others are special Dissemino tags that result in one or more HTML tags.
	//
	//	For HTML generated by Dissemino strict rules apply. In particular, if a tag is allowed to have text content, the only subtags permitted may also contain
	//	text but no further subtags. The content of a tag can be devoid of subtags in which case it can be evaluated directly. But where subtags are included in
	//	the content (such as a link), the content is said to be 'composite'.
	//
	//	Such composite content is constructed by means of a pre-text value on subtags. Such content is then evaluated by visiting each subtag in turn and first
	//	evaluating and adding the pre-text value and then eveluating and adding the content. Only when there are no more subtags to visit are the actual contents
	//	of the tag in hand evaluated and added to the final string.
	//	                    
	//	It thus stands to reason that a text-bearing tag may either have content and children but no pre-text value OR it may have content and a pre-text value
	//	but no children!

	uint32_t	m_Children ;	//	Sub nodes of this node
	uint32_t	m_Sibling ;		//	Next node (in the series m_Chridren belonging to the parent of this)

	//	Prevent copies
	hdsVE	(const hdsVE&) ;
	hdsVE&	operator=	(const hdsVE&) ;

public:
	hdsApp*		m_pApp ;		//	Parent Dissemino Application

	hzString	m_strPretext ;	//	If the parent is complex, this text is to be displayed as part of the parent content
	hzString	m_strContent ;	//	Tag content (value)
	hdsUSL		m_usiPretext ;	//	The unique string id for language support
	hdsUSL		m_usiContent ;	//	The unique string id for language support
	hzString	m_Tag ;			//	The HTML or Dissemino tag
	hzString	m_CSS ;			//	Style-sheet class (if set this will be default for the table's <td> tags)

	uint32_t	m_VID ;			//	Absolute unique VE identifier
	uint32_t	m_Access ;		//	Access control etc
	uint32_t	m_flagVE ;		//	Operational flags
	uint32_t	m_BgColor ;		//	Background color (if not set use table value)
	uint32_t	m_FgColor ;		//	Foreground color (if not set use table value)
	uint32_t	m_Line ;		//	Line number of opening tag (drawn from the line number of the associated XML node)
	uint16_t	m_nAttrs ;		//	Number of attributes
	uint16_t	m_nChildren ;	//	Number of chidren
	uint16_t	m_Indent ;		//	Indentation
	uint16_t	m_Resv ;		//	???

	hdsVE	(void) ;

	virtual	~hdsVE	(void)	{ m_pApp = 0 ; }

	//	Set functions
	void	SetCSS		(const char* cpCSS)	{ m_CSS = cpCSS ; }
	void	Complete	(void)	{ m_flagVE |= VE_COMPLETE ; }

	hzEcode	InitVE		(hdsApp* pApp) ;
	hzEcode	SetSibling	(hdsVE* pSibling) ;
	hzEcode	AddChild	(hdsVE* pChild) ;
	hzEcode	AddAttr		(const hzString& name, const hzString& value) ;

	//	Get functions
	hdsVE*	Children	(void) const ;
	hdsVE*	Sibling		(void) const ;

	//	Virtual functions
	virtual	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) = 0 ;
	virtual xTag	Whatami		(void) = 0 ;
} ;

class	hdsField	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	A hdsField instance is created for each <xfield> tag and serves as a field in a form (created by <xform> tag). The <xfield> tag must name either a free
	//	standing variable or a class member as without this, Dissemino would not know how to name the field in the form's HTML or what to do with any submitted
	//	value.
	//
	//	When HTML is generated and fields are include in the output, they may contain a pre-populated value by means of a percent entity. The value is obtained
	//	DURING the HTML generation process. Where the field relates to a class member, the lookup for the value must have the repository class, the class member
	//	and the object id. By default, the class will be that of the host form unless otherwise stated in the 'var' attribute of the <xfield> tag. The object id
	//	is not part of the hdsField class or the <xfield> tag and is supplied as the argument to the host page.

public:
	const hdbClass*		m_pClass ;	//	That of the host form unless otherwise stated
	const hdbMember*	m_pMem ;	//	Must be member of the class (m_pClass) or null

	hdsFldspec		m_Fldspec ;		//	Data & HTML type, size etc
	hzString		m_Varname ;		//	Name of field as it appears in the form and in any subsequent submission
	hzString		m_Source ;		//	Source data (drives pre-population)

	hdsField	(hdsApp* pApp) ;
	~hdsField	(void) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami		(void)	{ return HZ_VISENT_FIELD ; }
} ;

/*
**	Graphics
*/

class	hdsText
{
	//	Category:	Dissemino Graphics
	//
	//	This supplies text components to Dissemino graphics objects

public:
	hzString	text ;			//	diagram component associated text
	hzString	font ;			//	Font (default is diagram main font)
	uint32_t	linecolor ;		//	Line color
	uint16_t	ypos ;			//	West point x-coord (lft, midway between bot and top)
	uint16_t	xpos ;			//	West point x-coord (lft, midway between bot and top)
	uint16_t	alignCode ;		//	0 is left, 1 is center and 2 is right
	uint16_t	resv1 ;			//	Reserved
	uint16_t	resv2 ;			//	Reserved
	uint16_t	resv3 ;			//	Reserved

	hdsText	(void) ;

	void	Clear	(void) ;
	hdsText	operator=	(const hdsText& op) ;
} ;

/*
**	Canvas Support
*/

class	hdsPieChart	: public hdsVE
{
	//	Category:	Dissemino Graphics
	//
	//	Dissemino 'pie' chart.

public:
	struct	_pie
	{
		//	Pie chart component

		hzString	header ;		//	Data set title
		uint32_t	value ;			//	Single value
		uint32_t	color ;			//	Color for data set

		_pie	(void)	{ color = DS_NULL_COLOR ; value = 0 ; }
	} ;

	hzList	<hdsText>	m_Texts ;	//	Text items
	hzList	<_pie*>		m_Parts ;	//	Parts of the pie chart

	double		m_Total ;		//	Total value
	hzString	m_Id ;			//	Unique document element id
	hzString	m_Font ;		//	Main text font
	uint32_t	m_fillColor ;	//	Fill color
	uint32_t	m_lineColor ;	//	Line color
	uint16_t	m_Height ;		//	Total hieght of canvas
	uint16_t	m_Width ;		//	Total width of canvas
	uint16_t	m_ccY ;			//	Circle center y-coord
	uint16_t	m_ccX ;			//	Circle center x-coord
	uint16_t	m_Rad ;			//	Radius
	uint16_t	m_IdxY ;		//	Index top left y-coord
	uint16_t	m_IdxX ;		//	Index top left x-coord
	uint16_t	m_IdxW ;		//	Index width

	hdsPieChart		(hdsApp* pApp) ;
	~hdsPieChart	(void) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami		(void)	{ return HZ_VISENT_PIECHART ; }
} ;

class	hdsBarChart	: public hdsVE
{
	//	Category:	Dissemino Graphics
	//
	//	Dissemino bar graph.
	//
	//	Note: With a bar graph, there is a bar for each marker on the y-axis and the first set of values in the m_vals array, are the y-axis marker values. For
	//	a line graph, the m_vals array are pairs of y,x values.

public:
	class	_bset
	{
		//	Control parameters for a single bar chart data set

	public:
		hzString	header ;		//	Data set title
		uint32_t	color ;			//	Color for data set
		uint16_t	start ;			//	Offset into vals
		uint16_t	popl ;			//	Number of values

		_bset	(void)	{ color = 0 ; start = popl = 0 ; }
	} ;

	hzList	<hdsText>	m_Texts ;	//	Text items
	hzArray	<double>	m_vals ;	//	For bar charts Y values are pre-defined so only X values here
	hzList	<_bset*>	m_Sets ;	//	Datasets

	double		m_ratX ;			//	Ratio of x-value per pixel
	double		m_ratY ;			//	Ratio of y-value per pixel

	double		m_minX ;			//	Minimum X-axis value
	double		m_maxX ;			//	Maximum X-axis value
	double		m_minY ;			//	Minimum Y-axis value
	double		m_maxY ;			//	Maximum Y-axis value
	double		m_xdiv ;			//	Divider x-axis
	double		m_ydiv ;			//	Divider y-axis

	hzString	m_Id ;				//	Unique document element id
	hzString	m_Font ;			//	Main text font
	hzString	m_HdrX ;			//  X-axis heading
	hzString	m_HdrY ;			//  Y-axis heading
	uint32_t	m_fillColor ;		//	Fill color
	uint32_t	m_lineColor ;		//	Line color
	uint16_t	m_Height ;			//	Total hieght of canvas
	uint16_t	m_Width ;			//	Total width of canvas
	uint16_t*	m_xpad ;			//	Scratch pad for x-values (bar charts only)
	uint16_t	m_yLft ;			//  Lft y coord of y-axix (y coord of x axis)
	uint16_t	m_yRht ;			//  Rht y coord of y-axis
	uint16_t	m_xTop ;			//  Top x coord of x-axix
	uint16_t	m_xBot ;			//  Bot x coord of x-axix (x coord of y axis)

	uint16_t	m_marginLft ;		//	Left margin (default 20)
	uint16_t	m_marginRht ;		//	Left margin (default 20)
	uint16_t	m_marginTop ;		//	Left margin (default 15)
	uint16_t	m_marginBot ;		//	Left margin (default 15)
	uint16_t	m_xpix ;			//	Number of pixels on x-axis
	uint16_t	m_ypix ;			//	Number of pixels on y-axis
	uint16_t	m_gap ;				//	Gap between the bars (default 5)
	uint16_t	m_slotw ;			//	Slot width (y-axis)
	uint16_t	m_ypop ;			//	Number of y-axis values
	uint16_t	m_IdxY ;			//	Index top left y-coord
	uint16_t	m_IdxX ;			//	Index top left x-coord
	uint16_t	m_IdxW ;			//	Index width

	hdsBarChart		(hdsApp* pApp) ;
	~hdsBarChart	(void) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;

	xTag	Whatami	(void)	{ return HZ_VISENT_STDCHART ; }
} ;

class	hdsStdChart	: public hdsVE
{
	//	Category:	Dissemino Graphics
	//
	//	Dissemino standard (line) graph.

public:
	class	_rset
	{
		//	Line chart dataset

	public:
		hzString	header ;		//	Data set title
		uint32_t	color ;			//	Color for data set
		uint16_t	start ;			//	Offset into vals
		uint16_t	popl ;			//	Number of values

		_rset	(void)	{ color = 0 ; start = popl = 0 ; }
	} ;
		
	hzList	<hdsText>	m_Texts ;	//	Text items
	hzArray	<double>	m_vals ;	//	Y,X plot values (in pairs)
	hzList	<_rset*>	m_Sets ;	//	Datasets

	double		m_ratX ;			//	Ratio of x-value per pixel
	double		m_ratY ;			//	Ratio of y-value per pixel
	double		m_minX ;			//	Minimum X-axis value
	double		m_maxX ;			//	Maximum X-axis value
	double		m_minY ;			//	Minimum Y-axis value
	double		m_maxY ;			//	Maximum Y-axis value
	double		m_xdiv ;			//	Divider x-axis
	double		m_ydiv ;			//	Divider y-axis
	hzString	m_Id ;				//	Unique document element id
	hzString	m_Font ;			//	Main text font
	hzString	m_HdrX ;			//  X-axis heading
	hzString	m_HdrY ;			//  Y-axis heading
	uint32_t	m_fillColor ;		//	Fill color
	uint32_t	m_lineColor ;		//	Line color
	uint16_t	m_Height ;			//	Total hieght of canvas
	uint16_t	m_Width ;			//	Total width of canvas
	uint16_t*	m_xpad ;			//	Scratch pad for x-values (bar charts only)
	uint16_t	m_yLft ;			//  Lft y coord of y-axix (y coord of x axis)
	uint16_t	m_yRht ;			//  Rht y coord of y-axis
	uint16_t	m_xTop ;			//  Top x coord of x-axix
	uint16_t	m_xBot ;			//  Bot x coord of x-axix (x coord of y axis)
	uint16_t	m_marginLft ;		//	Left margin (default 20)
	uint16_t	m_marginRht ;		//	Left margin (default 20)
	uint16_t	m_marginTop ;		//	Left margin (default 15)
	uint16_t	m_marginBot ;		//	Left margin (default 15)
	uint16_t	m_flags ;			//	Range as double
	uint16_t	m_xpix ;			//	Number of pixels on x-axis
	uint16_t	m_ypix ;			//	Number of pixels on y-axis
	uint16_t	m_ypop ;			//	Number of y-axis values
	uint16_t	m_IdxY ;			//	Index top left y-coord
	uint16_t	m_IdxX ;			//	Index top left x-coord
	uint16_t	m_IdxW ;			//	Index width

	hdsStdChart		(hdsApp* pApp) ;
	~hdsStdChart	(void) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami	(void)	{ return HZ_VISENT_STDCHART ; }
} ;

enum	hdsShape
{
	//	Category:	Dissemino Graphics
	//
	//	Defines basic shapes for diagram components

	HZSHAPE_NULL,			//	No shape defined
	HZSHAPE_LINE,			//	Line from y1,x1 to y2,x2
	HZSHAPE_DIAMOND,		//	Comprises four lines where bot-top = rht-lft
	HZSHAPE_ARROW,			//	connecting line plus arrow head
	HZSHAPE_RECT,			//	Rectangle
	HZSHAPE_RRECT,			//	Rounded rectangle
	HZSHAPE_STADIUM,		//	Two semi-circles joined by two paralell lines
	HZSHAPE_CIRCLE,			//	Circle (center y,x) of radius r
	HZSHAPE_LGATE_AND,		//	AND gate
	HZSHAPE_LGATE_OR,		//	OR gate
	HZSHAPE_LGATE_NOT,		//	NOT gate (arrow head)
	HZSHAPE_LGATE_NAND,		//	NAND gate
	HZSHAPE_LGATE_NOR,		//	NOR gate
	HZSHAPE_CONNECT,		//	Shape connector
} ;

class	hdsGraphic
{
	//	Category:	Dissemino Graphics
	//
	//	A Dissemino diagram shape.

public:
	hzString	uid ;		//	diagram component unique id (within diagram)
	hzString	text ;		//	diagram component associated text
	hdsShape	type ;		//	Type of shape
	uint32_t	fillcolor ;	//	Fill color
	uint32_t	linecolor ;	//	Line color
	uint16_t	nid ;		//	Reflects position within array of shapes
	uint16_t	top ;		//	Top coord within display window, of chart rectangle
	uint16_t	bot ;		//	Top coord within display window, of chart rectangle
	uint16_t	lft ;		//	Top coord within display window, of chart rectangle
	uint16_t	rht ;		//	Top coord within display window, of chart rectangle
	uint16_t	rad ;		//	Radius where applicable
	uint16_t	thick ;		//	Line thickness
	uint16_t	width ;		//	Rht-Lft
	uint16_t	height ;	//	Bot-Top
	uint16_t	northY ;	//	North point y-coord (top, midmay between lft & rht)
	uint16_t	northX ;	//	North point x-coord (top, midmay between lft & rht)
	uint16_t	eastY ;		//	East point y-coord (rht, midway between bot and top)
	uint16_t	eastX ;		//	East point x-coord (rht, midway between bot and top)
	uint16_t	southY ;	//	South point y-coord (bot, midmay between lft & rht)
	uint16_t	southX ;	//	South point x-coord (bot, midmay between lft & rht)
	uint16_t	westY ;		//	West point y-coord (lft, midway between bot and top)
	uint16_t	westX ;		//	West point x-coord (lft, midway between bot and top)
	uint16_t	text_y ;	//	West point x-coord (lft, midway between bot and top)
	uint16_t	text_x ;	//	West point x-coord (lft, midway between bot and top)
	uint16_t	from ;		//	For connectors (numeric id for source shape)
	uint16_t	to ;		//	For connectors (numeric id for source shape)
	uint16_t	head ;		//	For arrows and connectors only

	hdsGraphic	(void) ;
} ;

class	hdsLine
{
	//	Category:	Dissemino Graphics
	//
	//	A Dissemino diagram line.

public:
	uint32_t	linecolor ;		//	Line color
	uint16_t	y1 ;			//	Start of line y-coord
	uint16_t	x1 ;			//	Start of line x-coord
	uint16_t	y2 ;			//	End of line y-coord
	uint16_t	x2 ;			//	End of line x-coord
	uint16_t	thickness ;		//	Line thickness
	uint16_t	style ;			//	Eg dotted

	hdsLine	(void)	{ linecolor = 0 ; thickness = 1 ; style = 0 ; y1 = x1 = y2 = x2 = DS_NULL_COORD ; }

	void	Clear	(void)	{ linecolor = 0 ; thickness = 1 ; style = 0 ; y1 = x1 = y2 = x2 = DS_NULL_COORD ; }
} ;

class	hdsDiagram	: public hdsVE
{
	//	Category:	Dissemino Graphics
	//
	//	Dissemino diagrams utilize the HTML5 canvas tag and are created as a set of shapes, lines and items of text. Dissemino converts these to a canvas tag
	//	and an accompanying JavaScript.

public:
	hzArray	<hdsGraphic*>	m_Shapes ;	//	Diagram components
	hzArray	<hdsLine>		m_Lines ;	//	Diagram connecting lines
	hzArray	<hdsText>		m_Texts ;	//	Diagram text components

	hzString	m_Id ;			//	Unique document element id
	hzString	m_Font ;		//	Main text font
	uint32_t	m_fillColor ;	//	Fill color
	uint32_t	m_lineColor ;	//	Line color
	uint16_t	m_Height ;		//	Total hieght of canvas
	uint16_t	m_Width ;		//	Total width of canvas

	hdsDiagram	(hdsApp* pApp) ;
	~hdsDiagram	(void) ;

	hzEcode	MakeJS	(hzChain& C) ;
	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami	(void)	{ return HZ_VISENT_DIAGRAM ; }
} ;

/*
**	Dissemino Resources
*/

enum	hdsRtype
{
	//	Category:	Dissemino Config
	//
	//	Resource type.

	DS_RES_FILE,		//	Dissemino resource is a passive file
	DS_RES_PAGE,		//	Dissemino resource is a page
	DS_RES_PAGE_CIF,	//	Dissemino resource is a page implimented by a C-Interface function
	DS_RES_ARTICLE,		//	Dissemino resource is an article
	DS_RES_ART_CIF,		//	Dissemino resource is an article implimented by a C-Interface function
} ;

class	hdsResource
{
	//	Category:	Dissemino Config
	//
	//	Pure base class for entities that have a URL. This includes pages declared by an <xpage> tag, passive pages (.html files), images and other downloadable
	//	files, articles and article groups, form-handler response pages or articles.

public:
	hzString	m_Url ;			//	URL of resource
	hzString	m_Title ;		//	Title of Servable Resource
	uint32_t	m_Access ;		//	Access flags
	uint32_t	m_flagVE ;		//	Operational flags
	uint32_t	m_HitCount ;	//	Hit counter
	uint32_t	m_Reserved ;	//	Reserved

	hdsResource	(void)	{ m_Access = m_flagVE = m_HitCount = m_Reserved = 0 ; }

	virtual	const char*	TxtURL	(void) = 0 ;
	virtual	hdsRtype	Whatami	(void) = 0 ;
} ;

class	hdsFile :	public hdsResource
{
	//	Category:	Dissemino Config
	//
	//	The hdsFile or Dissemino file class, declares a standalone file to be a servable resource. hdsFile instances are created for each <xfixfile> tag in the configs. There are a
	//	number of ways Dissemino can be made to serve files of HTML or other content, all of which are rudimentary. The files, which can be of any MIME type and be for any purpose,
	//	are transmitted to the client as-is. No processing is applied.

public:
	hzChain		m_rawValue ;	//	The Raw content, unzipped
	hzChain		m_zipValue ;	//	The zipped content
	hzString	m_filepath ;	//	Filename (relative to document root)
	hzMimetype	m_Mimetype ;	//	File MIME type

	hdsFile		(void)	{ m_Mimetype = HMTYPE_TXT_HTML ; }

	const char*	TxtURL	(void)	{ return *m_Url ; }
	hdsRtype	Whatami	(void)	{ return DS_RES_FILE ; }
} ;

#define INC_SCRIPT_CKEMAIL		0x01	//	Include in page header, ckEmail script
#define INC_SCRIPT_CKURL		0x02	//	Include in page header, ckUrl script
#define INC_SCRIPT_EXISTS		0x04	//	Include in page header, ckExists script
#define INC_SCRIPT_NAVBAR		0x08	//	Include in page header, navbar script
#define INC_SCRIPT_NAVTREE		0x10	//	Include in page header, navtree scripts
#define INC_SCRIPT_WINDIM		0x20	//	Include in page header, get window params script
#define INC_SCRIPT_RECAPTCHA	0x40	//	Include in page header, google recaptcha script

class	hdsCIFunc :	public hdsResource
{
public:
	hzEcode   (*m_pFunc)(hzHttpEvent*) ;

	//hzString	m_Url ;
	HttpMethod	m_Method ;

	hdsCIFunc	(void)
	{
		m_pFunc = 0 ;
		m_Method = HTTP_INVALID ;
	}

	const char*	TxtURL	(void)	{ return *m_Url ; }
	hdsRtype	Whatami	(void)	{ return DS_RES_PAGE_CIF ; }
} ;

class	hdsFormref ;
class	hdsExec ;

class	hdsPage :	public hdsResource
{
	//	Category:	Dissemino Config
	//
	//	Each hdsPage instance directs generation of a complete HTML page, including all the javascript nessesary to support all the components of the
	//	page. The javascript is compiled once at program startup and thereafter written out verbatim each time the page is invoked. The compilation is
	//	driven by cumulatively ORed flags with all the pages components ...
	//
	//	Note the member variable m_uidLang. This must either be set to 0 for no language support (the default) or a unique positive value for language support.
	//	This number will be combined with the element number and then used to lookup strings in the string table for the current language if that is othen than
	//	the default.

public:
	hzList	<hdsExec*>		m_Exec ;	//	List of commands to run upon display (generation)
	hzList	<hdsFormref*>	m_xForms ;	//	No of forms in page (used to assign standard scripts)
	hzList	<hzString>		m_Links ;	//	All detected links in webpages
	hzList	<hzString>		m_Scripts ;	//	All detected links in webpages
	hzArray	<hdsVE*>		m_VEs ;		//	First level visual entities

	hdsApp*		m_pApp ;			//	Parent Dissemino Application
	hzChain		m_XML ;				//	XML config export (for editing by admin)
	hzChain		m_Bodytext ;		//	Body text (that found within <p> tags)
	hzMD5		m_MD5 ;				//	Page config MD5 (determines if page config has altered during runtime)
	hzString	m_SrcFile ;			//	Name of XML config file the page is specified in (needed for saving online page edits)
	hzString	m_Subj ;			//	Subject (material classification)
	hzString	m_Desc ;			//	Description (will generate meta tags)
	hzString	m_Keys ;			//	Keywords (will generate meta tags)
	hzString	m_CSS ;				//	Name of style to include in the body tag
	hzString	m_Onpage ;			//	Name of script to run on onpageshow event (if any)
	hzString	m_Onload ;			//	Name of script to run on onload event (if any)
	hzString	m_Resize ;			//	Name of script to run on onresize event (if any)
	hzString	m_validateJS ;		//	Validation javascript (built by first run of WriteValidationJS)
	hdsUSL		m_USL ;				//	Universal string label. This will be of the form 'p' followed directly by a number (the order of appearence)
	uint32_t	m_EID ;				//	Config Entity ID
	uint32_t	m_bScriptFlags ;	//	Which scripts to include
	uint32_t	m_BgColor ;			//	Background color
	uint32_t	m_Line ;			//	Line number of defining <xpage> tag in config file
	uint16_t	m_Width ;			//	Margin width
	uint16_t	m_Height ;			//	Margin hieght
	uint16_t	m_Left ;			//	Margin left
	uint16_t	m_Top ;				//	Margin top

	hdsPage		(hdsApp* pApp) ;
	~hdsPage	(void)	{}

	//	Initialization
	hzEcode	AddVisent	(hdsVE* pVE)	{ return m_VEs.Add(pVE) ; }

	//	Write javascript to validate the form (takes JS from field data types)
	void	WriteValidationJS	(void) ;

	//	Write out HTML for page
	void	Head		(hzHttpEvent* pE) ;
	void	EvalHtml	(hzChain& Z, hdsLang* pLang) ;
	void	Display		(hzHttpEvent* pE) ;

	const char*	TxtURL	(void)	{ return *m_Url ; }
	hdsRtype	Whatami	(void)	{ return DS_RES_PAGE ; }
} ;

/*
//	Class Group:	Dissemino Trees and Tree items
*/

#define	HZ_TREEITEM_LINK	0x01	//	Use the refname as the URL
#define	HZ_TREEITEM_FORM	0x02	//	Article is a heading with a blank form for a sub-class
#define	HZ_TREEITEM_OPEN	0x04	//	Tree item is expanded in the navtree

class	hdsArticle :	public hdsResource
{
//protected:
public:
	hzString	m_Refname ;			//	Reference name of item
	uint32_t	m_Uid ;				//	Unique ID within tree
	uint32_t	m_ParId ;			//	Unique ID of parent item
	uint16_t	m_nCtx ;			//	Class context (for articles that are or have a form)
	uchar		m_nLevel ;			//	Level in tree
	uchar		m_bFlags ;			//	Operation flags (use refname as URL, expand/contracted navtree state)

public:
	int32_t		Level	(void)	{ return m_nLevel ; }
	const char*	TxtName	(void)	{ return *m_Refname ; }
	const char*	TxtURL	(void)	{ return *m_Refname ; }
} ;

class	hdsArticleStd :	public hdsArticle
{
	//	Category:	Dissemino HTML Generation
	//
	//	Used for contructing the internal model for navigation trees
	//

	hdsFormref*	m_pFormref ;		//	Articles may hold a maximum of one form

public:
	hzList	<hdsExec*>	m_Exec ;	//	List of commands to run upon display (generation)
	hzArray	<hdsVE*>	m_VEs ;		//	First level visual entities

	hzString	m_Content ;			//	Item content (applies only where article is of fixed content)
	hdsUSL		m_USL ;				//	Universal string label. Of the form 'gX.aY' where X is the group number and Y is the artile number within the group
	//uint32_t	m_CID ;				//	Collection ID

	hdsArticleStd	(void)	{ m_pFormref = 0 ; m_Uid = m_ParId = m_nCtx = 0 ; m_Access = m_flagVE = 0 ; m_nLevel = m_bFlags = 0 ; }

	~hdsArticleStd	(void)	{}

	//	Config functions
	hzEcode	AddVisent	(hdsVE* pVE) ;
	hzEcode	AddFormref	(hdsFormref* pFR) ;

	//	Display function
	void	Generate	(hzChain& Z, hzHttpEvent* pE) ;

	//	Get functions
	hdsRtype	Whatami	(void)	{ return DS_RES_ARTICLE ; }
} ;

class	hdsArticleCIF :	public hdsArticle
{
public:
	hzEcode   (*m_pFunc)(hzChain&, hdsArticleCIF* pArtCIF, hzHttpEvent*) ;

	//hzString	m_Url ;
	HttpMethod	m_Method ;

	hdsArticleCIF	(void)
	{
		m_pFunc = 0 ;
		m_Method = HTTP_INVALID ;
	}

	hzEcode		Run		(hzHttpEvent* pE) ;

	const char*	TxtURL	(void)	{ return *m_Url ; }
	hdsRtype	Whatami	(void)	{ return DS_RES_ART_CIF ; }
} ;

class	hdsTree
{
	//	Category:	Dissemino HTML Generation
	//
	//	Navigation or other tree

	hzMapM<uint32_t,hdsArticle*>	m_ItemsByParent ;	//	All items by parent ID
	hzMapS<hzString,hdsArticle*>	m_ItemsByName ;		//	All items by name

	hzEcode	_procArticle	(hzChain& J, hdsArticle* parent) ;
	hzEcode	_procTreeitem	(hzChain& J, hdsArticle* pItem) ;

	//	Prevent copies
	hdsTree	(const hdsTree&) ;
	hdsTree&	operator=	(const hdsTree&) ;

public:
	hzString	m_Groupname ;		//	Tree reference name/id or where used to group articles, the group name. It is needed to reference trees in pages and in
									//	form handler exec commands.
	hzString	m_Hostpage ;		//	URL of page that hosts the navtree (if the tree is to manifest as a navtree).
	hdsUSL		m_USL ;				//	Universal string label. This will be of the form 't' followed directly by a number (the order of appearence)

	hdsTree		(void)	{}
	~hdsTree	(void)	{ Clear() ; }

	hzEcode	Clear	(void) ;

	hdsArticle*		AddHead	(hdsArticle* pParent, const hzString& refname, const hzString& title, bool bSlct) ;
	hdsArticle*		AddHead	(const hzString& parId, const hzString& refname, const hzString& title, bool bSlct) ;

	hdsArticleStd*	AddItem	(hdsArticle* pParent, const hzString& refname, const hzString& title, bool bSlct) ;
	hdsArticleStd*	AddItem	(const hzString& parId, const hzString& refname, const hzString& title, bool bSlct) ;

	hdsArticleCIF*	AddArticleCIF	(hdsArticle* pParent, const hzString& refname, const hzString& title, hzEcode (*pFunc)(hzChain&, hdsArticleCIF*, hzHttpEvent*), bool bSlct) ;
	hdsArticleCIF*	AddArticleCIF	(const hzString& parId, const hzString& refname, const hzString& title, hzEcode (*pFunc)(hzChain&, hdsArticleCIF*, hzHttpEvent*), bool bSlct) ;

	//hdsArticle*	AddItem	(hdsArticle* pParent, hdsArticle* pItem, const hzString& refname, const hzString& title, bool bSlct) ;
	//hdsArticle*	AddItem	(const hzString& parId, hdsArticle* pItem, const hzString& refname, const hzString& title, bool bSlct) ;

	hdsArticle*		AddForm	(const hzString& parId, const hzString& refname, const hzString& title, const hzString& fname, const hzString& ctx, bool bSlct) ;

	hzEcode	Sync	(const hdbObject& obj) ;

	hzEcode	ExportArticleSet	(hzChain& Z) ;
	hzEcode	ExportDataScript	(hzChain& J) ;

	hdsArticle*	Item	(const hzString& name)	{ return m_ItemsByName[name] ; }
	hdsArticle*	Item	(uint32_t itemId)		{ return m_ItemsByName.GetObj(itemId) ; }

	uint32_t	Count	(void) const	{ return m_ItemsByName.Count() ; }
} ;

/*
**	Exec Commands
*/

class	hdsPage ;
class	hzHttpEvent ;

enum	Exectype
{
	//	Category:	Dissemino Exec
	//
	//	Enumerated Dissemino executive dommand types

	EXEC_NULL,			//	No instruction
	EXEC_SESS_COOKIE,	//	Set a session cookie. Will do nothing if either a session or permanent cookie is in already in use.
	EXEC_PERM_COOKIE,	//	Set a permanant cookie. Will do nothing if a permanent cookie is in already in use but will upgrade a session cookie.
	EXEC_SENDEMAIL,		//	Send an email
	EXEC_SETVAR,		//	Explicity set a user session standalone variable
	EXEC_ADDUSER,		//	Create a user
	EXEC_LOGON,			//	Logon a user
	EXEC_TEST,			//	Test if two values are equal
	EXEC_EXTRACT,		//	Extract text from a submitted file
	EXEC_OBJ_TEMP,		//	Create a HTTP event duration object (non cookie forms only). This must specify an object reference key and a pre-defined data class.
	EXEC_OBJ_START,		//	Assert a session duration object. This must specify an object reference key and a pre-defined data class.
	EXEC_OBJ_FETCH,		//	Load the named user session object. This must supply the object reference key and name a pre-declared repository.

	EXEC_OBJ_IMPORT,	//	Load the named user session object from a JSON file. This command must supply the object reference key and specify the full pathname
						//	of the data file.

	EXEC_OBJ_EXPORT,	//	Export the named user session object to a JSON file. This command must supply the object reference key and specify the full pathname
						//	of the data file.

	EXEC_OBJ_SETMBR,	//	Set a member value within the named session object. This command must supply the object reference key, the applicable class (either
						//	the object host class or a sub-class of it), the member name and define the source of the value.

	EXEC_OBJ_COMMIT,	//	Commit the named user session object to a repository
	EXEC_OBJ_CLOSE,		//	Closes the named user session object. This only needs to supply the object reference key.

	EXEC_TREE_DCL,		//	This declares a tree. This requires a unique tree id 
	EXEC_TREE_HEAD,		//	Adds an blank header to a user tree. This requires the tree id, parent node id, refname and title.
	EXEC_TREE_ITEM,		//	Adds an article to a tree. This requires the tree id, parent node id, refname and title.
	EXEC_TREE_FORM,		//	Adds an form as an article to a tree. This requires the tree id, parent node id, refname and title but also form name and class context.
	EXEC_TREE_SYNC,		//	Syncs the tree to a standalone object. Both must be tied to a user session. This requires the tree id and the object key.

	EXEC_TREE_DEL,		//	Deletes an item from a user tree. 1 parameter of id
	EXEC_TREE_EXP,		//	Expands an item from a user tree. 1 parameter of id
	EXEC_TREE_CLR,		//	Un-expands an item from a user tree. 1 parameter of id.

	EXEC_SRCH_PAGES,	//	A free text search on site pages
	EXEC_SRCH_REPOS,	//	A search on a repository
	EXEC_FILESYS,		//	File system commands direct the creation and deletion of directories and then save/delete files to/from those directories.
} ;

class	hdsExec
{
	//	Category:	Dissemino Exec
	//
	//	A Dissemino exec command

public:
	hdsApp*			m_pApp ;			//	Parent Dissemino Application
	hdsResource*	m_pFailResponse ;	//	Page to display in event of execution failure
	hzString		m_FailGoto ;		//	Page to goto in event of execution failure (if any)
	Exectype		m_Command ;			//	Type of command
	hdbBasetype		m_type ;			//	Datatype (applies to SETVAR)
	uint32_t		m_FstParam ;		//	First parameter (position in hdsApp array m_ExecParams)
	uint16_t		m_ReposId ;			//	Repository if any. This is needed for COMMIT and FETCH commands only
	uint16_t		m_ClassId ;			//	Class if aplicable. This is needed for all commands on standalone objects

	/*
	hdsExec		(hdsApp* pApp)
	{
		m_pApp = pApp ;
		m_pFailResponse = 0 ;
		m_FstParam = 0 ;
		m_ReposId = 0 ;
		m_ClassId = 0 ;
	}
	*/

	hdsExec		(hdsApp* pApp, Exectype cmd)
	{
		m_pApp = pApp ;
		m_pFailResponse = 0 ;
		m_FstParam = 0 ;
		m_ReposId = 0 ;
		m_ClassId = 0 ;
		m_Command = cmd ;
	}

	virtual	~hdsExec	(void)
	{
		m_pFailResponse = 0 ;
	}

	hzEcode	SendEmail	(hzChain& error, hzHttpEvent* pE) ;
	hzEcode	Adduser		(hzChain& error, hzHttpEvent* pE) ;
	hzEcode	Logon		(hzChain& error, hzHttpEvent* pE) ;
	hzEcode	Extract		(hzChain& error, hzHttpEvent* pE) ;
	hzEcode	Filesys		(hzChain& error, hzHttpEvent* pE) ;
	hzEcode	SrchPages	(hzChain& error, hzHttpEvent* pE) ;
	hzEcode	SrchRepos	(hzChain& error, hzHttpEvent* pE) ;

	virtual	hzEcode		Exec	(hzChain& error, hzHttpEvent* pE) ;
	virtual	Exectype	Whatami	(void)	{ return m_Command ; }
} ;

class	hdsFunc
{
	//	Category:	Dissemino Exec
	//
	//	This comprises a name and a pointer to a user defined C++ function that takes as arguments, a hzChain reference, a pointer to the HTTP event
	//	and a pointer to the applicable language - and returns a hzEcode value.
	//
	//	The <xfunc> tag has the form <xfunc funcname="some_function"/> and the Dissemino instance has a map of name to hdsFunc instances. The function
	//	will be called if a page is being generated and an <xfunc> tag with a matching name is encountered.

public:
	hzEcode	(*m_pFunc)(hzChain&, hzHttpEvent*, hdsLang*) ;	//	Function pointer

	hzString	m_funcname ;	//	Name of the function to dissemino

	hdsFunc	(void)	{ m_pFunc = 0 ; }
} ;

class	hdsProc
{
	//	Category:	Dissemino Exec
	//
	//	This comprises a name and a pointer to a user defined C++ function that takes as arguments, a pointer to the HTTP event and a pointer to the
	//	applicable language - and returns a hzEcode value.
	//
	//	The <xproc> tag has the form <xproc funcname="some_function"/> and the Dissemino instance has a map of name to hdsProc instances. The function
	//	will be called if a form is being processed and an <xproc> tag with a matching name is encountered.

public:
	hzEcode	(*m_pFunc)(hzHttpEvent*, hdsLang*) ;	//	Function pointer

	hzString	m_funcname ;	//	Name of the function to dissemino

	hdsProc	(void)	{ m_pFunc = 0 ; }
} ;

/*
**	Forms, For-handlers and form references.
*/

class	hdsFormdef
{
	//	Category:	Dissemino Exec
	//
	//	The hdsFormdef or form definition class, defines a set of fields for a form and holds JavaScript that will enable browsers to prevalidate form input. It
	//	also states the host data class and form name. Although the hdsFormdef constructor is public, circumstances in which C++ developers would opt for direct
	//	creation of hdsFormdef instances are expected to be few. Instead, hdsFormdef instances are generally created by the hdsApp::_readFormDef() function when
	//	processing <xformDef> tags in the configs.
	//
	//	All hdsFormdef instances are expected to be stored by name in the hdsApp map m_FormDefs. Form names by convention are prepended with 'form_' but this is
	//	not enforced by hdsApp::_readFormDef(). Instead form names are required only to be unique.
	//
	//	In accordance with the form-class guideline, forms are required to have a host data class. Forms must also have at least one field and one active button
	//	and associated form handler. Forms should ideally, also have an abort button but this generally a matter of the form HTML and is not compulsory.
	//
	//	For further information please see the synopsis "Dissemino Forms"

	hdsFormdef	(const hdsFormdef&) ;
	hdsFormdef&	operator=	(hdsFormdef& op) ;

public:
	hzMapS	<hzString,hdsField*>	m_mapFlds ;	//	All fields in order of name (for checking fields are not duplicated)
	hzVect	<hdsField*>				m_vecFlds ;	//	All fields in order of appearance
	hzArray	<hdsVE*>				m_VEs ;		//	Form visual entities

	const hdbClass*	m_pClass ;			//	Default class of cache to retrieve objects from
	hdsFormdef*		m_pParentForm ;		//	This is only set where the hostpage is a response/error page in a form-handler for another form
	hzString		m_Formname ;		//	Name of form as known to application
	hzString		m_DfltAct ;			//	Default action (if form handler is not named in the form buttons)
	hzString		m_DfltURL ;			//	URL of default action (if not supplied with the form buttons)
	hzString		m_ValJS ;			//	Validation JavaScript (to include functionality for all the buttons)
	uint16_t		m_nActions ;		//	Number of form actions (form handlers).
	int16_t			m_nReferrals ;		//	Set to -1 if form defined within a page or article definition. Otherwise indicates number of references.
	uint32_t		m_bScriptFlags ;	//	Which scripts to include

	hdsFormdef	(void)
	{
		m_pParentForm = 0 ;
		m_pClass = 0 ;
		m_nActions = 0 ;
		m_nReferrals = -1 ;
		m_bScriptFlags = 0 ;
	}
} ;

class	hdsFormhdl
{
	//	Category:	Dissemino Exec
	//
	//	The form-handler operates a set of executive steps (m_Exec) which each return a success or a fail. The process is halted as soon as a step fails and if
	//	the step has an error action (defined in an <OnError> tag), this error action will be invoked. This may take one of two forms. The <OnError> tag could
	//	comprise a (either a goto page or the 
	//
	//	ANY of the steps fail the form is not handled
public:
	hzList	<hdsExec*>	m_Exec ;		//	Array (list) of instructions to run upon submission

	hdsResource*	m_pCompletePage ;	//	Response to form submission success
	hdsResource*	m_pFailDfltPage ;	//	Response to form submission in error
	hdsFormdef*		m_pFormdef ;		//	Form definition in which the form handler is named
	hzString		m_Refname ;			//	Form handler name (needed to set action in forms)
	hzString		m_CompleteGoto ;	//	Response to successful form submission
	hzString		m_FailDfltGoto ;	//	Response to failed form submission if not reported by the executive steps
	uint32_t		m_Access ;			//	This assumes value of forms that name (call) the handler
	uint32_t		m_flgFH ;			//	Flags for issuance of cookies

	hdsFormhdl	(void) ;
	~hdsFormhdl	(void) ;
} ;

class	hdsFormref	: public hdsVE
{
	//	Category:	Dissemino Config
	//
	//	The hdsFormref or form reference class performs two roles. Firstly as a derivative of hdsVE, the hdsFormref is a visual entity and so enables a form to
	//	manifest as HTML. Secondly the form reference provides the context of operation for the form
	//	A form reference points to a form but can set an alternative class delta id for the host class
	//
	//	For further information please see the synopsis on "Application Specific Cloud Computing"

public:
	hzString	m_Formname ;		//	Formname (of form definition)
	uint32_t	m_ClsId ;			//	Class delta id to apply

	hdsFormref	(hdsApp* pApp) ;
	~hdsFormref	(void) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami	(void)	{ return HZ_VISENT_FORM ; }
} ;

class	hdsButton	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	HTML button
	//
	//	hdsButton manifests HTML to effect a button. The title of button is given in m_strContent. The button will either amount to a simple link or it will be
	//	the trigger for a form action. In the former case, the URL of the button will be in m_Link. In the latter case m_Link will be blank and the URL has to
	//	be derived.
	//
	//	As described in the synopsis 3.a Dissemino Forms, the button is tied to a named form handler but the URL will vary each time the form is referenced. The
	//	hdsButton instance is part of the form definition and so is not a decsendent of the hdsFormref visual entity. Even if it were, the form reference would
	//	still be unknown to the hdsButton at the point of HTML generation as the Generate() functions of the visible entities have no obvious means of passing
	//	this information. Instead the URL ...
	//
	//	and as such, may or may not be part of a form. Or the button has to manifest HTM that includes a unique URL

public:
	hzString	m_Formname ;	//	Formname (of form definition)
	//hdsFormdef*	m_pFormdef ;	//	Is button within a form (only relevent if it submits anything)
	hzString	m_Linkto;		//	Link to POST for form action button or GET if just a link button
	//uint32_t	m_FrmRefId ;	//	Form reference id (form action buttons only)
	//hzString	m_Action ;		//	Needed in cases where forms have more than one button

	//hdsButton	(void)	{ m_pFormdef = 0 ; }
	hdsButton	(hdsApp* pApp) ;
	~hdsButton	(void) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami	(void)	{ return HZ_VISENT_BUTTON ; }
} ;

class	hdsRecap	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	This should appear once within an <xform> tag and will generate a human test icon (google recaptcha)

public:
	hdsRecap	(hdsApp* pApp) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami	(void)	{ return HZ_VISENT_RECAP ; }
} ;

/*
**	Table Output Tags
*/

class	hdsCol
{
	//	Category:	Dissemino HTML Generation
	//
	//	This specifies columns in table, using the <xcol> tag. It is a syntax error to have an <xcol> tag outside the context of a table, which may either be a
	//	standard table of a directory listing. This class is created outside the confines of either so that it may be used in both.

public:
	hzString	m_Title ;			//	Colum title
	hzString	m_Member ;			//	Data class member name
	hzString	m_CSS_head ;		//	Title text class
	hzString	m_CSS_data ;		//	Title text class
	uint32_t	m_bgcol_head ;		//	Title area background color
	uint32_t	m_bgcol_data ;		//	Title area background color (even rows)
	uint32_t	m_bgcol_alt ;		//	Data area background color (odd rows)
	uint32_t	m_nSize ;			//	Max length of entries
	uint32_t	m_mbrNo ;			//	Data class member number
	bool		m_bEdit ;			//	Is column editable

	hdsCol	(void)
	{
		m_bgcol_head = m_bgcol_data = m_bgcol_alt = m_nSize = 0 ;
	}

	void	Clear	(void)
	{
		m_Title = m_CSS_head = m_CSS_data = (char*) 0 ;
		m_bgcol_head = m_bgcol_data = m_bgcol_alt = 0 ;
		m_mbrNo = m_nSize = 0 ;
		m_bEdit = false ;
	}
} ;

class	hdsDirlist	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	The hdsDirlist class as a visible entity is similar to hdsTable, drawing its data from a directory instead of a source repository. Instances of hdsDirlist are configured by
	//	means of the <xdirlist> tag. The HTML generated by this tag is a table with columns that are related to members of the hzDirent class. The <xcol> sub-tags within <xdirlist>
	//	must specifically name these members.
	//
	//	Within the <xdirlist> tag are the following important subtags:-
	//
	//		1)	<none>		Compulsory, contains - This must contain subtags to be rendered in the event of an EMPTY listing.
	//		2)	<header>	Optional, contains subtags to be rendered BEFORE the listing.
	//		3)	<footer>	Optional, contains subtags to be rendered AFTER the listing.
	//		4)	<foreach>	Compulsory, contains subtags to be rendered for each directory entry.
	//
	//	The directories listed in the output will form HTML links to the same page in which the <xdirlist> appears but with an appended resource argument. Files
	//	listed in the output will have links of the same form. The <xdirlist> tag has parameters of 'directory' which can be specified using percent entitits,
	//	and criteria (the file selection criteria) which must be specified using the usual globing notation.

public:
	hzList	<hdsCol>	m_Cols ;	//	Column names

	hdsVE*		m_pNone ;			//	HTML to be dispalyed in the event of no files/directories found
	hdsVE*		m_pHead ;			//	HTML to be displayed before listing
	hdsVE*		m_pFoot ;			//	HTML to be displayed after listing
	hzString	m_Directory ;		//	Data source name (repository class name, directory name)
	hzString	m_Criteria ;		//	Selection criteria
	hzString	m_Url ;				//	Page URL (needed as a base for links)
	uint16_t	m_Width ;			//	Width of table
	uint16_t	m_Height ;			//	Width of table
	uint16_t	m_nRows ;			//	Number of rows to display
	uint16_t	m_Order ;			//	Order to display date/asc, date/dec, name/asc, name/dec

	hdsDirlist	(hdsApp* pApp) ;
	~hdsDirlist	(void) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami		(void)	{ return HZ_VISENT_LOOP ; }
} ;

class	hdsTable	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	
	//	The hdsTable class as a visible entity is arguably a misnomer because it does not itself generate a visible manifestation. Instead it controls
	//	the presentation of sets of objects or values by child visible entities. hdsLoop is as its name implies, a loop controller. HTML is generated
	//	via the subtags for each object or value in the set.
	//
	//	In the configs hdsLoop instances are effected by the <xloop> tag. This requires an attribute of 'src' to specify the source of the list. This
	//	can be either a single class member of a single object that is itself a list (of objects or values) or it can be search criteria whose result
	//	will be a set of objects which, because we are not using SQL, is nessesarily limited to one class. Where the result is a list of values, the
	//	<xloop> subtags would generally be list tags (<ul> and <li>) and where the result is a list of objects, the <xloop> subtags would generally be
	//	table tags.
	//
	//	In the configs, the list or table subtag contents (the values to be displayed) are derived from the usual percent notation used for variables.
	//	There is no need for subscripts to the variables as within an <xloop> the context is always taken to mean 'the current iteration' of the loop
	//	ie the current member of the list.

public:
	hzList	<hdsCol>	m_Cols ;	//	Column names

	hdbObjCache*	m_pCache ;		//	Data repository
	hdsVE*			m_pNone ;		//	HTML to be dispalyed in the event of no files/directories found
	hdsVE*			m_pHead ;		//	HTML to be displayed before listing
	hdsVE*			m_pFoot ;		//	HTML to be displayed after listing
	hzString		m_Repos ;		//	Data source name (repository class name, directory name)
	hzString		m_Criteria ;	//	Selection criteria
	uint32_t		m_nWidth ;		//	Width of table
	uint32_t		m_nHeight ;		//	Width of table
	uint32_t		m_nRows ;		//	Number of rows to display
	bool			m_bEdit ;		//	Make table cells editable

	hdsTable	(hdsApp* pApp) ;
	~hdsTable	(void) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami		(void)	{ return HZ_VISENT_LOOP ; }
} ;

class	hdsHtag	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	Dissemino representation of a standard HTML5 tag.

public:
	hdsHtag	(hdsApp* pApp) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami		(void)	{ return HZ_VISENT_HTAG ; }
} ;

class	hdsXtag	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	Text supporting language translation. The <x> is to be inserted when text is to be translated into other languages when it otherwise would not be. For
	//	example, by default, the text contents of <pre> and <td> are exempt from translation. This is because <pre> commonly contains code fragments which are
	//	not meaningful when translated. The <td> also frequently contains matter that is not such as numeric values. The <x> tags switches on translation for
	//	either the whole or a part of these tags. It has the opposite effect within the <p> tag where it switches off translation.

public:
	hdsXtag	(hdsApp* pApp) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami		(void)	{ return HZ_VISENT_XTAG ; }
} ;

class	hdsBlock	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	Dissemino include block: A block of HTML tags to be included in resources such as pages. Include blocks are not resources since there is no associated URL, however they are
	//	'configurable entities' and so have an entitiy id.

public:
	hzArray	<hdsVE*>	m_VEs ;		//	Html entities within page (only first level, all sub-tags hang off of these)

	hzString	m_Refname ;			//	Refname
	hdsUSL		m_USL ;				//	Universal string label. This will be of the form 'b' followed directly by a number (the order of appearence)
	uint32_t	m_EID ;				//	Collection ID
	uint32_t	m_bScriptFlags ;	//	Scripts that need to appear in any page including this block

	hdsBlock	(hdsApp* pApp) ;

	hzEcode	AddVisent	(hdsVE* pVE) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;

	xTag	Whatami		(void)	{ return HZ_VISENT_HBLOCK ; }
} ;

class	hdsXdiv	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	The presence of hdsXdiv instances in a page template do not give rise to HTML <div> tags but they do include or exclude their sub-tags depending on who
	//	the user is. Instances of hdsXdiv are created by the <xdiv> tag in the configs. As the resulting page may differ, <xdiv> is an active tag and any page
	//	template containing one is deeemed active.

public:
	hdsXdiv	(hdsApp* pApp) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;

	xTag	Whatami	(void)	{ return HZ_VISENT_XDIV ; }
} ;

#define XCOND_EXISTS	1	//	The named entity exists and is not null
#define XCOND_ISNULL	2	//	The named entity does not exists or is null
#define XCOND_ACTION	4	//	The reserved event variable x-action has the specified value

class	hdsCond	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	The presence of hdsCond instances in a page template, due to <xcond> tags in the configs, will not produce a HTML tag directly but will include/exclude
	//	their sub-tags depending on the condition specified in the <xcond> attributes.

public:
	uint32_t	m_cflags ;		//	Generator condition flags

	hdsCond	(hdsApp* pApp) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;

	xTag	Whatami	(void)	{ return HZ_VISENT_XCOND ; }
} ;

class	hdsNavbar	: public hdsVE
{
	//	Category:	Dissemino HTML Generation
	//
	//	A hierarchical pull-down menu of links that is generated depending on the status of the user. There are headings which are omnipresent (always
	//	visible) and for each of these, there is either a direct link to a page or there is a sub-menu. The sub-menu if applicable, becomes visible by
	//	virtue of an onmouseover event on the heading, and contains a set of one or more links. In theory these could also support sub-menus but this
	//	is not currently implimented.
	//
	//	By default, the menu is populated by the subject attribute in each page.

public:
	hzString	m_JS ;		//	Javascript to replace <navbar> tag in page

	hdsNavbar	(hdsApp* pApp) ;

	void	MakeJS		(void) ;
	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;

	xTag	Whatami	(void)	{ return HZ_VISENT_NAVBAR ; }
} ;

/*
**	Users and Sessions
*/

class	hdsInfo	: public hzHttpSession
{
	//	Category:	Dissemino Exec
	//
	//	Generic user session. These apply wherever it is necessary to tie a browser instance to user data. A user session is created when a registered user logs into thier existing
	//	account or begins the process of user registration, however these are not the only cases that lead to a user session. For example, a shopping cart requires a session to add
	//	items to the cart but it is not generally necessary to be an already registered customer prior to and even after checkout.
	//
	//	Under the Dissemino regime, there is a direct 1:1 relationship between user sessions and cookies. Cookies are only ever issued upon creation of a session and are the key to
	//	locate the session on the server. User sessions are invoked under the direction of the configs. If an <xformHdl> tag has an "ops" attribute set as "cookies", the applicable
	//	form handler will issue a cookie and thus a session - unless these have already been issued by an earlier form submission.
	//
	//	Not all forms invoke user sessions. The 'contact-us' form for example, would not need to invoke a session if all it does is accept comments or enquiries from anyone without
	//	email or SMS verification. If however, email or SMS verification codes are required to complete the submission, a cookie will be issued and a session created. 
	//
	//	Since sessions and cookies are 1:1 the session must either hold or refer to ALL data applicable to the user. In the case of customers and contacts this can be simple. Every
	//	user will have a subscriber id and this alone will link to everything held on the user in the user repository. However there are circumstances where sessions will hold much
	//	more complex data. To this end, there are session values and the current object. Session values are standalone variables for any purpose, implemented as a name-value map.
	//	These can be set by 'setvar' instructions in form handlers and subsequently accessed by s-class percent entities. The current object if it applies, points to an instance of
	//	hdsObject and so can be an object of any class.

public:
	hzMapS	<hzString,hzAtom>	m_Sessvals ;	//	Session values
	hzList	<hdbObject*>		m_Objects ;		//	List of open objects

	hdsTree*	m_pTree ;		//	User nav tree
	hdbObject*	m_pObj ;		//	Current object
	void*		m_pMisc ;		//	Application specific
	hzString	m_Username ;	//	Username as per subscriber repository
	uint32_t	m_Access ;		//	Controls what the user will see (pages and parts of pages)
	uint32_t	m_SubId ;		//	Unique subscriber ID (object id) of user in the subscriber cache
	uint32_t	m_UserId ;		//	Unique ID (object id) of user (for lookup in the cache)
	uint32_t	m_CurrObj ;		//	The current record in view
	uint16_t	m_UserRepos ;	//	User repository id
	uint16_t	m_CurrRepos ;	//	Current repository if any
	uint16_t	m_CurrClass ;	//	Current class (within repos)
	uint16_t	m_Resv ;		//	Reserved

	hdsInfo		(void) ;
	~hdsInfo	(void) ;

	//hzEcode		ObjectAssert	(const hzString& objKey, const hzString& classname) ;
	hzEcode		ObjectAssert	(const hzString& objKey, const hdbClass* pClass) ;
	hdbObject*	ObjectSelect	(const hzString& objKey) ;
	hzEcode		ObjectClose		(const hzString& objKey) ;
} ;

/*
**	Dissemino Served Resources
*/

class	hdsArtref	: public hdsVE
{
	//	Category:	Dissemino Config
	//
	//	An in-page reference to a hdsArticle instance. Note that while a hdsArticle instance is not a visible entity, a hdsArtref is.

public:
	hzString	m_Group ;		//	Name of article group
	hzString	m_Article ;		//	Name of article
	uint32_t	m_Show ;		//	Show flags (1 for title, 2 for content, 3 for both)

	hdsArtref	(hdsApp* pApp) ;

	void	Generate	(hzChain& C, hzHttpEvent* pE, uint32_t& nLine) ;
	xTag	Whatami		(void)	{ return HZ_VISENT_ARTREF ; }
} ;

class	hdsSubject
{
	//	Category:	Dissemino Config

public:
	hzVect<hdsPage*>	pglist ;	//	Pages found under category in hand

	hzString	subject ;			//	Name of subject or category
	hzString	first ;				//	First item in list (use if only 1 page)
	hdsUSL		m_USL ;				//	Universal string label. This will be of the form 's' followed directly by a number (the order of appearence)
} ;

/*
**	Dissemino Specifics
*/

#define LIST_MODE_TREE		0x01	//	List structure is heirarchical (otherwise linear)
#define	LIST_LONE_PROMOTE	0x04	//	Promote items in the tree if they are alone

#define	COMMIT_NULL		0	//	Null instruction
#define	COMMIT_SETEQ	1	//	Set the class member to the named variable, even if the named variable is null
#define COMMIT_SETIF	2	//	Set the class member to the named variable, only if the named variable is non null

class	hdsProfile
{
	//	Category:	Dissemino Monitoring
	//
	//	Dissemino IP address/location non-personal client profile. Dissemino keeps a record of visitors as a 1:1 mapping of IP address to visitor profiles (hdsProfile instances).
	//	hdsProfile holds the aggregate total requests of each visitor. This information allows webmasters to see at a glance, which users are robots and which are human.

public:
	hzIpaddr	m_addr ;		//	IP address of client
	uint32_t	m_art ;			//	GET 200 - Articles requested
	uint32_t	m_page ;		//	GET 200 - Pages requested
	uint16_t	m_robot ;		//	GET 200 - Number of requests for robot.txt
	uint16_t	m_favicon ;		//	GET 200 - Number of requests for favicon.ico
	uint16_t	m_scr ;			//	GET 200 - Scripts requested
	uint16_t	m_img ;			//	GET 200 - Images requested
	uint16_t	m_spec ;		//	GET 200 - Special requests
	uint16_t	m_fix ;			//	GET 200 - Fix page requests
	uint16_t	m_G404 ;		//	GET 404 - non-existant resources requested
	uint16_t	m_post ;		//	POST OK Form submissions
	uint16_t	m_P404 ;		//	POST FAIL, Non-existant resources requested
	uint16_t	m_bFlags ;		//	Misc flags

	hdsProfile	(void)
	{
		m_art = m_page = 0 ;
		m_robot = m_favicon = m_scr = m_img = m_spec = m_fix = m_G404 = m_post = m_P404 = m_bFlags = 0 ;
	}
} ;

/*
**	Most Important Class - the App
*/

#define	DS_APP_ROBOT			0x01	//	The app supports robots.txt
#define	DS_APP_GUIDE			0x02	//	The app generates a siteguide
#define	DS_APP_SITEINDEX		0x04	//	The app generates index page content for searching
#define	DS_APP_SUBSCRIBERS		0x08	//	The app authenticates users
#define	DS_APP_FINANCIALS		0x10	//	The app uses the accounts package
#define DS_APP_NORMFILE			0x20	//	Allow files to be served when present relative to the document root

/*
**	Data Aquistion and Initstate Commands
*/

class	hdsLoad
{
	//	Category:	Dissemino Exec
	//
	//	Directive to load repository

public:
	const hdbClass*	m_pClass ;	//	Class applicable to imput data (CSV) if not same as that of the repository
	hdbObjRepos*	m_pRepos ;	//	Repository to be loaded

	hzString	m_Filepath ;	//	File to be imported

	hdsLoad	(void)	{ m_pRepos = 0 ; m_pClass = 0 ; }

	void	Clear	(void)	{ m_pRepos = 0 ; m_pClass = 0 ; m_Filepath.Clear() ; }
} ;

class	hdsApp
{
	//	Category:	Dissemino Config
	//
	//	hdsApp gathers all the control data required to effect a single 'Dissemino Application'

	struct	_tagArg
	{
		//	This is purely to simplify arguments to the recursive tag reader function hdsApp::_readTag(). It provides the following data:-

		hdsResource*	m_pLR ;			//	Page or article, if one is in the process of being defined
		hdsFormdef*		m_pFormdef ;	//	Form definitio if in progress
		hdsFormref*		m_pFormref ;	//	Form definitio if in progress
		const char*		m_pCaller ;		//	Calling function name

		_tagArg	()
		{
			m_pLR = 0 ;
			m_pFormdef = 0 ;
			m_pFormref = 0 ;
			m_pCaller = 0 ;
		}
	} ;

	//	Private Methods
	uint32_t	_calcAccessFlgs		(hzString& a) ;
	hzEcode		_readRgxType		(hzXmlNode* pN) ;
	hzEcode		_readDataEnum		(hzXmlNode* pN) ;
	hzEcode		_readUser			(hzXmlNode* pN) ;
	hzEcode		_readClass			(hzXmlNode* pN) ;
	hzEcode		_readRepos			(hzXmlNode* pN) ;
	hzEcode		_readInitstate		(hzXmlNode* pN) ;
	hzEcode		_readExec			(hzXmlNode* pN, hzList<hdsExec*>& execList, hdsPage* pPage, hdsFormhdl* pFhdl) ;
	hzEcode		_readFormHdl		(hzXmlNode* pN) ;
	hdsVE*		_readFormBut		(hzXmlNode* pN, hdsFormdef* pFormdef, hdsFormref* pFormref) ;
	hzEcode		_readFormDef		(hzXmlNode* pN) ;
	hdsFormref*	_readFormDef		(hzXmlNode* pN, hdsResource* pPage) ;
	hdsFormref*	_readFormRef		(hzXmlNode* pN, hdsResource* pPage) ;
	hzEcode		_readCSS			(hzXmlNode* pN) ;
	hzEcode		_readInclude		(hzXmlNode* pN, hdsVE* parent, uint32_t nLevel) ;
	hzEcode		_readFldspec		(hzXmlNode* pN) ;
	hzEcode		_readMember			(hdbClass* pClass, hzXmlNode* pN) ;
	hdsFormref*	_readLoginForm		(hzXmlNode* pN, hdsPage* pPage) ;
	hdsVE*		_readField			(hzXmlNode* pN, hdsFormdef* pFormdef) ;
	hdsVE*		_readXhide			(hzXmlNode* pN, hdsFormdef* pFormdef) ;
	hzEcode		_readText			(hdsText& tx, hzXmlNode* pN) ;
	hdsVE*		_readPieChart		(hzXmlNode* pN) ;
	hdsVE*		_readBarChart		(hzXmlNode* pN) ;
	hdsVE*		_readStdChart		(hzXmlNode* pN) ;
	hzEcode		_readShapes			(hzXmlNode* pN, hdsDiagram* pDiag) ;
	hdsVE*		_readDiagram		(hzXmlNode* pN) ;
	hdsVE*		_readFunc			(hzXmlNode* pN, uint32_t nLevel) ;
	hzEcode		_readColumn			(hdsCol& col, hzXmlNode* pN) ;
	hdsVE*		_readDirlist		(hzXmlNode* pN, hdsResource* pPage) ;
	hdsVE*		_readTable			(hzXmlNode* pN, hdsResource* pPage) ;
	hdsVE*		_readTag			(_tagArg* tga, hzXmlNode* pN, uint32_t& bScrFlags, hdsVE* parent = 0, uint32_t level = 0) ;
	hzEcode		_readStdLogin		(hzXmlNode* pN) ;
	hzEcode		_readLogout			(hzXmlNode* pN) ;
	hzEcode		_readResponse		(hzXmlNode* pN, hdsFormhdl* pFhdl, hzString& pageGoto, hdsResource** pPageGoto) ;
	hzEcode		_readFixFile		(hzXmlNode* pN) ;
	hzEcode		_readFixDir			(hzXmlNode* pN) ;
	hzEcode		_readMiscDir		(hzXmlNode* pN) ;
	hzEcode		_readPageBody		(hdsPage* pPage, hzXmlNode* pN) ;
	hzEcode		_readPage			(hzXmlNode* pN) ;
	hzEcode		_readArticle		(hzXmlNode* pN) ;
	hzEcode		_readXtreeDcl		(hzXmlNode* pN, hdsPage* pPage) ;
	hzEcode		_readXtreeItem		(hzXmlNode* pN, hdsTree* pAG) ;
	hdsVE*		_readXtreeCtl		(hzXmlNode* pN) ;
	hzEcode		_readScript			(hzXmlNode* pN) ;
	hzEcode		_readSiteLangs		(hzXmlNode* pN) ;
	hzEcode		_readNav			(hzXmlNode* pN) ;
	hzEcode		_readInclFile		(hzXmlNode* pN) ;
	hzEcode		_loadInclFile		(const hzString& dir, const hzString& fname) ;
	hzEcode		_autoFormClass		(hzChain& Z, const hdbClass* pClass, const hzString strAuto) ;
	hzEcode		_autoFormRepos		(hdbObjRepos* pRepos, const hzString strAuto) ;
	void		_assignveids_r		(hdsVE* pVE, const hdsUSL& base, uint32_t& flags, uint32_t& nId) ;
	void		AssignVisentIDs		(hzArray<hdsVE*>& listVW, uint32_t& flags, hdsUSL& base) ;

	//	Scripts
	hzEcode		MakeNavbarJS		(hzChain& Z, hdsLang* pLang, uint32_t access) ;
	hzEcode		MakeNavtreeJS		(hzChain& Z, uint32_t access) ;

	//	Misc Print Functions
	void	_doHead		(hzChain& Z, const char* cpPage) ;
	void	_doHeadR	(hzChain& Z, const char* cpPage, const char* cpUrl, int nDelay) ;
	void	_exportStr	(hzChain& Z, hdsVE* pVE) ;
	hzEcode	_insertPage	(hdsPage* pPage, const char* fn) ;

	//	Private constructor
	hdsApp	(void)
	{
		m_pLog = 0 ;
		m_Allusers = 0 ;
		m_MasterLogin = 0 ;
		m_MasterPage = 0 ;
		m_pDfltLang = 0 ;
		m_nPortSTD = 0 ;
		m_nPortSSL = 0 ;
		m_nLoadComplete = 0 ;
		m_OpFlags |= DS_APP_NORMFILE ;
	}

	//	Copy prevention
	hdsApp	(const hdsApp&) ;
	hdsApp&	operator=	(const hdsApp&) ;

public:
	hzList	<hdsLoad>				m_InitstateLoads ;		//	Data loads in event of -newData argument
	//hzMapS	<hzString,hdsUsertype>	m_UserTypes ;			//	All known types of users
	hzMapS	<hzString,uint32_t>		m_UserTypes ;			//	All known types of users vs access flags

	hzMapS	<hzString,hzDirent>		m_Configs ;				//	All known config files
	hzSet	<hzString>				m_CfgEdits ;			//	All known config files under edit
	hzMapS	<hzString,hzDirent>		m_Misc ;				//	Miscellaneous editable files
	hzMapS	<hzString,hdsFldspec>	m_Fldspecs ;			//	All variables definitions

	hzMapS	<hzString,hdsFormdef*>	m_FormDefs ;			//	All known form definitions by form definition name
	hzMapS	<hzString,hdsFormhdl*>	m_FormHdls ;			//	All known form handlers by name
	hzMapS	<hzString,hzString>		m_FormUrl2Hdl ;			//	Translates form action URLs to form handlers (used to invoke form handlers)
	hzMapS	<hzString,hdsFormref*>	m_FormUrl2Ref ;			//	Translates form action URLs to form references (used to obtain form handler op context)
	hzMapM	<hdsFormref*,hzString>	m_FormRef2Url ;			//	Translates form references to form action URLs (used by hdsButton::Generate())

	hzMapS	<hzString,hdsBlock*>	m_Includes ;			//	All known include blocks
	hzMapS	<hzString,hdsLang*>		m_Languages ;			//	All supported languages
	hzMapS	<hzString,hdsTree*>		m_ArticleGroups ;		//	All article trees. If the tree pointer is NULL the tree will be bound to the user session.
	hzMapS	<hzString,hdsResource*>	m_ResourcesPath ;		//	All known pages by path
	hzMapS	<hzString,hdsResource*>	m_ResourcesName ;		//	All known pages by name

	hzLookup<hzString>				m_rawScripts ;			//	Scripts
	hzLookup<hzString>				m_zipScripts ;			//	Scripts zipped
	hzLookup<uint32_t>				m_UserAgents ;			//	Collection of all encountered user agents

	hzMapS	<hzString,hdsPage*>		m_Responses ;			//	All known form response and error pages by name
	hzMapS	<hzSysID,hdsInfo*>		m_SessCookie ;			//	Session cookies by id
	hzMapS	<hzString,hdbBasetype>	m_tmpVarsSess ;			//	Temp map for percent entity validation
	hzMapS	<hzIpaddr,hdsProfile*>	m_Visitors ;			//	IP addresses of clients
	hzSet	<hzString>				m_Links ;				//	All links to other pages
	hzSet	<hzString>				m_Styles ;				//	All CSS classes
	hzList	<hzPair>				m_Passives ;			//	List of passive file directives

	hzMapS	<hzString,hdsSubject*>	m_setPgSubjects ;		//	Set of page subjects to limit vector below
	hzVect	<hdsSubject*>			m_vecPgSubjects ;		//	All page subjects
	hzVect	<hdsPage*>				m_vecPages ;			//	All known pages by name
	hzMapM	<uint32_t,hzFixPair>	m_VE_attrs ;			//	Attributes for the visual entitiies
	hzArray	<hdsVE*>				m_arrVEs ;				//	All known visual entitiies
	hzMapS	<hzString,hzString>		m_SObj2Class ;			//	Map of all known user session or independent single object container keys - to class name
	hzArray	<hzString>				m_ExecParams ;			//	Exec parameters

	hdbADP			m_ADP ;					//	Application delta profile
	hdsTree			m_DataModel ;			//	Data model of application
	hdbIndexText	m_PageIndex ;			//	The index of words/documents
	hzLogger*		m_pLog ;				//	Logger
	hdbObjCache*	m_Allusers ;			//	The allusers cache for logins
	hdsPage*		m_MasterLogin ;			//	Admin page 
	hdsPage*		m_MasterPage ;			//	Admin page 
	hdsLang*		m_pDfltLang ;			//	Default language
	hzChain			m_txtCSS ;				//	The stylesheet content (unzipped)
	hzChain			m_zipCSS ;				//	The stylesheet content (zipped)
	hzChain			m_zipSitemapTxt ;		//	The sitemap.txt (zipped)
	hzChain			m_zipSitemapXml ;		//	The sitemap.xml (zipped)
	hzChain			m_rawSitemapTxt ;		//	The sitemap.txt (unzipped)
	hzChain			m_rawSitemapXml ;		//	The sitemap.xml (unzipped)
	hzChain			m_rawSiteguide ;		//	The siteguide.txt (zipped)
	hzChain			m_zipSiteguide ;		//	The siteguide.xml (zipped)
	hzChain			m_cfgErr ;				//	Written to by config read functions associated with key resources
	hzDomain		m_Domain ;				//	The URL base string (eg www.mydomain.com)
	hzString		m_BaseDir ;				//	The base dir
	hzString		m_RootFile ;			//	The root of the application configs
	hzString		m_namCSS ;				//	The stylesheet name
	hzString		m_Appname ;				//	Name of project
	hzString		m_CookieName ;			//	Name of cookie "_hz_" + adaption of domain name
	hzString		m_Docroot ;				//	Base directory for miscellaneous items
	hzString		m_Configdir ;			//	Base directory for config files
	hzString		m_Images ;				//	Base directory for images
	hzString		m_Datadir ;				//	Base directory for data files
	hzString		m_Logroot ;				//	Base directory for log files
	hzString		m_MasterPath ;			//	Admin entry URL
	hzString		m_MasterUser ;			//	Admin username
	hzString		m_MasterPass ;			//	Admin password
	hzString		m_SmtpAddr ;			//	Email address for outgoing messages, e.g. info@domain
	hzString		m_SmtpUser ;			//	Username for local email server
	hzString		m_SmtpPass ;			//	Password for local email server
	hzString		m_Recaptcha ;			//	Google recaptcha key
	hzString		m_AllHits ;				//	The all hits repository name
	hzString		m_UsernameFld ;			//	Name of the username field (eg 'username' or 'email'). This will be looked for in the login inputs.
	hzString		m_UserpassFld ;			//	Name of the password field (usually 'password'). This will be looked for in the login form inputs
	hzString		m_KeyPublic ;			//	Recaptcha public key
	hzString		m_KeyPrivate ;			//	Recaptcha private key
	hzString		m_DefaultLang ;			//	Default language (en-US unless otherwise specified in a <siteLanguages> tag)
	hzString		m_Robot ;				//	The robot.txt response
	hzString		m_LoginPost ;			//	The URL to which login form submissions must be posted
	hzString		m_LoginFail ;			//	The URL the user will be directed to in event of username/password mismatch
	hzString		m_LoginAuth ;			//	The URL the user will be directed to in event of successful authentication
	hzString		m_LoginResume ;			//	The URL the user will be directed to in event of successful session resumption
	hzString		m_LoginAJAX ;			//	The URL to which AJAX login form submissions must be posted
	hzString		m_LogoutURL ;			//	The logout URL
	hzString		m_LogoutDest ;			//	The logout destination page
	uint32_t		m_LastCfgEpoch ;		//	Last recorded epoch time of config directory (aux thread in Dissemino uses this to detect updates
	uint32_t		m_nLoadComplete ;		//	Set once load complete (so any read config functions tolerate duplicate resources)
	//bool			m_bIndex ;				//	Index pages for site search facility
	uint16_t		m_nPortSTD ;			//	HTTP Port
	uint16_t		m_nPortSSL ;			//	HTTPS Port
	uint16_t		m_AppID ;				//	Application ID as assigned by Dissemino
	uint16_t		m_OpFlags ;				//	DS_APP series flags

	~hdsApp	(void)	{ Shutdown() ; }

	hzEcode		Init					(const hzString& domain, const hzString& baseDir, const hzString& rootfile) ;
	hzEcode		InitMasterLogin			(const hzString& masterPath, const hzString& masterUser, const hzString& masterPass) ;
	hzEcode		InitMailerAuth			(const hzString& smtpAddr, const hzString& smtpUser, const hzString& smtpPass) ;

	hzEcode		LoadPassives			(void) ;
	hzEcode		AddCIFunc				(hzEcode (*pFunc)(hzHttpEvent*), const hzString funcname, uint32_t access, HttpMethod eMethod) ;
	hzEcode		ReadProject				(void) ;
	hzEcode		ReloadConfig			(const char* cpFilename) ;
	hzEcode		SetLoginPost			(const hzString& post, const hzString& fail, const hzString& auth, const hzString& resume) ;
	hzEcode		SetLoginAJAX			(const hzString& cmd) ;
	hzEcode		AddUserType				(const hzString& utname) ;
	hzEcode		CheckProject			(void) ;
	hzEcode		ExportStrings			(void) ;
	void		ImportStrings			(void) ;
	hzEcode		CreateDefaultForm		(const hzString& cname) ;
	hzEcode		ExportDefaultForm		(const hzString& cname) ;
	hzEcode		IndexPages				(void) ;
	void		SetStdTypeValidations	(void) ;
	void		SetupMasterMenu			(void) ;
	void		SetupScripts			(void) ;
	void		SendErrorPage			(hzHttpEvent* pE, HttpRC rv, const char* func, const char* va_alist ...) ;
	void		SendErrorPage			(hzHttpEvent* pE, HttpRC rv, const char* func, hzChain& error) ;
	bool		IsPcEnt					(hzString& pcntEnt, const char* i) ;
	bool		AtPcEnt					(hzString& pcntEnt, hzChain::Iter& pos) ;
	hdbBasetype	PcEntTest				(hzString& error, hdsFormdef* pFormdef, const hdbClass* pHost, const hzString& e) ;
	hzEcode		PcEntConv				(hzAtom& atom, const hzString& v, hzHttpEvent* pE) ;
	hzEcode		PcEntScanStr			(hzString& error, hdsFormdef* pFormdef, hdbClass* pHost, const hzString& input) ;
	hzEcode		PcEntScanChain			(hzString& error, hdsFormdef* pFormdef, hdbClass* pHost, const hzChain& input) ;
	void		ConvertText				(hzChain& Z, hzHttpEvent* pE) ;
	hzString	ConvertText				(const hzString& s, hzHttpEvent* pE) ;
	hzSysID		MakeCookie				(const hzIpaddr& ipa, uint32_t eventNo) ;

	//	Admin Section
	hzEcode		_SubscriberAuthenticate	(hzHttpEvent* pE) ;

	//	Operational Post Init
	void		InPageQuery				(hzHttpEvent* pE) ;
	void		MasterArticle			(hzHttpEvent* pE) ;
	void		SendDocument			(hzHttpEvent* pE) ;
	void		ProcForm				(hzHttpEvent* pE, hdsFormref* pFormref, hdsFormhdl* pFhdl) ;
	hzTcpCode	ProcHTTP				(hzHttpEvent* pE) ;
	void		Shutdown				(void) ;

	static		hdsApp*	GetInstance		(hzLogger& pLog) ;
} ;

class	Dissemino
{
	//	Category:	Dissemino Config
	//
	//	Dissemino - the singleton Dissemino instance.
	//
	//	This holds one or more DIssemino applications, allowing Dissemino as a program, to run several applications simultaneously.

	hzMapS	<hzString,hdsApp*>	m_AppsByHost ;	//	Map of Dissemino Applications by Hostname
	hzMapS	<uint16_t,hdsApp*>	m_AppsByID ;	//	Map of Dissemino Applications by ID

	Dissemino	(void)
	{
		m_pLog = 0 ;
		m_nPortSTD = m_nPortSSL = 0 ;
	}

	//	Copy prevention
	Dissemino	(const hdsApp&) ;
	Dissemino&	operator=	(const Dissemino&) ;

public:
	hzLogger*	m_pLog ;			//	Logger
	hzString	m_CookieName ;		//	Name of cookie "_hz_" + adaption of domain name
	uint16_t	m_nPortSTD ;		//	HTTP Port
	uint16_t	m_nPortSSL ;		//	HTTPS Port

	static		Dissemino*	GetInstance	(hzLogger& pLog) ;

	hzEcode		SetCookieName	(const hzString& cookieBase) ;

	hzEcode		AddApplication	(const hzString& domain, const hzString& basedir, const hzString& rootfile, uint32_t bOpFlags, uint32_t nPortSTD, uint32_t nPortSSL) ;
	hdsApp*		GetApplication	(const hzString& domain) ;
	hdsApp*		GetApplication	(uint32_t appId) ;

	hzEcode		ReadSphere		(const char* cpFilename) ;

	uint32_t	Count	(void) const	{ return m_AppsByID.Count() ; }
} ;

/*
**	Application Admin Functions. Note that these would be hdsApp members since they are closely tied to the application being administered. But in order for them to be added to the
**	application's m_Resources map as C-Interface functions, they have to be non-members.
*/

hzEcode		_masterLoginPage		(hzHttpEvent* pE) ;
hzEcode		_masterProcAuth			(hzHttpEvent* pE) ;
//	hzEcode		_masterMainMenu			(hzHttpEvent* pE) ;
hzEcode		_masterLogout			(hzHttpEvent* pE) ;
//	hzEcode		_masterResList			(hzHttpEvent* pE) ;
//	hzEcode		_masterVisList			(hzHttpEvent* pE) ;
//	hzEcode		_masterBanned			(hzHttpEvent* pE) ;
//	hzEcode		_masterDomain			(hzHttpEvent* pE) ;
//	hzEcode		_masterEmaddr			(hzHttpEvent* pE) ;
//	hzEcode		_masterStrFix			(hzHttpEvent* pE) ;
//	hzEcode		_masterStrGen			(hzHttpEvent* pE) ;
//	hzEcode		_masterMemstat			(hzHttpEvent* pE) ;
//	hzEcode		_masterUSL				(hzHttpEvent* pE) ;
//	hzEcode		_masterFileList			(hzHttpEvent* pE) ;
//	hzEcode		_masterFileEdit			(hzHttpEvent* pE) ;
hzEcode		_masterFileEditHdl		(hzHttpEvent* pE) ;
//	hzEcode		_masterDataModel		(hzHttpEvent* pE) ;
//	hzEcode		_masterCfgList			(hzHttpEvent* pE) ;
//	hzEcode		_masterPageEdit			(hzHttpEvent* pE, const hzString& fname) ;
//	hzEcode		_masterPageEditHdl		(hzHttpEvent* pE) ;
//	hzEcode		_masterCfgEdit			(hzHttpEvent* pE, const hzString& fname) ;
hzEcode		_masterCfgEditHdl		(hzHttpEvent* pE) ;
//	hzEcode		_masterCfgRestart		(hzHttpEvent* pE) ;

/*
**	Externals
*/

extern	Dissemino*	_hzGlobal_Dissemino ;	//	The global Dissemino instance

#endif	//	hzDissemino_h
