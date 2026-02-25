#include <sys.h>
#include <io.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int idx = 0;
    char name[128];
    uint32_t state;
    uintptr_t ptr;

    while (lsmod(idx++, &name[0], &ptr, &state) > 0)
        printf("  %d  %s @ 0x%x (%s)\n", idx-1, name, ptr, state ? "loaded" : "unloaded");

    return 0;
}
