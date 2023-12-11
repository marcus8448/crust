#include "ir.h"

#include "register.h"
#include "util.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

LIST_IMPL(Allocation, allocation, Allocation);
LIST_IMPL(Instruction, inst, Instruction);

void allocation_init_from(Allocation* alloc, Type type, Reference from) {
  alloc->width = typekind_width(type.kind); // fixme
  alloc->source.location = Copy;
  alloc->source.reference = from;
}

void allocation_init_from_reg(Allocation* alloc, Type type, int8_t reg) {
  alloc->width = typekind_width(type.kind); // fixme
  alloc->source.location = Register;
  alloc->source.reg = reg;
}

void allocation_init_from_stack(Allocation* alloc, Type type, int16_t offset) {
  alloc->width = typekind_width(type.kind); // fixme
  alloc->source.location = Stack;
  alloc->source.offset = offset;
}

void allocation_init(Allocation* alloc, Type type) {
  alloc->width = typekind_width(type.kind); // fixme
  alloc->source.location = None;
}

void allocate_arguments(InstructionTable* table, const Function* function) {
  // rdi, rsi, rdx, rcx, r8, r9 -> stack
  for (int i = 0; i < function->arguments.len; i++) {
    Allocation* ref = allocationlist_grow(&table->allocations);
    if (i < 6) {
      allocation_init_from_reg(ref, function->arguments.array[i].type, argumentRegisters[i]);
    } else {
      table->stackDepth += size_bytes(typekind_width(function->arguments.array[i].type.kind));
      allocation_init_from_stack(ref, function->arguments.array[i].type, table->stackDepth);
    }
  }
}

Reference reference_direct(Allocation* allocation) {
  Reference reference;
  reference.type = Direct;
  reference.allocation = allocation;
  return reference;
}

Reference reference_deref(Allocation* allocation) {
  Reference reference;
  reference.type = Dereference;
  reference.allocation = allocation;
  return reference;
}

void instruction_init(Instruction* instruction) {
  instruction->type = -1;
  instruction->inputs[0].type = UNINIT;
  instruction->inputs[0].allocation = NULL;
  instruction->inputs[1].type = UNINIT;
  instruction->inputs[1].allocation = NULL;
  instruction->inputs[2].type = UNINIT;
  instruction->inputs[2].allocation = NULL;
  instruction->output = NULL;
  instruction->comment = NULL;
}

void instruction_unary(Instruction* instruction, const InstructionTable *table, InstructionType type, Reference a, Allocation *output, char *comment) {
  instruction->type = type;
  instruction->inputs[0] = a;
  instruction->output = output;
  instruction->comment = comment;
  output->lastInstr = table->instructions.len - 1;
  if (a.type == Dereference || a.type == Direct) a.allocation->lastInstr = table->instructions.len - 1;
}

void instruction_binary(Instruction* instruction, const InstructionTable *table, InstructionType type, Reference a, Reference b, Allocation *output, char *comment) {
  instruction->type = type;
  instruction->inputs[0] = a;
  instruction->inputs[1] = b;
  instruction->output = output;
  instruction->comment = comment;
  output->lastInstr = table->instructions.len - 1;
  if (a.type == Dereference || a.type == Direct) a.allocation->lastInstr = table->instructions.len - 1;
  if (b.type == Dereference || b.type == Direct) b.allocation->lastInstr = table->instructions.len - 1;
}

void instruction_ternary(Instruction* instruction, const InstructionTable *table, InstructionType type, Reference a, Reference b, Reference c,
                         Allocation* output, char* comment) {
  instruction->type = type;
  instruction->inputs[0] = a;
  instruction->inputs[1] = b;
  instruction->inputs[2] = c;
  instruction->output = output;
  instruction->comment = comment;
  output->lastInstr = table->instructions.len - 1;
  if (a.type == Dereference || a.type == Direct) a.allocation->lastInstr = table->instructions.len - 1;
  if (b.type == Dereference || b.type == Direct) b.allocation->lastInstr = table->instructions.len - 1;
  if (c.type == Dereference || c.type == Direct) c.allocation->lastInstr = table->instructions.len - 1;
}

Allocation* table_allocate(InstructionTable* table) {
  return allocationlist_grow(&table->allocations);
}

Allocation* table_allocate_from(InstructionTable* table, Reference ref) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Copy;
  alloc->source.reference = ref;
  // alloc->width = ref->width;
  alloc->lvalue = false;
  alloc->name = NULL;
  return alloc;
}

Allocation* table_allocate_from_register(InstructionTable* table, int8_t reg) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Register;
  alloc->source.reg = reg;
  alloc->width = -1;
  alloc->lvalue = false;
  alloc->name = NULL;
  return alloc;
}

Allocation* table_allocate_from_stack(InstructionTable* table, int16_t offset) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Stack;
  alloc->source.offset = offset;
  alloc->width = -1;
  alloc->lvalue = false;
  alloc->name = NULL;
  return alloc;
}

Instruction* table_next(InstructionTable* table) {
  Instruction* instruction = instlist_grow(&table->instructions);
  instruction_init(instruction);
  return instruction;
}

void statement_init(Statement* statement) {
  statement->values[0] = NULL;
  statement->values[1] = NULL;
  statement->values[2] = NULL;
}

