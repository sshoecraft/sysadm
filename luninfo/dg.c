
#include "luninfo.h"
#include <fcntl.h>

struct dev_vol {
	char dev[DEV_SIZE];
	char vol[VOL_SIZE];
};

static char *vxdisk_paths[]= {
#ifdef __MINGW32__
	"vxdisk.exe",
#else
	"/sbin/vxdisk",
	"/usr/sbin/vxdisk",
#endif
	0
};

void get_dgnames(list devs, list mpdevs) {
//	char vxdisk_path[1024],tmpfile[256],line[128];
	char vxdisk_path[1024],*tmpfile,line[128];
	FILE *fp;
	struct luninfo *info;
	struct dev_vol dv, *dvp;
	int fd;
	list dgs;

	if (get_path(vxdisk_path,vxdisk_paths)) return;
	printf("vxdisk_path: %s\n", vxdisk_path);

	dgs = list_create();

//	concat_path(tmpfile,tempdir,"vxdisk.tmp");
	tmpfile = get_tmpfile("vxdisk");
	fd = open(tmpfile,O_RDWR|O_CREAT);
	if (fd < 0) {
		perror("get_dgnames: open");
		return;
	}
	dprintf("tmpfile: %s\n", tmpfile);
	sprintf(line,"%s list > %s 2>&1", vxdisk_path, tmpfile);
	dprintf("cmd: %s", line);
	system(line);

	fp = fdopen(fd,"r");
	if (!fp) {
		perror("get_dgnames: fdopen");
		goto done;
	}
	while(fgets(line,sizeof(line),fp) != 0) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		dprintf("line: %s\n", line);
		dv.dev[0] = 0;
		strcat(dv.dev,"/dev/");
#if defined(__hpux)
		strcat(dv.dev,"rdsk/");
#endif
		strcat(dv.dev,strele(0," ",line));
		dv.vol[0] = 0;
		strcat(dv.vol,strele(3," ",line));
		if (strcmp(dv.vol,"-")==0) dv.vol[0] = 0;
		dprintf("dev: %s, vol: %s\n", dv.dev,dv.vol);
		list_add(dgs, &dv, sizeof(dv));
	}

	list_reset(devs);
	while((info = list_get_next(devs)) != 0) {
		list_reset(dgs);
		while((dvp = list_get_next(dgs)) != 0) {
			if (strcmp(info->dev, dvp->dev) == 0) {
				strcpy(info->vol, dvp->vol);
				break;
			}
		}
	}
done:
	close(fd);
	if (fp) fclose(fp);
	unlink(tmpfile);
	list_destroy(dgs);
}
