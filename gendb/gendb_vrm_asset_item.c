
/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include <stdio.h>
#include <string.h>
#include "gendb_vrm_asset_item.h"

int vrm_asset_item_fetch(DB *db) {
#if vrm_asset_item_field_count > 0
	SQLRETURN ret;

	dprintf("Fetching data...\n");
	ret = SQLFetch(db->hstmt);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	if (ret != 100) ret = dberr(SQL_HANDLE_STMT,db->hstmt,"vrm_asset_item_fetch");
	return ret;
#else
	return 0;
#endif
}

int vrm_asset_item_fetch_record(DB *db, struct vrm_asset_item_record *rec) {
#if vrm_asset_item_field_count > 0
	SQLRETURN ret;

	if ((ret = vrm_asset_item_fetch(db)) != 0) {
		SQLCloseCursor(db->hstmt);
		return ret;
	}
	memset(rec,0,sizeof(*rec));
	SQLGetData(db->hstmt, 1, SQL_C_BINARY, (SQLPOINTER) rec->guid, sizeof(rec->guid), 0);
	SQLGetData(db->hstmt, 2, SQL_C_BINARY, (SQLPOINTER) rec->classguid, sizeof(rec->classguid), 0);
	SQLGetData(db->hstmt, 3, SQL_C_BINARY, (SQLPOINTER) rec->resourcetypeguid, sizeof(rec->resourcetypeguid), 0);
	SQLGetData(db->hstmt, 4, SQL_C_CHAR, (SQLPOINTER) rec->name, sizeof(rec->name), 0);
	SQLGetData(db->hstmt, 5, SQL_C_CHAR, (SQLPOINTER) rec->description, sizeof(rec->description), 0);
	SQLGetData(db->hstmt, 6, SQL_C_BINARY, (SQLPOINTER) rec->ownernsguid, sizeof(rec->ownernsguid), 0);
	SQLGetData(db->hstmt, 7, SQL_C_BINARY, (SQLPOINTER) rec->productguid, sizeof(rec->productguid), 0);
	SQLGetData(db->hstmt, 8, SQL_C_BINARY, (SQLPOINTER) rec->securityguid, sizeof(rec->securityguid), 0);
	SQLGetData(db->hstmt, 9, SQL_C_SLONG, (SQLPOINTER) &rec->attributes, sizeof(rec->attributes), 0);
	SQLGetData(db->hstmt, 10, SQL_C_CHAR, (SQLPOINTER) rec->alias, sizeof(rec->alias), 0);
	SQLGetData(db->hstmt, 11, SQL_C_CHAR, (SQLPOINTER) rec->createdby, sizeof(rec->createdby), 0);
	SQLGetData(db->hstmt, 12, SQL_C_CHAR, (SQLPOINTER) rec->modifiedby, sizeof(rec->modifiedby), 0);
	SQLGetData(db->hstmt, 13, SQL_C_CHAR, (SQLPOINTER) rec->createddate, sizeof(rec->createddate), 0);
	SQLGetData(db->hstmt, 14, SQL_C_CHAR, (SQLPOINTER) rec->modifieddate, sizeof(rec->modifieddate), 0);
	SQLGetData(db->hstmt, 15, SQL_C_SLONG, (SQLPOINTER) &rec->productuninstalled, sizeof(rec->productuninstalled), 0);
	SQLGetData(db->hstmt, 16, SQL_C_CHAR, (SQLPOINTER) rec->state, sizeof(rec->state), 0);
	SQLGetData(db->hstmt, 17, SQL_C_SLONG, (SQLPOINTER) &rec->ismanaged, sizeof(rec->ismanaged), 0);
	SQLGetData(db->hstmt, 18, SQL_C_SLONG, (SQLPOINTER) &rec->resourceitemdeleted, sizeof(rec->resourceitemdeleted), 0);
	dprintf("----------------- vrm_asset_item record -----------------\n");
	dprintf("guid                :  %x\n",rec->guid);
	dprintf("classguid           :  %x\n",rec->classguid);
	dprintf("resourcetypeguid    :  %x\n",rec->resourcetypeguid);
	dprintf("name                :  %s\n",rec->name);
	dprintf("description         :  %s\n",rec->description);
	dprintf("ownernsguid         :  %x\n",rec->ownernsguid);
	dprintf("productguid         :  %x\n",rec->productguid);
	dprintf("securityguid        :  %x\n",rec->securityguid);
	dprintf("attributes          :  %d\n",rec->attributes);
	dprintf("alias               :  %s\n",rec->alias);
	dprintf("createdby           :  %s\n",rec->createdby);
	dprintf("modifiedby          :  %s\n",rec->modifiedby);
	dprintf("createddate         :  %s\n",rec->createddate);
	dprintf("modifieddate        :  %s\n",rec->modifieddate);
	dprintf("productuninstalled  :  %d\n",rec->productuninstalled);
	dprintf("state               :  %s\n",rec->state);
	dprintf("ismanaged           :  %d\n",rec->ismanaged);
	dprintf("resourceitemdeleted :  %d\n",rec->resourceitemdeleted);
#endif
	return 0;
}

