#pragma once

#include "arch/x86_64/asm_wrappers.h"
#include "lib/spinlock/spinlock.h"

#define DLIST_NO_ATTR

#define DLIST_INSTANCE(name, node_type, attr) \
    attr struct {        \
        node_type *head; \
    } name

#define DLIST_INSTANCE_ATOMIC(name, node_type, attr) \
    attr struct {               \
        node_type *head;        \
        struct spinlock_t lock; \
    } name

#define DLIST_TYPE(name, node_type) \
    struct name {        \
        node_type *head; \
    }

#define DLIST_TYPE_ATOMIC(name, node_type) \
    struct name {               \
        node_type *head;        \
        struct spinlock_t lock; \
    }

#define DLIST_INIT(list) do { \
    (list).head = NULL; \
} while (0)

#define DLIST_INIT_ATOMIC(list) do { \
    (list).head = NULL;          \
    (list).lock = SPINLOCK_INIT; \
} while (0)

#define DLIST_DELETE(list, node, prev, next) do { \
    if ((node)->prev) {                    \
        (node)->prev->next = (node)->next; \
    }                                      \
                                           \
    if ((node)->next) {                    \
        (node)->next->prev = (node)->prev; \
    }                                      \
                                           \
    if ((node) == (list).head) {           \
        (list).head = (list).head->next;   \
    }                                      \
} while (0)

#define DLIST_DELETE_ATOMIC(list, node, prev, next) do { \
    DLIST_LOCK(list);                      \
                                           \
    if ((node)->prev) {                    \
        (node)->prev->next = (node)->next; \
    }                                      \
                                           \
    if ((node)->next) {                    \
        (node)->next->prev = (node)->prev; \
    }                                      \
                                           \
    if ((node) == (list).head) {           \
        (list).head = (list).head->next;   \
    }                                      \
                                           \
    DLIST_UNLOCK(list);                    \
} while (0)

#define DLIST_INSERT(list, node, prev, next) do { \
    if ((list).head == NULL) {    \
        (list).head = (node);     \
        (list).head->prev = NULL; \
        (list).head->next = NULL; \
        break;                    \
    }                             \
                                  \
    (node)->prev = NULL;          \
    (node)->next = (list).head;   \
    (list).head->prev = (node);   \
    (list).head = (node);         \
} while (0)

#define DLIST_INSERT_ATOMIC(list, node, prev, next) do { \
    DLIST_LOCK(list);             \
                                  \
    if ((list).head == NULL) {    \
        (list).head = (node);     \
        (list).head->prev = NULL; \
        (list).head->next = NULL; \
        DLIST_UNLOCK(list);       \
        break;                    \
    }                             \
                                  \
    (node)->prev = NULL;          \
    (node)->next = (list).head;   \
    (list).head->prev = (node);   \
    (list).head = (node);         \
                                  \
    DLIST_UNLOCK(list);           \
} while (0)

#define DLIST_LOCK(list) do { \
    spinlock_acquire(&(list).lock); \
} while (0)

#define DLIST_UNLOCK(list) do { \
    spinlock_release(&(list).lock); \
} while (0)
