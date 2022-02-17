
#if defined(__linux__)
#include "luninfo.h"
#include "esx.h"

#include <stdlib.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <sys/errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>

#define MPATH "/sbin/multipath"
#define CHECK_UNAME 0
//#define SCAN_ALL 1

void *_mknewdev(void) {
	struct mpdevs_entry *newdev;

	newdev = malloc(sizeof(struct mpdevs_entry));
	if (!newdev) {
		perror("malloc");
		exit(1);
	}
	memset(newdev,0,sizeof(*newdev));
	newdev->devs = list_create();
	if (!newdev->devs) {
		printf("error: unable to create newdev.devs list!\n");
		exit(1);
	}
	return newdev;
}

void get_mp(list mpdevs) {
//	char tmpfile[256],line[1024],*p;
	char temp[1024],line[1024],*p, *tmpfile;
	FILE *fp;
	int fd,start,c;
	struct mpdevs_entry newdev;
	struct utsname uts;

	if (access(MPATH,X_OK) != 0) return;

	uname(&uts);
	dprintf("sysname: %s, release: %s\n", uts.sysname, uts.release);

//	concat_path(tmpfile,tempdir,"mpinfo.tmp");
	tmpfile = get_tmpfile("mpinfo");
	fd = open(tmpfile,O_RDWR|O_CREAT);
	if (fd < 0) {
		perror("get_mp: open");
		return;
	}
	dprintf("tmpfile: %s\n", tmpfile);
	sprintf(line,"%s -l > %s 2>&1", MPATH, tmpfile);
	dprintf("cmd: %s\n", line);
	system(line);

	fp = fdopen(fd,"r");
	if (!fp) {
		perror("get_mp: fdopen");
		goto done;
	}
	start = 0;
	memset(&newdev,0,sizeof(newdev));
	newdev.devs = list_create();
	while(fgets(line,sizeof(line),fp) != 0) {
		dprintf("line(1): %s\n", line);
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		dprintf("line(2): %s\n", line);
		dprintf("newdev.devs: %p\n", newdev.devs);
		if (strlen(line) == 0) {
			if (start) {
				dprintf("adding to mpdevs: newdev.devs: %p\n", newdev.devs);
				list_add(mpdevs,&newdev,sizeof(newdev));
				memset(&newdev,0,sizeof(newdev));
				newdev.devs = list_create();
				dprintf("newdev.devs(C): %p\n", newdev.devs);
			}
			start = 0;
			continue;
		} else if (strchr(line,'(') && strchr(line,')')) {
			if (start) {
				dprintf("adding to mpdevs: newdev.devs: %p\n", newdev.devs);
				list_add(mpdevs,&newdev,sizeof(newdev));
				memset(&newdev,0,sizeof(newdev));
				newdev.devs = list_create();
				dprintf("newdev.devs(C): %p\n", newdev.devs);
			}
			start = 0;
		}
		dprintf("start: %d\n", start);
		if (start) {
//			if (strncmp(line,"\\_",2) == 0 && strchr(line,':')) {
			if (strchr(line,':')) {
				char dev[16];
//				struct stat sb;

				p = strele(2," ",line);
				if (!strlen(p)) continue;
				sprintf(dev,"/dev/%s",p);
				dprintf("dev: %s\n", dev);
//				if (stat(dev,&sb) == 0) {
				if (access(dev,R_OK) == 0) {
					dprintf("adding new dev...\n");
					dprintf("newdev.devs(BA): %p\n", newdev.devs);
					list_add(newdev.devs,dev,strlen(dev)+1);
					dprintf("newdev.devs(AA): %p\n", newdev.devs);
				}
			}
		} else {
			char temp2[256];

			dprintf("*** START ***\n");

			/* See if the 3rd item is dm-[0-9] */
			p = strele(2," ",line);
			if (strncmp(p,"dm-",3) == 0) {
				dprintf("p: %s\n", p);
//				sprintf(temp,"/dev/mpath/%s", p);
//				sprintf(newdev.name,"/dev/%s",p);
				sprintf(temp,"/dev/%s",p);
				dprintf("temp(%d): %s\n", (int)strlen(temp), temp);
				newdev.name[0] = 0;
				strncat(newdev.name,temp,sizeof(newdev.name)-1);
				/* XXX get the 1st one as alt_name */
				p = strele(0," ",line);
				sprintf(newdev.alt_name,"/dev/mapper/%s",p);
				dprintf("name: %s, alt_name: %s\n", newdev.name, newdev.alt_name);
			} else {
				/* Nope, see if the 1st item /dev/<1st item> exists and is a symlink */
				p = strele(0," ",line);
				dprintf("p: %s\n", p);
				/* readlink /dev/mpath/<p> */
				sprintf(temp,"/dev/mpath/%s", p);
				c = readlink(temp,temp2,sizeof(temp2));
				dprintf("c: %d\n", c);
				if (c < 0) {
					printf("start? %s\n", line);
				} else {
					temp2[c] = 0;
					dprintf("link: %s\n", temp2);
//					sprintf(newdev.name,"/dev/%s",&temp2[3]);
					sprintf(temp,"/dev/%s",&temp2[3]);
					dprintf("temp(%d): %s\n", (int)strlen(temp), temp);
					newdev.name[0] = 0;
					strncat(newdev.name,temp,sizeof(newdev.name)-1);
					dprintf("name: %s\n", newdev.name);
				}
			}
			dprintf("newdev.devs(S): %p\n", newdev.devs);
			start = 1;
		}
	}
	fclose(fp);
	if (newdev.name[0] && newdev.devs) list_add(mpdevs,&newdev,sizeof(newdev));
done:

	return;
}

