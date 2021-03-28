//
//	File:	hzProcess.cpp
//
//	Desc:	General functions to provide singleton processes, demons etc
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

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <execinfo.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "hzTextproc.h"
#include "hzProcess.h"
#include "hzDirectory.h"

/*
**	Global Variables
*/

global	hzString	_hzGlobal_HOME ;					//	Environment variable $HOME
global	hzString	_hzGlobal_HADRONZOO ;				//	Environment variable $HADRONZOO
global	uint32_t	_hzGlobal_Debug = 0 ;				//	Global debug mode
global	uint32_t	_hzGlobal_callStack_size = 200 ;	//	Call stack record to a depth of 200
global	uint32_t	_hzGlobal_callHist_size = 2000 ;	//	Call history record of the last 2000 calls
global	bool		_hzGlobal_kill = false ;			//	Program is doomed. This is set to stop memory cleanup accessing memory management collection classes.

static	hzLockS		s_thread_lock ;			//	Mutext for threds
static	uint32_t	s_num_threads ;			//	Current number of threads in register

static	hzLogger*	s_pDfltLog ;			//	Default logger for HadronZoo errors
static	hzProcess*	s_allThreadsA[64] ;		//	First register of threads
static	hzProcess*	s_allThreadsB[64] ;		//	Second register of threads
static	hzProcess**	s_actThreadReg ;		//	Currently active thread register (either A or B)

/*
**	SECTION 1:	Thread registration
*/

static	hzEcode	_hz_register_thread	(hzProcess* pProc)
{
	//	Register the current thread. The thread register must always be in order of thread id so to this end, this function copies the thread pointers from the
	//	current thread register array to a new array, inserting the new thread in it's appropriate position. It then swaps the active thread register pointer
	//	(used to lookup threads), to the new array.
	//
	//	Arguments:	1)	pProc	The process instance
	//
	//	Returns:	E_ARGUMENT	If no process controller is supplied
	//				E_OK		If the thread is registered

	_hzfunc("_hz_register_thread") ;

	hzProcess**	pActive ;			//	Active thread pointer
	hzProcess*	phz ;				//	Active process to examine
	uint32_t	nOrig ;				//	Active thread iterator
	uint32_t	nNew ;				//	Position in active thread array to place this process
	bool		bAdd = false ;		//	Set true when thread added
	hzEcode		rc = E_OK ;			//	Return code

	if (!pProc)
		return E_ARGUMENT ;

	if (!s_actThreadReg)
	{
		s_actThreadReg = s_allThreadsA ;
		s_allThreadsA[0] = pProc ;
		s_num_threads = 1 ;
		return E_OK ;
	}

	//	Lock so that multiple threads cannot register/deregister at the same time.
	s_thread_lock.Lock() ;

		if (s_actThreadReg == s_allThreadsA)
			pActive = s_allThreadsB ;
		else
			pActive = s_allThreadsA ;

		for (nOrig = nNew = 0 ; nOrig < s_num_threads ; nOrig++, nNew++)
		{
			phz = s_actThreadReg[nOrig] ;
			if (!bAdd && (pProc->GetTID() < phz->GetTID()))
			{
				pActive[nNew] = pProc ;
				nNew++ ;
				bAdd = true ;
			}
			pActive[nNew] = phz ;
		}

		if (!bAdd && (nNew == s_num_threads))
			pActive[nNew] = pProc ;
		s_num_threads++ ;
		_hzGlobal_MT = true ;
		s_actThreadReg = pActive ;

	s_thread_lock.Unlock() ;
	return rc ;
}

static	hzEcode _hz_deregister_thread	(hzProcess* pProc)
{
	//	Removes the current thread from the register. The thread register must always be in order of thread id so to this end, this function copies
	//	the thread pointers from the current thread register array to a new array, omiting the deprecated thread. It then swaps the active thread
	//	register pointer (used to lookup threads), to the new array.
	//
	//	//	Arguments:	1)	pProc	The process to de-register
	//
	//	//	Returns:	E_ARGUMENT	If the process was not supplied

	_hzfunc("_hz_deregister_thread") ;

	hzProcess**	pActive ;			//	Active thread pointer
	hzProcess*	phz ;				//	Active process to examine
	uint32_t	nOrig ;				//	Active thread iterator
	uint32_t	nNew ;				//	Position in active thread array to place this process
	//bool		bAdd = false ;		//	Set true when thread added
	hzEcode		rc = E_NOTFOUND ;	//	Return code

	if (!pProc)
		return E_ARGUMENT ;

	//	Lock so that multiple threads cannot register/deregister at the same time.
	s_thread_lock.Lock() ;

		if (s_actThreadReg == s_allThreadsA)
			pActive = s_allThreadsB ;
		else
			pActive = s_allThreadsA ;

		for (nOrig = nNew = 0 ; nOrig < s_num_threads ; nOrig++)
		{
			phz = s_actThreadReg[nOrig] ;
			if (phz->GetTID() == pProc->GetTID())
				continue ;

			pActive[nNew] = phz ;
			nNew++ ;
		}

		pActive[nNew] = 0 ;
		s_num_threads-- ;
		s_actThreadReg = pActive ;

	s_thread_lock.Unlock() ;

	return rc ;
}

