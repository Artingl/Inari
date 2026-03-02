#include <io.h>
#include <sys.h>

int main(int argc, char const *argv[])
{
    /* Load video drivers */
    insmod("vesa");

    pid_t pid;
    if (execp(&pid, "/shell/ism.exe") == 0)
        waitpid(pid);
    return 0;
}