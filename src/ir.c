#include "ir.h"

#include "util.h"
#include <string.h>

LIST_IMPL(Instruction, inst, Instruction)

bool isAllocated(const AccessType type) {
  return type == Direct || type == Dereference;
}

void instructiontable_init(InstructionTable *table) {
  instlist_init(&table->instructions, 8);
  ptrlist_init(&table->allocations, 4);
  table->nextIId = 0;
}

void table_allocate_arguments(InstructionTable *table, const Function *function) {
  // rdi, rsi, rdx, rcx, r8, r9 -> stack
  for (int i = 0; i < function->arguments.len; i++) {
    Allocation *allocation = table_allocate(table, function->arguments.array[i].type);
    allocation->source = FnArgument;
  }
}

void instructiontable_free(InstructionTable *table) {
  // todo
}

Reference reference_direct(Allocation *allocation) {
  Reference reference;
  reference.access = Direct;
  reference.allocation = allocation;
  return reference;
}

Reference reference_deref(Allocation *allocation) {
  Reference reference;
  reference.access = Dereference;
  reference.allocation = allocation;
  return reference;
}

void instruction_init(Instruction *instruction) {
  instruction->type = -1;
  instruction->inputs[0].access = UNINIT;
  instruction->inputs[0].allocation = NULL;
  instruction->inputs[1].access = UNINIT;
  instruction->inputs[1].allocation = NULL;
  instruction->output.access = UNINIT;
  instruction->output.allocation = NULL;
  instruction->comment = NULL;
}

Allocation *table_allocate(InstructionTable *table, Type type) {
  void **allocation = ptrlist_grow(&table->allocations);
  Allocation *alloc = *allocation = malloc(sizeof(Allocation));

  alloc->name = NULL;
  alloc->type = type;
  alloc->lvalue = false;
  alloc->index = table->allocations.len - 1;
  alloc->source = None;
  return *allocation;
}

Allocation *table_allocate_infer_types(InstructionTable *table, Reference a, Reference b) {
  void **allocation = ptrlist_grow(&table->allocations);
  Allocation *alloc = *allocation = malloc(sizeof(Allocation));

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
    alloc->type.kind = u64;
    alloc->type.inner = NULL;
    puts("warn: alloc type guess");
  }

  alloc->index = table->allocations.len - 1;
  return *allocation;
}

Allocation *table_allocate_infer_type(InstructionTable *table, Reference ref) {
  Type type = {.kind = i64, .inner = NULL};
  if (isAllocated(ref.access)) {
    type = ref.allocation->type;
  }
  return table_allocate(table, type);
}

Allocation *table_allocate_variable(InstructionTable *table, Variable variable) {
  Allocation *alloc = table_allocate(table, variable.type);
  alloc->name = variable.name;
  return alloc;
}

Allocation *table_allocate_register(InstructionTable *table, Type type) {
  Allocation *alloc = table_allocate(table, type);
  alloc->source = ForceRegister;
  return alloc;
}

Allocation *table_allocate_stack(InstructionTable *table, Type type) {
  Allocation *alloc = table_allocate(table, type);
  alloc->source = ForceStack;
  return alloc;
}

Instruction *table_next(InstructionTable *table) {
  Instruction *instruction = instlist_grow(&table->instructions);
  instruction_init(instruction);
  instruction->id = table->nextIId++;
  return instruction;
}

void update_reference(const Instruction *instruction, const Reference reference) {
  if (isAllocated(reference.access)) {
    reference.allocation->lastInstr = instruction->id;
  }
}

Reference instruction_mov(InstructionTable *table, const Reference from, Reference to, char *comment) {
  assert(isAllocated(to.access));
  if (to.allocation->name != NULL) {
    to.allocation = table_allocate_variable(table, (Variable){.name = to.allocation->name, .type = to.allocation->type});
  }

  Instruction *instruction = table_next(table);
  instruction->type = MOV;
  instruction->inputs[0] = from;
  instruction->output = to;
  instruction->comment = comment;

  update_reference(instruction, from);
  // update_reference(instruction, to);
  return to;
}

Reference instruction_lea(InstructionTable *table, const Reference from, const Reference to, char *comment) {
  assert(isAllocated(to.access));

  Instruction *instruction = table_next(table);
  instruction->type = LEA;
  instruction->inputs[0] = from;
  instruction->output = to;
  instruction->comment = comment;

  update_reference(instruction, from);
  // update_reference(instruction, to);
  return to;
}

