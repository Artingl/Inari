// Credit: from linux sources

#ifndef _INARI_ERRNO_H
#define _INARI_ERRNO_H

#define ENODEV     1  /* No such device */
#define EPERM      2  /* Operation not permitted */
#define EBUSY      3  /* Device or resource busy */
#define EINVAL     4  /* Invalid argument */
#define ENOMEM     5  /* Out of memory */
#define EINVFS     6  /* Invalid file-system */
#define EIO        7  /* I/O error */
#define ETIMEDOUT  8  /* Connection timed out */
#define ENOENT     9  /* No such file or directory */
#define EBADHNDL   10 /* Bad handle */
#define ENOSYS     11 /* Function not implemented */
#define ESRCH      12 /* No such process */
#define EACCES     13 /* Permission denied */
#define EFAULT     14 /* Bad address */
#define ENOEXEC    15 /* Exec format error */
#define EINTR      16 /* Interrupted */
#define EKILLED    17 /* Killed */
#define ESUBSYSDIS 18 /* Subsystem disabled */

#define IPCBSY   19 /* IPC is busy */
#define IPCNONE  20 /* IPC endpoint not found */
#define IPCACCES 21 /* IPC access denied */

__attribute__((unused)) static const char *errstr[] = {
    [0] = "OK",
    [ENODEV] = "No such device",
    [EPERM] = "Operation not permitted",
    [EBUSY] = "Device or resource busy",
    [EINVAL] = "Invalid argument",
    [ENOMEM] = "Out of memory",
    [EINVFS] = "Invalid file-system",
    [EIO] = "I/O error",
    [ETIMEDOUT] = "Connection timed out",
    [ENOENT] = "No such file or directory",
    [EBADHNDL] = "Bad handle",
    [ENOSYS] = "Function not implemented",
    [ESRCH] = "No such process",
    [EACCES] = "Permission denied",
    [EFAULT] = "Bad address",
    [ENOEXEC] = "Exec format error",
    [IPCBSY] = "IPC is busy",
    [IPCNONE] = "IPC endpoint not found",
    [IPCACCES] = "IPC access denied",
    [ESUBSYSDIS] = "Subsystem disabled",
};

#undef errno
#define errno(err) (errstr[((err) < 0 ? -(err) : err)] ? errstr[((err) < 0 ? -(err) : err)] : "Invalid error")

