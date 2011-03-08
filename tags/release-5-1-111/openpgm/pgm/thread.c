/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * mutexes and locks.
 *
 * Copyright (c) 2010 Miru Limited.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <errno.h>
#include <impl/framework.h>

//#define THREAD_DEBUG


/* Globals */

bool pgm_smp_system PGM_GNUC_READ_MOSTLY = TRUE;


/* Locals */

#if defined(_WIN32) && !defined(CONFIG_HAVE_WIN_COND)
static DWORD cond_event_tls = TLS_OUT_OF_INDEXES;
#endif

static volatile uint32_t thread_ref_count = 0;


#if !defined( _WIN32 ) && defined( __GNU__ )
#	define posix_check_err(err, name) \
		do { \
			const int save_error = (err); \
			if (PGM_UNLIKELY(save_error)) { \
				char errbuf[1024]; \
				pgm_error ("file %s: line %d (%s): error '%s' during '%s'", \
					__FILE__, __LINE__, __PRETTY_FUNCTION__, \
					pgm_strerror_s (errbuf, sizeof (errbuf), save_error), name); \
			} \
		} while (0)
#	define posix_check_cmd(cmd) posix_check_err ((cmd), #cmd)
#elif !defined( _WIN32 ) && !defined( __GNU__ )
#	define posix_check_err(err, name) \
		do { \
			const int save_error = (err); \
			if (PGM_UNLIKELY(save_error)) { \
				char errbuf[1024]; \
				pgm_error ("file %s: line %d): error '%s' during '%s'", \
					__FILE__, __LINE__, \
					pgm_strerror_s (errbuf, sizeof (errbuf), save_error), name); \
			} \
		} while (0)
#	define posix_check_cmd(cmd) posix_check_err ((cmd), #cmd)
#elif defined( _WIN32 ) && defined( __GNU__ )
#	define win32_check_err(err, name) \
		do { \
			const bool save_error = (err); \
			if (PGM_UNLIKELY(!save_error)) { \
				pgm_error ("file %s: line %d (%s): error '%s' during '%s'", \
					__FILE__, __LINE__, __PRETTY_FUNCTION__, \
					pgm_wsastrerror (GetLastError ()), name); \
			} \
		} while (0)
#	define win32_check_cmd(cmd) win32_check_err ((cmd), #cmd)
#elif defined( _WIN32 ) && !defined( __GNU__ )
#	define win32_check_err(err, name) \
		do { \
			const bool save_error = (err); \
			if (PGM_UNLIKELY(!save_error)) { \
				pgm_error ("file %s: line %d: error '%s' during '%s'", \
					__FILE__, __LINE__, \
					pgm_wsastrerror (GetLastError ()), name); \
			} \
		} while (0)
#	define win32_check_cmd(cmd) win32_check_err ((cmd), #cmd)
#else
#	error "Uncaught error handling scenario."
#endif /* !_WIN32 */


/* only needed for Win32 pre-Vista read-write locks
 */
void
pgm_thread_init (void)
{
	if (pgm_atomic_exchange_and_add32 (&thread_ref_count, 1) > 0)
		return;

#if defined(_WIN32) && !defined(CONFIG_HAVE_WIN_COND)
	win32_check_cmd (TLS_OUT_OF_INDEXES != (cond_event_tls = TlsAlloc ()));
#endif

	if (pgm_get_nprocs() <= 1)
		pgm_smp_system = FALSE;
}

void
pgm_thread_shutdown (void)
{
	pgm_return_if_fail (pgm_atomic_read32 (&thread_ref_count) > 0);

	if (pgm_atomic_exchange_and_add32 (&thread_ref_count, (uint32_t)-1) != 1)
		return;

#if defined(_WIN32) && !defined(CONFIG_HAVE_WIN_COND)
	TlsFree (cond_event_tls);
#endif
}

/* prefer adaptive-mutexes over regular mutexes, an adaptive mutex is wrapped by
 * a count-limited spinlock to optimize short waits.
 *
 * a mutex here is defined as process-private and contention can be 2 or more threads.
 *
 * multiple calls to pgm_mutex_init is undefined.
 */

