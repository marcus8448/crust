#ifndef CLAMOR_LIST_H
#define CLAMOR_LIST_H
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

#define LIST_IMPL(name, prefix, type) \
void prefix##list_init(name##List* list, const int capacity) \
{ \
    list->array = malloc(sizeof(type) * capacity); \
    list->capacity = capacity; \
    list->len = 0; \
} \
\
void prefix##list_add(name##List* list, type value) \
{ \
    if (list->len == list->capacity) \
    { \
        list->capacity *= 2; \
        void* alloc = realloc(list->array, list->capacity * sizeof(type)); \
        if (alloc == NULL) abort(); \
        list->array = alloc; \
    } \
    list->array[list->len++] = value; \
} \
\
void prefix##list_remove(name##List* list, const int index) \
{ \
    if (index != list->len - 1) \
    { \
        memcpy(list->array + index, list->array + index + 1, (list->len - index - 1) * sizeof(type)); \
    } \
    list->len--; \
} \
\
type prefix##list_get(const name##List* list, const int index) \
{ \
    assert(index < list->len); \
    return list->array[index]; \
} \
\
int prefix##list_indexof(const name##List* list, type value) \
{ \
    for (int i = 0; i < list->len; i++) \
    { \
        if (list->array[i] == value) \
        { \
            return i; \
        } \
    } \
    return -1; \
} \
\
int prefix##list_indexof_after(const name##List* list, const int start, type value) \
{ \
    for (int i = start; i < list->len; i++) \
    { \
        if (list->array[i] == value) \
        { \
            return i; \
        } \
    } \
    return -1; \
}

LIST_API(Int, int, int)
LIST_API(Str, str, char *)

int strlist_indexof_after_val(const StrList* list, int start, const char *value);
int strlist_indexof_val(const StrList* list, const char *value);

#endif //CLAMOR_LIST_H
