//
//	File:	hzLock.h
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

#ifndef hzLock_h
#define hzLock_h

#include "hzBasedefs.h"

//	Please note the synopsis on HadronZoo locking is given on our page on Multithreading. Please visit http://www.hadronzoo.com/mthread.
//
//	Synopsis:	Synchronization
//
//	<b>Thread Intentions and Implied Lock Types</b>
//
//	In a multithreaded environment, any thread seeking access to a shared resource must first lock the resource to ensure no other thread can perform operations
//	on it. The thread can have any of the following intentions:-
//
//		1)	READ	The thread only wants to read the resource. Multiple threads can simultaneously READ a resource as long as NO threads are writing to it.
//
//		2)	WRITE	The thread intends to alter to resource. No other thread can either write to or read from the resource.
//
//		3)	ATTACH	Attachment is a matter of setting a pointer to a resource and incrementing an associated copy count. Although the copy count commonly exists
//					within the resource itself, this is not the same thing as a WRITE operation. Attaching can be done while other threads are either reading or
//					writing to the resource. Attaching is done to impliment a soft copy operation.
//
//		4)	DETACH	Detachment is not quite the opposite of attachment. The resource copy count is decremented and a pointer previously pointing to the resource
//					is pointed elsewhere or set to NULL. However if the detaching entity is the only entity attached to the resource, the resource is deleted.
//
//	Note that while a thread can intend to DELETE the resource, this should be considered unusual. It begs an obvious question: If a resource is shared, can any
//	thread have the right to delete it? It may be possible but very unlikely, at least as far as HadronZoo is concerned! The standard policy is either to define
//	resources as global, omnipresent during runtine and not subject to copying and therefore not subject to attachment and detachment - OR to define a resources
//	as dependent on having at least one entity attached. When the last attached entity detaches, the resource is automatically deleted.
//
//	Note also a thread can intend to CREATE a resource. However this also is a conundrum. Before a resource is created by any given thread, it is impossible for
//	any other thread to access that resource!
//
//	So we are left with only four possible thread intentions towards a resource. This means there are four possible forms of LOCK operation, two of which do not
//	apply to global omnipresent resources. There is therefore, at least two fundamental types of lock, one which only supports READ/WRITE and another supporting
//	all four operations. In practice, more are needed as discussed below.
//
//	<b>Lock Operations and Outcomes</p>
//
//	All HadronZoo locks are spinlocks meaning the thread constantly loops round until it can be granted the lock. This is a waste of time but considered less of
//	a waste than context switching. In the case of a global omnipresent resource, the lock operations are as follows:-
//
//		1)	LockRead()	- This first checks the 'thread value' of the lock. This will be 0 if no WRITE lock is in place, -1 if the resource has been deleted or
//			is otherwise invalid, and if a write lock is in place, the id of the thread that has the write lock. If the thread value is -1 LockRead() fails and
//			returns E_NOTFOUND. If the thread value is a valid thread id, LockRead() reads the thread value in a constant loop until it either becomes 0 or -1.
//			If 0 LockRead() then increments a read counter and returns E_OK therby granting a read lock.
//
//		2)	LockWrite()	- This first loops until it can set the thread value to the thread id using the atomic function __sync_val_compare_and_swap(). For this
//			function to succeed, the thread value has to first become 0 but even then, it may have set the value to something other than the thread id. So the
//			loop continues until __sync_val_compare_and_swap() either does set the thread value to the thread id or the thread value becomes -1. If the latter,
//			LockWrite() fails and returns E_NOTFOUND. If the thread value has been set to the thread id, LockWrite() then enters another loop, this time waiting
//			for the read counter to drop to 0. Only when this state is reached will LockWrite() return E_OK thereby granting a write lock.
//
//		3)	LockAttach() - This does not care about what can or cannot be done with the resource, it is only concerned with effecting a soft copy. To this end,
//			the thread value can be anything except -1. It is worth noting however that if a resource has a copy count, there should not be any operations that
//			can delete the resource, with or without a write lock. So LockAttach() does not check the thread value. All it does is increment the copy count by 1
//			using __sync_add_and_fetch().
//
//		4)	LockDetach() - This is or should be, the principle means by which deletion of a resource is controlled. However, all LockDetach() does is decrement
//			the copy count using __sync_add_and_fetch().
//
//	<b>Do locks actually lock anything?</b>
//
//	No, they do not. A lock is more like a "Wet floor" sign. To be effective the sign has to first be seen and by EVERYONE who approaches the floor and then ALL
//	who see the sign have to decide they will avoid walking on the floor until the sign is removed. As is obvious, the sign itself cannot prevent anyone persons
//	from walking on the wet floor. Like the sign, a lock is only the focal point of a locking protocol. The lock only work by the good grace of the program code
//	obeying the protocol, something that is notoriously difficult to get right.
//
//	In terms of where to place the lock, there are only two choices. Have the lock separate to the resource it protects or make the lock an integral part of the
//	resource. Both arrangements work well with global omnipresent resources that should never be copied. However, only the latter works with resources that are
//	not omnipresent and only permit soft copying - and this is less than straightforward. The problem with such resources, is that they can be deleted. And when
//	they are, they take the lock with them.
//
//	<b>General Operational Considerations</b>
//
//	HadronZoo considers hard copying of anything other than small datum to be an anathema and takes a pretty dim view of copying in General. There are no really
//	good grounds for soft copying a hzMapS instance for example. You can pass the map by reference so why pass it by value? And you can do things like reorder a
//	map of object name to object to say, object date to object - by iterating through the first and inserting into the second. But at what point would you ever
//	want to make a copy of a map?
//
//	Making a copy of the objects in a map however, is something applications need to do all the time. And that copying will often need to be soft. If the object
//	was a string, you would not want to hard copy it as it might be very long. And if you are going to soft copy strings the strings will have to be pointed at
//	by pointers and have a count of the number of pointers pointing at it.
//
//	The string (hzString) and string-like classes (hzEmaddr and hzUrl), are relatively straightforward. These all comprise a pointer which can either be NULL or
//	point to a 'string space'. It is the string space that is the 'object' in this context and it is the string space that has the copy count. The string space
//	cannot be operated on by anything other than member functions of the host class (hzString, hzEmaddr or hzUrl). And these all follow a simple protocol. None
//	ever change a string space without incrementing the copy count first to perform the lock. Then they create new string space and populate it by applying the
//	process to bytes read from the original.
//
//	However, as the synopsis on strings points out, one can screw up strings by using the * operator to access string space content without first making a copy
//	of the string. It is a question of habbit. Throughout the library code there are examples of code of the following form:-
//	<pre>
//		for (count = 0 ; count < some_array.Count() ; count++)
//		{
//			S = some_array[count] ;
//			some_print_function("... %s ...", *S) ;
//		}
//	</pre>
//	Where 'S' is previously defined as a hzString or similar. What you should NOT do is have some_print_function("... %s ...", *some_arrray[count]). We are very
//	aware this is a common practice among developers but it is not a good idea. If the thread is context-switched out halfway through some_print_function, upon
//	resumption of execution, said function could easily find itself printing what it thinks is a null terminated string but which has since been reallocted to
//	something else entirely.
//
//	As an aside, it is also a bad idea to use array and other collection class template subsripts directly for performance reasons. Each subscript is evaluated
//	by performing a lookup in the host collection. If for example, we wanted to lookup a person by social security number (SSN) and print out their name, phone
//	number and email address, what we would NOT do is this:-
//	<pre>
//		some_print_function("Name: %s", Persons[SSN].name) ;
//		some_print_function("Phone: %s", Persons[SSN].phone) ;
//		some_print_function("Email: %s", Persons[SSN].email) ;
//	</pre>
//	Quite apart from the integrity issue of losing the email address, phone number or even the name as a result of action by another thread, the code would run
//	three times slower than it needs to becuase it is three lookups instead of only one. The correct approach is to declare an instance of Person as P, then set
//	P = Persons[SSN], then use P.name, P.phone and P.email.
//
//	<b>Locking in Collection Class Templates</b>
//
//	The correct approach in the above example, depends on the Person class being properly protected. Very commonly, such a class would just comprise strings and
//	maybe a handful of other members. There would be no state variable defined in the Person class that advised a reader function an instance was in the process 
//	of being updated. Nothing to ensure you got either the whole old value or the whole new value. But does there need to be? Not if the Person instances are in
//	a collection that employs a lock. If none of the objects held in the collection can change without a thread having a write lock, and while the write lock is
//	locked nothing in the collection can be read, then the collection does guarentee the object change runs to completion before anything can read the object.
//
//	Except in the common case where the objects in the collection are only pointers to objects. This would not make any sense if the objects were atomic values,
//	such at numbers (always hard copied) or strings (copying internally handled). However, it is a common approach where objects are complex (comprise multiple
//	atomic data memebers) and it makes particular sense in cases where it expedient to order object collections by more than one member.
//
//	There are two problems here. Firstly as is obvious, the objects must have locking built into their class. This will affect all member functions of the class
//	that copy or transform instances in any way. There are two possible approaches. Either the class comprises a pointer and has an internal data area (like the 
//	string classes). Or the applicable functions explicitly enforce the appropriate 'logical unit of work'. Same difference really.
//
//	The second problem is the integrity of set of maps. One map could update quite some time before all maps did. As a consequence, a search on a key can result
//	in finding a mismatched object or far worse, an invalid object. Currently the HadronZoo library does not have a multi-key collection class.
//
//

