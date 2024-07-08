#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOL_LEN_MAX 256
// #define DEBUG

// utils
// https://github.com/tadd/my-c-lisp
#define throw(fmt, ...)                                                        \
  {                                                                            \
    fprintf(stderr, "%s:%d of %s: " fmt "\n", __FILE__, __LINE__,              \
            __func__ __VA_OPT__(, ) __VA_ARGS__);                              \
    exit(1);                                                                   \
  }
size_t MEMP = 0;
void *MEM[1000000] = {0};
void *xmalloc(size_t size) {
  void *p = malloc(size);
  MEM[MEMP++] = p;
  if (MEMP == 1000000) {
    throw("Internal Error xmalloc");
  }
  if (p == NULL) {
    throw("malloc failed");
  }
  return p;
}

// types
typedef struct Expr expr;
typedef struct Cell cell;
typedef struct Lambda lambda;
typedef struct Frame frame;
typedef struct KeyVal keyval;
typedef struct KV kv;
typedef expr *(*ifunc)(expr *, frame *);
struct Expr {
  enum { NUMBER, SYMBOL, CELL, LAMBDA, IFUNC, BOOLEAN } type;
  union {
    float number;
    char *symbol;
    cell *cell;
    lambda *lmd;
    ifunc func;
    int boolean;
  } body;
};
struct Cell {
  expr *car;
  expr *cdr;
};
struct Lambda {
  cell *args;
  expr *body;
  frame *env;
};
// environment
struct Frame {
  frame *parent;
  kv *kv;
};
struct KV {
  char *key;
  expr *value;
  kv *next;
};

// expr
#define E_NUMBER(x) (x->body.number)
#define E_SYMBOL(x) (x->body.symbol)
#define E_CELL(x) (x->body.cell)
#define E_LAMBDA(x) (x->body.lmd)
#define E_IFUNC(x) (x->body.func)
#define E_BOOLEAN(x) (x->body.boolean)
#define CAR(x) (E_CELL(x)->car)
#define CDR(x) (E_CELL(x)->cdr)
void print_list(cell *c);
void print_expr(expr *e) {
  if (e == NULL)
    return;
  switch (e->type) {
  case NUMBER:
    printf("%f", E_NUMBER(e));
    break;
  case SYMBOL:
    printf("%s", E_SYMBOL(e));
    break;
  case CELL:
    print_list(E_CELL(e));
    break;
  case LAMBDA:
    printf("LAMBDA ");
    print_list(E_LAMBDA(e)->args);
    break;
  case IFUNC:
    printf("IFUNC %p", E_IFUNC(e));
    break;
  case BOOLEAN:
    printf("%s", E_NUMBER(e) ? "#t" : "#f");
    break;
  }
}
void print_list(cell *c) {
  printf("(");
  while (c != NULL) {
    print_expr(c->car);
    if (E_CELL(c->cdr) != NULL) {
      printf(" ");
    }
    c = E_CELL(c->cdr);
  }
  printf(")");
}
expr *mk_number_expr(float number) {
  expr *e = xmalloc(sizeof(expr));
  e->type = NUMBER;
  E_NUMBER(e) = number;
  return e;
}
expr *mk_symbol_expr(char *symbol) {
  expr *e = xmalloc(sizeof(expr));
  e->type = SYMBOL;
  char *s = xmalloc(strlen(symbol) + 1);
  strcpy(s, symbol);
  E_SYMBOL(e) = s;
  return e;
}
expr *mk_empty_cell_expr() {
  expr *e = xmalloc(sizeof(expr));
  e->type = CELL;
  E_CELL(e) = NULL;
  return e;
}
expr *mk_cell_expr(expr *car, expr *cdr) {
  expr *e = xmalloc(sizeof(expr));
  e->type = CELL;
  E_CELL(e) = xmalloc(sizeof(cell));
  CAR(e) = car;
  E_CELL(e)->cdr = cdr;
  return e;
}
expr *mk_lambda_expr(cell *args, expr *body, frame *env) {
  expr *e = xmalloc(sizeof(expr));
  e->type = LAMBDA;
  E_LAMBDA(e) = xmalloc(sizeof(lambda));
  E_LAMBDA(e)->args = args;
  E_LAMBDA(e)->body = body;
  E_LAMBDA(e)->env = env;
  return e;
}
expr *mk_boolean_expr(int b) {
  expr *e = xmalloc(sizeof(expr));
  e->type = BOOLEAN;
  E_BOOLEAN(e) = b;
  return e;
}
expr *mk_ifunc_expr(ifunc f) {
  expr *e = xmalloc(sizeof(expr));
  e->type = IFUNC;
  E_IFUNC(e) = f;
  return e;
}
int cell_len(cell *c) {
  int len = 0;
  while (c != NULL) {
    len++;
    c = E_CELL(c->cdr);
  }
  return len;
}
int truish(expr *e) {
  // truish: E → T
  // truish = λε . (ε = false → false, true)
  if (e->type == BOOLEAN) {
    return E_BOOLEAN(e);
  }
  return 1;
}

