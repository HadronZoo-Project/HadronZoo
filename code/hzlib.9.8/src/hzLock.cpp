//
//	File:	hzLock.cpp
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
#include <sys/ipc.h>
#include <sys/sem.h>

#include <sys/stat.h>

#include "hzTextproc.h"
#include "hzLock.h"
#include "hzDirectory.h"

/*
**	Variables
*/

static	hzLockS		s_crMtx ;			//	Mutex to control creation of mutexes!
static	hzLockRWD*	s_allMtx = 0 ;		//	First link in linked list of all active mutext (with it's own mutex deactivated)
static	hzLockRWD*	s_lastMtx = 0 ;		//	Last link in linked list of all active mutext (with it's own mutex deactivated)
static	uint32_t	s_SeqMutex = 100 ;	//	For unique ids of hzLockRWD instances

/*
**	SECTION 1:	hzLockS. Simple Lock
*/

hzEcode	hzLockS::Lock	(int32_t nTries)
{
	//	Obtain a lock on a resource. This will spin until either the lock is granted or the lock is deactivated (host entity destructed)
	//
	//	Arguments:	1)	nTries	Number of retries (in thousands) before abandoning efforts to obtain the lock
	//
	//	Returns:	E_NOTFOUND	If the lock has been killed by a previous holder
	//				E_TIMEOUT	If the lock has not been release within nTries
	//				E_OK		If the lock is granted
	//
	//	Note: This will return E_OK immeadiately if _hzGlobal_MT is false (the program is single threaded)

	uint32_t	cont ;		//	Contention or spin count
	uint32_t	tries ;		//	Contention or spin count
	uint32_t	tid ;		//	Thread id
	uint32_t	limit ;		//	Timeout as number of tries

	if (!_hzGlobal_MT)
		return E_OK ;

	tid = pthread_self() ;
	limit = nTries < 0 ? 0xfffffffe : nTries * 1000 ;

	if (m_lockval == tid)
		Fatal("hzLockS::hzLockS. Attempt by thread %u to re-lock address %p\n", tid, &m_lockval) ;

	for (tries = cont = 0 ;;)
	{
		if (m_lockval == 0xffffffff)
			return E_NOTFOUND ;

		if (m_lockval)
			{ cont++ ; continue ; }

		if (!__sync_val_compare_and_swap(&m_lockval, 0, tid))
		{
			if (m_lockval == tid)
				break ;
		}

		tries++ ;
		if (tries > limit)
			return E_TIMEOUT ;
	}

	m_lockval = tid ;
	return E_OK ;
}

void	hzLockS::Kill	(void)
{
	//	Releases a lock on a resource but instead of leaving the lock free (a value of 0), it sets the lock to invalid (a value of 0xffffffff). This signals to
	//	any thread waiting on the lock, that it must abandon the intended operation on the resource protected by the lock. Kill is called where the resourse is
	//	about to be deleted.
	//
	//	Arguments:	None
	//	Returns:	None
	//
	//	Note this operation terminates execution if the current thread is trying to unlock a resource that is locked by another thread or not locked at all

	uint32_t	tid ;		//	Caller thread id

	if (_hzGlobal_MT)
	{
		tid = pthread_self() ;

		if (!m_lockval)
			Fatal("hzLockS::hzKill. Attempt by thread %u to kill unaquired lock\n", tid) ;

		if (m_lockval == 0xffffffff)
			Fatal("hzLockS::hzKill. Attempt by thread %u to kill a deprecated lock\n", tid) ;

		if (m_lockval != tid)
			Fatal("hzLockS::hzKill. Attempt by thread %u to kill lock aquired by thread (%u)\n", tid, m_lockval) ;

		m_lockval = 0xffffffff ;
	}
}

void	hzLockS::Unlock	(void)
{
	//	Release a lock on a resource
	//
	//	Arguments:	None
	//	Returns:	None
	//
	//	Note this operation terminates execution if the current thread is trying to unlock a resource that is locked by another thread or not locked at all

	uint32_t	tid ;		//	Caller thread id

	if (_hzGlobal_MT)
	{
		tid = pthread_self() ;

		if (!m_lockval)
			Fatal("Attempt by thread %u to unlock address %p that is not locked by any thread", tid, &m_lockval) ;

		if (m_lockval == 0xffffffff)
			Fatal("hzLockS::hzKill. Attempt by thread %u to unlock a deprected lock\n", tid) ;

		if (m_lockval != tid)
			Fatal("Attempt by thread %u to unlock address %p that is locked by another thread (%u)", tid, &m_lockval, m_lockval) ;

		m_lockval = 0 ;
	}
}

