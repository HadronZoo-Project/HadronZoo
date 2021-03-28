//
//	File:	hzMCH.h
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

#ifndef hzMCH_h
#define hzMCH_h

#include "hzChain.h"

class	hzMCH
{
	//	Category:	Data
	//
	//	The hzMCH or 'micro chain' class provides an efficient means of storing variable length entities. It is implemented as a series of blocks rather than as
	//	contiguous memory. In this respect it is similar to the hzChain class, however that is about where the similarity ends. hzChain is aimed at facilitating
	//	construction and processing of long string like entities, such as HTML output. The hzMCH class is aimed only at storage and retrieval.
	//
	//	hzMCH instances may only be read from, written to and cleared. No other methods are defined and the copy constructor and operator= function are declared
	//	private and are undefined, thereby preventing copying. hzMCH comprises a single unsigned 32-bit number which is an encoded block address. The blocks are
	//	managed by a regime that is global to the application. There are four possible block types known as A, B, C and D. Type A blocks comprise a 68 byte data
	//	area but are 72 bytes in size as they include the 32-bit address of the next block. Types B, C and D are terminator only blocks, given entirely to data
	//	with respective sizes of 48, 32 and 16 bytes. An hzMCP value is considered to be a series of zero or more type A blocks plus a remainder which may be of
	//	type A, B, C or D depending on its size.
	//
	//	The global block management regime manages four sets of blocks, one for each type. The top 2 bits of the 32-bit addresses indicate the block type, while
	//	the remaining 30 is the address of the block within its set. The total data capacity of the system is difficult to precisely define. Each set of blocks
	//	has a total address range of 1 Gig so in theory it would be possible to store  164 Gb (68 + 48 + 32 + 16). In practice it is better to think in terms of
	//	the capacity offered by the set of type A blocks or 68GB. That way there will be fewer surprises.
	//
	//
	//	left without implementation. hzMCH instances cannot be copied. write operation will change the address if the size of the entity necessitates this. 

	uint32_t	m_Addr ;		//	Block size and first block address.

	void*	_xlate	(uint32_t addr) ;

	//	Prevent copies
	hzMCH	(const hzMCH&) ;
	hzMCH&	operator=	(const hzMCH&) ;

public:
	//	Constructors/Destructors
	hzMCH	(void) ;
	~hzMCH	(void) ;

	//	Set chain contents
	void	Read	(hzChain& Z) ;
	void	Clear	(void) ;
	void	Write	(const hzChain& Z) ;
} ;

#endif	//	hzMCH_h
