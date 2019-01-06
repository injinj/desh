/* input.c -- read input from files or strings ($Revision: 1.2 $) */
/* stdgetenv is based on the FreeBSD getenv */

#include <desh/es.h>
#include <desh/input.h>
#include <desh/var.h>

#include <stdio.h>
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
extern "C" {
Input           * input;
static char     * prompt[ MAX_PROMPT_COUNT ];
static LineCook * lc;
static TTYCook  * tty;

Boolean is_prompt2; /* use prompt2 if 1 */
Boolean heredoc_input  = FALSE;
Boolean reset_terminal = FALSE;
Boolean is_completion  = FALSE;

static char * history_filename;
}
/*
 * errors and warnings
 */

/* locate -- identify where an error came from */
static const char *
locate( Input *in, const char *s )
{
  return ( in->runflags & RUN_INTERACTIVE )
           ? s
           : str( "%s:%d: %s", in->name, in->lineno, s );
}

static const char *error = NULL;

/* yyerror -- yacc error entry point */
extern "C" void
yyerror( char *s )
{
  if ( error == NULL ) /* first error is generally the most informative */
    error = locate( input, s );
}

/* warn -- print a warning */
static void
warn( const char *s )
{
  eprint( "warning: %s\n", locate( input, s ) );
}

/* sethistory -- change the file for the history log */
extern "C" void
sethistory( char *file )
{
  history_filename = file;
}

extern "C" void
setevalstatus( char *status )
{
  if ( lc != NULL && is_completion == FALSE ) {
    int res  = 0;
    bool neg = false;
    if ( status != NULL ) {
      if ( status[ 0 ] == '-' ) {
        neg = true;
        status++;
      }
      if ( status[ 0 ] >= '0' && status[ 0 ] <= '9' )
        res = atoi( status ) * ( neg ? -1 : 1 );
    }
    lc_set_eval_status( lc, res );
  }
}

/*
 * unget -- character pushback
 */

/* ungetfill -- input->fill routine for ungotten characters */
static int
ungetfill( Input *in )
{
  int c;
  assert( in->ungot > 0 );
  c = in->unget[ --in->ungot ];
  if ( in->ungot == 0 ) {
    assert( in->rfill != NULL );
    in->fill  = in->rfill;
    in->rfill = NULL;
    assert( in->rbuf != NULL );
    in->buf  = in->rbuf;
    in->rbuf = NULL;
  }
  return c;
}

/* unget -- push back one character */
extern "C" void
unget( Input *in, int c )
{
  if ( in->ungot > 0 ) {
    assert( in->ungot < MAXUNGET );
    in->unget[ in->ungot++ ] = c;
  }
  else if ( in->bufbegin < in->buf && in->buf[ -1 ] == c &&
            ( input->runflags & RUN_ECHOINPUT ) == 0 )
    --in->buf;
  else {
    assert( in->rfill == NULL );
    in->rfill = in->fill;
    in->fill  = ungetfill;
    assert( in->rbuf == NULL );
    in->rbuf = in->buf;
    in->buf  = in->bufend;
    assert( in->ungot == 0 );
    in->ungot      = 1;
    in->unget[ 0 ] = c;
  }
}

/*
 * getting characters
 */

/* get -- get a character, filter out nulls */
static int
get( Input *in )
{
  for (;;) {
    int c;
    if ( in->buf < in->bufend )
      c = *in->buf++;
    else
      c = ( *in->fill )( in );
    if ( c != 0 )
      return c;
    if ( ( in->runflags & RUN_INTERRUPT ) != 0 ) {
      in->runflags &= ~RUN_INTERRUPT;
      return 0;
    }
    warn( "null character ignored" );
  }
}

/* getverbose -- get a character, print it to standard error */
static int
getverbose( Input *in )
{
  if ( in->fill == ungetfill )
    return get( in );
  else {
    int c = get( in );
    if ( c != EOF ) {
      char buf = c;
      ewrite( 2, &buf, 1 );
    }
    return c;
  }
}

