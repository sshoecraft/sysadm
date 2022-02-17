
#ifndef __GENPC_H
#define __GENPC_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef __MINGW32__
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif
#include "list.h"
#include "db.h"

/* Define the program options */
#define OPT_RDT		1		/* Add RDT struct to inc file */
#define OPT_NET		2		/* Add NET struct to inc file */
#define OPT_INS		4		/* ? */
#define OPT_PORT	16		/* Make functions portable */
#define OPT_QUIET	32		/* Shut up */
#define OPT_VERBOSE	64		/* Chatty */
#define OPT_DEBUG	128		/* Enable debugging output */

enum FIELD_TYPES { FT_UNK,FT_CHAR,FT_STRING,FT_SHORT,FT_INT,FT_LONG,FT_QUAD,
			FT_FLOAT,FT_DOUBLE,FT_LONGDBL,FT_DATE,FT_BIN,FT_MAX };
enum FUNCTIONS { FUNC_CURSOR,FUNC_INSERT,FUNC_DELETE,FUNC_UPDATE,
		 FUNC_SELECT,FUNC_WIPE };

/* Type of the ID field */
#define ID_TYPE		FT_QUAD
#define ID_TYPE_STRING	"long long *"

/* Define the function info structure */
struct _func_info {
	int type;		/* Type of function */
	list keys;		/* Keys to use */
};
typedef struct _func_info FUNCTION;
#define FUNCTION_SIZE sizeof(struct _func_info)

/* Define the column info structure */
struct _column_info {
	char name[80];		/* Column name */
	int type;		/* Data type */
	char type_name[32];	/* Data type name */
	int len;		/* Length for char fields */
	char comment[128];	/* Column comment */
	char *ctype;		/* C decl type */
	char *pfmt;		/* printf format */
	char *dtype;		/* SQL C type */
	int ptr;		/* Data requires ptr to access */
	int noins;		/* Dont ref in insert */
	int noupd;		/* Dont ref in update */
};
typedef struct _column_info COLUMN;
#define COLUMN_SIZE sizeof(struct _column_info)
#define FIELD COLUMN

/* Define the table info structure */
struct _table_info {
	char schema[80];	/* Table owner/schema */
	char name[80];		/* Table name */
	char comment[128];	/* Table comment */
	list fields;		/* list of type COLUMN */
	list keys;		/* list of type COLUMN */
};
typedef struct _table_info TABLE;
#define TABLE_SIZE sizeof(struct _table_info)

/* Define the table list structure */
struct _table_list {
	char owner[80];		/* Table owner/schema */
	char name[80];		/* Table name */
};
typedef struct _table_list TABLE_LIST;
#define TABLE_LIST_SIZE sizeof(struct _table_list)

list get_table_list(DB *);
TABLE *get_table_info(DB *,char *,char *,char *);
typedef int (outfunc)(TABLE *,char *);

#if 0
void write_bind(FILE *fp, TABLE *,int);
void write_fetch(FILE *fp, TABLE *,int);
void write_select(FILE *fp, TABLE *,int);
void write_select_by(FILE *fp, TABLE *,FIELD *,int);
void write_insert(FILE *fp, TABLE *,int);
void write_update(FILE *fp, TABLE *,int);
void write_update_by(FILE *fp, TABLE *,FIELD *,int);
void write_delete(FILE *fp, TABLE *,int);
void write_delete_by(FILE *fp, TABLE *,FIELD *,int);
#endif

COLUMN *get_column(TABLE *,char *);
char *trim(char *);
char *strele(int,char *,char *);
char *stredit(char *,char *);
void oserr(char *);

#ifdef DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif

#endif
