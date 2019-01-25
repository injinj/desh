#ifndef __es__decimal_h__
#define __es__decimal_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct { uint64_t n[ 2 ]; } Dec128Store;

void es_dec128_itod( Dec128Store *fp,  int i );
void es_dec128_from_string( Dec128Store *fp,  const char *str );
void es_dec128_zero( Dec128Store *fp );
size_t es_dec128_to_string( const Dec128Store *fp,  char *str );
void es_dec128_sum( Dec128Store *out,  const Dec128Store *l,
                    const Dec128Store *r );
void es_dec128_diff( Dec128Store *out,  const Dec128Store *l,
                     const Dec128Store *r );
void es_dec128_mul( Dec128Store *out,  const Dec128Store *l,
                    const Dec128Store *r );
void es_dec128_div( Dec128Store *out,  const Dec128Store *l,
                    const Dec128Store *r );
void es_dec128_mod( Dec128Store *out,  const Dec128Store *l,
                    const Dec128Store *r );
void es_dec128_pow( Dec128Store *out,  const Dec128Store *l,
                    const Dec128Store *r );
void es_dec128_ln( Dec128Store *out,  const Dec128Store *l );
void es_dec128_log10( Dec128Store *out,  const Dec128Store *l );
void es_dec128_floor( Dec128Store *out,  const Dec128Store *l );
void es_dec128_round( Dec128Store *out,  const Dec128Store *l );
int es_dec128_isinf( const Dec128Store *fp );
int es_dec128_isnan( const Dec128Store *fp );
int es_dec128_compare( const Dec128Store *l, const Dec128Store *r );
int es_dec128_eq( const Dec128Store *l, const Dec128Store *r );
int es_dec128_lt( const Dec128Store *l, const Dec128Store *r );
int es_dec128_gt( const Dec128Store *l, const Dec128Store *r );

#ifdef __cplusplus
}

namespace es {

static inline char *
null_term( char *buf,  size_t buflen,  const char *str,  size_t strlen ) {
  if ( strlen > buflen - 1 ) strlen = buflen - 1;
  ::memcpy( buf, str, strlen );
  buf[ strlen ] = '\0';
  return buf;
}

struct Dec128 {
  Dec128Store fp;

  static Dec128 parse( const char *str ) {
    Dec128 x; es_dec128_from_string( &x.fp, str ); return x;
  }
  static Dec128 parse_len( const char *str,  size_t len ) {
    char buf[ 64 ];
    return Dec128::parse( null_term( buf, sizeof( buf ), str, len ) );
  }
  void zero( void ) {
    es_dec128_zero( &this->fp );
  }
  size_t to_string( char *str ) {
    return es_dec128_to_string( &this->fp, str );
  }
  void from_string( const char *str,  size_t len ) {
    char buf[ 64 ];
    es_dec128_from_string( &this->fp,
      null_term( buf, sizeof( buf ), str, len ) );
  }
  void from_string( const char *str ) {
    es_dec128_from_string( &this->fp, str );
  }
  Dec128& operator =( int i ) {
    es_dec128_itod( &this->fp, i );
    return *this;
  }
  Dec128& operator =( double f ) {
    es_dec128_itod( &this->fp, f );
    return *this;
  }
  Dec128& operator +=( const Dec128 &f ) {
    es_dec128_sum( &this->fp, &this->fp, &f.fp );
    return *this;
  }
  Dec128& operator -=( const Dec128 &f ) {
    es_dec128_diff( &this->fp, &this->fp, &f.fp );
    return *this;
  }
  Dec128 operator +( const Dec128 &f ) const {
    Dec128 tmp;
    es_dec128_sum( &tmp.fp, &this->fp, &f.fp );
    return tmp;
  }
  Dec128& operator *=( const Dec128 &f ) {
    es_dec128_mul( &this->fp, &this->fp, &f.fp );
    return *this;
  }
  Dec128& operator /=( const Dec128 &f ) {
    es_dec128_div( &this->fp, &this->fp, &f.fp );
    return *this;
  }
  Dec128& operator %=( const Dec128 &f ) {
    es_dec128_mod( &this->fp, &this->fp, &f.fp );
    return *this;
  }
  Dec128& pow( const Dec128 &f ) {
    es_dec128_pow( &this->fp, &this->fp, &f.fp );
    return *this;
  }
  Dec128& ln( void ) {
    es_dec128_ln( &this->fp, &this->fp );
    return *this;
  }
  Dec128& log10( void ) {
    es_dec128_log10( &this->fp, &this->fp );
    return *this;
  }
  Dec128& floor( void ) {
    es_dec128_floor( &this->fp, &this->fp );
    return *this;
  }
  Dec128& round( void ) {
    es_dec128_round( &this->fp, &this->fp );
    return *this;
  }
  bool isinf( void ) const {
    return es_dec128_isinf( &this->fp );
  }
  bool isnan( void ) const {
    return es_dec128_isnan( &this->fp );
  }
  Dec128 operator *( const Dec128 &f ) const {
    Dec128 tmp;
    es_dec128_mul( &tmp.fp, &this->fp, &f.fp );
    return tmp;
  }
  bool operator ==( const Dec128 &f ) const {
    return es_dec128_eq( &this->fp, &f.fp );
  }
  bool operator !=( const Dec128 &f ) const {
    return ! es_dec128_eq( &this->fp, &f.fp );
  }
  bool operator <( const Dec128 &f ) const {
    return es_dec128_lt( &this->fp, &f.fp );
  }
  bool operator >( const Dec128 &f ) const {
    return es_dec128_gt( &this->fp, &f.fp );
  }
  bool operator >=( const Dec128 &f ) const {
    return ! es_dec128_gt( &this->fp, &f.fp );
  }
  bool operator <=( const Dec128 &f ) const {
    return ! es_dec128_lt( &this->fp, &f.fp );
  }
};

}
#endif
#endif
