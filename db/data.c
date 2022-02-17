
#include "db.h"

int _db_get_data(DB *db, int field, int type, void *dest, int dest_size, long int *return_len) {
	SQLRETURN ret;

	ret = SQLGetData(db->hstmt, field, type, dest, dest_size, return_len);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"db_get_data");
	return ret;
}
