#include <sys.h>
#include <io.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("usage: %s [name]\n", argv[0]);
        return -1;
    }

    if (rmmod(argv[1]) != 0)
    {
        printf("%s: Module %s not loaded.\n", argv[0], argv[1]);
        return -1;
    }

    return 0;
}
