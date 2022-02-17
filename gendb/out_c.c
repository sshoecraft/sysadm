
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "gendb.h"

extern int opts;

static void write_bind(FILE *fp, TABLE *table, int decl) {
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

static void write_fetch_base(FILE *fp, TABLE *table, int decl) {
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

static void write_fetch(FILE *fp, TABLE *table, int decl) {
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

static void write_select_base(FILE *fp, TABLE *table, int decl) {
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

static void write_select(FILE *fp, TABLE *table, int decl) {
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

static void write_select_by(FILE *fp, TABLE *table, FIELD *field, int decl) {
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

static void write_insert(FILE *fp, TABLE *table, int decl) {
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

static void write_update(FILE *fp, TABLE *table, int decl) {
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

static void write_update_by(FILE *fp, TABLE *table, FIELD *field, int decl) {
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

static void write_delete(FILE *fp, TABLE *table, int decl) {
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

static void write_delete_by(FILE *fp, TABLE *table, FIELD *field, int decl) {
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

int write_c_inc(TABLE *table, char *funcs) {
	FILE *fp;
	COLUMN *field;
//	FUNCTION *func;
	char defname[32];
	char temp[128], *p;
	register int offset,len,i;

	if (opts & OPT_DEBUG) printf("write_inc: opening file: gendb_%s.h\n",table->name);
	sprintf(temp,"gendb_%s.h",table->name);
	fp = fopen(temp,"w+");
	if (!fp) {
		perror("fopen inc");
		return 1;
	}

	for(i=0; i < strlen(table->name); i++) temp[i] = toupper(table->name[i]);
	temp[i] = 0;
	sprintf(defname,"__%s_H", temp);

	fprintf(fp,"\n");
	fprintf(fp,"#ifndef %s\n", defname);
	fprintf(fp,"#define %s\n", defname);
	fprintf(fp,"\n");

	/* Write the comment */
	fprintf(fp,"/*\n");
	fprintf(fp,"*** this file is automatically generated - DO NOT MODIFY\n");
	fprintf(fp,"*/\n");
	fprintf(fp,"\n");

	fprintf(fp,"#include \"db.h\"");
	fprintf(fp,"\n");

	/* Write out the structure definition */
	if (opts & OPT_DEBUG) printf("write_inc: writing structure def...\n");
	fprintf(fp,"\n");
	fprintf(fp,"/* Structure definition for the %s table */\n",
		table->name);
	fprintf(fp,"struct %s_record {\n",table->name);
	dprintf("count: %d\n", list_count(table->fields));
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		if (field->ptr)
			sprintf(temp,"\t%s %s[%d];", field->ctype, field->name, field->len);
		else
			sprintf(temp,"\t%s %s;", field->ctype, field->name);
		dprintf("temp: %s\n", temp);

		/* Add the comment */
		if (strlen(field->comment)) {
			len = 8 + (strlen(temp) - 1);
			if ( (len % 8) != 0)
				offset = 8 - (len % 8);
			else
				offset = 8;
			while(len < 40) {
				strcat(temp,"\t");
				len += offset;
				offset = 8;
			}
			strcat(temp,"/* ");
			strcat(temp,field->comment);
			strcat(temp," */");
		}

		/* Write the line */
		fprintf(fp,"%s\n",temp);
	}
	fprintf(fp,"};\n");
	fprintf(fp,"#define %s_field_count %d\n", table->name, list_count(table->fields));

	/* write non-keyed func decl */
	for(p = funcs; *p; p++) {
//		dprintf("func: %c\n", *p);
		switch(*p) {
		case 'b':
			write_bind(fp,table,1);
			break;
		case 'f':
			write_fetch(fp,table,1);
			break;
		case 's':
			write_select(fp,table,1);
			break;
		case 'i':
			write_insert(fp,table,1);
			break;
		case 'u':
			write_update(fp,table,1);
			break;
		case 'd':
			write_delete(fp,table,1);
			break;
		}
	}

	/* write keyed func decl */
	dprintf("key count: %d\n", list_count(table->keys));
	list_reset(table->keys);
	while((field = list_get_next(table->keys)) != 0) {
//		dprintf("key: %s\n", field->name);
		for(p = funcs; *p; p++) {
//			dprintf("func: %c\n", *p);
			switch(*p) {
			case 's':
				write_select_by(fp, table, field, 1);
				break;
			case 'u':
				write_update_by(fp, table, field, 1);
				break;
			case 'd':
				write_delete_by(fp, table, field, 1);
				break;
			}
		}
	}

#if 0
	if (opts & OPT_RDT) {
		if (opts & OPT_DEBUG) printf("write_inc: writing rdt...\n");
		/* Write out the record descriptor table */
		offset = 0;
		fprintf(fp,"\n");
		fprintf(fp,"#ifdef NEED_%s_RDT\n",stredit(table->name,"UPCASE"));
		fprintf(fp,"/* Record Descriptor Table for %s */\n", table->name);
		fprintf(fp,"RDT %s_rdt[] = {\n",table->name);
		list_reset(table->fields);
		while( (field = list_get_next(table->fields)) != 0) {
			fprintf(fp,"\t{ \"%s\",", stredit(field->name,"UPCASE"));
			if (strlen(field->name) < 19) fprintf(fp,"\t");
			if (strlen(field->name) < 11) fprintf(fp,"\t");
			if (strlen(field->name) < 3) fprintf(fp,"\t");
			fprintf(fp,"%d,\t",offset);
			printf("type: %d\n", field->type);
			switch(field->type) {
				case FT_CHAR:
					fprintf(fp,"%s,%d },\n", "DATA_TYPE_CHAR",field->len);
					offset+=field->len;
					break;
				case FT_STRING:
					fprintf(fp,"%s,%d },\n", "DATA_TYPE_STRING",field->len);
					offset+=field->len;
					break;
				case FT_SHORT:
					fprintf(fp,"%s,0 },\n", "DATA_TYPE_SHORT");
					offset+=sizeof(short);
					break;
				case FT_INT:
					fprintf(fp,"%s,0 },\n", "DATA_TYPE_INT");
					offset+=sizeof(int);
					break;
				case FT_LONG:
					fprintf(fp,"%s,0 },\n", "DATA_TYPE_LONG");
					offset+=sizeof(long);
					break;
				case FT_FLOAT:
					fprintf(fp,"%s,0 },\n", "DATA_TYPE_FLOAT");
					offset+=sizeof(float);
					break;
				case FT_DOUBLE:
					fprintf(fp,"%s,0 },\n", "DATA_TYPE_DOUBLE");
					offset+=sizeof(double);
					break;
				case FT_LONGDBL:
					fprintf(fp,"%s,0 },\n", "DATA_TYPE_LONGDBL");
					offset+=sizeof(long double);
					break;
				case FT_DATE:
					fprintf(fp,"%s,21 },\n", "DATA_TYPE_STRING");
					offset+=21;
					break;
				default:
					fprintf(fp,"%s,0 },\n", "DATA_TYPE_UNKNOWN");
					break;
			}
		}
		fprintf(fp,"\t{ 0,%d,0,0 }\n",offset);
		fprintf(fp,"};\n");
		fprintf(fp,"#endif\n");
	}

	if (opts & OPT_NET) {
		if (opts & OPT_DEBUG) printf("write_inc: writing net rec...\n");
		/* Write out the net rec structure definition */
		fprintf(fp,"\n");
		fprintf(fp,"/* Network record for %s */\n",table->name);
		fprintf(fp,"struct %s_net_record {\n",table->name);
		list_reset(table->fields);
		while((field = list_get_next(table->fields)) != 0) {
			switch(field->type) {
			case FT_CHAR:
			case FT_STRING:
				fprintf(fp,"\tchar %s",field->name);
				if (field->len > 1)
					fprintf(fp,"[%d];\n", field->len);
				else
					fprintf(fp,";\n");
				break;
			case FT_SHORT:
				fprintf(fp,"\tchar %s[%d];\n", field->name, (int)(sizeof(short)*3)+1);
				break;
			case FT_INT:
				fprintf(fp,"\tchar %s[%d];\n", field->name, (int)(sizeof(int)*3)+1);
				break;
			case FT_LONG:
				fprintf(fp,"\tchar %s[%d];\n", field->name, (int)(sizeof(int)*3)+1);
				break;
			case FT_FLOAT:
				fprintf(fp,"\tchar %s[%d];\n", field->name, (int)(sizeof(float)*3)+1);
				break;
			case FT_DOUBLE:
				fprintf(fp,"\tchar %s[%d];\n", field->name, (int)(sizeof(double)*3)+1);
				break;
			case FT_LONGDBL:
				fprintf(fp,"\tchar %s[%d];\n", field->name, (int)(sizeof(long double)*3)+1);
				break;
			case FT_DATE:
				fprintf(fp,"\tchar %s[21];\n", field->name);
				break;
			default:
				fprintf(fp,"\tunknown %s;\n", field->name);
			}
		}
		fprintf(fp,"};\n");
		fprintf(fp,"typedef struct %s_net_record %s_NET_REC;\n",table->name,
			stredit(table->name,"UPCASE"));
		fprintf(fp, "#define %s_NET_REC_SIZE sizeof(struct %s_net_record)\n",
			stredit(table->name,"UPCASE"),table->name);
	}

	/* Write out the function declarations */
	if (opts & OPT_DEBUG) printf("write_inc: writing func decs:");
	fprintf(fp,"\n");
	fprintf(fp,"/* %s function declarations */\n",table->name);
	if (opts & OPT_PORT) {
		if (opts & OPT_DEBUG) printf(" open");
		fprintf(fp,"void open_%s_cursor(long *",table->name);
		if (list_count(table->keys)) {
			fprintf(fp,",");
			list_reset(table->keys);
			while((p = list_get_next(table->keys)) != 0) {
				field = get_column(table,p);
				if (!field) continue;
				fprintf(fp,"%d",field->type);
				if (field->ptr) fprintf(fp," *");
				if (list_is_next(table->keys)) fprintf(fp,",");
			}
		}
		fprintf(fp,");\n");
		if (opts & OPT_DEBUG) printf(" fetch");
		fprintf(fp,"void fetch_%s_record(long *,%s_REC *);\n",
			table->name,stredit(table->name,"UPCASE"));
		if (opts & OPT_DEBUG) printf(" close");
		fprintf(fp,"void close_%s_cursor(long *);\n",table->name);
#if 0
		if (strlen(seq_name)) {
			if (opts & OPT_DEBUG) printf(" get_id");
			fprintf(fp,"void get_%s_id(long *,%s *);\n",
				table->name,ID_TYPE_STRING);
		}
#endif
		if (opts & OPT_INS) {
			if (opts & OPT_DEBUG) printf(" insert");
			fprintf(fp,"void insert_%s_record(long *,%s_REC *);\n",
				table->name,stredit(table->name,"UPCASE"));
		}
	}
	else {
		if (opts & OPT_DEBUG) printf(" open");
		fprintf(fp,"int open_%s_cursor(",table->name);
		if (list_count(table->keys)) {
			list_reset(table->keys);
			while((p = list_get_next(table->keys)) != 0) {
				field = get_column(table,p);
				fprintf(fp,"%d",field->type);
				if (field->ptr) fprintf(fp," *");
				if (list_is_next(table->keys)) fprintf(fp,",");
			}
		}
		else
			fprintf(fp,"void");
		fprintf(fp,");\n");
		if (opts & OPT_DEBUG) printf(" fetch");
		fprintf(fp,"int fetch_%s_record(%s_REC *);\n",
			table->name,stredit(table->name,"UPCASE"));
		if (opts & OPT_DEBUG) printf(" close");
		fprintf(fp,"int close_%s_cursor(void);\n",table->name);
#if 0
		if (strlen(seq_name)) {
			if (opts & OPT_DEBUG) printf(" get_id");
			fprintf(fp,"int get_%s_id(%s *);\n",
				table->name,ID_TYPE_STRING);
		}
#endif
		if (opts & OPT_INS) {
			if (opts & OPT_DEBUG) printf(" insert");
			fprintf(fp,"int insert_%s_record(%s_REC *);\n",
				table->name,stredit(table->name,"UPCASE"));
		}
	}
	if (opts & OPT_DEBUG) printf(".\n");
#endif
	fprintf(fp,"\n#endif /* %s */\n",defname);

	/* Close the file and return */
	if (opts & OPT_DEBUG) printf("write_inc: done!\n");
	fclose(fp);
	return 0;
}

int write_c_src(TABLE *table,char *funcs) {
	FILE *fp;
	COLUMN *field;
	char temp[128], *p;

	if (opts & OPT_DEBUG) printf("write_src: opening file: gendb_%s.c\n", table->name);
	sprintf(temp,"gendb_%s.c",table->name);
	fp = fopen(temp,"w+");
	if (!fp) {
		perror("fopen src");
		return 1;
	}

	/* Write the comment */
	fprintf(fp,"\n");
	fprintf(fp,"/*\n");
	fprintf(fp,"*** this file is automatically generated - DO NOT MODIFY\n");
	fprintf(fp,"*/\n");

	fprintf(fp,"\n");
	fprintf(fp,"#include <stdio.h>\n");
	fprintf(fp,"#include <string.h>\n");
	fprintf(fp,"#include \"gendb_%s.h\"\n",table->name);
	fprintf(fp,"\n");

#if 0
	fprintf(fp,"#ifndef dprintf\n");
	fprintf(fp,"#ifdef DEBUG\n");
	fprintf(fp,"#define dprintf(format, args...) printf(\"%%s(%%d): \" format,__FUNCTION__,__LINE__, ## args)\n");
	fprintf(fp,"#else\n");
	fprintf(fp,"#define dprintf(format, args...) /* noop */\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"#endif\n");
	fprintf(fp,"\n");
#endif

	/* write non-keyed func */
	for(p = funcs; *p; p++) {
//		dprintf("func: %c\n", *p);
		switch(*p) {
		case 'b':
			write_bind(fp,table,0);
			break;
		case 'f':
			write_fetch(fp,table,0);
			break;
		case 's':
			write_select(fp,table,0);
			break;
		case 'i':
			write_insert(fp,table,0);
			break;
		case 'u':
			write_update(fp,table,0);
			break;
		case 'd':
			write_delete(fp,table,0);
			break;
		}
	}

	/* write keyed func */
	dprintf("key count: %d\n", list_count(table->keys));
	list_reset(table->keys);
	while((field = list_get_next(table->keys)) != 0) {
		dprintf("key: %s\n", field->name);
		for(p = funcs; *p; p++) {
//			dprintf("func: %c\n", *p);
			switch(*p) {
			case 's':
				write_select_by(fp, table, field, 0);
				break;
			case 'u':
				write_update_by(fp, table, field, 0);
				break;
			case 'd':
				write_delete_by(fp, table, field, 0);
				break;
			}
		}
	}

#if 0
	/* Write out the structure definition */
	if (opts & OPT_DEBUG) printf("write_src: writing structure def...\n");
	fprintf(fp,"\n");
	fprintf(fp,"/* Structure definition for the %s table */\n",
		table->name);
	fprintf(fp,"struct %s_record {\n",table->name);
	dprintf("count: %d\n", list_count(table->fields));
	list_reset(table->fields);
	while( (field = list_get_next(table->fields)) != 0) {
		sprintf(temp,"\t%s ",typestr(1,field->type));
		strcat(temp,field->name);
//		dprintf("temp: %s\n", temp);
		if (field->len > 1) {
			strcat(temp,"[");
			sprintf(temp2,"%d",field->len);
			strcat(temp,temp2);
			strcat(temp,"];");
		}
		else
			strcat(temp,";");
		if (field->type == FT_CHAR || field->type == FT_STRING) {
			sprintf(temp,"\tchar %s",field->name);
			if (field->len > 1) {
				strcat(temp,"[");
				sprintf(temp2,"%d",field->len);
				strcat(temp,temp2);
				strcat(temp,"];");
			}
			else
				strcat(temp,";");
		}
		else if (field->type == FT_SHORT)
			sprintf(temp,"\tshort %s;",field->name);
		else if (field->type == FT_INT)
			sprintf(temp,"\tint %s;",field->name);
		else if (field->type == FT_LONG)
			sprintf(temp,"\tlong %s;",field->name);
		else if (field->type == FT_QUAD)
			sprintf(temp,"\tlong long %s;",field->name);
		else if (field->type == FT_FLOAT)
			sprintf(temp,"\tfloat %s;",field->name);
		else if (field->type == FT_DOUBLE)
			sprintf(temp,"\tdouble %s;",field->name);
		else if (field->type == FT_LONGDBL)
			sprintf(temp,"\tlong double %s;",field->name);
		else if (field->type == FT_DATE)
			sprintf(temp,"\tchar %s[21];",field->name);
		else
			sprintf(temp,"\tunknown %s;",field->name);

		/* Add the comment */
		if (strlen(field->comment)) {
			len = 8 + (strlen(temp) - 1);
			if ( (len % 8) != 0)
				offset = 8 - (len % 8);
			else
				offset = 8;
			while(len < 40) {
				strcat(temp,"\t");
				len += offset;
				offset = 8;
			}
			strcat(temp,"/* ");
			strcat(temp,field->comment);
			strcat(temp," */");
		}

		/* Write the line */
		fprintf(fp,"%s\n",temp);
	}
	fprintf(fp,"};\n");
	fprintf(fp,"typedef struct %s_record %s_REC;\n",table->name,
		stredit(prefix,"UPCASE"));
	fprintf(fp,"#define %s_REC_SIZE sizeof(struct %s_record)\n",
		stredit(prefix,"UPCASE"),table->name);

	if (opts & OPT_RDT) {
		if (opts & OPT_DEBUG) printf("write_src: writing rdt...\n");
		/* Write out the record descriptor table */
		offset = 0;
		fprintf(fp,"\n");
		fprintf(fp,"#ifdef NEED_%s_RDT\n",stredit(prefix,"UPCASE"));
		fprintf(fp,"/* Record Descriptor Table for %s */\n",
			table->name);
		fprintf(fp,"RDT %s_rdt[] = {\n",prefix);
		list_reset(table->fields);
		while( (field = list_get_next(table->fields)) != 0) {
			fprintf(fp,"\t{ \"%s\",",
				stredit(field->name,"UPCASE"));
			if (strlen(field->name) < 19)
				fprintf(fp,"\t");
			if (strlen(field->name) < 11)
				fprintf(fp,"\t");
			if (strlen(field->name) < 3)
				fprintf(fp,"\t");
			fprintf(fp,"%d,\t",offset);
			switch(field->type) {
				case FT_CHAR:
					fprintf(fp,"%s,%d },\n",
						"DATA_TYPE_CHAR",field->len);
					offset+=field->len;
				break;
				case FT_STRING:
					fprintf(fp,"%s,%d },\n",
						"DATA_TYPE_STRING",field->len);
					offset+=field->len;
					break;
				case FT_SHORT:
					fprintf(fp,"%s,0 },\n",
						"DATA_TYPE_SHORT");
					offset+=2;
					break;
				case FT_LONG:
					fprintf(fp,"%s,0 },\n",
						"DATA_TYPE_LONG");
					offset+=4;
					break;
				case FT_DOUBLE:
					fprintf(fp,"%s,0 },\n",
						"DATA_TYPE_DOUBLE");
					offset+=8;
					break;
				case FT_LONGDBL:
					fprintf(fp,"%s,0 },\n",
						"DATA_TYPE_LONGDBL");
					offset+=16;
					break;
				case FT_DATE:
					fprintf(fp,"%s,21 },\n",
						"DATA_TYPE_STRING");
					offset+=21;
					break;
				default:
					fprintf(fp,"%s,0 },\n",
						"DATA_TYPE_UNKNOWN");
					break;
			}
		}
		fprintf(fp,"\t{ 0,%d,0,0 }\n",offset);
		fprintf(fp,"};\n");
		fprintf(fp,"#endif\n");
	}
#endif

#ifdef XXX
	if (opts & OPT_NET) {
		if (opts & OPT_DEBUG) printf("write_src: writing net rec...\n");
		/* Write out the net rec structure definition */
		fprintf(fp,"\n");
		fprintf(fp,"/* Network record for %s */\n",table_name);
		fprintf(fp,"struct %s_net_record {\n",prefix);
		cur_field = field_list;
		while(cur_field) {
			if (cur_field->field_type == FT_CHAR) {
				fprintf(fp,"\tchar %s",cur_field->field_name);
				if (cur_field->field_len > 1)
					fprintf(fp,"[%d];\n",
						cur_field->field_len);
				else
					fprintf(fp,";\n");
			}
			else if (cur_field->field_type == FT_SHORT)
				fprintf(fp,"\tchar %s[5];\n",
					cur_field->field_name);
			else if (cur_field->field_type == FT_LONG)
				fprintf(fp,"\tchar %s[12];\n",
					cur_field->field_name);
			else if (cur_field->field_type == FT_DOUBLE)
				fprintf(fp,"\tchar %s[40];\n",
					cur_field->field_name);
			else if (cur_field->field_type == FT_LONGDBL)
				fprintf(fp,"\tchar %s[80];\n",
					cur_field->field_name);
			else if (cur_field->field_type == FT_DATE)
				fprintf(fp,"\tchar %s[21];\n",
					cur_field->field_name);
			else
				fprintf(fp,"\tunknown %s;\n",
					cur_field->field_name);
			cur_field = cur_field->next;
		}
		fprintf(fp,"};\n");
		fprintf(fp,"typedef struct %s_net_record %s_NET_REC;\n",prefix,
			stredit(prefix,"UPCASE"));
		fprintf(fp,
		    "#define %s_NET_REC_SIZE sizeof(struct %s_net_record)\n",
			stredit(prefix,"UPCASE"),prefix);
	}

	/* Write out the function declarations */
	if (opts & OPT_DEBUG) printf("write_src: writing func decs:");
	fprintf(fp,"\n");
	fprintf(fp,"/* %s function declarations */\n",table->name);
	if (opts & OPT_PORT) {
		if (opts & OPT_DEBUG) printf(" open");
		fprintf(fp,"void open_%s_cursor(long *",prefix);
		if (key_list) {
			fprintf(fp,",");
			cur_key = key_list;
			while(cur_key) {
				fprintf(fp,"%s",cur_key->key_type);
				if (cur_key->is_ptr) fprintf(fp," *");
				if (cur_key->next) fprintf(fp,",");
				cur_key = cur_key->next;
			}
		}
		fprintf(fp,");\n");
		if (opts & OPT_DEBUG) printf(" fetch");
		fprintf(fp,"void fetch_%s_record(long *,%s_REC *);\n",
			prefix,stredit(prefix,"UPCASE"));
		if (opts & OPT_DEBUG) printf(" close");
		fprintf(fp,"void close_%s_cursor(long *);\n",prefix);
		if (strlen(seq_name)) {
			if (opts & OPT_DEBUG) printf(" get_id");
			fprintf(fp,"void get_%s_id(long *,%s *);\n",
				prefix,ID_TYPE_STRING);
		}
		if (opts & OPT_INS) {
			if (opts & OPT_DEBUG) printf(" insert");
			fprintf(fp,"void insert_%s_record(long *,%s_REC *);\n",
				prefix,stredit(prefix,"UPCASE"));
		}
	}
	else {
		if (opts & OPT_DEBUG) printf(" open");
		fprintf(fp,"int open_%s_cursor(",prefix);
		if (key_list) {
			cur_key = key_list;
			while(cur_key) {
				fprintf(fp,"%s",cur_key->key_type);
				if (cur_key->is_ptr) fprintf(fp," *");
				if (cur_key->next) fprintf(fp,",");
				cur_key = cur_key->next;
			}
		}
		else
			fprintf(fp,"void");
		fprintf(fp,");\n");
		if (opts & OPT_DEBUG) printf(" fetch");
		fprintf(fp,"int fetch_%s_record(%s_REC *);\n",
			prefix,stredit(prefix,"UPCASE"));
		if (opts & OPT_DEBUG) printf(" close");
		fprintf(fp,"int close_%s_cursor(void);\n",prefix);
		if (strlen(seq_name)) {
			if (opts & OPT_DEBUG) printf(" get_id");
			fprintf(fp,"int get_%s_id(%s *);\n",
				prefix,ID_TYPE_STRING);
		}
		if (opts & OPT_INS) {
			if (opts & OPT_DEBUG) printf(" insert");
			fprintf(fp,"int insert_%s_record(%s_REC *);\n",
				prefix,stredit(prefix,"UPCASE"));
		}
	}
	if (opts & OPT_DEBUG) printf(".\n");
#endif

	/* Close the file and return */
	if (opts & OPT_DEBUG) printf("write_src: done!\n");
	fclose(fp);
	return 0;
}

#if 0
int write_pc_file(table_name,prefix,field_list)
char *table_name,*prefix;
TABLE_INFO *field_list;
{
	FILE *fp;
	char filename[40],line[80];
	TABLE_INFO *cur_field;
	KEY_INFO *cur_key;
	int has_string;

	if (opts & OPT_DEBUG) printf("write_pc: opening file: %s.pc\n",prefix);
	sprintf(filename,"%s.pc",prefix);
	fp = fopen(filename,"w+");
	if (!fp) {
		perror("unable to open pc file");
		return 1;
	}

	/* Write a comment */
	fprintf(fp,"\n");
	fprintf(fp,"/*\n");
	fprintf(fp,"*** %s pc file generated by genpc\n",table_name);
	fprintf(fp,"*/\n");
	if (opts & OPT_DEBUG) printf("write_pc: writing funcs:");

	/* Write the open function */
	if (opts & OPT_DEBUG) printf(" open");
	fprintf(fp,"\n");
	if (opts & OPT_PORT) {
		fprintf(fp,"void open_%s_cursor(long *sqlcode",prefix);
		if (key_list) {
			fprintf(fp,",");
			cur_key = key_list;
			while(cur_key) {
				fprintf(fp,"%s ",cur_key->key_type);
				if (cur_key->is_ptr) fprintf(fp,"*");
				fprintf(fp,"%s",cur_key->key_name);
				if (cur_key->next) fprintf(fp,",");
				cur_key = cur_key->next;
			}
		}
		fprintf(fp,") {\n");
	}
	else {
		fprintf(fp,"int open_%s_cursor(",prefix);
		if (key_list) {
			cur_key = key_list;
			while(cur_key) {
				fprintf(fp,"%s ",cur_key->key_type);
				if (cur_key->is_ptr) fprintf(fp,"*");
				fprintf(fp,"%s",cur_key->key_name);
				if (cur_key->next) fprintf(fp,",");
				cur_key = cur_key->next;
			}
		}
		else
			fprintf(fp,"void");
		fprintf(fp,") {\n");
	}
	fprintf(fp,"\tsql_return_code = 0;\n");
	fprintf(fp,"\tEXEC SQL DECLARE %s_cursor CURSOR FOR \\\n",
		prefix);
	fprintf(fp,"\t\tSELECT ");
	cur_field = field_list;
	line[0] = 0;
	while(cur_field) {
		strcat(line,cur_field->field_name);
		if (cur_field->next) strcat(line,",");
		if (strlen(line) > 40) {
			fprintf(fp,"%s \\\n",line);
			if (cur_field->next)
				strcpy(line,"\t\t\t");
			else
				line[0] = 0;
		}
		cur_field = cur_field->next;
	}
	if (strlen(line)) fprintf(fp,"%s \\\n",line);
	fprintf(fp,"\t\tFROM %s",table_name);
	if (key_list) {
		fprintf(fp," \\\n\t\tWHERE ");
		line[0] = 0;
		cur_key = key_list;
		while(cur_key) {
			strcat(line,cur_key->key_name);
			strcat(line," = :");
			strcat(line,cur_key->key_name);
			if (cur_key->next)
				strcat(line," AND ");
			else
				strcat(line,";");
			if (strlen(line) > 40) {
				fprintf(fp,"%s",line);
				if (cur_key->next) {
					fprintf(fp," \\\n");
					strcpy(line,"\t\t\t");
				}
				else {
					fprintf(fp,"\n");
					line[0] = 0;
				}
			}
			cur_key = cur_key->next;
		}
		if (strlen(line)) fprintf(fp,"%s\n",line);
	}
	else
		fprintf(fp,";\n");
	if (opts & OPT_PORT) {
		fprintf(fp,"\tif (sql_return_code != 0) {\n");
		fprintf(fp,"\t\t*sqlcode = sql_return_code;\n");
		fprintf(fp,"\t\treturn;\n");
		fprintf(fp,"\t}\n");
	}
	else
		fprintf(fp,"\tif (sql_return_code != 0) return 1;\n");
	fprintf(fp,"\tEXEC SQL OPEN %s_cursor;\n",prefix);
	if (opts & OPT_PORT) {
		fprintf(fp,"\t*sqlcode = sql_return_code;\n");
		fprintf(fp,"\treturn;\n");
	}
	else
		fprintf(fp,"\treturn (sql_return_code == 0 ? 0 : 1);\n");
	fprintf(fp,"}\n");

	/* Write the fetch function */
	if (opts & OPT_DEBUG) printf(" fetch");
	fprintf(fp,"\n");
	if (opts & OPT_PORT)
		fprintf(fp,
			"void fetch_%s_record(long *sqlcode,%s_REC *rec) {\n",
			prefix,stredit(prefix,"UPCASE"));
	else
		fprintf(fp,"int fetch_%s_record(%s_REC *rec) {\n",
			prefix,stredit(prefix,"UPCASE"));
	fprintf(fp,"\tsql_return_code = 0;\n");
	fprintf(fp,"\tEXEC SQL FETCH %s_cursor INTO :rec;\n",prefix);
	/* Check to see if the table has any strings */
	has_string = 0;
	cur_field = field_list;
	while(cur_field) {
		if (cur_field->field_type == FT_STRING) {
			has_string = 1;
			break;
		}
		cur_field = cur_field->next;
	}
	if (has_string) {
		fprintf(fp,"\tif (sql_return_code == 0) {\n");
		cur_field = field_list;
		while(cur_field) {
			if (cur_field->field_type == FT_STRING)
				fprintf(fp,"\t\ttrim(rec->%s);\n",
					cur_field->field_name);
			cur_field = cur_field->next;
		}
		fprintf(fp,"\t}\n");
	}
	if (opts & OPT_PORT) {
		fprintf(fp,"\t*sqlcode = sql_return_code;\n");
		fprintf(fp,"\treturn;\n");
	}
	else
		fprintf(fp,"\treturn (sql_return_code == 0 ? 0 : 1);\n");
	fprintf(fp,"}\n");

	/* Write the close function */
	if (opts & OPT_DEBUG) printf(" close");
	fprintf(fp,"\n");
	if (opts & OPT_PORT)
		fprintf(fp,"void close_%s_cursor(long *sqlcode) {\n",prefix);
	else
		fprintf(fp,"int close_%s_cursor(void) {\n",prefix);
	fprintf(fp,"\tsql_return_code = 0;\n");
	fprintf(fp,"\tEXEC SQL CLOSE %s_cursor;\n",prefix);
	if (opts & OPT_PORT) {
		fprintf(fp,"\t*sqlcode = sql_return_code;\n");
		fprintf(fp,"\treturn;\n");
	}
	else
		fprintf(fp,"\treturn (sql_return_code == 0 ? 0 : 1);\n");
	fprintf(fp,"}\n");

	if (strlen(seq_name)) {
		/* Write the get id function */
		if (opts & OPT_DEBUG) printf(" get_id");
		fprintf(fp,"\n");
		if (opts & OPT_PORT)
			fprintf(fp,"void get_%s_id(long *sqlcode,%s *id) {\n",
				prefix,ID_TYPE_STRING);
		else
			fprintf(fp,"int get_%s_id(%s *id) {\n",
				prefix,ID_TYPE_STRING);
		fprintf(fp,"\tsql_return_code = 0;\n");
		fprintf(fp,"\tEXEC SQL SELECT %s.NEXTVAL ",seq_name);
		fprintf(fp,"INTO :id FROM dual;\n");
		if (opts & OPT_PORT) {
			fprintf(fp,"\t*sqlcode = sql_return_code;\n");
			fprintf(fp,"\treturn;\n");
		}
		else
			fprintf(fp,
				"\treturn (sql_return_code == 0 ? 0 : 1);\n");
		fprintf(fp,"}\n");
	}

	if (opts & OPT_INS) {
		/* Write the insert function */
		if (opts & OPT_DEBUG) printf(" insert");
		fprintf(fp,"\n");
		if (opts & OPT_PORT)
			fprintf(fp,
			 "void insert_%s_record(long *sqlcode,%s_REC *rec) {\n",
				prefix,stredit(prefix,"UPCASE"));
		else
			fprintf(fp,"int insert_%s_record(%s_REC *rec) {\n",
				prefix,stredit(prefix,"UPCASE"));
		fprintf(fp,"\tsql_return_code = 0;\n");
		fprintf(fp,"\tEXEC SQL INSERT INTO %s (\n",table_name);
		cur_field = field_list;
		while(cur_field) {
			fprintf(fp,"\t\t%s",cur_field->field_name);
			if (cur_field->next) fprintf(fp,",");
			fprintf(fp,"\n");
			cur_field = cur_field->next;
		}
		fprintf(fp,"\t) VALUES (\n");
		cur_field = field_list;
		while(cur_field) {
			if (cur_field->field_type == FT_DATE) {
				fprintf(fp,"\t\tTO_DATE(:rec->%s,",
					cur_field->field_name);
				fprintf(fp,"'DD-MON-YYYY HH24:MI:SS')");
			}
			else
				fprintf(fp,"\t\t:rec->%s",
					cur_field->field_name);
			if (cur_field->next) fprintf(fp,",");
			fprintf(fp,"\n");
			cur_field = cur_field->next;
		}
		fprintf(fp,"\t);\n");
		if (opts & OPT_PORT) {
			fprintf(fp,"\t*sqlcode = sql_return_code;\n");
			fprintf(fp,"\treturn;\n");
		}
		else
			fprintf(fp,
				"\treturn (sql_return_code == 0 ? 0 : 1);\n");
		fprintf(fp,"}\n");
	}
	if (opts & OPT_DEBUG) printf(".\n");

	/* Close the pc file and return */
	if (opts & OPT_DEBUG) printf("write_pc: done!\n");
	fclose(fp);
	return 0;
}
#endif
