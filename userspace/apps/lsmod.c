#include <sys.h>
#include <io.h>
#include <string.h>

#include <kernel/module.h>

int main(int argc, char *argv[])
{
    int idx = 0;
    char name[128];
    uint32_t flags;
    uintptr_t ptr;

    while (lsmod(idx++, &name[0], &ptr, &flags) > 0)
        printf(" (%s%s)\t%d  %s @ 0x%x\n",
            (flags & MODULE_FLAG_IS_LOADED) ? "loaded" : "unloaded",
            (flags & MODULE_FLAG_BUILTIN) ? "; builtin" : "",
            idx-1, name, ptr);

    return 0;
}
