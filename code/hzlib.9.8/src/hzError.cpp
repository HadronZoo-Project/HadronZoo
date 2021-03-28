//
//	File:	hzError.cpp
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
//	Error reporting and logging functions.
//

#include <cstdarg>
#include <iostream>

#include <unistd.h>
#include <sys/sem.h>
#include <errno.h>
#include <execinfo.h>

#include "hzCtmpls.h"
#include "hzChars.h"
#include "hzErrcode.h"
#include "hzProcess.h"

/*
**	General system errors
*/

#define HZERR_OK			0			//	Everything is hunky dory
#define HZERR_ARGUMENT		1001		//	Bad or missing argument
#define HZERR_TYPE			1002		//	Data Type not valid or not appropriate in this context
#define HZERR_FORMAT		1003		//	Incorrect data or string format
#define HZERR_RANGE			1004		//	Value out of range
#define HZERR_BADVALUE		1005		//	Illegal value for type

global	const hzEcode	E_OK			(HZERR_OK) ;		//	Everything is hunky dory
global	const hzEcode	E_ARGUMENT		(HZERR_ARGUMENT) ;	//	Bad argument
global	const hzEcode	E_TYPE			(HZERR_TYPE) ;		//	Data Type not valid or not appropriate in this context
global	const hzEcode	E_FORMAT		(HZERR_FORMAT) ;	//	Incorrect data or string format
global	const hzEcode	E_RANGE			(HZERR_RANGE) ;		//	Value out of range
global	const hzEcode	E_BADVALUE		(HZERR_BADVALUE) ;	//	Illegal value for type

static	const char*	hzerr_OK		= "0000 E_OK" ;
static	const char*	hzerr_ARGUMENT	= "1001 E_ARGUMENT" ;
static	const char*	hzerr_TYPE		= "1002 E_TYPE" ;
static	const char*	hzerr_FORMAT	= "1003 E_FORMAT" ;
static	const char*	hzerr_RANGE		= "1004 E_RANGE" ;
static	const char*	hzerr_BADVALUE	= "1005 E_BADVALUE" ;

/*
**	Confilcts when setting values
*/

#define HZERR_INITFAIL		1011		//	Initialization failed
#define HZERR_INITDUP		1012		//	Resource already initialized
#define HZERR_NOINIT		1013		//	Resource not initialized
#define HZERR_SETONCE		1014		//	A variable that can only be set once, has already been set
#define HZERR_EXCLUDE		1015		//	Exclusive resource already in use
#define HZERR_CONFLICT		1016		//	Attempt to define object in doublespeak
#define HZERR_SEQUENCE		1017		//	Call out of sequence

global	const hzEcode	E_INITFAIL		(HZERR_INITFAIL) ;	//	Initialization failed
global	const hzEcode	E_INITDUP		(HZERR_INITDUP) ;	//	Resource already initialized
global	const hzEcode	E_NOINIT		(HZERR_NOINIT) ;	//	Resource not initialized
global	const hzEcode	E_SETONCE		(HZERR_SETONCE) ;	//	A variable that can only be set once, has already been set
global	const hzEcode	E_EXCLUDE		(HZERR_EXCLUDE) ;	//	Exclusive resource already in use
global	const hzEcode	E_CONFLICT		(HZERR_CONFLICT) ;	//	Attempt to define object in doublespeak
global	const hzEcode	E_SEQUENCE		(HZERR_SEQUENCE) ;	//	Call out of sequence

static	const char*	hzerr_INITFAIL	= "1011 E_INITFAI" ;
static	const char*	hzerr_INITDUP	= "1012 E_INITDUP" ;
static	const char*	hzerr_NOINIT	= "1013 E_NOINIT" ;
static	const char*	hzerr_SETONCE	= "1014 E_SETONCE" ;
static	const char*	hzerr_EXCLUDE	= "1015 E_EXCLUDE" ;
static	const char*	hzerr_CONFLICT	= "1016 E_CONFLICT" ;
static	const char*	hzerr_SEQUENCE	= "1017 E_SEQUENCE" ;

/*
**	Database errors
*/

#define HZERR_DUPLICATE		1021		//	Illegal duplicate or redefinition
#define HZERR_NODATA		1022		//	No data or empty file
#define HZERR_NOTFOUND		1023		//	Item not found or search criteria resulted in 0 rows
#define HZERR_INSERT		1024		//	Could not insert data
#define HZERR_DELETE		1025		//	Could not delete data
#define HZERR_CORRUPT		1026		//	Database, index or tree is corrupted

