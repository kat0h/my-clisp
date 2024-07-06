#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOL_LEN_MAX 256
char debug = 0;

size_t MEMP = 0;
void *MEM[1000000] = {0};

void *malloc_e(size_t size) {
  void *p = malloc(size);
  MEM[MEMP++] = p;
  if (MEMP == 1000000) {
    fprintf(stderr, "Internal Error malloc_e\n");
    exit(1);
  }
  if (p == NULL) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }
  return p;
}

typedef struct Expression expression;
typedef struct Cell cell;
typedef struct Lambda lambda;
typedef struct Frame frame;

struct Expression {
  union {
    float value;
    char *symbol;
    cell *cell;
    lambda *lmd;
  } body;
  enum { VALUE, SYMBOL, CELL, LAMBDA } type;
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

struct Lambda {
  cell *args;
  cell *body;
  frame *env;
};

expression *make_lambda_expression(cell *args, cell *body, frame *env) {
  expression *expr = (expression *)malloc_e(sizeof(expression));
  lambda *lmd = (lambda *)malloc_e(sizeof(lambda));
  lmd->args = args;
  lmd->body = body;
  lmd->env = env;
  expr->body.lmd = lmd;
  expr->type = LAMBDA;
  return expr;
}

void print_list(cell *list);
void print_frame(frame *env);
void print_lambda(expression *expr) {
  lambda *l = expr->body.lmd;
  printf("Closure\n");
  printf("Arguments:\n  ");
  print_list(l->args);
  puts("");
  printf("Body:\n  ");
  print_list(l->body);
  puts("");
  print_frame(l->env);
}

void print_lambda(expression *expr);
void print_expression(expression *expr) {
  if (expr->type == VALUE) {
    printf("%f", expr->body.value);
  } else if (expr->type == SYMBOL) {
    printf("%s", expr->body.symbol);
  } else if (expr->type == CELL) {
    print_list(expr->body.cell);
  } else if (expr->type == LAMBDA) {
    print_lambda(expr);
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

void check_is_list(cell *list) {
  if (list->car != NULL) {
    print_list(list);
    fprintf(stderr, "list is not a list\n");
    exit(1);
  }
}

// リストの先頭がダミーかどうかをチェック
cell *get_first(cell *list) {
  check_is_list(list);
  return list->cdr;
}

cell *get_last(cell *list) {
  check_is_list(list);
  while (list->cdr != NULL)
    list = list->cdr;
  return list;
}

cell *get_nth(cell *list, int n) {
  check_is_list(list);
  list = get_first(list);
  for (int i = 0; i < n; i++) {
    if (list == NULL)
      return NULL;
    list = list->cdr;
  }
  return list;
}

size_t list_len(cell *list) {
  size_t len = 0;
  while (list->cdr != NULL) {
    len++;
    list = list->cdr;
  }
  return len;
}

void append_list(cell *list, expression *expr) {
  check_is_list(list);
  // 最後のセルを取得
  list = get_last(list);
  list->cdr = (cell *)malloc_e(sizeof(cell));
  list = list->cdr;

  // 要素をセット
  list->car = expr;
  list->cdr = NULL;
}

void print_list(cell *list) {
  check_is_list(list);
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
  while (env != NULL) {
    cell *pair = find_symbol(env, symbol);
    if (pair != NULL)
      return pair;
    env = env->parent;
  }
  return NULL;
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
  while (env != NULL) {
    printf("  ");
    print_list(env->kv);
    puts("");
    env = env->parent;
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
  cell *list = parse_list();
  if (*input != '\0') {
    fprintf(stderr, "parser error input is not empty \"%s\"\n", input);
    exit(1);
  }

  return list;
}

expression *eval(expression *exp, frame *env) {
  if (exp->type == VALUE)
    // 値はそのまま返す
    return exp;

  // シンボルから値を引く
  else if (exp->type == SYMBOL) {
    return lookup_frame(env, exp);

  } else if (exp->type == CELL) {
    // 最初の要素を取得
    cell *list = exp->body.cell;

    // 空のリストを評価するとリスト
    // R5RSによると，Schemeで()は妥当な式ではないらしい
    if (list_len(list) == 0)
      return exp;

    // 関数名から関数を取得
    expression *func = get_nth(list, 0)->car;
    if (func->type == SYMBOL) {
      if (strcmp("begin", func->body.symbol) == 0) {
        list = list->cdr->cdr;
        expression *result = make_value_expression(0);
        while (list != NULL) {
          result = eval(list->car, env);
          list = list->cdr;
        }
        return result;

      } else if (strcmp("lambda", func->body.symbol) == 0) {
        if (list_len(list) < 3) {
          fprintf(stderr,
                  "wrong number of arguments lambda expected 2 got %d\n",
                  (int)list_len(list) - 1);
          exit(1);
        }

        // lambda式には下記のような定義がある
        // (lambda (x) x)
        //    => ((lambda (x) x) 1) => 1
        //
        // 未 | (lambda x x)
        // 実 |    => ((lambda x x) 1 2) => (1 2)
        // 装 | (lambda (x . y) (print x y))
        //    |    => ((lambda (x . y) (print x y)) 1 2 3) => 1 (2 3)
        if (get_nth(list, 1)->car->type != CELL) {
          fprintf(stderr, "lambda: arguments must be list\n");
          exit(1);
        }
        cell *args = get_nth(list, 1)->car->body.cell;
        check_is_list(args);

        // 本体
        cell *body = make_empty_list();
        body->cdr = get_nth(list, 2);

        expression *l = make_lambda_expression(args, body, env);
        return l;

      } else if (strcmp("+", func->body.symbol) == 0) {
        float sum = 0;
        list = list->cdr->cdr;
        while (list != NULL) {
          expression *arg = eval(list->car, env);
          if (arg->type != VALUE) {
            fprintf(stderr, "operation + is not defined for arguments");
            exit(1);
          }
          sum += arg->body.value;
          list = list->cdr;
        }
        return make_value_expression(sum);

      } else if (strcmp("trace", func->body.symbol) == 0) {
        print_frame(env);
        return make_value_expression(0);

      } else if (strcmp("define", func->body.symbol) == 0) {
        int len = list_len(list);
        if (len != 3) {
          fprintf(stderr,
                  "wrong number of arguments define expected 2 got %d\n",
                  len - 1);
          exit(1);
        }
        expression *symbol = get_nth(list, 1)->car;
        expression *value = eval(get_nth(list, 2)->car, env);
        return define_frame(env, symbol, value);

      } else if (strcmp("set!", func->body.symbol) == 0) {
        int len = list_len(list);
        if (len != 3) {
          fprintf(stderr, "wrong number of arguments set! expected 2 got %d\n",
                  len - 1);
          exit(1);
        }
        expression *symbol = get_nth(list, 1)->car;
        expression *value = eval(get_nth(list, 2)->car, env);
        return set_frame(env, symbol, value);
      }
      fprintf(stderr, "Undefined function %s\n", func->body.symbol);
      exit(1);
    }
  }

  fprintf(stderr, "can't eval the expression\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <program>\n", argv[0]);
    return 1;
  }

  frame *environment = make_frame(NULL);

  cell *list = parse_program(argv[1]);
  if (debug) {
    puts("Source code");
    print_list(list);
    puts("");
  }

  expression *result = eval(make_list_expression(list), environment);
  printf("=> ");
  print_expression(result);
  puts("");

  // MEMとMEMPの内容を表示
  if (debug)
    for (int i = 0; i < MEMP; i++) {
      printf("MEM[%d] = %p\n", i, MEM[i]);
    }

  for (int i = 0; i < MEMP; i++) {
    free(MEM[i]);
  }
}
