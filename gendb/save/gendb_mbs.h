
#ifndef __MBS_H
#define __MBS_H

/*
*** this file is automatically generated - DO NOT MODIFY
*/

#include "db.h"

/* Structure definition for the mbs table */
struct mbs_record {
	int mbsid;
	char site[51];
	char build_week[51];
	int wssid;
	char wssmbslist[51];
	char quarter[51];
	int priority;
	char target_build_completion_date[17];
	char deploy_team_status[51];
	unsigned char san_alloc;
	char hostname[51];
	char virtual_vmware_hosts[51];
	char blade_enclosure_host[51];
	char blade_guest_assigned[51];
	char domain[51];
	char business_unit[51];
	char application[101];
	char instance[51];
	char server_model[101];
	char server_or_service[51];
	char cpu[51];
	char memory[51];
	char pod[51];
	char rack_location[51];
	char u_location[51];
	char pdu_connection[51];
	char os_image[51];
	char win_os_role[51];
	char windows_domain[51];
	char cluster_type[51];
	char servers_in_cluster[51];
	char clustered_with[201];
	char cluster_virtual_name[101];
	char unixsql_cluster_name[2001];
	int lan_primary;
	int lan_console;
	int lan_backup;
	int lan_heartbt;
	int lan_iconnect;
	int num_fiber_conns;
	char primary_subnet[51];
	char network_segment[51];
	char primary_ip_address_or_dhcp[51];
	char console_ip_address_or_dhcp[51];
	char backup_ip_address_or_dhcp[51];
	char cluster_virtual_ip[51];
	char cluster_heartbeat[51];
	char cluster_interconnect[51];
	int san_t1ded64;
	int san_t1ded16;
	int san_t1ded4;
	int san_t1shr64;
	int san_t1shr16;
	int san_t1shr4;
	int san_t2ded64;
	int san_t2shr64;
	int bus_copy;
	char volume_config[16384];
	char serial_number[51];
	char wwn_p1[51];
	char wwn_a1[51];
	char wwn_t1[51];
	char wwn_p2[51];
	char wwn_a2[51];
	char wwn_t2[51];
	char wwn_p3[51];
	char wwn_a3[51];
	char wwn_t3[51];
	char wwn_p4[51];
	char wwn_a4[51];
	char wwn_t4[51];
	char wwn_p5[51];
	char wwn_a5[51];
	char wwn_t5[51];
	char wwn_p6[51];
	char wwn_a6[51];
	char wwn_t6[51];
	char wwn_p7[51];
	char wwn_a7[51];
	char wwn_t7[51];
	char wwn_p8[51];
	char wwn_a8[51];
	char wwn_t8[51];
	char mb_scheduler_status[256];
	char comments[1051];
	char request_contact[256];
	char building_id[51];
	char modulecell[51];
	char platform[51];
	char handoff_date[17];
	char handoffsent[51];
	char dcc_request_number[51];
	int plan_service_id;
	char modified[17];
	char modified_by[51];
	int ovsdid;
	char pmassigned[51];
	unsigned char deleted;
	int issuecounts;
	char created[17];
	char createdby[51];
	char builder[51];
	char clustername[11];
	char infraservice[51];
	unsigned char addedbyuser;
	char sql_virtual_ip[201];
	char secondary_ip_address_or_dhcp[51];
	char secondary_segment[51];
	char cluster_backup_virtual_ip[51];
	char cluster_backup_virtual_name[51];
	int san_t2ded510;
	int san_t2shr510;
	unsigned char decommissioned;
	char rfc[21];
	char expecteddate[17];
	char actualdate[17];
	char newhostname[11];
	int san_t1cmd1;
	char backplane[33];
	int san_l1shr14;
	int san_l1ded14;
	char location_type[11];
	char ppmid[51];
	char project_type[51];
	char build_rfc[51];
	char need_by_date[24];
	int esx_console;
	char cluster_heartbeat_2[51];
	char generation[51];
	int san_t3total;
	char mac_address[51];
	char need_by_week[51];
	char cu_l_dev[16384];
	char scan_ip_1[51];
	char scan_ip_2[51];
	char scan_ip_3[51];
	int san_t2ded16;
	int san_t2ded4;
	int san_t2shr16;
	int san_t2shr4;
	char ngdc_generation[51];
	unsigned long long timestamp;
	char fencing_lun1[129];
	char service_ci[51];
	char infra_gen[51];
	int t1;
	int t2;
	int t3;
	char dccpservice[51];
	unsigned char buildtool_extracted;
	unsigned char top200;
	char coordinator_notes[256];
	char coe_profile[51];
	char compute_space[51];
};
int mbs_fetch(DB *db);
int mbs_fetch_record(DB *db, struct mbs_record *rec);
int mbs_select(DB *db, char *clause);
int mbs_select_record(DB *db, struct mbs_record *rec, char *clause);
int mbs_insert(DB *db, struct mbs_record *rec);
int mbs_update_record(DB *db, struct mbs_record *rec, char *clause);
int mbs_delete(DB *db, char *clause);

#endif /* __MBS_H */
