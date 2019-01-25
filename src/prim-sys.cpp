/* prim-sys.c -- system call primitives ($Revision: 1.2 $) */

#define REQUIRE_IOCTL 1
#define REQUIRE_STAT 1

#include <desh/es.h>
#include <desh/prim.h>
#include <desh/term.h>

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <termios.h>

static List *
prim_newpgrp( List *list, Binding *binding, int evalflags )
{
  int pid;
  if ( list != NULL )
    fail( "$&newpgrp", "usage: newpgrp" );
  pid = getpid();
  setpgrp( /*pid, pid*/ ); /* could be setsid() */
/*#ifdef TIOCSPGRP*/
  {
    Sigeffect sigtstp = esignal( SIGTSTP, sig_ignore );
    Sigeffect sigttin = esignal( SIGTTIN, sig_ignore );
    Sigeffect sigttou = esignal( SIGTTOU, sig_ignore );
    ioctl( 2, TIOCSPGRP, &pid );
    esignal( SIGTSTP, sigtstp );
    esignal( SIGTTIN, sigttin );
    esignal( SIGTTOU, sigttou );
  }
/*#endif*/
  return ltrue;
}

static List *
prim_background( List *list, Binding *binding, int evalflags )
{
  int pid = efork( TRUE, TRUE, 0, list );
  setpgid( pid, getpid() );
  if ( pid == 0 ) {
    exit( exitstatus( eval( list, NULL, evalflags | EVAL_INCHILD ) ) );
  }
  return mklist( mkstr( str( "%d", pid ) ), NULL );
}

static List *
prim_fork( List *list, Binding *binding, int evalflags )
{
  int pid, status;
  pid = efork( TRUE, FALSE, 0, list );
  if ( pid == 0 )
    exit( exitstatus( eval( list, NULL, evalflags | EVAL_INCHILD ) ) );
  status = ewaitfor( pid );
  SIGCHK();
  printstatus( 0, status );
  return mklist( mkstr( mkstatus( status ) ), NULL );
}

static List *
prim_run( List *list, Binding *binding, int evalflags )
{
  char *file;
  if ( list == NULL )
    fail( "$&run", "usage: %%run file argv0 argv1 ..." );
  XRef<List *> lp( list );
  file = getstr( lp.ptr->term );
  lp.ptr = forkexec( file, lp.ptr->next,
                     ( evalflags & EVAL_INCHILD ) != 0 ? TRUE : FALSE );
  return lp.ptr;
}

static List *
prim_umask( List *list, Binding *binding, int evalflags )
{
  if ( list == NULL ) {
    mode_t mask = umask( 0 );
    umask( mask );
    print( "%04o\n", mask );
    return ltrue;
  }
  if ( list->next == NULL ) {
    mode_t mask;
    char *s, *t;
    s    = getstr( list->term );
    mask = strtol( s, &t, 8 );
    if ( ( t != NULL && *t != '\0' ) || ( (unsigned) mask ) > 07777 )
      fail( "$&umask", "bad umask: %s", s );
    umask( mask );
    return ltrue;
  }
  fail( "$&umask", "usage: umask [mask]" );
  NOTREACHED;
}

static List *
prim_cd( List *list, Binding *binding, int evalflags )
{
  char *dir;
  if ( list == NULL || list->next != NULL )
    fail( "$&cd", "usage: $&cd directory" );
  dir = getstr( list->term );
  if ( chdir( dir ) == -1 )
    fail( "$&cd", "chdir %s: %s", dir, esstrerror( errno ) );
  return ltrue;
}

