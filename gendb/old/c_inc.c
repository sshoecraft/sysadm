
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "gendb.h"

extern int opts;

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