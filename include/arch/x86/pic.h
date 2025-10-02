#ifndef _INARI_X86_PIC
#define _INARI_X86_PIC

extern int x86_pic_init(void);
extern void x86_pic_acknowledge(uint8_t irq_no);
extern void x86_pic_irq_mask(uint8_t irq_line);

#endif