int vrm_asset_item_select(DB *db, char *clause) {
#if vrm_asset_item_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"SELECT vrm_asset_item.guid,vrm_asset_item.classguid,vrm_asset_item.resourcetypeguid,vrm_asset_item.name,vrm_asset_item.description,vrm_asset_item.ownernsguid,vrm_asset_item.productguid,vrm_asset_item.securityguid,vrm_asset_item.attributes,vrm_asset_item.alias,vrm_asset_item.createdby,vrm_asset_item.modifiedby,vrm_asset_item.createddate,vrm_asset_item.modifieddate,vrm_asset_item.productuninstalled,vrm_asset_item.state,vrm_asset_item.ismanaged,vrm_asset_item.resourceitemdeleted FROM vrm_asset_item %s",(clause ? clause : ""));
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"vrm_asset_item_select");
	return ret;
#else
	return 0;
#endif
}

int vrm_asset_item_select_record(DB *db, struct vrm_asset_item_record *rec, char *clause) {
#if vrm_asset_item_field_count > 0
	SQLRETURN ret;

	if ((ret = vrm_asset_item_select(db, clause)) != 0) return ret;
	if ((ret = vrm_asset_item_fetch_record(db, rec)) != 0) return ret;
	SQLCloseCursor(db->hstmt);
#endif
	return 0;
}

int vrm_asset_item_insert(DB *db, struct vrm_asset_item_record *rec) {
#if vrm_asset_item_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"INSERT INTO vrm_asset_item (guid,classguid,resourcetypeguid,name,description,ownernsguid,productguid,securityguid,attributes,alias,createdby,modifiedby,createddate,modifieddate,productuninstalled,state,ismanaged,resourceitemdeleted) VALUES (%x,%x,%x,'%s','%s',%x,%x,%x,%d,'%s','%s','%s','%s','%s',%d,'%s',%d,%d)",rec->guid,rec->classguid,rec->resourcetypeguid,rec->name,rec->description,rec->ownernsguid,rec->productguid,rec->securityguid,rec->attributes,rec->alias,rec->createdby,rec->modifiedby,rec->createddate,rec->modifieddate,rec->productuninstalled,rec->state,rec->ismanaged,rec->resourceitemdeleted);
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"vrm_asset_item_insert");
	printf("Offending query: %s\n", query);
	return ret;
#else
	return 0;
#endif
}

int vrm_asset_item_update_record(DB *db, struct vrm_asset_item_record *rec, char *clause) {
#if vrm_asset_item_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"UPDATE vrm_asset_item SET guid = %x,classguid = %x,resourcetypeguid = %x,name = '%s',description = '%s',ownernsguid = %x,productguid = %x,securityguid = %x,attributes = %d,alias = '%s',createdby = '%s',modifiedby = '%s',createddate = '%s',modifieddate = '%s',productuninstalled = %d,state = '%s',ismanaged = %d,resourceitemdeleted = %d",rec->guid,rec->classguid,rec->resourcetypeguid,rec->name,rec->description,rec->ownernsguid,rec->productguid,rec->securityguid,rec->attributes,rec->alias,rec->createdby,rec->modifiedby,rec->createddate,rec->modifieddate,rec->productuninstalled,rec->state,rec->ismanaged,rec->resourceitemdeleted);
	if (clause) {
		if (clause[0] != ' ') strcat(query," ");
		strcat(query,clause);
	}
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"vrm_asset_item_update_record");
	return ret;
#else
	return 0;
#endif
}

int vrm_asset_item_delete(DB *db, char *clause) {
#if vrm_asset_item_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"DELETE FROM vrm_asset_item %s",(clause ? clause : ""));
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"vrm_asset_item_delete");
	return ret;
#else
	return 0;
#endif
}

