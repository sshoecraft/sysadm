
#include "gendb.h"

#include "dberr.c"

#if 0
char *typestr(int type) {
	switch(type) {
	case FT_UNK:
		return "unknown";
	case FT_CHAR:
		return "char";
	case FT_STRING:
		return "string";
	case FT_SHORT:
		return "short";
	case FT_LONG:
		return "long";
	case FT_QUAD:
		return "long long";
	case FT_FLOAT:
		return "float";
	case FT_DOUBLE:
		return "double";
	case FT_LONGDBL:
		return "long double";
	case FT_DATE:
		return "date";
	default:
		return "invalid";
	}
}
#endif

DB *db_connect(char *name, char *user, char *pass) {
	DB *db;
	HENV henv;
	HDBC hdbc;
	SQLRETURN ret;

	printf("Connecting to db...\n");
	ret = SQLAllocEnv(&henv);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_ENV,henv);
		return 0;
	}

	ret = SQLAllocConnect(henv, &hdbc);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_ENV,henv);
		return 0;
	}

	ret = SQLConnect(hdbc, (SQLCHAR *) name, SQL_NTS, (SQLCHAR *) user, SQL_NTS, (SQLCHAR *) pass, SQL_NTS);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		dberr(SQL_HANDLE_DBC,hdbc);
		return 0;
	}
//	SQLSetConnectOption(hdbc, SQL_AUTOCOMMIT_ON);

	db = malloc(sizeof(*db));
	db->henv = henv;
	db->hdbc = hdbc;

	return db;
}

int db_disconnect(DB *db) {
	SQLDisconnect(db->hdbc);
	SQLFreeConnect(db->hdbc);
	SQLFreeEnv(db->henv);
	return 0;
}

list get_table_list(DB *db) {
	HSTMT hstmt;
	SQLRETURN ret;
	list tables;

	tables = list_create();
	ret = SQLAllocStmt(db->hdbc, &hstmt);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_DBC,db->hdbc);
		return 0;
	}
//	SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,(SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
	ret = SQLTables(hstmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
	if (ret != SQL_SUCCESS) dberr(SQL_HANDLE_STMT, hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	return tables;
}

static list get_field_info(HSTMT hstmt, char *table) {
	SQLRETURN ret;
	COLUMN info;
	list fields;
	char name[128], type[128], comment[128];
	int length;
	short scale,nullable;
	SQLLEN name_ind, type_ind, comment_ind, length_ind, scale_ind, nullable_ind;

	dprintf("table: %s\n", table);

	ret = SQLColumns(hstmt, 0,0,0,0,(SQLCHAR *) table, SQL_NTS,0,0);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_STMT,hstmt);
		SQLCloseCursor(hstmt);
		return 0;
	}
	dprintf("ret: %d\n", ret);

	SQLBindCol(hstmt, 4, SQL_C_CHAR, (SQLPOINTER) name, sizeof(name), &name_ind);
	SQLBindCol(hstmt, 6, SQL_C_CHAR, (SQLPOINTER) type, sizeof(type), &type_ind);
	SQLBindCol(hstmt, 7, SQL_C_SLONG, (SQLPOINTER) &length, sizeof(length), &length_ind);
	SQLBindCol(hstmt, 9, SQL_C_SHORT, (SQLPOINTER) &scale, sizeof(scale), &scale_ind);
	SQLBindCol(hstmt, 11, SQL_C_SHORT, (SQLPOINTER) &nullable, sizeof(nullable), &nullable_ind);
	SQLBindCol(hstmt, 12, SQL_C_CHAR, (SQLPOINTER) comment, sizeof(comment), &comment_ind);

	fields = list_create();
	while ((ret = SQLFetch(hstmt)) == SQL_SUCCESS) {
		dprintf("column: name: %s, type: %s, len: %d, scale: %d\n", name,type,length,scale);
		/* Fill in the info structure */
		memset(&info, 0, sizeof(info));
		strcpy(info.name,stredit(name,"LOWERCASE"));
		strcpy(type,stredit(type,"LOWERCASE"));
		if (strcmp(type,"char")==0) {
			info.type = FT_CHAR;
			info.len = length;
			if (info.len > 1) info.len++;
		} else if (strcmp(type,"varchar")==0 || strcmp(type,"nvarchar") == 0) {
			info.type = FT_STRING;
			info.len = length;
//			if (info.len > 1) info.len++;
		} else if (strcmp(type,"varchar2")==0) {
			info.type = FT_STRING;
			info.len = length + 1;
		} else if (strcmp(type,"short") == 0) {
			info.type = FT_SHORT;
		} else if (strcmp(type,"integer") == 0 || strcmp(type,"int") == 0) {
			if (length < 5)
				info.type = FT_SHORT;
			else if (length < 10)
				info.type = FT_LONG;
			else
				info.type = FT_QUAD;
		} else if (strcmp(type,"long") == 0) {
			info.type = FT_LONG;
		} else if (strcmp(type,"float") == 0 || strcmp(type,"decimal") == 0) {
			info.type = FT_FLOAT;
		} else if (strcmp(type,"date") == 0 || strcmp(type,"datetime") == 0 || strcmp(type,"timestamp") == 0) {
			info.type = FT_DATE;
			info.len = length;
#if 0
		} else if (strcmp(type,"number")==0) {
//			size = length;
//			size -= precision - scale;
			if (scale == 0) {
				if (size < 5)
					info.type = FT_SHORT;
				else if (size < 10)
					info.type = FT_LONG;
				else
					info.type = FT_DOUBLE;
			}
			else {
				if (size < 8)
					info.type = FT_FLOAT;
				else if (size < 16)
					info.type = FT_DOUBLE;
				else
					info.type = FT_LONGDBL;
			}
#endif
		}

//		strcpy(info.comment,comment);

		/* Add this field to the list */
//		dprintf("adding: name: %s, type: %s, length: %d\n", info.name, typestr(0,info.type), info.len);
		list_add(fields,&info,COLUMN_SIZE);
	}
	SQLCloseCursor(hstmt);

	/* Close the cursor */
	dprintf("done\n");
	return fields;
}

