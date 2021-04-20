//
//	File:	hzProcess.h
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

#ifndef hzProcess_h
#define hzProcess_h

#include "hzDate.h"
#include "hzLock.h"

/*
**	SECTION 1:	The hzProcess and hzProginfo classes
*/

class	hzLogger ;

class	hzProcess
{
	//	Category:	System
	//
	//	The hzProcess class is used to hold dynamic information concerning threads, direct log messages arising and to facilitate stack traces. In multithreaded
	//	HadronZoo based applications it is compulsory to declare one instance per thread and there must be one instance in a single threaded applications if any
	//	logging is to be done.
	//
	//	

	class	_fncall_rec
	{
		//	Record of a function call

	public:
		const char*	m_func ;		//	Function full name (class::fnname)
		uint32_t	m_callNo ;		//	Sequence number
		uint16_t	m_series ;		//	Number of times sequence flipped back to zero
		uint16_t	m_level ;		//	Call depth of call

		_fncall_rec	()	{ m_func = 0 ; m_callNo = 0 ; m_series = m_level = 0 ; }
	} ;

	const char**	m_Stack ;		//	Function call stack
	_fncall_rec*	m_Hist ;		//	Recent function call history

	hzLogger*	m_pLog ;			//	The default (first) logger for the thread
	void*		m_CurrentLock ;		//	Memory location Microlock
	pthread_t	m_TID ;				//	The current thread id
	pid_t		m_PID ;				//	The current process id
	uint32_t	m_nFuncs ;			//	The current function (current position in stack)
	uint32_t	m_nPeak ;			//	Function depth high water mark
	uint32_t	m_nSeqCall ;		//	Function call sequence number
	uint32_t	m_nCallOset ;		//	Current offset into history

public:
	hzProcess	(void) ;
	~hzProcess	(void) ;

	//	Set the default logger for the thread to the supplied logger unless already set 
	void	SetLog	(hzLogger* pLog)	{ m_pLog = pLog ; }

	hzLogger*	GetLog	(void)	{ return m_pLog ; }			//	Get logger for thread
	pthread_t	GetTID	(void)	{ return m_TID ; }			//	Get the thread id
	uint32_t	Level	(void)	{ return m_nFuncs-1 ; }		//	Return current function call level

	void	PushFunction	(const char* funcname) ;		//	Notify thread stack of function call
	void	PullFunction	(void) ;						//	Notify thread of function return
	void	StackTrace		(void) ;						//	Export Current Stack Trace
	void	CallHistory		(void) ;						//	Export recent function call history
} ;

class	hzProginfo
{
	//	Category:	System
	//
	//	Information on currently running program - as held in the /proc directory.

public:
	hzString	m_Progname ;		//	Name of program
	hzString	m_Executable ;		//	Full path to the executable
	uint32_t	m_PID ;				//	Process ID (from proc)
	uint32_t	m_Resv ;			//	Reserved
} ;

/*
**	SECTION 2:	Shared Memory
*/

class	hzShmem
{
	//	Category:	System
	//
	//	Shared memory segment for Inter-process Communication.

	void*		m_pStart ;			//	Pointer to start
	hzString	m_name ;			//	Shared memory segment name
	uint32_t	m_size ;			//	Shared memory segment size
	uint32_t	m_perm ;			//	Access permissions
	uint32_t	m_fd ;				//	Shared memory 'file descriptor'
	uint32_t	m_resv ;			//	Reserved

public:
	hzShmem		(void)	{ m_size = 0 ; m_pStart = 0 ; }
	~hzShmem	(void)	{}

	hzEcode		Init	(const char* name, uint32_t size, uint32_t mask) ;
	uint32_t	Size	(void) ;

	void*	GetMem	(void)	{ return m_pStart ; }
} ;

/*
**	SECTION 3:	Resource locking. The hzSysLock class
*/

class	hzSysLock
{
	//	Category:	System
	//
	//	Controls 'external' resources such as files

	int32_t		m_nSemId ;		//	Unique id for semahore group
	int32_t		m_lockval ;		//	The thread that has the lock
	uint32_t	m_nResource ;	//	Slot within semaphore group
	uint32_t	m_count ;		//	Number of nested calls to lock by same thread

	void	_init	(void) ;	//	Obtain a semaphore from a semaphore set
	void	_halt	(void) ;	//	Free the semaphore back to the set (for possible reallocation)
public:
	hzSysLock	(void)	{ m_lockval = 0 ; m_count = 0 ; _init() ; }

	~hzSysLock	(void)	{ _halt() ; }

	void	Lock	(void) ;
	void	Unlock	(void) ;
} ;

