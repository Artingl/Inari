#!/bin/sh
CC=i686-w64-mingw32-gcc
LD=i686-w64-mingw32-ld

make -C ../libc clean
make -C ../libc build -j4

$CC $(cat /home/artingl/dev/inari/cflags.txt) -I ../libc/include/ -c main.c -o main.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o main.exe -m i386pe main.o -L../libc -lc


$CC $(cat /home/artingl/dev/inari/cflags.txt) -I ../libc/include/ -c test.c -o test.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o test.exe -m i386pe test.o -L../libc -lc
