
#include "xplist.h"
#include <ctype.h>

#define SMART 1

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

#define VGSTART "--- Volume group"
#define LVSTART "--- Logical volume"
#define PVSTART "--- Physical volume"

enum states {
	NONE,
	IN_VG,
	IN_LV,
	IN_PV
};

list get_vginfo(char *vgdisplay_path, char *name) {
	char line[1024], *p;
	struct vginfo new_vg;
	struct lvinfo new_lv;
	struct pvinfo new_pv;
	char tmp[] = "/tmp/vginfoXXXXXX";
	int fd,state,have_vg;
	list vgs;
	FILE *fp;

	vgs = list_create();

	fd = mkstemp(tmp);
	if (fd < 0) {
		perror("get_vginfo: mkstemp");
		return 0;
	}
	dprintf("tmpfile: %s\n", tmp);
	sprintf(line,"%s -v %s > %s 2>&1", vgdisplay_path, name, tmp);
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
		dprintf("line(%d): %s\n", strlen(line),line);
		if (!strlen(line)) continue;
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
				memset(&new_pv,0,sizeof(new_pv));
				strncpy(new_pv.name,strele(2," ",line),sizeof(new_pv.name)-1);
#if defined(__linux__)
			} else if (strncmp(line,"Total PE",8)==0) {
				new_pv.total = atoi(strele(5," ",line));
				new_pv.free = atoi(strele(7," ",line));
				new_pv.size = (long long)new_pv.total * (long long)new_vg.pe.size;
				p = new_pv.name;
				if (isdigit(p[strlen(p)-1])) p[strlen(p)-1] = 0;
				dprintf("adding PV: %s, size: %lld\n", new_pv.name, new_pv.size);
				list_add(new_vg.pv,&new_pv,sizeof(new_pv));
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
	unlink(tmp);
	return vgs;
}

void free_vginfo(struct vginfo *info) {
	list_destroy(info->lv);
	list_destroy(info->pv);
	free(info);
}

#define PVINFO "/tmp/pvdisplay.out"

#if !SMART
char *pvdisplay_paths[] = {
	"/sbin/pvdisplay",
	"/usr/sbin/pvdisplay",
	0
};

static char pvdisplay_path[1024];
static int get_vgname(char *dev, char *name) {
	char line[128];
	FILE *fp;

	name[0] = 0;

	sprintf(line,"%s %s > %s 2>&1", pvdisplay_path, dev, PVINFO);
	dprintf("pvdisplay cmd: %s\n", line);
	system(line);

	fp = fopen(PVINFO,"r");
	if (fp) {
		while(fgets(line,sizeof(line),fp)) {
			strcpy(line,stredit(line,"TRIM,COMPRESS"));
			if (line[0] == 0) continue;
			dprintf("pvinfo line: %s\n", line);
			if (strncmp(line,"VG Name",7)==0) {
				strcpy(name, strele(2," ",line));
				break;
			}
		}
		fclose(fp);
	}
	unlink(PVINFO);

	dprintf("name: %s\n", name);
	return (name[0] != 0);
}
#endif

static char vgdisplay_path[1024];

static char *vgdisplay_paths[] = {
	"/sbin/vgdisplay",
	"/usr/sbin/vgdisplay",
	0
};

void get_vgnames(list devs) {
	struct xplist_entry *xp;
#if SMART
	struct vginfo *vg;
	struct pvinfo *pv;
	list vgs;
	int found;
#endif

#if SMART
	if (get_path(vgdisplay_path,vgdisplay_paths)) return;
	vgs = get_vginfo(vgdisplay_path,"");
	if (!vgs) return;
#else
	if (get_path(pvdisplay_path,pvdisplay_paths)) return;
#endif

	list_reset(devs);
	while((xp = list_get_next(devs)) != 0) {
#if SMART
		dprintf("xp->dev: %s\n", xp->dev);
		found = 0;
		list_reset(vgs);
		while((vg = list_get_next(vgs)) != 0) {
			list_reset(vg->pv);
			while((pv = list_get_next(vg->pv)) != 0) {
				if (strcmp(xp->dev, pv->name)==0) {
					found = 1;
					break;
				}
			}
			if (found) break;
		}
		if (found) {
			dprintf("xp->vol: %s\n", xp->vol);
			strcpy(xp->vol, vg->name);
		}
#else
		get_vgname(xp->dev,xp->vol);
#endif
	}
	return;
}
