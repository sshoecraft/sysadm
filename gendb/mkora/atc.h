
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
