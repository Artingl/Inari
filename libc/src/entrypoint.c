#include <typedefs.h>
#include <sys.h>

extern int main();

void __main() {}

void _start()
{
    exit(main());
}
