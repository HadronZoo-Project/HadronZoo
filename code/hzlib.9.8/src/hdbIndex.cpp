//
//	File:	hdbIndex.cpp
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
//	This file impliments the following classes which are part of the HadronZoo Database Suite
//
//	1)	_hz_sqle_expr		For imlimentation of searches based on SQL-esce
//	2)	hdbIndexEnum		Set of bitmaps, one per enum value
//	3)	hdbIndexUkey
//	4)	hdbIndexText
//

#include <iostream>
#include <fstream>
#include <cstdio>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hzBasedefs.h"
#include "hzString.h"
#include "hzChars.h"
#include "hzChain.h"
#include "hzDate.h"
#include "hzTextproc.h"
#include "hzTokens.h"
#include "hzCtmpls.h"
#include "hzCodec.h"
#include "hzDocument.h"
#include "hzDirectory.h"
#include "hzDatabase.h"
#include "hzProcess.h"

using namespace std ;

/*
**	SECTION 1:	SQL-esce
**
**	SQL-esce is implimented using four classes as follows:-
**
**  1) _hz_sqle_term - This is the base class for a SQL-esce term allowing a term to be of two types as follows:-
**
**  	a) _hz_sqle_unit - This is the SQL-esce term of the form member-condOp-value together with the hdbIdset needed to stored the evaluation result.
**		b) _hz_sqle_form - (formula) This comprises a boolean operator and a pair of _hz_sqle_unit or _hz_sqle_form operands - i.e. the remainder of the expression.
**
**	2) _hz_sql_expr - The root of the expression.
*/

enum	hzVconOp
{
	//	Category:	Expression
	//
	//	Binary ID-set operators

	BIN_OP_SET_NULL,		//	No operation
	BIN_OP_SET_ASSIGN,		//	=
	BIN_OP_SET_EQUAL,		//	==
	BIN_OP_SET_PLUS,		//	+
	BIN_OP_SET_MINUS,		//	-
	BIN_OP_SET_AND,			//	&&
	BIN_OP_SET_OR			//	||
} ;

class	_hz_sqle_term
{
	//	Category:	Expression
	//
	//	Base class for SQL-esce terms (see synopsis 'SQL-esce')

public:
	virtual	hzEcode	SetUop		(bool bNot) = 0 ;
	virtual	hzEcode	Evaluate	(hdbIdset& Result, hdbIndexText* pIndex) = 0 ;
} ;

class	_hz_sqle_unit	:	public	_hz_sqle_term
{
	//	Category:	Expression
	//
	//	A single SQL-esce term (see synopsis 'SQL-esce')

protected:
	hzString	m_Key ;			//	Word or value with which set is associated
	bool		m_bNot ;		//	Apply the ! unary operator when evaluating

public:
	_hz_sqle_unit		(void)
	{
		m_bNot = false ;
	}
	~_hz_sqle_unit	(void)
	{
	}

	//	Set the key
	void	SetValue	(const hzString& s)	{ m_Key = s ; }
	void	SetValue	(const char* s)		{ m_Key = s ; }

	//	Get the key
	hzString&	Key		(void)	{ return m_Key ; }

	//	Set unary op (if any)
	hzEcode	SetUop	(bool bNot)	{ m_bNot = bNot ; return E_OK ; }

	//	Evaluate (search for key on index)
	hzEcode	Evaluate	(hdbIdset& Result, hdbIndexText* pIndex) ;
	//void	Show		(hzLogger& xlog) ;
} ;

/*
**	The hzIdsetExp class provides the method of parenthesis
*/

class	_hz_sqle_form	:	public	_hz_sqle_term
{
	//	Category:	Expression
	//
	//	A single SQL-esce expression of the form term-boolean_op-remainder_of_expression (see synopsis 'SQL-esce')

protected:
	_hz_sqle_term*	m_pA ;		//	First term in SQL-esce expression
	_hz_sqle_term*	m_pB ;		//	Second term in SQL-esce expression

	hzVconOp	m_eBinary ;		//	Binary operator to apply to the two terms
	bool		m_bNot ;		//	Flag to direct negation of the result

public:
	_hz_sqle_form	(void)
	{
		m_pA = 0 ;
		m_pB = 0 ;
		m_eBinary = BIN_OP_SET_NULL ;
		m_bNot = false ;
	}
	~_hz_sqle_form	(void)
	{
	}

	hzEcode	SetUop	(bool bNot)	{ m_bNot = bNot ; return E_OK ; }

	hzEcode	AddOperand	(_hz_sqle_term* pOperand)
	{
		//	Add operand to the SQL term
		if (!m_pA)
			{ m_pA = pOperand ; return E_OK ; }
		if (!m_pB)
			{ m_pB = pOperand ; return E_OK ; }

		return E_DUPLICATE ;
	}

	hzEcode	SetBop	(hzVconOp eOperator)
	{
		//	Set SQL term-pair operator
		m_eBinary = eOperator ;
		return E_OK ;
	}

	hzEcode	Evaluate	(hdbIdset& Result, hdbIndexText* pIndex) ;
} ;

