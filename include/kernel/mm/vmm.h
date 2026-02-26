#ifndef _INARI_VMM_H
#define _INARI_VMM_H

#include <misc/types.h>
#include <misc/list.h>
#include <kernel/inari.h>

#define VMM_PAGE_AVAILABLE     (1 << 0)
#define VMM_PAGE_RESERVED      (1 << 1)
#define VMM_PAGE_USED          (1 << 2)
#define VMM_PAGE_NO_PHYS       (1 << 3)

struct vmm_pool_entry
{
    pagedir_t *pagedir;
    uintptr_t pool_offset;
    struct vmm_page *pages_pool;
    struct list_head list;
};

struct vmm_page {
    uint8_t flags;
};

extern char vmm_pages_pool;
extern char kern_phys_end;

/* Only for upper 1GB */
#define VMM_KERN_SIZE_BYTES   (sizeof(struct vmm_page) * ((0x3fffffff + PAGE_SIZE) >> 12))
#define VMM_KERN_SIZE_PAGES   (VMM_KERN_SIZE_BYTES >> 12)

/* Only for lower 3GB */
#define VMM_USR_SIZE_BYTES    (sizeof(struct vmm_page) * ((0xc0000000 + PAGE_SIZE) >> 12))
#define VMM_USR_SIZE_PAGES    (VMM_USR_SIZE_BYTES >> 12)

#define VMM_KERN_VBASE        ((uintptr_t)&vmm_pages_pool)

#define VMM_IS_RANGE_USERSPACE(start, end)    ((uintptr_t)&kern_phys_end + PAGE_SIZE < ((uintptr_t)start) && ((uintptr_t)end) < VIRTUAL_ADDR - PAGE_SIZE)
#define VMM_IS_PTR_USERSPACE(ptr)    (VMM_IS_RANGE_USERSPACE(ptr, ptr) || ((uintptr_t)ptr) == 0)

int vmm_init(void);
void *vmm_alloc_vmem_kern(size_t npages);
void *vmm_alloc_user(pagedir_t *target_dir, size_t npages);
void *vmm_alloc_kernel(size_t npages);
void vmm_init_directory(pagedir_t *pagedir);
void vmm_cleanup_directory(pagedir_t *pagedir);
int vmm_free_pages(pagedir_t *target_dir, void *base, size_t npages);
int vmm_disable_region(pagedir_t *target_dir, struct reserved_memory region);
int vmm_enable_region(pagedir_t *target_dir, struct reserved_memory region);
int vmm_check_flag(uintptr_t start, uintptr_t end, uint8_t flag);

#endif