// env
frame *make_frame(frame *parent) {
  frame *f = xmalloc(sizeof(frame));
  f->parent = parent;
  f->kv = NULL;
  return f;
}
void add_kv_to_frame(frame *env, char *symbol, expr *value) {
  kv *i = xmalloc(sizeof(kv));
  i->key = symbol;
  i->value = value;
  i->next = env->kv;
  env->kv = i;
}
kv *find_pair_in_current_frame(frame *env, char *symbol) {
  kv *i = env->kv;
  while (i != NULL) {
    if (strcmp(i->key, symbol) == 0)
      return i;
    i = i->next;
  }
  return NULL;
}
kv *find_pair_recursive(frame *env, char *symbol) {
  if (env == NULL) {
    throw("symbol %s not found", symbol);
  }
  kv *i = find_pair_in_current_frame(env, symbol);
  if (i != NULL)
    return i;
  return find_pair_recursive(env->parent, symbol);
}
expr *define_to_env(frame *env, char *symbol, expr *value) {
  add_kv_to_frame(env, symbol, value);
  return mk_symbol_expr(symbol);
}
expr *set_to_env(frame *env, char *symbol, expr *value) {
  kv *i = find_pair_recursive(env, symbol);
  if (i == NULL)
    throw("symbol %s not found", symbol);
  i->value = value;
  return mk_symbol_expr(symbol);
}
expr *lookup_frame(frame *env, char *symbol) {
  kv *v = find_pair_recursive(env, symbol);
  return v->value;
}
void print_frame(frame *env) {
  while (env != NULL) {
    puts("....Frame...........................................");
    kv *i = env->kv;
    while (i != NULL) {
      printf("  %s: ", i->key);
      print_expr(i->value);
      puts("");
      i = i->next;
    }
    env = env->parent;
  }
  puts("....................................................\n");
}

// parser
char *input;
void skip_ws() {
  while (*input == ' ' || *input == '\n' || *input == '\t')
    input++;
}
int is_symbol_char() {
  return ('a' <= *input && *input <= 'z') || ('A' <= *input && *input <= 'Z') ||
         *input == '_' || *input == '!' || *input == '+' || *input == '-' ||
         *input == '*' || *input == '/' || ('0' <= *input && *input <= '9');
}
expr *parse_hash_literal() {
  if (*input != '#')
    throw("parse error: not hash literal");
  input++;
  if (*input == 't') {
    input++;
    return mk_boolean_expr(1);
  } else if (*input == 'f') {
    input++;
    return mk_boolean_expr(0);
  } else {
    throw("parse error: unexpected token %c", *input);
  }
}
expr *parse_list();
expr *parse_expr() {
#ifdef DEBUG
  printf("parse_expr: %s\n", input);
#endif
  // numeber
  if ('0' <= *input && *input <= '9') {
    return mk_number_expr(strtof(input, &input));
    // symbol
  } else if (is_symbol_char()) {
    char buf[SYMBOL_LEN_MAX];
    int i = 0;
    while (is_symbol_char()) {
      buf[i++] = *input++;
      if (i == SYMBOL_LEN_MAX - 1) {
        throw("symbol is too long");
      }
    }
    buf[i] = '\0';
    return mk_symbol_expr(buf);
  } else if (*input == '#') {
    return parse_hash_literal();
  } else if (*input == '(') {
    input++;
    return parse_list();
  }
  throw("Unexpected expression '%c' \"%s\"", *input, input);
}
expr *parse_list() {
#ifdef DEBUG
  printf("parse_list: %s\n", input);
#endif
  skip_ws();
  if (*input == ')') {
    input++;
    return mk_empty_cell_expr();
  }
  expr *e = xmalloc(sizeof(expr));
  e->type = CELL;
  E_CELL(e) = xmalloc(sizeof(cell));
  CAR(e) = parse_expr();
  skip_ws();
  E_CELL(e)->cdr = parse_list();
  skip_ws();
  return e;
}
expr *parse_paren() {
#ifdef DEBUG
  printf("parse_paren: %s\n", input);
#endif
  if (*input == '(') {
    input++;
    skip_ws();
    expr *e = parse_list();
    skip_ws();
    return e;
  }
  throw("Unexpected token %c", *input);
}
expr *parse_program(char *prg) {
#ifdef DEBUG
  printf("parse_program: %s\n", prg);
#endif
  input = prg;
  skip_ws();
  expr *e;
  if (*input == '(')
    e = parse_paren();
  else
    e = parse_expr();
  if (*input != '\0') {
    throw("parser error input is not empty \"%s\"", input);
  }
  return e;
}