class	_hz_sqle_expr
{
	//	Category:	Expression
	//
	//	This is the class which holds and parses the expression part of the SQL-esce statement (see synopsis 'SQL-esce')

protected:
	hzVect<hzToken>	m_Tokens ;		//	Tokens of the SQL-esce expression
	_hz_sqle_term*	m_pRoot ;		//	Root term of the SQL-esce expression

	uint32_t	m_tokIter ;			//	Token iterator
	uint32_t	m_nParLevel ;		//	Current tree level

	_hz_sqle_term*	_proctoks	(void) ;

public:
	hdbIndexText*	m_pIndex ;			//	Freetext index

	_hz_sqle_expr	(void)
	{
		m_pRoot = 0 ;
		m_tokIter = 0 ;
		m_nParLevel = 0 ;
	}

	~_hz_sqle_expr	(void)	{}

	hzEcode	Evaluate	(hdbIdset& result) ;
	hzEcode	Parse		(hdbIdset& result, const hzString& pExpression) ;
} ;

/*
**	_hz_sqle_unit Functions
*/

hzEcode	_hz_sqle_unit::Evaluate	(hdbIdset& Result, hdbIndexText* pIndex)
{
	//	Evaluate (search for) this single SQL-Esce term within the supplied index and place the result in the supplied bitmap.
	//
	//	Arguments:	1)	Result	The bitmap (of document ids) to be populated by the search operation.
	//				2)	pIndex	The index to search for this term in.
	//
	//	Returns:	E_NODATA	No search term available
	//				E_ARGUMENT	No index pointer supplied
	//				E_CORRUPT	An error occurred during searching
	//				E_OK		The operation was successfule (even if nothing was found)

	_hzfunc("_hz_sqle_unit::Evaluate\n`") ;

	hzEcode	rc = E_OK ;		//	Return code

	if (!m_Key.Length())
		return hzerr(_fn, HZ_ERROR, E_NODATA, "Empty _cant") ;

	if (!pIndex)
		hzexit(_fn, 0, E_ARGUMENT, "No index supplied") ;

	rc = pIndex->Select(Result, m_Key) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Evaluating _cant [%s] failed in select\n", *m_Key) ;

	threadLog("%s: eval of %s - finds %d records\n", *_fn, *m_Key, Result.Count()) ;

	return rc ;
}

hzEcode	_hz_sqle_form::Evaluate	(hdbIdset& Result, hdbIndexText* pIndex)
{
	//	Evaluate (search for) this composite SQL-Esce term within the supplied index and place the result in the supplied bitmap.
	//
	//	Arguments:	1)	The bitmap (of document ids) to be populated by the search operation.
	//				2)	The index to search for this term in.
	//
	//	Returns:	E_NODATA	No search term available
	//				E_ARGUMENT	No index pointer supplied
	//				E_CORRUPT	An error occurred during searching
	//				E_OK		The operation was successfule (even if nothing was found)

	_hzfunc("_hz_sqle_form::Evaluate") ;

	hdbIdset	B ;		//	Working intermeadiate bitmap
	hzEcode		rc ;	//	Return code

	Result.Clear() ;

	if (!pIndex)
		hzexit(_fn, 0, E_CORRUPT, "No index supplied") ;

	threadLog("Applying formula\n") ;
	switch (m_eBinary)
	{
	case BIN_OP_SET_AND:
		threadLog("Applying formula (AND) - ") ;

		rc = m_pA->Evaluate(Result, pIndex) ;
		if (rc != E_OK)
			return hzerr(_fn, HZ_ERROR, rc, "Case 1 (AND) Corruption in index") ;

		if (Result.Count() > 0)
		{
			threadLog("%d recs with ", Result.Count()) ;

			rc = m_pB->Evaluate(B, pIndex) ;
			if (rc != E_OK)
				return hzerr(_fn, HZ_ERROR, rc, "Case 2 (AND) Corruption in index") ;

			threadLog("%d recs ", B.Count()) ;

			Result &= B ;

			threadLog(" -> %d records total\n", Result.Count()) ;
		}
		break ;

	case BIN_OP_SET_OR:
		threadLog("Applying formula (OR) - ") ;

		rc = m_pA->Evaluate(Result, pIndex) ;
		if (rc != E_OK)
			return hzerr(_fn, HZ_ERROR, rc, "Case 1 (OR) eveluation failed") ;

		threadLog("%d recs with ", Result.Count()) ;

		m_pB->Evaluate(B, pIndex) ;
		if (rc != E_OK)
			return hzerr(_fn, HZ_ERROR, rc, "Case 2 (OR) evaluation failed") ;

		threadLog("%d recs ", B.Count()) ;

		Result |= B ;

		threadLog(" -> %d records total\n", Result.Count()) ;

		break ;
	}

	threadLog("Found %d records in %d segments for formula\n", Result.Count(), Result.NoSegs()) ;
	return E_OK ;
}

/*
**	_hz_sqle_expr Functions
*/

