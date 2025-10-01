#ifndef _INARI_ARCH_H
#define _INARI_ARCH_H

#define __sti() __asm__ volatile("sti")
#define __cli() __asm__ volatile("cli")
#define __halt() do { __asm__ volatile("hlt"); } while (1)


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

#endif