/*
**	SECTION 4:	Event Loging Regime. This covers the following:-
**
**	1)	Periodic log files
**	2)	Dynamic debug levels
**	3)	A naming regime to build function names into log output.
**	4)	A protocol for loging via the HadronZoo logserver.
**	5)	The hzLogger class for writing log output to.
*/

#define PORT_LOGSERVER	4592
#define HZ_LOGCHUNK		1449	//	HZ_MAXPACKET - 11

enum	hzLogRotate
{
	//	Category:	Diagnostics
	//
	//	Enum to control periodicity of roliing logfiles

	LOGROTATE_NEVER,		//	Never change logfile and truncate logfile at runtime
	LOGROTATE_OFLOW,		//	Change file when filesize reached 2GB (appname.log .. appname.1.log)
	LOGROTATE_YEAR,			//	Change logfile once per year	appname_YYYY.log
	LOGROTATE_QTR,			//	Change logfile every quarter	appname_YYYYQ?.log
	LOGROTATE_MONTH,		//	Change logfile every month		appname_YYYYMM.log
	LOGROTATE_MON,			//	Change logfile every Monday		appname_YYYYMMDDMon.log
	LOGROTATE_TUE,			//	Change logfile every Tuesday	appname_YYYYMMDDTue.log
	LOGROTATE_WED,			//	Change logfile every Wednesday	appname_YYYYMMDDWed.log
	LOGROTATE_THR,			//	Change logfile every Thursday	appname_YYYYMMDDThr.log
	LOGROTATE_FRI,			//	Change logfile every Friday		appname_YYYYMMDDFri.log
	LOGROTATE_SAT,			//	Change logfile every Saturday	appname_YYYYMMDDSat.log
	LOGROTATE_SUN,			//	Change logfile every Sunday		appname_YYYYMMDDSun.log
	LOGROTATE_DAY,			//	Change logfile every day		appname_YYYYMMDD.log
	LOGROTATE_12HR,			//	Change logfile every half day	appname_YYYYMMDD?M.log
	LOGROTATE_HOUR,			//	Change logfile every hour		appname_YYYYMMDDHH.log
} ;

enum	hzDebug
{
	//	Category:	Diagnostics
	//
	//	Controls which log messages are printed.

	DEBUG_NONE		= 0,	//	No debug messages (the default)
	DEBUG_LEVEL1	= 1,	//	Mega critical 
	DEBUG_LEVEL2	= 2,	//	Importantish 
	DEBUG_LEVEL3	= 3		//	Trivial 
} ;

/*
**	SECTION 5:	Thread information prototypes
*/

hzProcess*	GetThreadInfo	(void) ;
hzLogger*	GetThreadLogger	(void) ;
void		SetThreadLogger	(hzLogger* plog) ;

class	hzFuncname
{
	//	Category:	Diagnostics
	//
	//	hzFuncname is just a char string pointer storing a function name. It was created to avoid the compiler confusing it with
	//	anything else

	const char*	m_pStr ;	//	Current function name

public:
	hzFuncname	(void)	{ m_pStr = 0 ; }
	~hzFuncname	(void)	{ m_pStr = 0 ; }

	void	operator=	(const char* fnName)
	{
		//	Argument:	fnName	Sets the current function name
		//	Returns:	None

		if (!m_pStr)
			m_pStr = fnName ;
	}

	const char*	operator*	(void) const	{ return m_pStr ; }
	operator const char*	(void) const	{ return m_pStr ; }
} ;

class	_hz_func_reg
{
	//	Category:	Diagnostics
	//
	//	Internal class to facilitate registration of a function call to the calling thread. The macro _hzfunc is placed at the top of most HadronZoo functions (both independent and
	//	class members). This macro takes a char* argument, the value of which is assumed to be the fully qualified name of the function or a close variant of it. The macro declares
	//	an instance of _hz_func_reg called _thisfn and assigns the function name to it.
	//
	//	The _hz_func_reg assignment operator calls GetThreadInfo to obtain a pointer to the hzProcess instance for the current thread and stores in the the public member _phz. Then
	//	it uses the pointer to call hzProcess::PushFunction - which pushes the function name on to the stack.
	//
	//	In any function with the _hzfunc macro, the current thread is available without a further call to GetThreadInfo, as _thisfn._phz - not that a GetThreadInfo call is onerous.
	//	The ready availablity of the hzProcess instance has a lot of potential for holding data 'private to the thread'.
	//
	//	The _hzfunc macro must not be called outside the scope of a function even though the compiler would allow it. Doing so would place a false entry in the hzProcess stack. The
	//	macro when called at the top of a function as intended, creates the _hz_func_reg instance _thisfn as an automatic variable. This is deleted when the function returns so the
	//	call to hzProcess::PullFunction which pulls the function name from the hzProcess stack, is placed in the _hz_func_reg destructor.

public:
	hzProcess*	_phz ;	//	Pointer to hzProcess instance (process information)

