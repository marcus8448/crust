#include "ir.h"

#include <stddef.h>
#include <stdio.h>

void allocate_arguments(AllocationTable* table, const Function* function) {
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

Allocation* table_allocate(AllocationTable* table) {
  return allocationlist_grow(&table->allocations);
}

Instruction* table_next(AllocationTable* table) {
  return instlist_grow(&table->instructions);
}

void statement_init(Statement* statement) {
  statement->values[0] = NULL;
  statement->values[1] = NULL;
  statement->values[2] = NULL;
}

Allocation* solve_node_allocation(const char* contents, AllocationTable* table, VarList* globals, FunctionList* functions,
                                StrList* literals, AstNode* node, FILE* file);

Allocation* simple_op(const char* contents, InstructionType op, AllocationTable* table, VarList* globals, FunctionList* functions,
                    StrList* literals, AstNode* node, FILE* file) {
  Allocation* left = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
  Allocation* right = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);

  Allocation* allocation = table_allocate(table);

  Instruction* instruction = table_next(table);
  instruction->inputs[0] = left;
  instruction->inputs[1] = right;
  instruction->inputs[2] = NULL;

  instruction->type = op;

  return allocation;
}

Allocation* unarry_op(const char* contents, const char* op, AllocationTable* frame, VarList* globals, FunctionList* functions,
                    StrList* literals, AstNode* node, FILE* file) {
  Allocation* ref = solve_node_allocation(contents, frame, globals, functions, literals, node->inner, file);
  stackframe_allocate_temporary_from(frame, &ref, file);
  char* mnemonic = allocation_mnemonic(ref);
  fprintf(file, "%s%c %s\n", op, get_suffix(typekind_width(ref->value_type.kind)), mnemonic);
  free(mnemonic);
  return ref;
}