/*
**	hzProcess Constructor & Destructor
*/

hzProcess::hzProcess	(void)
{
	//	Constructs a hzProcess instance. Sets the process and thread ids and registers the thread

	m_pLog = 0 ;
	m_nFuncs = 0 ;
	m_nPeak = 0 ;
	m_nCallOset = 0 ;
	m_nSeqCall = 0 ;

	if (_hzGlobal_callStack_size)
		m_Stack = new const char*[_hzGlobal_callStack_size+1] ;

	if (_hzGlobal_callHist_size)
		m_Hist = new _fncall_rec[_hzGlobal_callHist_size+1] ;

	m_PID = getpid() ;
	m_TID = pthread_self() ;

	_hz_register_thread(this) ;
}

hzProcess::~hzProcess	(void)
{
	//	Category:	Process
	//
	//	Removes the hzProcess instance from thread register and destructs it.

	_hz_deregister_thread(this) ;
}

void	hzProcess::PushFunction	(const char* funcname)
{
	//	Category:	Diagnostics
	//
	//	This is called by the _hzfunc macro if stack tracing is switched on. This adds the function called to the function stack of
	//	the hzProcess (the thread calling the function)
	//
	//	Arguments:	1)	funcname	The name of the current function. This should be the fully qualified name if a class method
	//	Returns:	None

	if (m_nPeak < _hzGlobal_callStack_size)
	{
		//	Plac function call in the stack
		m_Stack[m_nFuncs] = funcname ;
		m_nFuncs++ ;
		if (m_nFuncs > m_nPeak)
			m_nPeak = m_nFuncs ;
	}

	if (m_Hist)
	{
		//	Plac function call in the history
		m_Hist[m_nCallOset].m_func = funcname ;
		m_Hist[m_nCallOset].m_callNo = ++m_nSeqCall ;
		if (m_nSeqCall == 0)
			m_Hist[m_nCallOset].m_series++ ;
		m_Hist[m_nCallOset].m_level = m_nFuncs ;

		m_nCallOset++ ;
		if (m_nCallOset == _hzGlobal_callHist_size)
			m_nCallOset = 0 ;
	}

	if (m_nFuncs == _hzGlobal_callStack_size)
	{
		//	Stack is exceeded - terminate execution with a trace
		StackTrace() ;
		exit(200) ;
	}
}

void	hzProcess::PullFunction	(void)
{
	//	Category:	Diagnostics
	//
	//	This is called by the Return() macro is stack tracing is switched on. This removes the function as it exits from the function
	//	stack of the hzProcess (the thread calling the function)
	//
	//	Arguments:	None
	//	Returns:	None

	if (m_nPeak < _hzGlobal_callStack_size && m_nFuncs > 0)
	{
		//m_Stack[m_nFuncs] = 0 ;
		m_nFuncs-- ;
	}
}

void	hzProcess::StackTrace	(void)
{
	//	Category:	Diagnostics
	//
	//	Exports stack trace for this thread if there is an available logfile. The stack trace assumes all functions are using the _hzfunc() macro.
	//
	//	Arguments:	None
	//	Returns:	None

	hzLogger*	plog ;		//	Find applicable logger
	uint32_t	n ;			//	Function stack iterator

	plog = m_pLog ? m_pLog : s_pDfltLog ;
	if (!plog)
	{
		if (m_nPeak >= _hzGlobal_callStack_size)
		{
			printf("Stack was blown\n") ;
			fflush(stdout);
		}
		else
		{
			printf("Most Current Function at %d (peak %d) functions:-\n", m_nFuncs, m_nPeak) ;
			for (n = m_nFuncs - 1 ; n >= 0 ; n--)
				printf("%d -> %s\n", n + 1, m_Stack[n]) ;
			printf("End of stack\n") ;
			fflush(stdout);
		}
		return ;
	}

	if (m_nPeak >= _hzGlobal_callStack_size)
		plog->Out("Stack was blown\n") ;
	else
	{
		plog->Out("Most Current Function at %d (peak %d) functions:-\n", m_nFuncs, m_nPeak) ;
		for (n = m_nFuncs - 1 ; n >= 0 ; n--)
			plog->Out("%d -> %s\n", n + 1, m_Stack[n]) ;
		plog->Out("End of stack\n") ;
	}
}