global	const hzEcode	E_DUPLICATE		(HZERR_DUPLICATE) ;	//	Illegal duplicate or redefinition
global	const hzEcode	E_NODATA		(HZERR_NODATA) ;	//	No data or empty file
global	const hzEcode	E_NOTFOUND		(HZERR_NOTFOUND) ;	//	Item not found or search criteria resulted in 0 rows
global	const hzEcode	E_INSERT		(HZERR_INSERT) ;	//	Could not insert data
global	const hzEcode	E_DELETE		(HZERR_DELETE) ;	//	Could not delete data
global	const hzEcode	E_CORRUPT		(HZERR_CORRUPT) ;	//	Database, index or tree is corrupted

static	const char*	hzerr_DUPLICATE	= "1021 E_DUPLICATE" ;
static	const char*	hzerr_NODATA	= "1022 E_NODATA" ;
static	const char*	hzerr_NOTFOUND	= "1023 E_NOTFOUND" ;
static	const char*	hzerr_INSERT	= "1024 E_INSERT" ;
static	const char*	hzerr_DELETE	= "1025 E_DELETE" ;
static	const char*	hzerr_CORRUPT	= "1026 E_CORRUPT" ;

/*
**		Expressions
*/

#define HZERR_PARSE			1031		//	Cannot parse expression
#define HZERR_SYNTAX		1032		//	Syntax error in expression

global	const hzEcode	E_PARSE			(HZERR_PARSE) ;		//	Cannot parse expression
global	const hzEcode	E_SYNTAX		(HZERR_SYNTAX) ;	//	Syntax error in expression

static	const char*	hzerr_PARSE		= "1031 E_PARSE" ;
static	const char*	hzerr_SYNTAX	= "1032 E_SYNTAX" ;

/*
**	System resources
*/

#define HZERR_NOCREATE		1041		//	Failed to create a resorce (could be file or other data source)
#define HZERR_OPENFAIL		1042		//	Failed to open a resorce (could be file or other data source)
#define HZERR_NOTOPEN		1043		//	Resource is not open
#define HZERR_READFAIL		1044		//	Read operation failed
#define HZERR_WRITEFAIL		1045		//	Write operation failed
#define HZERR_MEMORY		1046		//	Out of memory
#define HZERR_OVERFLOW		1047		//	Buffer overflowed

global	const hzEcode	E_NOCREATE		(HZERR_NOCREATE) ;	//	Failed to create a resorce = could be file or other data source
global	const hzEcode	E_OPENFAIL		(HZERR_OPENFAIL) ;	//	Failed to open a resorce = could be file or other data source
global	const hzEcode	E_NOTOPEN		(HZERR_NOTOPEN) ;	//	Resource is not open
global	const hzEcode	E_READFAIL		(HZERR_READFAIL) ;	//	Read operation failed
global	const hzEcode	E_WRITEFAIL		(HZERR_WRITEFAIL) ;	//	Write operation failed
global	const hzEcode	E_MEMORY		(HZERR_MEMORY) ;	//	Out of memory
global	const hzEcode	E_OVERFLOW		(HZERR_OVERFLOW) ;	//	Buffer overflowed

static	const char*	hzerr_NOCREATE	= "1041 E_NOCREATE" ;
static	const char*	hzerr_OPENFAIL	= "1042 E_OPENFAIL" ;
static	const char*	hzerr_NOTOPEN	= "1043 E_NOTOPEN" ;
static	const char*	hzerr_READFAIL	= "1044 E_READFAIL" ;
static	const char*	hzerr_WRITEFAIL	= "1045 E_WRITEFAIL" ;
static	const char*	hzerr_MEMORY	= "1046 E_MEMORY" ;
static	const char*	hzerr_OVERFLOW	= "1047 E_OVERFLOW" ;

/*
**	DNS errors
*/

#define HZERR_DNS_NOHOST	1051		//	The domain exists but no data was obtained
#define HZERR_DNS_NODATA	1052		//	The domain exists but no data was obtained
#define HZERR_DNS_RETRY		1053		//	End this server session and try again int16_tly
#define HZERR_DNS_FAILED	1054		//	Something terrible is wrong with the DNS

global	const hzEcode	E_DNS_NOHOST	(HZERR_DNS_NOHOST) ;	//  The domain does not exist
global	const hzEcode	E_DNS_NODATA	(HZERR_DNS_NODATA) ;	//  The domain exists but no data obtained
global	const hzEcode	E_DNS_RETRY		(HZERR_DNS_RETRY) ;		//  Timeout, please retry
global	const hzEcode	E_DNS_FAILED	(HZERR_DNS_FAILED) ;	//  DNS failed

static	const char*	hzerr_DNS_NOHOST	= "1051 E_DNS_NOHOST" ;
static	const char*	hzerr_DNS_NODATA	= "1052 E_DNS_NODATA" ;
static	const char*	hzerr_DNS_RETRY		= "1053 E_DNS_RETRY" ;
static	const char*	hzerr_DNS_FAILED	= "1054 E_DNS_FAILED" ;

/*
**	Comms
*/

