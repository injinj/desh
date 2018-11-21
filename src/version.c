/* version.c -- version number ($Revision: 1.2 $) */
#include <desh/es.h>
#define xstr(a) str(a)
#define str(a) #a
static const char id[] = "@(#)" SHNAME " version " xstr(DESH_VER);
const char * const version = id + (sizeof "@(#)" - 1);
