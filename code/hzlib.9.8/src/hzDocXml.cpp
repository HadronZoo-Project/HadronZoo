//
//	File:	hzDocXml.cpp
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
//	Generic XML parsing
//

#include <fstream>

#include <sys/stat.h>

#include "hzChars.h"
#include "hzChain.h"
#include "hzDirectory.h"
#include "hzDocument.h"
#include "hzTextproc.h"
#include "hzProcess.h"

using namespace std ;

hzDocXml::hzDocXml	(void)
{
	m_pRoot = 0 ;
	m_FileEpoch = 0 ;
	m_bXmlesce = 0 ;
	_hzGlobal_Memstats.m_numDocxml++ ;
}

hzDocXml::~hzDocXml   (void)
{
	threadLog("Called hzDocXml::~hzDocXml\n") ;
	Clear() ;
	_hzGlobal_Memstats.m_numDocxml-- ;
	threadLog("Exiting hzDocXml::~hzDocXml\n") ;
}

int32_t	hzDocXml::_proctagopen	(hzXmlNode** ppChild, hzXmlNode* pParent, chIter& ci)
{
	//	Determine if the current char (in the working chain) amounts to the start of an XML tag instance (node). If is is then a new
	//	node is allocated and the pointer to this returned to the caller (hzDocXml::Load()) via the first argument. If the current
	//	char in the chain does not amount to the start of the tag ..
	//
	//	Arguments:	1)	ppChild	Populated with a pointer for a new node if allocatedi (we are at an opening tag)
	//				2)	pParent	The pointer to the current node (which will be new node's parent)
	//				3)	ci		The chain iterator being processed
	//
	//	Returns:	-1	We are not at an opening tag or the format is errant
	//				0	If the tag is valid and left open. The iterator will be advanced to the closing '>' char.
	//				1	If the tag is valid but self closed. The iterator will be advanced to the closing '>' char.

	_hzfunc("_proctagopen (XML tags)") ;

	hzChain			W ;					//	For building tokens
	chIter			zi ;				//	For iterating tag
	chIter			xi ;				//	Reference iterator
	hzXmlNode*		pNewnode ;			//	Pointer to node
	hzNumPair		attr ;				//	Attribute name/value pair
	hzString		Name ;				//	Tag/node or attr name
	hzString		Value ;				//	Attr value
	hzString		S ;					//	Temp string
	uint32_t		nCol ;				//	Column
	uint32_t		strNo_name ;		//	String number for name
	bool			bOpen = false ;		//	Opening tag indicator
	bool			bXE = false ;		//	XMLesce mode

	if (!ppChild)
		Fatal("%s (Non app fn): No child node recepticle\n", *_fn) ;
	*ppChild = 0 ;

	if (*ci != '<')
		return -1 ;
	nCol = ci.Col() ;
	zi = ci ;
	zi++ ;

	//	Get tag name which must start with an alphanum but then may contain colon, minus or uscore
	if (*zi <= CHAR_SPACE)	return -1 ;
	if (!IsAlphanum(*zi))	return -1 ;

	for (xi = zi ;; xi++)
	{
		if (IsAlphanum(*xi) || *xi == CHAR_COLON || *xi == CHAR_MINUS || *xi == CHAR_USCORE)
			W.AddByte(*xi) ;
		else
			break ;
	}

	Name = W ;
	W.Clear() ;
	zi = xi ;

	//	Now we have the name we can see if the tag is on the m_Xmlesce list. If it is we set a flag. Later this flag will be used to
	//	suppress the formation of subnodes where tags which could be legal HTML tags are encountered.

	if (m_Xmlesce.Count())
	{
		if (m_Xmlesce.Exists(Name))
			bXE = true ;
	}

	/*
	**	If at tag end
	*/

	strNo_name = m_Dict.Insert(*Name) ;

	if (*zi == '>')
	{
		pNewnode = new hzXmlNode() ;
		pNewnode = pNewnode->Init(this, pParent, strNo_name, zi.Line(), nCol, bXE) ;

		ci = zi ;
		*ppChild = pNewnode ;
		if (pParent)
			pParent->AddNode(pNewnode) ;
		return 0 ;
	}

	if (zi == " />")
		zi++ ;
	if (zi == "/>")
	{
		//	Self closing tag without any parameters (null tags, legal though, eg <br/>)
		pNewnode = new hzXmlNode() ;
		pNewnode = pNewnode->Init(this, pParent, strNo_name, zi.Line(), nCol, bXE) ;

		zi++ ;
		ci = zi ;
		*ppChild = pNewnode ;
		if (pParent)
			pParent->AddNode(pNewnode) ;
		return 1 ;
	}

	/*
	**	Get tag params if any
	*/

	//	If no whitespace there are no params and tag is not a tag
	if (!IsWhite(*zi))
	{
		threadLog("%s: Line %d: tag %s expected whitespace before attr list\n", *_fn, zi.Line(), *Name) ;
		return -1 ;
	}

	//	Allocate node for tag
	pNewnode = new hzXmlNode() ;
	pNewnode = pNewnode->Init(this, pParent, strNo_name, zi.Line(), nCol, bXE) ;

	zi.Skipwhite() ;
	for (;;)
	{
		//	Get param name (note that these can contain colons as well as a-z, A-Z and 0-9
		for (xi = zi ; !xi.eof() ; xi++)
		{
			if (IsAlphanum(*xi) || *xi == CHAR_COLON || *xi == CHAR_PERIOD || *xi == CHAR_MINUS || *xi == CHAR_USCORE)
				W.AddByte(*xi) ;
			else
				break ;
		}

		Name = W ;
		W.Clear() ;

		if (*xi != CHAR_EQUAL)
			{ threadLog("%s: Line %d: param name (%s) not followed by a '='\n", *_fn, zi.Line(), *Name) ; return -1 ; }

		zi = xi ;
		zi++ ;

		//	Get param value
		if (*zi == CHAR_DQUOTE)
		{
			zi++ ;
			for (xi = zi ; *xi && *xi != CHAR_DQUOTE ; xi++)
				W.AddByte(*xi) ;

			if (*xi != CHAR_DQUOTE)
				{ threadLog("%s: Line %d: unmatched double-quote\n", *_fn, zi.Line()) ; return -1 ; }

			Value = W ;
			W.Clear() ;
			zi = xi ;
			zi++ ;
		}
		else
		{
			if (!IsAlphanum(*zi))
				{ threadLog("%s: Line %d: Non-alphanumeric parameter value\n", *_fn, zi.Line()) ; return -1 ; }

			for (xi = zi ; !xi.eof() && IsAlphanum(*xi) ; xi++)
				W.AddByte(*xi) ;

			Value = W ;
			W.Clear() ;
			zi = xi ;
		}

		attr.m_snName = m_Dict.Insert(*Name) ;
		attr.m_snValue = m_Dict.Insert(*Value) ;
		//attributes.Add(attr) ;
		m_NodeAttrs.Insert(pNewnode->GetUid(), attr) ;

		//	Check chars. At this point we could have "/>", ">", " />", " >" or whitespace followed by another param

		zi.Skipwhite() ;

		if (zi == "/>")
			{ zi++ ; break ; }
		if (zi == '>')
			{ bOpen = true ; break ; }

		if (!IsAlphanum(*zi))
		{
			threadLog("%s. Line %d: Illegal char (%c) in tag (name=%s, value=%s)\n", *_fn, zi.Line(), *zi, *Name, *Value) ;
			return -1 ;
		}
	}

	//	if (attributes.Count())
	//		pNewnode->_setnodeattrs(attributes) ;

	if (bOpen)
		*ppChild = pNewnode ;
	if (pParent)
		pParent->AddNode(pNewnode) ;

	ci = zi ;
	return bOpen ? 0:1 ;
}

