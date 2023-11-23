#include "stack.h"
#include <stdlib.h>

#define STACK_IMPL(name, prefix, type) \
void prefix##node_init(struct name##Node *node, type value) \
{ \
    node->next = NULL; \
    node->value = value; \
} \
\
void prefix##stack_init(name##Stack* stack) \
{ \
    stack->head = NULL; \
} \
\
size_t prefix##stack_size(const name##Stack* stack) \
{ \
    const struct name##Node* node = stack->head; \
    size_t i = 0; \
    while (node != NULL) \
    { \
        node = node->next; \
        i++; \
    } \
    return i; \
} \
\
void prefix##stack_push(name##Stack* stack, type value) \
{ \
    struct name##Node *node = malloc(sizeof(struct name##Node)); \
    prefix##node_init(node, value); \
    if (stack->head == NULL) \
    { \
        stack->head = node; \
    } else \
    { \
        node->next = stack->head; \
        stack->head = node; \
    } \
} \
\
type prefix##stack_pop(name##Stack* stack) \
{ \
    struct name##Node *node = stack->head; \
    type value = stack->head->value; \
    stack->head = stack->head->next; \
    free(node); \
    return value; \
} \
\
int prefix##stack_get(name##Stack* stack, int index) \
{ \
    const struct name##Node* node = stack->head; \
    for (int i = 0; i < index; ++i) \
    { \
        node = node->next; \
    } \
    return node->value; \
}

STACK_IMPL(Int, int, int)
