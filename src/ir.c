#include "ir.h"

#include "register.h"
#include "util.h"
#include <string.h>

LIST_IMPL(Instruction, inst, Instruction)

bool isAllocated(const AccessType type) {
  return type == Direct || type == Dereference;
}

void instructiontable_init(InstructionTable* table, int stackDepth) {
  instlist_init(&table->instructions, 8);
  ptrlist_init(&table->allocations, 4);
  table->stackDepth = stackDepth;
}

void table_allocate_arguments(InstructionTable* table, const Function* function) {
  // rdi, rsi, rdx, rcx, r8, r9 -> stack
  for (int i = 0; i < function->arguments.len; i++) {
    if (i < 6) {
      table_allocate_from_register(table, argumentRegisters[i], function->arguments.array[i].type);
    } else {
      table->stackDepth += size_bytes(typekind_width(function->arguments.array[i].type.kind));
      table_allocate_from_stack(table, table->stackDepth, function->arguments.array[i].type); // fixme todo
    }
  }
}
void instructiontable_free(InstructionTable* table) {
  // todo
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
  instruction->output.access = UNINIT;
  instruction->output.allocation = NULL;
  instruction->comment = NULL;
}

void instruction_binary(Instruction* instruction, const InstructionTable* table, InstructionType type, Reference a,
                       Reference output, char* comment) {
  assert(isAllocated(output.access));
  instruction->type = type;
  instruction->inputs[0] = a;
  instruction->output = output;
  instruction->comment = comment;
  output.allocation->lastInstr = table->instructions.len - 1;
  if (isAllocated(a.access))
    a.allocation->lastInstr = table->instructions.len - 1;
}

void instruction_unary(Instruction* instruction, const InstructionTable* table, InstructionType type,
                       const Reference ref, char* comment) {
  assert(isAllocated(ref.access));
  instruction->type = type;
  instruction->inputs[0] = ref;
  instruction->output = ref;
  instruction->comment = comment;
  ref.allocation->lastInstr = table->instructions.len - 1;
}

void instruction_ternary(Instruction* instruction, const InstructionTable* table, InstructionType type, Reference a,
                       Reference b, Reference output, char* comment) {
  assert(isAllocated(output.access));
  instruction->type = type;
  instruction->inputs[0] = a;
  instruction->inputs[1] = b;
  instruction->output = output;
  instruction->comment = comment;
  output.allocation->lastInstr = table->instructions.len - 1;
  if (isAllocated(a.access))
    a.allocation->lastInstr = table->instructions.len - 1;
  if (isAllocated(b.access))
    b.allocation->lastInstr = table->instructions.len - 1;
}

Allocation* table_allocate(InstructionTable* table) {
  void** allocation = ptrlist_grow(&table->allocations);
  Allocation* alloc = *allocation = malloc(sizeof(Allocation));

  alloc->name = NULL;
  alloc->type.kind = -1;
  alloc->type.inner = NULL;
  alloc->lvalue = false;
  alloc->index = table->allocations.len - 1;
  return *allocation;
}

Allocation* table_allocate_infer_type(InstructionTable* table, Reference a, Reference b) {
  void** allocation = ptrlist_grow(&table->allocations);
  Allocation* alloc = *allocation = malloc(sizeof(Allocation));

  alloc->name = NULL;
  if (isAllocated(a.access)) {
    if (isAllocated(b.access)) {
      if (a.allocation->name != NULL) {
        if (a.access == Dereference) {
          alloc->type = *a.allocation->type.inner;
        } else {
          alloc->type = a.allocation->type;
        }
      } else if (b.allocation->name != NULL) {
        if (b.access == Dereference) {
          alloc->type = *b.allocation->type.inner;
        } else {
          alloc->type = b.allocation->type;
        }
      } else {
        if (a.access == Dereference) {
          alloc->type = *a.allocation->type.inner;
        } else {
          alloc->type = a.allocation->type;
        }
      }
    } else {
      if (a.access == Dereference) {
        alloc->type = *a.allocation->type.inner;
      } else {
        alloc->type = a.allocation->type;
      }
    }
  } else if (isAllocated(b.access)) {
    if (b.access == Dereference) {
      alloc->type = *b.allocation->type.inner;
    } else {
      alloc->type = b.allocation->type;
    }
  } else {
    alloc->type.kind = Quad;
    alloc->type.inner = NULL;
    puts("warn: alloc type guess");
  }

  alloc->lvalue = false;
  alloc->index = table->allocations.len - 1;
  return *allocation;
}