bool	_istagclose	(hzString& S, hzChain::Iter& ci)
{
	//	Determine if the current location in the string corresponds to the end of an XML tag.
	//
	//	Arguments:	1)	S	A hzString reference for the tagname
	//				2)	ci	The chain iterator into the XML source
	//
	//	Returns:	True	If the XML pointer is at the end of the tag
	//				False	Otherwise.

	hzChain	W ;		//	For building tokens
	chIter	zi ;	//	For iterating tag

	zi = ci ;
	if (zi != "</")
		return false ;

	for (zi += 2 ; IsAlphanum(*zi) || *zi == CHAR_COLON || *zi == CHAR_MINUS || *zi == CHAR_USCORE ; zi++)
		W.AddByte(*zi) ;

	if (*zi != CHAR_MORE)
		return false ;

	ci += 2 ;
	S = W ;
	ci = zi ;
	return true ;
}

/*
**	hzAttrset members
*/

hzAttrset&	hzAttrset::operator=	(hzHtmElem* pNode)
{
	m_NodeUid = pNode->GetUid() ;

	if (pNode)
		m_pHostDoc = pNode->GetHostDoc() ;

	if (!m_pHostDoc)
	{
		m_Current = m_Start = m_Final = -1 ;
		m_Pair.m_snName = m_Pair.m_snValue = 0 ;
	}
	else
	{
		m_Current = m_Start = m_pHostDoc->m_NodeAttrs.First(pNode->GetUid()) ;
		if (m_Start >= 0)
			m_Final = m_pHostDoc->m_NodeAttrs.Last(pNode->GetUid()) ;

		m_Pair = m_pHostDoc->m_NodeAttrs.GetObj(m_Current) ;
	}
}

hzAttrset&	hzAttrset::operator=	(hzXmlNode* pNode)
{
	m_NodeUid = pNode->GetUid() ;

	if (pNode)
		m_pHostDoc = pNode->GetHostDoc() ;

	if (!m_pHostDoc)
	{
		m_Current = m_Start = m_Final = -1 ;
		m_Pair.m_snName = m_Pair.m_snValue = 0 ;
	}
	else
	{
		m_Current = m_Start = m_pHostDoc->m_NodeAttrs.First(pNode->GetUid()) ;
		if (m_Start >= 0)
			m_Final = m_pHostDoc->m_NodeAttrs.Last(pNode->GetUid()) ;

		m_Pair = m_pHostDoc->m_NodeAttrs.GetObj(m_Current) ;
	}
}

void	hzAttrset::Advance (void)
{
	//  Advance the iterator

	if (m_Current == m_Final)
		m_Pair.m_snName = m_Pair.m_snValue = 0 ;
	else
	{
		m_Current++ ;
		m_Pair = m_pHostDoc->m_NodeAttrs.GetObj(m_Current) ;
	}
}

bool	hzAttrset::NameEQ    (const char* cstr) const
{
	//	Confirm or deny that the current attribute name equals the supplied null terminated string
	//
	//	Argument:	cstr	The test name
	//
	//	Returns:	True	If the supplied test string matches the name
	//				false	otherwise

	if (!m_pHostDoc)
		return false ;

	return CstrCompare(m_pHostDoc->Xlate(m_Pair.m_snName), cstr) == 0 ? true : false ;
}

bool	hzAttrset::ValEQ    (const char* cstr) const
{
	//	Confirm or deny that the current attribute value equals the supplied null terminated string
	//
	//	Argument:	cstr	The test string
	//
	//	Returns:	True	If the supplied test string matches the name
	//				false	otherwise

	if (!m_pHostDoc)
		return false ;

	return CstrCompare(m_pHostDoc->Xlate(m_Pair.m_snValue), cstr) == 0 ? true : false ;
}

const char*	hzAttrset::Name    (void) const
{
	//  Return the attribute name

	if (!m_pHostDoc)			return 0 ;
	if (m_Pair.m_snName < 1)	return 0 ;

	return m_pHostDoc->Xlate(m_Pair.m_snName) ;
}

const char*	hzAttrset::Value   (void) const
{
	//  Return the attribute name

	if (!m_pHostDoc)			return 0 ;
	if (m_Pair.m_snValue < 1)	return 0 ;

	return m_pHostDoc->Xlate(m_Pair.m_snValue) ;
}

/*
**	hzXmlNode members
*/

hzXmlNode*	hzXmlNode::Init	(hzDocXml* pHostDoc, hzXmlNode* pParent, uint32_t snName, uint32_t nLineNo, uint32_t nCol, bool bXmlesce)
{
	//	Initialize a hzXmlNode, insert it into the host document's array of nodes and return the in-situ pointer
	//
	//	Arguments:	1)	pHostDoc	The host document
	//				2)	pParent		The parent node (to this)
	//				3)	snName		The string number for the node name
	//				4)	nLineNo		The config file line number
	//				5)	nCol		The config file column position
	//				6)	bXmlesce	Flag for XML-esce rules
	//
	//	Returns:	Pointer to node in-situ

	_hzfunc("hzXmlNode::Init") ;

	hzXmlNode*	pInSitu ;	//	Final address in host document array of nodes

	if (!pHostDoc)
		hzexit(_fn, 0, E_ARGUMENT, "No host document supplied") ;

	m_pHostDoc = pHostDoc ;

	if (!pParent)
	{
		m_Parent = 0 ;
		m_nLevel = 0 ;
	}
	else
	{
		m_Parent = pParent->m_Uid ;
		m_nLevel = pParent->m_nLevel + 1 ;
	}

	m_bXmlesce = bXmlesce ? 1 : 0 ;

	m_snName = snName ;
	m_nLine = m_nAnti = nLineNo ;
	m_nCol = nCol ;

	m_Uid = pHostDoc->m_arrNodes.Count() + 1 ;
    pHostDoc->m_arrNodes.Add(*this) ;

	pInSitu = pHostDoc->m_arrNodes.InSitu(m_Uid-1) ;

	return pInSitu ;
}

const char*	hzXmlNode::TxtName	(void) const
{
	_hzfunc("hzXmlNode::TxtName") ;

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	return m_pHostDoc->Xlate(m_snName) ;
}

const char*	hzXmlNode::TxtPtxt	(void) const
{
	_hzfunc("hzXmlNode::TxtPtxt") ;

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	return m_pHostDoc->Xlate(m_snPtxt) ;
}

bool	hzXmlNode::NameEQ	(const char* testval) const
{
	_hzfunc("hzXmlNode::NameEQ") ;

	const char*	i ;			//	Node name

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	i = m_pHostDoc->Xlate(m_snName) ;

	return !strcmp(i, testval) ? true : false ;
}

const char*	hzXmlNode::Xlate	(uint32_t strNo) const
{
	_hzfunc("hzXmlNode::Xlate") ;

	if (!this)
		hzexit(_fn, 0, E_NOINIT, "No Node instance") ;
	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	return m_pHostDoc->Xlate(strNo) ;
}

hzXmlNode*	hzXmlNode::GetFirstChild	(void) const
{
	_hzfunc("hzXmlNode::GetFirstChild") ;

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	if (!m_Children)
		return 0 ;

	return m_pHostDoc->m_arrNodes.InSitu(m_Children-1) ;
}

hzXmlNode*	hzXmlNode::Sibling	(void) const
{
	_hzfunc("hzXmlNode::Sibling") ;

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	if (!m_Sibling)
		return 0 ;

	return m_pHostDoc->m_arrNodes.InSitu(m_Sibling-1) ;
}

hzXmlNode*	hzXmlNode::Parent	(void) const
{
	_hzfunc("hzXmlNode::Parent") ;

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	if (!m_Parent)
		return 0 ;

	return m_pHostDoc->m_arrNodes.InSitu(m_Parent-1) ;
}

