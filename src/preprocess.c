#include "preprocess.h"

#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "struct/list.h"
#include "types.h"

Result parse_function_declaration(const char* contents, const Token** token, Function* function, bool has_decl) {
  *token = (*token)->next;
  token_matches(*token, token_identifier);
  function->name = token_copy(*token, contents);

  *token = (*token)->next;
  token_matches(*token, token_opening_paren);

  while ((*token = (*token)->next)->type != token_closing_paren) {
    if ((*token)->type == token_identifier) {
      Variable argument;
      argument.name = token_copy(*token, contents);

      *token = (*token)->next;
      token_matches(*token, token_colon);

      forward_err(parse_type(contents, token, &argument.type));
      varlist_add(&function->arguments, argument);
    }

    *token = (*token)->next;
    if ((*token)->type == token_closing_paren)
      break;
    token_matches_ext(*token, token_comma, ", or )");
  }

  *token = (*token)->next;
  if ((*token)->type == token_arrow) {
    forward_err(parse_type(contents, token, &function->retVal));
    *token = (*token)->next;
  }

  if (has_decl) {
    token_matches_ext(*token, token_opening_curly_brace, "-> or {");

    function->start = *token;
    int depth = 0;
    while ((*token)->type != token_eof) {
      if ((*token)->type == token_opening_curly_brace) {
        depth += 1;
      } else if ((*token)->type == token_closing_curly_brace) {
        if (--depth == 0)
          return success();
      } else if ((*token)->type == token_keyword_fn) {
        return failure(*token, "unexpected function delcaration (expected statement)");
      }
      *token = (*token)->next;
    }
    return failure(*token, "unexpected eof ({})");
  }
  token_matches(*token, token_semicolon);
  return success();
}

int add_str_literal(char* contents, const Token* token, StrList* strLiterals, FILE* output) {
  char* buf = malloc(token->len + 1);
  memcpy(buf, contents + token->index, token->len);
  buf[token->len] = '\0';
  const int index = strlist_indexof(strLiterals, buf);
  if (index != -1) {
    free(buf);
    return index;
  }
  fprintf(output, ".L.STR%i:\n\t.asciz\t%s\n\t.size\t.L.STR%i, %i\n", strLiterals->len, buf, strLiterals->len,
          token->len - 1);
  strlist_add(strLiterals, buf);
  return strLiterals->len - 1;
}

Result preprocess_globals(char* contents, const Token* token, StrList* strLiterals, VarList* variables,
                          FunctionList* functions, FILE* output) {
  while (token != NULL) {
    switch (token->type) {
    case token_eof:
      return success();
    case token_semicolon:
      break;
    case token_keyword_fn: {
      Function function;
      function_init(&function);
      forward_err(parse_function_declaration(contents, &token, &function, true));

      if (functionlist_indexof(functions, function.name) != -1) {
        return failure(token, "redefinition of function");
      }
      fprintf(output, ".globl %s\n", function.name);
      functionlist_add(functions, function);
    } break;
    case token_keyword_let: {
      Variable variable;
      token = token->next;
      token_matches(token, token_identifier);
      variable.name = token_copy(token, contents);

      if (varlist_indexof(variables, variable.name) != -1) {
        return failure(token, "redefinition of global variable");
      }

      token = token->next;
      token_matches(token, token_colon);

      Type type;
      forward_err(parse_type(contents, &token, &type));
      variable.type = type;

      varlist_add(variables, variable);

      token = token->next;
      if (token->type == token_semicolon) {
        typekind_width(type.kind);
        fprintf(output, "%s:\n", variable.name);
        const int bytes = size_bytes(typekind_width(type.kind));
        fprintf(output, "\t.zero\t%i\n", bytes);
        fprintf(output, "\t.size\t%s, %i\n", variable.name, bytes);
      } else {
        token_matches(token, token_equals_assign);
        token = token->next;
        const Token* value = token;
        if (token->type == token_constant) {
          token_matches(token, token_constant);
          token = token->next;
          token_matches(token, token_semicolon);
          const int bytes = size_bytes(typekind_width(type.kind));
          fprintf(output, "%s:\n", variable.name);
          fprintf(output, "\t.%s\t%.*s\n", size_mnemonic(typekind_width(type.kind)), value->len, contents + value->index);
          fprintf(output, "\t.size\t%s, %i\n", variable.name, bytes);
        } else if (token->type == token_string) {
          token_matches(token, token_string);
          const int index = add_str_literal(contents, token, strLiterals, output);
          token = token->next;
          token_matches(token, token_semicolon);
          fprintf(output, "%s:\n", variable.name);
          fprintf(output, "\t.quad\t.L.STR%i\n", index);
        } else {
          return failure(token, "expected constant or string literal");
        }
      }
    } break;
    case token_string: {
      add_str_literal(contents, token, strLiterals, output);
    } break;
    case token_keyword_extern: {
      token = token->next;
      switch (token->type) {
      case token_keyword_fn: {
        Function function;
        function_init(&function);
        forward_err(parse_function_declaration(contents, &token, &function, false));
        functionlist_add(functions, function);
        fprintf(output, ".extern %s\n", function.name);
      } break;
      case token_keyword_let: {
        Variable variable;
        token = token->next;
        token_matches(token, token_identifier);
        variable.name = token_copy(token, contents);

        if (varlist_indexof(variables, variable.name) != -1) {
          return failure(token, "redefinition of global variable");
        }

        token = token->next;
        token_matches(token, token_colon);

        Type type;
        forward_err(parse_type(contents, &token, &type));
        variable.type = type;

        varlist_add(variables, variable);
        fprintf(output, ".extern %s\n", variable.name);
      } break;
      default:
        return failure(token, "expected function or variable definition");
      }
    } break;
    default:
      return failure(token, "expected function or variable definition");
    }
    token = token->next;
  }
  return success();
}
