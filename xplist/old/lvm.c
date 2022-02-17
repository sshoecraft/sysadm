
#include "xplist.h"

#define SMART 1

#define PVINFO "/tmp/pvdisplay.out"

#if !SMART
static int get_vgname(char *dev, char *name) {
	char line[128];
	FILE *fp;

	sprintf(line,"%s %s > %s 2>&1", pvdisplay_path, dev, PVINFO);
	dprintf(("pvdisplay cmd: %s\n", line));
	system(line);

	name[0] = 0;
	fp = fopen(PVINFO,"r");
	if (fp) {
		while(fgets(line,sizeof(line),fp)) {
			strcpy(line,stredit(line,"TRIM,COMPRESS"));
			if (line[0] == 0) continue;
			dprintf(("pvinfo line: %s\n", line));
			if (strncmp(line,"VG Name",7)==0) {
				strcpy(name, strele(2," ",line));
				break;
			}
		}
		fclose(fp);
	}
	unlink(PVINFO);

	dprintf(("name: %s\n", name));
	return (name[0] != 0);
}
#endif

void get_vgnames(list devs) {
	struct xplist_entry *xp;
#if SMART
	struct vginfo *vg;
	struct pvinfo *pv;
	list vgs;
	int found;
#endif

	if (!strlen(pvdisplay_path)) return;

#if SMART
	vgs = get_vginfo(vgdisplay_path,"");
	if (!vgs) return;
#endif

	list_reset(devs);
	while((xp = list_get_next(devs)) != 0) {
#if SMART
		dprintf(("xp->dev: %s\n", xp->dev));
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
			dprintf(("xp->vol: %s\n", xp->vol));
			strcpy(xp->vol, vg->name);
		}
#else
		get_vgname(xp->dev,xp->vol);
#endif
	}
	return;
}