list get_devs(void) {
	list lp;
	FILE *fp;
	char line[1024], name[64], *p;
#ifdef SCAN_ALL
	DIR *dirp;
	struct dirent *ent;
	struct stat sb;
#endif
	struct utsname uts;
#if CHECK_UNAME
	int ver;
#endif

	lp = list_create();
	if (!lp) {
		perror("list_create");
		return 0;
	}

	uname(&uts);
#if CHECK_UNAME
	dprintf("sysname: %s, release: %s\n", uts.sysname, uts.release);
	name[0] = uts.release[0];
	name[1] = 0;
	dprintf("name: %s\n", name);
	ver = uts.release[0] - '0';
//	ver = atoi(name);
	dprintf("ver: %d\n", ver);
	dprintf("uts.sysname: %s, ver: %d\n", uts.sysname, ver);
	if (strcmp(uts.sysname,"VMkernel") == 0 && (ver >= 5 || access("/vmfs/volumes",R_OK) == 0) ) {
		dprintf("getting vmware devs...\n");
		return get_esxdevs(lp);
	}
#else
	if (access("/vmfs/devices/disks",R_OK) ==0) return get_esxdevs(lp);
#endif

#ifdef SCAN_ALL
	dprintf("scanning all\n");
	fp = 0;
	line[0] = 0;
	p = 0;
	dprintf("opening /dev\n");
	dirp = opendir("/dev");
	if (!dirp) {
		perror("opendir(/dev)");
		return lp;
	}
	dprintf("reading ents...\n");
	while((ent = readdir(dirp)) != 0) {
		if (strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0) continue;
		name[0] = 0;
		strcat(name,"/dev/");
		strncat(name,ent->d_name,sizeof(name)-strlen(name));
		if (stat(name,&sb) < 0) continue;
		if (!S_ISBLK(sb.st_mode)) continue;
		dprintf("name: %s\n", name);
		list_add(lp, name, strlen(name)+1);
	}
	dprintf("closing...\n");
	closedir(dirp);
#else
	dprintf("scanning parts\n");
	fp = fopen("/proc/partitions","r");
	if (!fp) {
		perror("fopen /proc/partitions");
		exit(1);
	}
	dprintf("reading parts...\n");
	while(fgets(line,sizeof(line),fp)) {
		dprintf("line: %s\n", line);
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		dprintf("line(2): %s\n", line);
		if (!strlen(line)) continue;
		dprintf("partitions line: %s\n", line);

		p = strele(3," ",line);
		dprintf("dev: %s\n", p);
		if (!strlen(p)) continue;
		if (strncmp(p,"psd",3) == 0 && isdigit(p[strlen(p)-1]));
		else if (strncmp(p,"nvme",4) != 0 || isdigit(p[strlen(p)-1]));
		else if (strncmp(p,"sd",2) != 0 || isdigit(p[strlen(p)-1]))
			continue;
		name[0] = 0;
		strcat(name, "/dev/");
		strcat(name, p);
		dprintf("dev: %s\n", name);
		list_add(lp, name, strlen(name)+1);
	}
	fclose(fp);
#endif

	dprintf("done!\n");
	return lp;
}

int do_ioctl(int fd, unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
	unsigned char sense_buffer[32];
	sg_io_hdr_t pass;
	int err;

	dprintf("fd: %d, cdb: %p, len: %d, buffer: %p, buflen: %d\n",fd,cdb,cdb_len,buffer,buflen);

	/* Try SG_IO first */
	memset(buffer,0,buflen);
	memset(&pass, 0, sizeof(pass));
	pass.interface_id = 'S';
	pass.dxfer_direction = SG_DXFER_FROM_DEV;
	pass.cmd_len = cdb_len;
	pass.mx_sb_len = sizeof(sense_buffer);
	pass.dxfer_len = buflen;
	pass.dxferp = buffer;
	pass.cmdp = cdb;
	pass.sbp = sense_buffer;
	pass.timeout = 2000;
	err = ioctl(fd, SG_IO, (void *)&pass);
	dprintf("SG_IO err: %d (%s), status: %d\n", err, strerror(errno), pass.status);
	if (err == 0 && pass.status != 0) err = 1;
	/* Fallback to SEND_COMMAND */
	if (err) {
		char cmdbuf[256];
		int *ip;

		dprintf("falling back to SEND_COMMAND...\n");
		memset(cmdbuf, 0, sizeof(cmdbuf));
		ip = (int *)&(cmdbuf[0]);
		*ip = 0;
		*(ip+1) = sizeof(cmdbuf) - 13;
		memcpy(&cmdbuf[8],cdb,cdb_len);

		err = ioctl(fd, SCSI_IOCTL_SEND_COMMAND, cmdbuf);
		dprintf("SEND_COMMAND err: %d (%s)\n", err, strerror(errno));
		if (!err) {
			dprintf("copying data...\n");
			memcpy(buffer,&cmdbuf[8],buflen-8);
		}
	}
	return err;
}
#endif
