#include "parse.h"

#include "codegen.h"

Result parse_function(const char* contents, Function* function, VarList* globals, FunctionList* functions,
                      StrList* literals, FILE* output) {
  assert(contents != NULL);
  if (function->start != NULL) {
    InstructionTable table;
    instructiontable_init(&table, 0);

    fprintf(output, "%s:\n", function->name);
    table_allocate_arguments(&table, function);
    const Token* token = function->start;
    forward_err(parse_scope(contents, &token, &table, globals, functions, literals, output));
    Registers registers;

    registers_init(&registers, &table);
    generate_statement(&registers, contents, &table, globals, functions, literals, output);

    instructiontable_free(&table);
  }
  return success();
}

Result parse_scope(const char* contents, const Token** token, InstructionTable* table, VarList* globals,
                   FunctionList* functions, StrList* literals, FILE* output) {
  assert(contents != NULL);
  token_matches(*token, token_opening_curly_brace);
  while ((*token)->next != NULL) {
    int deref = 0;
    switch ((*token = (*token)->next)->type) {
    case token_closing_curly_brace: {
      return success();
    }
    case token_keyword_let: {
      Variable variable;
      *token = (*token)->next;
      token_matches(*token, token_identifier);
      const Token* token1 = *token;
      variable.name = token_copy(token1, contents);

      *token = (*token)->next;
      token_matches(*token, token_colon);

      Type type;
      forward_err(parse_type(contents, token, &type));
      variable.type = type;

      *token = (*token)->next;
      table_allocate_variable(table, variable);
      if ((*token)->type != token_semicolon) {
        AstNode node;
        token_matches(*token, token_equals_assign);
        *token = (*token)->next;
        forward_err(parse_statement(contents, token, globals, functions, token_semicolon, &node));

        AstNode eq_left;
        eq_left.type = op_value_variable;
        eq_left.token = token1;
        eq_left.val_type = type;

        AstNode eq;
        eq.type = op_assignment;
        eq.left = &eq_left;
        eq.right = &node;

        solve_ast_node(contents, table, globals, functions, literals, &eq);
      }
      break;
    }
    case token_asterik: {
      while ((*token)->type == token_asterik) {
        deref++;
        *token = (*token)->next;
      }
    }
    // fall through
    case token_identifier: {
      const Token *token1 = *token;

      *token = (*token)->next;

      switch ((*token)->type) {
      case token_equals_assign: {
        AstNode node;
        token_matches(*token, token_equals_assign);
        *token = (*token)->next;
        forward_err(parse_statement(contents, token, globals, functions, token_semicolon, &node));

        AstNode eq_left;
        eq_left.type = op_value_variable;
        eq_left.token = token1;
        eq_left.val_type.kind=UNINIT;
        eq_left.val_type.inner=NULL;
        AstNode* out = &eq_left;
        for (int i = 0; i < deref; i++) {
          AstNode *inner = malloc(sizeof(AstNode));
          inner->type = op_unary_derefernce;
          inner->token = token1;
          inner->inner = out;
          out = inner;
        }

        AstNode eq;
        eq.type = op_assignment;
        eq.left = out;
        eq.right = &node;

        solve_ast_node(contents, table, globals, functions, literals, &eq);
      } break;
      case token_opening_paren: {
        char* name = token_copy(token1, contents);
        const int indexof = functionlist_indexof(functions, name);
        free(name);
        if (indexof == -1) {
          return failure(*token, "unknown function");
        }
        forward_err(invoke_function(contents, token, table, globals, functions, functions->array[indexof], output));
      } break;
      default:
        token_matches_ext(*token, token_eof, "invalid id sx");
        break;
      }
      break;
    }
    case token_cf_if:
      AstNode condition;
      forward_err(parse_statement(contents, token, globals, functions, token_opening_curly_brace, &condition));
      token_matches(*token, token_opening_curly_brace);
      //todo
      break;
    case token_cf_else: {
      // *token = (*token)->next;
      // token_matches(*token, opening_curly_brace);

      break;
    }
    case token_cf_return: {
      *token = (*token)->next;
      AstNode node;
      forward_err(parse_statement(contents, token, globals, functions, token_semicolon, &node));
      Reference node_allocation = solve_ast_node(contents, table, globals, functions, literals, &node);
      instruction_unary(table_next(table), table, RET, node_allocation, "Return");
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

Result invoke_function(const char* contents, const Token** token, InstructionTable* table, VarList* vars,
                       FunctionList* function, Function file, FILE* output) {
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