#define HZERR_NOSOCKET		1061		//	Cannot get a TCP/IP socket
#define HZERR_HOSTFAIL		1062		//	Cannot get host by name
#define HZERR_HOSTRETRY		1063		//	Host server accepts connections, states it is busy and closes connection
#define HZERR_SENDFAIL		1064		//	Cannot send to host (socket error)
#define HZERR_RECVFAIL		1065		//	Cannot recv from host (socket error)
#define HZERR_PROTOCOL		1066		//	Client/Server protocol violation
#define HZERR_TIMEOUT		1067		//	Operation timed out
#define HZERR_RELEASE		1068		//	Failed to release a resorce
#define HZERR_SHUTDOWN		1069		//	Error occured during shutdown

global	const hzEcode	E_NOSOCKET		(HZERR_NOSOCKET) ;		//	Cannot get a TCP/IP socket
global	const hzEcode	E_HOSTFAIL		(HZERR_HOSTFAIL) ;		//	Host server offline
global	const hzEcode	E_HOSTRETRY		(HZERR_HOSTRETRY) ;		//	Host server is up, accepts clients but cuts them off with too busy message
global	const hzEcode	E_SENDFAIL		(HZERR_SENDFAIL) ;		//	Cannot connect to host, server maybe down
global	const hzEcode	E_RECVFAIL		(HZERR_RECVFAIL) ;		//	Cannot connect to host, server maybe down
global	const hzEcode	E_PROTOCOL		(HZERR_PROTOCOL) ;		//	Client/Server protocol violation
global	const hzEcode	E_TIMEOUT		(HZERR_TIMEOUT) ;		//	Operation timed out
global	const hzEcode	E_RELEASE		(HZERR_RELEASE) ;		//	Failed to release a resorce
global	const hzEcode	E_SHUTDOWN		(HZERR_SHUTDOWN) ;		//	Error occured during shutdown

static	const char*	hzerr_NOSOCKET	= "1061 E_NOSOCKET" ;
static	const char*	hzerr_HOSTFAIL	= "1062 E_HOSTFAIL" ;
static	const char*	hzerr_HOSTRETRY	= "1063 E_HOSTRETRY" ;
static	const char*	hzerr_SENDFAIL	= "1064 E_SENDFAIL" ;
static	const char*	hzerr_RECVFAIL	= "1065 E_RECVFAIL" ;
static	const char*	hzerr_PROTOCOL	= "1066 E_PROTOCOL" ;
static	const char*	hzerr_TIMEOUT	= "1067 E_TIMEOUT" ;
static	const char*	hzerr_RELEASE	= "1068 E_RELEASE" ;
static	const char*	hzerr_SHUTDOWN	= "1069 E_SHUTDOWN" ;

/*
**	Email errors
*/

#define HZERR_BADSENDER		1071		//	No sender email addr
#define HZERR_BADADDRESS	1072		//	Badly formed address
#define HZERR_NOACCOUNT		1073		//	A Mail server has denied the existence of the account
#define HZERR_SYSERROR		1074		//	System command error

global	const hzEcode	E_BADSENDER		(HZERR_BADSENDER) ;		//	No sender email addr
global	const hzEcode	E_BADADDRESS	(HZERR_BADADDRESS) ;	//	Badly formed address
global	const hzEcode	E_NOACCOUNT		(HZERR_NOACCOUNT) ;		//	A Mail server has denied the existence of the account
global	const hzEcode	E_SYSERROR		(HZERR_SYSERROR) ;		//	System command error

static	const char*	hzerr_BADSENDER		= "1071 E_BADSENDER" ;
static	const char*	hzerr_BADADDRESS	= "1072 E_BADADDRESS" ;
static	const char*	hzerr_NOACCOUNT		= "1073 E_NOACCOUNT" ;
static	const char*	hzerr_SYSERROR		= "1074 E_SYSERROR" ;

/*
**	Functions
*/