class	hzLockS
{
	//	Category:	Synchronization
	//
	//	hzLockS or simple lock is a pure atomic non-blocking spinlock and nothing else. It comprises a single 4 byte value and is aimed at fine grain locking.
	//	It works by using the atomic function __sync_val_compare_and_swap to test if the 4 byte value is zero (unlocked) and if it is, sets it to the thread id.
	//	It is a spinlock because it loops continuously until __sync_val_compare_and_swap succeeds.
	//
	//	Because there is no lock counter it is not possible to allow re-locking by the holding thread. Arguably, re-locking is bad practice but it can simplfy
	//	code and so is allowed by other lock classes. A hzLockRWD for example remains locked until each call to Lock() has been matched with a call to Unlock().
	//	If re-locking were allowed by hzLockS, this sequence would be dangerous as the first call to Unlock() would leave the resource unprotected.
	//
	//	Note that with grain locking, is is common practice to have the lock exist as an integral part of the protected resource. This arrangement is prone to
	//	lock destruction by one thread while other thread(s) wait for it to become available. To mitigate against this a Kill() method has been added. This sets
	//	the lock value to 0xffffffff to indicate out of use and must be called INSTEAD of Unlock() by the thread that is going to destroy the resource and thus
	//	the lock. The Lock() method, in addition to calling __sync_val_compare_and_swap, tests for out of use and returns with an error. Any waiting thread must
	//	deal with this error but at least it has been told and not kept waiting.

