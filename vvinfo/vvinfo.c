
/* vvinfo - Written by stephen.shoecraft@hp.com */
/* Display 3PAR LUN Vendor specific info */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

//#define DEBUG 1

#ifndef INQUIRY
#define INQUIRY 0x12
#endif

#ifdef __MINGW32__
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
typedef void * filehandle_t;
#define OPENDEV(dev) CreateFile(dev,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
#define CLOSEDEV(fd) CloseHandle(fd);
#define INVALID_HANDLE(fd) (fd == INVALID_HANDLE_VALUE)
#else
typedef int filehandle_t;
#define OPENDEV(dev) open(dev,O_RDONLY|O_NONBLOCK);
#define CLOSEDEV(fd) close(fd)
#define INVALID_HANDLE(fd) (fd < 0)
#endif

#define _getlong(p) ((long) (*(p) << 24) | (*((p)+1) << 16) | (*((p)+2) << 8) | *((p)+3) )
#define _getquad(p) ((long long) (*(p) << 24) | (*((p)+1) << 16) | (*((p)+2) << 8) | *((p)+3) | (*((p)+4) << 24) | (*((p)+5) << 16) | (*((p)+6) << 8) | *((p)+7))
typedef unsigned long long u64;

#define KB (long double)1024
#define MB (KB*1024)
#define GB (MB*1024)
#define TB (GB*1024)

#ifndef dprintf
#if DEBUG
#if defined(__hpux)
#define dprintf(format, args...) printf(format,## args)
#else
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#endif
#else
#define dprintf(format, args...) /* noop */
#endif
#endif

char devdir[256];
int summary = 0;

char *trim(char *string) {
	register char *src,*dest;

	/* If string is empty, just return it */
	if (!string || *string == 0) return string;

	/* Trim the front */
	src = dest = string;
	while(*src > 0 && *src < 33) src++;
	while(*src) *dest++ = *src++;

	/* Trim the back */
	*dest-- = 0;
	while((dest >= string) && (*dest > 0 && *dest < 33)) dest--;
	*(dest+1) = 0;

	return string;
}

#if DEBUG
void _bindump(long offset,unsigned char *buf,int len) {
        char line[128];
        int end;
        register char *ptr;
        register int x,y;

        printf("buf: %p, len: %d\n", buf, len);
#ifdef __WIN32__
        if (buf == (void *)0xBAADF00D) return;
#endif

        for(x=y=0; x < len; x += 16) {
                sprintf(line,"%04lX: ",offset);
                ptr = line + strlen(line);
                end=(x+16 >= len ? len : x+16);
                for(y=x; y < end; y++) {
                        sprintf(ptr,"%02X ",buf[y]);
                        ptr += 3;
                }
                for(y=end; y < x+17; y++) {
                        sprintf(ptr,"   ");
                        ptr += 3;
                }
                for(y=x; y < end; y++) {
                        if (buf[y] > 31 && buf[y] < 127)
                                *ptr++ = buf[y];
                        else
                                *ptr++ = '.';
                }
                for(y=end; y < x+16; y++) *ptr++ = ' ';
                *ptr = 0;
                printf("%s\n",line);
                offset += 16;
        }
}
#else
#define _bindump(a,b,c) /* noop */
#endif

#if defined(__linux__)
#include <sys/ioctl.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>

static int is_vmware = 0;
void set_devdir(void) {
	if (access("/vmfs/devices/disks",0) == 0) {
		strcpy(devdir,"/vmfs/devices/disks");
		is_vmware = 1;
	} else
		strcpy(devdir,"/dev");
}

int do_ioctl(filehandle_t fd, unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
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
#define _error_text strerror(errno)

#elif defined(__hpux)
#include <sys/scsi.h>

void set_devdir(void) {
	strcpy(devdir,"/dev/rdsk");
}

static int do_ioctl(filehandle_t fd, unsigned char *cdb, int cdb_len, unsigned char *buffer, int buflen) {
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
		err = 1;
	} else {
		err = (pass.cdb_status != S_GOOD);
	}

	return err;
}
#define _error_text strerror(errno)

#elif defined(__MINGW32__)
#include <stddef.h> // For offsetof
#include <ddk/ntdddisk.h>
#include <ddk/ntddscsi.h>

void set_devdir(void) {
	devdir[0] = 0;
}

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

#define ERROR_TEXT_SIZE 1024
char *_error_text(void) {
	static char error_text[ERROR_TEXT_SIZE];
	DWORD dwRet;
	LPTSTR lpszTemp = NULL;
	DWORD nError = GetLastError();

	dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           nError,
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

	if ( !dwRet)
		sprintf( error_text, "Unknown error %u", (int)nError);
	else {
		// trim if supplied buffer is too small
		if ((long)ERROR_TEXT_SIZE < (long)dwRet+20)
			lpszTemp[ ERROR_TEXT_SIZE - 20 ] = 0;
		else
			//remove cr and newline character
			lpszTemp[lstrlen(lpszTemp)-2] = 0;
		sprintf( error_text, "%s (%u)", lpszTemp, (int)nError);
	}

	if ( lpszTemp ) LocalFree((HLOCAL) lpszTemp );

	return error_text;
}
#else
#error Unsupported OS!
#endif

