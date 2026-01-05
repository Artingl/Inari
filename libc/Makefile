TARGET=x86
CC = i686-w64-mingw32-gcc
LD = i686-w64-mingw32-ld

CFLAGS = -I include/ $(shell cat ../arch/$(TARGET)/cflags.txt) $(shell cat ../cflags.txt)

libc_source := $(shell find src/ -name '*.c')
libc_objects := $(patsubst src/%.c, build/%.o, $(libc_source))

$(libc_objects): build/%.o: src/%.c
	mkdir -p $(dir $@) && \
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	-rm -rf build && \
	rm libc.a

build: $(libc_objects)
	i686-w64-mingw32-ar rcs libc.a $(shell find build/ -name *.o)
