//
//	File:	enforcer.h
//
//  Legal Notice: This file is part of the HadronZoo::CodeEnforcer program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
//
//  Copyright 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//  HadronZoo::CodeEnforcer is free software: You can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or any later version.
//
//  The HadronZoo C++ Class Library is also free software: You can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or any later version.
//
//  HadronZoo::CodeEnforcer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
//  A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with HadronZoo::CodeEnforcer. If not, see http://www.gnu.org/licenses.

//	Synopsys:	CodeEnforcer
//
//	<b>Introduction to CodeEnforcer</b>
//
//	CodeEnforcer is a C++ project document compiler. It examines a body of C++ code (set of header and source files), from which it compiles a set of articles for publication as an
//	online reference manual. During the process, the code is checked for deviations from the HadronZoo Coding Standards - hence the name. CodeEnforcer tokenizes and parses the body
//	of code in much the same way a C++ compiler would, to populate a series of related entity tables. Comments are lifted from the code and associated with the entities. The tables
//	are then exported as a hierarchical document.
//
//	As the intention is to produce documentation rather than an executable binary, the term 'entitiy' has a wider definition and includes project entities such as source and header
//	files, as well as the C++ entities the files contain. It is important the overall code structure, is refelected in the compiled document. Broadly, each entity that qualifies as
//	an 'item of interest' produces an article, although some entities are naturally grouped and so produce only one article for the group, e.g. overloaded functions. Other entities
//	can be grouped as directed by specially formatted comments.
//
//	Only C++ entities defined within the project code body can qualify as an item of interest and not all of them do. For example class member functions that are defined inline are
//	not treated as items of interest in their own right and appear only as part of the article that describes the class. Furthermore, not all articles will appear in the navigation
//	tree. Articles relating to entities whose names begin with an underscore are suppressed in this way. This convention is usually applied to support and private member functions.
//	The article is available but the only link to it is in the article for the class or supported entity.
//
//	<b>CodeEnforcer Internal Model</b>
//
//	Project entities and the CodeEnforcer C++ classes that represent them, are as follows:-
//
//		ceProject:	The project itself
//		ceComp:		A project component (see note)
//		ceSynp:		A synopsis or major comment block
//		ceFile:		A source, header or comment file
//
//	C++ Entities are represented by derivatives of the base class ceEntity. Most C++ entities either HAVE or ARE a data type and to reflect this, ceEntity has derivatives of ceReal
//	and ceDatatype respectively. There are exceptions however, notably namespaces (ceNamsp), and macros (ceMacro).
//
//	In addition there are entities for internal operation namely ...
//
//	In summary we have:-
//
//	1 ceFrame
//
//		1.1 ceSynp:		An out of code synopsis that will create a separate webpage
//		1.2 ceFile:		A C++ header or source file
//		1.3 ceComp:		A project component (either a library or a program)
//		1.4 ceProject:	The whole project
//		1.5 ceEntity:	The base class for all type of C++ entities ocuring within the code base being processed. It has the following derivatives:-
//
//			1.5.1 ceDatatype: Base class for C++ entities that ARE a data type.
//				1.5.1.1 ceKlass:
//					1.5.1.1.1 ceClass:	A C++ class or struct or class or struct template
//					1.5.1.1.2 ceUnion:	A union
//				1.5.1.2 ceTarg:		A class template argument (data type substitute)
//				1.5.1.3 ceCStd:		Standard C++ data type
//				1.5.1.4 ceEnum:		An enum
//				1.5.1.5 ceTypedef:	A renamed data type via a typedef statement
//
//			1.5.2 ceReal:	This is the base class for C++ entities that HAVE a data type so with the sole exceptions of void functions, can be evaluated.
//				1.5.2.1 ceEnumval:	Enumerated value
//				1.5.2.2 ceVar:		Any declared variable
//				1.5.2.3 ceCpFn:
//				1.5.2.4 ceFunc:		C++ function (note even void functions that cannot be evaluated)
//				1.5.2.5 ceFtmpl:	Function template
//			1.5.3 ceNamsp:		Namespace
//			1.5.4 ceFngrp:
//			1.5.5 ceDefine:		A #define (direct code substitution)
//			1.5.6 ceLiteral:	Special case of a #define that serves as a named literal value
//			1.5.7 ceMacro:		Special case of a #define that serves as a function
//	2 ceToken
//	3 cePara
//	4 ceEntbl	Entity Table (ET)
//	5 ceTyplex
//	6 ceCodent
//		6.1 ceStmt
//		6.2 ceCbody
//	7 ceFnset	A user defined group of fuctions that will result in a single webpage
//	8 ceCpFn	???
//	9 ceView	???
//	10 ceClgrp	A user defined class group that will result is a single webpage
//
//	<b>Entity tables and Scoping Rules</b>
//
//	CodeEnforcer ETs reflect the complex scoping and precedence rules of the C++ language and apply further rules in accordance with HadronZoo coding standards. In C++ the names of
//	standard data types such as int are reserved words and within any given namespace, names of data types such as classes must be unique. However, what is a class in one namespace
//	can be something else in another and a class defined within a namespace can have the same name as the namespace! Under HadronZoo coding standards, a namespace may not have the
//	same name as a data type or any other entitiy and data type names must be unique across the entire code base. Namespacing is limited to function and variables. The only way two
//	classes can have the same name, is if they are both subclasses of different host classes or class templates. There are no circumstances under which unions or enums, can share a
//	name. The extra rules are there to avoid confusion. It is also not permitted to redefine global entities locally, even where those global entities are kept out of scope by not
//	including the headers in which they are declared.
//
//	The ETs are hierarchical and begin with the root ET. Entities stored directly in the root ET are said to be at the first or the 'file' level. The hierarchy arises because some
//	entities incorporate an ET. These are:-
//
//		1)	ceNamsp	- A namespace
//		1)	ceFile	- A source or header file
//		2)	ceKlass	- A class or class template or a union.
//		3)	ceCBody	- A function code body (can be nested)
//
//	The root ET is taken to apply to a global overarching and unnamed namespace, with any declared namespaces being nested within it. Thus, all entities are in a namespace. Lookups
//	always begin at the highest level so in a nested code body, the ET for the code body is searched first, then the ET for the parent code body, all the way back to the ET for the
//	function (see note). If the entity is not found in the function ET the search moves to the parent ET which will be the class if the function is a class member. Failing that the
//	next ET to query is that of the file. This may seem strange but it copes with entities that are static to the file. After that the namespace ET - which if no named namespace is
//	applicable, will be the root. The lookup process simply returns the first match.
//
//	Two forms of lookup are available, by token and by string. The former means the current token within the code is treated by the lookup function as the first token of an entity.
//	The latter is only for processing comments and will process strings with forms like "class::member" into their constituent parts before performing the lookup.
//
//	The CodeEnforcer.sys File
//
//	Note that Code Enforcer will not treat entities defined outside the code base under examination, as items of interest. Fundamental C++ data types and other standard entities of
//	the C/C++ language are specifically excluded from the reports produced. Not only would reporting such entities be pointless but code used in their implementation, to the extent
//	that it can be located, falls outside the standards Code Enforcer is seeking to enforce. In order to process code that refers to these entities, Code Enforcer has to know what
//	these entties and their interfaces look like. To resolve this a set of dummy definitions of the standard entities is provided in $HADRONZOO/data/CodeEnforcer.sys.

#if !defined(enforcer_h)
#define enforcer_h

#include "hzBasedefs.h"
#include "hzChars.h"
#include "hzCtmpls.h"
#include "hzDocument.h"
#include "hzDissemino.h"
#include "hzDatabase.h"

#define TOKTYPE_MASK_LITERAL	0x000100	//	Indicates a literal number or string
#define TOKTYPE_MASK_COMMENT	0x000200	//	Indicates a comment (redundant)
#define	TOKTYPE_MASK_DIRECTIVE	0x000400	//	Indicates token is a compiler directive
#define	TOKTYPE_MASK_KEYWORD	0x000800	//	Indicates token is C++ keyword affecting scope
#define	TOKTYPE_MASK_VTYPE		0x001000	//	Indicates token is C++ keyword affecting operation of or type of variable
#define TOKTYPE_MASK_STRUCT		0x002000	//	Indicates code structure tokens
#define TOKTYPE_MASK_COMMAND	0x004000	//	Indicates token directs branches and jumps
#define TOKTYPE_MASK_UNARY		0x008000	//	Indicates token is a C++ operators unary

#define TOKTYPE_MASK_ASSIGN		0x010000	//	Indicates token is a C++ operator conditional
#define TOKTYPE_MASK_CONDITION	0x020000	//	Indicates token is a C++ operator conditional

#define TOKTYPE_MASK_MATH_ADD	0x100000	//	Indicates token is a C++ operators math add/sub
#define TOKTYPE_MASK_MATH_MLT	0x200000	//	Indicates token is a C++ operators math mult/div
#define TOKTYPE_MASK_LOGIC		0x400000	//	Indicates token is a C++ operators math mult/div
#define TOKTYPE_MASK_BINARY		0x730000	//	Indicates token is a C++ operators binary
#define	TOKTYPE_MASK_OPERATOR	0x738000	//	If thie bit is set token is C++ operator

#define	TOK_FLAG_COMMENT_LINE	0x000001	//	If the token is a comment and this is set, the comments are line comments (default block)
#define	TOK_FLAG_COMMENT_PROC	0x000002	//	If the token is a comment and this is set, the comment has been processed

enum	CppLex
{
	//	Parsing of C++ header and source files is firstly a matter of tokenizing the file content into lexical tokens based on charachter sets. Then the tokens are grouped together
	//	to form C++ statements. CppLex means 'lexical' C++ type.

	TOK_UNKNOWN			= 0x000000,		//	No known type
	TOK_WORD			= 0x000001,		//	Alphanumeric string, could be a name or a type
	TOK_TEMPLATE		= 0x000002,		//	The word 'template'
	TOK_CLASS			= 0x000003,		//	The word 'class'
	TOK_STRUCT			= 0x000004,		//	The word 'struct'
	TOK_UNION			= 0x000005,		//	The word 'union'
	TOK_ENUM			= 0x000006,		//	The word 'enum'
	TOK_TYPEDEF			= 0x000007,		//	typedef
	TOK_SQ_OPEN			= 0x000008,		//	The '[' char
	TOK_SQ_CLOSE		= 0x000009,		//	The ']' char
	TOK_ROUND_OPEN		= 0x00000A,		//	The '(' char
	TOK_ROUND_CLOSE		= 0x00000B,		//	The ')' char
	TOK_CURLY_OPEN		= 0x00000C,		//	The '{' char
	TOK_CURLY_CLOSE		= 0x00000D,		//	The '}' char
	TOK_SEP				= 0x00000E,		//	The comma char (as in seperation of args)
	TOK_END				= 0x00000F,		//	The ';' terminating a statement
	TOK_ESCAPE			= 0x000010,		//	The '\' character
	TOK_ELIPSIS			= 0x000011,		//	Three periods in a row (varargs)

	//	Literal values
	TOK_QUOTE			= 0x000101,		//	Double quoted string (null terminated)
	TOK_SCHAR			= 0x000102,		//	Single quoted string (single char value)
	TOK_NUMBER			= 0x000103,		//	Token evaluates to a decimal number
	TOK_STDNUM			= 0x000104,		//	Token evaluates to a standard form number
	TOK_OCTNUM			= 0x000105,		//	Token evaluates to an octal number
	TOK_HEXNUM			= 0x000106,		//	Token evaluates to a hexadecimal number
	TOK_BOOLEAN			= 0x000107,		//	Token evaluates to either true or false

	//	Type groupings - comments
	TOK_COMMENT			= 0x000200,		//	Comment (line or block). Note that a series of line comments are treated as a single comment block if they are not separated by either a
										//	blank line or a non-comment token.

