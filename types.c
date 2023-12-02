#include "types.h"

#include <stddef.h>
#include <stdlib.h>

Size typekind_size(const TypeKind kind)
{
    return kind & 0b001111;
}

int size_bytes(const Size size)
{
    switch (size)
    {
    case unused:
        return 0;
    case s8:
        return 1;
    case s16:
        return 2;
    case s32:
        return 4;
    case s64:
        return 8;
    }
    exit(5);
}

const char* size_mnemonic(const Size size)
{
    switch (size)
    {
    case unused:
        return 0;
    case s8:
        return "byte";
    case s16:
        return "short";
    case s32:
        return "long";
    case s64:
        return "quad";
    }
    return 0;
}

bool is_fp(const TypeKind kind)
{
    return (kind & 0b010000) == 0b010000;
}

bool is_pointer(const TypeKind kind)
{
    return kind == ptr;
}

Result parse_type(const char* contents, const Token** token, Type* type)
{
    int indirection = 0;
    while (*token != NULL)
    {
        *token = (*token)->next;
        if ((*token)->type == token_opening_sqbr)
        {
            indirection++;
            type->kind = ptr;
            type->inner = malloc(sizeof(Type));
            type = type->inner;
        }
        else
        {
            token_matches(*token, token_identifier);
            type->inner = NULL;

            if (token_value_compare(*token, contents, "i64"))
            {
                type->kind = i64;
            }
            else if (token_value_compare(*token, contents, "i32"))
            {
                type->kind = i32;
            }
            else if (token_value_compare(*token, contents, "i16"))
            {
                type->kind = i16;
            }
            else if (token_value_compare(*token, contents, "i8") || token_value_compare(*token, contents, "char"))
            {
                type->kind = i8;
            }
            else if (token_value_compare(*token, contents, "f64"))
            {
                type->kind = f64;
            }
            else if (token_value_compare(*token, contents, "f32"))
            {
                type->kind = f32;
            }
            else
            {
                return failure(*token, "Unknown type");
            }
            break;
        }
    }

    while (indirection > 0)
    {
        *token = (*token)->next;
        token_matches(*token, token_closing_sqbr);
        indirection--;
    }
    return success();
}
