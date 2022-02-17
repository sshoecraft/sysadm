
#ifndef __XPLIST_H
#define __XPLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"

#define DEV_SIZE 32
#define VOL_SIZE 256

struct mpdevs_entry {
	char name[DEV_SIZE];		/* /dev/dm-x | disk_x */
	char alt_name[DEV_SIZE];	/* alt name, if any */
	list devs;			/* Dev names */
};

struct dev_info {
	char name[DEV_SIZE];
	int major;
	int minor;
};

struct xplist_entry {
	char dev[DEV_SIZE];		/* sd[a-z][a-z][a-z] | cxxtxxdxx */
	char vol[VOL_SIZE];		/* Volume name */
	char size[8];			/* Size */
	int lunid;			/* LUN ID */
	char port[5];			/* Port, CLXX */
	unsigned char cu,ldev;		/* cu & ldev */
	char type[17];			/* OPEN-V[*#] */
	int serial;			/* Serial # */
	unsigned long long id;		/* 64-bit device ID */
};

struct xplist_inqdata {
	char vendor[9];
	char product[16];
};

#ifdef __MINGW32__
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
typedef void * filehandle_t;
#else
typedef int filehandle_t;
#endif

extern char temp[1024];
extern char tempdir[256];
extern list mpdevs;

/* utils */
char *trim(char *);
char *strele(int,char *,char *);
char *stredit(char *,char *);
void bindump(char *, void *, int);
int get_path(char *,char **);
char *concat_path(char *,char *,char *);

/* os-specific */
void get_mp(void);
int is_mp(char *);
list get_mp_devs(char *);
list get_devs(void);
int do_ioctl(filehandle_t,unsigned char *, int, unsigned char *, int);

/* Common */
int inquiry(filehandle_t,unsigned char *,int *);
int identify(filehandle_t,int *,char *,char *);
int capacity(filehandle_t,int *);

/* main funcs */
list get_xplist(void);
void get_esxvols(list);
void get_vgnames(list);
void get_dgnames(list);

#if DEBUG
#if defined(__hpux)
//#define dprintf(format, args...) printf("%s(%d): " format,"",__LINE__, ## args)
#define dprintf(format, args...) printf(format,## args)
#else
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#endif
#else
#define dprintf(format, args...) /* noop */
#endif

#endif