void	hzProcess::CallHistory	(void)
{
	//	Category:	Diagnostics
	//
	//	Exports recent call history for this thread if there is an available logfile. The stack trace assumes all functions are using the _hzfunc() macro.
	//
	//	Arguments:	None
	//	Returns:	None

	hzLogger*	plog ;		//	Find applicable logger
	uint32_t	n ;			//	Function history iterator
	uint32_t	lev ;		//	Function level

	plog = m_pLog ? m_pLog : s_pDfltLog ;
	if (!plog)
		return ;

	//	Start with current position and work backwards to the begining. Then go to end of the history array and work backward to the current position
	plog->Out("Most Recent Function Calls:\n") ;
	for (n = m_nCallOset ; n ; n--)
	{
		lev = m_Hist[n].m_level ;

		plog->Out("%10u %02u ", m_Hist[n].m_callNo, lev) ;
		for (; lev ; lev--)
			plog->Out(".") ;
		plog->Out(" %s\n", m_Hist[n].m_func) ;
	}

	for (n = _hzGlobal_callHist_size ; n > m_nCallOset ; n--)
	{
		lev = m_Hist[n].m_level ;

		plog->Out("%10u %02u ", m_Hist[n].m_callNo, lev) ;
		for (; lev ; lev--)
			plog->Out(".") ;
		plog->Out(" %s\n", m_Hist[n].m_func) ;
	}
	plog->Out("End of History\n") ;
}

hzProcess*	GetThreadInfo	(void)
{
	//	Category:	Process
	//
	//	Return the hzProcess instance applicable to the current thread
	//
	//	Arguments:	None
	//	Returns:	Pointer to the process information of the current thread

	hzProcess*	phz ;	//	Thread
	pthread_t	tid ;	//	Current thread id
	uint32_t	n ;		//	Thread iterator

	tid = pthread_self() ;
	for (n = 0 ; n < s_num_threads ; n++)
	{
		phz = s_actThreadReg[n] ;

		if (tid == phz->GetTID())
			return phz ;
	}

	return 0 ;
}

void	StackTrace	(void)
{
	//	Category:	Diagnostics
	//
	//	Provides the current stack status for the current thread.
	//
	//	Arguments:	None
	//	Returns:	None

	hzProcess*	phz ;		//	Current thread

	phz = GetThreadInfo() ;
	if (!phz)
		Fatal("StackTrace. (tid=%u) cannot find it's own thread (no previous call to hzProcess contructor)\n", pthread_self()) ;

	phz->StackTrace() ;
}

void	CallHistory	(void)
{
	//	Category:	Diagnostics
	//
	//	Provides the recent function call history for the current thread.
	//
	//	Arguments:	None
	//	Returns:	None

	hzProcess*	phz ;		//	Current thread

	phz = GetThreadInfo() ;
	if (!phz)
		Fatal("CallHistory. (tid=%u) cannot find it's own thread (no previous call to hzProcess contructor)\n", pthread_self()) ;

	phz->CallHistory() ;
}

hzLogger*	GetThreadLogger	(void)
{
	//	Category:	Diagnostics
	//
	//	Get the hzLogger pointer for the current thread. This is achieved by finding the hzProcess instance associated with the current
	//	thread and from there, the hzLogger. If there is no hzLogger for the current thread this function will return the hzLogger from
	//	the main thread of the program (if any).
	//
	//	Arguments:	None
	//
	//	Returns:	Pointer to the logger of the current thread
	//				NULL if no logger is in force

	hzLogger*	pLog = 0 ;	//	Current thread logger
	hzProcess*	phz ;		//	Current thread

	phz = GetThreadInfo() ;
	if (phz)
		pLog = phz->GetLog() ;
	if (!pLog)
		pLog = s_pDfltLog ;

	return pLog ;
}

void	SetThreadLogger	(hzLogger* pLog)
{
	//	Category:	Diagnostics
	//
	//	Set the hzLogger pointer for the current thread.
	//
	//	Arguments:	1)	pLog	Pointer to the hzLogger instance declared for the main process or a separately created thread.
	//	Returns:	None

	hzProcess*	phz ;		//	Current thread

	phz = GetThreadInfo() ;
	if (!phz)
		Fatal("SetThreadLogger. (tid=%u) cannot find it's own thread (no previous call to hzProcess contructor)\n", pthread_self()) ;
	if (!phz->GetLog())
		phz->SetLog(pLog) ;

	if (!s_pDfltLog)
		s_pDfltLog = pLog ;
}

uint32_t	ActiveThreadCount	(void)
{
	//	Category:	Diagnostics
	//
	//	Arguments:	None
	//	Returns:	Number of thread currently registered

	return s_num_threads ;
}

/*
**	SECTION 2:	System/Process Stuff
*/

hzEcode	HadronZooInitEnv	(void)
{
	//	Category:	Process
	//
	//	Read the environment variables $HOME and $HADRONZOO
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOTFOUND	If either environment variable is not set
	//				E_OK		If both are set

	char*	i ;
	char*	j ;

	if (_hzGlobal_HOME && _hzGlobal_HADRONZOO)
		return E_OK ;

	if (!_hzGlobal_HOME)
		_hzGlobal_HOME = i = getenv("HOME") ;
	if (!_hzGlobal_HADRONZOO)
		_hzGlobal_HADRONZOO = j = getenv("HADRONZOO") ;

	printf("HAVE HOME=%s, HADRONZOO=%s\n", i, j) ;

	if (_hzGlobal_HOME && _hzGlobal_HADRONZOO)
		return E_OK ;
	return E_NOTFOUND ;
}

