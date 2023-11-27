#ifndef PREPROCESS_H
#define PREPROCESS_H
#include <stdio.h>

#include "list.h"
#include "token.h"

Result preprocess_globals(char *contents, const Token *token, StrList *strLiterals, FILE* output);

#endif //PREPROCESS_H
