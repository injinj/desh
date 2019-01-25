#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <desh/decimal.h>
#define DECNUMDIGITS 64
#include <libdecnumber/bid/decimal128.h>

#if 0
static void
print( const char *what,  Dec128Store *d )
{
  char buf[ 128 ];
  es_dec128_to_string( d, buf );
  printf( "%s : %s\n", what, buf );
}

int
main( int argc, char *argv[] )
{
  Dec128Store e, d, f, g, h;
  es_dec128_itod( &d, 101 );
  print( "d = int(101)", &d );
  es_dec128_from_string( &e, "101.101" );
  print( "e = str(101.101)", &e );
  printf( "e > d : %d\n", es_dec128_gt( &e, &d ) );
  printf( "e < d : %d\n", es_dec128_lt( &e, &d ) );
  printf( "e == d : %d\n", es_dec128_eq( &e, &d ) );
  es_dec128_sum( &f, &e, &d );
  print( "f = d + e", &f );
  es_dec128_itod( &g, 10 );
  print( "g = int(10)", &g );
  es_dec128_mul( &h, &g, &f );
  print( "h = g * f", &h );
  es_dec128_div( &h, &f, &g );
  print( "h = f / g", &h );
  es_dec128_mod( &h, &f, &g );
  print( "h = f % g", &h );
  es_dec128_itod( &g, 2 );
  print( "g = int(2)", &g );
  es_dec128_pow( &h, &f, &g );
  print( "h = f ^ g", &h );
  printf( "h > f : %d\n", es_dec128_gt( &h, &f ) );
  printf( "h < f : %d\n", es_dec128_lt( &h, &f ) );
  printf( "h == f : %d\n", es_dec128_eq( &h, &f ) );
  printf( "h == h : %d\n", es_dec128_eq( &h, &h ) );
  return 0;
}
#endif

void
es_dec128_itod( Dec128Store *fp,  int i )
{
  /**(_Decimal128 *) fp = i;*/
  decContext ctx128;
  decNumber n;
  decNumberFromInt32( &n, i );
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128FromNumber( (decimal128 *) fp, &n, &ctx128 );
}

void
es_dec128_from_string( Dec128Store *fp,  const char *str )
{
  decContext ctx128;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128FromString( (decimal128 *) fp, str, &ctx128 );
}

void
es_dec128_zero( Dec128Store *fp )
{
  /**(_Decimal128 *) fp = 0;*/
  es_dec128_itod( fp, 0 );
}

size_t
es_dec128_to_string( const Dec128Store *fp,  char *str )
{
  Dec128Store fp2 = *fp;
  /*gcc_decimal128( &fp2 );*/
  /* this adds garbage at the end of NaN (as far as I can tell) */
  decimal128ToString( (const decimal128 *) &fp2, str );
  if ( str[ 0 ] == '-' ) {
    if ( str[ 1 ] == 'N' || str[ 1 ] == 'I' ) {
      str[ 4 ] = '\0'; /* -NaN, -Inf */
      return 4;
    }
  }
  else {
    if ( str[ 0 ] == 'N' || str[ 0 ] == 'I' ) {
      str[ 3 ] = '\0'; /* NaN, Inf */
      return 3;
    }
  }
  return strlen( str );
}

static void
dec128_binop( Dec128Store *out,  const Dec128Store *l,
              const Dec128Store *r,  char op )
{
  decContext ctx128;
  decNumber lhs, rhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  decimal128ToNumber( (decimal128 *) r, &rhs );
  switch ( op ) {
    default:
    case '+': decNumberAdd( &fp, &lhs, &rhs, &ctx128 ); break;
    case '-': decNumberSubtract( &fp, &lhs, &rhs, &ctx128 ); break;
    case '*': decNumberMultiply( &fp, &lhs, &rhs, &ctx128 ); break;
    case '/': decNumberDivide( &fp, &lhs, &rhs, &ctx128 ); break;
    case '%': decNumberRemainder( &fp, &lhs, &rhs, &ctx128 ); break;
    case 'p': decNumberPower( &fp, &lhs, &rhs, &ctx128 ); break;
  }
  decimal128FromNumber( (decimal128 *) out, &fp, &ctx128 );
}

