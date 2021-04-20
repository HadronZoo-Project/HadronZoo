//
//	File:	hzTypes.cpp
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

#include <stdarg.h>

#include "hzChars.h"
#include "hzProcess.h"
#include "hzTextproc.h"
#include "hzDissemino.h"

/*
**	Text form of internal and external data types
*/

global	const char*	Basetype2Txt	(hdbBasetype dtype)
{
	//	Category:	Diagnostics
	//
	//	Convert a HadroZoo data type to its text name and return as const char*
	//
	//	Arguments:	1)	dtype	HadronZoo or standard C++ datatype
	//
	//	Returns:	Pointer to datytype description as cstr

	static const hzString	_typstrUKN		= "BASETYPE_NOT_DEFINED" ;		//	used to indicate illegal type mix

	//	Group 1: Fundamental C++ types (fixed size)
	static const hzString	_typstrCPP_UKN	= "BASETYPE_CPP_UNDEF" ;	//	Group 1 undefined
	static const hzString	_typstrDBL		= "BASETYPE_DOUBLE" ;		//	64 bit floating point value
	static const hzString	_typstrI64		= "BASETYPE_INT64" ;		//	64 bit Signed Integer
	static const hzString	_typstrI32		= "BASETYPE_INT32" ;		//	32 bit Signed Integer
	static const hzString	_typstrI16		= "BASETYPE_INT16" ;		//	16 bit Signed Integer
	static const hzString	_typstrBYTE		= "BASETYPE_BYTE" ;			//	8 bit Signed Integer
	static const hzString	_typstrU64		= "BASETYPE_UINT64" ;		//	64 bit unsigned Integer
	static const hzString	_typstrU32		= "BASETYPE_UINT32" ;		//	32 bit unsigned Integer
	static const hzString	_typstrU16		= "BASETYPE_UINT16" ;		//	16 bit unsigned Integer
	static const hzString	_typstrUBYTE	= "BASETYPE_UBYTE" ;		//	8 bit unsigned Integer
	static const hzString	_typstrBOOL		= "BASETYPE_BOOL" ;			//	either true or false

	//	Group 2: HadronZoo Defined types (fixed size)
	static const hzString	_typstrHZO_UKN	= "BASETYPE_HZO_UNDEF" ;	//	Group 2 undefined
	static const hzString	_typstrEMA		= "BASETYPE_EMADDR" ;		//	Email Address
	static const hzString	_typstrURL		= "BASETYPE_URL" ;			//	URL (Web address)
	static const hzString	_typstrIPA		= "BASETYPE_IPADDR" ;		//	IP Address
	static const hzString	_typstrTIME		= "BASETYPE_TIME" ;			//	No of seconds since midnight (4 bytes)
	static const hzString	_typstrSDATE	= "BASETYPE_SDATE" ;		//	No of days since Jan 1st year 0000
	static const hzString	_typstrXDATE	= "BASETYPE_XDATE" ;		//	Full date & time
	static const hzString	_typstrSTR		= "BASETYPE_STRING" ;		//	Variable length string (hzString class), treated as a single value
	static const hzString	_typstrTEXT		= "BASETYPE_TEXT" ;			//	Variable length string (hzString class), treated as a series of words
	static const hzString	_typstrBINARY	= "BASETYPE_BINARY" ;		//	Document or file (possible text source)
	static const hzString	_typstrTXTDOC	= "BASETYPE_TXTDOC" ;		//	Document or file (possible text source)

	//	Group 3: Application defined data enumerations
	static const hzString	_typstrENUM		= "BASETYPE_ENUM" ;			//  String enumeration set

	//	Group 4: Application defined special text types
	static const hzString	_typstrAPPDEF	= "BASETYPE_APPDEF" ;		//  String enumeration set

	//	Group 5: Application defined data class
	static const hzString	_typstrCLASS	= "BASETYPE_CLASS" ;		//  String enumeration set

	switch (dtype)
	{
	case BASETYPE_CPP_UNDEF:	return *_typstrCPP_UKN ;
	case BASETYPE_DOUBLE:		return *_typstrDBL ;
	case BASETYPE_INT64:		return *_typstrI64 ;
	case BASETYPE_INT32:		return *_typstrI32 ;
	case BASETYPE_INT16:		return *_typstrI16 ;
	case BASETYPE_BYTE:			return *_typstrBYTE ;
	case BASETYPE_UINT64:		return *_typstrU64 ;
	case BASETYPE_UINT32:		return *_typstrU32 ;
	case BASETYPE_UINT16:		return *_typstrU16 ;
	case BASETYPE_UBYTE:		return *_typstrUBYTE ;
	case BASETYPE_BOOL:			return *_typstrBOOL ;

	case BASETYPE_HZO_UNDEF:	return *_typstrHZO_UKN ;
	case BASETYPE_EMADDR:		return *_typstrEMA ;
	case BASETYPE_URL:			return *_typstrURL ;
	case BASETYPE_IPADDR:		return *_typstrIPA ;
	case BASETYPE_TIME:			return *_typstrTIME ;
	case BASETYPE_SDATE:		return *_typstrSDATE ;
	case BASETYPE_XDATE:		return *_typstrXDATE ;
	case BASETYPE_STRING:		return *_typstrSTR ;
	case BASETYPE_TEXT:			return *_typstrTEXT ;
	case BASETYPE_BINARY:		return *_typstrBINARY ;
	case BASETYPE_TXTDOC:		return *_typstrTXTDOC ;

	case BASETYPE_ENUM:			return *_typstrENUM ;
	case BASETYPE_APPDEF:		return *_typstrAPPDEF ;
	case BASETYPE_CLASS:		return *_typstrCLASS ;
	}

	return *_typstrUKN ;
}

hdbBasetype	Str2Basetype	(const hzString& S)
{
	//	Category:	Diagnostics
	//
	//	Convert the name of a HadronZoo data type to the enum
	//
	//	Arguments:	1)	S	String presumed to indicate a HadronZoo or standard C++ datatype
	//
	//	Returns:	Enum value being the data type matching supplied description

	if (S == "BASETYPE_STRING")		return BASETYPE_STRING ;
	if (S == "BASETYPE_EMADDR")		return BASETYPE_EMADDR ;
	if (S == "BASETYPE_IPADDR")		return BASETYPE_IPADDR ;
	if (S == "BASETYPE_URL")		return BASETYPE_URL ;
	if (S == "BASETYPE_SDATE")		return BASETYPE_SDATE ;
	if (S == "BASETYPE_TIME")		return BASETYPE_TIME ;
	if (S == "BASETYPE_XDATE")		return BASETYPE_XDATE ;
	if (S == "BASETYPE_DOUBLE")		return BASETYPE_DOUBLE ;
	if (S == "BASETYPE_INT64")		return BASETYPE_INT64 ;
	if (S == "BASETYPE_INT32")		return BASETYPE_INT32 ;
	if (S == "BASETYPE_INT16")		return BASETYPE_INT16 ;
	if (S == "BASETYPE_BYTE")		return BASETYPE_BYTE ;
	if (S == "BASETYPE_UINT64")		return BASETYPE_UINT64 ;
	if (S == "BASETYPE_UINT32")		return BASETYPE_UINT32 ;
	if (S == "BASETYPE_UINT16")		return BASETYPE_UINT16 ;
	if (S == "BASETYPE_UBYTE")		return BASETYPE_UBYTE ;
	if (S == "BASETYPE_ENUM")		return BASETYPE_ENUM ;
	if (S == "BASETYPE_BOOL")		return BASETYPE_BOOL ;
	if (S == "BASETYPE_TEXT")		return BASETYPE_TEXT ;
	if (S == "BASETYPE_BINARY")		return BASETYPE_BINARY ;
	if (S == "BASETYPE_TXTDOC")		return BASETYPE_TXTDOC ;

	if (S == "string")		return BASETYPE_STRING ;
	if (S == "emaddr")		return BASETYPE_EMADDR ;
	if (S == "ipaddr")		return BASETYPE_IPADDR ;
	if (S == "url")			return BASETYPE_URL ;
	if (S == "sdate")		return BASETYPE_SDATE ;
	if (S == "time")		return BASETYPE_TIME ;
	if (S == "xdate")		return BASETYPE_XDATE ;
	if (S == "double")		return BASETYPE_DOUBLE ;
	if (S == "int64_t")		return BASETYPE_INT64 ;
	if (S == "int32_t")		return BASETYPE_INT32 ;
	if (S == "int16_t")		return BASETYPE_INT16 ;
	if (S == "byte")		return BASETYPE_BYTE ;
	if (S == "uint64_t")	return BASETYPE_UINT64 ;
	if (S == "uint32_t")	return BASETYPE_UINT32 ;
	if (S == "uint16_t")	return BASETYPE_UINT16 ;
	if (S == "ubyte")		return BASETYPE_UBYTE ;
	if (S == "enum")		return BASETYPE_ENUM ;
	if (S == "bool")		return BASETYPE_BOOL ;
	if (S == "text")		return BASETYPE_TEXT ;
	if (S == "binary")		return BASETYPE_BINARY ;
	if (S == "txtdoc")		return BASETYPE_TXTDOC ;

	return BASETYPE_UNDEF ;
}

/*
**	MIME Types
*/

struct	_mimeType
{
	//	Stores the standard file ending, the enum value and a text description of a mimtype

	hzString	E ;		//	File ending
	hzString	S ;		//	Description
	hzMimetype	v ;		//	MIME type enum value

	_mimeType	(void)	{ v = HMTYPE_INVALID ; }

	_mimeType&	operator=	(const _mimeType& op)
	{
		E = op.E ;
		S = op.S ;
		v = op.v ;
		return *this ;
	}
} ;

static	hzMapS<hzString,_mimeType>	s_mimesFile ;	//	MIME types by file ending
static	hzMapS<hzString,_mimeType>	s_mimesDesc ;	//	MIME types by description
static	hzMapS<int32_t,_mimeType>	s_mimesEnum ;	//	MIME types by enumerated value

