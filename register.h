#ifndef REGISTER_H
#define REGISTER_H
#include <stdio.h>

#include "ast.h"
#include "list.h"
#include "types.h"

enum RegisterFP
{
    xmm0 = 16,
    //don't overlap with normal registers
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
extern int argumentRegisters[6];

typedef enum
{
    ref_variable,
    ref_temporary,
    ref_constant
} ValueRefType;

typedef struct
{
    ValueRefType type;
    Type value_type;

    union
    {
        struct
        {
            const char *name;
            char reg;
            int offset;
            bool on_stack;
        };

        struct
        {
            char* repr;
        };
    };
} ValueRef;

LIST_API(StackAlloc, stackalloc, ValueRef)

typedef struct StackFrame
{
    const struct StackFrame* parent;
    StackAllocList allocations;
    bool activeRegisters[16];
    int curOffset;
} StackFrame;

typedef struct
{
    FunctionList functions;
    VarList globals;
    StrList strLiterals;
} Globals;

char get_suffix(Width size);

void stackframe_init(StackFrame* frame, const StackFrame* parent);
ValueRef* stackframe_claim_or_copy_from(StackFrame* frame, Variable variable, char reg, FILE* output);
ValueRef* stackframe_allocate(StackFrame* frame, Variable variable);
ValueRef* stackframe_allocate_temporary(StackFrame* frame, Type type, FILE* output);
void stackframe_allocate_temporary_from(StackFrame* frame, ValueRef** maybe_temp, FILE* output);
ValueRef* stackframe_allocate_variable_from(StackFrame* frame, ValueRef* value, Variable variable, FILE* file);
void stackframe_free_ref(StackFrame* frame, ValueRef* ref);
ValueRef* stackframe_get(StackFrame* frame, Variable variable);
ValueRef* stackframe_get_by_name(StackFrame* frame, const char* name);
ValueRef* stackframe_get_by_token(StackFrame* frame, const char* contents, Token* id);
char* allocation_mnemonic(const ValueRef* alloc);
char* stackframe_mnemonic(StackFrame* frame, Variable variable);
char stackframe_make_register_available(StackFrame* frame, FILE* output);
void stackframe_load_arguments(StackFrame* frame, const Function* function);
void stackframe_moveto_register(StackFrame* frame, ValueRef* alloc, FILE* output);
void stackframe_force_into_register(StackFrame* frame, ValueRef* alloc, char reg, FILE* output);
void stackframe_set_or_copy_register(StackFrame* frame, ValueRef* ref, char reg, FILE* output);
void stackframe_moveto_stack(StackFrame* frame, ValueRef* ref, FILE* output);
void stackframe_set_register(StackFrame* frame, ValueRef* ref, char reg);

void stackframe_moveto_register_v(StackFrame* frame, Variable variable, FILE* output);
void stackframe_free(const StackFrame* frame, FILE* output);

const char* get_register_mnemonic(Width size, int index);

char ref_get_register(const ValueRef *ref);
void ref_init_var(ValueRef* ref, Variable variable);
void ref_init_temp(ValueRef* ref, Type type);

#endif //REGISTER_H
