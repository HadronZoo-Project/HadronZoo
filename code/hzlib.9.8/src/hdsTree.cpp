//
//	File:	hdsTree.cpp
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

//	Description:
//
//	Implements the hdsTree class which is described in the synopsis on Dissemino trees. The hdsApp member functions that read the configs pertinent to Dissemino
//	trees are also implemented here.

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>

#include "hzErrcode.h"
#include "hzDissemino.h"

using namespace std ;

/*
**	Functions
*/

hdsArticle*	hdsTree::AddHead	(hdsArticle* pParent, const hzString& refname, const hzString& title, bool bSlct)
{
	//	Add a heading to the tree

	_hzfunc("hdsTree::AddHead(1)") ;

	hdsArticleStd*	pItem = 0 ;

	//	Qualify item
	if (!refname)	hzexit(_fn, m_Groupname, E_ARGUMENT, "No item refname supplied") ;
	if (!title)		hzexit(_fn, m_Groupname, E_ARGUMENT, "No item title supplied") ;

	if (m_ItemsByName.Exists(refname))
		hzexit(_fn, m_Groupname, E_DUPLICATE, "Heading %s already exists", *refname) ;

	//	Create the blank article
	pItem = new hdsArticleStd() ;
	pItem->m_Refname = refname ;
	pItem->m_Title = title ;
	//pItem->m_bLink = false ;
	if (bSlct)
		pItem->m_bFlags |= HZ_TREEITEM_OPEN ;

	//	Insert the blck article
	m_ItemsByName.Insert(pItem->m_Refname, pItem) ;
	pItem->m_Uid = m_ItemsByName.Count() ;

	if (pParent)
	{
		pItem->m_ParId = pParent->m_Uid ;
		pItem->m_nLevel = pParent->m_nLevel + 1 ;
	}
	else
	{
		pItem->m_ParId = 0 ;
		pItem->m_nLevel = 0 ;
	}

	m_ItemsByParent.Insert(pItem->m_ParId, pItem) ;
	//threadLog("%s. Added tree heading (%s) uid %d title %s level %d\n", *_fn, *pItem->m_Refname, pItem->m_Uid, *pItem->m_Title, pItem->m_nLevel) ;

	return pItem ;
}

hdsArticle*	hdsTree::AddHead	(const hzString& parId, const hzString& refname, const hzString& title, bool bSlct)
{
	_hzfunc("hdsTree::AddHead(2)") ;

	hdsArticle*	pBase = 0 ;

	if (parId)
	{
		if (!m_ItemsByName.Exists(parId))
			hzexit(_fn, m_Groupname, E_NOTFOUND, m_Groupname, "Parent %s does not exist", *refname) ;
		pBase = m_ItemsByName[parId] ;
	}

	return AddHead(pBase, refname, title, bSlct) ;
}

hdsArticleStd*	hdsTree::AddItem	(hdsArticle* pParent, const hzString& refname, const hzString& headline, bool bSlct)
{
	//	Add an item. The first item should have a NULL parent
	//
	//	Arguments:	1)	pParent	Parent Node
	//				2)	pItem	This item - only use this if using a hdsArticle derivative
	//				3)	Name	Name of item
	//				4)	Link	URL of item

	_hzfunc("hdsTree::AddItem") ;

	hdsArticleStd*	pItem = 0 ;		//	New article

	//	Qualify item
	if (!refname)	hzexit(_fn, m_Groupname, E_ARGUMENT, m_Groupname, "No item refname supplied") ;
	if (!headline)	hzexit(_fn, m_Groupname, E_ARGUMENT, m_Groupname, "No item headline supplied") ;

	if (m_ItemsByName.Exists(refname))		hzexit(_fn, m_Groupname, E_DUPLICATE, m_Groupname, "Duplicate entry attempted: Item %s alredy exists", *refname) ;
	if (!m_ItemsByName.Count() && pParent)	hzexit(_fn, m_Groupname, E_CORRUPT, m_Groupname, "Adding first item %s but with non-null parent", *refname) ;

	//	ADD the item
	pItem = new hdsArticleStd() ;
	pItem->m_Refname = refname ;
	pItem->m_Title = headline ;

	if (bSlct)
		pItem->m_bFlags |= HZ_TREEITEM_OPEN ;
	pItem->m_bFlags |= HZ_TREEITEM_LINK ;

	m_ItemsByName.Insert(pItem->m_Refname, pItem) ;
	pItem->m_Uid = m_ItemsByName.Count() ;

	if (pParent)
	{
		pItem->m_ParId = pParent->m_Uid ;
		pItem->m_nLevel = pParent->m_nLevel + 1 ;
	}
	else
	{
		pItem->m_ParId = 0 ;
		pItem->m_nLevel = 0 ;
	}

	m_ItemsByParent.Insert(pItem->m_ParId, pItem) ;

	return pItem ;
}

