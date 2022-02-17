
#ifndef __XPLIST_H
#define __XPLIST_H

struct xplist_entry {
	char dev[16];			/* /dev/sd[a-z][a-z][a-z] */
	char volume[32];		/* Volume name */
	int size;			/* Size, in MB */
	char port[5];			/* Port, CLXX */
	unsigned char cu,ldev;		/* cu & ldev */
	char type[17];			/* OPEN-V[*#] */
	int serial;			/* Serial # */
	unsigned long long id;		/* 64-bit device ID */
};

#include "utils.h"

list get_xplist(void);

#endif
