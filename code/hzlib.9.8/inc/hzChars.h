//
//	File:	hzChars.h
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
//	See synopsis 	Treatment of Character Types
//
//	The characters of the ASCII table fall into distinct groups which are important for processing of text. The most ovious of these are:-
//
//	1)	Whitespace	(eg SPACE, TAB, CR and NL)
//	2)	Digits		(0-9)
//	3)	Alpha		(a-z and A-Z)
//	4)	Punctuation	(eg period, comma, colon, semi-colon, apostrophy)
//	5)	Symbols		(eg dollar and pound sign)
//	6)	Binary		(unprintable characters)
//
//	To cope with processing of such things as URLs, email addresses and standard form numbers, additional groups were introduced as follows:-
//
//	7)	Alphanum	Either a digit or an alpha
//	8)	Hex			Allowed chars in a hexadecimal integer (Either 0-9 or a-f or A-F)
//	9)	Numchar		Chars used in standard form number (0-9, period, hat)
//	10)	Hyphen		Chars that can serve as hyphens
//	11)	Url			Chars allowed in a URL
//
//	HadronZoo provides a global array of character types indexed by the ASCII value of the characters. This is called chartype although this is
//	rarely referenced by applications. Instead applications generally use the inline functions with prefixes of 'is' (followed by char type) or
//	'Is' (for chars as int32_t).

#ifndef hzChars_h
#define hzChars_h

#include "hzBasedefs.h"

