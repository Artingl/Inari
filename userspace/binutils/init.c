#include <io.h>
#include <sys.h>

static void ism_wait() {
    int res = -1;
    handle_t ipc;
    /* Really dumb way to do so */

    printf("init: waiting for ISM.\n");
    while((res = ipc_open("ism.window", &ipc)) != 0)
        usleep(10000);

    printf("init: ISM is alive.\n");
    ipc_close(ipc);
}

int main(int argc, char const *argv[])
{
    pid_t ism_pid;

    do {
        /* Load video drivers */
        insmod("vesa");

        execp(&ism_pid, "/shell/ism.exe");
        ism_wait();
        execp(NULL, "/programs/terminal.exe");
        waitpid(ism_pid);
        rmmod("vesa");
        printf("%s: ISM died! Attempting restart in 2 seconds.\n", get_name());
        usleep(2000000);
    } while (1);

    return 0;
}
