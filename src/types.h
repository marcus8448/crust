#ifndef TYPES_H
#define TYPES_H
#include "ir.h"
#include "token.h"

typedef enum {
  i8 = 0b000001,
  i16 = 0b000010,
  i32 = 0b000100,
  i64 = 0b001000,

  f32 = 0b010100,
  f64 = 0b011000,

  ptr = 0b101000,
} TypeKind;

typedef struct Type {
  TypeKind kind;

  union {
    struct Type* inner; // only valid with PTR
  };
} Type;

Result parse_type(const char* contents, const Token** token, Type* type);

Width typekind_width(TypeKind kind);
int size_bytes(Width size);
const char* size_mnemonic(Width size);

#endif // TYPES_H