Reference solve_ast_node(const char* contents, InstructionTable* table, VarList* globals,
                                FunctionList* functions, StrList* literals, AstNode* node, FILE* file);

Allocation* table_get_variable_by_token(InstructionTable* table, const char* contents, const Token* token) {
  for (int i = 0; i < table->allocations.len; ++i) {
    if (token_str_cmp(token, contents, table->allocations.array[i].name)) {
      return table->allocations.array + i;
    }
  }
  return NULL;
}

Reference solve_ast_node(const char* contents, InstructionTable* table, VarList* globals, FunctionList* functions,
                                StrList* literals, AstNode* node, FILE* file) {
  switch (node->type) {
  case op_nop:
    exit(112);
  case op_function:
    break;
  case op_array_index: {
    Reference array = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference index = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    //todo
    return array;
  }
  case op_comma: {
    // discard left, keep right
    solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    return solve_ast_node(contents, table, globals, functions, literals, node->right, file);
  }
  case op_unary_negate: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner, file);
    Allocation* allocation = table_allocate_from(table, inner);
    Reference reference = reference_direct(allocation);
    instruction_unary(table_next(table), table, NEG, reference, allocation, "unary");
    return reference;
  }
  case op_unary_plus: {
    return solve_ast_node(contents, table, globals, functions, literals, node->inner, file); // no effect
  }
  case op_unary_addressof: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner, file);
    if (inner.type == Dereference) {
      inner.type = Direct;
      return inner;
    }
    assert(inner.type == Direct);
    Allocation* allocation = table_allocate(table);
    Reference reference = reference_direct(allocation);
    instruction_binary(table_next(table), table, LEA, inner, reference, allocation, "take address");
    return reference_direct(allocation);
  }
  case op_unary_derefernce: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner, file);
    switch (inner.type) {
    case Direct:
      inner.type = Dereference;
      return inner;
    case Dereference:
      Allocation* allocation = table_allocate_from(table, inner);
      return reference_deref(allocation);
      // case ConstantS:
      //   break;
    default:
      assert(false);
    }
  }
  case op_unary_not: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner, file);
    instruction_binary(table_next(table), table, TEST, inner, inner, NULL, "test not");
    Allocation* allocation = table_allocate_from_register(table, al);
    Reference reference = reference_direct(allocation);
    instruction_unary(table_next(table), table, SETE, reference, allocation, "sete");
    return reference;
  }
  case op_unary_bitwise_not: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner, file);
    Allocation* allocation = table_allocate_from(table, inner);
    Reference reference = reference_direct(allocation);
    instruction_unary(table_next(table), table, NOT, reference, allocation, "not");
    return reference;
  }
  case op_add: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, ADD, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_subtract: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, SUB, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_multiply: { // fixme
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, IMUL, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_divide:
    // fixme div is not simple
      break;
  case op_modulo:
    // fixme div is not simple
      break;
  case op_bitwise_or: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, OR, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_bitwise_xor: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, XOR, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_bitwise_and: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, ADD, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_assignment: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    assert(left.type == Direct || left.type == Dereference);
    instruction_unary(table_next(table), table, MOV, right, left.allocation, "mov unary");
    return left;
  }
  case op_and: {
    break;
  }
  case op_or: {
    break;
  }
  case op_deref_member_access:
    break;
  case op_member_access:
    break;
  case op_bitwise_left_shift: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, SAL, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_bitwise_right_shift: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, SAR, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_compare_equals: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);

    instruction_binary(table_next(table), table, CMP, right, left, NULL, "cmp eq");

    Allocation* output = table_allocate_from_register(table, al);
    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETE, reference, output, "==");
    return reference;
  }
  case op_compare_not_equals: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);

    instruction_binary(table_next(table), table, CMP, right, left, NULL, "cmp eq");

    Allocation* output = table_allocate_from_register(table, al);
    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETNE, reference, output, "!=");
    return reference;
  }
  case op_less_than: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);

    instruction_binary(table_next(table), table, CMP, right, left, NULL, "cmp eq");

    Allocation* output = table_allocate_from_register(table, al);
    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETL, reference, output, "<");
    return reference;
  }
  case op_greater_than: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);

    instruction_binary(table_next(table), table, CMP, right, left, NULL, "cmp eq");

    Allocation* output = table_allocate_from_register(table, al);
    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETG, reference, output, ">");
    return reference;
  }
  case op_less_than_equal: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);

    instruction_binary(table_next(table), table, CMP, right, left, NULL, "cmp eq");

    Allocation* output = table_allocate_from_register(table, al);
    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETLE, reference, output, "<=");
    return reference;
  }
  case op_greater_than_equal: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left, file);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right, file);

    instruction_binary(table_next(table), table, CMP, right, left, NULL, "cmp eq");

    Allocation* output = table_allocate_from_register(table, al);
    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETGE, reference, output, ">=");
    return reference;
  }
  case op_value_constant: { // todo string literals
    Reference reference;
    reference.type = ConstantI;
    reference.value = token_copy(node->token, contents);
    return reference;
  }
  case op_value_variable: {
    Reference reference;
    reference.type = Direct;
    reference.allocation = table_get_variable_by_token(table, contents, node->token);
    return reference;
  }
  case op_cast:
    break;
  };
  exit(23);
}