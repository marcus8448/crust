#include "list.h"

LIST_IMPL(Int, int, int)
LIST_IMPL(Str, str, char *)

int strlist_indexof_after_val(const StrList* list, const int start, const char *value)
{
    for (int i = start; i < list->len; i++)
    {
        if (strcmp(list->array[i], value) == 0)
        {
            return i;
        }
    }
    return -1;
}

int strlist_indexof_val(const StrList* list, const char *value)
{
    for (int i = 0; i < list->len; i++)
    {
        if (strcmp(list->array[i], value) == 0)
        {
            return i;
        }
    }
    return -1;
}
