#include <ucontext.h>

void init_TCB (TCB_t *tcb, void *function, void *stackP, int stack_size, int *arg, char *fname)
{
    memset(tcb, '\0', sizeof(TCB_t));       // wash, rinse
    getcontext(&tcb->context);              // have to get parent context, else snow forms on hell
    tcb->context.uc_stack.ss_sp = stackP;
    tcb->context.uc_stack.ss_size = (size_t) stack_size;
    makecontext(&tcb->context, function, 2, arg, fname);// context is now cooked - function with one arguement
}