	//	Compiler directives
	TOK_DIR_IF			= 0x000401,		//	#if
	TOK_DIR_ELSE		= 0x000402,		//	#else
	TOK_DIR_ELSEIF		= 0x000403,		//	#elseif
	TOK_DIR_ENDIF		= 0x000404,		//	#endif
	TOK_DIR_IFDEF		= 0x000405,		//	#ifdef
	TOK_DIR_IFNDEF		= 0x000406,		//	#ifndef
	TOK_DIR_DEFINE		= 0x000407,		//	#define
	TOK_DIR_UNDEF		= 0x000408,		//	#undef
	TOK_DIR_INCLUDE		= 0x000409,		//	#inclue

	//	C++ keywords affecting scope
	TOK_KW_INLINE		= 0x000801,		//	inline
	TOK_KW_STATIC		= 0x000802,		//	static
	TOK_KW_EXTERN		= 0x000803,		//	extern
	TOK_KW_FRIEND		= 0x000804,		//	friend
	TOK_KW_REGISTER		= 0x000805,		//	register
	TOK_KW_VIRTUAL		= 0x000806,		//	virtual
	TOK_KW_PUBLIC		= 0x000807,		//	public
	TOK_KW_PRIVATE		= 0x000808,		//	private
	TOK_KW_PROTECTED	= 0x000809,		//	protected
	TOK_KW_USING		= 0x00080A,		//	using
	TOK_KW_NAMESPACE	= 0x00080B,		//	namespace
	TOK_KW_OPERATOR		= 0x00080C,		//	the word 'operator'

	//	C++ keywords affecting operation of or type of variable
	TOK_VTYPE_CONST		= 0x001001,		//	const
	TOK_VTYPE_UNSIGNED	= 0x001002,		//	The word 'unsigned'
	TOK_VTYPE_VOID		= 0x001003,		//	The word 'void'
	TOK_VTYPE_CHAR		= 0x001004,		//	The word 'char'
	TOK_VTYPE_SHORT		= 0x001005,		//	The word 'short'
	TOK_VTYPE_INT		= 0x001006,		//	The word 'int'
	TOK_VTYPE_LONG		= 0x001007,		//	The word 'long'
	TOK_VTYPE_MUTABLE	= 0x001008,		//	mutable

	//	C++ operators (0x2000 series, structure)
	TOK_OP_SCOPE		= 0x002001,		//	C++ operator ::
	TOK_OP_SEP			= 0x002002,		//	C++ operator :
	TOK_OP_MEMB_PTR		= 0x002003,		//	C++ operator ->
	TOK_INDIR			= 0x002004,		//	Asterisks in series
	TOK_CMD_NEW			= 0x002005,		//	'new'
	TOK_CMD_DELETE		= 0x002006,		//	'delete'
	TOK_OP_QUERY		= 0x002007,		//	C++ operator ?
	TOK_OP_PERIOD		= 0x002008,		//	C++ operator .
	TOK_OP_THIS			= 0x002009,		//	The 'this' operator
	TOK_OP_SIZEOF		= 0x00200A,		//	The 'sizeof' macro
	TOK_OP_DYN_CAST		= 0x00200B,		//	'dynamic_cast'

	//	C++ commands (0x4000 series, commands)
	TOK_CMD_IF			= 0x004001,		//	'if'
	TOK_CMD_ELSE		= 0x004002,		//	'else'
	TOK_CMD_SWITCH		= 0x004003,		//	'switch'
	TOK_CMD_CASE		= 0x004004,		//	'case'
	TOK_CMD_DEFAULT		= 0x004005,		//	'default'
	TOK_CMD_FOR			= 0x004006,		//	'for'
	TOK_CMD_DO			= 0x004007,		//	'do'
	TOK_CMD_WHILE		= 0x004008,		//	'while'
	TOK_CMD_BREAK		= 0x004009,		//	'break'
	TOK_CMD_CONTINUE	= 0x00400A,		//	'continue'
	TOK_CMD_GOTO		= 0x00400B,		//	'goto'
	TOK_CMD_RETURN		= 0x00400C,		//	'return'

	//	C++ operators (0x8000 series, unary)
	TOK_OP_INCR			= 0x008001,		//	C++ operator ++
	TOK_OP_DECR			= 0x008002,		//	C++ operator --
	TOK_OP_NOT			= 0x008003,		//	C++ operator !
	TOK_OP_INVERT		= 0x008004,		//	C++ operator ~

	//	C++ operators (0x01 0000 series, binary assignment)
	TOK_OP_EQ			= 0x010001,		//	C++ operator =
	TOK_OP_PLUSEQ		= 0x010002,		//	C++ operator +=
	TOK_OP_MINUSEQ		= 0x010003,		//	C++ operator -=
	TOK_OP_REMEQ		= 0x010004,		//	C++ operator %=
	TOK_OP_MULTEQ		= 0x010005,		//	C++ operator *=
	TOK_OP_DIVEQ		= 0x010006,		//	C++ operator /=
	TOK_OP_COMPLMENTEQ	= 0x010007,		//	C++ operator ~=
	TOK_OP_ANDEQ		= 0x010008,		//	C++ operator &=
	TOK_OP_OREQ			= 0x010009,		//	C++ operator |=
	TOK_OP_LSHIFTEQ		= 0x01000A,		//	C++ operator <<=
	TOK_OP_RSHIFTEQ		= 0x01000B,		//	C++ operator >>=

	//	C++ operators (0x02 0000 series, binary condititional)
	TOK_OP_LESSEQ		= 0x020001,		//	C++ operator <=
	TOK_OP_LESS			= 0x020002,		//	C++ operator <
	TOK_OP_MOREEQ		= 0x020003,		//	C++ operator >=
	TOK_OP_MORE			= 0x020004,		//	C++ operator >
	TOK_OP_TESTEQ		= 0x020005,		//	C++ operator ==
	TOK_OP_NOTEQ		= 0x020006,		//	C++ operator !=
	TOK_OP_COND_AND		= 0x020007,		//	C++ operator &&
	TOK_OP_COND_OR		= 0x020008,		//	C++ operator ||

	//	C++ operators (unary or binary add/sub)
	TOK_OP_PLUS			= 0x100001,		//	C++ operator +
	TOK_OP_MINUS		= 0x100002,		//	C++ operator -

	//	C++ operators (0x20---- series, binary, mult/div)
	TOK_OP_MULT			= 0x200001,		//	C++ operator *
	TOK_OP_DIV			= 0x200002,		//	C++ operator /
	TOK_OP_LSHIFT		= 0x200003,		//	C++ operator <<
	TOK_OP_RSHIFT		= 0x200004,		//	C++ operator >>
	TOK_OP_REM			= 0x200005,		//	C++ operator %

	//	C++ operators (0x40 0000 series, binary, add/sub)
	TOK_OP_LOGIC_AND	= 0x400001,		//	C++ operator &
	TOK_OP_LOGIC_OR		= 0x400002,		//	C++ operator |
	TOK_OP_LOGIC_EXOR	= 0x400003,		//	C++ operator ^
} ;

class	cePara
{
	//	Paragraph
	//
	//	Only comments comprise paragraphs. A paragraph begins as soon as non-whitespace is encountered. It ends with a double newline or when the comment itself ends. The paragraph
	//	indent is set at the start (first non-whitespace). This is compared to the indent of the comment as a whole and this comparison drives the generation of <p> and </p> tags.
	//
	//	Paragrahs are collected during the run and tested for similarity at the end.

public:
	cePara*		next ;			//	Next paragraph
	hzString	m_Content ;		//	Paragraph content
	hzString	m_File ;		//	File source of paragraph
	uint32_t	m_nLine ;		//	Paragraph line (in file)
	uint16_t	m_nCol ;		//	Paragraph column
	uint16_t	m_nCell ;		//	Cell number (column number in table)

	cePara	(void)	{ next = 0 ; m_nLine = m_nCol = m_nCell = 0 ; }
	~cePara	(void)	{}
} ;

class	ceToken
{
	//	The C++ token
	//
	//	The source and header files are tokenized according to the syntax rules of C++. They must be of one of the types described in the enum CppLex described above. These in turn
	//	are addined bitwise values allowing them to be categorized according to the #define series TOKTYPE_MASK.
	//
	//	Note that all tokens found in a source file are loaded in array P. The pre-processor resolves macros and #defines and removes all comments, leaving only active tokens (pure
	//	C++ code), in array X. The tokens in X are then parsed according to the syntax rules of C++.
	//
	//	In order for Code Enforcer to check that comments appear where expected and to examine them, active tokens in X are linked to adjacent comments where these exist in P. This
	//	is arranged by means of three variables, 'orig', 'comPre' and 'comPost', which are respectively the position of the token in P, that of the preceeding comment in P and that
	//	of the following comment in P. These will either be 0xffffffff if the comment does not exist or orig-1 and orig+1 respectively where the comment does. Not all active tokens
	//	have an orig value because they are generated in response to macros. The head token of the macro will however, have an orig value.
	//
	//	Active tokens can have both a preceeding and following comment but each comment can only be associated with one token. A comment then, is deemed to either preceed or follow
	//	a token, it cannot be both. The ultimate objective is to attach comments to C++ entities and to sections of code which either test or set entities. The HadronZoo commenting
	//	standards are assumed and to a lesser extent enforced. One strict rule is that comments must not occur within statements. Comments can only be placed before the first token
	//	of the statement or after the closing semi-colon. A comment directly after the opening curly brace of a code block will be applied to the code block, and is expected at the
	//	start of function bodies and class/struct definitions. A variable declaration expects the comment after the closing semi-colon and to begin on the same line. Application of
	//	these rules requires the token themselves to be attached to comments.

public:
	hzString	value ;			//	Only present when type merits
	uint32_t	strNo ;			//	String number
	uint32_t	match ;			//	Match ( to ), { to } and [ to ]
	uint32_t	orig ;			//	Match to posn in orig file
	uint32_t	comPre ;		//	Token number (posn in orig) for a preceeding comment
	uint32_t	comPost ;		//	Token number (posn in orig) for a following comment
	uint32_t	line ;			//	Line number of token for error reporting
	uint32_t	col ;			//	Column number
	uint16_t	codeLevel ;		//	Code Level of token (wihin levels of curly braces)
	uint16_t	t_argno ;		//	Misc (used by macros)
	uint16_t	t_excl ;		//	Excluded (comment or be excluded by compiler directive)
	uint16_t	t_cmtf ;		//	Comment format (comments only)
	CppLex		type ;			//	Token type or subtype

	ceToken	(void)
	{
		Clear() ;
	}

	void	Clear	(void)
	{
		//	Clears the token
		//	pParas = 0 ;
		value = (char*) 0 ;
		strNo = 0 ;
		orig = comPre = comPost = match = 0xffffffff ;
		line = 0 ;
		col = 0 ;
		type = TOK_UNKNOWN ;
		codeLevel = 0 ;
		t_argno = 0 ;
		t_excl = 0 ;
		t_cmtf = 0 ;
	}

	bool	IsLiteral	(void)	{ return type & TOKTYPE_MASK_LITERAL   ? true : false ; }
	bool	IsComment	(void)	{ return type & TOKTYPE_MASK_COMMENT   ? true : false ; }
	bool	IsDirective	(void)	{ return type & TOKTYPE_MASK_DIRECTIVE ? true : false ; }
	bool	IsKeyword	(void)	{ return type & TOKTYPE_MASK_KEYWORD   ? true : false ; }
	bool	IsVtype		(void)	{ return type & TOKTYPE_MASK_VTYPE     ? true : false ; }
	bool	IsStruct	(void)	{ return type & TOKTYPE_MASK_STRUCT    ? true : false ; }
	bool	IsCommand	(void)	{ return type & TOKTYPE_MASK_COMMAND   ? true : false ; }
	bool	IsOpAssign	(void)	{ return type & TOKTYPE_MASK_ASSIGN    ? true : false ; }
	bool	IsOpCond	(void)	{ return type & TOKTYPE_MASK_CONDITION ? true : false ; }
	bool	IsOpUnary	(void)	{ return type & TOKTYPE_MASK_UNARY     ? true : false ; }
	bool	IsOpBinary	(void)	{ return type & TOKTYPE_MASK_BINARY    ? true : false ; }
	bool	IsOpMultDv	(void)	{ return type & TOKTYPE_MASK_MATH_MLT  ? true : false ; }
	bool	IsOpAddSub	(void)	{ return type & TOKTYPE_MASK_MATH_ADD  ? true : false ; }
	bool	IsOperator	(void)	{ return type & TOKTYPE_MASK_OPERATOR  ? true : false ; }

