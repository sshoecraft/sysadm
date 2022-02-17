
#include "xplist.h"

#ifndef INQUIRY
#define INQUIRY 0x12
#endif

static int get_inq_page(filehandle_t fd, int num, unsigned char *buffer, int buflen) {
	unsigned char cdb[12];

	dprintf("fd: %d, num: %d, buffer: %p, buflen: %d\n", (int)fd, num, buffer, buflen);

	memset(&cdb,0,sizeof(cdb));
	cdb[0] = INQUIRY;
	if (num >= 0) {
		cdb[1] = 1;
		cdb[2] = num;
	}
	cdb[3] = buflen / 256;
	cdb[4] = buflen % 256;

	return do_ioctl(fd, cdb, 6, buffer, buflen);
}

int inquiry(filehandle_t fd, unsigned char *buffer, int *buflen) {
	int err, len;

	/* Try to get 36 byte page first */
	err = get_inq_page(fd, -1, buffer, 36);
	if (err) return err;
//	bindump("basic inq",buffer,36);

	/* Get full page size */
	len = buffer[4] + 5;
	if (len > 255) len = 255;
	dprintf("full inquiry page len: %d\n", len);
	if (len != 36) err = get_inq_page(fd, -1, buffer, len);
	if (!err) *buflen = len;
	return err;
}

#if 0
static int has_serial_page(int fd) {
	unsigned char temp[64],pages[16];
	int npages,i;

	/* Get the list of supported vpd pages */
	dprintf("%d: getting vpd page 00...\n", fd);
	if (get_inq_page(fd, 0, temp, sizeof(temp))) return 1;
//	bindump(temp,sizeof(temp));

	/* For every page in list ... */
	npages = temp[3];
	dprintf("%d: npages: %d\n", fd, npages);
	memset(pages,0,sizeof(pages));
	memcpy(pages,&temp[4],(npages < 16 ? npages : 16));
	for(i=0; i < npages; i++) {
		if (pages[i] == 0x80) {
			/* Found */
			return 1;
		}
	}
	/* Not found */
	return 0;
}
#endif

#if 0
0 PROTOCOL IDENTIFIER CODE SET
1 PIV Reserved ASSOCIATION DESIGNATOR TYPE
2 Reserved
3 DESIGNATOR LENGTH (n-3)
4
DESIGNATOR
n
#endif

int get_naa(filehandle_t fd, int *serial, char *cu, char *ldev) {
	unsigned char temp[1024];
	int page_length;

//	if (!has_page_80) return 1;

	*serial = 0;
	*cu = *ldev = 0;
	dprintf("%d: getting vpd page %02x...\n", (int)fd, 0x83);
	if (get_inq_page(fd,0x83,temp,sizeof(temp))) return 1;
	page_length = (temp[2] << 8 | temp[3]) + 3;
	dprintf("page_length: %x\n", page_length);
	bindump("Device Identification VPD page",temp,page_length);

	return 1;
}

/* 
0 OPERATION CODE (A0h)
1 Reserved
2 Reserved
3 Reserved
4 Reserved
5 Reserved
6 (MSB)
ALLOCATION LENGTH
7
8
9 (LSB)
10 Reserved
11 CONTROL
*/

typedef unsigned long long u64;

int get_lunid(filehandle_t fd, int *lunid) {
	unsigned char cmd[] = { 0xa0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0 };
	unsigned char data[256], *p;
	unsigned long long lun;
	int i,count;

	if (do_ioctl(fd, cmd, 10, data, sizeof(data))) {
		dprintf("get_lunid: do_ioctl failed!\n");
		return 1;
	}
//	bindump("report luns data",&data,256);
	p = data;
	count = (((*(p+0) << 24) | (*(p+1) << 16) | (*(p+2) << 8) | *(p+3)) - 8) / 8;
	dprintf("count: %d\n", count);
	p = &data[10];
#if 0
0000: 00 00 00 58 00 00 00 00 00 00 00 00 00 00 00 00   ...X............
0010: 00 01 00 00 00 00 00 00 00 02 00 00 00 00 00 00   ................
0020: 00 03 00 00 00 00 00 00 00 04 00 00 00 00 00 00   ................
0030: 00 05 00 00 00 00 00 00 00 06 00 00 00 00 00 00   ................
0040: 00 07 00 00 00 00 00 00 00 08 00 00 00 00 00 00   ................
0050: 00 09 00 00 00 00 00 00 00 0A 00 00 00 00 00 00   ................
#endif
	for(i=0; i < count; i++) {
#if 0
	        lun =  ((u64)*(p+0)) << 56 | ((u64)*(p+1)) << 48 | ((u64)*(p+2)) << 40 | ((u64)*(p+3)) << 32 | \
			((u64)*(p+4)) << 24 | ((u64)*(p+5)) << 16 | ((u64)*(p+6)) << 8 | ((u64)*(p+7));
		dprintf("lun: %lld\n", lun);
#endif
		dprintf("lun[7]: %d\n", *(p+7));
		p += 8;
	}
	return 1;
}

int capacity(filehandle_t fd, int *size) {
	unsigned char cmd[16] = { 0x9e, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0 };
	unsigned char data[32], *p;
	unsigned long long bytes;
	unsigned long long lba,len;

	p = data;
	if (do_ioctl(fd, cmd, 16, data, sizeof(data))) {
		memset(cmd,0,sizeof(cmd));
		cmd[0] = 0x25;
		if (do_ioctl(fd, cmd, 10, data, sizeof(data))) return 1;
		lba = ((long long)*(p+0) << 24) | ((long long)*(p+1) << 16) | ((long long)*(p+2) << 8) | (long long)*(p+3);
		len = ((long long)*(p+4) << 24) | ((long long)*(p+5) << 16) | ((long long)*(p+6) << 8) | (long long)*(p+7);
	} else {
	        lba =  ((u64)*(p+0)) << 56 | ((u64)*(p+1)) << 48 | ((u64)*(p+2)) << 40 | ((u64)*(p+3)) << 32 | \
			((u64)*(p+4)) << 24 | ((u64)*(p+5)) << 16 | ((u64)*(p+6)) << 8 | ((u64)*(p+7));
		len = ((long long)*(p+8) << 24) | ((long long)*(p+9) << 16) | ((long long)*(p+10) << 8) | (long long)*(p+11);
	}
	dprintf("lba: %lld, len: %lld\n", lba, len);
	bytes = lba;
	bytes *= len;
	dprintf("bytes: %lld\n", bytes);
	*size = bytes / 1048576;
	dprintf("size: %dMB\n", *size);
	return 0;
}

list get_mp_devs(char *mp_name) {
	struct mpdevs_entry *mp;
	list devs;

	dprintf("mp_name: %s\n", mp_name);
	devs = 0;
	list_reset(mpdevs);
	while((mp = list_get_next(mpdevs)) != 0) {
		dprintf("mp->name: %s, mp->alt_name: %s\n", mp->name, mp->alt_name);
		if (strcmp(mp->name,mp_name) == 0 || strcmp(mp->alt_name,mp_name) == 0) {
			dprintf("fount it\n");
			devs = mp->devs;
		}
	}
	if (!devs) devs = list_create();
	return devs;
}
