#ifndef _INARI_X86_ARCH_H
#define _INARI_X86_ARCH_H

#define x86_io_wait() x86_outb(0x80, 0)

void x86_outb(uint16_t port, uint8_t val);
void x86_outw(uint16_t port, uint16_t val);
void x86_outsw(unsigned short int __port, const void *__addr,
                                unsigned long int __count);
void x86_insw(unsigned short int __port, void *__addr, unsigned long int __count);
uint8_t x86_inb(uint16_t port);
uint16_t x86_inw(uint16_t port);
void x86_get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi);
void x86_set_msr(uint32_t msr, uint32_t lo, uint32_t hi);
void x86_rdtsc(uint32_t *lo, uint32_t *hi);
uint64_t x86_rdtsc_v();
void x86_cpuid(uint32_t leah, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

#endif