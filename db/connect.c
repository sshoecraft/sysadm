
#include "db.h"

DB *newdb(void) {
	DB *db;

	db = malloc(sizeof(*db));
	if (!db) {
		perror("newdb");
		return 0;
	}
	memset(db,0,sizeof(*db));

	return db;
}

int db_connect(DB *db, char *dsn, char *user, char *pass) {
	SQLRETURN ret;

	/* Alloc env */
	ret = SQLAllocEnv(&db->henv);
	if (ret != SQL_SUCCESS) {
		dberr(0,0,"db_connect - SQLAllocEnv");
		return 1;
	}

	/* Alloc Connect */
	ret = SQLAllocConnect(db->henv, &db->hdbc);
	dprintf("SQLAllocConnect ret: %d\n", ret);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_ENV,db->henv,"db_connect - SQLAllocConnect");
		return 1;
	}

	/* Connect */
	ret = SQLConnect(db->hdbc, (SQLCHAR *) dsn, SQL_NTS, (SQLCHAR *) user, SQL_NTS, (SQLCHAR *) pass, SQL_NTS);
	dprintf("SQLConnect ret: %d\n", ret);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		dberr(SQL_HANDLE_DBC,db->hdbc,"db_connect - SQLConnect");
		return 1;
	}

	/* Alloc stmt */
	ret = SQLAllocStmt(db->hdbc, &db->hstmt);
	dprintf("SQLAllocStmt: ret: %d\n", ret);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_DBC,db->hdbc,"db_connect - SQLAllocStmt");
		return 0;
	}

	return 0;
}

void db_disconnect(DB *db) {
        if (db->hstmt) SQLFreeStmt(db->hstmt, SQL_DROP);
        if (db->hdbc) {
		SQLDisconnect(db->hdbc);
		SQLFreeConnect(db->hdbc);
	}
        if (db->henv) SQLFreeEnv(db->henv);
}
