#include <malloc.h>
#include <assert.h>
#include "token.h"

#include <stdio.h>
#include <stdlib.h>


const char *token_name(const TokenType type)
{
    switch (type) {
    case eof:
        return "eof";
    case opening_paren:
        return "opening_paren";
    case closing_paren:
        return "closing_paren";
    case comma:
        return "comma";
    case opening_curly_brace:
        return "opening_curly_brace";
    case closing_curly_brace:
        return "closing_curly_brace";
    case semicolon:
        return "semicolon";
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
    case _:
        return "INVALID";
    case op_lsh:
        return "op_lsh";
    case op_rsh:
        return "op_rsh";
    case keyword_define:
        return "keyword_define";
    case keyword_var:
        return "keyword_var";
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

void token_create(Token* token, TokenType type, int index, int len)
{
    token->type = type;
    token->index = index;
    token->len = len;
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
