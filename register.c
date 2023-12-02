#include "register.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LIST_IMPL(StackAlloc, stackalloc, ValueRef)

void stackframe_init(StackFrame* frame, const StackFrame* parent)
{
    //TODO: rbx, rsp, rbp, r12, r13, r14, and r15
    stackalloclist_init(&frame->allocations, 2);
    frame->parent = parent;
    for (int i = 0; i < 16; i++)
    {
        if (parent != NULL)
        {
            frame->activeRegisters[i] = parent->activeRegisters[i];
        }
        else
        {
            frame->activeRegisters[i] = false;
        }
    }
    if (parent != NULL)
    {
        frame->curOffset = parent->curOffset;
        for (int i = 0; i < parent->allocations.len; i++)
        {
            stackalloclist_add(&frame->allocations, parent->allocations.array[i]);
        }
    }
    else
    {
        frame->curOffset = 0;
    }
}

ValueRef* stackframe_claim_or_copy_from(StackFrame* frame, const Variable variable, const char reg, FILE* output)
{
    if (!frame->activeRegisters[reg])
    {
        for (int i = 0; i < frame->allocations.len; ++i)
        {
            if (strcmp(variable.name, frame->allocations.array[i].variable.name) == 0)
            {
                if (frame->allocations.array[i].reg != -1)
                {
                    frame->activeRegisters[frame->allocations.array[i].reg] = false;
                }
                frame->allocations.array[i].reg = reg;
                frame->activeRegisters[reg] = true;
                return &frame->allocations.array[i];
            }
        }

        ValueRef ref;
        ref.type = ref_variable;
        ref.offset = 0;
        ref.on_stack = false;
        ref.reg = reg;
        ref.variable = variable;
        frame->activeRegisters[reg] = true;
        stackalloclist_add(&frame->allocations, ref);
        return &frame->allocations.array[frame->allocations.len - 1];
    }
    for (int i = 0; i < frame->allocations.len; ++i)
    {
        if (strcmp(variable.name, frame->allocations.array[i].variable.name) == 0)
        {
            if (frame->allocations.array[i].reg != -1)
            {
                if (frame->allocations.array[i].reg != reg)
                {
                    const Size size = typekind_size(variable.type.kind);
                    fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, reg),
                            get_register_mnemonic(size, frame->allocations.array[i].reg));
                }
            }
            else
            {
                frame->allocations.array[i].reg = stackframe_make_register_available(frame, output);
                frame->activeRegisters[frame->allocations.array[i].reg] = true;
                const Size size = typekind_size(variable.type.kind);
                fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, reg),
                        get_register_mnemonic(size, frame->allocations.array[i].reg));
                return &frame->allocations.array[i];
            }
        }
    }

    ValueRef ref;
    ref.type = ref_variable;
    ref.offset = 0;
    ref.on_stack = false;
    ref.reg = stackframe_make_register_available(frame, output);
    ref.variable = variable;
    frame->activeRegisters[ref.reg] = true;
    const Size size = typekind_size(variable.type.kind);
    fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, reg),
            get_register_mnemonic(size, ref.reg));
    stackalloclist_add(&frame->allocations, ref);
    return &frame->allocations.array[frame->allocations.len - 1];
}

ValueRef* stackframe_allocate(StackFrame* frame, const Variable variable)
{
    for (int i = 0; i < frame->allocations.len; ++i)
    {
        if (strcmp(variable.name, frame->allocations.array[i].variable.name) == 0)
        {
            return &frame->allocations.array[i];
        }
    }

    ValueRef ref;
    ref.type = ref_variable;
    ref.offset = 0;
    ref.on_stack = false;
    ref.reg = -1;
    ref.variable = variable;
    stackalloclist_add(&frame->allocations, ref);
    return &frame->allocations.array[frame->allocations.len - 1];
}

ValueRef* stackframe_allocate_temporary(StackFrame* frame, FILE* output)
{
    ValueRef ref;
    ref.type = ref_temporary;
    ref.offset = 0;
    ref.on_stack = false;
    ref.reg = stackframe_make_register_available(frame, output);
    frame->activeRegisters[ref.reg] = true;
    stackalloclist_add(&frame->allocations, ref);
    return &frame->allocations.array[frame->allocations.len - 1];
}

void stackframe_allocate_temporary_from(StackFrame* frame, ValueRef** maybe_temp, FILE* output)
{
    switch ((*maybe_temp)->type)
    {
    case ref_temporary:
        break;
    case ref_variable:
    case ref_constant:
        {
            ValueRef* temp = stackframe_allocate_temporary(frame, output);
            char* mnemonicN = allocation_mnemonic(temp);
            char* mnemonicL = allocation_mnemonic(*maybe_temp);
            fprintf(output, "mov%c %s, %s\n", 'q', mnemonicL, mnemonicN); //TODO
            free(mnemonicL);
            if ((*maybe_temp)->type == ref_constant) stackframe_free_ref(frame, *maybe_temp);
            *maybe_temp = temp;
        }
        break;
    }
}

