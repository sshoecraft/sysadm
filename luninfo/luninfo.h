
#ifndef __LUNINFO_H
#define __LUNINFO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"

#define DEV_SIZE 256
#define VOL_SIZE 256

struct mpdevs_entry {
	char name[DEV_SIZE];		/* /dev/dm-x | disk_x */
	char alt_name[DEV_SIZE];	/* alt name, if any */
	list devs;			/* Dev names */
};

struct luninfo {
	char dev[DEV_SIZE];		/* sd[a-z][a-z][a-z] | cxxtxxdxx */
	char label[64];			/* Disk label/name */
	unsigned char inqdata[256];	/* std inquiry page */
	char vol[VOL_SIZE];		/* Volume name */
	int size;			/* Size */
	char ssize[8];			/* Size string w/unit */
	int lunid;			/* LUN ID */
	char vendor[32];		/* Vendor */
	char product[32];		/* Product */
	char wwn[64];			/* WWN */
	char spec[64];			/* Vendor-specific data */
	void *extra;			/* Device specific (only used for xp atm) */
};

#ifdef __MINGW32__
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
typedef void * filehandle_t;
#define OPENDEV(dev) CreateFile(dev,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
#define CLOSEDEV(fd) CloseHandle(fd);
#define INVALID_HANDLE(fd) (fd == INVALID_HANDLE_VALUE)
#else
typedef int filehandle_t;
#define OPENDEV(dev) open(dev,O_RDONLY|O_NONBLOCK);
#define CLOSEDEV(fd) close(fd)
#define INVALID_HANDLE(fd) (fd < 0)
#endif

#define KB (long double)1024
#define MB (KB*1024)
#define GB (MB*1024)
#define TB (GB*1024)

extern char temp[1024];
//extern char tempdir[256];
//extern list mpdevs;

list get_info(int,int,int);

/* utils */
char *get_tmpfile(char *);
list get_mp_devs_from_name(list,char *);
char *trim(char *);
char *strele(int,char *,char *);
char *stredit(char *,char *);
void bindump(char *, void *, int);
int get_path(char *,char **);
char *concat_path(char *,char *,char *);

/* os-specific */
void get_mp(list);
int is_mp(list,char *);
list get_devs(void);
int do_ioctl(filehandle_t,unsigned char *, int, unsigned char *, int);

/* Common SCSI commands */
#include "scsi.h"

/* main funcs */
void get_vgnames(list,list);
void get_dgnames(list,list);

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
