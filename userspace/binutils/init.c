#include <io.h>
#include <sys.h>

int main(int argc, char const *argv[])
{
    pid_t pid;
    /* Load video drivers */
    insmod("vesa");

    do {
        if (execp(&pid, "/shell/ism.exe") == 0)
            waitpid(pid);
        printf("%s: ISM died! Attempting restart in 2 seconds.\n", get_name());
        usleep(2000000);
    } while (1);
    
    return 0;
}