/* main.c -- initialization for es ($Revision: 1.3 $) */
#define _GNU_SOURCE

#include <desh/es.h>
#include <desh/input.h>
#include <locale.h>
#if defined(__has_include)
#if __has_include(<xlocale.h>)
#include <xlocale.h>
#endif
#endif

#if GCVERBOSE
Boolean gcverbose	= FALSE;	/* -G */
#endif
#if GCINFO
Boolean gcinfo		= FALSE;	/* -I */
#endif


/* #if 0 && !HPUX && !defined(linux) && !defined(sgi) */
/* extern int getopt (int argc, char **argv, const char *optstring); */
/* #endif */

extern int optind;
extern char *optarg;

/* extern int isatty(int fd); */
extern char **environ;


/* checkfd -- open /dev/null on an fd if it is closed */
static void checkfd(int fd, OpenKind r) {
	int new;
	new = dup(fd);
	if (new != -1)
		close(new);
	else if (errno == EBADF && (new = eopen("/dev/null", r)) != -1)
		mvfd(new, fd);
}

/* initpath -- set $path based on the configuration default */
static void initpath(void) {
	int i;
	static const char * const path[] = { INITIAL_PATH };
	
	Ref(List *, list, NULL);
	for (i = arraysize(path); i-- > 0;) {
		Term *t = mkstr((char *) path[i]);
		list = mklist(t, list);
	}
	vardef("path", NULL, list);
	RefEnd(list);
}

/* initpid -- set $pid for this shell */
static void initpid(void) {
	vardef("pid", NULL, mklist(mkstr(str("%d", getpid())), NULL));
}

/* runstartup -- run a startup file */
static void runstartup(char *filename) {
	int fd = eopen(filename, oOpen);
	if (fd != -1) {
		ExceptionHandler
			runfd(fd, filename, 0);
		CatchException (e)
			if (termeq(e->term, "exit"))
				exit(exitstatus(e->next));
			else if (termeq(e->term, "error"))
				eprint("%L\n",
				       e->next == NULL ? NULL : e->next->next,
				       " ");
			else if (!issilentsignal(e))
				eprint("uncaught exception: %L\n", e, " ");
			return;
		EndExceptionHandler
	}
}
/* from Nix OS */
/* runenv -- run /etc/env and $HOME/.env, if each exists */
static void runenv(void) {
  runstartup(str("/etc/"SHNAME"env"));
  runstartup(str("%L/."SHNAME"env", varlookup("home", NULL), "\001"));
}

/* runprofile -- run /etc/profile and $HOME/.profile, if each exists */
static void runprofile(void) {
  runstartup(str("/etc/"SHNAME"profile"));
  runstartup(str("%L/."SHNAME"profile", varlookup("home", NULL), "\001"));
}

/* runrc -- run /etc/rc and $HOME/.rc, if each exists */
static void runrc(void) {
  runstartup(str("/etc/"SHNAME"rc"));
  runstartup(str("%L/."SHNAME"rc", varlookup("home", NULL), "\001"));
}

/* usage -- print usage message and die */
static noreturn usage(void) {
  eprint(
    "usage: " SHNAME " [-c command] [-silevxnpo] [file [args ...]]\n"
    "	-c cmd	execute argument\n"
    "	-s	read commands from standard input; stop option parsing\n"
    "	-i	interactive shell\n"
    "	-l	login shell\n"
    "	-e	exit if any command exits with false status\n"
    "	-v	print input to standard error\n"
    "	-x	print commands to standard error before executing\n"
    "	-n	just parse; don't execute\n"
    "	-p	don't load functions from the environment\n"
    "	-o	don't open stdin, stdout, and stderr if they were closed\n"
    "	-d	don't ignore SIGQUIT or SIGTERM\n"
#if GCINFO
    "	-I	print garbage collector information\n"
#endif
#if GCVERBOSE
    "	-G	print verbose garbage collector information\n"
#endif
#if LISPTREES
    "	-L	print parser results in LISP format\n"
#endif
	);
	exit(1);
}


