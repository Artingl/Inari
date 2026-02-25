TARGET = x86
AS = nasm
CC = i686-elf-gcc
LD = i686-elf-ld

LDFLAGS = -no-pie -nostdlib
CFLAGS = --include "../../build/config.h" -I ../../include/ $(shell cat cflags.txt) $(shell cat ../../cflags.txt)

arch_source_c := $(shell find . -name '*.c')
arch_source_s := $(shell find . -name '*.S')

arch_objects_c := $(patsubst ./%.c, ../../build/arch/$(TARGET)/%.o, $(arch_source_c))
arch_objects_s := $(patsubst ./%.S, ../../build/arch/$(TARGET)/%.o, $(arch_source_s))

$(arch_objects_c): ../../build/arch/$(TARGET)/%.o: ./%.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(arch_objects_s): ../../build/arch/$(TARGET)/%.o: ./%.S
	mkdir -p $(dir $@)
	$(AS) -g -F dwarf -f elf32 $< -o $@

clean:
	-rm -rf kernel.elf

build: $(arch_objects_c) $(arch_objects_s)

build_ld:
	$(LD) $(LDFLAGS) -o kernel.elf -T arch.ld -m elf_i386 $(shell find ../../build/ -name *.o)