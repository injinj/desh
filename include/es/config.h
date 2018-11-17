#ifndef __es__config_h__
#define __es__config_h__
/* Linux config.h
 * These are used mostly in stdenv.h and prim-* */

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#define GETGROUPS_T gid_t

/* Define if you have the wait3 system call.  */
#define HAVE_WAIT3 1

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Do you have a sysconf? */
#define HAVE_SYSCONF 1

/* Define if you have the lstat function.  */
#define HAVE_LSTAT 1

/* Define if you have the setrlimit function.  */
#define HAVE_SETRLIMIT 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Do you have a /dev/fd/ directory? */
#define HAVE_DEV_FD 1

/* Do you have sighold? */
#define HAVE_SIGHOLD 1

/* Do you have sigrelse? */
#define HAVE_SIGRELSE 1

/* Do you have sigaction? */
#define HAVE_SIGACTION 1

/* Does your kernel support #!? */
#define KERNEL_POUNDBANG 1

/* What type are your signals? */
#define VOID_SIGNALS 1

/* What type are your limits? */
#define LIMIT_T rlim_t

/* Do you have setsid? */
#define HAVE_SETSID 1

/* Do you have a new fnagled sys/cdefs.h? */
#define HAVE_SYS_CDEFS_H 1

/* Can I assign to va_lists's */
#define NO_VA_LIST_ASSIGN 1

#endif
