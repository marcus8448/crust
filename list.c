#include "list.h"
#include <string.h>

LIST_IMPL(Ptr, ptr, void *)
LIST_IMPL(Str, str, char *)

int strlist_indexof_after(const StrList* list, const int start, const char* value)
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

int strlist_indexof(const StrList* list, const char* value)
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
