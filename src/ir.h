#ifndef IR_H
#define IR_H
#include "ast.h"
#include "struct/list.h"
#include "types.h"

#include <stdint.h>

struct Allocation;

typedef enum {
  None,
  Stack,
  Register,
  Copy
} InitialLocationType;

typedef enum {
  UNINIT,
  Direct,
  Dereference,
  ConstantI,
  ConstantS
} ReferenceType;

typedef struct Reference {
  ReferenceType type;
  union {
    struct Allocation* allocation;
    char* value;
    int str;
  };
} Reference;

typedef struct {
  InitialLocationType location;
  union {
    int8_t reg;
    int16_t offset;
    Reference reference;
  };
} InitialLocation;

typedef struct Allocation {
  Width width;
  InitialLocation source;
  const char *name; //NULLABLE
  bool lvalue;
  bool require_memory;
  int lastInstr;
} Allocation;

LIST_API(Allocation, allocation, Allocation);

typedef enum {
  // unary
  NEG,

  SETE,
  SETL,
  SETG,
  SETNE,
  SETLE,
  SETGE,

  PUSH,
  POP,

  //binary
  MOV,
  LEA,

  ADD,
  SUB,
  IMUL,
  OR,
  XOR,
  AND,
  NOT,
  SAL,
  SAR,
  CMP,

  TEST,

  //non-instr
  DEREF,

  CALL
} InstructionType;

typedef struct {
  InstructionType type;
  Reference inputs[3];
  Allocation *output;
  char* comment;
} Instruction;

LIST_API(Instruction, inst, Instruction);

typedef struct {
  InstructionList instructions;
  AllocationList allocations;
  int stackDepth;
} InstructionTable;

void allocation_init_from(Allocation *alloc, Type type, Reference from);
void allocation_init_from_reg(Allocation *alloc, Type type, int8_t reg);
void allocation_init_from_stack(Allocation *alloc, Type type, int16_t offset);
void allocation_init(Allocation *alloc, Type type);

void allocate_arguments(InstructionTable* table, const Function* function);

Reference reference_direct(Allocation* allocation);
Reference reference_deref(Allocation* allocation);

void instruction_init(Instruction* instruction);

void instruction_unary(Instruction* instruction, const InstructionTable *table, InstructionType type, Reference a, Allocation *output, char *comment);
void instruction_binary(Instruction* instruction, const InstructionTable *table, InstructionType type, Reference a, Reference b, Allocation *output, char *comment);
void instruction_ternary(Instruction* instruction, const InstructionTable *table, InstructionType type, Reference a, Reference b, Reference c, Allocation *output, char *comment);

Allocation* table_get_variable_by_token(InstructionTable* table, const char* contents, const Token* token);

Allocation* table_allocate(InstructionTable* table);
Allocation* table_allocate_from(InstructionTable* table, Reference ref);
Allocation* table_allocate_from_register(InstructionTable* table, int8_t reg);
Allocation* table_allocate_from_stack(InstructionTable* table, int16_t offset);

Instruction* table_next(InstructionTable* table);

typedef struct {
  Width size;
  InstructionType operation;
  Allocation* values[3];
} Statement;

void statement_init(Statement* statement);
#endif // IR_H