	ceToken&	operator=	(const ceToken& op)
	{
		value = op.value ;
		strNo = op.strNo ;
		match = op.match ;
		orig = op.orig ;
		comPre = op.comPre ;
		comPost = op.comPost ;
		line = op.line ;
		col = op.col ;
		codeLevel = op.codeLevel ;
		t_argno = op.t_argno ;
		t_excl = op.t_excl ;
		t_cmtf = op.t_cmtf ;
		type = op.type ;
		return *this ;
	}

	//	Advance the iterator (bypassing comments)
	//	ceToken&	operator[]	(uint32_t oset) ;

	//	Return contents of value when this is known to be set
	const char*	Text	(void)	{ return *value ; }

	//	Return lex value of token in all other cases
	const char*	Show	(void) ;
} ;

hzEcode		Beautify	(hzVect<ceToken>& toklist, const hzString& filename) ;
hzEcode		Tokenize	(hzVect<ceToken>& toklist, const hzChain& C, const hzString& filename) ;
hzEcode		MatchTokens	(hzVect<ceToken>& toklist) ;

const char*	PrimaryTokType2Str	(CppLex eType) ;
const char*	Token2Str			(CppLex eType) ;

//	Concept:	Entity, Type and Scope.
//
//	A C++ entity can be any valid standard or user defined data type (including classes, structs, unions and enums), any variable or function (either independent or a member of any
//	of the above), any macro etc. In short an entity is anything that has been defined and that is legal within the confines of the C++ language.
//
//	Entities have relationships with other entities. Functions and variables can be members of entities and will evaluate to a value of a particular data type, which are themselves
//	entities. Entities and types are very closely related. Essentially any entity either IS or HAS a data type - or it is a template which will assume a type upon instantiation. By
//	template we do not just mean a template for a class or function. A macro is a form of template. Even a #define can act as a form of template although this might be stretching a
//	point. Ultimately any template, upon instantiation or translation, will amount to something that either is or has a type.

enum	ceEntType
{
	//	The ceEntType enumeration is a list of all the above entity types (marked with ENTITY) together with a list of codeEnforcer operational entities (marked with CE_UNIT_)

	//	Operationl non-C++ entities

	CE_UNIT_UNKNOWN,	//	Default value
	CE_UNIT_PROJECT,	//	The entity is a project
	CE_UNIT_COMPONENT,	//	The entity is a lirary or program (project component)
	CE_UNIT_FILE,		//	The entity is a source or header file
	CE_UNIT_SYNOPSIS,	//	The entity is a synopsis (description, inactive)
	CE_UNIT_CATEGORY,	//	Class/function category (for nav tree guidance only)
	CE_UNIT_CLASS_GRP,	//	Class group (for tying small classes together in a single publication item)
	CE_UNIT_FUNC_GRP,	//	Function set (for tying related but not overloaded, non-member functions together in a single publication item)

	//	ceDatatype - C++ Entities that are a data type.

	ENTITY_NAMSP,		//	Entity is a namespace
	ENTITY_CLASS,		//	Entity is a class
	ENTITY_UNION,		//	All unions regardless of scope
	ENTITY_ENUM,		//	Enumeration
	ENTITY_ENVAL,		//	Enumeration value
	ENTITY_CSTD,		//	A standard C++ type (non-composite)

	//	ceReal - C++ Entities that have a data type

	ENTITY_VARIABLE,	//	Only global variables and class member variables
	ENTITY_FUNCTION,	//	Entity is a function (class member or standalone)
	ENTITY_FN_OVLD,		//	A function group is a set of functions sharing the same name (overloaded).
	ENTITY_FNPTR,		//	Function pointer
	ENTITY_CFUNC,		//	A standard C++ function

	//	ceTmpl - Templates
	ENTITY_TMPLARG,		//	Class/Function template argument
	ENTITY_MACRO,		//	All macros
	ENTITY_DEFINE,		//	All #defines
	ENTITY_LITERAL,		//	A literal value (must be standard C++ type)
	ENTITY_TYPEDEF,		//	A typlex substitution based on any of the above types
} ;

enum	Scope
{
	//	All C++ entities have a scope in which they are defined but they also have range within thier scopes.

	SCOPE_UNKNOWN,		//	Scope is undefined
	SCOPE_GLOBAL,		//	Scope is global (not class member)
	SCOPE_STATIC_FILE,	//	Scope is limited to src file (not class member)
	SCOPE_FUNC,			//	Scope is limited to a function and cannot under any circumstances be refered to outside the function.
	SCOPE_PRIVATE,		//	Private (class members only). The entity (variable or function) is only accessible within class member functions.
	SCOPE_PROTECTED,	//	Protected (class members only). The entity is accessable within member function of the class and derivations of the class.
	SCOPE_PUBLIC,		//	Public (class members only). The entity is accessable outside the class
} ;

enum	Filetype
{
	//	Type of file

	FILE_UNKNOWN,		//	No file type
	FILE_HEADER,		//	File is a header
	FILE_SOURCE,		//	File is a source
	FILE_DOCUMENT,		//	File is a document (list of synopsis's)
	FILE_SYSINC,		//	System include file
	FILE_NOTE			//	File is for notes only
} ;

const char*	Filetype2Str	(Filetype e) ;

enum	EAttr
{
	//	Entity attributes
	EATTR_NULL			= 0x0,		//	Null attribute

	//	Class attributes
	CL_ATTR_STRUCT		= 0x000001,	//	Class is actually a struct
	CL_ATTR_TEMPLATE	= 0x000002,	//	Class is templated
	CL_ATTR_ABSTRACT	= 0x000004,	//	Class cannot be instansiated

	//	Function attributes
	FN_ATTR_GLOBAL		= 0x000010,	//	Function is global, not a class member
	FN_ATTR_CONSTRUCTOR	= 0x000020,	//	Function is a contructor
	FN_ATTR_DESTRUCTOR	= 0x000040,	//	Function is a destructor
	FN_ATTR_OPERATOR	= 0x000080,	//	Function is an operator
	FN_ATTR_STDFUNC		= 0x000100,	//	Function is standard member function
	FN_ATTR_STATIC		= 0x000200,	//	Function is static (to the class)
	FN_ATTR_FRIEND		= 0x000400,	//	Function is a friend (to the class)
	FN_ATTR_TEMPLATE	= 0x000800,	//	Function is templated

	//	Class member function attributes
	FN_ATTR_CONST		= 0x001000,	//	Set by 'const' appearing after the argument block.
	FN_ATTR_VIRTUAL		= 0x002000,	//	Set by 'virtual' appearing before the function declaration

	//	Printability
	EATTR_INTERNAL		= 0x010000,	//	Entity name begins with _, marking it as for 'internal support'. This will exclude it from the nav tree.
	EATTR_GRPSOLO		= 0x020000,	//	Function group has only one printable function
	EATTR_PRINTABLE		= 0x040000,	//	Function group has printable functions/Entity will produce a page
	EATTR_PRINTDONE		= 0x080000,	//	Any page for the entity has been exported to HTML.
} ;

enum	SType
{
	//	Statement type

	STMT_NULL,				//	No command

	//	Applicable at the file level
	STMT_USING,				//	Tells lookup functions to add a namespace to the search
	STMT_NAMESPACE,			//	Declares a namespace and makes it current (all things from now belong to it)
	STMT_TYPEDEF,			//	Typedef (typedef existing_type new_type)
	STMT_CLASS_DCL,			//	Class/Forward declaration of a class or struct
	STMT_CLASS_DEF,			//	Class definition
	STMT_CTMPL_DEF,			//	Class template definition
	STMT_UNION_DCL,			//	Forward declaration of a union
	STMT_UNION_DEF,			//	Union definition
	STMT_ENUM_DCL,			//	Forward declaration of an enum
	STMT_ENUM_DEF,			//	Enum definition
	STMT_FUNC_DCL,			//	Function prototype
	STMT_FUNC_DEF,			//	Function definition
	STMT_FTMPL_DEF,			//	Function template definition

	//	Variable declarations
    STMT_VARDCL_FNPTR,		//	Function ptr declaration:					Of the form [keywords] typlex (*var_name)(type_args) ;
    STMT_VARDCL_FNASS,		//	Function ptr declation and assignement:		Of the form [keywords] typlex (*var_name)(type_args) = function_addr ;
    STMT_VARDCL_STD,		//	Variable declarations:						Of the form [keywords] typlex var_name ;
    STMT_VARDCL_ASSIG,		//	Variable declaration and assignment:		Of the form [keywords] typlex var_name = exp ;
    STMT_VARDCL_ARRAY,		//	Variable array declarations:				Of the form [keywords] typlex var_name[noElements] ;
    STMT_VARDCL_ARASS,		//	Variable array declaration and assignment:	Of the form [keywords] typlex var_name[noElements] = { values } ;
    STMT_VARDCL_CONS,		//	Variable declarations: 						Of the form [keywords] class/typlex var_name(value_args) ;
 
	//	Branches
    STMT_BRANCH_IF,			//	If:                     Of the form if (condition) then statement/statement block
    STMT_BRANCH_ELSE,		//	Else:                   An 'else' then statement/statement block. The 'else' must follow an if or an else
    STMT_BRANCH_ELSEIF,		//	Else:                   An 'else' then statement/statement block. The 'else' must follow an if or an else
    STMT_BRANCH_FOR,		//	For:                    Of the form for (statements ; condition ; statements) followed by statement/statement block
    STMT_BRANCH_DOWHILE,	//	Do while:               Of the form do statement block while (condition)
    STMT_BRANCH_WHILE,		//	While:                  Of the form while (condition) then statement/statement block
    STMT_BRANCH_SWITCH,		//	Switches:               Of the form switch (value) then block of switch cases
    STMT_BRANCH_CASE,		//	Switch cases:           Of the form case value: then series of statements

	//	Operations
    STMT_VAR_INCA,			//	Variable increments:    Of the form var++
    STMT_VAR_INCB,			//	Variable increments:    Of the form ++var
    STMT_VAR_DECA,			//	Variable decrements:    Of the form var--
    STMT_VAR_DECB,			//	Variable decrements:    Of the form --var
    STMT_VAR_ASSIGN,		//	Variable assignments:   Of the form var = evalutable expression
    STMT_VAR_MATH,			//	Variable operations:    Of the form var +=, -=, *= or /= etc followed by an evalutable expresion
    STMT_FUNC_CALL,			//	Function calls:         Of the form funcname(args)
    STMT_DELETE,			//	Delete variable:        Of the form delete varname ;

	//	Jumps and exits
	STMT_CONTINUE,			//	19) To top of for/while loop	
	STMT_BREAK,				//	20) Break out of loop or case block
	STMT_GOTO,				//	21) Goto label
	STMT_RETURN				//	22) Break out of function
} ;


#define	CPP_DT_INT	0x0100	//	Signed integer
#define	CPP_DT_UNT	0x0200	//	Signed integer

const char*	Statement2Txt	(SType eS) ;

class	ceComp ;
class	ceFile ;