/*
**	SECTION 2:	hzLockRW. Read/Write lock
*/

hzEcode	hzLockRW::LockWrite	(int32_t nTries)
{
	//	Obtain a write lock on a resource. This will spin until either the lock is granted or the lock is deactivated (host entity destructed). And
	//	then spin until the read count falls to zero. This latter step ensures no thread can read the resource during a write.
	//
	//	Arguments:	1)	nTries	Number of retries (in thousands) before abandoning efforts to obtain the lock
	//
	//	Returns:	E_NOTFOUND	The previous thread with write access killed the lock (should be because it deleted the applicable resource)
	//				E_TIMEOUT	The thread holding write access has held the lock for too long
	//				E_OK		Write access granted

	uint32_t	cont ;		//	Contention or spin count
	uint32_t	tries ;		//	Contention or spin count
	uint32_t	tid ;		//	Thread id
	uint32_t	limit ;		//	Timeout as number of tries

	if (!_hzGlobal_MT)
		return E_OK ;

	tid = pthread_self() ;
	limit = nTries < 0 ? 0xfffffffe : nTries * 1000 ;

	if (m_lockval == tid)
		Fatal("hzLock::hzLock. Attempt by thread %u to re-lock address %p\n", tid, &m_lockval) ;

	//	Wait for lock to become free (no other threads with write access)
	for (tries = cont = 0 ;;)
	{
		if (m_lockval == 0xffffffff)
			return E_NOTFOUND ;

		if (m_lockval)
			{ cont++ ; continue ; }

		//	Try to grab the lock
		if (!__sync_val_compare_and_swap(&m_lockval, 0, tid))
		{
			if (m_lockval == tid)
				break ;
		}

		//	Check for timeout
		tries++ ;
		if (tries > limit)
			return E_TIMEOUT ;
	}

	//	Wait for counter to drop to zero
	for (; m_counter ; cont++) ;
	return E_OK ;
}

hzEcode	hzLockRW::LockRead	(int32_t timeout)
{
	//	Obtain a read lock on a resource. This is only spin if there is a write lock in place.
	//
	//	Arguments:	1)	timout	The timeout limit (limit of retries in thousands)
	//
	//	Returns:	E_NOTFOUND	The previous thread with write access killed the lock (should be because it deleted the applicable resource)
	//				E_TIMEOUT	The thread holding write access has held the lock for too long
	//				E_OK		Write access granted

	uint32_t	cont ;		//	Contention or spin count
	uint32_t	tid ;		//	Caller thread id

	if (!_hzGlobal_MT)
		return E_OK ;

	tid = pthread_self() ;

	if (m_lockval == 0xffffffff)
		return E_NOTFOUND ;

	if (m_lockval == tid)
		Fatal("hzLock::hzLock. Attempt by thread %u to re-lock address %p\n", tid, &m_lockval) ;

	//	Spin until the lock is free (m_lockval == 0)
	for (cont = 0 ; m_lockval ; cont++)
	{
		if (m_lockval == 0xffffffff)
			return E_NOTFOUND ;
		if (!m_lockval)
			break ;
	}

	//	Increment count of read locks obtained
	while (__sync_add_and_fetch(&m_counter, 1)) ;
	return E_OK ;
}

void	hzLockRW::Kill	(void)
{
	//	Kill a lock (signal that the resouce controlled by the lock is to be deleted). The calling thread must hold the write lock. This function will terminate
	//	the program if this is not the case or if the lock has already been killed.
	//
	//	Arguments:	None
	//	Returns:	None

	uint32_t	tid ;		//	Caller thread id

	if (_hzGlobal_MT)
	{
		tid = pthread_self() ;

		if (!m_lockval)
			Fatal("hzLockRW::hzKill. Attempt by thread %u to kill unaquired lock\n", tid) ;

		if (m_lockval == 0xffffffff)
			Fatal("hzLockRW::hzKill. Attempt by thread %u to kill a deprecated lock\n", tid) ;

		if (m_lockval != tid)
			Fatal("hzLockRW::hzKill. Attempt by thread %u to kill lock aquired by thread (%u)\n", tid, m_lockval) ;

		m_lockval = 0xffffffff ;
	}
}

