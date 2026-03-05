#include <io.h>
#include <sys.h>

void ipc_handler() {
    pid_t source;
    uint32_t message;
    void *data;
    size_t data_size;

    do {
        int fetch = ipc_fetch_next(&source, &message, &data, &data_size);
        if (fetch == 0) {
            if (data) {
                printf("init: received ipc message %u from pid %llu, at 0x%x with size %u; data contents: %s\n", message, source, data, data_size, (char*)data);
            }
            else
                printf("init: received ipc message %u from pid %llu, at 0x%x with size %u\n", message, source, data, data_size);
        }

        ipc_reply(69);
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
