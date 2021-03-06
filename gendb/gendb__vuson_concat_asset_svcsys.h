
#ifndef ___VUSON_CONCAT_ASSET_SVCSYS_H
#define ___VUSON_CONCAT_ASSET_SVCSYS_H

/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include "db.h"

/* Structure definition for the _vuson_concat_asset_svcsys table */
struct _vuson_concat_asset_svcsys_record {
	unsigned char assetguid[36];
	unsigned char sysguid[36];
	char server_name[251];
	char services[251];
	char supporting_systems[16384];
};
#define _vuson_concat_asset_svcsys_field_count 5
int _vuson_concat_asset_svcsys_fetch(DB *db);
int _vuson_concat_asset_svcsys_fetch_record(DB *db, struct _vuson_concat_asset_svcsys_record *rec);
int _vuson_concat_asset_svcsys_select(DB *db, char *clause);
int _vuson_concat_asset_svcsys_select_record(DB *db, struct _vuson_concat_asset_svcsys_record *rec, char *clause);
int _vuson_concat_asset_svcsys_insert(DB *db, struct _vuson_concat_asset_svcsys_record *rec);
int _vuson_concat_asset_svcsys_update_record(DB *db, struct _vuson_concat_asset_svcsys_record *rec, char *clause);
int _vuson_concat_asset_svcsys_delete(DB *db, char *clause);

#endif /* ___VUSON_CONCAT_ASSET_SVCSYS_H */