void	hzLockRW::Unlock	(void)
{
	//	Release a lock on a resource. This could be either a write lock where the lock value equals the thread id of the calling thread, or a read lock where it
	//	does not. If the lock value equals this thread id, this thread has the write lock and the action is simply to release it and return. There is no need to
	//	use a sync function to do this as no other thread can have write access and no need to check the counter as no other thread can obtain read access while
	//	any thread has the write lock.
	//
	//	If the lock value does not equal this thread id this thread only has a read lock so the action is only to decrement the lock using sync. The counter is
	//	checked beforehand to see if it is already zero. The program is terminated if the lock is already zero.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzLockRWD::Unlock") ;

	uint32_t	tid ;	//	Caller thread id

	if (_hzGlobal_MT)
	{
		if (m_lockval == 0xffffffff)
			hzexit(_fn, 0, E_CORRUPT, "Attempting to unlock a deprecated lock") ;

		tid = pthread_self() ;
		if (m_lockval == tid)
		{
			//	This thread has the write lock so just release it
			m_lockval = 0 ;
		}
		else
		{
			//	Assume a read lock decrement counter but do nothing if the lock has been killed
			if (m_lockval != 0xffffffff)
			{
				if (m_counter == 0)
					hzexit(_fn, 0, E_CORRUPT, "Lock count already zero") ;
				while (__sync_add_and_fetch(&m_counter, -1)) ;
			}
		}
	}
}

/*
**	SECTION 3:	hzLockRWD. Read-Write Lock with Diagnostics
*/

hzLockRWD::hzLockRWD	(void)
{
	//	hzLockRWD default constructor. Note that this assigns the mutex internal structure using malloc in order to avoid using new which is overloaded and
	//	itself controlled by a mutex etc!
	//
	//	Arguments:	None

	next = prev = 0 ;
	m_Granted = m_WaitThis = m_WaitTotal = m_Inuse = m_SpinsTotal = 0 ;
	m_SpinsThis = m_TriesTotal = m_TriesThis = m_LockOpsW = m_LockOpsR = m_Unlocks = 0 ;
	m_lockval = m_counter = 0 ;
	m_Id = m_recurse = 0 ;
	m_name[0] = 0 ;

	s_crMtx.Lock() ;

		m_Id = ++s_SeqMutex ;

		if (!s_allMtx)
			s_allMtx = s_lastMtx = this ;
		else
		{
			prev = s_lastMtx ;
			s_lastMtx->next = this ;
			s_lastMtx = this ;
		}

	s_crMtx.Unlock() ;
}

hzLockRWD::hzLockRWD	(const char* name)
{
	//	hzLockRWD default constructor but with name supplied for diagnostic purposes.
	//
	//	Arguments:	1)	name	Lock name

	next = prev = 0 ;
	m_Granted = m_WaitThis = m_WaitTotal = m_Inuse = m_SpinsTotal = 0 ;
	m_SpinsThis = m_TriesTotal = m_TriesThis = m_LockOpsW = m_LockOpsR = m_Unlocks = 0 ;
	m_lockval = m_counter = 0 ;
	m_Id = m_recurse = 0 ;
	m_name[0] = 0 ;

	s_crMtx.Lock() ;

		m_Id = ++s_SeqMutex ;
		strncpy(m_name, name, 15) ;

		if (!s_allMtx)
			s_allMtx = s_lastMtx = this ;
		else
		{
			prev = s_lastMtx ;
			s_lastMtx->next = this ;
			s_lastMtx = this ;
		}

	s_crMtx.Unlock() ;
}

hzLockRWD::~hzLockRWD	(void)
{
	//	hzLockRWD destructor.
	//
	//	Arguments:	None

	s_crMtx.Lock() ;

		if (s_allMtx == this)
			s_allMtx = s_allMtx->next ;
		if (this == s_lastMtx)
			s_lastMtx = s_lastMtx->prev ;

		if (prev)
			prev->next = next ;
		if (next)
			next->prev = prev ;

	s_crMtx.Unlock() ;
}

