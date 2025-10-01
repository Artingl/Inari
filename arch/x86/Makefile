TARGET = x86
AS = nasm
CC = gcc

CFLAGS = -ffreestanding -fno-asynchronous-unwind-tables -fno-stack-protector -nostdlib -Wall -Wextra -I ../../include/ $(shell cat cflags.txt) -std=gnu99

arch_source_c := $(shell find . -name '*.c')
arch_source_s := $(shell find . -name '*.S')

arch_objects_c := $(patsubst ./%.c, ../../build/arch/$(TARGET)/%.o, $(arch_source_c))
arch_objects_s := $(patsubst ./%.S, ../../build/arch/$(TARGET)/%.o, $(arch_source_s))

$(arch_objects_c): ../../build/arch/$(TARGET)/%.o: ./%.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(arch_objects_s): ../../build/arch/$(TARGET)/%.o: ./%.S
	mkdir -p $(dir $@)
	$(AS) -f elf32 $< -o $@

clean:
	-rm -rf kernel.elf

build: $(arch_objects_c) $(arch_objects_s)

build_ld:
	$(LD) -o kernel.elf -T arch.ld -m elf_i386 $(shell find ../../build/ -name *.o)