/* prim-rel.cxx -- relational primitives */

#include <desh/es.h>
#include <desh/prim.h>
#include <desh/decimal.h>
#include <string.h>

using namespace es;

static List *
prim_cmp( List *list, Binding *binding, int evalflags )
{
  int r = 0;
  XRef<List *> lp( list );
  if ( lp.ptr != NULL ) {
    XRef<char *> a( getstr( lp.ptr->term ) );
    XRef<List *> rp( lp.ptr->next );
    if ( rp.ptr != NULL ) {
      XRef<char *> b( getstr( rp.ptr->term ) );
      Dec128 n, m;
      n.from_string( a.ptr );
      if ( ! n.isnan() ) {
        m.from_string( b.ptr );
        if ( ! m.isnan() ) {
          if ( n > m )
            r = 1;
          else if ( n < m )
            r = -1;
        }
        else
          goto is_nan;
      }
      else {
      is_nan:;
        r = strcoll( a.ptr, b.ptr );
      }
    }
  }
  static char one[] = "1", zero[] = "0", neg[] = "-1";
  return mklist( mkstr( r > 0 ? one : r < 0 ? neg : zero ), NULL );
}

extern "C" Dict *
initprims_rel( Dict *primdict ) {
  static char cmp_str[] = "cmp";
  primdict = dictput( primdict, cmp_str, (void *) prim_cmp );
  return primdict;
}
