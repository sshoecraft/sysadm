
#include "luninfo.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#ifdef _WIN32
#include "win32.h"
#else
#include "esx.h"
#endif
#include "xp.h"
#include "vv.h"

int compdev(list_item i1, list_item i2) {
	register const struct luninfo *r1 = i1->item;
	register const struct luninfo *r2 = i2->item;
	int v;

	v = strcmp(r1->dev, r2->dev);
	if (v < 0) v = -1;
	else if (v > 0) v = 1;
//	dprintf("v: %d, r1: %s, r2: %s\n", v, r1->dev, r2->dev);
	return v;
}

int compvol(list_item i1, list_item i2) {
	register const struct luninfo *r1 = i1->item;
	register const struct luninfo *r2 = i2->item;
	int v;

	v = strcmp(r1->vol, r2->vol);
	if (v < 0) v = -1;
	else if (v > 0) v = 1;
//	dprintf("v: %d, r1: %s, r2: %s\n", v, r1->vol, r2->vol);
	return v;
}

void remove_dups(list devs) {
	struct luninfo *d1,*d2;
	list_item save_next;

	list_reset(devs);
	while((d1 = list_get_next(devs)) != 0) {
		dprintf("d1->dev: %s, d1->wwn: %s\n", d1->dev, d1->wwn);
		if (!strlen(d1->wwn)) continue;
		save_next = devs->next;
		list_reset(devs);
		while((d2 = list_get_next(devs)) != 0) {
			if (d2 == d1) continue;
			if (!strlen(d2->wwn)) continue;
			dprintf("d2->dev: %s, d2->wwn: %s\n", d2->dev, d2->wwn);
			if (strcmp(d1->wwn,d2->wwn) == 0) {
				/* Keep the one with the vol info filled in */
				dprintf("d1->vol: %s, d2->vol: %s\n", d1->vol, d2->vol);
				if (d1->vol[0] == 0 && d2->vol[0] != 0)
					list_delete(devs, d1);
				else if (d2->vol[0] == 0 && d1->vol[0] != 0)
					list_delete(devs, d2);
				else
					list_delete(devs, d1);
//				break;
			}
		}
		devs->next = save_next;
	}
}

static struct _vfuncs {
	char *vendor;
	char *product;
	int (*func)(filehandle_t,struct luninfo *);
} vfuncs[] = {
	{ "HP", "OPEN-V", get_xpinfo },
	{ "3PARdata", "VV", get_vvinfo },
	{ 0,0,0 }
};

static void get_spec(filehandle_t fd, struct luninfo *info) {
	struct _vfuncs *vfp;

//	printf("vendor: %s, product: %s\n", info->vendor, info->product);
	for(vfp = vfuncs; vfp->product; vfp++) {
		if (strcmp(vfp->vendor,info->vendor) == 0 && strcmp(vfp->product,info->product) == 0) {
			vfp->func(fd,info);
		}
	}
}

list get_info(int all,int namesort,int vols) {
	list lp,devs,mpdevs;
	struct luninfo newent,*info;
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
		dprintf("fd: %d\n", fd);
		if (INVALID_HANDLE(fd)) continue;
		dprintf("opened.\n");

//		if (!isready(fd)) continue;

		memset(&newent,0,sizeof(newent));
		strcpy(newent.dev, p);

		dprintf("getting inquiry\n");
		len = sizeof(newent.inqdata);
		if (inquiry(fd,(unsigned char *)&newent.inqdata,&len)) continue;
#if DEBUG
		sprintf(temp,"inquiry_data(%d)", len);
		bindump(temp,newent.inqdata,len);
#endif

		/* Vendor */
		memcpy(vendor, &newent.inqdata[8], 8);
		vendor[8] = 0;
		trim(vendor);
		dprintf("vendor: %s\n", vendor);

		/* Product */
		memcpy(product, &newent.inqdata[16], 16);
		product[16] = 0;
		trim(product);
		dprintf("product: %s\n", product);

		/* Get capacity */
		if (capacity(fd,&cap)) {
			fsize = -1;
			unit = "";
		} else if (cap == 0) {
			continue;
		} else {
			unit = "MB";
			fsize = (float)cap;
			dprintf("size: %4.2f\n", fsize);
			newent.size = fsize;
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
		if (fsize < 0) continue;

		sprintf(temp,"%4.0f",fsize);
		isize = atof(temp);
		dprintf("isize: %d\n", isize);
		sprintf(newent.ssize,"%5d%s",isize,unit);
		strcpy(newent.vendor, vendor);
		strcpy(newent.product, product);

		get_wwn(fd,(char *)&newent.wwn);
#define DD "/dev/disks/"
#define VD "/vmfs/devices/disks/"
		if (newent.wwn[0] == 0 && (strncmp(newent.dev,DD,strlen(DD)) == 0 || strncmp(newent.dev,VD,strlen(VD)) == 0)) {
			p = newent.dev;
			if (strncmp(p,DD,strlen(DD)) == 0)
				p += strlen(DD);
			else if (strncmp(p,VD,strlen(VD)) == 0)
				p += strlen(VD);
			strncat(newent.wwn,p,sizeof(newent.wwn));
		}
		get_lunid(fd,&newent.lunid);
		get_spec(fd,&newent);

		dprintf(">>>ADDING: dev: %s\n", newent.dev);
		list_add(lp, &newent, sizeof(newent));
		CLOSEDEV(fd);
	}

	mpdevs = list_create();

	if (vols) {
#ifndef __MINGW32__
		/* Get multipath map */
		if (mpdevs) get_mp(mpdevs);

		/* Get ESX volume names */
		get_esxvols(lp);

		/* Get LVM VG names */
#if defined(__linux__) || defined(__hpux)
		if (mpdevs) get_vgnames(lp, mpdevs);
#endif
#else
		get_win32vols(lp);
#endif
		/* Get VxVM DG names */
		if (mpdevs) get_dgnames(lp, mpdevs);
	}

//	printf("vols: %d, namesort: %d\n", vols, namesort);
	if (vols && namesort == 0) {
		/* Sort by volume name */
		list_sort(lp, compvol, 0);
	} else {
		/* Sort by devname */
		list_sort(lp, compdev, 0);
	}

	/* Remove duplicates */
	if (!all) remove_dups(lp);

	list_reset(lp);
	while((info = list_get_next(lp)) != 0) {
		dprintf("dev: %s\n", info->dev);
	}

	return lp;
}
