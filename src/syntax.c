/* syntax.c -- abstract syntax tree re-writing rules ($Revision: 1.1.1.1 $) */

#include <es/es.h>
#include <es/input.h>
#include <es/syntax.h>
#include <es/token.h>

Tree errornode;
Tree *parsetree;

/* initparse -- called at the dawn of time */
void
initparse( void )
{
  globalroot( &parsetree );
}

/* treecons -- create new tree list cell */
Tree *
treecons( Tree *car, Tree *cdr )
{
  assert( cdr == NULL || cdr->kind == nList );
  return mk( nList, car, cdr );
}

/* treecons2 -- create new tree list cell or do nothing if car is NULL */
Tree *
treecons2( Tree *car, Tree *cdr )
{
  assert( cdr == NULL || cdr->kind == nList );
  return car == NULL ? cdr : mk( nList, car, cdr );
}

/* treeappend -- destructive append for tree lists */
Tree *
treeappend( Tree *head, Tree *tail )
{
  Tree *p, **prevp;
  for ( p = head, prevp = &head; p != NULL; p = *( prevp = &p->CDR ) )
    assert( p->kind == nList || p->kind == nRedir );
  *prevp = tail;
  return head;
}

/* treeconsend -- destructive add node at end for tree lists */
Tree *
treeconsend( Tree *head, Tree *tail )
{
  return treeappend( head, treecons( tail, NULL ) );
}

/* treeconsend2 -- destructive add node at end for tree lists or nothing if
 * tail is NULL */
Tree *
treeconsend2( Tree *head, Tree *tail )
{
  if ( tail == NULL ) {
    assert( head == NULL || head->kind == nList || head->kind == nRedir );
    return head;
  }
  return treeappend( head, treecons( tail, NULL ) );
}

/* thunkify -- wrap a tree in thunk braces if it isn't already a thunk */
Tree *
thunkify( Tree *tree )
{
  if ( tree != NULL && ( ( tree->kind == nThunk ) ||
                         ( tree->kind == nList && tree->CAR->kind == nThunk &&
                           tree->CDR == NULL ) ) )
    return tree;
  return mk( nThunk, tree );
}

/* firstis -- check if the first word of a literal command matches a known
 * string */
static Boolean
firstis( Tree *t, const char *s )
{
  if ( t == NULL || t->kind != nList )
    return FALSE;
  t = t->CAR;
  if ( t == NULL || t->kind != nWord )
    return FALSE;
  assert( t->u[ 0 ].s != NULL );
  return streq( t->u[ 0 ].s, s );
}

/* prefix -- prefix a tree with a given word */
Tree *
prefix( char *s, Tree *t )
{
  return treecons( mk( nWord, s ), t );
}

/* flatten -- flatten the output of the glommer so we can pass the result as a
 * single element */
Tree *
flatten( Tree *t, char *sep )
{
  return mk( nCall,
           prefix( "%flatten",
             treecons( mk( nQword, sep ), treecons( t, NULL ) ) ) );
}

/* backquote -- create a backquote command */
Tree *
backquote( Tree *ifs, Tree *body )
{
  return mk( nCall,
             prefix( "%backquote",
                     treecons( flatten( ifs, "" ), treecons( body, NULL ) ) ) );
}

/* fnassign -- translate a function definition into an assignment */
Tree *
fnassign( Tree *name, Tree *defn )
{
  return mk( nAssign, mk( nConcat, mk( nWord, "fn-" ), name ), defn );
}

/* mklambda -- create a lambda */
Tree *
mklambda( Tree *params, Tree *body )
{
  return mk( nLambda, params, body );
}

/* mkseq -- destructively add to a sequence of nList/nThink operations */
Tree *
mkseq( char *op, Tree *t1, Tree *t2 )
{
  Tree *  tail;
  Boolean sametail;

  if ( streq( op, "%seq" ) ) {
    if ( t1 == NULL )
      return t2;
    if ( t2 == NULL )
      return t1;
  }

  sametail = firstis( t2, op );
  tail     = sametail ? t2->CDR : treecons( thunkify( t2 ), NULL );
  if ( firstis( t1, op ) )
    return treeappend( t1, tail );
  t1 = thunkify( t1 );
  if ( sametail ) {
    t2->CDR = treecons( t1, tail );
    return t2;
  }
  return prefix( op, treecons( t1, tail ) );
}

