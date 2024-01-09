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

#endif // PARSE_H
