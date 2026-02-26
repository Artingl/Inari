#include <stddef.h>
#include <stdint.h>
#include <io.h>
#include <string.h>
#include <sys.h>
#include <errno.h>
#include <lib.h>

#include <kernel/console/console.h>
#include <kernel/subsys/hid.h>

char command[128] = {0};
char history[8][128] = {0};
char current_dir[128] = {'/', NULL};
int history_offset = 0, history_scroll = 0;
int command_offset = 0, last_exitcode = 0;
handle_t kb_hndl;

char shift_keys[] = {
    [ '1' ] = '!',
    [ '2' ] = '@',
    [ '3' ] = '#',
    [ '4' ] = '$',
    [ '5' ] = '%',
    [ '6' ] = '^',
    [ '7' ] = '&',
    [ '8' ] = '*',
    [ '9' ] = '(',
    [ '0' ] = ')',
    [ '-' ] = '_',
    [ '=' ] = '+',
    [ ';' ] = ':',
    [ '\\' ] = '|',
    [ ',' ] = '<',
    [ '.' ] = '>',
    [ '/' ] = '?',
};

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

int try_to_execute_at(const char *prefix, const char *suffix, int *exit_code, int is_background, const char *bin_path, int argc, char **argv)
{
    pid_t pid;
    int res, offset = 0;

    char combined_path[128];
    memset((void*)&combined_path[0], 0, sizeof(combined_path));
    if (prefix)
    {
        strcpy(combined_path, prefix);
        combined_path[strlen(prefix)] = '/';
        offset = strlen(prefix) + 1;
        strcpy(combined_path + offset, bin_path);
    }
    else strcpy(combined_path, bin_path);
    offset += strlen(bin_path);
    if (suffix)
    {
        strcpy(combined_path + offset, suffix);
    }

    if ((res = execpv(&pid, combined_path, argc, argv)) != 0)
        return res;

    if (!is_background)
        *exit_code = waitpid(pid);
    else *exit_code = 0;
    return 0;
}

void exec_cmd()
{
    int res, i;
    char buff[256] = {0};
    char *exec_path = NULL;
    int is_background = command[command_offset-1] == '&';
    if (is_background) command[command_offset-1] = '\0';
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
    else if (strcmp(exec_path, "cd") == 0) {
        if (argc < 1)
        {
            printf("cd: requires at least one param.\n");
            return;
        }
        struct fs_node node = {0};
        int cmd_res = readdir(argv[0], &node);
        if (cmd_res == -ENOENT)
        {
            if (errstr[-cmd_res] == NULL)
                printf("cd: Invalid error.\n");
            else 
                printf("cd: %s\n", errstr[-cmd_res]);
            return;
        }

        memset((void*)&current_dir[0], 0, sizeof(current_dir));
        strcpy(current_dir, argv[0]);
        return;
    }
    else if (strcmp(exec_path, "clear") == 0) {
        ioctl(stdout, CONSOLE_IOCTL_CLR, NULL);
        return;
    }

    if ((res = try_to_execute_at("/prog", NULL, &last_exitcode, is_background, exec_path, argc, argv)) == 0) return;
    if ((res = try_to_execute_at(current_dir, NULL, &last_exitcode, is_background, exec_path, argc, argv)) == 0) return;
    if ((res = try_to_execute_at("/prog", ".exe", &last_exitcode, is_background, exec_path, argc, argv)) == 0) return;
    if ((res = try_to_execute_at(current_dir, ".exe", &last_exitcode, is_background, exec_path, argc, argv)) == 0) return;

    if (errstr[-res] == NULL)
        printf("%s: Invalid error.\n", exec_path);
    else 
        printf("%s: %s\n", exec_path, errstr[-res]);
}

int main(int argc, char const *argv[])
{
    pid_t pid;
    int res;
    int is_shift_pressed = 0;
    char *cat_argv[] = { "/sys/motd.txt", NULL };
    struct kbd_event kbd;
    struct hid_device_info kbd_info;

    if (execpv(&pid, "/prog/cat.exe", 1, cat_argv) == 0)
        waitpid(pid);

    if (open(&kb_hndl, "/dev/input/char_kbd0", READ) != 0)
    {
        printf("error: unable to open keyboard.\n");
        exit(1);
    }

    printf("0@%s # ", current_dir);
    command_offset = 0;
    history_offset = 0;
    history_scroll = 0;
    last_exitcode = 0;
    memset((void*)&command[0], 0, sizeof(command));
    flush(stdout);

    do
    {
        if ((res = read(kb_hndl, (void*)&kbd, sizeof(kbd), NULL)) != 0)
        {
            printf("%s: unable to read keyboard: %s.", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
            return -1;
        }

        if (kbd.key == 0x106) // KEY_LSHIFT
            is_shift_pressed = !kbd.released;

        if (kbd.released)
        {
            if (kbd.key == 0x101) // enter
            {
                printf("\n");
                exec_cmd();
                command_offset = 0;
                memset((void*)&command[0], 0, sizeof(command));
                printf("%d@%s # ", last_exitcode, current_dir);
            }
            else if (kbd.key == 0x102 && command_offset > 0) // backspace
            {
                command_offset--;
                ioctl(stdout, CONSOLE_IOCTL_REWIND_CLR, (void*)1);
            }
            else if (kbd.key == 0x136) // arrow up
            {
                if (history_scroll >= 0)
                {
                    memcpy((void*)&command[0], (void*)&history[history_scroll--], 128);
                    ioctl(stdout, CONSOLE_IOCTL_REWIND_CLR, (void*)command_offset);
                    printf(command);
                    command_offset = strlen(command);
                }
            }
            else if (kbd.key == 0x13b) // arrow down
            {
                if (history_scroll < 8 && history_scroll < history_offset)
                {
                    memcpy((void*)&command[0], (void*)&history[history_scroll++], 128);
                    ioctl(stdout, CONSOLE_IOCTL_REWIND_CLR, (void*)command_offset);
                    printf(command);
                    command_offset = strlen(command);
                }
            }
            else if (kbd.key >= 0x20 && kbd.key <= 0x7E) { // is ascii
                char key = kbd.key;
                if (is_shift_pressed && shift_keys[key] != 0)
                    key = shift_keys[key];

                command[command_offset++] = key;
                command[command_offset] = '\0';

                printf("%c", command[command_offset-1]);
            }
        }
        flush(stdout);
    } while(1);

    return 0;
}
