#ifndef AST_H
#define AST_H

#include "list.h"
#include "types.h"

typedef struct
{
    char *name;
    Type type;
} Variable;

LIST_API(Var, var, Variable)

typedef struct
{
    char *name;
    VarList arguments;
    Type retVal;
} Function;

LIST_API(Function, function, Function)

typedef enum
{
    t_add,
    t_sub,
    t_mul,
    t_div,

    t_and,
    t_or,

    t_equal_to,
    t_less_than,
    t_greater_than,
    t_less_than_equal,
    t_greater_than_equal
} OpType;

typedef struct
{
    int left;
    int right;
    OpType opType;
} Operation;

#endif //AST_H