/* main -- initialize, parse command arguments, and start running */
int main(int argc, char **argv) {
	int c;
	volatile int ac;
	char **volatile av;

	volatile int runflags = 0;		/* -[einvxL] */
	volatile Boolean protected = FALSE;	/* -p */
	volatile Boolean allowquit = FALSE;	/* -d */
	volatile Boolean cmd_stdin = FALSE;		/* -s */
	volatile Boolean loginshell = FALSE;	/* -l or $0[0] == '-' */
	Boolean keepclosed = FALSE;		/* -o */
	const char *volatile cmd = NULL;	/* -c */

	initgc();
	initconv();

	if (argc == 0) {
		argc = 1;
		argv = ealloc(2 * sizeof (char *));
		argv[0] = SHNAME;
		argv[1] = NULL;
	}
	if (*argv[0] == '-')
		loginshell = TRUE;

	while ((c = getopt(argc, argv, "eilxvnpodsc:?GIL")) != EOF)
		switch (c) {
		case 'c':	cmd = optarg;			break;
		case 'e':	runflags |= EVAL_EXITONFALSE;	break;
		case 'i':	runflags |= RUN_INTERACTIVE;	break;
		case 'n':	runflags |= RUN_NOEXEC;		break;
		case 'v':	runflags |= RUN_ECHOINPUT;	break;
		case 'x':	runflags |= RUN_PRINTCMDS;	break;
#if LISPTREES
		case 'L':	runflags |= RUN_LISPTREES;	break;
#endif
		case 'l':	loginshell = TRUE;		break;
		case 'p':	protected = TRUE;		break;
		case 'o':	keepclosed = TRUE;		break;
		case 'd':	allowquit = TRUE;		break;
		case 's':	cmd_stdin = TRUE;			goto getopt_done;
#if GCVERBOSE
		case 'G':	gcverbose = TRUE;		break;
#endif
#if GCINFO
		case 'I':	gcinfo = TRUE;			break;
#endif
		default:
			usage();
		}

getopt_done:
	if (cmd_stdin && cmd != NULL) {
		eprint(SHNAME": -s and -c are incompatible\n");
		exit(1);
	}

	if (!keepclosed) {
		checkfd(0, oOpen);
		checkfd(1, oCreate);
		checkfd(2, oCreate);
	}

	if (
		cmd == NULL
	     && (optind == argc || cmd_stdin)
	     && (runflags & RUN_INTERACTIVE) == 0
	     && isatty(0)
	)
		runflags |= RUN_INTERACTIVE;

	ac = argc;
	av = argv;

	ExceptionHandler
		roothandler = &_localhandler;	/* unhygeinic */

                /* from xs */
                uselocale(newlocale(LC_ALL_MASK, "", (locale_t)0));
		initinput();
		initprims();
		initvars();
	
		runinitial();
	
		initpath();
		initpid();
		initsignals(runflags & RUN_INTERACTIVE, allowquit);
		hidevariables();
		initenv(environ, protected);
	
	        /* Nix OS init */
		runenv();

		if (loginshell)
			runprofile();

		if ((runflags & RUN_INTERACTIVE) > 0)
			runrc();

		if (cmd == NULL && !cmd_stdin && optind < ac) {
			int fd;
			char *file = av[optind++];
			if ((fd = eopen(file, oOpen)) == -1) {
				eprint("%s: %s\n", file, esstrerror(errno));
				return 1;
			}
			vardef("*", NULL, listify(ac - optind, av + optind));
			vardef("0", NULL, mklist(mkstr(file), NULL));
			return exitstatus(runfd(fd, file, runflags));
		}
	
		vardef("*", NULL, listify(ac - optind, av + optind));
		vardef("0", NULL, mklist(mkstr(av[0]), NULL));
		if (cmd != NULL)
			return exitstatus(runstring(cmd, NULL, runflags));
		return exitstatus(runfd(0, "stdin", runflags));

	CatchException (e)

		if (termeq(e->term, "exit"))
			return exitstatus(e->next);
		else if (termeq(e->term, "error"))
			eprint("%L\n",
			       e->next == NULL ? NULL : e->next->next,
			       " ");
		else if (!issilentsignal(e))
			eprint("uncaught exception: %L\n", e, " ");
		return 1;

	EndExceptionHandler
}
