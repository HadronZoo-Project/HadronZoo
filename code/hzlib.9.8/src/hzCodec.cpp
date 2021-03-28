//
//	File:	hzCodec.cpp
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

#include <iostream>
#include <fstream>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include <stdio.h>
#include <crypt.h>
#include <zlib.h>
#include <sys/stat.h>

#include "hzBasedefs.h"
#include "hzChars.h"
#include "hzErrcode.h"
#include "hzDirectory.h"
#include "hzCodec.h"
#include "hzProcess.h"

using namespace std ;

/*
**	SECTION 1:	BASE 64 CODEC
*/

hzString	Base64Encode	(const char* pAscii) ;
hzString	Base64Decode	(const char* pBase64) ;

/*
**	Support macro definitions for Base-64 CODEC:
*/

#define b64is7bit(c)	((c) > 0x7f ? 0 : 1)	// Valid 7-Bit ASCII character?
#define b64blocks(l)	(((l) + 2) / 3 * 4 + 1)	// Length rounded to 4 byte block.
#define b64octets(l)	((l) / 4  * 3 + 1)		// Length rounded to 3 byte octet. 

#define _Base64Size(n)	((((n+2)/3)*4)+4)
#define AsciiSize(n)	(((n/4)*3)+1)
#define Is7Bit(ch)		(ch & 0x80 ? 0:1)
#define IsBase64Ch(ch)	()

#define	_get6bit(z)		(z>='A'&&z<='Z'? z-'A' : z>='a'&&z<='z'? z-'a'+26 : z>='0'&&z<='9'? z-'0'+52 : z=='+'? 62 : z=='/'? 63 : z=='='? 100 : -1)
#define	_get1hex(z)		(z>='A' && z<='F' ? z-'A'+10 : z>='a' && z<='f' ? z-'a'+10 : z>='0' && z<='9' ? z-'0' : -1)

// Note:	Tables are in hex to support different collating sequences
static	const char*	s_base64_array = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" ;
			//	Array of allowed base-64 characters

static	const uchar  pIndex	[] =
{
	//	Collation sequence
	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
	0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
	0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
	0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
	0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f
} ;

static	const uchar	pBase64	[] =
{
	//	Collation sequence
	0x3e, 0x7f, 0x7f, 0x7f, 0x3f, 0x34, 0x35, 0x36,
	0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x7f,
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x01,
	0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
	0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x1a, 0x1b,
	0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
	0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
} ;

bool	_b64valid	(uchar* c)
{
	//	Category:	Codec
	//
	// Tests for a valid Base64 char.
	//
	// 	Arguments:	1)	c	The test char
	//
	// 	Returns:	True	If the char belongs in the Base64 character set
	// 				False	Otherwise

	if ((*c < 0x2b) || (*c > 0x7a))
		return false ;
	
	if ((*c = pBase64[*c - 0x2b]) == 0x7f)
		return false ;

	return true ;
}

/*
**	Base-64 functions
*/

hzString	Base64Encode	(const char* s)
{
	//	Category:	Codec
	//
	//	Encode a 7-Bit ASCII string to a Base64 string.
	//
	//	Arguments:	1)	s	The 7-Bit ASCII string to encode
	//
	//	Returns:	Instance of hzString by value being the encoded string

	_hzfunc("Base64Encode(cstr)") ;

	char*		b ;			//	Base64 buffer
	hzString	S ;			//	String to populate and return
	int32_t		nBytes ;	//	Size of string to encode
	int32_t		x = 0 ;		//	Input iterator

	nBytes = strlen(s) ;

	while (x < nBytes)
	{
		if (!b64is7bit((uchar) *(s + x)))
		{
			hzerr(_fn, HZ_ERROR, E_FORMAT, "String [%s] not 7-bit ASCII", s) ;
			return S ;
		}
		x++ ;
	}

	b = new char[_Base64Size(nBytes)] ;
	if (!b)
		return S ;

	memset(b, 0x3d, _Base64Size(nBytes) - 1) ;

	x = 0 ;

	while (x < (nBytes - (nBytes % 3)))
	{
		*b++ = pIndex[s[x] >> 2];
		*b++ = pIndex[((s[x] & 0x03) << 4) + (s[x + 1] >> 4)];
		*b++ = pIndex[((s[x + 1] & 0x0f) << 2) + (s[x + 2] >> 6)];
		*b++ = pIndex[s[x + 2] & 0x3f] ;
		x += 3 ;
	}

	if (nBytes - x)
	{
		*b++ = pIndex[s[x] >> 2] ;

		if (nBytes - x == 1)
			*b = pIndex[ (s[x] & 0x03) << 4];
		else							
		{
			*b++ = pIndex[((s[x] & 0x03) << 4) + (s[x + 1] >> 4)];
			*b = pIndex[ (s[x + 1] & 0x0f) << 2];
		}
	}

	S = b ;
	delete [] b ;
	return S ;
}

