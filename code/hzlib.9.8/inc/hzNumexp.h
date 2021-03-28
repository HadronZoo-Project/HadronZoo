//
//	File:	hzNumexp.h
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
//	This package provides a means of evaluating arithmetic (numerical) expressions. An expression as a minimum, has a single term but otherwise
//	has the general form of 'term operator remainder-of-expression' (effectively term-operator-term). In the single term case, evaluation of an
//	expression is done by evaluating the term and returning the result. In all other cases evaluation of an expression is done by evaluating the
//	first term, then recursing to evaluate the remainder-of-expression as though it were just a second term, and then applying the operator to
//	the to the two evaluations and returning the result.
//
//	Note that only binary operators can be applied to two terms. Unary operators are applied directly to the terms as part of term evaluation.
//
//	Five classes are used as follows:-
//
//	1)	hzNumexpTerm	This is the 'term' and is the base class for two types of term as follows:-
//
//	a)	hzNumexpValue	This is fixed value of any legal data type.
//
//	b)	hzNumexpForm	By having this as a derivative of hzNumexpTerm is what enables expressions to be built in tree form. It is comprised
//						of a single binary operator and a pair of operands which can be themselves hzNumexpForm instances.
//
//	2)	hzNumexp		This is the class that parses the expression.
//

#ifndef hzNumexp_h
#define hzNumexp_h

/*
**	Other includes and re-emptive class defs
*/

#include "hzTokens.h"

/*
**	Enumerated operators
*/

enum	hzUnary
{
	//	Category:	Expression
	//
	//	Unary operators

	UOP_NULL,		//	No operator
	UOP_NOT,		//	!	reverse bool
	UOP_COMP		//	~	apply one's compliment
} ;

enum	hzOperator
{
	//	Category:	Expression
	//
	//	Binary operators

	OP_NULL,		//	No operator
	OP_ASSIGN,		//	=
	OP_EQUAL,		//	==
	OP_GT,			//	>
	OP_LT,			//	<
	OP_GTEQ,		//	>=
	OP_LTEQ,		//	<=
	OP_PLUS,		//	+
	OP_MINUS,		//	-
	OP_MULT,		//	*
	OP_DIVIDE,		//	/
	OP_POWER,		//	^
	OP_AND,			//	&&
	OP_OR			//	||
} ;

class	hzNumexpTerm
{
	//	Category:	Expression
	//
	//	Base class unifying the three primary entities of constants, references and formulae

public:
	virtual	void	SetUnaryOp	(hzUnary uop) = 0 ;
	virtual	double	Evaluate	(void) = 0 ;
} ;

class	hzNumexpValue	: public hzNumexpTerm
{
	//	Category:	Expression
	//
	//	The hzNumexpValue class is a value of any legal HadronZoo type

protected:
	double	m_Atom ;		//	Result of term eveluation
	hzUnary	m_eUnary ;		//	Unary operator to apply to the result (if applicable)

public:
	hzNumexpValue	(void)	{}
	~hzNumexpValue	(void)	{}

	void	operator=	(double v)		{ m_Atom = v ; }

	void	SetUnaryOp	(hzUnary eUOP)	{ m_eUnary = eUOP ; }
	double	Evaluate	(void)			{ return m_Atom ; }
} ;

/*
**	The hzNumexpForm class provides the method of parenthesis
*/

class	hzNumexpForm	: public hzNumexpTerm
{
	//	Category:	Expression
	//
	//	The hzNumexpForm class provides the method of parenthesis by allowing two operands to be tied to a binary operator. The operands
	//	can themselves be formulae and so any expression can be represented as a series of hzNumexpForm instance. Any unary operations
	//	are to be applied to the formula as a whole and not to the individual operands.

protected:
	double		m_Result ;			//	Result of expression evaluation

	hzNumexpTerm*	m_pA ;			//	First term in numeric expression
	hzNumexpTerm*	m_pB ;			//	Second term in numeric expression

	hzUnary		m_eUnary ;			//	Unary operator to apply to the result (if applicable)
	hzOperator	m_eBinary ;			//	Operator to apply to the two terms

public:
	hzNumexpForm	(void)	{}
	~hzNumexpForm	(void)	{}

	//	These called by hzNumexp::Parse()
	void	SetUnaryOp	(hzUnary uop)		{ m_eUnary = uop ; }
	void	SetOperator	(hzOperator bOp)	{ m_eBinary = bOp ; }

	bool	AddOperand	(hzNumexpTerm* pOperand)
	{
		if (!m_pA)
			{ m_pA = pOperand ; return true ; }
		if (!m_pB)
			{ m_pB = pOperand ; return true ; }
		return false ;
	}

	double	Evaluate	(void) ;
} ;

class	hzNumexp
{
	//	Category:	Expression
	//
	//	The hzNumexp class can now evaluate an unlimited tree of constants, variables and expressions in parenthesis.

protected:
	hzVect<hzToken>	m_Tokens ;			//	Tokens of numeric expression

	hzNumexpTerm*	m_pRoot ;			//	Expression root
	hzChain			m_Error ;			//	Error reporting
	double			m_Default ;			//	Default value
	uint32_t		m_nParLevel ;		//	Current level in expression tree
	uint32_t		m_tokIter ;			//	Token iterator

	//uint32_t	_ProcessReference	(void) ;

public:
	hzNumexp	(void)
	{
		m_pRoot = 0 ;
		m_nParLevel = 0 ;
	}

	~hzNumexp	(void)	{}

	hzChain&	Show	(void)	{ return m_Error ; }

	double	Evaluate	(void) ;
	hzEcode	Parse		(const char* pExpression) ;

	hzNumexpTerm*	Parse	(void) ;
} ;

#endif	//	hzNumexp_h
