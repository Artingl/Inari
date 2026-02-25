#!/bin/sh
CC=i686-w64-mingw32-gcc
LD=i686-w64-mingw32-ld

make -C ../libc clean
make -C ../libc build -j4

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c ls.c -o ls.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o ls.exe -m i386pe ls.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c shell.c -o shell.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o shell.exe -m i386pe shell.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c cat.c -o cat.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o cat.exe -m i386pe cat.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c ipc.c -o ipc.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o ipc.exe -m i386pe ipc.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c signal.c -o signal.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o signal.exe -m i386pe signal.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c power.c -o power.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o power.exe -m i386pe power.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c rmmod.c -o rmmod.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o rmmod.exe -m i386pe rmmod.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c insmod.c -o insmod.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o insmod.exe -m i386pe insmod.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c lsmod.c -o lsmod.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o lsmod.exe -m i386pe lsmod.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) -I ../libc/include/ -c ps.c -o ps.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o ps.exe -m i386pe ps.o -L../libc -lc --image-base 0x020000000
