
#if defined(__linux__) || defined(__hpux)

#include "luninfo.h"
#include <ctype.h>
#include <fcntl.h>

#define PVNAME_SIZE DEV_SIZE
#define LVNAME_SIZE 16
#define VGNAME_SIZE 16

struct pvinfo {
	char name[DEV_SIZE];
	unsigned long long size;
	int total;
	int free;
};

struct lvinfo {
	char name[LVNAME_SIZE];
	unsigned long long size;
};

struct vginfo {
	char name[VGNAME_SIZE];
	unsigned long long size;
	struct {
		int size;
		int total;
		int alloc;
		int free;
	} pe;
	list pv;
	list lv;
};

static int v2b(char *string, char *units) {
	double val;
	int vmb;

	dprintf("string: %s, units: %s\n", string, units);

	val = strtod(string,0);
	dprintf("val: %lf\n", val);
	if (strcmp(units,"TB")==0)
		val *= 1099511627776.0;
	else if (strcmp(units,"GB")==0)
		val *= 1073741824.0;
	else if (strcmp(units,"MB")==0)
		val *= 1048576.0;
	else if (strcmp(units,"KB")==0)
		val *= 1024.0;
	dprintf("val in bytes: %f\n", val);

	/* Convert to MB */
	vmb = val / 1048576;

	dprintf("returning: %d\n", vmb);
	return vmb;
}

#if defined(__linux__)
static unsigned long long getsize(char *line) {
	char val[16],units[8];

	strcpy(val, strele(2," ",line));
	strcpy(units, strele(3," ",line));
	return v2b(val,units);
}
#define PE_INDEX 4
#elif defined(__hpux)
static unsigned long long getsize(char *line) {
	char val[16];

	strcpy(val, strele(3," ",line));
	return v2b(val,"MB");
}
#define PE_INDEX 2
#endif

#if 0
void dump_vg(struct vginfo *vg) {
	struct lvinfo *lv;
	struct pvinfo *pv;

	printf("Name: %s\n", vg->name);
	printf("Size: %lld\n", vg->size);
	printf("PE Size: %d\n", vg->pe.size);
	printf("PE Total: %d\n", vg->pe.total);
	printf("PE Alloc: %d\n", vg->pe.alloc);
	printf("PE Free: %d\n", vg->pe.free);
	printf("Logical Volumes:\n");
	list_reset(vg->lv);
	while((lv = list_get_next(vg->lv)) != 0) {
		printf("\tName: %s\n", lv->name);
		printf("\tSize: %lld\n", lv->size);
	}
	printf("Physical Volumes:\n");
	list_reset(vg->pv);
	while((pv = list_get_next(vg->pv)) != 0) {
		printf("\tName: %s\n", pv->name);
		printf("\tTotal PE: %d\n", pv->total);
		printf("\tFree PE: %d\n", pv->free);
	}
}
#endif

#define VGSTART "--- Volume group"
#define LVSTART "--- Logical volume"
#define PVSTART "--- Physical volume"

enum states {
	NONE,
	IN_VG,
	IN_LV,
	IN_PV
};