hzEcode	_hz_sqle_expr::Evaluate	(hdbIdset& result)
{
	//	Evaluate this SQL-Esce expression within the supplied index and place the result in the supplied bitmap.
	//
	//	Arguments:	1)	The bitmap (of document ids) to be populated by the search operation.
	//
	//	Returns:	E_NODATA	No search term available
	//				E_ARGUMENT	No index pointer supplied
	//				E_CORRUPT	An error occurred during searching
	//				E_OK		The operation was successfule (even if nothing was found)

	_hzfunc("_hz_sqle_expr::Evaluate") ;

	//	Result.Clear() ;

	if (!m_pIndex)
		hzexit(_fn, 0, E_CORRUPT, "No index supplied") ;

	if (!m_pRoot)
	{
		threadLog("%s - no root!\n", *_fn) ;
		return E_OK ;
	}
	threadLog("%s calling Eval\n", *_fn) ;

	return m_pRoot->Evaluate(result, m_pIndex) ;
}

_hz_sqle_term* _hz_sqle_expr::_proctoks	(void)
{
	//	Recursively convert text tokens into tree of _hz_sqle_term instances. This is a support function to Parse()
	//
	//	Arguments:	None
	//
	//	Returns:	Pointer to a valid SQL term if tokens remain in the expression
	//				NULL Otherwise

	_hzfunc("_hz_sqle_expr::_proctoks") ;

	_hz_sqle_form*	pFormula = 0 ;		//	Pointer to rest of expression
	_hz_sqle_unit*	pConstant = 0 ;		//	Pointer to a constant (if applicaple)
	_hz_sqle_term*	pOperand1 = 0 ;		//	Pointer to 1st operand
	_hz_sqle_term*	pOperand2 = 0 ;		//	Pointer to 2nd operand

	hzString	tval ;					//	Temp string - token value
	hzVconOp	eOp = BIN_OP_SET_NULL ;	//	Applicable binary operator
	//bool		bNot = false ;			//	Negator

	threadLog("%s - Now on level %d\n", *_fn, m_nParLevel) ;

	if (m_tokIter >= m_Tokens.Count())
	{
		threadLog("%s - No more tokens - returning null\n", *_fn) ;
		return 0 ;
	}

	tval = m_Tokens[m_tokIter].Value() ;

	threadLog("%s - token is %s\n", *_fn, *tval) ;

	/*
	**	Occupy first operand
	*/

	//	Unary operators are permitted while expecting operand
	if (tval == "!")
	{
		threadLog("%s - Setting unary for first operand\n", *_fn) ;
		//bNot = true ;
		m_tokIter++ ;
		if (m_tokIter >= m_Tokens.Count())
		{
			hzerr(_fn, HZ_ERROR, E_SYNTAX, "Expected an operand or '(' to follow unary") ;
			return 0 ;
		}
		tval = m_Tokens[m_tokIter].Value() ;
	}

	//	Binary operators are not permitted here though
	if (tval == "&&" || tval == "||")
	{
		hzerr(_fn, HZ_ERROR, E_SYNTAX, "Expected an operand or '('. Got an operator") ;
		return 0 ;
	}

	if (tval == "(")
	{
		//	The operand can be a formula, recurse if it is
		m_nParLevel++ ;
		m_tokIter++ ;
		pOperand1 = _proctoks() ;
		tval = m_Tokens[m_tokIter].Value() ;
	}
	else
	{
		//	Token is not a unary or an open bracket. We assume token is a const
		threadLog("%s. making a const of %s\n", *_fn, *tval) ;
		pConstant = new _hz_sqle_unit() ;
		pConstant->SetValue(tval) ;
		pOperand1 = pConstant ;
		m_tokIter++ ;
		tval = m_Tokens[m_tokIter].Value() ;
	}

	/*
	**	Obtain the operator
	*/

	if (m_tokIter < m_Tokens.Count())
	{
		//	we now expect an binary operator or a terminating )
		if (tval == ")")
		{
			//tm.Advance() ;
			m_tokIter++ ;
			return pOperand1 ;
		}

		//	Test for operator
		if (tval == "&&" || tval == "&")
		{
			eOp = BIN_OP_SET_AND ;
			m_tokIter++ ;
			tval = m_Tokens[m_tokIter].Value() ;
		}
		else if (tval == "||" || tval == "|")
		{
			eOp = BIN_OP_SET_OR ;
			m_tokIter++ ;
			tval = m_Tokens[m_tokIter].Value() ;
		}
		else if (tval == "(")
		{
			hzerr(_fn, HZ_ERROR, E_SYNTAX, "Line %d: Expected an operator", m_Tokens[m_tokIter].LineNo()) ;
			return 0 ;
		}
		else
		{
			//	Token is not an operator and not an open or a close.
			//	We assume it is another _cant and thus there is
			//	an implied AND operator.

			threadLog("%s - asserting operator as AND\n", *_fn) ;
			eOp = BIN_OP_SET_AND ;
		}
	}

	/*
	**	Recurse to get second operand
	*/

	if (tval)
	{
		m_nParLevel++ ;
		pOperand2 = _proctoks() ;
		m_nParLevel-- ;
	}

	/*
	**	Decide if we return a const or a formula
	*/

	if (!pOperand1)
	{
		threadLog("%s - returning null (no first operand)\n", *_fn) ;
		return 0 ;
	}
	if (eOp == BIN_OP_SET_NULL)
	{
		threadLog("%s - returning single operand\n", *_fn) ;
		return pOperand1 ;
	}
	if (!pOperand2)
	{
		//	report syntax error
		threadLog("%s - returning null (no second operand)\n", *_fn) ;
		return 0 ;
	}

	//	Allocate a formula node
	threadLog("%s - returning double operand\n", *_fn) ;

	pFormula = new _hz_sqle_form() ;
	pFormula->AddOperand(pOperand1) ;
	pFormula->AddOperand(pOperand2) ;
	pFormula->SetBop(eOp) ;

	return pFormula ;
}

