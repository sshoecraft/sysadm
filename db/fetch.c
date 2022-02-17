
#include "db.h"

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