hzEcode	hzLockRWD::Setname	(const char* name)
{
	//	Set diagnostic name on lock as an explicit step. This can only be done once. Subsequent calls will have no effect
	//
	//	Arguments:	1)	name	Lock name
	//
	//	Returns:	E_SETONCE	If the name already set
	//				E_OK		If the operaton was successful

	if (m_name[0])
		return E_SETONCE ;
	strncpy(m_name, name, 15) ;
	return E_OK ;
}

void*		hzLockRWD::Address		(void)	{ return this ; }
uint64_t	hzLockRWD::Granted		(void)	{ return m_Granted ; }
uint64_t	hzLockRWD::WaitThis		(void)	{ return m_WaitThis ; }
uint64_t	hzLockRWD::WaitTotal	(void)	{ return m_WaitTotal ; }
uint64_t	hzLockRWD::Inuse		(void)	{ return m_Inuse ; }
uint64_t	hzLockRWD::SpinTotal	(void)	{ return m_SpinsTotal ; }
uint32_t	hzLockRWD::SpinThis		(void)	{ return m_SpinsThis ; }
uint32_t	hzLockRWD::TriesTotal	(void)	{ return m_TriesTotal ; }
uint32_t	hzLockRWD::TriesThis	(void)	{ return m_TriesThis ; }
uint32_t	hzLockRWD::Thread		(void)	{ return m_lockval ; }
uint32_t	hzLockRWD::Level		(void)	{ return m_recurse ; }
uint32_t	hzLockRWD::UID			(void)	{ return m_Id ; }
const char*	hzLockRWD::Name			(void)	{ return m_name ; }

hzEcode	hzLockRWD::LockWrite	(int32_t timeout)
{
	//	Obtain a write lock on a resource. This will spin until either the lock is granted or the lock is deactivated (host entity destructed). And
	//	then spin until the read count falls to zero. This latter step ensures no thread can read the resource during a write.
	//
	//	Arguments:	1)	timout	The limit of retries (in thousands)
	//
	//	Returns:	E_NOTFOUND	The previous thread with write access killed the lock (should be because it deleted the applicable resource)
	//				E_TIMEOUT	The thread holding write access has held the lock for too long
	//				E_OK		Write access granted

	_hzfunc("hzLockRWD::LockWrite") ;

	uint64_t	now ;		//	Time lock sought
	uint64_t	got ;		//	Time lock granted
	uint32_t	cont ;		//	Contention or spin count
	uint32_t	tries ;		//	Contention or spin count
	uint32_t	tid ;		//	Thread id
	uint32_t	limit ;		//	Timeout as number of tries

	if (!_hzGlobal_MT)
		return E_OK ;

	now = RealtimeNano() ;
	tid = pthread_self() ;
	limit = timeout < 0 ? 0xfffffffe : timeout * 1000 ;

	if (m_lockval == tid)
	{
		m_recurse++ ;
		//m_SpinLocksR++ ;
		m_LockOpsW++ ;
		m_SpinsThis = 0 ;
		return E_OK ;
	}

	for (tries = cont = 0 ;;)
	{
		if (m_lockval)
		{
			cont++ ;
			continue ;
		}

		if (!__sync_val_compare_and_swap(&(m_lockval), 0, tid))
		{
			if (m_lockval == tid)
				break ;
		}

		tries++ ;
		if (tries > limit)
			return E_TIMEOUT ;
	}

	//	Wait for counter to drop to zero
	for (; m_counter ; cont++) ;

	//	Note: Can only set m_lockval when we have the lock
	m_lockval = tid ;
	m_recurse = 0 ;
	//m_SpinLocksP++ ;
	m_LockOpsW++ ;
	m_TriesThis = tries ;
	m_TriesTotal += tries ;
	m_SpinsThis = cont ;
	m_SpinsTotal += cont ;

	got = RealtimeNano() ;

	m_Granted = got ;
	m_WaitThis = (got - now) ;
	m_WaitTotal += m_WaitThis ;
	return E_OK ;
}