//
// #define	ENOENT		 2	/* No such file or directory */
// #define	ESRCH		 3	/* No such process */
// #define	EINTR		 4	/* Interrupted system call */
// #define	EIO		 5	/* I/O error */
// #define	ENXIO		 6	/* No such device or address */
// #define	E2BIG		 7	/* Arg list too long */
// #define	ENOEXEC		 8	/* Exec format error */
// #define	EBADF		 9	/* Bad file number */
// #define	ECHILD		10	/* No child processes */
// #define	EAGAIN		11	/* Try again */
//
// #define	EACCES		13	/* Permission denied */
// #define	EFAULT		14	/* Bad address */
// #define	ENOTBLK		15	/* Block device required */
//
// #define	EEXIST		17	/* File exists */
// #define	EXDEV		18	/* Cross-device link */
// #define	ENODEV		19	/* No such device */
// #define	ENOTDIR		20	/* Not a directory */
// #define	EISDIR		21	/* Is a directory */
//
// #define	ENFILE		23	/* File table overflow */
// #define	EMFILE		24	/* Too many open files */
// #define	ENOTTY		25	/* Not a typewriter */
// #define	ETXTBSY		26	/* Text file busy */
// #define	EFBIG		27	/* File too large */
// #define	ENOSPC		28	/* No space left on device */
// #define	ESPIPE		29	/* Illegal seek */
// #define	EROFS		30	/* Read-only file system */
// #define	EMLINK		31	/* Too many links */
// #define	EPIPE		32	/* Broken pipe */
// #define	EDOM		33	/* Math argument out of domain of func */
// #define	ERANGE		34	/* Math result not representable */
// #define	EDEADLK		35	/* Resource deadlock would occur */
// #define	ENAMETOOLONG	36	/* File name too long */
// #define	ENOLCK		37	/* No record locks available */
// #define	ENOSYS		38	/* Function not implemented */
// #define	ENOTEMPTY	39	/* Directory not empty */
// #define	ELOOP		40	/* Too many symbolic links encountered */
// #define	EWOULDBLOCK	41	/* Operation would block */
// #define	ENOMSG		42	/* No message of desired type */
// #define	EIDRM		43	/* Identifier removed */
// #define	ECHRNG		44	/* Channel number out of range */
// #define	EL2NSYNC	45	/* Level 2 not synchronized */
// #define	EL3HLT		46	/* Level 3 halted */
// #define	EL3RST		47	/* Level 3 reset */
// #define	ELNRNG		48	/* Link number out of range */
// #define	EUNATCH		49	/* Protocol driver not attached */
// #define	ENOCSI		50	/* No CSI structure available */
// #define	EL2HLT		51	/* Level 2 halted */
// #define	EBADE		52	/* Invalid exchange */
// #define	EBADR		53	/* Invalid request descriptor */
// #define	EXFULL		54	/* Exchange full */
// #define	ENOANO		55	/* No anode */
// #define	EBADRQC		56	/* Invalid request code */
// #define	EBADSLT		57	/* Invalid slot */
// #define	EDEADLOCK	58	/* File locking deadlock error */
// #define	EBFONT		59	/* Bad font file format */
// #define	ENOSTR		60	/* Device not a stream */
// #define	ENODATA		61	/* No data available */
// #define	ETIME		62	/* Timer expired */
// #define	ENOSR		63	/* Out of streams resources */
// #define	ENONET		64	/* Machine is not on the network */
// #define	ENOPKG		65	/* Package not installed */
// #define	EREMOTE		66	/* Object is remote */
// #define	ENOLINK		67	/* Link has been severed */
// #define	EADV		68	/* Advertise error */
// #define	ESRMNT		69	/* Srmount error */
// #define	ECOMM		70	/* Communication error on send */
// #define	EPROTO		71	/* Protocol error */
// #define	EMULTIHOP	72	/* Multihop attempted */
// #define	EDOTDOT		73	/* RFS specific error */
// #define	EBADMSG		74	/* Not a data message */
// #define	EOVERFLOW	75	/* Value too large for defined data type */
// #define	ENOTUNIQ	76	/* Name not unique on network */
// #define	EBADFD		77	/* File descriptor in bad state */
// #define	EREMCHG		78	/* Remote address changed */
// #define	ELIBACC		79	/* Can not access a needed shared library */
// #define	ELIBBAD		80	/* Accessing a corrupted shared library */
// #define	ELIBSCN		81	/* .lib section in a.out corrupted */
// #define	ELIBMAX		82	/* Attempting to link in too many shared libraries */
// #define	ELIBEXEC	83	/* Cannot exec a shared library directly */
// #define	EILSEQ		84	/* Illegal byte sequence */
// #define	ERESTART	85	/* Interrupted system call should be restarted */
// #define	ESTRPIPE	86	/* Streams pipe error */
// #define	EUSERS		87	/* Too many users */
// #define	ENOTSOCK	88	/* Socket operation on non-socket */
// #define	EDESTADDRREQ	89	/* Destination address required */
// #define	EMSGSIZE	90	/* Message too long */
// #define	EPROTOTYPE	91	/* Protocol wrong type for socket */
// #define	ENOPROTOOPT	92	/* Protocol not available */
// #define	EPROTONOSUPPORT	93	/* Protocol not supported */
// #define	ESOCKTNOSUPPORT	94	/* Socket type not supported */
// #define	EOPNOTSUPP	95	/* Operation not supported on transport endpoint */
// #define	EPFNOSUPPORT	96	/* Protocol family not supported */
// #define	EAFNOSUPPORT	97	/* Address family not supported by protocol */
// #define	EADDRINUSE	98	/* Address already in use */
// #define	EADDRNOTAVAIL	99	/* Cannot assign requested address */
// #define	ENETDOWN	100	/* Network is down */
// #define	ENETUNREACH	101	/* Network is unreachable */
// #define	ENETRESET	102	/* Network dropped connection because of reset */
// #define	ECONNABORTED	103	/* Software caused connection abort */
// #define	ECONNRESET	104	/* Connection reset by peer */
// #define	ENOBUFS		105	/* No buffer space available */
// #define	EISCONN		106	/* Transport endpoint is already connected */
// #define	ENOTCONN	107	/* Transport endpoint is not connected */
// #define	ESHUTDOWN	108	/* Cannot send after transport endpoint shutdown */
// #define	ETOOMANYREFS	109	/* Too many references: cannot splice */
// #define	ETIMEDOUT	110	/* Connection timed out */
// #define	ECONNREFUSED	111	/* Connection refused */
// #define	EHOSTDOWN	112	/* Host is down */
// #define	EHOSTUNREACH	113	/* No route to host */
// #define	EALREADY	114	/* Operation already in progress */
// #define	EINPROGRESS	115	/* Operation now in progress */
// #define	ESTALE		116	/* Stale NFS file handle */
// #define	EUCLEAN		117	/* Structure needs cleaning */
// #define	ENOTNAM		118	/* Not a XENIX named type file */
// #define	ENAVAIL		119	/* No XENIX semaphores available */
// #define	EISNAM		120	/* Is a named type file */
// #define	EREMOTEIO	121	/* Remote I/O error */
#endif
