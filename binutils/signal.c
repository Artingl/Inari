#include <sys.h>
#include <io.h>
#include <lib.h>
#include <signals.h>
#include <errno.h>


int main(int argc, char const *argv[])
{
    pid_t pid;
    int signo;
    int res = 0;

    if (argc < 3)
    {
        printf("usage: %s [signal] [pid]\n", argv[0]);
        res = -1;
        goto end;
    }

    signo = atoi(argv[1]);
    pid = atol(argv[2]);

    if ((res = signal(pid, signo)) != 0)
        printf("%s: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
end:
    return res;
}
