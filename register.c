#include "register.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char globuf_reg[16];

LIST_IMPL(StackAlloc, stackalloc, StackAllocation)

void stackframe_init(StackFrame* frame, const StackFrame *parent)
{
    //TODO: rbx, rsp, rbp, r12, r13, r14, and r15
    stackalloclist_init(&frame->allocations, 2);
    frame->parent = parent;
    for (int i = 0; i < 16; i++)
    {
        if (parent != NULL)
        {
            frame->activeRegisters[i] = parent->activeRegisters[i];
        } else
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
    } else
    {
        frame->curOffset = 0;
    }
}

StackAllocation *stackframe_claim_or_copy_from(StackFrame* frame, const Variable variable, const char reg, FILE* output)
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

        StackAllocation allocation;
        allocation.offset = 0;
        allocation.on_stack = false;
        allocation.reg = reg;
        allocation.variable = variable;
        frame->activeRegisters[reg] = true;
        stackalloclist_add(&frame->allocations, allocation);
        return &frame->allocations.array[frame->allocations.len-1];
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
                    fprintf(output, "mov%c %s, %s", get_suffix(size), get_register_mnemonic(size, reg), get_register_mnemonic(size, frame->allocations.array[i].reg));
                }
            } else
            {
                frame->allocations.array[i].reg = stackframe_make_register_available(frame, output);
                frame->activeRegisters[frame->allocations.array[i].reg] = true;
                const Size size = typekind_size(variable.type.kind);
                fprintf(output, "mov%c %s, %s", get_suffix(size), get_register_mnemonic(size, reg), get_register_mnemonic(size, frame->allocations.array[i].reg));
                return &frame->allocations.array[i];
            }
        }
    }

    StackAllocation allocation;
    allocation.offset = 0;
    allocation.on_stack = false;
    allocation.reg = stackframe_make_register_available(frame, output);
    allocation.variable = variable;
    frame->activeRegisters[allocation.reg] = true;
    const Size size = typekind_size(variable.type.kind);
    fprintf(output, "mov%c %s, %s", get_suffix(size), get_register_mnemonic(size, reg), get_register_mnemonic(size, allocation.reg));
    stackalloclist_add(&frame->allocations, allocation);
    return &frame->allocations.array[frame->allocations.len-1];
}

StackAllocation *stackframe_allocate(StackFrame* frame, const Variable variable)
{
    for (int i = 0; i < frame->allocations.len; ++i)
    {
        if (strcmp(variable.name, frame->allocations.array[i].variable.name) == 0)
        {
            return &frame->allocations.array[i];
        }
    }

    StackAllocation allocation;
    allocation.offset = 0;
    allocation.on_stack = false;
    allocation.reg = -1;
    allocation.variable = variable;
    stackalloclist_add(&frame->allocations, allocation);
    return &frame->allocations.array[frame->allocations.len-1];
}

StackAllocation* stackframe_get(StackFrame* frame, const Variable variable)
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

StackAllocation* stackframe_get_name(StackFrame* frame, const char* name)
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

StackAllocation *stackframe_get_id(StackFrame* frame, const char* contents, Token* id)
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

const char* allocation_mnemonic(const StackAllocation* alloc)
{
    if (alloc->reg != -1)
    {
        return get_register_mnemonic(typekind_size(alloc->variable.type.kind), alloc->reg);
    }
    assert(alloc->on_stack);
    globuf_reg[snprintf(globuf_reg, 15, "%i(%%rbp)", alloc->offset)] = '\0';
    return globuf_reg;
}

const char* stackframe_mnemonic(StackFrame* frame, const Variable variable)
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
        StackAllocation alloc = frame->allocations.array[i];
        if (alloc.reg != -1)
        {
            const Size size = typekind_size(alloc.variable.type.kind);
            if (!alloc.on_stack)
            {
                alloc.on_stack = true;
                frame->curOffset -= size_bytes(size);
                alloc.offset = frame->curOffset;
            }
            fprintf(output, "mov%c %s, %i(%s)", get_suffix(size), get_register_mnemonic(size, alloc.reg), alloc.offset, "%rbp");

            frame->activeRegisters[alloc.reg] = false;
            alloc.reg = -1;
            return i;
        }
    }
    // impossible
    abort();
}

void stackframe_load_arguments(StackFrame* frame, const Function *function)
{
    //rdi, rsi, rdx, rcx, r8, r9 -> stack
    for (int i = 0; i < function->arguments.len; i++)
    {
        StackAllocation alloc;
        alloc.variable = function->arguments.array[i];
        if (i < 6)
        {
            alloc.on_stack = false;
            alloc.offset = 0;
            alloc.reg = argumentRegisters[i];
        } else
        {
            alloc.on_stack = true;
            alloc.offset = 16 * (i - 6);
            alloc.reg = -1;
        }
        stackalloclist_add(&frame->allocations, alloc);
    }
}