void	HadronZooInitMimes	(void)
{
	//	Category:	Internet
	//
	//	Setup table of MIME types
	//
	//	Arguments:	None
	//	Returns:	None

	_mimeType	m ;		//	MIME type info
	uint32_t	n ;		//	MIME type info iterator

	if (s_mimesEnum.Count())
	{
		threadLog("Init MIMES - Already DONE\n") ;
		return ;
	}

	threadLog("Init MIMES\n") ;

	//	Audio
	m.v=HMTYPE_AUD_ADP;			m.E=".adp";			m.S="audio/adpcm";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_AAC;			m.E=".aac";			m.S="audio/x-aac";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_AIFF;		m.E=".aif";			m.S="audio/x-aiff";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_DECE;		m.E=".uva";			m.S="audio/vnd.dece.audio";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_EOL;			m.E=".eol";			m.S="audio/vnd.digital-winds";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_DRA;			m.E=".dra";			m.S="audio/vnd.dra";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_DTS;			m.E=".dts";			m.S="audio/vnd.dts";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_DTS_HD;		m.E=".dtshd";		m.S="audio/vnd.dts.hd";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_RIP;			m.E=".rip";			m.S="audio/vnd.rip";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_LUCENT;		m.E=".lvp";			m.S="audio/vnd.lucent.voice";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_MPEGURL;		m.E=".m3u";			m.S="audio/x-mpegurl";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_PYA;			m.E=".pya";			m.S="audio/vnd.ms-playready.media.pya";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_WMA;			m.E=".wma";			m.S="audio/x-ms-wma";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_WAX;			m.E=".wax";			m.S="audio/x-ms-wax";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_MID;			m.E=".mid";			m.S="audio/midi";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_MPEG;		m.E=".mpga";		m.S="audio/mpeg";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_MP4;			m.E=".mp4a";		m.S="audio/mp4";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_ECELP4800;	m.E=".ecelp4800";	m.S="audio/vnd.nuera.ecelp4800";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_ECELP7470;	m.E=".ecelp7470";	m.S="audio/vnd.nuera.ecelp7470";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_ECELP9600;	m.E=".ecelp9600";	m.S="audio/vnd.nuera.ecelp9600";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_OGA;			m.E=".oga";			m.S="audio/ogg";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_WEBA;		m.E=".weba";		m.S="audio/webm";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_REAL_RAM;	m.E=".ram";			m.S="audio/x-pn-realaudio";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_REAL_RMP;	m.E=".rmp";			m.S="audio/x-pn-realaudio-plugin";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_BASIC_AU;	m.E=".au";			m.S="audio/basic";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_AUD_WAV;			m.E=".wav";			m.S="audio/x-wav";							s_mimesEnum.Insert(m.v,m);

	//	Chemicals
	m.v=HMTYPE_CHM_CDX;			m.E=".cdx";			m.S="chemical/x-cdx";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_CHM_CML;			m.E=".cml";			m.S="chemical/x-cml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_CHM_CSML;		m.E=".csml";		m.S="chemical/x-csml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_CHM_CIF;			m.E=".cif";			m.S="chemical/x-cif";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_CHM_CMDF;		m.E=".cmdf";		m.S="chemical/x-cmdf";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_CHM_XYZ;			m.E=".xyz";			m.S="chemical/x-xyz";						s_mimesEnum.Insert(m.v,m);

	//	Image
	m.v=HMTYPE_IMG_DXF;			m.E=".dxf";			m.S="image/vnd.dxf";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_BMP;			m.E=".bmp";			m.S="image/bmp";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_BTIF;		m.E=".btif";		m.S="image/prs.btif";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_DVB;			m.E=".sub";			m.S="image/vnd.dvb.subtitle";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_RASTER;		m.E=".ras";			m.S="image/x-cmu-raster";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_CGM;			m.E=".cgm";			m.S="image/cgm";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_CMX;			m.E=".cmx";			m.S="image/x-cmx";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_DECE;		m.E=".uvi";			m.S="image/vnd.dece.graphic";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_DJVU;		m.E=".djvu";		m.S="image/vnd.djvu";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_DWG;			m.E=".dwg";			m.S="image/vnd.dwg";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_EDMIC_MMR;	m.E=".mmr";			m.S="image/vnd.fujixerox.edmics-mmr";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_EDMIC_RLC;	m.E=".rlc";			m.S="image/vnd.fujixerox.edmics-rlc";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_XIFF;		m.E=".xif";			m.S="image/vnd.xiff";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_FST;			m.E=".fst";			m.S="image/vnd.fst";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_BIDSHEET;	m.E=".fbs";			m.S="image/vnd.fastbidsheet";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_FPX;			m.E=".fpx";			m.S="image/vnd.fpx";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_NET_FPX;		m.E=".npx";			m.S="image/vnd.net-fpx";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_FREEHAND;	m.E=".fh";			m.S="image/x-freehand";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_G3FAX;		m.E=".g3";			m.S="image/g3fax";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_GIF;			m.E=".gif";			m.S="image/gif";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_ICON;		m.E=".ico";			m.S="image/x-icon";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_IEF;			m.E=".ief";			m.S="image/ief";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_JPEG;		m.E=".jpeg";		m.S="image/jpeg";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_MODI;		m.E=".mdi";			m.S="image/vnd.ms-modi";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_KTX;			m.E=".ktx";			m.S="image/ktx";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_PCX;			m.E=".pcx";			m.S="image/x-pcx";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_PHOTOSHOP;	m.E=".psd";			m.S="image/vnd.adobe.photoshop";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_PICT;		m.E=".pic";			m.S="image/x-pict";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_ANYMAP;		m.E=".pnm";			m.S="image/x-portable-anymap";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_BITMAP;		m.E=".pbm";			m.S="image/x-portable-bitmap";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_GRAYMAP;		m.E=".pgm";			m.S="image/x-portable-graymap";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_PNG;			m.E=".png";			m.S="image/png";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_PIXMAP;		m.E=".ppm";			m.S="image/x-portable-pixmap";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_SVG;			m.E=".svg";			m.S="image/svg+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_RGB;			m.E=".rgb";			m.S="image/x-rgb";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_TIFF;		m.E=".tiff";		m.S="image/tiff";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_WBMP;		m.E=".wbmp";		m.S="image/vnd.wap.wbmp";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_WEBP;		m.E=".webp";		m.S="image/webp";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_XBITMAP;		m.E=".xbm";			m.S="image/x-xbitmap";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_XPIXMAP;		m.E=".xpm";			m.S="image/x-xpixmap";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_IMG_XWINMAP;		m.E=".xwd";			m.S="image/x-xwindowdump";					s_mimesEnum.Insert(m.v,m);

	//	Model
	m.v=HMTYPE_MOD_DWF;			m.E=".dwf";			m.S="model/vnd.dwf";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_MOD_COLLDATA;	m.E=".dae";			m.S="model/vnd.collada+xml";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_MOD_GTW;			m.E=".gtw";			m.S="model/vnd.gtw";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_MOD_GDL;			m.E=".gdl";			m.S="model/vnd.gdl";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_MOD_IGES;		m.E=".igs";			m.S="model/iges";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_MOD_MESH;		m.E=".msh";			m.S="model/mesh";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_MOD_VRML;		m.E=".wrl";			m.S="model/vrml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_MOD_MTS;			m.E=".mts";			m.S="model/vnd.mts";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_MOD_VTU;			m.E=".vtu";			m.S="model/vnd.vtu";						s_mimesEnum.Insert(m.v,m);

	//	Text
	m.v=HMTYPE_TXT_ASM;			m.E=".s";			m.S="text/x-asm";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_PLAINBAS;	m.E=".par";			m.S="text/plain-bas";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_C;			m.E=".c";			m.S="text/x-c";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_CSS;			m.E=".css";			m.S="text/css";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_CSV;			m.E=".csv";			m.S="text/csv";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_CURL;		m.E=".curl";		m.S="text/vnd.curl";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_DCURL;		m.E=".dcurl";		m.S="text/vnd.curl.dcurl";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_MCURL;		m.E=".mcurl";		m.S="text/vnd.curl.mcurl";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_SCURL;		m.E=".scurl";		m.S="text/vnd.curl.scurl";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_FILESTOR;	m.E=".flx";			m.S="text/vnd.fmi.flexstor";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_FORTRAN;		m.E=".f";			m.S="text/x-fortran";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_GRAPHVIZ;	m.E=".gv";			m.S="text/vnd.graphviz";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_HTML;		m.E=".html";		m.S="text/html";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_CALENDAR;	m.E=".ics";			m.S="text/calendar";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_IND3_3DML;	m.E=".3dml";		m.S="text/vnd.in3d.3dml";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_IND3_SPOT;	m.E=".spot";		m.S="text/vnd.in3d.spot";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_J2ME;		m.E=".jad";			m.S="text/vnd.sun.j2me.app-descriptor";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_XJAVA;		m.E=".java";		m.S="text/x-java-source";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_JS;			m.E=".js";			m.S="text/javascript";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_FLY;			m.E=".fly";			m.S="text/vnd.fly";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_N3;			m.E=".n3";			m.S="text/n3";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_PASCAL;		m.E=".p";			m.S="text/x-pascal";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_PRS_LINES;	m.E=".dsc";			m.S="text/prs.lines.tag";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_RICHTEXT;	m.E=".rtx";			m.S="text/richtext";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_SETEXT;		m.E=".etx";			m.S="text/x-setext";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_SGML;		m.E=".sgml";		m.S="text/sgml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_TABSEP;		m.E=".tsv";			m.S="text/tab-separated-values";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_PLAIN;		m.E=".txt";			m.S="text/plain";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_TROFF;		m.E=".t";			m.S="text/troff";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_TURTLE;		m.E=".ttl";			m.S="text/turtle";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_URI_LIST;	m.E=".uri";			m.S="text/uri-list";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_UUENCODE;	m.E=".uu";			m.S="text/x-uuencode";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_VCALENDAR;	m.E=".vcs";			m.S="text/x-vcalendar";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_VCARD;		m.E=".vcf";			m.S="text/x-vcard";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_WAP_WML;		m.E=".wml";			m.S="text/vnd.wap.wml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_WAP_SCR;		m.E=".wmls";		m.S="text/vnd.wap.wmlscript";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_TXT_XML;			m.E=".xml";			m.S="text/xml";								s_mimesEnum.Insert(m.v,m);

	//	Video
	m.v=HMTYPE_VID_3GP;			m.E=".3gp";			m.S="video/3gpp";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_3G2;			m.E=".3g2";			m.S="video/3gpp2";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_AVI;			m.E=".avi";			m.S="video/x-msvideo";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_UVH;			m.E=".uvh";			m.S="video/vnd.dece.hd";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_UVM;			m.E=".uvm";			m.S="video/vnd.dece.mobile";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_UVU;			m.E=".uvu";			m.S="video/vnd.uvvu.mp4";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_UVP;			m.E=".uvp";			m.S="video/vnd.dece.pd";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_UVS;			m.E=".uvs";			m.S="video/vnd.dece.sd";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_UUV;			m.E=".uvv";			m.S="video/vnd.dece.video";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_FVT;			m.E=".fvt";			m.S="video/vnd.fvt";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_F4V;			m.E=".f4v";			m.S="video/x-f4v";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_FLV;			m.E=".flv";			m.S="video/x-flv";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_FLI;			m.E=".fli";			m.S="video/x-fli";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_H261;		m.E=".h261";		m.S="video/h261";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_H263;		m.E=".h263";		m.S="video/h263";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_H264;		m.E=".h264";		m.S="video/h264";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_JPM;			m.E=".jpm";			m.S="video/jpm";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_JPEG;		m.E=".jpgv";		m.S="video/jpeg";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_MV4;			m.E=".m4v";			m.S="video/x-m4v";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_ASF;			m.E=".asf";			m.S="video/x-ms-asf";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_PYV;			m.E=".pyv";			m.S="video/vnd.ms-playready.media.pyv";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_WM;			m.E=".wm";			m.S="video/x-ms-wm";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_WMV;			m.E=".wmv";			m.S="video/x-ms-wmv";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_MVX;			m.E=".wvx";			m.S="video/x-ms-wvx";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_MJ2;			m.E=".mj2";			m.S="video/mj2";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_MXU;			m.E=".mxu";			m.S="video/vnd.mpegurl";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_MPEG;		m.E=".mpeg";		m.S="video/mpeg";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_MP4;			m.E=".mp4";			m.S="video/mp4";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_OGG;			m.E=".ogv";			m.S="video/ogg";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_WEBM;		m.E=".webm";		m.S="video/webm";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_QTIME;		m.E=".qt";			m.S="video/quicktime";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_SGI;			m.E=".movie";		m.S="video/x-sgi-movie";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_VID_VIVO;		m.E=".viv";			m.S="video/vnd.vivo";						s_mimesEnum.Insert(m.v,m);

	//	Applications
	m.v=HMTYPE_APP_001;			m.E=".x3d";			m.S="application/vnd.hzn-3d-crossword";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_002;			m.E=".mseq";		m.S="application/vnd.mseq";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_003;			m.E=".pwn";			m.S="application/vnd.3m.post-it-notes";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_004;			m.E=".plb";			m.S="application/vnd.3gpp.pic-bw-large";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_005;			m.E=".psb";			m.S="application/vnd.3gpp.pic-bw-small";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_006;			m.E=".pvb";			m.S="application/vnd.3gpp.pic-bw-var";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_007;			m.E=".tcap";		m.S="application/vnd.3gpp2.tcap";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_008;			m.E=".7z";			m.S="application/x-7z-compressed";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_009;			m.E=".abw";			m.S="application/x-abiword";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_010;			m.E=".ace";			m.S="application/x-ace-compressed";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_011;			m.E=".acc";			m.S="application/vnd.americandynamics.acc";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_012;			m.E=".acu";			m.S="application/vnd.acucobol";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_013;			m.E=".atc";			m.S="application/vnd.acucorp";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_014;			m.E=".aab";			m.S="application/x-authorware-bin";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_015;			m.E=".aam";			m.S="application/x-authorware-map";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_016;			m.E=".aas";			m.S="application/x-authorware-seg";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_FLASH;		m.E=".swf";			m.S="application/x-shockwave-flash";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_018;			m.E=".fxp";			m.S="application/vnd.adobe.fxp";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_PDF;			m.E=".pdf";			m.S="application/pdf";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_020;			m.E=".ppd";			m.S="application/vnd.cups-ppd";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_021;			m.E=".dir";			m.S="application/x-director";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_022;			m.E=".xdp";			m.S="application/vnd.adobe.xdp+xml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_023;			m.E=".xfdf";		m.S="application/vnd.adobe.xfdf";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_024;			m.E=".ahead";		m.S="application/vnd.ahead.space";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_025;			m.E=".azf";			m.S="application/vnd.airzip.filesecure.azf";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_026;			m.E=".azs";			m.S="application/vnd.airzip.filesecure.azs";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_027;			m.E=".azw";			m.S="application/vnd.amazon.ebook";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_028;			m.E=".ami";			m.S="application/vnd.amiga.ami";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_029;			m.E="";				m.S="application/andrew-inset";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_030;			m.E=".apk";			m.S="application/vnd.android.package-archive";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_031;			m.E=".cii";			m.S="application/vnd.anser-web-certificate-issue-initiation";	s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_032;			m.E=".fti";			m.S="application/vnd.anser-web-funds-transfer-initiation";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_033;			m.E=".atx";			m.S="application/vnd.antix.game-component";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_034;			m.E=".mpkg";		m.S="application/vnd.apple.installer+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_035;			m.E=".aw";			m.S="application/applixware";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_036;			m.E=".les";			m.S="application/vnd.hhe.lesson-player";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_037;			m.E=".swi";			m.S="application/vnd.aristanetworks.swi";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_038;			m.E=".atomcat";		m.S="application/atomcat+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_039;			m.E=".atomsvc";		m.S="application/atomsvc+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_040;			m.E=".xml";			m.S="application/atom+xml";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_041;			m.E=".ac";			m.S="application/pkix-attr-cert";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_042;			m.E=".aep";			m.S="application/vnd.audiograph";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_043;			m.E=".bcpio";		m.S="application/x-bcpio";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_044;			m.E=".bin";			m.S="application/octet-stream";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_045;			m.E=".torrent";		m.S="application/x-bittorrent";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_046;			m.E=".cod";			m.S="application/vnd.rim.cod";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_047;			m.E=".mpm";			m.S="application/vnd.blueice.multipass";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_048;			m.E=".bmi";			m.S="application/vnd.bmi";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_049;			m.E=".sh";			m.S="application/x-sh";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_050;			m.E=".rep";			m.S="application/vnd.businessobjects";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_BZIP;		m.E=".bz";			m.S="application/x-bzip";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_BZIP2;		m.E=".bz2";			m.S="application/x-bzip2";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_053;			m.E=".csh";			m.S="application/x-csh";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_054;			m.E=".cdxml";		m.S="application/vnd.chemdraw+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_055;			m.E=".cdbcmsg";		m.S="application/vnd.contact.cmsg";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_056;			m.E=".cla";			m.S="application/vnd.claymore";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_057;			m.E=".c4g";			m.S="application/vnd.clonk.c4group";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_058;			m.E=".cdmia";		m.S="application/cdmi-capability";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_059;			m.E=".cdmic";		m.S="application/cdmi-container";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_060;			m.E=".cdmid";		m.S="application/cdmi-domain";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_061;			m.E=".cdmio";		m.S="application/cdmi-object";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_062;			m.E=".cdmiq";		m.S="application/cdmi-queue";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_063;			m.E=".c11amc";		m.S="application/vnd.cluetrust.cartomobile-config";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_064;			m.E=".c11amz";		m.S="application/vnd.cluetrust.cartomobile-config-pkg";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_065;			m.E=".cpt";			m.S="application/mac-compactpro";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_066;			m.E=".wmlc";		m.S="application/vnd.wap.wmlc";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_067;			m.E=".xar";			m.S="application/vnd.xara";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_068;			m.E=".cmc";			m.S="application/vnd.cosmocaller";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_069;			m.E=".cpio";		m.S="application/x-cpio";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_070;			m.E=".clkx";		m.S="application/vnd.crick.clicker";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_071;			m.E=".clkk";		m.S="application/vnd.crick.clicker.keyboard";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_072;			m.E=".clkp";		m.S="application/vnd.crick.clicker.palette";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_073;			m.E=".clkt";		m.S="application/vnd.crick.clicker.template";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_074;			m.E=".clkw";		m.S="application/vnd.crick.clicker.wordbank";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_075;			m.E=".wbs";			m.S="application/vnd.criticaltools.wbs+xml";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_076;			m.E=".cryptonote";	m.S="application/vnd.rig.cryptonote";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_077;			m.E=".cu";			m.S="application/cu-seeme";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_078;			m.E=".cww";			m.S="application/prs.cww";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_079;			m.E=".car";			m.S="application/vnd.curl.car";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_080;			m.E=".pcurl";		m.S="application/vnd.curl.pcurl";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_081;			m.E=".cmp";			m.S="application/vnd.yellowriver-custom-menu";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_082;			m.E=".dssc";		m.S="application/dssc+der";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_083;			m.E=".xdssc";		m.S="application/dssc+xml";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_084;			m.E=".deb";			m.S="application/x-debian-package";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_085;			m.E=".dvi";			m.S="application/x-dvi";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_086;			m.E=".seed";		m.S="application/vnd.fdsn.seed";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_087;			m.E=".dtb";			m.S="application/x-dtbook+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_088;			m.E=".res";			m.S="application/x-dtbresource+xml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_089;			m.E=".ait";			m.S="application/vnd.dvb.ait";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_090;			m.E=".svc";			m.S="application/vnd.dvb.service";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_091;			m.E=".dtd";			m.S="application/xml-dtd";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_092;			m.E=".mlp";			m.S="application/vnd.dolby.mlp";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_093;			m.E=".wad";			m.S="application/x-doom";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_094;			m.E=".dpg";			m.S="application/vnd.dpgraph";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_095;			m.E=".dfac";		m.S="application/vnd.dreamfactory";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_096;			m.E=".geo";			m.S="application/vnd.dynageo";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_097;			m.E=".es";			m.S="application/ecmascript";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_098;			m.E=".mag";			m.S="application/vnd.ecowin.chart";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_099;			m.E=".exi";			m.S="application/exi";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_100;			m.E=".mgz";			m.S="application/vnd.proteus.magazine";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_EPUB;		m.E=".epub";		m.S="application/epub+zip";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_102;			m.E=".nml";			m.S="application/vnd.enliven";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_103;			m.E=".xpr";			m.S="application/vnd.is-xpr";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_104;			m.E=".xfdl";		m.S="application/vnd.xfdl";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_105;			m.E=".emma";		m.S="application/emma+xml";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_106;			m.E=".ez2";			m.S="application/vnd.ezpix-album";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_107;			m.E=".ez3";			m.S="application/vnd.ezpix-package";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_108;			m.E=".fe_launch";	m.S="application/vnd.denovo.fcselayout-link";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_109;			m.E=".ftc";			m.S="application/vnd.fluxtime.clip";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_110;			m.E=".fdf";			m.S="application/vnd.fdf";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_111;			m.E=".mif";			m.S="application/vnd.mif";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_112;			m.E=".fm";			m.S="application/vnd.framemaker";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_113;			m.E=".fsc";			m.S="application/vnd.fsc.weblaunch";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_114;			m.E=".fnc";			m.S="application/vnd.frogans.fnc";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_115;			m.E=".ltf";			m.S="application/vnd.frogans.ltf";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_116;			m.E=".ddd";			m.S="application/vnd.fujixerox.ddd";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_117;			m.E=".xdw";			m.S="application/vnd.fujixerox.docuworks";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_118;			m.E=".xbd";			m.S="application/vnd.fujixerox.docuworks.binder";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_119;			m.E=".oas";			m.S="application/vnd.fujitsu.oasys";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_120;			m.E=".oa2";			m.S="application/vnd.fujitsu.oasys2";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_121;			m.E=".oa3";			m.S="application/vnd.fujitsu.oasys3";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_122;			m.E=".fg5";			m.S="application/vnd.fujitsu.oasysgp";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_123;			m.E=".bh2";			m.S="application/vnd.fujitsu.oasysprs";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_124;			m.E=".spl";			m.S="application/x-futuresplash";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_125;			m.E=".fzs";			m.S="application/vnd.fuzzysheet";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_126;			m.E=".gmx";			m.S="application/vnd.gmx";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_127;			m.E=".txd";			m.S="application/vnd.genomatix.tuxedo";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_128;			m.E=".ggb";			m.S="application/vnd.geogebra.file";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_129;			m.E=".ggt";			m.S="application/vnd.geogebra.tool";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_130;			m.E=".gex";			m.S="application/vnd.geometry-explorer";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_131;			m.E=".gxt";			m.S="application/vnd.geonext";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_132;			m.E=".g2w";			m.S="application/vnd.geoplan";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_133;			m.E=".g3w";			m.S="application/vnd.geospace";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_134;			m.E=".gsf";			m.S="application/x-font-ghostscript";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_135;			m.E=".bdf";			m.S="application/x-font-bdf";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_TAR;			m.E=".tar";			m.S="application/tar";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_GTAR;		m.E=".gtar";		m.S="application/x-gtar";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_137;			m.E=".texinfo";		m.S="application/x-texinfo";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_138;			m.E=".gnumeric";	m.S="application/x-gnumeric";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_139;			m.E=".kml";			m.S="application/vnd.google-earth.kml+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_140;			m.E=".kmz";			m.S="application/vnd.google-earth.kmz";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_141;			m.E=".gqf";			m.S="application/vnd.grafeq";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_142;			m.E=".gac";			m.S="application/vnd.groove-account";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_143;			m.E=".ghf";			m.S="application/vnd.groove-help";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_144;			m.E=".gim";			m.S="application/vnd.groove-identity-message";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_145;			m.E=".grv";			m.S="application/vnd.groove-injector";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_146;			m.E=".gtm";			m.S="application/vnd.groove-tool-message";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_147;			m.E=".tpl";			m.S="application/vnd.groove-tool-template";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_148;			m.E=".vcg";			m.S="application/vnd.groove-vcard";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_149;			m.E=".hpid";		m.S="application/vnd.hp-hpid";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_150;			m.E=".hps";			m.S="application/vnd.hp-hps";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_151;			m.E=".hdf";			m.S="application/x-hdf";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_152;			m.E=".hbci";		m.S="application/vnd.hbci";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_153;			m.E=".jlt";			m.S="application/vnd.hp-jlyt";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_154;			m.E=".pcl";			m.S="application/vnd.hp-pcl";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_155;			m.E=".hpgl";		m.S="application/vnd.hp-hpgl";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_156;			m.E=".hvs";			m.S="application/vnd.yamaha.hv-script";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_157;			m.E=".hvd";			m.S="application/vnd.yamaha.hv-dic";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_158;			m.E=".hvp";			m.S="application/vnd.yamaha.hv-voice";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_159;			m.E=".sfd-hdstx";	m.S="application/vnd.hydrostatix.sof-data";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_160;			m.E=".stk";			m.S="application/hyperstudio";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_161;			m.E=".hal";			m.S="application/vnd.hal+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_162;			m.E=".irm";			m.S="application/vnd.ibm.rights-management";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_163;			m.E=".sc";			m.S="application/vnd.ibm.secure-container";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_164;			m.E=".icc";			m.S="application/vnd.iccprofile";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_165;			m.E=".igl";			m.S="application/vnd.igloader";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_166;			m.E=".ivp";			m.S="application/vnd.immervision-ivp";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_167;			m.E=".ivu";			m.S="application/vnd.immervision-ivu";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_168;			m.E=".rif";			m.S="application/reginfo+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_169;			m.E=".i2g";			m.S="application/vnd.intergeo";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_170;			m.E=".cdy";			m.S="application/vnd.cinderella";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_171;			m.E=".xpw";			m.S="application/vnd.intercon.formnet";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_172;			m.E=".fcs";			m.S="application/vnd.isac.fcs";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_173;			m.E=".ipfix";		m.S="application/ipfix";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_174;			m.E=".cer";			m.S="application/pkix-cert";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_175;			m.E=".pki";			m.S="application/pkixcmp";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_176;			m.E=".crl";			m.S="application/pkix-crl";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_177;			m.E=".pkipath";		m.S="application/pkix-pkipath";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_178;			m.E=".igm";			m.S="application/vnd.insors.igm";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_179;			m.E=".rcprofile";	m.S="application/vnd.ipunplugged.rcprofile";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_180;			m.E=".irp";			m.S="application/vnd.irepository.package+xml";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_181;			m.E=".jar";			m.S="application/java-archive";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_182;			m.E=".class";		m.S="application/java-vm";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_183;			m.E=".jnlp";		m.S="application/x-java-jnlp-file";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_184;			m.E=".ser";			m.S="application/java-serialized-object";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_185;			m.E=".js";			m.S="application/javascript";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_186;			m.E=".json";		m.S="application/json";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_187;			m.E=".joda";		m.S="application/vnd.joost.joda-archive";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_188;			m.E=".ktz";			m.S="application/vnd.kahootz";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_189;			m.E=".mmd";			m.S="application/vnd.chipnuts.karaoke-mmd";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_190;			m.E=".karbon";		m.S="application/vnd.kde.karbon";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_191;			m.E=".chrt";		m.S="application/vnd.kde.kchart";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_192;			m.E=".kfo";			m.S="application/vnd.kde.kformula";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_193;			m.E=".flw";			m.S="application/vnd.kde.kivio";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_194;			m.E=".kon";			m.S="application/vnd.kde.kontour";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_195;			m.E=".kpr";			m.S="application/vnd.kde.kpresenter";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_196;			m.E=".ksp";			m.S="application/vnd.kde.kspread";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_197;			m.E=".kwd";			m.S="application/vnd.kde.kword";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_198;			m.E=".htke";		m.S="application/vnd.kenameaapp";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_199;			m.E=".kia";			m.S="application/vnd.kidspiration";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_200;			m.E=".kne";			m.S="application/vnd.kinar";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_201;			m.E=".sse";			m.S="application/vnd.kodak-descriptor";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_202;			m.E=".lasxml";		m.S="application/vnd.las.las+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_203;			m.E=".latex";		m.S="application/x-latex";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_204;			m.E=".lbd";			m.S="application/vnd.llamagraphics.life-balance.desktop";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_205;			m.E=".lbe";			m.S="application/vnd.llamagraphics.life-balance.exchange+xml";	s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_206;			m.E=".jam";			m.S="application/vnd.jam";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_207;			m.E=".123";			m.S="application/vnd.lotus-1-2-3";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_208;			m.E=".apr";			m.S="application/vnd.lotus-approach";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_209;			m.E=".pre";			m.S="application/vnd.lotus-freelance";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_210;			m.E=".nsf";			m.S="application/vnd.lotus-notes";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_211;			m.E=".org";			m.S="application/vnd.lotus-organizer";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_212;			m.E=".scm";			m.S="application/vnd.lotus-screencam";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_213;			m.E=".lwp";			m.S="application/vnd.lotus-wordpro";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_214;			m.E=".portpkg";		m.S="application/vnd.macports.portpkg";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_215;			m.E=".mgp";			m.S="application/vnd.osgeo.mapguide.package";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_216;			m.E=".mrc";			m.S="application/marc";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_217;			m.E=".mrcx";		m.S="application/marcxml+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_218;			m.E=".mxf";			m.S="application/mxf";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_219;			m.E=".nbp";			m.S="application/vnd.wolfram.player";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_220;			m.E=".ma";			m.S="application/mathematica";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_221;			m.E=".mathml";		m.S="application/mathml+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_222;			m.E=".mbox";		m.S="application/mbox";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_223;			m.E=".mc1";			m.S="application/vnd.medcalcdata";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_224;			m.E=".mscml";		m.S="application/mediaservercontrol+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_225;			m.E=".cdkey";		m.S="application/vnd.mediastation.cdkey";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_226;			m.E=".mwf";			m.S="application/vnd.mfer";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_227;			m.E=".mfm";			m.S="application/vnd.mfmp";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_228;			m.E=".mads";		m.S="application/mads+xml";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_229;			m.E=".mets";		m.S="application/mets+xml";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_230;			m.E=".mods";		m.S="application/mods+xml";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_231;			m.E=".meta4";		m.S="application/metalink4+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_232;			m.E=".potm";		m.S="application/vnd.ms-powerpoint.template.macroenabled.12";	s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_233;			m.E=".docm";		m.S="application/vnd.ms-word.document.macroenabled.12";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_234;			m.E=".dotm";		m.S="application/vnd.ms-word.template.macroenabled.12";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_235;			m.E=".mcd";			m.S="application/vnd.mcd";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_236;			m.E=".flo";			m.S="application/vnd.micrografx.flo";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_237;			m.E=".igx";			m.S="application/vnd.micrografx.igx";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_238;			m.E=".es3";			m.S="application/vnd.eszigno3+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_239;			m.E=".mdb";			m.S="application/x-msaccess";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_MS_EXE;		m.E=".exe";			m.S="application/x-msdownload";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_241;			m.E=".cil";			m.S="application/vnd.ms-artgalry";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_242;			m.E=".cab";			m.S="application/vnd.ms-cab-compressed";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_243;			m.E=".ims";			m.S="application/vnd.ms-ims";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_244;			m.E=".clp";			m.S="application/x-msclip";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_245;			m.E=".eot";			m.S="application/vnd.ms-fontobject";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_MSXCEL_XLS;	m.E=".xls";			m.S="application/vnd.ms-excel";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_MSXCEL_XLAM;	m.E=".xlam";		m.S="application/vnd.ms-excel.addin.macroenabled.12";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_MSXCEL_XLSB;	m.E=".xlsb";		m.S="application/vnd.ms-excel.sheet.binary.macroenabled.12";	s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_MSXCEL_XLTM;	m.E="";				m.S="application/vnd.ms-excel.template.macroenabled.12=xltm";	s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_MSXCEL_XLSM;	m.E=".xlsm";		m.S="application/vnd.ms-excel.sheet.macroenabled.12";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_251;			m.E=".chm";			m.S="application/vnd.ms-htmlhelp";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_252;			m.E=".crd";			m.S="application/x-mscardfile";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_253;			m.E=".lrm";			m.S="application/vnd.ms-lrm";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_254;			m.E=".mvb";			m.S="application/x-msmediaview";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_MS_MONEY;	m.E=".mny";			m.S="application/x-msmoney";									s_mimesEnum.Insert(m.v,m);

	m.v=HMTYPE_APP_OPEN_PPTX;	m.E=".pptx";		m.S="application/vnd.openxmlformats-officedocument.presentationml.presentation";	s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_OPEN_SLDX;	m.E=".sldx";		m.S="application/vnd.openxmlformats-officedocument.presentationml.slide";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_OPEN_PPSX;	m.E=".ppsx";		m.S="application/vnd.openxmlformats-officedocument.presentationml.slideshow";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_OPEN_POTX;	m.E=".potx";		m.S="application/vnd.openxmlformats-officedocument.presentationml.template";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_OPEN_XLSX;	m.E=".xlsx";		m.S="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_OPEN_XLTX;	m.E=".xltx";		m.S="application/vnd.openxmlformats-officedocument.spreadsheetml.template";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_OPEN_DOCX;	m.E=".docx";		m.S="application/vnd.openxmlformats-officedocument.wordprocessingml.document";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_OPEN_DOTX;	m.E=".dotx";		m.S="application/vnd.openxmlformats-officedocument.wordprocessingml.template";		s_mimesEnum.Insert(m.v,m);

	m.v=HMTYPE_APP_264;			m.E="";				m.S="application/x-msbinder=obd";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_265;			m.E=".thmx";		m.S="application/vnd.ms-officetheme";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_266;			m.E=".onetoc";		m.S="application/onenote";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_267;			m.E=".hqx";			m.S="application/mac-binhex40";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_268;			m.E=".ppt";			m.S="application/vnd.ms-powerpoint";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_269;			m.E=".ppam";		m.S="application/vnd.ms-powerpoint.addin.macroenabled.12";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_270;			m.E=".sldm";		m.S="application/vnd.ms-powerpoint.slide.macroenabled.12";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_271;			m.E=".pptm";		m.S="application/vnd.ms-powerpoint.presentation.macroenabled.12";	s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_272;			m.E=".ppsm";		m.S="application/vnd.ms-powerpoint.slideshow.macroenabled.12";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_273;			m.E=".mpp";			m.S="application/vnd.ms-project";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_274;			m.E=".pub";			m.S="application/x-mspublisher";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_275;			m.E=".scd";			m.S="application/x-msschedule";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_276;			m.E=".xap";			m.S="application/x-silverlight-app";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_277;			m.E=".stl";			m.S="application/vnd.ms-pki.stl";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_278;			m.E=".cat";			m.S="application/vnd.ms-pki.seccat";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_279;			m.E=".vsd";			m.S="application/vnd.visio";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_280;			m.E=".wmd";			m.S="application/x-ms-wmd";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_281;			m.E=".wpl";			m.S="application/vnd.ms-wpl";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_282;			m.E=".wmz";			m.S="application/x-ms-wmz";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_283;			m.E=".wmf";			m.S="application/x-msmetafile";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_284;			m.E=".trm";			m.S="application/x-msterminal";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_MS_WORD;		m.E=".doc";			m.S="application/msword";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_286;			m.E=".wri";			m.S="application/x-mswrite";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_287;			m.E=".wps";			m.S="application/vnd.ms-works";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_288;			m.E=".xbap";		m.S="application/x-ms-xbap";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_289;			m.E=".xps";			m.S="application/vnd.ms-xpsdocument";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_290;			m.E=".mpy";			m.S="application/vnd.ibm.minipay";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_291;			m.E=".afp";			m.S="application/vnd.ibm.modcap";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_292;			m.E=".rms";			m.S="application/vnd.jcp.javame.midlet-rms";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_293;			m.E=".tmo";			m.S="application/vnd.tmobile-livetv";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_294;			m.E=".prc";			m.S="application/x-mobipocket-ebook";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_295;			m.E=".mbk";			m.S="application/vnd.mobius.mbk";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_296;			m.E=".dis";			m.S="application/vnd.mobius.dis";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_297;			m.E=".plc";			m.S="application/vnd.mobius.plc";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_298;			m.E=".mqy";			m.S="application/vnd.mobius.mqy";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_299;			m.E=".msl";			m.S="application/vnd.mobius.msl";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_300;			m.E=".txf";			m.S="application/vnd.mobius.txf";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_301;			m.E=".daf";			m.S="application/vnd.mobius.daf";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_302;			m.E="";				m.S="application/vnd.mophun.certificate=mpc";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_303;			m.E=".mpn";			m.S="application/vnd.mophun";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_304;			m.E=".m21";			m.S="application/mp21";												s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_305;			m.E=".mp4";			m.S="application/mp4";												s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_306;			m.E=".m3u8";		m.S="application/vnd.apple.mpegurl";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_307;			m.E=".mus";			m.S="application/vnd.musician";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_308;			m.E=".msty";		m.S="application/vnd.muvee.style";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_309;			m.E=".mxml";		m.S="application/xv+xml";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_310;			m.E=".ngdat";		m.S="application/vnd.nokia.n-gage.data";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_311;			m.E=".n-gage";		m.S="application/vnd.nokia.n-gage.symbian.install";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_312;			m.E=".ncx";			m.S="application/x-dtbncx+xml";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_313;			m.E=".nc";			m.S="application/x-netcdf";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_314;			m.E=".nlu";			m.S="application/vnd.neurolanguage.nlu";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_315;			m.E=".dna";			m.S="application/vnd.dna";											s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_316;			m.E=".nnd";			m.S="application/vnd.noblenet-directory";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_317;			m.E=".nns";			m.S="application/vnd.noblenet-sealer";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_318;			m.E=".nnw";			m.S="application/vnd.noblenet-web";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_319;			m.E=".rpst";		m.S="application/vnd.nokia.radio-preset";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_320;			m.E=".rpss";		m.S="application/vnd.nokia.radio-presets";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_321;			m.E=".edm";			m.S="application/vnd.novadigm.edm";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_322;			m.E=".edx";			m.S="application/vnd.novadigm.edx";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_323;			m.E=".ext";			m.S="application/vnd.novadigm.ext";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_324;			m.E=".gph";			m.S="application/vnd.flographit";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_325;			m.E=".oda";			m.S="application/oda";												s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_326;			m.E=".ogx";			m.S="application/ogg";												s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_327;			m.E=".dd2";			m.S="application/vnd.oma.dd2+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_328;			m.E=".oth";			m.S="application/vnd.oasis.opendocument.text-web";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_329;			m.E=".opf";			m.S="application/oebps-package+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_330;			m.E=".qbo";			m.S="application/vnd.intu.qbo";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_331;			m.E=".oxt";			m.S="application/vnd.openofficeorg.extension";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_332;			m.E=".osf";			m.S="application/vnd.yamaha.openscoreformat";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_333;			m.E=".odc";			m.S="application/vnd.oasis.opendocument.chart";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_334;			m.E=".otc";			m.S="application/vnd.oasis.opendocument.chart-template";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_335;			m.E=".odb";			m.S="application/vnd.oasis.opendocument.database";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_336;			m.E=".odf";			m.S="application/vnd.oasis.opendocument.formula";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_337;			m.E=".odft";		m.S="application/vnd.oasis.opendocument.formula-template";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_338;			m.E=".odg";			m.S="application/vnd.oasis.opendocument.graphics";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_339;			m.E=".otg";			m.S="application/vnd.oasis.opendocument.graphics-template";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_340;			m.E=".odi";			m.S="application/vnd.oasis.opendocument.image";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_341;			m.E=".oti";			m.S="application/vnd.oasis.opendocument.image-template";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_342;			m.E=".odp";			m.S="application/vnd.oasis.opendocument.presentation";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_343;			m.E=".otp";			m.S="application/vnd.oasis.opendocument.presentation-template";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_344;			m.E=".ods";			m.S="application/vnd.oasis.opendocument.spreadsheet";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_345;			m.E=".ots";			m.S="application/vnd.oasis.opendocument.spreadsheet-template";		s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_346;			m.E=".odt";			m.S="application/vnd.oasis.opendocument.text";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_347;			m.E=".odm";			m.S="application/vnd.oasis.opendocument.text-master";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_348;			m.E=".ott";			m.S="application/vnd.oasis.opendocument.text-template";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_349;			m.E=".sxc";			m.S="application/vnd.sun.xml.calc";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_350;			m.E=".stc";			m.S="application/vnd.sun.xml.calc.template";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_351;			m.E=".sxd";			m.S="application/vnd.sun.xml.draw";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_352;			m.E=".std";			m.S="application/vnd.sun.xml.draw.template";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_353;			m.E=".sxi";			m.S="application/vnd.sun.xml.impress";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_354;			m.E=".sti";			m.S="application/vnd.sun.xml.impress.template";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_355;			m.E=".sxm";			m.S="application/vnd.sun.xml.math";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_356;			m.E=".sxw";			m.S="application/vnd.sun.xml.writer";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_357;			m.E=".sxg";			m.S="application/vnd.sun.xml.writer.global";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_358;			m.E=".stw";			m.S="application/vnd.sun.xml.writer.template";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_359;			m.E=".otf";			m.S="application/x-font-otf";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_360;			m.E=".osfpvg";		m.S="application/vnd.yamaha.openscoreformat.osfpvg+xml";			s_mimesEnum.Insert(m.v,m);

	m.v=HMTYPE_APP_361;			m.E=".dp";			m.S="application/vnd.osgi.dp";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_362;			m.E=".pdb";			m.S="application/vnd.palm";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_363;			m.E=".paw";			m.S="application/vnd.pawaafile";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_364;			m.E=".pclxl";		m.S="application/vnd.hp-pclxl";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_365;			m.E=".efif";		m.S="application/vnd.picsel";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_366;			m.E=".prf";			m.S="application/pics-rules";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_367;			m.E=".chat";		m.S="application/x-chat";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_368;			m.E=".p10";			m.S="application/pkcs10";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_369;			m.E=".p12";			m.S="application/x-pkcs12";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_370;			m.E=".p7m";			m.S="application/pkcs7-mime";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_371;			m.E=".p7s";			m.S="application/pkcs7-signature";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_372;			m.E=".p7r";			m.S="application/x-pkcs7-certreqresp";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_373;			m.E=".p7b";			m.S="application/x-pkcs7-certificates";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_374;			m.E=".p8";			m.S="application/pkcs8";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_375;			m.E=".plf";			m.S="application/vnd.pocketlearn";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_376;			m.E=".pcf";			m.S="application/x-font-pcf";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_377;			m.E=".pfr";			m.S="application/font-tdpfr";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_378;			m.E=".pgn";			m.S="application/x-chess-pgn";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_379;			m.E=".pskcxml";		m.S="application/pskc+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_380;			m.E=".pml";			m.S="application/vnd.ctc-posml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_381;			m.E=".ai";			m.S="application/postscript";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_382;			m.E=".pfa";			m.S="application/x-font-type1";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_383;			m.E=".pbd";			m.S="application/vnd.powerbuilder6";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_384;			m.E=".pgp";			m.S="application/pgp-signature";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_385;			m.E=".box";			m.S="application/vnd.previewsystems.box";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_386;			m.E=".ptid";		m.S="application/vnd.pvi.ptid1";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_387;			m.E=".pls";			m.S="application/pls+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_388;			m.E=".str";			m.S="application/vnd.pg.format" ;							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_389;			m.E=".ei6";			m.S="application/vnd.pg.osasli";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_390;			m.E=".psf";			m.S="application/x-font-linux-psf";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_391;			m.E=".qps";			m.S="application/vnd.publishare-delta-tree";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_392;			m.E=".wg";			m.S="application/vnd.pmi.widget";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_393;			m.E=".qxd";			m.S="application/vnd.quark.quarkxpress";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_394;			m.E=".esf";			m.S="application/vnd.epson.esf";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_395;			m.E=".msf";			m.S="application/vnd.epson.msf";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_396;			m.E=".ssf";			m.S="application/vnd.epson.ssf";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_397;			m.E=".qam";			m.S="application/vnd.epson.quickanime";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_398;			m.E=".qfx";			m.S="application/vnd.intu.qfx";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_399;			m.E=".rar";			m.S="application/x-rar-compressed";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_400;			m.E=".rsd";			m.S="application/rsd+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_401;			m.E=".rm";			m.S="application/vnd.rn-realmedia";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_402;			m.E=".bed";			m.S="application/vnd.realvnc.bed";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_403;			m.E=".mxl";			m.S="application/vnd.recordare.musicxml";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_404;			m.E=".musicxml";	m.S="application/vnd.recordare.musicxml+xml";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_405;			m.E=".rnc";			m.S="application/relax-ng-compact-syntax";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_406;			m.E=".rdz";			m.S="application/vnd.data-vision.rdz";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_407;			m.E=".rdf";			m.S="application/rdf+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_408;			m.E=".rp9";			m.S="application/vnd.cloanto.rp9";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_409;			m.E=".jisp";		m.S="application/vnd.jisp";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_410;			m.E=".rtf";			m.S="application/rtf";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_411;			m.E=".link66";		m.S="application/vnd.route66.link66+xml";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_412;			m.E=".rss";			m.S="application/rss+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_413;			m.E=".shf";			m.S="application/shf+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_414;			m.E=".st";			m.S="application/vnd.sailingtracker.track";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_415;			m.E=".sus";			m.S="application/vnd.sus-calendar";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_416;			m.E=".sru";			m.S="application/sru+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_417;			m.E=".setpay";		m.S="application/set-payment-initiation";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_418;			m.E=".setreg";		m.S="application/set-registration-initiation";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_419;			m.E=".sema";		m.S="application/vnd.sema";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_420;			m.E=".semd";		m.S="application/vnd.semd";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_421;			m.E=".semf";		m.S="application/vnd.semf";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_422;			m.E=".see";			m.S="application/vnd.seemail";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_423;			m.E=".snf";			m.S="application/x-font-snf";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_424;			m.E=".spq";			m.S="application/scvp-vp-request";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_425;			m.E=".spp";			m.S="application/scvp-vp-response";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_426;			m.E=".scq";			m.S="application/scvp-cv-request";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_427;			m.E=".scs";			m.S="application/scvp-cv-response";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_428;			m.E=".sdp";			m.S="application/sdp";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_429;			m.E=".ifm";			m.S="application/vnd.shana.informed.formdata";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_430;			m.E=".itp";			m.S="application/vnd.shana.informed.formtemplate";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_431;			m.E=".iif";			m.S="application/vnd.shana.informed.interchange";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_432;			m.E=".ipk";			m.S="application/vnd.shana.informed.package";				s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_433;			m.E=".tfi";			m.S="application/thraud+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_434;			m.E=".shar";		m.S="application/x-shar";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_435;			m.E=".slt";			m.S="application/vnd.epson.salt";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_436;			m.E=".aso";			m.S="application/vnd.accpac.simply.aso";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_437;			m.E=".imp";			m.S="application/vnd.accpac.simply.imp";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_438;			m.E=".twd";			m.S="application/vnd.simtech-mindmapper";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_439;			m.E=".csp";			m.S="application/vnd.commonspace";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_440;			m.E=".saf";			m.S="application/vnd.yamaha.smaf-audio";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_441;			m.E=".mmf";			m.S="application/vnd.smaf";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_442;			m.E=".spf";			m.S="application/vnd.yamaha.smaf-phrase";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_443;			m.E=".teacher";		m.S="application/vnd.smart.teacher";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_444;			m.E=".svd";			m.S="application/vnd.svd";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_445;			m.E=".rq";			m.S="application/sparql-query";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_446;			m.E=".srx";			m.S="application/sparql-results+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_447;			m.E=".gram";		m.S="application/srgs";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_448;			m.E=".grxml";		m.S="application/srgs+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_449;			m.E=".ssml";		m.S="application/ssml+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_450;			m.E=".skp";			m.S="application/vnd.koan";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_451;			m.E=".sdc";			m.S="application/vnd.stardivision.calc";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_452;			m.E=".sda";			m.S="application/vnd.stardivision.draw";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_453;			m.E=".sdd";			m.S="application/vnd.stardivision.impress";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_454;			m.E=".smf";			m.S="application/vnd.stardivision.math";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_455;			m.E=".sdw";			m.S="application/vnd.stardivision.writer";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_456;			m.E=".sgl";			m.S="application/vnd.stardivision.writer-global";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_457;			m.E=".sm";			m.S="application/vnd.stepmania.stepchart";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_458;			m.E=".sit";			m.S="application/x-stuffit";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_459;			m.E=".sitx";		m.S="application/x-stuffitx";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_460;			m.E=".sdkm";		m.S="application/vnd.solent.sdkm+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_461;			m.E=".xo";			m.S="application/vnd.olpc-sugar";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_462;			m.E=".wqd";			m.S="application/vnd.wqd";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_463;			m.E=".sis";			m.S="application/vnd.symbian.install";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_464;			m.E=".smi";			m.S="application/smil+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_465;			m.E=".xsm";			m.S="application/vnd.syncml+xml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_466;			m.E=".bdm";			m.S="application/vnd.syncml.dm+wbxml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_467;			m.E=".xdm";			m.S="application/vnd.syncml.dm+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_468;			m.E=".sv4cpio";		m.S="application/x-sv4cpio";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_469;			m.E=".sv4crc";		m.S="application/x-sv4crc";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_470;			m.E=".sbml";		m.S="application/sbml+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_471;			m.E=".tao";			m.S="application/vnd.tao.intent-module-archive";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_XTAR;		m.E="" ;			m.S="application/x-tar=tar";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_473;			m.E=".tcl";			m.S="application/x-tcl";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_474;			m.E=".tex";			m.S="application/x-tex";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_475;			m.E=".tfm";			m.S="application/x-tex-tfm";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_476;			m.E=".tei";			m.S="application/tei+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_477;			m.E=".dxp";			m.S="application/vnd.spotfire.dxp";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_478;			m.E=".sfs";			m.S="application/vnd.spotfire.sfs";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_479;			m.E=".tsd";			m.S="application/timestamped-data";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_480;			m.E=".tpt";			m.S="application/vnd.trid.tpt";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_481;			m.E=".mxs";			m.S="application/vnd.triscape.mxs";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_482;			m.E=".tra";			m.S="application/vnd.trueapp";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_483;			m.E=".ttf";			m.S="application/x-font-ttf";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_484;			m.E=".umj";			m.S="application/vnd.umajin";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_485;			m.E=".uoml";		m.S="application/vnd.uoml+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_486;			m.E=".unityweb";	m.S="application/vnd.unity";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_487;			m.E=".ufd";			m.S="application/vnd.ufdl";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_488;			m.E=".utz";			m.S="application/vnd.uiq.theme";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_USTAR;		m.E=".ustar";		m.S="application/x-ustar";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_490;			m.E=".vcd";			m.S="application/x-cdlink";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_491;			m.E=".vsf";			m.S="application/vnd.vsf";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_492;			m.E=".vcx";			m.S="application/vnd.vcx";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_493;			m.E=".vis";			m.S="application/vnd.visionary";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_494;			m.E=".ccxml";		m.S="application/ccxml+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_495;			m.E=".vxml";		m.S="application/voicexml+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_496;			m.E=".src";			m.S="application/x-wais-source";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_497;			m.E=".wbxml";		m.S="application/vnd.wap.wbxml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_498;			m.E=".davmount";	m.S="application/davmount+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_499;			m.E=".woff";		m.S="application/x-font-woff";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_500;			m.E=".wspolicy";	m.S="application/wspolicy+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_501;			m.E=".wtb";			m.S="application/vnd.webturbo";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_502;			m.E=".wgt";			m.S="application/widget";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_503;			m.E=".hlp";			m.S="application/winhlp";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_504;			m.E=".wmlsc";		m.S="application/vnd.wap.wmlscriptc";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_505;			m.E=".wpd";			m.S="application/vnd.wordperfect";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_506;			m.E=".stf";			m.S="application/vnd.wt.stf";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_507;			m.E=".wsdl";		m.S="application/wsdl+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_508;			m.E=".der";			m.S="application/x-x509-ca-cert";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_509;			m.E=".fig";			m.S="application/x-xfig";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_510;			m.E=".xhtml";		m.S="application/xhtml+xml";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_XML;			m.E=".xml";			m.S="application/xml";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_512;			m.E=".xdf";			m.S="application/xcap-diff+xml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_513;			m.E=".xenc";		m.S="application/xenc+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_514;			m.E=".xer";			m.S="application/patch-ops-error+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_515;			m.E=".rl";			m.S="application/resource-lists+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_516;			m.E=".rs";			m.S="application/rls-services+xml";							s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_517;			m.E=".rld";			m.S="application/resource-lists-diff+xml";					s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_518;			m.E=".xslt";		m.S="application/xslt+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_519;			m.E=".xop";			m.S="application/xop+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_520;			m.E=".xpi";			m.S="application/x-xpinstall";								s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_521;			m.E=".xspf";		m.S="application/xspf+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_522;			m.E=".xul";			m.S="application/vnd.mozilla.xul+xml";						s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_523;			m.E=".yang";		m.S="application/yang";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_524;			m.E=".yin";			m.S="application/yin+xml";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_525;			m.E=".zir";			m.S="application/vnd.zul";									s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_ZIP;			m.E=".zip";			m.S="application/zip";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_GZIP;		m.E=".gz";			m.S="application/gzip";										s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_527;			m.E=".zmm";			m.S="application/vnd.handheld-entertainment+xml";			s_mimesEnum.Insert(m.v,m);
	m.v=HMTYPE_APP_528;			m.E=".zaz";			m.S="application/vnd.zzazz.deck+xml";						s_mimesEnum.Insert(m.v,m);

	threadLog("Init MIMES\n") ;
	for (n = 0 ; n < s_mimesEnum.Count() ; n++)
	{
		m = s_mimesEnum.GetObj(n) ;

		if (s_mimesDesc.Exists(m.S))
			threadLog("Already have mimetype %s\n", *m.S) ;
		else
			s_mimesDesc.Insert(m.S,m) ;

		if (s_mimesFile.Exists(m.E))
			threadLog("Already have file_end %s\n", *m.E) ;
		else
			s_mimesFile.Insert(m.E,m) ;
	}
	threadLog("Done MIMES. Total mime types %d, desc %d, endings %d\n", s_mimesEnum.Count(), s_mimesDesc.Count(), s_mimesFile.Count()) ;
}

