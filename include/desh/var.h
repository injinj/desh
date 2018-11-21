/* var.h -- es variables ($Revision: 1.1.1.1 $) */
#ifndef __es__var_h__
#define __es__var_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Var Var;
struct Var {
	List *defn;
	char *env;
	int flags;
};

#define	var_hasbindings		1
#define	var_isinternal		2

extern Dict *vars;

#ifdef __cplusplus
}
#endif

#endif
