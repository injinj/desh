/* input.c -- read input from files or strings ($Revision: 1.2 $) */
/* stdgetenv is based on the FreeBSD getenv */

#include <es/es.h>
#include <es/input.h>
#include <es/var.h>

#include <stdint.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <linecook/linecook.h>
#include <linecook/ttycook.h>

/*
 * constants
 */

#define	BUFSIZE		((size_t) 1024)		/* buffer size to fill reads into */

/*
 * macros
 */

#define	ISEOF(in)	((in)->fill == eoffill)

/*
 * globals
 */

Input           * input;
int               prompt2; /* use prompt2 if 1 */
static char     * prompt[ MAX_PROMPT_COUNT ];
static LineCook * lc;
static TTYCook  * tty;

Boolean heredoc_input  = FALSE;
Boolean reset_terminal = FALSE;

static char * history_filename;

/*
 * errors and warnings
 */

/* locate -- identify where an error came from */
static char *locate(Input *in, char *s) {
	return (in->runflags & run_interactive)
		? s
		: str("%s:%d: %s", in->name, in->lineno, s);
}

static char *error = NULL;

/* yyerror -- yacc error entry point */
extern void yyerror(char *s) {
#if sgi
	/* this is so that trip.es works */
	if (streq(s, "Syntax error"))
		s = "syntax error";
#endif
	if (error == NULL)	/* first error is generally the most informative */
		error = locate(input, s);
}

/* warn -- print a warning */
static void warn(char *s) {
	eprint("warning: %s\n", locate(input, s));
}


/* sethistory -- change the file for the history log */
extern void sethistory(char *file) {
	history_filename = file;
}


/*
 * unget -- character pushback
 */

/* ungetfill -- input->fill routine for ungotten characters */
static int ungetfill(Input *in) {
	int c;
	assert(in->ungot > 0);
	c = in->unget[--in->ungot];
	if (in->ungot == 0) {
		assert(in->rfill != NULL);
		in->fill = in->rfill;
		in->rfill = NULL;
		assert(in->rbuf != NULL);
		in->buf = in->rbuf;
		in->rbuf = NULL;
	}
	return c;
}

/* unget -- push back one character */
extern void unget(Input *in, int c) {
	if (in->ungot > 0) {
		assert(in->ungot < MAXUNGET);
		in->unget[in->ungot++] = c;
	} else if (in->bufbegin < in->buf && in->buf[-1] == c && (input->runflags & run_echoinput) == 0)
		--in->buf;
	else {
		assert(in->rfill == NULL);
		in->rfill = in->fill;
		in->fill = ungetfill;
		assert(in->rbuf == NULL);
		in->rbuf = in->buf;
		in->buf = in->bufend;
		assert(in->ungot == 0);
		in->ungot = 1;
		in->unget[0] = c;
	}
}


/*
 * getting characters
 */

/* get -- get a character, filter out nulls */
static int get(Input *in) {
	int c;
	while ((c = (in->buf < in->bufend ? *in->buf++ : (*in->fill)(in))) == '\0')
		warn("null character ignored");
	return c;
}

/* getverbose -- get a character, print it to standard error */
static int getverbose(Input *in) {
	if (in->fill == ungetfill)
		return get(in);
	else {
		int c = get(in);
		if (c != EOF) {
			char buf = c;
			ewrite(2, &buf, 1);
		}
		return c;
	}
}

/* eoffill -- report eof when called to fill input buffer */
static int eoffill(Input *in) {
	assert(in->fd == -1);
	return EOF;
}

typedef struct {
  LineCook *lc;
  int count;
} VarsCompleteClosure;

static void
scan_dict_fn_complete( void *cl,  char *key,  void *value )
{
  if ( strncmp( "fn-", key, 3 ) == 0 ) {
    VarsCompleteClosure *vc = (VarsCompleteClosure *) cl;
    vc->count++;
    lc_add_completion( (LineCook *) vc->lc, 'e', &key[ 3 ],
                       strlen( &key[ 3 ] ) );
  }
}

