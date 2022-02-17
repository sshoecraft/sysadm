
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include "vv.h"

int get_vvinfo(filehandle_t fd, struct luninfo *info) {
	return 0;
}

#if 0
#include "luninfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

//#define DEBUG 1

char devdir[256];
int summary = 0;

struct vvinfo {
	char volume[32];
	unsigned long long id;		/* 64-bit device ID */
};

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
#endif