hzEcode	hzLockRWD::LockRead	(int32_t nTries)
{
	//	Obtain a read lock on a resource. This is only spin if there is a write lock in place.
	//
	//	Arguments:	1)	timout	The timeout limit (limit of retries in thousands)
	//
	//	Returns:	E_NOTFOUND	The previous thread with write access killed the lock (should be because it deleted the applicable resource)
	//				E_TIMEOUT	The thread holding write access has held the lock for too long
	//				E_OK		Write access granted


	uint32_t	cont ;		//	Contention or spin count
	uint32_t	tid ;		//	Thread id

	if (!_hzGlobal_MT)
		return E_OK ;

	tid = pthread_self() ;

	if (m_lockval == 0xffffffff)
		return E_NOTFOUND ;

	if (m_lockval == tid)
		Fatal("hzLock::hzLock. Attempt by thread %u to re-lock address %p\n", tid, &m_lockval) ;

	for (cont = 0 ; m_lockval ; cont++)
	{
		if (m_lockval == 0xffffffff)
			return E_NOTFOUND ;
		if (!m_lockval)
			break ;
	}

	while (__sync_add_and_fetch(&m_counter, 1)) ;
	return E_OK ;
}

void	hzLockRWD::Kill	(void)
{
	//	Kill a lock (signal that the resouce controlled by the lock is to be deleted). The calling thread must hold the write lock. This function will terminate
	//	the program if this is not the case or if the lock has already been killed.
	//
	//	Arguments:	None
	//	Returns:	None

	uint32_t	tid ;		//	Caller thread id

	if (_hzGlobal_MT)
	{
		tid = pthread_self() ;

		if (!m_lockval)
			Fatal("hzLockRW::hzKill. Attempt by thread %u to kill unaquired lock\n", tid) ;

		if (m_lockval == 0xffffffff)
			Fatal("hzLockRW::hzKill. Attempt by thread %u to kill a deprecated lock\n", tid) ;

		if (m_lockval != tid)
			Fatal("hzLockRW::hzKill. Attempt by thread %u to kill lock aquired by thread (%u)\n", tid, m_lockval) ;

		m_lockval = 0xffffffff ;
	}
}

void	hzLockRWD::Unlock	(void)
{
	//	Release a lock on a resource
	//
	//	Arguments:	None
	//	Returns:	None
	//
	//	Note this operation terminates execution if the current thread is trying to unlock a resource that is locked by another thread or not locked at all

	_hzfunc("hzLockRWD::Unlock") ;

	uint32_t	tid ;	//	Caller thread id

	/*
	if (_hzGlobal_MT)
	{
		tid = pthread_self() ;

		if (!m_lockval)
			hzexit(_fn, E_CORRUPT, "Attempting to unlock mutex %d that is not locked by any thread", m_Id) ;

		if (m_lockval != tid)
			hzexit(_fn, E_CORRUPT, "Attempting to unlock mutex %d that is locked by another thread (%u)", m_Id, m_lockval) ;

		//	Must unset m_lockval before we release the lock
		if (m_recurse)
		{
			m_recurse-- ;
			m_UnlocksR++ ;
		}
		else
		{
			m_lockval = 0 ;
			m_UnlocksP++ ;
			m_Inuse += (RealtimeNano() - m_Granted) ;
		}
	}
	*/

	if (_hzGlobal_MT)
	{
		if (m_lockval == 0xffffffff)
			hzexit(_fn, 0, E_CORRUPT, "Attempting to unlock a deprecated lock") ;

		tid = pthread_self() ;
		if (m_lockval == tid)
		{
			//	This thread has the write lock so just release it
			m_lockval = 0 ;
		}
		else
		{
			//	Assume a read lock decrement counter but do nothing if the lock has been killed
			if (m_lockval != 0xffffffff)
			{
				if (m_counter == 0)
					hzexit(_fn, 0, E_CORRUPT, "Lock count already zero") ;
				while (__sync_add_and_fetch(&m_counter, -1)) ;
			}
		}
	}
}