#if 0
#endif

hdsArticleStd*	hdsTree::AddItem	(const hzString& parId, const hzString& refname, const hzString& title, bool bSlct)
{
	_hzfunc("hdsTree::AddItem(2)") ;

	hdsArticle*	pBase = 0 ;

	if (parId)
	{
		if (!m_ItemsByName.Exists(parId))
			hzexit(_fn, 0, E_NOTFOUND, "Parent %s of item %s does not exist", *parId, *refname) ;
		pBase = m_ItemsByName[parId] ;
	}

	return AddItem(pBase, refname, title, bSlct) ;
}

hdsArticleCIF*	hdsTree::AddArticleCIF	(hdsArticle* pParent, const hzString& refname, const hzString& headline, hzEcode (*pFn)(hzChain&, hdsArticleCIF*, hzHttpEvent*), bool bSlct)
{
	//	Add an item. The first item should have a NULL parent
	//
	//	Arguments:	1)	pParent	Parent Node
	//				2)	pItem	This item - only use this if using a hdsArticle derivative
	//				3)	Name	Name of item
	//				4)	Link	URL of item

	_hzfunc("hdsTree::AddItem") ;

	hdsArticleCIF*	pArtCIF = 0 ;		//	New article

	//	Qualify item
	if (!refname)	hzexit(_fn, m_Groupname, E_ARGUMENT, m_Groupname, "No item refname supplied") ;
	if (!headline)	hzexit(_fn, m_Groupname, E_ARGUMENT, m_Groupname, "No item headline supplied") ;
	if (!pFn)		hzexit(_fn, m_Groupname, E_ARGUMENT, m_Groupname, "No C-Interface function supplied") ;

	if (m_ItemsByName.Exists(refname))		hzexit(_fn, m_Groupname, E_DUPLICATE, m_Groupname, "Duplicate entry attempted: Item %s alredy exists", *refname) ;
	if (!m_ItemsByName.Count() && pParent)	hzexit(_fn, m_Groupname, E_CORRUPT, m_Groupname, "Adding first item %s but with non-null parent", *refname) ;

	//	ADD the item
	pArtCIF = new hdsArticleCIF() ;
	pArtCIF->m_Refname = refname ;
	pArtCIF->m_Title = headline ;
	pArtCIF->m_pFunc = pFn ;

	if (bSlct)
		pArtCIF->m_bFlags |= HZ_TREEITEM_OPEN ;
	pArtCIF->m_bFlags |= HZ_TREEITEM_LINK ;
    pArtCIF->m_flagVE |= VE_ACTIVE ;

	m_ItemsByName.Insert(pArtCIF->m_Refname, pArtCIF) ;
	pArtCIF->m_Uid = m_ItemsByName.Count() ;

	if (pParent)
	{
		pArtCIF->m_ParId = pParent->m_Uid ;
		pArtCIF->m_nLevel = pParent->m_nLevel + 1 ;
	}
	else
	{
		pArtCIF->m_ParId = 0 ;
		pArtCIF->m_nLevel = 0 ;
	}

	m_ItemsByParent.Insert(pArtCIF->m_ParId, pArtCIF) ;

	return pArtCIF ;
}

