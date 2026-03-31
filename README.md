# Inari
A monolithic 32-bit x86 Operating System built from scratch.

Now in its third architectural iteration. The goal is to get to a sustainable working userspace, custom GUI, fully-featured network stack. Some of that work is being done currently.

> [!NOTE]  
 This kernel is not unix-based, not posix compliant. You _might_ get a feeling of otherwise, but frame it rather as getting things from different concepts I enjoy. E.g. not supporting forking and rather implementing spawing.

## Screenshots
![Screenshot](screenshot.png)
![GUI](screenshot2.png)

## Building
To build the kernel simply run `make build`. This will generate file in `build/kernel.elf`, which can be booted with any multiboot compliant bootloader. The `make build` will also build the userspace programs.

To build rootfs you can use `./mkimg.sh`, this will bundle all apps, kernel file and install grub.

## Booting
You you can run the kernel via `qemu-system-i386 -serial stdio -cdrom build/boot.iso` if constructing rootfs with `./mkimg.sh`.

You can also specify bootargs, which kernel will parse (e.g. by specifying in `grub.cfg`). Here are supported params:
 - earlycon: The earlycon device to print the early logs. Supported values: vga_text, pc8250.
 - root: The block dev to mount as root. Can be any block device in format %s%d, where %s is name and %d is index.
 - init: Path to the init script which will run as PID 1.

Example for bootargs: `earlycon=vga_text root=mbr0 init=/programs/cmd.exe`

## Core Architecture & Features
Unlike many hobby OS projects, Inari focuses on a robust internal architecture and hardware isolation:

* **Memory Management:** Custom PMM and VMM with direct page-table manipulation and userspace isolation.
* **Process Management:** Ring 3 userspace processes and Ring 0 kernel threads, with a custom preemptive scheduler.
* **Inter-Process Communication (IPC):** Fast message-passing mechanism between isolated processes (using direct page mapping for zero-copy data transfer).
* **Loadable Kernel Modules:** Dynamic module loading.
* **Storage & File Systems:** Virtual File System, `devfs`, MBR/GPT partition parsing, and FAT filesystem support.
* **Custom GUI & Compositor:** A fully custom windowing system with a GUI compositor, rendering multiple overlapping windows and handling PS/2 mouse/keyboard inputs.
* **Network Stack:** Custom implementation of Ethernet, IPv4, ARP, and ICMP directly over the RTL8139 driver.
* **Userland:** A custom `libc` implementation and a set of basic core utilities.