static void
scan_dict_env_complete( void *cl,  char *key,  void *value )
{
  VarsCompleteClosure *vc = (VarsCompleteClosure *) cl;
  char   tmpkey[ 1024 ];
  size_t len = 0;

  if ( strncmp( "fn-", key, 3 ) != 0 ) {
    len = strlen( key );
    if ( ++len < sizeof( tmpkey ) ) {
      tmpkey[ 0 ] = '$';
      strcpy( &tmpkey[ 1 ], key );
      key = tmpkey;
    }
  }
  else if ( strncmp( "fn-$", key, 4 ) == 0 ) {
    key = &key[ 3 ];
    len = strlen( key );
  }
  if ( len > 0 ) {
    vc->count++;
    lc_add_completion( (LineCook *) vc->lc, 'v', key, len );
  }
}

static int
tty_complete( LineCook *lc,  const char *buf,  size_t off,  size_t len,
              int comp_type )
{
  int count = 0;
  if ( comp_type == 0 ) { /* if any completion type */
    VarsCompleteClosure cl;
    cl.lc    = lc;
    cl.count = 0;
    /* if first arg, or { precedes the offset */
    if ( off == 0 || ( off > 0 && buf[ off - 1 ] == '{' ) ) {
      dictforall( vars, scan_dict_fn_complete, &cl );
      count += cl.count;
      comp_type = 'e'; /* 1st argument, use exe completion */
    }
    else if ( len > 0 && buf[ off ] == '$' ) {
      dictforall( vars, scan_dict_env_complete, &cl );
      return cl.count; /* no need to continue $env completion */
      //comp_type = 'v';
    }
    /* if command starts with 'cd ', use directory completion */
    else if ( off + len >= 2 && buf[ 0 ] == 'c' && buf[ 1 ] == 'd' ) {
      if ( off + len == 2 || buf[ 2 ] == ' ' )
        comp_type = 'd';
    }
  }
  return count + lc_tty_file_completion( lc, buf, off, len, comp_type );
}

static int
tty_init( Input *in )
{
  const char * brk = " \t\n\\'`><=;|&{()}",
             * qc  = " \t\n\\\"'@<>=;|&()#$`?*[!:{";

  lc  = lc_create_state( 80, 25 );
  tty = lc_tty_create( lc );
  if ( lc == NULL || tty == NULL )
    return -1;

  if ( lc_tty_init_fd( tty, in->fd, 1 )  != 0 ||
       lc_tty_set_default_prompts( tty ) != 0 ||
       lc_tty_init_geom( tty )           != 0 ||
       lc_tty_init_sigwinch( tty )       != 0 )
    return -1;

  lc_set_completion_break( lc, brk, strlen( brk ) );
  lc_set_quotables( lc, qc, strlen( qc ), '\'' );
  if ( history_filename != NULL )
    lc_tty_open_history( tty, history_filename );
  lc->complete_cb = tty_complete;

  return 0;
}

static int
tty_read( Input *in )
{
  int r;
  if ( tty == NULL || tty->in_fd != in->fd )
    if ( tty_init( in ) != 0 )
      return -1;
  /* set the current prompts */
  for ( r = 0; r < TTYP_MAX; r++ ) {
    if ( prompt[ r ] != NULL &&
         lc_tty_set_prompt( tty, (TTYPrompt) r, prompt[ r ] ) != 0 )
      lc_tty_set_default_prompt( tty, (TTYPrompt) r );
  }
  if ( ! prompt2 ) /* flush history buffer if not a continuation */
    lc_tty_flush_history( tty );
  lc_tty_set_continue( tty, prompt2 ); /* use prompt2 continue when 1 */
  for (;;) {
    r = lc_tty_get_line( tty ); /* retry line and run timed events */
    if ( r < 0 )
      break;
    if ( r > 0 ) { /* if a line avail */
      r = tty->line_len + 1;
      if ( in->buflen < r ) {
        while ( in->buflen < r )
          in->buflen *= 2;
        in->bufbegin = erealloc( in->bufbegin, in->buflen );
      }
      memcpy( in->bufbegin, tty->line, r - 1 );
      in->bufbegin[ r - 1 ] = '\n';
      /* save line for history, don't know if it will be a continuation
       * until after lex runs */
      if ( heredoc_input )
        lc_tty_break_history( tty ); /* anything buffered will be tossed */
      else
        lc_tty_push_history( tty, tty->line, tty->line_len );
      break;
    }
    /* r == 0 to continue, caused by EAGAIN or empty action,
     * poll will recognize EAGAIN and not do anything for the other case */
    /* if signaled while in lc_get_line() with read() errno=EINTR or
     * an ctrl-c is typed at the terminal */
    if ( tty->lc_status != LINE_STATUS_INTERRUPT ) {
      r = lc_tty_poll_wait( tty, 250 ); /* wait at most 250ms */
      if ( r < 0 ) /* if error in poll wait */
        break;
    }
    /* if signaled while in lc_get_line() / poll EINTR */
    if ( tty->lc_status == LINE_STATUS_INTERRUPT ) {
      errno = EINTR;
      r = -1;
      break;
    }
  }
  lc_tty_normal_mode( tty ); /* reset terminal to normal state */

  if ( r < 0 )
    return -1;
  return r;
}