class	ceFrame
{
	//	The ceFrame base class.

protected:
	ceComp*		m_pParComp ;	//	Project component to which this entity belongs
	hzString	m_Desc ;		//	Description
	hzString	m_Name ;		//	Name
	uint32_t	m_Uid ;			//	Unique id
	uint32_t	m_Id ;			//	Genre id

public:
	ceFrame		(void)	{ m_pParComp = 0 ; m_Uid = 0 ; m_Id = 0 ; }
	~ceFrame	(void)	{}

	//	Set functions
	void	SetParComp	(ceComp* pComp)			{ m_pParComp = pComp ; }
	void	SetName		(const hzString& name)	{ m_Name = name ; }
	void	SetDesc		(const hzChain& C)		{ _hzfunc("SetDesc(hzChain&)") ; m_Desc = C ; }
	void	SetDesc		(const hzString& S) ;

	//	Get functions
	const hzString&	StrName		(void) const	{ return m_Name ; }
	const hzString&	StrDesc		(void) const	{ return m_Desc ; }
	const char*		TxtName		(void) const 	{ return *m_Name ; }
	const char*		TxtDesc		(void) const	{ return *m_Desc ; }
	const char*		EntDesc		(void) const ;
	ceComp*			ParComp		(void) const	{ return m_pParComp ; }

	virtual	ceEntType	Whatami	(void) const	{ return CE_UNIT_UNKNOWN ; }
} ;

class	ceSynp	: public ceFrame
{
	//	Synopsis

public:
	hzChain		m_Content ;		//	Content (allows larger texts than hzStr)
	hzString	m_Docname ;		//	Article name (with 'sy' pretext)
	hzString	m_Parent ;		//	Chapter and paragraph number

	void	SynopInit	(ceFile* pFile, const hzString& order, const hzString& name) ;
} ;


class	ceNamsp ;

class	ceEntity	: public ceFrame
{
	//	Entitiy base class

protected:
	ceNamsp*	m_pNamsp ;		//	Namespace entity defined within
	hzString	m_fqname ;		//	Fully qualified name (if member of class or struct)
	uint32_t	m_UEID ;		//	Unique entity id
	uint32_t	m_Entattr ;		//	Attributes (means different things to different entities)
	Scope		m_Scope ;		//	Scope to be applied (compulsory for all entities along with m_parent)

public:
	ceEntity	(void) ;

	//	Set functios
	void	SetAttr		(EAttr attr)		{ m_Entattr |= (uint32_t) attr ; }
	void	SetScope	(Scope range)		{ m_Scope = range ; }

	//	Get functions
	hzString&	Fqname	(void)	{ return m_fqname ; }

	ceNamsp*	Namespace	(void) const	{ return m_pNamsp ; }
	Scope		GetScope	(void) const	{ return m_Scope ; }
	EAttr		GetAttr		(void) const	{ return (EAttr) m_Entattr ; }
	uint32_t	GetUEID		(void) const	{ return m_UEID ; }

	virtual	ceEntType	Whatami		(void) const = 0 ;
	virtual bool		IsType		(void) const = 0 ;
	virtual bool		IsReal		(void) const = 0 ;
} ;

class	ceEntbl
{
	//	Entity table

	hzMapS	<hzString,ceEntity*>	m_ents ;	//	Must be a hzMapS as entities within the same scope cannot share names

	ceFrame*	applied ;		//	Code Enforcer object hosting this scope
	ceEntbl*	m_parent ;		//	Only used in functions where declarations occur in nested code blocks (or for loops)

public:
	ceEntbl	(void)
	{
		m_ents.SetDefaultObj((ceEntity*)0) ;
		applied = 0 ;
		m_parent = 0 ;
	}

	void	InitET	(ceFrame* pAD)
	{
		if (applied)
			Fatal("ceEntbl::InitET - Already applied %s\n", applied->TxtName()) ;
		applied = pAD ;
	}

	ceFrame*	GetApplied	(void)	{ return applied ; }

	const hzString	StrName	(void)	{ return applied ? applied->StrName() : _hzGlobal_nullString ; }
	const char*		TxtName	(void)	{ return applied ? applied->TxtName() : 0 ; }

	hzEcode		AddEntity		(ceFile* pFile, ceEntity* pE, const char* caller = 0) ;
	void		EntityExport	(hzChain& Z, ceComp* pComp, uint32_t recurse) ;

	//	Get entities from table
	uint32_t	Count		(void) const		{ return m_ents.Count() ; }
	ceEntbl*	Parent		(void) const		{ return m_parent ; }
	ceEntity*	GetEntity	(uint32_t n) const	{ return m_ents.GetObj(n) ; }
	ceEntity*	GetEntity	(const hzString& s)	{ return m_ents[s] ; }
} ;

class	ceNamsp	: public ceEntity
{
	//	Namespace.

public:
	ceEntbl		m_ET ;

	ceNamsp		(void) ;
	~ceNamsp	(void) ;

	ceEntType	Whatami	(void) const	{ return ENTITY_NAMSP ; }

	void	Init	(const hzString& name, const hzString& desc)
	{
		m_Name = name ;
		m_Desc = desc ;
	}

	ceEntity*	GetEntity	(uint32_t nIndex) const	{ return m_ET.GetEntity(nIndex) ; }
	ceEntity*	GetEntity	(hzString& ename)		{ return m_ET.GetEntity(ename) ; }

	bool	IsType	(void) const	{ return false ; }
	bool	IsReal	(void) const	{ return false ; }
} ;

/*
**	DATA TYPES
*/

class	ceFunc ;
class	ceTarg ;	//	Forward decl of template argument

enum	ceBasis
{
	ADT_NULL	= 0,		//	used to indicate illegal type mix
	ADT_VOID	= 0x0001,	//	Type void
	ADT_ENUM	= 0x0002,	//	Positive integer that cannot take part in arithmetic operations
	ADT_BOOL	= 0x0003,	//	either true or false
	ADT_STRING	= 0x0004,	//	Any string, treated as a single value
	ADT_TEXT	= 0x0005,	//	Any string, treated as a series of words, stored on disk, frequent change
	ADT_DOUBLE	= 0x0006,	//	64 bit floating point value
	ADT_INT64	= 0x0101,	//	64-bit Signed integer
	ADT_INT32	= 0x0102,	//	32-bit Signed integer
	ADT_INT16	= 0x0103,	//	16-bit Signed integer
	ADT_BYTE	= 0x0104,	//	8-bit Signed integer
	ADT_UNT64	= 0x0201,	//	64-bit Positive integer
	ADT_UNT32	= 0x0202,	//	32-bit Positive integer
	ADT_UNT16	= 0x0203,	//	16-bit Positive integer
	ADT_UBYTE	= 0x0204,	//	8-bit Positive integer
	ADT_CLASS	= 0x1001,	//	Class or class template.
	ADT_TMPLARG	= 0x1002,	//	Template argument.
	ADT_UNION	= 0x1003,	//	Union.
	ADT_VARG	= 0x1004,	//	Variable argument.
} ;

class	ceDatatype	: public ceEntity
{
	//	The ceDatatype class is the constructed data type of a real entity. This is necessary because although the base or fundamental data type suffices in most cases, it does not
	//	describe abstract constructs such as class templates or the complex data types that arise with manifestation of a class template.  

public:
	hzList<ceFunc*>	m_Ops ;		//	Map of tokens to operator functions

	ceBasis		m_Basis ;		//	Indicator of composite or fundamental data type
	uint16_t	m_Order ;		//	Used only by template arguments
	uint16_t	m_Size ;		//	Size of the data type

	virtual ceBasis	Basis	(void) const = 0 ;

	virtual bool	IsType	(void) const	{ return true ; }
	virtual bool	IsReal	(void) const	{ return false ; }
} ;

enum	DAttr
{
	//	Datatype attributes. These are used to set the operational context in 'typlexes' (fully qualified data types).

	DT_ATTR_NULL		= 0x0000,	//	No attributes set
	DT_ATTR_TEMPLATE	= 0x0001,	//	The data type is templated
	DT_ATTR_TMPLARG		= 0x0002,	//	The data type is a template arg
	DT_ATTR_VARARG		= 0x0004,	//	The data type is a template arg
	DT_ATTR_STATIC		= 0x0008,	//	The data type implies the object is static
	DT_ATTR_CONST		= 0x0010,	//	The data type is readonly once set
	DT_ATTR_SYSTEM		= 0x0020,	//	The data type is a C++ standard and cannot be processed by Code Enforcer
	DT_ATTR_LITERAL		= 0x0040,	//	The data type is a literal value (has no operators)
	DT_ATTR_ZERO		= 0x0080,	//	The special case of literal zero which can be used as a numeric or pointer value
	DT_ATTR_LVALUE		= 0x0100,	//	The special case of literal zero which can be used as a numeric or pointer value
	DT_ATTR_REFERENCE	= 0x0200,	//	The typlex is a reference to an instance
	DT_ATTR_FNPTR		= 0x0400,	//	The typlex is a function pointer
} ;

class	ceTyplex
{
	//	The ceTyplex class provides the 'typlex' (fully qualified data type), of a real entity. It combines the base data type (ceDatatype) with the operational context (DAttr) and
	//	states the entity to be (a) an instance, (b) a reference to an instance, (c) a pointer to an instance, OR (d) an array or multi-dimensional collection of either (a), (b) or
	//	(c).
	//
	//	Typlexes can be complex! For a class template (operational context DT_ATTR_TEMPLATE), the base data type will be the template itself but the data type that actually applies
	//	in a given instance, is qualified by template arguments. Where the data type is a pointer to a function (operational context DT_ATTR_FNPTR), the base data type will be the
	//	return data type of the function and m_Args will be the typlexes of the function arguments. 

	ceDatatype*	m_pType ;			//	General data type

public:
	hzList	<ceTyplex>	m_Args ;	//	If m_pType is a class template, then this is used to specify the class

	uint32_t	m_nElements ;		//	Number of elements, 1 not an array, >1 an array
	uint16_t	m_Indir ;			//	0 instance, -1 ref, 1 *, 2 ** etc
	uint16_t	m_dt_attrs ;		//	Special attributes

	ceTyplex	(void)
	{
		Clear() ;
	}

	ceTyplex	(const ceTyplex& op)
	{
		operator=(op) ;
	}

	~ceTyplex	(void)
	{
		Clear() ;
	}

	void	Clear	(void)
	{
		m_Args.Clear() ;
		m_pType = 0 ;
		m_nElements = 1 ;
		m_Indir = 0 ;
		m_dt_attrs = DT_ATTR_NULL ;
	}

	void	SetType	(ceDatatype* pDT)	{ m_pType = pDT ; }
	void	SetAttr	(DAttr eA)			{ m_dt_attrs |= eA ; }
	bool	IsAttr	(DAttr eA) const	{ return m_dt_attrs & eA ? true : false ; }

	ceTyplex&	operator=	(const ceTyplex& op) ;

	hzString	Str		(void) const ;

	bool	Same	(const ceTyplex& supp) const ;
	hzEcode	Testset	(const ceTyplex& test) const ;

	bool	operator==	(const ceTyplex& op) const ;
	bool	operator!=	(const ceTyplex& op) const	{ return !operator==(op) ; }

	bool	IsTemplate	(void) const	{ return m_dt_attrs & DT_ATTR_TEMPLATE ? true : false ; }
	bool	IsTmplarg	(void) const	{ return m_dt_attrs & DT_ATTR_TMPLARG ? true : false ; }
	bool	IsVararg	(void) const	{ return m_dt_attrs & DT_ATTR_VARARG ? true : false ; }
	bool	IsStatic	(void) const	{ return m_dt_attrs & DT_ATTR_STATIC ? true : false ; }
	bool	IsConst		(void) const	{ return m_dt_attrs & DT_ATTR_CONST ? true : false ; }
	bool	IsSystem	(void) const	{ return m_dt_attrs & DT_ATTR_SYSTEM ? true : false ; }
	bool	IsLiteral	(void) const	{ return m_dt_attrs & DT_ATTR_LITERAL ? true : false ; }
	bool	IsZero		(void) const	{ return m_dt_attrs & DT_ATTR_ZERO ? true : false ; }
	bool	IsLval		(void) const	{ return m_dt_attrs & DT_ATTR_LVALUE ? true : false ; }
	bool	IsReference	(void) const	{ return m_dt_attrs & DT_ATTR_REFERENCE ? true : false  ; }
	bool	IsFnptr		(void) const	{ return m_dt_attrs & DT_ATTR_FNPTR ? true : false  ; }

