#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"
#include <unistd.h>
#include <valgrind/valgrind.h>

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	int tid;
	void *stack;
	ucontext_t context;
};

/* This is the wait queue structure */
struct wait_queue {
	struct thread *node;
	struct wait_queue *next;
};

struct wait_queue *ready;
struct wait_queue *wait;

int thread_exists[THREAD_MAX_THREADS] = {0};
int num_threads = 0;
int num_wait = 0;

void pushToHead(struct wait_queue **q, struct wait_queue **item);
void pushToTail(struct wait_queue **q, struct wait_queue **item);
struct wait_queue* popHead(struct wait_queue **q);
struct wait_queue* findAndRemove(struct wait_queue **q, Tid id);
void thread_stub(void (*thread_main)(void *), void *arg);

void pushToHead(struct wait_queue **q, struct wait_queue **item)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	if (*q == NULL) 
	{
		*q = *item;
		(*q)->next = NULL;
	}
	
	else
	{
		struct wait_queue *old = *q;
		*q = *item;
		(*q)->next = old;
	}
	
	interrupts_set(enabled);
}

void pushToTail(struct wait_queue **q, struct wait_queue **item) 
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	if (*q == NULL) 
	{
		*q = *item;
		(*q)->next = NULL;
	}
	
	else
	{
		struct wait_queue *current = *q;
		
		while(current->next != NULL)
		{
			current = current->next;
		}

		current->next = *item;
		current->next->next = NULL;
	}

	interrupts_set(enabled);
}

struct wait_queue* popHead(struct wait_queue **q)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	struct wait_queue *current = *q;
	
	if(current != NULL)
	{
		*q = (*q)->next;
		current->next = NULL;
	}

	interrupts_set(enabled);
	return current;
}

struct wait_queue* findAndRemove(struct wait_queue **q, Tid id)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	struct wait_queue *current = *q;
	struct wait_queue *prev = NULL;

	while(current != NULL)
	{
		if(current->node->tid == id)
		{
			if (prev == NULL)
			{
				return popHead(q);
			}
			
			prev->next = current->next;
			current->next = NULL;
			return current;
		}
		
		prev = current;
		current = current->next;		
	}
	
	interrupts_set(enabled);
	return NULL;
}

void thread_stub(void (*thread_main)(void *), void *arg)
{
	interrupts_on();
	assert(interrupts_enabled());
	thread_main(arg);
	thread_exit();
}

void
thread_init(void)
{	
	int enabled = interrupts_off();
	assert(!interrupts_enabled());

	ready = wait_queue_create();
	
	if (ready == NULL) {
		exit(0);
	}
	
	ready->node = calloc(1,sizeof(struct thread));
	
	if (ready->node == NULL) {
		free(ready);
		exit(0);
	}
	
	int err = getcontext(&(ready->node->context));
	assert(!err);
	
	ready->node->tid = 0;
	ready->node->stack = NULL;
	
	num_threads += 1;
	thread_exists[0] = 1;

	wait = wait_queue_create();
	
	if (wait == NULL) {
		exit(0);
	}
	
	interrupts_set(enabled);
}

