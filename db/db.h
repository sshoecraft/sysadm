#ifndef __DB_H
#define __DB_H

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#ifdef __MINGW32__
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif
#include "sqlext.h"

struct _dbconxinfo {
	HENV henv;
	HDBC hdbc;
	HSTMT hstmt;
};
typedef struct _dbconxinfo DB;

DB *newdb(void);
int dberr(SQLSMALLINT, SQLHANDLE, char *);
int db_connect(DB *, char *,char *,char *);
void db_disconnect(DB *);
int _db_exec(HSTMT, char *);
#define db_exec(db,query) _db_exec((db)->hstmt,query)
int _db_fetch(HSTMT);
#define db_fetch(db) _db_fetch((db)->hstmt)
int _db_fetch_done(HSTMT);
#define db_fetch_done(db) _db_fetch_done((db)->hstmt)
int _db_get_data(DB *db, int field, int type, void *dest, int dest_size, long int *return_len);
#define db_get_data(db,f,t,d,s,l) _db_get_data((db)->hstmt,f,t,d,s,l)

extern int _db_disperr;

#ifndef dprintf
#ifdef DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif
#endif

#endif