Reference instruction_basic_op(InstructionTable *table, const InstructionType type, const Reference a, const Reference b, char *comment) {
  const Reference output = instruction_mov(table, a, reference_direct(table_allocate_infer_types(table, a, b)), NULL);

  Instruction *instruction = table_next(table);
  instruction->type = type;
  instruction->inputs[0] = b;
  instruction->output = output;
  instruction->comment = comment;

  // update_reference(instruction, a);
  update_reference(instruction, b);
  // update_reference(instruction, output);
  return output;
}

void instruction_no_output(InstructionTable *table, const InstructionType type, const Reference a, const Reference b, char *comment) {
  Instruction *instruction = table_next(table);
  instruction->type = type;
  instruction->inputs[0] = a;
  instruction->inputs[1] = b;
  instruction->comment = comment;

  update_reference(instruction, a);
  update_reference(instruction, b);
}

Reference instruction_ret(InstructionTable *table, const Reference value) {
  Instruction *instruction = table_next(table);
  instruction->type = RET;
  instruction->inputs[0] = value;
  instruction->comment = "ret";

  update_reference(instruction, value);
  return value;
}

Reference instruction_sp_reg_read(InstructionTable *table, const InstructionType type, char *comment) {
  const Type typ = {.kind = u8, .inner = NULL};
  Instruction *instruction = table_next(table);
  instruction->type = type;
  Allocation *allocation = table_allocate(table, typ);
  instruction->output = reference_direct(allocation);
  instruction->comment = comment;

  // update_reference(instruction, instruction->output);
  return instruction->output;
}

void statement_init(Statement *statement) {
  statement->values[0] = NULL;
  statement->values[1] = NULL;
  statement->values[2] = NULL;
}

Allocation *table_get_variable_by_token(const InstructionTable *table, const char *contents, const Token *token) {
  for (int i = table->allocations.len - 1; i >= 0; --i) {
    if (token_str_cmp(token, contents, ((Allocation *)table->allocations.array[i])->name) == 0) {
      return table->allocations.array[i];
    }
  }
  return NULL;
}

Reference ast_basic_op(const InstructionType type, const char *contents, InstructionTable *table, VarList *globals,
                       FunctionList *functions, StrList *literals, const AstNode *node, char *comment) {
  return instruction_basic_op(table, type, solve_ast_node(contents, table, globals, functions, literals, node->left),
                              solve_ast_node(contents, table, globals, functions, literals, node->right), comment);
}

Reference instr_test_self(InstructionTable *table, const InstructionType type, const Reference ref, char* comment) {
  instruction_no_output(table, TEST, ref, ref, NULL);

  return instruction_sp_reg_read(table, type, comment);
}

Reference instr_cmp_self(InstructionTable *table, const InstructionType type, const Reference left,
                         const Reference right, char *comment) {
  instruction_no_output(table, CMP, left, right, NULL);

  return instruction_sp_reg_read(table, type, comment);
}

void instruction_unary(InstructionTable *table, InstructionType type, Reference reference, char *comment) {
  Instruction *instruction = table_next(table);
  instruction->type = type;
  instruction->inputs[0] = reference;
  instruction->output = reference;
  instruction->comment = comment;

  update_reference(instruction, reference);
}

