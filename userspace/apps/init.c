#include <io.h>
#include <sys.h>

static void ism_wait() {
    int res = -1, timeout = 100;
    handle_t ipc = 0;
    /* Really dumb way to do so */

    printf("init: waiting for ISM.\n");
    while ((res = ipc_open("ism.window", &ipc)) != 0 && timeout-- > 0)
        usleep(10000);

    if (timeout <= 0) {
        printf("init: ISM wait timedout.\n");
        if (ipc)
            ipc_close(ipc);
        return;
    }

    printf("init: ISM is alive.\n");
    ipc_close(ipc);
}

int main(int argc, char const *argv[]) {
    pid_t ism_pid;

    // execp(&ism_pid, "/programs/nettest.exe");
    // waitpid(ism_pid);

    // char *a[] = {"87.240.132.78"};
    // execpv(&ism_pid, "/programs/ping.exe", 1, a);
    // waitpid(ism_pid);

    // execp(&ism_pid, "/programs/cmd.exe");
    // waitpid(ism_pid);
    //
    char *a[] = {"/devices/terminals"};
    execpv(&ism_pid, "/programs/ls.exe", 1, a);
    waitpid(ism_pid);

    // handle_t tty0, serial; /devices/terminals/char_tty0
    // open(&tty0, "/devices/terminals/char_tty0", WRITE);
    // open(&tty0, "/devices/terminals/tty0_char", WRITE);
    // ioctl(tty0, 4, ); // CONSOLE_IOCTL_TTY_ATTACH_DEV

    do {
        /* Load video drivers */
        insmod("vesa");

        execp(&ism_pid, "/shell/ism.exe");
        ism_wait();
        execp(NULL, "/programs/cube.exe");
        execp(NULL, "/programs/terminal.exe");
        waitpid(ism_pid);
        rmmod("vesa");
        printf("%s: ISM died! Attempting restart in 2 seconds.\n", get_name());
        usleep(2000000);
    } while (1);

    return 0;
}