#define CHAR_NULL		(char)  0		//	Null terminator
#define CHAR_CTRLA		(char)  1		//	Control-A
#define CHAR_SOH		(char)  1		//	Start of Heading
#define CHAR_CTRLB		(char)  2		//	Control-B
#define CHAR_STX		(char)  2		//	Start of Text
#define CHAR_CTRLC		(char)  3		//	Control-C
#define CHAR_ETX		(char)  3		//	End of Text
#define CHAR_CTRLD		(char)  4		//	Control-D
#define CHAR_EOT		(char)  4		//	End of Transmission
#define CHAR_CTRLE		(char)  5		//	Control-E
#define CHAR_ENQ		(char)  5		//	Enquiry
#define CHAR_CTRLF		(char)  6		//	Control-F
#define CHAR_ACK		(char)  6		//	Acknowledgement
#define CHAR_CTRLG		(char)  7		//	Control-G
#define CHAR_BEL		(char)  7		//	Bell
#define CHAR_CTRLH		(char)  8		//	Control-H
#define CHAR_BS			(char)  8		//	Backspace[e][f]
#define CHAR_CTRLI		(char)  9		//	Control-I
#define CHAR_TAB		(char)  9		//	Tab
#define CHAR_CTRLJ		(char) 10		//	Control-J
#define CHAR_NL			(char) 10		//	Newline
#define NEWLINE			(char) 10		//	Newline
#define CHAR_CTRLK		(char) 11		//	Control-K
#define CHAR_VT			(char) 11		//	Vertical Tab
#define CHAR_CTRLL		(char) 12		//	Control-L
#define CHAR_FF			(char) 12		//	Form Feed
#define CHAR_CTRLM		(char) 13		//	Control-M
#define CHAR_CR			(char) 13		//	Carriage return
#define CHAR_CTRLN		(char) 14		//	Control-N
#define CHAR_SO			(char) 14		//	Shift Out
#define CHAR_CTRLO		(char) 15		//	Control-O
#define CHAR_SI			(char) 15		//	Shift In
#define CHAR_CTRLP		(char) 16		//	Control-P
#define CHAR_DLE		(char) 16		//	Data Link Escape
#define CHAR_CTRLQ		(char) 17		//	Control-Q
#define CHAR_DC1		(char) 17		//	Device Control 1
#define CHAR_XON		(char) 17		//	XON (Device Control 1)
#define CHAR_CTRLR		(char) 18		//	Control-R
#define CHAR_DC2		(char) 18		//	Device Control 2
#define CHAR_CTRLS		(char) 19		//	Control-S
#define CHAR_DC3		(char) 19		//	Device Control 3
#define CHAR_XOFF		(char) 19		//	XOFF (Device Control 3)
#define CHAR_CTRLT		(char) 20		//	Control-T
#define CHAR_DC4		(char) 20		//	Device Control 4
#define CHAR_CTRLU		(char) 21		//	Control-U
#define CHAR_NAK		(char) 21		//	Negative Acknowledgement
#define CHAR_CTRLV		(char) 22		//	Control-V
#define CHAR_SYN		(char) 22		//	Synchronous Idle
#define CHAR_CTRLW		(char) 23		//	Control-W
#define CHAR_ETB		(char) 23		//	End of Transmission Block
#define CHAR_CTRLX		(char) 24		//	Control-X
#define CHAR_CAN		(char) 24		//	Cancel
#define CHAR_CTRLY		(char) 25		//	Control-Y
#define CHAR_EM			(char) 25		//	End of Medium
#define CHAR_CTRLZ		(char) 26		//	Control-Z
#define CHAR_SUB		(char) 26		//	Substitute
#define CHAR_ESC		(char) 27		//	Escape[j]
#define CHAR_FS			(char) 28		//	File Separator
#define CHAR_GS			(char) 29		//	Group Separator
#define CHAR_RS			(char) 30		//	Record Separator
#define CHAR_US			(char) 31		//	Unit Separator
#define CHAR_SPACE		(char) 32		//	Space
#define CHAR_PLING		(char) 33		//	Logicl NOT symbol
#define CHAR_DQUOTE		(char) 34		//	Double quotation mark
#define CHAR_HASH		(char) 35		//	Hash symbol
#define CHAR_DOLLAR		(char) 36		//	Dollar sign
#define CHAR_PERCENT	(char) 37		//	Percent sign
#define CHAR_AMPSAND	(char) 38		//	Ampersand
#define CHAR_SQUOTE		(char) 39		//	Single quotation mark
#define CHAR_PAROPEN	(char) 40		//	Opening round brace
#define CHAR_PARCLOSE	(char) 41		//	Closing round brace
#define CHAR_ASTERISK	(char) 42		//	Asterisk/Multiply sign
#define CHAR_PLUS		(char) 43		//	Plus sign
#define CHAR_COMMA		(char) 44		//	Comma
#define CHAR_MINUS		(char) 45		//	Minus sign
#define CHAR_PERIOD		(char) 46		//	Period symbol
#define CHAR_FWSLASH	(char) 47		//	Forward slash
#define CHAR_0			(char) 48		//	Digit 0
#define CHAR_1			(char) 49		//	Digit 1
#define CHAR_2			(char) 50		//	Digit 2
#define CHAR_3			(char) 51		//	Digit 3
#define CHAR_4			(char) 52		//	Digit 4
#define CHAR_5			(char) 53		//	Digit 5
#define CHAR_6			(char) 54		//	Digit 6
#define CHAR_7			(char) 55		//	Digit 7
#define CHAR_8			(char) 56		//	Digit 8
#define CHAR_9			(char) 57		//	Digit 9
#define CHAR_COLON		(char) 58		//	Colon
#define CHAR_SCOLON		(char) 59		//	Semi-colon
#define CHAR_LESS		(char) 60		//	Less-than symbol
#define CHAR_EQUAL		(char) 61		//	Equality symbol
#define CHAR_MORE		(char) 62		//	Greater-than symbol
#define CHAR_QUERY		(char) 63		//	Question mark
#define CHAR_AT			(char) 64		//	The @ char
#define CHAR_UC_A		(char) 65		//	Upper case a
#define CHAR_UC_B		(char) 66		//	Upper case b
#define CHAR_UC_C		(char) 67		//	Upper case c
#define CHAR_UC_D		(char) 68		//	Upper case d
#define CHAR_UC_E		(char) 69		//	Upper case e
#define CHAR_UC_F		(char) 70		//	Upper case f
#define CHAR_UC_G		(char) 71		//	Upper case g
#define CHAR_UC_H		(char) 72		//	Upper case h
#define CHAR_UC_I		(char) 73		//	Upper case i
#define CHAR_UC_J		(char) 74		//	Upper case j
#define CHAR_UC_K		(char) 75		//	Upper case k
#define CHAR_UC_L		(char) 76		//	Upper case l
#define CHAR_UC_M		(char) 77		//	Upper case m
#define CHAR_UC_N		(char) 78		//	Upper case n
#define CHAR_UC_O		(char) 79		//	Upper case o
#define CHAR_UC_P		(char) 80		//	Upper case p
#define CHAR_UC_Q		(char) 81		//	Upper case q
#define CHAR_UC_R		(char) 82		//	Upper case r
#define CHAR_UC_S		(char) 83		//	Upper case s
#define CHAR_UC_T		(char) 84		//	Upper case t
#define CHAR_UC_U		(char) 85		//	Upper case u
#define CHAR_UC_V		(char) 86		//	Upper case v
#define CHAR_UC_W		(char) 87		//	Upper case w
#define CHAR_UC_X		(char) 88		//	Upper case x
#define CHAR_UC_Y		(char) 89		//	Upper case y
#define CHAR_UC_Z		(char) 90		//	Upper case z
#define CHAR_SQOPEN		(char) 91		//	Opening square brace
#define CHAR_BKSLASH	(char) 92		//	Back slash '\'
#define CHAR_SQCLOSE	(char) 93		//	Closing square brace
#define CHAR_HAT		(char) 94		//	Hat char (to power of)
#define CHAR_USCORE		(char) 95		//	Underscore
#define CHAR_BKQUOTE	(char) 96		//	Back quote (Opening single quote)
#define CHAR_LC_A		(char) 97		//	Lower case a
#define CHAR_LC_B		(char) 98		//	Lower case b
#define CHAR_LC_C		(char) 99		//	Lower case c
#define CHAR_LC_D		(char)100		//	Lower case d
#define CHAR_LC_E		(char)101		//	Lower case e
#define CHAR_LC_F		(char)102		//	Lower case f
#define CHAR_LC_G		(char)103		//	Lower case g
#define CHAR_LC_H		(char)104		//	Lower case h
#define CHAR_LC_I		(char)105		//	Lower case i
#define CHAR_LC_J		(char)106		//	Lower case j
#define CHAR_LC_K		(char)107		//	Lower case k
#define CHAR_LC_L		(char)108		//	Lower case l
#define CHAR_LC_M		(char)109		//	Lower case m
#define CHAR_LC_N		(char)110		//	Lower case n
#define CHAR_LC_O		(char)111		//	Lower case o
#define CHAR_LC_P		(char)112		//	Lower case p
#define CHAR_LC_Q		(char)113		//	Lower case q
#define CHAR_LC_R		(char)114		//	Lower case r
#define CHAR_LC_S		(char)115		//	Lower case s
#define CHAR_LC_T		(char)116		//	Lower case t
#define CHAR_LC_U		(char)117		//	Lower case u
#define CHAR_LC_V		(char)118		//	Lower case v
#define CHAR_LC_W		(char)119		//	Lower case w
#define CHAR_LC_X		(char)120		//	Lower case x
#define CHAR_LC_Y		(char)121		//	Lower case y
#define CHAR_LC_Z		(char)122		//	Lower case z
#define CHAR_CUROPEN	(char)123		//	Opening curly brace
#define CHAR_OR			(char)124		//	Vertical bar (logical OR symbol)
#define CHAR_CURCLOSE	(char)125		//	Closing curly brace
#define CHAR_TILDA		(char)126		//	Tilda
#define CHAR_DEL		(char)127		//	Delete
#define CHAR_BLOCK		(char)127		//	Block char

