#ifndef TYPES_H
#define TYPES_H
#include "result.h"
#include "token.h"

typedef enum {
  // 8bit
  Byte = 1,
  // 16bit
  Word,
  // 32bit
  Long,
  // 64bit
  Quad
} Width;

typedef enum {
  i8 = 1,
  i16,
  i32,
  i64,

  u8,
  u16,
  u32,
  u64,

  f32,
  f64,

  ptr
} TypeKind;

typedef struct Type {
  TypeKind kind;

  union {
    struct Type *inner; // only valid with PTR
  };
} Type;

Result parse_type(const char *contents, const Token **token, Type *type);

Width type_width(Type type);
int type_size(Type type);
Width typekind_width(TypeKind type);
int typekind_size(TypeKind type);

int size_bytes(Width size);
const char *size_mnemonic(Width size);

#endif // TYPES_H
