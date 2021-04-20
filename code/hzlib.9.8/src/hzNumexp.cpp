//
//	File:	hzNumexp.cpp
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
//	Implimentation of the hzNumexp (numeric expression) class and the hzNumexpTerm classes defined in hzNumexp.h
//

#include <fstream>

#include "hzTextproc.h"
#include "hzProcess.h"

#include "hzNumexp.h"

/*
**	hzNumexpForm Functions
*/

double	hzNumexpForm::Evaluate	(void)
{
	//	Evaluate this expression (hzNumexpForm instance) to a single atomic value. A hzNumexpForm instance is created by an expression parser when the
	//	expression amounts to a 'term operator term' rather than only a single term. hzNumexpForm Evaluation is thus always a matter of evaluating
	//	both terms and applying the operator. Where the terms are themselves hzNumexpForm instances, this function recurses.
	//
	//	Arguments:	None
	//	Returns:	Value - the numeric result

	_hzfunc("hzNumexpForm::Evaluate_a") ;

	double	A ;		//	Term A
	double	B ;		//	Term B

	A = m_pA->Evaluate() ;
	B = m_pB->Evaluate() ;

	switch (m_eBinary)
	{
	//case OP_ASSIGN:	break ;
	case OP_EQUAL:	m_Result = (A == B) ;	break ;
	case OP_GT:		m_Result = (A >  B) ;	break ;
	case OP_LT:		m_Result = (A <  B) ;	break ;
	case OP_GTEQ:	m_Result = (A >= B) ;	break ;
	case OP_LTEQ:	m_Result = (A <= B) ;	break ;

	case OP_PLUS:	m_Result = A ; m_Result += B ;	break ;
	case OP_MINUS:	m_Result = A ; m_Result -= B ;	break ;
	case OP_MULT:	m_Result = A ; m_Result *= B ;	break ;
	case OP_DIVIDE:	m_Result = A ; m_Result /= B ;	break ;
	//case OP_AND:	m_Result = A ; m_Result &= B ;	break ;
	//case OP_OR:	m_Result = A ; m_Result |= B ;	break ;
	}

	return m_Result ;
}

/*
**	hzNumexp Functions
*/

double	hzNumexp::Evaluate	(void)
{
	//	Evaluate expression into single atomic value.
	//
	//	Arguments:	None
	//	Returns:	Value - the numeric result

	_hzfunc("hzNumexp::Evaluate_b") ;

	if (m_pRoot)
		return m_pRoot->Evaluate() ;
	return m_Default ;
}

hzEcode	hzNumexp::Parse	(const char* pExp)
{
	//	Parses extression into a single operand or more generally, a tree of formulae that will evaluate to a single value.
	//
	//	Arguments:	1)	pExp	Numeric expression
	//
	//	Returns:	E_SYNTAX	If the expression could not be tokenized or could not be parsed.
	//				E_OK		If the expression was parsed.

	_hzfunc("hzNumexp::Parse_a") ;

	hzEcode	rc ;	//	Return code

	//	tokenize the expression

	rc = TokenizeString(m_Tokens, pExp, TOK_MO_BOOL) ;
	if (rc != E_OK)
		return E_SYNTAX ;

	m_pRoot = Parse() ;
	if (!m_pRoot)
		return E_SYNTAX ;

	return E_OK ;
}

