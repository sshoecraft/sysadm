
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
