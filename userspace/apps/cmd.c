#include <errno.h>
#include <io.h>
#include <lib.h>
#include <string.h>
#include <sys.h>
#include <net.h>
#include <types.h>

#define CONSOLE_IOCTL_REWIND     0 // Rewind N chars in the console buffer
#define CONSOLE_IOCTL_REWIND_CLR 1 // Rewind N chars in the console buffer AND clear them
#define CONSOLE_IOCTL_CLR        2 // Clear
#define CONSOLE_IOCTL_FLUSH      3

char command[128] = {0};
char history[8][128] = {0};
char current_dir[128] = {'/', 0};
int history_offset = 0, history_scroll = 0;
int command_offset = 0, last_exitcode = 0;

char **split_into_buffer(void *buffer, const char *input, int *argc, char **first_arg_out) {
    if (!buffer || !input || !argc)
        return NULL;

    const char *p = input;
    while (*p && (*p == ' ' || *p == '\t'))
        p++;

    if (*p == '\0') {
        *argc = 0;
        if (first_arg_out)
            *first_arg_out = NULL;
        char **empty_argv = (char **)buffer;
        empty_argv[0] = NULL;
        return empty_argv;
    }

    const char *first_word_start = p;
    while (*p && (*p != ' ' && *p != '\t'))
        p++;
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

    if (in_word)
        *data_ptr = '\0';

    new_argv[count] = NULL;
    *argc = count;
    return new_argv;
}

int try_to_execute_at(const char *prefix, const char *suffix, int *exit_code, int is_background, const char *bin_path,
                      int argc, char **argv) {
    pid_t pid;
    int res, offset = 0;

    char combined_path[128];
    memset((void *)&combined_path[0], 0, sizeof(combined_path));
    if (prefix) {
        strcpy(combined_path, prefix);
        combined_path[strlen(prefix)] = '/';
        offset = strlen(prefix) + 1;
        strcpy(combined_path + offset, bin_path);
    } else
        strcpy(combined_path, bin_path);
    offset += strlen(bin_path);
    if (suffix) {
        strcpy(combined_path + offset, suffix);
    }

    if ((res = execpv(&pid, combined_path, argc, argv)) != 0)
        return res;

    if (!is_background)
        *exit_code = waitpid(pid);
    else
        *exit_code = 0;
    return 0;
}

void exec_cmd() {
    int res;
    char buff[256] = {0};
    char *exec_path = NULL;
    int is_background = command[command_offset - 1] == '&';
    if (is_background)
        command[command_offset - 1] = '\0';
    int argc = 0;
    char **argv = split_into_buffer((void *)&buff[0], command, &argc, &exec_path);
    if (!exec_path)
        return;

    if (history_offset >= 8) {
        memcpy((void *)&history[0], (void *)&history[0] + 128, sizeof(history) - 128);
        history_offset = 7;
    }

    history_scroll = history_offset;
    history[history_offset][0] = 0;
    memcpy((void *)&history[history_offset++], (void *)&command[0], 128);

    if (strcmp(exec_path, "exit") == 0) {
        exit(0);
        return;
    } else if (strcmp(exec_path, "cd") == 0) {
        if (argc < 1) {
            printf("cd: requires at least one param.\n");
            return;
        }
        struct fs_node node = {0};
        int cmd_res = readdir(argv[0], &node);
        if (cmd_res == -ENOENT) {
            if (errstr[-cmd_res] == NULL)
                printf("cd: Invalid error.\n");
            else
                printf("cd: %s\n", errstr[-cmd_res]);
            return;
        }

        union p_option_value option;
        memcpy(option.value, argv[0], strlen(argv[0]) + 1);
        if (p_option_set("path", option) != 0) {
            printf("cd: Unable to save change directory.");
        }
        else {
            memset((void *)&current_dir[0], 0, sizeof(current_dir));
            strcpy(current_dir, argv[0]);
        }
        return;
    } else if (strcmp(exec_path, "clear") == 0) {
        ioctl(stdio, CONSOLE_IOCTL_CLR, NULL);
        return;
    } else if (strcmp(exec_path, "p") == 0) {
        if (argc < 1) {
            printf("p: provide option name.\n");
            return;
        }

        union p_option_value result;
        if (p_option_get(argv[0], &result) == 0)
            printf("str: %s\nu32: %u\nu64: %llu\nsz: 0x%x\n", result.value, result.u32, result.u64, result.sz);
        else
            printf("p: option %s doesn't exist.\n", argv[0]);

        return;
    }

    if ((res = try_to_execute_at("/programs", NULL, &last_exitcode, is_background, exec_path, argc, argv)) == 0)
        return;
    if ((res = try_to_execute_at(current_dir, NULL, &last_exitcode, is_background, exec_path, argc, argv)) == 0)
        return;
    if ((res = try_to_execute_at("/programs", ".exe", &last_exitcode, is_background, exec_path, argc, argv)) == 0)
        return;
    if ((res = try_to_execute_at(current_dir, ".exe", &last_exitcode, is_background, exec_path, argc, argv)) == 0)
        return;

    if (errstr[-res] == NULL)
        printf("%s: Invalid error.\n", exec_path);
    else
        printf("%s: %s\n", exec_path, errstr[-res]);
}