/* eoffill -- report eof when called to fill input buffer */
static int
eoffill( Input *in )
{
  assert( in->fd == -1 );
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
    lc_add_completion( (LineCook *) vc->lc, &key[ 3 ], strlen( &key[ 3 ] ) );
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
    lc_add_completion( (LineCook *) vc->lc, key, len );
  }
}

/* getenv -- fake version of getenv for readline (or other libraries) */
static char *
esgetenv( const char *name )
{
  static Dict *  envdict;
  static Boolean initialized = FALSE;

  XRef<List *> value( varlookup( name, NULL ) );
  XRef<char *> string( NULL );
  char       * exstr;

  if ( value.ptr == NULL )
    return NULL;

  gcdisable();
  if ( !initialized ) {
    initialized = TRUE;
    envdict     = mkdict();
    globalroot( &envdict );
  }

  string.ptr = (char *) dictget( envdict, name );
  if ( string.ptr != NULL )
    efree( string.ptr );

  exstr = str( "%W", value.ptr );
  string.ptr = (char *) ealloc( strlen( exstr ) + 1 );
  strcpy( string.ptr, exstr );
  envdict = dictput( envdict, (char *) name, string.ptr );

  gcenable();
  return string.ptr;
}

extern "C" char **environ;
static char *
stdgetenv( const char *name )
{
  int          len;
  const char * np;
  char **      p, *c;

  if ( name == NULL || environ == NULL )
    return NULL;
  for ( np = name; *np && *np != '='; ++np )
    continue;
  len = np - name;
  for ( p = environ; ( c = *p ) != NULL; ++p ) {
    if ( strncmp( c, name, len ) == 0 && c[ len ] == '=' ) {
      return ( c + len + 1 );
    }
  }
  return NULL;
}

static char *stdgetenv(const char *);
static char *(*realgetenv)(const char *) = stdgetenv;

extern "C" char *
getenv( const char *name )
{
  return realgetenv( name );
}

void
initgetenv( void )
{
  realgetenv = esgetenv;
}

static int
tty_complete( LineCook *lc,  const char *buf,  size_t off,  size_t len )
{
  CompleteType ctype = lc_get_complete_type( lc );
  int count = 0;
  if ( ctype == COMPLETE_ANY ) { /* if any completion type */
    VarsCompleteClosure cl;
    cl.lc    = lc;
    cl.count = 0;
    /* if first arg, or { precedes the offset */
    if ( off == 0 || ( off > 0 && buf[ off - 1 ] == '{' ) ) {
      lc_set_complete_type( lc, COMPLETE_EXES );
      dictforall( vars, scan_dict_fn_complete, &cl );
      count += cl.count;
    }
    else if ( len > 0 && buf[ off ] == '$' ) {
      lc_set_complete_type( lc, COMPLETE_ENV );
      dictforall( vars, scan_dict_env_complete, &cl );
      return cl.count; /* no need to continue $env completion */
    }
    /* if command starts with 'cd ', use directory completion */
    else if ( off + len >= 2 && buf[ 0 ] == 'c' && buf[ 1 ] == 'd' ) {
      if ( off + len == 2 || buf[ 2 ] == ' ' )
        lc_set_complete_type( lc, COMPLETE_DIRS );
    }
  }
  return count + lc_tty_file_completion( lc, buf, off, len );
}

static struct save_kb {
  struct save_kb * next;
  char          ** args;
  int              argc;
} * kb_hd, * kb_tl;

