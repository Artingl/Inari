#!/bin/sh
CC=i686-w64-mingw32-gcc
LD=i686-w64-mingw32-ld

make -C ../libc clean
make -C ../libc build -j4

CFLAGS="-I../libc/include/ -I../include"

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c ls.c -o ls.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o ls.exe -m i386pe ls.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c cmd.c -o cmd.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o cmd.exe -m i386pe cmd.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c cat.c -o cat.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o cat.exe -m i386pe cat.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c video.c -o video.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o video.exe -m i386pe video.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c signal.c -o signal.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o signal.exe -m i386pe signal.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c power.c -o power.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o power.exe -m i386pe power.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c rmmod.c -o rmmod.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o rmmod.exe -m i386pe rmmod.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c insmod.c -o insmod.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o insmod.exe -m i386pe insmod.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c lsmod.c -o lsmod.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o lsmod.exe -m i386pe lsmod.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c ps.c -o ps.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o ps.exe -m i386pe ps.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c debug.c -o debug.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o debug.exe -m i386pe debug.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c uname.c -o uname.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o uname.exe -m i386pe uname.o -L../libc -lc --image-base 0x020000000

$CC $(cat $(pwd)/../cflags.txt) $CFLAGS -c mouse.c -o mouse.o -fleading-underscore
$LD --entry=__start -no-pie -nostdlib -o mouse.exe -m i386pe mouse.o -L../libc -lc --image-base 0x020000000
