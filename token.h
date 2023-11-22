#ifndef CLAMOR_TOKEN_H
#define CLAMOR_TOKEN_H

#include <stdint.h>

typedef enum
{
    eof = '\0',

    opening_paren = '(',
    closing_paren = ')',
    comma = ',',
    opening_curly_brace = '{',
    closing_curly_brace = '}',
    semicolon = ';',

    op_add = '+',
    op_sub = '-',
    op_mul = '*',
    op_div = '/',

    op_or = '|',
    op_xor = '^',
    op_and = '&',
    op_not = '~',

    _ = 128,

    op_lsh,
    op_rsh,

    keyword_define,
    keyword_var,

    identifier,
    constant,

    compare_equals,
    less_than,
    greater_than,
    less_than_equal,
    greater_than_equal,

    cf_if,
    cf_else,
    cf_return,

    equals_assign
} TokenType;

const char *token_name(TokenType type);

typedef struct Token
{
    TokenType type;
    struct Token* next;
    struct Token* prev;

    int index;
    int len;
} Token;


typedef struct
{
    Token* head;
    Token* tail;
} TokenList;

typedef struct
{
    TokenList tokens;
} Arguments;

void token_create(Token* token, TokenType type, int index, int len);

void tokens_init(TokenList* list);

Token* tokens_next(TokenList* list);

Token* tokens_last(const TokenList* list);

void tokens_free(TokenList* list);

Token* tokens_unwrap(TokenList* list);

#endif //CLAMOR_TOKEN_H