extern "C" int
keybind( char **args, int argc )
{
  int i;
  if ( tty == NULL ) { /* save the bindkeys until tty is opened */
    struct save_kb * kb;
    char * s;
    size_t len;
    len = sizeof( struct save_kb ) * sizeof( char * ) * argc;
    for ( i = 0; i < argc; i++ )
      len += strlen( args[ i ] ) + 1;
    kb = (struct save_kb *) ealloc( len );
    memset( kb, 0, len );
    kb->args = (char **) (void *) &kb[ 1 ];
    kb->argc = argc;
    s = (char *) (void *) &kb->args[ argc ];
    for ( i = 0; i < argc; i++ ) {
      kb->args[ i ] = s;
      strcpy( s, args[ i ] );
      s = &s[ strlen( s ) + 1 ];
    }
    if ( kb_hd == NULL )
      kb_hd = kb;
    else
      kb_tl->next = kb;
    kb_tl = kb;
    return 0;
  }
  return lc_bindkey( tty->lc, args, argc ); /* binding can happen */
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

  while ( kb_hd != NULL ) { /* set and free the bindkeys */
    kb_tl = kb_hd->next;
    lc_bindkey( lc, kb_hd->args, kb_hd->argc );
    efree( kb_hd );
    kb_hd = kb_tl;
  }
  return 0;
}

static size_t
copy_term( char *s,  size_t off,  size_t max,  char *t,  size_t len )
{
  size_t i = 0;
  if ( len == 0 ) { /* use '' for when empty */
  empty_arg:;
    s[ off++ ] = '\'';
    s[ off++ ] = '\'';
    return off;
  }
  if ( t[ 0 ] != '\'' ) { /* strip first and last quotes if they exist */
    s[ off++ ] = '\'';
    s[ off++ ] = t[ i++ ];
  }
  else {
    if ( len == 1 )
      goto empty_arg;
    i++;
  }
  if ( len > 1 ) {
    for ( ; i < len - 1; i++ ) {
      if ( off >= max - 3 )
        break;
      if ( t[ i ] == '\'' ) /* double '' */
        s[ off++ ] = '\'';
      s[ off++ ] = t[ i ];
    }
    if ( t[ i ] != '\'' )
      s[ off++ ] = t[ i ];
  }
  s[ off++ ] = '\'';
  return off;
}

