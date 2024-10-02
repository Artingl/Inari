#pragma once

#include <kernel/libc/typedefs.h>

#define BLOCK_DEVICE 0x00
#define CHR_DEVICE   0x01

// r/w position, size, buffer
typedef void(*write_handler)(uintptr_t, uint32_t, void*);
typedef void(*read_handler)(uintptr_t, uint32_t, void*);

struct dev_ref
{
    uint32_t dev_type;

    write_handler w_hndl;
    read_handler r_hndl;
};
