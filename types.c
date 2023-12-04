#include "types.h"

#include <stddef.h>
#include <stdlib.h>

Width typekind_width(const TypeKind kind)
{
    switch (kind) {
    case i8:
        return Byte;
    case i16:
        return Word;
    case i32:
        return Long;
    case ptr:
    case i64:
        return Quad;
    case f32:
        return Long;
    case f64:
        return Quad;
    }
    exit(32);
}

int size_bytes(const Width size)
{
    switch (size)
    {
    case Byte:
        return 1;
    case Word:
        return 2;
    case Long:
        return 4;
    case Quad:
        return 8;
    }
    exit(5);
}

const char* size_mnemonic(const Width size)
{
    switch (size)
    {
    case Byte:
        return "byte";
    case Word:
        return "short";
    case Long:
        return "long";
    case Quad:
        return "quad";
    }
    exit(30);
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
