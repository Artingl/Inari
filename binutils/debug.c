#include <sys.h>
#include <io.h>
#include <string.h>

int main(int argc, char *argv[])
{
    return syscall(3, 0, 0, 0, 0, 0);
}
