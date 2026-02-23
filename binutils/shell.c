#include <stddef.h>
#include <stdint.h>
#include <io.h>
#include <string.h>
#include <sys.h>
#include <errno.h>
#include <alloc.h>

#define NO_EXIT_CODE 0xFFFFFFFF

char command[128] = {0};
char history[8][128] = {0};
int history_offset = 0, history_scroll = 0;
int command_offset = 0, last_exitcode = NO_EXIT_CODE;
uint32_t last_kb_event;
handle_t kb_hndl;
struct
{
    uint32_t event_id;
    dev_t dev;

    int released;
    uint8_t code;
    uint16_t key;
} kbd_event;

char** split_into_buffer(void *buffer, const char *input, int *argc, char **first_arg_out) {
    if (!buffer || !input || !argc) return NULL;

    const char *p = input;
    while (*p && (*p == ' ' || *p == '\t')) p++;

    if (*p == '\0') {
        *argc = 0;
        if (first_arg_out) *first_arg_out = NULL;
        char **empty_argv = (char **)buffer;
        empty_argv[0] = NULL;
        return empty_argv;
    }

    const char *first_word_start = p;
    while (*p && (*p != ' ' && *p != '\t')) p++;
    size_t first_word_len = p - first_word_start;
    const char *start_from = p;

    int count = 0;
    int in_word = 0;
    while (*p) {
        if (*p != ' ' && *p != '\t') {
            if (!in_word) {
                count++;
                in_word = 1;
            }
        } else {
            in_word = 0;
        }
        p++;
    }

    char **new_argv = (char **)buffer;
    char *data_ptr = (char *)buffer + ((count + 1) * sizeof(char *));

    if (first_arg_out) {
        *first_arg_out = data_ptr;
        memcpy(data_ptr, first_word_start, first_word_len);
        data_ptr[first_word_len] = '\0';
        data_ptr += first_word_len + 1;
    }

    p = start_from;
    in_word = 0;
    int current_arg = 0;
    while (*p) {
        if (*p != ' ' && *p != '\t') {
            if (!in_word) {
                new_argv[current_arg++] = data_ptr;
                in_word = 1;
            }
            *data_ptr++ = *p;
        } else {
            if (in_word) {
                *data_ptr++ = '\0';
                in_word = 0;
            }
        }
        p++;
    }

    if (in_word) *data_ptr = '\0';

    new_argv[count] = NULL;
    *argc = count;
    return new_argv;
}

void exec_cmd()
{
    pid_t pid;
    int res, i;
    int is_bg = command[command_offset - 1] == '&';
    if (is_bg) command[command_offset - 1] = '\0';
    char buff[256] = {0};
    char *exec_path = NULL;
    int argc = 0;
    char **argv = split_into_buffer((void*)&buff[0], command, &argc, &exec_path);
    if (!exec_path) return;

    if (history_offset >= 8)
    {
        memcpy((void*)&history[0], (void*)&history[0] + 128, sizeof(history) - 128);
        history_offset = 7;
    }

    history_scroll = history_offset;
    history[history_offset][0] = 0;
    memcpy((void*)&history[history_offset++], (void*)&command[0], 128);

    if (strcmp(exec_path, "exit") == 0) {
        exit(0);
        return;
    }
    else if (strcmp(exec_path, "clear") == 0) {
        ioctl(stdout, 2, NULL); // CONSOLE_IOCTL_CLR
        return;
    }
    if ((res = execpv(&pid, exec_path, argc, argv)) != 0) {
        printf("error: code %d\n", res);
        return;
    }

    if (!is_bg)
        last_exitcode = waitpid(pid);
}

int main(int argc, char const *argv[])
{
    pid_t pid;
    char *cat_argv[] = { "/sys/motd.txt", NULL };
    if (execpv(&pid, "/prog/cat.exe", 1, cat_argv) == 0)
        waitpid(pid);

    if (open(&kb_hndl, "/dev/input/char_kbd0", READ) != 0)
    {
        printf("error: unable to open keyboard.\n");
        exit(1);
    }

    printf("+;/> ");
    command_offset = 0;
    history_offset = 0;
    history_scroll = 0;
    last_exitcode = NO_EXIT_CODE;
    memset((void*)&command[0], 0, sizeof(command));

    do
    {
        if (read(kb_hndl, (void*)&kbd_event, sizeof(kbd_event), NULL) >= 0)
        {
            if (last_kb_event != kbd_event.event_id && kbd_event.released)
            {
                last_kb_event = kbd_event.event_id;
                if (kbd_event.key == 0x101) // enter
                {
                    printf("\n");
                    exec_cmd();
                    command_offset = 0;
                    memset((void*)&command[0], 0, sizeof(command));
                    if (last_exitcode == NO_EXIT_CODE)
                        printf("+;/> ");
                    else
                        printf("%d;/> ", last_exitcode);
                }
                else if (kbd_event.key == 0x102 && command_offset > 0) // backspace
                {
                    command_offset--;
                    ioctl(stdout, 1, (void*)1); // CONSOLE_IOCTL_REWIND_CLR
                }
                else if (kbd_event.key == 0x136) // arrow up
                {
                    if (history_scroll >= 0)
                    {
                        memcpy((void*)&command[0], (void*)&history[history_scroll--], 128);
                        ioctl(stdout, 1, (void*)command_offset); // CONSOLE_IOCTL_REWIND_CLR
                        printf(command);
                        command_offset = strlen(command);
                    }
                }
                else if (kbd_event.key == 0x13b) // arrow down
                {
                    if (history_scroll < 8 && history_scroll < history_offset)
                    {
                        memcpy((void*)&command[0], (void*)&history[history_scroll++], 128);
                        ioctl(stdout, 1, (void*)command_offset); // CONSOLE_IOCTL_REWIND_CLR
                        printf(command);
                        command_offset = strlen(command);
                    }
                }
                else if (kbd_event.key >= 0x20 && kbd_event.key <= 0x7E) { // is ascii
                    command[command_offset++] = kbd_event.key;
                    command[command_offset] = '\0';

                    printf("%c", command[command_offset-1]);
                }
            }
        }

    } while(1);

    return 0;
}
