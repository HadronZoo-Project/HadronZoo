//
//	File:	hzCodec.h
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

#ifndef hzCodec_h
#define hzCodec_h

#include "hzChain.h"

enum	hzCoding
{
	//	Category:	Codec
	//
	//	List of supported encodings

	ENCODING_UNDEFINED,		//	No coding specified
	ENCODING_UTF8,			//	Standard unicode
	ENCODING_WIN1252,		//	The windows-1252 char set (char/byte ratio of 1, assigned meanings to high end ASCII)
	ENCODING_QP,			//	Quoted-printable encoding
	ENCODING_INVALID		//	Invalid encoding
} ;

class	hzMD5
{
	//	Category:	Codec
	//
	//	An MD5 digest is a 128-bit hash value, commonly used as a checksum in such applications as messaging and file transmission. The hzMD5 class is simply a
	//	wrapper to limit the operations that may be performed.

	uchar	m_md5val[16] ;

public:
	hzMD5	(void) ;

	//	Set functions
	hzEcode	CalcMD5File	(const char* filepath) ;
	hzEcode	CalcMD5		(const hzChain& Z) ;
	hzEcode	CalcMD5		(const char* cstr) ;

	//	Get functions
	const uchar*	Value	(void) const	{ return m_md5val ; }
	hzString		Str		(void) const ;

	//	Operators
	bool	operator==	(const hzMD5& op) const ;
	bool	operator!=	(const hzMD5& op) const ;
} ;

/*
**	Section 1:	Base-64 encoding/decoding
*/

hzString	Base64Encode	(const char*s) ;
void		Base64Encode	(hzChain& Encoded, hzChain& Raw) ;
hzEcode		Base64Encode	(hzChain& Encoded, const char* cpDir, const char* cpFilename) ;

hzString	Base64Decode	(const char*s) ;
hzEcode		Base64Decode	(hzChain& Decoded, hzChain::Iter& raw) ;
hzEcode		Base64Decode	(hzChain& Decoded, hzChain& Raw) ;

/*
**	Section 1:	Quoted-printable encoding/decoding
*/

hzEcode		QPDecode		(hzChain& Decoded, hzChain& Raw) ;

/*
**	Section 3:	Zipping & un-zipping (using system call to gzip/gunzip)
*/

hzEcode		Gzip	(hzChain& compressed, const hzChain& orig) ;
hzEcode		Gunzip	(hzChain& orig, const hzChain& compressed) ;
hzEcode		Punzip	(hzChain& orig, const hzChain& compressed) ;

#endif	//	hzCodec_h
