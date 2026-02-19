#!/bin/sh
CC=i686-w64-mingw32-gcc
LD=i686-w64-mingw32-ld

make -C ../libc clean
make -C ../libc build -j4

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c ls.c -o ls.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o ls.exe -m i386pe ls.o -L../libc -lc --image-base 0x010000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c shell.c -o shell.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o shell.exe -m i386pe shell.o -L../libc -lc --image-base 0x010000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c cat.c -o cat.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o cat.exe -m i386pe cat.o -L../libc -lc --image-base 0x010000000
