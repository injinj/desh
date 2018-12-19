/* input.h -- definitions for es lexical analyzer ($Revision: 1.1.1.1 $) */
#ifndef __es__input_h__
#define __es__input_h__

#ifdef __cplusplus
extern "C" { 
#endif

#define	MAXUNGET	2		/* maximum 2 character pushback */

typedef struct Input Input;
struct Input {
	int (*get)(Input *self);
	int (*fill)(Input *self), (*rfill)(Input *self);
	void (*cleanup)(Input *self);
	Input *prev;
	const char *name;
	unsigned char *buf, *bufend, *bufbegin, *rbuf;
	size_t buflen;
	int unget[MAXUNGET];
	int ungot;
	int lineno;
	int fd;
	int runflags;
};


#define	GETC()		(*input->get)(input)
#define	UNGETC(c)	unget(input, c)


/* input.c */

extern Input *input;
extern void unget(Input *in, int c);
extern Boolean heredoc_input;
extern Boolean reset_terminal;
extern void yyerror(char *s);
extern void initgetenv(void);

/* token.c */

extern const char non_word[];
extern const char dollar_non_word[];
extern int yylex(void);
extern void inityy(void);
extern void print_prompt2(void);


/* parse.y */

extern Tree *parsetree;
extern int yyparse(void);
extern void initparse(void);


/* heredoc.c */

extern void emptyherequeue(void);

#ifdef __cplusplus
}
#endif

#endif
