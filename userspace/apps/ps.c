#include <sys.h>
#include <io.h>

int main(int argc, char *argv[])
{
    int idx = 0;
    pid_t pid;
    time_t usg, totalusg = 1;
    char cmd[128];
    idx = 0;
    while (lsproc(idx++, &cmd[0], &pid, &usg) > 0)
        totalusg += usg;

    printf("  PID\tUSG\tPATH\n");
    idx = 0;
    while (lsproc(idx++, &cmd[0], &pid, &usg) > 0)
    {
        printf("  %llu\t%2f%%\t%s\n", pid, ((double)usg / (double)totalusg) * 100, cmd);
    }

    return 0;
}