void	ReportMutexContention	(hzChain& Z)
{
	//	Category:	Diagnostics
	//
	//	Provide a snapshot report on all active mutexes. This will be the total time the mutex was locked and the total time spent by threads waiting
	//	for the lock to become available.
	//
	//	Please note that the values are first copied from the mutex internals and then converted to strings and used to agregate to the output. This
	//	extra step is needed because the string manipulations affect the mutex that controls string memory allocation! This does not invalidate the
	//	report though as it is intended as a snapshot of mutexs at the point it is called.
	//
	//	Arguments:	1)	Z		Reference to chain populated by the report
	//				2)	bHtml	True if the report is to be fashioned as a web page
	//
	//	Returns:	None

	_hzfunc(__func__) ;

	static	hzString	S = "Untitled" ;

	hzXDate		now ;				//	Time now
	hzLockRWD*	pMtx ;				//	The lock
	double		fWait ;				//	Total nanoseconds wait time
	double		fInuse ;			//	Total nanoseconds in-use
	double		ratio ;				//	Ratio of in-use time to wait + inuse time
	hzString	vals[9] ;			//	Lock activity report fields
	uint64_t	nWaitTotal ;		//	Total nanoseconds wait time
	uint64_t	nInuse ;			//	Total nanoseconds in-use
	uint64_t	nSpinsTotal ;		//	Total spins
	uint32_t	nTriesTotal ;		//	Total compare and swaps
	uint32_t	nLockOpsW ;			//	Total calls to LockWrite
	uint32_t	nLockOpsR ;			//	Total calls to LockRead
	uint32_t	nUnlocks ;			//	Total Unlocks
	char		buf[24] ;			//	Mutex name buffer

	now.SysDateTime() ;
	Z.Printf("<p><center>Mutex Contention Report at %s</center></p>\n", *now.Str()) ;

	Z <<
	"<div align=\"right\">\n"
	"<table width=\"90%\" align=\"center\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\" "
		"style=\"text-decoration:none; font-family:verdana; font-size:11px; font-weight:normal; color:#000000;\">\n"
	"\t<tr>\n"
	"\t\t<th>Mutex ID</th>\n"
	"\t\t<th>Wr Locks</th>\n"
	"\t\t<th>Rd Locks</th>\n"
	"\t\t<th>Unlocks</th>\n"
	"\t\t<th>Status</th>\n"
	"\t\t<th>Spins</th>\n"
	"\t\t<th>Tries</th>\n"
	"\t\t<th>Wait/Overhead</th>\n"
	"\t\t<th>Lock Duration</th>\n"
	"\t\t<th>Ratio</th>\n"
	"\t\t<th>Name</th>\n"
	"\t</tr>\n" ;

	for (pMtx = s_allMtx ; pMtx ; pMtx = pMtx->next)
	{
		fWait = pMtx->m_WaitTotal ;
		fInuse = pMtx->m_Inuse ;
		if (fInuse)
			ratio = fInuse/(fWait + fInuse) ;
		else
			ratio = 0.0 ;
		sprintf(buf, "%f", ratio) ;

		nLockOpsW = pMtx->m_LockOpsW ;
		nLockOpsR = pMtx->m_LockOpsR ;
		nUnlocks = pMtx->m_Unlocks ;
		nSpinsTotal = pMtx->m_SpinsTotal ;
		nTriesTotal = pMtx->m_TriesTotal ;
		nWaitTotal = pMtx->m_WaitTotal ;
		nInuse = pMtx->m_Inuse ;

		vals[0] = FormalNumber(nLockOpsW, 12) ;
		vals[1] = FormalNumber(nLockOpsR, 12) ;
		vals[2] = FormalNumber(nUnlocks, 12) ;
		vals[3] = FormalNumber(nSpinsTotal, 15) ;
		vals[4] = FormalNumber(nTriesTotal, 15) ;
		vals[5] = FormalNumber(nWaitTotal, 15) ;
		vals[6] = FormalNumber(nInuse, 15) ;

		Z << "\t<tr align=\"right\">\n" ;
		Z.Printf("<td>%03d</td>", pMtx->m_Id) ;
		Z.Printf("<td>%s</td>", *vals[0]) ;
		Z.Printf("<td>%s</td>", *vals[1]) ;
		Z.Printf("<td>%s</td>", *vals[2]) ;
		Z.Printf("<td>%11u</td>", pMtx->m_lockval) ;
		Z.Printf("<td>%s</td>", *vals[3]) ;
		Z.Printf("<td>%s</td>", *vals[4]) ;
		Z.Printf("<td>%s</td>", *vals[5]) ;
		Z.Printf("<td>%s</td>", *vals[6]) ;
		Z.Printf("<td>%s</td>", buf) ;
		Z.Printf("<td align=\"left\">Name</td>", *S) ;
		Z << "\t</tr>\n" ;
	}
	Z << "</table>\n" ;
}

/*
**	SECTION 4: Semaphore based locking
*/

