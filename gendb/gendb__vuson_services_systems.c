
/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include <stdio.h>
#include <string.h>
#include "gendb__vuson_services_systems.h"

int _vuson_services_systems_fetch(DB *db) {
#if _vuson_services_systems_field_count > 0
	SQLRETURN ret;

	dprintf("Fetching data...\n");
	ret = SQLFetch(db->hstmt);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	if (ret != 100) ret = dberr(SQL_HANDLE_STMT,db->hstmt,"_vuson_services_systems_fetch");
	return ret;
#else
	return 0;
#endif
}

int _vuson_services_systems_fetch_record(DB *db, struct _vuson_services_systems_record *rec) {
#if _vuson_services_systems_field_count > 0
	SQLRETURN ret;

	if ((ret = _vuson_services_systems_fetch(db)) != 0) {
		SQLCloseCursor(db->hstmt);
		return ret;
	}
	memset(rec,0,sizeof(*rec));
	SQLGetData(db->hstmt, 1, SQL_C_CHAR, (SQLPOINTER) rec->services, sizeof(rec->services), 0);
	SQLGetData(db->hstmt, 2, SQL_C_CHAR, (SQLPOINTER) rec->supporting_systems, sizeof(rec->supporting_systems), 0);
	SQLGetData(db->hstmt, 3, SQL_C_BINARY, (SQLPOINTER) rec->svcguid, sizeof(rec->svcguid), 0);
	SQLGetData(db->hstmt, 4, SQL_C_BINARY, (SQLPOINTER) rec->sysguid, sizeof(rec->sysguid), 0);
	dprintf("----------------- _vuson_services_systems record -----------------\n");
	dprintf("services            :  %s\n",rec->services);
	dprintf("supporting_systems  :  %s\n",rec->supporting_systems);
	dprintf("svcguid             :  %x\n",rec->svcguid);
	dprintf("sysguid             :  %x\n",rec->sysguid);
#endif
	return 0;
}

int _vuson_services_systems_select(DB *db, char *clause) {
#if _vuson_services_systems_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"SELECT _vuson_services_systems.services,_vuson_services_systems.supporting_systems,_vuson_services_systems.svcguid,_vuson_services_systems.sysguid FROM _vuson_services_systems %s",(clause ? clause : ""));
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"_vuson_services_systems_select");
	return ret;
#else
	return 0;
#endif
}

int _vuson_services_systems_select_record(DB *db, struct _vuson_services_systems_record *rec, char *clause) {
#if _vuson_services_systems_field_count > 0
	SQLRETURN ret;

	if ((ret = _vuson_services_systems_select(db, clause)) != 0) return ret;
	if ((ret = _vuson_services_systems_fetch_record(db, rec)) != 0) return ret;
	SQLCloseCursor(db->hstmt);
#endif
	return 0;
}

int _vuson_services_systems_insert(DB *db, struct _vuson_services_systems_record *rec) {
#if _vuson_services_systems_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"INSERT INTO _vuson_services_systems (services,supporting_systems,svcguid,sysguid) VALUES ('%s','%s',%x,%x)",rec->services,rec->supporting_systems,rec->svcguid,rec->sysguid);
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"_vuson_services_systems_insert");
	printf("Offending query: %s\n", query);
	return ret;
#else
	return 0;
#endif
}

int _vuson_services_systems_update_record(DB *db, struct _vuson_services_systems_record *rec, char *clause) {
#if _vuson_services_systems_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"UPDATE _vuson_services_systems SET services = '%s',supporting_systems = '%s',svcguid = %x,sysguid = %x",rec->services,rec->supporting_systems,rec->svcguid,rec->sysguid);
	if (clause) {
		if (clause[0] != ' ') strcat(query," ");
		strcat(query,clause);
	}
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"_vuson_services_systems_update_record");
	return ret;
#else
	return 0;
#endif
}

int _vuson_services_systems_delete(DB *db, char *clause) {
#if _vuson_services_systems_field_count > 0
	char query[4096];
	SQLRETURN ret;

	sprintf(query,"DELETE FROM _vuson_services_systems %s",(clause ? clause : ""));
	dprintf("Executing query: %s\n", query);
	ret = SQLExecDirect(db->hstmt, (SQLCHAR *) query, SQL_NTS);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return 0;
	ret = dberr(SQL_HANDLE_STMT,db->hstmt,"_vuson_services_systems_delete");
	return ret;
#else
	return 0;
#endif
}

