#if defined(__hpux)

#include "vvinfo.h"
#include <sys/scsi.h>

static int do_ioctl(int fd, unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
	unsigned char sense_buffer[32];
	esctl_io_t pass;
	int err;

	memset(&pass, 0, sizeof(pass)); /* clear reserved fields */
	pass.flags = SCTL_READ; /* input data expected */
	memcpy(pass.cdb,cdb,cdb_len);
	pass.cdb_length = cdb_len;
	pass.data = (ptr64_t)buffer; /* data buffer location */
	pass.data_length = buflen; /* maximum transfer length */
	pass.max_msecs = 10000; /* allow 10 seconds for cmd */
	if (ioctl(fd, SIOC_IO_EXT, &pass) < 0) {
		/* request is invalid */
		perror("ioctl");
		err = 1;
	} else {
		err = (pass.cdb_status != S_GOOD);
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
	printf("inquiry: err: %d\n", err);
	if (err) return 1;

	/* Get full page size */
	len = buffer[4] + 5;
	if (len > 255) len = 255;
	printf("len: %d\n", len);
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