	_hz_func_reg	(void)	{ _phz = 0 ; }

	~_hz_func_reg	(void)
	{
		//	This destructor is called as any function starting with a call to _hzfunc() ends. As the latter pushes a function name onto the function stack for
		//	the thread, this must be pulled here.

		if (_phz)
			_phz->PullFunction() ;
	}

	void	operator=	(const char* fnName)
	{
		//	Set the function name and push it onto the function stack for the thread.
		//
		//	Argument:	fnName	The fully qualified function name
		//	Returns:	None

		if (!_phz)
			_phz = GetThreadInfo() ;

		if (_phz)
			_phz->PushFunction(fnName) ;
	}
} ;

#define _hzfunc(x)	static hzFuncname _fn ; _fn = x ; _hz_func_reg _thisfn ; _thisfn = *_fn
#define	_hzlevel()	_thisfn._phz->Level()

//	Specific to Collection Class Templates
#define _hzfunc_ct(x)	static hzFuncname _fn ; _fn = x ; _hz_func_reg _thisfn ; if (_hzGlobal_Debug & HZ_DEBUG_CTMPLS) _thisfn = *_fn ;

/*
**	Error reporting prototypes
*/

enum	hzEaction
{
	//	Category:	Diagnostics
	//
	//	HadronZoo Warning/Error action codes. This enum is provided specifically for calling the error reporting function hzerr(). Note that the action code for
	//	imeadiate program termination, HZ_FATAL, can be used to call hzerr() and will cause hzerr() to exit after generating its report. However the convention
	//	is to call hzexit() to terminate the program.

	HZ_WARNING,		//	A non critical error, execution may continue without remedial action.
	HZ_ERROR,		//	A critical error, continued execution requires remedial action. 
	HZ_SHUTDOWN,	//	A critical error, system must enter shutdown mode and stop accepting new requests.
	HZ_FATAL		//	A critical error, execution will terminate imeadiately.
} ;

hzEcode hzerr	(const hzFuncname& fn, hzEaction eAction, hzEcode eError) ;
hzEcode hzerr	(const hzFuncname& fn, hzEaction eAction, hzEcode eError, hzChain& error) ;
hzEcode hzerr	(const hzFuncname& fn, hzEaction eAction, hzEcode eError, const char* va_alist ...) ;

hzEcode hzerr	(const hzFuncname& fn, const hzString objName, hzEaction eAction, hzEcode eError) ;
hzEcode hzerr	(const hzFuncname& fn, const hzString objName, hzEaction eAction, hzEcode eError, hzChain& error) ;
hzEcode hzerr	(const hzFuncname& fn, const hzString objName, hzEaction eAction, hzEcode eError, const char* va_alist ...) ;

void	hzexit	(const hzFuncname& fn, hzEcode eError, hzChain& error) ;
void	hzexit	(const hzFuncname& fn, hzEcode eError, const char* va_alist ...) ;

void	hzexit	(const hzFuncname& fn, const hzString objName, hzEcode eError, hzChain& error) ;
void	hzexit	(const hzFuncname& fn, const hzString objName, hzEcode eError, const char* va_alist ...) ;

void	Fatal	(hzChain& error) ;
void	Fatal	(const char* va_alist ...) ;

/*
**	SECTION 6:	Log signals to and from the HadronZoo logserver
*/

enum	hzLogSignal
{
	//	Category:	Diagnostics
	//
	//	Commands to server

	LS_NULL		= 0,		//	No action
	LS_OPEN_PUB	= 1,		//	Open a channel to a public logfile
	LS_OPEN_PVT	= 2,		//	Open a channel to a private logfile
	LS_STOP		= 3,		//	Close a channel (either public or private)
	LS_LOG		= 4,		//	Log to a log file (either public or private)

	//	Server responses
	LS_ERR_CONN	= 10,		//	Could not connect to logserver
	LS_ERR_SEND	= 11,		//	Data could not be written to a socket
	LS_ERR_RECV	= 12,		//	Date could not be read from a socket
	LS_ERR_PERM = 13,		//	Process does not have write permission
	LS_ERR_DUP	= 14,		//	Channel already open
	LS_FAIL		= 15,		//	Request failed
	LS_OK		= 16,		//	Request succesful
} ;

/*
**	The LogChannel class
*/

class	hzTcpClient ;

class	hzLogger
{
	//	Category:	Diagnostics
	//
	//	The hzLogger class manages a 'log channel'. Instead of writing to a logfile applications write to a hzLogger instance. This
	//	usually is a file but it might be a logserver instance (socket program)

