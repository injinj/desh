/* prim-ctl.c -- control flow primitives ($Revision: 1.1.1.1 $) */

#include <desh/es.h>
#include <desh/prim.h>

PRIM(seq) {
	Ref(List *, result, ltrue);
	Ref(List *, lp, list);
	for (; lp != NULL; lp = lp->next)
		result = eval1(lp->term, evalflags &~ (lp->next == NULL ? 0 : EVAL_INCHILD));
	RefEnd(lp);
	RefReturn(result);
}

PRIM(if) {
	Ref(List *, lp, list);
	for (; lp != NULL; lp = lp->next) {
		List *cond = eval1(lp->term, evalflags & (lp->next == NULL ? EVAL_INCHILD : 0));
		lp = lp->next;
		if (lp == NULL) {
			RefPop(lp);
			return cond;
		}
		if (istrue(cond)) {
			List *result = eval1(lp->term, evalflags);
			RefPop(lp);
			return result;
		}
	}
	RefEnd(lp);
	return ltrue;
}

PRIM(forever) {
	Ref(List *, body, list);
	for (;;)
		list = eval(body, NULL, evalflags & EVAL_EXITONFALSE);
	RefEnd(body);
	return list;
}

PRIM(throw) {
	if (list == NULL)
		fail("$&throw", "usage: throw exception [args ...]");
	throw_exception(list);
	NOTREACHED;
}

PRIM(catch) {
	Atomic retry;

	if (list == NULL)
		fail("$&catch", "usage: catch catcher body");

	Ref(List *, result, NULL);
	Ref(List *, lp, list);

	do {
		retry = FALSE;

		ExceptionHandler

			result = eval(lp->next, NULL, evalflags);

		CatchException (frombody)

			blocksignals();
			ExceptionHandler
				result
				  = eval(mklist(mkstr("$&noreturn"),
					        mklist(lp->term, frombody)),
					 NULL,
					 evalflags);
				unblocksignals();
			CatchException (fromcatcher)

				if (termeq(fromcatcher->term, "retry")) {
					retry = TRUE;
					unblocksignals();
				} else {
		
					unblocksignals();
					throw_exception(fromcatcher);
				}
	   
			EndExceptionHandler

		EndExceptionHandler
	} while (retry);
	RefEnd(lp);
	RefReturn(result);
}

extern Dict *initprims_controlflow(Dict *primdict) {
	X(seq);
	X(if);
	X(throw);
	X(forever);
	X(catch);
	return primdict;
}
