TARGET=x86
CC = i686-w64-mingw32-gcc
LD = i686-w64-mingw32-ld

CFLAGS=$(shell cat ../../cflags.txt) -I../../libc/include/ -I../../include -fleading-underscore

binutils_source := $(shell find ./ -name '*.c')
binutils_objects := $(patsubst ./%.c, ./%.exe, $(binutils_source))

$(binutils_objects): ./%.exe: ./%.c
	mkdir -p $(dir $@) && \
	$(CC) -c $(CFLAGS) -c $< -o $@.o
	$(LD) --entry=__start -no-pie -nostdlib -o $@ -m i386pe $@.o -L../../libc -lc --image-base 0x020000000

build: $(binutils_objects)
