
// Not required for win32
#ifndef __MINGW32__

#include "luninfo.h"
#include <errno.h>

char *esxdev_paths[] = {
	"/usr/sbin/esxcfg-vmhbadevs",
	"/usr/sbin/esxcfg-scsidevs",
	0
};

#if 0
static char line[4096], *devs_tmpfile;

list get_esxdevs(list lp) {
	char hbacmd[1024],*p;
	FILE *fp;

	if (get_path(hbacmd,esxdev_paths)) return lp;

	devs_tmpfile = get_tmpfile("esxvols");

	/* Get vmhbadevs output */
	sprintf(line,"%s -m > %s 2>&1", hbacmd, devs_tmpfile);
	dprintf("cmd: %s\n", line);
	system(line);

	fp = fopen(devs_tmpfile,"r");
	if (!fp) {
		printf("error: unable to open tmpfile: %s\n",strerror(errno));
		return lp;
	}
	while ((fgets(line,sizeof(line),fp)) != 0) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		dprintf("line: %s\n", line);
		p = strele(1," ",line);
		dprintf("p: %s\n", p);
		list_add(lp,p,strlen(p)+1);
	}
	fclose(fp);

	return lp;
}

void get_esxvols(list devs) {
	struct luninfo *info;
	char *p;
	FILE *fp;

	fp = fopen(devs_tmpfile,"r");
	if (!fp) return;
	while ((fgets(line,sizeof(line),fp)) != 0) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		dprintf("line: %s\n", line);
		p = strele(1," ",line);
		dprintf("p: %s\n", p);
		list_reset(devs);
		while((info = list_get_next(devs)) != 0) {
			dprintf("info->dev: %s\n", info->dev);
			if (strcmp(info->dev,p) == 0) {
				p = strele(-1," ",line);
				dprintf("volp: %s\n", p);
				info->vol[0] = 0;
				strncat(info->vol,p,sizeof(info->vol)-1);
				break;
			}
		}
	}
	fclose(fp);

	return;
}

#else
/* Old method */
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>

struct uid_vol {
	char uid[128];
	char vol[VOL_SIZE];
};

list get_esxdevs(list lp) {
	char *path, name[128];
	DIR *dirp;
	struct dirent *ent;
//	struct stat sb;

	/* XXX volname links point to /dev/disks */
	path = "/dev/disks";
	dprintf("path: %s\n", path);
	dirp = opendir(path);
	dprintf("dirp: %p\n",dirp);
	if (!dirp) {
		sprintf(name,"opendir(%s)",path);
		perror(name);
		return lp;
	}
	dprintf("opened\n");
	while((ent = readdir(dirp)) != 0) {
		dprintf("ent->d_name: %s\n", ent->d_name);
		if (strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0) continue;
		/* No partitions */
//		if (strstr(ent->d_name,":")) continue;
		if (strlen(ent->d_name) > 2 && ent->d_name[strlen(ent->d_name)-2] == ':') continue;
		/* No VMLs */
		if (strncmp(ent->d_name,"vml",3) == 0) continue;
		sprintf(name,"%s/",path);
		strncat(name,ent->d_name,sizeof(name)-strlen(name));
//		dprintf("testing: name: %s\n", name);
//		if (stat(name,&sb) < 0) continue;
//		if (!S_ISBLK(sb.st_mode)) continue;
		dprintf("name: %s\n", name);
		list_add(lp, name, strlen(name)+1);
	}
	closedir(dirp);
	return lp;
}

void get_esxvols(list devs) {
//	char hbacmd[1024],tmpfile[256];
	char temp[1024],hbacmd[1024],*tmpfile;
	char line[128], dev[256];
	struct uid_vol uv, *uvp;
	struct luninfo *info;
	struct stat sb;
	list t;
	int i;
	FILE *fp;
	char *p;

	tmpfile = 0;

	if (get_path(hbacmd,esxdev_paths)) return;

	t = list_create();

	/* Try to change to vols */
	if (chdir("/vmfs/volumes") != 0) {
		perror("chdir");
		goto done;
	}

//	concat_path(tmpfile,tempdir,"xplist.tmp");
	tmpfile = get_tmpfile("esxvols");

	sprintf(temp,"ls /vmfs/volumes > %s", tmpfile);
	system(temp);
	fp = fopen(tmpfile,"r");
	if (fp) {
		while((fgets(line,sizeof(line),fp)) != 0) {
			trim(line);
//			dprintf("line: %s\n", line);
			if (lstat(line,&sb) != 0) {
//				perror("lstat");
				continue;
			}	
			if (!S_ISLNK(sb.st_mode)) continue;
			i = readlink(line, temp, sizeof(temp)-1);
			if (i >= 0) temp[i] = 0;
			uv.uid[0] = 0;
			strncat(uv.uid, temp, sizeof(uv.uid)-1);
			uv.vol[0] = 0;
			strncat(uv.vol, line, sizeof(uv.vol)-1);
			dprintf("adding: uid: %s, vol: %s\n", uv.uid, uv.vol);
			list_add(t, &uv, sizeof(uv));
		}
		fclose(fp);
	}

	/* Get vmhbadevs output */
	sprintf(temp,"%s -m > %s 2>&1", hbacmd, tmpfile);
	dprintf("cmd: %s\n", temp);
	system(temp);

	fp = fopen(tmpfile,"r");
	if (!fp) goto done;
	while ((fgets(temp,sizeof(temp),fp)) != 0) {
		strcpy(temp,stredit(temp,"TRIM,COMPRESS"));
		dprintf("line: %s\n", temp);
		p = strele(2," ",temp);
		dprintf("p: %s\n", p);
		list_reset(t);
		while ((uvp = list_get_next(t)) != 0) {
			dprintf("uvp->uid: %s\n",uvp->uid);
			if (strcmp(uvp->uid,p) == 0) {
//				memset(&dev,0,sizeof(dev));
				dev[0] = 0;
				strncat(dev,strele(1," ",temp),sizeof(dev));
#define ESXDD "/vmfs/devices/disks"
				dprintf("dev: %s\n", dev);
//get_esxvols(195): dev: /vmfs/devices/disks/t10.usodtlvas0042Dvg012Dlv020000000000000000000000000000:1
//get_esxvols(213): NEW dev: /dev/disks/t

				if (strncmp(dev,ESXDD,strlen(ESXDD)) == 0) {
					p = strrchr(dev,'/');
					sprintf(dev,"/dev/disks%s",p);
					dprintf("FIXED dev: %s\n", dev);
				}
#if 0
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
#endif
				if (strlen(dev) > 2) {
					p = dev + strlen(dev);
					if (*(p-2) == ':') {
						*(p-2) = 0;
						dprintf("NEW dev: %s\n", dev);
					}
				}
				list_reset(devs);
				while((info = list_get_next(devs)) != 0) {
					dprintf("dev: %s, info->dev: %s\n", dev, info->dev);
					if (strcmp(dev, info->dev) == 0) {
						strcpy(info->vol, uvp->vol);
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

#endif
