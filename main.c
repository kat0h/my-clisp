#include <alloca.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "main.h"
#include "continuation.h"

// #define DEBUG

// utils
size_t MEMP = 0;
void *MEM[1000000] = {0};
void *xmalloc(size_t size) {
  void *p = malloc(size);
  // MEM[MEMP++] = p;
  // if (MEMP == 1000000) {
  //   throw("Internal Error xmalloc");
  // }
  if (p == NULL)
    throw("malloc failed");
  return p;
}


// expr
void print_list(cell *c);
void print_expr(expr *e) {
  if (e == NULL)
    return;
  switch (TYPEOF(e)) {
  case NUMBER: {
    float n = E_NUMBER(e);
    if (n - (float)(int)n == 0.0)
      printf("%d", (int)n);
    else
      printf("%f", n);
    break;
  }
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
  case STRING:
    printf("%s", E_STRING(e));
    break;
  case CONTINUATION:
    printf("CONTINUATION");
    break;
  }
}
void print_list(cell *c) {
  printf("(");
  while (c != NULL) {
    print_expr(c->car);
    if (TYPEOF(c->cdr) != CELL) {
      printf(" . ");
      print_expr(c->cdr);
      break;
    }
    if (E_CELL(c->cdr) != NULL) {
      printf(" ");
    }
    c = E_CELL(c->cdr);
  }
  printf(")");
}
expr *mk_number_expr(float number) {
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = NUMBER;
  E_NUMBER(e) = number;
  return e;
}
expr *mk_symbol_expr(char *symbol) {
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = SYMBOL;
  char *s = xmalloc(strlen(symbol) + 1);
  strcpy(s, symbol);
  E_SYMBOL(e) = s;
  return e;
}
expr *mk_empty_cell_expr() {
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = CELL;
  E_CELL(e) = NULL;
  return e;
}
expr *mk_cell_expr(expr *car, expr *cdr) {
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = CELL;
  E_CELL(e) = xmalloc(sizeof(cell));
  CAR(e) = car;
  CDR(e) = cdr;
  return e;
}
expr *mk_lambda_expr(cell *args, expr *body, frame *env) {
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = LAMBDA;
  E_LAMBDA(e) = xmalloc(sizeof(lambda));
  E_LAMBDA(e)->args = args;
  E_LAMBDA(e)->body = body;
  E_LAMBDA(e)->env = env;
  return e;
}
expr *mk_boolean_expr(int b) {
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = BOOLEAN;
  E_BOOLEAN(e) = b;
  return e;
}
expr *mk_ifunc_expr(ifunc f) {
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = IFUNC;
  E_IFUNC(e) = f;
  return e;
}
expr *mk_string_expr(char *str) {
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = STRING;
  char *s = xmalloc(strlen(str) + 1);
  strcpy(s, str);
  E_STRING(e) = s;
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
  if (TYPEOF(e) == BOOLEAN) {
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
         *input == '=' || *input == '<' || *input == '>' || *input == '*' ||
         *input == '/' || ('0' <= *input && *input <= '9');
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
  } else if (*input == '"') {
    input++;
    // TODO
    char buf[SYMBOL_LEN_MAX];
    int i = 0;
    while (*input != '"') {
      buf[i++] = *input++;
      if (i == SYMBOL_LEN_MAX - 1) {
        throw("string is too long");
      }
    }
    input++;
    buf[i] = '\0';
    return mk_string_expr(buf);
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
  TYPEOF(e) = CELL;
  E_CELL(e) = xmalloc(sizeof(cell));
  CAR(e) = parse_expr();
  skip_ws();
  CDR(e) = parse_list();
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
expr *parse_program_list() {
#ifdef DEBUG
  printf("parse_program_list: %s\n", input);
#endif
  if (*input == '\0')
    return mk_empty_cell_expr();
  expr *e = xmalloc(sizeof(expr));
  TYPEOF(e) = CELL;
  E_CELL(e) = xmalloc(sizeof(cell));
  CAR(e) = parse_paren();
  skip_ws();
  CDR(e) = parse_program_list();
  skip_ws();
  return e;
}
expr *parse_program(char *prg) {
#ifdef DEBUG
  printf("parse_program: %s\n", prg);
#endif
  input = prg;
  expr *e = parse_program_list();
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
  switch (TYPEOF(exp)) {
  case NUMBER:
    // NUMBERはそれ以上評価できない終端の値
    return exp;
  case SYMBOL:
    // 環境からSYMBOLの値を探す
    return lookup_frame(env, E_SYMBOL(exp));
  case CELL:
    return eval_cell(exp, env);
  case LAMBDA:
    // LAMBDAはそれ以上評価できない終端の値
    return exp;
  case IFUNC:
    // IFUNCはそれ以上評価できない終端の値
    return exp;
  case BOOLEAN:
    // BOOLEANはそれ以上評価できない終端の値
    return exp;
  case STRING:
    // STRINGはそれ以上評価できない終端の値
    return exp;
  case CONTINUATION:
    return exp;
  }
  throw("Not implemented");
}

expr *eval_top(expr *exp, frame *env) {
  INIT_CONTINUATION();
  return eval(exp, env);
}

expr *eval_lambda(lambda *f, cell *args, frame *env);
expr *eval_cell(expr *exp, frame *env) {
  if (exp == NULL) {
    throw("eval error: exp is NULL");
  }
  if (TYPEOF(exp) != CELL) {
    throw("eval error: exp is not CELL");
  }
  // 空リストは妥当な式ではない
  if (E_CELL(exp) == NULL) {
    return exp;
  }
  // 関数を取得
  expr *func = eval(CAR(exp), env);
  expr *args = CDR(exp);
  if (TYPEOF(func) == IFUNC) {
    return E_IFUNC(func)(args, env);
  } else if (TYPEOF(func) == LAMBDA) {
    if (TYPEOF(args) != CELL)
      throw("eval error: args is not CELL");
    return eval_lambda(E_LAMBDA(func), E_CELL(args), env);
  } else if (TYPEOF(func) == CONTINUATION) {
    continuation *cont = E_CONTINUATION(func);
    // 引数の数をチェック
    if (cell_len(E_CELL(args)) > 1)
      throw("call/cc error: invalid number of arguments");
    if (cell_len(E_CELL(args)) == 0)
      call_continuation(cont, mk_empty_cell_expr());
    call_continuation(cont, eval(CAR(args), env));
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
    if (TYPEOF(i) != NUMBER)
      throw("add error: not number");
    sum += E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_expr(sum);
}
expr *ifunc_sub(expr *args, frame *env) {
  expr *first = eval(CAR(args), env);
  if (TYPEOF(first) != NUMBER)
    throw("sub error: not number");
  float sum = E_NUMBER(first);
  if (cell_len(E_CELL(args)) == 1)
    return mk_number_expr(-sum);
  args = CDR(args);
  while (E_CELL(args) != NULL) {
    expr *i = eval(CAR(args), env);
    if (TYPEOF(i) != NUMBER)
      throw("sub error: not number");
    sum -= E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_expr(sum);
}
expr *ifunc_mul(expr *args, frame *env) {
  float sum = 0;
  while (E_CELL(args) != NULL) {
    expr *i = eval(CAR(args), env);
    if (TYPEOF(i) != NUMBER) {
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
    if (TYPEOF(i) != NUMBER)
      throw("mul error: not number");
    if (E_NUMBER(i) == 0)
      throw("div error: zero division");
    sum /= E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_expr(sum);
}
expr *ifunc_modulo(expr *args, frame *env) {
  if (cell_len(E_CELL(args)) != 2)
    throw("modulo error: invalid number of arguments");
  expr *a = eval(CAR(args), env);
  expr *b = eval(CAR(CDR(args)), env);
  if (TYPEOF(a) != NUMBER || TYPEOF(b) != NUMBER)
    throw("modulo error: not number");
  int ia = (int)E_NUMBER(a);
  int ib = (int)E_NUMBER(b);
  return mk_number_expr(ia % ib);
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
  if (TYPEOF(CAR(args)) != SYMBOL) {
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
  if (TYPEOF(CAR(args)) != SYMBOL) {
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
  if (TYPEOF(args) != CELL)
    return 0;
  // 空のリストはargsとして妥当
  if (E_CELL(args) == NULL)
    return 1;
  // 各要素はSYMBOLでないとならない
  if (TYPEOF(CAR(args)) != SYMBOL)
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
    if (cell_len(E_CELL(args)) == 2)
      return mk_empty_cell_expr();
    return eval(CAR(CDR(CDR(args))), env);
  }
}
expr *ifunc_quote(expr *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("quote error: invalid number of arguments");
  return CAR(args);
}
enum { EQ = 0, LT, LE, GT, GE };
int comp(expr *args, frame *env, char type) {
  int len = cell_len(E_CELL(args));
  if (len < 2)
    throw("comp error: too few arguments");
  expr *car = eval(CAR(args), env);
  if (TYPEOF(car) != NUMBER)
    throw("comp error: not number");
  expr *cdr;
  int result = 1;
  for (int i = 0; i < len - 1; i++) {
    cdr = eval(CAR(CDR(args)), env);
    if (TYPEOF(cdr) != NUMBER)
      throw("comp error: not number");
    switch (type) {
    case EQ: // =
      result &= E_NUMBER(car) == E_NUMBER(cdr);
      break;
    case LT: // <
      result &= E_NUMBER(car) < E_NUMBER(cdr);
      break;
    case LE: // <=
      result &= E_NUMBER(car) <= E_NUMBER(cdr);
      break;
    case GT: // >
      result &= E_NUMBER(car) > E_NUMBER(cdr);
      break;
    case GE: // >=
      result &= E_NUMBER(car) >= E_NUMBER(cdr);
      break;
    }
    car = cdr;
    args = CDR(args);
  }
  return result;
}
expr *ifunc_eq(expr *args, frame *env) {
  return mk_boolean_expr(comp(args, env, EQ));
}
expr *ifunc_lt(expr *args, frame *env) {
  return mk_boolean_expr(comp(args, env, LT));
}
expr *ifunc_le(expr *args, frame *env) {
  return mk_boolean_expr(comp(args, env, LE));
}
expr *ifunc_gt(expr *args, frame *env) {
  return mk_boolean_expr(comp(args, env, GT));
}
expr *ifunc_ge(expr *args, frame *env) {
  return mk_boolean_expr(comp(args, env, GE));
}
expr *ifunc_and(expr *args, frame *env) {
  while (E_CELL(args) != NULL) {
    int i = truish(eval(CAR(args), env));
    if (i == 0)
      return mk_boolean_expr(0);
    args = CDR(args);
  }
  return mk_boolean_expr(1);
}
expr *ifunc_or(expr *args, frame *env) {
  while (E_CELL(args) != NULL) {
    int i = truish(eval(CAR(args), env));
    if (i)
      return mk_boolean_expr(1);
    args = CDR(args);
  }
  return mk_boolean_expr(0);
}
expr *eval_list(expr *args, frame *env, expr *default_value) {
  expr *i = default_value;
  while (E_CELL(args) != NULL) {
    i = eval_top(CAR(args), env);
    args = CDR(args);
  }
  return i;
}
expr *ifunc_cond(expr *args, frame *env) {
  while (E_CELL(args) != NULL) {
    // (cond list* (else expr*)?)
    // list = (expr*)
    expr *list = CAR(args);
    if (TYPEOF(list) != CELL)
      throw("cond error: not list");
    expr *cond;
    while (E_CELL(list) != NULL) {
      if (TYPEOF(CAR(list)) == SYMBOL &&
          strcmp(E_SYMBOL(CAR(list)), "else") == 0) {
        // elseのあとをチェック
        if (E_CELL(CDR(args)) != NULL)
          throw("cond error: else is not last");
        return eval_list(CDR(list), env, mk_number_expr(0));
      }
      cond = eval(CAR(list), env);
      if (truish(cond))
        return eval_list(CDR(list), env, cond);
      else
        break;
      list = CDR(list);
    }
    args = CDR(args);
  }
  return mk_empty_cell_expr();
}
expr *ifunc_cons(expr *args, frame *env) {
  if (cell_len(E_CELL(args)) != 2)
    fprintf(stderr, "cons error: invalid number of arguments");
  expr *car = eval(CAR(args), env);
  expr *cdr = eval(CAR(CDR(args)), env);
  return mk_cell_expr(car, cdr);
}
expr *ifunc_car(expr *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("car error: invalid number of arguments");
  expr *c = eval(CAR(args), env);
  if (TYPEOF(c) != CELL)
    throw("car error: not pair");
  return CAR(c);
}
expr *ifunc_cdr(expr *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("car error: invalid number of arguments");
  expr *c = eval(CAR(args), env);
  if (TYPEOF(c) != CELL)
    throw("car error: not pair");
  return CDR(c);
}

// main
frame *mk_initial_env() {
  frame *env = make_frame(NULL);
  add_kv_to_frame(env, "+", mk_ifunc_expr(ifunc_add));
  add_kv_to_frame(env, "-", mk_ifunc_expr(ifunc_sub));
  add_kv_to_frame(env, "*", mk_ifunc_expr(ifunc_mul));
  add_kv_to_frame(env, "/", mk_ifunc_expr(ifunc_div));
  add_kv_to_frame(env, "modulo", mk_ifunc_expr(ifunc_modulo));
  add_kv_to_frame(env, "begin", mk_ifunc_expr(ifunc_begin));
  add_kv_to_frame(env, "define", mk_ifunc_expr(ifunc_define));
  add_kv_to_frame(env, "set!", mk_ifunc_expr(ifunc_setbang));
  add_kv_to_frame(env, "showenv", mk_ifunc_expr(ifunc_showenv));
  add_kv_to_frame(env, "lambda", mk_ifunc_expr(ifunc_lambda));
  add_kv_to_frame(env, "print", mk_ifunc_expr(ifunc_print));
  add_kv_to_frame(env, "if", mk_ifunc_expr(ifunc_if));
  add_kv_to_frame(env, "quote", mk_ifunc_expr(ifunc_quote));
  add_kv_to_frame(env, "=", mk_ifunc_expr(ifunc_eq));
  add_kv_to_frame(env, "<", mk_ifunc_expr(ifunc_lt));
  add_kv_to_frame(env, "<=", mk_ifunc_expr(ifunc_le));
  add_kv_to_frame(env, ">", mk_ifunc_expr(ifunc_gt));
  add_kv_to_frame(env, ">=", mk_ifunc_expr(ifunc_ge));
  add_kv_to_frame(env, "and", mk_ifunc_expr(ifunc_and));
  add_kv_to_frame(env, "or", mk_ifunc_expr(ifunc_or));
  add_kv_to_frame(env, "cond", mk_ifunc_expr(ifunc_cond));
  add_kv_to_frame(env, "cons", mk_ifunc_expr(ifunc_cons));
  add_kv_to_frame(env, "call/cc", mk_ifunc_expr(ifunc_callcc));
  add_kv_to_frame(env, "car", mk_ifunc_expr(ifunc_car));
  add_kv_to_frame(env, "cdr", mk_ifunc_expr(ifunc_cdr));
  return env;
}
void repl() {
  puts("kat0h's scheme");
  char buf[1024];
  frame *environ = mk_initial_env();
  printf("scm> ");
  while (fgets(buf, 1024, stdin) != NULL) {
    input = buf;
    if (*input == '\n') {
      printf("scm> ");
      continue;
    }
    expr *program = parse_expr();
    expr *ret = eval_top(program, environ);
    printf("=> ");
    print_expr(ret);
    puts("");
    printf("scm> ");
  }
}
int main(int argc, char *argv[]) {
  if (argc < 2) {
    if (isatty(fileno(stdin)))
      repl();
    else
      throw("repl must be run in tty");
  } else {
    expr *program = parse_program(argv[1]);
    frame *environ = mk_initial_env();
    eval_list(program, environ, mk_empty_cell_expr());
  }

  for (int i = 0; i < MEMP; i++)
    free(MEM[i]);
}