/* fdfill -- fill input buffer by reading from a file descriptor */
static int
fdfill( Input *in )
{
  long    nread;
  assert( in->buf == in->bufend );
  assert( in->fd >= 0 );

  do {
    if ( ( in->runflags & run_interactive ) != 0 )
      nread = tty_read( in );
    else
      nread = eread( in->fd, (char *) in->bufbegin, in->buflen );
    SIGCHK();
  } while ( nread == -1 && errno == EINTR );
  if ( nread <= 0 ) {
    close( in->fd );
    in->fd   = -1;
    in->fill = eoffill;
    in->runflags &= ~run_interactive;
    if ( nread == -1 )
      fail( "$&parse", "%s: %s", in->name == NULL ? "es" : in->name,
            esstrerror( errno ) );
    return EOF;
  }

  in->buf    = in->bufbegin;
  in->bufend = &in->buf[ nread ];
  return *in->buf++;
}

/*
 * the input loop
 */

/* parse -- call yyparse(), but disable garbage collection and catch errors */
extern Tree *parse(char **pr) {
	int result;
        int i = 0;
	assert(error == NULL);

	inityy();
	emptyherequeue();

	if (ISEOF(input))
		throw_exception(mklist(mkstr("eof"), NULL));

        prompt2 = 0;
        if ( pr != NULL )
          for ( ; i < MAX_PROMPT_COUNT && pr[ i ] != NULL; i++ )
            prompt[ i ] = pr[ i ];
        for ( ; i < MAX_PROMPT_COUNT; i++ )
          prompt[ i ] = NULL;

	gcreserve(300 * sizeof (Tree));
	gcdisable();
	result = yyparse();
	gcenable();

	if (result || error != NULL) {
		char *e;
		assert(error != NULL);
		e = error;
		error = NULL;
		fail("$&parse", "%s", e);
	}
#if LISPTREES
	if (input->runflags & run_lisptrees)
		eprint("%B\n", parsetree);
#endif
	return parsetree;
}

/* resetparser -- clear parser errors in the signal handler */
extern void resetparser(void) {
	error = NULL;
}

/* runinput -- run from an input source */
extern List *runinput(Input *in, int runflags) {
	volatile int flags = runflags;
	List * volatile result = NULL;
	List *repl, *dispatch;
	Push push;
	const char *dispatcher[] = {
		"fn-%eval-noprint",
		"fn-%eval-print",
		"fn-%noeval-noprint",
		"fn-%noeval-print",
	};

	flags &= ~eval_inchild;
	in->runflags = flags;
	in->get = (flags & run_echoinput) ? getverbose : get;
	in->prev = input;
	input = in;

	ExceptionHandler

		dispatch
	          = varlookup(dispatcher[((flags & run_printcmds) ? 1 : 0)
					 + ((flags & run_noexec) ? 2 : 0)],
			      NULL);
		if (flags & eval_exitonfalse)
			dispatch = mklist(mkstr("%exit-on-false"), dispatch);
		varpush(&push, "fn-%dispatch", dispatch);
	
		repl = varlookup((flags & run_interactive)
				   ? "fn-%interactive-loop"
				   : "fn-%batch-loop",
				 NULL);
		result = (repl == NULL)
				? prim("batchloop", NULL, NULL, flags)
				: eval(repl, NULL, flags);
	
		varpop(&push);

	CatchException (e)

		(*input->cleanup)(input);
		input = input->prev;
		throw_exception(e);

	EndExceptionHandler

	input = in->prev;
	(*in->cleanup)(in);
	return result;
}


