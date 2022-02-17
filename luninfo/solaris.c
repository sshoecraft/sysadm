
#if (defined(__sun) && defined(__SVR4))
#include "luninfo.h"

#include <stdlib.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/scsi/impl/uscsi.h>
#include <sys/errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <sys/scsi/generic/commands.h>
#include <sys/scsi/impl/uscsi.h>
#include <errno.h>

void get_mp(list lp) {
	return;
}

list get_devs(void) {
	list lp;
	char name[1024];
	DIR *dirp;
	struct dirent *ent;
	struct stat sb;

	lp = list_create();
	if (!lp) {
		perror("list_create");
		return 0;
	}

	dprintf("opening /dev/rdsk\n");
	dirp = opendir("/dev/rdsk");
	if (!dirp) {
		perror("opendir(/dev/rdsk)");
		return lp;
	}
	dprintf("reading ents...\n");
	while((ent = readdir(dirp)) != 0) {
//		dprintf("ent: %s\n", ent->d_name);
		if (strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0) continue;
		if (strlen(ent->d_name) > 2 && ent->d_name[strlen(ent->d_name)-2] == 's' && ent->d_name[strlen(ent->d_name)-1] != '0') continue;
		if (strncmp(ent->d_name,"disk_",5) == 0) continue;
		dprintf("ent: %s\n", ent->d_name);
		name[0] = 0;
		strcat(name,"/dev/rdsk/");
		strncat(name,ent->d_name,sizeof(name)-strlen(name));
		if (stat(name,&sb) < 0) continue;
//		if (!S_ISBLK(sb.st_mode)) continue;
		dprintf("+++ADDING: name: %s\n", name);
		list_add(lp, name, strlen(name)+1);
//		break;
	}
	dprintf("closing...\n");
	closedir(dirp);

	dprintf("done!\n");
	return lp;
}

int do_ioctl(filehandle_t fd,unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
	struct uscsi_cmd sc;

	dprintf("fd: %d, cdb: %p, cdb_len: %d, buffer: %p, buflen: %d\n", fd, cdb, cdb_len, buffer, buflen);

	memset(&sc, 0, sizeof(sc));
	sc.uscsi_cdb = (caddr_t)cdb;
	sc.uscsi_cdblen = cdb_len;
	sc.uscsi_bufaddr = (caddr_t)buffer;
	sc.uscsi_buflen = buflen;
	sc.uscsi_flags = USCSI_SILENT|USCSI_ISOLATE|USCSI_READ;
    
	memset(buffer,0,buflen);
	if (ioctl(fd, USCSICMD, &sc)) {
		dprintf("ioctl error: %s\n", strerror(errno));
		return 1;
	}
	dprintf("sc.uscsi_status: %d\n", sc.uscsi_status);

	if (sc.uscsi_status) {
		return 1;
	}
//	bindump("ioctl",buffer,(buflen < 128 ? buflen : 128));

	return 0;
}

#endif
