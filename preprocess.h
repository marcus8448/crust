#ifndef PREPROCESS_H
#define PREPROCESS_H
#include <stdio.h>

#include "ast.h"
#include "list.h"
#include "token.h"

Result preprocess_globals(char* contents, const Token* token, StrList* strLiterals, VarList* variables, FunctionList* functions, FILE* output);

#endif //PREPROCESS_H
