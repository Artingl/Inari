#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/uname.h>

#include <misc/string.h>

static struct utsname inari_iname = {.sysname = "Inari",
                                     .nodename = "none", /* Hostname */
                                     .release = "1.0",
                                     .version = "1",
#ifdef CONFIG_ARCH_X86
                                     .machine = "x86"
#elif CONFIG_ARCH_AARCH64
                                     .machine = "aarch64"
#else
                                     .machine = "Invalid"
#endif
};

int uname(struct utsname *buf) {
    if (!buf)
        return -EINVAL;
    memcpy(buf, &inari_iname, sizeof(struct utsname));
    return 0;
}
