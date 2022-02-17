
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#ifdef __MINGW32__
#include "winsock.h"
#else
#include <arpa/telnet.h>
#endif
#include "util.h"

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)

static char fd_buf[4096];
static char *sptr = fd_buf, *eptr = fd_buf;

/* Printf to a socket */
int fd_printf(int fd,char *fmt,...) {
	char temp[1024];
	va_list ap;
	unsigned short len;
	int bytes;

	/* Build the string */
	va_start(ap,fmt);
	vsprintf(temp,fmt,ap);
	va_end(ap);

	/* Write the data */
	len = strlen(temp);
	if (temp[len-1] == '\n')
		dprintf("temp: %s",temp);
	else
		dprintf("temp: %s\n",temp);
	bytes = write(fd,temp,len);
	dprintf("bytes: %d\n", bytes);
	if (bytes != len)
		return 1;
	else
		return 0;
}

static int _refresh(int fd, int wait) {
	int bytes;

	dprintf("wait: %d\n", wait);
	if (wait) {
		struct timeval tv;
		fd_set fds;
		int r;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		/* Setup timeval */
		memset(&tv,0,sizeof(tv));
		tv.tv_sec = wait;

		r = select(fd+1, &fds, NULL, NULL, &tv);
		dprintf("r: %d\n", r);
		if (!r) return 1;
	}

	dprintf("reading...\n");
	if ((bytes = read(fd,fd_buf,sizeof(fd_buf))) < 1) {
		dprintf("errno: %d\n", errno);
//		perror("read");
		return 1;
	}
	dprintf("bytes: %d\n",bytes);

	sptr = fd_buf;
	eptr = fd_buf + bytes;
	return 0;
}

int fd_gets(int fd, char *buffer, unsigned short buflen) {
	int done;
	register char ch;
	register int i;

	dprintf("buflen: %d\n",buflen);
	done = i = 0;
	while(done == 0) {
//		dprintf("sptr: %p, eptr: %p\n", sptr,eptr);
		while(sptr < eptr) {
			ch = *sptr++;
			dprintf("ch: %x\n", ch);
			buffer[i++] = ch;
			if (ch == '\n' || i == buflen) {
				done = 1;
				break;
			}
		}
		if (done == 0 && _refresh(fd,0)) return 0;
	}
	buffer[i] = 0;
	dprintf("i: %d\n", i);
	return i;
}

#ifndef IAC
#define IAC     255             /* interpret as command: */
#define DONT    254             /* you are not to use option */
#define DO      253             /* please, you use option */
#define WONT    252             /* I won't use option */
#define WILL    251             /* I will use option */
#define SB      250             /* interpret as subnegotiation */
#define GA      249             /* you may reverse the line */
#define EL      248             /* erase the current line */
#define EC      247             /* erase the current character */
#define AYT     246             /* are you there */
#define AO      245             /* abort output--but let prog finish */
#define IP      244             /* interrupt process--permanently */
#define BREAK   243             /* break */
#define DM      242             /* data mark--for connect. cleaning */
#define NOP     241             /* nop */
#define SE      240             /* end sub negotiation */
#define EOR     239             /* end of record (transparent mode) */
#define ABORT   238             /* Abort process */
#define SUSP    237             /* Suspend process */
#define xEOF    236             /* End of file: EOF is already used... */
#endif

enum NEED_STATES { NEED_IAC,NEED_CMD,NEED_OPT };

static char *state2str(int state) {
	switch(state) {
	case NEED_IAC:
		return "NEED_IAC";
	case NEED_CMD:
		return "NEED_CMD";
	case NEED_OPT:
		return "NEED_OPT";
	}
	return "unknown";
}

static char *cmd2str(int cmd) {
	switch(cmd) {
	case WILL:
		return "WILL";
	case WONT:
		return "WONT";
	case DO:
		return "DO";
	case DONT:
		return "DONT";
	}
	return "unknown";
}

static char *opt2str(int opt) {
	switch(opt) {
	case 1:
		return "echo";
	case 3:
		return "suppress go ahead";
	case 5:
		return "status";
	case 6:
		return "timing mark";
	case 24:
		return "terminal type";
	case 31:
		return "window size";
	case 32:
		return "terminal speed";
	case 33:
		return "remote flow control";
	case 34:
		return "linemode";
	case 36:
		return "environment variables";
	}
	return "unknown";
}

static int supported(unsigned char ch) {
	switch(ch) {
	case 1:
	case 3:
		return 1;
	}
	return 0;
}

static int respond(int fd, int cmd, int opt) {
	unsigned char buf[4];

	dprintf("%s %s\n", cmd2str(cmd), opt2str(opt));
	sprintf((char *)buf,"%c%c%c",IAC,cmd,opt);
	return (write(fd,buf,3) != 3);
}

int fd_telgets(int fd,char *buffer,unsigned short buflen,int wait) {
	unsigned char ch,prev;
	int state,cmd,done,r;
	register int i;

	dprintf("buflen: %d\n", buflen);

	/* Sift through the data for a line */
	done = i = cmd = r = 0;
	state =  NEED_IAC;
	while(done == 0) {
//		dprintf("sptr: %p, eptr: %p\n", sptr,eptr);
		prev = 0;
		while(sptr < eptr) {
			ch = *sptr++;
			dprintf("ch: %x\n", ch);
			dprintf("state: %s\n", state2str(state));
			switch(state) {
			case NEED_IAC:
				if (ch == IAC) state = NEED_CMD;
				else if (ch == 0) done = 1;
				else if (ch == '\n') {
					buffer[i++] = ch;
					done = 1;
				}
				else if (ch != '\r') {
					buffer[i++] = ch;
					if (i == buflen) done = 1;
				}
				break;
			case NEED_CMD:
				cmd = ch;
				state = NEED_OPT;
				break;
			case NEED_OPT:
				dprintf("opt: %s\n", opt2str(ch));
				switch(cmd) {
				case WILL:
					if (supported(ch))
						respond(fd,DO,ch);
					else
						respond(fd,DONT,ch);
					break;
				case DO:
					if (supported(ch))
						respond(fd,WILL,ch);
					else
						respond(fd,WONT,ch);
					break;
				case WONT:
					respond(fd,DONT,ch);
					break;
				case DONT:
					respond(fd,WONT,ch);
					break;
				}
				state = NEED_IAC;
				break;
			default:
				done = 1;
				break;
			}
			dprintf("done: %d, i: %d\n", done, i);
			if (done) break;
			prev = ch;
		}
		if (wait == 0 && i != 0) done = 1;
		if (done == 0) {
			if (_refresh(fd,wait)) {
				buffer[i] = 0;
				dprintf("i: %d\n", i);
				r = (i ? 1 : 0);
				dprintf("returning %d\n", r);
				return r;
			}
		}
	}
	buffer[i] = 0;
	return 1;
}
