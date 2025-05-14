#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#define SYMBOL_LEN_MAX 256

// https://github.com/tadd/my-c-lisp
#define throw(fmt, ...)                                                        \
  {                                                                            \
    fprintf(stderr, "%s:%d of %s: " fmt "\n", __FILE__, __LINE__,              \
            __func__ __VA_OPT__(, ) __VA_ARGS__);                              \
    exit(1);                                                                   \
  }

#define TYPEOF(x) (x->type)
#define E_NUMBER(x) (x->body.number)
#define E_SYMBOL(x) (x->body.symbol)
#define E_CELL(x) (x->body.cell)
#define E_LAMBDA(x) (x->body.lmd)
#define E_IFUNC(x) (x->body.func)
#define E_BOOLEAN(x) (x->body.boolean)
#define E_STRING(x) (x->body.string)
#define E_CONTINUATION(x) (x->body.cont)
#define CAR(x) (E_CELL(x)->car)
#define CDR(x) (E_CELL(x)->cdr)

void *xmalloc(size_t size);
// types
typedef struct Value value;
typedef struct Cell cell;
typedef struct Lambda lambda;
typedef struct Frame frame;
typedef struct KeyVal keyval;
typedef struct Continuation continuation;
typedef struct KV kv;
typedef value *(*ifunc)(value *, frame *);
struct Value {
  enum {
    NUMBER,
    SYMBOL,
    CELL,
    LAMBDA,
    IFUNC,
    BOOLEAN,
    STRING,
    CONTINUATION
  } type;
  union {
    float number;
    char *symbol;
    cell *cell;
    lambda *lmd;
    ifunc func;
    int boolean;
    char *string;
    continuation *cont;
  } body;
};
struct Cell {
  value *car;
  value *cdr;
};
int cell_len(cell *c);
struct Lambda {
  cell *args;
  value *body;
  frame *env;
};
// environment
struct Frame {
  frame *parent;
  kv *kv;
};
struct KV {
  char *key;
  value *value;
  kv *next;
};

value *eval(value *exp, frame *env);
value *eval_lambda(lambda *f, cell *args, frame *env);
value *mk_cell_value(value *car, value *cdr);
value *mk_empty_cell_value();

#endif
