//
//	File:	hzTree.cpp
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
//	Implements the hzTree class which is described in the synopsis on Dissemino trees. The hdsApp member functions that read the configs pertinent to Dissemino
//	trees are also implemented here.

#if 0
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>

#include "hzErrcode.h"
//#include "hzFsTbl.h"
#include "hzTree.h"

using namespace std ;

/*
**	Functions
*/

hzTreeitem*	hzTree::AddHead	(const hzString& parname, const hzString& refname, const hzString& title, bool bSlct)
{
	//	Add a heading to the tree

	_hzfunc("hzTree::AddHead(1)") ;

	hzTreeitem*	pItem = 0 ;
	hzTreeitem*	pParent = 0 ;
	uint32_t	parId ;			//	ID of item parent

	//	Qualify item
	if (!refname)	{ hzerr(_fn, HZ_WARNING, E_ARGUMENT, "No item refname supplied") ; return 0 ; }
	if (!title)		{ hzerr(_fn, HZ_WARNING, E_ARGUMENT, "No item headline supplied") ; return 0 ; }

	if (m_ItemsByName.Exists(refname))
		hzexit(_fn, m_Groupname, E_DUPLICATE, "Heading %s already exists", *refname) ;

	if (parname)
	{
		if (!m_ItemsByName.Exists(parname))
			hzexit(_fn, m_Groupname, E_NOTFOUND, m_Groupname, "Parent %s does not exist", *parname) ;
		parId = m_ItemsByName[parname] ;
		pParent = m_arrItems[parId] ;
	}

	//	Create the blank article
	pItem = new hzTreeitem() ;
	pItem->m_Refname = refname ;
	pItem->m_Title = title ;
	//pItem->m_bLink = false ;
	if (bSlct)
		pItem->m_bFlags |= HZ_TREEITEM_OPEN ;

	//	Insert the blck article
	pItem->m_Uid = m_arrItems.Count() ;
	m_arrItems.Add(pItem) ;
	m_ItemsByName.Insert(pItem->m_Refname, pItem->m_Uid) ;
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

	m_ItemsByParent.Insert(pItem->m_ParId, pItem->m_Uid) ;
	//threadLog("%s. Added tree heading (%s) uid %d title %s level %d\n", *_fn, *pItem->m_Refname, pItem->m_Uid, *pItem->m_Title, pItem->m_nLevel) ;

	return pItem ;
}

hzEcode	hzTree::AddItem	(const hzString& parname, const hzString& refname, const hzString& headline, hzTreeitem* pItem, bool bSlct)
{
	//	Add an item. The first item should have a NULL parent
	//
	//	Arguments:	1)	parname	Name of parent item
	//				2)	pItem	This item - only use this if using a hzTreeitem derivative
	//				3)	Name	Name of item
	//				4)	Link	URL of item
	//
	//	Returns:	E_ARGUMENT	If either the item, the refname or headline is not supplied
	//				E_DUPLICATE	If the item already exists in the tree
	//				E_CORRUPT	If the tree is empty and a parent has been supplied
	//				E_OK		If the operation was successful

	_hzfunc("hzTree::AddItem") ;

	hzTreeitem*	pParent = 0 ;	//	Parent item
	uint32_t	parId ;			//	ID of item parent

	threadLog("%s. parent %s, ref=%s hdln=%s\n", *_fn, *parname, *refname, *headline) ;

	//	Qualify item
	if (!pItem)		return hzerr(_fn, HZ_WARNING, E_ARGUMENT, "No item supplied") ;
	if (!refname)	return hzerr(_fn, HZ_WARNING, E_ARGUMENT, "No item refname supplied") ;
	if (!headline)	return hzerr(_fn, HZ_WARNING, E_ARGUMENT, "No item headline supplied") ;

	if (parname)
	{
		if (!m_ItemsByName.Exists(parname))
		{
			//return hzerr(_fn, HZ_WARNING, E_NOTFOUND, "Parent %s does not exist", *parname) ;
			threadLog("%s. Parent %s does not exist\n", *_fn, *parname) ;
			return E_NOTFOUND ;
		}

		parId = m_ItemsByName[parname] ;
		if (parId)
			pParent = m_arrItems[parId] ;
	}

	//if (m_ItemsByName.Exists(refname))		hzexit(_fn, m_Groupname, E_DUPLICATE, m_Groupname, "Duplicate entry attempted: Item %s alredy exists", *refname) ;
	//if (!m_ItemsByName.Count() && pParent)	hzexit(_fn, m_Groupname, E_CORRUPT, m_Groupname, "Adding first item %s but with non-null parent", *refname) ;
	if (m_ItemsByName.Exists(refname))		return hzerr(_fn, HZ_WARNING, E_DUPLICATE, "Duplicate entry attempted: Item %s alredy exists", *refname) ;
	if (!m_ItemsByName.Count() && pParent)	return hzerr(_fn, HZ_WARNING, E_CORRUPT, "Adding first item %s but with non-null parent", *refname) ;

	//	ADD the item
	pItem->m_Refname = refname ;
	pItem->m_Title = headline ;

	if (bSlct)
		pItem->m_bFlags |= HZ_TREEITEM_OPEN ;
	pItem->m_bFlags |= HZ_TREEITEM_LINK ;

	m_ItemsByName.Insert(pItem->m_Refname, pItem->m_Uid) ;
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

	m_ItemsByParent.Insert(pItem->m_ParId, pItem->m_Uid) ;

	return E_OK ;
}