/*
 * pushing new input sources
 */

/* fdcleanup -- cleanup after running from a file descriptor */
static void fdcleanup(Input *in) {
	unregisterfd(&in->fd);
	if (in->fd != -1)
		close(in->fd);
	efree(in->bufbegin);
}

/* runfd -- run commands from a file descriptor */
extern List *runfd(int fd, const char *name, int flags) {
	Input in;
	List *result;

	memzero(&in, sizeof (Input));
	in.lineno = 1;
	in.fill = fdfill;
	in.cleanup = fdcleanup;
	in.fd = fd;
	registerfd(&in.fd, TRUE);
	in.buflen = BUFSIZE;
	in.bufbegin = in.buf = ealloc(in.buflen);
	in.bufend = in.bufbegin;
	in.name = (name == NULL) ? str("fd %d", fd) : name;

	RefAdd(in.name);
	result = runinput(&in, flags);
	RefRemove(in.name);

	return result;
}

/* stringcleanup -- cleanup after running from a string */
static void stringcleanup(Input *in) {
	efree(in->bufbegin);
}

/* stringfill -- placeholder than turns into EOF right away */
static int stringfill(Input *in) {
	in->fill = eoffill;
	return EOF;
}

/* runstring -- run commands from a string */
extern List *runstring(const char *str, const char *name, int flags) {
	Input in;
	List *result;
	unsigned char *buf;

	assert(str != NULL);

	memzero(&in, sizeof (Input));
	in.fd = -1;
	in.lineno = 1;
	in.name = (name == NULL) ? str : name;
	in.fill = stringfill;
	in.buflen = strlen(str);
	buf = ealloc(in.buflen + 1);
	memcpy(buf, str, in.buflen);
	in.bufbegin = in.buf = buf;
	in.bufend = in.buf + in.buflen;
	in.cleanup = stringcleanup;

	RefAdd(in.name);
	result = runinput(&in, flags);
	RefRemove(in.name);
	return result;
}

/* parseinput -- turn an input source into a tree */
extern Tree *parseinput(Input *in) {
	Tree * volatile result = NULL;

	in->prev = input;
	in->runflags = 0;
	in->get = get;
	input = in;

	ExceptionHandler
		result = parse(NULL);
		if (get(in) != EOF)
			fail("$&parse", "more than one value in term");
	CatchException (e)
		(*input->cleanup)(input);
		input = input->prev;
		throw_exception(e);
	EndExceptionHandler

	input = in->prev;
	(*in->cleanup)(in);
	return result;
}

/* parsestring -- turn a string into a tree; must be exactly one tree */
extern Tree *parsestring(const char *str) {
	Input in;
	Tree *result;
	unsigned char *buf;

	assert(str != NULL);

	/* TODO: abstract out common code with runstring */

	memzero(&in, sizeof (Input));
	in.fd = -1;
	in.lineno = 1;
	in.name = str;
	in.fill = stringfill;
	in.buflen = strlen(str);
	buf = ealloc(in.buflen + 1);
	memcpy(buf, str, in.buflen);
	in.bufbegin = in.buf = buf;
	in.bufend = in.buf + in.buflen;
	in.cleanup = stringcleanup;

	RefAdd(in.name);
	result = parseinput(&in);
	RefRemove(in.name);
	return result;
}

/* isinteractive -- is the innermost input source interactive? */
extern Boolean isinteractive(void) {
	return input == NULL ? FALSE : ((input->runflags & run_interactive) != 0);
}


/*
 * initialization
 */

/* initinput -- called at dawn of time from main() */
extern void initinput(void) {
        int i;
	input = NULL;

	/* declare the global roots */
	globalroot( &history_filename );        /* history file */
	globalroot( &error );		        /* parse errors */
        for ( i = 0; i < MAX_PROMPT_COUNT; i++ )
          globalroot( &prompt[ i ] );           /* prompts */

	/* call the parser's initialization */
	initparse();
}
