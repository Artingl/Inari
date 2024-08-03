#include <kernel/kernel.h>
#include <kernel/include/string.h>

char mount_point[256];

static void parse_cmdline_cmd(const char *command, const char *argument)
{
    printk("cmdline: cmd = %s, argument = %s", command, argument);

    if (memcmp(command, "root", 4) == 0)
    {
        // the root command must always have an argument
        if (argument == NULL)
        {
            panic("cmdline: Usage of the \"root\" command is not possible without an argument.");
        }

        // set the mount root
        memcpy(&mount_point[0], &argument[0], strlen(argument)+1);
    }
}

void kparse_cmdline()
{
    char buffer[128];

    const char *cmdline = kernel_cmdline();
    size_t i, cmdline_len = strlen(cmdline);

    char *cmd = NULL;
    char *arg = NULL;
    size_t cmd_len = 0;
    size_t arg_len = 0;

    for (i = 0; i < cmdline_len; i++)
    {
        if (cmdline[i] == ' ')
        {
            // The end of the argument.
            //
            // Check that we actually parsed something
            if (cmd != NULL)
            {
                // copy command and argument to dedicated buffer, so we can NULL-terminate them.
                memcpy(&buffer[0], cmd, cmd_len);
                buffer[cmd_len] = '\0';
                cmd = &buffer[0];

                // NULL-terminate argument only if we found one
                if (arg != NULL)
                {
                    memcpy(&buffer[cmd_len + 1], arg, arg_len);
                    buffer[cmd_len + arg_len + 1] = '\0';
                    arg = &buffer[cmd_len + 1];
                }

                // execute the command
                parse_cmdline_cmd(cmd, arg);
            }

            // reset values and parse next argument
            cmd = NULL;
            arg = NULL;
            cmd_len = 0;
            arg_len = 0;
            continue;
        }
        else if (cmdline[i] == '=')
        {
            // The end of the command. Parse argument if not the end of the cmdline
            if (i + 1 < cmdline_len)
            {
                arg = (char*)&cmdline[i + 1];
            }
            continue;
        }

        if (cmd == NULL)
        {
            cmd = (char*)&cmdline[i];
        }

        // increment the length of the command string if we're not parsing the argument
        if (arg == NULL)
        {
            cmd_len++;
        }
        else
        {
            // We're parsing the argument
            arg_len++;
        }
    }

    // if we have command left in the buffer, execute it
    if (cmd != NULL)
    {
        // copy command and argument to dedicated buffer, so we can NULL-terminate them.
        memcpy(&buffer[0], cmd, cmd_len);
        buffer[cmd_len] = '\0';
        cmd = &buffer[0];

        // NULL-terminate argument only if we found one
        if (arg != NULL)
        {
            memcpy(&buffer[cmd_len + 1], arg, arg_len);
            buffer[cmd_len + arg_len + 1] = '\0';
            arg = &buffer[cmd_len + 1];
        }

        // execute the command
        parse_cmdline_cmd(cmd, arg);
    }
}
