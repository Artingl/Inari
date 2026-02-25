#ifdef CONFIG_SUBSYS_PCI

#include <kernel/inari.h>

#include <misc/types.h>

#include <arch/x86/arch.h>
#include <arch/sys.h>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC

uint32_t _arch_pci_read_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
  
    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
  
    // Write out the address
    x86_outl(CONFIG_ADDRESS, address);
    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    return x86_inl(CONFIG_DATA) >> ((offset & 2) * 8);
}

void _arch_pci_write_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val)
{

}

#endif