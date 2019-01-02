/* input.h -- definitions for es lexical analyzer ($Revision: 1.1.1.1 $) */
#ifndef __es__input_h__
#define __es__input_h__

#ifdef __cplusplus
extern "C" { 
#endif

#define	MAXUNGET	2		/* maximum 2 character pushback */

typedef struct Input Input;
struct Input {
  int ( *get )( Input *self );
  int ( *fill )( Input *self ), ( *rfill )( Input *self );
  void ( *cleanup )( Input *self );
  Input *        prev;
  const char *   name;
  unsigned char *buf, *bufend, *bufbegin, *rbuf;
  size_t         buflen;
  int            unget[ MAXUNGET ];
  int            ungot;
  int            lineno;
  int            fd;
  int            runflags;
};

#define GETC() ( *input->get )( input )
#define UNGETC( c ) unget( input, c )

/* eval_* flags are also understood as runflags */
#define run_interactive          4      /* -i or $0[0] = '-' */
#define run_noexec               8      /* -n */
#define run_echoinput           16      /* -v */
#define run_printcmds           32      /* -x */
#define run_lisptrees           64      /* -L and defined(LISPTREES) */
#define run_interrupt          128      /* ctrl-c on input */

/* input.c */
extern Boolean heredoc_input;
extern Boolean reset_terminal;
extern Input *input;
#define MAX_PROMPT_COUNT 16
extern Boolean is_prompt2; /* display secondary prompt */

void yyerror( char *s );
void sethistory( char *file );
int keybind( char **args, int argc );
void unget( Input *in, int c );
void initgetenv( void );
Tree *parse( char **pr );
void resetparser( void );
List *runinput( Input *in, int runflags );
List *runfd( int fd, const char *name, int flags );
List *runstring( const char *str, const char *name, int flags );
Tree *parseinput( Input *in );
Tree *parsestring( const char *str );
Boolean isinteractive( void );
void initinput( void );

/* token.c */
extern const char non_word[];
extern const char dollar_non_word[];
int yylex( void );
void inityy( void );
void print_prompt2( void );

/* parse.y */
extern Tree *parsetree;
int yyparse( void );
void initparse( void );

/* heredoc.c */
void emptyherequeue( void );

#ifdef __cplusplus
}
#endif

#endif
