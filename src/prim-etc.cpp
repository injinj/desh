/* prim-etc.c -- miscellaneous primitives ($Revision: 1.2 $) */

#define	REQUIRE_PWD	1

#include <es/es.h>
#include <es/prim.h>

#ifdef HAVE_LIBREADLINE
#define PARSE_AND_BIND( S ) rl_parse_and_bind( S )
#else
#define PARSE_AND_BIND( S ) 0
#endif

/* bind keyboard to a function name */
static List *
prim_keybind( List *list, Binding *binding, int evalflags )
{
  return mklist(
           mkstr(
             str( "%d",
               PARSE_AND_BIND(
                 str( "%L", list, " " ) ) ) ), NULL );
}

static List *
prim_result( List *list, Binding *binding, int evalflags )
{
  return list;
}

static List *
prim_echo( List *list, Binding *binding, int evalflags )
{
  const char *eol = "\n";
  if ( list != NULL ) {
    if ( termeq( list->term, "-n" ) ) {
      eol  = "";
      list = list->next;
    }
    else if ( termeq( list->term, "--" ) )
      list = list->next;
  }
  print( "%L%s", list, " ", eol );
  return ltrue;
}
/* count items in list */
static List *
prim_count( List *list, Binding *binding, int evalflags )
{
  return mklist( mkstr( str( "%d", length( list ) ) ), NULL );
}

static List *
prim_setnoexport( List *list, Binding *binding, int evalflags )
{
  XRef<List *> lp( list );
  setnoexport( lp.ptr );
  return lp.ptr;
}

static List *
prim_version( List *list, Binding *binding, int evalflags )
{
  return mklist( mkstr( (char *) version ), NULL );
}

static List *
prim_exec( List *list, Binding *binding, int evalflags )
{
  return eval( list, NULL, evalflags | eval_inchild );
}

static List *
prim_dot( List *list, Binding *binding, int evalflags )
{
  int               c, fd;
  volatile int      runflags = ( evalflags & eval_inchild );
  const char *const usage    = ". [-einvx] file [arg ...]";

  esoptbegin( list, "$&dot", usage );
  while ( ( c = esopt( "einvx" ) ) != EOF )
    switch ( c ) {
      case 'e': runflags |= eval_exitonfalse; break;
      case 'i': runflags |= run_interactive; break;
      case 'n': runflags |= run_noexec; break;
      case 'v': runflags |= run_echoinput; break;
      case 'x': runflags |= run_printcmds; break;
    }

  XRef<List *> result( NULL );
  XRef<List *> lp( esoptend() );
  if ( lp.ptr == NULL )
    fail( "$&dot", "usage: %s", usage );

  XRef<char *> file( getstr( lp.ptr->term ) );
  lp.ptr = lp.ptr->next;
  fd = eopen( file.ptr, oOpen );
  if ( fd == -1 )
    fail( "$&dot", "%s: %s", file.ptr, esstrerror( errno ) );

  static char star_str[] = "*";
  static char zero_str[] = "0";
  XPush star( star_str, lp.ptr ),
        zero( zero_str, mklist( mkstr( file.ptr ), NULL ) );

  result.ptr = runfd( fd, file.ptr, runflags );

  return result.ptr;
}

static List *
prim_flatten( List *list, Binding *binding, int evalflags )
{
  char *sep;
  if ( list == NULL )
    fail( "$&flatten", "usage: %%flatten separator [args ...]" );
  XRef<List *> lp( list );
  sep = getstr( lp.ptr->term );
  return mklist( mkstr( str( "%L", lp.ptr->next, sep ) ), NULL );
}

