#!/bin/sh
unset GTK_PATH
unset GIO_MODULE_DIR
make clean && make build
qemu-system-i386 \
    -cdrom build/boot.iso \
    -serial stdio \
    -m 1G \
    -smp 4 \
    -drive file=disk.img,if=ide \
    -machine pc \
    -boot d \
    -s -S
