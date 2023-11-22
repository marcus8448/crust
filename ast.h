#ifndef AST_H
#define AST_H
#include <stdio.h>

#include "list.h"

enum Size
{
    unused,
    s64,
    s32,
    s16,
    s8
};

enum Register64
{
    rax,
    rbx,
    rcx,
    rdx,
    rsi,
    rdi,
    rbp,
    rsp,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15
};

extern const char* mnemonic64[16];

enum Register32
{
    eax,
    ebx,
    ecx,
    edx,
    esi,
    edi,
    ebp,
    esp,
    r8d,
    r9d,
    r10d,
    r11d,
    r12d,
    r13d,
    r14d,
    r15d,
};

extern const char* mnemonic32[16];

enum Register16
{
    ax,
    bx,
    cx,
    dx,
    si,
    di,
    bp,
    sp,
    r8w,
    r9w,
    r10w,
    r11w,
    r12w,
    r13w,
    r14w,
    r15w,
};

extern const char* mnemonic16[16];

enum Register8
{
    al,
    bl,
    cl,
    dl,
    sil,
    dil,
    bpl,
    spl,
    r8b,
    r9b,
    r10b,
    r11b,
    r12b,
    r13b,
    r14b,
    r15b,
};

extern const char* mnemonic8[16];

struct RegisterHandle
{
    size_t id;
};

typedef struct
{
    char *registers[16];
    enum Size size[16];

    StrList globals;
    IntList globalSizes;
} Registers;

void register_init(Registers *registers);
int register_get(const Registers *registers, const char *variable);
const char *register_get_mnemonic_v(const Registers *registers, const char *variable);
const char *register_get_mnemonic(const Registers* registers, int reg);
void register_release(Registers *registers, int reg);
void register_release_v(Registers *registers, const char *variable);
int register_claim(Registers* registers, char* variable, enum Size size);
void register_push_all(const Registers* registers, FILE* output);
void register_pop_all(const Registers* registers, FILE* output);

// typedef struct
// {
//
// } Stack;

typedef struct
{
    char *name;
    StrList arguments;
} Function;

typedef struct
{
    char *name;
    char *location;
} Variable;

typedef struct
{
    StrList variables;
} Scope;

typedef enum
{
    t_add,
    t_sub,
    t_mul,
    t_div,

    t_and,
    t_or,

    t_equal_to,
    t_less_than,
    t_greater_than,
    t_less_than_equal,
    t_greater_than_equal
} OpType;

typedef struct
{
    int left;
    int right;
    OpType opType;
} Operation;

#endif //AST_H
