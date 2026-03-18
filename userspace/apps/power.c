#include <sys.h>
#include <io.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("usage: %s reboot/off\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "reboot") == 0) reboot();
    else if (strcmp(argv[1], "off") == 0) poweroff();
    else return -1;
    return 0;
}