const char*	Err2Txt	(hzEcode errCode)
{
	//	Category:	Diagnostics
	//
	//	Converts a hzEcode value to it's text description.
	//
	//	Arguments:	1)	errCode	The error code
	//
	//	Returns:	Pointer (cstr) being description of supplied error code

	_hzfunc(__func__) ;

	switch	(errCode.Value())
	{
	case HZERR_OK:			return hzerr_OK ;

	case HZERR_ARGUMENT:	return hzerr_ARGUMENT ;
	case HZERR_TYPE:		return hzerr_TYPE ;
	case HZERR_FORMAT:		return hzerr_FORMAT ;
	case HZERR_RANGE:		return hzerr_RANGE ;
	case HZERR_BADVALUE:	return hzerr_BADVALUE ;

	case HZERR_INITFAIL:	return hzerr_INITFAIL ;
	case HZERR_INITDUP:		return hzerr_INITDUP ;
	case HZERR_NOINIT:		return hzerr_NOINIT ;
	case HZERR_SETONCE:		return hzerr_SETONCE ;
	case HZERR_EXCLUDE:		return hzerr_EXCLUDE ;
	case HZERR_CONFLICT:	return hzerr_CONFLICT ;
	case HZERR_SEQUENCE:	return hzerr_SEQUENCE ;
	case HZERR_DUPLICATE:	return hzerr_DUPLICATE ;
	case HZERR_NODATA:		return hzerr_NODATA ;
	case HZERR_NOTFOUND:	return hzerr_NOTFOUND ;
	case HZERR_INSERT:		return hzerr_INSERT ;
	case HZERR_DELETE:		return hzerr_DELETE ;
	case HZERR_CORRUPT:		return hzerr_CORRUPT ;

	case HZERR_PARSE:		return hzerr_PARSE ;
	case HZERR_SYNTAX:		return hzerr_SYNTAX ;

	case HZERR_NOCREATE:	return hzerr_NOCREATE ;
	case HZERR_OPENFAIL:	return hzerr_OPENFAIL ;
	case HZERR_NOTOPEN:		return hzerr_NOTOPEN ;
	case HZERR_READFAIL:	return hzerr_READFAIL ;
	case HZERR_WRITEFAIL:	return hzerr_WRITEFAIL ;
	case HZERR_MEMORY:		return hzerr_MEMORY ;
	case HZERR_OVERFLOW:	return hzerr_OVERFLOW ;

	case HZERR_DNS_NOHOST:	return hzerr_DNS_NOHOST ;
	case HZERR_DNS_NODATA:	return hzerr_DNS_NODATA ;
	case HZERR_DNS_RETRY:	return hzerr_DNS_RETRY ;
	case HZERR_DNS_FAILED:	return hzerr_DNS_FAILED ;

	case HZERR_NOSOCKET:	return hzerr_NOSOCKET ;
	case HZERR_HOSTFAIL:	return hzerr_HOSTFAIL ;
	case HZERR_HOSTRETRY:	return hzerr_HOSTRETRY ;
	case HZERR_SENDFAIL:	return hzerr_SENDFAIL ;
	case HZERR_RECVFAIL:	return hzerr_RECVFAIL ;
	case HZERR_PROTOCOL:	return hzerr_PROTOCOL ;
	case HZERR_TIMEOUT:		return hzerr_TIMEOUT ;
	case HZERR_RELEASE:		return hzerr_RELEASE ;
	case HZERR_SHUTDOWN:	return hzerr_SHUTDOWN ;

	case HZERR_BADSENDER:	return hzerr_BADSENDER ;
	case HZERR_BADADDRESS:	return hzerr_BADADDRESS ;
	case HZERR_NOACCOUNT:	return hzerr_NOACCOUNT ;
	case HZERR_SYSERROR:	return hzerr_SYSERROR ;
	}

	return 0 ;
}

