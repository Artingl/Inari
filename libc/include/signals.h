#ifndef _INARI_SIGNAL_H
#define _INARI_SIGNAL_H

/* Standard signals */
#define SIGSEGV     1   /* Segmentation fault */
#define SIGTRAP     2   /* Trace/breakpoint trap */
#define SIGQUIT     3   /* Quit */
#define SIGFPE      4   /* Floating point exception */
#define SIGILL      5   /* Illegal instruction */
#define SIGSYS      6   /* Bad system call */
#define SIGKILL     9   /* Kill, cannot be caught or ignored */

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