static int
tty_completion( Input *in )
{
  static char default_complete[] = "fn-default_complete",
              history_complete[] = "fn-_history_complete",
              man_complete[]     = "fn-_man_complete",
              help_complete[]    = "fn-_help_complete",
              next_complete[]    = "fn-_next_complete";
  /*#define FZFCMD "compreply=`{find . -print | fzf '--layout=reverse' '--height=50%'}"*/
  CompleteType ctype = lc_get_complete_type( lc );
  const char * s, * fn;
  char   cmd[ 1024 ],
         term[ 128 ],
         func[ 128 ],
         complete[ 1024 ];
  int    arg_num,   /* which arg is completed, 0 = first */
         arg_count, /* how many args */
         arg_off[ 128 ], /* offset of args */
         arg_len[ 128 ], /* length of args */
         n = lc_tty_get_completion_term( tty, term, sizeof( term ) ),
         m = lc_tty_get_completion_cmd( tty, cmd, sizeof( cmd ),
                                        &arg_num, &arg_count,
                                        arg_off, arg_len, 128 ),
         i;
  size_t off;
  if ( n < 0 || m < 0 || arg_count == 0 || arg_len[ 0 ] == 0 ||
       arg_len[ 0 ] > (int) ( sizeof( func ) - 16 ) )
    return -1;
  fn = NULL;
  switch ( ctype ) {
    default:
      /* look for function xyz_complete */
      memcpy( func, "fn-", 3 );
      i = 3;
      memcpy( &func[ 3 ], &cmd[ arg_off[ 0 ] ], arg_len[ 0 ] );
      i += arg_len[ 0 ];
      strcpy( &func[ i ], "_complete" );
      if ( varlookup( func, NULL ) != NULL ) {
        fn = &func[ 3 ];
        break;
      }
      /* FALLTHRU */
    case COMPLETE_FZF: /* look for function default_complete */
      if ( varlookup( default_complete, NULL ) != NULL )
        fn = &default_complete[ 3 ];
      break;
    case COMPLETE_HIST: /* look for function history_complete */
      if ( varlookup( history_complete, NULL ) != NULL )
        fn = &history_complete[ 3 ];
      break;
    case COMPLETE_HELP: /* look for function help_complete */
      if ( varlookup( help_complete, NULL ) != NULL )
        fn = &help_complete[ 3 ];
      break;
    case COMPLETE_MAN: /* look for function man_complete */
      if ( varlookup( man_complete, NULL ) != NULL )
        fn = &man_complete[ 3 ];
      break;
    case COMPLETE_NEXT: /* look for function next_complete */
      if ( varlookup( next_complete, NULL ) != NULL )
        fn = &next_complete[ 3 ];
      break;
  }
  if ( fn == NULL )
    return 0; /* no complete function found, use internal complete */
  strcpy( complete, "compreply = <={" );
  off = strlen( complete );
  strcpy( &complete[ off ], fn );
  off += strlen( &complete[ off ] );

  /* complete function args are:
   *   xyz_complete <ctype> <argnum> <term> <argv...>
   *
   * ctype is below, argnum is the arg that is completed
   * term is the text of the argnum that is completed
   * argv is an array of strings split by spaces
   */
  switch ( ctype ) {
    default:
    case COMPLETE_ANY:   s = "any";   break;
    case COMPLETE_FILES: s = "files"; break;
    case COMPLETE_DIRS:  s = "dirs";  break;
    case COMPLETE_EXES:  s = "exes";  break;
    case COMPLETE_HIST:  s = "hist";  break;
    case COMPLETE_SCAN:  s = "scan";  break;
    case COMPLETE_ENV:   s = "env";   break;
    case COMPLETE_FZF:   s = "fzf";   break;
    case COMPLETE_HELP:  s = "help";  break;
    case COMPLETE_MAN:   s = "man";   break;
    case COMPLETE_NEXT:  s = "next";  break;
  }
  complete[ off++ ] = ' ';
  while ( *s != '\0' )
    complete[ off++ ] = *s++;
  complete[ off++ ] = ' ';
  off = copy_term( complete, off, sizeof( complete ) - 10, term, n );
  complete[ off++ ] = ' ';
  arg_num++; /* 1s based arg: argv[1] = cmd */
  if ( arg_num > 100 )
    complete[ off++ ] = ( arg_num / 100 ) + '0';
  if ( arg_num > 10 )
    complete[ off++ ] = ( ( arg_num / 10 ) % 10 ) + '0';
  complete[ off++ ] = ( arg_num % 10 ) + '0';
  for ( i = 0; i < arg_count; i++ ) {
    if ( off + 4 + arg_len[ i ] + 2 > sizeof( complete ) )
      break;
    complete[ off++ ] = ' ';
    off = copy_term( complete, off, sizeof( complete ) - 6,
                     &cmd[ arg_off[ i ] ], arg_len[ i ] );
  }
  strcpy( &complete[ off ], "}\n" );
  off += 2;
  if ( in->buflen < off ) {
    while ( in->buflen < off )
      in->buflen *= 2;
    in->bufbegin = (unsigned char *) erealloc( in->bufbegin, in->buflen );
  }
  memcpy( in->bufbegin, complete, off );
  is_completion = TRUE;
  return (int) off;
}

