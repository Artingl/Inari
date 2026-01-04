#!/bin/sh
unset GTK_PATH
unset GIO_MODULE_DIR
make clean && make build
qemu-system-i386 \
    -serial stdio \
    -m 1G \
    -smp 4 \
    -drive file=build/boot.iso,if=ide \
    -machine pc \
    -s -S
