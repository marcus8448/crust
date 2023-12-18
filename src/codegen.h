#ifndef CODEGEN_H
#define CODEGEN_H
#include "ir.h"

#include <stdio.h>
typedef enum { L_None, L_Stack, L_Register } LocationType;

typedef struct {
  LocationType location;
  union {
    int offset;
    uint8_t reg;
  };
} Storage;

LIST_API(Storage, storage, Storage)

typedef struct {
  bool registers[16];
  int offset;
  Storage* storage;
} Registers;

void registers_init(Registers* registers, InstructionTable* table);
void registers_free(Registers* registers);
void registers_claim(Registers* registers, Allocation* allocation);
Storage* registers_get_storage(const Registers* registers, Allocation* allocation);
Allocation* registers_allocationfrom_register(const Registers* registers, InstructionTable* table, uint8_t reg);
void registers_move_tostack(Registers* registers, Allocation* allocation, FILE* output);
void registers_claim_register(Registers* registers, Allocation* output, uint8_t reg);
void registers_claim_stack(const Registers* registers, Allocation* output, int16_t offset);
void registers_force_register(const Registers* registers, Reference allocation, uint8_t reg, FILE* output);
void registers_override(const Registers* registers, Allocation* output, Allocation* from);

char* registers_get_mnemonic(const Registers* registers, Reference reference);
void generate_statement(Registers* registers, const char* contents, InstructionTable* table, VarList* globals,
                        FunctionList* functions, StrList* literals, FILE* output);
#endif // CODEGEN_H
