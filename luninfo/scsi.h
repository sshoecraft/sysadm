
#ifndef _COMMON_H
#define _COMMON_H 1

int isready(filehandle_t);
int inquiry(filehandle_t, unsigned char *, int *);
int std_inquiry(filehandle_t, unsigned char *, int *);
int identify(filehandle_t,int *,char *,char *);
int capacity(filehandle_t,int *);
int get_lunid(filehandle_t, int *);
int get_wwn(filehandle_t, char *);

#endif
