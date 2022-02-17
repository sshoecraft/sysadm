#ifdef __MINGW32__

#include "vvinfo.h"

#include <stddef.h> // For offsetof
#include <ddk/ntdddisk.h>
#include <ddk/ntddscsi.h>
#if 0 
#define WIN32_LEAN_AND_MEAN 1
#include <stddef.h> // For offsetof
#include <devguid.h>    // Device guids
#include <setupapi.h>   // for SetupDiXxx functions.
#include <initguid.h>
#include <ddk/scsi.h>
#include <ddk/ntdddisk.h>
#include <ddk/ntddscsi.h>
#include <ddk/ntddstor.h>
#endif

#define SENSELEN 24

int do_ioctl(filehandle_t fd, unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
	struct _req {
		SCSI_PASS_THROUGH_DIRECT Sptd;
		unsigned char SenseBuf[SENSELEN];
	} req;
	int status;
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
		dprintf("errno: %d\n", (int)GetLastError());
		return 1;
	}
	dprintf("returnedLen: %d\n", (int) returnedLen);
	return 0;
}

#if 0
int inquiry(HANDLE hDevice, unsigned char *data, int buflen) {
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
	req.Sptd.DataTransferLength = buflen;
	req.Sptd.TimeOutValue = 2;
	req.Sptd.DataBuffer = data;
	req.Sptd.SenseInfoOffset = offsetof(struct _req, SenseBuf);
	req.Sptd.Cdb[0] = 0x12;
        req.Sptd.Cdb[3] = buflen / 256;
        req.Sptd.Cdb[4] = buflen / 256;


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

int capacity(HANDLE hDevice, int *size) {
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

int try(list l) {
    HDEVINFO hDevInfo;
       SP_DEVINFO_DATA DeviceInfoData;
       DWORD i;

       // Create a HDEVINFO with all present devices.
//	hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_PRESENT);
//	hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_DEVICEINTERFACE);
	hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_ALLCLASSES);
	printf("hDevInfo: %p\n", hDevInfo);
       
       if (hDevInfo == INVALID_HANDLE_VALUE)
       {
           // Insert error handling here.
           return 1;
       }
       
       // Enumerate through all devices in Set.
       
       DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
       for (i=0;SetupDiEnumDeviceInfo(hDevInfo,i,&DeviceInfoData);i++) {
           DWORD DataT;
           LPTSTR buffer = NULL;
           DWORD buffersize = 0;
 
	printf("i: %d\n", (int)i);
           //
           // Call function with null to begin with, 
           // then use the returned buffer size (doubled)
           // to Alloc the buffer. Keep calling until
           // success or an unknown failure.
           //
           //  Double the returned buffersize to correct
           //  for underlying legacy CM functions that 
           //  return an incorrect buffersize value on 
           //  DBCS/MBCS systems.
           // 
//               SPDRP_DEVICEDESC,
           while (!SetupDiGetDeviceRegistryProperty(
               hDevInfo,
               &DeviceInfoData,
		SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
               &DataT,
               (PBYTE)buffer,
               buffersize,
               &buffersize))
           {
               if (GetLastError() == 
                   ERROR_INSUFFICIENT_BUFFER)
               {
                   // Change the buffer size.
                   if (buffer) LocalFree(buffer);
                   // Double the size to avoid problems on 
                   // W2k MBCS systems per KB 888609. 
                   buffer = LocalAlloc(LPTR,buffersize * 2);
               }
               else
               {
                   // Insert error handling here.
                   break;
               }
           }
           
           printf("Result:[%s]\n",buffer);
		list_add(l,buffer,strlen(buffer)+1);
           
           if (buffer) LocalFree(buffer);
       }

	return 0;
}

list win32_getdisks(void) {
	list l;
	int i;
	HDEVINFO hIntDevInfo;
	SP_INTERFACE_DEVICE_DATA interfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA interfaceDetailData;
	DWORD reqSize, interfaceDetailDataSize;
	SCSI_PASS_THROUGH_DIRECT Sptd;
	DWORD status;

	l = list_create();

	// Only Devices present & Interface class
	hIntDevInfo = SetupDiGetClassDevs (&GUID_DEVINTERFACE_DISK,0,0,(DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));
	printf("hDevInfo: %p\n", hIntDevInfo);

#if 0
	Sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	Sptd.ScsiStatus = 0;
	Sptd.PathId = 0;
	Sptd.TargetId = 0;
	Sptd.Lun = 0;
	Sptd.CdbLength = pass->cdb_len;
	Sptd.SenseInfoLength = SENSELEN;
	if (pass->buflen)
		req.Spt.DataIn = (pass->write ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_IN);
	else
		req.Spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	Sptd.DataTransferLength = pass->buflen;
	Sptd.TimeOutValue = 2;
	Sptd.DataBuffer = buffer;
	Sptd.SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);
#endif

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

//	try(l);
//	try2(l);
	return l;
}
#endif

#endif