hzMimetype	Filename2Mimetype	(const char* fpath)
{
	//	Category:	Config
	//
	//	Return the MIME type description for any given file ending
	//
	//	Arguments:	1)	fpath	Relative or full pathname of file
	//
	//	Returns:	Enum hzMimetype

	_hzfunc(__func__) ;

	if (!s_mimesEnum.Count())
		HadronZooInitMimes() ;

	const char*		i ;		//	Path iterator
	_mimeType		m ;		//	MIME type info
	hzString		S ;		//	Lookup string

	i = strchr(fpath, CHAR_PERIOD) ;
	if (i)
		S = i + 1 ;
	else
		S = fpath ;

	m = s_mimesFile[S] ;
	return m.v ;
}

hzMimetype	Str2Mimetype	(const hzString& S)
{
	//	Category:	Config
	//
	//	Return mimetype if the supplied string matches a known file ending for the mimetype
	//
	//	Arguments:	1)	S	String presumed to name a known MIME type
	//
	//	Returns:	Enum value being the MIME type matching supplied description

	if (!s_mimesEnum.Count())
		HadronZooInitMimes() ;

	_mimeType	m ;		//	MIME type info

	m = s_mimesDesc[S] ;
	return m.v ;
}

const char*	Mimetype2Txt	(hzMimetype mtype)
{
	//	Category:	Diagnostics
	//
	//	Return description of the mimetype
	//
	//	Arguments:	1)	mtype	The enumerated MIME type
	//
	//	Returns:	Pointer to const string being MIME type description

	if (!s_mimesEnum.Count())
		HadronZooInitMimes() ;

	_mimeType	m ;		//	MIME type info

	m = s_mimesEnum[mtype] ;

	return *m.S ;
}

