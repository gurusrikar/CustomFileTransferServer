#include "q.h"

TCB_t *ReadyQ;

TCB_t *Curr_Thread;

int threadCounter = 0;

void start_thread(void (*thread_function), int *arg, char *fname)
{
    void *stack;
    TCB_t *tcb;

    stack = malloc(8192);
    tcb = NewItem();
    
    init_TCB (tcb, thread_function, stack, 8192, arg, fname);
    tcb->thread_id = ++threadCounter;

    AddQueue(ReadyQ, tcb);
}

void run()
{   // real code
	Curr_Thread = DelQueue(ReadyQ);
    ucontext_t parent;     // get a place to store the main context, for faking
    getcontext(&parent);   // magic sauce
    swapcontext(&parent, &(Curr_Thread->context));  // start the first thread
}
 

void yield() // similar to run
{
	TCB_t *Prev_Thread;
	AddQueue(ReadyQ, Curr_Thread);
	
	Prev_Thread = Curr_Thread;
	Curr_Thread = DelQueue(ReadyQ);
	//swap the context, from Prev_Thread to the thread pointed to Curr_Thread
	swapcontext(&(Prev_Thread->context), &(Curr_Thread->context));
}

//Yields without adding itself to the ReadyQ
void yieldNoReadyQ()
{
    TCB_t *Prev_Thread;
    
    Prev_Thread = Curr_Thread;
    Curr_Thread = DelQueue(ReadyQ);
    //swap the context, from Prev_Thread to the thread pointed to Curr_Thread
    swapcontext(&(Prev_Thread->context), &(Curr_Thread->context));
}

void printThreadId(TCB_t *tcb) {
	printf("%d\n", tcb->thread_id);
}