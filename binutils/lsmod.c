#include <sys.h>
#include <io.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int idx = 0;
    char name[128];
    uintptr_t ptr;

    while (lsmod(idx++, &name[0], &ptr) > 0)
        printf("  %d  %s @ 0x%x\n", idx-1, name, ptr);

    return 0;
}