/* mkpipe -- assemble a pipe from the commands that make it up (destructive) */
Tree *
mkpipe( Tree *t1, int outfd, int infd, Tree *t2 )
{
  Tree *  tail;
  Boolean pipetail;

  pipetail = firstis( t2, "%pipe" );
  tail =
    prefix( str( "%d", outfd ),
            prefix( str( "%d", infd ),
                    pipetail ? t2->CDR : treecons( thunkify( t2 ), NULL ) ) );
  if ( firstis( t1, "%pipe" ) )
    return treeappend( t1, tail );
  t1 = thunkify( t1 );
  if ( pipetail ) {
    t2->CDR = treecons( t1, tail );
    return t2;
  }
  return prefix( "%pipe", treecons( t1, tail ) );
}

/*
 * redirections -- these involve queueing up redirection in the prefix of a
 *	tree and then rewriting the tree to include the appropriate commands
 */

static Tree placeholder = { nRedir };

Tree *
redirect( Tree *t )
{
  Tree *r, *p;
  if ( t == NULL )
    return NULL;
  if ( t->kind != nRedir )
    return t;
  r = t->CAR;
  t = t->CDR;
  for ( ; r->kind == nRedir; r = r->CDR )
    t = treeappend( t, r->CAR );
  for ( p = r; p->CAR != &placeholder; p = p->CDR ) {
    assert( p != NULL );
    assert( p->kind == nList );
  }
  if ( firstis( r, "%heredoc" ) )
    if ( !queueheredoc( r ) )
      return &errornode;
  p->CAR = thunkify( redirect( t ) );
  return r;
}

Tree *
mkredircmd( char *cmd, int fd )
{
  return prefix( cmd, prefix( str( "%d", fd ), NULL ) );
}

Tree *
mkredir( Tree *cmd, Tree *file )
{
  Tree *word = NULL;
  if ( file != NULL && file->kind == nThunk ) { /* /dev/fd operations */
    char *     op;
    Tree *     var;
    static int id = 0;
    if ( firstis( cmd, "%open" ) )
      op = "%readfrom";
    else if ( firstis( cmd, "%create" ) )
      op = "%writeto";
    else {
      yyerror( "bad /dev/fd redirection" );
      op = "";
    }
    var  = mk( nWord, str( "_devfd%d", id++ ) );
    cmd  = treecons( mk( nWord, op ), treecons( var, NULL ) );
    word = treecons( mk( nVar, var ), NULL );
  }
  else if ( !firstis( cmd, "%heredoc" ) && !firstis( cmd, "%here" ) )
    file = mk( nCall, prefix( "%one", treecons( file, NULL ) ) );
  cmd = treeappend( cmd, treecons( file, treecons( &placeholder, NULL ) ) );
  if ( word != NULL )
    cmd = mk( nRedir, word, cmd );
  return cmd;
}

/* mkclose -- make a %close node with a placeholder */
Tree *
mkclose( int fd )
{
  return prefix( "%close",
                 prefix( str( "%d", fd ), treecons( &placeholder, NULL ) ) );
}

/* mkdup -- make a %dup node with a placeholder */
Tree *
mkdup( int fd0, int fd1 )
{
  return prefix( "%dup", prefix( str( "%d", fd0 ),
                                 prefix( str( "%d", fd1 ),
                                         treecons( &placeholder, NULL ) ) ) );
}

/* redirappend -- destructively add to the list of redirections, before any
 * other nodes */
Tree *
redirappend( Tree *tree, Tree *r )
{
  Tree *t, **tp;
  for ( ; r->kind == nRedir; r = r->CDR )
    tree = treeappend( tree, r->CAR );
  assert( r->kind == nList );
  for ( t = tree, tp = &tree; t != NULL && t->kind == nRedir;
        t = *( tp = &t->CDR ) )
    ;
  assert( t == NULL || t->kind == nList );
  *tp = mk( nRedir, r, t );
  return tree;
}

/* relop -- build the prefix form of an infix relop */
Tree *
relop( Tree *left, Tree *right, Relation rel )
{
  Tree *match = NULL;
  switch ( rel ) {
    case Less: match = mk( nWord, "-1" ); break;
    case LessEqual:
      match = treecons( mk( nWord, "-1" ), treecons( mk( nWord, "0" ), NULL ) );
      break;
    case Greater: match = mk( nWord, "1" ); break;
    case GreaterEqual:
      match = treecons( mk( nWord, "1" ), treecons( mk( nWord, "0" ), NULL ) );
      break;
    case Equal: match = mk( nWord, "0" ); break;
    case NotEqual:
      match = treecons( mk( nWord, "-1" ), treecons( mk( nWord, "1" ), NULL ) );
      break;
  }
  return mk( nMatch,
             mk( nCall, treecons( mk( nWord, "%cmp" ),
                                  treecons( left, treecons( right, NULL ) ) ) ),
             match );
}