	uint32_t	m_lockval ;	//	Lock value (0 or thread_id)

	//	Exclude copy construction and copying
	hzLockS		(const hzLockS& op)	{ m_lockval = 0 ; }
	hzLockS&	operator=	(const hzLockS& op)	{ return *this ; }

public:
	hzLockS		(void)	{ m_lockval = 0 ; }
	~hzLockS	(void)	{}

	hzEcode	Lock	(int32_t timeout = -1) ;	//	Lock operation
	void	Kill	(void) ;					//	Deactivate lock (ahead of destructor)
	void	Unlock	(void) ;					//	Release the lock
} ;

class	hzLocker
{
	//	Category:	Synchronization
	//
	//	Pure base class for unifying hzLockRW (read/write lock without diagnostics) and hzLockRWD (read/write lock with diagnostics)

public:
	virtual	hzEcode	LockWrite	(int32_t timeout = -1) = 0 ;
	virtual	hzEcode	LockRead	(int32_t timeout = -1) = 0 ;
	virtual	void	Kill		(void) = 0 ;
	virtual	void	Unlock		(void) = 0 ;
} ;

enum	hzLockOpt
{
	//	Category:	System
	//
	//	Note this is for simplification of arguments within collection class templates only

	HZ_NOLOCK,		//	No locking in use
	HZ_ATOMIC,		//	Atomic locks no contention reporting
	HZ_MUTEX		//	Atomic locks with contention reporting
} ;