int main(int argc, char const *argv[]) {
    pid_t pid;
    int res;
    char hostname[64] = "unknown";
    char *cat_argv[] = {"/system/motd.txt", NULL};

    if (execpv(&pid, "/programs/cat.exe", 1, cat_argv) == 0)
        waitpid(pid);

    handle_t video_handle;
    open(&video_handle, "/devices/video/char_video0", WRITE);
    ioctl(video_handle, 5, NULL); // VIDEO_IOCTL_DISABLE
    close(video_handle);
    get_hostname(hostname);

    printf("--@%s:%s # ", hostname, current_dir);
    command_offset = 0;
    history_offset = 0;
    history_scroll = 0;
    last_exitcode = 0;
    memset((void *)&command[0], 0, sizeof(command));
    flush(stdio);

    struct console_input {
        uint8_t pressed;
        uint8_t modifier;
        uint16_t key;
        uint16_t chr;
    } __attribute__((packed)) in;

    do {
        if ((res = read(stdio, &in, sizeof(in), NULL)) <= 0) {
            continue;
        }
        get_hostname(hostname);

        if (in.pressed) {
            if (in.chr == '\n') // enter
            {
                printf("\n");
                exec_cmd();
                command_offset = 0;
                memset((void *)&command[0], 0, sizeof(command));
                printf("--@%s:%s", hostname, current_dir);

                if (last_exitcode != 0) {
                    printf(" [%d] # ", last_exitcode);
                }
                else printf(" # ");
            } else if (in.key == 0x102 && command_offset > 0) // backspace
            {
                command_offset--;
                ioctl(stdio, CONSOLE_IOCTL_REWIND, (void *)1);
            } else if (in.key == 0x136) // arrow up
            {
                if (history_scroll >= 0) {
                    memcpy((void *)&command[0], (void *)&history[history_scroll--], 128);
                    ioctl(stdio, CONSOLE_IOCTL_REWIND_CLR, (void *)command_offset);
                    printf(command);
                    command_offset = strlen(command);
                }
            } else if (in.key == 0x13b) // arrow down
            {
                if (history_scroll < 8 && history_scroll < history_offset) {
                    memcpy((void *)&command[0], (void *)&history[history_scroll++], 128);
                    ioctl(stdio, CONSOLE_IOCTL_REWIND_CLR, (void *)command_offset);
                    printf(command);
                    command_offset = strlen(command);
                }
            } else if (in.key >= 0x20 && in.key <= 0x7E) { // is ascii
                command[command_offset++] = in.chr;
                command[command_offset] = '\0';

                printf("%c", command[command_offset - 1]);
            }
        }
        flush(stdio);
    } while (1);

    return 0;
}
