/* term.h -- definition of term structure ($Revision: 1.1.1.1 $) */
#ifndef __es__term_h__
#define __es__term_h__

#ifdef __cplusplus
extern "C" {
#endif

struct Term {
	char *str;
	Closure *closure;
};

#ifdef __cplusplus
}
#endif

#endif
