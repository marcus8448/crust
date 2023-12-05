#ifndef STRUCT_LINKEDLIST_H
#define STRUCT_LINKEDLIST_H
#include <stddef.h>

#define LINKEDLIST_API(name, prefix, type)                                                                             \
  typedef struct {                                                                                                     \
    struct name##Node* head;                                                                                           \
  } name##LinkedList;                                                                                                  \
                                                                                                                       \
  void prefix##linkedlist_init(name##LinkedList* list);                                                                \
  size_t prefix##linkedlist_size(const name##LinkedList* list);                                                        \
  void prefix##linkedlist_push(name##LinkedList* list, type value);                                                    \
  type prefix##linkedlist_pop(name##LinkedList* list);                                                                 \
  type prefix##linkedlist_get(name##LinkedList* list, int index);

LINKEDLIST_API(Int, int, int)

#endif // STRUCT_LINKEDLIST_H
