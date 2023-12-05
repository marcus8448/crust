#include "linkedlist.h"
#include <stdlib.h>

#define LINKEDLIST_IMPL(name, prefix, type)                                                                            \
  struct name##Node {                                                                                                  \
    struct name##Node* next;                                                                                           \
    type value;                                                                                                        \
  };                                                                                                                   \
                                                                                                                       \
  static void prefix##node_init(struct name##Node* node, type value) {                                                 \
    node->next = NULL;                                                                                                 \
    node->value = value;                                                                                               \
  }                                                                                                                    \
                                                                                                                       \
  void prefix##linkedlist_init(name##LinkedList* list) {                                                               \
    list->head = NULL;                                                                                                 \
  }                                                                                                                    \
                                                                                                                       \
  size_t prefix##linkedlist_size(const name##LinkedList* list) {                                                       \
    const struct name##Node* node = list->head;                                                                        \
    size_t i = 0;                                                                                                      \
    while (node != NULL) {                                                                                             \
      node = node->next;                                                                                               \
      i++;                                                                                                             \
    }                                                                                                                  \
    return i;                                                                                                          \
  }                                                                                                                    \
                                                                                                                       \
  void prefix##linkedlist_push(name##LinkedList* list, type value) {                                                   \
    struct name##Node* node = malloc(sizeof(struct name##Node));                                                       \
    prefix##node_init(node, value);                                                                                    \
    if (list->head == NULL) {                                                                                          \
      list->head = node;                                                                                               \
    } else {                                                                                                           \
      node->next = list->head;                                                                                         \
      list->head = node;                                                                                               \
    }                                                                                                                  \
  }                                                                                                                    \
                                                                                                                       \
  int prefix##linkedlist_pop(name##LinkedList* list) {                                                                 \
    struct name##Node* removed = list->head;                                                                           \
    list->head = list->head->next;                                                                                     \
    type value = removed->value;                                                                                       \
    free(removed);                                                                                                     \
    return value;                                                                                                      \
  }                                                                                                                    \
                                                                                                                       \
  int prefix##linkedlist_get(name##LinkedList* list, int index) {                                                      \
    const struct name##Node* node = list->head;                                                                        \
    for (int i = 0; i < index; ++i) {                                                                                  \
      node = node->next;                                                                                               \
    }                                                                                                                  \
    return node->value;                                                                                                \
  }

LINKEDLIST_IMPL(Int, int, int)
