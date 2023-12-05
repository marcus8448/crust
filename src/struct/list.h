#ifndef STRUCT_LIST_H
#define STRUCT_LIST_H
#include <assert.h>
#include <stdlib.h>

// if only c had templates
#define LIST_API(name, prefix, type)                                                                                   \
  typedef struct {                                                                                                     \
    type* array;                                                                                                       \
    int len;                                                                                                           \
    int capacity;                                                                                                      \
  } name##List;                                                                                                        \
                                                                                                                       \
  void prefix##list_init(name##List* list, const int capacity);                                                        \
  void prefix##list_add(name##List* list, type value);                                                                 \
  void prefix##list_remove(name##List* list, int index);                                                               \
  type prefix##list_get(const name##List* list, int index);
#define LIST_IMPL(name, prefix, type)                                                                                  \
  void prefix##list_init(name##List* list, const int capacity) {                                                       \
    list->array = malloc(sizeof(type) * capacity);                                                                     \
    list->capacity = capacity;                                                                                         \
    list->len = 0;                                                                                                     \
  }                                                                                                                    \
                                                                                                                       \
  void prefix##list_add(name##List* list, type value) {                                                                \
    if (list->len == list->capacity) {                                                                                 \
      list->capacity *= 2;                                                                                             \
      void* alloc = realloc(list->array, list->capacity * sizeof(type));                                               \
      if (alloc == NULL)                                                                                               \
        abort();                                                                                                       \
      list->array = alloc;                                                                                             \
    }                                                                                                                  \
    list->array[list->len++] = value;                                                                                  \
  }                                                                                                                    \
                                                                                                                       \
  void prefix##list_remove(name##List* list, const int index) {                                                        \
    if (index != list->len - 1) {                                                                                      \
      memcpy(list->array + index, list->array + index + 1, (list->len - index - 1) * sizeof(type));                    \
    }                                                                                                                  \
    list->len--;                                                                                                       \
  }                                                                                                                    \
                                                                                                                       \
  type prefix##list_get(const name##List* list, const int index) {                                                     \
    assert(index < list->len);                                                                                         \
    return list->array[index];                                                                                         \
  }

LIST_API(Ptr, ptr, void*)
LIST_API(Str, str, char*)

int strlist_indexof_after(const StrList* list, int start, const char* value);
int strlist_indexof(const StrList* list, const char* value);

#endif // STRUCT_LIST_H
