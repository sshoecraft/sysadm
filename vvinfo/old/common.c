
#include "vvinfo.h"

#ifndef INQUIRY
#define INQUIRY 0x12
#endif

#if 0
int get_inq_page(filehandle_t fd, int num, unsigned char *buffer, int buflen) {
	unsigned char cdb[12];

	dprintf("fd: %p, num: %d, buffer: %p, buflen: %d\n", fd, num, buffer, buflen);

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

	/* Get full page size */
	len = buffer[4] + 5;
	if (len > 255) len = 255;
	dprintf("full inquiry page len: %d\n", len);
	if (len != 36) err = get_inq_page(fd, -1, buffer, len);
	if (!err) *buflen = len;
	return err;
}

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
	dprintf("%p: getting vpd page %02x...\n", fd, 0x83);
	if (get_inq_page(fd,0x83,temp,sizeof(temp))) return 1;
	page_length = (temp[2] << 8 | temp[3]) + 3;
	dprintf("page_length: %x\n", page_length);
//	hexdump("Device Identification VPD page",temp,page_length);

	return 1;
}

int capacity(filehandle_t fd, int *size) {
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