/*
**	Languages
*/

#if XXX
hzString	_hzGlobal_langcodes[] =
{
	//	Code	Name
	"af",		"Afrikaans",
	"af-ZA",	"Afrikaans (South Africa)",
	"ar",		"Arabic",
	"ar-AE",	"Arabic (U.A.E.)",
	"ar-BH",	"Arabic (Bahrain)",
	"ar-DZ",	"Arabic (Algeria)",
	"ar-EG",	"Arabic (Egypt)",
	"ar-IQ",	"Arabic (Iraq)",
	"ar-JO",	"Arabic (Jordan)",
	"ar-KW",	"Arabic (Kuwait)",
	"ar-LB",	"Arabic (Lebanon)",
	"ar-LY",	"Arabic (Libya)",
	"ar-MA",	"Arabic (Morocco)",
	"ar-OM",	"Arabic (Oman)",
	"ar-QA",	"Arabic (Qatar)",
	"ar-SA",	"Arabic (Saudi Arabia)",
	"ar-SY",	"Arabic (Syria)",
	"ar-TN",	"Arabic (Tunisia)",
	"ar-YE",	"Arabic (Yemen)",
	"az",		"Azeri (Latin)",
	"az-AZ",	"Azeri (Latin) (Azerbaijan)",
	"az-AZ",	"Azeri (Cyrillic) (Azerbaijan)",
	"be",		"Belarusian",
	"be-BY",	"Belarusian (Belarus)",
	"bg",		"Bulgarian",
	"bg-BG",	"Bulgarian (Bulgaria)",
	"bs-BA",	"Bosnian (Bosnia and Herzegovina)",
	"ca",		"Catalan",
	"ca-ES",	"Catalan (Spain)",
	"cs",		"Czech",
	"cs-CZ",	"Czech (Czech Republic)",
	"cy",		"Welsh",
	"cy-GB",	"Welsh (United Kingdom)",
	"da",		"Danish",
	"da-DK",	"Danish (Denmark)",
	"de",		"German",
	"de-AT",	"German (Austria)",
	"de-CH",	"German (Switzerland)",
	"de-DE",	"German (Germany)",
	"de-LI",	"German (Liechtenstein)",
	"de-LU",	"German (Luxembourg)",
	"dv",		"Divehi",
	"dv-MV",	"Divehi (Maldives)",
	"el",		"Greek",
	"el-GR",	"Greek (Greece)",
	"en",		"English",
	"en-AU",	"English (Australia)",
	"en-BZ",	"English (Belize)",
	"en-CA",	"English (Canada)",
	"en-CB",	"English (Caribbean)",
	"en-GB",	"English (United Kingdom)",
	"en-IE",	"English (Ireland)",
	"en-JM",	"English (Jamaica)",
	"en-NZ",	"English (New Zealand)",
	"en-PH",	"English (Republic of the Philippines)",
	"en-TT",	"English (Trinidad and Tobago)",
	"en-US",	"English (United States)",
	"en-ZA",	"English (South Africa)",
	"en-ZW",	"English (Zimbabwe)",
	"eo",		"Esperanto",
	"es",		"Spanish",
	"es-AR",	"Spanish (Argentina)",
	"es-BO",	"Spanish (Bolivia)",
	"es-CL",	"Spanish (Chile)",
	"es-CO",	"Spanish (Colombia)",
	"es-CR",	"Spanish (Costa Rica)",
	"es-DO",	"Spanish (Dominican Republic)",
	"es-EC",	"Spanish (Ecuador)",
	"es-ES",	"Spanish (Castilian)",
	"es-ES",	"Spanish (Spain)",
	"es-GT",	"Spanish (Guatemala)",
	"es-HN",	"Spanish (Honduras)",
	"es-MX",	"Spanish (Mexico)",
	"es-NI",	"Spanish (Nicaragua)",
	"es-PA",	"Spanish (Panama)",
	"es-PE",	"Spanish (Peru)",
	"es-PR",	"Spanish (Puerto Rico)",
	"es-PY",	"Spanish (Paraguay)",
	"es-SV",	"Spanish (El Salvador)",
	"es-UY",	"Spanish (Uruguay)",
	"es-VE",	"Spanish (Venezuela)",
	"et",		"Estonian",
	"et-EE",	"Estonian (Estonia)",
	"eu",		"Basque",
	"eu-ES",	"Basque (Spain)",
	"fa",		"Farsi",
	"fa-IR",	"Farsi (Iran)",
	"fi",		"Finnish",
	"fi-FI",	"Finnish (Finland)",
	"fo",		"Faroese",
	"fo-FO",	"Faroese (Faroe Islands)",
	"fr",		"French",
	"fr-BE",	"French (Belgium)",
	"fr-CA",	"French (Canada)",
	"fr-CH",	"French (Switzerland)",
	"fr-FR",	"French (France)",
	"fr-LU",	"French (Luxembourg)",
	"fr-MC",	"French (Principality of Monaco)",
	"gl",		"Galician",
	"gl-ES",	"Galician (Spain)",
	"gu",		"Gujarati",
	"gu-IN",	"Gujarati (India)",
	"he",		"Hebrew",
	"he-IL",	"Hebrew (Israel)",
	"hi",		"Hindi",
	"hi-IN",	"Hindi (India)",
	"hr",		"Croatian",
	"hr-BA",	"Croatian (Bosnia and Herzegovina)",
	"hr-HR",	"Croatian (Croatia)",
	"hu",		"Hungarian",
	"hu-HU",	"Hungarian (Hungary)",
	"hy",		"Armenian",
	"hy-AM",	"Armenian (Armenia)",
	"id",		"Indonesian",
	"id-ID",	"Indonesian (Indonesia)",
	"is",		"Icelandic",
	"is-IS",	"Icelandic (Iceland)",
	"it",		"Italian",
	"it-CH",	"Italian (Switzerland)",
	"it-IT",	"Italian (Italy)",
	"ja",		"Japanese",
	"ja-JP",	"Japanese (Japan)",
	"ka",		"Georgian",
	"ka-GE",	"Georgian (Georgia)",
	"kk",		"Kazakh",
	"kk-KZ",	"Kazakh (Kazakhstan)",
	"kn",		"Kannada",
	"kn-IN",	"Kannada (India)",
	"ko",		"Korean",
	"ko-KR",	"Korean (Korea)",
	"kok",		"Konkani",
	"kok-IN",	"Konkani (India)",
	"ky",		"Kyrgyz",
	"ky-KG",	"Kyrgyz (Kyrgyzstan)",
	"lt",		"Lithuanian",
	"lt-LT",	"Lithuanian (Lithuania)",
	"lv",		"Latvian",
	"lv-LV",	"Latvian (Latvia)",
	"mi",		"Maori",
	"mi-NZ",	"Maori (New Zealand)",
	"mk",		"FYRO Macedonian",
	"mk-MK",	"FYRO Macedonian (Former Yugoslav Republic of Macedonia)",
	"mn",		"Mongolian",
	"mn-MN",	"Mongolian (Mongolia)",
	"mr",		"Marathi",
	"mr-IN",	"Marathi (India)",
	"ms",		"Malay",
	"ms-BN",	"Malay (Brunei Darussalam)",
	"ms-MY",	"Malay (Malaysia)",
	"mt",		"Maltese",
	"mt-MT",	"Maltese (Malta)",
	"nb",		"Norwegian (Bokm?l)",
	"nb-NO",	"Norwegian (Bokm?l) (Norway)",
	"nl",		"Dutch",
	"nl-BE",	"Dutch (Belgium)",
	"nl-NL",	"Dutch (Netherlands)",
	"nn-NO",	"Norwegian (Nynorsk) (Norway)",
	"ns",		"Northern Sotho",
	"ns-ZA",	"Northern Sotho (South Africa)",
	"pa",		"Punjabi",
	"pa-IN",	"Punjabi (India)",
	"pl",		"Polish",
	"pl-PL",	"Polish (Poland)",
	"ps",		"Pashto",
	"ps-AR",	"Pashto (Afghanistan)",
	"pt",		"Portuguese",
	"pt-BR",	"Portuguese (Brazil)",
	"pt-PT",	"Portuguese (Portugal)",
	"qu",		"Quechua",
	"qu-BO",	"Quechua (Bolivia)",
	"qu-EC",	"Quechua (Ecuador)",
	"qu-PE",	"Quechua (Peru)",
	"ro",		"Romanian",
	"ro-RO",	"Romanian (Romania)",
	"ru",		"Russian",
	"ru-RU",	"Russian (Russia)",
	"sa",		"Sanskrit",
	"sa-IN",	"Sanskrit (India)",
	"se",		"Sami (Northern)",
	"se-FI",	"Sami (Northern) (Finland)",
	"se-FI",	"Sami (Skolt) (Finland)",
	"se-FI",	"Sami (Inari) (Finland)",
	"se-NO",	"Sami (Northern) (Norway)",
	"se-NO",	"Sami (Lule) (Norway)",
	"se-NO",	"Sami (Southern) (Norway)",
	"se-SE",	"Sami (Northern) (Sweden)",
	"se-SE",	"Sami (Lule) (Sweden)",
	"se-SE",	"Sami (Southern) (Sweden)",
	"sk",		"Slovak",
	"sk-SK",	"Slovak (Slovakia)",
	"sl",		"Slovenian",
	"sl-SI",	"Slovenian (Slovenia)",
	"sq",		"Albanian",
	"sq-AL",	"Albanian (Albania)",
	"sr-BA",	"Serbian (Latin) (Bosnia and Herzegovina)",
	"sr-BA",	"Serbian (Cyrillic) (Bosnia and Herzegovina)",
	"sr-SP",	"Serbian (Latin) (Serbia and Montenegro)",
	"sr-SP",	"Serbian (Cyrillic) (Serbia and Montenegro)",
	"sv",		"Swedish",
	"sv-FI",	"Swedish (Finland)",
	"sv-SE",	"Swedish (Sweden)",
	"sw",		"Swahili",
	"sw-KE",	"Swahili (Kenya)",
	"syr",		"Syriac",
	"syr-SY",	"Syriac (Syria)",
	"ta",		"Tamil",
	"ta-IN",	"Tamil (India)",
	"te",		"Telugu",
	"te-IN",	"Telugu (India)",
	"th",		"Thai",
	"th-TH",	"Thai (Thailand)",
	"tl",		"Tagalog",
	"tl-PH",	"Tagalog (Philippines)",
	"tn",		"Tswana",
	"tn-ZA",	"Tswana (South Africa)",
	"tr",		"Turkish",
	"tr-TR",	"Turkish (Turkey)",
	"tt",		"Tatar",
	"tt-RU",	"Tatar (Russia)",
	"ts",		"Tsonga",
	"uk",		"Ukrainian",
	"uk-UA",	"Ukrainian (Ukraine)",
	"ur",		"Urdu",
	"ur-PK",	"Urdu (Islamic Republic of Pakistan)",
	"uz",		"Uzbek (Latin)",
	"uz-UZ",	"Uzbek (Latin) (Uzbekistan)",
	"uz-UZ",	"Uzbek (Cyrillic) (Uzbekistan)",
	"vi",		"Vietnamese",
	"vi-VN",	"Vietnamese (Viet Nam)",
	"xh",		"Xhosa",
	"xh-ZA",	"Xhosa (South Africa)",
	"zh",		"Chinese",
	"zh-CN",	"Chinese (S)",
	"zh-HK",	"Chinese (Hong Kong)",
	"zh-MO",	"Chinese (Macau)",
	"zh-SG",	"Chinese (Singapore)",
	"zh-TW",	"Chinese (T)",
	"zu",		"Zulu",
	"zu-ZA",	"Zulu (South Africa)",
	""
} ;
#endif