	ceDatatype*	Type	(void) const	{ return m_pType ; }
	ceBasis		Basis	(void) const	{ return m_pType ? m_pType->Basis() : ADT_NULL ; }
	ceEntType	Whatami	(void) const	{ return m_pType ? m_pType->Whatami() : CE_UNIT_UNKNOWN ; }
} ;

class	ceReal	: public ceEntity
{
	//	While ceDatatype and ceTyplex ARE data types, derivatives of ceReal HAVE a data type. E.g. a variable is real because it has a value that can be read. A
	//	function is real because it returns a value.

public:
	ceTyplex	m_Tpx ;			//	Basic evaluation class/struct, union, enum or standard C++ type

	const ceTyplex&	Typlex	(void)	{ return m_Tpx ; }

	virtual bool	IsType	(void) const	{ return false ; }
	virtual bool	IsReal	(void) const	{ return true ; }
} ;

//	Code Enforcer Statements
//
//	Program statement within a function body. These are classified as follows:-
//
//		1)	Statements that declare variables
//		2)	Statements that control branching
//		3)	Statements that call a function, operate on variables or evaluate an expression of variables
//
//	Note that statements point to the next statement in the series (code block) and a null pointer marks the end of the series. Only statements of IF, ELSE, DO, FOR, WHILE, SWITCH
//	and CASE will point to a branch.
//
//	Notes regarding IF, ELSE_IF and IF statements, which may be of the following forms:-
//
//		1) if (condition) series_of_statement(s);
//			This generates an IF statement with the branch pointer set to the statement or series of statements executed if the condition is true.
//
//		2) else if (condition) series_of_staement(s);
//			This generates an ELSE_IF statement with the branch pointer set to the conditional statement(s). It must follow either an IF or an ELSE_IF
//			statement.
//
//		3) else series;
//			This generates an ELSE statement with the branch pointer set to the statement or series of statements. The ELSE must directly follow an IF
//			or an ELSE_IF.
//
//	Conditional expresions of the form (condition) ? expression-A : expression-B. Technically these are the equivelent of if (condition) series-A else series-B; If expresion-B is
//	replaced by (condition2) ? expression-B : expression-C the statement becomes the equivelent of if (condition) series-A else if (condition2) series-B else series-C;
//
//	Note that the technical equivelence does not work in all case. Expressions and statements are not the same thing. The starting point is always the 	statement. If this statement
//	does not contain an expression then it will generate a single ceStmt instance. The same is true if the statement contains an expression but not if the terms in that expression
//	themselves contain statements.
//
//	Nor is there always a direct lexical equivelence. The statement return ptr ? E_OK:E_NOINIT; cannot be written as return if (ptr) E_OK else E_NOINIT; It may be instead become if
//	(ptr) return E_OK; else return E_NOINIT; Or because we are returning, the else can be omitted. This is important because in Code Enforcer, return points within a function are
//	assessed in order to determine the set of possible outcomes which are to be listed as part of the generated documentation.
//
//	In the case of:- if (condition1) series-A; else if (condition2) series-B; else series-C, this gives rise to an IF, an ELSE_IFs and an ELSE. There may be an umlimited number of
//	ELSE_IF statements in a series and the final ELSE may be missing (legal but considered to be poor coding standards).
//
//	Note that the FOR equates to (init-series; condition; final-series) { main_series; } and WHILE equates to (condition) { main-series; }

class	ceCodent
{
	//	Code entity. Base class for ceCbody (code body) and ceStmt (statement)
public:
	virtual int32_t	Whatami	(void) const = 0 ;
} ;

class	ceStmt	: public ceCodent
{
	//	Standard statement

public:
	hzString	m_Pretext ;		//	Preceeding comment
	hzString	m_Comment ;		//	Primary comment
	hzString	m_Object ;		//	Object, operation description or condition
	uint32_t	m_nLine ;		//	Line number of instruction
	uint32_t	m_Start ;		//	Token position of statement start
	uint32_t	m_End ;			//	Token position of statement end
	uint32_t	m_nLevel ;		//	Level of instruction (indents)
	SType		m_eSType ;		//	Type of instruction
    bool		m_bReturn ;		//	Is the statement a function return point

	ceStmt	(void)	{ Clear() ; }

	void	Clear	(void)
	{
		m_eSType = STMT_NULL ;
		m_nLevel = m_nLine = m_Start = m_End = 0 ;
		m_bReturn = false ;
	}

	ceStmt&	operator=	(const ceStmt& op)
	{
		m_Pretext = op.m_Pretext ;
		m_Comment = op.m_Comment ;
		m_Object = op.m_Object ;
		m_nLevel = op.m_nLevel ;
		m_nLine = op.m_nLine ;
		m_Start = op.m_Start ;
		m_End = op.m_End ;
		m_eSType = op.m_eSType ;
		m_bReturn = op.m_bReturn ;
		return *this ;
	}

	int32_t	Whatami	(void) const	{ return 1 ; }
} ;

class	ceCbody	: public ceCodent
{
	//	Code body
public:
	ceCodent*	parent ;		//	Parent code body
	ceEntbl*	m_pET ;			//	For variables declared within the code body
	ceStmt*		m_Stmts ;		//	Only applicable to IF, 

	ceCbody	(void)
	{
		parent = 0 ;
		m_pET = 0 ;
		m_Stmts = 0 ;
	}

	int32_t	Whatami	(void) const	{ return 2 ; }
} ;

/*
**	Class and class templates
*/

class	ceKlass	: public ceDatatype
{
	//	This is a pure virtual class to tie together ceClass and ceUnion

public:
	hzVect	<ceStmt>	m_Statements ;	//	All statements assciated with the class definition body
	hzList	<ceFunc*>	m_Funcs ;		//	All functions

	ceEntbl		m_ET ;					//	All entities in the class/ctmpl/union
	ceKlass*	m_ParKlass ;			//	Parent class (parent of a class can be a class or class template)

	ceKlass*	ParKlass	(void) const	{ return m_ParKlass ; }
	ceBasis		Basis		(void) const	{ return m_Basis ; }

	virtual bool	IsType	(void) const	{ return true ; }
	virtual bool	IsReal	(void) const	{ return false ; }
} ;

/*
**	Class definition
*/

class	ceCpFn ;

class	ceTarg	: public ceDatatype
{
	//	A template argument. These 'belong' to a class or function template and typically have names like 'OBJ' and 'KEY'. Within the function template or class template definition
	//	these arguments are treated as though they were meaningful data types in their own right. They must take up a real data type (a typlex) when used by real entities.

public:
	ceTarg	(void)	{ m_Scope = SCOPE_PUBLIC ; }

	hzEcode	Init	(const hzString& name)
	{
		m_Order = 0xffff ;
		m_Size = 0 ;
		SetName(name) ;
		return E_OK ;
	}

	ceTarg&	operator=	(const ceTarg& op)
	{
	}

	ceBasis		Basis	(void) const	{ return ADT_TMPLARG ; }
	bool		IsType	(void) const	{ return true ; }
	ceEntType	Whatami	(void) const	{ return ENTITY_TMPLARG ; }
} ;

class	ceClass	: public ceKlass
{
	ceClass*	m_pBase ;				//	Base class if applicable
	ceFile*		m_pFileDef ;			//	File in which class is defined
	hzString	m_Docname ;				//	Article name (with 'cl' pretext)
	hzString	m_Category ;			//	Group name
	uint32_t	m_DefTokStart ;			//	First token of entity definition (used to output def bodies to HTML)
	uint32_t	m_DefTokEnd ;			//	Last token of entity definition

public:
	//	hzVect	<ceStmt>	m_Statements ;	//	All statements assciated with the class definition body
	//	hzList	<ceFunc*>	m_Funcs ;		//	All functions
	hzList	<ceFunc*>	m_Friends ;		//	Friend functions
	hzArray	<ceTarg*>	m_ArgsTmpl ;	//	List of class arguments (templates only)

	ceClass	(void) ;

	hzEcode	InitClass	(ceFile* pFile, ceClass* pBase, ceKlass* pParent, const hzString& name, Scope scope, bool bStruct) ;
	void	SetBase		(ceClass* pBase)	{ m_pBase = pBase ; }
	void	SetCateg	(const hzString& S)	{ m_Category = S ; }

	ceEntity*	GetEntity	(uint32_t nIndex) const	{ return m_ET.GetEntity(nIndex) ; }
	ceEntity*	GetEntity	(hzString& ename)		{ return m_ET.GetEntity(ename) ; }

	ceClass*	Base		(void) const	{ return m_pBase ; }
	ceFile*		DefFile		(void) const	{ return m_pFileDef ; }
	hzString	Artname		(void) const	{ return m_Docname ; }
	hzString	Category	(void) const	{ return m_Category ; }
	uint32_t	GetDefStart	(void) const	{ return m_DefTokStart ; }
	uint32_t	GetDefEnd	(void) const	{ return m_DefTokEnd ; }
	bool		IsStruct	(void) const	{ return m_Entattr & CL_ATTR_STRUCT ? true : false ; }
	ceEntType	Whatami		(void) const	{ return ENTITY_CLASS ; }
	bool		IsType		(void) const	{ return true ; }
	bool		IsReal		(void) const	{ return false ; }
} ;

class	ceCStd	: public ceDatatype
{
	//	Standard C++ types

	ceBasis	m_Real ;
public:
	ceCStd	(void)	{ m_Real = ADT_NULL ; m_Scope = SCOPE_GLOBAL ; }
	~ceCStd	(void)	{}

	ceBasis		Basis	(void) const	{ return m_Real ; }
	ceEntType	Whatami	(void) const	{ return ENTITY_CSTD ; }

	hzEcode	Init	(const hzString& name, ceBasis eDT)
	{
		m_Name = m_fqname = name ;
		m_Real = eDT ;
		return E_OK ;
	}
} ;

class	ceFile ;

/*
**	ceEnum class
*/

class	ceEnumval ;

class	ceEnum	: public ceDatatype
{
	ceFile*		m_pFileDef ;		//	File in which enum is defined
	hzString	m_Docname ;			//	Article name (with 'en' pretext)
	hzString	m_Category ;		//	General category of function (for tree asthetics)
	uint32_t	m_DefTokStart ;		//	First token of entity definition (used to output def bodies to HTML)
	uint32_t	m_DefTokEnd ;		//	Last token of entity definition

public:
	hzMapS	<hzString,ceEnumval*>	m_ValuesByLex ;		//	Enum values by name
	hzMapS	<int32_t,ceEnumval*>	m_ValuesByNum ;		//	Enum values by number

	ceEnum	(void)
	{
		m_Scope = SCOPE_GLOBAL ;
		m_pFileDef = 0 ;
		m_DefTokStart = 0 ;
		m_DefTokEnd = 0 ;
	}

	hzEcode	InitEnum	(ceFile* pFile, const hzString& name, uint32_t start, uint32_t ultimate, Scope scope = SCOPE_GLOBAL) ;

	void	SetCateg	(const hzString& S)	{ m_Category = S ; }

	const char*	DefFname	(void) const ;