static List *
prim_whatis( List *list, Binding *binding, int evalflags )
{
  /* the logic in here is duplicated in eval() */
  if ( list == NULL || list->next != NULL )
    fail( "$&whatis", "usage: $&whatis program" );
  XRef<Term *> term( list->term );
  if ( getclosure( term.ptr ) == NULL ) {
    XRef<char *> prog( getstr( term.ptr ) );
    List *fn;
    assert( prog.ptr != NULL );
    fn = varlookup2( "fn-", prog.ptr, binding );
    if ( fn != NULL )
      list = fn;
    else {
      if ( isabsolute( prog.ptr ) ) {
        char *error = checkexecutable( prog.ptr );
        if ( error != NULL )
          fail( "$&whatis", "%s: %s", prog.ptr, error );
      }
      else
        list = pathsearch( term.ptr );
    }
  }
  return list;
}

static List *
prim_split( List *list, Binding *binding, int evalflags )
{
  char *sep;
  if ( list == NULL )
    fail( "$&split", "usage: %%split separator [args ...]" );
  XRef<List *> lp( list );
  sep = getstr( lp.ptr->term );
  return fsplit( sep, lp.ptr->next, TRUE );
}

static List *
prim_fsplit( List *list, Binding *binding, int evalflags )
{
  char *sep;
  if ( list == NULL )
    fail( "$&fsplit", "usage: %%fsplit separator [args ...]" );
  XRef<List *> lp( list );
  sep = getstr( lp.ptr->term );
  return fsplit( sep, lp.ptr->next, FALSE );
}

static List *
prim_var( List *list, Binding *binding, int evalflags )
{
  if ( list == NULL )
    return NULL;
  XRef<List *> rest( list->next );
  XRef<char *> name( getstr( list->term ) );
  XRef<List *> defn( varlookup( name.ptr, NULL ) );
  rest.ptr = prim_var( rest.ptr, NULL, evalflags );
  Term *term = mkstr( str( "%S = %#L", name.ptr, defn.ptr, " " ) );
  return mklist( term, rest.ptr );
}

static List *
prim_sethistory( List *list, Binding *binding, int evalflags )
{
  if ( list == NULL ) {
    sethistory( NULL );
    return NULL;
  }
  XRef<List *> lp( list );
  sethistory( getstr( lp.ptr->term ) );
  return lp.ptr;
}

static List *
prim_parse( List *list, Binding *binding, int evalflags )
{
  Tree *tree;
  int   i;
  XRefArray<char *, MAX_PROMPT_COUNT> pr( NULL );
  {
    XRef<List *> lp( list );
    for ( i = 0; lp.ptr != NULL && i < MAX_PROMPT_COUNT; i++ ) {
      pr.ptr[ i ] = getstr( lp.ptr->term );
      lp.ptr = lp.ptr->next;
    }
  }
  if ( (tree = parse( pr.ptr )) == NULL )
    return NULL;
  return mklist( mkterm( NULL, mkclosure( mk( nThunk, tree ), NULL ) ), NULL );
}

static List *
prim_exitonfalse( List *list, Binding *binding, int evalflags )
{
  return eval( list, NULL, evalflags | eval_exitonfalse );
}

static List *
prim_batchloop( List *list, Binding *binding, int evalflags )
{
  XRef<List *> result( ltrue );
  XRef<List *> dispatch( NULL );

  SIGCHK();

  ExceptionHandler

  for (;;) {
    List *parser, *cmd;
    parser = varlookup( "fn-%parse", NULL );
    cmd    = ( parser == NULL ) ? prim( "parse", NULL, NULL, 0 )
                                : eval( parser, NULL, 0 );
    SIGCHK();
    dispatch.ptr = varlookup( "fn-%dispatch", NULL );
    if ( cmd != NULL ) {
      if ( dispatch.ptr != NULL )
        cmd = append( dispatch.ptr, cmd );
      result.ptr = eval( cmd, NULL, evalflags );
      SIGCHK();
    }
  }

  CatchException( e )

  if ( !termeq( e->term, "eof" ) )
    throw_exception( e );

  EndExceptionHandler

  return result.ptr;
}

static List *
prim_collect( List *list, Binding *binding, int evalflags )
{
  gc();
  return ltrue;
}