const char*	Htmltype2Txt	(hzHtmltype ht)
{
	//	Category:	Diagnostics
	//
	//	Return const char* description of HTML type
	//
	//	Arguments:	1)	mtype	The enumerated HTML field type
	//
	//	Returns:	Pointer to HTML type description as cstr

	static const hzString	_htmltypeNULL	= "HTMLTYPE_NULL" ;			//	Invalid HTML type
	static const hzString	_htmltypeTEXT	= "HTMLTYPE_TEXT" ;			//	Text field
	static const hzString	_htmltypePASS	= "HTMLTYPE_PASSWORD" ;		//	Password field
	static const hzString	_htmltypeAREA	= "HTMLTYPE_TEXTAREA" ;		//	Text Area field
	static const hzString	_htmltypeSLCT	= "HTMLTYPE_SELECT" ;		//	Selector field
	static const hzString	_htmltypeCBOX	= "HTMLTYPE_CHECKBOX" ;		//	Checkbox field
	static const hzString	_htmltypeRBUT	= "HTMLTYPE_RADIO" ;		//	Radio button field
	static const hzString	_htmltypeFILE	= "HTMLTYPE_FILE" ;			//	File Uploader field
	static const hzString	_htmltypeHIDE	= "HTMLTYPE_HIDDEN" ;		//	Hidden field

	switch	(ht)
	{
	case HTMLTYPE_TEXT:			return *_htmltypeTEXT ;
	case HTMLTYPE_PASSWORD:		return *_htmltypePASS ;
	case HTMLTYPE_TEXTAREA:		return *_htmltypeAREA ;
	case HTMLTYPE_SELECT:		return *_htmltypeSLCT ;
	case HTMLTYPE_CHECKBOX:		return *_htmltypeCBOX ;
	case HTMLTYPE_RADIO:		return *_htmltypeRBUT ;
	case HTMLTYPE_FILE:			return *_htmltypeFILE ;
	case HTMLTYPE_HIDDEN:		return *_htmltypeHIDE ;
	}

	return *_htmltypeNULL ;
}

