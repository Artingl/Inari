#ifndef _INARI_UNAME_H
#define _INARI_UNAME_H

struct utsname {
    char sysname[16];    /* Operating system name */
    char nodename[16];   /* Name within communications network
                            to which the node is attached, if any */
    char release[16];    /* Operating system release
                            (e.g., "2.6.28") */
    char version[16];    /* Operating system version */
    char machine[16];    /* Hardware type identifier */
}__attribute__((packed));

int uname(struct utsname *buf);

#endif