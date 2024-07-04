#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOL_LEN_MAX 256
char debug = 0;

void *malloc_e(size_t size) {
  void *p = malloc(size);
  if (p == NULL) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }
  return p;
}

typedef struct Expression expression;
typedef struct Cell cell;
typedef cell *list;

struct Expression {
  union {
    float value;
    char *symbol;
    cell *cell;
  } body;
  enum { VALUE, SYMBOL, CELL } type;
};

expression *make_value_expression(float value) {
  expression *expr = (expression *)malloc_e(sizeof(expression));
  expr->body.value = value;
  expr->type = VALUE;
  return expr;
}

expression *make_symbol_expression(char *symbol) {
  expression *expr = (expression *)malloc_e(sizeof(expression));
  expr->body.symbol = strndup(symbol, SYMBOL_LEN_MAX);
  if (expr->body.symbol == NULL) {
    fprintf(stderr, "strndup failed\n");
    exit(1);
  }
  expr->type = SYMBOL;
  return expr;
}

expression *make_list_expression(cell *list) {
  expression *expr = (expression *)malloc_e(sizeof(expression));
  expr->body.cell = list;
  expr->type = CELL;
  return expr;
}

void print_list(cell *list);
void print_expression(expression *expr) {
  if (expr->type == VALUE) {
    printf("%f", expr->body.value);
  } else if (expr->type == SYMBOL) {
    printf("%s", expr->body.symbol);
  } else if (expr->type == CELL) {
    print_list(expr->body.cell);
  }
}

int exp_equal(expression *a, expression *b) {
  if (a->type != b->type)
    return 0;
  if (a->type == VALUE)
    return a->body.value == b->body.value;
  if (a->type == SYMBOL)
    return strcmp(a->body.symbol, b->body.symbol) == 0;
  if (a->type == CELL) {
    // TODO: リスト同士の比較は未実装
  }
  return 0;
}

struct Cell {
  expression *car;
  cell *cdr;
};

cell *make_empty_list() {
  cell *list = (cell *)malloc_e(sizeof(cell));
  list->car = NULL;
  list->cdr = NULL;
  return list;
}

// リストの先頭がダミーかどうかをチェック
cell *get_first(cell *list) { return list->cdr; }

cell *get_last(cell *list) {
  while (list->cdr != NULL)
    list = list->cdr;
  return list;
}

cell *get_nth(cell *list, int n) {
  list = get_first(list);
  for (int i = 0; i < n; i++) {
    if (list == NULL)
      return NULL;
    list = list->cdr;
  }
  return list;
}

void append_list(cell *list, expression *expr) {
  // 最後のセルを取得
  list = get_last(list);
  list->cdr = (cell *)malloc_e(sizeof(cell));
  list = list->cdr;

  // 要素をセット
  list->car = expr;
  list->cdr = NULL;
}

void print_list(cell *list) {
  // skip dummy cell
  list = get_first(list);
  printf("(");
  while (list != NULL) {
    expression *expr = list->car;
    print_expression(expr);
    if (list->cdr != NULL)
      printf(" ");
    list = list->cdr;
  }
  printf(")");
}

typedef struct Frame frame;
struct Frame {
  // 親環境へのポインタ(グローバル環境のときはNULL)
  frame *parent;
  // ((symbol value) ...)
  cell *kv;
};

frame *make_frame(frame *parent) {
  frame *f = (frame *)malloc_e(sizeof(frame));
  f->parent = parent;
  f->kv = make_empty_list();
  return f;
}

cell *find_symbol(frame *env, expression *symbol) {
  cell *kv = get_first(env->kv);
  while (kv != NULL) {
    cell *pair = kv->car->body.cell;
    if (exp_equal(get_first(pair)->car, symbol)) {
      return pair;
    }
    kv = kv->cdr;
  }
  return NULL;
}

cell *find_symbol_recursive(frame *env, expression *symbol) {
loop:
  if (env == NULL)
    return NULL;
  cell *pair = find_symbol(env, symbol);
  if (pair != NULL)
    return pair;
  env = env->parent;
  goto loop;
}

// 環境から変数を取得
expression *lookup_frame(frame *env, expression *symbol) {
  cell *pair = find_symbol_recursive(env, symbol);
  if (pair == NULL) {
    fprintf(stderr, "symbol %s is not defined\n", symbol->body.symbol);
    exit(1);
  }
  return get_nth(pair, 1)->car;
}

// 現在の環境に変数をセット
// 値渡し，参照渡し 注意 TODO
expression *define_frame(frame *env, expression *symbol, expression *value) {
  if (symbol->type != SYMBOL) {
    fprintf(stderr, "define: symbol must be symbol\n");
    exit(1);
  }
  // 既に定義されいているか確認
  cell *v = find_symbol(env, symbol);
  if (v == NULL) {
    // あたらしいペア
    cell *pair = make_empty_list();
    append_list(pair, symbol);
    append_list(pair, value);
    append_list(env->kv, make_list_expression(pair));
  } else {
    v = get_first(v);
    v->cdr->car = value;
  }
  return symbol;
}

