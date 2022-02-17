
// Not required for win32
#ifndef __MINGW32__

#include "xplist.h"
#include <sys/stat.h>
#include <ctype.h>

char *esxdev_paths[] = {
	"/usr/sbin/esxcfg-vmhbadevs",
	"/usr/sbin/esxcfg-scsidevs",
	0
};

struct uid_vol {
	char uid[128];
	char vol[32];
};

void get_esxvols(list devs) {
	char hbacmd[1024],tmpfile[256];
	char line[128], dev[256];
	struct uid_vol uv, *uvp;
	struct xplist_entry *xp;
	struct stat sb;
	list t;
	int i;
	FILE *fp;
	char *p;

	if (get_path(hbacmd,esxdev_paths)) return;

	t = list_create();

	/* Try to change to vols */
	if (chdir("/vmfs/volumes") != 0) {
		perror("chdir");
		goto done;
	}

	concat_path(tmpfile,tempdir,"xplist.tmp");

	sprintf(temp,"ls /vmfs/volumes > %s", tmpfile);
	system(temp);
	fp = fopen(tmpfile,"r");
	if (fp) {
		while((fgets(line,sizeof(line),fp)) != 0) {
			trim(line);
//			printf("line: %s\n", line);
			if (lstat(line,&sb) != 0) {
//				perror("lstat");
				continue;
			}	
//			printf("ent: %s\n", line);
			if (!S_ISLNK(sb.st_mode)) continue;
			i = readlink(line, temp, sizeof(temp)-1);
			if (i >= 0) temp[i] = 0;
			uv.uid[0] = 0;
			strncat(uv.uid, temp, sizeof(uv.uid)-1);
			uv.vol[0] = 0;
			strncat(uv.vol, line, sizeof(uv.vol)-1);
//			printf("adding: uid: %s, vol: %s\n", uv.uid, uv.vol);
			list_add(t, &uv, sizeof(uv));
		}
		fclose(fp);
	}

	/* Get vmhbadevs output */
	sprintf(temp,"%s -m > %s 2>&1", hbacmd, tmpfile);
//	printf("cmd: %s\n", temp);
	system(temp);

	fp = fopen(tmpfile,"r");
	if (!fp) goto done;
	while ((fgets(temp,sizeof(temp),fp)) != 0) {
		strcpy(temp,stredit(temp,"TRIM,COMPRESS"));
//		printf("line: %s\n", temp);
		p = strele(2," ",temp);
		list_reset(t);
		while ((uvp = list_get_next(t)) != 0) {
			if (strcmp(uvp->uid,p) == 0) {
//				memset(&dev,0,sizeof(dev));
				dev[0] = 0;
				strncat(dev,strele(1," ",temp),sizeof(dev));
#define DD "/vmfs/devices/disks"
//printf("dev: %s\n", dev);
				if (strncmp(dev,DD,strlen(DD)) == 0) {
					p = strrchr(dev,'/');
					sprintf(dev,"/dev/disks%s",p);
				}
				if (strstr(dev,"naa") == 0) {
					for(p = dev; *p; p++) {
						if (isdigit(*p)) {
							*p = 0;
							break;
						}
					}
				} else {
					p = dev + (strlen(dev) - 1);
					if (isdigit(*p) && (*(p-1) == ':')) {
						*(p-1) = 0;
					}
				}
//printf("NEW dev: %s\n", dev);
				list_reset(devs);
				while((xp = list_get_next(devs)) != 0) {
					dprintf("dev: %s, xp->dev: %s\n", dev, xp->dev);
					if (strcmp(dev, xp->dev) == 0) {
						strcpy(xp->vol, uvp->vol);
						break;
					}
				}
				break;
			}
		}
	}

done:
	list_destroy(t);
	unlink(tmpfile);
	return;
}
#endif
