
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
