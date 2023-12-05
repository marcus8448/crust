#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "preprocess.h"
#include "register.h"
#include "struct/list.h"
#include "token.h"

// strdup is fine in C23
#ifdef _MSC_VER
#define strdup _strdup
#endif

typedef struct {
  const char* filename;
  char* contents;
  size_t len;
} FileData;

ValueRef* simple_op(const char* contents, const char* op, StackFrame* frame, VarList* globals, FunctionList* functions,
                    StrList* literals, AstNode* node, FILE* file);

ValueRef* unarry_op(const char* contents, const char* op, StackFrame* frame, VarList* globals, FunctionList* functions,
                    StrList* literals, AstNode* node, FILE* file);

ValueRef* solve_node_allocation(const char* contents, StackFrame* frame, VarList* globals, FunctionList* functions,
                                StrList* literals, AstNode* node, FILE* file);

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

Result parse_scope(const char* contents, const Token** token, StackFrame* frame, VarList* globals, FunctionList* functions,
                   StrList* literals, FILE* output);

Result parse_function(const char* contents, Function* function, VarList* globals, FunctionList* functions,
                      StrList* literals, FILE* output) {
  assert(contents != NULL);
  if (function->start != NULL) {
    StackFrame frame;
    stackframe_init(&frame, NULL);

    fprintf(output, "%s:\n", function->name);
    // fputs("pushq %rbp # save frame pointer\n"
    //       "movq %rsp, %rbp # update frame pointer for this function\n", output);

    stackframe_load_arguments(&frame, function);
    const Token* token = function->start;
    forward_err(parse_scope(contents, &token, &frame, globals, functions, literals, output));

    // fputs("popq %rbp # restore frame pointer\n"
    //       "retq\n", output);

    stackframe_free(&frame, output);
  }
  return success();
}

Result invoke_function(const char* contents, const Token** token, StackFrame* frame, VarList* globals,
                       FunctionList* functions, Function function, FILE* file);