void stackframe_free_ref(StackFrame* frame, ValueRef* ref)
{
    switch (ref->type)
    {
    case ref_variable:
    case ref_temporary:
        if (ref->reg != -1)
        {
            frame->activeRegisters[ref->reg] = false;
            ref->reg = -1;
        }
        break;
    case ref_constant:
        free(ref->repr);
        free(ref);
        break;
    }
}

ValueRef* stackframe_get(StackFrame* frame, const Variable variable)
{
    for (int i = 0; i < frame->allocations.len; ++i)
    {
        if (strcmp(variable.name, frame->allocations.array[i].variable.name) == 0)
        {
            return &frame->allocations.array[i];
        }
    }
    return NULL;
}

ValueRef* stackframe_get_name(StackFrame* frame, const char* name)
{
    for (int i = 0; i < frame->allocations.len; ++i)
    {
        if (strcmp(name, frame->allocations.array[i].variable.name) == 0)
        {
            return &frame->allocations.array[i];
        }
    }
    return NULL;
}

ValueRef* stackframe_get_id(StackFrame* frame, const char* contents, Token* id)
{
    for (int i = 0; i < frame->allocations.len; ++i)
    {
        bool pass = false;
        for (int j = 0; j < id->len; j++)
        {
            if (contents[id->index + j] != frame->allocations.array[i].variable.name[j])
            {
                pass = false;
                break;
            }
            pass = true;
        }
        if (pass)
        {
            return &frame->allocations.array[i];
        }
    }
    return NULL;
}

char* allocation_mnemonic(const ValueRef* alloc)
{
    if (alloc->type == ref_constant)
    {
        return strdup(alloc->repr);
    }
    if (alloc->reg != -1)
    {
        if (alloc->type == ref_temporary)
        {
            return strdup(get_register_mnemonic(s64, alloc->reg)); //FIXME
        }
        return strdup(get_register_mnemonic(typekind_size(alloc->variable.type.kind), alloc->reg));
    }

    assert(alloc->on_stack);
    int len = snprintf(NULL, 0, "%i(%%rbp)", alloc->offset);
    char* mnemonic = malloc(len + 1);
    snprintf(mnemonic, len, "%i(%%rbp)", alloc->offset);
    mnemonic[len] = '\0';
    return mnemonic;
}

char* stackframe_mnemonic(StackFrame* frame, const Variable variable)
{
    return allocation_mnemonic(stackframe_get(frame, variable));
}

char stackframe_make_register_available(StackFrame* frame, FILE* output)
{
    for (int i = 0; i < 16; i++)
    {
        if (i == rbp || i == rsp || i == rbx) continue;
        if (!frame->activeRegisters[i])
        {
            return i;
        }
    }

    for (int i = 0; i < frame->allocations.len; i++)
    {
        ValueRef alloc = frame->allocations.array[i];
        if (alloc.type != ref_constant && alloc.reg != -1)
        {
            const Size size = typekind_size(alloc.variable.type.kind);
            if (!alloc.on_stack)
            {
                alloc.on_stack = true;
                frame->curOffset -= size_bytes(size);
                alloc.offset = frame->curOffset;
            }
            fprintf(output, "mov%c %s, %i(%s)\n", get_suffix(size), get_register_mnemonic(size, alloc.reg), alloc.offset,
                    "%rbp");

            frame->activeRegisters[alloc.reg] = false;
            alloc.reg = -1;
            return i;
        }
    }
    // impossible
    abort();
}

void stackframe_load_arguments(StackFrame* frame, const Function* function)
{
    //rdi, rsi, rdx, rcx, r8, r9 -> stack
    for (int i = 0; i < function->arguments.len; i++)
    {
        ValueRef alloc;
        alloc.variable = function->arguments.array[i];
        if (i < 6)
        {
            alloc.on_stack = false;
            alloc.offset = 0;
            alloc.reg = argumentRegisters[i];
        }
        else
        {
            alloc.on_stack = true;
            alloc.offset = 16 * (i - 6);
            alloc.reg = -1;
        }
        stackalloclist_add(&frame->allocations, alloc);
    }
}

void stackframe_moveto_register(StackFrame* frame, ValueRef* alloc, FILE* output)
{
    if (alloc->reg == -1)
    {
        alloc->reg = stackframe_make_register_available(frame, output);
        const Size size = typekind_size(alloc->variable.type.kind);
        fprintf(output, "mov%c %i(%%rbp), %s\n", get_suffix(size), alloc->offset,
                get_register_mnemonic(size, alloc->reg));
    }
}

