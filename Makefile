TARGET=x86
CC = i686-elf-gcc
LD = i686-elf-ld
GRUB_MKRESCUE := $(shell if command -v grub-mkrescue >/dev/null 2>&1; then echo grub-mkrescue; else echo grub2-mkrescue; fi)

CFLAGS = --include "build/config.h" -I include/ $(shell cat arch/$(TARGET)/cflags.txt) $(shell cat cflags.txt)

kernel_source := $(shell find kernel/ -name '*.c')
kernel_objects := $(patsubst kernel/%.c, build/kernel/%.o, $(kernel_source))

driver_source := $(shell find driver/ -name '*.c')
driver_objects := $(patsubst driver/%.c, build/driver/%.o, $(driver_source))

arch_sources := $(shell find arch/$(TARGET) -name '*.c' -o -name '*.S')
arch_build_stamp := build/arch/$(TARGET)/.built

headers := $(shell find include/ -name '*.h')

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

mkconfig:
	mkdir -p build && \
	cat config.h > build/config.h && \
	echo "#define CONFIG_ARCH_$(shell echo $(TARGET) | tr '[:lower:]' '[:upper:]')" >> build/config.h

clean:
	-rm -rf build userspace/binutils/*.exe userspace/binutils/*.o && \
	make -C userspace/libc clean && \
	make -C userspace/binutils clean && \
	make -C userspace/shell/ism clean && \
	make -C userspace/terminal clean && \
	make -C arch/${TARGET} clean

format: $(kernel_source) $(driver_source) $(headers)
	make -C arch/${TARGET} format && \
	clang-format -i $(kernel_source) $(driver_source) $(headers)

build: mkconfig $(kernel_objects) $(driver_objects) $(arch_build_stamp)
	make -C arch/${TARGET} build_ld && \
	make -C userspace/libc build && \
	make -C userspace/binutils build && \
	make -C userspace/shell/ism build && \
	make -C userspace/terminal build && \
	cp arch/${TARGET}/kernel.elf build/kernel.elf