hzEcode	hzTree::Sync	(const hdbObject& obj)
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

	_hzfunc("hzTree::Sync") ;

	hzArray<uint32_t>	list ;		//	List of sub-class objects

	//const hdbClass*	pClass ;		//	Object class (expected to be complex)
	hzTreeitem*		pArt ;			//	Article pointer
	hzTreeitem*		pNew ;			//	Article pointer
	//hdsTreeitem*		pArt ;			//	Article pointer
	//hdsTreeitem*		pNew ;			//	Article pointer
	hzString		S ;				//	Temp string
	uint32_t		x ;				//	Class counter
	uint32_t		y ;				//	Object counter
	uint32_t		itmId ;			//	ID of item
	char			buf [12] ;		//	Number buffer
	hzEcode			rc = E_OK ;		//	Return code

	//	Get host class and use it to look up class articles
	//pClass = obj.Class() ;

	//	Get known sub-classes and use these to look up more class articles. If multiple instances ...
	for (x = 0 ; x < m_ItemsByName.Count() ; x++)
	{
		itmId = m_ItemsByName.GetObj(x) ;
		pArt = m_arrItems[itmId] ;
		if (!(pArt->m_bFlags & HZ_TREEITEM_FORM))
			continue ;

		//	Obtain the list of sub-class objects
		/* fix
		obj.ListSubs(list, pArt->m_nCtx) ;
		*/

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
				pNew = new hzTreeitem() ;
				pNew->m_Refname = S ;
				pNew->m_Title = pArt->m_Title ;
				pNew->m_ParId = pArt->m_Uid ;
				pNew->m_nLevel = pArt->m_nLevel + 1 ;
				pNew->m_bFlags |= HZ_TREEITEM_LINK ;
				pNew->m_Uid = m_ItemsByName.Count() ;
				m_ItemsByName.Insert(pNew->m_Refname, pNew->m_Uid) ;
				m_ItemsByParent.Insert(pNew->m_ParId, pNew->m_Uid) ;
			}
		}
	}

	threadLog("%s. DONE SYNC\n", *_fn) ;
	return rc ;
}

hzEcode	hzTree::Clear	(void)
{
	_hzfunc("hzTree::Clear") ;

	hzTreeitem*	pItem ;		//	Current item
	uint32_t		n ;			//	Items iterator

	for (n = 0 ; n < m_arrItems.Count() ; n++)
	{
		pItem = m_arrItems[n] ;
		delete pItem ;
	}

	m_ItemsByParent.Clear() ;
	m_ItemsByName.Clear() ;
}

