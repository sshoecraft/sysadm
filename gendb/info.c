
#include "gendb.h"
#include <ctype.h>

list get_table_list(DB *db) {
	HSTMT hstmt;
	SQLRETURN ret;
	list tables;
	char qual[128],owner[128],name[128],type[128],comment[256];
	char entry[256];
	SQLLEN qual_ind, owner_ind, name_ind, type_ind, comment_ind;
 
	tables = list_create();
	ret = SQLAllocStmt(db->hdbc, &hstmt);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_DBC,db->hdbc,"get_table_list");
		return 0;
	}
//	SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,(SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
	ret = SQLTables(hstmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
	if (ret != SQL_SUCCESS) {
		ret = dberr(SQL_HANDLE_STMT, hstmt,"get_table_list - SQLTables");
		return 0;
	}

	SQLBindCol(hstmt, 1, SQL_C_CHAR, (SQLPOINTER) qual, sizeof(qual), &qual_ind);
	SQLBindCol(hstmt, 2, SQL_C_CHAR, (SQLPOINTER) owner, sizeof(owner), &owner_ind);
	SQLBindCol(hstmt, 3, SQL_C_CHAR, (SQLPOINTER) name, sizeof(name), &name_ind);
	SQLBindCol(hstmt, 4, SQL_C_CHAR, (SQLPOINTER) type, sizeof(name), &type_ind);
	SQLBindCol(hstmt, 5, SQL_C_CHAR, (SQLPOINTER) comment, sizeof(comment), &comment_ind);

	qual_ind = owner_ind = name_ind = type_ind = comment_ind = SQL_NTS;
	while ((ret = SQLFetch(hstmt)) == SQL_SUCCESS) {
		if (comment_ind != 0) comment[0] = 0;
		dprintf("qual: %s, owner: %s, name: %s, type: %s, comment: %s\n",
			qual, owner, name, type, comment);
		if (strlen(owner))
			sprintf(entry,"%s.%s",owner,name);
		else
			sprintf(entry,"%s",name);
		list_add(tables,entry,strlen(entry)+1);
	}
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	return tables;
}

static list get_field_info(HSTMT hstmt, TABLE *table) {
	SQLRETURN ret;
	COLUMN info;
	char name[128], type[128], comment[255],*p;
	unsigned long length;
	short scale,nullable;
	SQLLEN name_ind, dtype_ind, type_ind, comment_ind, length_ind, scale_ind, nullable_ind;
	short dtype;

	dprintf("name: %s\n", table->name);

//	ret = SQLColumns(hstmt, 0,0,(SQLCHAR *) table->schema, SQL_NTS, (SQLCHAR *) table->name, SQL_NTS,0,0);
	ret = SQLColumns(hstmt, 0,0,0, SQL_NTS, (SQLCHAR *) table->name, SQL_NTS,0,0);
	dprintf("SQLColumns ret: %d\n", ret);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_STMT,hstmt,"get_field_info");
		SQLCloseCursor(hstmt);
		return 0;
	}

	SQLBindCol(hstmt, 4, SQL_C_CHAR, (SQLPOINTER) name, sizeof(name), &name_ind);
	SQLBindCol(hstmt, 5, SQL_C_SSHORT, (SQLPOINTER) &dtype, sizeof(dtype), &dtype_ind);
	SQLBindCol(hstmt, 6, SQL_C_CHAR, (SQLPOINTER) type, sizeof(type), &type_ind);
	SQLBindCol(hstmt, 7, SQL_C_ULONG, (SQLPOINTER) &length, sizeof(length), &length_ind);
	SQLBindCol(hstmt, 9, SQL_C_SHORT, (SQLPOINTER) &scale, sizeof(scale), &scale_ind);
	SQLBindCol(hstmt, 11, SQL_C_SHORT, (SQLPOINTER) &nullable, sizeof(nullable), &nullable_ind);
	SQLBindCol(hstmt, 12, SQL_C_CHAR, (SQLPOINTER) comment, sizeof(comment), &comment_ind);