// eval
expr *eval_cell(expr *exp, frame *env);
expr *eval(expr *exp, frame *env) {
  if (exp == NULL) {
    throw("eval error: exp is NULL");
  }
  switch (exp->type) {
  case (NUMBER):
    // NUMBERはそれ以上評価できない終端の値
    return exp;
  case (SYMBOL):
    // 環境からSYMBOLの値を探す
    return lookup_frame(env, E_SYMBOL(exp));
  case (CELL):
    return eval_cell(exp, env);
  case (LAMBDA):
    // LAMBDAはそれ以上評価できない終端の値
    return exp;
  case (IFUNC):
    // IFUNCはそれ以上評価できない終端の値
    return exp;
  case (BOOLEAN):
    // BOOLEANはそれ以上評価できない終端の値
    return exp;
  }
  throw("Not implemented");
}
expr *eval_lambda(lambda *f, cell *args, frame *env);
expr *eval_cell(expr *exp, frame *env) {
  if (exp == NULL) {
    throw("eval error: exp is NULL");
  }
  if (exp->type != CELL) {
    throw("eval error: exp is not CELL");
  }
  // 空リストは妥当な式ではない
  if (E_CELL(exp) == NULL) {
    return exp;
  }
  // 関数を取得
  expr *func = eval(CAR(exp), env);
  expr *args = E_CELL(exp)->cdr;
  if (func->type == IFUNC) {
    return E_IFUNC(func)(args, env);
  } else if (func->type == LAMBDA) {
    if (args->type != CELL)
      throw("eval error: args is not CELL");
    return eval_lambda(E_LAMBDA(func), E_CELL(args), env);
  }
  throw("call error: not callable");
}
expr *eval_lambda(lambda *f, cell *args, frame *env) {
  frame *newenv = make_frame(f->env);
  cell *fargs = f->args;
  int fargc = cell_len(fargs);
  int argc = cell_len(args);
  if (fargc != argc)
    throw("lambda error: argument count mismatch expect %d but got %d", fargc,
          argc);
  while (fargs != NULL) {
    add_kv_to_frame(newenv, E_SYMBOL(fargs->car), eval(args->car, env));
    fargs = E_CELL(fargs->cdr);
    args = E_CELL(args->cdr);
  }
  return eval(f->body, newenv);
}

