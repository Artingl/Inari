#include <sys.h>
#include <io.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int idx = 0;
    pid_t pid;
    double usg;
    char cmd[128];

    printf("  PID\tUSG\tPATH\n");
    while (lsproc(idx++, &cmd[0], &pid, &usg) > 0)
    {
        printf("  %llu\t%2f%%\t%s\n", pid, usg * 100, cmd);
    }

    return 0;
}
