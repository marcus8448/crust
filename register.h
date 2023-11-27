#ifndef REGISTER_H
#define REGISTER_H
#include <stdio.h>

#include "ast.h"
#include "list.h"
#include "types.h"


enum RegisterFP
{
    xmm0 = 16, //don't overlap with normal registers
    xmm1,
    xmm2,
    xmm3,
    xmm4,
    xmm5,
    xmm6,
    xmm7,
    xmm8,
    xmm9,
    xmm10,
    xmm11,
    xmm12,
    xmm13,
    xmm14,
    xmm15
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

extern const char* mnemonicFP[16];
extern const char* mnemonic64[16];
extern const char* mnemonic32[16];
extern const char* mnemonic16[16];
extern const char* mnemonic8[16];

struct RegisterHandle
{
    size_t id;
};

typedef struct
{
    Variable variable;
    bool isRegister;
    int allocation;
} VariableAllocation;

typedef struct
{
    char *registers[16];
    Size size[16];

    StrList globals;
    IntList globalSizes;
} Registers;

typedef struct StackFrame
{
    const struct StackFrame *parent;
    Variable registers[16];
    bool active[16];
    int curOffset;
} StackFrame;

typedef struct
{
    FunctionList functions;
    VarList globals;
    StrList strLiterals;
} Globals;

void stackframe_init(StackFrame *frame, const StackFrame *parent);
void stackframe_claim(StackFrame *frame, Variable variable);
const char *stackframe_get(StackFrame *frame, Variable variable);
void stackframe_free(StackFrame *frame, FILE* output);

void register_init(Registers *registers);
int register_get(const Registers *registers, const char *variable);
const char *register_get_mnemonic_v(const Registers *registers, const char *variable);
const char *register_get_mnemonic(const Registers* registers, int reg);
void register_release(Registers *registers, int reg);
void register_release_v(Registers *registers, const char *variable);
int register_claim(Registers* registers, char* variable, Size size);
void register_push_all(const Registers* registers, FILE* output);
void register_pop_all(const Registers* registers, FILE* output);

#endif //REGISTER_H
