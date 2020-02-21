#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>


const char *
getsigname( int sig,  char *buf,  size_t n )
{
  char dig[ 3 ], * name;
  int len = 0;
  size_t i, j, k;

  if ( sig > 10 )
    dig[ len++ ] = ( ( sig / 10 ) % 10 ) + '0';
  dig[ len++ ] = ( sig % 10 ) + '0';
  dig[ len ] = '\0';
  for ( i = 0; i < n; i++ ) {
    if ( strncmp( dig, &buf[ i ], len ) == 0 ) {
      i += len;
      if ( ! isdigit( buf[ i ] ) ) {
        while ( i + 1 < n && ! isalpha( buf[ i + 1 ] ) )
          i++;
        for ( j = 1; ; j++ ) {
          if ( i + j >= n || ! isalpha( buf[ i + j ] ) ) {
            name = (char *) malloc( 3 + j );
            strcpy( name, "sig" );
            for ( k = 1; k < j; k++ )
              name[ 3 + k - 1 ] = tolower( buf[ i + k ] );
            name[ 3 + k - 1 ] = '\0';
            return name;
          }
        }
      }
    }
  }
  name = (char *) malloc( 8 );
  snprintf( name, 8, "sig%d", sig );
  return name;
}

int
main( int argc, char *argv[] )
{
  FILE *fp = popen( "/bin/kill -L", "r" );
  char buf[ 4 * 1024 ];
  size_t n = 0;
  int sig;
  const char *name, *descr;

  if ( fp != NULL ) {
    n = fread( buf, 1, sizeof( buf ) - 1, fp );
    pclose( fp );
  }
  if ( fp == NULL || n == 0 ) {
    perror( "kill" );
    printf( "#error \"/bin/kill -L (errno=%d)\"\n", errno );
    return 1;
  }
  buf[ n ] = '\0';

  printf( "#include <desh/es.h>\n"
          "#include <desh/sigmsgs.h>\n"
          "\n"
          "/*\n"
          "$ /bin/kill -L\n"
          "%.*s\n"
          "*/\n"
          "const Sigmsgs signals[] = {\n", (int) n, buf );

  for ( sig = 1; sig < NSIG; sig++ ) {
    descr = strsignal( sig );
    name  = getsigname( sig, buf, n );
    printf( "{ %d, \"%s\", \"%s\" },\n", sig, name, descr );
  }

  printf( "};\n"
          "\n"
          "const int nsignals = arraysize(signals); /* NSIG=%d */\n", NSIG );

  return 0;
}

