#include "parse.h"

#include "codegen.h"

Result parse_function(const char *contents, Function *function, VarList *globals, FunctionList *functions,
                      StrList *literals, FILE *output) {
  assert(contents != NULL);
  if (function->start != NULL) {
    AstNodeList nodes;
    astnodelist_init(&nodes, 16);
    InstructionTable table;
    instructiontable_init(&table, 0);

    fprintf(output, "%s:\n", function->name);
    table_allocate_arguments(&table, function);
    const Token *token = function->start;
    forward_err(parse_scope(contents, &token, globals, functions, literals, &nodes));

    for (int i = 0; i < nodes.len; ++i) {
      solve_ast_node(contents, &table, globals, functions, literals, &nodes.array[i]);
    }
    Registers registers;

    registers_init(&registers, &table);
    generate_statement(&registers, contents, &table, globals, functions, literals, output);

    instructiontable_free(&table);
  }
  return success();
}

Result parse_scope(const char *contents, const Token **token, VarList *globals, FunctionList *functions,
                   StrList *literals, AstNodeList *nodes) {
  assert(contents != NULL);
  token_matches(*token, token_opening_curly_brace);
  while ((*token)->next != NULL) {
    switch ((*token = (*token)->next)->type) {
    case token_closing_curly_brace: {
      return success();
    }
    case token_keyword_let: {
      Variable variable;
      *token = (*token)->next;
      token_matches(*token, token_identifier);
      const Token *token1 = *token;
      variable.name = token_copy(token1, contents);

      *token = (*token)->next;
      token_matches(*token, token_colon);

      Type type;
      forward_err(parse_type(contents, token, &type));
      variable.type = type;

      *token = (*token)->next;

      if ((*token)->type == token_equals_assign) {
        AstNode *left = malloc(sizeof(AstNode));
        left->type = op_value_let;
        left->token = token1;
        left->variable = variable;

        AstNode *eq_right = malloc(sizeof(AstNode));
        *token = (*token)->next;
        forward_err(parse_statement(contents, token, globals, functions, token_semicolon, eq_right));

        AstNode *node = astnodelist_grow(nodes);

        node->type = op_assignment;
        node->left = left;
        node->right = eq_right;
      } else {
        token_matches(*token, token_semicolon);
        AstNode *let = astnodelist_grow(nodes);
        let->type = op_value_let;
        let->token = token1;
        let->variable = variable;
      }
      break;
    }
    case token_asterik:
    case token_identifier: {
      forward_err(parse_statement(contents, token, globals, functions, token_semicolon, astnodelist_grow(nodes)));
      break;
    }
    case token_cf_if: {
      AstNode *condition = malloc(sizeof(AstNode));
      AstNodeList *actions = malloc(sizeof(AstNodeList));
      AstNodeList *otherwise = malloc(sizeof(AstNodeList));

      forward_err(parse_statement(contents, token, globals, functions, token_opening_curly_brace, condition));
      token_matches(*token, token_opening_curly_brace);
      forward_err(parse_scope(contents, token, globals, functions, literals, actions));
      if ((*token)->type == token_cf_else) {
        free(otherwise);
        otherwise = NULL;
      } else {
        *token = (*token)->next;
        if ((*token)->type == token_opening_curly_brace) {
          forward_err(parse_scope(contents, token, globals, functions, literals, otherwise));
        } else {
          token_matches(*token, token_cf_if);
          forward_err(parse_statement(contents, token, globals, functions, token_opening_curly_brace, condition));
          token_matches(*token, token_opening_curly_brace);
          forward_err(parse_scope(contents, token, globals, functions, literals, otherwise));
        }
      }
      break;
    }
    case token_cf_return: {
      AstNode *node = astnodelist_grow(nodes);
      AstNode *value = malloc(sizeof(AstNode));
      *token = (*token)->next;
      forward_err(parse_statement(contents, token, globals, functions, token_semicolon, value));

      node->type = cf_return;
      node->inner = value;
      break;
    }
    case token_cf_break: {
      break;
    }
    case token_semicolon:
      continue;
    default:
      return failure(*token, "expected start of statement");
    }
  }
  // unreachable
  abort();
}

Result invoke_function(const char *contents, const Token **token, InstructionTable *table, VarList *vars,
                       FunctionList *function, Function file, FILE *output) {
  token_matches(*token, token_opening_paren);
  *token = (*token)->next;
  // FIXME todo
  //  for (int i = 0; i < registers->allocations.len; ++i) {
  //    switch (ref_get_register(&frame->allocations.array[i])) {
  //    case rax:
  //    case rdi:
  //    case rsi:
  //    case rdx:
  //    case rcx:
  //    case r8:
  //    case r9:
  //    case r10:
  //    case r11:
  //      stackframe_moveto_stack(frame, &frame->allocations.array[i], file);
  //      break;
  //    default:
  //      break;
  //    }
  //  }
  //
  //  for (int i = 0; i < function.arguments.len; i++) {
  //    AstNode node;
  //    forward_err(parse_statement(contents, token, globals, functions, token_closing_paren, &node));
  //    if (i < 6) {
  //    } else {
  //    }
  //
  //    if (i + 1 == function.arguments.len) {
  //      token_matches(*token, token_closing_paren);
  //    } else {
  //      token_matches(*token, token_comma);
  //    }
  //  }
  //  fprintf(file, "call %s\n", function.name);
  return success();
}
