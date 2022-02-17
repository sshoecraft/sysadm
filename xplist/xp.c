
/*
 * xplist - list xp san devices available on the system
 *
 * Steve Shoecraft (stephen.shoecraft@hp.com)
*/

#include "xplist.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

#define INQLEN 96

/* List vendors/products we support */
struct _sup {
	char *vendor;
	char *product;
} supported[] = {
	{ "HP", "OPEN-*" },
	{ "HP", "HSV*" },
	{ "HP", "P2000*" },
	{ "COMPAQ", "HSV*" },
	{ "COMPAQ", "MSA*" },
	{ "3PARdata", "VV" },
	{ "NETAPP", "LUN C-Mode" },
	{ 0, 0 }
};

int supported_vendor(char *vendor) {
	struct _sup *sup;

	dprintf("looking for: vendor: %s\n", vendor);
	for(sup=supported; sup->vendor; sup++) {
//		dprintf("sup->vendor: %s, vendor: %s\n", sup->vendor, vendor);
		if (strcmp(sup->vendor,vendor) == 0) {
			dprintf("found\n");
			return 1;
		}
	}
	dprintf("not found.\n");
	return 0;
}

int supported_product(char *vendor, char *product) {
	struct _sup *sup;

	dprintf("looking for: vendor: %s, product: %s\n", vendor, product);
	for(sup=supported; sup->vendor; sup++) {
//		dprintf("vendor: %s, product: %s\n", sup->vendor, sup->product);
		if (strcmp(sup->vendor,vendor) == 0) {
			if (strchr(sup->product,'*') && strncmp(product,sup->product,strlen(sup->product)-1) == 0) {
				dprintf("found\n");
				return 1;
			}
			if (strcmp(sup->product,product) == 0) {
				dprintf("found\n");
				return 1;
			}
		}
	}
	dprintf("not found.\n");
	return 0;
}

#ifdef __MINGW32__
#define OPENDEV(dev) CreateFile(dev,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
#define CLOSEDEV(fd) CloseHandle(fd);
#define INVALID_HANDLE(fd) (fd == INVALID_HANDLE_VALUE)
#else
#define OPENDEV(dev) open(dev,O_RDONLY|O_NONBLOCK);
#define CLOSEDEV(fd) close(fd)
#define INVALID_HANDLE(fd) (fd < 0)
#endif

static int num = 0;

list get_xplist(void) {
	list lp,devs;
	unsigned char data[256];
	struct xplist_entry newent;
	char dev[DEV_SIZE], vendor[9], product[17], temp[128], *unit, *p;
	int len,cap,isize;
	float fsize;
	filehandle_t fd;

	/* Get a list of disks */
	devs = get_devs();
	if (!devs) return 0;

	lp = list_create();
	if (!lp) return 0;

	/* For each disk... */
	list_reset(devs);
	while((p = list_get_next(devs)) != 0) {
		dev[0] = 0;
		strcat(dev, p);
		dprintf("dev: %s\n", dev);

		fd = OPENDEV(dev);
		if (INVALID_HANDLE(fd)) continue;
		memset(data,0,sizeof(data));
		len = sizeof(data);
		if (inquiry(fd,data,&len)) continue;
#if DEBUG
		sprintf(temp,"inquiry_data(%d)", len);
		bindump(temp,data,len);
#endif

		/* Check if supported vendor */
		memcpy(vendor, &data[8], 8);
		vendor[8] = 0;
		trim(vendor);
		dprintf("vendor: %s\n", vendor);
		if (!supported_vendor(vendor)) {
			CLOSEDEV(fd);
			continue;
		}

		/* Check if supported product */
		memcpy(product, &data[16], 16);
		product[16] = 0;
		trim(product);
		dprintf("product: %s\n", product);
		if (!supported_product(vendor,product)) {
			CLOSEDEV(fd);
			continue;
		}

		/* Get capacity */
		if (capacity(fd,&cap)) {
			fsize = -1;
			unit = "";
		} else {
			unit = "MB";
			fsize = (float)cap;
			dprintf("size: %4.2f\n", fsize);
			if (fsize > 1024.0) {
				fsize /= 1024.0;
				unit = "GB";
			}
#if 0
			if (fsize > 1024.0) {
				fsize /= 1024.0;
				unit = "TB";
			}
#endif
		}
		dprintf("fsize: %4.2f, unit: %s\n", fsize, unit);

		/* We have a supported disk, fill in the info */
		memset(&newent,0,sizeof(newent));
		strcpy(newent.dev, p);
		sprintf(temp,"%4.0f",fsize);
		isize = atof(temp);
		dprintf("isize: %d\n", isize);
		sprintf(newent.size,"%3d%s",isize,unit);
		strcpy(newent.type, product);

		/* Get NAA */
//		identify(fd,&newent);
//		get_naa(fd,&newent);
		get_lunid(fd,&newent.lunid);

		/* Info is embedded in vendor_specific */
		if (strncmp(product,"OPEN",4) == 0) {
			memcpy(temp,&data[39],5);
			temp[5] = 0;
			dprintf("serial string: %s\n", temp);
			newent.serial = strtol(temp, (char **)NULL, 16);
			dprintf("serial: %d\n", newent.serial);
			temp[0] = data[36+8];
			temp[1] = data[36+9];
			temp[2] = 0;
			newent.cu = strtol(temp, 0, 16);
			dprintf("cu: %02x\n", newent.cu);
			temp[0] = data[36+10];
			temp[1] = data[36+11];
			temp[2] = 0;
			newent.ldev = strtol(temp, 0, 16);
			dprintf("ldev: %02x\n", newent.ldev);
			newent.port[0] = 'C';
			newent.port[1] = 'L';
			newent.port[2] = data[36+13];
			newent.port[3] = data[36+14];
			newent.port[4] = 0;
			dprintf("port: %s\n", newent.port);
#if 0
		} else if (strncmp(product,"HSV",3) == 0) {
			/* Get unique identity (NAA) */
			identify(fd,&newent);

			memcpy(temp,&data[32],5);
			temp[5] = 0;
			dprintf("serial string: %s\n", temp);
			newent.serial = strtol(temp, (char **)NULL, 10);
			newent.serial = serial;
			newent.cu = cu;
			newent.ldev = ldev;
			strcpy(newent.port,"UN00");
#endif
#if 0
		} else if (strcmp(product,"VV") == 0) {
			get_vvinfo(fd,&vvinfo);
			newent.serial = vvinfo.domain_id;
			newent.cu = 0;
			newent.ldev = 0;
#endif
		} else {
//			printf("Unknown product: %s:%s\n", vendor, product);
			newent.serial = 0;
			newent.cu = 0;
			newent.ldev = num++;
//			newent.port[0] = 0;
//			strcpy(newent.port,"UN00");
		}

		/* Generate a unique id from the serial+cu+ldev */
		newent.id = newent.serial;
		newent.id <<= 32;
		newent.id |= (unsigned long long) newent.cu << 8;
		newent.id |= (unsigned long long) newent.ldev;
		dprintf("id: %llx\n", newent.id);

		list_add(lp, &newent, sizeof(newent));

		CLOSEDEV(fd);
	}

	return lp;
}