void stackframe_moveto_register(StackFrame* frame, StackAllocation* alloc, FILE* output)
{
    if (alloc->reg == -1)
    {
        alloc->reg = stackframe_make_register_available(frame, output);
        const Size size = typekind_size(alloc->variable.type.kind);
        fprintf(output, "mov%c %i(%%rbp), %s", get_suffix(size), alloc->offset, get_register_mnemonic(size, alloc->reg));
    }
}

void stackframe_set_or_copy_register(StackFrame* frame, StackAllocation* ref, char reg, FILE* output)
{
    if (!frame->activeRegisters[reg])
    {
        ref->reg = reg;
        frame->activeRegisters[reg] = true;
    } else
    {
        ref->reg = stackframe_make_register_available(frame, output);
        frame->activeRegisters[ref->reg] = true;
        const Size size = typekind_size(ref->variable.type.kind);
        fprintf(output, "mov%c %s, %s", get_suffix(size), get_register_mnemonic(size, reg), get_register_mnemonic(size, ref->reg));
    }
}

void stackframe_moveto_stack(StackFrame* frame, StackAllocation *ref, FILE* output)
{
    const Size size = typekind_size(ref->variable.type.kind);
    if (!ref->on_stack)
    {
        ref->on_stack = true;
        frame->curOffset -= size_bytes(size);
        ref->offset = frame->curOffset;
    }

    if (ref->reg != -1)
    {
        fprintf(output, "mov%c %s, %i(%%rbp)\n", get_suffix(size), get_register_mnemonic(size, ref->reg), ref->offset);
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

void register_init(Registers* registers)
{
    for (int i = 0; i < 16; ++i)
    {
        registers->size[i] = 0;
        registers->registers[i] = NULL;
    }
    strlist_init(&registers->globals, 2);
    intlist_init(&registers->globalSizes, 2);
}

int register_get(const Registers* registers, const char* variable)
{
    for (int i = 0; i < 16; ++i)
    {
        if (registers->size[i] != unused)
        {
            if (strcmp(registers->registers[i], variable) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

const char *register_get_mnemonic_v(const Registers* registers, const char* variable)
{
    for (int i = 0; i < 16; ++i)
    {
        if (registers->size[i] != unused && strcmp(registers->registers[i], variable) == 0)
        {
            switch (registers->size[i])
            {
            case unused:
                break;
            case s64:
                return mnemonic64[i];
            case s32:
                return mnemonic32[i];
            case s16:
                return mnemonic16[i];
            case s8:
                return mnemonic8[i];
            }
        }
    }
    int glob = strlist_indexof(&registers->globals, variable);
    if (glob != -1)
    {
        const size_t size = strlen(variable) + 5 + 1 + 4 + 6;
        char *buf = malloc(size);
        switch ((Size)registers->globalSizes.array[glob]) {
        case unused:
            break;
        case s64:
            snprintf(buf, size, "QWORD PTR %s[rip]", variable);
            break;
        case s32:
            snprintf(buf, size, "DWORD PTR %s[rip]", variable);
            break;
        case s16:
            snprintf(buf, size, "WORD PTR %s[rip]", variable);
            break;
        case s8:
            snprintf(buf, size, "BYTE PTR %s[rip]", variable);
            break;
        }
    }
    return NULL;
}

const char *register_get_mnemonic(const Registers* registers, int reg)
{
    switch (registers->size[reg])
    {
    case unused:
        break;
    case s64:
        return mnemonic64[reg];
    case s32:
        return mnemonic32[reg];
    case s16:
        return mnemonic16[reg];
    case s8:
        return mnemonic8[reg];
    }

    return NULL;
}

const char *get_register_mnemonic(const Size size, const int index)
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

void register_release(Registers* registers, int reg)
{
    if (registers->size[reg] == unused)
    {
        printf("Tried to release unused register");
        abort();
    }
    registers->registers[reg] = NULL;
    registers->size[reg] = unused;
}

void register_release_v(Registers* registers, const char* variable)
{
    int reg = register_get(registers, variable);
    if (reg == -1)
    {
        printf("Tried to release unused register");
        abort();
    }
    register_release(registers, reg);
}

int register_claim(Registers* registers, char* variable, Size size)
{
    for (int i = 0; i < 16; ++i)
    {
        if (i == rbp || i == rsp || i == rbx) continue;
        if (registers->size[i] == unused)
        {
            registers->size[i] = size;
            registers->registers[i] = variable;
            return i;
        }
    }
    printf("out of registers");
    abort();

}

void register_push_all(const Registers* registers, FILE* output)
{
    for (int i = 0; i < 16; ++i)
    {
        if (registers->size[i] != unused)
        {
            const char* mnemonic = mnemonic64[i];
            fprintf(output, "push %s ; store variable %s before function call\n", mnemonic, registers->registers[i]);
        }
    }
}

void register_pop_all(const Registers* registers, FILE* output)
{
    for (int i = 15; i >= 0; --i)
    {
        if (registers->size[i] != unused)
        {
            fprintf(output, "pop %s ; restore variable %s after function call\n", mnemonic64[i], registers->registers[i]);
        }
    }
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