expression *set_frame(frame *env, expression *symbol, expression *value) {
  if (symbol->type != SYMBOL) {
    fprintf(stderr, "set!: symbol must be symbol\n");
    exit(1);
  }
  cell *v = find_symbol_recursive(env, symbol);
  if (v == NULL) {
    fprintf(stderr, "symbol %s is not defined\n", symbol->body.symbol);
    exit(1);
  }
  get_nth(v, 1)->car = value;

  return symbol;
}

void _print_frame(frame *env) {
  printf("  ");
  print_list(env->kv);
  puts("");
  if (env->parent != NULL) {
    _print_frame(env->parent);
  }
}

void print_frame(frame *env) {
  puts("Env");
  _print_frame(env);
}

// program = expr*  (現状はlist)
// list = '(' expr* ')'
// expr = value | symbol | list

char *input;

void skip_ws() {
  while (*input == ' ' || *input == '\n' || *input == '\t')
    input++;
}

int is_symbol_char() {
  return ('a' <= *input && *input <= 'z') || ('A' <= *input && *input <= 'Z') ||
         *input == '_' || *input == '!' || *input == '+' || *input == '-' ||
         *input == '*' || *input == '/';
}

cell *parse_list();
expression *parse_expression() {
  if (debug)
    printf("input: %s\n", input);
  if ('0' <= *input && *input <= '9') {
    return make_value_expression(strtof(input, &input));
  } else if (is_symbol_char()) {
    char buf[SYMBOL_LEN_MAX];
    int i = 0;
    while (is_symbol_char()) {
      buf[i++] = *input++;
      if (i == SYMBOL_LEN_MAX) {
        fprintf(stderr, "symbol is too long\n");
        exit(1);
      }
    }
    buf[i] = '\0';
    return make_symbol_expression(buf);
  } else if (*input == '(') {
    return make_list_expression(parse_list());
  }
  fprintf(stderr, "Unexpected expression %c \"%s\"\n", *input, input);
  exit(1);
}

cell *parse_list() {
  if (debug)
    printf("input: %s\n", input);
  cell *list = make_empty_list();
  if (*input == '(') {
    input++;
    skip_ws();
    while (*input != ')') {
      append_list(list, parse_expression());
      skip_ws();
    }
    input++;
    return list;
  }
  fprintf(stderr, "Unexpected token %c\n", *input);
  exit(1);
}

cell *parse_program(char *prg) {
  input = prg;
  return parse_list();
}

expression *eval(expression *exp, frame *env) {
  if (exp->type == VALUE)
    // 値はそのまま返す
    return exp;

  else if (exp->type == SYMBOL) {
    return lookup_frame(env, exp);

  } else if (exp->type == CELL) {
    // 最初の要素を取得
    cell *l = exp->body.cell->cdr;

    // 関数名から関数を取得
    expression *func = l->car;
    // まだ関数は第一級ではない
    if (func->type != SYMBOL) {
      fprintf(stderr, "function name must be symbol\n");
      exit(1);
    }

    if (strcmp("+", func->body.symbol) == 0) {
      float sum = 0;
      l = l->cdr;
      while (l != NULL) {
        expression *arg = eval(l->car, env);
        if (arg->type != VALUE) {
          fprintf(stderr, "operation + is not defined for arguments");
          exit(1);
        }
        sum += arg->body.value;
        l = l->cdr;
      }
      return make_value_expression(sum);
    } else if (strcmp("trace", func->body.symbol) == 0) {
      print_frame(env);
      return make_value_expression(0);
    }
    fprintf(stderr, "Undefined function %s\n", func->body.symbol);
    exit(1);
  }

  fprintf(stderr, "Unreachable\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <program>\n", argv[0]);
    return 1;
  }

  cell *list = parse_program(argv[1]);
  if (debug) {
    puts("Source code");
    print_list(list);
    puts("");
  }

  frame *environment = make_frame(NULL);
  define_frame(environment, make_symbol_expression("a"),
               make_value_expression(0));
  define_frame(environment, make_symbol_expression("b"),
               make_value_expression(1));
  define_frame(environment, make_symbol_expression("a"),
               make_value_expression(2));
  frame *child = make_frame(environment);
  define_frame(child, make_symbol_expression("c"), make_value_expression(3));

  set_frame(child, make_symbol_expression("a"), make_value_expression(4));

  print_frame(child);

  expression *result = eval(make_list_expression(list), child);
  printf("=> ");
  print_expression(result);
  puts("");
}