hdsArticle*	hdsTree::AddForm	(const hzString& parId, const hzString& refname, const hzString& title, const hzString& fname, const hzString& ctx, bool bSlct)
{
	//	Directly add a predefined form as an article, to the tree.
	//
	//	This function is called to execute the EXEC_TREE_SYNC command which is given as an <xtreeForm> tag in the configs. This is in effect shorthand. Instead
	//	of defining an article with <xarticle> and either defining or referencing a form within the article, an article with a form reference is created in one
	//	step.
	//
	//	The form in question must be predefined and the article added to the tree will solely comprise the reference to the form and have no other content.
	//
	//	Arguments:	1)	pParent	Parent Node
	//				2)	pItem	This item - only use this if using a hdsArticle derivative
	//				3)	Name	Name of item
	//				4)	Link	URL of item
	//
	//	Returns:	Pointer to article if the operation was successful
	//
	//	This function terminates execution if an error occurs. 

	_hzfunc("hdsTree::AddForm") ;

	hdsFormref*		pFormRef ;		//	New form reference
	hdsArticle*		pParent = 0 ;	//	Parent article
	hdsArticleStd*	pArt ;			//	New article

	threadLog("%s. par %s, ref=%s hdln=%s\n", *_fn, *parId, *refname, *title) ;

	//	Qualify item
	if (!refname)	hzexit(_fn, m_Groupname, E_ARGUMENT, "No item refname supplied") ;
	if (!title)		hzexit(_fn, m_Groupname, E_ARGUMENT, "No item article title supplied") ;
	if (!fname)		hzexit(_fn, m_Groupname, E_ARGUMENT, "No form name supplied") ;
	if (!ctx)		hzexit(_fn, m_Groupname, E_ARGUMENT, "No form data class context supplied") ;

	pArt = (hdsArticleStd*) m_ItemsByName[refname] ;
	if (pArt)
		return pArt ;

	if (parId)
	{
		if (!m_ItemsByName.Exists(parId))
			hzexit(_fn, m_Groupname, E_NOTFOUND, "Parent %s of item %s does not exist", *parId, *refname) ;
		pParent = m_ItemsByName[parId] ;
	}

	if (m_ItemsByName.Exists(refname))		hzexit(_fn, m_Groupname, E_DUPLICATE, "Duplicate entry attempted: Item %s alredy exists", *refname) ;
	if (!m_ItemsByName.Count() && pParent)	hzexit(_fn, m_Groupname, E_CORRUPT, "Adding first item %s but with non-null parent", *refname) ;

	//	ADD the item
	pArt = new hdsArticleStd() ;
	pArt->m_Refname = refname ;
	pArt->m_Title = title ;
	//pArt->m_bLink = true ;
	//pArt->m_bOpen = bSlct ;
	if (bSlct)
		pArt->m_bFlags |= HZ_TREEITEM_OPEN ;
	pArt->m_bFlags |= HZ_TREEITEM_FORM ;
	pArt->m_bFlags |= HZ_TREEITEM_LINK ;

	//	Add the form reference to the article
	pFormRef = new hdsFormref(0) ;
	pFormRef->m_Formname = fname ;
	pArt->AddFormref(pFormRef) ;
	pArt->AddVisent(pFormRef) ;

	m_ItemsByName.Insert(pArt->m_Refname, pArt) ;
	pArt->m_Uid = m_ItemsByName.Count() ;

	if (pParent)
	{
		pArt->m_ParId = pParent->m_Uid ;
		pArt->m_nLevel = pParent->m_nLevel + 1 ;
	}
	else
	{
		pArt->m_ParId = 0 ;
		pArt->m_nLevel = 0 ;
	}

	m_ItemsByParent.Insert(pArt->m_ParId, pArt) ;
	//threadLog("%s. Added tree form ref-%s title %s level %d\n", *_fn, *pArt->m_Refname, *pArt->m_Title, pArt->m_nLevel) ;

	return pArt ;
}

#if 0
hzEcode	hdsTree::Sync	(const hdbObject& obj)
{
	//	Sync a private server tree to a (user session) hdbObject instance.
	//
	//	For the data class of the supplied standalone data object, there will be a set of forms that create and edit an instance. With these forms added to the
	//	tree as articles, it becomes possible to populate the tree in accordance with the data found in a particular instance of the class. The rule is simple.
	//	If there can be only one instance of any given form, the entry in the tree for the form stays constant. If there can be multiple instances of the form,
	//	the entry in the tree for the form will always produce the blank form, and for each instance that exists, there will be an entry directly below it.
	//
	//	The process is to go through the standalone object looking for sub-class objects that can be multiple. If any are found the refname that would apply to
	//	them as entries in the tree, would be that of the blank form with the object id as resource argument. If this refname is not already present, it is added.

	_hzfunc("hdsTree::Sync") ;

	hzArray	<uint32_t>	list ;		//	List of sub-class objects

	hdsArticle*		pArt ;			//	Article pointer
	hdsArticle*		pNew ;			//	Article pointer
	hzString		S ;				//	Temp string
	uint32_t		x ;				//	Class counter
	uint32_t		y ;				//	Object counter
	char			buf [12] ;		//	Number buffer
	hzEcode			rc = E_OK ;		//	Return code

	//	Get known sub-classes and use these to look up more class articles. If multiple instances ...
	for (x = 0 ; x < m_ItemsByName.Count() ; x++)
	{
		pArt = m_ItemsByName.GetObj(x) ;
		if (!(pArt->m_bFlags & HZ_TREEITEM_FORM))
			continue ;

		for (y = 0 ; y < list.Count() ; y++)
		{
			sprintf(buf, "-%d", list[y]) ;
			S = pArt->m_Refname ;
			S += buf ;

			if (m_ItemsByName.Exists(S))
				threadLog("%s. Already have %s\n", *_fn, *S) ;
			else
			{
				threadLog("%s. Adding sub-class item %s\n", *_fn, *S) ;
				pNew = new hdsArticle() ;
				pNew->m_Refname = S ;
				pNew->m_Title = pArt->m_Title ;
				pNew->m_ParId = pArt->m_Uid ;
				pNew->m_nLevel = pArt->m_nLevel + 1 ;
				pNew->m_bFlags |= HZ_TREEITEM_LINK ;
				pNew->m_Uid = m_ItemsByName.Count() ;
				m_ItemsByName.Insert(pNew->m_Refname, pNew) ;
				m_ItemsByParent.Insert(pNew->m_ParId, pNew) ;
			}
		}
	}

	threadLog("%s. DONE SYNC\n", *_fn) ;
	return rc ;
}
#endif

