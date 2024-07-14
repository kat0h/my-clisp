#include <stdlib.h>
#include <setjmp.h>
#include "continuation.h"
static void *main_rbp;
static void *e_expr;

void init_continuation(void *rbp) {
  main_rbp = rbp;
}
void *get_continuation(continuation *c) {
  void *rsp;
  GETRSP(rsp);
  c->rsp = rsp;
  c->stacklen = main_rbp - rsp + 1;
  c->stack = malloc(sizeof(char) * c->stacklen);
  char *dst = c->stack;
  char *src = c->rsp;
  for (int i = c->stacklen; 0 <= --i;)
    *dst++ = *src++;
  if (setjmp(c->cont_reg) == 0)
    return NULL;
  else
   return e_expr;
}
void _cc(continuation *c, void *expr) {
  char *dst = c->rsp;
  char *src = c->stack;
  for (int i = c->stacklen; 0 <= --i;)
    *dst++ = *src++;
  e_expr = expr;
  longjmp(c->cont_reg, 1);
}
void call_continuation(continuation *c, void *expr) {
  volatile void *q = 0;
  do {
    q=alloca(16);
  } while (q > c->rsp);
  _cc(c, expr);
}
void free_continuation(continuation *c) {
  free(c->stack);
}
