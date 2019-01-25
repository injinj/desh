/* stdenv.h -- set up an environment we can use ($Revision: 1.3 $) */
#ifndef __es__stdenv_h__
#define __es__stdenv_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <desh/esconfig.h>
#ifndef __sun__
#include <sys/cdefs.h>
#endif

/*
 * type qualifiers
 */

#if !USE_VOLATILE
# ifndef volatile
#  define volatile
# endif
#endif


/*
 * protect the rest of es source from the dance of the includes
 */

#include <unistd.h>

#if REQUIRE_PARAM
#include <sys/param.h>
#endif

#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>

#if REQUIRE_STAT || REQUIRE_IOCTL || 1
#include <sys/types.h>
#endif

#if REQUIRE_IOCTL
#include <sys/ioctl.h>
#endif

#if REQUIRE_STAT
#include <sys/stat.h>
#endif

#if REQUIRE_DIRENT
#include <dirent.h>
typedef struct dirent Dirent;
#endif

#if REQUIRE_PWD
#include <pwd.h>
#endif

#if REQUIRE_FCNTL
#include <fcntl.h>
#endif

#include <sys/wait.h>

/* stdlib */
#if __GNUC__
typedef volatile void noreturn;
#else
typedef void noreturn;
#endif

#include <stdlib.h>

/*
 * things that should be defined by header files but might not have been
 */

#ifndef	offsetof
#define	offsetof(t, m)	((size_t) (((char *) &((t *) 0)->m) - (char *)0))
#endif

#ifndef	EOF
#define	EOF	(-1)
#endif

/*
 * macros
 */

#define	streq(s, t)		(strcmp(s, t) == 0)
#define	strneq(s, t, n)		(strncmp(s, t, n) == 0)
#define	hasprefix(s, p)		strneq(s, p, (sizeof p) - 1)
#define	arraysize(a)		((int) (sizeof (a) / sizeof (*a)))
#define	memzero(dest, count)	memset(dest, 0, count)
#define	atoi(s)			strtol(s, NULL, 0)

#define	STMT(stmt)		do { stmt; } while (0)
#define	NOP			do ; while (0)

#define CONCAT(a,b)	a ## b
#define STRING(s)	#s

/*
 * types we use throughout es
 */

#undef FALSE
#undef TRUE
typedef enum { FALSE, TRUE } Boolean;
typedef volatile sig_atomic_t Atomic;
typedef gid_t gidset_t;

/*
 * assertion checking
 */

#if ASSERTIONS
#define	assert(expr) \
	STMT( \
		if (!(expr)) { \
			eprint("%s:%d: assertion failed (%s)\n", \
				__FILE__, __LINE__, STRING(expr)); \
			abort(); \
		} \
	)
#else
#define	assert(ignore)	NOP
#endif

enum { UNREACHABLE = 0 };


#define	NOTREACHED	STMT(assert(UNREACHABLE))

/*
 * macros for picking apart statuses
 *	we should be able to use the W* forms from <sys/wait.h> but on
 *	some machines they take a union wait (what a bad idea!) and on
 *	others an integer.  we just renamed the first letter to s and
 *	let things be.  on some systems these could just be defined in
 *	terms of the W* forms.
 */

#define SIFSTOPPED(status)      ((status) & 0x40)
#define	SIFEXITED(status)	(!((status) & 0xff))
#define	SIFSIGNALED(status)	(!SIFEXITED(status) && !SIFSTOPPED(status))
#define	STERMSIG(status)	((status) & 0x7f)
#define	SCOREDUMP(status)	((status) & 0x80)
#define	SEXITSTATUS(status)	(((status) >> 8) & 0xff)

#ifdef __cplusplus
}
#endif

#endif
