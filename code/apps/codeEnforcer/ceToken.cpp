//
//	File:	ceToken.cpp
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

#include <iostream>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzDirectory.h"
#include "hzProcess.h"

#include "enforcer.h"

using namespace std ;

static	int32_t	s_nTabsize = 4 ;

extern	ceProject	g_Project ;				//	The autodoc project

/*
**	String table
*/

hzMapS	<hzString,int32_t>	g_mapStrings ;	//	All strings encountered in all tokens
hzVect	<hzString>			g_vecStrings ;	//	All strings as a vector

int32_t	PutString	(hzString& S)
{
	//	Return a unique number identifying a string value

	_hzfunc(__func__) ;

	int32_t		val ;

	if (!S)
		return 0 ;

	if (g_mapStrings.Exists(S))
		return g_mapStrings[S] ;

	g_vecStrings.Add(S) ;
	val = g_vecStrings.Count() ;

	g_mapStrings.Insert(S, val) ;
	return val ;
}

hzString	GetString	(int32_t val)
{
	_hzfunc(__func__) ;

	return g_vecStrings[val] ;
}

/*
**	Token type strings
*/

/*
**	Common C++ keyword strings
*/

//	Entities
hzString	tok_class			= "class" ;
hzString	tok_struct			= "struct" ;
hzString	tok_union			= "union" ;
hzString	tok_enum			= "enum" ;

//	Compiler directives
hzString	tok_hash_if			= "#if" ;
hzString	tok_hash_ifdef		= "#ifdef" ;
hzString	tok_hash_ifndef		= "#ifndef" ;
hzString	tok_hash_else		= "#else" ;
hzString	tok_hash_elseif		= "#elseif" ;
hzString	tok_hash_endif		= "#endif" ;
hzString	tok_hash_define		= "#define" ;
hzString	tok_hash_include	= "#include" ;
hzString	tok_using			= "using" ;
hzString	tok_namespace		= "namespace" ;
hzString	tok_operator		= "operator" ;
hzString	tok_typedef			= "typedef" ;

//	Attributes
hzString	tok_const			= "const" ;
hzString	tok_inline			= "inline" ;
hzString	tok_static			= "static" ;
hzString	tok_extern			= "extern" ;
hzString	tok_friend			= "friend" ;
hzString	tok_unsigned		= "unsigned" ;
hzString	tok_void			= "void" ;
hzString	tok_char			= "char" ;
hzString	tok_short			= "short" ;
hzString	tok_int				= "int" ;
hzString	tok_long			= "long" ;
hzString	tok_register		= "register" ;
hzString	tok_virtual			= "virtual" ;
hzString	tok_public			= "public" ;
hzString	tok_private			= "private" ;
hzString	tok_protected		= "protected" ;
hzString	tok_mutable			= "mutable" ;

hzString	tok_template		= "template" ;

//	Boolean true/false
hzString	tok_bool_true		= "true" ;
hzString	tok_bool_false		= "false" ;

//	C++ commands
hzString	tok_cmd_if			= "if" ;
hzString	tok_cmd_else		= "else" ;
hzString	tok_cmd_switch		= "switch" ;
hzString	tok_cmd_case		= "case" ;
hzString	tok_cmd_default		= "default" ;
hzString	tok_cmd_for			= "for" ;
hzString	tok_cmd_do			= "do" ;
hzString	tok_cmd_while		= "while" ;
hzString	tok_cmd_break		= "break" ;
hzString	tok_cmd_cont		= "continue" ;
hzString	tok_cmd_goto		= "goto" ;
hzString	tok_cmd_return		= "return" ;
hzString	tok_cmd_new			= "new" ;
hzString	tok_cmd_delete		= "delete" ;

//	Operators
hzString	tok_op_destruct		= "::~" ;
hzString	tok_op_scope		= "::" ;
hzString	tok_op_sep			= ":" ;
hzString	tok_op_incr			= "++" ;
hzString	tok_op_pluseq		= "+=" ;
hzString	tok_op_plus			= "+" ;
hzString	tok_op_decr			= "--" ;
hzString	tok_op_ptr			= "->" ;
hzString	tok_op_minuseq		= "-=" ;
hzString	tok_op_minus		= "-" ;
hzString	tok_op_multeq		= "*=" ;
hzString	tok_op_mult			= "*" ;
hzString	tok_indir3			= "***" ;
hzString	tok_indir2			= "**" ;
hzString	tok_indir1			= "*" ;
hzString	tok_op_diveq		= "/=" ;
hzString	tok_op_div			= "/" ;
hzString	tok_op_remeq		= "%=" ;
hzString	tok_op_rem			= "%" ;	
hzString	tok_op_lshifteq		= "<<=" ;
hzString	tok_op_lshift		= "<<" ;
hzString	tok_op_lesseq		= "<=" ;
hzString	tok_op_less			= "<" ;
hzString	tok_op_rshifteq		= ">>=" ;
hzString	tok_op_rshift		= ">>" ;
hzString	tok_op_moreeq		= ">=" ;
hzString	tok_op_more			= ">" ;
hzString	tok_op_testeq		= "==" ;
hzString	tok_op_eq			= "=" ;
hzString	tok_op_complmenteq	= "~=" ;
hzString	tok_op_power		= "^" ;	
hzString	tok_op_cond_and		= "&&" ;
hzString	tok_op_andeq		= "&=" ;
hzString	tok_op_logic_and	= "&" ;
hzString	tok_op_cond_or		= "||"  ;
hzString	tok_op_oreq			= "|=" ;
hzString	tok_op_logic_or		= "|" ;
hzString	tok_op_query		= "?" ;
hzString	tok_op_period		= "." ;	
hzString	tok_op_noteq		= "!=" ;
hzString	tok_op_not			= "!" ;	
hzString	tok_op_invert		= "~" ;	
hzString	tok_elipsis			= "..." ;
hzString	tok_this			= "this" ;
hzString	tok_sizeof			= "sizeof" ;
hzString	tok_dyn_cast		= "dynamic_cast" ;