class	hzLockRW	: public hzLocker
{
	//	Category:	Synchronization
	//
	//	hzLockRW is a pure atomic non-blocking spinlock comprising the minimum needed to impliment read-write locking. Threads call LockWrite() if they wish to
	//	write to a resource and LockRead() if they only want to read it. LockRead() grants the lock to read a resource no matter how many other threads are also
	//	reading it - but not if ANY threads are seeking to write to it. LockWrite() first stops other threads from gaining any form of access and then waits
	//	until all threads that have read access have relinquished it before granting the lock.
	//
	//	The lock part works the same was as the simple lock hzLockS. It is locked by setting a 4 byte value to the thread id using __sync_val_compare_and_swap.
	//	So it is either locked, unlocked or out of use. But in addition to the 4 byte value there is a counter and this is is critical as it is the only means
	//	by which it can be known how many threads currently hold a read lock. The counter itself must be protected as it is possible that two or more threads
	//	can increment it simultaneously with the effect that one or more increments fail.
	//
	//	So LockRead() locks the lock, increments the counter and unlocks the lock. LockWrite() locks the lock, loops until the counter is zero then returns to
	//	the caller with E_OK to say the lock is granted. Unlock() checks if its thread holds the lock (meaning it is unlocking a write lock). If it does it just
	//	unlocks but if it doesn't then it decrements the counter using a __sync_fetch_and sub call.

	uint32_t	m_lockval ;	//	Lock value (0 or thread_id)
	uint32_t	m_counter ;	//	Read thread counter

	//	Exclude copy construction and copying
	hzLockRW	(const hzLockRW& op)	{ m_lockval = m_counter = 0 ; }
	hzLockRW&	operator=	(const hzLockRW& op)	{ return *this ; }

public:
	hzLockRW	(void)	{ m_lockval = m_counter = 0 ; }
	~hzLockRW	(void)	{}

	hzEcode	LockRead	(int32_t timeout = -1) ;	//	Obtain a read only lock
	hzEcode	LockWrite	(int32_t timeout = -1) ;	//	Obtain a write lock
	void	Kill		(void) ;					//	Deactivate lock (ahead of destructor)
	void	Unlock		(void) ;					//	Release the lock
} ;

class	hzLockRWD	: public hzLocker
{
	//	Category:	Synchronization
	//
	//	hzLockRWD is a resource lock effected as a 'wrapper' based on a single mutex. The lock is granted to the first thread that requests it and until
	//	the winning thread releases the lock, all other threads seeking the lock have to wait.
	//
	//	Mutex locks can be blocking or non-blocking. When blockng, all waiting threads are suspended. The CPU performs a context switch (swaps out the
	//	current thread and runs another in its place). When non-blocking the waiting threads loop or 'spin', constantly testing if the lock has become
	//	available and then seeking the look. This spinning only stops when the lock is granted. Both blocking and non-blocking locks can be subjected
	//	to a timeout but if they are, the calling thread must be able to contend with being denied access to the resource controlled by the lock.
	//
	//	While spining round a loop is clearly a waste of time, it should be noted that thread suspension is also a waste of time - and in the general
	//	case, a much worse waste of time. Context switching is very expensive. For this reason the decision was taken that hzLockRWD should not support
	//	blocking. So hzLockRWD Lock() method never blocks and hzLockRWD is exclusively a spinlock.
	//
	//	The Lock() method has a single optional argument of timeout (as an integer number of milliseconds), with a default of -1 (not applicable). A
	//	timeout of zero will try for a lock but imeadiately return with or without it. The return is E_OK for lock granted, but also E_OK if the lock
	//	is not initialized (not made active). If the timeout expires E_TIMEOUT is returned. Note that outside these circumstances, Lock() won't return!
	//
	//	The purpose of the hzLockRWD class is essentially diagnostic - as otherwise a mutex is too simple to require a wrapper! hzLockRWD keeps track of
	//	contentions and of time wasted by those contentions. To this end, hzLockRWD instances must be named, either by passing a hzString or char* to the
	//	constructor or later with a call to the Init() method. Indeed without being named, a hzLockRWD is inactive and does not lock anything.
	//
	//	The global function ContentionReport(hzChain& Z) reports on contentions of all mutexes and adds up time wasted in spinlocks by all threads.

	//	Exclude copy construction and copying
	hzLockRWD	(const hzLockRWD& op) ;	//	{ m_lockval = m_counter = 0 ; }
	hzLockRWD&	operator=	(const hzLockRWD& op) ;	//	{ return *this ; }

public:
	hzLockRWD*	next ;				//	For accomodation in an explicit linked list and mutex internal instances
	hzLockRWD*	prev ;				//	For accomodation in an explicit linked list and mutex internal instances

