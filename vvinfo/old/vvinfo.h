
#ifndef __VVINFO_H
#define __VVINFO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __MINGW32__
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
typedef void * filehandle_t;
#else
typedef int filehandle_t;
#endif

/* os-specific */
int do_ioctl(filehandle_t,unsigned char *, int, unsigned char *, int);

/* Common */
int inquiry(filehandle_t,unsigned char *,int *);
int identify(filehandle_t,int *,char *,char *);
int capacity(filehandle_t,int *);

#ifndef dprintf
#if DEBUG
#if defined(__hpux)
#define dprintf(format, args...) printf(format,## args)
#else
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#endif
#else
#define dprintf(format, args...) /* noop */
#endif
#endif

#endif