const char*	ShowErrno	(void)
{
	//	Category:	Diagnostics
	//
	//	Convert errno to a text string for diagnostic purposes
	//
	//	Arguments:	None
	//
	//	Returns:	Pointer (cstr) being description of current value of errno as cstr

	static	const char*	_errnos	[] =
	{
		"0 NOERROR: Alles ist in ordnung",
		"1 EPERM: Operation not permitted",
		"2 ENOENT: No such file or directory",
		"3 ESRCH: No such process",
		"4 EINTR: Interrupted system call",
		"5 EIO: I/O error",
		"6 ENXIO: No such device or address",
		"7 E2BIG: Argument list too long",
		"8 ENOEXEC: Exec format error",
		"9 EBADF: Bad file number",
		"10 ECHILD: No child processes",
		"11 EAGAIN: Try again",
		"12 ENOMEM: Out of memory",
		"13 EACCES: Permission denied",
		"14 EFAULT: Bad address",
		"15 ENOTBLK: Block device required",
		"16 EBUSY: Device or resource busy",
		"17 EEXIST: File exists",
		"18 EXDEV: Cross-device link",
		"19 ENODEV: No such device",
		"20 ENOTDIR: Not a directory",
		"21 EISDIR: Is a directory",
		"22 EINVAL: Invalid argument",
		"23 ENFILE: File table overflow",
		"24 EMFILE: Too many open files",
		"25 ENOTTY: Not a typewriter",
		"26 ETXTBSY: Text file busy",
		"27 EFBIG: File too large",
		"28 ENOSPC: No space left on device",
		"29 ESPIPE: Illegal seek",
		"30 EROFS: Read-only file system",
		"31 EMLINK: Too many links",
		"32 EPIPE: Broken pipe",
		"33 EDOM: Math argument out of domain of func",
		"34 ERANGE: Math result not representable",
		"35 EDEADLK: Resource deadlock would occur",
		"36 ENAMETOOLONG: File name too long",
		"37 ENOLCK: No record locks available",
		"38 ENOSYS: Function not implemented",
		"39 ENOTEMPTY: Directory not empty",
		"40 ELOOP: Too many symbolic links encountered",
		"41 EWOULDBLOCK: (EAGAIN 11) Operation would block",
		"42 ENOMSG: No message of desired type",
		"43 EIDRM: Identifier removed",
		"44 ECHRNG: Channel number out of range",
		"45 EL2NSYNC: Level 2 not synchronized",
		"46 EL3HLT: Level 3 halted",
		"47 EL3RST: Level 3 reset",
		"48 ELNRNG: Link number out of range",
		"49 EUNATCH: Protocol driver not attached",
		"50 ENOCSI: No CSI structure available",
		"51 EL2HLT: Level 2 halted",
		"52 EBADE: Invalid exchange",
		"53 EBADR: Invalid request descriptor",
		"54 EXFULL: Exchange full",
		"55 ENOANO: No anode",
		"56 EBADRQC: Invalid request code",
		"57 EBADSLT: Invalid slot",
		"58 EDEADLOCK: (EDEADLK 35) Deadlock",
		"59 EBFONT: Bad font file format",
		"60 ENOSTR: Device not a stream",
		"61 ENODATA: No data available",
		"62 ETIME: Timer expired",
		"63 ENOSR: Out of streams resources",
		"64 ENONET: Machine is not on the network",
		"65 ENOPKG: Package not installed",
		"66 EREMOTE: Object is remote",
		"67 ENOLINK: Link has been severed",
		"68 EADV: Advertise error",
		"69 ESRMNT: Srmount error",
		"70 ECOMM: Communication error on send",
		"71 EPROTO: Protocol error",
		"72 EMULTIHOP: Multihop attempted",
		"73 EDOTDOT: RFS specific error",
		"74 EBADMSG: Not a data message",
		"75 EOVERFLOW: Value too large for defined data type",
		"76 ENOTUNIQ: Name not unique on network",
		"77 EBADFD: File descriptor in bad state",
		"78 EREMCHG: Remote address changed",
		"79 ELIBACC: Can not access a needed shared library",
		"80 ELIBBAD: Accessing a corrupted shared library",
		"81 ELIBSCN: .lib section in a.out corrupted",
		"82 ELIBMAX: Attempting to link in too many shared libraries",
		"83 ELIBEXEC: Cannot exec a shared library directly",
		"84 EILSEQ: Illegal byte sequence",
		"85 ERESTART: Interrupted system call should be restarted",
		"86 ESTRPIPE: Streams pipe error",
		"87 EUSERS: Too many users",
		"88 ENOTSOCK: Socket operation on non-socket",
		"89 EDESTADDRREQ: Destination address required",
		"90 EMSGSIZE: Message too long",
		"91 EPROTOTYPE: Protocol wrong type for socket",
		"92 ENOPROTOOPT: Protocol not available",
		"93 EPROTONOSUPPORT: Protocol not supported",
		"94 ESOCKTNOSUPPORT: Socket type not supported",
		"95 EOPNOTSUPP: Operation not supported on transport endpoint",
		"96 EPFNOSUPPORT: Protocol family not supported",
		"97 EAFNOSUPPORT: Address family not supported by protocol",
		"98 EADDRINUSE: Address already in use",
		"99 EADDRNOTAVAIL: Cannot assign requested address",
		"100 ENETDOWN: Network is down",
		"101 ENETUNREACH: Network is unreachable",
		"102 ENETRESET: Network dropped connection because of reset",
		"103 ECONNABORTED: Software caused connection abort",
		"104 ECONNRESET: Connection reset by peer",
		"105 ENOBUFS: No buffer space available",
		"106 EISCONN: Transport endpoint is already connected",
		"107 ENOTCONN: Transport endpoint is not connected",
		"108 ESHUTDOWN: Cannot send after transport endpoint shutdown",
		"109 ETOOMANYREFS: Too many references: cannot splice",
		"110 ETIMEDOUT: Connection timed out",
		"111 ECONNREFUSED: Connection refused",
		"112 EHOSTDOWN: Host is down",
		"113 EHOSTUNREACH: No route to host",
		"114 EALREADY: Operation already in progress",
		"115 EINPROGRESS: Operation now in progress",
		"116 ESTALE: Stale NFS file handle",
		"117 EUCLEAN: Structure needs cleaning",
		"118 ENOTNAM: Not a XENIX named type file",
		"119 ENAVAIL: No XENIX semaphores available",
		"120 EISNAM: Is a named type file",
		"121 EREMOTEIO: Remote I/O error",
		"122 EDQUOT: Quota exceeded",
		"123 ENOMEDIUM: No medium found",
		"124 EMEDIUMTYPE: Wrong medium type",
		"125 ECANCELED: Operation Canceled",
		"126 ENOKEY: Required key not available",
		"127 EKEYEXPIRED: Key has expired",
		"128 EKEYREVOKED: Key has been revoked",
		"129 EKEYREJECTED: Key was rejected by service",
		"130 EOWNERDEAD: Owner died",
		"131 ENOTRECOVERABLE: State not recoverable",
		"Invalid errno",
		""
	} ;

	if (errno < 0 || errno > 131)
		return _errnos[132] ;
	else
		return _errnos[errno] ;
}