// internal func
expr *ifunc_add(expr *args, frame *env) {
  float sum = 0;
  while (E_CELL(args) != NULL) {
    expr *i = eval(CAR(args), env);
    if (i->type != NUMBER) {
      throw("add error: not number");
    }
    sum += E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_expr(sum);
}
expr *ifunc_sub(expr *args, frame *env) {
  float sum = 0;
  while (E_CELL(args) != NULL) {
    expr *i = eval(CAR(args), env);
    if (i->type != NUMBER) {
      throw("sub error: not number");
    }
    sum -= E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_expr(sum);
}
expr *ifunc_mul(expr *args, frame *env) {
  float sum = 0;
  while (E_CELL(args) != NULL) {
    expr *i = eval(CAR(args), env);
    if (i->type != NUMBER) {
      throw("mul error: not number");
    }
    sum *= E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_expr(sum);
}
expr *ifunc_div(expr *args, frame *env) {
  float sum = 0;
  while (E_CELL(args) != NULL) {
    expr *i = eval(CAR(args), env);
    if (i->type != NUMBER) {
      throw("mul error: not number");
    }
    if (E_NUMBER(i) == 0)
      throw("div error: zero division");
    sum /= E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_expr(sum);
}
expr *ifunc_begin(expr *args, frame *env) {
  expr *i = mk_number_expr(0);
  while (E_CELL(args) != NULL) {
    i = eval(CAR(args), env);
    args = CDR(args);
  }
  return i;
}
expr *ifunc_define(expr *args, frame *env) {
  if (E_CELL(args) == NULL) {
    throw("define error: no symbol");
  }
  if (CAR(args)->type != SYMBOL) {
    throw("define error: symbol is not symbol");
  }
  char *symbol = E_SYMBOL(CAR(args));
  args = CDR(args);
  if (E_CELL(args) == NULL) {
    throw("define error: too few arguments");
  }
  expr *value = eval(CAR(args), env);
  if (E_CELL(CDR(args)) != NULL) {
    throw("define error: too many arguments");
  }
  return define_to_env(env, symbol, value);
}
expr *ifunc_setbang(expr *args, frame *env) {
  if (E_CELL(args) == NULL) {
    throw("define error: no symbol");
  }
  if (CAR(args)->type != SYMBOL) {
    throw("define error: symbol is not symbol");
  }
  char *symbol = E_SYMBOL(CAR(args));
  args = CDR(args);
  if (E_CELL(args) == NULL) {
    throw("define error: too few arguments");
  }
  expr *value = eval(CAR(args), env);
  if (E_CELL(CDR(args)) != NULL) {
    throw("define error: too many arguments");
  }
  return set_to_env(env, symbol, value);
}
expr *ifunc_showenv(expr *args, frame *env) {
  if (E_CELL(args) != NULL) {
    throw("showenv error: too many arguments");
  }
  print_frame(env);
  return mk_number_expr(0);
}
int check_args(expr *args) {
  // argsはリストでないとならない
  if (args->type != CELL)
    return 0;
  // 空のリストはargsとして妥当
  if (E_CELL(args) == NULL)
    return 1;
  // 各要素はSYMBOLでないとならない
  if (CAR(args)->type != SYMBOL)
    return 0;
  // 残りの要素も再帰的にチェック
  return check_args(CDR(args));
}
expr *ifunc_lambda(expr *args, frame *env) {
  // (lambda (args) body)
  if (E_CELL(args) == NULL)
    throw("lambda error: no args");
  expr *first = CAR(args);
  if (!check_args(first))
    throw("lambda error: args is not list of symbol");
  cell *largs = E_CELL(first);
  if (E_CELL(CDR(args)) == NULL)
    throw("lambda error: no body");
  expr *body = CAR(CDR(args));
  if (E_CELL(CDR(CDR(args))) != NULL)
    throw("lambda error: too many body");
  return mk_lambda_expr(largs, body, env);
}
expr *ifunc_print(expr *args, frame *env) {
  while (E_CELL(args) != NULL) {
    print_expr(eval(CAR(args), env));
    puts("");
    args = CDR(args);
  }
  return mk_number_expr(0);
}
expr *ifunc_if(expr *args, frame *env) {
  if (cell_len(E_CELL(args)) != 2 && cell_len(E_CELL(args)) != 3)
    throw("if error: invalid number of arguments");
  expr *cond = eval(CAR(args), env);
  if (truish(cond)) {
    return eval(CAR(CDR(args)), env);
  } else {
    return eval(CAR(CDR(CDR(args))), env);
  }
}
expr *ifunc_quote(expr *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("quote error: invalid number of arguments");
  return CAR(args);
}

// main
frame *mk_initial_env() {
  frame *env = make_frame(NULL);
  add_kv_to_frame(env, "+", mk_ifunc_expr(ifunc_add));
  add_kv_to_frame(env, "-", mk_ifunc_expr(ifunc_sub));
  add_kv_to_frame(env, "*", mk_ifunc_expr(ifunc_mul));
  add_kv_to_frame(env, "/", mk_ifunc_expr(ifunc_div));
  add_kv_to_frame(env, "begin", mk_ifunc_expr(ifunc_begin));
  add_kv_to_frame(env, "define", mk_ifunc_expr(ifunc_define));
  add_kv_to_frame(env, "set!", mk_ifunc_expr(ifunc_setbang));
  add_kv_to_frame(env, "showenv", mk_ifunc_expr(ifunc_showenv));
  add_kv_to_frame(env, "lambda", mk_ifunc_expr(ifunc_lambda));
  add_kv_to_frame(env, "print", mk_ifunc_expr(ifunc_print));
  add_kv_to_frame(env, "if", mk_ifunc_expr(ifunc_if));
  add_kv_to_frame(env, "quote", mk_ifunc_expr(ifunc_quote));
  return env;
}
int main(int argc, char *argv[]) {
  if (argc < 2) {
    throw("Usage: %s <program>", argv[0]);
    return 1;
  }

  expr *program = parse_program(argv[1]);
  printf("Program: ");
  print_expr(program);
  puts("");
  frame *environ = mk_initial_env();
  // expr *res =
  eval(program, environ);
  // 評価結果を表示
  // printf("=> ");
  // print_expr(res);
  // puts("");
  for (int i = 0; i < MEMP; i++) {
    // printf("MEM[%d] = %p\n", i, MEM[i]);
    free(MEM[i]);
  }
}
