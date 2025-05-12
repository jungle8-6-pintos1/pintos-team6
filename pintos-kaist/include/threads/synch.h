#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>

// 세마포어 구조체
struct semaphore {
	unsigned value;             // 현재 사용 가능한 자원(또는 허용 가능한 접근 횟수)
	struct list waiters;        // 접근을 기다리고 있는 스레드 리스트(blocked 상태)
};

void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
bool sema_try_down (struct semaphore *);
void sema_up (struct semaphore *);
void sema_self_test (void);

// 락 구조체(뮤텍스)
struct lock {
	struct thread *holder;      // 현재 이 락을 가지고 있는(자원을 점유 중인) 스레드
	struct semaphore semaphore; // // 락을 구현하기 위해 사용하는 이진 세마포어(value = 1)
};

void lock_init (struct lock *);
void lock_acquire (struct lock *);
bool lock_try_acquire (struct lock *);
void lock_release (struct lock *);
bool lock_held_by_current_thread (const struct lock *);

// 조건 변수 구조체
struct condition {
	struct list waiters;        // 조건 변수에서 기다리는 스레드들의 리스트
};

void cond_init (struct condition *);
void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);
void cond_broadcast (struct condition *, struct lock *);

/* Optimization barrier.
 *
 * The compiler will not reorder operations across an
 * optimization barrier.  See "Optimization Barriers" in the
 * reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */
