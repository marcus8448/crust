#ifndef TYPES_H
#define TYPES_H
#include "token.h"

typedef enum
{
    i8 = 0b000001,
    i16 = 0b000010,
    i32 = 0b000100,
    i64 = 0b001000,

    f32 = 0b010100,
    f64 = 0b011000,

    ptr = 0b101000,
} TypeKind;

typedef struct Type
{
    TypeKind kind;

    union
    {
        struct Type* inner; //only valid with PTR
    };
} Type;

typedef enum
{
    unused = 0b0000,
    s8 = 0b0001,
    s16 = 0b0010,
    s32 = 0b0100,
    s64 = 0b1000
} Size;

Result parse_type(const char* contents, const Token** token, Type* type);

Size typekind_size(TypeKind kind);
int size_bytes(Size size);
const char* size_mnemonic(Size size);

#endif //TYPES_H
