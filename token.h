#ifndef CLAMOR_TOKEN_H
#define CLAMOR_TOKEN_H
#include <stdbool.h>
#include <stddef.h>

#define token_seek_until(token, token_type) while ((token)->type != token_type) { token = (token)->next; if ((token)->type == eof) { token_matches(token, token_type); } }
#define token_matches(token, token_type) if ((token)->type != token_type) { return failure(token, "Expected: "#token_type); }
#define token_matches_ext(token, token_type, def) if ((token)->type != token_type) { return failure(token, "Expected: " def); }

typedef enum
{
    eof = '\0',

    opening_paren = '(',
    closing_paren = ')',
    opening_sqbr = '[',
    closing_sqbr = ']',
    comma = ',',
    opening_curly_brace = '{',
    closing_curly_brace = '}',
    semicolon = ';',
    colon = ':',

    op_add = '+',
    op_sub = '-',
    op_mul = '*',
    op_div = '/',

    op_or = '|',
    op_xor = '^',
    op_and = '&',
    op_not = '~',

    string = '"',

    _ = 128,

    arrow,

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
    const Token *failure;
    const char *reason;
} Result;

bool successful(Result result);

Result success();

Result failure(const Token* failure, const char *reason);

void token_create(Token* token, TokenType type, int index, int len);
bool token_value_compare(const Token *token, const char* contents, const char *compare);
char *token_copy(const Token *token, const char* contents);

void tokens_init(TokenList* list);
Token* tokens_next(TokenList* list);
void tokens_free(TokenList* list);

bool tokenize(const char *data, size_t len, TokenList* list);

#endif //CLAMOR_TOKEN_H