void
pgm_mutex_init (
	pgm_mutex_t*	mutex
	)
{
	pgm_assert (NULL != mutex);

#ifdef PTHREAD_MUTEX_ADAPTIVE_NP
/* non-portable but define on Linux & FreeBSD, uses spinlock for 200 spins then waits as mutex */
	pthread_mutexattr_t attr;
	posix_check_cmd (pthread_mutexattr_init (&attr));
	pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	posix_check_cmd (pthread_mutex_init (&mutex->pthread_mutex, &attr));
	pthread_mutexattr_destroy (&attr);
#elif !defined( _WIN32 )
	posix_check_cmd (pthread_mutex_init (&mutex->pthread_mutex, NULL));
#elif defined(CONFIG_HAVE_CRITICAL_SECTION_EX)
/* reduce memory consumption on mutexes on Vista+ */
	InitializeCriticalSectionEx (&mutex->win32_crit, PGM_ADAPTIVE_MUTEX_SPINCOUNT, CRITICAL_SECTION_NO_DEBUG_INFO);
#else
/* Windows "mutexes" are default shared, "critical sections" are default process-private */
	InitializeCriticalSection (&mutex->win32_crit);
	SetCriticalSectionSpinCount (&mutex->win32_crit, PGM_ADAPTIVE_MUTEX_SPINCOUNT);
#endif
}

/* multiple calls to pgm_mutex_free is undefined.
 * call to pgm_mutex_free on locked mutex or non-init pointer is undefined.
 */

void
pgm_mutex_free (
	pgm_mutex_t*	mutex
	)
{
	pgm_assert (NULL != mutex);

#ifndef _WIN32
	posix_check_cmd (pthread_mutex_destroy (&mutex->pthread_mutex));
#else
	DeleteCriticalSection (&mutex->win32_crit);
#endif /* !_WIN32 */
}

/* contention on spin-locks is limited to two threads, a receiving thread and a sending thread.
 */

void
pgm_spinlock_init (
	pgm_spinlock_t*	spinlock
	)
{
	pgm_assert (NULL != spinlock);

#ifdef CONFIG_TICKET_SPINLOCK
	pgm_ticket_init (&spinlock->ticket_lock);
#elif defined( CONFIG_HAVE_POSIX_SPINLOCK )
	posix_check_cmd (pthread_spin_init (&spinlock->pthread_spinlock, PTHREAD_PROCESS_PRIVATE));
#elif defined( __APPLE__ )
	spinlock->darwin_spinlock = OS_SPINLOCK_INIT;
#else	/* Win32/GCC atomics */
	spinlock->taken = 0;
#endif
}

void
pgm_spinlock_free (
	pgm_spinlock_t*	spinlock
	)
{
	pgm_assert (NULL != spinlock);

#ifdef CONFIG_TICKET_SPINLOCK
	pgm_ticket_free (&spinlock->ticket_lock);
#elif defined( CONFIG_HAVE_POSIX_SPINLOCK )
/* ignore return value */
	pthread_spin_destroy (&spinlock->pthread_spinlock);
#elif defined(__APPLE__)
	/* NOP */
#else	/* Win32/GCC atomics */
	/* NOP */
#endif
}

void
pgm_cond_init (
	pgm_cond_t*	cond
	)
{
	pgm_assert (NULL != cond);

#ifndef _WIN32
	posix_check_cmd (pthread_cond_init (&cond->pthread_cond, NULL));
#elif defined(CONFIG_HAVE_WIN_COND)
/* requires Vista+ */
	InitializeConditionVariable (&cond->win32_cond);
#else
	cond->len = 0;
	cond->allocated_len = pgm_nearest_power (1, 2 + 1);
	cond->phandle = pgm_new (HANDLE, cond->allocated_len);
#	if defined(CONFIG_HAVE_CRITICAL_SECTION_EX)
/* requires Vista+ */
	InitializeCriticalSectionEx (&cond->win32_crit, PGM_ADAPTIVE_MUTEX_SPINCOUNT, CRITICAL_SECTION_NO_DEBUG_INFO);
#	else
	InitializeCriticalSection (&cond->win32_crit);
	SetCriticalSectionSpinCount (&cond->win32_crit, PGM_ADAPTIVE_MUTEX_SPINCOUNT);
#	endif
#endif /* !_WIN32 */
}

void
pgm_cond_signal (
	pgm_cond_t*	cond
	)
{
	pgm_assert (NULL != cond);

#ifndef _WIN32
	pthread_cond_signal (&cond->pthread_cond);
#elif defined(CONFIG_HAVE_WIN_COND)
	WakeConditionVariable (&cond->win32_cond);
#else
	EnterCriticalSection (&cond->win32_crit);
	if (cond->len > 0) {
		SetEvent (cond->phandle[ 0 ]);
		memmove (&cond->phandle[ 0 ], &cond->phandle[ 1 ], cond->len - 1);
		cond->len--;
	}
	LeaveCriticalSection (&cond->win32_crit);
#endif /* !_WIN32 */
}

