/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
   */

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
   decrement it.

   - up or "V": increment the value (and wake up one waiting
   thread, if any). */



/*
 * 함수명    : sema_init
 * 설명      : 세마포어를 초기화한다.
 *             세마포어의 초기 값(sema->value)을 value로 설정하고,
 *             대기 리스트(sema->waiters)를 초기화한다.
 * 인자      : struct semaphore *sema - 초기화할 세마포어 객체
 *             unsigned value         - 세마포어의 초기 값 (0 이상)
 * 반환값    : 없음
 * 관련 함수 : list_init()
 * 주의 사항 : value는 음수가 될 수 없다 (0 이상의 값이어야 함)
 */
void
sema_init (struct semaphore *sema, unsigned value) {
	ASSERT (sema != NULL);

	sema->value = value;
	list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. This is
   sema_down function. */

/*
 * 함수명    : sema_down
 * 설명      : 세마포어 값을 1 감소시켜 자원을 점유
 *             만약 세마포어 값이 0이면 현재 스레드는 BLOCKED 상태로 전환
 *             waiters 리스트에 삽입되어 자원이 생길 때까지 대기
 * 인자      : struct semaphore *sema - 자원을 요청할 세마포어 객체
 * 반환값    : 없음
 * 관련 함수 : intr_context(), intr_disable(), thread_block()
 * 주의 사항 : 없음
 */
void
sema_down (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);
	ASSERT (!intr_context ());

	old_level = intr_disable ();
	while (sema->value == 0) { // 할당 가능한 세마포어가 없을 때 block
		list_insert_ordered (&(sema->waiters), &thread_current ()->elem, cmp_priority,NULL);
		thread_block ();
	}
	sema->value--;
	intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */


/*
 * 함수명    : sema_try_down
 * 설명      : 세마포어 값을 1 감소시켜 자원을 점유
 *             세마포어의 값이 0보다 크면 값을 감소시키고 true를 반환
 *             값이 0이면 자원이 없으므로 아무 동작 없이 false를 반환
 *             ※ sema_down()과는 달리, 대기(BLOCKED)하지 않음)
 * 인자      : struct semaphore *sema - 자원을 요청할 세마포어 객체
 * 반환값    : bool - 자원 점유 성공 여부 (true: 성공, false: 실패)
 * 관련 함수 : intr_disable(), intr_set_level()
 * 주의 사항 : 인터럽트 상태 복원 필요 (intr_disable() 호출 후 반드시 intr_set_level() 호출)
 */
bool
sema_try_down (struct semaphore *sema) {
	enum intr_level old_level;
	bool success;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level (old_level);

	return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */

/*
 * 함수명    : sema_up
 * 설명      : 세마포어 값을 1 증가시켜 자원을 반환한다.
 *             대기 중인 스레드가 있다면 하나를 꺼내어 ready_list에 넣고 실행 가능 상태로 만든다.
 *             대기 중인 스레드가 없으면 단순히 세마포어 값만 증가시킨다.
 * 인자      : struct semaphore *sema - 자원을 반환할 세마포어 객체
 * 반환값    : 없음
 * 관련 함수 : intr_disable(), thread_unblock(), intr_set_level()
 * 주의 사항 : 없음
 */
void
sema_up (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (!list_empty (&sema->waiters))
		thread_unblock (list_entry (list_pop_front (&sema->waiters),
					struct thread, elem));
	sema->value++;
	intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */

/*
 * 함수명    : sema_self_test
 * 설명      : 세마포어 동작을 테스트하는 함수이다.
 *             값이 0인 세마포어 두 개를 초기화한 뒤,
 *             하나는 helper 스레드에서 sema_down()을 실행하고,
 *             다른 하나는 메인 스레드에서 sema_up()을 실행하여
 *             세마포어 동기화가 제대로 작동하는지 확인한다.
 * 인자      : 없음
 * 반환값    : 없음
 * 관련 함수 : sema_init(), sema_up(), sema_down(), thread_create()
 * 주의 사항 : 세마포어 waiters 리스트 조작 시 인터럽트를 비활성화하고, 이후 복원할 것
 */
void
sema_self_test (void) {
	struct semaphore sema[2];
	int i;

	printf ("Testing semaphores...");
	sema_init (&sema[0], 0);
	sema_init (&sema[1], 0);
	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up (&sema[0]);
		sema_down (&sema[1]);
	}
	printf ("done.\n"); 
}

/* Thread function used by sema_self_test(). */
/*
 * 함수명    : sema_test_helper
 * 설명      : 세마포어 동작을 테스트하는 함수이다.
 *             값이 0인 세마포어 두 개를 초기화한 뒤,
 *             하나는 helper 스레드에서 sema_down()을 실행하고,
 *             다른 하나는 메인 스레드에서 sema_up()을 실행하여
 *             세마포어 동기화가 제대로 작동하는지 확인한다.
 * 인자      : void *sema_ 테스트를 진행할 값
 * 반환값    : 없음
 * 관련 함수 : sema_up(), sema_down()
 * 주의 사항 : 세마포어 waiters 리스트 조작 시 인터럽트를 비활성화하고, 이후 복원할 것
 */
static void
sema_test_helper (void *sema_) {
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down (&sema[0]);
		sema_up (&sema[1]);
	}
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */

/*
 * 함수명    : lock_init
 * 설명      : 락 구조체를 초기화한다.
 *             내부적으로 value가 1인 이진 세마포어를 생성하고,
 *             락을 보유한 스레드 포인터(holder)를 NULL로 설정한다.
 * 인자      : struct lock *lock - 초기화할 락 구조체 포인터
 * 반환값    : 없음
 * 관련 함수 : sema_init()
 * 주의 사항 : 없음
 */
void
lock_init (struct lock *lock) {
	ASSERT (lock != NULL);

	lock->holder = NULL;
	sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */

/*
 * 함수명    : lock_acquire
 * 설명      : 현재 실행 중인 스레드가 해당 락을 획득한다.
 *             내부적으로 세마포어를 사용하여 동기화하며,
 *             락 획득에 성공하면 holder를 현재 스레드로 설정한다.
 * 인자      : struct lock *lock - 획득할 락 구조체 포인터
 * 반환값    : 없음
 * 관련 함수 : sema_down(), lock_held_by_current_thread(), thread_current()
 * 주의 사항 : 락을 이미 보유 중인 상태에서 다시 호출하면 ASSERT 실패가 발생한다.
 */
void
lock_acquire (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (!intr_context ()); // 현재 코드가 인터럽트 핸들러 안에서 실행되고 있지 않음
	ASSERT (!lock_held_by_current_thread (lock)); // 락을 이미 보유한 스레드는 ASSERT발생

	sema_down (&lock->semaphore); // 세마포어 할당
	lock->holder = thread_current (); 
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */

/*
 * 함수명    : lock_try_acquire
 * 설명      : 현재 실행 중인 스레드가 해당 락을 비차단 방식으로 획득을 시도한다.
 *             락이 사용 중이면 대기하지 않고 즉시 false를 반환하며,
 *             락 획득에 성공하면 holder를 현재 스레드로 설정한다.
 * 인자      : struct lock *lock - 획득할 락 구조체 포인터
 * 반환값    : 성공 시 true, 실패 시 false
 * 관련 함수 : sema_try_down(), lock_held_by_current_thread(), thread_current()
 * 주의 사항 : 락을 이미 보유 중인 상태에서 다시 호출하면 ASSERT 실패가 발생한다.
 */
bool
lock_try_acquire (struct lock *lock) {
	bool success;

	ASSERT (lock != NULL);
	ASSERT (!lock_held_by_current_thread (lock)); // 데드락 방지 // 현재 실행 중인 스레드가 해당 락을 사용하고 있는지 확인

	success = sema_try_down (&lock->semaphore);
	if (success)
		lock->holder = thread_current (); 
	return success;
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */

/*
 * 함수명    : lock_release
 * 설명      : 현재 실행 중인 스레드가 해당 락을 해제한다.
 *             내부적으로 세마포어를 반환(sema_up)하고,
 *             락의 보유자(holder)를 NULL로 초기화한다.
 * 인자      : struct lock *lock - 해제할 락 구조체 포인터
 * 반환값    : 없음
 * 관련 함수 : sema_up(), lock_held_by_current_thread()
 * 주의 사항 : 락을 보유하지 않은 스레드가 호출하면 ASSERT 실패가 발생한다.
 */
void
lock_release (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (lock_held_by_current_thread (lock)); 

	lock->holder = NULL;
	sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */

/*
 * 함수명    : lock_held_by_current_thread
 * 설명      : 현재 실행 중인 스레드가 해당 락을 보유하고 있는지 여부를 확인한다.
 * 인자      : struct lock *lock - 확인할 락 구조체 포인터
 * 반환값    : 보유 중이면 true, 아니면 false
 * 관련 함수 : thread_current()
 * 주의 사항 : 없음
 */
bool
lock_held_by_current_thread (const struct lock *lock) {
	ASSERT (lock != NULL);

	return lock->holder == thread_current ();
}



/* One semaphore in a list. */

// 조건 변수에서 사용하는 개인 스레드용 세마포어 (일종의 1인 알람 시계 역할)
struct semaphore_elem {
    struct list_elem elem;              // condition의 waiters 리스트에 들어갈 리스트 노드
    struct semaphore semaphore;         // 현재 스레드만 기다리는 개인용 세마포어
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */

/*
 * 함수명    : cond_init
 * 설명      : 조건 변수 구조체를 초기화한다.
 *             조건을 기다리는 스레드들을 담을 waiters 리스트를 초기화한다.
 * 인자      : struct condition *cond - 초기화할 조건 변수 구조체 포인터
 * 반환값    : 없음
 * 관련 함수 : list_init()
 * 주의 사항 : 없음
 */
void
cond_init (struct condition *cond) {
	ASSERT (cond != NULL);

	list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */

/*
 * 함수명    : cond_wait
 * 설명      : 조건 변수에서 현재 스레드를 조건 만족 시까지 대기시킨다.
 *             대기 중인 스레드는 개인 세마포어에 의해 BLOCKED 되며,
 *             조건이 충족되어 signal을 받으면 다시 깨어나 lock을 재획득한다.
 * 인자      : struct condition *cond - 대기할 조건 변수
 *             struct lock *lock - 보호하고 있는 락 (wait 전 release하고, 깨어나면 다시 acquire)
 * 반환값    : 없음
 * 관련 함수 : sema_init(), sema_down(), lock_release(), lock_acquire()
 * 주의 사항 : 반드시 락을 소유한 상태에서 호출해야 하며, signal이 와도 조건이 만족됐는지 재확인 필요 (while 루프 안에서 써야 함)
 */
void
cond_wait (struct condition *cond, struct lock *lock) {
	struct semaphore_elem waiter;

	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock)); // 현재 실행 중인 스레드가 해당 lock 사용

	sema_init (&waiter.semaphore, 0);  // value가 0인 세마포어 구조체 생성
	list_insert_ordered (&cond->waiters, &waiter.elem, cmp_priority, 0); 
	lock_release (lock);              // 락을 놓고 다른 스레드가 조건을 충족시킬 수 있게 함
   sema_down (&waiter.semaphore);    // 현재 스레드를 BLOCKED 상태로 전환 (신호 오기 전까지 잠듦)
   lock_acquire (lock);              // 신호를 받고 깨어난 뒤 다시 락을 얻고 진행 재개
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */

/*
 * 함수명    : cond_signal
 * 설명      : 조건 변수에서 대기 중인 스레드가 있다면,
 *             그 중 하나를 선택하여 해당 스레드의 세마포어를 통해 깨운다.
 *             (즉, 조건이 충족되었음을 알리고 한 스레드만 재시작하게 함)
 * 인자      : struct condition *cond - 신호를 보낼 조건 변수
 *             struct lock *lock - 조건을 보호하는 락 (현재 스레드가 보유 중이어야 함)
 * 반환값    : 없음
 * 관련 함수 : sema_up(), list_entry(), list_pop_front(), lock_held_by_current_thread()
 * 주의 사항 : 반드시 락을 보유한 상태에서 호출해야 하며,
 *             signal을 받은 스레드도 조건이 실제로 만족됐는지 반드시 재확인해야 하므로,
 *             cond_wait()은 항상 while문 안에서 사용되어야 한다.
 */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	if (!list_empty (&cond->waiters))
		sema_up (&list_entry (list_pop_front (&cond->waiters),
					struct semaphore_elem, elem)->semaphore); // 대기 중인 스레드 중에 제일 앞에 있는 스레드 가져오기
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */

/*
 * 함수명    : cond_broadcast
 * 설명      : 조건 변수에서 대기 중인 스레드가 없을 때까지 계속해서
 * 			   대기 중인 스레드를 꺼내어 사용한다
 *             그 중 하나를 선택하여 해당 스레드의 세마포어를 통해 깨운다.
 * 인자      : struct condition *cond - 신호를 보낼 조건 변수
 *             struct lock *lock - 조건을 보호하는 락 (현재 스레드가 보유 중이어야 함)
 * 반환값    : 없음
 * 관련 함수 : cond_signal()
 * 주의 사항 : 
 */   
void
cond_broadcast (struct condition *cond, struct lock *lock) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);

	while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
}
