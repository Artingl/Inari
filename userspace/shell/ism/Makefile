TARGET=x86
CC = i686-w64-mingw32-gcc
LD = i686-w64-mingw32-ld

CFLAGS = $(shell cat ../../../arch/$(TARGET)/cflags.txt) $(shell cat ../../../cflags.txt) -I../../../libc/include/ -I../../../include -fleading-underscore -mno-stack-arg-probe -fno-stack-check -fno-stack-protector

ism_source := $(shell find src/ -name '*.c')
ism_objects := $(patsubst src/%.c, build/%.o, $(ism_source))

$(ism_objects): build/%.o: src/%.c
	mkdir -p $(dir $@) && \
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -rf ../ism.exe build

build: $(ism_objects)
	$(LD) --entry=__start -no-pie -nostdlib -o ../ism.exe -m i386pe $(shell find build/ -name *.o) -L../../../libc -lc --image-base 0x020000000
