
#include <stdio.h>
#include <string.h>
#include "gendb.h"

/* External vars & funcs */
extern int opts;

int gendbaf(char *db_name, char *user, char *pass, list tables) {
	DB *db;
	list table_names,tables,funcs;
	TABLE_LIST *tlptr;
	FUNCTION func;
	COLUMN *field;
	TABLE *table;
	register char *ptr;

	table_names = 0;

	/* Connect to the database */
	if (opts & OPT_VERBOSE) printf("Connecting to database...\n");
	db = connect_to_db("sysadm",0,0);
	if (!db) {
		dberr("unable to connect to database");
		return 1;
	}

	/* If there are no table names, create a list */
	if (opts & OPT_VERBOSE) printf("Getting table list...\n");
	if (!table_names) {
		table_names = get_table_list(db);
		if (!table_names) {
			dberr("unable to get table list");
			return 1;
		}
	}

	/* Create the tables itemlist */
	tables = list_create();
	if (!tables) {
		oserr("unable to create tables itemlist");
		return 1;
	}

	/* Go get the info for each table in table_names */
	list_reset(table_names);
	while( (tlptr = list_get_next(table_names)) != 0) {
		if (opts & OPT_VERBOSE)
			printf("Getting info for table %s.%s...\n",
				tlptr->owner,tlptr->name);
		table = get_table_info(db,tlptr->name);
		if (!table) dberr("unable to get table info");
		if (!table->keys) continue;
		list_add(tables,table,TABLE_SIZE);
		break;
	}

	/* Create the function list */
	funcs = list_create();
	if (!funcs) {
		oserr("unable to create funcs itemlist");
		return 1;
	}
	func.type = FUNC_CURSOR;
	func.keys = 0;
	list_add(funcs,&func,FUNCTION_SIZE);

	/* All info has been collected, write the files */
	list_reset(tables);
	while( (table = list_get_next(tables)) != 0) {
		if (opts & OPT_VERBOSE) {
		printf("Table owner: %s, name: %s, comment: %s\n",
			table->owner,table->name,table->comment);
		list_reset(table->fields);
		while( (field = list_get_next(table->fields)) != 0)
		    printf("\tField name: %s, type: %s, len: %d, comment: %s\n",
			field->name,dstrtype(field->type),field->len,
			field->comment);
		printf("\tPrimary keys:");
		list_reset(table->keys);
		while( (ptr = list_get_next(table->keys)) != 0)
			printf(" %s",ptr);
		printf(".\n");
		}
		/* Check if this is a header or result table with an id */
		list_reset(table->fields);
		field = list_get_next(table->fields);
		if ((strstr(table->name,"_header") != 0 ||
		    strstr(table->name,"_result") != 0) &&
		    strstr(field->name,"_id") != 0)
			field->type = ID_TYPE;

		printf("Writing include file...\n");
		write_inc(table,funcs,"try");
	}

	/* Clean up and get outta here */
	list_destroy(table_names);
	list_destroy(tables);
	return 0;
}
char *userid,char *table_info) {
