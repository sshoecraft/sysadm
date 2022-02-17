
#include "xplist.h"

#if defined(__linux__)
#include <ctype.h>
#include <sys/ioctl.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <sys/errno.h>
#ifdef SCAN_ALL
#include <dirent.h>
#endif
#include <fcntl.h>

/* Get a list of block devices in /dev and their major/minor numbers */
block_devs = list_create();


#define DMSETUP "/sbin/dmsetup"

void get_mp(void) {
	char tmpfile[256],line[256],*p;
	FILE *fp;
	int fd,i;

	if (access(DMSETUP,X_OK) != 0) return;

	concat_path(tmpfile,tempdir,"mpinfo.tmp");
	fd = open(tmpfile,O_RDWR|O_CREAT);
	if (fd < 0) {
		perror("get_mp: open");
		return;
	}
	dprintf("tmpfile: %s\n", tmpfile);
	sprintf(line,"%s status --target multipath > %s 2>&1", DMSETUP, tmpfile);
	dprintf("cmd: %s\n", line);
	system(line);

	fp = fdopen(fd,"r");
	if (!fp) {
		perror("get_mp: fdopen");
		goto done;
	}
	while(fgets(line,sizeof(line),fp) != 0) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		dprintf("line: %s\n", line);
		for(i = 14; i < 999; i += 3) {
			p = strele(i," ",line);
			printf("p(%d): %s\n", i, p);
			if (!strlen(p)) break;
		}
	}
	fclose(fp);
done:
	exit(0);
}

list get_devs(void) {
	list lp;
	FILE *fp;
	char line[128], name[32], *p;
#ifdef SCAN_ALL
	DIR *dirp;
	struct dirent *ent;
#endif

	lp = list_create();
	if (!lp) {
		perror("list_create");
		return 0;
	}

#ifndef SCAN_ALL
	fp = fopen("/proc/partitions","r");
	if (!fp) {
		perror("fopen /proc/partitions");
		exit(1);
	}

	while(fgets(line,sizeof(line),fp)) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		if (!strlen(line)) continue;
		dprintf("partitions line: %s\n", line);

		p = strele(3," ",line);
		dprintf("dev: %s\n", p);
		if (!strlen(p)) continue;
		if (strncmp(p,"psd",3) == 0 && isdigit(p[strlen(p)-1]));
		else if (strncmp(p,"sd",2) != 0 || isdigit(p[strlen(p)-1]))
			continue;
		name[0] = 0;
		strcat(name, "/dev/");
		strcat(name, p);
		dprintf("dev: %s\n", name);
		list_add(lp, name, strlen(name)+1);
	}
	fclose(fp);
#else
	fp = 0;
	line[0] = 0;
	p = 0;
	dirp = opendir("/dev");
	if (!dirp) {
		perror("opendir(/dev)");
		return lp;
	}
	while((ent = readdir(dirp)) != 0) {
		p = ent->d_name;
		stat(ent->d_name);
#if 0
		if (strncmp(p,"psd",3) == 0 && isdigit(p[strlen(p)-1]));
		else if (strncmp(p,"sd",2) != 0 || isdigit(p[strlen(p)-1]))
			continue;
#endif
		name[0] = 0;
		strcat(name, "/dev/");
		strcat(name, p);
		dprintf("dev: %s\n", name);
		list_add(lp, name, strlen(name)+1);
	}
	closedir(dirp);
#endif

	return lp;
}

static int do_ioctl(int fd, unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
	unsigned char sense_buffer[32];
	sg_io_hdr_t pass;
	int err;

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
	dprintf("SG_IO err: %d, status: %d\n", err, pass.status);
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
		dprintf("SEND_COMMAND err: %d\n", err);
		if (!err) {
			printf("copying data...\n");
			memcpy(buffer,&cmdbuf[8],buflen-8);
		}
	}
	return err;
}

int inquiry(int fd, unsigned char *buffer, int *buflen) {
	unsigned char cdb[12];
	int err,len;

	memset(&cdb,0,sizeof(cdb));
	cdb[0] = 0x12;

	/* Try to get 36 byte page first */
	cdb[4] = 36;
	err = do_ioctl(fd, cdb, 6, buffer, 36);
	dprintf("inquiry36: err: %d\n", err);
	if (err) return 1;

	/* Get full page size */
	len = buffer[4] + 5;
	if (len > 255) len = 255;
	dprintf("inquiry len: %d\n", len);
	cdb[4] = len;
	err = do_ioctl(fd, cdb, 6, buffer, len);
	if (!err) *buflen = len;
	return err;
}

int capacity(int fd, int *size) {
	unsigned char cmd[10] = { 0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char data[32], *p;
	unsigned long long bytes;
	unsigned long lba,len;

	if (do_ioctl(fd, cmd, 10, data, sizeof(data))) return 1;
	p = data;
	lba = (*(p+0) << 24) | (*(p+1) << 16) | (*(p+2) << 8) | *(p+3);
	len = (*(p+4) << 24) | (*(p+5) << 16) | (*(p+6) << 8) | *(p+7);
	dprintf("lba: %ld, len: %ld\n", lba, len);
	bytes = lba;
	bytes *= len;
	dprintf("bytes: %lld\n", bytes);
	*size = bytes / 1048576;
	dprintf("size: %dMB\n", *size);
	return 0;
}

#endif
