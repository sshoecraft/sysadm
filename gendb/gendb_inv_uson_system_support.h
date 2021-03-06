
#ifndef __INV_USON_SYSTEM_SUPPORT_H
#define __INV_USON_SYSTEM_SUPPORT_H

/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include "db.h"

/* Structure definition for the inv_uson_system_support table */
struct inv_uson_system_support_record {
	long long _id;
	unsigned char _resourceguid[36];
	char tech_support[51];
	char app_support[51];
};
#define inv_uson_system_support_field_count 4
int inv_uson_system_support_fetch(DB *db);
int inv_uson_system_support_fetch_record(DB *db, struct inv_uson_system_support_record *rec);
int inv_uson_system_support_select(DB *db, char *clause);
int inv_uson_system_support_select_record(DB *db, struct inv_uson_system_support_record *rec, char *clause);
int inv_uson_system_support_insert(DB *db, struct inv_uson_system_support_record *rec);
int inv_uson_system_support_update_record(DB *db, struct inv_uson_system_support_record *rec, char *clause);
int inv_uson_system_support_delete(DB *db, char *clause);

#endif /* __INV_USON_SYSTEM_SUPPORT_H */