static List *
prim_home( List *list, Binding *binding, int evalflags )
{
  struct passwd *pw;
  if ( list == NULL )
    return varlookup( "home", NULL );
  if ( list->next != NULL )
    fail( "$&home", "usage: %%home [user]" );
  pw = getpwnam( getstr( list->term ) );
  return ( pw == NULL ) ? NULL : mklist( mkstr( gcdup( pw->pw_dir ) ), NULL );
}

static List *
prim_vars( List *list, Binding *binding, int evalflags )
{
  return listvars( FALSE );
}

static List *
prim_internals( List *list, Binding *binding, int evalflags )
{
  return listvars( TRUE );
}

static List *
prim_isinteractive( List *list, Binding *binding, int evalflags )
{
  return isinteractive() ? ltrue : lfalse;
}

static List *
prim_noreturn( List *list, Binding *binding, int evalflags )
{
  if ( list == NULL )
    fail( "$&noreturn", "usage: $&noreturn lambda args ..." );
  XRef<List *> lp( list );
  XRef<Closure *> closure( getclosure( lp.ptr->term ) );
  if ( closure.ptr == NULL || closure.ptr->tree->kind != nLambda )
    fail( "$&noreturn", "$&noreturn: %E is not a lambda", lp.ptr->term );
  XRef<Tree *> tree( closure.ptr->tree );
  XRef<Binding *> context(
       bindargs( tree.ptr->u[ 0 ].p, lp.ptr->next, closure.ptr->binding ) );
  return walk( tree.ptr->u[ 1 ].p, context.ptr, evalflags );
}

static List *
prim_setmaxevaldepth( List *list, Binding *binding, int evalflags )
{
  char *s;
  long  n;
  if ( list == NULL ) {
    maxevaldepth = MAXmaxevaldepth;
    return NULL;
  }
  if ( list->next != NULL )
    fail( "$&setmaxevaldepth", "usage: $&setmaxevaldepth [limit]" );
  XRef<List *> lp( list );
  n = strtol( getstr( lp.ptr->term ), &s, 0 );
  if ( n < 0 || ( s != NULL && *s != '\0' ) )
    fail( "$&setmaxevaldepth",
          "max-eval-depth must be set to a positive integer" );
  if ( n < MINmaxevaldepth )
    n = ( n == 0 ) ? MAXmaxevaldepth : MINmaxevaldepth;
  maxevaldepth = n;
  return lp.ptr;
}

static List *
prim_resetterminal( List *list, Binding *binding, int evalflags )
{
  reset_terminal = TRUE;
  return ltrue;
}

/*
 * initialization
 */

extern "C" Dict *
initprims_etc( Dict *primdict )
{
  static struct {
    char   name[ 16 ];
    List* (*func)(List*, Binding*, int);
  } d[] = {
    { "keybind",         prim_keybind         },
    { "echo",            prim_echo            },
    { "count",           prim_count           },
    { "version",         prim_version         },
    { "exec",            prim_exec            },
    { "dot",             prim_dot             },
    { "flatten",         prim_flatten         },
    { "whatis",          prim_whatis          },
    { "sethistory",      prim_sethistory      },
    { "split",           prim_split           },
    { "fsplit",          prim_fsplit          },
    { "var",             prim_var             },
    { "parse",           prim_parse           },
    { "batchloop",       prim_batchloop       },
    { "collect",         prim_collect         },
    { "home",            prim_home            },
    { "setnoexport",     prim_setnoexport     },
    { "vars",            prim_vars            },
    { "internals",       prim_internals       },
    { "result",          prim_result          },
    { "isinteractive",   prim_isinteractive   },
    { "exitonfalse",     prim_exitonfalse     },
    { "noreturn",        prim_noreturn        },
    { "setmaxevaldepth", prim_setmaxevaldepth },
    { "resetterminal",   prim_resetterminal   }
  };
  for ( size_t i = 0; i < sizeof( d ) / sizeof( d[ 0 ] ); i++ )
    primdict = dictput( primdict, d[ i ].name, (void *) d[ i ].func );

  return primdict;
}