#if 0
#define SQL_DATE                                9
#define SQL_INTERVAL                                                    10
#define SQL_TIME                                10
#define SQL_TIMESTAMP                           11
#define SQL_LONGVARCHAR                         (-1)
#define SQL_BINARY                              (-2)
#define SQL_VARBINARY                           (-3)
#define SQL_LONGVARBINARY                       (-4)
#define SQL_BIGINT                              (-5)
#define SQL_TINYINT                             (-6)
#define SQL_BIT                                 (-7)
#define SQL_GUID                                (-11)
#endif

	name_ind = dtype_ind = type_ind = length_ind = scale_ind = nullable_ind = comment_ind = SQL_NTS;
	while ((ret = SQLFetch(hstmt)) == SQL_SUCCESS) {
//printf("name_ind: %d, dtype_ind: %d, type_ind: %d, length_ind: %d, scale_ind: %d, nullable_ind: %d, comment_ind: %d\n", name_ind, dtype_ind, type_ind, length_ind, scale_ind, nullable_ind, comment_ind);
		if (comment_ind != 0) comment[0] = 0;
		dprintf("column: name: %s, type: %s, dtype: %d, len: %ld, scale: %d, comment: %s\n",
			name,type,dtype,length,scale,comment);
		memset(&info, 0, sizeof(info));
		strcpy(info.name,name);
		for(p = info.name; *p; p++) {
			if (isspace(*p)) *p = '_';
			else *p = tolower(*p);
		}
		info.len = (length > 16383 ? 16383 : length);
		info.type = 9999;
		strcpy(info.type_name,type);
		strcpy(info.comment,comment);
		dprintf("dtype: %d\n", dtype);
		switch(dtype) {
		case -1: /* long varchar / text */
		case -9: /* nvarchar */
		case -10: /* ntext */
		case 12: /* varchar */
			info.type = FT_STRING;
			break;
		case 1: /* char */
			info.type = FT_CHAR;
			break;
		case 9: /* date */
		case 10: /* time */
		case 11: /* timestamp */
			info.type = FT_DATE;
			break;
		case -2: /* binary */
		case -3: /* varbinary */
		case -4: /* long varbinary / blob */
		case -11: /* GUID */
			info.type = FT_BIN;
			break;
		case -5: /* bigint */
			info.type = FT_QUAD;
			break;
		case -6: /* tinyint */
		case -7: /* bit */
			info.type = FT_CHAR;
			info.len = 1;
			break;
		case -8: /* unknown */
		case 2: /* unknown */
		case 6: /* unknown */
			info.type = FT_UNK;
			break;
		case 3: /* decimal */
		case 7: /* float */
			info.type = FT_FLOAT;
			break;
		case 4: /* integer */
			info.type = FT_INT;
			break;
		case 5: /* smallint */
			info.type = FT_SHORT;
			break;
		case 8: /* double */
			info.type = FT_DOUBLE;
			break;
		default:
			break;
		}

		switch(dtype) {
		case -1: /* long varchar / text */
		case -9: /* nvarchar */
		case 1: /* char */
		case 9: /* date */
		case 10: /* time */
		case 11: /* timestamp */
		case 12: /* varchar */
			info.ctype = "char";
			info.ptr = 1;
			info.pfmt = "%s";
			info.dtype = "SQL_C_CHAR";
			info.len++;
			break;
		case -10: /* ntext */
			info.ctype = "char";
			info.ptr = 1;
			info.pfmt = "%s";
			info.dtype = "SQL_C_CHAR";
			info.len++;
			break;
		case -2: /* binary / timestamp on MSSQL */
			info.ctype = "unsigned long long";
			info.pfmt = "%lld";
			info.dtype = "SQL_C_TYPE_TIMESTAMP";
			break;
		case -3: /* varbinary */
		case -4: /* long varbinary / blob */
		case -11: /* GUID */
			info.ctype = "unsigned char";
			info.ptr = 1;
			info.pfmt = "%x";
			info.dtype = "SQL_C_BINARY";
			break;
		case -5: /* bigint */
			info.ctype = "long long";
			info.pfmt = "%lld";
			info.dtype = "SQL_C_BIGINT";
			break;
		case -6: /* tinyint */
			info.ctype = "unsigned char";
			info.pfmt = "%d";
			info.dtype = "SQL_C_TINYINT";
			break;
		case -7: /* bit */
			info.ctype = "unsigned char";
			info.pfmt = "%d";
			info.dtype = "SQL_C_BIT";
			break;
		case -8: /* unknown */
		case 2: /* unknown */
		case 6: /* unknown */
			info.ctype = "void";
			info.pfmt = "%x";
			info.dtype = "SQL_UNKNOWN";
			break;
			break;
		case 3: /* decimal */
		case 7: /* float */
			info.ctype = "float";
			info.pfmt = "%f";
			info.dtype = "SQL_C_FLOAT";
			break;
		case 4: /* integer */
//			info.ctype = "unsigned long";
//			info.pfmt = "%ld";
			info.ctype = "int";
			info.pfmt = "%d";
			info.dtype = "SQL_C_SLONG";
			break;
		case 5: /* smallint */
			info.ctype = "short";
			info.pfmt = "%d";
			info.dtype = "SQL_C_SSHORT";
			break;
		case 8: /* double */
			info.ctype = "double";
			info.pfmt = "%f";
			info.dtype = "SQL_C_DOUBLE";
			break;
		default:
			dprintf("unknown type: %d\n", dtype);
			info.ctype = "char";
			info.ptr = 1;
			info.pfmt = "%s";
			info.dtype = "SQL_C_CHAR";
			info.len++;
			break;
		}

		/* Add this field to the list */
//		dprintf("adding: name: %s, type: %s, length: %d\n", info.name, typestr(0,info.type), info.len);
		list_add(table->fields,&info,COLUMN_SIZE);
		name_ind = dtype_ind = type_ind = length_ind = scale_ind = nullable_ind = comment_ind = SQL_NTS;
	}
	SQLCloseCursor(hstmt);

	dprintf("done\n");
	return 0;
}

