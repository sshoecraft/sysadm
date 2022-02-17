
/*
 * xplist - list xp san devices available on the system
 *
 * Steve Shoecraft (stephen.shoecraft@hp.com)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stropts.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG

#include "xp.h"

#define INQLEN 96
#if defined(__linux__)
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <sys/errno.h>
static list getdevs(void) {
	list lp;
	FILE *fp;
	char line[128], name[32], *p;

	lp = list_create();
	if (!lp) {
		perror("list_create");
		return 0;
	}

	fp = fopen("/proc/partitions","r");
	if (!fp) {
		perror("fopen /proc/partitions");
		exit(1);
	}

	while(fgets(line,sizeof(line),fp)) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		if (!strlen(line)) continue;
		dprintf(("partitions line: %s\n", line));

		p = strele(3," ",line);
		dprintf(("dev: %s\n", p));
		if (!strlen(p)) continue;
		if (strncmp(p,"sd",2) != 0 || isdigit(p[strlen(p)-1]))
			continue;
		strcpy(name, "/dev/");
		strcat(name, p);
		dprintf(("dev: %s\n", name));
		list_add(lp, name, strlen(name)+1);
	}
	fclose(fp);

	return lp;
}

static int do_ioctl(int fd, unsigned char *cmd, int cmd_len, unsigned char *buffer, int buflen) {
        unsigned char sense[32];
        struct sg_io_hdr io_hdr;
        int result;

        dprintf(("fd: %d, cmd: %x, cmd_len: %d, buffer: %p, buflen: %d\n",
                fd, cmd[0], cmd_len, buffer, buflen));

	if (cmd[0] != 0x12) bindump("buffer",buffer,buflen);
        memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
        io_hdr.interface_id = 'S';
        io_hdr.cmd_len = cmd_len;
        io_hdr.mx_sb_len = sizeof(sense);
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
        io_hdr.dxfer_len = buflen;
        io_hdr.dxferp = buffer;
        io_hdr.cmdp = cmd;
        io_hdr.sbp = sense;
#define DEF_TIMEOUT 60000
        io_hdr.timeout = DEF_TIMEOUT;

        result = ioctl(fd, SG_IO, &io_hdr);
        dprintf(("result: %d\n", result));
        if (result < 0) perror("ioctl");
	if ((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
#ifdef DEBUG
		if (io_hdr.sb_len_wr > 0) bindump("sense_data",sense,sizeof(sense));
		if (io_hdr.masked_status) printf("status=0x%x\n", io_hdr.status);
		if (io_hdr.host_status) printf("host_status=0x%x\n", io_hdr.host_status);
		if (io_hdr.driver_status) printf("driver_status=0x%x\n", io_hdr.driver_status);
#endif
		return 1;
	}

        if (!result) result = ((io_hdr.status & 0x7e) == 0 || io_hdr.host_status == 0 || io_hdr.driver_status == 0 ? 0 : -1);
        dprintf(("result2: %d\n", result));
	if (cmd[0] != 0x12) bindump("buffer",buffer,buflen);
        return (result != 0);
}

#if 0
static int do_ioctl(int fd, unsigned char *cmd, int cmd_len, unsigned char *data, int data_len) {
#define BUFLEN 256
	unsigned char buffer[BUFLEN], sense[32], *inqptr;
	struct sg_io_hdr io_hdr;
	int result,*ip;

	dprintf(("fd: %d, cmd_len: %d, data_len: %d\n", fd, cmd_len, data_len));

	/* Try SG_IO first, if it is not supported, use SCSI_IOCTL_SEND_COMMAND */
	memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.cmd_len = cmd_len;
	io_hdr.mx_sb_len = sizeof(sense);
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.dxfer_len = 200;
	io_hdr.dxferp = buffer;
	io_hdr.cmdp = cmd;
	io_hdr.sbp = sense;
#define DEF_TIMEOUT 60000
	io_hdr.timeout = DEF_TIMEOUT;
	inqptr = buffer;

	result = ioctl(fd, SG_IO, &io_hdr);
    	if (!result) result = ((io_hdr.status & 0x7e) == 0 || io_hdr.host_status == 0 || io_hdr.driver_status == 0 ? 0 : -1);
	if (result) {
		if (errno == ENOTTY || errno == EINVAL) {
			fprintf(stderr,"WARNING: falling back to SEND_COMMAND ioctl\n");
			memset(buffer, 0, BUFLEN);
			ip = (int *)&(buffer[0]);
			*ip = 0;
			*(ip+1) = BUFLEN - 13;
			memcpy(&buffer[8],cmd,cmd_len);

			result = ioctl(fd, SCSI_IOCTL_SEND_COMMAND, buffer);
			inqptr = buffer + 8;
		}
	}
	if (!result) memcpy(data,inqptr,data_len);
	return (result != 0);
}
#endif

static int inquiry(int fd, unsigned char *data) {
	unsigned char cmd[6] = { 0x12, 0, 0, 0, INQLEN, 0 };

	memset(data,0,INQLEN);
	return do_ioctl(fd, cmd, 6, data, INQLEN);
}

typedef unsigned long long u64;

