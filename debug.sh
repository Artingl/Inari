#!/bin/sh

make clean && make build
qemu-system-i386 \
    -cdrom build/boot.iso \
    -serial stdio \
    -m 1G \
    -s -S
