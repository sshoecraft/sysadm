
#ifndef __ODBC_H
#define __ODBC_H

#include <sql.h>
#include <sqlext.h>

struct _dbinfo {
	HENV henv;
	HDBC hdbc;
	HSTMT hstmt;
};
typedef struct _dbinfo DB;

DB *db_connect(char *,char *,char *);
int db_disconnect(DB *);

#endif