//	Symbols
hzString	tok_posn_open = "[" ;
hzString	tok_posn_close = "]" ;
hzString	tok_cond_open = "(" ;
hzString	tok_cond_close = ")" ;
hzString	tok_body_open = "{" ;
hzString	tok_body_close = "}" ;
hzString	tok_sep = "," ;
hzString	tok_terminator = ";" ;
hzString	tok_escape = "\\" ;

const char*	ceToken::Show	(void)
{
	if (type == TOK_UNKNOWN)
		return "___unknown_type___" ;

	return value ? *value : "__no_value__" ;
}

hzEcode	DeTab	(hzChain& Z)
{
	//	Replace all tabs with spaces such that text appears the same.
	//
	//	Arguments:	1)	The reference to the final hzChain
	//				2)	The reference to the initial hzChain
	//
	//	Returns		1)	E_MEMORY	If the operation could not allocate space for the final hzChain
	//				2)	E_OK		If operation successful

	_hzfunc(__func__) ;

	hzChain::Iter	zi ;		//	Temporary store of start of token

	hzChain	Final ;				//	Re-formulated text
	int32_t	nCol = 0 ;
	int32_t	nCount ;

	if (s_nTabsize < 4)
		s_nTabsize = 4 ;
	if (s_nTabsize > 4)
		s_nTabsize = 8 ;

	for (zi = Z ; !zi.eof() ; zi++)
	{
		//	Eliminate whitespace
		if (*zi == CHAR_SPACE)
		{
			nCol++ ;
			Final.AddByte(CHAR_SPACE) ;
			continue ;
		}

		if (*zi == CHAR_TAB)
		{
			nCount = (s_nTabsize - (nCol % s_nTabsize)) ;
			nCol += nCount ;
			for (; nCount ; nCount--)
				Final.AddByte(CHAR_SPACE) ;
			continue ;
		}

		if (*zi == CHAR_CR)
			zi++ ;

		if (*zi == CHAR_NL)
		{
			nCol = 0 ;
			Final.AddByte(CHAR_NL) ;
			continue ;
		}

		//	Ignore other white chars
		if (*zi < CHAR_SPACE)
			continue ;

		//	Just add chars as normal
		nCol++ ;
		Final.AddByte(*zi) ;
	}

	Z.Clear() ;
	Z = Final ;

	return E_OK ;
}

bool	_iscppopchar	(const char c)
{
	//	Determine if a char could be or form part of an operator

	switch  (c)
	{
	case ':': case '+': case '-': case '*': case '/': case '%': case '<': case '>':
	case '=': case '^': case '&': case '|': case '?': case '.': case '!': case '~':
		return true ;
	}

	return false ;
}

