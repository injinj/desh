/* signal.c -- signal handling ($Revision: 1.1.1.1 $) */

#include <desh/es.h>
#include <desh/sigmsgs.h>
#include <desh/input.h>

typedef void (*Sighandler)(int);

Boolean sigint_newline = TRUE;

jmp_buf slowlabel;
Atomic slow = FALSE;
Atomic interrupted = FALSE;
static Atomic sigcount;
static Atomic caught[NSIG];
static Sigeffect sigeffect[NSIG];

#ifndef	SA_NOCLDSTOP
#define	SA_NOCLDSTOP	0
#endif

#ifndef	SA_NOCLDWAIT
#define	SA_NOCLDWAIT	0
#endif

#ifndef	SA_INTERRUPT		/* for sunos */
#define	SA_INTERRUPT	0
#endif

#include <termios.h>
int has_job_control;
int shell_pgid;
int shell_tty;
struct termios shell_tmodes;


/*
 * name<->signal mappings
 */

extern int signumber(const char *name) {
	int i;
	char *suffix;
	if (!hasprefix(name, "sig"))
		return -1;
	for (i = 0; i < nsignals; i++)
		if (streq(signals[i].name, name))
			return signals[i].sig;
	i = strtol(name + 3, &suffix, 10);
	if (0 < i && i < NSIG && (suffix == NULL || *suffix == '\0'))
		return i;
	return -1;
}

extern char *signame(int sig) {
	int i;
	for (i = 0; i < nsignals; i++)
		if (signals[i].sig == sig)
			return (char *) signals[i].name;
	return str("sig%d", sig);
}

extern char *sigmessage(int sig) {
	int i;
	for (i = 0; i < nsignals; i++)
		if (signals[i].sig == sig)
			return (char *) signals[i].msg;
	return str("unknown signal %d", sig);
}

static Sighandler setsignal(int sig, Sighandler handler);

/*
 * the signal handler
 */

/* catcher -- catch (and defer) a signal from the kernel */
extern void catcher(int sig) {
	int errno_sav=errno;

#if 0 /* only do this for unreliable signals */
		signal(sig, catcher);
#endif

	if (hasforked) {
		/* exit unconditionally on a signal in a child process */
		exit(1);
	}
	
	if (caught[sig] == 0) {
		caught[sig] = TRUE;
		++sigcount;
	}
	interrupted = TRUE;
	if (slow)
		longjmp(slowlabel, 1);

	errno=errno_sav;
}


/*
 * setting and getting signal effects
 */

static Sighandler setsignal(int sig, Sighandler handler) {
	struct sigaction nsa, osa;
	sigemptyset(&nsa.sa_mask);
	nsa.sa_handler = handler;
	nsa.sa_flags = SA_INTERRUPT;
	if (sigaction(sig, &nsa, &osa) == -1)
		return SIG_ERR;
	return osa.sa_handler;
}

extern Sigeffect esignal(int sig, Sigeffect effect) {
	Sigeffect old;
	assert(0 < sig && sig <= NSIG);
	old = sigeffect[sig];
	if (effect != sig_nochange && effect != old) {
		switch (effect) {
		case sig_ignore:
			if (setsignal(sig, SIG_IGN) == SIG_ERR) {
				eprint("$&setsignals: cannot ignore %s\n", signame(sig));
				return old;
			}
			break;
		case sig_special:
			if (sig != SIGINT) {
				eprint("$&setsignals: special handler not defined for %s\n", signame(sig));
				return old;
			}
		case sig_catch:
		case sig_noop:
			if (setsignal(sig, catcher) == SIG_ERR) {
				eprint("$&setsignals: cannot catch %s\n", signame(sig));
				return old;
			}
			break;
		case sig_default:
			setsignal(sig, SIG_DFL);
			break;
		default:
			NOTREACHED;
		}
		sigeffect[sig] = effect;
	}
	return old;
}

extern void setsigeffects(const Sigeffect effects[]) {
	int sig;
	for (sig = 1; sig < NSIG; sig++)
		esignal(sig, effects[sig]);
}

extern void getsigeffects(Sigeffect effects[]) {
	memcpy(effects, sigeffect, sizeof sigeffect);
}


/*
 * internal signal blocking
 */

/* signal blocking stack */
static sigset_t *sigbs=NULL, *sigbs_tos=NULL, *sigbs_end=NULL;

/* internally blocks a signal */
extern void block(int sig) {
	static sigset_t ss/*, oss*/;
	if (sigbs_tos == sigbs_end) {
		int sz = sigbs_end-sigbs;
		sigbs = (sigset_t *) erealloc(sigbs, (sz+10)*sizeof(sigbs));
		sigbs_end = sigbs+sz+10;
	}
	sigemptyset(&ss);
	sigaddset(&ss, sig); 
	sigprocmask(SIG_BLOCK, &ss, sigbs_tos++);
	
}

/* restores the previous sig block mask from the stack */
extern void unblock(int sig) {  /* sig is only used as reminder */
	assert(sigbs-sigbs_tos>0);
	sigprocmask(SIG_SETMASK, --sigbs_tos, NULL);
}

/*
 * initialization
 */

