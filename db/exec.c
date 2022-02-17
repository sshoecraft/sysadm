
#include "db.h"

int _db_exec(HSTMT hstmt, char *query) {
	SQLRETURN ret;

	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(hstmt, (SQLCHAR *) query, SQL_NTS);
	dprintf("ret: %d\n", ret);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	dprintf("calling err\n");
	ret = dberr(SQL_HANDLE_STMT,hstmt,"_db_exec");
	if (_db_disperr) printf("Offending query: %s\n", query);
	dprintf("ret: %d\n", ret);
	return ret;
}
