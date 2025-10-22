#pragma once

#include <stddef.h>

#include "lib/spinlock/spinlock.h"

#define DLIST_LOCK_IRQSAVE(list) do { \
    spin_lock_irqsave(&(list).lock); \
} while (0)

#define DLIST_UNLOCK_IRQRESTORE(list) do { \
    spin_unlock_irqrestore(&(list).lock); \
} while (0)

#define DLIST_TYPE(name, node_type) \
struct name {        \
    node_type *head; \
}

#define DLIST_TYPE_SYNCED(name, node_type) \
struct name {               \
    node_type *head;        \
    struct spinlock_t lock; \
}

#define DLIST_HEAD(name, type) \
struct {        \
    type *head; \
} name

#define DLIST_HEAD_SYNCED(name, type) \
struct {                    \
    type *head;             \
    struct spinlock_t lock; \
} name

#define DLIST_INIT(list) do { \
    (list).head = NULL; \
} while (0)

#define DLIST_INIT_SYNCED(list) do { \
    (list).head = NULL;          \
    (list).lock = SPINLOCK_INIT; \
} while (0)

#define DLIST_INSERT(list, node, links) do { \
    if ((list).head == NULL) {              \
        (list).head = (node);               \
            (list).head->links.prev = NULL; \
            (list).head->links.next = NULL; \
        break;                              \
    }                                       \
                                            \
    (node)->links.prev = NULL;              \
    (node)->links.next = (list).head;       \
    (list).head->links.prev = (node);       \
    (list).head = (node);                   \
} while (0)

#define DLIST_INSERT_SYNCED(list, node, links) do { \
    DLIST_LOCK_IRQSAVE(list);        \
    DLIST_INSERT(list, node, links); \
    DLIST_UNLOCK_IRQRESTORE(list);   \
} while (0)

#define DLIST_DELETE(list, node, links) do { \
    if ((node)->links.prev) {                                \
        (node)->links.prev->links.next = (node)->links.next; \
    }                                                        \
                                                             \
    if ((node)->links.next) {                                \
        (node)->links.next->links.prev = (node)->links.prev; \
    }                                                        \
                                                             \
    if ((node) == (list).head) {                             \
        (list).head = (list).head->links.next;               \
    }                                                        \
} while (0)

#define DLIST_DELETE_SYNCED(list, node, links) do { \
    DLIST_LOCK_IRQSAVE(list);        \
    DLIST_DELETE(list, node, links); \
    DLIST_UNLOCK_IRQRESTORE(list);   \
} while (0)