const char*	ShowErrorSSL	(uint32_t err)
{
	//	Category:	Diagnostics
	//
	//	Convert SSL Errors to a text string for diagnostic purposes
	//
	//	Arguments:	1)	err		SSL error code
	//
	//	Returns:	Pointer (cstr) being description of current value of SSL error

	static const char*	_err	[] =
	{
		"SSL_ERROR_NONE I/O operation success",
		"SSL_ERROR_ZERO_RETURN Closure alert has occured (for clean closure)",
		"SSL_ERROR_WANT_READ Read operation incomplete",
		"SSL_ERROR_WANT_WRITE Write operation incomplete",
		"SSL_ERROR_WANT_CONNECT Connect operation incomplete",
		"SSL_ERROR_WANT_ACCEPT Accept operation incomplete",
		"SSL_ERROR_WANT_X509_LOOKUP Op did not complete because an app callback has asked to be called again",
		"SSL_ERROR_SYSCALL Socket error",
		"SSL_ERROR_SSL Library error",
		"SSL_ERROR_SSL undefined",
		""
	} ;

	if (err > 9)
		return _err[9] ;
	else
		return _err[err] ;
}

/*
**	Error Support Functions
*/

static	void	_makeErrmsg	(hzChain& E, const hzFuncname& fn, hzEaction eAction, hzEcode eError, const char* objName)
{
	//	Support function to the public HadronZoo error reporting functions hzerr() and hzwarn() and well as the report and exit function hzexit(). Its role is
	//	simply to build the error report in the supplied chain.
	//
	//	Arguments:	1)	E		Reference to chain for building error message
	//				2)	fn		Reference to function name
	//				3)	eError	The error code
	//				4)	level	Warning/error
	//
	//	Returns:	None

	hzXDate	x ;		//	System full date

	x.SysDateTime() ;

	if		(eAction == HZ_WARNING)		E.Printf("HadronZoo Library Warning\n") ;
	else if (eAction == HZ_ERROR)		E.Printf("HadronZoo Library Error\n") ;
	else if (eAction == HZ_SHUTDOWN)	E.Printf("HadronZoo Library Shutdown\n") ;
	else
		E.Printf("HadronZoo Library Fatal Error\n") ;

	E.Printf("\tTime:       %04d/%02d/%02d-%02d:%02d:%02d.%06d\n", x.Year(), x.Month(), x.Day(), x.Hour(), x.Min(), x.Sec(), x.uSec()) ;
	E.Printf("\tProcess id: %05d\n", getpid()) ;
	E.Printf("\tThread id:  %u\n", pthread_self()) ;
	E.Printf("\tFunction:   %s\n", *fn) ;
	E.Printf("\tError:      %s\n", Err2Txt(eError)) ;
	if (objName)
		E.Printf("\tObject:     %s\n", objName) ;
	E.Printf("\tDetails:    ") ;
}

/*
**	hzerr functions without object name
*/

//	FnGrp:		hzerr
//	Category:	Diagnostics
//
//	Report the error for the supplied error code that is assumed to occur within the named function.

hzEcode	hzerr	(const hzFuncname& fn, hzEaction eAction, hzEcode eError)
{
	//	Report and return a HadronZoo error condition.
	//
	//	Arguments:	1)	fn			Function name
	//				2)	eAction		Error action 
	//				3)	eError		Error code
	//
	//	Returns:	Enum error code as supplied

	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile

	_makeErrmsg(E, fn, HZ_ERROR, eError, 0) ;
	E << "None\n" ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}

	if (eAction == HZ_FATAL)
		exit(eError.Value()) ;

	return eError ;
}

hzEcode	hzerr	(const hzFuncname& fn, hzEaction eAction, hzEcode eError, const char* va_alist ...)
{
	//	Gernerate a logfile report and return error code with description supplied as a variable argument
	//
	//	Arguments:	1)	fn			Function name
	//				2)	eAction		Error action (error/warn/shutdown/exit)
	//				3)	eError		Error code
	//				4)	va_alist	Variable argument message
	//
	//	Returns:	Enum error code as supplied

	va_list		ap1 ;		//	Variable arguments list
	const char*	fmt ;		//	Format control string

	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile

	//	Do the args
	va_start(ap1, va_alist) ;
	fmt = va_alist ;

	_makeErrmsg(E, fn, eAction, eError, 0) ;
	E._vainto(fmt, ap1) ;
	va_end(ap1) ;
	E.AddByte(CHAR_NL) ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}

	if (eAction == HZ_FATAL)
		exit(eError.Value()) ;

	return eError ;
}