Tid
thread_id()
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	return ready->node->tid;
	interrupts_set(enabled);
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());

	if (num_threads == THREAD_MAX_THREADS) {
		interrupts_set(enabled);
		return THREAD_NOMORE;
	}

	struct wait_queue *new = wait_queue_create();
	
	if (new == NULL) {
		interrupts_set(enabled);
		return THREAD_NOMEMORY;
	}
	
	new->node = calloc(1,sizeof(struct thread));
	
	if (new->node == NULL) {
		free(new);
		interrupts_set(enabled);
		return THREAD_NOMEMORY;
	}
	
	new->node->stack = calloc(1,THREAD_MIN_STACK);
	
	if (new->node->stack == NULL) {
		free(new->node);
		free(new);
		interrupts_set(enabled);
		return THREAD_NOMEMORY;
	}
	
	num_threads += 1;

	for (int i = 0; i < THREAD_MAX_THREADS; i++) {
		if (thread_exists[i] == 0) {
			new->node->tid = i;
			thread_exists[i] = 1;
			break;
		}
	}

	new->next = NULL;

	getcontext(&(new->node->context));
	new->node->context.uc_mcontext.gregs[REG_RDI] = (greg_t) fn;
	new->node->context.uc_mcontext.gregs[REG_RSI] = (greg_t) parg;
	new->node->context.uc_mcontext.gregs[REG_RIP] = (greg_t) thread_stub;
	new->node->context.uc_mcontext.gregs[REG_RSP] = (greg_t) (new->node->stack + THREAD_MIN_STACK - 8);
	new->node->context.uc_mcontext.gregs[REG_RBP] = (greg_t) new->node->stack;
	new->node->context.uc_stack.ss_size = THREAD_MIN_STACK;
    new->node->context.uc_stack.ss_sp = new->node->stack;
    new->node->context.uc_stack.ss_flags = 0;

	pushToTail(&ready, &new);

	interrupts_set(enabled);
	return new->node->tid;
}

Tid
thread_yield(Tid want_tid)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	volatile int setcontext_flag = 0;
	
	if ((want_tid == THREAD_SELF) || (want_tid == ready->node->tid)) {
		interrupts_set(enabled);
		return  ready->node->tid;
	}
	
	if ((want_tid < -2) || (want_tid > THREAD_MAX_THREADS-1) || ((want_tid >= 0) && (thread_exists[want_tid] == 0))) {
		interrupts_set(enabled);
		return THREAD_INVALID;
	}

	if (want_tid == THREAD_ANY) {
		if (ready->next == NULL) {
			//printf("any yes\n");
			interrupts_set(enabled);
			return THREAD_NONE;
		}
		
/* 		int count = 0;
		struct wait_queue *tmp = ready;
		while (tmp != NULL) {
			count += 1;
			tmp = tmp->next;
		}
		printf("\t\t%d\t%d\n", count, num_threads); */
		
		unintr_printf("does it die?\n");
		
		unintr_printf("%p\n", ready);
		unintr_printf("%p\n", ready->node);
		unintr_printf("%p\n", ready->node->context);
		unintr_printf("%p\n", &(ready->node->context));

		getcontext(&(ready->node->context));
		
		if (setcontext_flag == 1) {
			interrupts_set(enabled);
			return ready->node->tid;
		}
		
		struct wait_queue *old_head = popHead(&ready);
		pushToTail(&ready, &old_head);
		
		setcontext_flag = 1;
		setcontext(&(ready->node->context));
		
		interrupts_set(enabled);
	}
	
/* 	int count = 0;
	struct wait_queue *tmp = ready;
	while (tmp != NULL) {
		count += 1;
		tmp = tmp->next;
	}
	unintr_printf("\t\t%d\t%d\n", count, num_threads); */

	getcontext(&(ready->node->context));
	
	if (setcontext_flag == 1) {
		interrupts_set(enabled);
		return want_tid;
	}

	struct wait_queue *old_head = popHead(&ready);
	pushToTail(&ready, &old_head);
	struct wait_queue *new_head = findAndRemove(&ready, want_tid);
	pushToHead(&ready, &new_head);

	setcontext_flag = 1;
	setcontext(&(ready->node->context));
	
	interrupts_set(enabled);
	return THREAD_FAILED;
}

void
thread_exit()
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());

	//printf("test\n");

	struct wait_queue *delete = popHead(&ready);
	
	thread_exists[delete->node->tid] = 0;
	num_threads -= 1;

	//printf("%d\n", num_threads);

	free(delete->node->stack);
	free(delete->node);
	free(delete);

	if (ready != NULL) {
		setcontext(&(ready->node->context));
	}

	interrupts_set(enabled);
	exit(0);
}

