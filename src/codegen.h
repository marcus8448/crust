#ifndef CODEGEN_H
#define CODEGEN_H
#include "ir.h"

#include <stdio.h>
typedef enum {
  L_None,
  L_Stack,
  L_Register
} LocationType;

typedef struct {
  LocationType location;
  union {
    int offset;
    int8_t reg;
  };
} Storage;

typedef struct {
  bool inUse;
  bool mustRestore;
  int16_t restoreOffset;
} RegisterState;

LIST_API(Storage, storage, Storage)

typedef struct Registers {
  RegisterState registers[16];
  int offset;
  Storage *storage;
  int parentCutoff;
} Registers;

void registers_init(Registers *registers, InstructionTable *table);
void registers_init_child(Registers *registers, InstructionTable *table, const Registers *parent);
void registers_free(Registers *registers);

void registers_claim(Registers *registers, Allocation *allocation);
void registers_make_stack(Registers *registers, Allocation *allocation, FILE *output);
void registers_free_register(Registers *registers, Allocation *allocation);

Storage *registers_get_storage(const Registers *registers, Allocation *allocation);
void registers_move_tostack(Registers *registers, Allocation *allocation, FILE *output);
void registers_claim_register(Registers *registers, Allocation *output, int8_t reg);
void registers_claim_stack(const Registers *registers, Allocation *output, int16_t offset);
void registers_force_register(const Registers *registers, Reference allocation, int8_t reg, FILE *output);
void registers_override(const Registers *registers, Allocation *output, Allocation *from);

char *registers_get_mnemonic(const Registers *registers, Reference reference);
void generate_statement(Registers *registers, const char *contents, InstructionTable *table, VarList *globals,
                        FunctionList *functions, StrList *literals, FILE *output);
#endif // CODEGEN_H