#define HZLOCKNOKEY	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH|IPC_CREAT	//	Semaphore settings for 'lock no key'
#define HZLOCKKEY	S_IRUSR|S_IWUSR												//	Semaphore settings for 'lock key'

class	_lockset
{
	//	Comprises a set of 32 semaphores - even though most applications will frequently either use only a few or quite large numbers of them. They are grouped
	//	in blocks of 32 because they can be set and tested by bitwise operations and giving 32 at a time reduces demand on system resorces.

	void	_init	(void)
	{
		//	Initializes the set of 32 semaphores
		//
		//	Arguments:	None
		//	Returns:	None

		_hzfunc("_lock::_init") ;

		if ((m_nSemId = semget(IPC_PRIVATE, 32, HZLOCKNOKEY)) < 0)
			hzexit(_fn, 0, E_INITFAIL, "SERIOUS ERROR Could not create semaphore\n") ;

		if (semctl(m_nSemId, 0, SETVAL, 0) == -1)
			hzexit(_fn, 0, E_INITFAIL, "Problem initializing semaphore id=%d", m_nSemId) ;
	}

	void	_halt	(void)
	{
		//	Release all resources back to operating system
		//
		//	Arguments:	None
		//	Returns:	None

		_hzfunc("_lock::_halt") ;

		if (semctl(m_nSemId, 0, IPC_RMID) < 0)
			hzerr(_fn, HZ_ERROR, E_SHUTDOWN, "Cannot free semaphore (%d)", m_nSemId) ;
	}

public:
	_lockset*	next ;			//	Next lock in the series
	uint32_t	m_bFree ;		//	Bitwise free list
	int32_t		m_nSemId ;		//	Unique id for semahore group

	_lockset	(void)
	{
		m_bFree = 0xffffffff ;
		next = 0 ;
		_init() ;
	}

	~_lockset	(void)
	{
		_halt() ;
	}
} ;

class	_lockmgr
{
	//	Manages all issued sets of semaphores

	_lockset*	s_pSemaGroups ;		//	Allocated semaphore groups
	_lockset*	s_pSemaLast ;		//	Last semaphore group in series

	void	_close	(void)
	{
		_lockset*	pl ;			//	Semaphone lock set iterator
		_lockset*	plnext ;		//	Next semaphore lock set in series

		for (pl = s_pSemaGroups ; pl ; pl = plnext)
		{
			plnext = pl->next ;
			delete pl ;
		}
	}

public:
	_lockmgr	()
	{
		s_pSemaGroups = 0 ;
		s_pSemaLast = 0 ;
	}

	~_lockmgr	()
	{
		_close() ;
	}

	hzEcode	Alloc	(int32_t& semid, uint32_t& resource) ;
	hzEcode	Free	(int32_t semid, uint32_t resource) ;
} ;

hzEcode	_lockmgr::Alloc	(int32_t& semid, uint32_t& resource)
{
	_hzfunc("_lockmgr::Alloc") ;

	_lockset*	pl ;		//	Current lock set (of 32 locks)
	uint32_t	div ;		//	Lock set iterator mask
	uint32_t	nIndex ;	//	Lock set bitwise iterator

	for (pl = s_pSemaGroups ; pl ; pl = pl->next)
	{
		if (pl->m_bFree)
		{
			for (div = 1, nIndex = 0 ; nIndex < 32 ; div *= 2, nIndex++)
			{
				if (pl->m_bFree & div)
				{
					pl->m_bFree &= ~div ;
					semid = pl->m_nSemId ;
					resource = nIndex ;
					//cout << "Got lock with m_nSemId of " << m_nSemId << " and resource of " << m_nResource << endl ;
					return E_OK ;
				}
			}
		}
	}

	//	None free, allocate a new semaphore group and take the first slot

	pl = new _lockset() ;
	if (!pl)
		hzexit(_fn, 0, E_MEMORY, "Could not allocate a _lockset") ;

	pl->m_bFree = 0xfffffffe ;

	if (!s_pSemaGroups)
		s_pSemaGroups = s_pSemaLast = pl ;
	else
	{
		s_pSemaLast->next = pl ;
		s_pSemaLast = pl ;
	}

	semid = pl->m_nSemId ;
	resource = 0 ;
	return E_OK ;
}

