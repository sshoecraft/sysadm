
#ifndef __ORACLE_H
#define __ORACLE_H

/* Structure definition for the all_catalog table */
struct acr_record {
	char owner[31];
	char table_name[31];
	char table_type[12];
};
typedef struct acr_record ACR_REC;
#define ACR_REC_SIZE sizeof(struct acr_record)

/* Structure definition for the all_tab_columns table */
struct atc_record {
	double column_id;
	char column_name[31];
	char data_type[10];
	double data_length;
	double data_precision;
	double data_scale;
};
typedef struct atc_record ATC_REC;
#define ATC_REC_SIZE sizeof(struct atc_record)

/* all_tab_columns function declarations */
int open_atc_cursor(char *,char *);
int fetch_atc_record(ATC_REC *);
int close_atc_cursor(void);

/* Structure definition for the all_cons_columns table */
struct acc_record {
	char owner[31];
	char constraint_name[31];
	char table_name[31];
	char column_name[31];
	long position;
};
typedef struct acc_record ACC_REC;
#define ACC_REC_SIZE sizeof(struct acc_record)

/* Standard function declarations */
int connect_to_db(char *);
int set_ro_trans(void);
int set_rw_trans(void);
int rollback_trans(void);
int commit_trans(void);

/* Declare the sql_error_text variable */
#ifdef NEED_SQL_ERROR_TEXT
char sql_error_text[128];
int sql_return_code;
#else
extern char sql_error_text[128];
extern int sql_return_code;
#endif

#endif