hzEcode	hzerr	(const hzFuncname& fn, hzEaction eAction, hzEcode eError, hzChain& error)
{
	//	Generate a logfile report and return error code with description supplied as a pre-populated chain
	//
	//	Arguments:	1)	fn			Function name
	//				2)	eAction		Error action (error/warn/shutdown/exit)
	//				3)	eError		Error code
	//				4)	va_alist	Variable argument message
	//
	//	Returns:	Enum error code as supplied

	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile

	_makeErrmsg(E, fn, eAction, eError, 0) ;
	E << error ;
	E.AddByte(CHAR_NL) ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}

	if (eAction == HZ_FATAL)
		exit(eError.Value()) ;

	return eError ;
}

/*
**	hzerr functions with object name argument
*/

hzEcode	hzerr	(const hzFuncname& fn, const hzString objName, hzEaction eAction, hzEcode eError, bool bTrace)
{
	//	Report and return a HadronZoo error condition.
	//
	//	Arguments:	1)	fn			Function name
	//				2)	eAction		Error action 
	//				3)	eError		Error code
	//
	//	Returns:	Enum error code as supplied

	hzChain		E ;			//	Error chain
    hzProcess*  phz ;		//	Process controller
	hzLogger*	pLog ;		//	Output logfile

	_makeErrmsg(E, fn, HZ_ERROR, eError, *objName) ;
	E << "None\n" ;

	pLog = GetThreadLogger() ;

	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}

	if (bTrace)
	{
    	phz = GetThreadInfo() ;
		if (phz)
			phz->StackTrace() ;
	}

	if (eAction == HZ_FATAL)
		exit(eError.Value()) ;

	return eError ;
}

hzEcode	hzerr	(const hzFuncname& fn, const hzString objName, hzEaction eAction, hzEcode eError, const char* va_alist ...)
{
	//	Gernerate a logfile report and return error code with description supplied as a variable argument
	//
	//	Arguments:	1)	fn			Function name
	//				2)	eAction		Error action (error/warn/shutdown/exit)
	//				3)	eError		Error code
	//				4)	va_alist	Variable argument message
	//
	//	Returns:	Enum error code as supplied

	va_list		ap1 ;		//	Variable arguments list
	const char*	fmt ;		//	Format control string

	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile

	//	Do the args
	va_start(ap1, va_alist) ;
	fmt = va_alist ;

	_makeErrmsg(E, fn, eAction, eError, *objName) ;
	E._vainto(fmt, ap1) ;
	va_end(ap1) ;
	E.AddByte(CHAR_NL) ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}

	if (eAction == HZ_FATAL)
		exit(eError.Value()) ;

	return eError ;
}

hzEcode	hzerr	(const hzFuncname& fn, const hzString objName, hzEaction eAction, hzEcode eError, hzChain& error)
{
	//	Generate a logfile report and return error code with description supplied as a pre-populated chain
	//
	//	Arguments:	1)	fn			Function name
	//				2)	eAction		Error action (error/warn/shutdown/exit)
	//				3)	eError		Error code
	//				4)	va_alist	Variable argument message
	//
	//	Returns:	Enum error code as supplied

	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile

	_makeErrmsg(E, fn, eAction, eError, *objName) ;
	E << error ;
	E.AddByte(CHAR_NL) ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}

	if (eAction == HZ_FATAL)
		exit(eError.Value()) ;

	return eError ;
}

//	FnGrp:		hzexit
//	Category:	Diagnostics
//
//	Report the error for the supplied error code that is assumed to occur within the named function. Then provide a stack trace for the calling thread and
//	terminate execution of the program.

void	hzexit	(const hzFuncname& fn, hzEcode eError, const char* va_alist ...)
{
	//	Arguments:	1)	fn			Formal function name
	//				2)	eError		Error code
	//				3)	va_alist	Error description
	//
	//	Returns:	None

	va_list		ap1 ;		//	Variable argument list
	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile
    hzProcess*  phz ;		//	Process controller
	const char*	fmt ;		//	Format control string

    phz = GetThreadInfo() ;
	_hzGlobal_kill = true ;
	_makeErrmsg(E, fn, HZ_FATAL, eError, 0) ;

	va_start(ap1, va_alist) ;
	fmt = va_alist ;

	E._vainto(fmt, ap1) ;
	va_end(ap1) ;
	E.AddByte(CHAR_NL) ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}
	phz->StackTrace() ;
	exit(eError.Value()) ;
}