hzEcode	hzTree::_procArticle	(hzChain& Z, hzTreeitem* parent)
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

	_hzfunc("hzTree::_procArticle") ;

	hzTreeitem*	pItem ;		//	Next tree item

	hzString	parnam ;		//	Refname of parent if applicable
	uint32_t	parId ;			//	ID of item parent
	uint32_t	itmId ;			//	ID of item
	int32_t		nLo ;			//	Lowest position in m_AllItemsByParent
	int32_t		nHi ;			//	Higest position in m_AllItemsByParent
	int32_t		n ;				//	Item counter

	parId = parent ? parent->m_Uid : 0 ;

	nLo = m_ItemsByParent.First(parId) ;
	if (nLo < 0)
	{
		//	Dead end
		return E_OK ;
	}
	nHi = m_ItemsByParent.Last(parId) ;
	threadLog(" ... items %d to %d\n", nLo, nHi) ;

	if (parent)
		parnam = parent->m_Refname ;

	//	Print:	Item is a folder. In the first instance (the only time the level is 1) for channel J we don't want to start with a comma, otherwise we do
	for (n = nLo ; n <= nHi ; n++)
	{
		itmId = m_ItemsByParent.GetObj(n) ;
		pItem = m_arrItems[itmId] ;

		threadLog("\t[%s -> %s %s]\n", *parnam, *pItem->m_Refname, *pItem->m_Title) ;

		if (!pItem->m_Content)
		{
			if (pItem->m_bFlags & HZ_TREEITEM_LINK)
				Z.Printf("\n<xtreeItem parent=\"%s\" id=\"%s\" hdln=\"%s\"/>\n", *parnam, *pItem->m_Refname, *pItem->m_Title) ;
			else
				Z.Printf("\n<xtreeHead parent=\"%s\" id=\"%s\" hdln=\"%s\"/>\n", *parnam, *pItem->m_Refname, *pItem->m_Title) ;
		}
		else
		{
			if (pItem->m_bFlags & HZ_TREEITEM_LINK)
			{
				Z.Printf("\n<xtreeItem parent=\"%s\" id=\"%s\" hdln=\"%s\">\n", *parnam, *pItem->m_Refname, *pItem->m_Title) ;
				Z << pItem->m_Content ;
				Z << "\n</xtreeItem>\n" ;
			}
			else
			{
				Z.Printf("\n<xtreeHead parent=\"%s\" id=\"%s\" hdln=\"%s\">\n", *parnam, *pItem->m_Refname, *pItem->m_Title) ;
				Z << pItem->m_Content ;
				Z << "\n</xtreeHead>\n" ;
			}
		}

		_procArticle(Z, pItem) ;
	}

	return E_OK ;
}

hzEcode	hzTree::_procTreeitem	(hzChain& J, hzTreeitem* parent)
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

	_hzfunc("hzTree::_procTreeitem") ;

	hzTreeitem*	pItem ;			//	Next tree item

	hzString	parnam ;		//	Refname of parent if applicable
	int32_t		nLo ;			//	Lowest position in m_AllItemsByParent
	int32_t		nHi ;			//	Higest position in m_AllItemsByParent
	int32_t		n ;				//	Item counter
	int32_t		i ;				//	Indent counter
	int32_t		nChild ;		//	True if link
	uint32_t	parId ;			//	ID of item parent
	uint32_t	itmId ;			//	ID of item

	parId = parent ? parent->m_Uid : 0 ;

	nLo = m_ItemsByParent.First(parId) ;
	if (nLo < 0)
		return E_OK ;
	nHi = m_ItemsByParent.Last(parId) ;

	if (parent)
		parnam = parent->m_Refname ;

	if (J.Size() > 100000)
		Fatal("tree too big\n") ;

	for (n = nLo ; n <= nHi ; n++)
	{
		itmId = m_ItemsByParent.GetObj(n) ;
		pItem = m_arrItems[itmId] ;

		nChild = m_ItemsByParent.First(pItem->m_Uid) >= 0 ? 1 : 0 ;

		if (pItem->m_nLevel || n > nLo)
			J.AddByte(CHAR_COMMA) ;
		J.AddByte(CHAR_NL) ;
		for (i = pItem->m_nLevel ; i ; i--)
			J.AddByte(CHAR_TAB) ;

		if (pItem->m_bFlags & HZ_TREEITEM_LINK)
			J.Printf("[%d,%d,\"%s\",\"%s\"]", pItem->m_nLevel, nChild, *pItem->m_Title, *pItem->m_Refname) ;
		else
			J.Printf("[%d,%d,\"%s\",\"\"]", pItem->m_nLevel, nChild, *pItem->m_Title) ;

		if (nChild)
			_procTreeitem(J, pItem) ;
	}

	return E_OK ;
}

hzEcode	hzTree::ExportArticleSet	(hzChain& Z)
{
	//	Purpose:	Export tree as an article set
	//
	//	Arguments:	1)	J	The chain in which tree is being built
	//
	//	Returns:	E_NODATA	If the tree is empty
	//				E_OK		If the tree is exported to the supplied chain

	_hzfunc("hzTree::ExportArtcleSet") ;

	if (!m_ItemsByName.Count())
		return E_NODATA ;

	_procArticle(Z, 0) ;

	return E_OK ;
}

hzEcode	hzTree::ExportDataScript	(hzChain& J)
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

	_hzfunc("hzTree::ExportDataScript") ;

	if (!m_ItemsByName.Count())
		return E_NODATA ;

	J << "var ntX=new Array();\nntX=[" ;

	_procTreeitem(J, 0) ;

	J << "\n\t];\n" ;

	return E_OK ;
}
#endif