Allocation* solve_node_allocation(const char* contents, AllocationTable* table, VarList* globals, FunctionList* functions,
                                StrList* literals, AstNode* node, FILE* file) {
  switch (node->type) {
  case op_nop:
    exit(112);
  case op_function:
    break;
  case op_array_index: {
    Allocation* array = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    Allocation* index = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);
    //todo
    return array;
  }
  case op_comma: {
    solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    return solve_node_allocation(contents, table, globals, functions, literals, node->right, file);
  }
  case op_unary_negate:
    return unarry_op(contents, "neg", table, globals, functions, literals, node, file);
  case op_unary_plus:
    return solve_node_allocation(contents, table, globals, functions, literals, node->inner, file);
  case op_unary_addressof: {
    Allocation* node_allocation = solve_node_allocation(contents, table, globals, functions, literals, node->inner, file);
    if (node_allocation->type != ref_variable) {
      exit(26);
    }

    Type type;
    type.kind = ptr;
    type.inner = &node_allocation->value_type;
    Allocation* temp = stackframe_allocate_temporary(table, type, file);

    stackframe_moveto_stack(table, node_allocation, file);

    char* mnemonicL = allocation_mnemonic(temp);
    char* mnemonicR = allocation_mnemonic(node_allocation);
    fprintf(file, "lea%c %s, %s\n", get_suffix(Quad), mnemonicR, mnemonicL); //todo: ptr always quad?
    free(mnemonicL);
    free(mnemonicR);
    return temp;
  }
  case op_unary_derefernce: {
    Allocation* node_allocation = solve_node_allocation(contents, table, globals, functions, literals, node->inner, file);
    if (node_allocation->value_type.kind != ptr) {
      exit(27);
    }

    Allocation* temp = stackframe_allocate_temporary(table, *node_allocation->value_type.inner, file);
    char* mnemonicL = allocation_mnemonic(temp);
    char* mnemonicR = allocation_mnemonic(node_allocation);
    fprintf(file, "mov%c (%s), %s\n", get_suffix(typekind_width(temp->value_type.kind)), mnemonicR, mnemonicL);
    free(mnemonicL);
    free(mnemonicR);
    return temp;
  }
  case op_unary_not:
    break;
  case op_unary_bitwise_not:
    return unarry_op(contents, "neg", table, globals, functions, literals, node, file);
  case op_add:
    return simple_op(contents, "add", table, globals, functions, literals, node, file);
  case op_subtract:
    return simple_op(contents, "sub", table, globals, functions, literals, node, file);
  case op_multiply: // fixme
    return simple_op(contents, "imul", table, globals, functions, literals, node, file);
  case op_divide:
    // fixme div is not simple
      break;
  case op_modulo:
    // fixme div is not simple
      break;
  case op_bitwise_or:
    return simple_op(contents, "or", table, globals, functions, literals, node, file);
  case op_bitwise_xor:
    return simple_op(contents, "xor", table, globals, functions, literals, node, file);
  case op_bitwise_and:
    return simple_op(contents, "and", table, globals, functions, literals, node, file);
  case op_assignment: {
    Allocation* left = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    Allocation* right = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);
    if (left->type == ref_constant) {
      exit(28);
    }

    char* mnemonicL = allocation_mnemonic(left);
    char* mnemonicR = allocation_mnemonic(right);
    fprintf(file, "mov%c %s, %s\n # assignment to %s", get_suffix(typekind_width(left->value_type.kind)), mnemonicR, mnemonicL, left->name);
    free(mnemonicL);
    free(mnemonicR);
    stackframe_free_ref(table, left);
    stackframe_free_ref(table, right);
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
  case op_bitwise_left_shift:
    return simple_op(contents, "sal", table, globals, functions, literals, node, file);
  case op_bitwise_right_shift:
    return simple_op(contents, "sar", table, globals, functions, literals, node, file);
  case op_compare_equals: {
    Allocation* left = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    Allocation* right = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);

    stackframe_force_move_register(table, al, file);

    char* mnemonicL = allocation_mnemonic(left);
    char* mnemonicR = allocation_mnemonic(right);
    fprintf(file, "cmp%c %s, %s\n", get_suffix(typekind_width(left->value_type.kind)), mnemonicR, mnemonicL);
    fputs("sete %al\n", file);
    free(mnemonicL);
    free(mnemonicR);
    if (left->type == ref_constant || left->type == ref_temporary) stackframe_free_ref(table, left);
    if (right->type == ref_constant || right->type == ref_temporary) stackframe_free_ref(table, right);
    Type type;
    type.kind = i8;
    type.inner = NULL;
    return stackframe_allocate_temporary_reg(table, type, al, file);
  }
  case op_compare_not_equals: {
    Allocation* left = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    Allocation* right = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);

    stackframe_force_move_register(table, rax, file);

    char* mnemonicL = allocation_mnemonic(left);
    char* mnemonicR = allocation_mnemonic(right);
    fprintf(file, "cmp%c %s, %s\n", get_suffix(typekind_width(left->value_type.kind)), mnemonicR, mnemonicL);
    fputs("setne %al\n", file);
    free(mnemonicL);
    free(mnemonicR);
    if (left->type == ref_constant || left->type == ref_temporary) stackframe_free_ref(table, left);
    if (right->type == ref_constant || right->type == ref_temporary) stackframe_free_ref(table, right);
    Type type;
    type.kind = i8;
    type.inner = NULL;
    Allocation* temp = stackframe_allocate_temporary(table, type, file);
    stackframe_set_register(table, temp, al);
    return temp;
  }
  case op_less_than: {
    Allocation* left = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    Allocation* right = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);

    stackframe_force_move_register(table, rax, file);

    char* mnemonicL = allocation_mnemonic(left);
    char* mnemonicR = allocation_mnemonic(right);
    fprintf(file, "cmp%c %s, %s\n", get_suffix(typekind_width(left->value_type.kind)), mnemonicR, mnemonicL);
    fputs("setl %al\n", file);
    free(mnemonicL);
    free(mnemonicR);
    if (left->type == ref_constant || left->type == ref_temporary) stackframe_free_ref(table, left);
    if (right->type == ref_constant || right->type == ref_temporary) stackframe_free_ref(table, right);
    Type type;
    type.kind = i8;
    type.inner = NULL;
    Allocation* temp = stackframe_allocate_temporary(table, type, file);
    stackframe_set_register(table, temp, al);
    return temp;
  }
  case op_greater_than: {
    Allocation* left = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    Allocation* right = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);

    stackframe_force_move_register(table, rax, file);

    char* mnemonicL = allocation_mnemonic(left);
    char* mnemonicR = allocation_mnemonic(right);
    fprintf(file, "cmp%c %s, %s\n", get_suffix(typekind_width(left->value_type.kind)), mnemonicR, mnemonicL);
    fputs("setg %al\n", file);
    free(mnemonicL);
    free(mnemonicR);
    if (left->type == ref_constant || left->type == ref_temporary) stackframe_free_ref(table, left);
    if (right->type == ref_constant || right->type == ref_temporary) stackframe_free_ref(table, right);
    Type type;
    type.kind = i8;
    type.inner = NULL;
    Allocation* temp = stackframe_allocate_temporary(table, type, file);
    stackframe_set_register(table, temp, al);
    return temp;
  }
  case op_less_than_equal: {
    Allocation* left = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    Allocation* right = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);

    stackframe_force_move_register(table, rax, file);

    char* mnemonicL = allocation_mnemonic(left);
    char* mnemonicR = allocation_mnemonic(right);
    fprintf(file, "cmp%c %s, %s\n", get_suffix(typekind_width(left->value_type.kind)), mnemonicR, mnemonicL);
    fputs("setle %al\n", file);
    free(mnemonicL);
    free(mnemonicR);
    if (left->type == ref_constant || left->type == ref_temporary) stackframe_free_ref(table, left);
    if (right->type == ref_constant || right->type == ref_temporary) stackframe_free_ref(table, right);
    Type type;
    type.kind = i8;
    type.inner = NULL;
    Allocation* temp = stackframe_allocate_temporary(table, type, file);
    stackframe_set_register(table, temp, al);
    return temp;
  }
  case op_greater_than_equal:  {
    Allocation* left = solve_node_allocation(contents, table, globals, functions, literals, node->left, file);
    Allocation* right = solve_node_allocation(contents, table, globals, functions, literals, node->right, file);

    stackframe_force_move_register(table, rax, file);

    char* mnemonicL = allocation_mnemonic(left);
    char* mnemonicR = allocation_mnemonic(right);
    fprintf(file, "cmp%c %s, %s\n", get_suffix(typekind_width(left->value_type.kind)), mnemonicR, mnemonicL);
    fputs("setge %al\n", file);
    free(mnemonicL);
    free(mnemonicR);
    if (left->type == ref_constant || left->type == ref_temporary) stackframe_free_ref(table, left);
    if (right->type == ref_constant || right->type == ref_temporary) stackframe_free_ref(table, right);
    Type type;
    type.kind = i8;
    type.inner = NULL;
    Allocation* temp = stackframe_allocate_temporary(table, type, file);
    stackframe_set_register(table, temp, al);
    return temp;
  }
  case op_value_constant: { //todo string literals
    Allocation* alloc = malloc(sizeof(Allocation));
    alloc->type = ref_constant;
    alloc->value_type.kind = i64; // fixme
    alloc->value_type.inner = NULL;
    char* refr = malloc(node->token->len + 2);
    refr[0] = '$';
    strncpy(refr + 1, contents + node->token->index, node->token->len);
    refr[node->token->len + 1] = '\0';
    alloc->repr = refr;
    return alloc;
  }
  case op_value_variable: {
    Allocation* ref = stackframe_get_by_token(table, contents, node->token);
    if (ref == NULL) {
      exit(99);
    }
    return ref;
  }
  case op_cast:
    break;
  };
  exit(23);
}