#if 0
// XXX never got this to work 
static list get_table_keys(HSTMT hstmt, TABLE *table) {
	char key_table[128],key_name[128];
	COLUMN *field;
	SQLRETURN ret;
	SQLSMALLINT key_seq;
	SQLINTEGER key_table_ind, key_name_ind, key_seq_ind;

	dprintf("table: %s.%s\n", table->schema, table->name);

	ret = SQLPrimaryKeys(hstmt, 0, 0, (SQLCHAR *) table->schema, SQL_NTS, (SQLCHAR *) table->name, SQL_NTS);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_STMT,hstmt,"get_table_keys");
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

	table->keys = list_create();

	SQLBindCol(hstmt, 3, SQL_C_CHAR, key_table, SQL_NTS, &key_table_ind);
	SQLBindCol(hstmt, 4, SQL_C_CHAR, key_name, SQL_NTS, &key_name_ind);
	SQLBindCol(hstmt, 6, SQL_C_SSHORT, &key_seq, sizeof(key_seq), &key_seq_ind);
	while ((ret = SQLFetch(hstmt)) == SQL_SUCCESS) {
		dprintf("key_table: %s, ind: %d\n", key_table, key_table_ind);
		dprintf("key_name: %s, ind: %d\n", key_name, key_name_ind);
		dprintf("key_seq: %d, ind: %d\n", key_seq, key_seq_ind);
		field = get_column(table,key_name);
		if (field) list_add(table->keys, field, sizeof(*field));
	}
	SQLCloseCursor(hstmt);

	return 0;
}
#endif

TABLE *get_table_info(DB *db, char *schema, char *table_name, char *keys) {
	TABLE *table;
	HSTMT hstmt;
	SQLRETURN ret;
	COLUMN *field;
	char *p;
	int i;

	dprintf("schema: %s, table_name: %s, keys: %s\n", schema, table_name, keys);

	ret = SQLAllocStmt(db->hdbc, &hstmt);
	if (ret != SQL_SUCCESS) {
		dberr(SQL_HANDLE_DBC,db->hdbc,"get_table_info");
		return 0;
	}
//	SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);

	/* Init the structure */
	table = malloc(sizeof(*table));
	if (!table) return 0;
	memset(table,0,sizeof(*table));
	table->fields = list_create();
	table->keys = list_create();

	/* Add in the schema and name */
	strcpy(table->schema,stredit(schema,"LOWERCASE"));
	strcpy(table->name,stredit(table_name,"LOWERCASE"));

	/* Get the field info */
	if (get_field_info(hstmt,table)) return 0;

	/* Get the key info */
//	get_table_keys(hstmt,table);
	i = 0;
	while(1) {
		p = strele(i++,",",keys);
		if (!strlen(p)) break;
		if (strchr(p,'+')) {
			char compkey[128];
			int j;

			strcpy(compkey,p);
			for(j=0; j < 99; j++) {
				p = strele(j,"+",compkey);
				if (!strlen(p)) break;
			}
		} else {
			field = get_column(table,p);
			if (!field) {
				printf("error: key column not found: %s\n", p);
				continue;
			}
			dprintf("adding key: %s\n", p);
			list_add(table->keys,field,COLUMN_SIZE);
		}
	}

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	return(table);
}