hzNumexpTerm*	hzNumexp::Parse	(void)
{
	//	convert text tokens into tree of hzNumexpForm, hzReference & hzNumexpValue
	//
	//	Arguments:	None
	//	Returns:	Pointer to numeric expresssion term base class instance

	_hzfunc("hzIdSetExp::Parse(hzTokenlist&)") ;

	hzNumexpForm*	pFormula = 0 ;			//	Pointer to a formula
	hzNumexpValue*	pConstant = 0 ;			//	Pointer to a constant
	hzNumexpTerm*	pOperand1 = 0 ;			//	Pointer to term A
	hzNumexpTerm*	pOperand2 = 0 ;			//	Pointer to term B
	hzOperator		binOp = OP_NULL ;		//	Binary op applicable to A and B
	hzUnary			unaOp = UOP_NULL ;		//	Unary op applicable to result
	hzToken			tok ;					//	Current token
	double			val ;					//	result value

	tok = m_Tokens[m_tokIter] ;

	if (!tok)
	{
		m_Error.Printf("No more tokens - returning null\n") ;
		return 0 ;
	}

	/*
	**	Occupy first operand
	*/

	//	Unary operators are permitted while expecting operand
	if (tok == "!")
	{
		m_Error.Printf("Setting unary for first operand\n") ;
		unaOp = UOP_NOT ;
		m_tokIter++ ;
		tok = m_Tokens[m_tokIter] ;
		if (!tok)
		{
			m_Error.Printf("Expected an operand or '(' to follow unary") ;
			return 0 ;
		}
	}

	//	Binary operators are not permitted here though
	if (tok == "&&" || tok == "||")
	{
		m_Error.Printf("Expected an operand or '('. Got an operator") ;
		return 0 ;
	}

	if (tok == "(")
	{
		//	The operand can be a formula, recurse if it is
		m_nParLevel++ ;
		m_tokIter++ ;
		tok = m_Tokens[m_tokIter] ;
		pOperand1 = Parse() ;
	}
	else
	{
		//	Token is not a unary or an open bracket. We assume token is a constant
		m_Error.Printf("Set first operand as constant\n") ;
		pConstant = new hzNumexpValue() ;
		IsDouble(val, *tok.Value()) ;
		*pConstant = val ;	//tok.Value() ;
		pOperand1 = pConstant ;
		m_tokIter++ ;
		tok = m_Tokens[m_tokIter] ;
	}

	/*
	**	Obtain the operator
	*/

	if (tok.Value())
	{
		m_Error.Printf("Processing token %s\n", *tok.Value()) ;

		//	we now expect an binary operator or a terminating )
		if (tok == ")")
		{
			m_tokIter++ ;
			tok = m_Tokens[m_tokIter] ;
			return pOperand1 ;
		}

		//	Test for operator
		if (tok == "&&")
		{
			binOp = OP_AND ;
			m_tokIter++ ;
			tok = m_Tokens[m_tokIter] ;
		}
		else if (tok == "||")
		{
			binOp = OP_OR ;
			m_tokIter++ ;
			tok = m_Tokens[m_tokIter] ;
		}
		else if (tok == "(")
		{
			m_Error.Printf("Line %d: Expected an operator", tok.LineNo()) ;
			return 0 ;
		}
		else
		{
			//	Token is not an operator and not an open or a close. We assume it is another constant and thus there is an implied AND operator.

			m_Error.Printf("asserting operator as AND\n") ;
			binOp = OP_AND ;
		}
	}

	/*
	**	Recurse to get second operand
	*/

	if (tok.Value())
	{
		m_Error.Printf("Recursing for 2nd operand\n") ;
		m_nParLevel++ ;
		pOperand2 = Parse() ;
		m_nParLevel-- ;
		if (pOperand2)
			m_Error.Printf("Got 2nd operand\n") ;
		else
			m_Error.Printf("Got 2nd operand\n") ;
	}

	/*
	**	Decide if we return a constant or a formula
	*/

	if (!pOperand1)
	{
		m_Error.Printf("returning null (no first operand)\n") ;
		return 0 ;
	}
	if (binOp == OP_NULL)
	{
		m_Error.Printf("returning single operand\n") ;
		return pOperand1 ;
	}
	if (!pOperand2)
	{
		//	report syntax error
		m_Error.Printf("returning null (no second operand)\n") ;
		return 0 ;
	}

	//	Allocate a formula node
	m_Error.Printf("returning double operand\n") ;
	pFormula = new hzNumexpForm() ;
	pFormula->AddOperand(pOperand1) ;
	pFormula->AddOperand(pOperand2) ;
	pFormula->SetUnaryOp(unaOp) ;
	pFormula->SetOperator(binOp) ;
	return pFormula ;
}
