#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <desh/decimal.h>
#define DECNUMDIGITS 64
#include <libdecnumber/bid/decimal128.h>

void
es_dec128_itod( Dec128Store *fp,  int i )
{
  *(_Decimal128 *) fp = i;
}

void
es_dec128_ftod( Dec128Store *fp,  double f )
{
  *(_Decimal128 *) fp = f;
}

void
es_dec128_from_string( Dec128Store *fp,  const char *str )
{
  decContext ctx128;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128FromString( (decimal128 *) fp, str, &ctx128 );
  /*gcc_decimal128( fp );*/
}

void
es_dec128_zero( Dec128Store *fp )
{
  *(_Decimal128 *) fp = 0;
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

void
es_dec128_sum( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  *(_Decimal128 *) out = *(const _Decimal128 *) l +
                         *(const _Decimal128 *) r;
}

void
es_dec128_diff( Dec128Store *out,  const Dec128Store *l,
                const Dec128Store *r )
{
  *(_Decimal128 *) out = *(const _Decimal128 *) l -
                         *(const _Decimal128 *) r;
}

void
es_dec128_mul( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  *(_Decimal128 *) out = *(const _Decimal128 *) l *
                         *(const _Decimal128 *) r;
}

void
es_dec128_div( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  *(_Decimal128 *) out = *(const _Decimal128 *) l /
                         *(const _Decimal128 *) r;
}

void
es_dec128_mod( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  decContext ctx128;
  decNumber lhs, rhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  decimal128ToNumber( (decimal128 *) r, &rhs );
  decNumberRemainder( &fp, &lhs, &rhs, &ctx128 );
  decimal128FromNumber( (decimal128 *) out, &fp, &ctx128 );
}

void
es_dec128_pow( Dec128Store *out,  const Dec128Store *l,
               const Dec128Store *r )
{
  decContext ctx128;
  decNumber lhs, rhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  decimal128ToNumber( (decimal128 *) r, &rhs );
  decNumberPower( &fp, &lhs, &rhs, &ctx128 );
  decimal128FromNumber( (decimal128 *) out, &fp, &ctx128 );
}

void
es_dec128_ln( Dec128Store *out,  const Dec128Store *l )
{
  decContext ctx128;
  decNumber lhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  decNumberLn( &fp, &lhs, &ctx128 );
  decimal128FromNumber( (decimal128 *) out, &fp, &ctx128 );
}

void
es_dec128_log10( Dec128Store *out,  const Dec128Store *l )
{
  decContext ctx128;
  decNumber lhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  decNumberLog10( &fp, &lhs, &ctx128 );
  decimal128FromNumber( (decimal128 *) out, &fp, &ctx128 );
}

void
es_dec128_floor( Dec128Store *out,  const Dec128Store *l )
{
  decContext ctx128;
  decNumber lhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decContextSetRounding( &ctx128, DEC_ROUND_DOWN );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  decNumberToIntegralValue( &fp, &lhs, &ctx128 );
  decimal128FromNumber( (decimal128 *) out, &fp, &ctx128 );
}

void
es_dec128_round( Dec128Store *out,  const Dec128Store *l )
{
  decContext ctx128;
  decNumber lhs, fp;
  decContextDefault( &ctx128, DEC_INIT_DECIMAL128 );
  decContextSetRounding( &ctx128, DEC_ROUND_HALF_UP );
  decimal128ToNumber( (decimal128 *) l, &lhs );
  decNumberToIntegralValue( &fp, &lhs, &ctx128 );
  decimal128FromNumber( (decimal128 *) out, &fp, &ctx128 );
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
es_dec128_eq( const Dec128Store *l, const Dec128Store *r )
{
  return *(const _Decimal128 *) l == *(const _Decimal128 *) r;
}

int
es_dec128_lt( const Dec128Store *l, const Dec128Store *r )
{
  return *(const _Decimal128 *) l < *(const _Decimal128 *) r;
}

int
es_dec128_gt( const Dec128Store *l, const Dec128Store *r )
{
  return *(const _Decimal128 *) l > *(const _Decimal128 *) r;
}

