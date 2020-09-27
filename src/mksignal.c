#define _GNU_SOURCE
#include <stdio.h>

#ifndef __APPLE__
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
          if ( i + j >= n || ! isalnum( buf[ i + j ] ) ) {
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
#else
int
main( void )
{
  printf( "#include <desh/es.h>\n"
          "#include <desh/sigmsgs.h>\n"
          "\n"
          "/*\n"
          "$ apple sigs from manpage\n"
          "*/\n"
          "const Sigmsgs signals[] = {\n" );
  printf( "{ 1, \"sighup\", \"terminal line hangup\" },\n" );
  printf( "{ 2, \"sigint\", \"interrupt program\" },\n" );
  printf( "{ 3, \"sigquit\", \"quit program\" },\n" );
  printf( "{ 4, \"sigill\", \"illegal instruction\" },\n" );
  printf( "{ 5, \"sigtrap\", \"trace trap\" },\n" );
  printf( "{ 6, \"sigabrt\", \"abort program (formerly SIGIOT)\" },\n" );
  printf( "{ 7, \"sigemt\", \"emulate instruction executed\" },\n" );
  printf( "{ 8, \"sigfpe\", \"floating-point exception\" },\n" );
  printf( "{ 9, \"sigkill\", \"kill program\" },\n" );
  printf( "{ 10, \"sigbus\", \"bus error\" },\n" );
  printf( "{ 11, \"sigsegv\", \"segmentation violation\" },\n" );
  printf( "{ 12, \"sigsys\", \"non-existent system call invoked\" },\n" );
  printf( "{ 13, \"sigpipe\", \"write on a pipe with no reader\" },\n" );
  printf( "{ 14, \"sigalrm\", \"real-time timer expired\" },\n" );
  printf( "{ 15, \"sigterm\", \"software termination signal\" },\n" );
  printf( "{ 16, \"sigurg\", \"urgent condition present on socket\" },\n" );
  printf( "{ 17, \"sigstop\", \"stop (cannot be caught or ignored)\" },\n" );
  printf( "{ 18, \"sigtstp\", \"stop signal generated from keyboard\" },\n" );
  printf( "{ 19, \"sigcont\", \"continue after stop\" },\n" );
  printf( "{ 20, \"sigchld\", \"child status has changed\" },\n" );
  printf( "{ 21, \"sigttin\", \"background read attempted from control terminal\" },\n" );
  printf( "{ 22, \"sigttou\", \"background write attempted to control terminal\" },\n" );
  printf( "{ 23, \"sigio\", \"I/O is possible on a descriptor\" },\n" );
  printf( "{ 24, \"sigxcpu\", \"cpu time limit exceeded\" },\n" );
  printf( "{ 25, \"sigxfsz\", \"file size limit exceeded\" },\n" );
  printf( "{ 26, \"sigvtalrm\", \"virtual time alarm\" },\n" );
  printf( "{ 27, \"sigprof\", \"profiling timer alarm\" },\n" );
  printf( "{ 28, \"sigwinch\", \"Window size change\" },\n" );
  printf( "{ 29, \"siginfo\", \"status request from keyboard\" },\n" );
  printf( "{ 30, \"sigusr1\", \"User defined signal 1\" },\n" );
  printf( "{ 31, \"sigusr2\", \"User defined signal 2\" },\n" );
  printf( "};\n"
          "\n"
          "const int nsignals = arraysize(signals);\n" );

  return 0;
}
#endif
