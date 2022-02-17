
#include "gendb.h"

void write_bind(FILE *fp, TABLE *table, int decl) {
	COLUMN *field;
	int pos;

	fprintf(fp,"int %s_bind_record(DB *db, struct %s_record *rec)", table->name, table->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
#if 0
	fprintf(fp,"\tSQLLEN ");
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		fprintf(fp,"%s_ind",field->name);
		if (list_is_next(table->fields)) fprintf(fp,", ");
	}
	fprintf(fp,";\n");
#endif
	fprintf(fp,"\n");

	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tdprintf(\"Binding...\\n\");\n");
	pos = 1;
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		fprintf(fp,"\tSQLBindCol(db->hstmt, %d, %s, (SQLPOINTER) %srec->%s, sizeof(rec->%s), 0);\n",
			pos++, field->dtype, (field->ptr ? "" : "&"), field->name, field->name);
	}
	fprintf(fp,"#endif\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"}\n");
}

void write_fetch_base(FILE *fp, TABLE *table, int decl) {
	fprintf(fp,"int %s_fetch(DB *db)", table->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tSQLRETURN ret;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tdprintf(\"Fetching data...\\n\");\n");
	fprintf(fp,"\tret = SQLFetch(db->hstmt);\n");
	fprintf(fp,"\tif (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;\n");
	fprintf(fp,"\tif (ret != 100) ret = dberr(SQL_HANDLE_STMT,db->hstmt,\"%s_fetch\");\n",table->name);
	fprintf(fp,"\treturn ret;\n");
	fprintf(fp,"#else\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_fetch(FILE *fp, TABLE *table, int decl) {
	FIELD *field;
	int pos;

	write_fetch_base(fp,table,decl);

	/* Fetch a record */
	fprintf(fp,"int %s_fetch_record(DB *db, struct %s_record *rec)", table->name, table->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tSQLRETURN ret;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tif ((ret = %s_fetch(db)) != 0) {\n", table->name);
	fprintf(fp,"\t\tSQLCloseCursor(db->hstmt);\n");
	fprintf(fp,"\t\treturn ret;\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\tmemset(rec,0,sizeof(*rec));\n");
	pos = 1;
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		fprintf(fp,"\tSQLGetData(db->hstmt, %d, %s, (SQLPOINTER) %srec->%s, sizeof(rec->%s), 0);\n",
			pos++, field->dtype, (field->ptr ? "" : "&"), field->name, field->name);
	}
	fprintf(fp,"\tdprintf(\"----------------- %s record -----------------\\n\");\n", table->name);
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		fprintf(fp,"\tdprintf(\"%-20.20s:  %s\\n\",rec->%s);\n", field->name, field->pfmt, field->name);
	}
	fprintf(fp,"#endif\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_select_base(FILE *fp, TABLE *table, int decl) {
	FIELD *field;
	int i;

	fprintf(fp,"int %s_select(DB *db, char *clause)", table->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tchar query[4096];\n");
	fprintf(fp,"\tSQLRETURN ret;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tsprintf(query,\"SELECT ");
	list_reset(table->fields);
	i = 0;
	while( (field = list_get_next(table->fields)) != 0) {
		if (i++) fprintf(fp,",");
		fprintf(fp,"%s.%s",table->name,field->name);
	}
	fprintf(fp," FROM %s %%s\",(clause ? clause : \"\"));\n", table->name);
	fprintf(fp,"\tdprintf(\"Executing query: %%s\\n\", query);\n");
	fprintf(fp,"\tret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);\n");
	fprintf(fp,"\tif (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;\n");
	fprintf(fp,"\tret = dberr(SQL_HANDLE_STMT,db->hstmt,\"%s_select\");\n",table->name);
	fprintf(fp,"\treturn ret;\n");
	fprintf(fp,"#else\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_select(FILE *fp, TABLE *table, int decl) {
	write_select_base(fp,table,decl);
	fprintf(fp,"int %s_select_record(DB *db, struct %s_record *rec, char *clause)", table->name, table->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tSQLRETURN ret;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tif ((ret = %s_select(db, clause)) != 0) return ret;\n", table->name);
	fprintf(fp,"\tif ((ret = %s_fetch_record(db, rec)) != 0) return ret;\n",table->name);
	fprintf(fp,"\tSQLCloseCursor(db->hstmt);\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_select_by(FILE *fp, TABLE *table, FIELD *field, int decl) {
	if (!field) return;
	fprintf(fp,"int %s_select_by_%s(DB *db, struct %s_record *rec, %s %s%s)",
		table->name, field->name, table->name, field->ctype, (field->ptr ? "*" : ""), field->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tchar temp[%d];\n", ((field->len + 33) / 16) * 16);
	fprintf(fp,"\n");
	fprintf(fp,"\tsprintf(temp,\" WHERE %s = ", field->name);
	if (strchr(field->pfmt,'s'))
		fprintf(fp,"'%s'", field->pfmt);
	else
		fprintf(fp,"%s", field->pfmt);
	fprintf(fp,"\", %s);\n", field->name);
	fprintf(fp,"\treturn %s_select_record(db, rec, temp);\n", table->name);
	fprintf(fp,"#else\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_insert(FILE *fp, TABLE *table, int decl) {
	FIELD *field;
	int i;

	fprintf(fp,"int %s_insert(DB *db, struct %s_record *rec)", table->name, table->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tchar query[4096];\n");
	fprintf(fp,"\tSQLRETURN ret;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tsprintf(query,\"INSERT INTO %s (",table->name);
	i = 0;
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		if (field->noins) continue;
		if (i++) fprintf(fp,",");
		fprintf(fp,"%s",field->name);
	}
	fprintf(fp,") VALUES (");
	i = 0;
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		if (field->noins) continue;
		if (i++) fprintf(fp,",");
		if (strchr(field->pfmt,'s'))
			fprintf(fp,"'%s'",field->pfmt);
		else
			fprintf(fp,"%s",field->pfmt);
	}
	fprintf(fp,")\",");
	i = 0;
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		if (field->noins) continue;
		if (i++) fprintf(fp,",");
		fprintf(fp,"rec->%s",field->name);
	}
	fprintf(fp,");\n");
	fprintf(fp,"\tdprintf(\"Executing query: %%s\\n\", query);\n");
	fprintf(fp,"\tret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);\n");
	fprintf(fp,"\tif (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;\n");
	fprintf(fp,"\tret = dberr(SQL_HANDLE_STMT,db->hstmt,\"%s_insert\");\n",table->name);
	fprintf(fp,"\tprintf(\"Offending query: %%s\\n\", query);\n");
	fprintf(fp,"\treturn ret;\n");
	fprintf(fp,"#else\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_update(FILE *fp, TABLE *table, int decl) {
	FIELD *field;
	int i;

	fprintf(fp,"int %s_update_record(DB *db, struct %s_record *rec, char *clause)", table->name, table->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tchar query[4096];\n");
	fprintf(fp,"\tSQLRETURN ret;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tsprintf(query,\"UPDATE %s SET ",table->name);
	i = 0;
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		if (field->noins || field->noupd) continue;
		if (i++) fprintf(fp,",");
		if (strchr(field->pfmt,'s'))
			fprintf(fp,"%s = '%s'",field->name, field->pfmt);
		else
			fprintf(fp,"%s = %s",field->name,field->pfmt);
	}
	fprintf(fp,"\",");
	i = 0;
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		if (field->noins || field->noupd) continue;
		if (i++) fprintf(fp,",");
		fprintf(fp,"rec->%s",field->name);
	}
	fprintf(fp,");\n");
	fprintf(fp,"\tif (clause) {\n");
	fprintf(fp,"\t\tif (clause[0] != ' ') strcat(query,\" \");\n");
	fprintf(fp,"\t\tstrcat(query,clause);\n");
	fprintf(fp,"\t}\n");
	fprintf(fp,"\tdprintf(\"Executing query: %%s\\n\", query);\n");
#if 0
	fprintf(fp,"\tret = SQLPrepare(db->hstmt, (SQLCHAR *) query, SQL_NTS);\n");
	fprintf(fp,"\tprintf(\"ret: %%d\\n\", ret);\n");
	fprintf(fp,"\tif (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {\n");
	fprintf(fp,"\t\tret = SQLExecute(db->hstmt);\n");
	fprintf(fp,"\t\tprintf(\"ret: %%d\\n\", ret);\n");
#endif
	fprintf(fp,"\tret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);\n");
	fprintf(fp,"\tif (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;\n");
//	fprintf(fp,"\t}\n");
	fprintf(fp,"\tret = dberr(SQL_HANDLE_STMT,db->hstmt,\"%s_update_record\");\n",table->name);
	fprintf(fp,"\treturn ret;\n");
	fprintf(fp,"#else\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_update_by(FILE *fp, TABLE *table, FIELD *field, int decl) {
	if (!field) return;
	fprintf(fp,"int %s_update_by_%s(DB *db, struct %s_record *rec, %s %s%s)",
		table->name, field->name, table->name, field->ctype, (field->ptr ? "*" : ""), field->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tchar temp[%d];\n", ((field->len + 33) / 16) * 16);
	fprintf(fp,"\n");
	fprintf(fp,"\tsprintf(temp,\" WHERE %s = ", field->name);
	if (strchr(field->pfmt,'s'))
		fprintf(fp,"'%s'", field->pfmt);
	else
		fprintf(fp,"%s", field->pfmt);
	fprintf(fp,"\", %s);\n", field->name);
	fprintf(fp,"\treturn %s_update_record(db, rec, temp);\n", table->name);
	fprintf(fp,"#else\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_delete(FILE *fp, TABLE *table, int decl) {
	fprintf(fp,"int %s_delete(DB *db, char *clause)", table->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tchar query[4096];\n");
	fprintf(fp,"\tSQLRETURN ret;\n");
	fprintf(fp,"\n");
	fprintf(fp,"\tsprintf(query,\"DELETE FROM %s %%s\",(clause ? clause : \"\"));\n", table->name);
	fprintf(fp,"\tdprintf(\"Executing query: %%s\\n\", query);\n");
	fprintf(fp,"\tret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);\n");
	fprintf(fp,"\tif (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;\n");
	fprintf(fp,"\tret = dberr(SQL_HANDLE_STMT,db->hstmt,\"%s_delete\");\n",table->name);
	fprintf(fp,"\treturn ret;\n");
	fprintf(fp,"#else\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}

void write_delete_by(FILE *fp, TABLE *table, FIELD *field, int decl) {
	if (!field) return;
	fprintf(fp,"int %s_delete_by_%s(DB *db, %s %s%s)",
		table->name, field->name, field->ctype, (field->ptr ? "*" : ""), field->name);
	if (decl) {
		fprintf(fp,";\n");
		return;
	}
	fprintf(fp," {\n");
	fprintf(fp,"#if %s_field_count > 0\n", table->name);
	fprintf(fp,"\tchar temp[%d];\n", ((field->len + 33) / 16) * 16);
	fprintf(fp,"\n");
	fprintf(fp,"\tsprintf(temp,\" WHERE %s = ", field->name);
	if (strchr(field->pfmt,'s'))
		fprintf(fp,"'%s'", field->pfmt);
	else
		fprintf(fp,"%s", field->pfmt);
	fprintf(fp,"\", %s);\n", field->name);
	fprintf(fp,"\treturn %s_delete(db, temp);\n", table->name);
	fprintf(fp,"#else\n");
	fprintf(fp,"\treturn 0;\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"}\n");
	fprintf(fp,"\n");
}
