#ifndef STACK_H
#define STACK_H
#include <stddef.h>

#define STACK_API(name, prefix, type) \
struct name##Node \
{ \
    struct name##Node *next; \
    type value; \
}; \
\
typedef struct { \
    struct name##Node* head; \
} name##Stack; \
\
void prefix##stack_init(name##Stack *stack); \
size_t prefix##stack_size(const name##Stack *stack); \
void prefix##stack_push(name##Stack *stack, type value); \
type prefix##stack_pop(name##Stack *stack); \
type prefix##stack_get(name##Stack *stack, int index);

STACK_API(Int, int, int)

#endif //STACK_H