hzEcode	_hz_sqle_expr::Parse	(hdbIdset& result, const hzString& srchExp)
{
	//	Parse the supplied expression. This is first a matter of tokenization, then a call to the root of the expression by the recursive _proctoks()
	//	(process tokens) function. After parsing the expression is ready for evaluation.
	//
	//	Arguments:	1)	result	The result bitmap
	//				2)	srchExp	The search expression
	//
	//	Returns:	E_ARGUMENT	If no expression was supplied
	//				E_FORMAT	If no tokens were identified in the supplied expression
	//				E_OK		If no errors occured

	_hzfunc("_hz_sqle_expr::Parse") ;

	//	tokenize the expression

	if (!srchExp)
		return E_ARGUMENT ;

	TokenizeString(m_Tokens, *srchExp, TOK_MO_BOOL) ;
	if (m_Tokens.Count() == 0)
	{
		threadLog("%s: - no tokens found in expression\n", *_fn) ;
		return E_FORMAT ;
	}

	threadLog("%s: - parsing %d tokens\n", *_fn, m_Tokens.Count()) ;

	m_pRoot = _proctoks() ;
	if (!m_pRoot)
		return E_FORMAT ;
	//return Evaluate(result) ;
	return m_pRoot ? E_OK : E_FORMAT ;
}

/*
**	SECTION 2:	hdbIndexEnum Functions
*/

void	hdbIndexEnum::Halt	(void)
{
	//	Close down the enumerated index. De-allocate the bitmaps and clear the bitmap array.
	//
	//	Arguments:	None
	//	Returns:	None

	hdbIdset*	pS ;	//	Allocated bitmap
	uint32_t	n ;		//	Allowed value iterator

	for (n = 0 ; n < m_Maps.Count() ; n++)
	{
		pS = m_Maps.GetObj(n) ;
		delete pS ;
	}

	m_Maps.Clear() ;
}

hzEcode	hdbIndexEnum::Insert	(uint32_t objId, const hzAtom& Key)
{
	//	Purpose:	Add an object/key combination
	//
	//	Arguments:	1)	objId	The object id (row number)
	//				2)	Key		The key
	//
	//	Returns:	E_RANGE	If the supplied value is beyond the supported range of enum values.
	//				E_OK	If the operation was successful.

	_hzfunc("hdbIndexEnum::Insert") ;

	hdbIdset*	pS ;		//	Applicable bitmap to insert object id
	uint32_t	val ;		//	Enum value from supplied atom
	hzEcode		rc = E_OK ;	//	Return code

	val = (uint32_t) Key ;

	if (val <= 0 || val > m_Maps.Count())
		return E_RANGE ;
	pS = m_Maps[val] ;

	if ((rc = pS->Insert(objId)) != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "(case key exists) Bitmap could not insert object") ;

	return rc ;
}

hzEcode	hdbIndexEnum::Delete	(uint32_t objId, const hzAtom& Key)
{
	//	Purpose:	Delete an object/key combination
	//
	//	Arguments:	1)	objId	The object id (row number)
	//				2)	Key		The key
	//
	//	Returns:	E_RANGE	If the supplied value is beyond the supported range of enum values.
	//				E_OK	If the operation was successful.

	hdbIdset*	pS ;		//	Applicable bitmap to delete object id from
	uint32_t	val ;		//	Enum value from supplied atom
	hzEcode		rc = E_OK ;	//	Return code

	val = (uint32_t) Key ;

	if (val <= 0 || val > m_Maps.Count())
		return E_RANGE ;
	pS = m_Maps[val] ;
	pS->Delete(objId) ;

	//	if (!pS->Count())
	//		m_Maps.Delete(K) ;
	return rc ;
}

hzEcode	hdbIndexEnum::Select	(hdbIdset& Result, const hzAtom& Key)
{
	//	Purpose:	Select into a set, all identifiers matching the key
	//
	//	Arguments:	1)	The object id (row number)
	//				2)	The key
	//
	//	Returns:	E_RANGE	If the key was not located
	//				E_OK	If the operation was successful.

	_hzfunc("hdbIndexEnum::Select") ;

	hdbIdset*	pS ;		//	Applicable bitmap for value
	uint32_t	val ;		//	Enum value from supplied atom
	hzEcode		rc = E_OK ;	//	Return code

	Result.Clear() ;

	val = (uint32_t) Key ;

	//	Querry this?
	if (val <= 0 || val > m_Maps.Count())
		return E_RANGE ;

	pS = m_Maps[val] ;
	if (!pS)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Bitmap stated as existing could not be retrieved from the map") ;
	Result = *pS ;

	if (Result.Count() != pS->Count())
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Index bound bitmap has %d records, copy has %d records", pS->Count(), Result.Count()) ;

	return rc ;
}