void
pgm_cond_broadcast (
	pgm_cond_t*	cond
	)
{
	pgm_assert (NULL != cond);

#ifndef _WIN32
	pthread_cond_broadcast (&cond->pthread_cond);
#elif defined(CONFIG_HAVE_WIN_COND)
	WakeAllConditionVariable (&cond->win32_cond);
#else
	EnterCriticalSection (&cond->win32_crit);
	for (unsigned i = 0; i < cond->len; i++)
		SetEvent (cond->phandle[ i ]);
	cond->len = 0;
	LeaveCriticalSection (&cond->win32_crit);
#endif /* !_WIN32 */
}

#ifndef _WIN32
void
pgm_cond_wait (
	pgm_cond_t*		cond,
	pthread_mutex_t*	mutex
	)
{
	pgm_assert (NULL != cond);
	pgm_assert (NULL != mutex);

	pthread_cond_wait (&cond->pthread_cond, mutex);
}
#else
void
pgm_cond_wait (
	pgm_cond_t*		cond,
	CRITICAL_SECTION*	spinlock
	)
{
	pgm_assert (NULL != cond);
	pgm_assert (NULL != spinlock);

#	if defined(CONFIG_HAVE_WIN_COND)
	SleepConditionVariableCS (&cond->win32_cond, spinlock, INFINITE);
#	else
	DWORD status;
	HANDLE event = TlsGetValue (cond_event_tls);

	if (!event) {
		win32_check_cmd (NULL != (event = CreateEvent (0, FALSE, FALSE, NULL)));
		TlsSetValue (cond_event_tls, event);
	}

	EnterCriticalSection (&cond->win32_crit);
	pgm_assert (WAIT_TIMEOUT == WaitForSingleObject (event, 0));
	if ((cond->len + 1) > cond->allocated_len) {
		cond->allocated_len = pgm_nearest_power (1, cond->len + 1 + 1);
		cond->phandle	    = pgm_realloc (cond->phandle, cond->allocated_len);
	}
	cond->phandle[ cond->len++ ] = event;
	LeaveCriticalSection (&cond->win32_crit);

	EnterCriticalSection (spinlock);
	win32_check_cmd (WAIT_FAILED != (status = WaitForSingleObject (event, INFINITE)));
	LeaveCriticalSection (spinlock);

	if (WAIT_TIMEOUT == status) {
		EnterCriticalSection (&cond->win32_crit);
		for (unsigned i = 0; i < cond->len; i++) {
			if (cond->phandle[ i ] == event) {
				if (i != cond->len - 1)
					memmove (&cond->phandle[ i ], &cond->phandle[ i + 1 ], sizeof(HANDLE) * (cond->len - i - 1));
				cond->len--;
				break;
			}
		}
		win32_check_cmd (WAIT_FAILED != (status = WaitForSingleObject (event, 0)));
		LeaveCriticalSection (&cond->win32_crit);
	}
#	endif /* !CONFIG_HAVE_WIN_COND */
}
#endif /* !_WIN32 */

void
pgm_cond_free (
	pgm_cond_t*	cond
	)
{
	pgm_assert (NULL != cond);

#ifndef _WIN32
	posix_check_cmd (pthread_cond_destroy (&cond->pthread_cond));
#elif defined(CONFIG_HAVE_WIN_COND)
	/* nop */
#else
	DeleteCriticalSection (&cond->win32_crit);
	pgm_free (cond->phandle);
#endif /* !_WIN32 */
}

void
pgm_rwlock_init (
	pgm_rwlock_t*	rwlock
	)
{
	pgm_assert (NULL != rwlock);

#if defined( CONFIG_DUMB_RWSPINLOCK )
	pgm_rwspinlock_init (&rwlock->rwspinlock);
#elif defined( CONFIG_TICKET_RWSPINLOCK )
	pgm_rwticket_init (&rwlock->rwticket_lock);
#elif defined( CONFIG_HAVE_WIN_SRW_LOCK )
/* requires Vista+ */
	InitializeSRWLock (&rwlock->win32_rwlock);
#elif !defined( _WIN32 )
	posix_check_cmd (pthread_rwlock_init (&rwlock->pthread_rwlock, NULL));
#else
#	if defined(CONFIG_HAVE_CRITICAL_SECTION_EX)
/* requires Vista+ */
	InitializeCriticalSectionEx (&rwlock->win32_crit, PGM_ADAPTIVE_MUTEX_SPINCOUNT, CRITICAL_SECTION_NO_DEBUG_INFO);
#	else
	InitializeCriticalSection (&rwlock->win32_crit);
	SetCriticalSectionSpinCount (&rwlock->win32_crit, PGM_ADAPTIVE_MUTEX_SPINCOUNT);
#	endif
	pgm_cond_init (&rwlock->read_cond);
	pgm_cond_init (&rwlock->write_cond);
	rwlock->read_counter	= 0;
	rwlock->have_writer	= FALSE;
	rwlock->want_to_read	= 0;
	rwlock->want_to_write	= 0;
#endif /* !CONFIG_HAVE_WIN_SRW_LOCK */
}

