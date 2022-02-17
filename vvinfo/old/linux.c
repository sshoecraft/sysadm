
/* Written by stephen.shoecraft@hp.com */

#include "vvinfo.h"

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
#include <sys/stat.h>
#include <unistd.h>

#ifndef INQUIRY
#define INQUIRY 0x12
#endif

int do_ioctl(int fd, unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
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
			dprintf("copying data...\n");
			memcpy(buffer,&cmdbuf[8],buflen-8);
		}
	}
	return err;
}

#if 0
int get_inq_page(int fd, int num, unsigned char *buffer, int buflen) {
	unsigned char cdb[12];
	int err;

	dprintf("fd: %d, num: %d, buffer: %p, buflen: %d\n", fd, num, buffer, buflen);

	memset(&cdb,0,sizeof(cdb));
	cdb[0] = INQUIRY;
	if (num >= 0) {
		cdb[1] = 1;
		cdb[2] = num;
	}
	cdb[3] = buflen / 256;
	cdb[4] = buflen % 256;

	err = do_ioctl(fd, cdb, 6, buffer, buflen);
#if 0
	if (!err) {
		int page_length;

		if (num < 0) {
			page_length = buffer[4] + 5;
			dprintf("inq page: std, length: %d\n", page_length);
		} else {
			page_length = (buffer[2] << 8 | buffer[3]) + 4;
			dprintf("inq page: %02x, length: %d\n", num, page_length);
		}
//		bindump(buffer,page_length);
	}
	else {
		printf("get_inq_page(%d,%d): %d\n", fd, num, err);
	}
#endif
	return err;
}


#if 0
int inquiry(int fd, unsigned char *buffer, int *buflen) {
	int err, len;

	/* Try to get 36 byte page first */
	err = get_inq_page(fd, -1, buffer, 36);
	if (err) return err;

	/* Get full page size */
	len = buffer[4] + 5;
	if (len > 255) len = 255;
	dprintf("full inquiry page len: %d\n", len);
	if (len != 36) err = get_inq_page(fd, -1, buffer, len);
	if (!err) *buflen = len;
	return err;
#if 0
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
#endif
}
#endif

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

#endif