	//	For contention report
	uint64_t	m_Granted ;			//	Time mutex was last granted. Used to calculate both in-use and wait time
	uint64_t	m_WaitThis ;		//	Time mutex was last granted. Used to calculate both in-use and wait time
	uint64_t	m_WaitTotal ;		//	Total time thread waited for access (either for read or write)
	uint64_t	m_Inuse ;			//	Total time lock was in-use (a thread had write access)
	uint64_t	m_SpinsTotal ;		//	Runtime total spins on the lock by all waiting threads
	uint64_t	m_TriesTotal ;		//	Runtime total number of compare and swap calls
	uint32_t	m_LockOpsW ;		//	Total lock operations (write)
	uint32_t	m_LockOpsR ;		//	Total lock operations (read)

	//	For lock operations
	uint32_t	m_TriesThis ;		//	Number of compare and swap calls before lock granted
	uint32_t	m_SpinsThis ;		//	Spins before current lock granted
	uint32_t	m_Unlocks ;			//	Unlocks operations (should equal the total of read and write lock operations)
	uint32_t	m_lockval ;			//	The lock itself - set to 0 or thread id that has the lock
	uint32_t	m_counter ;			//	The read lock counter
	uint16_t	m_Id ;				//	Unique id
	uint16_t	m_recurse ;			//	Recursion count
	char		m_name[16] ;		//	Name for diagnostics. Note limited to a fixed buffer to allow mutex initialization within the hzHeap regime

	hzLockRWD	(void) ;
	hzLockRWD	(const char* name) ;
	~hzLockRWD	(void) ;

	hzEcode	Setname	(const char* name) ;

	void*		Address		(void) ;
	uint64_t	Granted		(void) ;
	uint64_t	WaitThis	(void) ;
	uint64_t	WaitTotal	(void) ;
	uint64_t	Inuse		(void) ;
	uint64_t	SpinTotal	(void) ;
	uint32_t	TriesTotal	(void) ;
	uint32_t	SpinThis	(void) ;
	uint32_t	TriesThis	(void) ;
	uint32_t	Thread		(void) ;
	uint32_t	Level		(void) ;
	uint32_t	UID			(void) ;

	const char*	Name	(void) ;

	hzEcode	LockRead	(int32_t timeout = -1) ;	//	Obtain a real only lock
	hzEcode	LockWrite	(int32_t timeout = -1) ;	//	Obtain a write lock
	void	Unlock		(void) ;					//	Release the lock (either read or write)
	void	Kill		(void) ;					//	Deactivate lock (ahead of destructor)
} ;

/*
**	SECTION 2:	Thread diodes (lock free que)
*/

class	hzDiode
{
	//	Category:	Synchronization
	//
	//	In thread specialization, each statge of a process can be assigned to a different thread. As objects complete one stage they are queued up for
	//	the next stage. Assuming each stage must be applied in the same order to all applicable objects (usually the case), then the flow of objects
	//	through the queues is always in one direction. And that means threads that write to the queue never read from it and threads that read from it
	//	never write to it. If there is only one writer thread the push operation can be lock free and likewise, if there is only one reader thread the
	//	pull operation can be lock free.
	//
	//	hzDiode is a lock free queue that works with this simple case. If there are multiple threads on one or both sides, then the push and/or pull
	//	operations would have to be protected by external measures.
	//
	//	hzDiode deals only in void* objects. This avoids any unessesary complications such as copying objects. This is strictly about passing objects
	//	as efficiently as possible! It is implimented as a list of buckets allocated from a freelist

	class	_bkt
	{
		//	Task object bucket

	public:
		_bkt*		next ;			//	Next bucket
		uint32_t	usage ;			//	Count in
		uint32_t	count ;			//	Count out
		void*		items[30] ;		//	Items in queue

		_bkt	(void)	{ next = 0 ; usage = count = 0 ; }
	} ;

	_bkt*	m_pFree ;		//	Freelist of buckets
	_bkt*	m_pFirst ;		//	First bucket
	_bkt*	m_pLast ;		//	Last bucket

	//	Hide copy constructor
	hzDiode	(const hzDiode& op)	{}

	_bkt*	_alloc	(void) ;
	void	_free	(_bkt* pB) ;
public:
	hzDiode	(void)
	{
		m_pFree = m_pFirst = m_pLast = 0 ;
	}

	~hzDiode	(void) ;

	void	Push	(void* pObj) ;
	void*	Pull	(void) ;
} ;

#endif	//	hzLock_h