static List *
prim_setsignals( List *list, Binding *binding, int evalflags )
{
  int       i;
  Sigeffect effects[ NSIG ];
  for ( i = 0; i < NSIG; i++ )
    effects[ i ] = sig_default;
  XRef<List *> lp( list );
  for ( ; lp.ptr != NULL; lp.ptr = lp.ptr->next ) {
    int         sig;
    const char *s      = getstr( lp.ptr->term );
    Sigeffect   effect = sig_catch;
    switch ( *s ) {
      case '-':
        effect = sig_ignore;
        s++;
        break;
      case '/':
        effect = sig_noop;
        s++;
        break;
      case '.':
        effect = sig_special;
        s++;
        break;
    }
    sig = signumber( s );
    if ( sig < 0 )
      fail( "$&setsignals", "unknown signal: %s", s );
    effects[ sig ] = effect;
  }
  blocksignals();
  setsigeffects( effects );
  unblocksignals();
  return mksiglist();
}

/*
 * limit builtin -- this is too much code for what it gives you
 */

typedef struct Suffix Suffix;
struct Suffix {
  const char *  name;
  long          amount;
  const Suffix *next;
};

static const Suffix sizesuf[] = {
  { "g", 1024 * 1024 * 1024, sizesuf + 1 },
  { "m", 1024 * 1024, sizesuf + 2 },
  { "k", 1024, NULL },
};

static const Suffix timesuf[] = {
  { "h", 60 * 60, timesuf + 1 },
  { "m", 60, timesuf + 2 },
  { "s", 1, NULL },
};

typedef struct {
  char          name[ 16 ];
  int           flag;
  const Suffix *suffix;
} Limit;

static const Limit limits[] = {

  { "cputime", RLIMIT_CPU, timesuf },
  { "filesize", RLIMIT_FSIZE, sizesuf },
  { "datasize", RLIMIT_DATA, sizesuf },
  { "stacksize", RLIMIT_STACK, sizesuf },
  { "coredumpsize", RLIMIT_CORE, sizesuf },
#ifdef RLIMIT_RSS /* SysVr4 does not have this */
  { "memoryuse", RLIMIT_RSS, sizesuf },
#endif

#if defined( RLIMIT_VMEM ) /* instead, they have this! */
  { "memorysize", RLIMIT_VMEM, sizesuf },
#elif defined( RLIMIT_AS ) /* from xs (same as VMEM on solaris) */
  { "virtualsize", RLIMIT_AS, sizesuf },
#endif

#ifdef RLIMIT_MEMLOCK /* 4.4bsd adds an unimpl. limit on non-pageable mem */
  { "lockedmemory", RLIMIT_CORE, sizesuf },
#endif

#ifdef RLIMIT_NOFILE /* SunOS 4.1 adds a limit on file descriptors */
  { "descriptors", RLIMIT_NOFILE, NULL },
#elif defined( RLIMIT_OFILE ) /* but 4.4bsd uses this name for it */
  { "descriptors", RLIMIT_OFILE, NULL },
#endif

#ifdef RLIMIT_NPROC /* 4.4bsd adds a limit on child processes */
  { "processes", RLIMIT_NPROC, NULL },
#endif
#ifdef RLIMIT_MSGQUEUE
  { "msgqueuesize", RLIMIT_MSGQUEUE, sizesuf },
#endif
#ifdef RLIMIT_NICE
  { "nicelimit", RLIMIT_NICE, NULL },
#endif
#ifdef RLIMIT_RTPRIO
  { "rtpriolimit", RLIMIT_RTPRIO, NULL },
#endif
#ifdef RLIMIT_RTTIME
  { "rtrunlimit", RLIMIT_RTTIME, NULL },
#endif
#ifdef RLIMIT_SIGPENDING
  { "sigqlimit", RLIMIT_SIGPENDING, NULL },
#endif
  { "", 0, NULL }
};

