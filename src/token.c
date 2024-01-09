#include "token.h"
#include <assert.h>
#include <malloc.h>

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void token_push(Token **token, const TokenType type, const size_t index, const uint8_t len) {
  assert(index < INT_MAX);
  (*token)->type = type;
  (*token)->index = (int)index;
  (*token)->len = len;

  if ((*token)->next == NULL) {
    (*token)->next = malloc(sizeof(Token));
    (*token)->next->prev = *token;
    (*token)->next->next = NULL;
  }
  *token = (*token)->next;
}

int sz_strncmp(const char *buffer, const char *cmp, const uint8_t len) {
  if (cmp == NULL)
    return -1;
  if (strlen(cmp) > len)
    return 1;
  return strncmp(buffer, cmp, len);
}

bool tokenize(const char *data, const size_t len, Token *head) {
  typedef enum {
    any,
    comment_nl,
    comment_cl,
    q_string
  } Mode;

  char buffer[256];
  uint8_t bufLen = 0;
  bool numeric = false;
  bool escaping = false;
  Mode mode = any;
  char last = 0;
  Token *next = head;
  head->prev = NULL;
  head->next = NULL;

  for (size_t i = 0; i < len; i++) {
    Token *previous = next->prev;
    const int c = data[i];
    if (c == EOF) {
      continue;
    }

    switch (mode) {
    case any: {
      if (isspace(c) || c == '=' || c == ',' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
          c == '<' || c == '>' || c == '~' || c == '&' || c == '|' || c == '!' || c == '+' || c == '-' || c == '/' ||
          c == '*' || c == ';' || c == '"' || c == ':' || c == '.' || c == '^') {
        if (bufLen > 0) {
          if (sz_strncmp(buffer, "fn", bufLen) == 0) {
            token_push(&next, token_keyword_fn, i - bufLen, bufLen);
          } else if (sz_strncmp(buffer, "let", bufLen) == 0) {
            token_push(&next, token_keyword_let, i - bufLen, bufLen);
          } else if (sz_strncmp(buffer, "extern", bufLen) == 0) {
            token_push(&next, token_keyword_extern, i - bufLen, bufLen);
          } else if (sz_strncmp(buffer, "as", bufLen) == 0) {
            token_push(&next, token_keyword_as, i - bufLen, bufLen);
          } else if (sz_strncmp(buffer, "if", bufLen) == 0) {
            token_push(&next, token_cf_if, i - bufLen, bufLen);
          } else if (sz_strncmp(buffer, "while", bufLen) == 0) {
            token_push(&next, token_cf_while, i - bufLen, bufLen);
          } else if (sz_strncmp(buffer, "return", bufLen) == 0) {
            token_push(&next, token_cf_return, i - bufLen, bufLen);
          } else if (sz_strncmp(buffer, "break", bufLen) == 0) {
            token_push(&next, token_cf_break, i - bufLen, bufLen);
          } else if (sz_strncmp(buffer, "else", bufLen) == 0) {
            token_push(&next, token_cf_else, i - bufLen, bufLen);
          } else {
            if (numeric) {
              token_push(&next, token_constant, i - bufLen, bufLen);
            } else {
              token_push(&next, token_identifier, i - bufLen, bufLen);
            }
          }
        }

        if (c == '=') {
          if (previous != NULL) {
            switch (previous->type) {
            case token_equals_assign: {
              previous->type = token_double_equals;
              previous->len += 1;
              break;
            }
            case token_greater_than: {
              previous->type = token_greater_than_equal;
              previous->len += 1;
              break;
            }
            case token_less_than: {
              previous->type = token_less_than_equal;
              previous->len += 1;
              break;
            }
            case token_exclaimation: {
              previous->type = token_not_equals;
              previous->len += 1;
              break;
            }
            default: {
              token_push(&next, token_equals_assign, i - bufLen, bufLen);
              break;
            }
            }
          }
        } else if (c == ',') {
          token_push(&next, token_comma, i - bufLen, bufLen);
        } else if (c == ';') {
          token_push(&next, token_semicolon, i - bufLen, bufLen);
        } else if (c == ':') {
          token_push(&next, token_colon, i - bufLen, bufLen);
        } else if (c == '+') {
          token_push(&next, token_plus, i - bufLen, bufLen);
        } else if (c == '-') {
          token_push(&next, token_minus, i - bufLen, bufLen);
        } else if (c == '/') {
          if (previous != NULL && previous->type == token_slash) {
            next = previous;
            mode = comment_nl;
          } else {
            token_push(&next, token_slash, i - bufLen, bufLen);
          }
        } else if (c == '*') {
          if (previous != NULL && previous->type == token_slash) {
            next = previous;
            mode = comment_cl;
          } else {
            token_push(&next, token_asterik, i - bufLen, bufLen);
          }
        } else if (c == '&') {
          if (previous != NULL && previous->type == token_amperstand) {
            previous->type = token_double_amperstand;
            previous->len += 1;
          } else {
            token_push(&next, token_amperstand, i - bufLen, bufLen);
          }
        } else if (c == '^') {
          token_push(&next, token_caret, i - bufLen, bufLen);
        } else if (c == '|') {
          if (previous != NULL && previous->type == token_vertical_bar) {
            previous->type = token_double_vertical_bar;
            previous->len += 1;
          } else {
            token_push(&next, token_vertical_bar, i - bufLen, bufLen);
          }
        } else if (c == '~') {
          token_push(&next, token_tilde, i - bufLen, bufLen);
        } else if (c == '!') {
          token_push(&next, token_exclaimation, i - bufLen, bufLen);
        } else if (c == '.') {
          token_push(&next, token_period, i - bufLen, bufLen);
        } else if (c == '<') {
          if (previous != NULL && previous->type == token_less_than) {
            previous->type = token_left_shift;
            previous->len += 1;
          } else {
            token_push(&next, token_less_than, i - bufLen, bufLen);
          }
        } else if (c == '>') {
          if (previous != NULL) {
            if (previous->type == token_greater_than) {
              previous->type = token_right_shift;
              previous->len += 1;
            } else if (previous->type == token_minus) {
              previous->type = token_arrow;
              previous->len += 1;
            } else {
              token_push(&next, token_greater_than, i - bufLen, bufLen);
            }
          } else {
            token_push(&next, token_greater_than, i - bufLen, bufLen);
          }
        } else if (c == '(') {
          token_push(&next, token_opening_paren, i - bufLen, bufLen);
        } else if (c == ')') {
          token_push(&next, token_closing_paren, i - bufLen, bufLen);
        } else if (c == '[') {
          token_push(&next, token_opening_sqbr, i - bufLen, bufLen);
        } else if (c == ']') {
          token_push(&next, token_closing_sqbr, i - bufLen, bufLen);
        } else if (c == '{') {
          token_push(&next, token_opening_curly_brace, i - bufLen, bufLen);
        } else if (c == '}') {
          token_push(&next, token_closing_curly_brace, i - bufLen, bufLen);
        } else if (c == '"') {
          mode = q_string;
        }
        bufLen = 0;
        numeric = false;
      } else if (isdigit(c)) {
        if (bufLen == 0)
          numeric = true;
        buffer[bufLen++] = (char)c;
      } else if (isalpha(c)) {
        if (numeric) {
          puts("Identifier cannot start with number");
          return false;
        }
        buffer[bufLen++] = (char)c;
      } else {
        printf("Unknown character '%c' (%i)\n", c, c);
        return false;
      }
      break;
    }
    case comment_nl: {
      if (c == '\n') {
        mode = any;
      }
      break;
    }
    case comment_cl: {
      if (last == '*' && c == '/') {
        mode = any;
      }
      break;
    }
    case q_string: {
      if (c == '"' && !escaping) {
        token_push(&next, token_string, i - (bufLen + 1), bufLen + 2);
        mode = any;
        buffer[bufLen] = '\0';
        bufLen = 0;
      } else {
        if (c == '\\') {
          escaping = !escaping;
        } else {
          escaping = false;
        }
        buffer[bufLen++] = (char)c;
      }
      break;
    }
    }
    last = (char)c;
  }

  assert(len < INT_MAX);
  next->type = token_eof;
  next->index = (int)len;
  next->len = 0;
  return true;
}

