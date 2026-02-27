#ifndef _INARI_PE_H
#define _INARI_PE_H

#include <misc/types.h>

#include <arch/paging.h>

#define PE_MAGIC 0x00004550
#define PE32_MAGIC 0x010b

struct pe_header {
    uint32_t magic;
    uint16_t machine;
    uint16_t sections_number;
    uint32_t timedate;
    uint32_t symtable_ptr;
    uint32_t syms_count;
    uint16_t optional_header_sz;
    uint16_t characteristics;
};

struct pe32_header {
    uint16_t magic;
    uint8_t major_linker_version;
    uint8_t minor_linker_version;
    uint32_t size_of_code;
    uint32_t size_of_initialized_data;
    uint32_t size_of_uninitialized_data;
    uint32_t address_of_entry_point;
    uint32_t base_of_code;
    uint32_t base_of_data;
    uint32_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;
    uint16_t major_operating_system_version;
    uint16_t minor_operating_system_version;
    uint16_t major_image_version;
    uint16_t minor_image_version;
    uint16_t major_subsystem_version;
    uint16_t minor_subsystem_version;
    uint32_t win32_version_value;
    uint32_t size_of_image;
    uint32_t size_of_headers;
    uint32_t check_sum;
    uint16_t subsystem;
    uint16_t dll_characteristics;
    uint32_t size_of_stack_reserve;
    uint32_t size_of_stack_commit;
    uint32_t size_of_heap_reserve;
    uint32_t size_of_heap_commit;
    uint32_t loader_flags;
    uint32_t number_of_rva_and_sizes;
};

struct pe_image_section {
    char name[8];
    uint32_t virt_sz;
    uint32_t vbase;
    uint32_t phys_sz;
    uint32_t pbase;
    uint32_t pointer_to_relocations;
    uint32_t pointer_to_linenumbers;
    uint16_t number_of_relocations;
    uint16_t number_of_linenumbers;
    uint32_t characteristics;
};

struct pe_symbol {
    char name[8];
    uint32_t value;
    uint16_t section_num;
    uint16_t type;
    uint8_t storage_class;
    uint8_t num_of_aux_syms;
};

int pe_load(pagedir_t *proc_pagedir, void **entrypoint, uint8_t *buf, size_t sz);

#endif