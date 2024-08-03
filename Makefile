CFLAGS = -std=c99 -O2 --include "config.h" -I . -fno-stack-protector -nostdlib -ffreestanding -c -m32 -mfpmath=387 -Werror -Wno-stringop-overflow
CC = $(HOME)/opt/cross/bin/i386-elf-gcc
LD = $(HOME)/opt/cross/bin/i386-elf-ld
ASM = nasm

bootloader_source := $(shell find bootloader/ -name *.c)
bootloader_objects := $(patsubst bootloader/%.c, build/bootloader/%.o, $(bootloader_source))

$(bootloader_objects): build/bootloader/%.o : bootloader/%.c
	mkdir -p $(dir $@) && \
	$(CC) -fno-pie $(CFLAGS) $(patsubst build/bootloader/%.o, bootloader/%.c, $@) -o $@

bootloader_asm_source := $(shell find bootloader/asm/ -maxdepth 1 -name *.asm)
bootloader_asm_objects := $(patsubst bootloader/asm/%.asm, build/bootloader/asm/%.o, $(bootloader_asm_source))

$(bootloader_asm_objects): build/bootloader/asm/%.o : bootloader/asm/%.asm
	mkdir -p $(dir $@) && \
	$(ASM) -f elf32 $(patsubst build/bootloader/%.o, bootloader/%.asm, $@) -o $@

drivers_source := $(shell find drivers/ -name *.c)
drivers_objects := $(patsubst drivers/%.c, build/drivers/%.o, $(drivers_source))

$(drivers_objects): build/drivers/%.o : drivers/%.c
	mkdir -p $(dir $@) && \
	$(CC) $(CFLAGS) $(patsubst build/drivers/%.o, drivers/%.c, $@) -o $@

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
	qemu-system-x86_64 -smp 4 -m 2G -monitor stdio -d int -no-reboot -no-shutdown -accel tcg -boot d -cdrom boot.iso -drive file=dummy_gpt.img,format=raw -device VGA

test_serial:
	qemu-system-x86_64 -smp 4 -m 128M -serial stdio -no-shutdown -no-reboot -boot d -cdrom boot.iso -drive file=dummy_gpt.img,format=raw -device VGA


build_kernel: $(bootloader_asm_objects) $(drivers_objects) $(kernel_objects)
	$(MAKE) -C liballoc compile && \
	$(LD) -T inari.ld -m elf_i386 -n $(bootloader_asm_objects) $(drivers_objects) $(kernel_objects) liballoc/liballoc.o -o build/Inari && \
	cp build/Inari target/grub/kernel && \
	grub-mkrescue -o boot.iso target/grub
