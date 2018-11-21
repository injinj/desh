/* sigmsgs.h -- interface to signal name and message date ($Revision: 1.1.1.1 $) */
#ifndef __es__sigmsgs_h__
#define __es__sigmsgs_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int sig;
	const char *name, *msg;
} Sigmsgs;
extern const Sigmsgs signals[];

extern const int nsignals;

#ifdef __cplusplus
}
#endif

#endif