int inquiry(filehandle_t fd, unsigned char *buffer, int *buflen) {
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


int get_inq_page(filehandle_t fd, int page, unsigned char *buffer, int buflen) {
	unsigned char cdb[12];

	dprintf("fd: %d, page: %x, buffer: %p, buflen: %d\n", (int)fd, page, buffer, buflen);

	memset(&cdb,0,sizeof(cdb));
	cdb[0] = INQUIRY;
	if (page >= 0) {
		cdb[1] = 1;
		cdb[2] = page;
	}
	cdb[3] = buflen / 256;
	cdb[4] = buflen % 256;

	return do_ioctl(fd, cdb, 6, buffer, buflen);
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

int get_capacity(filehandle_t fd, long long *size) {
	unsigned char cmd[16] = { 0x9e, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0 };
	unsigned char data[32], *p;
//	unsigned long long bytes;
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
	*size = lba;
	*size *= len;
	dprintf("size: %lld\n", *size);
#if 0
	bytes = lba;
	bytes *= len;
	dprintf("bytes: %lld\n", bytes);
	*size = bytes / 1048576;
	dprintf("size: %dMB\n", *size);
#endif
	return 0;
}

#if 0
int get_capacity(filehandle_t fd, long long *size) {
	unsigned char cmd[10] = { 0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char data[32], *p;
	unsigned long lba,len;

	if (do_ioctl(fd, cmd, 10, data, sizeof(data))) return 1;
	p = data;
	lba = (*(p+0) << 24) | (*(p+1) << 16) | (*(p+2) << 8) | *(p+3);
	len = (*(p+4) << 24) | (*(p+5) << 16) | (*(p+6) << 8) | *(p+7);
	dprintf("lba: %ld, len: %ld\n", lba, len);
	*size = lba;
	*size *= len;
	dprintf("size: %lld\n", *size);
	return 0;
}
#endif

void _strout(char *label,char *data) {
	char temp[64];

	sprintf(temp,"%s:",label);
	printf("%-30s %s\n",temp,data);
}

void _longout(char *label,unsigned char *data) {
	char temp[64];

	sprintf(temp,"%s:",label);
	printf("%-30s %ld\n",temp,_getlong(data));
}

void _quadout(char *label,unsigned char *data) {
	char temp[64];

	sprintf(temp,"%s:",label);
	printf("%-30s %llu\n",temp,_getquad(data));
}

void _lstrout(char *label,unsigned char *data,int max) {
	char temp[256];
	int len;

	len = _getlong(data);
        dprintf("len: %d\n", len);
	if (len < 0) len = 0;
	if (len > max) len = max;
	data += 4;
        memcpy(temp,data,max);
        temp[len-1] = 0;
        trim(temp);
        _strout(label,temp);
}

int disp(char *dev) {
	filehandle_t fd;
	char temp[1024],*p;
	char vendor[9], product[17], *pf;
	unsigned char data[255];
	long long size;
	long double sizef;
	int len;

	dprintf("dev: %s\n", dev);
	fd = OPENDEV(dev);
	if (INVALID_HANDLE(fd)) {
//		fprintf(stderr,"OPENDEV: %s\n",(char *)_error_text);
		return 1;
	}

	/* Get STD inquiry page */
	memset(data,0,sizeof(data));
	len = sizeof(data);
	if (std_inquiry(fd,data,&len)) {
//		fprintf(stderr,"STD inquiry: %s\n",(char *)_error_text);
		return 1;
	}
	_bindump(0,data,len);

	/* Check vendor/product */
	memcpy(vendor, &data[8], 8);
	vendor[8] = 0;
	trim(vendor);
	dprintf("vendor: %s\n", vendor);
	memcpy(product, &data[16], 16);
	product[16] = 0;
	trim(product);
	dprintf("product: %s\n", product);
	if (strcmp(vendor,"3PARdata") !=0 || strcmp(product,"VV") != 0) {
//		printf("vvinfo: not a 3PAR VV LUN.\n");
		return 1;
	}

	/* Get capacity */
	if (get_capacity(fd, &size)) {
		fprintf(stderr,"get_capacity: %s\n",(char *)_error_text);
		return 1;
	}
	dprintf(">>>>> size: %lld\n", size);

	/* Get Vendor-specific page (0xC0) */
	memset(data,0,sizeof(data));
	if (get_inq_page(fd, 0xC0, data, sizeof(data))) {
		fprintf(stderr,"get_inq_page: %s\n",(char *)_error_text);
		return 1;
	}
	len = data[3];
	_bindump(0,data,len);

	/* XXX even the versions im testing against are 4 ...
	dprintf("page_version: %d\n", data[4]);
	if (data[4] != 3) {
		printf("error: version != 3\n");
		return 1;
	}
	*/

	/* Display the info */
	_strout("Device",dev);
	sizef = size;
//printf("%Lf\n", sizef);
//printf("%Lf\n", sizef / 1048576);
	if (sizef > TB) {
		sizef /= TB;
//		printf("TB: %Lf\n", size / TB);
		pf = "TB";
	} else if (sizef > GB) {
		sizef /= GB;
//		printf("GB: %Lf\n", size / GB);
		pf = "GB";
	} else if (sizef > MB) {
		sizef /= MB;
//		printf("MB: %Lf\n", size / MB);
		pf = "MB";
	} else if (sizef > KB) {
		sizef /= KB;
//		printf("KB: %Lf\n", size / KB);
		pf = "KB";
	} else {
		pf = 0;
	}
	if (pf) {
		sprintf(temp,"%lld (%.0Lf%s)",size,sizef,pf);
	} else {
		sprintf(temp,"%lld",size);
	}
	_strout("Capacity",temp);
	if (summary) {
		_strout("Volume name",((char *)&data[44]));
		_quadout("VV ID",&data[28]);
		return 0;
	}
	temp[0] = 0;
	p = temp;
	if (data[5] & 0x01) p += sprintf(p,"Is_TPVV ");
	if (data[5] & 0x02) p += sprintf(p,"Is_Snapshot ");
	if (data[5] & 0x04) p += sprintf(p,"TPVV_Reclaim_Supported ");
	if (data[5] & 0x08) p += sprintf(p,"TPVV_SizesValid ");
	if (data[5] & 0x10) p += sprintf(p,"ATS_Supported ");
	if (data[5] & 0x20) p += sprintf(p,"XCOPY_Supported ");
	if (data[5] & 0x40) p += sprintf(p,"VV_Info_Valid ");
	trim(temp);
	dprintf("temp: %s\n", temp);
	_strout("Flags",temp);
	_longout("TPVV allocation unit",&data[8]);
	_quadout("TPVV data pool",&data[12]);
	_quadout("TPVV space allocated",&data[20]);
	_strout("Volume name",((char *)&data[44]));
	if (data[5] & 0x40) {
		_quadout("VV ID",&data[28]);
		_longout("Domain ID",&data[36]);
		_lstrout("Domain name",&data[76],32);
	}
	_lstrout("User CPG name",&data[112],32);
	_lstrout("Snap CPG name",&data[148],32);
	temp[0] = 0;
	p = temp;
	len = (int) _getlong(&data[184]);
	if (len & 0x01) p += sprintf(p,"Stale_SS ");
	if (len & 0x02) p += sprintf(p,"One_Host ");
	if (len & 0x04) p += sprintf(p,"TP_Bzero ");
	if (len & 0x08) p += sprintf(p,"Zero_Detect ");
	dprintf("temp: %s\n", temp);
	_strout("VV Policy",temp);

	CLOSEDEV(fd);
	return 0;
}

#ifdef __WIN32
int disp_all(void) { return 1; }
#else

#include <dirent.h>

int disp_all(void) {
	char name[256];
	DIR *dirp;
	struct dirent *ent;
	struct stat sb;
	int count;

	dprintf("devdir: %s\n", devdir);
	dirp = opendir(devdir);
	if (!dirp) {
		fprintf(stderr,"opendir: %s\n",(char *)_error_text);
		return 1;
	}
	count = 0;
	while((ent = readdir(dirp)) != 0) {
		if (strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0) continue;
#if defined(__linux__)
		if ((is_vmware == 0) && (strncmp(ent->d_name,"sd",2) != 0)) continue;
#endif
		if (is_vmware) {
			if (strncmp(ent->d_name,"naa.",4) != 0) continue;
			if (strchr(ent->d_name,':')) continue;
		}
		name[0] = 0;
		strcat(name,devdir);
		strcat(name,"/");
		strncat(name,ent->d_name,sizeof(name)-strlen(name));
		dprintf("name: %s\n", name);
		if (stat(name,&sb) < 0) {
			dprintf("cant stat\n");
			continue;
		}
#if 0
#if defined(__linux__)
		if (!S_ISBLK(sb.st_mode)) {
			dprintf("not a block dev\n");
			continue;
		}
		p = name + strlen(name) - 1;
		dprintf("p: %c\n", *p);
		if (isdigit(*p)) continue;
#elif defined(__hpux)
		if (strstr(name,"_p")) continue;
#endif	
#endif
		dprintf("displaying: %s\n", name);
		if (disp(name) == 0) {
			printf("\n");
			count++;
		}
	}
	closedir(dirp);
	if (!count) printf("no 3PAR luns found.\n");
	return 0;
}
#endif

int main(int argc, char **argv) {
	set_devdir();
//	printf("argc: %d\n", argc);
	if (argc > 1) {
		if (strcmp(argv[1],"-s") == 0) {
			summary = 1;
			argc--;
		}
	}
//	printf("argc: %d\n", argc);
	return (argc < 2 ? disp_all() : disp(argv[1]));
}