hzEcode	_lockmgr::Free	(int32_t semid, uint32_t resource)
{
	_hzfunc("_lockmgr::Free") ;

	_lockset*	pl ;		//	Current lock set (of 32 locks)
	uint32_t	div ;		//	Lock set iterator mask
	uint32_t	nIndex ;	//	Lock set bitwise iterator

	for (pl = s_pSemaGroups ; pl ; pl = pl->next)
	{
		if (semid != pl->m_nSemId)
			continue ;

		for (div = 1, nIndex = 0 ; nIndex < resource ; div *= 2, nIndex++) ;
		pl->m_bFree |= div ;
		return E_OK ;
	}

	hzexit(_fn, 0, E_CORRUPT, "Could not located sema group with semid of %d\n", semid) ;
}

/*
**	Variables
*/

static	_lockmgr	theLockMgr ;		//	Global semaphore

/*
**	Section 2:	hzSysLock functions
*/

void	hzSysLock::_init	(void)
{
	//	Obtains a semaphore id for the semaphore set and a resource id (both needed for Lock/Unlock)
	//
	//	Arguments:	None
	//	Returns:	None

	theLockMgr.Alloc(m_nSemId, m_nResource) ;
}

void	hzSysLock::_halt	(void)
{
	//	Releases semaphore
	//
	//	Arguments:	None
	//	Returns:	None

	theLockMgr.Free(m_nSemId, m_nResource) ;
}

void	hzSysLock::Lock	(void)
{
	//	Lock semaphore
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzSysLock::Lock") ;

	sembuf	lockbuf	[2] ;	//	Semaphore operation buffer
	int32_t	tid ;			//	Caller thread id

	tid = pthread_self() ;

	if (m_nSemId == -1)
		hzexit(_fn, 0, E_NOTFOUND, "Attempting to lock an uninitialized semaphore") ;
		//return ;

	if (m_lockval && m_lockval == tid)
	{
		fprintf(stderr, "thread %u re-locking sema %d:%d\n", tid, m_nResource + 1, m_nSemId) ;
		m_count++ ;
	}
	else
	{
		lockbuf[0].sem_num = m_nResource ;
		lockbuf[0].sem_op = 0 ;
		lockbuf[0].sem_flg = 0 ;
		lockbuf[1].sem_num = m_nResource ;
		lockbuf[1].sem_op = 1 ;
		lockbuf[1].sem_flg = 0 ;

		fprintf(stderr, "thread %u seeking sema %d:%d\n", tid, m_nResource + 1, m_nSemId) ;

		if (semop(m_nSemId, &lockbuf[0], 2) == -1)
			hzexit(_fn, 0, E_INITFAIL, "lock on sema %d [%d] could not be obtained\n", m_nResource + 1, m_nSemId) ;

		fprintf(stderr, "thread %u obtained sema %d:%d\n", tid, m_nResource + 1, m_nSemId) ;

		m_lockval = tid ;
		m_count = 1 ;
	}
}

void	hzSysLock::Unlock	(void)
{
	//	Unlock semaphore
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hzSysLock::Unlock") ;

	sembuf	lockbuf ;	//	Semaphore operation buffer
	int32_t	tid ;		//	Caller thread id

	tid = pthread_self() ;

	if (m_nSemId == -1)
		return ;

	if (!m_lockval)
		hzexit(_fn, 0, E_CORRUPT, "Attempting to unlock a semaphore that is not locked by any thread") ;

	if (m_lockval != tid)
		hzexit(_fn, 0, E_CORRUPT, "Attempting to unlock a semaphore that is locked by another thread (%u)", m_lockval) ;

	m_count-- ;
	if (m_count > 0)
		fprintf(stderr, "thread %u unwinding sema %d:%d\n", tid, m_nResource + 1, m_nSemId) ;
	if (m_count == 0)
	{
		m_lockval = 0 ;
		lockbuf.sem_num = m_nResource ;
		lockbuf.sem_op = -1 ;
		lockbuf.sem_flg = 0 ;

		fprintf(stderr, "thread %u releasing sema %d:%d\n", tid, m_nResource + 1, m_nSemId) ;

		if (semop(m_nSemId, &lockbuf, 1) == -1)
			hzerr(_fn, HZ_ERROR, E_RELEASE, "Unlocked sema %d [%d]\n", m_nResource, m_nSemId) ;
	}
}
