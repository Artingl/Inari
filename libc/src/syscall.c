#include <sys.h>

int syscall(
    uint32_t id,
    uint32_t param0,
    uint32_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4)
{
    __asm__ volatile("mov %0, %%ebx" :: "r"(id));
    __asm__ volatile("mov %0, %%ecx" :: "r"(param0));
    __asm__ volatile("mov %0, %%edx" :: "r"(param1));
    __asm__ volatile("mov %0, %%esi" :: "r"(param2));
    __asm__ volatile("mov %0, %%edi" :: "r"(param3));
    __asm__ volatile("mov %0, %%ebp" :: "r"(param4));
    __asm__ volatile("int $0x80");
    return 0;
}