void
es_dec128_sum( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  dec128_binop( out, l, r, '+' );
}

void
es_dec128_diff( Dec128Store *out,  const Dec128Store *l,
                const Dec128Store *r )
{
  dec128_binop( out, l, r, '-' );
}

void
es_dec128_mul( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  dec128_binop( out, l, r, '*' );
}

void
es_dec128_div( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  dec128_binop( out, l, r, '/' );
}

void
es_dec128_mod( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  dec128_binop( out, l, r, '%' );
}

void
es_dec128_pow( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  dec128_binop( out, l, r, 'p' );
}

static void
dec128_unaryop( Dec128Store *out,  const Dec128Store *l,  char op )
{
  decContext ctx128;
  decNumber lhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  if ( op == 'f' )
    decContextSetRounding( &ctx128, DEC_ROUND_DOWN );
  else if ( op == 'r' )
    decContextSetRounding( &ctx128, DEC_ROUND_HALF_UP );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  switch ( op ) {
    default:
    case 'l': decNumberLn( &fp, &lhs, &ctx128 ); break; /* ln */
    case 'L': decNumberLog10( &fp, &lhs, &ctx128 ); break; /* log10 */
    case 'r': /* round */
    case 'f': decNumberToIntegralValue( &fp, &lhs, &ctx128 ); break; /* floor */
  }
  decimal128FromNumber( (decimal128 *) out, &fp, &ctx128 );
}

void
es_dec128_ln( Dec128Store *out,  const Dec128Store *l )
{
  dec128_unaryop( out, l, 'l' );
}

void
es_dec128_log10( Dec128Store *out,  const Dec128Store *l )
{
  dec128_unaryop( out, l, 'L' );
}

void
es_dec128_floor( Dec128Store *out,  const Dec128Store *l )
{
  dec128_unaryop( out, l, 'f' );
}

void
es_dec128_round( Dec128Store *out,  const Dec128Store *l )
{
  dec128_unaryop( out, l, 'r' );
}

int
es_dec128_isinf( const Dec128Store *fp )
{
  decContext ctx128;
  decNumber val;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128ToNumber( (decimal128 *) fp, &val );
  switch ( decNumberClass( &val, &ctx128 ) ) {
    case DEC_CLASS_NEG_INF:
    case DEC_CLASS_POS_INF: return 1;
    default: return 0;
  }
}

int
es_dec128_isnan( const Dec128Store *fp )
{
  decContext ctx128;
  decNumber val;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128ToNumber( (decimal128 *) fp, &val );
  switch ( decNumberClass( &val, &ctx128 ) ) {
    case DEC_CLASS_SNAN:
    case DEC_CLASS_QNAN: return 1;
    default: return 0;
  }
}

int
es_dec128_compare( const Dec128Store *l,  const Dec128Store *r )
{
  decContext ctx128;
  decNumber lhs, rhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  decimal128ToNumber( (decimal128 *) r, &rhs );
  decNumberSubtract( &fp, &lhs, &rhs, &ctx128 );
  switch ( decNumberClass( &fp, &ctx128 ) ) {
    case DEC_CLASS_NEG_INF:
    case DEC_CLASS_NEG_NORMAL:
    case DEC_CLASS_NEG_SUBNORMAL:
    case DEC_CLASS_NEG_ZERO: return -1;
    case DEC_CLASS_POS_NORMAL:
    case DEC_CLASS_POS_SUBNORMAL:
    case DEC_CLASS_POS_INF:  return 1;
    case DEC_CLASS_POS_ZERO: return 0;
    default:
    case DEC_CLASS_SNAN:
    case DEC_CLASS_QNAN:     return 2;
  }
}

int
es_dec128_eq( const Dec128Store *l, const Dec128Store *r )
{
  return es_dec128_compare( l, r ) == 0;
}

int
es_dec128_lt( const Dec128Store *l, const Dec128Store *r )
{
  return es_dec128_compare( l, r ) < 0;
}

int
es_dec128_gt( const Dec128Store *l, const Dec128Store *r )
{
  return es_dec128_compare( l, r ) > 0;
}

