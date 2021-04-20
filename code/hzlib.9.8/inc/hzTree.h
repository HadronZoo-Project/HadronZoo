//
//	File:	hzTree.h
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

#ifndef hzTree_h
#define hzTree_h

#include <sys/stat.h>

#include "hzChars.h"
#include "hzChain.h"
#include "hzCodec.h"
#include "hzDate.h"
#include "hzStrRepos.h"
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

//	Class Group:	Dissemino Trees and Tree items
//
//	The hzTree and hzTreeitem classes are the most direct manifestation of the HadronZoo standard approach to hierarchical or tree-like constructs. All items in the tree are mapped
//	in two ways, 1:1 by unique name and 1:many by parent. This approach avoids the need for items to have parent and sibling pointers and lists of child items (or pointers to child
//	items). Items are required only to name the parent (A 4-byte string instead of a 8-byte pointer). All other relationships are implied in the mappings.
//
//	The hzTreeitem class only provides what is needed for an instance to take its place in a hzTree. Items in a real application are expected to be of a class defined as hzTreeitem
//	derivatives. Because of this, methods that add items to the tree cannot allocate the item and instead must have it supplied as an argument.
//
//	Trees are intended to hold standalone hierarchical entities, such as articles in a complex document. hzTree is used in Dissemino to effect navigation trees and because of this
//	use, has an extra Export function for creating navtree JavaScript. 
//

//#define	HZ_TREEITEM_LINK	0x01	//	Use the refname as the URL
//#define	HZ_TREEITEM_FORM	0x02	//	Article is a heading with a blank form for a sub-class
//#define	HZ_TREEITEM_OPEN	0x04	//	Tree item is expanded in the navtree

template<class KEY, class OBJ>	class	hzTree
{
	//	Category:	Dissemino HTML Generation
	//
	//	Navigation or other tree

	hzMapM<uint32_t,KEY>	m_ItemsByParent ;	//	All items by parent item ID
	hzMapS<KEY,uint32_t>	m_ItemsByName ;		//	Key to item ids

	hzArray<OBJ>	m_arrItems ;				//	The actual items

	/*
	hzMapM	<uint32_t,uint32_t>	m_ItemsByParent ;	//	All items by parent item ID
	hzMapS	<KEY,uint32_t>		m_ItemsByName ;		//	Key to item ids

	hzArray<OBJ>	m_arrItems ;					//	The actual items
	*/

	hzEcode	_procTreeitem	(hzChain& J, uint32_t parId, uint32_t nLevel)
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

		OBJ			obj ;			//	Object to hand
		KEY			key ;			//	Key of object to hand
		int32_t		nLo ;			//	Lowest position in m_AllItemsByParent
		int32_t		nHi ;			//	Higest position in m_AllItemsByParent
		int32_t		n ;				//	Item counter
		int32_t		i ;				//	Indent counter
		int32_t		nChild ;		//	True if link
		uint32_t	objId ;			//	ID of object to grab

		threadLog("%s called\n", *_fn) ;
		nLo = m_ItemsByParent.First(parId) ;
		if (nLo < 0)
			return E_OK ;
		nHi = m_ItemsByParent.Last(parId) ;

		for (n = nLo ; n <= nHi ; n++)
		{
			key = m_ItemsByParent.GetObj(n) ;
			objId = m_ItemsByName[key] ;

			nChild = m_ItemsByParent.First(objId & 0x7fff) >= 0 ? 1 : 0 ;

			if (nLevel || n > nLo)
				J.AddByte(CHAR_COMMA) ;
			J.AddByte(CHAR_NL) ;
			for (i = nLevel ; i ; i--)
				J.AddByte(CHAR_TAB) ;

			obj = m_arrItems[objId] ;

			if (objId & 0x8000)
				J.Printf("[%d,%d,\"%s\",\"\"]", nLevel, nChild, (const char*) key) ;
			else
				J.Printf("[%d,%d,\"%s\",\"%u\"]", nLevel, nChild, (const char*) key, objId) ;

			if (nChild)
				_procTreeitem(J, objId, nLevel + 1) ;
		}

		return E_OK ;
	}

	//	Prevent copies
	hzTree	(const hzTree&) ;
	hzTree&	operator=	(const hzTree&) ;

public:
	hzTree	(void)	{ threadLog("hzTree::hzTree\n"); }
	~hzTree	(void)	{ Clear() ; }

	hzEcode	Clear	(void)
	{
		_hzfunc("hzTree::Clear") ;

		threadLog("%s called\n", *_fn) ;
		m_ItemsByParent.Clear() ;
		m_ItemsByName.Clear() ;
	}

	hzEcode	AddHead	(const KEY& parKey, const KEY& refKey, const OBJ& obj)
	{
		//	Add a heading to the tree

		_hzfunc("hzTree::AddHead") ;

		uint32_t	parId ;		//	ID of parent object
		uint32_t	newId ;		//	ID of new object

		threadLog("%s called\n", *_fn) ;
		if (m_ItemsByName.Exists(refKey))
			return E_DUPLICATE ;

		parId = 0 ;
		newId = m_arrItems.Count() ;

		m_arrItems.Add(obj) ;

		if (parKey)
		{
			if (!m_ItemsByName.Exists(parKey))
				return E_NOTFOUND ;
			parId = m_ItemsByName[parKey] ;
		}

		threadLog("%s. Par %d Item id %d\n", *_fn, parId, newId) ;
		m_ItemsByParent.Insert(parId, refKey) ;
		m_ItemsByName.Insert(refKey, newId) ;
		return E_OK ;
	}

	hzEcode		AddItem	(const KEY& parKey, const KEY& refKey, const OBJ& obj)
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

		uint32_t	parId ;		//	ID of parent object
		uint32_t	newId ;		//	ID of new object

		threadLog("%s called\n", *_fn) ;
		if (m_ItemsByName.Exists(refKey))
			return E_DUPLICATE ;

		parId = 0 ;
		newId = m_arrItems.Count() ;

		m_arrItems.Add(obj) ;

		if (parKey)
		{
			if (!m_ItemsByName.Exists(parKey))
				return E_NOTFOUND ;
			parId = m_ItemsByName[parKey] ;
		}

		threadLog("%s. Par %d Item id %d\n", *_fn, parId, newId) ;
		m_ItemsByParent.Insert(parId, refKey) ;
		m_ItemsByName.Insert(refKey, newId) ;

		return E_OK ;
	}

	hzEcode	ExportDataScript	(hzChain& J)
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

		threadLog("%s case 1\n", *_fn) ;
		if (!m_ItemsByName.Count())
			return E_NODATA ;
		threadLog("%s case 2\n", *_fn) ;

		J << "var ntX=new Array();\nntX=[" ;
		threadLog("%s case 3\n", *_fn) ;

		_procTreeitem(J, 0, 0) ;
		threadLog("%s case 4\n", *_fn) ;

		J << "\n\t];\n" ;
		threadLog("%s case 5\n", *_fn) ;
		return E_OK ;
	}

	const OBJ&	GetItem	(const KEY& key)
	{
		_hzfunc("hzTree::GetItem") ;

		uint32_t	id ;

		threadLog("%s called\n", *_fn) ;
		id = m_ItemsByName[key] ;
		return m_arrItems[id] ;
	}

	uint32_t	Count	(void) const	{ return m_ItemsByName.Count() ; }
} ;

#endif	//	hzTree_h
