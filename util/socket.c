
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include "windows.h"
#include "winsock2.h"
#include <ws2tcpip.h>
#include <errno.h>
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/errno.h>
#endif
#include <ctype.h>
#include "socket.h"
#include "util.h"

#if DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif

#define DEFAULT_PORT 6996

#ifdef _WIN32
static int wsa_init = 0;
#define BASE_PATH "C:\\Windows\\"
#else
#define BASE_PATH "/usr/local/bin/"
#endif

int socket_init(void) {
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	if (!wsa_init) {
		dprintf("calling WSAStartup...\n");
		wVersionRequested = MAKEWORD(2, 2);
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0) {
			printf("WSAStartup failed with error: %d\n", err);
			return INVALID_SOCKET;
		}
		wsa_init = 1;
	}
#endif
	return 0;
}

socket_t socket_open(int type) {
	int s,socket_type,proto_type;

	/* Set the type */
	if (type == 0) {
		socket_type = SOCK_STREAM;
		proto_type = IPPROTO_TCP;
	} else {
		socket_type = SOCK_DGRAM;
		proto_type = IPPROTO_UDP;
	}

	dprintf("opening socket...\n");
	s = socket(AF_INET, socket_type, proto_type);
	if (socket_error(s)) {
		perror("socket");
		return INVALID_SOCKET;
	}

	return s;
}

#ifdef _WIN32
static socket_t get_socket_handle(void) {
	HANDLE hSocket = GetStdHandle(STD_INPUT_HANDLE);

	if (hSocket == INVALID_HANDLE_VALUE) {
		char msg[2048];

		sprintf(msg,"GetStdHandle: %s", (char *)GetLastError());
		printf("%s\n",msg);
	}
	return ((socket_t)hSocket);
}
#endif

socket_t socket_get_inetd(void) {
        socket_t s;

#ifdef _WIN32
        s = get_socket_handle();
#else
        s = 0;
#endif
	return s;
}

int socket_reuse(socket_t s) {
	int arg = 1, err;

	err = (setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *)&arg, sizeof(arg)) < 0 ? 1 : 0);
	if (err) {
		perror("setsockopt: SO_REUSEADDR");
		return 1;
	}
	return 0;
}

int socket_accept(socket_t s) {
	int c;
	struct sockaddr_in sin;
	socklen_t sin_size = sizeof(sin);

	c = accept(s,(struct sockaddr *)&sin,&sin_size);
	if (socket_error(c)) {
		perror("accept");
		return INVALID_SOCKET;
	}
	return c;
}

int socket_write(socket_t s,void *buffer,int bufsize) {
	register char *ptr = (char *) buffer;
	int count,total,bytes_left,bytes_to_send;

	dprintf("bufsize: %d\n", bufsize);

	total = 0;
	bytes_left = bufsize;
	while(bytes_left) {
		bytes_to_send = (bytes_left > 65534 ? 65534 : bytes_left);
		count = send(s,ptr,bytes_to_send,0);
		dprintf("count: %d\n", count);
		if (count < 0) {
			perror("error sending data");
			return -1;
		} else if (count < 1) {
			if (errno == EAGAIN)
				continue;
			else
				break;
		}
		bytes_left -= count;
		ptr += count;
		total += count;
	}
	dprintf("total: %d\n", total);
	return(total);
}

int socket_printf(socket_t s,char *fmt,...) {
	char temp[1024];
	va_list ap;
	int len;

	/* Build the string */
	va_start(ap,fmt);
	len = vsprintf(temp,fmt,ap);
	va_end(ap);

	/* Write the data */
	dprintf("temp: %s\n",temp);
	if (socket_write(s,temp,len) != len)
		return 1;
	else
		return 0;
}

int socket_recv(socket_t s,void *buffer,unsigned short bufsize) {
        register int bytes;

	do {
		dprintf("calling recv...\n");
		bytes = recv(s,buffer,bufsize,0);
		dprintf("bytes: %d\n", bytes);
		if (bytes < 0) {
			perror("recv");
			return -1;
		}
	} while(bytes < 1 && errno == EAGAIN);

	dprintf("returning: %d\n", bytes);
        return bytes;
}

char buffer[4096], *sptr, *eptr;

static int _refresh(socket_t s) {
	register int bytes;

	/* Read a 'chunk' from the socket */
	if ((bytes = socket_recv(s,buffer,sizeof(buffer))) < 1) return 1;

	dprintf("bytes: %d\n", bytes);

	sptr = buffer;
	eptr = buffer + bytes;
	return 0;
}

int socket_gets(socket_t s, char *buffer, int buflen) {
	int done;
	register char ch;
	register int i;

	dprintf("buflen: %d\n", buflen);
	done = i = 0;
	while(done == 0) {
		dprintf("sptr: %p, eptr: %p\n", sptr, eptr);
		while(sptr < eptr) {
			ch = *sptr++;
//			dprintf("ch: %d\n", ch);
			buffer[i++] = ch;
			if (ch == '\n' || i == buflen) {
				done = 1;
				break;
			}
		}
		if (done == 0) {
			if (_refresh(s))
				return 0;
		}
	}
	buffer[i] = 0;
	dprintf("returning: %d\n", i);
	trim(buffer);
	return i;
}

int socket_reserve(socket_t s,unsigned short port,int hear) {
	struct sockaddr_in sin;
	socklen_t sin_size = sizeof(sin);

	dprintf("port: %d\n", port);

	/* Set up the addr info */
	memset((char *)&sin,0,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = ntohs(port);

	/* Bind the port */
	if (bind(s,(struct sockaddr *)&sin,sin_size) < 0) {
		perror("bind");
		return 1;
	}

	/* Get the bound port */
	if (getsockname(s,(struct sockaddr *)&sin,&sin_size) < 0) {
		perror("getsockname");
		return 1;
	}

	dprintf("port reserved: %d\n", ntohs(sin.sin_port));

	/* Listen on the port? */
	if (hear) {
		dprintf("listening...\n");
		if (listen(s,SOMAXCONN) < 0) {
			perror("listen");
			return 1;
		}
	}

	return 0;
}

int socket_connect(socket_t s, char *addr, int port, int timeout) {
	struct sockaddr_in sin;
	int sin_size = sizeof(sin);
	struct timeval tv;

	dprintf("addr: %s, port: %d, timeout: %d\n", addr, port, timeout);

	if (timeout > 0) {
//		dprintf("setting timeout...\n");
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
			perror("setsockopt");
			/* XXX not an error? */
		}
	}

	/* Set up the addr info */
//	dprintf("setting up addr info...\n");
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr((char *)addr);
	sin.sin_port = htons(port);

	dprintf("connecting...\n");
	if (connect(s, (struct sockaddr *)&sin, sin_size) < 0) {
		perror("connect");
		return 1;
	}

	return 0;
}
