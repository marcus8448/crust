#ifndef IR_H
#define IR_H
#include "ast.h"
#include "struct/list.h"
#include "types.h"

#include <stdint.h>

struct Allocation;

typedef enum { None, Stack, Register, Copy } InitialLocationType;

typedef enum { UNINIT, Direct, Dereference, ConstantI, ConstantS } AccessType;

bool isAllocated(AccessType type);

typedef struct Reference {
  AccessType access;

  union {
    struct Allocation* allocation;
    char* value;
    int str;
  };
} Reference;

typedef struct {
  InitialLocationType location;
  int start;
  union {
    int8_t reg;
    int16_t offset;
    Reference reference;
  };
} InitialLocation;

typedef struct Allocation {
  int index;
  Type type;
  InitialLocation source;
  const char* name; // NULLABLE
  bool lvalue;
  bool require_memory;
  int lastInstr;
} Allocation;

typedef enum {
  __unary,
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

  CALL,

  __binary,
  // binary
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

  TEST
} InstructionType;

typedef struct {
  InstructionType type;
  Reference inputs[3];
  Allocation* output;
  char* comment;
} Instruction;

LIST_API(Instruction, inst, Instruction)

typedef struct {
  InstructionList instructions;
  PtrList allocations;
  int stackDepth;
} InstructionTable;

void instructiontable_init(InstructionTable* table, int stackDepth);

void table_allocate_arguments(InstructionTable* table, const Function* function);
void instructiontable_free(InstructionTable* table);

uint8_t get_param_count(InstructionType instruction);

Reference reference_direct(Allocation* allocation);
Reference reference_deref(Allocation* allocation);

void instruction_init(Instruction* instruction);

void instruction_unary(Instruction* instruction, const InstructionTable* table, InstructionType type, Reference a,
                       Allocation* output, char* comment);
void instruction_binary(Instruction* instruction, const InstructionTable* table, InstructionType type, Reference a,
                        Reference b, Allocation* output, char* comment);
void instruction_ternary(Instruction* instruction, const InstructionTable* table, InstructionType type, Reference a,
                         Reference b, Reference c, Allocation* output, char* comment);

Allocation* table_get_variable_by_token(InstructionTable* table, const char* contents, const Token* token);

Allocation* table_allocate(InstructionTable* table);
Allocation* table_allocate_infer_type(InstructionTable* table, Reference a, Reference b);
Allocation* table_allocate_variable(InstructionTable* table, Variable variable);
Allocation* table_allocate_from_variable(InstructionTable* table, Reference ref, Variable variable);
Allocation* table_allocate_from(InstructionTable* table, Reference ref);
Allocation* table_allocate_from_register(InstructionTable* table, int8_t reg, Type type);
Allocation* table_allocate_from_stack(InstructionTable* table, int16_t offset, Type type);

Instruction* table_next(InstructionTable* table);

typedef struct {
  Width size;
  InstructionType operation;
  Allocation* values[3];
} Statement;

void statement_init(Statement* statement);

Reference solve_ast_node(const char* contents, InstructionTable* table, VarList* globals, FunctionList* functions,
                         StrList* literals, AstNode* node);

#endif // IR_H
