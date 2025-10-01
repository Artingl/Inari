#include <kernel/inari.h>
#include <kernel/console/console.h>
#include <kernel/console/earlycon.h>
#include <kernel/errno.h>

#include <misc/list.h>
#include <misc/string.h>

struct list_head console_list;

int console_register(console_dev_t *dev)
{}

int console_unregister(console_dev_t *dev)
{

}