void stackframe_force_into_register(StackFrame* frame, ValueRef* alloc, const char reg, FILE* output)
{
    if (alloc->reg == reg) return;
    if (frame->activeRegisters[reg])
    {
        for (int i = 0; i < frame->allocations.len; ++i)
        {
            if (frame->allocations.array[i].reg == reg)
            {
                stackframe_moveto_stack(frame, &frame->allocations.array[i], output);
                frame->allocations.array[i].reg = -1;
                break;
            }
        }
    }

    Size size = typekind_size(alloc->variable.type.kind);
    if (alloc->type == ref_temporary) size = s64;
    fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, alloc->reg),
            get_register_mnemonic(size, reg));
    alloc->reg = reg;
}

void stackframe_set_or_copy_register(StackFrame* frame, ValueRef* ref, char reg, FILE* output)
{
    if (!frame->activeRegisters[reg])
    {
        ref->reg = reg;
        frame->activeRegisters[reg] = true;
    }
    else
    {
        ref->reg = stackframe_make_register_available(frame, output);
        frame->activeRegisters[ref->reg] = true;
        const Size size = typekind_size(ref->variable.type.kind);
        fprintf(output, "mov%c %s, %s\n", get_suffix(size), get_register_mnemonic(size, reg),
                get_register_mnemonic(size, ref->reg));
    }
}

void stackframe_moveto_stack(StackFrame* frame, ValueRef* ref, FILE* output)
{
    if (ref->type == ref_constant) return;
    Size size = typekind_size(ref->variable.type.kind);
    if (ref->type == ref_temporary) size = s64; //fixme
    if (!ref->on_stack)
    {
        ref->on_stack = true;
        frame->curOffset -= size_bytes(size);
        ref->offset = frame->curOffset;
    }

    if (ref->reg != -1)
    {
        fprintf(output, "mov%c %s, %i(%%rbp) # store variable '%s' to stack\n", get_suffix(size),
                get_register_mnemonic(size, ref->reg), ref->offset, ref->variable.name);
    }
    ref->reg = -1;
}


void stackframe_moveto_register_v(StackFrame* frame, const Variable variable, FILE* output)
{
    stackframe_moveto_register(frame, stackframe_get(frame, variable), output);
}

void stackframe_free(const StackFrame* frame, FILE* output)
{
    if (frame->parent == NULL) return;
    for (int i = 0; i < frame->parent->allocations.len; i++)
    {
        frame->parent->allocations.array[i] = frame->allocations.array[i]; //pass changes to previous variables
    }
}

const char* get_register_mnemonic(const Size size, const int index)
{
    switch (size)
    {
    case unused:
        break;
    case s64:
        return mnemonic64[index];
    case s32:
        return mnemonic32[index];
    case s16:
        return mnemonic16[index];
    case s8:
        return mnemonic8[index];
    }

    return NULL;
}

char get_suffix(const Size size)
{
    switch (size)
    {
    case unused:
        exit(30);
    case s8:
        return 'b';
    case s16:
        return 's';
    case s32:
        return 'l';
    case s64:
        return 'q';
    }
    return 0;
}


const char* mnemonicFP[16] = {
    "%xmm0",
    "%xmm1",
    "%xmm2",
    "%xmm3",
    "%xmm4",
    "%xmm5",
    "%xmm6",
    "%xmm7",
    "%xmm8",
    "%xmm9",
    "%xmm10",
    "%xmm11",
    "%xmm12",
    "%xmm13",
    "%xmm14",
    "%xmm15"
};

const char* mnemonic64[16] = {
    "%rax",
    "%rbx",
    "%rcx",
    "%rdx",
    "%rsi",
    "%rdi",
    "%rbp",
    "%rsp",
    "%r8",
    "%r9",
    "%r10",
    "%r11",
    "%r12",
    "%r13",
    "%r14",
    "%r15"
};

const char* mnemonic32[16] = {
    "%eax",
    "%ebx",
    "%ecx",
    "%edx",
    "%esi",
    "%edi",
    "%ebp",
    "%esp",
    "%r8d",
    "%r9d",
    "%r10d",
    "%r11d",
    "%r12d",
    "%r13d",
    "%r14d",
    "%r15d"
};

const char* mnemonic16[16] = {
    "%ax",
    "%bx",
    "%cx",
    "%dx",
    "%si",
    "%di",
    "%bp",
    "%sp",
    "%r8w",
    "%r9w",
    "%r10w",
    "%r11w",
    "%r12w",
    "%r13w",
    "%r14w",
    "%r15w"
};

const char* mnemonic8[16] = {
    "%al",
    "%bl",
    "%cl",
    "%dl",
    "%sil",
    "%dil",
    "%bpl",
    "%spl",
    "%r8b",
    "%r9b",
    "%r10b",
    "%r11b",
    "%r12b",
    "%r13b",
    "%r14b",
    "%r15b"
};

int argumentRegisters[6] = {
    rdi,
    rsi,
    rdx,
    rcx,
    r8,
    r9
};