static void
printlimit( const Limit *limit, Boolean hard )
{
  struct rlimit rlim;
  rlim_t        lim;
  getrlimit( limit->flag, &rlim );
  if ( hard )
    lim = rlim.rlim_max;
  else
    lim = rlim.rlim_cur;
  if ( lim == (rlim_t) RLIM_INFINITY )
    print( "%-8s\tunlimited\n", limit->name );
  else {
    const Suffix *suf;

    for ( suf = limit->suffix; suf != NULL; suf = suf->next )
      if ( lim % suf->amount == 0 && ( lim != 0 || suf->amount > 1 ) ) {
        lim /= suf->amount;
        break;
      }
    print( "%-8s\t%d%s\n", limit->name, lim,
           ( suf == NULL || lim == 0 ) ? "" : suf->name );
  }
}

static rlim_t
parselimit( const Limit *limit, char *s )
{
  rlim_t        lim;
  char *        t;
  const Suffix *suf = limit->suffix;
  if ( streq( s, "unlimited" ) )
    return RLIM_INFINITY;
  if ( !isdigit( *s ) )
    fail( "$&limit", "%s: bad limit value", s );
  if ( suf == timesuf && ( t = strchr( s, ':' ) ) != NULL ) {
    char *u;
    lim = strtol( s, &u, 0 ) * 60;
    if ( u != t )
      fail( "$&limit", "%s %s: bad limit value", limit->name, s );
    lim += strtol( u + 1, &t, 0 );
    if ( t != NULL && *t == ':' )
      lim = lim * 60 + strtol( t + 1, &t, 0 );
    if ( t != NULL && *t != '\0' )
      fail( "$&limit", "%s %s: bad limit value", limit->name, s );
  }
  else {
    lim = strtol( s, &t, 0 );
    if ( t != NULL && *t != '\0' )
      for ( ;; suf = suf->next ) {
        if ( suf == NULL )
          fail( "$&limit", "%s %s: bad limit value", limit->name, s );
        if ( streq( suf->name, t ) ) {
          lim *= suf->amount;
          break;
        }
      }
  }
  return lim;
}

static List *
prim_limit( List *list, Binding *binding, int evalflags )
{
  const Limit *lim  = limits;
  Boolean      hard = FALSE;
  XRef<List *> lp( list );

  if ( lp.ptr != NULL && streq( getstr( lp.ptr->term ), "-h" ) ) {
    hard   = TRUE;
    lp.ptr = lp.ptr->next;
  }

  if ( lp.ptr == NULL )
    for ( ; lim->name[ 0 ] != '\0'; lim++ )
      printlimit( lim, hard );
  else {
    char *name = getstr( lp.ptr->term );
    for ( ;; lim++ ) {
      if ( lim->name == NULL )
        fail( "$&limit", "%s: no such limit", name );
      if ( streq( name, lim->name ) )
        break;
    }
    lp.ptr = lp.ptr->next;
    if ( lp.ptr == NULL )
      printlimit( lim, hard );
    else {
      rlim_t        n;
      struct rlimit rlim;
      getrlimit( lim->flag, &rlim );
      if ( ( n = parselimit( lim, getstr( lp.ptr->term ) ) ) < 0 )
        fail( "$&limit", "%s: bad limit value", getstr( lp.ptr->term ) );
      if ( hard )
        rlim.rlim_max = n;
      else
        rlim.rlim_cur = n;
      if ( setrlimit( lim->flag, &rlim ) == -1 )
        fail( "$&limit", "setrlimit: %s", esstrerror( errno ) );
    }
  }
  return ltrue;
}

extern "C" Dict *
initprims_sys( Dict *primdict )
{
  static struct {
    char   name[ 16 ];
    List* (*func)(List*, Binding*, int);
  } d[] = {
    { "newpgrp",         prim_newpgrp         },
    { "background",      prim_background      },
    { "umask",           prim_umask           },
    { "cd",              prim_cd              },
    { "fork",            prim_fork            },
    { "run",             prim_run             },
    { "setsignals",      prim_setsignals      },
    { "limit",           prim_limit           }
  };
  for ( size_t i = 0; i < sizeof( d ) / sizeof( d[ 0 ] ); i++ )
    primdict = dictput( primdict, d[ i ].name, (void *) d[ i ].func );

  return primdict;
}
