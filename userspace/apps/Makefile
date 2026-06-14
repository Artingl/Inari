TARGET=x86
CC = i686-w64-mingw32-gcc
LD = i686-w64-mingw32-ld

CFLAGS=$(shell cat ../../cflags.txt) -I../shell/ism/include -I../libc/include/ -I../../include -fleading-underscore -mno-stack-arg-probe -fno-stack-check -fno-stack-protector

apps_source := $(shell find ./ -name '*.c')
apps_objects := $(patsubst ./%.c, ./%.exe, $(apps_source))

$(apps_objects): ./%.exe: ./%.c
	mkdir -p $(dir $@) && \
	$(CC) $(CFLAGS) -c $< -o $@.o
	$(LD) --entry=__start -no-pie -nostdlib -o $@ -m i386pe $@.o -L../libc -lc --image-base 0x020000000

clean:
	-rm ./*.exe ./*.o

build: $(apps_objects)
