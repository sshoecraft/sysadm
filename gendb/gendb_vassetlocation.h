
#ifndef __VASSETLOCATION_H
#define __VASSETLOCATION_H

/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include "db.h"

/* Structure definition for the vassetlocation table */
struct vassetlocation_record {
	unsigned char _assetguid[36];
	unsigned char _location[36];
	char asset_name[251];
	char location[251];
};
#define vassetlocation_field_count 4
int vassetlocation_fetch(DB *db);
int vassetlocation_fetch_record(DB *db, struct vassetlocation_record *rec);
int vassetlocation_select(DB *db, char *clause);
int vassetlocation_select_record(DB *db, struct vassetlocation_record *rec, char *clause);
int vassetlocation_insert(DB *db, struct vassetlocation_record *rec);
int vassetlocation_update_record(DB *db, struct vassetlocation_record *rec, char *clause);
int vassetlocation_delete(DB *db, char *clause);

#endif /* __VASSETLOCATION_H */