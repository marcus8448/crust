#ifndef PARSE_H
#define PARSE_H
#include "ast.h"
#include "ir.h"
#include "result.h"

#include <stdio.h>

Result parse_function(const char *contents, Function *function, VarList *globals, FunctionList *functions,
                      StrList *str_literals, FILE *output);

Result parse_scope(const char *contents, const Token **token, VarList *globals, FunctionList *functions,
                   StrList *literals, AstNodeList *nodes);

Result invoke_function(const char *contents, const Token **token, InstructionTable *table, VarList *vars,
                       FunctionList *function, Function file, FILE *output);

#endif // PARSE_H
