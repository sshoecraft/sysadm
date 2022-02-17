
/* Structure definition for the all_catalog table */
struct acr_record {
	char owner[31];
	char table_name[31];
	char table_type[12];
};
typedef struct acr_record ACR_REC;
#define ACR_REC_SIZE sizeof(struct acr_record)
