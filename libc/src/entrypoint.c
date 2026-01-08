#include <typedefs.h>
#include <sys.h>

extern int main();
extern int libc_io_init();

void __main() {}

void _start()
{
    int res;
    if ((res = libc_io_init()) != 0)
        exit(res);
    exit(main());
}
