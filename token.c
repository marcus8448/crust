#include <malloc.h>
#include <assert.h>
#include "token.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool tokenize(const char *data, const size_t len, TokenList* list)
{
    typedef enum
    {
        any,
        comment_nl,
        comment_cl,
        q_string
    } Mode;

    char buffer[256];
    unsigned char bufLen = 0;
    bool numeric = false;
    bool escaping = false;
    Mode mode = any;
    char last = 0;
    for (int i = 0; i < len; i++)
    {
        const int c = data[i];
        if (c == EOF)
        {
            continue;
        }

        switch (mode)
        {
        case any:
            if (isspace(c) || c == '=' || c == ',' || c == '(' || c == ')' || c == '[' || c == ']'
                || c == '{' || c == '}' || c == '<' || c == '>' || c == '~' || c == '&' || c == '|' || c == '!'
                || c == '+' || c == '-' || c == '/' || c == '*' || c == ';' || c == '"' || c == ':')
            {
                if (bufLen > 0)
                {
                    buffer[bufLen] = '\0';
                    if (strcmp(buffer, "fn") == 0)
                    {
                        token_create(tokens_next(list), keyword_fn, i - bufLen, bufLen);
                    }
                    else if (strcmp(buffer, "var") == 0)
                    {
                        token_create(tokens_next(list), keyword_var, i - bufLen, bufLen);
                    }
                    else if (strcmp(buffer, "extern") == 0)
                    {
                        token_create(tokens_next(list), keyword_extern, i - bufLen, bufLen);
                    }
                    else if (strcmp(buffer, "if") == 0)
                    {
                        token_create(tokens_next(list), cf_if, i - bufLen, bufLen);
                    }
                    else if (strcmp(buffer, "return") == 0)
                    {
                        token_create(tokens_next(list), cf_return, i - bufLen, bufLen);
                    }
                    else if (strcmp(buffer, "else") == 0)
                    {
                        token_create(tokens_next(list), cf_else, i - bufLen, bufLen);
                    }
                    else
                    {
                        if (numeric)
                        {
                            token_create(tokens_next(list), constant, i - bufLen, bufLen);
                        }
                        else
                        {
                            token_create(tokens_next(list), identifier, i - bufLen, bufLen);
                        }
                    }
                }

                if (c == '=')
                {
                    Token *previous = list->tail;
                    switch (previous->type)
                    {
                    case equals_assign:
                        previous->type = compare_equals;
                        previous->len += 1;
                        break;
                    case greater_than:
                        previous->type = greater_than_equal;
                        previous->len += 1;
                        break;
                    case less_than:
                        previous->type = less_than_equal;
                        previous->len += 1;
                        break;
                    default:
                        token_create(tokens_next(list), equals_assign, i - bufLen, bufLen);
                        break;
                    }
                }
                else if (c == ',')
                {
                    token_create(tokens_next(list), comma, i - bufLen, bufLen);
                }
                else if (c == ';')
                {
                    token_create(tokens_next(list), semicolon, i - bufLen, bufLen);
                }
                else if (c == ':')
                {
                    token_create(tokens_next(list), colon, i - bufLen, bufLen);
                }
                else if (c == '+')
                {
                    token_create(tokens_next(list), op_add, i - bufLen, bufLen);
                }
                else if (c == '-')
                {
                    token_create(tokens_next(list), op_sub, i - bufLen, bufLen);
                }
                else if (c == '/')
                {
                    if (list->tail->type == op_div)
                    {
                        mode = comment_nl;
                    } else
                    {
                        token_create(tokens_next(list), op_div, i - bufLen, bufLen);
                    }
                }
                else if (c == '*')
                {
                    if (list->tail->type == op_div)
                    {
                        mode = comment_cl;
                    } else {
                        token_create(tokens_next(list), op_mul, i - bufLen, bufLen);
                    }
                }
                else if (c == '&')
                {
                    token_create(tokens_next(list), op_and, i - bufLen, bufLen);
                }
                else if (c == '|')
                {
                    token_create(tokens_next(list), op_or, i - bufLen, bufLen);
                }
                else if (c == '~')
                {
                    token_create(tokens_next(list), op_not, i - bufLen, bufLen);
                }
                else if (c == '!')
                {
                    token_create(tokens_next(list), op_not, i - bufLen, bufLen);
                }
                else if (c == '<')
                {
                    Token *previous = list->tail;
                    if (previous->type == less_than)
                    {
                        previous->type = op_lsh;
                        previous->len += 1;
                    } else
                    {
                        token_create(tokens_next(list), less_than, i - bufLen, bufLen);
                    }
                }
                else if (c == '>')
                {
                    Token *previous = list->tail;
                    if (previous->type == greater_than)
                    {
                        previous->type = op_rsh;
                        previous->len += 1;
                    } else if (previous->type == op_sub)
                    {
                        previous->type = arrow;
                        previous->len += 1;
                    } else
                    {
                        token_create(tokens_next(list), greater_than, i - bufLen, bufLen);
                    }
                }
                else if (c == '(')
                {
                    token_create(tokens_next(list), opening_paren, i - bufLen, bufLen);
                }
                else if (c == ')')
                {
                    token_create(tokens_next(list), closing_paren, i - bufLen, bufLen);
                }
                else if (c == '{')
                {
                    token_create(tokens_next(list), opening_curly_brace, i - bufLen, bufLen);
                }
                else if (c == '}')
                {
                    token_create(tokens_next(list), closing_curly_brace, i - bufLen, bufLen);
                } else if (c == '"')
                {
                    mode = q_string;
                }
                bufLen = 0;
                numeric = false;
            }
            else if (isdigit(c))
            {
                if (bufLen == 0) numeric = true;
                buffer[bufLen++] = (char)c;
            }
            else if (isalpha(c))
            {
                if (numeric)
                {
                    puts("Identifier cannot start with number");
                    return false;
                }
                buffer[bufLen++] = (char)c;
            }
            else
            {
                printf("Unknown character '%c' (%i)\n", c, c);
                return false;
            }
            break;
        case comment_nl:
            if (last == '*' && c == '/')
            {
                mode = any;
            }
            break;
        case comment_cl:
            if (last == '*' && c == '/')
            {
                mode = any;
            }
            break;
        case q_string:
            if (c == '"' && !escaping)
            {
                token_create(tokens_next(list), string, i - (bufLen + 1), bufLen + 2);
                mode = any;
                buffer[bufLen] = '\0';
                bufLen = 0;
            } else
            {
                if (c == '\\')
                {
                    escaping = !escaping;
                } else
                {
                    escaping = false;
                }
                buffer[bufLen++] = c;
            }
            break;
        }
        last = c;
    }
    tokens_next(list)->type = eof;
    return true;
}