hzEcode	hzXmlNode::AddNode	(hzXmlNode* pNode)
{
	//	Add the supplied node to the calling node's list of child node.
	//
	//	This is achieved by either making m_pChildren equal to the supplied node (where the calling node does not yet have chidren) or by
	//	seeking to the end of the list and then appending (each of the existing children will have its m_pSibling pointer set to the next
	//	child).
	//	
	//	Arguments:	1)	pNode	The node to be added to this node's list of child nodes
	//
	//	Returns:	E_ARGUMENT	If no node is supplied
	//				E_DUPLICATE	If the supplied node is this node
	//				E_OK		If the node is added as a child

	_hzfunc("hzXmlNode::AddNode") ;

	hzXmlNode*	tmp ;		//	XML node pointer
	//uint32_t	nodeNo ;	//	Node number

	if (!pNode)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Attempt to add a null node to %s", TxtName()) ;
	if (pNode == this)
		return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Attempt to add a node (%d) to itself (%d, %s)", pNode->m_Uid, m_Uid, TxtName()) ;

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	m_pHostDoc->m_NodesPar.Insert(m_Uid, pNode->m_Uid) ;

	if (!m_Children)
		m_Children = pNode->GetUid() ;
	else
	{
		for (tmp = GetFirstChild() ; tmp->m_Sibling ; tmp = tmp->Sibling())
		{
			if (pNode == tmp)
				return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Attempt to add an already existing node to %s", TxtName()) ;
		}
		//	for (nodeNo = m_Children ; nodeNo ; nodeNo = tmp->m_Sibling)
		//	{
		//		tmp = m_pHostDoc->m_arrNodes.InSitu(nodeNo) ;
		//		if (!tmp->m_Sibling)
		//			break ;
		//		if (pNode == tmp)
		//			return hzerr(_fn, HZ_ERROR, E_DUPLICATE, "Attempt to add an already existing node to %s", TxtName()) ;
		//	}
		tmp->m_Sibling = pNode->m_Uid ;
	}

	return E_OK ;
}

hzEcode	hzXmlNode::SetPretext	(hzChain& Z)
{
	//	Set the XML node's pretext value to that of the supplied chain.
	//
	//	Arguments:	1)	Z	The hzChain containing the pretext value
	//
	//	Returns:	E_OVERFLOW	If the chain content exceeds the maximum allowed size for a hzString
	//				E_OK		If the operation is successful

	_hzfunc("hzXmlNode::SetPretext") ;

	hzString	P ;				//	Pretext value
	hzEcode		rc = E_OK ;		//	Return code

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	if (Z.Size() > HZSTRING_MAXLEN)
		{ P = "overflow" ; rc = E_OVERFLOW ; }
	else
		P = Z ;

	m_snPtxt = m_pHostDoc->InsertStr(*P) ;
	return rc ;
}

void	hzXmlNode::SetContent	(hzChain& Z)
{
	//	SetContent is called by hzDocXml::Load() once the node content has been established by encountering the anti-tag. Technically, the content of a node is
	//	the opening tag, the anti-tag and absoultely everything in-between. That is not how HadronZoo chooses to see it. The content does not include eiher the
	//	tag or anti-tag and text content is stipped of leading and trailing whitespace. This unifies the approach between single and multi-line tags.
	//
	//	Argument:	Z	The chain containing the content
	//
	//	Returns:	None

	_hzfunc("hzXmlNode::SetContent") ;

	hzChain		X ;		//	Result chain
	chIter		zi ;	//	Input chain iterator
	chIter		xi ;	//	Input chain forward iterator

	//	Strip leading whitespace
	//for (zi = Z ; !zi.eof() && *zi <= CHAR_SPACE ; zi++) ;

	//	Strip leading newlines and tabs from tag content - but not spaces
	for (zi = Z ; !zi.eof() && *zi < CHAR_SPACE ; zi++) ;

	for (; !zi.eof() ; zi++)
	{
		if (*zi == CHAR_NL)
		{
			//	Run to end of whitespace
			for (xi = zi, xi++ ; !xi.eof() && *xi <= CHAR_SPACE ; xi++) ;

			//	Terminate if no more non-whitespace chars
			if (xi.eof())
				break ;
		}
		X.AddByte(*zi) ;
	}

	//	if (X.Size() > HZSTRING_MAXLEN)
	//		{ m_fixContent.Clear() ; m_tmpContent = X ; }
	//	else
	//		{ m_tmpContent.Clear() ; m_fixContent = X ; }

	m_fixContent = X ;
}

void	hzXmlNode::SetCDATA	(hzChain& Z)
{
	//	This does exactly the same as SetContent
	//
	//	Arguments:	1)	Z	The chain containing the content
	//
	//	Returns:	None

	//	if (Z.Size() > HZSTRING_MAXLEN)
	//		{ m_fixContent.Clear() ; m_tmpContent = Z ; }
	//	else
	//		{ m_tmpContent.Clear() ; m_fixContent = Z ; }

	m_fixContent = Z ;
}

hzXmlNode*	hzXmlNode::_findsubnode	(bool& bMatch, const hzString& name, const hzString& attr, const hzString& value)
{
	//	Test if this node has the correct name and if so, meets the attr and value criteria. If it does place this node in the result and return true.
	//
	//	If the name is right but the other criteria fails, this node is placed in the result but
	//	false is returned.
	//
	//	If this node does not have the correct name we recurse to the subnodes until we get something
	//	in the result (have found the node) or we reach a dead end. Then we just return whatever the
	//	recursed call returns.
	//
	//	Arguments:	1)	bMatch	
	//	Returns:	Pointer to subnode

	hzAttrset		ai ;		//	Attribute iterator
	hzXmlNode*		result ;	//	Subnode found
	hzXmlNode*		pN ;		//	XML node pointer
	const char*		anam ;		//	Converted attribute name
	const char*		aval ;		//	Converted attribute value

	bMatch = false ;
	result = 0 ;

	if (name == TxtName())
	{
		//	We are on the node, all we need to do is check if the attr and value criteria are met

		result = this ;

		ai = this ;
		if (ai.Valid())
		{
			//	To qualify, the node must have an attribute named attrname. If there is also a specified
			//	value, then the attribute must be of this value.

			for (; ai.Valid() ; ai.Advance())
			{
				anam = ai.Name() ; aval = ai.Value() ;

				if (attr == anam)
				{
					if (!value)
						{ bMatch = true ; break ; }

					if (value == aval)
						{ bMatch = true ; break ; }
				}
			}
		}
		else
		{
			if (!value || m_fixContent == value)
				bMatch = true ;
		}

		if (bMatch)
			return result ;
	}

	//	We are not on the required node so call this function on all subnodes

	result = 0 ;
	for (pN = GetFirstChild() ; pN ; pN = pN->Sibling())
	{
		result = pN->_findsubnode(bMatch, name, attr, value) ;

		if (bMatch)
			break ;
	}

	return result ;
}

