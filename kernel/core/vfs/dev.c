#include <kernel/core/vfs/dev.h>
#include <kernel/kernel.h>
#include <kernel/list/dynlist.h>

dynlist_t devices = {0};
