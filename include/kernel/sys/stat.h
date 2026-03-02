#ifndef _INARI_STAT_H
#define _INARI_STAT_H

/* File type bitmask */
#define S_IFMT   00170000 /* bitmask for the file type bitfields */
#define S_IFSOCK 0140000  /* socket */
#define S_IFLNK  0120000  /* symbolic link */
#define S_IFREG  0100000  /* regular file */
#define S_IFBLK  0060000  /* block device */
#define S_IFDIR  0040000  /* directory */
#define S_IFCHR  0020000  /* character device */
#define S_IFIFO  0010000  /* FIFO */

/* File type test macros */
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)

/* Permission bits (owner, group, others) */
#define S_IRWXU 00700 /* owner: rwx */
#define S_IRUSR 00400 /* owner: read */
#define S_IWUSR 00200 /* owner: write */
#define S_IXUSR 00100 /* owner: execute/search */

#define S_IRWXG 00070 /* group: rwx */
#define S_IRGRP 00040 /* group: read */
#define S_IWGRP 00020 /* group: write */
#define S_IXGRP 00010 /* group: execute/search */

#define S_IRWXO 00007 /* others: rwx */
#define S_IROTH 00004 /* others: read */
#define S_IWOTH 00002 /* others: write */
#define S_IXOTH 00001 /* others: execute/search */

#endif