//
//	File:	hdbRepos.cpp
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
//	Implimentation of the HadronZoo Proprietary Database Suite
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
#include "hzCodec.h"
#include "hzDocument.h"
#include "hzDirectory.h"
#include "hzDatabase.h"
#include "hzDelta.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Variables
*/

//	global	hzMapS	<hzString,hdbObjRepos*>	_hzGlobal_Repositories ;	//	All data repositories
//	global	hzMapS	<hzString,hdbBinRepos*>	_hzGlobal_BinDataCrons ;	//	All hdbBinCron instances
//	global	hzMapS	<hzString,hdbBinRepos*>	_hzGlobal_BinDataStores ;	//	All hdbBinStore instances

/*
**	Functions
*/

hzEcode	hdbObjRepos::LoadCSV	(const hdbClass* pCsvClass, const char* filepath, const char* delim, bool bQuote)
{
	//	Import data from a CSV file into the repository. Note that the repository and CSV file do not have to have to be of the same data class but for any data
	//	to be imported, they must have at least one member in common. It is only members in common that are populated by this action.
	//
	//	Note that where the data class of the repository and CSV file differ, member commonality is based only on member name and member type, not on minimum or
	//	maximum populations. All data class members for the CSV will have a minimum of 0 and the maximum of 1 by definition. The repository data class members
	//	may have minimums of either 0 or 1 and a maximum of 1 or many.
	//
	//	Because the CSV class members always have a maximum population of 1 value, there is no need to create an object of the CSV class and instead an array of
	//	hzAtoms is used to read in each line. One of these atoms will correspond to the 
	//
	//	Arguments:	1)	pCsvClass	The data class of the CSV. Null indicates the CSV of the same class as the repository.
	//				2)	filepath	Full pathname of CSV file to be imported.
	//				3)	delim		The dilimiter sequence. Null indicates the comma is to be used.
	//				4)	bQuote		Indicates if the datum are expected to be enclosed in quotes.

	_hzfunc("hdbObjCache::LoadData") ;

	ifstream	is ;			//	Input stream
	FSTAT		fs ;			//	File status
	hzChain		Z ;				//	For line storage
	hzChain		W ;				//	For datum storage
	chIter		zi ;			//	For line processing
	hdbObject	objH ;			//	Object for host class
	hdbObject	objC ;			//	Object for CSV class
	hzAtom		atom ;			//	Datum
	hzString	blank ;			//	Blank string for hdbObject::Init()
	uint32_t	nSofar = 0 ;	//	Bytes read so far
	uint32_t	mbrNo ;			//	Member iteration
	uint32_t	dLen ;			//	Length of delimiter (minus 1)
	uint32_t	nLine ;			//	Line number
	uint32_t	objId ;			//	Object id
	hzEcode		rc = E_OK ;		//	Return code
	char		buf [504] ;		//	Getline buffer
	char		dbuf [4] ;		//	Delim buffer

	//	Check arguments
	if (!filepath || !filepath[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No filepath supplied") ;

	if (lstat(filepath, &fs) < 0)
		return hzerr(_fn, HZ_ERROR, E_NOTFOUND, "File %s does not exist", filepath) ;

	is.open(filepath) ;
	if (is.fail())
		return hzerr(_fn, HZ_ERROR, E_OPENFAIL, "Cannot open %s", filepath) ;

	if (!delim || !delim[0])
		{ dbuf[0] = CHAR_COMMA ; dbuf[1] = 0 ; delim = dbuf ; dLen = 0 ; }
	else
		dLen = strlen(delim) - 1 ;

	//	Init objects
	rc = objH.Init(blank, m_pClass) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not init host class object") ;

	if (pCsvClass)
	{
		rc = objC.Init(blank, pCsvClass) ;
		if (rc != E_OK)
			return hzerr(_fn, HZ_ERROR, rc, "Could not init CSV class object") ;
	}

	//	Fetch objects (lines) from CSV file
	for (nLine = 1 ; nSofar < fs.st_size ; nLine++)
	{
		do
		{
			is.getline(buf, 500) ;
			buf[is.gcount()] = 0 ;

			if (buf[0])
				Z << buf ;
			nSofar += is.gcount() ;
		}
		while (is.gcount() == 500) ;

		if (!Z.Size())
			continue ;

		//	Process it
		mbrNo = 0 ;
		for (zi = Z ; !zi.eof() ; zi++)
		{
			if (zi == delim)
			{
				atom = W ;

				if (!pCsvClass)
					objH.SetValue(mbrNo, atom) ;
				else
					objC.SetValue(mbrNo, atom) ;
				mbrNo++ ;
				W.Clear() ;
				zi += dLen ;
				continue ;
			}

			W.AddByte(*zi) ;
		}
		Z.Clear() ;

		//	Insert values
		//Insert(objId, &objH) ;
		Insert(objId, objH) ;
	}

	return rc ;
}
