
void dberr(SQLSMALLINT HandleType, SQLHANDLE Handle) {
	SQLCHAR SQLState[6], MessageText[SQL_MAX_MESSAGE_LENGTH];
	SQLSMALLINT RecNumber, BufferLength, TextLength;
	SQLINTEGER NativeError;
	SQLRETURN ret;

//	printf("dberr: HandleType: %d, Handle: %p\n", HandleType, Handle);
	if (!Handle) return;

	BufferLength = SQL_MAX_MESSAGE_LENGTH;
	RecNumber = 1;
	while(1) {
		ret = SQLGetDiagRec(HandleType, Handle, RecNumber, SQLState, &NativeError, MessageText, BufferLength, &TextLength);
//		printf("RecNumber: %d, ret: %d\n", RecNumber, ret);
		if (ret != SQL_SUCCESS) break;
//		printf("TextLength: %d\n", TextLength);
		MessageText[TextLength] = 0;
		printf("SQLError(%d): %s\n", (int)NativeError, MessageText);
		RecNumber++;
	}
}
