#ifndef _INARI_MISC_LIST_H
#define _INARI_MISC_LIST_H

/* intrusive doubly linked list node */
struct list_head {
    struct list_head *next, *prev;
};

/* initialize a list head */
#define LIST_HEAD_INIT(name)                                                                                           \
    { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

/* runtime init */
static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

/* internal helper */
static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/* add new node after head (stack style) */
static inline void list_add(struct list_head *new, struct list_head *head) { __list_add(new, head, head->next); }

/* add new node before head (queue style) */
static inline void list_add_tail(struct list_head *new, struct list_head *head) { __list_add(new, head->prev, head); }

/* internal helper */
static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

/* remove node from list */
static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    entry->next = (void *)0;
    entry->prev = (void *)0;
}

static inline int list_empty(const struct list_head *head) { return head->next == head; }

/* moves a specific element to the start of the list */
static inline int list_move_to_front(struct list_head *entry, struct list_head *head) {
    if (head->next == entry) {
        return -1;
    }
    __list_del(entry->prev, entry->next);
    list_add(entry, head);
    return 0;
}

/* moves a specific element to the very end of the list */
static inline int list_move_to_end(struct list_head *entry, struct list_head *head) {
    if (head->prev == entry) {
        return -1;
    }
    __list_del(entry->prev, entry->next);
    list_add_tail(entry, head);
    return 0;
}

/* turn list_head pointer back into struct pointer */
#define list_entry(ptr, type, member) ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/* iterate over list */
#define list_for_each(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

/* iterate over list backwards */
#define list_for_each_prev(pos, head) for (pos = (head)->prev; pos != (head); pos = pos->prev)

/* safe iteration when you may delete the current node */
#define list_for_each_safe(pos, n, head) for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

#endif
