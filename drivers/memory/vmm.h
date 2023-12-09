#pragma once

#include <kernel/include/C/typedefs.h>

struct page_table
{
    uint32_t pages[1024];
};

struct page_directory
{
    struct page_table *tables[1024];
    uintptr_t tablesPhys[1024];
} __attribute__((aligned(4096)));

void vmm_switch_directory(struct page_directory *dir);
struct page_directory *vmm_current_directory();

struct page_table *vmm_get_table(
    struct page_directory *directory,
    unsigned long table,
    bool allocate);

void vmm_identity(
    struct page_directory *directory,
    void *addr,
    size_t length,
    uint32_t flags);

int vmm_kmmap(
    struct page_directory *directory,
    void *virtual,
    void *real,
    size_t length,
    uint32_t flags);

void vmm_unident(
    struct page_directory *directory,
    void *addr,
    size_t length);

uintptr_t vmm_get_phys(
    struct page_directory *directory,
    void *virtual);


void vmm_init();
size_t vmm_allocated_pages();

extern struct cpu_core;
extern struct regs32;

int page_fault_handler(struct cpu_core *core, struct regs32 *r);