void	Base64Encode	(hzChain& Encoded, hzChain& Raw)
{
	//	Category:	Codec
	//
	//	Encode 7-Bit ASCII in the input chain to Base64 in the output chain.
	//
	//	Arguments:	1)	Encoded	The chain to contain the encoded data
	//				2)	Raw		The chain containing the 7-Bit ASCII data to encode
	//
	//	Returns:	None

	_hzfunc("Base64Encode(hzChain)") ;

	chIter		r ;				//	Raw input chain iterator
	int32_t		A ;				//	A is top 6 bits of byte 1 of 3 shifted down by 2
	int32_t		B ;				//	B is low 2 bits of byte 2 of 3 shifted up by 4 and top 4 bits of 'b' shifted down by 4
	int32_t		C ;				//	C is low 4 bits of byte 3 of 3 shifted up by 2 and top 2 bits of 'c' shifted down by 6
	int32_t		D ;				//	D is low 6 bits of byte 3
	int32_t		nSeg = 0 ;		//	Newline marker
	uchar		a ;				//	1st byte of 3-byte segment
	uchar		b ;				//	2nd byte of 3-byte segment
	uchar		c ;				//	3rd byte of 3-byte segment
	uchar		bits ;			//	Count of bits in segment (at end can be less than 3)
	char		buf	[8] ;		//	Export encoded set of 4 bytes

	for (r = Raw ; !r.eof() ; r++)
	{
		//	Fill the 3 bytes of source

		a = b = c = bits = 0 ;
		a = *r ;
		r++ ;
		bits++ ;
		if (!r.eof())
		{
			b = *r ;
			r++ ;
			bits++ ;
			if (!r.eof())
			{
				c = *r ;
				bits++ ;
			}
		}

		//	Then convert to 4 bytes.
		A  = B = C = D = 0 ;
		A  = ((a & 0xfc) >> 2) ;
		B  = ((a & 0x03) << 4) ;
		B |= ((b & 0xf0) >> 4) ;
		C  = ((b & 0x0f) << 2) ;
		C |= ((c & 0xc0) >> 6) ;
		D  = c & 0x3f ;

		buf[0] = bits ? s_base64_array[A] : '=' ;
		buf[1] = bits ? s_base64_array[B] : '=' ;
		buf[2] = bits > 1 ? s_base64_array[C] : '=' ;
		buf[3] = bits > 2 ? s_base64_array[D] : '=' ;
		buf[4] = 0 ;

		Encoded += buf ;

		nSeg++ ;
		if (nSeg == 19)
		{
			Encoded += "\r\n" ;
			nSeg = 0 ;
		}
	}
}

