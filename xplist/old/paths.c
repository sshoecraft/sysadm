
#include "xplist.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define VMHBADEVS "/usr/sbin/esxcfg-vmhbadevs"
#define VS4DEVS "/usr/sbin/esxcfg-scsidevs"

char hbacmd[1024];
char pvdisplay_path[1024];
char vgdisplay_path[1024];
char vxdisk_path[1024];

char *pvdisplay_paths[] = {
	"/sbin/pvdisplay",
	"/usr/sbin/pvdisplay",
	0
};

char *vgdisplay_paths[] = {
	"/sbin/vgdisplay",
	"/usr/sbin/vgdisplay",
	0
};

char *vxdisk_paths[]= {
	"/sbin/vxdisk",
	"/usr/sbin/vxdisk",
	0
};

static void get_path(char *var, char **paths) {
	int i;

	*var = 0;
	for(i=0; paths[i]; i++) {
//		printf("paths[%d]: %s\n", i, paths[i]);
        	if (access(paths[i],X_OK) == 0) {
			strcpy(var, paths[i]);
			break;
		}
	}
}

void get_paths(void) {
	get_path(pvdisplay_path, pvdisplay_paths);
	get_path(vgdisplay_path, vgdisplay_paths);
	get_path(vxdisk_path, vxdisk_paths);
}