const char *token_name(const TokenType type) {
  switch (type) {
  case token_eof:
    return "EOF";
  case token_opening_paren:
    return "(";
  case token_closing_paren:
    return ")";
  case token_opening_sqbr:
    return "[";
  case token_closing_sqbr:
    return "]";
  case token_comma:
    return ",";
  case token_opening_curly_brace:
    return "{";
  case token_closing_curly_brace:
    return "}";
  case token_semicolon:
    return ";";
  case token_colon:
    return ":";
  case token_plus:
    return "+";
  case token_minus:
    return "-";
  case token_asterik:
    return "*";
  case token_slash:
    return "/";
  case token_vertical_bar:
    return "|";
  case token_caret:
    return "^";
  case token_amperstand:
    return "&";
  case token_tilde:
    return "~";
  case token_string:
    return "<string>";
  case _:
    return "INVALID";
  case token_double_vertical_bar:
    return "||";
  case token_double_amperstand:
    return "&&";
  case token_period:
    return ".";
  case token_arrow:
    return "->";
  case token_left_shift:
    return "<<";
  case token_right_shift:
    return ">>";
  case token_keyword_fn:
    return "fn";
  case token_keyword_let:
    return "var";
  case token_keyword_extern:
    return "extern";
  case token_keyword_as:
    return "as";
  case token_identifier:
    return "<identifier>";
  case token_constant:
    return "<constant>";
  case token_not_equals:
    return "!=";
  case token_double_equals:
    return "==";
  case token_less_than:
    return "<";
  case token_greater_than:
    return ">";
  case token_less_than_equal:
    return "<=";
  case token_greater_than_equal:
    return ">=";
  case token_cf_if:
    return "if";
  case token_cf_else:
    return "else";
  case token_cf_while:
    return "while";
  case token_cf_return:
    return "return";
  case token_cf_break:
    return "break";
  case token_equals_assign:
    return "=";
  case token_exclaimation:
    return "!";
  }
  // unreachable
  puts("INVALID TYPE??");
  abort();
}

bool token_value_compare(const Token *token, const char *contents, const char *compare) {
  if (compare == NULL)
    return false;
  contents += token->index;
  for (int i = 0; i < token->len; i++) {
    if (contents[i] != compare[i]) {
      return false;
    }
  }
  return true;
}

char *token_copy(const Token *token, const char *contents) {
  char *str = malloc(token->len + 1);
  memcpy(str, contents + token->index, token->len);
  str[token->len] = '\0';
  return str;
}

void token_copy_to(const Token *token, const char *contents, char *output) {
  memcpy(output, contents + token->index, token->len);
}

int token_str_cmp(const Token *token, const char *contents, const char *cmp) {
  if (cmp == NULL)
    return -1;
  if (strlen(cmp) > token->len)
    return 1;
  return strncmp(contents + token->index, cmp, token->len);
}

void token_free(Token *token) {
  if (token != NULL) {
    token_free(token->next);
    free(token);
  }
}

void tokens_init(Token *token) {
  token->index = 0;
  token->len = 0;
  token->next = NULL;
  token->prev = NULL;
}

LIST_IMPL(Token, token, Token)