	ceFile*		DefFile		(void) const	{ return m_pFileDef ; }
	hzString	Artname		(void) const	{ return m_Docname ; }
	hzString	Category	(void) const	{ return m_Category ; }
	uint32_t	GetDefStart	(void) const	{ return m_DefTokStart ; }
	uint32_t	GetDefEnd	(void) const	{ return m_DefTokEnd ; }
	ceBasis		Basis		(void) const	{ return ADT_ENUM ; }
	ceEntType	Whatami		(void) const	{ return ENTITY_ENUM ; }
} ;

class	ceEnumval	: public ceReal
{
	//	Enums

	ceEnum*	m_ParEnum ;		//	Parent enum

public:
	hzString	m_txtVal ;		//	Enum value as a string
	int32_t		m_numVal ;		//	Numerical equivelent

	ceEnumval	(void)
	{
		m_Scope = SCOPE_GLOBAL ;
		m_numVal = -1 ;
	}

	hzEcode	InitEnval	(ceEnum* parent, const hzString& name) ;

	ceEnum*		ParEnum	(void) const	{ return m_ParEnum ; }
	ceEntType	Whatami	(void) const	{ return ENTITY_ENVAL ; }
} ;

/*
**	ceVar definition
*/

class	ceVar	: public ceReal
{
	//ceTyplex	m_Tpx ;			//	Basic evaluation class/struct, union, enum or standard C++ type

public:
	hzAtom		m_atom ;		//	Evaluation (will be a number or a string value)
	ceFile*		m_pFileDcl ;	//	File in which the variable is declared ...
	ceKlass*	m_ParKlass ;	//	Parent class or class template (if applicable).

	ceVar	(void)
	{
		m_Scope = SCOPE_UNKNOWN ;
		m_pFileDcl = 0 ;
		m_ParKlass = 0 ;
	}

	const ceTyplex&	Typlex	(void)	{ return m_Tpx ; }

	hzEcode	InitAsArg	(ceTyplex& tpx, Scope range, const hzString& name) ;
	hzEcode	InitDeclare	(ceFile* pFile, ceKlass* pKlass, ceTyplex& tpx, Scope range, const hzString& name) ;

	bool	operator==	(ceVar& op)
	{
		return m_Tpx == op.m_Tpx ;
	}

	ceEntType	Whatami	(void) const	{ return ENTITY_VARIABLE ; }
} ;

class	ceCpFn	: public ceReal
{
	//	Standard C++ function

public:
	//ceTyplex	m_Return ;		//	Evaluation of the function

	void	InitCpFn	(ceTyplex& retTpx, hzString name)	{ m_Tpx = retTpx, m_Scope = SCOPE_PUBLIC ; m_Name = name ; }

	ceEntType	Whatami		(void) const	{ return ENTITY_CFUNC ; }
} ;

class	ceFnset ;
class	ceFngrp ;

class	ceFunc	: public ceReal
{
	//	Represents a C++ function, either standalone or as a class method
	//
	//	Functions can be overloaded so there can be several functions of the same name and if a class member, the same fully qualified name. In order to uniquely identify functions
	//	the name is extended to include the argument typlexes.

	//ceTyplex	m_Return ;				//	Evaluation of the function
	ceFile*		m_pFileDcl ;			//	File in which function is declared
	ceFile*		m_pFileDef ;			//	File in which function is defined
	hzString	m_Category ;			//	General category of function (for tree asthetics)
	hzString	m_htmlProto ;			//	Part HTML for function prototype
	hzString	m_hmtlBody ;			//	Part HTML for function definition body (within a <pre> tag)
	hzString	m_Docname ;				//	Article name (with 'fg' pretext)
	hzString	m_exname ;				//	Full name with argument typlexes

public:
	hzVect	<ceStmt>	m_Execs ;		//	List of instructions to execute
	hzList	<ceVar*>	m_Args ;		//	Argument list
	hzList	<ceTarg*>	m_Targs ;		//	Template arguments translators if applicable
	hzList	<hzPair>	m_Argdesc ;		//	Argument descriptors from comments
	hzList	<hzPair>	m_Retdesc ;		//	Return descriptors from comments
	hzList	<ceClass*>	m_Friends ;		//	Classes that have declared this function as a friend

	ceFnset*	m_pSet ;				//	Set to which this function belongs (if any)
	ceFngrp*	m_pGrp ;				//	Pointer to function group
	ceCbody*	m_pBody ;				//	Code body
	ceKlass*	m_ParKlass ;			//	Parent class or class template
	uint32_t	m_DefTokStart ;			//	First token of entity definition (opening curly brace)
	uint32_t	m_DefTokEnd ;			//	Final token of entity definition (closing curly brace)
	uint16_t	m_minArgs ;				//	Will be less than m_Args.Count() is one or more args have defaults
	uint16_t	m_rating ;				//	Used by ProcFuncCall to assess parameter matching
	bool		m_bVariadic ;			//	The last argument is vararg

	ceFunc	(void)
	{
		m_pSet = 0 ;
		m_pGrp = 0 ;
		m_pBody = 0 ;
		m_ParKlass = 0 ;
		m_pFileDcl = 0 ;
		m_pFileDef = 0 ;
		m_DefTokStart = 0 ;
		m_DefTokEnd = 0 ;
		m_minArgs = 0 ;
		m_bVariadic = false ;
	}

	hzEcode	InitFuncDcl	(ceFile* pDcl, ceKlass* parent, ceTyplex& tpx, hzList<ceVar*>& args, Scope scope, const hzString& name) ;
	hzEcode	InitFuncDef	(ceFile* pDef, uint32_t start, uint32_t end) ;

	//	Insert descriptive HTML
	hzEcode	GetHtmlProto	(hzChain& Z) ;
	hzEcode	GetHtmlBody		(hzChain& Z) ;

	hzEcode	SetCategory	(const hzString& S) ;	//	{ m_Category = S ; }

	hzEcode	SetReturn	(const ceTyplex& tpx)
	{
		if (m_Tpx.Type())
			return E_CONFLICT ;
		m_Tpx = tpx ;
		return E_OK ;
	}

	const char*	DclFname	(void) const ;
	const char*	DefFname	(void) const ;

	const ceTyplex&	Return	(void) const	{ return m_Tpx ; }

	ceKlass*	ParKlass	(void) const	{ return m_ParKlass ; }
	ceFile*		DclFile		(void) const	{ return m_pFileDcl ; }
	ceFile*		DefFile		(void) const	{ return m_pFileDef ; }
	hzString	Artname		(void) const	{ return m_Docname ; }
	hzString	Extname		(void) const	{ return m_exname ; }
	uint32_t	GetDefStart	(void) const	{ return m_DefTokStart ; }
	uint32_t	GetDefEnd	(void) const	{ return m_DefTokEnd ; }
	hzString	GetCategory	(void) const ;	//	{ return m_Category ; }
	ceEntType	Whatami		(void) const	{ return ENTITY_FUNCTION ; }
} ;

class	ceFngrp	: public ceEntity
{
	//	All functions belong to a function group, to deal with overloaded functions where these arise. The policy for article generation, is that if an external comment is created
	//	for the function group, then all functions belonging to the group will appear in a single page.

public:
	hzList	<ceFunc*>	m_functions ;	//	All functions in the group
	hzList	<hzPair>	m_Argdesc ;		//	Argument descriptions
	hzList	<hzPair>	m_Retdesc ;		//	Returns descriptions

	hzString	m_Docname ;		//	Article name (with 'fs' pretext)
	hzString	m_Title ;		//	Name is always the generated article name (for uniqueness) but title can be anything appropriate.
	hzString	m_Category ;	//	General category of function (for tree asthetics)
	hzString	m_grpDesc ;		//	Description (overall)

	ceFngrp		(void) ;
	~ceFngrp	(void) ;
 
	ceFngrp&	operator=	(ceFngrp& op)
	{
		m_functions = op.m_functions ;
		m_Argdesc = op.m_Argdesc ;
		m_Retdesc = op.m_Retdesc ;
		m_Docname = op.m_Docname ;
		m_Title = op.m_Title ;
		m_Category = op.m_Category ;
		m_grpDesc = op.m_grpDesc ;
		return *this ;
	}

	void	SetCateg	(const hzString& S)	{ m_Category = S ; }

	bool		IsType	(void) const	{ return false ; }
	bool		IsReal	(void) const	{ return true ; }
	ceEntType	Whatami	(void) const	{ return ENTITY_FN_OVLD ; }
} ;

class	ceFnset
{
	//	A function set is explicitly defined in the source code documentation as an external comment (to any function or class defineition). It serves to group functions together,
	//	which would not be grouped by overloading. Usually because the functions have the same or very similar functionality or are otherwise strongly related. The effect is that
	//	instead of each function having a main description page pointed to by an entry in the navtree, there is only one navtree entry pointing to a description page entry for the
	//	whole group.
	//
	//	Note that this does not suppress generation of pages for the indivdual functions. These are still produced and links to them are given in the group page but these pages
	//	are not given navtree entries.
	//
	//	Note that a function set directive appears as a C++ comment containing the following fields:-
	//
	//	1)	FnSet:		function set name
	//	2)	Category:	The function category
	//	3)	Description (of the function set)
	//	4)	Func:		There must be one of these for each function in the set.
	//
	//	Note alse that the functions in the set must still have headline comments in their own right.

public:
	hzList	<ceFunc*>	m_functions ;	//	All functions in the set
	hzList	<hzPair>	m_Argdesc ;		//	Argument descriptions
	hzList	<hzPair>	m_Retdesc ;		//	Argument descriptions

	hzString	m_Docname ;		//	Article name (with 'fs' pretext)
	hzString	m_Title ;		//	Name is always the generated article name (for uniqueness) but title can be anything appropriate.
	hzString	m_Category ;	//	General category of function (for tree asthetics)
	hzString	m_grpDesc ;		//	Description (overall)
	bool		m_bDone ;		//	Article printed

	ceFnset		(void)	{ m_bDone = false ; }
	~ceFnset	(void)	{}

	hzEcode	InitFnset	(ceEntity* parent, const hzString& title) ;

	void	SetCateg	(const hzString& S)	{ m_Category = S ; }

	hzString	Artname		(void) const	{ return m_Docname ; }
	hzString	Category	(void) const	{ return m_Category ; }
	ceEntType	Whatami		(void) const	{ return CE_UNIT_FUNC_GRP ; }
} ;

class	ceFtmpl	: public ceReal
{
	//	Function template
} ;

class	ceUnion	: public ceKlass
{
	//	Union

	ceEntity*	m_pHost ;				//	Host class or class template if applicable
	ceFile*		m_pFileDef ;			//	File union is defined in
	hzString	m_Docname ;				//	Article name (with 'un' pretext)
	hzString	m_Htmlval ;				//	Part page HTML for function (function group)
	uint32_t	m_DefTokStart ;			//	First token of entity definition (used to output def bodies to HTML)
	uint32_t	m_DefTokEnd ;			//	Last token of entity definition

public:
	//	hzVect	<ceStmt>	m_Statements ;	//	All statements assciated with the class definition body
	//	hzList	<ceFunc*>	m_Funcs ;		//	All functions

	ceUnion	(void) ;

	hzEcode	InitUnion	(ceFile* pFile, ceEntity* pHost, const hzString& name) ;

	const char*	DefFname	(void) const ;

	ceFile*		DefFile		(void) const	{ return m_pFileDef ; }
	hzString	Artname		(void) const	{ return m_Docname ; }
	hzString	Htmlval		(void) const	{ return m_Htmlval ; }
	uint32_t	GetDefStart	(void) const	{ return m_DefTokStart ; }
	uint32_t	GetDefEnd	(void) const	{ return m_DefTokEnd ; }
	ceBasis		Basis		(void) const	{ return ADT_UNION ; }
	ceEntType	Whatami		(void) const	{ return ENTITY_UNION ; }
} ;

/*
**	Typedefs, Hash Defines and Macros
*/

