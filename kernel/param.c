#include <kernel/inari.h>
#include <misc/string.h>

int parse_cmdline_argument(const char *key, char *result) {
    const char *cmdline = get_cmdline();
    char *argument_start = NULL;
    size_t offset = 0, sz = 0;
    size_t key_len = strlen(key), cmdline_len = strlen(cmdline);

    if (result == NULL)
        return -1;

    do {
        /* Check that we'll not overflow */
        if (offset + key_len + 1 >= cmdline_len)
            return -1;

        if (strncmp(cmdline + offset, key, key_len) == 0 &&
            (*(cmdline + offset + key_len) == '=' || *(cmdline + offset + key_len) == ' ' ||
             *(cmdline + offset + key_len) == 0)) {
            /* If the param was provided without arguments, just return none */
            if (*(cmdline + offset + key_len) != '=') {
                memcpy(result, "none\0", 5);
                return 0;
            }

            /* Parse the arguments */
            argument_start = (char *)cmdline + offset + key_len + 1;
            sz = 0;
            while (*(cmdline + offset + key_len + 1) != ' ' && *(cmdline + offset + key_len + 1) != '\t' &&
                   *(cmdline + offset + key_len + 1) != '\n' && *(cmdline + offset + key_len + 1) != 0 &&
                   (cmdline + offset - argument_start) < ARG_MAX_LEN) {
                offset++;
                sz++;
            }

            memcpy(result, argument_start, sz);
            result[sz] = '\0';
            return 0;
        }

        while (*(cmdline + offset) != ' ' && *(cmdline + offset) != 0)
            offset++;
        offset++;
    } while (1);

    return -1;
}

const char *get_cmdline() {
    extern bootinfo_t bootinfo;
    return bootinfo.cmdline;
}