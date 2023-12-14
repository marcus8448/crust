#include "ir.h"

#include "register.h"
#include "util.h"
#include <string.h>

LIST_IMPL(Instruction, inst, Instruction);

void instructiontable_init(InstructionTable* table, int stackDepth) {
  instlist_init(&table->instructions, 8);
  ptrlist_init(&table->allocations, 4);
  table->stackDepth = stackDepth;
}

void table_allocate_arguments(InstructionTable* table, const Function* function) {
  // rdi, rsi, rdx, rcx, r8, r9 -> stack
  for (int i = 0; i < function->arguments.len; i++) {
    if (i < 6) {
      //function->arguments.array[i].type
      table_allocate_from_register(table, argumentRegisters[i]);
    } else {
      table->stackDepth += size_bytes(typekind_width(function->arguments.array[i].type.kind));
      table_allocate_from_stack(table, table->stackDepth); //fixme todo
    }
  }
}
void instructiontable_free(InstructionTable* table) {
  //todo
}

uint8_t get_param_count(InstructionType instruction) {
  return instruction < __binary ? 1 : 2;
}

Reference reference_direct(Allocation* allocation) {
  Reference reference;
  reference.access = Direct;
  reference.allocation = allocation;
  return reference;
}

Reference reference_deref(Allocation* allocation) {
  Reference reference;
  reference.access = Dereference;
  reference.allocation = allocation;
  return reference;
}

void instruction_init(Instruction* instruction) {
  instruction->type = -1;
  instruction->inputs[0].access = UNINIT;
  instruction->inputs[0].allocation = NULL;
  instruction->inputs[1].access = UNINIT;
  instruction->inputs[1].allocation = NULL;
  instruction->inputs[2].access = UNINIT;
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
  if (a.access == Dereference || a.access == Direct) a.allocation->lastInstr = table->instructions.len;
}

void instruction_binary(Instruction* instruction, const InstructionTable *table, InstructionType type, Reference a, Reference b, Allocation *output, char *comment) {
  instruction->type = type;
  instruction->inputs[0] = a;
  instruction->inputs[1] = b;
  instruction->output = output;
  instruction->comment = comment;
  output->lastInstr = table->instructions.len - 1;
  if (a.access == Dereference || a.access == Direct) a.allocation->lastInstr = table->instructions.len;
  if (b.access == Dereference || b.access == Direct) b.allocation->lastInstr = table->instructions.len;
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
  if (a.access == Dereference || a.access == Direct) a.allocation->lastInstr = table->instructions.len;
  if (b.access == Dereference || b.access == Direct) b.allocation->lastInstr = table->instructions.len;
  if (c.access == Dereference || c.access == Direct) c.allocation->lastInstr = table->instructions.len;
}

Allocation* table_allocate(InstructionTable* table) {
  void** allocation = ptrlist_grow(&table->allocations);
  Allocation *alloc = *allocation = malloc(sizeof(Allocation));

  alloc->name = NULL;
  alloc->require_memory = false;
  alloc->lvalue = false;
  alloc->index = table->allocations.len-1;
  return *allocation;
}

Allocation* table_allocate_from(InstructionTable* table, Reference ref) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Copy;
  alloc->source.reference = ref;
  alloc->source.start = table->instructions.len;
  alloc->width = ref.access == Direct || ref.access == Dereference ? ref.allocation->width : Quad; //todo
  alloc->lvalue = false;
  alloc->name = NULL;
  return alloc;
}

Allocation* table_allocate_variable(InstructionTable* table, Variable variable) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = None;
  alloc->source.start = table->instructions.len;
  alloc->width = typekind_width(variable.type.kind);
  alloc->lvalue = false;
  alloc->name = variable.name;
  return alloc;
}

Allocation* table_allocate_from_variable(InstructionTable* table, Reference ref, Variable variable) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Copy;
  alloc->source.reference = ref;
  alloc->source.start = table->instructions.len;
  alloc->width = typekind_width(variable.type.kind);
  alloc->lvalue = false;
  alloc->name = variable.name;
  return alloc;
}

Allocation* table_allocate_from_register(InstructionTable* table, int8_t reg) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Register;
  alloc->source.reg = reg;
  alloc->source.start = table->instructions.len;
  alloc->width = -1; //foixme
  alloc->lvalue = false;
  alloc->name = NULL;
  return alloc;
}

Allocation* table_allocate_from_stack(InstructionTable* table, int16_t offset) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Stack;
  alloc->source.offset = offset;
  alloc->source.start = table->instructions.len;
  alloc->width = -1; //fixme
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

Allocation* table_get_variable_by_token(InstructionTable* table, const char* contents, const Token* token) {
  for (int i = 0; i < table->allocations.len; ++i) {
    if (token_str_cmp(token, contents, ((Allocation*)table->allocations.array[i])->name) == 0) {
      return table->allocations.array[i];
    }
  }
  return NULL;
}

