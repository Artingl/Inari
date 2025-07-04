#pragma once

#include <kernel/libc/typedefs.h>

#define PD_SIZE (sizeof(struct page_directory) / PAGE_SIZE)

#define vmm_get_table(dir, tbl) (&dir->tables[(tbl)])

struct page_table
{
    uint32_t pages[1024];
};

struct page_directory
{
    struct page_table tables[1024];
    uintptr_t tables_phys[1024];
} __attribute__((aligned(4096)));

void vmm_switch_directory(struct page_directory *dir);
struct page_directory *vmm_current_directory();
    
uintptr_t vmm_get_phys(
    struct page_directory *directory,
    void *virtual);

int vmm_kmmap(
    struct page_directory *directory,
    void *virtual,
    void *real,
    size_t length,
    uint32_t flags);

int vmm_unident(
    struct page_directory *directory,
    void *addr,
    size_t length);

struct page_directory *vmm_kernel_directory();
struct page_directory *vmm_fork_directory(struct page_directory *target);
void vmm_deallocate_directory(struct page_directory *pd);

void vmm_page_invalidate(uintptr_t page);
int vmm_init(struct page_directory *bsp_directory);

size_t vmm_total_pages();
size_t vmm_allocated_pages();

struct cpu_core;
struct regs32;

int page_fault_handler(struct cpu_core *core, struct regs32 *r);