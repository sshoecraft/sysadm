
#ifndef __INV_USON_SYSTEM_INFO_H
#define __INV_USON_SYSTEM_INFO_H

/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include "db.h"

/* Structure definition for the inv_uson_system_info table */
struct inv_uson_system_info_record {
	long long _id;
	unsigned char _resourceguid[36];
	char environment[51];
	char criticality[51];
	char primary_storage_type[51];
	char primary_storage_description[201];
	char notes[501];
	char warranty_type[51];
	char warranty_information[201];
	char start[24];
	char end[24];
	char monitoring[51];
	char monitoring_comments[101];
};
#define inv_uson_system_info_field_count 13
int inv_uson_system_info_fetch(DB *db);
int inv_uson_system_info_fetch_record(DB *db, struct inv_uson_system_info_record *rec);
int inv_uson_system_info_select(DB *db, char *clause);
int inv_uson_system_info_select_record(DB *db, struct inv_uson_system_info_record *rec, char *clause);
int inv_uson_system_info_insert(DB *db, struct inv_uson_system_info_record *rec);
int inv_uson_system_info_update_record(DB *db, struct inv_uson_system_info_record *rec, char *clause);
int inv_uson_system_info_delete(DB *db, char *clause);

#endif /* __INV_USON_SYSTEM_INFO_H */
