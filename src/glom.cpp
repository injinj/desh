/* glom.c -- walk parse tree to produce list ($Revision: 1.1.1.1 $) */

#include <desh/es.h>
#include <desh/gc.h>

/* concat -- cartesion cross product concatenation */
extern "C" List *
concat( List *list1, List *list2 )
{
  List **p, *result = NULL;

  gcdisable();
  for ( p = &result; list1 != NULL; list1 = list1->next ) {
    List *lp;
    for ( lp = list2; lp != NULL; lp = lp->next ) {
      List *np = mklist( termcat( list1->term, lp->term ), NULL );
      *p       = np;
      p        = &np->next;
    }
  }

  XRef<List *> list( result );
  gcenable();
  return list.ptr;
}

/* qcat -- concatenate two quote flag terms */
static char *
qcat( const char *q1, const char *q2, Term *t1, Term *t2 )
{
  size_t len1, len2;
  char * result, *s;

  assert( gcisblocked() );

  if ( q1 == QUOTED && q2 == QUOTED )
    return QUOTED;
  if ( q1 == UNQUOTED && q2 == UNQUOTED )
    return UNQUOTED;

  len1 =
    ( q1 == QUOTED || q1 == UNQUOTED ) ? strlen( getstr( t1 ) ) : strlen( q1 );
  len2 =
    ( q2 == QUOTED || q2 == UNQUOTED ) ? strlen( getstr( t2 ) ) : strlen( q2 );
  result = s = (char *) gcalloc( len1 + len2 + 1, &StringTag );

  if ( q1 == QUOTED )
    memset( s, 'q', len1 );
  else if ( q1 == UNQUOTED )
    memset( s, 'r', len1 );
  else
    memcpy( s, q1, len1 );
  s += len1;
  if ( q2 == QUOTED )
    memset( s, 'q', len2 );
  else if ( q2 == UNQUOTED )
    memset( s, 'r', len2 );
  else
    memcpy( s, q2, len2 );
  s += len2;
  *s = '\0';

  return result;
}

/* qconcat -- cartesion cross product concatenation; also produces a quote list
 */
static List *
qconcat( List *list1, List *list2, StrList *ql1, StrList *ql2,
         StrList **quotep )
{
  List **   p, *result = NULL;
  StrList **qp;

  gcdisable();
  for ( p = &result, qp = quotep; list1 != NULL;
        list1 = list1->next, ql1 = ql1->next ) {
    List *   lp;
    StrList *qlp;
    for ( lp = list2, qlp = ql2; lp != NULL; lp = lp->next, qlp = qlp->next ) {
      List *   np;
      StrList *nq;
      np = mklist( termcat( list1->term, lp->term ), NULL );
      *p = np;
      p  = &np->next;
      nq = mkstrlist( qcat( ql1->str, qlp->str, list1->term, lp->term ), NULL );
      *qp = nq;
      qp  = &nq->next;
    }
  }

  XRef<List *> list( result );
  gcenable();
  return list.ptr;
}

/* subscript -- variable subscripting */
static List *
subscript( List *list, List *subs )
{
  int   lo, hi, len, counter;
  List *result, **prevp, *current;

  gcdisable();

  result  = NULL;
  prevp   = &result;
  len     = length( list );
  current = list;
  counter = 1;

  if ( subs != NULL && streq( getstr( subs->term ), "..." ) ) {
    lo = 1;
    goto mid_range;
  }

  while ( subs != NULL ) {
    lo = atoi( getstr( subs->term ) );
    if ( lo < 1 ) {
      XRef<char *> bad( getstr( subs->term ) );
      gcenable();
      fail( "es:subscript", "bad subscript: %s", bad.ptr );
    }
    subs = subs->next;
    if ( subs != NULL && streq( getstr( subs->term ), "..." ) ) {
    mid_range:
      subs = subs->next;
      if ( subs == NULL )
        hi = len;
      else {
        hi = atoi( getstr( subs->term ) );
        if ( hi < 1 ) {
          XRef<char *> bad( getstr( subs->term ) );
          gcenable();
          fail( "es:subscript", "bad subscript: %s", bad.ptr );
        }
        if ( hi > len )
          hi = len;
        subs = subs->next;
      }
    }
    else
      hi = lo;
    if ( lo > len )
      continue;
    if ( counter > lo ) {
      current = list;
      counter = 1;
    }
    for ( ; counter < lo; counter++, current = current->next )
      ;
    for ( ; counter <= hi; counter++, current = current->next ) {
      *prevp = mklist( current->term, NULL );
      prevp  = &( *prevp )->next;
    }
  }

  XRef<List *> r( result );
  gcenable();
  return r.ptr;
}