Allocation* table_allocate_from(InstructionTable* table, Reference ref) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Copy;
  alloc->source.reference = ref;
  alloc->source.start = table->instructions.len - 1;
  if (isAllocated(ref.access)) {
    alloc->type = ref.allocation->type;
  } else {
    alloc->type.kind = i64;
    alloc->type.inner = NULL;
  }
  alloc->lvalue = false;
  alloc->name = NULL;
  return alloc;
}

Allocation* table_allocate_variable(InstructionTable* table, Variable variable) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = None;
  alloc->source.start = table->instructions.len - 1;
  alloc->type = variable.type;
  alloc->lvalue = false;
  alloc->name = variable.name;
  return alloc;
}

Allocation* table_allocate_from_variable(InstructionTable* table, Reference ref, Variable variable) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Copy;
  alloc->source.reference = ref;
  alloc->source.start = table->instructions.len - 1;
  alloc->type = variable.type;
  alloc->lvalue = false;
  alloc->name = variable.name;
  return alloc;
}

Allocation* table_allocate_from_register(InstructionTable* table, uint8_t reg, Type type) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Register;
  alloc->source.reg = reg;
  alloc->source.start = table->instructions.len - 1;
  alloc->type = type;
  alloc->lvalue = false;
  alloc->name = NULL;
  return alloc;
}