const char *token_name(const TokenType type)
{
    switch (type) {
    case eof:
        return "eof";
    case opening_paren:
        return "opening_paren";
    case closing_paren:
        return "closing_paren";
    case opening_sqbr:
        return "opening_sqbr";
    case closing_sqbr:
        return "closing_sqbr";
    case comma:
        return "comma";
    case opening_curly_brace:
        return "opening_curly_brace";
    case closing_curly_brace:
        return "closing_curly_brace";
    case semicolon:
        return "semicolon";
    case colon:
        return "colon";
    case op_add:
        return "op_add";
    case op_sub:
        return "op_sub";
    case op_mul:
        return "op_mul";
    case op_div:
        return "op_div";
    case op_or:
        return "op_or";
    case op_xor:
        return "op_xor";
    case op_and:
        return "op_and";
    case op_not:
        return "op_not";
    case string:
        return "string";
    case _:
        return "INVALID";
    case arrow:
        return "arrow";
    case op_lsh:
        return "op_lsh";
    case op_rsh:
        return "op_rsh";
    case keyword_fn:
        return "keyword_fn";
    case keyword_var:
        return "keyword_var";
    case keyword_extern:
        return "keyword_extern";
    case identifier:
        return "identifier";
    case constant:
        return "constant";
    case compare_equals:
        return "compare_equals";
    case less_than:
        return "less_than";
    case greater_than:
        return "greater_than";
    case less_than_equal:
        return "less_than_equal";
    case greater_than_equal:
        return "greater_than_equal";
    case cf_if:
        return "cf_if";
    case cf_else:
        return "cf_else";
    case cf_return:
        return "cf_return";
    case equals_assign:
        return "equals_assign";
    }
    // unreachable
    puts("INVALID TYPE??");
    abort();
}

bool successful(const Result result)
{
    return result.failure == NULL && result.reason == NULL;
}

Result success()
{
    Result success;
    success.failure = NULL;
    success.reason = NULL;
    return success;
}

Result failure(const Token* failure, const char* reason)
{
    Result error;
    error.failure = failure;
    error.reason = reason;
    return error;
}

void token_create(Token* token, const TokenType type, const int index, const int len)
{
    token->type = type;
    token->index = index;
    token->len = len;
}

bool token_value_compare(const Token* token, const char* contents, const char* compare)
{
    contents += token->index;
    for (int i = 0; i < token->len; i++)
    {
        if (contents[i] != compare[i])
        {
            return false;
        }
    }
    return true;
}

char* token_copy(const Token* token, const char* contents)
{
    char *str = malloc(token->len + 1);
    memcpy(str, contents + token->index, token->len);
    str[token->len] = '\0';
    return str;
}

void tokens_init(TokenList* list)
{
    list->head = NULL;
    list->tail = NULL;
}

Token* tokens_next(TokenList* list)
{
    Token *token = malloc(sizeof(Token));
    token->next = NULL;
    token->index = 0;
    token->len = 0;
    if (list->head == NULL)
    {
        token->prev = NULL;
        list->head = list->tail = token;
    } else
    {
        list->tail->next = token;
        token->prev = list->tail;
        list->tail = token;
    }
    return token;
}

void tokens_free(TokenList* list)
{
    Token* token = list->head;
    while (token != NULL)
    {
        Token* tok = token;
        token = token->next;
        free(tok);
    }
    list->head = NULL;
    list->tail = NULL;
}
