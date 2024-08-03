#pragma once

#include <kernel/include/typedefs.h>

#include <kernel/kernel.h>
#include <kernel/arch/i686/memory/vmm.h>

void memory_init(
    struct page_directory *bsp_directory,
    struct kernel_mmap_entry *mmap_list,
    size_t mmap_list_length);
