
#ifndef __LUNINFO_XP_H
#define __LUNINFO_XP_H

#include "luninfo.h"

/* Needed for dump */
struct xpinfo {
	char port[5];			/* Port, CLXX */
	unsigned char cu,ldev;		/* cu & ldev */
	int serial;			/* Serial # */
};

int get_xpinfo(filehandle_t,struct luninfo *);

#endif
