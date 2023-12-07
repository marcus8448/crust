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
  Register
} InitialLocationType;

typedef struct {
  InitialLocationType location;
  union {
    int8_t reg;
    int16_t offset;
    struct Allocation *allocation;
  };
} InitialLocation;

typedef struct Allocation {
  int id;
  Width width;
  InitialLocation initial;
  const char *name; //NULLABLE
} Allocation;

LIST_API(Allocation, allocation, Allocation);

typedef enum { MOV, PUSH, POP, SUB, AND, OR, CALL } InstructionType;

typedef struct {
  InstructionType type;
  Allocation *inputs[3];
} Instruction;

LIST_API(Instruction, inst, Instruction);

typedef struct {
  InstructionList instructions;
  AllocationList allocations;
  int stackDepth;
} AllocationTable;

void allocation_init_from_reg(Allocation *alloc, Type type, int8_t reg);
void allocation_init_from_stack(Allocation *alloc, Type type, int16_t offset);
void allocation_any(Allocation *alloc, Type type, int16_t offset);

void allocate_arguments(AllocationTable* table, const Function* function);

Allocation* table_allocate(AllocationTable* table);
Instruction* table_next(AllocationTable* table);


typedef struct {
  Width size;
  InstructionType operation;
  Allocation* values[3];
} Statement;

void statement_init(Statement* statement);
#endif // IR_H
