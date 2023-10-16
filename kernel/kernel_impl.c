#include <kernel/kernel.h>

#include <drivers/video/video.h>
#include <drivers/cpu/cpu.h>

#include <kernel/sys/console/console.h>

#include <kernel/include/C/io.h>
#include <kernel/include/C/string.h>

#include <stdarg.h>

int __pr_wrapper_handler(char c, void **)
{
    console_printc(c);
}

// helper function for the "__pl_wrapper", to print formatted messages to the console
int __pr_wrapper_helper(const char *fmt, ...)
{
    int c;
    va_list args;
    va_start(args, fmt);
    c += do_printkn(fmt, args, &__pr_wrapper_handler, NULL);
    va_end(args);

    return c;
}

int __pr_wrapper(size_t line, const char *file, const char *func, const char *fmt, ...)
{
    int c = 0, shift = 0;
    char *prefix = NULL;

#ifdef CONFIG_NODEBUG
    if (fmt[0] == KERN_DEBUG[0])
        return 0;
#endif

    // extract the prefix from the fmt
    switch (fmt[0])
    {
    case '0':
    {
        shift++;
        prefix = "CRIT";
        break;
    }
    case '1':
    {
        shift++;
        prefix = "ERR";
        break;
    }
    case '2':
    {
        shift++;
        prefix = "WARNING";
        break;
    }
    case '3':
    {
        shift++;
        prefix = "NOTICE";
        break;
    }
    case '4':
    {
        shift++;
        prefix = "INFO";
        break;
    }
    case '5':
    {
        shift++;
        prefix = "DEBUG";
        break;
    }
    case '6':
    {
        shift++;
        prefix = "TODO";
        break;
    }
    default:
    {
        prefix = "DEFAULT";
    }
    }

    va_list args;
    va_start(args, fmt);
    // print formatted message to the console

    // prefix --------------
    if (fmt[0] == '6')
    { // different output for "TODO" prefix
        c += __pr_wrapper_helper("TODO{%f, %s:%d} :: ", kernel_uptime(), func, line);
    }
    else
    {
        c += __pr_wrapper_helper("[%f] %s :: ", kernel_uptime(), prefix);
    }
    // ----------------------

    // the message --------------
    c += do_printkn(fmt + shift, args, &__pr_wrapper_handler, NULL);
    c += console_printc('\n');
    // --------------------------

    va_end(args);

    return c;
}

extern double __timer_ticks;

double kernel_uptime()
{
    return __timer_ticks / 1000.0f; // / __cpu_timer_freq();
}

void panic(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    __pr_wrapper_helper("[%f] PANIC :: ", kernel_uptime());
    do_printkn(message, args, &__pr_wrapper_handler, NULL);

    // print the NL character so the screen would update
    console_printc('\n');
    va_end(args);

    cpu_shutdown();
}

char mount_point[256];

const char *kernel_root_mount_point()
{
    return mount_point;
}

void __parse_cmdline_cmd(const char *command, const char *argument)
{
    printk(KERN_DEBUG "cmdline: cmd = %s, argument = %s", command, argument);

    if (memcmp(command, "root", 4) == 0)
    {
        // the root command must always have an argument
        if (argument == NULL)
        {
            panic("cmdline: Usage of the \"root\" command is not possible without an argument.");
        }

        // mount root
        memcpy(&mount_point[0], &argument[0], strlen(argument)+1);
    }
}

void kernel_parse_cmdline()
{
    char buffer[128];

    struct kernel_payload const *config = kernel_configuration();
    char *cmdline = config->cmdline;
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
                __parse_cmdline_cmd(cmd, arg);
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
                arg = &cmdline[i + 1];
            }
            continue;
        }

        if (cmd == NULL)
        {
            cmd = &cmdline[i];
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
        __parse_cmdline_cmd(cmd, arg);
    }
}