hzEcode	Tokenize	(hzVect<ceToken>& toklist, const hzChain& content, const hzString& fname)
{
	//	Convert a hzChain into a series of C++ tokens.
	//
	//	This function keeps track of lines and column positions. For this purpose the supplied chain content is assumed to have been de-tabbed.
	//
	//	Arguments:		1)	The target token list (as hzVect)
	//					2)	The content to be tokenized
	//					3)	A chain to write error messages to

	_hzfunc(__func__) ;

	hzChain::Iter	zi ;		//	Iterator for file content
	hzChain::Iter	xi ;		//	Forward iterator
	hzChain::Iter	loopStop ;	//	Loop stop iterator

	hzChain		word ;			//	Chain for handling comments
	hzChain		pcon ;			//	Chain for comment paragraphs
	hzAtom		atom ;			//	For numbers
	ceToken		T ;				//	Token (current)
	hzString	W ;				//	Temporary string for token value
	hzString	X ;				//	Temporary string for token value
	const char*	i ;				//	Comment processor
	uint32_t	nLevel ;		//	Used for nested regions (eg excluded text)
	uint32_t	intval ;		//	For testing if a token evals to a number
	bool		bOk ;
	bool		bLoop = false ;
	hzEcode		rc = E_OK ;

	/*
	**	Tokenize the file
	*/

	//slog.Out("Tokenize file %s (%d bytes)\n", *fname, content.Size()) ;

	toklist.Clear() ;

	for (zi = content ; !zi.eof() ;)
	{
		if (bLoop && zi == loopStop)
		{
			slog.Out("Loop stop condition detected\n") ;
			rc = E_SYNTAX ;
			break ;
		}
		bLoop = true ;
		loopStop = zi ;

		//	Eliminate whitespace
		if (*zi == CHAR_SPACE)	{ zi++ ; continue ; }
		if (*zi == CHAR_NL)		{ zi++ ; continue ; }
		if (*zi == CHAR_TAB)	{ zi++ ; continue ; }
		if (*zi <= CHAR_SPACE)	{ zi++ ; continue ; }

		//	Set initial values
		T.Clear() ;
		T.line = zi.Line() ;
		T.col = zi.Col() ;

		/*
		**	Check for quoted strings
		*/

		if (*zi == CHAR_SQUOTE)
		{
			word.Clear() ;
			for (zi++ ; !zi.eof() ; zi++)
			{
				if (*zi == CHAR_BKSLASH)
				{
					if (zi == "\\\'")
						{ word.AddByte(CHAR_SQUOTE) ; zi++ ; continue ; }
				}

				if (*zi == CHAR_SQUOTE)
					break ;
				word.AddByte(*zi) ;
			}

			if (*zi == CHAR_SQUOTE)
			{
				T.value = word ;
				word.Clear() ;
				T.type = TOK_SCHAR ;
				toklist.Add(T) ;
				zi++ ;
			}
			continue ;
		}

		if (*zi == CHAR_DQUOTE)
		{
			//	Be aware thet string literals can be directly concatenated and that multiple string literals appearing directly after another should
			//	be considered as one string literal.

			word.Clear() ;

			for (; !zi.eof() ;)
			{
				for (zi++ ; !zi.eof() ; zi++)
				{
					if (*zi == CHAR_BKSLASH)
					{
						if (zi == "\\\"")
							{ word.AddByte(CHAR_DQUOTE) ; zi++ ; continue ; }
					}

					if (*zi == CHAR_DQUOTE)
						break ;
					word.AddByte(*zi) ;
				}

				if (*zi == CHAR_DQUOTE)
				{
					for (zi++ ; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

					if (zi != CHAR_DQUOTE)
					{
						T.value = word ;
						word.Clear() ;
						T.type = TOK_QUOTE ;
						toklist.Add(T) ;
						//	NOTE ????
						//	zi++ ;
						break ;
					}
				}
			}
			continue ;
		}

		if (zi == "0x")
		{
			xi = zi ;
			word.Clear() ;
			word << "0x" ;

			for (xi += 2 ; IsHex(*xi) ; xi++)
				word.AddByte(*xi) ;
			W = word ;
			word.Clear() ;

			if (atom.SetNumber(*W + 2) == E_OK)
			{
				T.type = TOK_HEXNUM ;
				T.value = W ;
				toklist.Add(T) ;
				zi = xi ;
				continue ;
			}
		}

		if (IsDigit(*zi))
		{
			xi = zi ;
			word.Clear() ;

			word.AddByte(*xi) ;
			for (xi++ ; IsDigit(*xi) ; xi++)
				word.AddByte(*xi) ;
			T.type = TOK_NUMBER ;

			if (*xi == CHAR_PERIOD)
			{
				word.AddByte(CHAR_PERIOD) ;
				for (xi++ ; IsDigit(*xi) ; xi++)
					word.AddByte(*xi) ;
				T.type = TOK_STDNUM ;

				if (*xi == 'e')
				{
					word.AddByte(*xi) ;
					xi++ ;
					if (*xi == CHAR_MINUS)
						{ word.AddByte(CHAR_MINUS) ; xi++ ; }
					for (; IsDigit(*xi) ; xi++)
						word.AddByte(*xi) ;
				}
			}

			T.value = word ;
			word.Clear() ;
			toklist.Add(T) ;
			zi = xi ;
			continue ;
		}

		//	Check for backslash line extenders
		if (zi == "\\\n")
		{
			T.value = tok_escape ;
			T.type = TOK_ESCAPE ;
			toklist.Add(T) ;
			zi++ ;
			continue ;
		}

		/*
		**	Check for comments
		*/

		//	Ignore (do not produce tokens for) comments consisting of '#if 0, #endif' blocks
		if (zi == "#if 0")
		{
			for (nLevel = 1, zi += 5 ; nLevel && *zi ;)
			{
				if (zi == "#if")
					{ nLevel++ ; zi += 3 ; continue ; }
				if (zi == "#endif")
					{ nLevel-- ; zi += 6 ; continue ; }
				zi++ ;
			}
			continue ;
		}

		//	Process Comments: Note that under HadronZoo commenting standards, comment blocks with the opening /* and closing */ sequences are recommended only for headline comments
		//	that mark out important sections of code.
		//
		//	In all other cases, line comments starting with // are to be used, even where comments span multiple lines. These are however, only recommendations, not a hard and fast
		//	rule. Note that code enforcer treats all comments as a single token. In the case of line comments, these are merged if they are adjacent and not separated by blank lines.

		if (zi == "//")
		{
			//	Process line comment and possible comment block

			word.Clear() ;
			pcon.Clear() ;
			T.col = zi.Col() ;
			T.line = zi.Line() ;

			for (;;)
			{
				//	After opening double-slash, ignore leading whitespace.
				zi += 2 ;
				if (*zi == CHAR_SPACE)
					for (zi++ ; *zi == CHAR_SPACE ; zi++) ;
				else
				{
					if (*zi == CHAR_TAB)
						zi++ ;
				}
				
				//	Add line content to the chain
				for (; !zi.eof() && *zi != CHAR_NL ; zi++)
					word.AddByte(*zi) ;

				if (*zi == CHAR_NL)
				{
					//	Look for any whitespace followed by another '//'
					for (xi = zi, xi++ ; !xi.eof() && (*xi == CHAR_SPACE || *xi == CHAR_TAB) ; xi++) ;
					if (xi == "//")
					{
						word.AddByte(CHAR_NL) ;
						zi = xi ;
						continue ;
					}
				}
				break ;
			}

			if (word.Size() > 100000)
				slog.Out("COMMENT LINE TOO LONG %d bytes\n", word.Size()) ;

			T.value = word ;
			T.type = TOK_COMMENT ;
			T.t_cmtf = TOK_FLAG_COMMENT_LINE ;
			toklist.Add(T) ;
			word.Clear() ;
			continue ;
		}

		if (zi == "/*")
		{
			//	Process comment blocks with the opening /* and closing */ sequence. These according to the HadronZoo commenting standards, should begin with the opening and closing
			//	sequences, each on a line by themselves. The commment text which can be separated by blank lines to mark paragraphs, then appears on the lines inbetween. All lines
			//	should begin with a ** or // followed by a newline when blank or a tab preceeding the content. The ** or // sequences are not considered part of the comment.
			//
			//	The exception to the above are where the comment block is not indented at all, the comment body can appear without either the ** or // starting
			//	the line. This is the common practice when writing synopses. Because this is allowed, lines must first be tested. If the first non-whitespace
			//	characters in the line are anything other than ** or // then the line as a whole is taken as-is including the leading whitespace.

			T.col = zi.Col() ;
			T.line = zi.Line() ;
			//slog.Out("Comment Column %d\n", T.col) ;

			for (zi += 2 ; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

			word.Clear() ;
			for (; !zi.eof() ;)
			{
				if (*zi == CHAR_NL)
				{
					//	Put the newline in but then skip all whitespace and any comment continuations chars
					//	in the next line (except the terminating sequence)

					word.AddByte(CHAR_NL) ;

					if (T.col != 1)
						for (zi++ ; !zi.eof() && *zi == CHAR_SPACE && *zi == CHAR_TAB ; zi++) ;
					else
						zi++ ;

					if (zi == "**" || zi == "//")
					{
						zi += 2 ;
						if (*zi == CHAR_TAB)
							zi++ ;
					}
					continue ;
				}

				//	Backslash escapes the next char in some circumstances
				if (*zi == CHAR_BKSLASH)
				{
					if (zi == "\\/")	{ word.AddByte(CHAR_FWSLASH) ; zi += 2 ; continue ; }
					if (zi == "\\*")	{ word.AddByte(CHAR_ASTERISK) ; zi += 2 ; continue ; }
				}

				if (zi == "*/")
					{ zi += 2 ; break ; }

				word.AddByte(*zi) ; zi++ ;
			}

			//	Now divide the comment into a set of paragraphs
			/*
			if (word.Size())
			{
				T.pParas = pPara = new cePara() ;
				xi = word ;
				for (; *xi == CHAR_SPACE || *xi == CHAR_TAB ; xi++) ;
				pPara->m_File = fname ;
				pPara->m_nLine = T.line ;
				pPara->m_nCol = xi.Col() ;

				for (; !xi.eof() ;)
				{
					if (*xi == CHAR_NL)
					{
						if (xi == "\n\n")
						{
							pPara->m_Content = pcon ;
							pcon.Clear() ;
							g_Project.m_AllParagraphs.Insert(pPara->m_Content, pPara) ;

							for (; *xi == CHAR_NL ; xi++) ;
							for (; *xi == CHAR_SPACE || *xi == CHAR_TAB ; xi++) ;

							if (!xi.eof())
							{
								pPara->next = new cePara() ;
								pPara = pPara->next ;
								pPara->m_File = fname ;
								pPara->m_nLine = T.line ;
								pPara->m_nCol = xi.Col() ;
							}
							continue ;
						}
					}

					if (*xi == CHAR_TAB)
					{
						//	A tab in mid line means paragraph is a table cell
						pPara->m_Content = pcon ;
						pPara->m_nCell++ ;
						pcon.Clear() ;
						g_Project.m_AllParagraphs.Insert(pPara->m_Content, pPara) ;

						for (; *xi == CHAR_SPACE || *xi == CHAR_TAB ; xi++) ;
						if (!xi.eof())
						{
							pPara->next = new cePara() ;
							pPara = pPara->next ;
							pPara->m_nCell++ ;
							pPara->m_nLine = T.line + xi.Line() ;
							pPara->m_nCol = xi.Col() ;
							pPara->m_File = fname ;
						}
						continue ;
					}

					pcon.AddByte(*xi) ;
					xi++ ;
				}

				if (pcon.Size())
				{
					pPara->m_Content = pcon ;
					pcon.Clear() ;
					g_Project.m_AllParagraphs.Insert(pPara->m_Content, pPara) ;
				}
			}
			*/

			T.value = word ;
			T.type = TOK_COMMENT ;
			toklist.Add(T) ;
			word.Clear() ;
			continue ;
		}

		/*
		**	Check for parenthesis chars, commas and terminators
		*/

		if (*zi == '[')	{ zi++ ; T.value = tok_posn_open ;	T.type = TOK_SQ_OPEN ;		toklist.Add(T) ; continue ; }
		if (*zi == ']')	{ zi++ ; T.value = tok_posn_close ;	T.type = TOK_SQ_CLOSE ;		toklist.Add(T) ; continue ; }
		if (*zi == '(')	{ zi++ ; T.value = tok_cond_open ;	T.type = TOK_ROUND_OPEN ;	toklist.Add(T) ; continue ; }
		if (*zi == ')')	{ zi++ ; T.value = tok_cond_close ;	T.type = TOK_ROUND_CLOSE ;	toklist.Add(T) ; continue ; }
		if (*zi == '{')	{ zi++ ; T.value = tok_body_open ;	T.type = TOK_CURLY_OPEN ;	toklist.Add(T) ; continue ; }
		if (*zi == '}')	{ zi++ ; T.value = tok_body_close ;	T.type = TOK_CURLY_CLOSE ;	toklist.Add(T) ; continue ; }
		if (*zi == ',')	{ zi++ ; T.value = tok_sep ;		T.type = TOK_SEP ;			toklist.Add(T) ; continue ; }
		if (*zi == ';')	{ zi++ ; T.value = tok_terminator ;	T.type = TOK_END ;			toklist.Add(T) ; continue ; }

		/*
		**	Check for operators
		*/

		if (_iscppopchar(*zi))
		{
			for (word.Clear() ; _iscppopchar(*zi) ; word.AddByte(*zi), zi++) ;
			W = word ;
			bOk = true ;

			switch	(W[0])
			{
			case ':':
				if (W == "::~")
				{
					T.value = tok_op_scope ; T.type = TOK_OP_SCOPE ; toklist.Add(T) ; T.col += 2 ;
					T.value = tok_op_invert ; T.type = TOK_OP_INVERT ;
				}
				else if (W == "::")
					{ T.value = tok_op_scope ; T.type = TOK_OP_SCOPE ; }
				else if (W == ":!")
				{
					T.value = tok_op_sep ; T.type = TOK_OP_SEP ; toklist.Add(T) ; T.col++ ;
					T.value = tok_op_not ; T.type = TOK_OP_NOT ;
				}
				else if (W == ":-")
				{
					T.value = tok_op_sep ; T.type = TOK_OP_SEP ; toklist.Add(T) ; T.col++ ;
					T.value = tok_op_minus ; T.type = TOK_OP_MINUS ;
				}
				else if (W == ":")
					{ T.value = tok_op_sep ; T.type = TOK_OP_SEP ; }
				else
					bOk = false ;
				break ;

			case '+':
				if		(W == "++")	{ T.value = tok_op_incr ; T.type = TOK_OP_INCR ; }
				else if (W == "+=")	{ T.value = tok_op_pluseq ; T.type = TOK_OP_PLUSEQ ; }
				else if (W == "+")	{ T.value = tok_op_plus ; T.type = TOK_OP_PLUS ; }
				else
					bOk = false ;
				break ;

			case '-':
				if		(W == "--")	{ T.value = tok_op_decr ; T.type = TOK_OP_DECR ; }
				else if (W == "->")	{ T.value = tok_op_ptr ; T.type = TOK_OP_MEMB_PTR ; }
				else if (W == "-=")	{ T.value = tok_op_minuseq ; T.type = TOK_OP_MINUSEQ ; }
				else if (W == "-*")
				{
					T.value = tok_op_minus ; T.type = TOK_OP_MINUS ; toklist.Add(T) ; T.col++ ;
					T.value = tok_op_mult ; T.type = TOK_OP_MULT ;
				}
				else if (W == "-")
					{ T.value = tok_op_minus ; T.type = TOK_OP_MINUS ; }
				else
					bOk = false ;
				break ;

			case '*':
				if (W == "*=")
					{ T.value = tok_op_multeq ; T.type = TOK_OP_MULTEQ ; }
				else if (W == "***")
					{ T.value = tok_indir3 ; T.type = TOK_INDIR ; }
				else if (W == "**")
					{ T.value = tok_indir2 ; T.type = TOK_INDIR ; }
				else if (W == "*>::")
				{
					T.value = tok_op_mult ; T.type = TOK_OP_MULT ;	toklist.Add(T) ; T.col++ ;
					T.value = tok_op_more ; T.type = TOK_OP_MORE ;	toklist.Add(T) ; T.col++ ;
					T.value = tok_op_scope ; T.type = TOK_OP_SCOPE ;
				}
				else if (W == "*>&")
				{
					T.value = tok_op_mult ; T.type = TOK_OP_MULT ;	toklist.Add(T) ; T.col++ ;
					T.value = tok_op_more ; T.type = TOK_OP_MORE ;	toklist.Add(T) ; T.col++ ;
					T.value = tok_op_logic_and ; T.type = TOK_OP_LOGIC_AND ;
				}
				else if (W == "*>")
				{
					T.value = tok_op_mult ; T.type = TOK_OP_MULT ;	toklist.Add(T) ; T.col++ ;
					T.value = tok_op_more ; T.type = TOK_OP_MORE ;
				}
				else if (W == "*")
					{ T.value = tok_op_mult ; T.type = TOK_OP_MULT ; }
				else
					bOk = false ;
				break ;

			case '/':
				if		(W == "/=")	{ T.value = tok_op_diveq ; T.type = TOK_OP_DIVEQ ; }
				else if (W == "/")	{ T.value = tok_op_div ; T.type = TOK_OP_DIV ; }
				else
					bOk = false ;
				break ;

			case '%':
				if		(W == "%=")	{ T.value = tok_op_remeq ; T.type = TOK_OP_REMEQ ; }
				else if (W == "%")	{ T.value = tok_op_rem ; T.type = TOK_OP_REM ; }
				else
					bOk = false ;
				break ;

			case '<':
				if		(W == "<<="){ T.value = tok_op_lshifteq ; T.type = TOK_OP_LSHIFTEQ ; }
				else if (W == "<<")	{ T.value = tok_op_lshift ; T.type = TOK_OP_LSHIFT ; }
				else if (W == "<=")	{ T.value = tok_op_lesseq ; T.type = TOK_OP_LESSEQ ; }
				else if (W == "<>")
					{ T.value = tok_op_less ; T.type = TOK_OP_LESS ; toklist.Add(T) ; T.col++ ; T.value = tok_op_more ; T.type = TOK_OP_MORE ; }
				else if (W == "<")	{ T.value = tok_op_less ; T.type = TOK_OP_LESS ; }
				else
					bOk = false ;
				break ;

			case '>':
				if		(W == ">>="){ T.value = tok_op_rshifteq ; T.type = TOK_OP_RSHIFTEQ ; }
				else if (W == ">>")	{ T.value = tok_op_rshift ; T.type = TOK_OP_RSHIFT ; }
				else if (W == ">=")	{ T.value = tok_op_moreeq ; T.type = TOK_OP_MOREEQ ; }
				else if (W == ">*")
				{
					T.value = tok_op_more ; T.type = TOK_OP_MORE ; toklist.Add(T) ; T.col++ ;
					T.value = tok_indir1 ; T.type = TOK_INDIR ;
				}
				else if (W == ">&")
				{
					T.value = tok_op_more ; T.type = TOK_OP_MORE ; toklist.Add(T) ; T.col++ ;
					T.value = tok_op_logic_and ; T.type = TOK_OP_LOGIC_AND ;
				}
				else if (W == ">::")
				{
					T.value = tok_op_more ; T.type = TOK_OP_MORE ; toklist.Add(T) ; T.col++ ;
					T.value = tok_op_scope ; T.type = TOK_OP_SCOPE ;
				}
				else if (W == ">")
					{ T.value = tok_op_more ; T.type = TOK_OP_MORE ; }
				else
					bOk = false ;
				break ;

			case '=':
				if (W == "==")
					{ T.value = tok_op_testeq ; T.type = TOK_OP_TESTEQ ; }
				else if (W == "=-")
					{ T.value = tok_op_eq ; T.type = TOK_OP_EQ ; toklist.Add(T) ; T.col++ ; T.value = tok_op_minus ; T.type = TOK_OP_MINUS ; }
				else if (W == "=")
					{ T.value = tok_op_eq ; T.type = TOK_OP_EQ ; }
				else
					bOk = false ;
				break ;

			case '^':
				if		(W == "^=")	{ T.value = tok_op_complmenteq ; T.type = TOK_OP_COMPLMENTEQ ; }
				else if (W == "^")	{ T.value = tok_op_power ; T.type = TOK_OP_LOGIC_EXOR ; }
				else
					bOk = false ;
				break ;

			case '&':
				if		(W == "&&")	{ T.value = tok_op_cond_and ; T.type = TOK_OP_COND_AND ; }
				else if (W == "&=")	{ T.value = tok_op_andeq ; T.type = TOK_OP_ANDEQ ; }
				else if (W == "&")	{ T.value = tok_op_logic_and ; T.type = TOK_OP_LOGIC_AND ; }
				else
					bOk = false ;
				break ;

			case '|':
				if		(W == "||")	{ T.value = tok_op_cond_or ; T.type = TOK_OP_COND_OR ; }
				else if (W == "|=")	{ T.value = tok_op_oreq ; T.type = TOK_OP_OREQ ; }
				else if (W == "|")	{ T.value = tok_op_logic_or ; T.type = TOK_OP_LOGIC_OR ; }
				else
					bOk = false ;
				break ;

			case '?':
				T.value = tok_op_query ; T.type = TOK_OP_QUERY ;
				break ;

			case '.':
				if		(W == "...")	{ T.value = tok_elipsis ; T.type = TOK_ELIPSIS ; }
				else if (W == ".")		{ T.value = tok_op_period ; T.type = TOK_OP_PERIOD ; }
				else
					bOk = false ;
				break ;

			case '!':
				if (W == "!=")
					{ T.value = tok_op_noteq ; T.type = TOK_OP_NOTEQ ; }
				else if (W == "!*")
					{ T.value = tok_op_not ; T.type = TOK_OP_NOT ; toklist.Add(T) ; T.col++ ; T.value = tok_indir1 ; T.type = TOK_INDIR ; }
				else if (W == "!")
					{ T.value = tok_op_not ; T.type = TOK_OP_NOT ; }
				else
					bOk = false ;
				break ;

			case '~':
				T.value = tok_op_invert ; T.type = TOK_OP_INVERT ;
				break ;

			default:
				bOk = false ;
			}

			if (!bOk)
				{ slog.Out("Line %d: Illegal operator (%s)\n", T.line, *W) ; rc = E_SYNTAX ; break ; }
			if (T.type == TOK_UNKNOWN)
				{ slog.Out("Line %d: Illegal typeless tokne (%s)\n", T.line, *W) ; rc = E_SYNTAX ; break ; }
			toklist.Add(T) ;
			continue ;
		}

		/*
		**	Check for compiler directives
		*/

		T.col = zi.Col() ;
		word.Clear() ;
		if (*zi == CHAR_HASH)
			{ word.AddByte(CHAR_HASH) ; zi++ ; }

		for (; !zi.eof() && IsAlphanum(*zi) ; word.AddByte(*zi), zi++) ;
		W = word ;

		X = (char*) 0 ;

		if (W[0] == CHAR_HASH)
		{

			if		(W == "#if")		{ T.type = TOK_DIR_IF ;			X = tok_hash_if ; }
			else if (W == "#ifdef")		{ T.type = TOK_DIR_IFDEF ;		X = tok_hash_ifdef ; }
			else if (W == "#ifndef")	{ T.type = TOK_DIR_IFNDEF ;		X = tok_hash_ifndef ; }
			else if (W == "#else")		{ T.type = TOK_DIR_ELSE ;		X = tok_hash_else ; }
			else if (W == "#elseif")	{ T.type = TOK_DIR_ELSEIF ;		X = tok_hash_elseif ; }
			else if (W == "#endif")		{ T.type = TOK_DIR_ENDIF ;		X = tok_hash_endif ; }
			else if (W == "#define")	{ T.type = TOK_DIR_DEFINE ;		X = tok_hash_define ; }
			else if (W == "#include")	{ T.type = TOK_DIR_INCLUDE ;	X = tok_hash_include ; }

			if (X)
				{ T.value = X ; toklist.Add(T) ; continue ; }

			/*
			**	Check for the '#' char. As this will now be a literal value
			*/
		}

		if (IsAlphanum(W[0]))
		{
			//	Check for keywords
			if (W == tok_using)			{ T.type = TOK_KW_USING ;		T.value = tok_using ;		toklist.Add(T) ;	continue ; }
			if (W == tok_namespace)		{ T.type = TOK_KW_NAMESPACE ;	T.value = tok_namespace ;	toklist.Add(T) ;	continue ; }
			if (W == tok_typedef)		{ T.type = TOK_TYPEDEF ;		T.value = tok_typedef ;		toklist.Add(T) ;	continue ; }
			if (W == tok_inline)		{ T.type = TOK_KW_INLINE ;		T.value = tok_inline ;		toklist.Add(T) ;	continue ; }
			if (W == tok_static)		{ T.type = TOK_KW_STATIC ;		T.value = tok_static ;		toklist.Add(T) ;	continue ; }
			if (W == tok_extern)		{ T.type = TOK_KW_EXTERN ;		T.value = tok_extern ;		toklist.Add(T) ;	continue ; }
			if (W == tok_friend)		{ T.type = TOK_KW_FRIEND ;		T.value = tok_friend ;		toklist.Add(T) ;	continue ; }
			if (W == tok_virtual)		{ T.type = TOK_KW_VIRTUAL ;		T.value = tok_virtual ;		toklist.Add(T) ;	continue ; }
			if (W == tok_public)		{ T.type = TOK_KW_PUBLIC ;		T.value = tok_public ;		toklist.Add(T) ;	continue ; }
			if (W == tok_private)		{ T.type = TOK_KW_PRIVATE ;		T.value = tok_private ;		toklist.Add(T) ;	continue ; }
			if (W == tok_protected)		{ T.type = TOK_KW_PROTECTED ;	T.value = tok_protected ;	toklist.Add(T) ;	continue ; }
			if (W == tok_using)			{ T.type = TOK_KW_USING ;		T.value = tok_using ;		toklist.Add(T) ;	continue ; }
			if (W == tok_operator)		{ T.type = TOK_KW_OPERATOR ;	T.value = tok_operator ;	toklist.Add(T) ;	continue ; }
			if (W == tok_register)		{ T.type = TOK_KW_REGISTER ;	T.value = tok_register ;	toklist.Add(T) ;	continue ; }

			//	Check for template statements
			if (W == "template")		{ T.type = TOK_TEMPLATE ;		T.value = tok_template ;	toklist.Add(T) ;	continue ; }

			//	Check for C/C++ constructs
			if (W == "class")			{ T.type = TOK_CLASS ;			T.value = tok_class ;		toklist.Add(T) ;	continue ; }
			if (W == "struct")			{ T.type = TOK_STRUCT ;			T.value = tok_struct ;		toklist.Add(T) ;	continue ; }
			if (W == "union")			{ T.type = TOK_UNION ;			T.value = tok_union ;		toklist.Add(T) ;	continue ; }
			if (W == "enum")			{ T.type = TOK_ENUM ;			T.value = tok_enum ;		toklist.Add(T) ;	continue ; }
			if (W == "unsigned")		{ T.type = TOK_VTYPE_UNSIGNED ;	T.value = tok_unsigned ;	toklist.Add(T) ;	continue ; }
			if (W == "void")			{ T.type = TOK_VTYPE_VOID ;		T.value = tok_void ;		toklist.Add(T) ;	continue ; }
			if (W == "char")			{ T.type = TOK_VTYPE_CHAR ;		T.value = tok_char ;		toklist.Add(T) ;	continue ; }
			if (W == "short")			{ T.type = TOK_VTYPE_SHORT ;	T.value = tok_short ;		toklist.Add(T) ;	continue ; }
			if (W == "int")				{ T.type = TOK_VTYPE_INT ;		T.value = tok_int ;			toklist.Add(T) ;	continue ; }
			if (W == "long")			{ T.type = TOK_VTYPE_LONG ;		T.value = tok_long ;		toklist.Add(T) ;	continue ; }
			if (W == "const")			{ T.type = TOK_VTYPE_CONST ;	T.value = tok_const ;		toklist.Add(T) ;	continue ; }
			if (W == "mutable")			{ T.type = TOK_VTYPE_MUTABLE ;	T.value = tok_mutable ;		toklist.Add(T) ;	continue ; }

			//	Check for C++ commands
			if (W == "if")				{ T.type = TOK_CMD_IF ;			T.value = tok_cmd_if ;		toklist.Add(T) ;	continue ; }
			if (W == "else")			{ T.type = TOK_CMD_ELSE ;		T.value = tok_cmd_else ;	toklist.Add(T) ;	continue ; }
			if (W == "switch")			{ T.type = TOK_CMD_SWITCH ;		T.value = tok_cmd_switch ;	toklist.Add(T) ;	continue ; }
			if (W == "case")			{ T.type = TOK_CMD_CASE ;		T.value = tok_cmd_case ;	toklist.Add(T) ;	continue ; }
			if (W == "default")			{ T.type = TOK_CMD_DEFAULT ;	T.value = tok_cmd_default ;	toklist.Add(T) ;	continue ; }
			if (W == "for")				{ T.type = TOK_CMD_FOR ;		T.value = tok_cmd_for ;		toklist.Add(T) ;	continue ; }
			if (W == "do")				{ T.type = TOK_CMD_DO ;			T.value = tok_cmd_do ;		toklist.Add(T) ;	continue ; }
			if (W == "while")			{ T.type = TOK_CMD_WHILE ;		T.value = tok_cmd_while ;	toklist.Add(T) ;	continue ; }
			if (W == "break")			{ T.type = TOK_CMD_BREAK ;		T.value = tok_cmd_break ;	toklist.Add(T) ;	continue ; }
			if (W == "continue")		{ T.type = TOK_CMD_CONTINUE ;	T.value = tok_cmd_cont ;	toklist.Add(T) ;	continue ; }
			if (W == "goto")			{ T.type = TOK_CMD_GOTO ;		T.value = tok_cmd_goto ;	toklist.Add(T) ;	continue ; }
			if (W == "return")			{ T.type = TOK_CMD_RETURN ;		T.value = tok_cmd_return ;	toklist.Add(T) ;	continue ; }
			if (W == "new")				{ T.type = TOK_CMD_NEW ;		T.value = tok_cmd_new ;		toklist.Add(T) ;	continue ; }
			if (W == "delete")			{ T.type = TOK_CMD_DELETE ;		T.value = tok_cmd_delete ;	toklist.Add(T) ;	continue ; }
			if (W == "this")			{ T.type = TOK_OP_THIS ;		T.value = tok_this ;		toklist.Add(T) ;	continue ; }
			if (W == "sizeof")			{ T.type = TOK_OP_SIZEOF ;		T.value = tok_sizeof ;		toklist.Add(T) ;	continue ; }
			if (W == "dynamic_cast")	{ T.type = TOK_OP_DYN_CAST ;	T.value = tok_dyn_cast ;	toklist.Add(T) ;	continue ; }

			//	Boolean true/false
			if (W == "true")			{ T.type = TOK_BOOLEAN ;		T.value = tok_bool_true ;	toklist.Add(T) ;	continue ; }
			if (W == "false")			{ T.type = TOK_BOOLEAN ;		T.value = tok_bool_false ;	toklist.Add(T) ;	continue ; }

			/*
			**	Is the token a numeric value of some description? Otherwise the alphanumic sequence has not been reinterpreted so we
			**	accept as a alphanumeric token
			*/

			T.value = W ;
			T.type = TOK_WORD ;

			if (IsDigit(W[0]))
			{
				i = *W ;

				if (i[0] == '0' && (i[1] == 'x' || i[1] == 'X'))
				{
					//	Check for Octal & Hax formats
					if (atom.SetNumber(i + 2) == E_OK)
						T.type = TOK_HEXNUM ;
				}
				else if (i[0] == '0' && i[1] >= '0' && i[1] <= '7')
				{
					T.type = TOK_OCTNUM ;
				}
				else
				{
					//	if (IsDouble(dblVal, i))
					//		T.type = TOK_STDNUM ;
					//	else
						if (IsPosint(intval, *W))
						{
							slog.Out("%s. File %s Line %d, col %d: Case 2 NUMBER\n", *_fn, *fname, zi.Line(), zi.Col(), *W) ;
							T.type = TOK_NUMBER ;
						}
				}
			}

			if (W == "true" || W == "false")
				T.type = TOK_BOOLEAN ;

			toklist.Add(T) ;
			continue ;
		}

		//	Should not get to here. If we do, try to build word from the chars to the end of the line and report the sequence below
		if (!W)
		{
			for (word.Clear() ; !zi.eof() && *zi != CHAR_NL ; word.AddByte(*zi), zi++) ;
			W = word ;
		}

		slog.Out("%s. File %s Line %d, col %d: Unknown sequence [%s]\n", *_fn, *fname, zi.Line(), zi.Col(), *W) ;
		rc = E_SYNTAX ;
		break ;
	}

	//	Iterate through tokens assigning orig position
	for (intval = 0 ; intval < toklist.Count() ; intval++)
	{
		toklist[intval].orig = intval ;
	}

	return rc ;
}

hzEcode	MatchTokens	(hzVect<ceToken>& T)
{
	//	Match tokens need only be applied to tokens deemed as active by the pre-processor. Matching does three things as follows:-
	//
	//		1) Set each token's next pointer to the next token so that the [] operator can be applied to tokens.
	//		2) The tokens are matched on parenthesis.
	//		3) Set token level. Tokens outside of any curly brackets (at the file level) are at 0, those inside are at a higer level.

	_hzfunc(__func__) ;

	ceToken	tok ;

	uint32_t	nIndex ;
	uint32_t	nCount ;
	uint32_t	nLevel ;
	hzEcode		rc = E_OK ;

	//	Step 1 - Set next pointers
	for (nIndex = 0 ; nIndex < T.Count() ; nIndex++)
	{
		tok = T[nIndex] ;
		tok.match = 0 ;
	}

	//	Step 2 - Match all curly braces
	for (nIndex = 0 ; nIndex < T.Count() ; nIndex++)
	{
		if (T[nIndex].type == TOK_CURLY_OPEN)
		{
			for (nLevel = 1, nCount = nIndex + 1 ; nLevel && nCount < T.Count() ; nCount++)
			{
				if (T[nCount].type == TOK_CURLY_OPEN)	nLevel++ ;
				if (T[nCount].type == TOK_CURLY_CLOSE)	nLevel-- ;
			}

			if (nLevel)
			{
				slog.Out("%s. line %d cols %d Token {} not matched\n", *_fn, T[nIndex].line, T[nIndex].col) ;
				rc = E_SYNTAX ;
				break ;
			}

			nCount-- ;
			T[nIndex].match = nCount ;
			T[nCount].match = nIndex ;
			continue ;
		}
	}

	//	Step 3 - Match all round braces
	for (nIndex = 0 ; nIndex < T.Count() ; nIndex++)
	{
		if (T[nIndex].type == TOK_ROUND_OPEN)
		{
			for (nLevel = 1, nCount = nIndex + 1 ; nLevel && nCount < T.Count() ; nCount++)
			{
				if (T[nCount].type == TOK_ROUND_OPEN)	nLevel++ ;
				if (T[nCount].type == TOK_ROUND_CLOSE)	nLevel-- ;
			}

			if (nLevel)
			{
				slog.Out("%s. line %d cols %d Token () not matched\n", *_fn, T[nIndex].line, T[nIndex].col) ;
				rc = E_SYNTAX ;
				break ;
			}

			nCount-- ;
			T[nIndex].match = nCount ;
			T[nCount].match = nIndex ;
			continue ;
		}
	}

	//	Step 4 - Match all round braces
	for (nIndex = 0 ; nIndex < T.Count() ; nIndex++)
	{
		if (T[nIndex].type == TOK_SQ_OPEN)
		{
			for (nLevel = 1, nCount = nIndex + 1 ; nLevel && nCount < T.Count() ; nCount++)
			{
				if (T[nCount].type == TOK_SQ_OPEN)	nLevel++ ;
				if (T[nCount].type == TOK_SQ_CLOSE)	nLevel-- ;
			}

			if (nLevel)
			{
				slog.Out("%s. line %d cols %d Token [] not matched\n", *_fn, T[nIndex].line, T[nIndex].col) ;
				rc = E_SYNTAX ;
				break ;
			}

			nCount-- ;
			T[nIndex].match = nCount ;
			T[nCount].match = nIndex ;
		}
	}

	//	Step 5 - Match all query tokens to colons
	for (nIndex = 0 ; nIndex < T.Count() ; nIndex++)
	{
		if (T[nIndex].type == TOK_OP_QUERY)
		{
			for (nLevel = 1, nCount = nIndex + 1 ; nLevel && nCount < T.Count() ; nCount++)
			{
				if (T[nCount].type == TOK_OP_QUERY)	nLevel++ ;
				if (T[nCount].type == TOK_OP_SEP)	nLevel-- ;
			}

			if (nLevel)
			{
				slog.Out("%s. line %d cols %d Token ?: not matched\n", *_fn, T[nIndex].line, T[nIndex].col) ;
				rc = E_SYNTAX ;
				break ;
			}

			nCount-- ;
			T[nIndex].match = nCount ;
			T[nCount].match = nIndex ;
		}
	}

	//	Step 6 - Set level of tokens
	for (nIndex = nLevel = 0 ; nIndex < T.Count() ; nIndex++)
	{
		if (T[nIndex].type == TOK_CURLY_OPEN)
			nLevel++ ;

		T[nIndex].codeLevel = nLevel ;

		if (T[nIndex].type == TOK_CURLY_CLOSE)
			nLevel-- ;

		if (nLevel < 0)
			Fatal("%s. Level mismatch case 1\n", *_fn) ;
	}

	if (nLevel)
		Fatal("%s. Level mismatch case 2\n", *_fn) ;

	return rc ;
}
