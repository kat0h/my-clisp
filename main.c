#include <alloca.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
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


void print_list(cell *c);
void print_value(value *e) {
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
    print_value(c->car);
    if (TYPEOF(c->cdr) != CELL) {
      printf(" . ");
      print_value(c->cdr);
      break;
    }
    if (E_CELL(c->cdr) != NULL) {
      printf(" ");
    }
    c = E_CELL(c->cdr);
  }
  printf(")");
}
value *mk_number_value(float number) {
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = NUMBER;
  E_NUMBER(e) = number;
  return e;
}
value *mk_symbol_value(char *symbol) {
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = SYMBOL;
  char *s = xmalloc(strlen(symbol) + 1);
  strcpy(s, symbol);
  E_SYMBOL(e) = s;
  return e;
}
value *mk_empty_cell_value() {
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = CELL;
  E_CELL(e) = NULL;
  return e;
}
value *mk_cell_value(value *car, value *cdr) {
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = CELL;
  E_CELL(e) = xmalloc(sizeof(cell));
  CAR(e) = car;
  CDR(e) = cdr;
  return e;
}
value *mk_lambda_value(cell *args, value *body, frame *env) {
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = LAMBDA;
  E_LAMBDA(e) = xmalloc(sizeof(lambda));
  E_LAMBDA(e)->args = args;
  E_LAMBDA(e)->body = body;
  E_LAMBDA(e)->env = env;
  return e;
}
value *mk_boolean_value(int b) {
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = BOOLEAN;
  E_BOOLEAN(e) = b;
  return e;
}
value *mk_ifunc_value(ifunc f) {
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = IFUNC;
  E_IFUNC(e) = f;
  return e;
}
value *mk_string_value(char *str) {
  value *e = xmalloc(sizeof(value));
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
int truish(value *e) {
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
void add_kv_to_frame(frame *env, char *symbol, value *value) {
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
value *define_to_env(frame *env, char *symbol, value *value) {
  add_kv_to_frame(env, symbol, value);
  return mk_symbol_value(symbol);
}
value *set_to_env(frame *env, char *symbol, value *value) {
  kv *i = find_pair_recursive(env, symbol);
  if (i == NULL)
    throw("symbol %s not found", symbol);
  i->value = value;
  return mk_symbol_value(symbol);
}
value *lookup_frame(frame *env, char *symbol) {
  kv *v = find_pair_recursive(env, symbol);
  return v->value;
}
void print_frame(frame *env) {
  while (env != NULL) {
    puts("....Frame...........................................");
    kv *i = env->kv;
    while (i != NULL) {
      printf("  %s: ", i->key);
      print_value(i->value);
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
value *parse_hash_literal() {
  if (*input != '#')
    throw("parse error: not hash literal");
  input++;
  if (*input == 't') {
    input++;
    return mk_boolean_value(1);
  } else if (*input == 'f') {
    input++;
    return mk_boolean_value(0);
  } else {
    throw("parse error: unexpected token %c", *input);
  }
}
value *parse_list();
value *parse_value() {
#ifdef DEBUG
  printf("parse_value: %s\n", input);
#endif
  // numeber
  if ('0' <= *input && *input <= '9') {
    return mk_number_value(strtof(input, &input));
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
    return mk_symbol_value(buf);
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
    return mk_string_value(buf);
  } else if (*input == '#') {
    return parse_hash_literal();
  } else if (*input == '(') {
    input++;
    return parse_list();
  }
  throw("Unexpected valueession '%c' \"%s\"", *input, input);
}
value *parse_list() {
#ifdef DEBUG
  printf("parse_list: %s\n", input);
#endif
  skip_ws();
  if (*input == ')') {
    input++;
    return mk_empty_cell_value();
  }
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = CELL;
  E_CELL(e) = xmalloc(sizeof(cell));
  CAR(e) = parse_value();
  skip_ws();
  CDR(e) = parse_list();
  skip_ws();
  return e;
}
value *parse_paren() {
#ifdef DEBUG
  printf("parse_paren: %s\n", input);
#endif
  if (*input == '(') {
    input++;
    skip_ws();
    value *e = parse_list();
    skip_ws();
    return e;
  }
  throw("Unexpected token %c", *input);
}
value *parse_program_list() {
#ifdef DEBUG
  printf("parse_program_list: %s\n", input);
#endif
  if (*input == '\0')
    return mk_empty_cell_value();
  value *e = xmalloc(sizeof(value));
  TYPEOF(e) = CELL;
  E_CELL(e) = xmalloc(sizeof(cell));
  CAR(e) = parse_paren();
  skip_ws();
  CDR(e) = parse_program_list();
  skip_ws();
  return e;
}
value *parse_program(char *prg) {
#ifdef DEBUG
  printf("parse_program: %s\n", prg);
#endif
  input = prg;
  value *e = parse_program_list();
  if (*input != '\0') {
    throw("parser error input is not empty \"%s\"", input);
  }
  return e;
}

// eval
value *eval_cell(value *exp, frame *env);
value *eval(value *exp, frame *env) {
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

value *eval_top(value *exp, frame *env) {
  INIT_CONTINUATION();
  return eval(exp, env);
}

value *eval_lambda(lambda *f, cell *args, frame *env);
value *eval_cell(value *exp, frame *env) {
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
  value *func = eval(CAR(exp), env);
  value *args = CDR(exp);
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
      call_continuation(cont, mk_empty_cell_value());
    call_continuation(cont, eval(CAR(args), env));
  }
  throw("call error: not callable");
}
value *eval_lambda(lambda *f, cell *args, frame *env) {
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
value *ifunc_add(value *args, frame *env) {
  float sum = 0;
  while (E_CELL(args) != NULL) {
    value *i = eval(CAR(args), env);
    if (TYPEOF(i) != NUMBER)
      throw("add error: not number");
    sum += E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_value(sum);
}
value *ifunc_sub(value *args, frame *env) {
  value *first = eval(CAR(args), env);
  if (TYPEOF(first) != NUMBER)
    throw("sub error: not number");
  float sum = E_NUMBER(first);
  if (cell_len(E_CELL(args)) == 1)
    return mk_number_value(-sum);
  args = CDR(args);
  while (E_CELL(args) != NULL) {
    value *i = eval(CAR(args), env);
    if (TYPEOF(i) != NUMBER)
      throw("sub error: not number");
    sum -= E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_value(sum);
}
value *ifunc_mul(value *args, frame *env) {
  float sum = 1.0;
  while (E_CELL(args) != NULL) {
    value *i = eval(CAR(args), env);
    if (TYPEOF(i) != NUMBER) {
      throw("mul error: not number");
    }
    sum *= E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_value(sum);
}
value *ifunc_div(value *args, frame *env) {
  float sum = 0;
  while (E_CELL(args) != NULL) {
    value *i = eval(CAR(args), env);
    if (TYPEOF(i) != NUMBER)
      throw("mul error: not number");
    if (E_NUMBER(i) == 0)
      throw("div error: zero division");
    sum /= E_NUMBER(i);
    args = CDR(args);
  }
  return mk_number_value(sum);
}
value *ifunc_modulo(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 2)
    throw("modulo error: invalid number of arguments");
  value *a = eval(CAR(args), env);
  value *b = eval(CAR(CDR(args)), env);
  if (TYPEOF(a) != NUMBER || TYPEOF(b) != NUMBER)
    throw("modulo error: not number");
  int ia = (int)E_NUMBER(a);
  int ib = (int)E_NUMBER(b);
  return mk_number_value(ia % ib);
}
value *ifunc_begin(value *args, frame *env) {
  value *i = mk_number_value(0);
  while (E_CELL(args) != NULL) {
    i = eval(CAR(args), env);
    args = CDR(args);
  }
  return i;
}
value *ifunc_define(value *args, frame *env) {
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
  value *value = eval(CAR(args), env);
  if (E_CELL(CDR(args)) != NULL) {
    throw("define error: too many arguments");
  }
  return define_to_env(env, symbol, value);
}
value *ifunc_setbang(value *args, frame *env) {
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
  value *value = eval(CAR(args), env);
  if (E_CELL(CDR(args)) != NULL) {
    throw("define error: too many arguments");
  }
  return set_to_env(env, symbol, value);
}
value *ifunc_showenv(value *args, frame *env) {
  if (E_CELL(args) != NULL) {
    throw("showenv error: too many arguments");
  }
  print_frame(env);
  return mk_number_value(0);
}
int check_args(value *args) {
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
value *ifunc_lambda(value *args, frame *env) {
  // (lambda (args) body)
  if (E_CELL(args) == NULL)
    throw("lambda error: no args");
  value *first = CAR(args);
  if (!check_args(first))
    throw("lambda error: args is not list of symbol");
  cell *largs = E_CELL(first);
  if (E_CELL(CDR(args)) == NULL)
    throw("lambda error: no body");
  value *body = CAR(CDR(args));
  if (E_CELL(CDR(CDR(args))) != NULL)
    throw("lambda error: too many body");
  return mk_lambda_value(largs, body, env);
}
value *ifunc_print(value *args, frame *env) {
  while (E_CELL(args) != NULL) {
    print_value(eval(CAR(args), env));
    puts("");
    args = CDR(args);
  }
  return mk_number_value(0);
}
value *ifunc_if(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 2 && cell_len(E_CELL(args)) != 3)
    throw("if error: invalid number of arguments");
  value *cond = eval(CAR(args), env);
  if (truish(cond)) {
    return eval(CAR(CDR(args)), env);
  } else {
    if (cell_len(E_CELL(args)) == 2)
      return mk_empty_cell_value();
    return eval(CAR(CDR(CDR(args))), env);
  }
}
value *ifunc_quote(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("quote error: invalid number of arguments");
  return CAR(args);
}
enum { EQ = 0, LT, LE, GT, GE };
int comp(value *args, frame *env, char type) {
  int len = cell_len(E_CELL(args));
  if (len < 2)
    throw("comp error: too few arguments");
  value *car = eval(CAR(args), env);
  if (TYPEOF(car) != NUMBER)
    throw("comp error: not number");
  value *cdr;
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
value *ifunc_eq(value *args, frame *env) {
  return mk_boolean_value(comp(args, env, EQ));
}
value *ifunc_lt(value *args, frame *env) {
  return mk_boolean_value(comp(args, env, LT));
}
value *ifunc_le(value *args, frame *env) {
  return mk_boolean_value(comp(args, env, LE));
}
value *ifunc_gt(value *args, frame *env) {
  return mk_boolean_value(comp(args, env, GT));
}
value *ifunc_ge(value *args, frame *env) {
  return mk_boolean_value(comp(args, env, GE));
}
value *ifunc_and(value *args, frame *env) {
  while (E_CELL(args) != NULL) {
    int i = truish(eval(CAR(args), env));
    if (i == 0)
      return mk_boolean_value(0);
    args = CDR(args);
  }
  return mk_boolean_value(1);
}
value *ifunc_or(value *args, frame *env) {
  while (E_CELL(args) != NULL) {
    int i = truish(eval(CAR(args), env));
    if (i)
      return mk_boolean_value(1);
    args = CDR(args);
  }
  return mk_boolean_value(0);
}
value *eval_list(value *args, frame *env, value *default_value) {
  value *i = default_value;
  while (E_CELL(args) != NULL) {
    i = eval_top(CAR(args), env);
    args = CDR(args);
  }
  return i;
}
value *ifunc_cond(value *args, frame *env) {
  while (E_CELL(args) != NULL) {
    // (cond list* (else value*)?)
    // list = (value*)
    value *list = CAR(args);
    if (TYPEOF(list) != CELL)
      throw("cond error: not list");
    value *cond;
    while (E_CELL(list) != NULL) {
      if (TYPEOF(CAR(list)) == SYMBOL &&
          strcmp(E_SYMBOL(CAR(list)), "else") == 0) {
        // elseのあとをチェック
        if (E_CELL(CDR(args)) != NULL)
          throw("cond error: else is not last");
        return eval_list(CDR(list), env, mk_number_value(0));
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
  return mk_empty_cell_value();
}
value *ifunc_cons(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 2)
    fprintf(stderr, "cons error: invalid number of arguments");
  value *car = eval(CAR(args), env);
  value *cdr = eval(CAR(CDR(args)), env);
  return mk_cell_value(car, cdr);
}
value *ifunc_car(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("car error: invalid number of arguments");
  value *c = eval(CAR(args), env);
  if (TYPEOF(c) != CELL)
    throw("car error: not pair");
  return CAR(c);
}
value *ifunc_cdr(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("car error: invalid number of arguments");
  value *c = eval(CAR(args), env);
  if (TYPEOF(c) != CELL)
    throw("car error: not pair");
  return CDR(c);
}

value *ifunc_rand(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 0) 
    throw("random error: arg");
  return mk_number_value(rand());
}

value *ifunc_length(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("length error: invalid number of arguments");
  value *c = eval(CAR(args), env);
  if (TYPEOF(c) != CELL)
    throw("length error: not pair");
  return mk_number_value(cell_len(E_CELL(c)));
}

// プログラムの動作をn秒停止
value *ifunc_sleep(value *args, frame *env) {
  if (cell_len(E_CELL(args)) != 1)
    throw("sleep error: invalid number of arguments");
  value *c = eval(CAR(args), env);
  if (TYPEOF(c) != NUMBER)
    throw("sleep error: not number");
  sleep(E_NUMBER(c));
  return mk_number_value(0);
}

// main
frame *mk_initial_env() {
  frame *env = make_frame(NULL);
  add_kv_to_frame(env, "+", mk_ifunc_value(ifunc_add));
  add_kv_to_frame(env, "-", mk_ifunc_value(ifunc_sub));
  add_kv_to_frame(env, "*", mk_ifunc_value(ifunc_mul));
  add_kv_to_frame(env, "/", mk_ifunc_value(ifunc_div));
  add_kv_to_frame(env, "modulo", mk_ifunc_value(ifunc_modulo));
  add_kv_to_frame(env, "begin", mk_ifunc_value(ifunc_begin));
  add_kv_to_frame(env, "define", mk_ifunc_value(ifunc_define));
  add_kv_to_frame(env, "set!", mk_ifunc_value(ifunc_setbang));
  add_kv_to_frame(env, "showenv", mk_ifunc_value(ifunc_showenv));
  add_kv_to_frame(env, "lambda", mk_ifunc_value(ifunc_lambda));
  add_kv_to_frame(env, "print", mk_ifunc_value(ifunc_print));
  add_kv_to_frame(env, "if", mk_ifunc_value(ifunc_if));
  add_kv_to_frame(env, "quote", mk_ifunc_value(ifunc_quote));
  add_kv_to_frame(env, "=", mk_ifunc_value(ifunc_eq));
  add_kv_to_frame(env, "<", mk_ifunc_value(ifunc_lt));
  add_kv_to_frame(env, "<=", mk_ifunc_value(ifunc_le));
  add_kv_to_frame(env, ">", mk_ifunc_value(ifunc_gt));
  add_kv_to_frame(env, ">=", mk_ifunc_value(ifunc_ge));
  add_kv_to_frame(env, "and", mk_ifunc_value(ifunc_and));
  add_kv_to_frame(env, "or", mk_ifunc_value(ifunc_or));
  add_kv_to_frame(env, "cond", mk_ifunc_value(ifunc_cond));
  add_kv_to_frame(env, "cons", mk_ifunc_value(ifunc_cons));
  add_kv_to_frame(env, "call/cc", mk_ifunc_value(ifunc_callcc));
  add_kv_to_frame(env, "car", mk_ifunc_value(ifunc_car));
  add_kv_to_frame(env, "cdr", mk_ifunc_value(ifunc_cdr));
  add_kv_to_frame(env, "rand", mk_ifunc_value(ifunc_rand));
  add_kv_to_frame(env, "length", mk_ifunc_value(ifunc_length));
  add_kv_to_frame(env, "sleep", mk_ifunc_value(ifunc_sleep));
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
    value *program = parse_value();
    value *ret = eval_top(program, environ);
    printf("=> ");
    print_value(ret);
    puts("");
    printf("scm> ");
  }
}
int main(int argc, char *argv[]) {
  srand(time(NULL));
  if (argc < 2) {
    if (isatty(fileno(stdin)))
      repl();
    else
      throw("repl must be run in tty");
  } else {
    value *program = parse_program(argv[1]);
    frame *environ = mk_initial_env();
    eval_list(program, environ, mk_empty_cell_value());
  }

  for (int i = 0; i < MEMP; i++)
    free(MEM[i]);
}
