.text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000) { *(.text) }
.rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000) {
    *(.rodata)
}
.earlycon ALIGN (4K) : AT (ADDR (.earlycon) - 0xC0000000) {
    __start_earlycon = .;
    KEEP(*(.earlycon))
    __stop_earlycon = .;
}
.modules ALIGN (4K) : AT (ADDR (.modules) - 0xC0000000) {
    __start_modules = .;
    KEEP(*(.modules))
    __stop_modules = .;
}
.data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000) { *(.data) }
.bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000) { *(.bss) }