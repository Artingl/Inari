cflags = -std=c99 -O2 --include "config.h" -I . -fno-stack-protector -nostdlib -ffreestanding -c -m32 -mfpmath=387 -Werror -Wno-stringop-overflow

CC = $(HOME)/opt/cross/bin/i386-elf-gcc
LD = $(HOME)/opt/cross/bin/i386-elf-ld

bootloader_source := $(shell find bootloader/ -name *.c)
bootloader_objects := $(patsubst bootloader/%.c, build/bootloader/%.o, $(bootloader_source))

$(bootloader_objects): build/bootloader/%.o : bootloader/%.c
	mkdir -p $(dir $@) && \
	$(CC) -fno-pie $(cflags) $(patsubst build/bootloader/%.o, bootloader/%.c, $@) -o $@

build_asm_bootloader:
	mkdir -p build/bootloader/asm && \
	nasm -f elf32 bootloader/asm/bootloader.asm -o build/bootloader/asm/bootloader.o && \
	nasm -f elf32 bootloader/asm/cpu/smp.asm -o build/bootloader/asm/smp.o && \
	nasm -f elf32 bootloader/asm/cpu/realmode.asm -o build/bootloader/asm/realmode.o && \
	nasm -f elf32 bootloader/asm/cpu/scheduler.asm -o build/bootloader/asm/scheduler.o

drivers_source := $(shell find drivers/ -name *.c)
drivers_objects := $(patsubst drivers/%.c, build/drivers/%.o, $(drivers_source))

$(drivers_objects): build/drivers/%.o : drivers/%.c
	mkdir -p $(dir $@) && \
	$(CC) $(cflags) $(patsubst build/drivers/%.o, drivers/%.c, $@) -o $@

kernel_source := $(shell find kernel/ -name *.c)
kernel_objects := $(patsubst kernel/%.c, build/kernel/%.o, $(kernel_source))

$(kernel_objects): build/kernel/%.o : kernel/%.c
	mkdir -p $(dir $@) && \
	$(CC) $(cflags) $(patsubst build/kernel/%.o, kernel/%.c, $@) -o $@

clean:
	-rm -rf build && \
	$(MAKE) -C liballoc clean && \
	mkdir build

test: build_kernel
	qemu-system-x86_64 -smp 4 -m 2G -monitor stdio -d int -no-reboot -no-shutdown -accel tcg -boot d -cdrom boot.iso -drive file=dummy_gpt.img,format=raw -device VGA

test_serial: build_kernel
	qemu-system-x86_64 -smp 4 -m 2G -serial stdio -no-shutdown -no-reboot -boot d -cdrom boot.iso -drive file=dummy_gpt.img,format=raw -device VGA

build_kernel: build_asm_bootloader $(bootloader_objects) $(drivers_objects) $(kernel_objects)
	$(MAKE) -C liballoc compile && \
	$(LD) -T inari.ld -m elf_i386 -n build/bootloader/asm/bootloader.o build/bootloader/asm/scheduler.o build/bootloader/asm/smp.o build/bootloader/asm/realmode.o $(bootloader_objects) \
								  liballoc/liballoc.o $(drivers_objects) $(kernel_objects) -o build/Inari && \
	cp build/Inari target/grub/kernel && \
	grub-mkrescue -o boot.iso target/grub