/*
**	String Interpretation
*/

//	FnGrp:		IsInteger
//	Category:	Text Processing
//
//	Purpose:	Determine if a token could be a valid negative or positive integer
//
//	Arguments:	1)	nVal	A reference to the result
//				2)	tok		The token
//
//	Varients	This function has four overloads in which arg1 can be either an int64_t, int32_t, int16_t or a byte
//
//	Returns:	True	If token could be a valid large integer
//				False	If string of zero length or contains non numeric chars
//
//	Errors:		Sets an E_ARGUMENT error if either first two arguments are invalid.
//
//	Func:	IsInteger(int64_t&,const char*)
//	Func:	IsInteger(int32_t&,const char*)
//	Func:	IsInteger(int16_t&,const char*)
//	Func:	IsInteger(char&,const char*)

bool	IsInteger	(int64_t& nVal, const char* tok)
{
	_hzfunc("IsInteger(int64)") ;

	const char*	i = tok ;		//	Input string iterator
	int64_t		val = 0 ;		//	Value aggregator
	uint32_t	count = 0 ;		//	Length test
	bool		bNeg = false ;	//	Negation indicator

	if (!tok || !tok[0])
		return false ;

	//	Ignore leading spaces
	for (; *i == CHAR_SPACE ; i++, count++) ;

	//	Check for minus sign
	if (*i == CHAR_MINUS)
		{ bNeg = true ; i++ ; }

	//	Check for plus sign
	if (*i == CHAR_PLUS)
	{
		if (bNeg)
			return false ;
		i++ ;
	}

	//	Remove any spaces between sign and digits
	for (; *i == CHAR_SPACE ; i++) ;

	//	Calculate value
	for (count = 0 ; count < 21 && *i ; count++, i++)
	{
		if (!(chartype[*i] & CTYPE_DIGIT))
			return false ;

		val *= 10 ;
		val += (int64_t) (*i - CHAR_0) ;
	}

	//	Check size
	if (count > 20)
		return false ;

	nVal = bNeg ? val * -1 : val ;
	return true ;
}

