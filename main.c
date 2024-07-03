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

void append_list(cell *list, expression *expr) {
  // 最後のセル
  while (list->cdr != NULL)
    list = list->cdr;
  list->cdr = (cell *)malloc_e(sizeof(cell));
  list = list->cdr;

  // 要素をセット
  list->car = expr;
  list->cdr = NULL;
}

void print_list(cell *list) {
  // skip dummy cell
  list = list->cdr;
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
  cell *list;
};

frame *make_frame(frame *parent) {
  frame *f = (frame *)malloc_e(sizeof(frame));
  f->parent = parent;
  f->list = make_empty_list();
  return f;
}

expression *find_symbol(frame *env, expression* symbol) {
  while (env == NULL) {
    cell *list = env->list->cdr;
    while (list != NULL) {
      cell *pair = list->car->body.cell;
      if (exp_equal(pair->car, symbol))
        return pair->cdr->car;
      list = list->cdr;
    }
    env = env->parent;
  }
  fprintf(stderr, "symbol %s is not found\n", symbol->body.symbol);
  exit(1);
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
    return find_symbol(env, exp);

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
  // frameにaを1として追加
  cell *pair = make_empty_list();
  append_list(pair, make_symbol_expression("a"));
  append_list(pair, make_value_expression(1));
  append_list(environment->list, make_list_expression(pair));

  print_expression(eval(make_list_expression(list), environment));
  puts("");
}
