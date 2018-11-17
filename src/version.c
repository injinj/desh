/* version.c -- version number ($Revision: 1.2 $) */
#include <es/es.h>
#define stringify(X) #X
static const char id[] = "@(#)" SHNAME " version " stringify(ES_VER);
const char * const version = id + (sizeof "@(#)" - 1);