bool	IsInteger	(int32_t& nVal, const char* tok)
{
	_hzfunc("IsInteger(int32)") ;

	const char*	i = tok ;		//	Input string iterator
	uint32_t	val = 0 ;		//	Value aggregator
	uint32_t	count = 0 ;		//	Length test
	bool		bNeg = false ;	//	Negation indicator

	if (!tok || !tok[0])
		return false ;

	//	Ignore leading spaces
	for (; *i == CHAR_SPACE ; i++) ;

	//	Test for minus sign
	if (*i == CHAR_MINUS)
		{ bNeg = true ; i++; }

	//	Test for plus sign
	if (*i == CHAR_PLUS)
	{
		if (bNeg)
			return false ;
		i++ ;
	}

	//	Remove any spaces between sign and digits
	for (; *i == CHAR_SPACE ; i++) ;

	//	Obtain value
	for (count = 0 ; count < 11 && *i ; i++, count++)
	{
		if (!(chartype[*i] & CTYPE_DIGIT))
			return false ;

		val *= 10 ;
		val += (uint32_t) (*i - CHAR_0) ;
	}

	//	Check size
	if (count > 10)
		return false ;

	//	Apply negation
	nVal = bNeg ? val * -1 : val ;
	return true ;
}

bool	IsInteger	(int16_t& nVal, const char* tok)
{
	_hzfunc("IsInteger(int16)") ;

	const char*	i = tok ;		//	Input string iterator
	int16_t		val = 0 ;		//	Value aggregator
	int16_t		bNeg = 0 ;		//	Negator
	uint32_t	count = 0 ;		//	Length test

	if (!tok || !tok[0])
		return false ;

	//	Remove leading spaces
	for (; *i == CHAR_SPACE ; i++, count++) ;

	//	Check for minus sign
	if (*i == CHAR_MINUS)
		{ bNeg = 1 ; i++; count++ ; }

	//	Check for plus sign
	if (*i == CHAR_PLUS)
	{
		if (bNeg)
			return false ;
		i++ ;
	}

	//	Remove any spaces between sign and digits
	for (; *i == CHAR_SPACE ; i++, count++) ;

	//	Calculate value
	for (count = 0 ; count < 6 && *i ; i++, count++)
	{
		if (!(chartype[*i] & CTYPE_DIGIT))
			return false ;

		val *= 10 ;
		val += (int16_t) (*i - CHAR_0) ;
	}

	//	Check size
	if (count > 5)
		return false ;

	nVal = bNeg ? val * -1 : val ;
	return true ;
}

bool	IsInteger	(char& nVal, const char* tok)
{
	_hzfunc("IsInteger(int8)") ;

	const char*	i = tok ;		//	Input string iterator
	uint32_t	count = 0 ;		//	Length test
	char		val = 0 ;		//	Value aggregator
	char		bNeg = 0 ;		//	Negator

	if (!tok || !tok[0])
		return false ;

	//	Ignore leading spaces
	for (; *i == CHAR_SPACE ; i++) ;

	//	Test for minus sign
	if (*i == CHAR_MINUS)
		{ bNeg = 1 ; i++ ; }

	//	Test for plus sign
	if (*i == CHAR_PLUS)
	{
		if (bNeg)
			return false ;
		i++ ;
	}

	//	Ignore spaces after sign and before digits
	for (; *i == CHAR_SPACE ; i++) ;

	for (count = 0 ; count < 4 && *i ; count++, i++)
	{
		if (!(chartype[*i] & CTYPE_DIGIT))
			return false ;

		val *= 10 ;
		val += (*i - CHAR_0) ;
	}

	//	Check size
	if (count > 3)
		return false ;

	nVal = bNeg ? val * -1 : val ;
	return true ;
}

//	FnGrp:		IsPosint
//
//	Category:	Text Processing
//
//	Determine if a token could be a valid integer (token does not have to be null terminated)
//
//	Arguments:	1)	nVal	A reference to a number populated in the event the token is a number. This can be either int32_t/int64_t or uint32_t/uint64_t
//				2)	tok		The token (char*)
//
//	Returns:	True if the token is a positive integer, false otherwise.
//
//	Func:	IsPosint(int32_t&,const char*)
//	Func:	IsPosint(uint32_t&,const char*)
//	Func:	IsPosint(int64_t&,const char*)
//	Func:	IsPosint(uint64_t&,const char*)