/*
**	Char Types
*/

#define	CTYPE_NOTYPE	0x0000		//	undefine char type
#define CTYPE_BINARY	0x0001		//	unprintable, ctl etc
#define CTYPE_WHITE		0x0002		//	space, tab or newline
#define CTYPE_DIGIT		0x0004		//	0 - 9 only
#define CTYPE_HEXDIGIT	0x0008		//	0 - 9 and (a - f and A - F)
#define CTYPE_ALPHA		0x0010		//	a - z and A - Z
#define CTYPE_HYPEN		0x0020		//	any symbol used as hyphen
#define CTYPE_PUNCT		0x0040		//	any punctuation char
#define	CTYPE_SYMB		0x0080		//	symbols, eg math operators
#define CTYPE_NUMCHAR	0x0100		//	any char used in a standard form number
#define CTYPE_URL_NORM	0x0200		//	Allowed in a URL (unreserved)
#define CTYPE_URL_RESV	0x0400		//	Allowed in a URL (reserved)
#define CTYPE_HTX_TAG	0x0800		//	Any char allowed as a html/xml tag name

extern	int16_t	chartype[256] ;		//	Character types by value

/*
**	Prototypes
*/

//	Char type functions
bool	IsBinary	(int32_t c) ;	//	Tests if the char is non-printable
bool	IsWhite		(int32_t c) ;	//	Tests if the char is whitespace
bool	IsDigit		(int32_t c) ;	//	Tests if the char is 0-9
bool	IsHex		(int32_t c) ;	//	Tests if the char is 0-9, a-f, A-F
bool	IsAlpha		(int32_t c) ;	//	Tests if the char is a-z or A-Z
bool	IsHyphen	(int32_t c) ;	//	Tests if the char is a hyphen
bool	IsAlphanum	(int32_t c) ;	//	Tests if the char is a-z, A-Z or 0-9
bool	IsPunct		(int32_t c) ;	//	Tests if the char is a punctuation char
bool	IsSymb		(int32_t c) ;	//	Tests if the char is a symbol (eg $)
bool	IsNumchar	(int32_t c) ;	//	Tests if the char is allowed in a standard form number. So 0-9, period, 'e', +, -
bool	IsUrlnorm	(int32_t c) ;	//	Tests if the char is allowed as part of a base URL (subdomain.domain/)
bool	IsUrlresv	(int32_t c) ;	//	Tests if the char is allowed as part of an extended URL (subdomain.domain/url-extn)
bool	IsTagchar	(int32_t c) ;	//	Tests if the char is allowed as part of a tag-name

