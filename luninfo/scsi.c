
#include "luninfo.h"

#ifndef INQUIRY
#define INQUIRY 0x12
#endif

static unsigned char scsi_temp[2048];

typedef unsigned char u8;
typedef unsigned long long u64;

#define _getlong(p) ((long) (*(p) << 24) | (*((p)+1) << 16) | (*((p)+2) << 8) | *((p)+3) )
//#define _getquad(p) ((long long) (*(p) << 24) | (*((p)+1) << 16) | (*((p)+2) << 8) | *((p)+3) | (*((p)+4) << 24) | (*((p)+5) << 16) | (*((p)+6) << 8) | *((p)+7))
#if 0
#define _getquad(p) ((u64)*(p+0)) << 56 | ((u64)*(p+1)) << 48 | ((u64)*(p+2)) << 40 | ((u64)*(p+3)) << 32 | \
                        ((u64)*(p+4)) << 24 | ((u64)*(p+5)) << 16 | ((u64)*(p+6)) << 8 | ((u64)*(p+7));
#endif
#define _getquad(p) ((u64)*((p)+0)) << 56 | ((u64)*((p)+1)) << 48 | ((u64)*((p)+2)) << 40 | ((u64)*((p)+3)) << 32 | \
                        ((u64)*((p)+4)) << 24 | ((u64)*((p)+5)) << 16 | ((u64)*((p)+6)) << 8 | ((u64)*((p)+7));

int isready(filehandle_t fd) {
	unsigned char cdb[6] = { 0,0,0,0,0,0 };
	int status;

	status = do_ioctl(fd, cdb, 6, scsi_temp, sizeof(scsi_temp));
	status = status;
	dprintf("status: %d\n", status);
	return 0;
}

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