Tid
thread_kill(Tid tid)
{	
	int enabled = interrupts_off();
	assert(!interrupts_enabled());

	if (((tid < 0) || (tid > THREAD_MAX_THREADS-1)) || (thread_exists[tid] == 0) || (tid == ready->node->tid)) {
		interrupts_set(enabled);
		return THREAD_INVALID;
	}

	struct wait_queue *delete = ready;

	while (delete->node->tid != tid) {
		delete = delete->next;
		
		if (delete == NULL) {
			break;
		}
	}
	
	if (delete == NULL) {
		delete = wait;
		
		while (delete->node->tid != tid) {
			delete = delete->next;
		}
	}
	
	delete->node->context.uc_mcontext.gregs[REG_RIP] = (greg_t) thread_exit;

	interrupts_set(enabled);
	return delete->node->tid;
}


/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	struct wait_queue *wq;

	wq = calloc(1,sizeof(struct wait_queue));
	assert(wq);
	
	wq->next = NULL;

	interrupts_set(enabled);
	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	if (wq != NULL) {
		thread_wakeup(wq, 1);
	}
	
	free(wq);
	
	interrupts_set(enabled);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	volatile int setcontext_flag = 0;
	
	if (queue == NULL) {
		interrupts_set(enabled);
		return THREAD_INVALID;
	}
	
	if (ready == NULL || ready->next == NULL) {
		interrupts_set(enabled);
		return THREAD_NONE;
	}
	
	getcontext(&(ready->node->context));
		
	if (setcontext_flag == 1) {
		interrupts_set(enabled);
		return ready->node->tid;
	}
		
	struct wait_queue *old_head = popHead(&ready);
	pushToTail(&queue, &old_head);
	
	num_wait += 1;
	
	wait = queue;
		
	setcontext_flag = 1;
	setcontext(&(ready->node->context));	
		
	interrupts_set(enabled);
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	//unintr_printf("h1\n");
	
	if (queue == NULL || num_wait == 0) {
		interrupts_set(enabled);
		return 0;
	}
	
	//unintr_printf("h2\t%d\n", all);
	
	if (all == 0) {
		/* unintr_printf("%d\n", num_wait);
		
		int count = 0;
		struct wait_queue *tmp = ready;
		while (tmp != NULL) {
			count += 1;
			tmp = tmp->next;
		}
		unintr_printf("\t%d\t%d\n", count, num_threads-num_wait); */
		
		struct wait_queue *old_head = popHead(&queue);
		pushToTail(&ready, &old_head);
		
		num_wait -= 1;
		
		// unintr_printf("%d\n", num_wait);
		
		/* count = 0;
		tmp = ready;
		while (tmp != NULL) {
			count += 1;
			tmp = tmp->next;
		}
		unintr_printf("\t%d\t%d\n", count, num_threads-num_wait);	 */
		
		wait = queue;
		
		interrupts_set(enabled);
		return 1;
	}
	
	int count = num_wait;
	// unintr_printf("%d\n", num_wait);
	
	while (num_wait > 0) {
		struct wait_queue *old_head = popHead(&queue);
		pushToTail(&ready, &old_head);
		
		num_wait -= 1;
		
		// unintr_printf("%d\n", num_wait);
	}
	
	wait = queue;
	
	interrupts_set(enabled);
	return count;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());

	TBD();
	
	interrupts_set(enabled);
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	struct lock *lock;

	lock = calloc(1,sizeof(struct lock));
	assert(lock);

	TBD();

	interrupts_set(enabled);
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	assert(lock != NULL);

	TBD();
	
	interrupts_set(enabled);
	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	assert(lock != NULL);

	TBD();
	
	interrupts_set(enabled);
}

void
lock_release(struct lock *lock)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	assert(lock != NULL);

	TBD();
	
	interrupts_set(enabled);
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	struct cv *cv;

	cv = calloc(1,sizeof(struct cv));
	assert(cv);

	TBD();

	interrupts_set(enabled);
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	assert(cv != NULL);

	TBD();

	free(cv);
	
	interrupts_set(enabled);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
	
	interrupts_set(enabled);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
	
	interrupts_set(enabled);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_off();
	assert(!interrupts_enabled());
	
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
	
	interrupts_set(enabled);
}