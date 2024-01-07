#include "ast.h"

#include <stdio.h>
#include <string.h>

LIST_IMPL(Var, var, Variable)
LIST_IMPL(Function, function, Function)

void function_init(Function *function) {
  varlist_init(&function->arguments, 2);
  function->name = NULL;
  function->retVal.kind = 0;
  function->retVal.inner = NULL;
  function->start = NULL;
}

int functionlist_indexof(const FunctionList *list, const char *name) {
  for (int i = 0; i < list->len; i++) {
    if (strcmp(list->array[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

int functionlist_indexof_tok(const FunctionList *list, const char *contents, const Token *token) {
  for (int i = 0; i < list->len; i++) {
    if (strncmp(list->array[i].name, contents + token->index, token->len) == 0) {
      return i;
    }
  }
  return -1;
}

int varlist_indexof(const VarList *list, const char *name) {
  for (int i = 0; i < list->len; i++) {
    if (strcmp(list->array[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

Variable *varlist_get_by_token(const VarList *list, const char *contents, const Token *token) {
  for (int i = 0; i < list->len; i++) {
    if (token_str_cmp(token, contents, list->array[i].name) == 0) {
      return &list->array[i];
    }
  }
  return NULL;
}

int ast_operand_count(AstNodeType type) {
  switch (type) {
  case op_nop:
  case op_value_constant:
  case op_value_string:
  case op_value_variable:
  case op_value_global:
    return 0;
  case op_unary_negate:
  case op_unary_plus:
  case op_unary_addressof:
  case op_unary_derefernce:
  case op_unary_not:
  case op_unary_bitwise_not:
    return 1;
  default:
    return 2;
  }
}

int ast_precedence(AstNodeType type) {
  switch (type) {
  case op_function:
    return 1;
  case op_array_index:
    return 1;
  case op_comma:
    return 15;
  case op_unary_negate:
    return 2;
  case op_unary_plus:
    return 2;
  case op_unary_addressof:
    return 2;
  case op_unary_derefernce:
    return 2;
  case op_unary_not:
    return 2;
  case op_unary_bitwise_not:
    return 2;
  case op_add:
    return 4;
  case op_subtract:
    return 4;
  case op_multiply:
    return 3;
  case op_divide:
    return 3;
  case op_modulo:
    return 3;
  case op_bitwise_or:
    return 10;
  case op_bitwise_xor:
    return 9;
  case op_bitwise_and:
    return 8;
  case op_assignment:
    return 14;
  case op_and:
    return 11;
  case op_or:
    return 12;
  case op_deref_member_access:
    return 1;
  case op_member_access:
    return 1;
  case op_bitwise_left_shift:
    return 5;
  case op_bitwise_right_shift:
    return 5;
  case op_compare_equals:
    return 7;
  case op_compare_not_equals:
    return 7;
  case op_less_than:
    return 6;
  case op_greater_than:
    return 6;
  case op_less_than_equal:
    return 6;
  case op_greater_than_equal:
    return 6;
  case op_value_constant:
    return -1;
  case op_value_string:
    return -1;
  case op_value_variable:
    return -1;
  case op_value_global:
    return -1;
  case op_nop:
    return -1;
  case op_cast:
    return 1;
  case op_value_let:
    return -1;
  case cf_if:
    return -1;
  case cf_while:
    return -1;
  case cf_return:
    return -1;
  }
  exit(17);
}

Result parse_statement(const char *contents, const Token **token, VarList *globals, FunctionList *functions,
                       const TokenType until, AstNode *node) {
  PtrList values;
  ptrlist_init(&values, 2);
  PtrList operators;
  ptrlist_init(&operators, 2);

  while ((*token)->type != until) {
    {
      AstNode *nxt = malloc(sizeof(AstNode));
      forward_err(parse_value(contents, token, globals, functions, nxt));
      ptrlist_add(&values, nxt);
    }

    *token = (*token)->next;
    if ((*token)->type == until) {
      assert(operators.len + 1 == values.len);
      if (operators.len == 0) {
        assert(values.len == 1);
        *node = *(AstNode *)values.array[0];
      } else {
        int valI = values.len - 1;
        AstNode *prev = operators.array[operators.len - 1];
        prev->right = values.array[valI--];
        prev->left = values.array[valI--];
        for (int i = operators.len - 2; i >= 0; i--) {
          AstNode *op = operators.array[i];
          op->right = prev;
          op->left = values.array[valI--];
          prev = op;
        }
        *node = *prev;
      }
      return success();
    }

    AstNode *nxt = malloc(sizeof(AstNode));
    switch ((*token)->type) {
    case token_plus:
      nxt->type = op_add;
      break;
    case token_minus:
      nxt->type = op_subtract;
      break;
    case token_asterik:
      nxt->type = op_multiply;
      break;
    case token_slash:
      nxt->type = op_divide;
      break;
    case token_vertical_bar:
      nxt->type = op_bitwise_or;
      break;
    case token_caret:
      nxt->type = op_bitwise_xor;
      break;
    case token_amperstand:
      nxt->type = op_bitwise_and;
      break;
    case token_period:
      nxt->type = op_member_access;
      break;
    case token_equals_assign:
      nxt->type = op_assignment;
      break;
    case token_double_vertical_bar:
      nxt->type = op_or;
      break;
    case token_double_amperstand:
      nxt->type = op_and;
      break;
    case token_arrow:
      nxt->type = op_deref_member_access;
      break;
    case token_left_shift:
      nxt->type = op_bitwise_left_shift;
      break;
    case token_right_shift:
      nxt->type = op_bitwise_right_shift;
      break;
    case token_not_equals:
      nxt->type = op_compare_not_equals;
      break;
    case token_double_equals:
      nxt->type = op_compare_equals;
      break;
    case token_less_than:
      nxt->type = op_less_than;
      break;
    case token_greater_than:
      nxt->type = op_greater_than;
      break;
    case token_less_than_equal:
      nxt->type = op_less_than_equal;
      break;
    case token_greater_than_equal:
      nxt->type = op_greater_than_equal;
      break;
    default:
      return failure(*token, "U token");
    }
    if (operators.len > 0 &&
        ast_precedence(nxt->type) >= ast_precedence(((AstNode *)operators.array[operators.len - 1])->type)) {
      AstNode *prev = operators.array[--operators.len];
      prev->val_type = ((AstNode *)values.array[--values.len])->val_type;
      prev->right = values.array[values.len];
      prev->left = values.array[--values.len];
      for (int i = operators.len - 1;
           i >= 0 && ast_precedence(nxt->type) >= ast_precedence(((AstNode *)operators.array[i])->type);
           i--, operators.len--) {
        AstNode *op = operators.array[i];
        op->right = prev;
        op->left = values.array[--values.len];
      }
      values.array[values.len++] = prev;
    }
    ptrlist_add(&operators, nxt);
    *token = (*token)->next;
  }
  if (operators.len == 0 && values.len == 0) {
    node->type = op_nop;
    return success();
  }
  return failure(*token, "unexp end of stmt");
}

Result parse_value(const char *contents, const Token **token, VarList *globals, FunctionList *functions,
                   AstNode *node) {
  node->token = *token;

  switch ((*token)->type) {
  case token_opening_paren:
    node->type = op_unary_plus;
    node->inner = malloc(sizeof(AstNode));
    *token = (*token)->next;
    forward_err(parse_statement(contents, token, globals, functions, token_closing_paren, node->inner));
    break;
  case token_plus:
    node->type = op_unary_plus;
    node->inner = malloc(sizeof(AstNode));
    *token = (*token)->next;
    forward_err(parse_value(contents, token, globals, functions, node->inner));
    break;
  case token_minus:
    node->type = op_unary_negate;
    node->inner = malloc(sizeof(AstNode));
    *token = (*token)->next;
    forward_err(parse_value(contents, token, globals, functions, node->inner));
    break;
  case token_asterik:
    node->type = op_unary_derefernce;
    node->inner = malloc(sizeof(AstNode));
    *token = (*token)->next;
    forward_err(parse_value(contents, token, globals, functions, node->inner));
    break;
  case token_amperstand:
    node->type = op_unary_addressof;
    node->inner = malloc(sizeof(AstNode));
    *token = (*token)->next;
    forward_err(parse_value(contents, token, globals, functions, node->inner));
    break;
  case token_tilde:
    node->type = op_unary_bitwise_not;
    node->inner = malloc(sizeof(AstNode));
    *token = (*token)->next;
    forward_err(parse_value(contents, token, globals, functions, node->inner));
    break;
  case token_exclaimation:
    node->type = op_unary_not;
    node->inner = malloc(sizeof(AstNode));
    *token = (*token)->next;
    forward_err(parse_value(contents, token, globals, functions, node->inner));
    break;
  case token_identifier: {
    switch ((*token)->next->type) {
    case token_opening_paren:
      node->inner = malloc(sizeof(AstNode));
      node->type = op_function;
      const int indexof_tok = functionlist_indexof_tok(functions, contents, *token);
      if (indexof_tok == -1)
        return failure(*token, "unknown funciton");

      *token = (*token)->next;
      Function *function = &functions->array[indexof_tok];
      node->arguments = malloc(sizeof(AstNodeList));
      astnodelist_init(node->arguments, function->arguments.len);
      for (int i = 0; i < function->arguments.len; ++i) {
        *token = (*token)->next;
        forward_err(parse_statement(contents, token, globals, functions, i == function->arguments.len - 1 ? token_closing_paren : token_comma, astnodelist_grow(node->arguments)));
      }
      node->function = function;
      break;
    case token_opening_sqbr:
      node->left = malloc(sizeof(AstNode));
      node->right = malloc(sizeof(AstNode));
      node->left->type = op_value_variable;
      node->left->token = *token;
      node->type = op_array_index;
      *token = (*token)->next;
      node->token = *token;
      *token = (*token)->next;
      forward_err(parse_statement(contents, token, globals, functions, token_closing_sqbr, node->right));
      break;
    default:
      node->type = op_value_variable;
      break;
    }
    break;
  }
  case token_constant:
    node->type = op_value_constant;
    break;
  case token_string:
    node->type = op_value_string;
    break;
  case token_cf_if:
  case token_cf_else:
  case token_cf_return:
  case token_cf_break:
    exit(43);
    break;
  default:
    return failure(*token, "unkon");
  }

  if ((*token)->next->type == token_keyword_as) {
    *token = (*token)->next;
    AstNode *node1 = malloc(sizeof(AstNode));
    *node1 = *node;
    node->token = *token;
    Type type;
    parse_type(contents, token, &type);

    node->val_type = type;
    node->type = op_cast;
    node->inner = node1;
  }
  return success();
}

Result parse_args(const char *contents, const Token **token, Function *function, VarList *vars, FunctionList *functions,
                  AstNode *inner) {
  if (function->arguments.len > 0) {
    for (int i = 0; i < function->arguments.len; ++i) {
    *token = (*token)->next;
      forward_err(parse_statement(contents, token, vars, functions,
                      i == function->arguments.len - 1 ? token_closing_paren : token_comma, inner));
    }
  }
  return success();
}

LIST_IMPL(AstNode, astnode, AstNode)
