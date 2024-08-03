TARGET = i686

CFLAGS = -std=c99 -O2 --include "config.h" -I . -fno-stack-protector -nostdlib -ffreestanding -c -m32 -mfpmath=387 -Werror -Wno-stringop-overflow
CC = $(HOME)/opt/cross/bin/i386-elf-gcc
LD = $(HOME)/opt/cross/bin/i386-elf-ld
ASM = nasm

bootloader_asm_source := $(shell find kernel/arch/$(TARGET)/boot/asm/ -maxdepth 1 -name *.asm)
bootloader_asm_objects := $(patsubst kernel/arch/$(TARGET)/boot/asm/%.asm, build/kernel/arch/$(TARGET)/boot/asm/%.o, $(bootloader_asm_source))

$(bootloader_asm_objects): build/kernel/arch/$(TARGET)/boot/asm/%.o : kernel/arch/$(TARGET)/boot/asm/%.asm
	mkdir -p $(dir $@) && \
	$(ASM) -ikernel/arch/$(TARGET)/boot/asm -f elf32 \
		$(patsubst build/kernel/arch/$(TARGET)/boot/asm/%.o, kernel/arch/$(TARGET)/boot/asm/%.asm, $@) -o $@

modules_source := $(shell find modules/ -name *.c)
modules_objects := $(patsubst modules/%.c, build/modules/%.o, $(modules_source))

$(modules_objects): build/modules/%.o : modules/%.c
	mkdir -p $(dir $@) && \
	$(CC) $(CFLAGS) $(patsubst build/modules/%.o, modules/%.c, $@) -o $@

kernel_source := $(shell find kernel/ -name *.c)
kernel_objects := $(patsubst kernel/%.c, build/kernel/%.o, $(kernel_source))

$(kernel_objects): build/kernel/%.o : kernel/%.c
	mkdir -p $(dir $@) && \
	$(CC) $(CFLAGS) $(patsubst build/kernel/%.o, kernel/%.c, $@) -o $@

clean:
	-rm -rf build && \
	$(MAKE) -C liballoc clean && \
	mkdir build

test:
	qemu-system-x86_64 -smp 4 -m 2G -monitor stdio -d int -no-reboot -no-shutdown -accel tcg -boot d -cdrom boot.iso -device VGA

test_serial:
	qemu-system-x86_64 -smp 4 -m 2G -serial stdio -no-shutdown -no-reboot -boot d -cdrom boot.iso -device VGA

build_kernel: $(bootloader_asm_objects) $(modules_objects) $(kernel_objects)
	$(MAKE) -C liballoc compile && \
	$(LD) -T inari.ld -m elf_i386 -n $(bootloader_asm_objects) $(modules_objects) $(kernel_objects) liballoc/liballoc.o -o build/Inari && \
	cp build/Inari grub/kernel && \
	grub-mkrescue -o boot.iso grub
