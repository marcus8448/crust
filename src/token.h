#ifndef TOKEN_H
#define TOKEN_H
#include "struct/list.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  token_eof = '\0',

  token_opening_paren = '(',
  token_closing_paren = ')',
  token_opening_sqbr = '[',
  token_closing_sqbr = ']',
  token_comma = ',',
  token_opening_curly_brace = '{',
  token_closing_curly_brace = '}',
  token_semicolon = ';',
  token_colon = ':',

  token_plus = '+',
  token_minus = '-',
  token_asterik = '*',
  token_slash = '/',

  token_vertical_bar = '|',
  token_caret = '^',
  token_amperstand = '&',
  token_tilde = '~',
  token_exclaimation = '!',

  token_period = '.',

  token_equals_assign = '=',

  token_string = '"',
  // represents full string (to next quote)

  _ = 128,

  token_double_vertical_bar,
  token_double_amperstand,

  token_arrow,
  // ->

  token_left_shift,
  token_right_shift,

  token_keyword_fn,
  token_keyword_let,
  token_keyword_extern,
  token_keyword_as,

  token_identifier,
  token_constant,

  token_not_equals,
  token_double_equals,
  token_less_than,
  token_greater_than,
  token_less_than_equal,
  token_greater_than_equal,

  token_cf_if,
  token_cf_else,
  token_cf_while,
  token_cf_return,
  token_cf_break
} TokenType;

const char *token_name(TokenType type);

typedef struct Token {
  TokenType type;
  struct Token *next;
  struct Token *prev;

  int index;
  uint8_t len;
} Token;

bool token_value_compare(const Token *token, const char *contents, const char *compare);
char *token_copy(const Token *token, const char *contents);
void token_copy_to(const Token *token, const char *contents, char *output);

int token_str_cmp(const Token *token, const char *contents, const char *cmp);
void token_free(Token *token);

bool tokenize(const char *data, size_t len, Token *head);

LIST_API(Token, token, Token)
#endif // TOKEN_H