uint32_t	hzXmlNode::_testnode	(hzVect<hzXmlNode*>& tmpResult, const char* srchExp, uint32_t& nLimit)
{
	//	This is the workhorse behind the FindSubnodes function below.
	//
	//	Split up first part of criteria (up to first period or null terminator), to a node/tag name and if present, a content speciifer
	//	(="some_value"), an attribute name (->"attr_name") an attribute content specifer.
	//
	//	We now apply the test to the current node and when required, to the children. We do not operate where nodes are at a higher
	//	level than the limit. This is because the FindSubnodes function is looking for the set of nodes matching the criteria that are
	//	found at the lowest level
	//
	//	Arguments:	1)	result	A reference to a vector of node pointers that will be populated by this operation
	//				2)	srchExp	The search expression that decendent nodes of this node, must match to be included in the result
	//				3)	nLimit	Limit the number of levels to decend
	//
	//	Returns:	Total subnodes found

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pNode ;			//	Node to be returned
	const char*		i ;				//	Criterion iterator
	const char*		j ;				//	Criterion iterator
	const char*		anam ;			//	Attribute name
	const char*		aval ;			//	Attribute value
	const char*		cpNext = 0 ;	//	Next part of search expression if present
	hzString		nname ;			//	Required name of node
	hzString		pname ;			//	Required name of node parameter (attribute)
	hzString		nvalue ;		//	Required value of node
	hzString		pvalue ;		//	Required value of parameter
	uint32_t		nSize ;			//	Size of string
	uint32_t		nTotal ;		//	Total nodes found matching criteria
	uint32_t		nA ;			//	Attribute iterator
	bool			bFound ;		//	Does this node pass this part of criteria?

	//	If we are already at too high a level, return
	if (nLimit && (m_nLevel > nLimit))
		return 0 ;

	/*
	**	Derive the required node name and if applicable, node value, attribute name and attribute value, from the first part of the
	**	criteria. The idea is to establish firstly if this node meets this criteria. If it does and there are no further criterion,
	**	we add this node to the results and exit.
	*/

	//	Get required name of node
	for (nSize = 0, i = j = srchExp ; *i ; nSize++, i++)
	{
		if (*i == CHAR_PERIOD)
			{ i++ ; cpNext = i ; break ; }
		if (*i == CHAR_EQUAL || (i[0] == CHAR_MINUS && i[1] == CHAR_MORE))
			break ;
	}

	nname.SetValue(j, nSize) ;

	//	Get required value of node if applicable
	if (*i == CHAR_EQUAL)
	{
		//	We are setting a value requirement for the node
		for (nSize = 0, i += 2, j = i ; *i != CHAR_DQUOTE ; nSize++, i++) ;
		i++ ;

		if (*i == CHAR_PERIOD)
			{ i++ ; cpNext = i ; }

		nvalue.SetValue(j, nSize) ;
	}

	//	Get required name of node parameter if applicable
	if (i[0] == CHAR_MINUS && i[1] == CHAR_MORE)
	{
		for (nSize = 0, i += 2, j = i ; *i ; nSize++, i++)
		{
			if (*i == CHAR_PERIOD)
				{ i++ ; cpNext = i ; break ; }
			if (*i == CHAR_EQUAL)
				break ;
		}

		pname.SetValue(j, nSize) ;

		//	Get required value of parameter if applicable
		if (*i == CHAR_EQUAL)
		{
			//	We are setting a value requirement for the node
			for (nSize = 0, i += 2, j = i ; *i != CHAR_SQUOTE ; nSize++, i++) ;
			i++ ;

			if (*i == CHAR_PERIOD)
				{ i++ ; cpNext = i ; }

			pvalue.SetValue(j, nSize) ;
		}
	}

	//	Now we have the first part of the criteria, we test to see if this node meets this. If it does we still have to establish if
	//	the remainder of the criteria (if it exists) is satisfied.

	bFound = false ;

	if (nname == m_pHostDoc->Xlate(m_snName))
	{
		//	We are on the specified node so if the value is not right, any named attribute does not exist or it does but with the
		//	wrong value, we return a zero (to end the examination of this branch of nodes)

		bFound = true ;

		if (nvalue)
		{
			if (nvalue != m_fixContent)
				return 0 ;
		}

		if (bFound && pname)
		{
			//	See if we can find a parameter of pname on this node
			for (ai = this ; ai.Valid() ; ai.Advance())
			{
				anam = ai.Name() ; aval = ai.Value() ;

				if (pname == anam)
				{
					if (pvalue)
					{
						if (pvalue != aval)
							continue ;
					}
					break ;
				}
			}

			if (nA == m_nAttrs)
				return 0 ;
		}
	}

	if (bFound)
	{
		//	Now we have passed the first part of the criteria, we can add this node to the results if there is no furthur criteria. But
		//	if there is, we have to establish if the remainder of the criteria is satisfied. This will nessesitate a recursive call of
		//	this function for each and every child of this node with the criteria pointer advanced. Only if at least one of these calls
		//	succeeds (returns a positive integer for nodes added to the result), can this call succeed.

		if (!cpNext)
		{
			//	if (plog)
			//		plog->Out("\tMatched. Adding %s at level %d  and position %d to array\n", *Lineage(), m_nLevel, tmpResult.Count()) ;

			nLimit = m_nLevel ;
			tmpResult.Add(this) ;
			return 1 ;
		}

		//	Test children on the further criteria
		nTotal = 0 ;
		for (pNode = GetFirstChild() ; pNode ; pNode = pNode->Sibling())
		{
			if (!pNode->IsAncestor(this))
				Fatal("Case 2: Proported child fails to be ancestor of this\n") ;

			if (nLimit && (pNode->m_nLevel > nLimit))
				continue ;

			nTotal += pNode->_testnode(tmpResult, cpNext, nLimit) ;
		}
		return nTotal ;
	}

	//	This node does not have the required name and so does not meet the first part of the criteria. However a child might meet the
	//	criteria so we try each in turn.

	nTotal = 0 ;
	for (pNode = GetFirstChild() ; pNode ; pNode = pNode->Sibling())
	{
		if (!pNode->IsAncestor(this))
			Fatal("Case 3: Proported child fails to be ancestor of this\n") ;

		if (nLimit && (pNode->m_nLevel > nLimit))
			continue ;

		nTotal += pNode->_testnode(tmpResult, srchExp, nLimit) ;
	}

	return nTotal ;
}

void	hzXmlNode::FindSubnodes	(hzVect<hzXmlNode*>& result, const char* srchExp)
{
	//	From the current node (the node used to call this member function), find all sub-nodes matching the supplied criteria.
	//
	//	Warning! This function has been known to cause a lot of confusion.
	//
	//	This function does not simply locate nodes that are children of the calling node whose name matches the supplied criteria. The
	//	aim is to locate descenant nodes, however far down the tree they are.
	//
	//	Arguments:	1)	result	A reference to a vector of nodes that will be populated by this operation
	//				2)	srchExp	The search expression that decendent nodes of this node, must match to be included in the result
	//
	//	Returns:	None

	_hzfunc("hzXmlNode::FindSubnodes") ;

	uint32_t	nLimit = 0 ;	//	Level limit

	if (!m_pHostDoc)
		hzexit(_fn, 0, E_NOINIT, "Node has no host document") ;

	result.Clear() ;
	_testnode(result, srchExp, nLimit) ;
}

hzXmlNode*	hzXmlNode::FindSubnode	(const char* srchExp)
{
	//	From the current node (the node used to call this member function), find all sub-nodes matching the supplied criteria.
	//
	//	The critieria will be that required to uniquely identify a tag and optionally that required to specify a tag attribute and optionally that
	//	required to specify a value for the tag or attribute. The convention is to use the :: symbol between tag levels where these are needed and
	//	the -> symbol to name the tag attribute and the = symbol to specify a value. Criteria can thus be of the forms:-
	//
	//	1)	tagname
	//	2)	level0tagname...levelNtagname
	//	3)	....tagname="some value"
	//	4)	....tagname->attribute
	//	5)	....tagname->attribute="some value"
	//
	//	The criteria must only serve to identify a single tag or tag attribute. All nodes matching matching this criteria are then compiled into the
	//	supplied hzVect. Note that only the nodes will be placed in the vector and not the attributes. The application will have to use the hzXmlNode
	//	member fuctions for accessing attributes and there values if these are required.
	//
	//	Arguments:	1)	srchExp	The search expression that decendent nodes of this node, must match to be included in the result
	//
	//	Returns:	Pointer to subnode matching supplied criteria
	//				NULL If no subnode matches

	hzXmlNode*	pResult ;		//	Node pointer
	char*		cpBuf ;			//	Buffer for breaking up criteria
	const char*	i ;				//	Char iterator
	const char*	x ;				//	Char iterator
	char*		j ;				//	Buffer populator
	hzString	tagname ;		//	The part of the criteria needed to name the tag
	hzString	attrname ;		//	The part of the criteria needed to name the tag attribute
	hzString	value ;			//	The part of the criteria needed to specify node or node attribute values
	bool		bMatch ;		//	Node has passed criteria

	/*
 	**	Create a temp buffer
	*/

	cpBuf = new char[strlen(srchExp) + 1] ;

	/*
 	**	We first check for criteria of the form tagname.tagname .....
	**	If we do have this we have to recurse this function
	*/

	x = strchr(srchExp, CHAR_PERIOD) ;
	if (x)
	{
		i = srchExp ;
		j = cpBuf ;

		for (; *i && *i != CHAR_PERIOD ;)
			*j++ = *i++ ;
		*j = 0 ;
		tagname = cpBuf ;

		pResult = _findsubnode(bMatch, tagname, attrname, value) ;
		delete cpBuf ;
		if (pResult && bMatch)
			return pResult->FindSubnode(x + 1) ;
		return 0 ;
	}

	/*
	**	Objain the tagname, any attribute name and value from the criteria
	*/

	i = srchExp ;
	j = cpBuf ;

	for (; *i ; i++)
	{
		if (*i == CHAR_EQUAL || (*i == '-' && i[1] == '>'))
			break ;
		*j++ = *i ;
	}
	*j = 0 ;
	tagname = j = cpBuf ;

	if (i[0] == '-' && i[1] == '>')
	{
		for (i += 2 ; *i ; i++)
		{
			if (*i == CHAR_EQUAL)
				break ;
			*j++ = *i ;
		}
		*j = 0 ;
		attrname = j = cpBuf ;
	}

	if (*i == CHAR_EQUAL)
	{
		for (i++ ; *i && *i != CHAR_SQUOTE ; i++) ;
		for (i++ ; *i ; i++)
		{
			if (*i == CHAR_SQUOTE)
				break ;
			*j++ = *i ;
		}
		*j = 0 ;
		value = cpBuf ;
	}

	delete cpBuf ;

	/*
	**	Find the subnode by recursive search
	*/

	pResult = _findsubnode(bMatch, tagname, attrname, value) ;

	if (pResult && bMatch)
		return (hzXmlNode*) pResult ;
	return 0 ;
}