hzEcode	hdbIndexEnum::Dump	(const hzString& Filename, bool bFull)
{
	//	Output list of keys and thier segment numbers together with the segment contents (Ids relative to the segment start)
	//
	//	Arguments:	1)	Full path of file to dump to
	//				2)	Do a full dump (true) or just segments (false)
	//
	//	Returns:	E_ARGUMENT	If the filename is not supplied
	//				E_OPENFAIL	If the supplied filename cannot be opened
	//				E_WRITEFAIL	If there was a write error
	//				E_OK		If the index was dumped to file

	_hzfunc("hdbIndexEnum::Dump") ;

	hzVect<uint32_t>	Results ;	//	Results of fetch

	ofstream	os ;				//	Output stream
	hzChain		Z ;					//	Used to build list of ids
	hdbIdset	proc ;				//	Processing bitmap
	hdbIdset*	pI ;				//	The Id set assoc with key

	uint32_t	ev ;				//	Enum value
	uint32_t	nRecs ;				//	Total number of records fetched
	uint32_t	nFetched = 0 ;		//	Number of record fetched in call to Fetch()
	uint32_t	nStart ;			//	Starter for Fetch
	char		cvLine	[120] ;		//	For output

	if (!Filename)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No filename supplied") ;

	os.open(*Filename) ;
	if (os.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open index dump file (%s)", *Filename) ;

	os << "Index Dump\n" ;

	for (ev = 0 ; ev < m_Maps.Count() ; ev++)
	{
		pI = m_Maps.GetObj(ev) ;

		if (!pI)
		{
			sprintf(cvLine, "Enum-Val %d (null list)\n", ev) ;
			os << cvLine ;
			continue ;
		}

		nRecs = pI->Count() ;
		if (!nRecs)
		{
			sprintf(cvLine, "Enum-Val %d (empty list)\n", ev) ;
			os << cvLine ;
			continue ;
		}

		sprintf(cvLine, "Enum-Val %d (%d objects)\n", ev, nRecs) ;
		os << cvLine ;

		if (!bFull)
			continue ;

		//	Extract ids from the binaries
		proc = *pI ;
		for (nStart = 0 ; nStart < nRecs ; nStart += 10)
		{
			//	for (nIndex = 0 ; nIndex < 10 ; nIndex++)
			//		Results[nIndex] = -1 ;

			nFetched = proc.Fetch(Results, nStart, 10) ;

			if (!nFetched)
				break ;

			sprintf(cvLine, "  %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d\n",
				Results[0], Results[1], Results[2], Results[3], Results[4],
				Results[5], Results[6], Results[7], Results[8], Results[9]) ;
			os << cvLine ;
		}

		if (os.fail())
		{
			os.close() ;
			return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Could not write to index dump file (%s)", *Filename) ;
		}
	}

	os << "Index Dump End\n" ;
	os.close() ;
	return E_OK ;
}

/*
**	SECTION 3:	hdbIndexUkey Functions
*/

hzEcode	hdbIndexUkey::Init	(const hdbObjRepos* pRepos, const hzString& mbrName, hdbBasetype dtype)
{
	//	Initialize the hdbIndexUkey (unique key index) to a datatype. The underlying hzMapS will be of keys to object ids but the keys may be either 4
	//	or 8 bytes in size, depending on the datatype.
	//
	//	Arguments:	1)	dtype	The data type of the member to which the index applies
	//
	//	Returns:	E_TYPE	If the supplied HadronZoo data type is undefned or cannot be applied to a hdbIndexUkey
	//				E_OK	If the operation was successful

	_hzfunc("hdbIndexUkey::Init") ;

	hzEcode	rc = E_OK ;		//	Return code

	if (m_bInit)
		return hzerr(_fn, HZ_ERROR, E_SEQUENCE, "%s already initialized", *m_Name) ;

	if (!pRepos)
		hzexit(_fn, 0, E_ARGUMENT, "No repository supplied") ;

	//m_pRepos = pRepos ;
	m_Name = pRepos->Name() ;
	m_Name += "::" ;
	m_Name += mbrName ;

	switch	(dtype)
	{
	case BASETYPE_DOMAIN:
	case BASETYPE_EMADDR:
	case BASETYPE_STRING:
	case BASETYPE_URL:		//	m_keys.pSt = new hzMapS<hzString,uint32_t> ;
							//	m_keys.pSt->SetDefaultObj(0) ;
							m_keys.pSu = new hzMapS<uint32_t,uint32_t> ;
							m_keys.pSu->SetDefaultObj(0) ;
							break ;

	case BASETYPE_IPADDR:
	case BASETYPE_TIME:
	case BASETYPE_SDATE:
	case BASETYPE_INT32:
	case BASETYPE_UINT32:	m_keys.pSu = new hzMapS<uint32_t,uint32_t> ;
							m_keys.pSu->SetDefaultObj(0) ;
							break ;

	case BASETYPE_DOUBLE:
	//case BASETYPE_UUID:
	case BASETYPE_XDATE:
	case BASETYPE_INT64:
	case BASETYPE_UINT64:	m_keys.pLu = new hzMapS<uint64_t,uint32_t> ;
							m_keys.pLu->SetDefaultObj(0) ;
							break ;
	default:
		rc = E_TYPE ;
		break ;
	}

	m_eBasetype = dtype ;
	m_bInit = true ;

	return rc ;
}

void	hdbIndexUkey::Halt	(void)
{
	//	Save index to disk and close files
	//
	//	Arguments:	None
	//	Returns:	None

	//	STUB
}

hzEcode	hdbIndexUkey::Insert	(const hzAtom& A, uint32_t objId)
{
	//	Insert an atomic-value/object-id pair into the unique key index.
	//
	//	Arguments:	1)	A		The atomic value
	//				2)	objId	The object identifier
	//
	//	Returns:	E_TYPE		If the atomic value is not of the expected data type
	//				E_NODATA	If the atomic value is not set (see note)
	//				E_OK		If the operation was successful
	//
	//	Note that as hdbIndexUkey can only be applied to data members with a minimun and maximum population of 1, the member cannot be NULL and therefore a NULL
	//	value to insert is an error.

	_hzfunc("hdbIndexUkey::Insert") ;

	uint64_t	lval ;			//	8 byte value
	uint32_t	sval ;			//	4 byte value
	hzEcode		rc = E_OK ;		//	Return code

	if (!m_bInit)
		hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	if (A.IsNull())
		return E_NODATA ;

	if (A.Type() != m_eBasetype)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Index type %s - supplied value type %s", Basetype2Txt(m_eBasetype), Basetype2Txt(A.Type())) ;

	switch (m_eBasetype)
	{
	case BASETYPE_DOMAIN:	sval = _hzGlobal_FST_Domain->Locate(*A.Str()) ;
							rc = m_keys.pSu->Insert(sval, objId) ;
							break ;

	case BASETYPE_EMADDR:	sval = _hzGlobal_FST_Emaddr->Locate(*A.Str()) ;
							rc = m_keys.pSu->Insert(sval, objId) ;
							break ;

	case BASETYPE_STRING:	
	case BASETYPE_URL:		sval = _hzGlobal_StringTable->Locate(*A.Str()) ;
							rc = m_keys.pSu->Insert(sval, objId) ;
							break ;

	case BASETYPE_DOUBLE:
	case BASETYPE_XDATE:
	case BASETYPE_INT64:
	case BASETYPE_UINT64:	lval = A.Unt64() ;
							rc = m_keys.pLu->Insert(lval, objId) ;
							break ;

	case BASETYPE_IPADDR:
	case BASETYPE_TIME:
	case BASETYPE_SDATE:
	case BASETYPE_INT32:
	case BASETYPE_UINT32:	sval = A.Unt32() ;
							rc = m_keys.pSu->Insert(sval, objId) ;
							break ;
	}

	return rc ;
}

hzEcode	hdbIndexUkey::Delete	(const hzAtom& key)
{
	//	Remove the key/object pair named by the supplied key, from the index
	//
	//	Arguments:	1)	atom	Reference to atom with the lookup value.
	//
	//	Returns:	E_NODATA	If the supplied atom has no value 
	//				E_TYPE		If the atom data type does not match that of the member to which this index applies
	//				E_NOTFOUND	If the value of the atom does not identify an object
	//				E_OK		If the value of the atom does identify an object

	_hzfunc("hdbIndexUkey::Delete") ;

	uint64_t	lval ;			//	8 byte value
	uint32_t	sval ;			//	4 byte value
	hzEcode		rc = E_OK ;		//	Return code

	if (!m_bInit)
		hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	if (key.IsNull())
		return E_NODATA ;

	if (key.Type() != m_eBasetype)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Index type %s - supplied value type %s", Basetype2Txt(m_eBasetype), Basetype2Txt(key.Type())) ;

	//	Do lookup based on type
	switch (m_eBasetype)
	{
	case BASETYPE_DOMAIN:	sval = _hzGlobal_FST_Domain->Locate(*key.Str()) ;
							rc = m_keys.pSu->Delete(sval) ;
							break ;

	case BASETYPE_EMADDR:	sval = _hzGlobal_FST_Emaddr->Locate(*key.Str()) ;
							rc = m_keys.pSu->Delete(sval) ;
							break ;

	case BASETYPE_STRING:	
	case BASETYPE_URL:		sval = _hzGlobal_StringTable->Locate(*key.Str()) ;
							rc = m_keys.pSu->Delete(sval) ;
							break ;

	case BASETYPE_DOUBLE:
	case BASETYPE_XDATE:
	case BASETYPE_INT64:
	case BASETYPE_UINT64:	memcpy(&lval, key.Binary(), 8) ;
							rc = m_keys.pLu->Delete(lval) ;
							break ;

	case BASETYPE_IPADDR:
	case BASETYPE_TIME:
	case BASETYPE_SDATE:
	case BASETYPE_INT32:
	case BASETYPE_UINT32:	memcpy(&sval, key.Binary(), 4) ;
							rc = m_keys.pSu->Delete(sval) ;
							break ;
	} ;

	return rc ;
}

hzEcode	hdbIndexUkey::Select	(uint32_t& Result, const hzAtom& key)
{
	//	Find the single object matching the supplied key - if it exists.
	//
	//	Arguments:	1)	objId	Reference to object id, set by this function
	//				2)	pAtom	Pointer to atom with the lookup value
	//
	//	Returns:	E_NOINIT	If the index is not initialized
	//				E_NODATA	If no key is supplied
	//				E_TYPE		If the atom data type does not match that of the member to which this index applies
	//				E_OK		If no errors occured

	_hzfunc("hdbIndexUkey::Select") ;

	hzString	S ;				//	Temp string
	uint64_t	lval ;			//	8 byte value
	uint32_t	sval ;			//	4 byte value
	hzEcode		rc = E_OK ;		//	Return code

	Result = 0 ;

	if (!m_bInit)
		return hzerr(_fn, HZ_ERROR, E_NOINIT) ;

	if (key.IsNull())
		return hzerr(_fn, HZ_ERROR, E_NODATA, "No key supplied") ;

	if (key.Type() != m_eBasetype)
		return hzerr(_fn, HZ_ERROR, E_TYPE, "Index type %s - supplied value type %s", Basetype2Txt(m_eBasetype), Basetype2Txt(key.Type())) ;

	switch (m_eBasetype)
	{
	case BASETYPE_DOMAIN:   S = key.Str() ;
							sval = _hzGlobal_FST_Domain->Locate(*S) ;
                            if (sval)
							{
								if (m_keys.pSu->Exists(sval))
									Result = m_keys.pSu->operator[](sval) ;
							}
                            break ;

    case BASETYPE_EMADDR:   S = key.Str() ;
							sval = _hzGlobal_FST_Emaddr->Locate(*S) ;
                            if (sval)
							{
								if (m_keys.pSu->Exists(sval))
									Result = m_keys.pSu->operator[](sval) ;
							}
                            break ;

    case BASETYPE_STRING:
    case BASETYPE_URL:      S = key.Str() ;
							sval = _hzGlobal_StringTable->Locate(*S) ;
                            if (sval)
							{
								if (m_keys.pSu->Exists(sval))
									Result = m_keys.pSu->operator[](sval) ;
							}
                            break ;

	case BASETYPE_DOUBLE:
	case BASETYPE_XDATE:
	case BASETYPE_INT64:
	case BASETYPE_UINT64:	if (key.Type() != m_eBasetype)
								{ rc = E_TYPE ; break ; }
							memcpy(&lval, key.Binary(), 8) ;
							if (m_keys.pLu->Exists(lval))
								Result = m_keys.pLu->operator[](lval) ;
							break ;

	case BASETYPE_IPADDR:
	case BASETYPE_TIME:
	case BASETYPE_SDATE:
	case BASETYPE_INT32:
	case BASETYPE_UINT32:	if (key.Type() != m_eBasetype)
								{ rc = E_TYPE ; break ; }
							sval = key.Unt32() ;
							if (m_keys.pSu->Exists(sval))
								Result = m_keys.pSu->operator[](sval) ;
							break ;
	} ;

	return rc ;
}

/*
**	SECTION 4:	hdbIndexText Functions
*/

hzEcode	hdbIndexText::Init	(const hzString& name, const hzString& opdir, const hzString& backup, uint32_t cacheMode)
{
	//	Initialize the free text index with a name, an operational directory and an optional backup directory. These parameters initialize the
	//	underlying hzIsam.
	//

	_hzfunc("hdbIndexText::Init") ;

	hzEcode	rc = E_OK ;		//	Return code

	//rc = m_Isam.Init(name, opdir, backup, cacheMode) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Failed on account of ISAM init") ;

	return rc ;
}

hzEcode	hdbIndexText::Halt		(void)
{
	//	Halt the operation of the free text index. This will halt the underlying hzIsam.

	//m_Isam.Halt() ;
	return E_OK ;
}

hzEcode	hdbIndexText::Insert	(const hzString& word, uint32_t docId)
{
	//	Locate or insert the word and its associated bitmap, assign object id in the bitmap.

	_hzfunc("hdbIndexText::Insert(int)") ;

	hdbIdset	bm ;			//	The bitmap associated with the word
	hzString	lcword ;		//	The word but all in lower case
	hzEcode		rc = E_OK ;		//	Error code

	lcword = word ;
	lcword.ToLower() ;

	if (m_Keys.Exists(lcword))
		m_Keys[lcword].Insert(docId) ;
	else
	{
		bm.Insert(docId) ;
		rc = m_Keys.Insert(lcword, bm) ;
		if (rc != E_OK)
			hzerr(_fn, HZ_ERROR, rc, "Failed to insert doc_id of %d into bitmap. Error=%s\n", docId, Err2Txt(rc)) ;
	}

	return rc ;
}

#if 0
hzEcode	hdbIndexText::Insert	(const hzString& word, const hzSet<uint32_t>& idset)
hzEcode	hdbIndexText::InsSeg	(const hzString& word, const hzBitseg& seg, uint32_t segNo)
#endif

hzEcode	hdbIndexText::Delete	(const hzString& word, uint32_t docId)
{
	//	Locate the bitmap for the word and delete the object id from it.

	hdbIdset	bm ;		//	The bitmap associated with the word
	hzString	lcword ;	//	The word but all in lower case

	lcword = word ;
	lcword.ToLower() ;

	if (m_Keys.Exists(lcword))
	{
		bm = m_Keys[lcword] ;
		bm.Delete(docId) ;
		return E_OK ;
	}

	return E_NOTFOUND ;
}

#if 0
hzEcode	hdbIndexText::Delete	(const hzString& word, const hzSet<uint32_t>& idset)
hzEcode	hdbIndexText::DelSeg	(const hzString& word, uint32_t segNo)
#endif

hzEcode	hdbIndexText::Clear		(void)
{
	//	Clear all contents of the free text index by clearing the ISAM and the map of keys

	//m_Isam.Clear() ;
	m_Keys.Clear() ;
	return E_OK ;
}

hzEcode	hdbIndexText::Select	(hdbIdset& Result, const hzString& word)
{
	//	Locate the bitmap for the word and assign it to the result

	hzString	lcword ;	//	The word but all in lower case

	lcword = word ;
	lcword.ToLower() ;

	Result.Clear() ;

	if (m_Keys.Exists(lcword))
		Result = m_Keys[lcword] ;
	return E_OK ;
}

hzEcode	hdbIndexText::Eval	(hdbIdset& result, const hzString& criteria)
{
	//	Perform a search on the freetext index and place the resulting set of document ids into the supplied bitmap (arg 1). The supplied criteria
	//	(arg 2) must comprise at least one whole word and may comprise a boolean expression.

	_hz_sqle_expr	exp ;	//	Working SQL expression
	hzEcode			rc ;	//	Return code

	rc = exp.Parse(result, criteria) ;
	if (rc != E_OK)
		return rc ;

	exp.m_pIndex = this ;
	return exp.Evaluate(result) ;
}

hzEcode	hdbIndexText::Export	(const hzString& filepath, bool bFull)
{
	//	Output list of words and their associated bitmaps to the supplied filepath. The output is human radable for diagnostic purposes 
	//
	//	Arguments:	1)	Full path of file to dump to
	//				2)	Do a full dump (true) or just segments (false)
	//
	//	Returns:	E_ARGUMENT	If the filename is not supplied
	//				E_OPENFAIL	If the supplied filename cannot be opened
	//				E_WRITEFAIL	If there was a write error
	//				E_OK		If the index was dumped to file

	_hzfunc("hdbIndexText::Export") ;

	hzVect<uint32_t>	res ;	//	Results of bitmap Fetch

	ofstream	os ;			//	Output file
	hzChain		Z ;				//	For constructing bitmap export
	hdbIdset	bm ;			//	Current word bitmap
	hdbIdset	S ;				//	Current segment
	hzString	word ;			//	Current word
	uint32_t	nIndex ;		//	Word/bitmap iterator
	uint32_t	nStart ;		//	Starting position for bitmap Fetch
	uint32_t	nFetched ;		//	Number of ids fetched
	uint32_t	nPosn ;			//	Fetch result iterator
	uint32_t	nSegs = 0 ;		//	Segment counter
	uint32_t	nInst = 0 ;		//	Incidence counter

	if (!filepath)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No pathname for Index Export") ;

	os.open(filepath) ;
	if (os.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot open %s", *filepath) ;

	threadLog("Index has:-\n") ;

	for (nIndex = 0 ; nIndex < m_Keys.Count() ; nIndex++)
	{
		word = m_Keys.GetKey(nIndex) ;
		bm = m_Keys.GetObj(nIndex) ;
		nSegs += bm.NoSegs() ;
		nInst += bm.Count() ;

		Z.Printf("%u %s: s=%u c=%u\n", nIndex, *word, bm.NoSegs(), bm.Count()) ;

		if (bFull)
		{
			for (nStart = 0 ; nStart < bm.Count() ; nStart += 20)
			{
				nFetched = bm.Fetch(res, nPosn, 20) ;

				for (nPosn = 0 ; nPosn < nFetched ; nPosn++)
					Z.Printf("\t%u", res[nPosn]) ;
			}
		}

		os << Z ;
		Z.Clear() ;
		if (os.fail())
		{
			os.close() ;
			return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Write fail to %s", *filepath) ;
		}
	}

	Z.Printf("\tWords (maps): %d\n", m_Keys.Count()) ;
	Z.Printf("\tSegments:     %d\n", nSegs) ;
	Z.Printf("\tInstances:    %d\n", nInst) ;
	os << Z ;
	os.close() ;
	Z.Clear() ;

	return E_OK ;
}