void	Demonize	(void)
{
	//	Category:	Process
	//
	//	Make program operate as a demon.
	//	Assumes UNIX type OS. Call very early on the the program. Has built in code to prevent multiple execution
	//
	//	Arguments:	None
	//	Returns:	None

	static	bool	bBeenHere = false ;	//	Already called marker

	if (bBeenHere)
	{
		std::cerr << "Attempt to call Demonize more than once" << std::endl ;
		return ;
	}

	bBeenHere = true ;

	pid_t	pid ;	//	Our process ID
	pid_t	sid ;	//	Our session ID

	//	Fork off the parent process
	pid = fork();
	if (pid < 0)
	{
		std::cerr << "Parent [" << getpid() << "] begets crap PID [" << pid << "] - exiting" << std::endl ;
		exit(EXIT_FAILURE);
	}

	//	If we got a good PID, then we can exit the parent process.
	if (pid > 0)
	{
		std::cout << "Exiting parent [" << getpid() << "] starting [" << pid << "]" << std::endl ;
		exit(EXIT_SUCCESS);
	}

	//	Change the file mode mask
	umask(0);

	//	Create a new SID for the child process
	sid = setsid();
	if (sid < 0)
	{
		std::cout << "Parent [" << getpid() << "] begets bad SID of [" << sid << "]" << std::endl ;
		exit(EXIT_FAILURE);
	}

	//	Close out the standard file descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

/*
**	Special function to make singleton process
*/

void	SingleProc	(void)
{
	//	Category:	Process
	//
	//	Assumes UNIX type OS. This ensures that only one instance of the calling application can run at any given time. Using the process id of the
	//	current (calling) process it obtains the link to the executable file from the /proc directory by calling stat on /proc/process_id/exe. It
	//	then examines all other sub-directories in /proc that are wholly numeric in name (the criteria for identifying current processes) to see if
	//	calling stat on subdir/exe results in a link to the same executable. If it does exit is called. If not this function simply returns allowing
	//	the calling process to proceed.
	//
	//	Arguments:	None
	//	Returns:	None

	FSTAT		fs ;				//	File status
	dirent*		pDE ;				//	Directory entry meta data
	DIR*		pDir ;				//	Operating directory
	uint32_t	ino ;				//	Inode number of directory entry
	uint32_t	cpid ;				//	Current process id
	uint32_t	xpid ;				//	Process id derived from sub-dirs of /proc
	char		buf	[512] ;			//	Directory entry name buffer

	/*
 	**	Obtain current executable
	*/

	cpid = getpid() ;
	sprintf(buf, "/proc/%d/exe", cpid) ;

	if (lstat(buf, &fs) == -1)
	{
		std::cerr << "Could not stat " << buf << std::endl ;
		exit(-1) ;
	}
	ino = fs.st_ino ;

	/*
 	**	Open the /proc directory and read it.
	*/

	pDir = opendir("/proc") ;
	if (!pDir)
	{
		std::cerr << "Cannot examine processes" << std::endl ;
		exit(-3) ;
	}

	for (; pDE = readdir(pDir) ;)
	{
		//	Eliminate this directory and the parent
		if (pDE->d_name[0] == '.')
			continue ;

		//	Eliminate all non numeric directories
		if (!IsAllDigits(pDE->d_name))
			continue ;

		xpid = atoi(pDE->d_name) ;

		//	Ignore self
		if (xpid == cpid)
			continue ;

		sprintf(buf, "/proc/%d/exe", xpid) ;
		if (lstat(buf, &fs) == -1)
			std::cout << "Could not stat " << buf << std::endl ;
		else
		{
			if (fs.st_ino == ino)
			{
				std::cout << "pid: " << cpid << " - Have located an existing process of " << xpid << std::endl ;
				std::cerr << "Both the existing and current process involve executable with inode of " << ino << std::endl ;
				exit(-4) ;
			}
		}
	}
	closedir(pDir) ;
}

hzEcode	ListProc	(hzMapS<uint32_t,hzProginfo*>& procs)
{
	//	Category:	Diagnostics
	//
	//	Examines the /proc directory to establish all the processes currently running on the machine. The function populates a map of process ids
	//	to hzProginfo instances.
	//
	//	Arguments:	1)	procs	This is a reference to a hzMapS<uint32_t,hzProginfo*> that this query function will populate.
	//
	//	Returns:	E_CORRUPT	If there was an error reading the /proc directory (see note)
	//				E_OK		If the operation was successful
	//
	//	Note this function will fail only if the /proc directory cannot be read or there is a duplicate entry in it. As the /proc is normally both readable and
	//	executable by everyone and Linux/Unix has few bugs, neither of these events ever acually happens.

	_hzfunc("ListProc") ;

	hzProginfo*	pPI ;				//	Program information (from /proc)

	FSTAT		fs ;				//	File status
	dirent*		pDE ;				//	Directory entry meta data
	DIR*		pDir ;				//	Operating directory
	uint32_t	ino ;				//	Inode number of directory entry
	uint32_t	cpid ;				//	Current process id
	uint32_t	xpid ;				//	Process id derived from sub-dirs of /proc
	uint32_t	nIndex ;			//	General iterator
	char		buf	[512] ;			//	Directory entry name buffer

	/*
 	**	Obtain current executable
	*/

	//	Clear tables
	for (nIndex = 0 ; nIndex < procs.Count() ; nIndex++)
	{
		pPI = procs.GetObj(nIndex) ;
		delete pPI ;
	}
	procs.Clear() ;

 	//	Open the /proc directory and read it.
	pDir = opendir("/proc") ;
	if (!pDir)
		return hzerr(_fn, HZ_ERROR, E_CORRUPT, "Could not open the /proc directory") ;

	for (; pDE = readdir(pDir) ;)
	{
		//	Eliminate self
		if (pDE->d_name[0] == '.')
			continue ;

		//	Eliminate all non numeric directories
		if (!IsAllDigits(pDE->d_name))
			continue ;

		xpid = atoi(pDE->d_name) ;

		pPI = procs[xpid] ;
		if (pPI)
			return hzerr(_fn, HZ_ERROR, E_SETONCE, "We already have process %d in the process list", xpid) ;

		pPI = new hzProginfo() ;
		pPI->m_PID = xpid ;

		sprintf(buf, "/proc/%d/exe", xpid) ;
		if (lstat(buf, &fs) == -1)
			std::cout << "Could not stat " << buf << std::endl ;
		else
		{
			if (fs.st_ino == ino)
			{
				std::cout << "pid: " << cpid << " - Have located an existing process of " << xpid << std::endl ;
				std::cerr << "Both the existing and current process involve executable with inode of " << ino << std::endl ;
				exit(-4) ;
			}
		}

		procs.Insert(pPI->m_PID, pPI) ;
	}
	closedir(pDir) ;

	return E_OK ;
}

/*
**	SECTION 3:	Shared Memory
*/

hzEcode	hzShmem::Init	(const char* name, uint32_t size, uint32_t mask)
{
	//	Category:	System
	//
	//	Initialize a segment of persistant shared memory.
	//
	//	Arguments:	1)	name	Name of segment
	//				2)	size	No of bytes
	//				3)	mask	Access permissions
	//
	//	Returns:	E_INITDUP	If shared memory segment is already initialized
	//				E_ARGUMENT	If no name is supplied
	//				E_INITFAIL	If the shared memory segment could not be set to the requested size
	//
	//	Note that as shared memory is a system wide rather than an application wide resource, the rules concerning program Unix User ID and access
	//	permissions apply. 

	_hzfunc("hzShmem::Init") ;

	//	Test the instance is not already initialized
	if (m_name)
		return hzerr(_fn, HZ_ERROR, E_INITDUP, "Shared memory segment %s already initialized", *m_name) ;
	if (!name || !name[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No name supplied") ;

	m_name = name ;
	m_size = ((size/4096)+1)*4096 ;
	m_perm = mask ;

	//  Open shared memory
	m_fd = shm_open(name, O_RDWR | O_CREAT, 0666) ;
	threadLog("%s: Set fd to %d\n", *_fn, m_fd) ;

	//	Set to requested size
	if (ftruncate(m_fd, m_size) != 0)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Shared memory could not be sized to %d bytes", m_size) ;
	threadLog("%s: Set size to %d\n", *_fn, m_size) ;
 
	//	Obtain pointer to start of segment
	m_pStart = mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
	if (!m_pStart || m_pStart == (void*)-1)
		return hzerr(_fn, HZ_ERROR, E_INITFAIL, "Shared memory address at %p errno is %d\n", m_pStart, errno) ;
	threadLog("%s: mem at %p\n", *_fn, m_pStart) ;

	return E_OK ;
}

uint32_t	hzShmem::Size	(void)
{
	//	Obtain the size of the shared memory segment
	//
	//	Arguments:	None
	//	Returns:	Total shared segment size

	shmid_ds	shmCtl ;	//	For reading shared memory info

	shmctl(m_fd, IPC_STAT, &shmCtl) ;
	return shmCtl.shm_segsz ;
}

/*
**	SECTION 4:	Thread diodes (lock free que)
*/

hzDiode::_bkt*	hzDiode::_alloc  (void)
{
	//	Allocate a diode bucket from a freelist regime
	//
	//	Arguments:	None
	//	Returns:	Pointer to the allocated bucket

    _bkt*   pB ;    //  Current bucket

    pB = m_pFree ;
    if (pB)
        m_pFree = m_pFree->next ;
    else
        pB = new _bkt() ;

    if (!m_pFirst)
        m_pFirst = pB ;
    if (!m_pLast)
        m_pLast = pB ;
    return pB ;
}

void	hzDiode::_free   (hzDiode::_bkt* pB)
{
	//	Put current block on free list
	//
	//	Arguments:	1)	pB	Diode bucket to be released to the free list
	//	Returns:	None

    pB->next = m_pFree ;
    m_pFree = pB ;
}

hzDiode::~hzDiode	(void)
{
	//	Delete this hzDiode instance, removing all allocated data object buckets

    _bkt*   pB ;    //  Current bucket
    _bkt*   pX ;    //  Next in series

    for (pB = m_pFirst ; pB ; pB = pX)
    {
        pX = pB->next ;
        delete pB ;
    }
}

void	hzDiode::Push	(void* pObj)
{
	//	Push data object into the to be processed queue. This is done by the object originator thread.
	//
	//	Arguments:	1)	pObj	Object to be handled by the next stage thread
	//	Returns:	None

	_bkt*	pB ;	//	Diode bucket pointer

	if (!m_pLast)
		pB = _alloc() ;
	else
		pB = m_pLast ;

	if (pB->usage == 30)
	{
		pB = _alloc() ;
		m_pLast->next = pB ;
		m_pLast = pB ;
	}

	pB->items[pB->usage] = pObj ;
	pB->usage++ ;

	if (!m_pLast)
		m_pFirst = m_pLast = pB ;
}

void*	hzDiode::Pull	(void)
{
	//	Pull data object from the to be processed queue. This is strickly done by the object receptor and processing thread.
	//
	//	Arguments:	None
	//	Returns:	Pointer to the process object

	_bkt*	pB ;	//	Diode bucket pointer
	void*	pObj ;	//	Objetc

	pB = m_pFirst ;
	if (!pB)
		return 0 ;
	if (pB->count == pB->usage)
	{
		if (!pB->next)
			return 0 ;

		//	For there to be a next bucket, this buckut must nessesarily be full
		pB = pB->next ;
		_free(m_pFirst) ;
		m_pFirst = pB ;
	}

	pObj = pB->items[pB->count] ;
	pB->count++ ;

	return pObj ;
}

//	SECTION 5:	Signals
//
//	Signals described in the original POSIX.1-1990 standard.
//
//	Signal     Value     Action   Comment
//	-------------------------------------
//	SIGHUP        1      Term	Hangup detected on controlling terminal or death of controlling process
//	SIGINT        2      Term	Interrupt from keyboard
//	SIGQUIT       3      Core	Quit from keyboard
//	SIGILL        4      Core	Illegal Instruction
//	SIGABRT       6      Core	Abort signal from abort(3)
//	SIGFPE        8      Core	Floating point exception
//	SIGKILL       9      Term	Kill signal
//	SIGSEGV      11      Core	Invalid memory reference
//	SIGPIPE      13      Term	Broken pipe: write to pipe with no readers
//	SIGALRM      14      Term	Timer signal from alarm(2)
//	SIGTERM      15      Term	Termination signal
//	SIGUSR1   30,10,16   Term	User-defined signal 1
//	SIGUSR2   31,12,17   Term	User-defined signal 2
//	SIGCHLD   20,17,18   Ign	Child stopped or terminated
//	SIGCONT   19,18,25   Cont	Continue if stopped
//	SIGSTOP   17,19,23   Stop	Stop process
//	SIGTSTP   18,20,24   Stop	Stop typed at terminal
//	SIGTTIN   21,21,26   Stop	Terminal input for background process
//	SIGTTOU   22,22,27   Stop	Terminal output for background process
//
//	Note: The signals SIGKILL and SIGSTOP cannot be caught, blocked, or ignored.
//
//	Signals not in the POSIX.1-1990 standard but described in SUSv2 and POSIX.1-2001.
//
//	Signal       Value   Action   Comment
//	-------------------------------------
//	SIGBUS      10,7,10  Core	Bus error (bad memory access)
//	SIGPOLL              Term	Pollable event (Sys V).  Synonym for SIGIO
//	SIGPROF     27,27,29 Term	Profiling timer expired
//	SIGSYS      12,31,12 Core	Bad argument to routine (SVr4)
//	SIGTRAP        5     Core	Trace/breakpoint trap
//	SIGURG      16,23,21 Ign	Urgent condition on socket (4.2BSD)
//	SIGVTALRM   26,26,28 Term	Virtual alarm clock (4.2BSD)
//	SIGXCPU     24,24,30 Core	CPU time limit exceeded (4.2BSD)
//	SIGXFSZ     25,25,31 Core	File size limit exceeded (4.2BSD)
//
//	Up to and including Linux 2.2, the default behavior for SIGSYS, SIGXCPU, SIGXFSZ, and (on architectures other than SPARC and MIPS) SIGBUS was to
//	terminate the process (without a core dump). (On some other UNIX systems the default action for SIGXCPU and SIGXFSZ is to terminate the process
//	without a core dump.)  Linux 2.4 conforms to the POSIX.1-2001 requirements for these signals, terminating the process with a core dump.
//
//	Remaining other signals.
//
//	Signal       Value   Action   Comment
//	-------------------------------------
//	SIGIOT         6     Core    IOT trap. A synonym for SIGABRT
//	SIGEMT       7,-,7   Term
//	SIGSTKFLT    -,16,-  Term    Stack fault on coprocessor (unused)
//	SIGIO       23,29,22 Term    I/O now possible (4.2BSD)
//	SIGCLD       -,-,18  Ign     A synonym for SIGCHLD
//	SIGPWR      29,30,19 Term    Power failure (System V)
//	SIGINFO      29,-,-             A synonym for SIGPWR
//	SIGLOST      -,-,-   Term    File lock lost (unused)
//	SIGWINCH    28,28,20 Ign     Window resize signal (4.3BSD, Sun)
//	SIGUNUSED    -,31,-  Core    Synonymous with SIGSYS
//
//	Notes:
//
//	- (Signal 29 is SIGINFO / SIGPWR on an alpha but SIGLOST on a sparc.)
//	- SIGEMT is not specified in POSIX.1-2001, but nevertheless appears on most other UNIX systems, where its default action is typically to terminate
//	the process with a core dump.
//	- SIGPWR (which is not specified in POSIX.1-2001) is typically ignored by default on those other UNIX systems where it appears.
//	- SIGIO (which is not specified in POSIX.1-2001) is ignored by default on several other UNIX systems.
//
//	Where defined, SIGUNUSED is synonymous with SIGSYS on most architectures.
//
//	Real-time signals Starting with version 2.2, Linux supports real-time signals as originally defined in the POSIX.1b real-time extensions (and now
//	included in POSIX.1-2001).  The range of supported real-time signals is defined by the macros SIGRTMIN and SIGRTMAX.  POSIX.1-2001 requires that
//	an implementation support at least _POSIX_RTSIG_MAX (8) real-time signals.
//
//	The Linux kernel supports a range of 33 different real-time signals, numbered 32 - 64. However, the glibc POSIX threads implementation internally
//	uses two (for NPTL) or three (for LinuxThreads) real-time signals (see pthreads(7)), and adjusts the value of SIGRTMIN suitably (to 34 or 35).
//	Because the range of available real-time signals varies according to the glibc threading implementation (and this variation can occur at run time
//	according to the available kernel and glibc), and indeed the range of real-time signals varies across UNIX systems, programs should never refer to
//	real-time signals using hard-coded numbers, but instead should always refer to real-time signals using the notation SIGRTMIN+n, and include suitable
//	(run-time) checks that SIGRTMIN+n does not exceed SIGRTMAX.
//
//	Unlike standard signals, real-time signals have no predefined meanings: the entire set of real-time signals can be used for application-defined purposes.
//
//	The default action for an unhandled real-time signal is to terminate the receiving process. Real-time signals are distinguished by the following:
//
//	1.  Multiple instances of real-time signals can be queued. By contrast, if multiple instances of a standard signal are delivered while that signal is
//	currently blocked, then only one instance is queued.
//
//	2.  If the signal is sent using sigqueue(3), an accompanying value (either an integer or a pointer) can be sent with the signal. If the receiving
//	process establishes a handler for this signal using the SA_SIGINFO flag to sigaction(2), then it can obtain this data via the si_value field of the
//	siginfo_t structure passed as the second argument to the handler.  Furthermore, the si_pid and si_uid fields of this structure can be used to obtain
//	the PID and real user ID of the process sending the signal.
//
//	3.  Real-time signals are delivered in a guaranteed order.  Multiple real-time signals of the same type are delivered in the order they were sent.
//	If different real-time signals are sent to a process, they are delivered starting with the lowest-numbered signal. (I.e., low-numbered signals
//	have highest priority.)  By contrast, if multiple standard signals are pending for a process, the order in which they are delivered is unspecified.
//
//	If both standard and real-time signals are pending for a process, POSIX leaves it unspecified which is delivered first.  Linux, like many other
//	implementations, gives priority to standard signals in this case.
//
//	According to POSIX, an implementation should permit at least _POSIX_SIGQUEUE_MAX (32) real-time signals to be queued to a process. However, Linux
//	does things differently.  In kernels up to and including 2.6.7, Linux imposes a system-wide limit on the number of queued real-time signals for all
//	processes.  This limit can be viewed and (with privilege) changed via the /proc/sys/kernel/rtsig-max file. A related file, /proc/sys/kernel/rtsig-nr,
//	can be used to find out how many real-time signals are currently queued.  In Linux 2.6.8, these /proc interfaces were replaced by the RLIMIT_SIGPENDING
//	resource limit, which specifies a per-user limit for queued signals; see setrlimit(2) for further details.
//
//	The addition or real-time signals required the widening of the signal set structure (sigset_t) from 32 to 64 bits.  Consequently, various system calls
//	were superseded by new system calls that supported the larger signal sets.  The old and new system calls are as follows:
//

static	const char*	s_signals	[] =
{
    "SIGNAL (00) UNUSED",
    "SIGNAL (01) SIGHUP",
    "SIGNAL (02) SIGINT",
    "SIGNAL (03) SIGQUIT",
    "SIGNAL (04) SIGILL",
    "SIGNAL (05) SIGTRAP",
    "SIGNAL (06) SIGABRT",
    "SIGNAL (07) SIGBUS",
    "SIGNAL (08) SIGFPE",
    "SIGNAL (09) SIGKILL",
    "SIGNAL (10) SIGUSR1",
    "SIGNAL (11) SIGSEGV",
    "SIGNAL (12) SIGUSR2",
    "SIGNAL (13) SIGPIPE",
    "SIGNAL (14) SIGALRM",
    "SIGNAL (15) SIGTERM",
	"SIGNAL_(16) UNUSED",
    "SIGNAL (17) SIGCHLD",
    "SIGNAL (18) SIGCONT",
    "SIGNAL (19) SIGSTOP",
	""
} ;

void	CatchSegVio	(int32_t sig)
{
	//	Category:	Process
	//
	//	Signal handler for segmentation error. Assumes UNIX type OS.
	//
	//	Arguments:	1)	sig		The incoming signal.
	//	Returns:	None
	//
	//	Note:	This function is never called by the application but is instead passed as a function pointer to signal during initialization where it
	//			surplants the default interupt handler for the given signal.

	hzLogger*	pLog ;			//	Current thread logger
	hzProcess*	phz ;			//	Current thread info
	void*		pvArr[1024];	//	Backtrace
	char**		cpSym ;			//	Backtrace symbols
	uint32_t	pid ;			//	Process id
	uint32_t	tid ;			//	Thread id
	uint32_t	nSize ;			//	Backtrace array size
	uint32_t	nIndex ;		//	Symbols iterator

	printf("SEGMENT VIOLATION\n") ;
	fflush(stdout);

	nSize = backtrace(pvArr, 1023) ;
	cpSym = backtrace_symbols(pvArr, nSize);

	pid = getpid() ;
	tid = pthread_self() ;
	pLog = GetThreadLogger() ;
	phz = GetThreadInfo() ;

	if (!pLog)
	{
		printf("1 CatchSegVio: %s\n", s_signals[sig]) ;
		printf("1 CatchSegVio: Signal %d Process %u, thread %u\n", sig, pid, tid) ;
		printf("1 Stack Trace is:\n") ;

		for (nIndex = 0 ; nIndex < nSize ; nIndex++)
			printf("%d - %s\n", nIndex, cpSym[nIndex]) ;

		printf("1 Stack Trace end:\n") ;
		fflush(stdout);
		phz->StackTrace() ;
	}
	else
	{
		pLog->Out("2 CatchSegVio: %s\n", s_signals[sig]) ;
		pLog->Out("2 CatchSegVio: Signal %d Process %u, thread %u\n", sig, pid, tid) ;
		pLog->Out("2 Stack Trace is:\n") ;

		for (nIndex = 0 ; nIndex < nSize ; nIndex++)
			pLog->Out("%d - %s\n", nIndex, cpSym[nIndex]) ;

		phz->StackTrace() ;
		pLog->Out("2 Stack Trace end:\n") ;

		phz->CallHistory() ;
		pLog->Out("2 Call History end:\n") ;
	}

	//	free(cpSym);
	exit(100) ;
}

void	CatchCtrlC	(int32_t sig)
{
	//	Category:	Process
	//
	//	Signal handler for Ctrl-C (SIGINT)
	//
	//	Arguments:	1)	sig		The incoming signal.
	//	Returns:	None
	//
	//	Note:	This function is never called by the application but is instead passed as a function pointer to signal during initialization where it
	//			surplants the default interupt handler for the given signal.

	hzLogger*	pLog ;			//	Current thread logger
	hzProcess*	phz ;			//	Current thread info
	void*		pvArr[1024];	//	Backtrace
	char**		cpSym ;			//	Backtrace symbols
	uint32_t	pid ;			//	Process id
	uint32_t	tid ;			//	Thread id
	uint32_t	nSize ;			//	Backtrace array size
	uint32_t	nIndex ;		//	Symbols iterator

	pid = getpid() ;
	tid = pthread_self() ;
	nSize = backtrace(pvArr, 1023) ;
	cpSym = backtrace_symbols(pvArr, nSize);
	pLog = GetThreadLogger() ;
	phz = GetThreadInfo() ;

	if (!pLog)
	{
		printf("CatchCtrlC: %s\n", s_signals[sig]) ;
		printf("CatchCtrlC: Signal %d Process %u, thread %u\n", sig, pid, tid) ;
		printf("Stack Trace is:\n") ;

		for (nIndex = 0 ; nIndex < nSize ; nIndex++)
			puts(cpSym[nIndex]);;

		printf("Stack Trace end:\n") ;
		fflush(stdout);
		phz->StackTrace() ;
	}
	else
	{
		pLog->Record("CatchCtrlC: %s\n", s_signals[sig]) ;
		pLog->Out("Signal %d Process %u, thread %u\n", sig, pid, tid) ;
		pLog->Out("Stack Trace is:\n") ;

		for (nIndex = 0 ; nIndex < nSize ; nIndex++)
			pLog->Out("%s\n", cpSym[nIndex]) ;

		phz->StackTrace() ;
		pLog->Out("Stack Trace end:\n") ;

		phz->CallHistory() ;
		pLog->Out("Call History end:\n") ;
	}

	//free(cpSym);

	//	re-set the signal handler again to this function for next time
	if (sig == 2)
		signal(sig, 0);
	else
		signal(sig, CatchCtrlC);
}
