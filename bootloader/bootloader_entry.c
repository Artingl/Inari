#include <kernel/kernel.h>

#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/math.h>

#include <drivers/video/video.h>
#include <drivers/video/vbe/vbe_drv.h>
#include <drivers/impl.h>

#include <bootloader/lower.h>

extern void *_hi_start_marker;
extern void *_hi_end_marker;

extern void *_kernel_phys_start;
extern void *_kernel_phys_end;

extern void *_lo_start_marker;
extern void *_lo_end_marker;

extern void *lo_early_heap_top;
extern void *lo_early_heap;
extern void *lo_early_heap_end;

extern void *hi_stack_top;
extern void *hi_stack_bottom;

struct page_directory core_directory __attribute__((aligned(4096), section(".lo_rodata")));

extern void lo_update_stack_and_jump();

LKERN int lower_initialize_vbe(struct vbe_block_info *vbe_block, struct vbe_mode *vbe_mode);
LKERN void jump_to_kernel();

extern multiboot_info_t *_lower_multiboot_info_struct;

LKERN void lo_kmain(multiboot_info_t *multiboot)
{
    size_t i;

    lower_vga_init();
    early_alloc_setup(multiboot);

    lower_vga_print(MESSAGES_POOL[MSG_FILLING]);

    // Allocate all page tables, so we'd not need to make page table allocator for the kernel
    for (i = 0; i < 1024; i++)
    {
        // core_directory.tablesPhys[i] = PAGE_RW;
        paging_get_table(i);
    }

    lower_vga_add(MESSAGES_POOL[MSG_DONE]);
    lower_vga_print(MESSAGES_POOL[MSG_IDENTIFY]);

    // map all necessary addresses
    paging_ident(0, (size_t)(&_lo_end_marker), PAGE_RW);
    paging_ident((void *)*(&lo_early_heap_top), (size_t)*(&lo_early_heap) + PAGE_SIZE * 32, PAGE_RW);

    // mmap higher kernel
    paging_mmap(
        &_hi_start_marker,
        &_kernel_phys_start,
        (size_t)(((uintptr_t)&_kernel_phys_end) - ((uintptr_t)&_kernel_phys_start)),
        PAGE_RW);

    lower_vga_add(MESSAGES_POOL[MSG_DONE]);

    // enable paging
    switch_page(&core_directory);

    bl_debug(MESSAGES_POOL[MSG_PASS_CONTROL]);

    // update stack pointer for the higher kernel and call function to jump to it
    lo_update_stack_and_jump();
}

LKERN void jump_to_kernel()
{
    // Setup the payload to be passed to the higher kernel
    struct kern_video_vbe vbe_service = {
        .framebuffer_back = NULL
    };

    struct kernel_payload kernel_payload = {
        .core_directory = &core_directory,

        .bootloader = _lower_multiboot_info_struct->boot_loader_name,
        .cmdline = _lower_multiboot_info_struct->cmdline,

        .video_service = {
            .mode = VIDEO_VBE,
            .info_structure = &vbe_service,
        },

        .mmap = (struct kernel_mmap_entry *)_lower_multiboot_info_struct->mmap_addr,
        .mmap_length = _lower_multiboot_info_struct->mmap_length / sizeof(struct kernel_mmap_entry),
    };

    // Initialize higher display resolution for the kernel
    if (lower_initialize_vbe(&vbe_service.vbe_block, &vbe_service.vbe_mode) == 0)
    {
        kernel_payload.video_service.mode = VIDEO_VBE;
        kernel_payload.video_service.info_structure = &vbe_service;
    }

    // call the higher kernel
    kmain(&kernel_payload);

    __asm__ volatile("hlt");
}