Reference solve_ast_node(const char *contents, InstructionTable *table, VarList *globals, FunctionList *functions,
                         StrList *literals, AstNode *node) {
  switch (node->type) {
  case op_nop:
    exit(112);
  case op_function:
    break;
  case op_array_index: {
    Reference index = ast_basic_op(ADD, contents, table, globals, functions, literals, node, "array index");
    index.access = Dereference;
    return index;
  }
  case op_comma: {
    // discard left, keep right
    solve_ast_node(contents, table, globals, functions, literals, node->left);
    return solve_ast_node(contents, table, globals, functions, literals, node->right);
  }
  case op_unary_negate: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    Reference reference = instruction_mov(table, inner, reference_direct(table_allocate_infer_type(table, inner)), NULL);
    instruction_unary(table, NEG, reference, "negate");
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
    if (inner.access == Global) {
      inner.access = GlobalRef;
      return inner;
    }
    Type type = {.kind = ptr, .inner = &inner.allocation->type};
    Allocation *output = table_allocate(table, type);
    instruction_lea(table, inner, reference_direct(output), "addressof");
    return reference_direct(output);
  }
  case op_unary_derefernce: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    switch (inner.access) {
    case Direct:
      inner.access = Dereference;
      return inner;
    case Dereference: {
      return reference_deref(instruction_mov(table, inner, reference_direct(table_allocate(table, *inner.allocation->type.inner)), NULL).allocation);
    }
      // case ConstantS:
      //   break;
    default:
      assert(false);
    }
  } break;
  case op_unary_not: {
    return instr_test_self(table, SETE, solve_ast_node(contents, table, globals, functions, literals, node->inner), "not");
  }
  case op_unary_bitwise_not: {
    Reference inner = solve_ast_node(contents, table, globals, functions, literals, node->inner);
    Reference reference = reference_direct(table_allocate_infer_type(table, inner));
    instruction_mov(table, inner, reference, NULL);
    instruction_unary(table, NOT, reference, "not");
    return reference;
  }
  case op_add: {
    return ast_basic_op(ADD, contents, table, globals, functions, literals, node, "add");
  }
  case op_subtract: {
    return ast_basic_op(SUB, contents, table, globals, functions, literals, node, "sub");
  }
  case op_multiply: {
    return ast_basic_op(IMUL, contents, table, globals, functions, literals, node, "mul");
  }
  case op_divide:
    // fixme div is not simple
    break;
  case op_modulo:
    // fixme div is not simple
    break;
  case op_bitwise_or: {
    return ast_basic_op(OR, contents, table, globals, functions, literals, node, "bitwise or");
  }
  case op_bitwise_xor: {
    return ast_basic_op(XOR, contents, table, globals, functions, literals, node, "xor");
  }
  case op_bitwise_and: {
    return ast_basic_op(AND, contents, table, globals, functions, literals, node, "bitwise and");
  }
  case op_assignment: {
    return instruction_mov(table, solve_ast_node(contents, table, globals, functions, literals, node->right), solve_ast_node(contents, table, globals, functions, literals, node->left), "assignment");
  }
  case op_and: {
    const Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    const Reference lhs = instr_test_self(table, SETNE, left, NULL);

    const Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    const Reference rhs = instr_test_self(table, SETNE, right, NULL);

    return instruction_basic_op(table, AND, lhs, rhs, "and");
  }
  case op_or: {
    ast_basic_op(OR, contents, table, globals, functions, literals, node, "or");
    return instruction_sp_reg_read(table, SETNE, NULL);
  }
  case op_deref_member_access:
    break;
  case op_member_access:
    break;
  case op_bitwise_left_shift: {
    return ast_basic_op(SAL, contents, table, globals, functions, literals, node, "lsh");
  }
  case op_bitwise_right_shift: {
    return ast_basic_op(SAR, contents, table, globals, functions, literals, node, "rsh");
  }
  case op_compare_equals: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    return instr_cmp_self(table, SETE, left, right, "==");
  }
  case op_compare_not_equals: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    return instr_cmp_self(table, SETNE, left, right, "!=");
  }
  case op_less_than: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    return instr_cmp_self(table, SETL, left, right, "<");
  }
  case op_greater_than: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    return instr_cmp_self(table, SETG, left, right, ">");
  }
  case op_less_than_equal: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    return instr_cmp_self(table, SETLE, left, right, "<=");
  }
  case op_greater_than_equal: {
    Reference left = solve_ast_node(contents, table, globals, functions, literals, node->left);
    Reference right = solve_ast_node(contents, table, globals, functions, literals, node->right);
    return instr_cmp_self(table, SETGE, left, right, ">=");
  }
  case op_value_constant: {
    Reference reference;
    reference.access = ConstantI;
    reference.value = token_copy(node->token, contents);
    return reference;
  }
  case op_value_string: {
    Reference reference;
    reference.access = ConstantS;
    char *copy = token_copy(node->token, contents);
    reference.str = strlist_indexof(literals, copy);
    free(copy);
    return reference;
  }
  case op_value_variable: {
    Reference reference;
    reference.access = Direct;
    reference.allocation = table_get_variable_by_token(table, contents, node->token);
    return reference;
  }
  case op_value_global: {
    Reference reference;
    reference.access = Global;
    char *copy = token_copy(node->token, contents);
    assert(varlist_indexof(globals, copy) != -1);
    reference.value = copy;
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
    return instruction_ret(table, solve_ast_node(contents, table, globals, functions, literals, node->inner));
  }
  }
  exit(23);
}
