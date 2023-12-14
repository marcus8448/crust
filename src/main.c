#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "codegen.h"
#include "ir.h"
#include "preprocess.h"
#include "register.h"
#include "struct/list.h"
#include "token.h"

typedef struct {
  const char* filename;
  char* contents;
  size_t len;
} FileData;

int filedata_init(FileData* data, const char* filename) {
  FILE* file = fopen(filename, "rb");
  if (file == NULL)
    return 2;
  fseek(file, 0, SEEK_END);
  const long len = ftell(file);
  if (len < 0 || len > 1024 * 64) {
    fclose(file);
    puts("file too large");
    return 1;
  }
  char* contents = malloc(len + 1);
  rewind(file);
  fread(contents, len, 1, file);
  fclose(file);
  contents[len] = '\0';

  data->filename = filename;
  data->contents = contents;
  data->len = len;
  return 0;
}

void print_error(const char* section, const Result result, const char* filename, const char* contents, const int len) {
  int line = 1;
  int lineStart = 0;
  for (int j = 0; j < result.failure->index; j++) {
    if (j < result.failure->index && contents[j] == '\n') {
      lineStart = j + 1;
      line++;
    }
  }
  int lineLen = 0;
  for (int j = lineStart; j < len && contents[j] != '\n'; j++) {
    lineLen = j - lineStart + 1;
  }
  printf("%s error at %s[%i:%i]\n", section, filename, line, result.failure->index - lineStart);
  printf("%.*s\n"
         "%*s%.*s\n",
         lineLen, contents + lineStart, result.failure->index - lineStart, "", result.failure->len,
         "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
         "^^^^^^^^^^^^^^^^^^^");
  printf("%*s'%.*s': %s\n", result.failure->index - lineStart, "", result.failure->len,
         contents + result.failure->index, result.reason);
}

Result parse_function(const char* contents, Function* function, VarList* globals, FunctionList* functions,
                      StrList* str_literals, FILE* output);

int main(const int argc, char** argv) {
  if (argc < 2) {
    if (argv[0] != NULL) {
      printf("Usage: %s <filenames...>\n", argv[0]);
    } else {
      puts("Usage: clamor <filenames..>");
    }
    return 1;
  }

  FileData* files = malloc(sizeof(FileData) * (argc - 1));
  Token* tokens = malloc(sizeof(Token) * (argc - 1));
  StrList* strLiterals = malloc(sizeof(StrList) * (argc - 1));

  for (int i = 1; i < argc; i++) {
    if (filedata_init(&files[i - 1], argv[i]) != 0) {
      exit(-1);
    }
    strlist_init(&strLiterals[i - 1], 1);
  }

  for (int i = 0; i < argc - 1; i++) {
    if (!tokenize(files[i].contents, files[i].len, &tokens[i])) {
      exit(3);
    }
  }

  FunctionList functions;
  VarList globals;
  functionlist_init(&functions, 2);
  varlist_init(&globals, 2);

  FILE* output = fopen("output.asm", "wb");
  for (int i = 0; i < argc - 1; i++) {
    const Result result =
        preprocess_globals(files[i].contents, &tokens[i], &strLiterals[i], &globals, &functions, output);
    if (!successful(result)) {
      fflush(output);
      print_error("Preprocessing", result, files[i].filename, files[i].contents, files[i].len);
      exit(1);
    }
  }

  fputc('\n', output);

  for (int i = 0; i < argc - 1; i++) {
    for (int j = 0; j < functions.len; ++j) {
      const Result result =
          parse_function(files[i].contents, &functions.array[j], &globals, &functions, &strLiterals[i], output);
      if (!successful(result)) {
        fflush(output);
        print_error("Parsing", result, files[i].filename, files[i].contents, files[i].len);
        exit(1);
      }
    }
  }
  fclose(output);

  // fixme
  free(files);
  free(tokens);
  free(strLiterals);

  return 0;
}

Result parse_scope(const char* contents, const Token** token, InstructionTable* table, VarList* globals, FunctionList* functions,
                   StrList* literals, FILE* output);

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

Result invoke_function(const char* contents, const Token** token, InstructionTable* table, VarList* vars, FunctionList* function,
                    Function file, FILE* output);
Result parse_scope(const char* contents, const Token** token, InstructionTable* table, VarList* globals, FunctionList* functions,
                   StrList* literals, FILE* output) {
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
      Token* token1 = *token;
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
    case token_identifier: {
      char* name = token_copy(*token, contents);

      *token = (*token)->next;

      switch ((*token)->type) {
      case token_equals_assign: {
        AstNode node;
        forward_err(parse_statement(contents, token, globals, functions, token_semicolon, &node));
      } break;
      case token_opening_paren: {
        const int indexof = functionlist_indexof(functions, name);
        if (indexof == -1) {
          return failure(*token, "unknown function");
        }
        forward_err(invoke_function(contents, token, table, globals, functions, functions->array[indexof], output));
      } break;
      default:
        token_matches_ext(*token, token_eof, "invalid id sx");
        break;
      }
      free(name);
      break;
    }
    case token_cf_if:
      // token = list->array[(*index)++];
      // if (token->type != opening_paren)
      // {
      //     printf("expected (, found %s", token_name(token->type));
      //     return false;
      // }
      //
      // int reg = register_claim(registers, "@@IF!", Quad);
      // parse_statement(list, index, registers, reg, output);
      //
      // register_release(registers, reg);
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
      //todo ret
      // registers_force_register(registers, node_allocation, rax, output);
      // generate_statement(registers, contents, table, globals, functions, literals, output);
      //
      // fputs("ret\n", output);
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

Result invoke_function(const char* contents, const Token** token, InstructionTable* table, VarList* vars, FunctionList* function,
                    Function file, FILE* output) {
  token_matches(*token, token_opening_paren);
  *token = (*token)->next;
  //FIXME todo
  // for (int i = 0; i < registers->allocations.len; ++i) {
  //   switch (ref_get_register(&frame->allocations.array[i])) {
  //   case rax:
  //   case rdi:
  //   case rsi:
  //   case rdx:
  //   case rcx:
  //   case r8:
  //   case r9:
  //   case r10:
  //   case r11:
  //     stackframe_moveto_stack(frame, &frame->allocations.array[i], file);
  //     break;
  //   default:
  //     break;
  //   }
  // }
  //
  // for (int i = 0; i < function.arguments.len; i++) {
  //   AstNode node;
  //   forward_err(parse_statement(contents, token, globals, functions, token_closing_paren, &node));
  //   if (i < 6) {
  //   } else {
  //   }
  //
  //   if (i + 1 == function.arguments.len) {
  //     token_matches(*token, token_closing_paren);
  //   } else {
  //     token_matches(*token, token_comma);
  //   }
  // }
  // fprintf(file, "call %s\n", function.name);
  return success();
}