	//	Variables for operation on direct files
	FILE*			m_pFile ;					//	The active logfile
	hzLockS			m_Lock ;					//	Mutex (if required)
	hzXDate			m_datCurr ;					//	Current date & time
	hzXDate			m_datLast ;					//	Last date & time rotation occured
	//	hzChain			m_Temp ;					//	Used to log events that will be discarded if operation is successful
	hzString		m_Base ;					//	Basename of logfile
	hzString		m_File ;					//	Full name of logfile

	//	Variables for operation with logserver
	hzTcpClient*	m_pConnection ;				//	Connection as client to logserver
	uchar*			m_pData ;					//	Buffer for preparation of log message
	uint16_t		m_nIndent ;					//	No of tabs to indent
	uint16_t		m_nSessID ;					//	Session ID

	//	Variable for both types of operation
	char*			m_pDataPtr ;				//	Set at m_cvData or an offset to allow packet hdr
	hzLogRotate		m_eRotate ;					//	Log rotate edict
	bool			m_bVerbose ;				//	Send log messages to stdin as well as file
	char			m_cvData[HZ_MAXPACKET+4] ;	//	Buffer for preparation of log message

	void	_logrotate	(void) ;				//	Functions for operation on direct files
	hzEcode	_write		(uint32_t nBytes) ;		//	Internal write function

	//	Prevent copying
	hzLogger	(const hzLogger& never) ;
	hzLogger&	operator=	(const hzLogger& never) ;

public:
	hzLogger	(void) ;
	~hzLogger	(void) ;

	void	Verbose		(bool bVerbose)		{ m_bVerbose = bVerbose ; }
	void	SetIndent	(uint16_t nIndent)	{ m_nIndent = nIndent ; }

	bool	IsOpen		(void)	{ return m_Base || m_pFile || m_nSessID ? true : false ; }
	bool	IsVerbose	(void)	{ return m_bVerbose ; }

	hzEcode	OpenFile	(const char* cpPath, hzLogRotate eRotate) ;
	hzEcode	OpenPublic	(const char* cpPath, hzLogRotate eRotate) ;
	hzEcode	OpenPrivate	(const char* cpPath, hzLogRotate eRotate, uint32_t Perms) ;

	hzEcode	Close		(void) ;
	hzEcode	Log			(hzFuncname& fn, const char* va_alist ...) ;
	hzEcode	Out			(const hzChain& msg) ;
	hzEcode	Out			(const char* va_alist ...) ;
	hzEcode	Record		(const char* va_alist ...) ;

	hzLogger&	operator<<	(hzChain& Z) ;
	hzLogger&	operator<<	(hzString& Z) ;
	hzLogger&	operator<<	(const char* cpStr) ;
} ;

hzEcode	threadLog	(const char* va_alist ...) ;
hzEcode	threadLog	(const hzChain& msg) ;

/*
**	Globals
*/

extern	bool		_hzGlobal_MT ;					//	Set true be set true by the application if it is to be run in multi-threaded mode
extern	bool		_hzGlobal_kill ;				//	True if shutdown pending. Don't accept new connections.
extern	uint32_t	_hzGlobal_callStack_size ;		//	Depth of per-thread call stack
extern	uint32_t	_hzGlobal_callHist_size ;		//	Depth of per-thread call history
extern	hzString	_hzGlobal_HOME ;				//	Environment variable $HOME
extern	hzString	_hzGlobal_HADRONZOO ;			//	Environment variable $HADRONZOO

/*
**	Prototypes
*/

//	Process control
hzEcode	HadronZooInitEnv	(void) ;		//	Read environment variables $HOME and $HADRONZOO
void	Demonize			(void) ;		//	Run the program as a demon
void	SingleProc			(void) ;		//	Ensure singleton operation (prevent further program instances)

void	StackTrace			(void) ;		//	Log current stack trace for calling thread
void	CallHistory			(void) ;		//	Log recent function call history for calling thread

void	CatchSegVio			(int32_t sig) ;	//	Provides backtrace of function calls in event of segment violation (SIGSEG)
void	CatchCtrlC			(int32_t sig) ;	//	Catches Ctrl-C keypress (SIGINT)

//	General Diagnostics
hzEcode		RegisterCollection		(const hzString, void*) ;
uint32_t	ActiveThreadCount		(void) ;
void		ReportMutexContention	(hzChain& Z) ;
void		ReportStringAllocations	(hzChain& Z) ;
void		ReportMemoryUsage		(hzChain& Z) ;
void		RecordMemoryUsage		(void) ;

#endif	//	hzProcess_h
