
#include <windows.h>
#include <ddk/ntdddisk.h>
#include <ddk/ntddscsi.h>
#include <io.h>
#include <stddef.h> // For offsetof
#include <ddk/scsi.h>
extern list win32_getdisks(void);
static list getdevs(void) {
	list lp;

#if 1
	lp = win32_getdisks();
#else
	char devs[8192], *p;
	char drive[64];
	int sz, err;
	int i;
	HANDLE hDevice;

	lp = list_create();

	sz = QueryDosDevice(0, devs, sizeof(devs));
	printf("sz: %d\n", sz);
	err = GetLastError();
	if (!sz) {
		printf("errno: %d(%x)\n", err, err);
		return lp;
	}
	p = devs;
	while(1) {
#define PD "PhysicalDrive"
//		dprintf("p: %s\n", p);
		if (strncmp(p,PD,strlen(PD)) == 0) {
			sprintf(drive,"\\\\.\\%s", p);
			printf("drive: %s\n", drive);
			list_add(lp,drive,strlen(drive)+1);
		}
		while(*p) p++;
		if (!*p) p++;
		if (!*p) break;
	}
#endif

	return lp;
}

#define SENSELEN 24
static int inquiry(HANDLE hDevice, unsigned char *data) {
	struct _req {
		SCSI_PASS_THROUGH_DIRECT Sptd;
		unsigned char SenseBuf[SENSELEN];
	} req;
	int status;
	DWORD returnedLen;

//	hDevice = (HANDLE) _get_osfhandle(fd);

	memset(&req,0,sizeof(req));
	req.Sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	req.Sptd.CdbLength = 6;
	req.Sptd.SenseInfoLength = SENSELEN;
	req.Sptd.DataIn = SCSI_IOCTL_DATA_IN;
	req.Sptd.DataTransferLength = INQLEN;
	req.Sptd.TimeOutValue = 2;
	req.Sptd.DataBuffer = data;
	req.Sptd.SenseInfoOffset = offsetof(struct _req, SenseBuf);
	req.Sptd.Cdb[0] = 0x12;
	req.Sptd.Cdb[4] = INQLEN;

	status = DeviceIoControl(
		hDevice,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&req,
		sizeof(req),
		&req,
		sizeof(req),
		&returnedLen,
		0
	);
	dprintf("status: %d\n", status);
	if (!status) {
		dprintf("errno: %d\n", (int)GetLastError());
		return 1;
	}
	dprintf("returnedLen: %d\n", (int) returnedLen);

	return 0;
}

static int capacity(HANDLE hDevice, int *size) {
	SCSI_PASS_THROUGH_DIRECT Sptd;
	int status;
	DWORD rlen;
        unsigned char data[32], *p;
        unsigned long long bytes;
        unsigned long lba,len;

	memset(&Sptd,0,sizeof(Sptd));
	Sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	Sptd.CdbLength = 10;
	Sptd.DataIn = SCSI_IOCTL_DATA_IN;
	Sptd.DataTransferLength = sizeof(data);
	Sptd.TimeOutValue = 2;
	Sptd.DataBuffer = data;
	Sptd.Cdb[0] = SCSIOP_READ_CAPACITY;

	dprintf("calling DeviceIoControl...\n");
	status = DeviceIoControl(
		hDevice,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&Sptd,
		sizeof(Sptd),
		&Sptd,
		sizeof(Sptd),
		&rlen,
		0
	);
	dprintf("status: %d\n", status);
	if (!status) {
		dprintf("errno: %d\n", (int)GetLastError());
		return 1;
	}
	dprintf("rlen: %d\n", (int) rlen);

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
