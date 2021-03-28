//
//	File:	hzErrcode.h
//
//	Legal Notice: This file is part of the HadronZoo C++ Class Library.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	The HadronZoo C++ Class Library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as
//	published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Lesser Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License along with the HadronZoo C++ Class Library. If not, see http://www.gnu.org/licenses.
//

#ifndef hzErrcode_h
#define hzErrcode_h

#include "hzBasedefs.h"

class	hzEcode
{
	//	Category:	Error
	//
	//	hzEcode (error code), is a common return data type for many functions in the HadronZoo library. Although essentially an integer, it was redefined as a class to enforce type
	//	checking. There was also a degree of 'future proofing' behind this redefinition. Throughout the library's history error handling has evolved, mostly through improvements in
	//	logging. In many cases a function declares a hzChain instance as an auxillary 'log file'. Messages are then aggregated to the chain, both by the function itself but also by
	//	functions it calls. If this sequence of events results in success, the chain is deleted. But if an error occurs, the entire chain is written to the actual logfile. This has
	//	proved to be a useful diagnostic approach but is cumbersome. The chain is passed as an argument so functions have to be adapted to accept it. Very often these functions are
	//	called in other circumstances where no chain applies.
	//
	//	It is the intention 'at some point' to develop a formal and universal approach using hzEcode in conjuction with the call stack managed by the hzProcess class. Currently the
	//	call stack is only used to output the stack trace - but only when the program is terminating and then only as a series of fuction names. If function A calls B and B calls C
	//	and C detects a terminal condition, exit functions such as hzexit or Fatal output a stack trace which tells you that C failed and that C was called by B which was called by
	//	A! What the stack trace does not tell you is that C failed because it called D which didn't get what it wanted from E because F could not find the file it was looking for.
	//	And if the program does not actually exit, the stack trace tells you nothing because it is not output! The infomation is in the logs but only because there are far too many
	//	log statements in the library code.
	//
	//	The plan is to introduce extra columns in the stack managed by hzProcess, to hold the outcome (the return code), plus some information about arguments and/or which resource
	//	caused a problem. There would then be a hzEcode::Report function that would export this information. Note that this would require 'return' to be written as 'Return' or some
	//	such variant on the theme. This would be because it would be a macro that recorded the outcome before returning. This would work the same way as the _hzfunc macro, which is
	//	in most library functions and is responsible for recording the call on the stack.

	uint16_t	m_code ;	//	HadronZoo error code
	uint16_t	m_errno ;	//	System error code

public:

	hzEcode	(void)			{ m_code = 0 ; }
	hzEcode	(uint32_t v)	{ m_code = v ; }

	void		SetErrno	(uint32_t en)	{ m_errno = en ; }

	uint32_t	Value		(void)	{ return (uint32_t) m_code ; }
	uint32_t	Errno		(void)	{ return (uint32_t) m_errno ; }

	hzEcode&	operator=	(hzEcode v)		{ m_code = v.m_code ; return *this ; }
	bool		operator==	(hzEcode v)		{ return m_code == v.m_code ? true : false ; }
	bool		operator!=	(hzEcode v)		{ return m_code != v.m_code ? true : false ; }
} ;

/*
**	Error codes (including those based on errno)
*/

//	General
extern	const	hzEcode	E_OK ;				//	Everything is hunky dory

//	HadronZoo general error codes
extern	const	hzEcode	E_ARGUMENT ;		//	Bad argument
extern	const	hzEcode	E_TYPE ;			//	Data Type not valid or not appropriate in this context
extern	const	hzEcode	E_FORMAT ;			//	Incorrect data or string format
extern	const	hzEcode	E_RANGE ;			//	Value out of range
extern	const	hzEcode	E_BADVALUE ;		//	Illegal value for type

//	Confilcts when setting values
extern	const	hzEcode	E_INITFAIL ;		//	Initialization failed
extern	const	hzEcode	E_INITDUP ;			//	Resource already initialized
extern	const	hzEcode	E_NOINIT ;			//	Resource not initialized
extern	const	hzEcode	E_SETONCE ;			//	A variable that can only be set once, has already been set
extern	const	hzEcode	E_EXCLUDE ;			//	Exclusive resource already in use
extern	const	hzEcode	E_CONFLICT ;		//	Resource already defined as something else
extern	const	hzEcode	E_SEQUENCE ;		//	Expected sequence is not observed

//	Database errors
extern	const	hzEcode	E_DUPLICATE ;		//	Attempt to create an already existing object
extern	const	hzEcode	E_NODATA ;			//	No data or empty file
extern	const	hzEcode	E_NOTFOUND ;		//	Item not in collecton or search criteria ressulted in 0 objects found
extern	const	hzEcode	E_INSERT ;			//	Could not insert data
extern	const	hzEcode	E_DELETE ;			//	Could not delete data
extern	const	hzEcode	E_CORRUPT ;			//	Database, index or tree is corrupted

//	Expressions
extern	const	hzEcode	E_PARSE ;			//	Cannot parse expression
extern	const	hzEcode	E_SYNTAX ;			//	Syntax error in expression

//	System resources
extern	const	hzEcode	E_NOCREATE ;		//	Failed to create a resorce (could be file or other data source)
extern	const	hzEcode	E_OPENFAIL ;		//	Failed to open a resorce (could be file or other data source)
extern	const	hzEcode	E_NOTOPEN ;			//	Resource is not open
extern	const	hzEcode	E_READFAIL ;		//	Read operation failed
extern	const	hzEcode	E_WRITEFAIL ;		//	Write operation failed
extern	const	hzEcode	E_MEMORY ;			//	Out of memory
extern	const	hzEcode	E_OVERFLOW ;		//	Buffer overflowed

//	DNS
extern	const	hzEcode E_DNS_NOHOST ;		//	The domain does not exist
extern	const	hzEcode E_DNS_NODATA ;		//	The domain exists but no records exists of the requested type (e.g. HTTP/SMTP)
extern	const	hzEcode	E_DNS_RETRY ;		//	Operation timed out
extern	const	hzEcode E_DNS_FAILED ;		//	Something terrible is wrong with the DNS

//	Comms
extern	const	hzEcode	E_NOSOCKET ;		//	Cannot get a TCP/IP socket
extern	const	hzEcode	E_HOSTFAIL ;		//	Host server is down or not accepting clients
extern	const	hzEcode	E_HOSTRETRY ;		//	Host server is up, accepts clients but cuts them off with too busy message
extern	const	hzEcode	E_SENDFAIL ;		//	Cannot connect to host, server maybe down
extern	const	hzEcode	E_RECVFAIL ;		//	Cannot connect to host, server maybe down
extern	const	hzEcode	E_PROTOCOL ;		//	Client/Server protocol violation
extern	const	hzEcode	E_TIMEOUT ;			//	Operation timed out
extern	const	hzEcode	E_RELEASE ;			//	Failed to release a resorce
extern	const	hzEcode	E_SHUTDOWN ;		//	Error occured during shutdown

//	Emails
extern	const	hzEcode	E_BADSENDER ;		//	No sender email addr
extern	const	hzEcode	E_BADADDRESS ;		//	Badly formed address
extern	const	hzEcode	E_NOACCOUNT ;		//	A Mail server has denied the existence of the account
extern	const	hzEcode	E_SYSERROR ;		//	System command error

/*
**	Prototypes
*/

//	Converts error code to text error description
const char*	ShowErrno		(void) ;
const char*	Err2Txt			(hzEcode eError) ;
const char*	ShowErrorSSL	(uint32_t err) ;

#endif	//	hzErrcode.h