Reference solve_ast_node(const char* contents, InstructionTable* table, VarList* globals, FunctionList* functions,
                                StrList* literals, AstNode* node) {
  switch (node->type) {
  case op_nop:
    exit(112);
  case op_function:
    break;
  case op_array_index: {
    Reference array = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference index = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* allocation = table_allocate_from(table, array);
    instruction_binary(table_next(table), table, ADD, index, reference_direct(allocation), allocation, "index");
    return reference_deref(allocation);
  }
  case op_comma: {
    // discard left, keep right
    solve_ast_node(contents, table, globals, functions, literals, node->left);
    return solve_ast_node(contents, table, globals, functions, literals, node->right);
  }
  case op_unary_negate: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    Allocation* allocation = table_allocate_from(table, inner);
    Reference reference = reference_direct(allocation);
    instruction_unary(table_next(table), table, NEG, reference, allocation, "unary");
    return reference;
  }
  case op_unary_plus: {
    return solve_ast_node(contents, table, globals, functions, literals, node->inner); // no effect
  }
  case op_unary_addressof: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    if (inner.access == Dereference) {
      inner.access = Direct;
      return inner;
    }
    assert(inner.access == Direct);
    Allocation* allocation = table_allocate(table);
    Reference reference = reference_direct(allocation);
    instruction_binary(table_next(table), table, LEA, inner, reference, allocation, "take address");
    return reference_direct(allocation);
  }
  case op_unary_derefernce: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    switch (inner.access) {
    case Direct:
      inner.access = Dereference;
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
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    Allocation* allocation = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, TEST, inner, inner, allocation, "test not");
    Reference reference = reference_direct(allocation);
    instruction_unary(table_next(table), table, SETE, reference, allocation, "sete");
    return reference;
  }
  case op_unary_bitwise_not: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    Allocation* allocation = table_allocate_from(table, inner);
    Reference reference = reference_direct(allocation);
    instruction_unary(table_next(table), table, NOT, reference, allocation, "not");
    return reference;
  }
  case op_add: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, ADD, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_subtract: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, SUB, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_multiply: { // fixme
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
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
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, OR, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_bitwise_xor: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, XOR, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_bitwise_and: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, AND, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_assignment: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    assert(left.access == Direct || left.access == Dereference);
    instruction_binary(table_next(table), table, MOV, right, left, left.allocation, "mov unary");
    return left;
  }
  case op_and: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* lhs = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, TEST, left, left, lhs, "mov unary");
    Allocation* rhs = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, TEST, right, right, rhs, "mov unary");
    Allocation* out = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, AND, reference_direct(lhs), reference_direct(rhs), out, "mov unary");
    return reference_direct(out);
  }
  case op_or: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* lhs = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, TEST, left, left, lhs, "mov unary");
    Allocation* rhs = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, TEST, right, right, rhs, "mov unary");
    Allocation* out = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, OR, reference_direct(lhs), reference_direct(rhs), out, "mov unary");
    return reference_direct(out);
  }
  case op_deref_member_access:
    break;
  case op_member_access:
    break;
  case op_bitwise_left_shift: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, SAL, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_bitwise_right_shift: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* output = table_allocate(table);
    instruction_binary(table_next(table), table, SAR, left, right, output, "add a");
    return reference_direct(output);
  }
  case op_compare_equals: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Allocation* output = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, CMP, right, left, output, "cmp eq");

    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETE, reference, output, "==");
    return reference;
  }
  case op_compare_not_equals: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Allocation* output = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, CMP, right, left, output, "cmp eq");

    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETNE, reference, output, "!=");
    return reference;
  }
  case op_less_than: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Allocation* output = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, CMP, right, left, output, "cmp eq");

    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETL, reference, output, "<");
    return reference;
  }
  case op_greater_than: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Allocation* output = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, CMP, right, left, output, "cmp eq");

    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETG, reference, output, ">");
    return reference;
  }
  case op_less_than_equal: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Allocation* output = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, CMP, right, left, output, "cmp eq");

    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETLE, reference, output, "<=");
    return reference;
  }
  case op_greater_than_equal: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Allocation* output = table_allocate_from_register(table, al);
    instruction_binary(table_next(table), table, CMP, right, left, output, "cmp eq");

    Reference reference = reference_direct(output);
    instruction_unary(table_next(table), table, SETGE, reference, output, ">=");
    return reference;
  }
  case op_value_constant: { // todo string literals
    Reference reference;
    reference.access = ConstantI;
    reference.value = token_copy(node->token, contents);
    return reference;
  }
  case op_value_variable: {
    Reference reference;
    reference.access = Direct;
    reference.allocation = table_get_variable_by_token(table, contents, node->token);
    return reference;
  }
  case op_cast:
    break;
  };
  exit(23);
}