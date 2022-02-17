
#include "luninfo.h"

// implemented pass-through ops

#if defined(__hpux)
void get_mp(mpdevs) { return; }
#define TMPFILE "/tmp/xplist.tmp"
#include <sys/scsi.h>
list get_devs(void) {
	list lp;
	FILE *fp;
	char line[1024], *p, *p2;
	int skipnext;

	lp = list_create();
	if (!lp) {
		perror("list_create");
		return 0;
	}

	sprintf(line,"/usr/sbin/ioscan -funC disk > %s 2>&1",TMPFILE);
	system(line);
	fp = fopen(TMPFILE,"r");
	if (!fp) {
		sprintf(line,"get_devs: fopen %s", TMPFILE);
		perror(line);
		return 0;
	}
	skipnext = 0;
	while(fgets(line,sizeof(line),fp)) {
		strcpy(line,stredit(line,"TRIM,COMPRESS"));
		if (line[0] == 0) continue;
		dprintf("ioscan line: %s\n", line);
		if (skipnext) {
			skipnext = 0;
			continue;
		}
		if (strstr(line,"ROM")) skipnext = 1;
		p = strstr(line,"/dev/rdsk");
		if (!p) continue;
		/* XXX Skip past /dev/rdsk/ */
//		p += 10;
		p = strele(0," ",p);
		dprintf("dev: %s\n", p);
		if (strlen(p) < 2) continue;
		p2 = p + (strlen(p) - 2);
		dprintf("p2: %s\n", p2);
		if (p2[0] == 's' && isdigit(p2[1])) continue;
		list_add(lp, p, strlen(p)+1);
	}
	fclose(fp);
	unlink(TMPFILE);

	return lp;
}
#endif

#if 0
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
	dprintf("inquiry: err: %d\n", err);
	if (err) return 1;

	/* Get full page size */
	len = buffer[4] + 5;
	if (len > 255) len = 255;
	dprintf("len: %d\n", len);
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
