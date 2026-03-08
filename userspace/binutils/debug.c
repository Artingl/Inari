#include <sys.h>
#include <io.h>

int main(int argc, char *argv[])
{
    return syscall(3, 0, 0, 0, 0, 0);
}
