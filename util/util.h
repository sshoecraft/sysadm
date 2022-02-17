
#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef __MINGW32__
#include <netinet/in.h>
#include <netdb.h>
#else
#include <windows.h>
#endif

#if 0
#ifdef __MINGW32__
#include "getopt.h"
#endif
#endif

char *stredit(char *string, char *list);
char *strele(int num,char *delimiter,char *string);
char *trim(char *string);
void _bindump(long offset,unsigned char *buf,int len);
int bindump(char *);
int fd_printf(int fd,char *fmt,...);
int fd_gets(int fd, char *buffer, unsigned short buflen);
int fd_telgets(int fd,char *buffer,unsigned short buflen,int wait);
struct hostent *getdnshostent(char *name);
int name2addr(char *,char *);

#if NEED_DPRINTF
extern int debug;
#define dprintf(level, format, args...) if (debug >= level) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#endif

#endif