/* glom1 -- glom when we don't need to produce a quote list */
static List *
glom1( Tree *tree, Binding *binding )
{
  XRef<List *>    result( NULL );
  XRef<List *>    tail( NULL );
  XRef<Tree *>    tp( tree );
  XRef<Binding *> bp( binding );

  assert( !gcisblocked() );

  while ( tp.ptr != NULL ) {
    XRef<List *> list( NULL );

    switch ( tp.ptr->kind ) {
      case nQword:
        list.ptr = mklist( mkterm( tp.ptr->u[ 0 ].s, NULL ), NULL );
        tp.ptr   = NULL;
        break;
      case nWord:
        list.ptr = mklist( mkterm( tp.ptr->u[ 0 ].s, NULL ), NULL );
        tp.ptr   = NULL;
        break;
      case nThunk:
      case nLambda:
        list.ptr = mklist( mkterm( NULL, mkclosure( tp.ptr, bp.ptr ) ), NULL );
        tp.ptr   = NULL;
        break;
      case nPrim:
        list.ptr = mklist( mkterm( NULL, mkclosure( tp.ptr, NULL ) ), NULL );
        tp.ptr   = NULL;
        break;
      case nVar: {
        XRef<List *> var( glom1( tp.ptr->u[ 0 ].p, bp.ptr ) );
        tp.ptr = NULL;
        for ( ; var.ptr != NULL; var.ptr = var.ptr->next ) {
          list.ptr = listcopy( varlookup( getstr( var.ptr->term ), bp.ptr ) );
          if ( list.ptr != NULL ) {
            if ( result.ptr == NULL )
              tail.ptr = result.ptr = list.ptr;
            else
              tail.ptr->next = list.ptr;
            for ( ; tail.ptr->next != NULL; tail.ptr = tail.ptr->next )
              ;
          }
          list.ptr = NULL;
        }
        break;
      }
      case nVarsub: {
        list.ptr = glom1( tp.ptr->u[ 0 ].p, bp.ptr );
        if ( list.ptr == NULL )
          fail( SHNAME ":glom", "null variable name in subscript" );
        if ( list.ptr->next != NULL )
          fail( SHNAME ":glom", "multi-word variable name in subscript" );
        XRef<char *> name( getstr( list.ptr->term ) );
        list.ptr = varlookup( name.ptr, bp.ptr );
        XRef<List *> sub( glom1( tp.ptr->u[ 1 ].p, bp.ptr ) );
        tp.ptr   = NULL;
        list.ptr = subscript( list.ptr, sub.ptr );
        break;
      }
      case nCall:
        list.ptr = listcopy( walk( tp.ptr->u[ 0 ].p, bp.ptr, 0 ) );
        tp.ptr   = NULL;
        break;
      case nList:
        list.ptr = glom1( tp.ptr->u[ 0 ].p, bp.ptr );
        tp.ptr   = tp.ptr->u[ 1 ].p;
        break;
      case nConcat: {
        XRef<List *> l( glom1( tp.ptr->u[ 0 ].p, bp.ptr ) );
        XRef<List *> r( glom1( tp.ptr->u[ 1 ].p, bp.ptr ) );
        tp.ptr   = NULL;
        list.ptr = concat( l.ptr, r.ptr );
        break;
      }
      default:
        fail( SHNAME ":glom", "glom1: bad node kind %d", tp.ptr->kind );
        break;
    }

    if ( list.ptr != NULL ) {
      if ( result.ptr == NULL )
        tail.ptr = result.ptr = list.ptr;
      else
        tail.ptr->next = list.ptr;
      for ( ; tail.ptr->next != NULL; tail.ptr = tail.ptr->next )
        ;
    }
  }

  return result.ptr;
}

