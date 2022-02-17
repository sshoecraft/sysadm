
#ifndef __SOCKET_H
#define __SOCKET_H

#ifdef _WIN32
#define socket_t SOCKET
#define socket_error(sock) (sock == INVALID_SOCKET)
#define socket_close(sock) closesocket(sock);
#else
#define socket_t int
#define INVALID_SOCKET -1
#define socket_error(sock) (sock < 0)
#define socket_close(sock) close(sock)
#endif

int socket_init(void);
socket_t socket_open(int);
socket_t socket_get_inetd(void);
int socket_reuse(socket_t s);
int socket_accept(socket_t s);
int socket_write(socket_t s,void *buffer,int bufsize);
int socket_printf(socket_t s,char *fmt,...);
int socket_recv(socket_t s,void *buffer,unsigned short bufsize);
int socket_gets(socket_t s, char *buffer, int buflen);
int socket_reserve(socket_t s,unsigned short port,int hear);
int socket_connect(socket_t s,char *addr, int port, int timeout);

#endif