hzEcode	hdsTree::Clear	(void)
{
	_hzfunc("hdsTree::Clear") ;

	hdsArticle*	pItem ;		//	Current item
	uint32_t		n ;			//	Items iterator

	for (n = 0 ; n < m_ItemsByName.Count() ; n++)
	{
		pItem = m_ItemsByName.GetObj(n) ;
		delete pItem ;
	}

	m_ItemsByParent.Clear() ;
	m_ItemsByName.Clear() ;
}

hzEcode	hdsTree::_procArticle	(hzChain& Z, hdsArticle* parent)
{
	//	Support function for ExportDataScript (see below)
	//
	//	Adds tree item to supplied chain
	//
	//	Arguments:	1)	J		Chain in which tree is being built
	//				2)	pItem	Tree item to process
	//
	//	Returns:	E_ARGUMENT	If the tree item pointer is NULL
	//				E_OK		If the tree is exported to the supplied chain

	_hzfunc("hdsTree::_procArticle") ;

	hdsArticle*		pItem ;		//	Next tree item
	hdsArticleStd*	pArtStd ;	//	Next tree item
	hdsArticleCIF*	pArtCIF ;	//	Next tree item
	hzString		parnam ;	//	Refname of parent if applicable
	hzString		state ;		//	Open or closed
	uint32_t		id ;		//	ID of item
	int32_t			nLo ;		//	Lowest position in m_AllItemsByParent
	int32_t			nHi ;		//	Higest position in m_AllItemsByParent
	int32_t			n ;			//	Item counter

	id = parent ? parent->m_Uid : 0 ;

	threadLog("%s. Item uid %d", *_fn, id) ;
	nLo = m_ItemsByParent.First(id) ;
	if (nLo < 0)
	{
		threadLog(" ... dead end\n") ;
		return E_OK ;
	}
	nHi = m_ItemsByParent.Last(id) ;
	threadLog(" ... items %d to %d\n", nLo, nHi) ;

	if (parent)
		parnam = parent->m_Refname ;

	//	Print:	Item is a folder. In the first instance (the only time the level is 1) for channel J we don't want to start with a comma, otherwise we do
	for (n = nLo ; n <= nHi ; n++)
	{
		pItem = m_ItemsByParent.GetObj(n) ;
		threadLog("\t[%s -> %s %s]\n", *parnam, *pItem->m_Refname, *pItem->m_Title) ;

		pArtStd = dynamic_cast<hdsArticleStd*>(pItem) ;
		if (pArtStd)
		{
			if (pArtStd->m_bFlags & HZ_TREEITEM_OPEN)
				state = "open" ;
			else
				state = "closed" ; 

			if (!pArtStd->m_Content)
			{
				if (pArtStd->m_bFlags & HZ_TREEITEM_LINK)
					Z.Printf("\n<xtreeItem parent=\"%s\" id=\"%s\" hdln=\"%s\" display=\"%s\"/>\n", *parnam, *pArtStd->m_Refname, *pArtStd->m_Title, *state) ;
				else
					Z.Printf("\n<xtreeHead parent=\"%s\" id=\"%s\" hdln=\"%s\" display=\"%s\"/>\n", *parnam, *pArtStd->m_Refname, *pArtStd->m_Title, *state) ;
			}
			else
			{
				if (pArtStd->m_bFlags & HZ_TREEITEM_LINK)
				{
					Z.Printf("\n<xtreeItem parent=\"%s\" id=\"%s\" hdln=\"%s\" display=\"%s\">\n", *parnam, *pArtStd->m_Refname, *pArtStd->m_Title, *state) ;
					Z << pArtStd->m_Content ;
					Z << "\n</xtreeItem>\n" ;
				}
				else
				{
					Z.Printf("\n<xtreeHead parent=\"%s\" id=\"%s\" hdln=\"%s\" display=\"%s\">\n", *parnam, *pArtStd->m_Refname, *pArtStd->m_Title, *state) ;
					Z << pArtStd->m_Content ;
					Z << "\n</xtreeHead>\n" ;
				}
			}
		}

		_procArticle(Z, pItem) ;
	}

	return E_OK ;
}

