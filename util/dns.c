
#ifndef __MINGW32__
#include <netinet/in.h>
#include <netdb.h>
#else
#include <windows.h>
#include <winsock.h>
#endif
#include "util.h"

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#ifndef dprintf
#ifdef DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif
#endif

struct hostent *getdnshostent(char *name) {
	struct hostent *he;
	int retries;

	retries = 3;
again:
        he = gethostbyname(name);
        if (he) return he;
	switch(h_errno) {
	case HOST_NOT_FOUND:
		dprintf("HOST_NOT_FOUND\n");
		break;
	case NO_DATA:
		dprintf("NO_DATA\n");
		break;
	case NO_RECOVERY:
		dprintf("NO_RECOVERY\n");
		break;
	case TRY_AGAIN:
		if (retries--) goto again;
		break;
        }
	return 0;
}

int name2addr(char *addr,char *name) {
	struct hostent *he;
	unsigned char *ptr;

	he = getdnshostent(name);
	if (!he) return 1;
	ptr = (unsigned char *) he->h_addr;
	sprintf((char *)addr,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
	return 0;
}