LKERN int lower_initialize_vbe(
    struct vbe_block_info *vbe_block, struct vbe_mode *mode)
{
    if (_lower_multiboot_info_struct->flags & MULTIBOOT_INFO_VIDEO_INFO)
    { // multiboot has set up VBE for us. no need to do this manually
        mode->mode_id = _lower_multiboot_info_struct->vbe_mode;

        memcpy(vbe_block, _lower_multiboot_info_struct->vbe_control_info, sizeof(struct vbe_block_info));
        memcpy(&mode->info, _lower_multiboot_info_struct->vbe_mode_info, sizeof(struct vbe_mode_info));

        return 0;
    }
    else
    {
        // 0x80000
        // TODO: fix it...
        return 1;

        struct regs16 r;
        r.ax = VBE_BIOS_INFO;
        r.es = 0x8000;
        r.di = vbe_block;
        int32(0x10, &r);

        if (r.ax != 0x4f)
        { // error response from the BIOS
            return 1;
        }

        if (vbe_block->vbe_signature[0] != 'V' ||
            vbe_block->vbe_signature[1] != 'E' ||
            vbe_block->vbe_signature[2] != 'S' ||
            vbe_block->vbe_signature[3] != 'A')
        { // got invalid VESA signature
            return 2;
        }

#define DIFF(v0, v1) ((v0) - (v1))

        // query thru all available modes to find closest to the 1280x720
        size_t i;
        uint16_t *modes;
        uint32_t width = 1280, height = 720;

        int pixdiff, bestpixdiff = -1;
        struct vbe_mode *query_mode = (struct vbe_mode *)(0x80000 + sizeof(struct vbe_block_info) * 2);

        modes = (uint16_t *)REAL_PTR(vbe_block->video_mode_ptr);
        for (i = 0; modes[i] != 0xFFFF; ++i)
        {
            query_mode->mode_id = modes[i];

            r.ax = 0x4F01;
            r.cx = modes[i];
            r.es = SEG(&query_mode->info);
            r.di = OFF(&query_mode->info);
            int32(0x10, &r); // Get Mode Info

            if (r.ax != 0x4F)
                continue;
            if (query_mode->info.bpp != 32)
                continue;

            // Check if this is exactly the mode we're looking for
            if (width == query_mode->info.width && height == query_mode->info.height)
            {
                memcpy(mode, query_mode, sizeof(struct vbe_mode));
                bestpixdiff = 1;
                break;
            }

            // Otherwise, compare to the closest match so far, remember if best
            pixdiff = DIFF(query_mode->info.width * query_mode->info.height, width * height);
            if (bestpixdiff > pixdiff || bestpixdiff == -1)
            {
                memcpy(mode, query_mode, sizeof(struct vbe_mode));
                bestpixdiff = pixdiff;
            }
        }

        if (bestpixdiff == -1)
        { // unable to find mode
            return 3;
        }

        // set the video mode
        r.ax = 0x4F02;
        r.bx = mode->mode_id | 0x4000;
        r.es = 0;
        r.di = 0;
        // int32(0x10, &r);
    }

    return 0;
}

LKERN void paging_mmap(void *offset, void *ptr, size_t size, uint32_t flags)
{
    void *initial_addr = offset;

    for (; (uintptr_t)offset < (uintptr_t)(initial_addr + size); offset += PAGE_SIZE)
    {
        unsigned long pdindex = (unsigned long)offset >> 22;
        unsigned long ptindex = (unsigned long)offset >> 12 & 0x03FF;

        struct page_table *table = paging_get_table(pdindex);

        table->pages[ptindex] = ((unsigned long)ptr) | (flags & 0xFFF) | PAGE_PRESENT;
        ptr += PAGE_SIZE;
    }
}

LKERN void paging_ident(void *ptr, size_t size, uint32_t flags)
{
    void *initial_addr = ptr;

    for (; (uintptr_t)ptr < (uintptr_t)(initial_addr + size); ptr += PAGE_SIZE)
    {
        unsigned long pdindex = (unsigned long)ptr >> 22;
        unsigned long ptindex = (unsigned long)ptr >> 12 & 0x03FF;

        struct page_table *table = paging_get_table(pdindex);

        table->pages[ptindex] = ((unsigned long)ptr) | (flags & 0xFFF) | PAGE_PRESENT;
    }
}

LKERN struct page_table *paging_get_table(unsigned long table)
{
    if (!(core_directory.tablesPhys[table] & PAGE_PRESENT))
    {
        // allocate new directory
        core_directory.tables[table] = early_alloc(sizeof(struct page_table));
        core_directory.tablesPhys[table] = ((uintptr_t)core_directory.tables[table]) | 3; // PRESENT, RW
    }

    // the page table is already allocated, return it
    return core_directory.tables[table];
}

LKERN void switch_page(struct page_directory *dir)
{
    __asm__ volatile("mov %0, %%cr3" ::"r"(&dir->tablesPhys));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}
