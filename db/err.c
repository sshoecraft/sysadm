
#include "db.h"

int _db_disperr = 1;

int dberr(SQLSMALLINT HandleType, SQLHANDLE Handle, char *from) {
	SQLCHAR SQLState[6], MessageText[SQL_MAX_MESSAGE_LENGTH];
	SQLSMALLINT RecNumber, BufferLength, TextLength;
	SQLINTEGER NativeError;
	SQLRETURN ret;

	if (_db_disperr && from) printf("dberr: called from: %s\n", from);
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
