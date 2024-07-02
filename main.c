#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOL_LEN_MAX 256

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
    if (expr->type == VALUE) {
      printf("%f", expr->body.value);
    } else if (expr->type == SYMBOL) {
      printf("%s", expr->body.symbol);
    } else if (expr->type == CELL) {
      print_list(expr->body.cell);
    }
    if (list->cdr != NULL)
      printf(" ");
    list = list->cdr;
  }
  printf(")");
}

// program = list*
// list = '(' expr* ')'
// expr = value | symbol | list

char *input;

void skip_ws() {
  while (*input == ' ' || *input == '\n' || *input == '\t')
    input++;
}

int is_symbol_char() {
  return ('a' <= *input && *input <= 'z') || ('A' <= *input && *input <= 'Z') ||
         *input == '_' || *input == '!';
}

cell *parse_list();
expression *parse_expression() {
  printf("input: %s\n", input);
  if ('0' <= *input && *input <= '9') {
    return make_value_expression(strtof(input, &input));
  } else if (is_symbol_char()) {
    char buf[SYMBOL_LEN_MAX];
    int i =0;
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

int main(int argc, char *argv[]) {
  cell *list = parse_program("(1 (2 3) 4 (5 6))");
  print_list(list);
}