class	ceTypedef	: public ceDatatype
{
	ceTyplex	m_Tpx ;				//	Real type to which the typdef equates
	ceFile*		m_pFileDef ;		//	File the typdef appears (is defined)

public:
	ceTypedef	(void)
	{
		m_pFileDef = 0 ;
		m_Scope = SCOPE_GLOBAL ;
	}

	hzEcode	InitTypedef	(ceFile* pFile, ceTyplex& tpx, const hzString& name) ;

	const char*	DefFname	(void) const ;

	const ceTyplex&	Typlex	(void)	{ return m_Tpx ; }
	//ceBasis		Basis		(void) const	{ return m_Tpx.m_pType ? m_Tpx.m_pType->Basis() : ADT_NULL ; }
	ceBasis		Basis		(void) const	{ return m_Tpx.Basis() ; }
	ceFile*		DefFile		(void) const	{ return m_pFileDef ; }
	ceEntType	Whatami		(void) const	{ return ENTITY_TYPEDEF ; }
} ;

class	ceDefine	: public ceEntity
{
	//	A #define is used direct code substition and the ersatz tokens can amount to anything, a data type, an entity of some description or even a series of entities.

	//ceTyplex	m_Tpx ;					//	Type the ersatz is (if any)

public:
	hzVect	<ceToken>	m_Ersatz ;		//	The tail of the #define.

	ceFile*		m_pFileDef ;			//	File in which the #define occurs.
	uint32_t	m_DefTokStart ;			//	First token of entity definition (used to output def bodies to HTML)
	uint32_t	m_DefTokEnd ;			//	Last token of entity definition
	bool		m_bEval ;				//	Has the ersatz been evaluated (to check if it amounts to a type)

	ceDefine	(void) ;

	hzEcode	InitHashdef	(ceFile* pFile, const hzString& name, uint32_t start, uint32_t limit) ;

	const char*	DefFname	(void) const ;

	ceFile*		DefFile		(void) const	{ return m_pFileDef ; }
	uint32_t	GetDefStart	(void) const	{ return m_DefTokStart ; }
	uint32_t	GetDefEnd	(void) const	{ return m_DefTokEnd ; }
	ceEntType	Whatami		(void) const	{ return ENTITY_DEFINE ; }

	bool	IsType	(void) const	{ return false ; }
	bool	IsReal	(void) const	{ return true ; }
} ;

class	ceLiteral	: public ceEntity
{
	//	Special case of a #define that amounts to a literal value.

public:
	_atomval	m_Value ;		//	Literal value if applicable
	ceFile*		m_pFileDef ;	//	File in which the #define occurs.
	hzString	m_Str ;			//	String value
	hdbBasetype	m_Type ;		//	Base type

	ceLiteral	(void)	{ m_pFileDef = 0 ; m_Type = BASETYPE_UINT32 ; }

	hzEcode	InitLiteral	(ceFile* pFile, const hzString& name, const hzString& value) ;

	ceEntType	Whatami		(void) const	{ return ENTITY_LITERAL ; }

	bool	IsType	(void) const	{ return false ; }
	bool	IsReal	(void) const	{ return true ; }
} ;

class	ceMacro	: public ceEntity
{
	//	Special case of a #define that serves as a function. 
	//
	//	The macro is defined by a statement of the form #define macro_name(args) ersatz sequence. The macro definition is terminated by a newline. Any macro spanning multiple lines
	//	must escape the newline with the backslash. The arguments must each comprise a single word and be separated by a comma. The argument names are not required to mean anything
	//	and so are commonly a single letter. The names must be unique only within the macro. The ersatz (substitute) sequence must contain each named argument at least once.
	//
	//	Macro definitions and calls are translated by the pre-processor and so only their translations appear in the pre-processor output. Macro calls are syntactically the same as
	//	function calls: For each argument there is an expression that evaluates to a value. Both macro and function calls must be with arguments that match the defined arguments of
	//	the macro or function.

public:
	hzMapS	<hzString,uint32_t>	m_Defargs ;	//	Defined macro arguments

	hzVect	<ceToken>	m_Ersatz ;	//	Sequence of tokens forming the output template

	ceFile*		m_pFileDef ;		//	File in which the #define occurs.
	uint32_t	m_DefTokStart ;		//	First token of entity definition (used to output def bodies to HTML)
	uint32_t	m_DefTokEnd ;		//	Last token of entity definition

	ceMacro	(void) ;

	hzEcode	InitMacro	(ceFile* pFile, const hzString& name, uint32_t tokStart, uint32_t tokEnd) ;

	const char*	DefFname	(void) const ;

	ceFile*		DefFile		(void) const	{ return m_pFileDef ; }
	uint32_t	GetDefStart	(void) const	{ return m_DefTokStart ; }
	uint32_t	GetDefEnd	(void) const	{ return m_DefTokEnd ; }
	uint32_t	OrigLen		(void) const	{ return (m_Defargs.Count() * 2) + 2 ; }
	ceEntType	Whatami		(void) const	{ return ENTITY_MACRO ; }
	bool		IsType		(void) const	{ return false ; }
	bool		IsReal		(void) const	{ return true ; }
} ;

/*
**	Source File
*/

class	ceFile	: public ceFrame
{
public:
	hzMapM	<hzString,ceFile*>	m_Includes ;	//	All effective include files

	hzVect	<ceToken>	P ;						//	Original array of tokens (to be pre-processed)
	hzVect	<ceToken>	X ;						//	Pre-processed array of tokens
	hzVect	<ceFile*>	directInc ;				//	Directly include files
	hzVect	<ceStmt>	m_Statements ;			//	All top level C++ statements in the file

	hzChain		m_Content ;						//	Chain for de-tabbed content
	ceFile*		m_pRoot ;						//	Temporary setting to control searches
	ceEntbl*	m_pET ;							//	For variables static to file (should be cpp files only)
	hzString	m_Docname ;						//	Article name (with 'fi' pretext)
	hzString	m_Path ;						//	Fullpath
	Filetype	m_Filetype ;					//	File is header or source
	uint32_t	m_codeLevel ;					//	Code level
	bool		m_bStage1 ;						//	Set when file completed
	bool		m_bStage2 ;						//	Set when file completed
	bool		m_bMain ;						//	Set if the file contanins a function body of main()

	ceFile	(void)
	{
		m_pRoot = 0 ;
		m_pET = 0 ;
		m_Filetype = FILE_UNKNOWN ;
		m_codeLevel = 0 ;
		m_bStage1 = false ;
		m_bStage2 = false ;
		m_bMain = false ;
	}

	~ceFile	(void)
	{
	}

	hzEcode	Init		(ceComp* parent, const hzString& Path) ;
	hzEcode	Activate	(void) ;

	void	SetPath		(const char* cpPath)	{ m_Path = cpPath ; }

	const hzString&	Path	(void)	{ return m_Path ; }
	const char*		GetPath	(void)	{ return *m_Path ; }

	//	Active functions
	hzEcode		Parse			(ceFile* pRoot, uint32_t nRecurse) ;
	hzEcode		_incorporate	(ceFile* pFile) ;
	hzEcode		_preproc		(uint32_t nRecurse) ;
	ceMacro*	TryMacro		(const hzString& name, uint32_t& ultimate, uint32_t start, uint32_t stmtLimit) ;
	hzEcode		Preproc			(uint32_t nRecurse) ;
	hzEcode		ProcExtComment	(uint32_t start) ;
	hzEcode		ProcCommentFunc	(ceFunc* pFunc, uint32_t pt) ;
	hzEcode		ProcCommentClas	(ceClass* pClass, uint32_t pt) ;
	hzEcode		ProcCommentEnum	(ceEnum* pEnum, uint32_t pt) ;
	//hzEcode		ProcCondition	(hzVect<hzString>& cond, hzString& summary, uint32_t& ultimate, uint32_t start) ;
	hzEcode		ProcCondition	(hzString& summary, uint32_t& ultimate, uint32_t start) ;
	hzEcode		CheckValue		(ceDatatype* pType, uint32_t& ultimate, uint32_t start) ;
	hzEcode		GetArrayExtent	(uint32_t& ultimate, uint32_t start) ;
	hzEcode		GetTyplex		(ceTyplex& typlex, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start) ;
	bool		IsCast			(ceTyplex& typlex, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start) ;
	ceClass*	GetClass		(uint32_t& ultimate, uint32_t start) ;
	ceEntity*	LookupString	(ceKlass* pHost, const hzString& entname, const char* caller) ;
	ceEntity*	LookupToken		(hzVect<ceToken>& T, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, bool bSeries, const char* Caller) ;
	ceEntity*	GetClassObj		(hzVect<ceFunc*>& funcs, hzString& fqname, uint32_t& ultimate, uint32_t start) ;
	hzEcode		ProcStmtTypedef	(ceStmt& Stmt, ceKlass* thisKlass, ceEntbl* pFET, uint32_t& ultimate, uint32_t start) ;

	hzEcode		ProcStatement	(hzVect<ceStmt>& Stmts, ceKlass* thisKlass, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, hzString ctx, uint32_t cLev) ;
	hzEcode		ProcStructStmt	(hzVect<ceStmt>& Stmts, ceKlass* thisKlass, ceEntbl* pFET, Scope opRange, uint32_t& ultimate, uint32_t start, uint32_t cLev) ;
	hzEcode		ProcCodeStmt    (ceFunc* pFunc, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t nStart, uint32_t cLev) ;
	hzEcode		ProcCodeBody    (ceFunc* pFunc, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t nStart, uint32_t cLev) ;
	hzEcode		ProcFuncDef		(ceStmt& stmt, ceKlass* pHost, ceTyplex& tpx, const hzString& fname, EAttr attr, Scope dr, uint32_t& ultimate, uint32_t start, hzString ctx) ;

	hzEcode		PostProcFunc	(ceFunc* pFunc, ceKlass* pHost, uint32_t start) ;
	uint32_t	ProcTmplBase	(uint32_t& ultimate, uint32_t start) ;
	hzEcode		AssignTmplArgs	(hzList<ceTyplex>& args, ceKlass* pHost, uint32_t& ultimate, uint32_t start) ;
	hzEcode		TestNamespace	(hzVect<ceStmt>& Stmts, ceKlass* thisKlass, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, hzString ctx, uint32_t cLev) ;
	hzEcode		ProcNamespace	(hzVect<ceStmt>& Stmts, ceKlass* parKlass, ceNamsp* pNamsp, uint32_t& ultimate, uint32_t start, uint32_t cLev) ;
	hzEcode		ProcClass		(ceStmt& callStmt, ceKlass* pHost, uint32_t& ultimate, uint32_t start, uint32_t cLev, Scope clScope = SCOPE_PUBLIC) ;
	hzEcode		ProcUnion		(ceStmt& callStmt, ceKlass* pHost, uint32_t& ultimate, uint32_t start, uint32_t cLev) ;
	hzEcode		Evalnum			(ceKlass* pHost, uint32_t& atom, uint32_t& ultimate, uint32_t start) ;
	hzEcode		ProcEnum		(ceStmt& callStmt, ceKlass* pHost, uint32_t& ultimate, uint32_t start) ;
	hzEcode		ReadLiteral		(ceTyplex& tpx, hzAtom& val, uint32_t tokno, bool neg=false) ;
	ceEntity*	GetObject		(hzVect<ceStmt>& Stmts, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, uint32_t cLev) ;

	hzEcode		AssessTerm		(hzVect<ceStmt>& Stmts, hzString& diag, ceTyplex& tpx, hzAtom& Val, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, uint32_t limit, uint32_t cLev) ;
	hzEcode		AssessExpr		(hzVect<ceStmt>& Stmts, ceTyplex& tpx, hzAtom& Val, ceKlass* pHost, ceEntbl* pFET, uint32_t& ultimate, uint32_t start, uint32_t limit, uint32_t cLev) ;