void	hzXmlNode::Export_r	(hzDocXml* pDoc, hzChain& Z, uint32_t& relLine)
{
	//	Export the combined value of this node to the supplied chain. Please note this is a recursive process so the chain is not cleared. Note also the caller
	//	must provide a reference to a 32-bit unsigned value for tracking the relative line number. This number should be set to the line number of the original
	//	node.
	//
	//	This will incorporate the node name and attributes, the direct content of the node including subtags.
	//
	//	Thsis function was introduced for the purpose of establishing an MD5 value for the node. This is used in large XML config files in which many resources
	//	are configured, to determine which resources have changed and so need to be reloaded.
	//
	//	Arguments:	1)	val		The chain populated with this node's combined value
	//				2)	relLine	Relative line number
	//
	//	Returns:	None

	_hzfunc("hzXmlNode::Export_r") ;

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pSub ;			//	Subnodes
	const char*		anam ;			//	Attribute name
	const char*		aval ;			//	Attribute value
	const char*		name ;			//	Name derived from string number
	uint32_t		n ;				//	Level iterator
	hzEcode			rc = E_OK ;		//	Return code

	if (!pDoc)
		hzexit(_fn, 0, E_CORRUPT, "No host document supplied") ;

 	//	Write out the opening of the tag
	if (m_nLine > relLine)
	{
		Z.AddByte(CHAR_NL) ;
		for (n = m_nCol ; n >= 4 ; n -= 4)
			Z.AddByte(CHAR_TAB) ;
		relLine = m_nLine ;
	}

	name = pDoc->Xlate(m_snName) ;
	Z.Printf("<%s", name) ;

	for (ai = this ; ai.Valid() ; ai.Advance())
	{
		anam = ai.Name() ; aval = ai.Value() ;

		Z.Printf(" %s=\"%s\"", anam, aval) ;
	}

	//if (!m_Children && !m_tmpContent.Size() && !m_fixContent)
	if (!m_Children && !m_fixContent)
	{
		Z << "/>" ;
		return ;
	}

	Z << ">" ;

	//	Visit child nodes if any
	if (m_Children)
	{
		for (pSub = GetFirstChild() ; rc == E_OK && pSub ; pSub = pSub->Sibling())
		{
			Z << pSub->TxtPtxt() ;
			pSub->Export_r(pDoc, Z, relLine) ;
		}
	}

 	//	Then do content
	//	if (m_tmpContent.Size() || m_fixContent)
	if (m_fixContent)
	{
		if (m_nAnti > m_nLine)
		{
			Z.AddByte(CHAR_NL) ;
			for (n = m_nCol ; n > 4 ; n -= 4)
				Z.AddByte(CHAR_TAB) ;
			relLine = m_nAnti ;
		}

		//	if (m_tmpContent.Size())
		//		Z << m_tmpContent ;
		//	else
			Z << m_fixContent ;
	}

 	//	Write out the closing of the tag
	if (m_nAnti > m_nLine)
	{
		Z.AddByte(CHAR_NL) ;
		for (n = m_nCol ; n >= 4 ; n -= 4)
			Z.AddByte(CHAR_TAB) ;
		relLine = m_nAnti ;
	}
	Z.Printf("</%s>", name) ;
}

void	hzXmlNode::Export	(hzChain& Z)
{
	//	Export the combined value of this node to the supplied chain. This function is non-recursive allowing it to clear the chain at the outset. Because nodes
	//	have child nodes, the process is recursive so this function calls Export_r to effect the export.
	//
	//	This will incorporate the node name and attributes, the direct content of the node including subtags.
	//
	//	Thsis function was introduced for the purpose of establishing an MD5 value for the node. This is used in large XML config files in which many resources
	//	are configured, to determine which resources have changed and so need to be reloaded.
	//
	//	Arguments:	1)	val		The chain populated with this node's combined value
	//				2)	relLine	Relative line number
	//
	//	Returns:	None

	_hzfunc("hzXmlNode::Export") ;

	uint32_t	relLine ;		//	Line management

	Z.Clear() ;
	relLine = m_nLine ;

	Export_r(m_pHostDoc, Z, relLine) ;
}

hzEcode	hzXmlNode::SelectSubnodes	(hzVect<hzXmlNode*>& result, hzMapM<hzString,hzXmlNode*>& allsubnodes, const char* criteria)
{
	//	Select from a map of subnodes (from a call to hzXmlNode::MapAllSubnodes) according to the supplied criteria.
	//
	//	The criteria is of the form tagname...tagname->param=value in which the parameter value could be missing, both
	//	the parameter and the value coule be missing and the tagname may be singular. The tagname (if multiple) will be
	//	applied in reverse order. A node will be selected only if it's name matches the last tagname and it has a parent
	//	whose name matches the last but one tagname and a grandparent whose name matches the last but two tagname and so
	//	on until there are no more tagnames to apply.
	//
	//	1)	tagname
	//	2)	level0tagname...levelNtagname
	//	3)	....tagname="some value"
	//	4)	....tagname->attribute
	//	5)	....tagname->attribute="some value"
	//
	//	The criteria must only serve to identify a single tag or tag attribute. All nodes matching
	//	matching this criteria are then compiled into the supplied hzVect. Note that only the
	//	nodes will be placed in the vector and not the attributes. The application will have to
	//	use the hzXmlNode member fuctions for accessing attributes and there values if these are
	//	required.
	//
	//	Arguments:	1)	result		Vector of selected nodes
	//				2)	subnodes	Map of subnodes from which to select into result
	//				3)	criteria	Selection criteria
	//
	//	Returns:	E_NODATA	If there are no subnodes
	//				E_OK		If subnodes are selected

	_hzfunc("hzXmlNode::SelectSubnodes") ;

	hzVect<hzString>	ar ;	//	List of tagnames to be applied in reverse

	hzXmlNode*	pnode ;		//	Node pointer
	const char*	i ;			//	Char iterator
	const char*	j ;			//	Buffer populator
	hzString	S ;			//	The partial tagname
	uint32_t	nIndex ;	//	Iterator
	uint32_t	nLo ;		//	Iterator to first matching node
	uint32_t	nHi ;		//	Iterator to first matching node
	uint32_t	nX ;		//	Iterator between first and last

	result.Clear() ;

	//	Check input data
	if (!allsubnodes.Count())
		return E_NODATA ;

	//	If no filter
	if (!criteria || !criteria[0])
	{
		for (nIndex = 0 ; nIndex < allsubnodes.Count() ; nIndex++)
		{
			pnode = allsubnodes.GetObj(nIndex) ;
			result.Add(pnode) ;
		}
		return E_OK ;
	}

	//	Obtain the list of tagnames from the criteria.
	for (j = i = criteria ; *i ; i++)
	{
		if (i[0] == CHAR_PERIOD)
		{
			//	Make a name string and continue
			S.SetValue(j, i) ;
			ar.Add(S) ;
			j = i + 1 ;
			continue ;
		}

		if (i[0] == CHAR_EQUAL || (i[0] == CHAR_MINUS && i[1] == CHAR_MORE))
		{
			//	Make a name string and break
			S.SetValue(j, i) ;
			ar.Add(S) ;
			j = 0 ;
			break ;
		}
	}
	if (j)
	{
		S = j ;
		ar.Add(S) ;
	}

	//	Locate all nodes with name of last tagname
	//	Apply list of tagnames in reverse
	for (nIndex = ar.Count() ; nIndex ; nIndex--)
	{
		S = ar[nIndex-1] ;
	}

	nLo = allsubnodes.First(S) ;
	if (nLo < 0)
		return E_NODATA ;
	nHi = allsubnodes.Last(S) ;

	for (nX = nLo ; nX <= nHi ; nX++)
	{
		pnode = allsubnodes.GetObj(nX) ;
		if (pnode->TxtName() != S)
			continue ;

		//	We now have a node with name match on last tagname. If there are no more tagnames we add this to the list
		if (ar.Count() == 1)
		{
			result.Add(pnode) ;
			continue ;
		}
	}

	return E_OK ;
}

