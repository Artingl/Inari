#include <io.h>
#include <sys.h>

void ipc_handler() {
    uint32_t message;
    void *data;
    size_t data_size;

    do {
        int fetch = ipc_fetch_next(&message, &data, &data_size);
        if (fetch == 0) {
            printf("init: received ipc message %u, at 0x%x with size %llu\n", message, data, data_size);
        }

        ipc_reply(0);
    } while(1);
}

int main(int argc, char const *argv[])
{
    ipc_create("init.handler", &ipc_handler);

    pid_t pid;

    do {
        /* Load video drivers */
        insmod("vesa");

        if (execp(&pid, "/shell/ism.exe") == 0)
            waitpid(pid);
        rmmod("vesa");
        printf("%s: ISM died! Attempting restart in 2 seconds.\n", get_name());
        usleep(2000000);
    } while (1);
    
    return 0;
}