static int
tty_read( Input *in )
{
  int r;
  Boolean is_signal;

  if ( tty == NULL || tty->in_fd != in->fd ) {
    if ( tty != NULL ) {
      warn( "Linecook: Reinitializing in_fd" );
    }
    if ( tty_init( in ) != 0 )
      return -1;
  }
  /* set the current prompts */
  for ( r = 0; r < TTYP_MAX; r++ ) {
    if ( prompt[ r ] != NULL &&
         lc_tty_set_prompt( tty, (TTYPrompt) r, prompt[ r ] ) != 0 )
      lc_tty_set_default_prompt( tty, (TTYPrompt) r );
  }

  /* flush history buffer if not a continuation */
  if ( ! is_prompt2 && ! is_completion )
    lc_tty_flush_history( tty );
  if ( is_completion ) {
    XRef<List *> list( NULL );
    list.ptr = varlookup( "compreply", NULL );
    for ( ; list.ptr != NULL; list.ptr = list.ptr->next ) {
      char *str = getstr( list.ptr->term );
      if ( str != NULL && str[ 0 ] != 0 )
        lc_add_completion( tty->lc, str, strlen( str ) );
    }
    is_completion = FALSE;
  }
  lc_tty_set_continue( tty, is_prompt2 ); /* use prompt2 continue when 1 */
  is_signal = FALSE;
  for (;;) {
    r = lc_tty_get_line( tty ); /* retry line and run timed events */
    if ( r < 0 )
      break;
    if ( r == 0 && tty->lc_status == LINE_STATUS_COMPLETE ) {
      if ( (r = tty_completion( in )) > 0 )
        break;
    }
    if ( r > 0 ) { /* if a line avail */
      size_t len = tty->line_len + 1;
      if ( in->buflen < len ) {
        while ( in->buflen < len )
          in->buflen *= 2;
        in->bufbegin = (unsigned char *) erealloc( in->bufbegin, in->buflen );
      }
      memcpy( in->bufbegin, tty->line, len - 1 );
      in->bufbegin[ len - 1 ] = '\n';
      r = (int) len;
      /* save line for history, don't know if it will be a continuation
       * until after lex runs */
      if ( heredoc_input )
        lc_tty_break_history( tty ); /* anything buffered will be tossed */
      else
        lc_tty_push_history( tty, tty->line, tty->line_len );
      break;
    }
    /* r == 0 to continue, caused by EAGAIN or EINTR or empty action,
     * poll will recognize EAGAIN and not do anything for the other case */
    if ( errno == EINTR ) { /* check for signals */
      r = -1;
      is_signal = TRUE;
      break;
    }
    /* if ctrl-c is typed at the terminal */
    if ( tty->lc_status != LINE_STATUS_INTERRUPT ) {
      r = lc_tty_poll_wait( tty, 500 ); /* wait at most 500ms */
      if ( r < 0 ) /* if error in poll wait */
        break;
      if ( r == 0 && errno == EINTR ) { /* check for signals */
        r = -1;
        is_signal = TRUE;
        break;
      }
    }
    /* if signaled while in lc_get_line() / poll EINTR */
    if ( tty->lc_status == LINE_STATUS_INTERRUPT ) {
      r = -1;
      break;
    }
  }
  lc_tty_normal_mode( tty ); /* reset terminal to normal state */

  if ( r < 0 ) {
    if ( ! is_signal ) {
      switch ( tty->lc_status ) {
        case LINE_STATUS_BAD_INPUT: /* XXX is fail is correct error? */
          warn( "Linecook: Bad UTF8 Input" );
          break;
        case LINE_STATUS_BAD_PROMPT:
          warn( "Linecook: Invalid Prompt" );
          break;
        case LINE_STATUS_BAD_CURSOR:
          warn( "Linecook: Bad Cursor Position" );
          break;

        case LINE_STATUS_WR_FAIL:
        case LINE_STATUS_RD_FAIL:
          return -2; /* eof */

        case LINE_STATUS_SUSPEND:
        case LINE_STATUS_INTERRUPT:
          in->runflags |= RUN_INTERRUPT; /* cause yyabort w/ERROR token */
          r = 2;
          in->bufbegin[ 0 ] = 0;
          in->bufbegin[ 1 ] = '\n';
          return r;

        default:
          break;
      }
    }
    errno = EINTR;
    return -1;
  }
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
    if ( ( in->runflags & RUN_INTERACTIVE ) != 0 )
      nread = tty_read( in );
    else
      nread = eread( in->fd, (char *) in->bufbegin, in->buflen );
    SIGCHK();
  } while ( nread == -1 && errno == EINTR );
  if ( nread <= 0 ) {
    close( in->fd );
    in->fd   = -1;
    in->fill = eoffill;
    in->runflags &= ~RUN_INTERACTIVE;
    if ( nread == -1 )
      fail( "$&parse", "%s: %s", in->name == NULL ? SHNAME : in->name,
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
extern "C" Tree *
parse( char **pr )
{
  int result;
  int i = 0;
  assert( error == NULL );

  inityy();
  emptyherequeue();

  if ( ISEOF( input ) ) {
    static char eof[] = "eof";
    throw_exception( mklist( mkstr( eof ), NULL ) );
  }

  is_prompt2    = FALSE;
  if ( pr != NULL )
    for ( ; i < MAX_PROMPT_COUNT && pr[ i ] != NULL; i++ )
      prompt[ i ] = pr[ i ];
  for ( ; i < MAX_PROMPT_COUNT; i++ )
    prompt[ i ] = NULL;

  gcreserve( 300 * sizeof( Tree ) );
  gcdisable();
  result = yyparse();
  gcenable();

  if ( result || error != NULL ) {
    const char *e;
    assert( error != NULL );
    e     = error;
    error = NULL;
    fail( "$&parse", "%s", e );
  }
#if LISPTREES
  if ( input->runflags & RUN_LISPTREES )
    eprint( "%B\n", parsetree );
#endif
  return parsetree;
}

/* resetparser -- clear parser errors in the signal handler */
extern "C" void
resetparser( void )
{
  error = NULL;
}

/* runinput -- run from an input source */
extern "C" List *
runinput( Input *in, int runflags )
{
  volatile int flags    = runflags;
  List * volatile result = NULL;
  List * repl, * dispatch;
  Push   push;
  const char *dispatcher[] = {
    "fn-%eval-noprint",
    "fn-%eval-print",
    "fn-%noeval-noprint",
    "fn-%noeval-print",
  };

  flags &= ~EVAL_INCHILD;
  in->runflags = flags;
  in->get      = ( flags & RUN_ECHOINPUT ) ? getverbose : get;
  in->prev     = input;
  input        = in;

  ExceptionHandler

    dispatch = varlookup( dispatcher[ ( ( flags & RUN_PRINTCMDS ) ? 1 : 0 ) +
                                      ( ( flags & RUN_NOEXEC ) ? 2 : 0 ) ],
                          NULL );
    if ( flags & EVAL_EXITONFALSE ) {
      static char exit_on_false[] = "%exit-on-false";
      dispatch = mklist( mkstr( exit_on_false ), dispatch );
    }
    static char fn_dispatch[] = "fn-%dispatch";
    varpush( &push, fn_dispatch, dispatch );

    repl   = varlookup( ( flags & RUN_INTERACTIVE ) ? "fn-%interactive-loop"
                                                    : "fn-%batch-loop", NULL );
    result = ( repl == NULL ) ? prim( "batchloop", NULL, NULL, flags )
                              : eval( repl, NULL, flags );
    varpop( &push );

  CatchException( e )

    ( *input->cleanup )( input );
    input = input->prev;
    throw_exception( e );

  EndExceptionHandler

  input = in->prev;
  ( *in->cleanup )( in );
  return result;
}

/*
 * pushing new input sources
 */

/* fdcleanup -- cleanup after running from a file descriptor */
static void
fdcleanup( Input *in )
{
  unregisterfd( &in->fd );
  if ( in->fd != -1 )
    close( in->fd );
  efree( in->bufbegin );
}

/* runfd -- run commands from a file descriptor */
extern "C" List *
runfd( int fd, const char *name, int flags )
{
  Input in;
  List *result;

  memzero( &in, sizeof( Input ) );
  in.lineno  = 1;
  in.fill    = fdfill;
  in.cleanup = fdcleanup;
  in.fd      = fd;
  registerfd( &in.fd, TRUE );
  in.buflen   = BUFSIZE;
  in.bufbegin = in.buf = (unsigned char *) ealloc( in.buflen );
  in.bufend   = in.bufbegin;
  in.name     = ( name == NULL ) ? str( "fd %d", fd ) : name;

  RefAdd( in.name );
  result = runinput( &in, flags );
  RefRemove( in.name );

  return result;
}

/* stringcleanup -- cleanup after running from a string */
static void
stringcleanup( Input *in )
{
  efree( in->bufbegin );
}

/* stringfill -- placeholder than turns into EOF right away */
static int
stringfill( Input *in )
{
  in->fill = eoffill;
  return EOF;
}

/* runstring -- run commands from a string */
extern "C" List *
runstring( const char *str, const char *name, int flags )
{
  Input          in;
  List *         result;
  unsigned char *buf;

  assert( str != NULL );

  memzero( &in, sizeof( Input ) );
  in.fd     = -1;
  in.lineno = 1;
  in.name   = ( name == NULL ) ? str : name;
  in.fill   = stringfill;
  in.buflen = strlen( str );
  buf       = (unsigned char *) ealloc( in.buflen + 1 );
  memcpy( buf, str, in.buflen );
  in.bufbegin = in.buf = buf;
  in.bufend   = in.buf + in.buflen;
  in.cleanup  = stringcleanup;

  RefAdd( in.name );
  result = runinput( &in, flags );
  RefRemove( in.name );
  return result;
}

/* parseinput -- turn an input source into a tree */
extern "C" Tree *
parseinput( Input *in )
{
  Tree *volatile result = NULL;

  in->prev     = input;
  in->runflags = 0;
  in->get      = get;
  input        = in;

  ExceptionHandler
    result = parse( NULL );
    if ( get( in ) != EOF )
      fail( "$&parse", "more than one value in term" );
    CatchException( e ) ( *input->cleanup )( input );
    input = input->prev;
    throw_exception( e );
  EndExceptionHandler

  input = in->prev;
  ( *in->cleanup )( in );
  return result;
}

/* parsestring -- turn a string into a tree; must be exactly one tree */
extern "C" Tree *
parsestring( const char *str )
{
  Input          in;
  Tree *         result;
  unsigned char *buf;

  assert( str != NULL );

  /* TODO: abstract out common code with runstring */

  memzero( &in, sizeof( Input ) );
  in.fd     = -1;
  in.lineno = 1;
  in.name   = str;
  in.fill   = stringfill;
  in.buflen = strlen( str );
  buf       = (unsigned char *) ealloc( in.buflen + 1 );
  memcpy( buf, str, in.buflen );
  in.bufbegin = in.buf = buf;
  in.bufend   = in.buf + in.buflen;
  in.cleanup  = stringcleanup;

  RefAdd( in.name );
  result = parseinput( &in );
  RefRemove( in.name );
  return result;
}

/* isinteractive -- is the innermost input source interactive? */
extern "C" Boolean
isinteractive( void )
{
  if ( input == NULL )
    return FALSE;
  if ( ( input->runflags & RUN_INTERACTIVE ) != 0 )
    return TRUE;
  return FALSE;
}

/*
 * initialization
 */

/* initinput -- called at dawn of time from main() */
extern "C" void
initinput( void )
{
  int i;
  input = NULL;

  /* declare the global roots */
  globalroot( &history_filename ); /* history file */
  globalroot( &error );            /* parse errors */
  for ( i = 0; i < MAX_PROMPT_COUNT; i++ )
    globalroot( &prompt[ i ] ); /* prompts */

  /* call the parser's initialization */
  initparse();
}
