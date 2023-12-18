#ifndef AST_H
#define AST_H

#include "struct/list.h"
#include "types.h"

typedef struct {
  char* name;
  Type type;
} Variable;

LIST_API(Var, var, Variable)

typedef struct {
  char* name;
  VarList arguments;
  Type retVal;
  const Token* start;
} Function;

LIST_API(Function, function, Function)

void function_init(Function* function);

int functionlist_indexof(const FunctionList* list, const char* name);
int functionlist_indexof_tok(const FunctionList* list, const char* contents, const Token* token);
int varlist_indexof(const VarList* list, const char* name);

typedef enum {
  op_nop,
  // (args)
  op_function,
  // []
  op_array_index,

  // ,
  op_comma,

  // <nil> - <expression>
  op_unary_negate,
  // <nil> + <expression> (no effect)
  op_unary_plus,
  // &
  op_unary_addressof,
  // *
  op_unary_derefernce,
  // !
  op_unary_not,
  // ~
  op_unary_bitwise_not,

  // +
  op_add,
  // -
  op_subtract,
  // *
  op_multiply,
  // /
  op_divide,
  // %
  op_modulo,

  // |
  op_bitwise_or,
  // ^
  op_bitwise_xor,
  // &
  op_bitwise_and,

  // =
  op_assignment,

  // &&
  op_and,
  // ||
  op_or,
  // ->
  op_deref_member_access,
  // .
  op_member_access,
  // <<
  op_bitwise_left_shift,
  // >>
  op_bitwise_right_shift,

  // ==
  op_compare_equals,
  // !=
  op_compare_not_equals,
  // <
  op_less_than,
  // >
  op_greater_than,
  // <=
  op_less_than_equal,
  // >=
  op_greater_than_equal,

  // _ as TYPE
  op_cast,

  // <number>
  op_value_constant,
  // <identifier>
  op_value_variable,

  cf_if,
  cf_while
} AstNodeType;

int ast_operand_count(AstNodeType type);
int ast_precedence(AstNodeType type);

typedef struct AstNode {
  AstNodeType type;
  const Token* token;

  union {
    // control flow
    struct {
      struct AstNode** children;
      int count;
    };

    struct {
      Type val_type;
      union {
        // binary operators
        struct {
          struct AstNode* left;
          struct AstNode* right;
        };

        struct {
          struct AstNode* inner;
        };
      };
    };
  };
} AstNode;

Result parse_value(const char* contents, const Token** token, VarList* globals, FunctionList* functions, AstNode* node);
Result parse_statement(const char* contents, const Token** token, VarList* globals, FunctionList* functions,
                       TokenType until, AstNode* node);
Result parse_args(const char* contents, const Token** token, Function* function, VarList* vars, FunctionList* functions,
                  AstNode* inner);

#endif // AST_H