bool	hzXmlNode::IsAncestor	(hzXmlNode* candidate)
{
	//	Is the candidate node an ancestor of this node?
	//
	//	Arguments:	1)	candidate	The XML node to be tested as an ancestor of this node
	//
	//	Returns:	True	If the candidate node is an ancestor of this
	//				False	Otherwise

	hzXmlNode*	pNode ;			//	XML node pointer

	//	This node cannot have a non-ancestor.
	if (!candidate)
		return false ;

	//	If this node is at the same or lower level than the candidate then the candidate cannot be an ancestor of this node.
	if (m_nLevel <= candidate->m_nLevel)
		return false ;

	//	Starting at the this node's level we work back to the candidate node's level.
	for (pNode = this ; pNode->m_nLevel > candidate->m_nLevel ; pNode = pNode->Parent()) ;

	//	Now pNode is on the same level as the candidate. If the pNode and the candidate are the same node, the candidate is an ancestor
	//	of this node.

	return pNode == candidate ? true : false ;
}

hzString	hzXmlNode::Filename	(void) const
{
	//	Category:	Diagnostics
	//
	//	This provides the name of the XML file used as the source of XML this XML node is a part. This is useful in diagnosing config errors as config
	//	files can be spread accross more than one XML file (eg SiteServer configs).
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString by value being document name

	hzString	x ;			//	Target string

	if (m_pHostDoc)
		return m_pHostDoc->Filename() ;
	else
		return x ;
}

const char*	hzXmlNode::Fname	(void) const
{
	//	Category:	Diagnostics
	//
	//	This provides the name of the XML file used as the source of XML this XML node is a part. This is useful in diagnosing config errors as config
	//	files can be spread accross more than one XML file (eg SiteServer configs).
	//
	//	Arguments:	None
	//	Returns:	Filename of host document if this is known, 0 otherwise.

	return m_pHostDoc ? m_pHostDoc->Fname() : 0 ;
}

void	hzDocXml::listnodes	(void)
{
	//	Category:	Diagnostics
	//
	//	List all nodes in this XML document for diagnostic purposes
	//
	//	Arguments:	None
	//	Returns:	None

	hzAttrset		ai ;		//	Attribute iterator
	hzLogger*		pLog ;		//	Logger for the thread
	hzXmlNode*		pNode ;		//	XML node
	const char*		anam ;		//	Name derived from string number
	const char*		aval ;		//	Attribute value
	uint32_t		nN ;		//	XML Document node iterator

	pLog = GetThreadLogger() ;
	if (!pLog)
		return ;

	for (nN = 0 ; nN < m_arrNodes.Count() ; nN++)
	{
		pNode = m_arrNodes.InSitu(nN) ;

		ai = pNode ;
		if (!ai.Valid())
			pLog->Out("<%s>\n", pNode->TxtName()) ;
		else
		{
			pLog->Out("<%s ", pNode->TxtName()) ;
			for (; ai.Valid() ; ai.Advance())
			{
				anam = ai.Name() ; aval = ai.Value() ;

				pLog->Out(" %s='%s'", anam, aval) ;
			}
			pLog->Out(">\n") ;
		}
	}
}

bool	_testHtag	(hzString& tagval, hzChain::Iter& ci)
{
	//	Determine if we are at a HTML tag as opposed to a XML tag. This is important because under some circumstances, HTML tags can be legally form part of the
	//	content of an XML tag. In such circumstances it is important incidence of HTML tags do not give rise to an XML node.
	//
	//	The function tests if the supplied iterator is at the start of a HTML tag/antitag. If it is then the iterator is advanced to the end of the tag/antitag
	//	and the tag/antitag (complete with any attributes) is returned as a string. If the test fails the iterator is  not advanced and the returned string left
	//	empty.
	//
	//	Arguments:	1)	tagval	Reference to string populated in the even of a HTML tag
	//				2)	ci		Input chain iterator
	//
	//	Returns:	True	If the iterator is at the start of a legal HTML tag
	//				False	Otherwise

	hzChain		W ;				//	For building complete tag
	chIter		zi ;			//	Chain iterator
	hzHtagtype	tt ;			//	HTML tag type
	char*		i ;				//	For testing tag value
	hzString	S ;				//	Possible HTML tag/antitag
	uint32_t	len ;			//	For limiting test value
	char		quote = 0 ;		//	Quote state (either single or double)
	char		buf [20] ;		//	For compiling test value

	tagval = 0 ;
	zi = ci ;

	if (zi.eof())
		return false ;
	if (*zi != CHAR_LESS)
		return false ;

	zi++ ;
	if (*zi == CHAR_FWSLASH)
		zi++ ;

	for (i = buf, len = 0 ; len < 18 && !zi.eof() && IsAlpha(*zi) ; *i++ = *zi, len++, zi++) ;
	*i = 0 ;
	S = buf ;

	tt = Txt2Tagtype(S) ;
	if (tt == HTAG_NULL)
		return false ;

	//	We do have a HTML tag so we need to populate tagval with the complete tag
	for (zi = ci ; !zi.eof() ; zi++)
	{
		W.AddByte(*zi) ;

		if (quote)
		{
			if (*zi == quote)
				quote = 0 ;
			continue ;
		}

		if (*zi == CHAR_MORE)
			break ;

		if (*zi == CHAR_SQUOTE)
			quote = CHAR_SQUOTE ;
		if (*zi == CHAR_DQUOTE)
			quote = CHAR_DQUOTE ;
	}

	if (*zi != CHAR_MORE)
		return false ;

	tagval = W ;
	return true ;
}

