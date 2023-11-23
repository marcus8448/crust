#include "build/linkedlist.h"
#include <stdlib.h>

#define LINKEDLIST_IMPL(name, prefix, type) \
void prefix##node_init(struct name##Node *node, type value) \
{ \
    node->next = NULL; \
    node->prev = NULL; \
    node->value = value; \
} \
\
void prefix##linkedlist_init(name##LinkedList* list) \
{ \
    list->first = NULL; \
    list->last = NULL; \
} \
\
size_t prefix##linkedlist_size(const name##LinkedList* list) \
{ \
    const struct name##Node* node = list->first; \
    size_t i = 0; \
    while (node != NULL) \
    { \
        node = node->next; \
        i++; \
    } \
    return i; \
} \
\
void prefix##linkedlist_push(name##LinkedList* list, type value) \
{ \
    struct name##Node *node = malloc(sizeof(struct name##Node)); \
    prefix##node_init(node, value); \
    if (list->last == NULL) \
    { \
        list->first = node; \
        list->last = node; \
    } else \
    { \
        list->last->next = node; \
        node->prev = list->last; \
        list->last = node; \
    } \
} \
\
int prefix##linkedlist_pop(name##LinkedList* list) \
{ \
    struct name##Node* removed = list->last; \
    list->last = list->last->prev; \
    if (list->last != NULL) \
    { \
        list->last->next = NULL; \
    } \
    type value = removed->value; \
    free(removed); \
    return value; \
} \
\
int prefix##linkedlist_get(name##LinkedList* list, int index) \
{ \
    const struct name##Node* node = list->first; \
    for (int i = 0; i < index; ++i) \
    { \
        node = node->next; \
    } \
    return node->value; \
}

// LINKEDLIST_IMPL(Int, int, int)