hzEcode	Base64Encode	(hzChain& Encoded, const char* dir, const char* fname)
{
	//	Category:	Codec
	//
	//	Encode the contents of the file with the supplied path, from 7-Bit ASCII to Base64. The encoded data is then in a hzChain instance.
	//
	//	Arguments:	1)	Encoded	The chain to contain the encoded data
	//				2)	dir		The directory of the file containing the 7-Bit ASCII data to encode
	//				3)	fname	The file name within the directory
	//
	//	Returns:	E_ARGUMENT	If either the directory or filename are not supplied
    //              E_NOTFOUND  If the filepath specified does not exist as a directory entry of any kind
    //              E_TYPE      If the filepath names a directory entry that is not a file
    //              E_NODATA    If the filepath is a file but empty
    //              E_OPENFAIL  If the file specified could not be opened for reading
    //              E_OK        If the input stream was opened and the content encoded

	_hzfunc("Base64Encode(file)") ;

	std::ifstream	is ;	//	Input file stream

	hzChain		Z ;			//	To store unencoded input file
	hzString	Path ;		//	Full pathname of file
	hzEcode		rc ;		//	Return code

	if (!dir || !dir[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No directory supplied") ;

	if (!fname || !fname[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No filename supplied") ;

	Path = dir ;
	Path += "/" ;
	Path += fname ;

	rc = OpenInputStrm(is, *Path, *_fn) ;

	if (rc == E_OK)
	{
		Z << is ;
		is.close() ;
		Base64Encode(Encoded, Z) ;
	}

	return rc ;
}

hzString	Base64Decode	(const char* s)
{
	//	Category:	Codec
	//
	//	Decode a Base64 string to a 7-Bit ASCII string
	//
	//	Arguments:	1)	s	The Base64 data to decode (as char string)
	//
	//	Returns:	Instance of hzString by value being the decoded data

	_hzfunc("Base64Decode(cstr)") ;

	static const char	pPad[] =
	{
		//	Padding sequence
		0x3d, 0x3d, 0x3d, 0x00
	} ;

	char*		b ;				//	Pointer into buffer
	hzString	S ;				//	String to populate and return
	int32_t		nBytes ;		//	Length of input string
	int32_t		x = 0 ;			//	Switchback counter (every 4 bytes)
	int32_t		posn = 0 ; 		//	Buffer position
	uchar		c = 0 ;			//	Current charcter

	//	Not valid unless the string is an exact multiple of 4 bytes
	nBytes = strlen(s) ;

	if (nBytes % 4)
		return S ;

	if  (b = (char*) strchr(s, pPad[0]))
	{
		if  ((b - s) < (nBytes - 3))
			return S ;
		if  (strncmp(b, (char*) pPad + 3 - (s + nBytes - b), s + nBytes - b))
			return S ;
	}

	//	Allocate temporary memory
	b = new char[AsciiSize(nBytes)] ;
	memset(b, 0, AsciiSize(nBytes)) ;
	if (!b)
	{
		hzerr(_fn, HZ_ERROR, E_MEMORY, "Could not allocate %d bytes", nBytes) ;
		return S ;
	}

	while ((c = *s++))
	{
		if  (c == pPad[0])
			break;

		if (!_b64valid(&c))
			return 0 ;
		
		switch	(x % 4)
		{
		case 0:		b[posn] = c << 2;
					break;						  

		case 1:		b[posn] |= c >> 4 ;

					if (!b64is7bit((uchar) b[posn++]))
					{
						delete [] b ;
						return S ;
					}

					b[posn] = (c & 0x0f) << 4 ;
					break;

		case 2:		b[posn] |= c >> 2 ;

					if (!b64is7bit((uchar) b[posn++]))
					{
						delete [] b ;
						return S ;
					}

					b[posn] = (c & 0x03) << 6 ;
					break;

		case 3:		b[posn] |= c ;

					if (!b64is7bit((uchar) b[posn++]))
					{
						delete [] b ;
						return S ;
					}
		}
		x++ ;
	}

	S = b ;
	delete [] b ;
	return S ;
}

hzEcode	Base64Decode	(hzChain& Decoded, hzChain::Iter& ci)
{
	//	Category:	Codec
	//
	//	Decode base64 encoded source from the supplied chain iterator to the end of the source.
	//
	//	This function is provided because bese64 encoded matter is often found partway into a chain (see email processing). By use of
	//	this function a separate step of copying the partial chain is avoided
	//
	//	Arguments:	1)	Decoded		Chain to contain result of decode operation
	//				2)	ci			Chain iterator into encoded data
	//
	//	Returns:	E_FORMAT	If the input chain contains unexpected sequences
	//				E_OK		If the input was decoded

	_hzfunc("Base64Decode(chIter)") ;

	chIter		zi ;		//	Iterator
	int32_t		val ;		//	Holds 24 bit value to be written as 3 bytes
	int32_t		a ;			//	1st value
	int32_t		b ;			//	2nd value
	int32_t		c ;			//	3rd value
	int32_t		d ;			//	4th value

	Decoded.Clear() ;

	//	Loop through collecting sets of 4 chars and writing out sets of 3 chars

	zi = ci ;
	for (; !zi.eof() ;)
	{
		a = _get6bit(*zi) ; zi++ ;
		b = _get6bit(*zi) ; zi++ ;
		c = _get6bit(*zi) ; zi++ ;
		d = _get6bit(*zi) ; zi++ ;

		if (a < 0 || b < 0 || c < 0 || d < 0)
			return E_FORMAT ;

		val = (a << 18) + (b << 12) ;

		if (c != 100)
			val += (c << 6) ;
		if (d != 100)
			val += d ;

		a = (val & 0xff0000) >> 16 ;
		b = (val & 0xff00) >> 8 ;
		c = val & 0xff ;

		Decoded.AddByte(a) ;
		if (b)
			Decoded.AddByte(b) ;
		if (c)
			Decoded.AddByte(c) ;

		if (*zi == CHAR_CR)	zi++ ;
		if (*zi == CHAR_NL)	zi++ ;
	}

	return E_OK ;
}

hzEcode	Base64Decode	(hzChain& Decoded, hzChain& Raw)
{
	//	Category:	Codec
	//
	//	Decode base64 encoded source
	//
	//	Arguments:	1)	Decoded		The decoded output
	//				2)	Raw			The encoded input
	//
	//	Returns:	E_FORMAT	If the input chain contains unexpected sequences
	//				E_OK		If the input was decoded

	_hzfunc("Base64Decode(hzChain)") ;

	hzChain::Iter	zi ;	//	Chain iterator (needed for call to BaseDecode(hzChain&,hzChain&)

	zi = Raw ;

	return Base64Decode(Decoded, zi) ;
}

/*
**	SECTION 2:	Quoted-Printable CODEC
*/

hzEcode	QPDecode	(hzChain& Decoded, hzChain& Raw)
{
	//	Category:	Codec
	//
	//	Convert a hzChain whose content is presumed to encoded using the Quoted-Printable method, back to it's original form.
	//
	//	Arguments:	1)	Decoded	The hzChain reference that will contain the result. Any pre-existing contents of this are cleared at the outset.
	//				2)	Raw		The encoded hzChain. This is not altered by this operation.
	//
	//	Returns:	E_NODATA	If the encoded chain is empty
	//				E_FORMAT	If there is an error in conversion
	//				E_OK		If the operation is successful

	_hzfunc("QPDecode") ;

	chIter	zi ;		//	Iterator
	int32_t	val ;		//	Holds 24 bit value to be written as 3 bytes

	Decoded.Clear() ;
	if (!Raw.Size())
		return E_NODATA ;

	//	Loop through collecting sets of 4 chars and writing out sets of 3 chars
	zi = Raw ;
	for (; !zi.eof() ;)
	{
		if (*zi == '=')
		{
			zi++ ;
			if (*zi == CHAR_CR || *zi == CHAR_NL)
			{
				//	Just skip to next line
				if (*zi == CHAR_CR)	zi++ ;
				if (*zi == CHAR_NL)	zi++ ;

				continue ;
			}

			if (zi.eof() || !IsHex(*zi))
				return E_FORMAT ;
			val = _get1hex(*zi) ;

			zi++ ;
			if (zi.eof() || !IsHex(*zi))
				return E_FORMAT ;
			val *= 16 ;
			val += _get1hex(*zi) ;

			Decoded.AddByte(val) ;
			zi++ ;
			continue ;
		}

		Decoded.AddByte(*zi) ;
		zi++ ;
	}

	return E_OK ;
}

/*
**	SECTION 3:	gzip encoding/decoding
*/
 
hzEcode	Gzip	(hzChain& gzipped, const hzChain& orig)
{
	//	Category:	Codec
	//
	//	Perform a gzip compression operation on the supplied chain.
	//
	//	Arguments:	1)	gzipped	The hzChain reference that will contain the result. Any pre-existing contents of this are cleared at the outset.
	//				2)	orig	The original hzChain. This is not altered by this operation.
	//
	//	Returns:	E_NODATA	If the original is empty
	//				E_NOINIT	If there is an error in conversion
	//				E_OK		If the operation is successful

	_hzfunc(__func__) ;

	chIter		zi ;		//	Chain iterator for input
	z_stream	defstream;	//	The zlib struct
	char*		tmp ;		//	A string to convert chain to contiguous memory
	char*		buf ;		//	Buffer to construct gzipped content
	int32_t		err ;		//	Return code from init

	gzipped.Clear() ;
	if (!orig.Size())
		return hzerr(_fn, HZ_WARNING, E_NODATA, "No input data provided") ;

	// Deflate original data
	defstream.zalloc	= Z_NULL;
	defstream.zfree		= Z_NULL;
	defstream.opaque	= Z_NULL;

	// setup tmp as the input and buf as the compressed output
	tmp = new char[orig.Size()+1] ;
	buf = new char[orig.Size()] ;

	//	for (n = 0, zi = orig ; !zi.eof() ; n++, zi++)
	//		tmp[n] = *zi ;
	//	tmp[n] = 0 ;
	zi = orig ;
	zi.Write(tmp, orig.Size()) ;
	tmp[orig.Size()] = 0 ;

	defstream.avail_in	= (uInt) orig.Size() ;
	defstream.next_in	= (Bytef*) tmp;
	defstream.avail_out	= (uInt) orig.Size() ;
	defstream.next_out	= (Bytef*) buf;

	//	Initialize the zlib deflation (i.e. compression) internals with deflateInit2(). 
	//	The parameters are as follows: 
	//
	//	z_streamp strm	- Pointer to a zstream struct 
	//	int level		- Compression level. Must be Z_DEFAULT_COMPRESSION, or between 0 and 9: 1 gives best speed, 9 gives best compression, 0 gives 
	//					no compression. 
	//	int method		- Compression method. Only method supported is "Z_DEFLATED". 
	//	int windowBits	- Base two logarithm of the maximum window size (the size of the history buffer). It should be in the range 8..15. Add  
	//					16 to windowBits to write a simple gzip header and trailer  around the compressed data instead of a zlib wrapper. The  
	//					gzip header will have no file name, no extra data, no comment, no modification time (set to zero), no header crc, and the  
	//					operating system will be set to 255 (unknown).  
	//
	//	int memLevel	- Amount of memory allocated for internal compression state. 
	//					1 uses minimum memory but is slow and reduces compression ratio; 9 uses maximum memory for optimal speed. Default value is 8. 
	//	int strategy	- Used to tune the compression algorithm. Use the value Z_DEFAULT_STRATEGY for normal data, Z_FILTERED for data produced by a
	//					filter (or predictor), or Z_HUFFMAN_ONLY to force Huffman encoding only (no string match)  

	err = deflateInit2(&defstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);  
	if (err != Z_OK)  
		return hzerr(_fn, HZ_ERROR, E_NOINIT, "Could not run deflate process") ;

	// the actual compression work.
	deflate(&defstream, Z_FINISH);
	deflateEnd(&defstream);

	//	for (n = 0 ; n < defstream.total_out ; n++)
	//		gzipped.AddByte(buf[n]) ;
	gzipped.Append(buf, defstream.total_out) ;
	delete tmp ;
	delete buf ;

	if (!gzipped.Size())
		return hzerr(_fn, HZ_ERROR, E_NODATA, "Input %d bytes but no output", orig.Size()) ;

	return E_OK ;
}

hzEcode	Gunzip	(hzChain& orig, const hzChain& gzipped)
{
	//	Category:	Codec
	//
	//	On the presumption that the supplied data is zipped by gzip, this functions writes the date to a temporary file, calls 'gunzip'
	//	via a system call on this file and reads in the unziped data. The temporary file and it's unziped associate are then unlinked.
	//
	//	Arguments:	1)	orig	The chain to be populated with the unzipped data
	//				2)	gzipped	The chain containing the original (zipped) data
	//
	//	Returns:	E_NODATA	If there is nothing to do
	//				E_WRITEFAIL	If the original data could not be written out to the temporary file
	//				E_SYSERROR	If the gunzip command could not be found or failed to process the data
	//				E_READFIAL	If the unzipped data could not be read back from the temporary file
	//				E_OK		If the operation was successful
	//
	//	Note:	Do not unzip files that contain multiple files and/or accross mutiple directories. It won't work and will leave a mess!

	_hzfunc(__func__) ;

	z_stream	infstream ;		//	Input
	chIter		zi ;			//	Chain iterator
	char*		tmp ;			//	Zipped buffer
	char*		buf ;			//	Unzipped buffer
	uint32_t	n ;				//	Buffer iterator

	tmp = new char[gzipped.Size()+1] ;
	buf = new char[20480] ;

	for (n = 0, zi = gzipped ; !zi.eof() ; n++, zi++)
		tmp[n] = *zi ;

	infstream.zalloc = Z_NULL;
	infstream.zfree = Z_NULL;
	infstream.opaque = Z_NULL;

	// setup "b" as the input and "c" as the compressed output
	infstream.avail_in = (uInt) gzipped.Size() ;	//((char*)defstream.next_out - b); // size of input
	infstream.next_in = (Bytef*)tmp ;				// input char array
	infstream.avail_out = 20480 ;					//	(uInt)sizeof(c); // size of output
	infstream.next_out = (Bytef*)buf ;				// output char array

	// the actual DE-compression work.
	if (inflateInit2(&infstream, 31) != Z_OK)
		return E_NOINIT ;

	inflate(&infstream, Z_NO_FLUSH);
	inflateEnd(&infstream);

	for (n = 0 ; n < infstream.total_out ; n++)
		orig.AddByte(buf[n]) ;
	delete tmp ;
	delete buf ;

	return E_OK ;
}

hzEcode	Punzip	(hzChain& orig, const hzChain& zipped)
{
	//	Category:	Codec
	//
	//	On the presumption that the supplied data is zipped by zip, this functions writes the date to a temporary file, calls 'unzip'
	//	via a system call on this file and reads in the unziped data. The temporary file and it's unziped associate are then unlinked.
	//
	//	Note that if the supplied data (arg 2) is zipped by the zip method it will have 'PK' as its first two bytes.
	//
	//	Arguments:	1)	orig	The chain to be populated with the unzipped data
	//				2)	gzipped	The chain containing the original (zipped) data
	//
	//	Returns:	E_NODATA	If there is nothing to do
	//				E_WRITEFAIL	If the original data could not be written out to the temporary file
	//				E_SYSERROR	If the gunzip command could not be found or failed to process the data
	//				E_READFIAL	If the unzipped data could not be read back from the temporary file
	//				E_OK		If the operation was successful
	//
	//	Note:	Do not unzip files that contain multiple files and/or accross mutiple directories. It won't work and will leave a mess!

	_hzfunc("Punzip") ;

	ofstream	os ;	//	Output stream
	ifstream	is ;	//	Input stream

	orig.Clear() ;

	os.open(".temp_zip.zip") ;
	os << zipped ;
	os.close() ;

	system("unzip -p .temp_zip.zip > .temp_zip") ;

	is.open(".temp_zip") ;
	orig << is ;
	is.close() ;

	unlink(".temp_zip.zip") ;
	unlink(".temp_zip") ;
	return E_OK ;
}

/*
**	SECTION 4:	MD5 Digest
*/

/*
**	Rotate amounts used in the algorithm
*/

static	uint32_t	s_sin_vals[] =
{
	//	Please note the following legal notice. This table is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
	//
	//	This array comprises 64 numbers such that the Ith value in the array is equal to the integer part of 4294967296 times abs(sin(I)) where I is in radians.
	//	This is as prescribed in RFC1321.

	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
} ;

static	uchar	s_sin_oset[] =
{
	//	Please note the following legal notice. This table is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm

	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	1,  6, 11,  0,  5, 10, 15,  4,  9, 14,  3,  8, 13,  2,  7, 12,
	5,  8, 11, 14,  1,  4,  7, 10, 13,  0,  3,  6,  9, 12, 15,  2,
	0,  7, 14,  5, 12,  3, 10,  1,  8, 15,  6, 13,  4, 11,  2,  9,
} ;

static	uchar	s_sin_shft[] =
{
	//	Please note the following legal notice. This table is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm

	7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
	5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
	4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,	
	6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,	
} ;

class	_md5_unit
{
	//	Please note the following legal notice. This class is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm

public:
	uint32_t	len ;
	uint32_t	state[4] ;
	uint32_t	resv ;
} ;

_md5_unit *nil ;

void	_md5_encode	(uchar *out, uint32_t* in, uint32_t len)
{
	//	Category:	Codec
	//
	//	Please note the following legal notice. This function is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
	//
	//	Encodes input. Assumes len is xple of 4
	//
	//	Arguments:	1)	out		Pointer into output buffer
	//				2)	in		Pointer into input buffer
	//				3)	len		Buffer length
	//
	//	Returns:	None

	_hzfunc(__func__) ;

	uint32_t	x ;		//	Interinm value
	uchar*		e ;		//	Output iterator

	for(e = out + len ; out < e ;)
	{
		x = *in++ ;
		*out++ = x ;
		*out++ = x >> 8 ;
		*out++ = x >> 16 ;
		*out++ = x >> 24 ;
	}
}

void	_md5_decode	(uint32_t* out, uchar* in, uint32_t len)
{
	//	Category:	Codec
	//
	//	Please note the following legal notice. This function is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
	//
	//	Decodes input. Assumes len is xple of 4
	//
	//	Arguments:	1)	out		Pointer into output buffer
	//				2)	in		Pointer into input buffer
	//				3)	len		Buffer length
	//
	//	Returns:	None

	_hzfunc(__func__) ;

	uchar*	e ;		//	Input limiter

	for(e = in + len ; in < e ; in += 4)
		*out++ = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24) ;
}

_md5_unit*	_xlate_md5	(uchar* p, uint32_t len, uchar* digest, _md5_unit* s)
{
	//	Category:	Codec
	//
	//	Please note the following legal notice. This function is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
	//
	//	Perform MD5 translation
	//
	//	Arguments:	1)	p		Pointer
	//				2)	len		Length of
	//				3)	digest	Pointer to the
	//				4)	s		Pointer to MD5 unit
	//
	//	Returns:	Pointer to an MD5 unit

	_hzfunc(__func__) ;

	uint32_t	a ;			//	Part a of 32-bit interim value
	uint32_t	b ;			//	Part b of 32-bit interim value
	uint32_t	c ;			//	Part c of 32-bit interim value
	uint32_t	d ;			//	Part d of 32-bit interim value
	uint32_t	tmp;
	uint32_t	i ;
	uint32_t	done;
	uchar*		end;
	uint32_t	x[16];

	if(s == nil)
	{
		s = new _md5_unit() ;

		if(s == nil)
			return nil ;

		//	Seed the state
		s->state[0] = 0x67452301 ;
		s->state[1] = 0xefcdab89 ;
		s->state[2] = 0x98badcfe ;
		s->state[3] = 0x10325476 ;
	}
	s->len += len ;

	i = len & 0x3f ;
	if(i || len == 0)
	{
		done = 1 ;

		//	Pad the input
		if(i < 56)
			i = 56 - i ;
		else
			i = 120 - i ;
		if(i > 0)
		{
			memset(p + len, 0, i) ;
			p[len] = 0x80 ;
		}
		len += i ;

		//	Append the count
		x[0] = s->len<<3 ;
		x[1] = s->len>>29 ;
		_md5_encode(p+len, x, 8) ;
	}
	else
		done = 0 ;

	for(end = p+len ; p < end ; p += 64)
	{
		a = s->state[0] ;
		b = s->state[1] ;
		c = s->state[2] ;
		d = s->state[3] ;

		_md5_decode(x, p, 64) ;
	
		for(i = 0 ; i < 64 ; i++)
		{
			switch(i>>4)
			{
			case 0:	a += (b & c) | (~b & d) ;	break ;
			case 1:	a += (b & d) | (c & ~d) ;	break ;
			case 2:	a += b ^ c ^ d ;			break ;
			case 3:	a += c ^ (b | ~d) ;			break ;
			}

			a += x[s_sin_oset[i]] + s_sin_vals[i] ;
			a = (a << s_sin_shft[i]) | (a >> (32 - s_sin_shft[i])) ;
			a += b ;
	
			/* rotate variables */
			tmp = d ;
			d = c ;
			c = b ;
			b = a ;
			a = tmp ;
		}

		s->state[0] += a ;
		s->state[1] += b ;
		s->state[2] += c ;
		s->state[3] += d ;
	}

	//	return result
	if(done)
	{
		_md5_encode(digest, s->state, 16) ;
		delete s ;
		return nil ;
	}
	return s ;
}

hzMD5::hzMD5	(void)	{ memset(m_md5val, 0, 16) ; }

bool	hzMD5::operator==	(const hzMD5& op) const	{ return memcmp(m_md5val, op.m_md5val, 16) == 0 ? true : false ; }
bool	hzMD5::operator!=	(const hzMD5& op) const	{ return memcmp(m_md5val, op.m_md5val, 16) == 0 ? false : true ; }

hzString	hzMD5::Str	(void) const
{
	//	Category:	Codec
	//
	//	Present MD5 as a string
	//
	//	Arguments:	None
	//	Returns:	Instance of hzString with the MD5 as 32-char hex

	hzString	S ;			//	Return string
	uint32_t	oset ;		//	Offset into MD5
	char		buf[36] ;	//	Working buffer

	for (oset = 0 ; oset < 16 ; oset++)
	{
		sprintf(buf + (oset * 2), "%02x", m_md5val[oset]) ;
	}

	buf[32] = 0 ;
	S = buf ;
	return  S ;
}

hzEcode	hzMD5::CalcMD5File	(const char* filepath)
{
	//	Category:	Codec
	//
	//	Please note the following legal notice. This function is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
	//
	//	Calculate the MD5 digest for the supplied file
	//
	//	Arguments:	1)	filepath	The file to be hashed
	//
	//	Returns:	E_ARGUMENT	If the filepath is not supplied
    //				E_NOTFOUND	If the filepath specified does not exist as a directory entry of any kind
    //				E_TYPE		If the filepath names a directory entry that is not a file
    //				E_NODATA	If the filepath is a file but empty
    //				E_OPENFAIL	If the file specified could not be opened for reading
	//				E_OK		If this MD5 value is calculated

	_hzfunc("hzMD5::CalcMD5(file)") ;

	ifstream	is ;			//	Input stream
	_md5_unit*	s ;				//	MD5 encoding unit
	uchar*		buf ;			//	Working data buffer
	int32_t		n ;				//	Buffer limit
	hzEcode		rc ;			//	Return code

	rc = OpenInputStrm(is, filepath, *_fn) ;
	if (rc != E_OK)
		return rc ;

	s = nil ;
	n = 0 ;
	buf = new uchar[16384] ;

	for (;;)
	{
		is.read((char*) (buf + n), 8192) ;
		if (!is.gcount())
			break ;

		n += is.gcount() ;
		if(n & 0x3f)
			continue ;

		s = _xlate_md5(buf, n, 0, s) ;
		n = 0 ;
	}
	_xlate_md5(buf, n, m_md5val, s) ;

	delete [] buf ;
	is.close()  ;

	return E_OK ;
}

hzEcode	hzMD5::CalcMD5	(const hzChain& Z)
{
	//	Category:	Codec
	//
	//	Please note the following legal notice. This function is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
	//
	//	Calculate the MD5 digest for the supplied file
	//
	//	Arguments:	1)	filepath	The file to be hashed
	//
	//	Returns:	E_NODATA	If the input chain is empty
	//				E_OK		If this MD5 value is calculated

	_hzfunc("hzMD5::CalcMD5(hzChain)") ;

	chIter		zi ;		//	Input chain iterator
	_md5_unit*	s ;			//	MD5 encoding unit
	uchar*		buf ;		//	Working data buffer
	int32_t		n ;			//	Buffer limit
	uint32_t	nBytes ;	//	No of bytes extracted from input chain

	if (!Z.Size())
		return E_NODATA ;

	zi = Z ;
	s = nil ;
	n = 0 ;
	buf = new uchar[16384] ;

	for (;;)
	{
		nBytes = zi.Write(buf + n, 8192) ;
		if (!nBytes)
			break ;

		zi += nBytes ;
		n += nBytes ;
		if(n & 0x3f)
			continue ;

		s = _xlate_md5(buf, n, 0, s) ;
		n = 0 ;
	}
	_xlate_md5(buf, n, m_md5val, s) ;

	delete [] buf ;
	return E_OK ;
}

hzEcode	hzMD5::CalcMD5	(const char* cstr)
{
	//	Category:	Codec
	//
	//	Please note the following legal notice. This function is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
	//
	//	Calculate the MD5 digest for the supplied file
	//
	//	Arguments:	1)	filepath	The file to be hashed
	//
	//	Returns:	E_ARGUMENT	If no input cstr is supplied
	//				E_OK		If this MD5 value is calculated

	_hzfunc("hzMD5::CalcMD5(cstr)") ;

	_md5_unit*	s ;			//	MD5 encoding unit
	uchar*		buf ;		//	Working data buffer
	uint32_t	len ;		//	String length

	if (!cstr || !cstr[0])
		return E_ARGUMENT ;

	len = strlen(cstr) ;

	s = nil ;
	buf = new uchar[16384] ;
	strcpy((char*) buf, cstr) ;

	_xlate_md5(buf, len, m_md5val, s) ;

	delete [] buf ;
	return E_OK ;
}

/*
**	SECTION 5:	Endian Conversions
*/

//	FnSet:		GetNByte
//	Category	Bitwise
//
//  Set an integer as a big-endian manifestation of a string or partial string. This is where the first character of the string is placed in the most
//  significant byte of the integer. Each successive byte in the string is placed in bytes of successively lower significant in the integer until the
//  least significant byte in the integer is populated. The integer is initialized to zero and the process stops upon the null terminator. Converting
//  a string of chars to big-endian manifestations allows inter-alia, for the rapid comparison of strings by direct comparison of the integers. This
//  is 8 times faster on a 64 bit machine. String comparison is not a slow process but where large populations of strings are being managed this can
//  result in a worthwhile saving.
//
//  Arguments:  1)  v   Reference to the integer value to be set
//              2)  i   The pointer into the string
//
//	Returns:	None
//
//	Func:	_getv1byte(uint32_t&,const uchar*)
//	Func:	_getv2byte(uint32_t&,const uchar*)
//	Func:	_getv3byte(uint32_t&,const uchar*)
//	Func:	_getv4byte(uint32_t&,const uchar*)
//	Func:	_getv5byte(uint64_t&,const uchar*)
//	Func:	_getv6byte(uint64_t&,const uchar*)
//	Func:	_getv7byte(uint64_t&,const uchar*)
//	Func:	_getv8byte(uint64_t&,const uchar*)

inline void _getv1byte  (uint32_t& v, const uchar* i) {v=0; if (i) v=i[0];}
inline void _getv2byte  (uint32_t& v, const uchar* i) {v=0; if (i) v=(i[0]<<8)+i[1];}
inline void _getv3byte  (uint32_t& v, const uchar* i) {v=0; if (i) v=(i[0]<<16)+(i[1]<<8)+i[2];}
inline void _getv4byte  (uint32_t& v, const uchar* i) {v=0; if (i) v=(i[0]<<24)+(i[1]<<16)+(i[2]<<8)+i[3];}
inline void _getv5byte  (uint64_t& v, const uchar* i) {v=0; if (i) v=i[0];v<<=32;v|=((i[1]<<24)+(i[2]<<16)+(i[3]<<8)+i[4]);}
inline void _getv6byte  (uint64_t& v, const uchar* i) {v=0; if (i) v=(i[0]<<8)+i[1];v<<=32;v|=((i[2]<<24)+(i[3]<<16)+(i[4]<<8)+i[5]);}
inline void _getv7byte  (uint64_t& v, const uchar* i) {v=0; if (i) v=(i[0]<<16)+(i[1]<<8)+i[2];v<<=32;v|=((i[3]<<24)+(i[4]<<16)+(i[5]<<8)+i[6]);}
inline void _getv8byte  (uint64_t& v, const uchar* i) {v=0; if (i) v=(i[0]<<24)+(i[1]<<16)+(i[2]<<8)+i[3];v<<=32;v|=((i[4]<<24)+(i[5]<<16)+(i[6]<<8)+i[7]);}

//	FnSet:		SetNbyte
//	Category	Bitwise
//
//  Populate a character buffer presumed to be of sufficent size, with an integer presumed to be a big-endian manifestation of a string. This is where
//  the most significant byte in the integer becomes the first byte of the buffer, the second most significant byte in the integer becomes the second
//  byte in the buffer and so on until the least significant byte in the integer is written to the buffer.
//
//  Arguments:  1)  s   The pointer into the buffer recieving the string
//              2)  n   The integer
//
//	Returns:	None
//
//	Func:	_setv1byte(uchar*,char)
//	Func:	_setv2byte(uchar*,uint16_t)
//	Func:	_setv3byte(uchar*,uint32_t)
//	Func:	_setv4byte(uchar*,uint32_t)
//	Func:	_setv5byte(uchar*,uint64_t)
//	Func:	_setv6byte(uchar*,uint64_t)
//	Func:	_setv7byte(uchar*,uint64_t)
//	Func:   _setv8byte(uchar*,uint64_t)
//	Func:   _setv2byte(uchar*,int16_t)
//	Func:   _setv3byte(uchar*,int32_t)
//	Func:   _setv4byte(uchar*,int32_t)
//	Func:   _setv5byte(uchar*,int64_t)
//	Func:   _setv6byte(uchar*,int64_t)
//	Func:   _setv7byte(uchar*,int64_t)
//	Func:   _setv8byte(uchar*,int64_t)

inline void _setv1byte  (uchar* s, char n)      {if (s) s[0]=(n&0xff);}
inline void _setv2byte  (uchar* s, uint16_t n)  {if (s) {s[1]=(n&0xff); s[0]=((n>>8)&0xff);}}
inline void _setv3byte  (uchar* s, uint32_t n)  {if (s) {s[2]=(n&0xff); s[1]=((n>>8)&0xff); s[0]=((n>>16)&0xff);}}
inline void _setv4byte  (uchar* s, uint32_t n)  {if (s) {s[3]=(n&0xff); s[2]=((n>>8)&0xff); s[1]=((n>>16)&0xff); s[0]=((n>>24)&0xff);}}
inline void _setv5byte  (uchar* s, uint64_t n)  {if (s) {s[4]=(n&0xff); s[3]=((n>>8)&0xff); s[2]=((n>>16)&0xff); s[1]=((n>>24)&0xff); s[0]=((n>>32)&0xff);}}

inline void _setv6byte  (uchar* s, uint64_t n)
    { if (s) {s[5]=(n&0xff); s[4]=((n>>8)&0xff); s[3]=((n>>16)&0xff); s[2]=((n>>24)&0xff); s[1]=((n>>32)&0xff); s[0]=((n>>40)&0xff);}}

inline void _setv7byte  (uchar* s, uint64_t n)
    {if (!s) return; s[6]=(n&0xff); s[5]=((n>>8)&0xff); s[4]=((n>>16)&0xff); s[3]=((n>>24)&0xff); s[2]=((n>>32)&0xff); s[1]=((n>>40)&0xff); s[0]=((n>>48)&0xff);}

inline  void    _setv8byte  (uchar* s, uint64_t n)
    {if (!s) return; s[7]=(n&0xff); s[6]=((n>>8)&0xff); s[5]=((n>>16)&0xff); s[4]=((n>>24)&0xff); s[3]=((n>>32)&0xff); s[2]=((n>>40)&0xff); s[1]=((n>>48)&0xff); s[0]=((n>>56)&0xff);}

inline  void    _setv2byte  (uchar* s, int16_t n)   { _setv2byte(s, (uint16_t)n) ; }
inline  void    _setv3byte  (uchar* s, int32_t n)   { _setv3byte(s, (uint32_t)n) ; }
inline  void    _setv4byte  (uchar* s, int32_t n)   { _setv4byte(s, (uint32_t)n) ; }
inline  void    _setv5byte  (uchar* s, int64_t n)   { _setv5byte(s, (uint64_t)n) ; }
inline  void    _setv6byte  (uchar* s, int64_t n)   { _setv6byte(s, (uint64_t)n) ; }
inline  void    _setv7byte  (uchar* s, int64_t n)   { _setv7byte(s, (uint64_t)n) ; }
inline  void    _setv8byte  (uchar* s, int64_t n)   { _setv8byte(s, (uint64_t)n) ; }