extern void initsignals(Boolean interactive, Boolean allowdumps) {
	int sig;
	Push settor;

	for (sig = 0; sig < nsignals; sig++)
		if (signals[sig].sig < 1 || NSIG <= signals[sig].sig)
			panic(
				"initsignals: bad signal in sigmsgs.c: %s (see mksignal)",
				signals[sig].name
			);

	for (sig = 1; sig < NSIG; sig++) {
		Sighandler h;
		struct sigaction sa;
		sigaction(sig, NULL, &sa);
		h = sa.sa_handler;
		if (h == SIG_IGN)
			sigeffect[sig] = sig_ignore;
		else if (h == SIG_DFL || h == SIG_ERR)
			sigeffect[sig] = sig_default;
		else
			panic(
				"initsignals: bad incoming signal value for %s: %x",
				signame(sig), h
			);
	}

	if (interactive || sigeffect[SIGINT] == sig_default)
		esignal(SIGINT, sig_special);
	if (!allowdumps) {
		if (interactive)
			esignal(SIGTERM, sig_noop);
		if (interactive || sigeffect[SIGQUIT] == sig_default)
			esignal(SIGQUIT, sig_noop);
	}

	if (interactive) {
		shell_tty=STDIN_FILENO;
		shell_pgid=getpid();
                if (getpgid(shell_pgid) != shell_pgid && setpgid(shell_pgid,shell_pgid)<0) {
			has_job_control=0;
			eprint("Couldn't put "SHNAME" into it's own process group (errno=%d)\n", errno);
			tcgetattr(shell_tty, &shell_tmodes);
		} else {
			has_job_control=1;
			assign_tty(shell_tty, shell_pgid);
			tcgetattr(shell_tty, &shell_tmodes);
			/* the SIG_DFT action for SIGCHLD is to ignore the signal.
			 * SIG_IGN is treated special for SIGCHLD! */
			esignal(SIGCHLD, sig_default);
			esignal(SIGTTIN, sig_noop);
			esignal(SIGTTOU, sig_noop);
			esignal(SIGTSTP, sig_catch);
		}
	}	  
  
	/* here's the end-run around set-signals */
	varpush(&settor, "set-signals", NULL);
	vardef("signals", NULL, mksiglist());
	varpop(&settor);
}

extern void setsigdefaults(void) {
	int sig;
	for (sig = 1; sig < NSIG; sig++) {
		Sigeffect e = sigeffect[sig];
		if (e == sig_catch || e == sig_noop || e == sig_special)
			esignal(sig, sig_default);
	}
}


/*
 * utility functions
 */

extern Boolean issilentsignal(List *e) {
	return (termeq(e->term, "signal"))
		&& e->next != NULL
		&& termeq(e->next->term, "sigint");
}

extern List *mksiglist(void) {
	int sig = NSIG;
	Sigeffect effects[NSIG];
	getsigeffects(effects);
	Ref(List *, lp, NULL);
	while (--sig > 0) {
		int prefix;
		switch (effects[sig]) {
		default: panic("mksiglist: bad sigeffects for %s: %d", signame(sig), effects[sig]);
		case sig_default:	continue;
		case sig_catch:		prefix = '\0';	break;
		case sig_ignore:	prefix = '-';	break;
		case sig_noop:		prefix = '/';	break;
		case sig_special:	prefix = '.';	break;
		}
		Ref(char *, name, signame(sig));
		if (prefix != '\0')
			name = str("%c%s", prefix, name);
		Ref(Term *, term, mkstr(name));
		lp = mklist(term, lp);
		RefEnd2(term, name);
	}
	RefReturn(lp);
}


/*
 * signal delivery
 */

static int blocked = 0;

/* blocksignals -- turn off delivery of signals as exceptions */
extern void blocksignals(void) {
	assert(blocked >= 0);
	++blocked;
}

/* unblocksignals -- turn on delivery of signals as exceptions */
extern void unblocksignals(void) {
	assert(blocked > 0);
	--blocked;
}

/* sigchk -- throw the signal as an exception */
extern void sigchk(void) {
	int sig;

	if (sigcount == 0 || blocked)
		return;
	if (hasforked)
		/* exit unconditionally on a signal in a child process */
		exit(1);

	for (sig = 0;; sig++) {
		if (sig >= NSIG) {
			sigcount = 0;
			return;
		}
		if (caught[sig] != 0) {
			--sigcount;
			caught[sig] = 0;
			break;
		}
	}
	resetparser();
	Ref(List *, e,
	    mklist(mkstr("signal"), mklist(mkstr(signame(sig)), NULL)));

	switch (sigeffect[sig]) {
	case sig_catch:
		while (gcisblocked())
			gcenable();
		throw_exception(e);
		NOTREACHED;
	case sig_special:
		assert(sig == SIGINT);
		/* this is the newline you see when you hit ^C while typing a command */
		if (sigint_newline)
			eprint("\n");
		sigint_newline = TRUE;
		while (gcisblocked())
			gcenable();
		throw_exception(e);
		NOTREACHED;
		break;
	case sig_noop:
		break;
	default:
		/* panic("sigchk: caught %L with sigeffect %d", e, " ", sigeffect[sig]); */
		break;
	}
	RefEnd(e);
}
