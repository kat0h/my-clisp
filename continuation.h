#ifndef CONTINUATION_H
#define CONTINUATION_H

#include <alloca.h>
#include <setjmp.h>
typedef struct {
  void *stack;
  unsigned long stacklen;
  void *rsp;
  jmp_buf cont_reg;
} continuation;

void init_continuation(void *rbp);
#define GETRSP(rsp) asm volatile("mov %%rsp, %0" : "=r"(rsp));
#define GETRBP(rbp) asm volatile("mov %%rbp, %0" : "=r"(rbp));
#define INIT_CONTINUATION()                                                    \
  {                                                                            \
    void *main_rbp;                                                            \
    GETRBP(main_rbp);                                                          \
    init_continuation(main_rbp);                                               \
  }
void *get_continuation(continuation *c);
void call_continuation(continuation *c, void *expr);
void free_continuation(continuation *c);

#endif