static list get_table_keys(HSTMT hstmt, TABLE *tp) {
	char name[128];
	COLUMN *field;
	list keys;
	SQLRETURN ret;
	SQLLEN name_ind;

	dprintf("table: %s\n", tp->name);

	ret = SQLPrimaryKeys(hstmt, 0, SQL_NTS, 0, 0, (SQLCHAR *) tp->name, SQL_NTS);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_STMT,hstmt);
		return 0;
	}
	dprintf("ret: %d\n", ret);

/*
1 TABLE_CAT VARCHAR(128) This is always null. 
2 TABLE_SCHEM VARCHAR(128) The name of the schema containing TABLE_NAME. 
3 TABLE_NAME VARCHAR(128) NOT NULL Name of the specified table. 
4 COLUMN_NAME VARCHAR(128) NOT NULL Primary key column name. 
5 KEY_SEQ SMALLINT NOT NULL Column sequence number in the primary key, starting with 1. 
6 PK_NAME VARCHAR(128) Primary key identifier. Contains a null value if not applicable to the data source. 
*/

	keys = list_create();

	SQLBindCol(hstmt, 4, SQL_C_CHAR, (SQLPOINTER) name, sizeof(name), &name_ind);
	while ((ret = SQLFetch(hstmt)) == SQL_SUCCESS) {
		dprintf("key: name: %s\n", name);
		/* Find the key in the field list */
		list_reset(tp->fields);
		while((field = list_get_next(tp->fields)) != 0) {
			if (strcmp(field->name,name) == 0) {
				list_add(keys, field, sizeof(*field));
				break;
			}
		}
	}
	SQLCloseCursor(hstmt);

	return keys;
}

TABLE *get_table_info(DB *db,char *table) {
	static TABLE info;
	HSTMT hstmt;
	SQLRETURN ret;

	dprintf("table: %s\n", table);

	ret = SQLAllocStmt(db->hdbc, &hstmt);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_DBC,db->hdbc);
		return 0;
	}
//	SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);

	/* Init the structure */
	memset(&info,0,sizeof(info));

	/* Add in the owner and name */
//	strcpy(info.owner,stredit(owner,"LOWERCASE"));
	strcpy(info.name,stredit(table,"LOWERCASE"));

	/* Get the field info */
	info.fields = get_field_info(hstmt,table);
	if (!info.fields) return 0;

	/* Get the primary keys */
	info.keys = get_table_keys(hstmt,&info);

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	return(&info);
}
