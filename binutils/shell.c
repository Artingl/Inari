#include <stddef.h>
#include <stdint.h>
#include <io.h>
#include <string.h>
#include <sys.h>
#include <errno.h>

char path[128] = {0};
int path_offset = 0;
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
    int is_bg = path[path_offset - 1] == '&';
    if (is_bg) path[path_offset - 1] = '\0';

    char buff[256] = {0};
    char *exec_path = NULL;
    int argc = 0;
    char **argv = split_into_buffer((void*)&buff[0], path, &argc, &exec_path);
    if (!exec_path) return;
    if (strcmp(exec_path, "exit") == 0) {
        exit(0);
        return;
    }
    if ((res = execpv(&pid, exec_path, argc, argv)) != 0) {
        printf("error: code %d\n", res);
        return;
    }

    if (!is_bg)
        waitpid(pid);
}

int main(int argc, char const *argv[])
{
    printf("Simple test shell.\n");
    printf("My name is %s\n\n", argv[0]);

    if (open(&kb_hndl, "/dev/input/char_kbd0", READ) != 0)
    {
        printf("error: unable to open keyboard.\n");
        exit(1);
    }

    printf(">> ");
    path_offset = 0;
    memset((void*)&path[0], 0, sizeof(path));

    do
    {
        if (read(kb_hndl, (void*)&kbd_event, sizeof(kbd_event), NULL) >= 0)
        {
            if (last_kb_event != kbd_event.event_id && kbd_event.released)
            {
                last_kb_event = kbd_event.event_id;
                if (kbd_event.key == 0x101)
                {
                    printf("\n");
                    exec_cmd();
                    path_offset = 0;
                    memset((void*)&path[0], 0, sizeof(path));
                    printf(">> ");
                }
                else {
                    path[path_offset++] = kbd_event.key;
                    path[path_offset] = '\0';

                    printf("%c", path[path_offset-1]);
                }
            }
        }

    } while(1);

    return 0;
}