hzEcode	hzDocXml::Load	(hzChain& Z)
{
	//	Loads an XML document supplied as a chain into a tree of XML nodes
	//
	//	Arguments:	1)	Z	The chain containing a full XML document
	//
	//	Returns:	E_FORMAT	If this XML document does not conform to XML
	//				E_OK		If this XML document loaded successfully

	_hzfunc("hzDocXml::Load") ;

	hzList<hzString>::Iter	exI ;	//	Excluded tags iterator if required

	std::ifstream	is ;			//	Input stream
	hzChain::Iter	ci ;			//	Chain iterator
	hzChain::Iter	xi ;			//	Chain iterator for inner loop

	hzChain		nodeContent ;		//	Chain for building node content
	hzXmlNode*	pCN = 0 ;			//	Current XML node
	hzXmlNode*	pNN ;				//	New XML node
	hzString	Test ;				//	To test if current tag is being closed
	hzString	tagval ;			//	Used by _ishtmltag() to test if we are currently at a HTML tag
	uint32_t	tagstate ;			//	Type of tag (-1 error, 0 no tag, 1 open tag 2 closed tag)
	bool		bNewline ;			//	True if we are at start of a line
	hzEcode		rc = E_OK ;			//	Return code

	m_Error.Clear() ;

	//	Can encounter chars with top bit set before the XML document gets going. Don't know why but if they occur, bypass them.
	ci = Z ;
	for (; *ci & 0x80 ; ci++) ;

	//	If the is an XML header, processes it
	if (ci == "<?xml")
	{
		//	Skip to the doctype
		for (ci++ ; *ci != CHAR_MORE ; ci++) ;
		for (ci++ ; *ci <= CHAR_SPACE ; ci++) ;
	}

	//	If the is an XML doctype, processes it
	if (ci.Equiv("<!doctype"))
	{
		//	For now just skip doctype

		for (ci += 9 ; *ci && *ci <= CHAR_SPACE ; ci++) ;

		tagstate = 1 ;
		for (; tagstate && *ci ; ci++)
		{
			if (*ci == CHAR_LESS)	tagstate++ ;
			if (*ci == CHAR_MORE)	tagstate-- ;
		}
	}

	/*
	**	Process document
	*/

	bNewline = true ;

	for (; !ci.eof() ;)
	{
		//	Handle newlines. Exclude lines begining with # and remove whitespace from start of line
		if (*ci == CHAR_CR)
			ci++ ;

		if (*ci == CHAR_NL)
			{ bNewline = true ; ci++ ; continue ; }

		if (bNewline)
		{
			//	Add the newline to the node content if there is a node and there is already some content!
			if (pCN && nodeContent.Size())
				nodeContent.AddByte(CHAR_NL) ;

			bNewline = false ;
		}

		if (!pCN)
		{
			//	If there is no current tag (the initial condition) then the only acceptable char is the start of a tag (<). If the
			//	chain iterator is pointing to a character other than < then this is an error condition.

			if (*ci != CHAR_LESS)
			{
				m_Error.Printf("%s. File %s Line %d: Encountered char (%c:%d) outside scope of any tag\n", *_fn, *m_Filename, ci.Line(), *ci, *ci) ;
				rc = E_SYNTAX ;
				break ;
			}
		}

		if (*ci == CHAR_LESS)
		{
			//	Remove HTML type comments
			if (ci == "<!--")
			{
				for (ci += 4 ; !ci.eof() ; ci++)
				{
					if (*ci == CHAR_MINUS && ci == "-->")
						break ;
				}
				if (ci.eof())
				{
					m_Error.Printf("%s: File %s Line %d: HTML comment block begins which is not terminated\n", *_fn, *m_Filename, ci.Line()) ;
					rc = E_SYNTAX ;
					break ;
				}
				ci += 3 ;
				continue ;
			}

			//	Handle <![CDATA[...]]> block by converting the innards to straight data
			if (ci == "<![CDATA[")
			{
				xi = ci ;
				for (xi += 9 ; !xi.eof() ; xi++)
				{
					if (xi == "]]>")
						{ xi += 3 ; ci = xi ; break ; }
					nodeContent.AddByte(*xi) ;
				}
				//	Bypass the entity conversions
				pCN->SetCDATA(nodeContent) ;
				nodeContent.Clear() ;
				continue ;
			}

			//	If bXmlesce is set, treat any legal HTML tag or antitag as node content
			if (m_bXmlesce)
			{
				//	Call _testHtag to see if the < marks the start of a HTML tag/antitag. If it does it is added to the content of the current node.
				if (_testHtag(tagval, ci))
				{
					nodeContent << *tagval ;
					ci += tagval.Length() ;
					continue ;
				}
			}

			if (pCN && pCN->IsXmlesce())
			{
				//	The current tag allows HTML tags as content
				if (_testHtag(tagval, ci))
				{
					threadLog("m_bXmlesce is on node=%s\n", *tagval) ;
					nodeContent << *tagval ;
					ci += tagval.Length() ;
					continue ;
				}
			}

			/*
			**	Handle tag open
			*/

			//	threadLog("Calling proctagopen with par at %p\n", pCN) ;

			tagstate = _proctagopen(&pNN, pCN, ci) ;

			if (tagstate < 0)
				{ m_Error.Printf("%s. File %s Line %d: Bad tag format\n", *_fn, *m_Filename, ci.Line()) ; break ; }

			if (tagstate == 0)
			{
				//	At a tag open. Write out any content gathered for the current tag if any and set this as pretext in the new node. Then set the
				//	current node to the new node (this will be reverted when the new node is closed)

				if (pNN)
				{
					if (nodeContent.Size())
						pNN->SetPretext(nodeContent) ;
					nodeContent.Clear() ;

					pCN = pNN ;
					if (!m_pRoot)
					{
						//	Add root and set root parent to this document
						m_pRoot = pNN ;
					}
					m_NodesName.Insert(pNN->StrnoName(), pNN->GetUid()) ;
				}
				ci++ ;
				continue ;
			}

			if (tagstate == 1)
			{
				//	At a self closing tag. This is a valid new node and will need adding to the document, together with it's pretext. But the current
				//	node should not be set to this new node as it would be in the open tag case.

				if (pNN)
				{
					if (nodeContent.Size())
						pNN->SetPretext(nodeContent) ;
					nodeContent.Clear() ;

					if (!m_pRoot)
					{
						//	Add root and set root parent to this document
						m_pRoot = pNN ;
					}
					m_NodesName.Insert(pNN->StrnoName(), pNN->GetUid()) ;
				}
				ci++ ;
				continue ;
			}
			rc = E_OK ;

			/*
			**	Handle tag close
			*/

			if (_istagclose(Test, ci))
			{
				if (Test != pCN->TxtName())
				{
					m_Error.Printf("%s: File %s Line %d: Mismatched XML tags: Current <%s> line %d closing <%s>",
						*_fn, *m_Filename, ci.Line(), pCN->TxtName(), pCN->Line(), *Test) ;
					rc = E_FORMAT ;
					break ;
				}

				if (nodeContent.Size())
					pCN->SetContent(nodeContent) ;
				nodeContent.Clear() ;
				pCN->_setanti(ci.Line()) ;
				pCN = pCN->Parent() ;
				if (pCN == 0)
					break ;
				ci++ ;
				continue ;
			}
		}

		if (pCN == 0)
			break ;

		//	Remove tabs
		if (*ci == CHAR_TAB)
			nodeContent.AddByte(CHAR_SPACE) ;
		else
			nodeContent.AddByte(*ci) ;
		ci++ ;
	}

	if (pCN)
	{
		m_Error.Printf("%s: File %s Line %d: End of file encountered whilst inside tag definition\n", *_fn, *m_Filename, ci.Line()) ;
		rc = E_FORMAT ;
	}

	return rc ;
}

hzEcode	hzDocXml::Load	(const char* fpath)
{
	//	Loads an XML document into a tree of XML nodes
	//
	//	Arguments:	1)	fpath	Pathname of XML document file
	//
	//	Returns:	E_ARGUMENT	If the file path is not supplied
	//				E_NOTFOUND	If the file path does not exist
	//				E_NODATA	If the XML file is empty
	//				E_OPENFAIL	If the XML file cannot be read
	//				E_FORMAT	If the XML file contains malformed tags
	//				E_OK		If the XML file is successfully loaded

	_hzfunc("hzDocXml::Load") ;

	ifstream	is ;	//	Input stream
	hzChain		Z ;		//	Chain for holding file content
	hzEcode		rc ;	//	Return code

	rc = OpenInputStrm(is, fpath, *_fn) ;
	if (rc != E_OK)
		return rc ;

	Z << is ;
	is.close() ;
	is.clear() ;

	m_Filename = fpath ;
	rc = Load(Z) ;

	return rc ;
}

void	hzDocXml::Clear	(void)
{
	//	Deletes all nodes from XML tree
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzDocXml::Clear") ;

	m_pRoot = 0 ;
	m_arrNodes.Clear() ;
	m_NodesName.Clear() ;
	m_NodesPar.Clear() ;
	m_NodeAttrs.Clear() ;
	m_Xmlesce.Clear() ;
	m_Dict.Clear() ;
}