void	hzexit	(const hzFuncname& fn, const hzString objName, hzEcode eError, const char* va_alist ...)
{
	//	Arguments:	1)	fn			Formal function name
	//				2)	eError		Error code
	//				3)	va_alist	Error description
	//
	//	Returns:	None

	va_list		ap1 ;		//	Variable argument list
	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile
    hzProcess*  phz ;		//	Process controller
	const char*	fmt ;		//	Format control string

    phz = GetThreadInfo() ;
	_hzGlobal_kill = true ;
	_makeErrmsg(E, fn, HZ_FATAL, eError, *objName) ;

	va_start(ap1, va_alist) ;
	fmt = va_alist ;

	E._vainto(fmt, ap1) ;
	va_end(ap1) ;
	E.AddByte(CHAR_NL) ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}
	phz->StackTrace() ;
	exit(eError.Value()) ;
}

void	hzexit	(const hzFuncname& fn, const hzString objName, hzEcode eError, hzChain& error)
{
	//	Arguments:	1)	fn		Formal function name
	//				2)	eError	Error code
	//				3)	msg		Error description
	//
	//	Returns:	None

	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile
    hzProcess*  phz ;		//	Process controller

    phz = GetThreadInfo() ;
	_hzGlobal_kill = true ;
	_makeErrmsg(E, fn, HZ_FATAL, eError, *objName) ;
	E << error ;
	E.AddByte(CHAR_NL) ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}
	phz->StackTrace() ;
	exit(eError.Value()) ;
}

void	hzexit	(const hzFuncname& fn, hzEcode eError, hzChain& error)
{
	//	Arguments:	1)	fn		Formal function name
	//				2)	eError	Error code
	//				3)	msg		Error description
	//
	//	Returns:	None

	hzChain		E ;			//	Error chain
	hzLogger*	pLog ;		//	Output logfile
    hzProcess*  phz ;		//	Process controller

    phz = GetThreadInfo() ;
	_hzGlobal_kill = true ;
	_makeErrmsg(E, fn, HZ_FATAL, eError, 0) ;
	E << error ;
	E.AddByte(CHAR_NL) ;

	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}
	phz->StackTrace() ;
	exit(eError.Value()) ;
}

//	FnGrp:		Fatal
//	Category:	Diagnostics
//
//	Argument:	msg		Error description
//	Returns:	None	Exits program
//
//	Simplified version of 'hzexit' using just the error description.
//
//	Func:	Fatal(hzChain&)
//	Func:	Fatal(const char* ...)

void	Fatal	(hzChain& error)
{
	//	Category:	Diagnostics
	//
	//	Make a HadronZoo fatal error report using the supplied hzChain (as error message) and place it both in the logfile for the thread and to stderr
	//
	//	Arguments:	1)	error	The error message as chain
	//
	//	Returns:	None

	hzChain		E ;			//	Error chain
	hzXDate		d ;			//	System full date
	hzLogger*	pLog ;		//	Output logfile
    hzProcess*  phz ;		//	Process controller

	//	Get log channel and current date & time
    phz = GetThreadInfo() ;
	_hzGlobal_kill = true ;
	d.SysDateTime() ;

	//	Populate the errmsg chain
	E.Printf("Fatal Error in process %05u (tid %u) at %04d%02d%02d-%02d%02d%02d -> ",
		getpid(), pthread_self(), d.Year(), d.Month(), d.Day(), d.Hour(), d.Min(), d.Sec()) ;
	E << error ;

	//	Output the message - deal with case where no logger is found for the current thread or the logger is closed or non-verbose (demon)
	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}
	phz->StackTrace() ;
	exit(0) ;
}

void	Fatal	(const char* va_alist ...)
{
	//	Category:	Diagnostics
	//
	//	Make a HadronZoo fatal error report using varags and place it both in the logfile for the thread and to stderr
	//
	//	Arguments:	1)	error	The variable argument error message
	//
	//	Returns:	None

	va_list		ap1 ;		//	Variable argument list
	hzChain		E ;			//	Error chain
	hzXDate		d ;			//	System full date
	hzLogger*	pLog ;		//	Output logfile
    hzProcess*  phz ;		//	Process controller
	const char*	fmt ;		//	Format control string

	//	Get log channel and current date & time
	//pLog = GetThreadLogger() ;
    phz = GetThreadInfo() ;
	_hzGlobal_kill = true ;
	d.SysDateTime() ;

	//	Sort the args
	va_start(ap1, va_alist) ;
	fmt = va_alist ;

	//	Populate the errmsg chain
	E.Printf("Fatal Error in process %05u (tid %u) at %04d%02d%02d-%02d%02d%02d -> ",
		getpid(), pthread_self(), d.Year(), d.Month(), d.Day(), d.Hour(), d.Min(), d.Sec()) ;
	E._vainto(fmt, ap1) ;
	va_end(ap1) ;

	//	Output the message - deal with case where no logger is found for the current thread or the logger is closed or non-verbose (demon)
	pLog = GetThreadLogger() ;
	if (!pLog)
		std::cerr << E ;
	else
	{
		if (pLog->IsOpen())
			pLog->Out(E) ;
		else
			std::cerr << E ;
	}
	phz->StackTrace() ;
	exit(0) ;
}