static int capacity(int fd, int *size) {
	unsigned char cmd[16] = { 0x9E, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0 };
	unsigned char data[32], *p;
	unsigned long long lba,s;
	int x,bs;

	memset(data,0,sizeof(data));
#if 0
	memset(cmd,0,sizeof(cmd));
	cmd[0] = 0x25;
	if (do_ioctl(fd, cmd, 10, data, 8)) return 1;
	p = data;
	lba = (*(p+0) << 24) | (*(p+1) << 16) | (*(p+2) << 8) | *(p+3);
	bs = (*(p+4) << 24) | (*(p+5) << 16) | (*(p+6) << 8) | *(p+7);
#endif

	p = data;
	if (do_ioctl(fd, cmd, 16, data, 32)) {
		memset(cmd,0,sizeof(cmd));
		cmd[0] = 0x25;
		if (do_ioctl(fd, cmd, 10, data, 8)) return 1;
		lba = (*(p+0) << 24) | (*(p+1) << 16) | (*(p+2) << 8) | *(p+3);
		bs = (*(p+4) << 24) | (*(p+5) << 16) | (*(p+6) << 8) | *(p+7);
	} else {
#if 0
		lba = 0;
		for(x=0; x < 8; x++) {
			if (x) lba <<= 8;
			lba |= *(p + x);
		}
#endif
		lba =  ((u64)*(p+0)) << 56 | ((u64)*(p+1)) << 48 | ((u64)*(p+2)) << 40 | ((u64)*(p+3)) << 32 | \
			((u64)*(p+4)) << 24 | ((u64)*(p+5)) << 16 | ((u64)*(p+6)) << 8 | ((u64)*(p+7));

		bs = (*(p+8) << 24) | (*(p+9) << 16) | (*(p+10) << 8) | *(p+11);
	}
	dprintf(("lba: %lld, bs: %d\n", lba, bs));
	*size = (lba * bs) / 1048576;
	dprintf(("size: %d\n", *size));
	return 0;
}
#elif defined(__hpux)
#include <sys/scsi.h>
static list getdevs(void) {
	list lp;
	FILE *fp;
	char line[128], *p;

	lp = list_create();
	if (!lp) {
		perror("list_create");
		return 0;
	}

	strcpy(line,"ioscan -funC disk > /tmp/xplist.tmp 2>&1");
	system(line);
	fp = fopen("/tmp/xplist.tmp","r");
	if (!fp) {
		perror("get_xplist: fopen /tmp/xplist.tmp");
		return 0;
	}
	while(fgets(line,sizeof(line),fp)) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		if (line[0] == 0) continue;
		dprintf(("ioscan line: %s\n", line));
		p = strstr(line,"/dev/rdsk");
		if (!p) continue;
		p = strele(0," ",p);
		dprintf(("dev: %s\n", p));
		list_add(lp, p, strlen(p)+1);
	}
	fclose(fp);
	unlink("/tmp/xplist.tmp");

	return lp;
}

static int inquiry(int fd, unsigned char *data) {
	if (ioctl(fd,SIOC_INQUIRY,data) < 0) {
		perror("ioctl inq");
		return 1;
	}
	return 0;
}

static int capacity(int fd, int *size) {
	storage_capacity_t cap;

	if (ioctl(fd,SIOC_STORAGE_CAPACITY,&cap) < 0) {
		perror("ioctl cap");
		return 1;
	}
	dprintf(("lba: %lld, blksz: %d\n", cap.lba, cap.blksz));
	*size = (cap.lba * cap.blksz) / 1048576;
	dprintf(("size: %d\n", *size));

	return 0;
}
#endif

list get_xplist(void) {
	list lp,devs;
	unsigned char data[INQLEN];
	struct xplist_entry newent;
	char temp[32];
	register char *p;
	int fd, size;

	/* Get a list of disks */
	devs = getdevs();
	if (!devs) return 0;

	lp = list_create();
	if (!lp) return 0;

	/* For each disk... */
	list_reset(devs);
	while((p = list_get_next(devs)) != 0) {
		dprintf(("dev: %s\n", p));
		/* Identify the device and get the size */
		fd = open(p,O_RDONLY);
		if (fd < 0) continue;
		if (inquiry(fd,data)) continue;
		if (capacity(fd,&size)) continue;
		close(fd);

		/* Verify vendor is HP & Product starts with OPEN */
		memcpy(temp, &data[8], 8);
		temp[8] = 0;
		trim(temp);
		dprintf(("vendor: %s\n", temp));
		if (strcmp(temp,"HP") != 0) continue;
		memcpy(temp, &data[16], 16);
		temp[16] = 0;
		trim(temp);
		dprintf(("product: %s\n", temp));
		if (strncmp(temp,"OPEN",4) != 0) continue;

		/* We have an XP disk, fill in the info */
		memset(&newent,0,sizeof(newent));
		strcpy(newent.dev, p);
		newent.size = size;
		strcpy(newent.type, temp);

		/* Info is embedded in vendor_specific */
		memcpy(temp,&data[39],5);
		temp[5] = 0;
		dprintf(("serial string: %s\n", temp));
		newent.serial = strtol(temp, (char **)NULL, 16);
		dprintf(("serial: %d\n", newent.serial));
		temp[0] = data[36+8];
		temp[1] = data[36+9];
		temp[2] = 0;
		newent.cu = strtol(temp, 0, 16);
		dprintf(("cu: %02x\n", newent.cu));
		temp[0] = data[36+10];
		temp[1] = data[36+11];
		temp[2] = 0;
		newent.ldev = strtol(temp, 0, 16);
		dprintf(("ldev: %02x\n", newent.ldev));
		newent.port[0] = 'C';
		newent.port[1] = 'L';
		newent.port[2] = data[36+13];
		newent.port[3] = data[36+14];
		newent.port[4] = 0;
		dprintf(("port: %s\n", newent.port));

		/* Generate a unique id from the serial+cu+ldev */
		newent.id = newent.serial;
		newent.id <<= 32;
		newent.id |= (unsigned long long) newent.cu << 8;
		newent.id |= (unsigned long long) newent.ldev;
		dprintf(("id: %llx\n", newent.id));

		list_add(lp, &newent, sizeof(newent));
	}

	return lp;
}