/* glom2 -- glom and produce a quoting list */
extern "C" List *
glom2( Tree *tree, Binding *binding, StrList **quotep )
{
  XRef<List *>    result( NULL );
  XRef<List *>    tail( NULL );
  XRef<StrList *> qtail( NULL );
  XRef<Tree *>    tp( tree );
  XRef<Binding *> bp( binding );

  assert( !gcisblocked() );
  assert( quotep != NULL );

  /*
   * this loop covers only the cases where we might produce some
   * unquoted (raw) values.  all other cases are handled in glom1
   * and we just add quoted word flags to them.
   */

  while ( tp.ptr != NULL ) {
    XRef<List *>    list( NULL );
    XRef<StrList *> qlist( NULL );

    switch ( tp.ptr->kind ) {
      case nWord:
        list.ptr  = mklist( mkterm( tp.ptr->u[ 0 ].s, NULL ), NULL );
        qlist.ptr = mkstrlist( UNQUOTED, NULL );
        tp.ptr    = NULL;
        break;
      case nList:
        list.ptr = glom2( tp.ptr->u[ 0 ].p, bp.ptr, &qlist.ptr );
        tp.ptr   = tp.ptr->u[ 1 ].p;
        break;
      case nConcat: {
        XRef<List *>    l( NULL );
        XRef<List *>    r( NULL );
        XRef<StrList *> ql( NULL );
        XRef<StrList *> qr( NULL );
        l.ptr    = glom2( tp.ptr->u[ 0 ].p, bp.ptr, &ql.ptr );
        r.ptr    = glom2( tp.ptr->u[ 1 ].p, bp.ptr, &qr.ptr );
        list.ptr = qconcat( l.ptr, r.ptr, ql.ptr, qr.ptr, &qlist.ptr );
        tp.ptr   = NULL;
        break;
      }
      default: {
        list.ptr = glom1( tp.ptr, bp.ptr );
        XRef<List *> lp( list.ptr );
        for ( ; lp.ptr != NULL; lp.ptr = lp.ptr->next )
          qlist.ptr = mkstrlist( QUOTED, qlist.ptr );
        tp.ptr = NULL;
        break;
      }
    }

    if ( list.ptr != NULL ) {
      if ( result.ptr == NULL ) {
        assert( *quotep == NULL );
        result.ptr = tail.ptr  = list.ptr;
        *quotep    = qtail.ptr = qlist.ptr;
      }
      else {
        assert( *quotep != NULL );
        tail.ptr->next  = list.ptr;
        qtail.ptr->next = qlist.ptr;
      }
      while ( tail.ptr->next != NULL ) {
        tail.ptr  = tail.ptr->next;
        qtail.ptr = qtail.ptr->next;
      }
      assert( qtail.ptr->next == NULL );
    }
  }

  return result.ptr;
}

/* glom -- top level glom dispatching */
extern "C" List *
glom( Tree *tree, Binding *binding, Boolean globit )
{
  if ( globit ) {
    XRef<List *>    list( NULL );
    XRef<StrList *> quote( NULL );
    list.ptr = glom2( tree, binding, &quote.ptr );
    list.ptr = glob( list.ptr, quote.ptr );
    return list.ptr;
  }
  return glom1( tree, binding );
}
