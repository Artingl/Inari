TARGET=x86
CC = gcc
LD = ld

CFLAGS = -ffreestanding -fno-asynchronous-unwind-tables -fno-stack-protector -nostdlib -Wall -Wextra -I include/ $(shell cat arch/$(TARGET)/cflags.txt) -std=gnu99

kernel_source := $(shell find kernel/ -name '*.c')
kernel_objects := $(patsubst kernel/%.c, build/kernel/%.o, $(kernel_source))

driver_source := $(shell find driver/ -name '*.c')
driver_objects := $(patsubst driver/%.c, build/driver/%.o, $(driver_source))

arch_sources := $(shell find arch/$(TARGET) -name '*.c' -o -name '*.S')
arch_build_stamp := build/arch/$(TARGET)/.built

$(kernel_objects): build/kernel/%.o: kernel/%.c
	mkdir -p $(dir $@) && \
	$(CC) -c $(CFLAGS) $< -o $@

$(driver_objects): build/driver/%.o: driver/%.c
	mkdir -p $(dir $@) && \
	$(CC) -c $(CFLAGS) $< -o $@

# Arch build marker
$(arch_build_stamp): $(arch_sources)
	$(MAKE) -C arch/$(TARGET) build
	mkdir -p $(dir $@)
	touch $@

clean:
	-rm -rf build && \
	make -C arch/${TARGET} clean

build: $(kernel_objects) $(driver_objects) $(arch_build_stamp)
	mkdir -p build/grub/boot/grub && \
	cp grub.cfg build/grub/boot/grub && \
	make -C arch/${TARGET} build_ld && \
	cp arch/${TARGET}/kernel.elf build/grub/kernel && \
	grub2-mkrescue -o build/boot.iso build/grub