hzEcode	hzDocXml::FindNodes	(hzVect<hzXmlNode*>& Nodes, const char* srchExp)
{
	//	Find all nodes within an XML document meeting the supplied search expression.
	//
	//	The critieria MUST as a minimum, specify the name of the XML nodes sought. The criteria can optionally specify an attribute the node must contain and can
	//	go on to require that the attribute has a particular value. In addition, node ancestry may be specified.
	//
	//	The notation convention is to use the :: symbol between tags to show ancestry, the -> symbol to name the tag attribute and the = symbol to specify a tag
	//	attribute value. Criteria can thus be of the forms:-
	//
	//		1)	tagname
	//		2)	level_N-1_tagname::level_N_tagname
	//		3)	...tagname->attribute_name
	//		4)	...tagname->attribute_name=attribute_value
	//
	//	All nodes matching this criteria are added to the supplied vector and appear in their order of incidence in the XML document.
	//
	//	Arguments:	1)	Nodes	Vector of nodes selected
	//				2)	srchExp	Selection criteria
	//
	//	Returns:	E_ARGUMENT	If no selection criteria is supplied
	//				E_NOTFOUND	If no nodes were selected
	//				E_OK		If nodes were selected

	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN ;			//	Node pointer
	const char*		anam ;			//	Name derived from string number
	const char*		aval ;			//	Attribute value
	const char*		i ;				//	Char iterator
	char*			j ;				//	Buffer populator
	char*			cpBuf ;			//	Buffer for breaking up criteria
	hzString		tagname ;		//	The part of the criteria needed to name the tag
	hzString		attrname ;		//	The part of the criteria needed to name the tag attribute
	hzString		value ;			//	The part of the criteria needed to specify node or node attribute values
	uint32_t		strNo_name ;	//	String number for tagname
	uint32_t		nodeNo ;		//	Node number
	uint32_t		nLo ;			//	First instance of tagname in m_AllNodes
	uint32_t		nHi ;			//	Last instance of tagname in m_AllNodes
	uint32_t		nIndex ;		//	Node iterator
	bool			bInclude ;		//	Node has passed criteria

	Nodes.Clear() ;

	if (!srchExp || !srchExp[0])
		return E_ARGUMENT ;

	/*
	**	Objain the tagname, any attribute name and value from the criteria
	*/

	j = cpBuf = new char[strlen(srchExp) + 1] ;
	for (i = srchExp ; *i ; *j++ = *i++)
	{
		if (*i == CHAR_EQUAL || (*i == '-' && i[1] == '>'))
			break ;
	}
	*j = 0 ;
	tagname = j = cpBuf ;

	if (i[0] == '-' && i[1] == '>')
	{
		for (i += 2 ; *i ; *j++ = *i++)
		{
			if (*i == CHAR_EQUAL)
				break ;
		}
		*j = 0 ;
		attrname = j = cpBuf ;
	}

	if (*i == CHAR_EQUAL)
	{
		for (i++ ; *i && *i != CHAR_DQUOTE ; i++) ;
		for (i++ ; *i && *i != CHAR_DQUOTE ; *j++ = *i++) ;
		*j = 0 ;
		value = cpBuf ;
	}

	delete cpBuf ;

	/*
	**	First obtain the nodes
	*/

	strNo_name = m_Dict.Locate(*tagname) ;

	nLo = m_NodesName.First(strNo_name) ;
	if (nLo < 0)
		{ threadLog("Cannot locate a tag of [%s] in tree\n", *tagname) ; return E_NOTFOUND ; }
	nHi = m_NodesName.Last(strNo_name) ;

	for (nIndex = nLo ; nIndex <= nHi ; nIndex++)
	{
		nodeNo = m_NodesName.GetObj(nIndex) ;
		pN = m_arrNodes.InSitu(nodeNo-1) ;

		bInclude = false ;
		if (attrname)
		{
			//	To qualify, the node must have an attribute named attrname. If there is also a specified
			//	value, then the attribute must be of this value.

			//	pAttr = pN->GetAttributes() ;
			//	for (nA = 0 ; nA < pN->GetNoAttrs() ; nA++)
			//	{
			//		anam = Xlate(pAttr[nA].snName) ;
			//		aval = Xlate(pAttr[nA].snValue) ;

			for (ai = pN ; ai.Valid() ; ai.Advance())
			{
				anam = ai.Name() ; aval = ai.Value() ;

				if (attrname == anam)
				{
					//	We have the attribute, but if there is a value this must match as well

					if (!value || value == aval)
						bInclude = true ;
					break ;
				}
			}
		}
		else
		{
			if (!value || pN->m_fixContent == value)
				bInclude = true ;
		}

		if (bInclude)
			Nodes.Add(pN) ;
	}

	return E_OK ;
}

hzString	hzDocXml::GetValue	(hzXmlNode* pRoot, hzString& Nodename, hzString& Info)
{
	//	Using a supplied starting node (arg 1) to define a sub-tree of the current document's tree of XML nodes (tags), obtain the set of nodes
	//	whose name matches the supplied node-name (arg 2). Then, depending on the value of the supplied control string (arg 3), build the string
	//	to be returned by value, as one of the following:-
	//
	//	1)	Info="aggr".	Aggregate the content of all the matching nodes.
	//	2)	Info="node".	Take the content from the first matching node.
	//	3)	Other value.	Take this value as the attribute name. The result will then be the attribute value (if found) of the first matching
	//						node.
	//
	//	Arguments:	1)	pRoot		Starting node
	//				2)	Nodename	Name nodes must have to be processed
	//				3)	Info		Processing directive
	//
	//	Returns:	Instance of hzString by value containing the requested sub-tree

	hzVect<hzXmlNode*>	nodelist ;	//	List of subnodes of (node supplied in arg 1) matching m_Slct

	hzChain			X ;				//	For aggregating content from a series of like nodes
	hzAttrset		ai ;			//	Attribute iterator
	hzXmlNode*		pN ;			//	Node pointer
	const char*		anam ;			//	Attribute value
	hzString		S ;				//	Output value (tag value garnered)
	uint32_t		nIndex ;		//	Iterator for nodelist

	if (!pRoot)
		return S ;

	pRoot->FindSubnodes(nodelist, *Nodename) ;

	if (!nodelist.Count())
		return S ;

	pN = nodelist[0] ;

	if (Info == "aggr")
	{
		//	We need a series of nodes meeting the criteria defined in m_Slct, not just a single node

		for (nIndex = 0 ; nIndex < nodelist.Count() ; nIndex++)
		{
			pN = nodelist[nIndex] ;
			X << pN->m_fixContent ;
		}
		S = X ;
	}
	else if (Info == "node")
		S = pN->m_fixContent ;
	else
	{
		if (memcmp(*Info, "->", 2) == 0)
		{
			for (ai = pN ; ai.Valid() ; ai.Advance())
			{
				anam = ai.Name() ;

				if (!strcmp(anam, *Info + 2))
				{
					S = ai.Value() ;
					break ;
				}
			}
		}
	}

	return S ;
}

hzEcode	hzDocXml::Export	(hzChain& Z)
{
	//	Exports the in-memory XML document as XML. Place resulting text in the supplied chain
	//
	//	Arguments:	1)	Z	Chain to be populated by this operation as the XML form of this document
	//
	//	Returns:	E_NODATA	If this XML document is empty
	//				E_OK		If the document is exported

	_hzfunc("hzDocXml::Export") ;

	uint32_t	relLine = 0 ;	//	For line management

	Z.Clear() ;

	if (!m_pRoot)
		{ threadLog("%s. Empty document\n", *_fn) ; return E_NODATA ; }

	if (m_Info.m_urlReq)	Z.Printf("URL (req): %s\n", *m_Info.m_urlReq) ;
	if (*m_Info.m_urlAct)	Z.Printf("URL (act): %s\n", *m_Info.m_urlAct) ;

	m_pRoot->Export_r(this, Z, relLine) ;
	return E_OK ;
}

hzEcode	hzDocXml::Export	(const hzString& fpath)
{
	//	Exports the in-memory XML document as XML. Place resulting text in the supplied filepath
	//
	//	Arguments:	1)	fpath	Pathname of exported XML file
	//
	//	Returns:	E_ARGUMENT	If not export pathname is supplied
	//				E_NODATA	If this XML document is empty
	//				E_OPENFAIL	If the export file cannot be opened for writing
	//				E_OK		If this document is exported

	_hzfunc("hzDocXml::Export(file)") ;

	ofstream	os ;	//	Output stream
	hzChain		Z ;		//	Chain for output construction
	hzEcode		rc ;	//	Return code

	if (!fpath)
		return hzerr(_fn, HZ_WARNING, E_ARGUMENT, "Document un-named") ;
	if (!m_pRoot)
		return hzerr(_fn, HZ_WARNING, E_ARGUMENT, "Document empty") ;

	os.open(*fpath) ;
	if (os.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Could not open file %s\n", *fpath) ;

	rc = Export(Z) ;
	if (rc == E_OK)
		os << Z ;
	os.close() ;
	return rc ;
}
