#ifndef CONTINUATION_H
#define CONTINUATION_H
#include <setjmp.h>

typedef struct Expr expr;
typedef struct Continuation continuation;
struct Continuation {
  void *stack;
  unsigned long stacklen;
  void *rsp;
  jmp_buf cont_reg;
};
void init_continuation(void *rbp);
#ifdef __x86_64__    
#define GETRSP(rsp) asm volatile("mov %%rsp, %0" : "=r"(rsp));
#define GETRBP(rbp) asm volatile("mov %%rbp, %0" : "=r"(rbp));
#else
#error "ARM is not supported"
#endif
#define INIT_CONTINUATION()                                                    \
  {                                                                            \
    void *main_rbp;                                                            \
    GETRBP(main_rbp);                                                          \
    init_continuation(main_rbp);                                               \
  }
void *get_continuation(continuation *c);
void call_continuation(continuation *c, void *expr);
void free_continuation(continuation *c);
expr *mk_continuation_expr(continuation *cont);
expr *ifunc_callcc(expr *args, frame *env);

static void *main_rbp;
// 継続の返り値を受け渡すための変数
static void *e_expr;

#endif