//	Endian conversion
void	_getv1byte	(uint32_t& v, const uchar* i) ;
void	_getv2byte	(uint32_t& v, const uchar* i) ;
void	_getv3byte	(uint32_t& v, const uchar* i) ;
void	_getv4byte	(uint32_t& v, const uchar* i) ;
void	_getv5byte	(uint64_t& v, const uchar* i) ;
void	_getv6byte	(uint64_t& v, const uchar* i) ;
void	_getv7byte	(uint64_t& v, const uchar* i) ;
void	_getv8byte	(uint64_t& v, const uchar* i) ;

void	_setv1byte	(uchar* s, char n) ;
void	_setv2byte	(uchar* s, uint16_t n) ;
void	_setv2byte	(uchar* s, int16_t n) ;
void	_setv3byte	(uchar* s, uint32_t n) ;
void	_setv3byte	(uchar* s, int32_t n) ;
void	_setv4byte	(uchar* s, uint32_t n) ;
void	_setv4byte	(uchar* s, int32_t n) ;
void	_setv5byte	(uchar* s, uint64_t n) ;
void	_setv5byte	(uchar* s, int64_t n) ;
void	_setv6byte	(uchar* s, uint64_t n) ;
void	_setv6byte	(uchar* s, int64_t n) ;
void	_setv7byte	(uchar* s, uint64_t n) ;
void	_setv7byte	(uchar* s, int64_t n) ;
void	_setv8byte	(uchar* s, uint64_t n) ;
void	_setv8byte	(uchar* s, int64_t n) ;

#endif	//	hzChars_h
