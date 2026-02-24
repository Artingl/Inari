#include <sys.h>
#include <io.h>
#include <signals.h>

char test[12];
char *addr = (char*)0;

void sigsev_handler(uint32_t signo)
{
    // printf("got seg fault 0x%x!\n", addr);
    addr = &test[0];
    sigreturn(); // dont forget sigreturn!
}

void sigquit_handler(uint32_t signo)
{
    printf("SIGQUIT!\n");

    exit(0);
}

int main(void)
{
    // signal_handler(&sigsev_handler, SIGSEGV);
    signal_handler(&sigquit_handler, SIGQUIT);
    pid_t pid;
    get_pid(&pid);
    printf("ipc: working! my pid is %llu\n", pid);

    while (1)
    {
        printf("testing!\n");
        addr[0] = 'a';
        printf("works!\n");
        /* Avoid busy looping */
        usleep(1000000);
    }
}