hzEcode	hdsTree::ExportArticleSet	(hzChain& Z)
{
	//	Purpose:	Export tree as an article set
	//
	//	Arguments:	1)	J	The chain in which tree is being built
	//
	//	Returns:	E_NODATA	If the tree is empty
	//				E_OK		If the tree is exported to the supplied chain

	_hzfunc("hdsTree::ExportArtcleSet") ;

	if (!m_ItemsByName.Count())
		return E_NODATA ;

	_procArticle(Z, 0) ;

	return E_OK ;
}

hzEcode	hdsTree::_procTreeitem	(hzChain& J, hdsArticle* parent)
{
	//	Support function for ExportDataScript (see below)
	//
	//	Adds tree item to supplied chain
	//
	//	Arguments:	1)	J		Chain in which tree is being built
	//				2)	pItem	Tree item to process
	//
	//	Returns:	E_ARGUMENT	If the tree item pointer is NULL
	//				E_OK		If the tree is exported to the supplied chain

	_hzfunc("hdsTree::_procTreeitem") ;

	hdsArticle*	pItem ;			//	Next tree item

	hzString	parnam ;		//	Refname of parent if applicable
	int32_t		nLo ;			//	Lowest position in m_AllItemsByParent
	int32_t		nHi ;			//	Higest position in m_AllItemsByParent
	int32_t		n ;				//	Item counter
	int32_t		i ;				//	Indent counter
	int32_t		nChild ;		//	True if link
	uint32_t	id ;			//	ID of item

	id = parent ? parent->m_Uid : 0 ;

	nLo = m_ItemsByParent.First(id) ;
	if (nLo < 0)
		return E_OK ;
	nHi = m_ItemsByParent.Last(id) ;

	if (parent)
		parnam = parent->m_Refname ;

	for (n = nLo ; n <= nHi ; n++)
	{
		pItem = m_ItemsByParent.GetObj(n) ;

		nChild = m_ItemsByParent.First(pItem->m_Uid) >= 0 ? 1 : 0 ;

		if (pItem->m_nLevel || n > nLo)
			J.AddByte(CHAR_COMMA) ;
		J.AddByte(CHAR_NL) ;
		for (i = pItem->m_nLevel ; i ; i--)
			J.AddByte(CHAR_TAB) ;

		if (pItem->m_bFlags & HZ_TREEITEM_LINK)
			J.Printf("[%d,%d,%d,\"%s\",\"%s\"]", pItem->m_nLevel, nChild, pItem->m_bFlags & HZ_TREEITEM_OPEN ? 1:0 , *pItem->m_Title, pItem->TxtName()) ;
		else
			J.Printf("[%d,%d,%d,\"%s\",\"%s\"]", pItem->m_nLevel, nChild, pItem->m_bFlags & HZ_TREEITEM_OPEN ? 1:0 , *pItem->m_Title, pItem->TxtName()) ;
			//J.Printf("[%d,%d,\"%s\",\"\"]", pItem->m_nLevel, nChild, *pItem->m_Title) ;

		if (nChild)
			_procTreeitem(J, pItem) ;
	}

	return E_OK ;
}

hzEcode	hdsTree::ExportDataScript	(hzChain& J)
{
	//	Purpose:	Build a HTML/JavaScript hierarchical tree.
	//
	//	Generate data script for the tree. A data script for trees is always an array of treeitems which in turn comprise four values. These are the
	//	level, a 0 or a 1 for either item/folder, the title and the URL (link to item).
	//
	//	The data script is used in conjuction with the _dsmScript_navtree/makeTree script which uses the data script to effect the tree in HTML bu a series of
	//	document.write operations.
	//
	//	Arguments:	1)	J	The chain in which tree is being built
	//
	//	Returns:	E_NODATA	If the tree is empty
	//				E_OK		If the tree is exported to the supplied chain

	_hzfunc("hdsTree::ExportDataScript") ;

	if (!this)
		hzexit(_fn, HZ_ERROR, "No instance") ;

	if (!m_ItemsByName.Count())
		return E_NODATA ;

	J << "var ntX=new Array();\nntX=[" ;

	_procTreeitem(J, 0) ;

	J << "\n\t];\n" ;

	return E_OK ;
}
