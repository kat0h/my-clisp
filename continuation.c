#include "main.h"
#include "continuation.h"

value *mk_continuation_value(continuation *cont) {
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = CONTINUATION;
  E_CONTINUATION(e) = cont;
  return e;
}
void init_continuation(void *rbp) { main_rbp = rbp; }
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
    return e_value;
}
void _cc(continuation *c, void *value) {
  char *dst = c->rsp;
  char *src = c->stack;
  for (int i = c->stacklen; 0 <= --i;)
    *dst++ = *src++;
  e_value = value;
  longjmp(c->cont_reg, 1);
}
void call_continuation(continuation *c, void *value) {
  volatile void *q = 0;
  do {
    q = alloca(16);
  } while (q > c->rsp);
  _cc(c, value);
}
void free_continuation(continuation *c) { free(c->stack); }
value *ifunc_callcc(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("call/cc error: invalid number of arguments");
  value *lmd = eval(CAR(args), env);
  if (TYPEOF(lmd) != LAMBDA)
    throw("call/cc error: not lambda");
  continuation *cont = xmalloc(sizeof(continuation));
  value *r = get_continuation(cont);
  if (r == NULL) {
    // lambdaにcontinuationを渡して実行
    return eval_lambda(
        E_LAMBDA(lmd),
        E_CELL(mk_cell_value(mk_continuation_value(cont), mk_empty_cell_value())),
        env);
  } else {
    // continuationが呼ばれた場合
    return r;
  }
}
