#include "parse.h"

#include "codegen.h"

Result parse_function(const char *contents, Function *function, VarList *globals, FunctionList *functions,
                      StrList *literals, FILE *output) {
  assert(contents != NULL);
  if (function->start != NULL) {
    AstNodeList nodes;
    astnodelist_init(&nodes, 16);
    InstructionTable table;
    instructiontable_init(&table, function->name);

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
      if ((*token)->next->type == token_opening_paren) {
        const Token *fn = *token;
        *token = (*token)->next;
        int index = functionlist_indexof_tok(functions, contents, fn);
        if (index == -1) {
          return failure(fn, "function not declared");
        }
        Function *function = &functions->array[index];

        AstNode *node = astnodelist_grow(nodes);
        node->type = op_function;
        node->arguments = malloc(sizeof(AstNode) * function->arguments.len);
        for (int i = 0; i < function->arguments.len; ++i) {
          *token = (*token)->next;
          forward_err(parse_statement(contents, token, globals, functions,
                                      i == function->arguments.len - 1 ? token_closing_paren : token_comma,
                                      &node->arguments[i]));
        }
        node->function = function;
        *token = (*token)->next;
      } else {
        forward_err(parse_statement(contents, token, globals, functions, token_semicolon, astnodelist_grow(nodes)));
      }
      break;
    }
    case token_cf_if: {
      AstNode *node = astnodelist_grow(nodes);
      node->type = cf_if;
      node->condition = malloc(sizeof(AstNode));
      node->actions = malloc(sizeof(AstNodeList));
      node->alternative = malloc(sizeof(AstNodeList));

      astnodelist_init(node->actions, 16);
      astnodelist_init(node->alternative, 16);

      *token = (*token)->next;
      forward_err(parse_statement(contents, token, globals, functions, token_opening_curly_brace, node->condition));
      token_matches(*token, token_opening_curly_brace);
      forward_err(parse_scope(contents, token, globals, functions, literals, node->actions));
      if ((*token)->next->type != token_cf_else) {
        free(node->alternative);
        node->alternative = NULL;
      } else {
        *token = (*token)->next->next;
        token_matches(*token, token_opening_curly_brace);
        // if ((*token)->type == token_opening_curly_brace) {
        forward_err(parse_scope(contents, token, globals, functions, literals, node->alternative));
        // } else {
        //   exit(48);
        //   token_matches(*token, token_cf_if);
        //   *token = (*token)->next;
        //   forward_err(parse_statement(contents, token, globals, functions, token_opening_curly_brace,
        //   node->condition)); token_matches(*token, token_opening_curly_brace); forward_err(parse_scope(contents,
        //   token, globals, functions, literals, node->alternative));
        // }
      }
      break;
    }
    case token_cf_while: {
      AstNode *node = astnodelist_grow(nodes);
      node->type = cf_while;
      node->condition = malloc(sizeof(AstNode));
      node->actions = malloc(sizeof(AstNodeList));

      astnodelist_init(node->actions, 16);

      *token = (*token)->next;
      forward_err(parse_statement(contents, token, globals, functions, token_opening_curly_brace, node->condition));
      token_matches(*token, token_opening_curly_brace);
      forward_err(parse_scope(contents, token, globals, functions, literals, node->actions));
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
  assert(false);
  return failure(NULL, "assertion");
}
