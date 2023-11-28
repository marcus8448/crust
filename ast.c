#include "ast.h"
#include <string.h>

LIST_IMPL(Var, var, Variable);
LIST_IMPL(Function, function, Function);

void function_init(Function* function)
{
    varlist_init(&function->arguments, 2);
    function->name = NULL;
    function->retVal.kind = 0;
    function->retVal.inner = NULL;
}

int functionlist_indexof(const FunctionList *list, const char* name)
{
    for (int i = 0; i < list->len; i++)
    {
        if (strcmp(list->array[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

int varlist_indexof(const VarList* list, const char* name)
{
    for (int i = 0; i < list->len; i++)
    {
        if (strcmp(list->array[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}
