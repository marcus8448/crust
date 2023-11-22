#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void register_init(Registers* registers)
{
    for (int i = 0; i < 16; ++i)
    {
        registers->size[i] = 0;
        registers->registers[i] = 0;
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
    int glob = strlist_indexof_val(&registers->globals, variable);
    if (glob != -1)
    {
        const size_t size = strlen(variable) + 5 + 1 + 4 + 6;
        char *buf = malloc(size);
        switch ((enum Size)registers->globalSizes.array[glob]) {
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

int register_claim(Registers* registers, char* variable, enum Size size)
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


const char* mnemonic64[16] = {
    "rax",
    "rbx",
    "rcx",
    "rdx",
    "rsi",
    "rdi",
    "rbp",
    "rsp",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15"
};

const char* mnemonic32[16] = {
    "eax",
    "ebx",
    "ecx",
    "edx",
    "esi",
    "edi",
    "ebp",
    "esp",
    "r8d",
    "r9d",
    "r10d",
    "r11d",
    "r12d",
    "r13d",
    "r14d",
    "r15d"
};

const char* mnemonic16[16] = {
    "ax",
    "bx",
    "cx",
    "dx",
    "si",
    "di",
    "bp",
    "sp",
    "r8w",
    "r9w",
    "r10w",
    "r11w",
    "r12w",
    "r13w",
    "r14w",
    "r15w"
};

const char* mnemonic8[16] = {
    "al",
    "bl",
    "cl",
    "dl",
    "sil",
    "dil",
    "bpl",
    "spl",
    "r8b",
    "r9b",
    "r10b",
    "r11b",
    "r12b",
    "r13b",
    "r14b",
    "r15b"
};
