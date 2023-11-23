#ifndef LINKEDLIST_H
#define LINKEDLIST_H
#include <stddef.h>

#define LINKEDLIST_API(name, prefix, type) \
struct name##Node \
{ \
    struct name##Node *next; \
    struct name##Node *prev; \
    type value; \
}; \
\
typedef struct { \
    struct name##Node* first; \
    struct name##Node *last; \
} name##LinkedList; \
\
void prefix##linkedlist_init(name##LinkedList *list); \
size_t prefix##linkedlist_size(const name##LinkedList *list); \
void prefix##linkedlist_push(name##LinkedList *list, type value); \
type prefix##linkedlist_pop(name##LinkedList *list); \
type prefix##linkedlist_get(name##LinkedList *list, int index);

// LINKEDLIST_API(Int, int, int)

#endif //LINKEDLIST_H
