
//#undef ALLREADY_HAVE_WINDOWS_TYPE
#include "db.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int _db_disperr = 1;

int dberr(SQLSMALLINT HandleType, SQLHANDLE Handle, char *from) {
	SQLCHAR SQLState[6], MessageText[SQL_MAX_MESSAGE_LENGTH];
	SQLSMALLINT RecNumber, BufferLength, TextLength;
	SQLINTEGER NativeError;
	SQLRETURN ret;

	if (from) printf("dberr: called from: %s\n", from);
//	printf("dberr: HandleType: %d, Handle: %p\n", HandleType, Handle);
	if (!Handle) return -1;

	BufferLength = SQL_MAX_MESSAGE_LENGTH;
	RecNumber = 1;
	while(1) {
		ret = SQLGetDiagRec(HandleType, Handle, RecNumber, SQLState, &NativeError, MessageText, BufferLength, &TextLength);
		if (_db_disperr) printf("RecNumber: %d, ret: %d\n", RecNumber, ret);
		if (ret != SQL_SUCCESS) break;
//		if (_db_disperr) printf("TextLength: %d\n", TextLength);
		MessageText[TextLength] = 0;
		if (_db_disperr && TextLength) printf("SQLError(%d): %s\n", (int)NativeError, MessageText);
		RecNumber++;
	}

	return ret;
}

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

int _db_exec(HSTMT hstmt, char *query) {
	SQLRETURN ret;

	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(hstmt, (SQLCHAR *) query, SQL_NTS);
	dprintf("ret: %d\n", ret);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	printf("calling err\n");
	ret = dberr(SQL_HANDLE_STMT,hstmt,"_db_exec");
	if (_db_disperr) printf("Offending query: %s\n", query);
	printf("ret: %d\n", ret);
	return ret;
}

int _db_fetch(HSTMT hstmt) {
	SQLRETURN ret;

	dprintf("Fetching data...\n");
	ret = SQLFetch(hstmt);
	dprintf("ret: %d\n", ret);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	if (ret != 100) ret = dberr(SQL_HANDLE_STMT,hstmt,"_db_fetch");
	return ret;
}

int _db_fetch_done(HSTMT hstmt) {
	SQLRETURN ret;

	ret = SQLCloseCursor(hstmt);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,hstmt,"_db_fetch_done");	
	return ret;
}