/* Gets the std inq page ... try with 36 char buf 1st, then full page size */
int std_inquiry(filehandle_t fd, unsigned char *buffer, int *buflen) {
	int err, len;

	/* Make sure buflen is at least 36 */
	if (*buflen < 36) {
		printf("std_inquiry: buflen too small!\n");
		return 1;
	}

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

#if 0
static int has_serial_page(int fd) {
	unsigned char pages[16];
	int npages,i;

	/* Get the list of supported vpd pages */
	dprintf("%d: getting vpd page 00...\n", fd);
	if (get_inq_page(fd, 0, scsi_temp, sizeof(scsi_temp))) return 1;
//	bindump(scsi_temp,sizeof(scsi_temp));

	/* For every page in list ... */
	npages = scsi_temp[3];
	dprintf("%d: npages: %d\n", fd, npages);
	memset(pages,0,sizeof(pages));
	memcpy(pages,&scsi_temp[4],(npages < 16 ? npages : 16));
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
0 PROTOCOL IDENTIFIER | CODE SET
1 PIV | Reserved | ASSOCIATION | DESIGNATOR TYPE
2 Reserved
3 DESIGNATOR LENGTH (n-3)
4
DESIGNATOR
n
#endif


/* SCSI protocols; these are taken from SPC-3 section 7.5 */
enum scsi_protocol {
        SCSI_PROTOCOL_FCP = 0,  /* Fibre Channel */
        SCSI_PROTOCOL_SPI = 1,  /* parallel SCSI */
        SCSI_PROTOCOL_SSA = 2,  /* Serial Storage Architecture - Obsolete */
        SCSI_PROTOCOL_SBP = 3,  /* firewire */
        SCSI_PROTOCOL_SRP = 4,  /* Infiniband RDMA */
        SCSI_PROTOCOL_ISCSI = 5,
        SCSI_PROTOCOL_SAS = 6,
        SCSI_PROTOCOL_ADT = 7,  /* Media Changers */
        SCSI_PROTOCOL_ATA = 8,
        SCSI_PROTOCOL_UNSPEC = 0xf, /* No specific protocol */
};

static void _pval(char *dest, unsigned char *src, int len) {
	register char *p;
	register int i;

	p = dest;
	for(i=0; i < len; i++) {
		p += sprintf(p,"%02x",src[i]);
	}
	dprintf("dest: %s\n", dest);
}

int get_wwn(filehandle_t fd, char *value) {
	int vpd_len;
	u8 *desc;
	char wwn[128];
	char *p;

	extern int wwn_type;

//	if (!has_page_80) return 1;

	dprintf("%d: getting vpd page %02x...\n", (int)fd, 0x83);
	if (get_inq_page(fd,0x83,scsi_temp,sizeof(scsi_temp))) return 1;
	vpd_len = (scsi_temp[2] << 8 | scsi_temp[3]) + 3;
	dprintf("page_length: %x\n", vpd_len);
//	bindump("Device Identification VPD page",scsi_temp,vpd_len+4);
        desc = scsi_temp + 4;
        while (desc < scsi_temp + vpd_len) {
		enum scsi_protocol proto = desc[0] >> 4;
		u8 code_set = desc[0] & 0x0f;
		u8 piv = desc[1] & 0x80;
		u8 assoc = (desc[1] & 0x30) >> 4;
		u8 type = desc[1] & 0x0f;
		u8 len = desc[3];
		dprintf("proto: %d, code: %d, piv: %d, assoc: %d, type: %d, len: %d\n",
			proto, code_set, piv, assoc, type, len);

#if 0
get_wwn(161): proto: 0, code: 1, piv: 0, assoc: 0, type: 0, len: 32
get_wwn(161): proto: 0, code: 1, piv: 0, assoc: 0, type: 0, len: 4
get_wwn(161): proto: 0, code: 1, piv: 0, assoc: 0, type: 2, len: 16
get_wwn(161): proto: 0, code: 1, piv: 0, assoc: 0, type: 3, len: 16
get_wwn(161): proto: 0, code: 1, piv: 0, assoc: 0, type: 3, len: 8
get_wwn(161): proto: 0, code: 1, piv: 0, assoc: 1, type: 3, len: 16
get_wwn(161): proto: 0, code: 1, piv: 0, assoc: 1, type: 4, len: 4
get_wwn(161): proto: 0, code: 1, piv: 0, assoc: 1, type: 5, len: 4
get_wwn(161): proto: 0, code: 1, piv: 128, assoc: 1, type: 3, len: 8
get_wwn(161): proto: 0, code: 1, piv: 128, assoc: 2, type: 3, len: 8
get_wwn(161): proto: 0, code: 2, piv: 0, assoc: 0, type: 1, len: 32
get_wwn(161): proto: 0, code: 3, piv: 0, assoc: 2, type: 8, len: 40
get_wwn(170): proto: 0, code: 2, piv: 0, assoc: 0, type: 1, len: 36   <-- Linux iSCSI disk on esxi
#endif

#if 0
CODE:
1h The IDENTIFIER DESIGNATOR field shall contain binary values.
2h The IDENTIFIER DESIGNATOR field shall contain ASCII printable characters (i.e., code values 20h through 7Eh)
3h The IDENTIFIER DESIGNATOR field shall contain ISO/IEC 10646-1 (UTF-8) codes

PIV:
A protocol identifier valid (PIV) bit set to zero indicates the PROTOCOL IDENTIFIER field contents are reserved. If the
ASSOCIATION field contains a value of 01b or 10b, then a PIV bit set to one indicates the PROTOCOL IDENTIFIER field
contains a valid protocol identifier selected from the values shown in table 262 (see 7.5.1). If the ASSOCIATION field
contains a value other than 01b or 10b, then the PIV bit contents are reserved

ASSOC:
00b The IDENTIFIER DESIGNATOR field is associated with the addressed logical unit.
01b The IDENTIFIER DESIGNATOR field is associated with the target port that received the request.
10b The IDENTIFIER DESIGNATOR field is associated with the SCSI target device that contains the addressed
logical unit.

TYPE:
0h Vendor specific 7.6.3.3
1h T10 vendor ID esxibased 7.6.3.4
2h EUI-64 based 7.6.3.5
3h NAA 7.6.3.6
4h Relative target port identifier 7.6.3.7
5h Target port group 7.6.3.8
6h Logical unit group 7.6.3.9
7h MD5 logical unit identifier 7.6.3.10
8h SCSI name string 7.6.3.11
#endif

		proto = proto;
		piv = piv;

		//get_wwn(150): proto: 0, code: 1, piv: 0, assoc: 0, type: 3, len: 16
		if (assoc == 0) {
			p = wwn;
			switch(type) {
			case 1: /* T10 */
			case 3: /* NAA */
				if (wwn_type) p += sprintf(p,"%d",type);
				if (code_set == 1) {
					_pval(p,&desc[4],len);
				} else {
					memcpy(p,&desc[4],len);
					trim(wwn);
				}
//				bindump("wwn",wwn,strlen(wwn));
				break;
#if 0
			case 8:
//get_wwn(161): proto: 0, code: 2, piv: 0, assoc: 0, type: 1, len: 32
//get_wwn(161): proto: 0, code: 3, piv: 0, assoc: 2, type: 8, len: 40
				memcpy(scsi_temp,&desc[4],len);
				scsi_temp[len] = 0;
				printf("name: %s\n", scsi_temp);
				break;
#endif
			}
		}
			
#if 0
                if (piv && code_set == 1 && assoc == 1
                    && proto == SCSI_PROTOCOL_SAS && type == 3 && len == 8)
                        efd.addr = (u64)desc[4] << 56 |
                                (u64)desc[5] << 48 |
                                (u64)desc[6] << 40 |
                                (u64)desc[7] << 32 |
                                (u64)desc[8] << 24 |
                                (u64)desc[9] << 16 |
                                (u64)desc[10] << 8 |
                                (u64)desc[11];
#endif

                desc += len + 4;
        }
	value[0] = 0;
	strcat(value,wwn);

	return 1;
}

struct scsi_lun { unsigned char scsi_lun[8]; };
int scsilun_to_int(struct scsi_lun *scsilun) {
	int i;
	unsigned int lun;

	lun = 0;
	for (i = 0; i < sizeof(lun); i += 2)
		lun = lun | (((scsilun->scsi_lun[i] << 8) |
			      scsilun->scsi_lun[i + 1]) << (i * 8));
	return lun;
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

int get_lunid(filehandle_t fd, int *lunid) {
	unsigned char data[256];
	unsigned char *p;
	unsigned char cmd[12];
	unsigned long len = ntohl(256);
	int i,count;

	memset(&cmd,0,sizeof(cmd));
	cmd[0] = 0xa0;
	memcpy(&cmd[6],&len,4);
//	bindump("report luns cmd",&cmd,12);
	if (do_ioctl(fd, cmd, 10, data, sizeof(data))) {
		dprintf("get_lunid: do_ioctl failed!\n");
		return 1;
	}
//	bindump("report luns data",&data,32);
	p = data;
	count = _getlong(p);
	dprintf("count: %d\n", count);
	if (count > 248) count = 248;
//	bindump("report luns data",p,count+8);
	count /= 8;
	dprintf("count: %d\n", count);
	for(i=0; i < count; i++) {
		p += 8;
		*lunid = scsilun_to_int((struct scsi_lun *)p);
		dprintf("lun: %d\n", *lunid);
	}
	return 0;
}

int capacity(filehandle_t fd, int *size) {
	unsigned char cmd[16] = { 0x9e, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 96, 0, 0 };
	unsigned char data[96], *p;
	unsigned long long bytes;
	unsigned long long lba,len;

	*size = 0;

	/* First, try the 16 byte command */
	/* Next, try the 10 byte */

/*
Byte
76 5 4 3 2 1 0
0 OPERATION CODE (9Eh)
1 Reserved SERVICE ACTION (10h)
2 (MSB) LOGICAL BLOCK ADDRESS
9 (LSB)
10 (MSB) ALLOCATION LENGTH
13 (LSB)
14 Reserved PMI
15 CONTROL

Byte
0 OPERATION CODE (25h)
1 Reserved | Obsolete
2 (MSB) LOGICAL BLOCK ADDRESS
5 (LSB)
6 Reserved
7
8 Reserved PMI
9 CONTROL
*/

	p = data;
	cmd[0] = 0x9e;
	cmd[1] = 0x10;
	cmd[12] = sizeof(data);
	if (do_ioctl(fd, cmd, 16, data, sizeof(data)) == 0) {
//		lba = _getquad(p);
//		len = _getlong(p+8);
	        lba =  ((u64)*(p+0)) << 56 | ((u64)*(p+1)) << 48 | ((u64)*(p+2)) << 40 | ((u64)*(p+3)) << 32 | \
			((u64)*(p+4)) << 24 | ((u64)*(p+5)) << 16 | ((u64)*(p+6)) << 8 | ((u64)*(p+7));
		len = ((long long)*(p+8) << 24) | ((long long)*(p+9) << 16) | ((long long)*(p+10) << 8) | (long long)*(p+11);
		dprintf("got16: lba: %lld, len: %lld\n", lba, len);
	} else  {
		memset(cmd,0,sizeof(cmd));
		cmd[0] = 0x25;
		if (do_ioctl(fd, cmd, 10, data, sizeof(data)) == 0) {
//			lba = _getlong(p);
//			len = _getlong(p+4);
			lba = ((long long)*(p+0) << 24) | ((long long)*(p+1) << 16) | ((long long)*(p+2) << 8) | (long long)*(p+3);
			len = ((long long)*(p+4) << 24) | ((long long)*(p+5) << 16) | ((long long)*(p+6) << 8) | (long long)*(p+7);
			dprintf("got10: lba: %lld, len: %lld\n", lba, len);
		} else {
			dprintf("error: not able to get capacity!\n");
			lba = len = 0;
		}
	}
	dprintf("lba: %lld, len: %lld\n", lba, len);
	bytes = lba;
	bytes *= len;
//	dprintf("bytes: %lld\n", bytes);
	*size = bytes / 1048576;
	dprintf("size: %dMB\n", *size);
	return (bytes == 0);
}
