#ifndef CLAMOR_LIST_H
#define CLAMOR_LIST_H

// if only c had templates
#define LIST_API(name, prefix, type) \
typedef struct \
{ \
    type* array; \
    int len; \
    int capacity; \
} name##List; \
\
void prefix##list_init(name##List* list, const int capacity); \
void prefix##list_add(name##List* list, type value); \
void prefix##list_remove(name##List* list, int index); \
type prefix##list_get(const name##List* list, int index); \
int prefix##list_indexof(const name##List* list, type value); \
int prefix##list_indexof_after(const name##List* list, int start, type value);

LIST_API(Int, int, int)
LIST_API(Str, str, char *)

int strlist_indexof_after_val(const StrList* list, const int start, const char *value);
int strlist_indexof_val(const StrList* list, const char *value);

#endif //CLAMOR_LIST_H
