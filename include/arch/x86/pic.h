#ifndef _INARI_X86_PIC
#define _INARI_X86_PIC

int x86_pic_init(void);
void x86_pic_acknowledge(uint8_t irq_no);
void x86_pic_irq_unmask(uint8_t irq_line);
void x86_pic_irq_mask(uint8_t irq_line);

#endif