#!/bin/sh
CC=i686-w64-mingw32-gcc
LD=i686-w64-mingw32-ld

$CC $(cat /home/artingl/dev/inari/cflags.txt) -c main.c -o main.o
$LD -no-pie -nostdlib -o main.exe -m i386pe main.o
