#include <stddef.h>
#include <stdint.h>
#include <sys.h>

extern int main();
extern int libc_io_init();

void __main() {}

void _start()
{
    int res = 0;
    int argc = 0;
    char **argv = (char**)0x900000;
    if ((res = libc_io_init()) != 0)
        exit(res);
    while (argv[argc] != NULL) argc++;
    exit(main(argc, argv));
}
