#include "ir.h"

#include "register.h"

#include <stddef.h>
#include <stdio.h>

// void allocate_arguments(AllocationTable* table, const Function* function) {
//   // rdi, rsi, rdx, rcx, r8, r9 -> stack
//   for (int i = 0; i < function->arguments.len; i++) {
//     Allocation* ref = allocationlist_grow(&table->allocations);
//     if (i < 6) {
//       allocation_init_from_reg(ref, function->arguments.array[i].type, argumentRegisters[i]);
//     } else {
//       table->stackDepth += size_bytes(typekind_width(function->arguments.array[i].type.kind));
//       allocation_init_from_stack(ref, function->arguments.array[i].type, table->stackDepth);
//     }
//   }
// }
//
// Allocation* table_allocate(AllocationTable* table) {
//   return allocationlist_grow(&table->allocations);
// }
//
// Instruction* table_next(AllocationTable* table) {
//   return instlist_grow(&table->instructions);
// }
//
// void statement_init(Statement* statement) {
//   statement->values[0] = NULL;
//   statement->values[1] = NULL;
//   statement->values[2] = NULL;
// }