Result parse_scope(const char* contents, const Token** token, StackFrame* frame, VarList* globals, FunctionList* functions,
                   StrList* literals, FILE* output) {
  assert(contents != NULL);
  token_matches(*token, token_opening_curly_brace);
  while ((*token)->next != NULL) {
    switch ((*token = (*token)->next)->type) {
    case token_closing_curly_brace: {
      fputs("ret\n", output);
      return success();
    }
    case token_keyword_let: {
      Variable variable;
      *token = (*token)->next;
      token_matches(*token, token_identifier);
      variable.name = token_copy(*token, contents);

      *token = (*token)->next;
      token_matches(*token, token_colon);

      Type type;
      forward_err(parse_type(contents, token, &type));
      variable.type = type;

      *token = (*token)->next;
      if ((*token)->type == token_semicolon) {
        typekind_width(type.kind);
        stackframe_allocate(frame, variable);
      } else {
        AstNode node;
        token_matches(*token, token_equals_assign);
        *token = (*token)->next;
        forward_err(parse_statement(contents, token, globals, functions, token_semicolon, &node));
        ValueRef* node_allocation = solve_node_allocation(contents, frame, globals, functions, literals, &node, output);
        stackframe_allocate_variable_from(frame, node_allocation, variable, output);
      }
      break;
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
        forward_err(invoke_function(contents, token, frame, globals, functions, functions->array[indexof], output));
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
      ValueRef* ref = solve_node_allocation(contents, frame, globals, functions, literals, &node, output);
      stackframe_force_into_register(frame, ref, rax, output);
      fputs("ret\n", output);
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

Result invoke_function(const char* contents, const Token** token, StackFrame* frame, VarList* globals,
                       FunctionList* functions, const Function function, FILE* file) {
  token_matches(*token, token_opening_paren);
  *token = (*token)->next;
  for (int i = 0; i < frame->allocations.len; ++i) {
    switch (ref_get_register(&frame->allocations.array[i])) {
    case rax:
    case rdi:
    case rsi:
    case rdx:
    case rcx:
    case r8:
    case r9:
    case r10:
    case r11:
      stackframe_moveto_stack(frame, &frame->allocations.array[i], file);
      break;
    default:
      break;
    }
  }

  for (int i = 0; i < function.arguments.len; i++) {
    AstNode node;
    forward_err(parse_statement(contents, token, globals, functions, token_closing_paren, &node));
    if (i < 6) {
    } else {
    }

    if (i + 1 == function.arguments.len) {
      token_matches(*token, token_closing_paren);
    } else {
      token_matches(*token, token_comma);
    }
  }
  fprintf(file, "call %s\n", function.name);
  return success();
}

ValueRef* solve_node_allocation(const char* contents, StackFrame* frame, VarList* globals, FunctionList* functions,
                                StrList* literals, AstNode* node, FILE* file) {
  switch (node->type) {
  case op_nop:
    exit(112);
  case op_function:
    break;
  case op_array_index:
    break;
  case op_comma:
    stackframe_free_ref(frame, solve_node_allocation(contents, frame, globals, functions, literals, node->left, file));
    return solve_node_allocation(contents, frame, globals, functions, literals, node->right, file);
  case op_unary_negate:
    return unarry_op(contents, "neg", frame, globals, functions, literals, node, file);
  case op_unary_plus:
    return solve_node_allocation(contents, frame, globals, functions, literals, node->inner, file);
  case op_unary_addressof:
    break;
  case op_unary_derefernce:
    break;
  case op_unary_not:
    break;
  case op_unary_bitwise_not:
    return unarry_op(contents, "neg", frame, globals, functions, literals, node, file);
  case op_add:
    return simple_op(contents, "add", frame, globals, functions, literals, node, file);
  case op_subtract:
    return simple_op(contents, "sub", frame, globals, functions, literals, node, file);
  case op_multiply: // fixme
    return simple_op(contents, "imul", frame, globals, functions, literals, node, file);
  case op_divide:
    // fixme div is not simple
    break;
  case op_modulo:
    // fixme div is not simple
    break;
  case op_bitwise_or:
    return simple_op(contents, "or", frame, globals, functions, literals, node, file);
  case op_bitwise_xor:
    return simple_op(contents, "xor", frame, globals, functions, literals, node, file);
  case op_bitwise_and:
    return simple_op(contents, "and", frame, globals, functions, literals, node, file);
  case op_assignment:
    break;
  case op_and:
    break;
  case op_or:
    break;
  case op_deref_member_access:
    break;
  case op_member_access:
    break;
  case op_bitwise_left_shift:
    break;
  case op_bitwise_right_shift:
    break;
  case op_compare_equals:
    break;
  case op_compare_not_equals:
    break;
  case op_less_than:
    break;
  case op_greater_than:
    break;
  case op_less_than_equal:
    break;
  case op_greater_than_equal: {
  } break;
  case op_value_constant: {
    ValueRef* alloc = malloc(sizeof(ValueRef));
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
    ValueRef* ref = stackframe_get_by_token(frame, contents, node->token);
    if (ref == NULL) {
      exit(99);
    }
    return ref;
  }
  };
  exit(23);
}

ValueRef* simple_op(const char* contents, const char* op, StackFrame* frame, VarList* globals, FunctionList* functions,
                    StrList* literals, AstNode* node, FILE* file) {
  ValueRef* left = solve_node_allocation(contents, frame, globals, functions, literals, node->left, file);
  ValueRef* right = solve_node_allocation(contents, frame, globals, functions, literals, node->right, file);

  stackframe_allocate_temporary_from(frame, &left, file);

  char* mnemonicL = allocation_mnemonic(left);
  char* mnemonicR = allocation_mnemonic(right);
  fprintf(file, "%s%c %s, %s\n", op, get_suffix(typekind_width(left->value_type.kind)), mnemonicR, mnemonicL);
  stackframe_free_ref(frame, right);
  free(mnemonicL);
  free(mnemonicR);
  return left;
}

ValueRef* unarry_op(const char* contents, const char* op, StackFrame* frame, VarList* globals, FunctionList* functions,
                    StrList* literals, AstNode* node, FILE* file) {
  ValueRef* ref = solve_node_allocation(contents, frame, globals, functions, literals, node->inner, file);
  stackframe_allocate_temporary_from(frame, &ref, file);
  char* mnemonic = allocation_mnemonic(ref);
  fprintf(file, "%s%c %s\n", op, get_suffix(typekind_width(ref->value_type.kind)), mnemonic);
  free(mnemonic);
  return ref;
}