list get_vginfo(char *vgdisplay_path, char *name, list mpdevs) {
//	char line[1024], tmpfile[256], *p;
	char line[1024], *tmpfile, *p;
	struct vginfo new_vg;
	struct lvinfo new_lv;
	struct pvinfo new_pv;
	int fd,state,have_vg;
	list vgs;
	FILE *fp;
	int len;

	vgs = list_create();

//	concat_path(tmpfile,tempdir,"lvminfo.tmp");
	tmpfile = get_tmpfile("vginfo");
//	printf("tmpfile: %s\n", tmpfile);
	fd = open(tmpfile,O_RDWR|O_CREAT);
	if (fd < 0) {
		perror("get_vginfo: open");
		return 0;
	}
	dprintf("tmpfile: %s\n", tmpfile);
	sprintf(line,"%s -v %s > %s 2>&1", vgdisplay_path, name, tmpfile);
	dprintf("cmd: %s", line);
	system(line);

	fp = fdopen(fd,"r");
	if (!fp) {
		perror("get_vginfo: fdopen");
		goto done;
	}
	state = NONE;
	have_vg = 0;
	while(fgets(line,sizeof(line),fp) != 0) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		len = strlen(line);
		dprintf("line(%d): %s\n", len,line);
		if (!len) continue;
		if (strncmp(line,VGSTART,strlen(VGSTART))==0) {
			if (have_vg) {
				new_vg.size = (unsigned long long)new_vg.pe.total * (unsigned long long)new_vg.pe.size;
				dprintf("adding VG: %s, size: %lld\n", new_vg.name, new_vg.size);
				list_add(vgs,&new_vg,sizeof(new_vg));
			}
			memset(&new_vg,0,sizeof(new_vg));
			new_vg.lv = list_create();
			new_vg.pv = list_create();
			have_vg = 1;
			state = IN_VG;
		} else if (have_vg && strncmp(line,LVSTART,strlen(LVSTART))==0) {
			state = IN_LV;
		} else if (have_vg && strncmp(line,PVSTART,strlen(PVSTART))==0) {
			state = IN_PV;
		}
		dprintf("state: %d\n", state);
		switch(state) {
		case IN_VG:
			if (strncmp(line,"VG Name",7)==0) {
				strncpy(new_vg.name,strele(2," ",line),sizeof(new_vg.name)-1);
			} else if (strncmp(line,"PE Size",7)==0) {
				new_vg.pe.size = getsize(line);
			} else if (strncmp(line,"Total PE",8)==0) {
				new_vg.pe.total = atoi(strele(2," ",line));
			} else if (strncmp(line,"Alloc PE",8)==0) {
				new_vg.pe.alloc = atoi(strele(PE_INDEX," ",line));
			} else if (strncmp(line,"Free PE",7)==0) {
				new_vg.pe.free = atoi(strele(PE_INDEX," ",line));
			}
			break;
		case IN_LV:
			if (strncmp(line,"LV Name",7)==0) {
				memset(&new_lv,0,sizeof(new_lv));
				strncpy(new_lv.name,strele(2," ",line),sizeof(new_lv.name)-1);
			} else if (strncmp(line,"LV Size",7)==0) {
				new_lv.size = getsize(line);
				dprintf("adding LV: %s, size: %lld\n", new_lv.name, new_lv.size);
				list_add(new_vg.lv,&new_lv,sizeof(new_lv));
			}
			break;
		case IN_PV:
			if (strncmp(line,"PV Name",7)==0) {
				dprintf("2: %s\n", strele(2," ",line));
				memset(&new_pv,0,sizeof(new_pv));
				strncpy(new_pv.name,strele(2," ",line),sizeof(new_pv.name)-1);
#if defined(__linux__)
			} else if (strncmp(line,"Total PE",8)==0) {
				list mp_devs;

				new_pv.total = atoi(strele(5," ",line));
				new_pv.free = atoi(strele(7," ",line));
				new_pv.size = (long long)new_pv.total * (long long)new_vg.pe.size;
				if (is_mp(mpdevs,new_pv.name)) {
					mp_devs = get_mp_devs_from_name(mpdevs,new_pv.name);
					list_reset(mp_devs);
					while((p = list_get_next(mp_devs)) != 0) {
						strcpy(new_pv.name,p);
						dprintf("adding PV: %s, size: %lld\n", new_pv.name, new_pv.size);
						list_add(new_vg.pv,&new_pv,sizeof(new_pv));
					}
				} else {
					p = new_pv.name;
					if (isdigit(p[strlen(p)-1]) && !strchr(p,'-')) p[strlen(p)-1] = 0;
					dprintf("adding PV: %s, size: %lld\n", new_pv.name, new_pv.size);
					list_add(new_vg.pv,&new_pv,sizeof(new_pv));
				}

			}
#elif defined(__hpux)
			} else if (strncmp(line,"Total PE",8)==0) {
				new_pv.total = atoi(strele(2," ",line));
			} else if (strncmp(line,"Free PE",7)==0) {
				new_pv.free = atoi(strele(2," ",line));
				new_pv.size = (long long)new_pv.total * (long long)new_vg.pe.size;
				dprintf("adding PV: %s, size: %lld\n", new_pv.name, new_pv.size);
				list_add(new_vg.pv,&new_pv,sizeof(new_pv));
			}
#endif
			break;
		default:
			break;
		}
	}
	if (have_vg) {
		new_vg.size = (unsigned long long)new_vg.pe.total * (unsigned long long)new_vg.pe.size;
		dprintf("adding VG: %s, size: %lld\n", new_vg.name, new_vg.size);
		list_add(vgs,&new_vg,sizeof(new_vg));
	}

done:
	close(fd);
	if (fp) fclose(fp);
	unlink(tmpfile);
	return vgs;
}

void free_vginfo(struct vginfo *info) {
	list_destroy(info->lv);
	list_destroy(info->pv);
	free(info);
}

static char *vgdisplay_paths[] = {
	"/sbin/vgdisplay",
	"/usr/sbin/vgdisplay",
	0
};

void get_vgnames(list devs, list mpdevs) {
	char vgdisplay_path[1024];
	struct luninfo *info;
	struct vginfo *vg;
	struct pvinfo *pv;
	list vgs;
	int found;

	if (get_path(vgdisplay_path,vgdisplay_paths)) return;

	vgs = get_vginfo(vgdisplay_path,"",mpdevs);
	if (!vgs) return;

	list_reset(devs);
	while((info = list_get_next(devs)) != 0) {
		dprintf("info->dev: %s\n", info->dev);
		found = 0;
		list_reset(vgs);
		while((vg = list_get_next(vgs)) != 0) {
			list_reset(vg->pv);
			while((pv = list_get_next(vg->pv)) != 0) {
				if (strcmp(info->dev, pv->name)==0) {
					found = 1;
					break;
				}
			}
			if (found) break;
		}
		if (found) {
			dprintf("info->vol: %s\n", info->vol);
			strcpy(info->vol, vg->name);
		}
	}
	return;
}
#endif