Allocation* table_allocate_from_stack(InstructionTable* table, int16_t offset, Type type) {
  Allocation* alloc = table_allocate(table);
  alloc->source.location = Stack;
  alloc->source.offset = offset;
  alloc->source.start = table->instructions.len - 1;
  alloc->type = type;
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

Allocation* table_get_variable_by_token(const InstructionTable* table, const char* contents, const Token* token) {
  for (int i = 0; i < table->allocations.len; ++i) {
    if (token_str_cmp(token, contents, ((Allocation*)table->allocations.array[i])->name) == 0) {
      return table->allocations.array[i];
    }
  }
  return NULL;
}

Reference solve_ast_node(const char* contents, InstructionTable* table, VarList* globals, FunctionList* functions,
                         StrList* literals, AstNode* node) {
  Type u8type = {.kind = u8, .inner = NULL};
  switch (node->type) {
  case op_nop:
    exit(112);
  case op_function:
    break;
  case op_array_index: {
    Reference array = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference index = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Allocation* output = table_allocate_infer_type(table, array, index);
    instruction_ternary(table_next(table), table, ADD, array, index, reference_direct(output), "index");
    return reference_deref(output);
  }
  case op_comma: {
    // discard left, keep right
    solve_ast_node(contents, table, globals, functions, literals, node->left);
    return solve_ast_node(contents, table, globals, functions, literals, node->right);
  }
  case op_unary_negate: {
    Reference reference = reference_direct(table_allocate_from(table, solve_ast_node(contents, table, globals, functions, literals, node->inner)));
    instruction_unary(table_next(table), table, NEG, reference, "negate");
    return reference;
  }
  case op_unary_plus: {
    return solve_ast_node(contents, table, globals, functions, literals, node->inner); // no effect
  }
  case op_unary_addressof: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    assert(isAllocated(inner.access));

    if (inner.access == Dereference) {
      inner.access = Direct;
      return inner;
    }

    Allocation* output = table_allocate(table);
    output->type.kind = ptr;
    output->type.inner = &inner.allocation->type;
    Reference reference = reference_direct(output);
    instruction_binary(table_next(table), table, LEA, inner, reference, "take address");
    return reference;
  }
  case op_unary_derefernce: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    switch (inner.access) {
    case Direct:
      inner.access = Dereference;
      return inner;
    case Dereference: {
      Allocation* allocation = table_allocate_from(table, inner);
      return reference_deref(allocation);
    }
      // case ConstantS:
      //   break;
    default:
      assert(false);
    }
  } break;
  case op_unary_not: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    Allocation* allocation = table_allocate_from_register(table, al, u8type);
    Reference reference = reference_direct(allocation);
    instruction_ternary(table_next(table), table, TEST, inner, inner, reference, "test not");
    instruction_unary(table_next(table), table, SETE, reference, "sete");
    return reference;
  }
  case op_unary_bitwise_not: {
    Reference reference = reference_direct(table_allocate_from(table, solve_ast_node(contents, table, globals, functions, literals, node->inner)));
    instruction_unary(table_next(table), table, NOT, reference, "not");
    return reference;
  }
  case op_add: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference output = reference_direct(table_allocate_infer_type(table, left, right));
    instruction_ternary(table_next(table), table, ADD, left, right, output, "add a");
    return output;
  }
  case op_subtract: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference output = reference_direct(table_allocate_infer_type(table, left, right));
    instruction_ternary(table_next(table), table, SUB, left, right, output, "add a");
    return output;
  }
  case op_multiply: { // fixme
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference output = reference_direct(table_allocate_infer_type(table, left, right));
    instruction_ternary(table_next(table), table, IMUL, left, right, output, "add a");
    return output;
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
    Reference output = reference_direct(table_allocate_infer_type(table, left, right));
    instruction_ternary(table_next(table), table, OR, left, right, output, "add a");
    return output;
  }
  case op_bitwise_xor: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference output = reference_direct(table_allocate_infer_type(table, left, right));
    instruction_ternary(table_next(table), table, XOR, left, right, output, "add a");
    return output;
  }
  case op_bitwise_and: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference output = reference_direct(table_allocate_infer_type(table, left, right));
    instruction_ternary(table_next(table), table, AND, left, right, output, "add a");
    return output;
  }
  case op_assignment: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    assert(isAllocated(left.access));
    instruction_binary(table_next(table), table, MOV, right, left, "mov unary");
    return left;
  }
  case op_and: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference lhs = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, TEST, left, left, lhs, "mov unary");
    Reference rhs = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, TEST, right, right, rhs, "mov unary");
    Reference out = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, AND, lhs, rhs, out, "mov unary");
    return out;
  }
  case op_or: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference lhs = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, TEST, left, left, lhs, "mov unary");
    Reference rhs = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, TEST, right, right, rhs, "mov unary");
    Reference out = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, OR, lhs, rhs, out, "mov unary");
    return out;
  }
  case op_deref_member_access:
    break;
  case op_member_access:
    break;
  case op_bitwise_left_shift: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference output = reference_direct(table_allocate_infer_type(table, left, right));
    instruction_ternary(table_next(table), table, SAL, left, right, output, "add a");
    return output;
  }
  case op_bitwise_right_shift: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    Reference output = reference_direct(table_allocate_infer_type(table, left, right));
    instruction_ternary(table_next(table), table, SAR, left, right, output, "add a");
    return output;
  }
  case op_compare_equals: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Reference output = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, CMP, right, left, output, "cmp eq");
    instruction_unary(table_next(table), table, SETE, output, "==");
    return output;
  }
  case op_compare_not_equals: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Reference output = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, CMP, right, left, output, "cmp eq");
    instruction_unary(table_next(table), table, SETNE, output, "!=");
    return output;
  }
  case op_less_than: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Reference output = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, CMP, right, left, output, "cmp eq");
    instruction_unary(table_next(table), table, SETE, output, "<");
    return output;
  }
  case op_greater_than: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Reference output = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, CMP, right, left, output, "cmp eq");
    instruction_unary(table_next(table), table, SETE, output, ">");
    return output;
  }
  case op_less_than_equal: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Reference output = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, CMP, right, left, output, "cmp eq");
    instruction_unary(table_next(table), table, SETE, output, "<=");
    return output;
  }
  case op_greater_than_equal: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);

    Reference output = reference_direct(table_allocate_from_register(table, al, u8type));
    instruction_ternary(table_next(table), table, CMP, right, left, output, "cmp eq");
    instruction_unary(table_next(table), table, SETE, output, ">=");
    return output;
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
  case op_value_let:
    return reference_direct(table_allocate_variable(table, node->variable));
  case cf_if:
    break;
  case cf_while:
    break;
  case cf_return: {
    Reference value = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    instruction_unary(table_next(table), table, RET, value, "ret");
    return value;
  }
  }
  exit(23);
}