void
pgm_rwlock_free (
	pgm_rwlock_t*	rwlock
	)
{
	pgm_assert (NULL != rwlock);

#if defined( CONFIG_DUMB_RWSPINLOCK )
	pgm_rwspinlock_free (&rwlock->rwspinlock);
#elif defined( CONFIG_TICKET_RWSPINLOCK )
	pgm_rwticket_free (&rwlock->rwticket_lock);
#elif defined( CONFIG_HAVE_WIN_SRW_LOCK )
	/* nop */
#elif !defined(_WIN32)
	pthread_rwlock_destroy (&rwlock->pthread_rwlock);
#else
	pgm_cond_free (&rwlock->read_cond);
	pgm_cond_free (&rwlock->write_cond);
	DeleteCriticalSection (&rwlock->win32_crit);
#endif /* !CONFIG_HAVE_WIN_SRW_LOCK */
}

#if !defined(CONFIG_DUMB_RWSPINLOCK) && !defined(CONFIG_TICKET_RWSPINLOCK) && !defined(CONFIG_HAVE_WIN_SRW_LOCK) && defined(_WIN32)
static inline
void
_pgm_rwlock_signal (
	pgm_rwlock_t*	rwlock
	)
{
	pgm_assert (NULL != rwlock);

	if (rwlock->want_to_write)
		pgm_cond_signal (&rwlock->write_cond);
	else if (rwlock->want_to_read)
		pgm_cond_broadcast (&rwlock->read_cond);
}

void
pgm_rwlock_reader_lock (
	pgm_rwlock_t*	rwlock
	)
{
	pgm_assert (NULL != rwlock);

	EnterCriticalSection (&rwlock->win32_crit);
	rwlock->want_to_read++;
	while (rwlock->have_writer || rwlock->want_to_write)
		pgm_cond_wait (&rwlock->read_cond, &rwlock->win32_crit);
	rwlock->want_to_read--;
	rwlock->read_counter++;
	LeaveCriticalSection (&rwlock->win32_crit);
}

bool
pgm_rwlock_reader_trylock (
	pgm_rwlock_t*	rwlock
	)
{
	bool status = FALSE;

	pgm_assert (NULL != rwlock);

	EnterCriticalSection (&rwlock->win32_crit);
	if (!rwlock->have_writer && !rwlock->want_to_write) {
		rwlock->read_counter++;
		status = TRUE;
	}
	LeaveCriticalSection (&rwlock->win32_crit);
	return status;
}

void
pgm_rwlock_reader_unlock(
	pgm_rwlock_t*	rwlock
	)
{
	pgm_assert (NULL != rwlock);

	EnterCriticalSection (&rwlock->win32_crit);
	rwlock->read_counter--;
	if (rwlock->read_counter == 0)
		_pgm_rwlock_signal (rwlock);
	LeaveCriticalSection (&rwlock->win32_crit);
}

void
pgm_rwlock_writer_lock (
	pgm_rwlock_t*	rwlock
	)
{
	pgm_assert (NULL != rwlock);

	EnterCriticalSection (&rwlock->win32_crit);
	rwlock->want_to_write++;
	while (rwlock->have_writer || rwlock->read_counter)
		pgm_cond_wait (&rwlock->write_cond, &rwlock->win32_crit);
	rwlock->want_to_write--;
	rwlock->have_writer = TRUE;
	LeaveCriticalSection (&rwlock->win32_crit);
}

bool
pgm_rwlock_writer_trylock (
	pgm_rwlock_t*	rwlock
	)
{
	bool status = FALSE;

	pgm_assert (NULL != rwlock);

	EnterCriticalSection (&rwlock->win32_crit);
	if (!rwlock->have_writer && !rwlock->read_counter) {
		rwlock->have_writer = TRUE;
		status = TRUE;
	}
	LeaveCriticalSection (&rwlock->win32_crit);
	return status;
}

void
pgm_rwlock_writer_unlock (
	pgm_rwlock_t*	rwlock
	)
{
	pgm_assert (NULL != rwlock);

	EnterCriticalSection (&rwlock->win32_crit);
	rwlock->have_writer = FALSE;
	_pgm_rwlock_signal (rwlock);
	LeaveCriticalSection (&rwlock->win32_crit);
}
#endif /* !_WIN32 && !CONFIG_HAVE_WIN_SRW_LOCK */


/* eof */
