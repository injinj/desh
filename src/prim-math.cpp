/* prim-rel.cxx -- relational primitives */

#include <es/es.h>
#include <es/prim.h>
#include <es/decimal.h>

using namespace es;

enum MathOp {
  DO_SUM    = 0,
  DO_DIFF   = 1,
  DO_MUL    = 2,
  DO_DIV    = 3,
  DO_MOD    = 4,
  DO_POW    = 5,
  DO_NEG    = 6,
  DO_LOG    = 7,
  DO_LOG10  = 8,
  DO_FLOOR  = 9,
  DO_ROUND  = 10,
  DO_ISINF  = 11,
  DO_ISNAN  = 12
};

static List *
do_math( List *list, Binding *binding, int evalflags, MathOp op )
{
  Dec128 r;
  XRef<List *> lp( list );
  r.zero();
  for ( int cnt = 0; ; cnt++ ) {
    if ( lp.ptr == NULL )
      break;
    XRef<char *> a( getstr( lp.ptr->term ) );
    Dec128 n;
    n.from_string( a.ptr );
    if ( cnt == 0 && op != DO_NEG ) {
      switch ( op ) {
        case DO_LOG:   r = n.ln();        break;
        case DO_LOG10: r = n.log10();     break;
        case DO_FLOOR: r = n.floor();     break;
        case DO_ROUND: r = n.round();     break;
        case DO_ISINF: r = n.isinf()?0:1; break;
        case DO_ISNAN: r = n.isnan()?0:1; break;
        default:       r = n;             break;
      }
    }
    else {
      switch ( op ) {
        case DO_SUM:    r += n;     break;
        case DO_DIFF:   r -= n;     break;
        case DO_MUL:    r *= n;     break;
        case DO_DIV:    r /= n;     break;
        case DO_MOD:    r %= n;     break;
        case DO_POW:    r.pow( n ); break;
        case DO_NEG:    r -= n;     break;
        default:                    break;
      }
    }
    lp.ptr = lp.ptr->next;
  }
  char tmp[ 64 ];
  size_t n = r.to_string( tmp );
  return mklist( mkstr( gcndup( tmp, n ) ), NULL );
}

static List *
prim_sum( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_SUM );
}

static List *
prim_diff( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_DIFF );
}

static List *
prim_mul( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_MUL );
}

static List *
prim_div( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_DIV );
}

static List *
prim_mod( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_MOD );
}

static List *
prim_pow( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_POW );
}

static List *
prim_neg( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_NEG );
}

static List *
prim_log( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_LOG );
}

static List *
prim_log10( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_LOG10 );
}

static List *
prim_floor( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_FLOOR );
}

static List *
prim_round( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_ROUND );
}

static List *
prim_isinf( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_ISINF );
}

static List *
prim_isnan( List *list, Binding *binding, int evalflags )
{
  return do_math( list, binding, evalflags, DO_ISNAN );
}

extern "C" Dict *
initprims_math( Dict *primdict ) {
  static struct {
    char   name[ 8 ];
    List* (*func)(List*, Binding*, int);
  } d[] = {
    { "sum",   prim_sum   },
    { "diff",  prim_diff  },
    { "mul",   prim_mul   },
    { "div",   prim_div   },
    { "mod",   prim_mod   },
    { "pow",   prim_pow   },
    { "neg",   prim_neg   },
    { "log",   prim_log   },
    { "log10", prim_log10 },
    { "floor", prim_floor },
    { "round", prim_round },
    { "isinf", prim_isinf },
    { "isnan", prim_isnan }
  };
  for ( size_t i = 0; i < sizeof( d ) / sizeof( d[ 0 ] ); i++ )
    primdict = dictput( primdict, d[ i ].name, (void *) d[ i ].func );

  return primdict;
}
