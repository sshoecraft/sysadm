
#include "xplist.h"

#ifdef __WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <devguid.h>    // Device guids
#include <setupapi.h>   // for SetupDiXxx functions.
#include <initguid.h>
#include <stddef.h> // For offsetof
#include <ddk/ntdddisk.h>
#include <ddk/ntddscsi.h>

#if 1
list get_devs(void) {
	list lp;

	char devs[8192], *p;
	char drive[64];
	int sz, err;

	lp = list_create();

	sz = QueryDosDevice(0, devs, sizeof(devs));
	dprintf("sz: %d\n", sz);
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
			dprintf("drive: %s\n", drive);
			list_add(lp,drive,strlen(drive)+1);
		}
		while(*p) p++;
		if (!*p) p++;
		if (!*p) break;
	}

	return lp;
}
#else
list get_devs(void) {
	list l;
	int i;
	HDEVINFO hIntDevInfo;
	SP_INTERFACE_DEVICE_DATA interfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA interfaceDetailData;
	DWORD reqSize, interfaceDetailDataSize;

	l = list_create();

	// Only Devices present & Interface class
	hIntDevInfo = SetupDiGetClassDevs (&GUID_DEVINTERFACE_DISK,0,0,(DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));
	printf("hDevInfo: %p\n", hIntDevInfo);

	interfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
	for (i=0; SetupDiEnumDeviceInterfaces(hIntDevInfo,0,&GUID_DEVINTERFACE_DISK,i,&interfaceData); i++) {
		printf("i: %d\n", i);

		interfaceDetailData = 0;
		interfaceDetailDataSize = 0;
		while(!SetupDiGetDeviceInterfaceDetail(hIntDevInfo,&interfaceData,interfaceDetailData,interfaceDetailDataSize,&reqSize,0)) {
			printf("reqSize: %d\n", (int) reqSize);
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				if (interfaceDetailData) LocalFree(interfaceDetailData);
				interfaceDetailDataSize = reqSize * 2;
				interfaceDetailData = malloc (interfaceDetailDataSize);
    				interfaceDetailData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);
			} else {
				printf("SetupDiGetDeviceInterfaceDetail failed: %d\n", (int)GetLastError());
				return 0;
			}
		}
		printf("path: %s\n", interfaceDetailData->DevicePath);
		list_add(l, interfaceDetailData->DevicePath, strlen(interfaceDetailData->DevicePath)+1);
	}
	return l;
}
#endif

#define SENSELEN 24
int do_ioctl(filehandle_t fd, unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
	struct _req {
		SCSI_PASS_THROUGH_DIRECT Sptd;
		unsigned char SenseBuf[SENSELEN];
	} req;
	int status,err;
	DWORD returnedLen;

	memset(&req,0,sizeof(req));
	req.Sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	req.Sptd.CdbLength = cdb_len;
	req.Sptd.SenseInfoLength = SENSELEN;
	req.Sptd.DataIn = SCSI_IOCTL_DATA_IN;
	req.Sptd.DataTransferLength = buflen;
	req.Sptd.TimeOutValue = 2;
	req.Sptd.DataBuffer = buffer;
	req.Sptd.SenseInfoOffset = offsetof(struct _req, SenseBuf);
	memcpy(&req.Sptd.Cdb,cdb,cdb_len);

	status = DeviceIoControl(
		fd,
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
		err = 1;
	} else {
		err = (req.Sptd.ScsiStatus != 0);
	}
	return err;
}
#endif
