/* prim.h -- definitions for es primitives ($Revision: 1.1.1.1 $) */
#ifndef __es__prim_h__
#define __es__prim_h__

#ifdef __cplusplus
extern "C" {
#endif

#define	PRIM(name)	static List *CONCAT(prim_,name)( \
				List *list, Binding *binding, int evalflags \
			)
#define	X(name)		(primdict = dictput( \
				primdict, \
				STRING(name), \
				(void *) CONCAT(prim_,name) \
			))

extern Dict *initprims_controlflow(Dict *primdict);	/* prim-ctl.c */
extern Dict *initprims_io(Dict *primdict);		/* prim-io.c */
extern Dict *initprims_etc(Dict *primdict);		/* prim-etc.cpp */
extern Dict *initprims_sys(Dict *primdict);		/* prim-sys.c */
extern Dict *initprims_rel(Dict *primdict);		/* prim-rel.cpp */
extern Dict *initprims_math(Dict *primdict);		/* prim-math.cpp */
extern Dict *initprims_proc(Dict *primdict);		/* proc.c */
extern Dict *initprims_access(Dict *primdict);		/* access.c */

#ifdef __cplusplus
}
#endif

#endif