bool	IsPosint	(uint64_t& nVal, const char* tok)
{
	_hzfunc("IsPosint(uint64)") ;

	const char*	i ;			//	Input string iterator
	uint64_t	val ;		//	Value aggregator
	uint32_t	count ;		//	Length check

	nVal = 0 ;
	i = tok ;

	if (!i || !i[0])
		return false ;

	//	cope with + and any leading whitespace
	for (; IsWhite(*i) ; i++) ;

	//	Reject on minus sign
	if (*i == CHAR_MINUS)
		return false ;

	//	Ignore plus sign
	if (*i == '+')
		for (i++ ; IsWhite(*i) ; i++) ;

	//	cope with spaces between sign and digits
	for (; IsWhite(*i) ; i++) ;

	val = 0 ;
	for (count = 0 ; count < 21 && IsDigit(*i) ; count++, i++)
	{
		val *= 10 ;
		val += (*i - CHAR_0) ;
	}

	//	Check length limit
	if (!count || count > 20)
		return false ;

	nVal = val ;
	return true ;
}

bool	IsPosint	(uint32_t& nVal, const char* tok)
{
	_hzfunc("IsPosint(uint32)") ;

	const char*	i ;			//	Input string iterator
	uint32_t	val ;		//	Value aggregator
	uint32_t	count ;		//	Length check

	nVal = 0 ;
	i = tok ;

	if (!i || !i[0])
		return false ;

	//	cope with + and any leading whitespace
	for (; IsWhite(*i) ; i++) ;

	//	Reject on minus sign
	if (*i == CHAR_MINUS)
		return false ;

	//	Ignore plus sign
	if (*i == '+')
		for (i++ ; IsWhite(*i) ; i++) ;

	//	cope with spaces between sign and digits
	for (; IsWhite(*i) ; i++) ;

	val = 0 ;
	for (count = 0 ; count < 11 && IsDigit(*i) ; count++, i++)
	{
		val *= 10 ;
		val += (*i - CHAR_0) ;
	}

	//	Check length limit
	if (!count || count > 10)
		return false ;

	nVal = val ;
	return true ;
}

bool	IsPosint	(uint16_t& nVal, const char* tok)
{
	_hzfunc("IsPosint(uint16)") ;

	const char*	i ;			//	Input string iterator
	uint16_t	val ;		//	Value aggregator
	uint16_t	count ;		//	Length check

	if (!tok || !tok[0])
		return false ;

	//	cope with + and any leading whitespace
	for (; IsWhite(*i) ; i++) ;

	//	Reject on minus sign
	if (*i == CHAR_MINUS)
		return false ;

	//	Ignore plus sign
	if (*i == '+')
		for (i++ ; IsWhite(*i) ; i++) ;

	//	cope with spaces between sign and digits
	for (; IsWhite(*i) ; i++) ;

	val = 0 ;
	for (count = 0 ; count < 6 && IsDigit(*i) ; count++, i++)
	{
		val *= 10 ;
		val += (*i - CHAR_0) ;
	}

	//	Check length limit
	if (!count || count > 5)
		return false ;

	nVal = val ;

	return true ;
}

bool	IsPosint	(uchar& nVal, const char* tok)
{
	_hzfunc("IsPosint") ;

	const char*	i ;			//	Input string iterator
	uchar		val ;		//	Value aggregator
	uchar		count ;		//	Length check

	if (!tok || !tok[0])
		return false ;

	//	cope with + and any leading whitespace
	for (; IsWhite(*i) ; i++) ;

	//	Reject on minus sign
	if (*i == CHAR_MINUS)
		return false ;

	//	Ignore plus sign
	if (*i == '+')
		for (i++ ; IsWhite(*i) ; i++) ;

	//	cope with spaces between sign and digits
	for (; IsWhite(*i) ; i++) ;

	val = 0 ;
	for (count = 0 ; count < 4 && IsDigit(*i) ; count++, i++)
	{
		val *= 10 ;
		val += (*i - CHAR_0) ;
	}

	//	Check length limit
	if (!count || count > 3)
		return false ;

	nVal = val ;

	return true ;
}

//	FnGrp:		IsHexnum
//	Category:	Text Processing
//
//	Purpose:	Determine if a token could represent a integer in hexadecimal form.
//
//	Arguments:	1)	nVal	A reference to the result - this may be a char/uchar or a 16, 32 or 64 bit signed or unsigened integer.
//				2)	tok		A char pointer assumed to be the start of the token
//
//	Varients	This function has eight overloads in which arg1 can be either an int64_t, int32_t, int16_t or a byte or the unsigned of these.
//
//	Returns:	True	If token could be a valid hexadecimal number
//				False	Otherwise
//
//	Errors:		Sets an E_ARGUMENT error if either first two arguments are invalid.
//
//	Func:	IsHexnum(uchar&,const char*)
//	Func:	IsHexnum(uint16_t&,const char*)
//	Func:	IsHexnum(uint32_t&,const char*)
//	Func:	IsHexnum(uint64_t&,const char*)

bool	IsHexnum	(uchar& nVal, const char* tok)
{
	_hzfunc("IsHexnum(uchar)") ;

	const char*	i = tok ;	//	String iterator
	uint32_t	c = 0 ;		//	String counter
	uchar		v = 0 ;		//	Target value

	nVal = 0 ;

	if (!tok || !tok[0])
		return false ;

	//	Allow for an '0x' to preceed the number
	if (i[0] == '0' && (i[1] == 'x' || i[1] == 'X'))
		i += 2 ;

	for (; IsHex(i[c]) ; c++)
	{
		v *= 16 ;

		if (i[c] >= '0' && i[c] <= '9') { v += (i[c] - '0') ; continue ; }
		if (i[c] >= 'A' && i[c] <= 'F') { v += 10 ; v += (i[c] - 'A') ; continue ; }
		if (i[c] >= 'a' && i[c] <= 'f') { v += 10 ; v += (i[c] - 'a') ; continue ; }

		return false ;
	}

	//	Check length not exceeded
	if (!c || c > 2)
		return false ;

	nVal = v ;
	return true ;
}

bool	IsHexnum	(uint16_t& nVal, const char* tok)
{
	_hzfunc("IsHexnum(uint16_t)") ;

	const char*	i = tok ;	//	String iterator
	uint32_t	c = 0 ;		//	String counter
	uint16_t	v = 0 ;		//	Target value

	nVal = 0 ;

	if (!tok || !tok[0])
		return false ;

	//	Allow for an '0x' to preceed the number
	if (i[0] == '0' && (i[1] == 'x' || i[1] == 'X'))
		i += 2 ;

	for (; IsHex(i[c]) ; c++)
	{
		v *= 16 ;

		if (i[c] >= '0' && i[c] <= '9') { v += (i[c] - '0') ; continue ; }
		if (i[c] >= 'A' && i[c] <= 'F') { v += 10 ; v += (i[c] - 'A') ; continue ; }
		if (i[c] >= 'a' && i[c] <= 'f') { v += 10 ; v += (i[c] - 'a') ; continue ; }

		return false ;
	}

	//	Check length not exceeded
	if (!c || c > 4)
		return false ;

	nVal = v ;
	return true ;
}

bool	IsHexnum	(uint32_t& nVal, const char* tok)
{
	_hzfunc("IsHexnum(uint32_t)") ;

	const char*	i = tok ;	//	String iterator
	uint32_t	c = 0 ;		//	String counter
	uint32_t	v = 0 ;		//	Target value

	nVal = 0 ;

	if (!tok || !tok[0])
		return false ;

	//	Allow for an '0x' to preceed the number
	if (i[0] == '0' && (i[1] == 'x' || i[1] == 'X'))
		i += 2 ;

	for (; IsHex(i[c]) ; c++)
	{
		v *= 16 ;

		if (i[c] >= '0' && i[c] <= '9') { v += (i[c] - '0') ; continue ; }
		if (i[c] >= 'A' && i[c] <= 'F') { v += 10 ; v += (i[c] - 'A') ; continue ; }
		if (i[c] >= 'a' && i[c] <= 'f') { v += 10 ; v += (i[c] - 'a') ; continue ; }

		return false ;
	}

	//	Check length not exceeded
	if (!c || c > 8)
		return false ;

	nVal = v ;
	return true ;
}

bool	IsHexnum	(uint64_t& nVal, const char* tok)
{
	_hzfunc("IsHexnum(uint64_t)") ;

	const char*	i = tok ;	//	String iterator
	uint64_t	v = 0 ;		//	Target value
	uint32_t	c = 0 ;		//	String counter

	nVal = 0 ;

	if (!tok || !tok[0])
		return false ;

	//	Allow for an '0x' to preceed the number
	if (i[0] == '0' && (i[1] == 'x' || i[1] == 'X'))
		i += 2 ;

	for (; IsHex(i[c]) ; c++)
	{
		v *= 16 ;

		if (i[c] >= '0' && i[c] <= '9') { v += (i[c] - '0') ; continue ; }
		if (i[c] >= 'A' && i[c] <= 'F') { v += 10 ; v += (i[c] - 'A') ; continue ; }
		if (i[c] >= 'a' && i[c] <= 'f') { v += 10 ; v += (i[c] - 'a') ; continue ; }

		return false ;
	}

	//	Check length not exceeded
	if (!c || c > 16)
		return false ;

	nVal = v ;
	return true ;
}

bool	IsDouble	(double& nVal, const char* tok)
{
	//	Category:	Text Processing
	//
	//	Determine if a token could represent a floating point number (double). To qualify, the token must be either:-
	//
	//		a) A series of one or more digits.
	//		b) (a) followed by a single period followed directly by another series of digits.
	//		c) (b) followed by a '*10^' followed by another series of digits (standard form number).
	//
	//	All of the above must be followed by a 'valid terminator sequence'. This could be a null terminator but may be a punctuating char as long as that we not
	//	followed by a number.
	//
	//	Arguments:	1)	A pointer to the result (double)
	//				2)	The token
	//
	//	Returns:	True	If token could be a value of type double
	//				False	If string of zero length or contains non numeric chars
	//
	//	Errors:		Sets an E_ARGUMENT error if either first two arguments are invalid.

	_hzfunc("IsDouble") ;

	const char*	i = tok ;		//	Input string iterator
	double		val = 0.0 ;		//	Agregated value
	double		pt_pos = 0.0 ;	//	Order of magnitude	
	bool		bNeg = false ;	//	Negator

	if (!tok || !tok[0])
		return false ;

	//	Ignore leading spaces
	for (i = tok ; *i == CHAR_SPACE ; i++) ;

	//	Ceck for minus sign
	if (*i == CHAR_MINUS)
		{ bNeg = true ; i++ ; }

	//	Ceck for plus sign
	if (*i == CHAR_PLUS)
	{
		if (bNeg)
			return false ;
		i++ ;
	}

	//	Ignore spaces between sign and digits
	for (; *i == CHAR_SPACE ; i++) ;

	for (; *i ; i++)
	{
		if (!(chartype[*i] & CTYPE_DIGIT))
		{
			if (*i != CHAR_PERIOD)
				return false ;
			if (pt_pos)
				return false ;
			pt_pos = 1.0 ;
		}

		val *= 10.0 ;
		val += (double) (*i - CHAR_0) ;
		pt_pos *= 10.0 ;
	}

	nVal = (val / pt_pos) ;
	if (bNeg)
		nVal *= -1.0 ;
	return true ;
}

bool	IsAlphaNum	(const char* tok)
{
	//	Category:	Text Processing
	//
	//	Determine if a token is alphanumeric
	//
	//	Arguments:	1)	tok	The token
	//
	//	Returns:	True	If token is alphanemeric
	//				False	If string of zero length or contains non alphanumeric chars

	_hzfunc("IsAlphaNum") ;

	const char*	i ;		//	Input string iterator

	if (!tok || !tok[0])
		return false ;

	for (i = tok ; *i ; i++)
	{
		if (chartype[*i] & (CTYPE_ALPHA | CTYPE_DIGIT))
			continue ;
		return false ;
	}

	return true ;
}

bool	IsAllDigits	(const char* tok)
{
	//	Category:	Text Processing
	//
	//	Determine if a string consists entirely of digits
	//
	//	Arguments:	1)	tok	The token
	//
	//	Returns:	True	If token is alphanemeric
	//				False	If string of zero length or contains non digit chars

	const char*	i ;		//	Input string iterator

	if (!tok || !tok[0])
		return false ;

	for (i = tok ; *i && *i >= '0' && *i <= '9' ; i++) ;
	return *i ? false : true ;
}
