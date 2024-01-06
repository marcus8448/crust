#ifndef IR_H
#define IR_H
#include "ast.h"
#include "struct/list.h"
#include "types.h"

extern int nextInstrId;

struct Allocation;

typedef enum {
  None = 1,
  FnArgument,
  ForceStack,
  ForceRegister
} AllocationProperty;

typedef struct {
  AllocationProperty prop;
  union {
    struct {
      int8_t reg;
      int16_t offset;
    };
  };
} AllocationSource;

typedef enum {
  UNINIT = 1,
  Direct,
  Dereference,
  ConstantI,
  ConstantS,
  GlobalRef,
  Global
} AccessType;

bool isAllocated(AccessType type);

typedef struct Reference {
  AccessType access;

  union {
    struct Allocation *allocation;
    char *value;
    int str;
  };
} Reference;

typedef struct Allocation {
  int index;
  Type type;
  AllocationSource source;
  const char *name; // NULLABLE
  bool lvalue;
  int lastInstr;
} Allocation;

typedef enum {
  LABEL,

  NEG,

  SETE,
  SETL,
  SETG,
  SETNE,
  SETLE,
  SETGE,

  JMP,
  JE,
  JNE,
  JG,
  JL,
  JGE,
  JLE,

  CALL,
  RET,

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

LIST_API(Instruction, inst, struct Instruction)

typedef struct InstructionTable {
  const struct InstructionTable *parent;
  InstructionList instructions;
  PtrList allocations;
  int *sections;
  int parentCutoff;
  char *name;
  int escape;
} InstructionTable;

typedef struct Instruction {
  InstructionType type;
  int id;
  union {
    struct {
      Reference inputs[2];
      Reference output;
      char *comment;
    };
    struct {
      int label;
      union {
        InstructionTable instructions;
        int escape;
        bool processed;
      };
    };
    struct {
      Function *function;
      Reference retVal;
      Reference* arguments;
    };
  };
} Instruction;

void instructiontable_init(InstructionTable *table, char *name);
void instructiontable_child(InstructionTable *table, const InstructionTable *parent, int escape);

void table_allocate_arguments(InstructionTable *table, const Function *function);
void instructiontable_free(InstructionTable *table);

Reference reference_direct(Allocation *allocation);
Reference reference_deref(Allocation *allocation);

void instruction_init(Instruction *instruction);

Allocation *table_get_variable_by_token(const InstructionTable *table, const char *contents, const Token *token);

Allocation *table_allocate(InstructionTable *table, Type type);
Allocation *table_allocate_infer_type(InstructionTable *table, Reference a);
Allocation *table_allocate_infer_types(InstructionTable *table, Reference a, Reference b);
Allocation *table_allocate_variable(InstructionTable *table, Variable variable);
Allocation *table_allocate_register(InstructionTable *table, Type type);
Allocation *table_allocate_stack(InstructionTable *table, Type type);

Instruction *table_next(InstructionTable *table);

Type ref_infer_type(Reference a, Reference b);

typedef struct {
  Width size;
  InstructionType operation;
  Allocation *values[3];
} Statement;

void statement_init(Statement *statement);

Reference solve_ast_node(const char *contents, InstructionTable *table, VarList *globals, FunctionList *functions,
                         StrList *literals, AstNode *node);

#endif // IR_H
