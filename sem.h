
#include "threads.h"

struct Semaphore_t {
	int counter;
	TCB_t *queue;
};

typedef struct Semaphore_t Semaphore_t;

Semaphore_t* CreateSem (int InputValue) {
	Semaphore_t *newSem = malloc(1 * sizeof(Semaphore_t));
	newSem->counter = InputValue;
	newSem->queue = newQueue();
	return newSem;
}

void P (Semaphore_t * sem) {
	sem->counter--;
	if(sem->counter < 0) {
		AddQueue(sem->queue, Curr_Thread);
		yieldNoReadyQ();
	}
}

void V (Semaphore_t *sem) {
    sem->counter++;
    if(sem->counter <= 0) {
    	TCB_t *tcb = DelQueue(sem->queue);
    	AddQueue(ReadyQ, tcb);
    	yield();
    } else {
    	yield();
    }
}