#include <types.h>
#include <sys.h>
#include <errno.h>
#include <signals.h>

extern int main();
extern int libc_io_init();

void __main() {}

static char *exec_path = NULL;

#include <io.h>
static void _signal(uint32_t signo)
{
    char *type = NULL;
    switch (signo)
    {
        case SIGSEGV:   { type = "Segmentation fault";        break; }
        case SIGTRAP:   { type = "Trace/breakpoint trap";     break; }
        case SIGQUIT:   { type = "Quit";                      break; }
        case SIGFPE:    { type = "Floating point exception";  break; }
        case SIGILL:    { type = "Illegal instruction";       break; }
        case SIGSYS:    { type = "Bad system call";           break; }
        case SIGKILL:   { type = "Kill";                      break; }
    }

    if (signo != SIGKILL && signo != SIGQUIT && type)
    {
        if (exec_path)
            printf("%s: %s.\n", exec_path, type);
        else
            printf("%s.\n", type);
    }
    exit(-EINTR);
    // sigreturn();
}

void _start()
{
    exec_path = NULL;
    int res = 0;
    int argc = 0;
    char **argv = (char**)0x1900000;
    if ((res = libc_io_init()) != 0)
        exit(res);
    while (argv[argc] != NULL) argc++;
    if (argc >= 1)
        exec_path = argv[0];
    signal_handler(&_signal, SIGSEGV);
    signal_handler(&_signal, SIGTRAP);
    signal_handler(&_signal, SIGQUIT);
    signal_handler(&_signal, SIGFPE);
    signal_handler(&_signal, SIGILL);
    signal_handler(&_signal, SIGSYS);
    signal_handler(&_signal, SIGKILL);
    exit(main(argc, argv));
}

const char *get_name(void)
{
    return get_argv()[0] ? get_argv()[0] : "undefined";
}

const char **get_argv(void)
{
    return (const char**)0x1900000;
}

int get_argc(void)
{
    int argc = 0;
    char **argv = (char**)0x1900000;
    while (argv[argc] != NULL) argc++;
    return argc;
}