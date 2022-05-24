
#ifndef __INV_USON_MPN_H
#define __INV_USON_MPN_H

/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include "db.h"

/* Structure definition for the inv_uson_mpn table */
struct inv_uson_mpn_record {
	long long _id;
	unsigned char _resourceguid[36];
	char part_no.[51];
};
#define inv_uson_mpn_field_count 3
int inv_uson_mpn_fetch(DB *db);
int inv_uson_mpn_fetch_record(DB *db, struct inv_uson_mpn_record *rec);
int inv_uson_mpn_select(DB *db, char *clause);
int inv_uson_mpn_select_record(DB *db, struct inv_uson_mpn_record *rec, char *clause);
int inv_uson_mpn_insert(DB *db, struct inv_uson_mpn_record *rec);
int inv_uson_mpn_update_record(DB *db, struct inv_uson_mpn_record *rec, char *clause);
int inv_uson_mpn_delete(DB *db, char *clause);

#endif /* __INV_USON_MPN_H */