TARGET=x86
CC = i686-w64-mingw32-gcc
LD = i686-w64-mingw32-ld

CFLAGS = -I include/ $(shell cat ../../arch/$(TARGET)/cflags.txt) $(shell cat ../../cflags.txt) -m32 -mfpmath=387

libc_source := $(shell find src/ -name '*.c')
libc_objects := $(patsubst src/%.c, build/%.o, $(libc_source))

headers := $(shell find include/ -name '*.h')

$(libc_objects): build/%.o: src/%.c
	mkdir -p $(dir $@) && \
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	-rm -rf build && \
	rm libc.a

format: $(libc_source) $(headers)
	clang-format -i $(libc_source) $(headers)

build: $(libc_objects)
	cp ../../include/kernel/proc/signals.h include/ && \
	i686-w64-mingw32-ar rcs libc.a $(shell find build/ -name *.o)
