#include <kernel/list/dynlist.h>
#include <kernel/driver/memory/memory.h>
#include <kernel/kernel.h>


uintptr_t __dynlist_pop(dynlist_t *list)
{
    uintptr_t elem = 0;
    spinlock_acquire(&list->lock);
    if (list->size == 0)
    {
        // the list is empty
        spinlock_release(&list->lock);
        return 0;
    }

    // get the last element and remove it
    elem = list->items[list->size - 1];
    spinlock_release(&list->lock);
    __dynlist_remove(list, list->size - 1);

    return elem;
}

int __dynlist_append(dynlist_t *list, uintptr_t data)
{
    int idx = 0;
    spinlock_acquire(&list->lock);
    if (list->size == 0)
    {
        // the list is empty, initialize it
        list->items = (uintptr_t*)kcalloc(sizeof(uintptr_t), ++list->size);
    }
    else {
        // expand the list
        idx = list->size++;
        list->items = (uintptr_t*)krealloc((void*)list->items, sizeof(uintptr_t) * list->size);
    }

    // store the data inside the list
    list->items[idx] = data;

    spinlock_release(&list->lock);
    return idx;
}

int __dynlist_remove(dynlist_t *list, int idx)
{
    size_t i, offset = 0;
    spinlock_acquire(&list->lock);
    if (list->size == 0)
    {
        // the list is empty
        goto end;
    }

    // create new list and store all data there except the data we want to delete
    uintptr_t *new_list = (uintptr_t*)kcalloc(sizeof(uintptr_t), list->size - 1);

    for (i = 0; i < list->size; i++)
    {
        if (i != idx)
        {
            new_list[i - offset] = list->items[i];
        }
        else offset = 1;
    }

    list->size--;
    kfree(list->items);
    list->items = new_list;

    if (list->size == 0)
    {
        kfree(list->items);
        list->items = NULL;
    }
end:
    spinlock_release(&list->lock);
    return 0;
}

uintptr_t __dynlist_get(dynlist_t *list, int idx)
{
    uintptr_t elem = 0;
    spinlock_acquire(&list->lock);
    if (list->size < idx)
    {
        // the list is empty
        goto end;
    }

    elem = list->items[idx];
end:
    spinlock_release(&list->lock);
    return elem;
}

int __dynlist_set(dynlist_t *list, int idx, uintptr_t data)
{
    int result = 0;
    spinlock_acquire(&list->lock);
    if (list->size == 0 || list->size < idx)
    {
        // the list is empty
        result = 1;
        goto end;
    }

    list->items[idx] = data;
end:
    spinlock_release(&list->lock);
    return result;
}

int __dynlist_insert(dynlist_t *list, int idx, uintptr_t data)
{
    size_t old_size = list->size, i, offset = 0;
    uintptr_t *new_list;
    spinlock_acquire(&list->lock);
    if (list->size == 0)
    {
        // the list is empty, initialize it
        list->size = idx + 1;
        list->items = (uintptr_t*)kcalloc(sizeof(uintptr_t), list->size);
    }
    else if (list->size < idx + 1) {
        // expand the list
        list->size = idx + 1;
        list->items = (uintptr_t*)krealloc((void*)list->items, sizeof(uintptr_t) * list->size);

        // fill the new allocated area with 0
        for (i = old_size; i < list->size; i++)
            list->items[i] = 0;
    }
    else {
        // make new list with +1 element and free the spot at idx
        new_list = (uintptr_t*)kcalloc(sizeof(uintptr_t), ++list->size);

        for (i = 0; i < list->size; i++)
        {
            // set element in the new list, and skip idx element
            if (i != idx)
            {
                new_list[i] = list->items[i - offset];
            }
            else offset = 1;
        }

        kfree(list->items);
        list->items = new_list;
    }

    // store the data inside the list
    list->items[idx] = data;

    spinlock_release(&list->lock);
    return idx;
}