	hzEcode		EvalFuncArg		(hzList<ceTyplex>& args, ceKlass* pHost, const hzString& fgname, uint32_t& ultimate, uint32_t start) ;
	hzEcode		ProcFuncArg		(hzList<ceVar*>& args, hzChain& cname, ceKlass* pHost, uint32_t& ultimate, uint32_t start) ;

	ceFunc*		ProcFuncCall	(hzVect<ceStmt>& Stmts, ceFngrp* pGrp, ceKlass* pHost, ceEntbl* pFet, uint32_t& ultimate, uint32_t start, uint32_t cLev) ; 

	ceEntType	Whatami	(void) const	{ return CE_UNIT_FILE ; }
} ;

const char*	Scope2Str	(Scope sc) ;

/*
**	Section X:	Programs, Components and the Project Class
*/

class	ceView
{
	//	This is a simple list of things that can appear as parts of a component in the tree

public:
	hzString	item_header ;		//	name="Headers"/>
	hzString	item_source ;		//	name="Sources"/>
	hzString	item_ctmpl ;		//	name="Class Templates"/>
	hzString	item_class ;		//	name="Classes"/>
	hzString	item_enums ;		//	name="Enums"/>
	hzString	item_funcs ;		//	name="Global Functions"/>
	hzString	item_progs ;		//	name="Global Functions"/>
	hzString	item_notes ;		//	name="Notes"/>
	char		m_Items	[8] ;		//	Controls order of tree items

	ceView	(void)
	{
		m_Items[0] = m_Items[1] = m_Items[2] = m_Items[3] = m_Items[4] = m_Items[5] = m_Items[6] = m_Items[7] = 0 ;
	}
} ;

class	ceComp	: public ceFrame
{
	//	Components arise in the code we are seeking to document. If we are documenting a single library we will have a single component of type library which will have header (.h)
	//	and source (.cpp) files only. There will be no 'main' function and so no main file. If we are documenting a single program with no dependency on a library, then there will
	//	be a single component of type program which has the headers (if any) and the sources of which one will be the main file. In both cases there will be a single root to the
	//	navigation tree with branches of headers, sources, class templates, classes, enums, functions and notes - all of which will appear subject to incidence. The incidence rules
	//	are such that if there are no enums for example, in the code the branch of enums will be empty. If there is only one enum the branch will be named after the enum and not be
	//	expandable. If there are multiple enums the branch will be called enums and will be expandable.
	//
	//	Things get more complex where a body of code will make more than one program. In this case each program will have a component of type program and these will exist within a
	//	component of type 'suite'. The suite component acts to collate header and source files common to more than one program, frequently leaving each program with only thier main
	//	file being unique. The resulting navigation tree will have a root bearing the name of the suite component and will have the same branches of headers, sources, class etc as
	//	described above but in this case describing only entities common to more than one program. These branches are then followed by a branch for each program and the program
	//	branches each expand out to have thier own set of the same standard branches. All subject to the same rules of entity incidence.
	//
	//	Where either a single program or a suite of programs depends on one or more libraries, the library component(s) can be placed either inside or ouitside the program or suite
	//	component depending on how you want the nav tree to look. Placing the library component outside the suite component so that the project has the library and suite components
	//	on the same level, will produce a tree of the form shown in Fig 1 whereas placing the library inside the suite will produce a tree of the form shown in Fig 2 (where project
	//	is named) or Fig 3 (where project name is omitted)
	//
	//		Fig 1							Fig 2								Fig 3
	//
	//		Project							Project 							Suite
	//			|____Library					|_____Suite							|_____Library
	//			|____Suite								|_____Library				|_____Program.1
	//					|____Program.1					|_____Program.1				|_____Program.2
	//					|____Program.2					|_____Program.2
	//
	//	Note that in the RHS diagram the Project is marked optional.

public:
	hzMapS	<hzString,ceSynp*>	m_mapSynopsis ;	//	All synopsis (found in code comments)
	hzMapS	<hzString,ceFile*>	m_SysIncl ;		//	Project's header files
	hzMapS	<hzString,ceFile*>	m_Headers ;		//	Project's header files
	hzMapS	<hzString,ceFile*>	m_Sources ;		//	Project's source files
	hzMapS	<hzString,ceFile*>	m_Documents ;	//	Project's notes files
	hzArray	<ceSynp*>			m_arrSynopsis ;	//	All synopsis (found in code comments)

	ceView		m_View ;		//	View of tree
	hzChain		m_Error ;		//	For collecting errors (part of reporting)
	ceFile*		m_pMain ;		//	File containing 'main' - only applies to cfgType='Program'
	hzString	m_Title ;		//	Comonent title (long form of name, appears at top of tree)
	hzString	m_Copyright ;	//	Copyright notice
	hzString	m_Docname ;		//	Name of HTML file describing the project component
	hzString	m_cfgType ;		//	Suite/Library/Program
	bool		m_bComplete ;	//	Has the component been reported?

	ceComp	(void)
	{
		m_bComplete = false ;
		m_pMain = 0 ;
	}

	//	Init functions
	hzEcode	ReadView		(hzXmlNode* pN) ;
	hzEcode	ReadFileLists	(hzXmlNode* pN) ;

	//	Final check to ensure only one main() per program etc
	hzEcode	Integrity		(void) ;

	//	Reporting functions
	hdsArticleStd*	MakeProjFile	(void) ;

	hzEcode	MakePageSynp	(hdsArticleStd* addPt, ceSynp* pSynp) ;
	hzEcode	MakePageFile	(hdsArticleStd* addPt, ceFile* pFile) ;
	hzEcode	MakePageEnum	(hdsArticleStd* addPt, ceEnum* pEnum) ;
	hzEcode	MakePageFunc	(hdsArticleStd* addPt, ceFunc* pFg) ;
	hzEcode	MakePageFnset	(hdsArticleStd* addPt, ceFnset* pFs) ;
	/*
	hzEcode	MakeHtmlClass		(hzChain& C, ceClass* pClass) ;
	*/
	hzEcode	MakePageClass	(hdsArticleStd* addPt, ceClass* pClass) ;
	hzEcode	MakePageCtmpl	(hdsArticleStd* addPt, ceClass* pClass) ;
	hzEcode	MakePageUnion	(hdsArticleStd* addPt, ceUnion* pUnion) ;

	//	Processing functions
	hzEcode	ReadConfig	(hzXmlNode* pN) ;
	hzEcode	Process		(void) ;
	hzEcode	Statements	(const hzString& htmlDir) const ;
	hzEcode	Report		(const hzString& htmlDir) ;	//const ;
	hzEcode	PostProc	(void) ;

	virtual	ceEntType	Whatami	(void) const	{ return CE_UNIT_COMPONENT ; }
} ;

class	ceProject	: public ceFrame
{
	//	The Code Enforcer project is all components implied within a config file

public:
	hzMapS	<hzString,ceEntbl*>	m_EntityTables ;	//	Project's entity tables

	hzMapM	<hzString,ceFile*>	m_SysInclByName ;	//	Project's header files (by name only)
	hzMapM	<hzString,ceFile*>	m_HeadersByName ;	//	Project's header files (by name only)
	hzMapM	<hzString,ceFile*>	m_SourcesByName ;	//	Project's source files (by name only)
	hzMapS	<hzString,ceFile*>	m_SysInclByPath ;	//	Project's header files (by full path)
	hzMapS	<hzString,ceFile*>	m_HeadersByPath ;	//	Project's header files (by full path)
	hzMapS	<hzString,ceFile*>	m_SourcesByPath ;	//	Project's source files (by full path)
	hzMapS	<hzString,ceFile*>	m_Documents ;		//	Project's document files (by full path)
	hzMapS	<hzString,ceComp*>	m_AllComponents ;	//	All components in the project by name
	hzMapS	<hzString,ceSynp*>	m_AllSynopsis ;		//	All synopsis (found in code comments)
	hzMapS	<hzString,cePara*>	m_AllParagraphs ;	//	All paragraphs found in all comments

	hzSet	<ceFunc*>	m_Funclist ;	//	All functions needing secondary parsing
	hzList	<ceComp*>	m_Libraries ;	//	All library components in the project by config order
	hzList	<ceComp*>	m_Programs ;	//	All program components in the project by config order
	hzSet	<ceNamsp*>	m_Using ;		//	All namespaces brought into scope by a using directive

	ceNamsp*	m_pCurrNamsp ;			//	Current namespace to add objects to

	ceEntbl		m_RootET ;				//	Root entity table
	hzChain		m_ArticleSum ;			//	All <xarticle> tags (for SiteServer)
	hzString	m_Docname ;				//	Name of HTML file describing the project
	hzString	m_Website ;				//	Websie address eg http://www.hadronzoo.com
	hzString	m_HtmlDir ;				//	Directory for HTML output of this project (site plus proj name)
	hzString	m_Page ;				//	This is the article group's intended hostpage (rel to Dissemino doc root)
	hzString	m_CfgFile ;				//	Project config file
	hzString	m_ExpFile ;				//	Project export file
	hzString	m_Copyright ;			//	Copyright notice
	bool		m_bSystemMask ;			//	Attribute all new entities as system entities

	ceProject	(void)
	{
		m_Using.SetDefault((ceNamsp*)0) ;
		m_pCurrNamsp = 0 ;
	}

	hzEcode	Init		(const hzString cfgfile) ;
	hzEcode	Process		(void) ;
	hzEcode	Export		(void) ;
	hzEcode	BuildMenu	(void) ;
	hzEcode	Report		(void) ;

	ceFile*	LocateFile	(const hzString& name) ;

	//	Obligatory
	ceEntType	Whatami	(void) const	{ return CE_UNIT_PROJECT ; }
} ;

/*
**	External Global Variables
*/

extern	hzLogger	slog ;
extern	hzChain		g_EC[50] ;		//	Global error chains - for function call level error reporting

//  Standard types
extern	ceCStd*		g_cpp_void ;
extern	ceCStd*		g_cpp_bool ;
extern	ceCStd*		g_cpp_char ;
extern	ceCStd*		g_cpp_uchar ;
extern	ceCStd*		g_cpp_short ;
extern	ceCStd*		g_cpp_ushort ;
extern	ceCStd*		g_cpp_int ;
extern	ceCStd*		g_cpp_uint ;
extern	ceCStd*		g_cpp_long ;
extern	ceCStd*		g_cpp_ulong ;
extern	ceCStd*		g_cpp_longlong ;
extern	ceCStd*		g_cpp_ulonglong ;
extern	ceCStd*		g_cpp_float ;
extern	ceCStd*		g_cpp_double ;

extern	hdsTree		g_treeEnts ;		//  All entities.

/*
**	Prototypes
*/

ceDatatype*	GetBaseType	(const hzString& S, ceClass* pHostClass = 0) ;

hzEcode		ApplyAssignment	(ceTyplex& resTpx, hzAtom& resVal, const ceTyplex& tpxA, const ceTyplex& tpxB, const hzString& opstr, CppLex oper, int32_t cLev) ;
hzEcode		ApplyOperator	(ceTyplex& resTpx, hzAtom& resVal, const ceTyplex& tpxA, const ceTyplex& tpxB, const hzString& opstr, CppLex oper, int32_t cLev) ;

hzEcode		AddGlobalEntity	(ceEntity* pE, hzString& Id) ;
ceEntType	WhatIs			(const hzString& Name) ;
ceClass*	GetClass		(const hzString& Name) ;
ceUnion*	GetUnion		(const hzString& Name) ;
ceEnum*		GetEnum			(const hzString& Name) ;
ceFunc*		GetFunction		(const hzString& Name) ;
int32_t		GetEnumValue	(const hzString& Name) ;
int32_t		GetConstant		(const hzString& Name) ;
const char*	Entity2Txt		(ceEntType e) ;
hzEcode		DeTab			(hzChain& Z) ;

#endif	//	enforcer_h
