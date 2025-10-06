#ifndef _INARI_SIGNAL_H
#define _INARI_SIGNAL_H

/* Standard signals */
#define SIGHUP      1   /* Hangup */
#define SIGINT      2   /* Interrupt */
#define SIGQUIT     3   /* Quit */
#define SIGILL      4   /* Illegal instruction */
#define SIGTRAP     5   /* Trace/breakpoint trap */
#define SIGABRT     6   /* Abort */
#define SIGBUS      7   /* Bus error */
#define SIGFPE      8   /* Floating point exception */
#define SIGKILL     9   /* Kill, cannot be caught or ignored */
#define SIGUSR1     10  /* User-defined signal 1 */
#define SIGSEGV     11  /* Segmentation fault */
#define SIGUSR2     12  /* User-defined signal 2 */
#define SIGPIPE     13  /* Broken pipe */
#define SIGALRM     14  /* Alarm clock */
#define SIGTERM     15  /* Termination */
#define SIGSTKFLT   16  /* Stack fault (unused on most archs) */
#define SIGCHLD     17  /* Child stopped or terminated */
#define SIGCONT     18  /* Continue */
#define SIGSTOP     19  /* Stop, cannot be caught or ignored */
#define SIGTSTP     20  /* Keyboard stop (Ctrl-Z) */
#define SIGTTIN     21  /* Background read */
#define SIGTTOU     22  /* Background write */
#define SIGURG      23  /* Urgent socket condition */
#define SIGXCPU     24  /* CPU time limit exceeded */
#define SIGXFSZ     25  /* File size limit exceeded */
#define SIGVTALRM   26  /* Virtual timer expired */
#define SIGPROF     27  /* Profiling timer expired */
#define SIGWINCH    28  /* Window resize */
#define SIGIO       29  /* I/O possible (SIGPOLL) */
#define SIGPWR      30  /* Power failure */
#define SIGSYS      31  /* Bad system call */

/* Range for real-time signals */
#define SIGRTMIN    32
#define SIGRTMAX    64

/* Useful constants */
#define NSIG        65   /* Total number of signals (1–64) */

/* Default signal actions */
#define SIG_DFL ((void (*)(int))0)  /* Default action */
#define SIG_IGN ((void (*)(int))1)  /* Ignore signal */
#define SIG_ERR ((void (*)(int))-1) /* Error return */

#endif