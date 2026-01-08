#!/bin/sh
CC=i686-w64-mingw32-gcc
LD=i686-w64-mingw32-ld

make -C ../libc clean
make -C ../libc build -j4

$CC $(cat /media/psf/Home/stuff/workspace/local/Inari/cflags.txt) -I ../libc/include/ -c main.c -o main.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o main.exe -m i386pe main.o -L../libc -lc
