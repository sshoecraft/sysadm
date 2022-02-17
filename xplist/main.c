
/*
 * xplist - list xp san devices available on the system
 *
 * Steve Shoecraft (stephen.shoecraft@hp.com)
*/

#include "xplist.h"
#include "version.h"

char temp[1024];
list mpdevs;

#ifdef __MINGW32__
#define DEFTEMPDIR "c:\\windows\\temp"
#else
#define DEFTEMPDIR "/tmp"
#endif
char tempdir[256];

int compid(list_item i1, list_item i2) {
	register const struct xplist_entry *r1 = i1->item;
	register const struct xplist_entry *r2 = i2->item;
	int val1,val2;

	val1 = r1->cu << 8 | r1->ldev;
//	printf("1: cu: %02x, ldev: %02x, val: %04x\n", r1->cu, r1->ldev, val1);
	val2 = r2->cu << 8 | r2->ldev;
//	printf("2: cu: %02x, ldev: %02x, val: %04x\n", r2->cu, r2->ldev, val2);
	if (val1 < val2)
		return -1;
	else if (val1 == val2)
		return 0;
	else
		return 1;
}

int compdev(list_item i1, list_item i2) {
	register const struct xplist_entry *r1 = i1->item;
	register const struct xplist_entry *r2 = i2->item;

	return strcmp(r1->dev, r2->dev);
}

void remove_dups(list devs) {
	struct xplist_entry *xp,*xp2;
	list_item save_next;

	list_reset(devs);
	while((xp = list_get_next(devs)) != 0) {
		save_next = devs->next;
		list_reset(devs);
		while((xp2 = list_get_next(devs)) != 0) {
			if (xp2 == xp) continue;
			if (xp->cu == xp2->cu && xp->ldev == xp2->ldev && xp->serial == xp2->serial) {
				/* Keep the one with the vol info filled in */
				if (xp->vol[0])
					list_delete(devs, xp2);
				else
					list_delete(devs, xp);
				break;
			}
		}
		devs->next = save_next;
	}
}

int main(int argc, char **argv) {
	list devs;
	struct xplist_entry *ent;
	int ch, all, dump, namesort, help, vols, vers, head, lunid;
	char line[128], *p;

	p = getenv("TEMP");
	if (!p) {
		p = DEFTEMPDIR;
	} else {
		if (!strlen(p))
			p = DEFTEMPDIR;
	}
	strcpy(tempdir,p);
	dprintf("tempdir: %s\n", tempdir);

	all = namesort = dump = vols = vers = help = lunid = 0;
	head = 1;
	while((ch = getopt(argc, argv, "adhnvVHl")) != -1) {
		switch(ch) {
		case 'H':
			head = 0;
			break;
		case 'a':
			all = 1;
			break;
		case 'd':
			dump = 1;
			break;
		case 'n':
			namesort = 1;
			break;
		case 'v':
			vers = 1;
			break;
		case 'V':
			vols = 1;
			break;
		case 'l':
			lunid = 1;
			break;
		default:
		case 'h':
			help = 1;
			break;
		}
	}
	if (dump) vols = 0;
	dprintf("all: %d, dump: %d, namesort: %d, vers: %d\n", all, dump, namesort, vers);

	if (help) {
		printf("usage: xplist [-adhnvVl]\n");
		printf("  where:\n");
		printf("    -H             dont display header\n");
		printf("    -a             show all devs\n");
		printf("    -d             dump in comma seperated format\n");
		printf("    -n             sort devs by name\n");
		printf("    -v             display version\n");
		printf("    -V             display volume\n");
		printf("    -l             display lunid\n");
		printf("    -h             this listing\n");
		return 1;
	}

	if (vers) {
		printf("xplist version %s\n", VERSIONSTR);
		printf("Copyright(C) 2010 ShadowIT, Inc.\n");
		printf("No rights reserved anywhere.\n");
		return 0;
	}

#if 0
#ifndef __MINGW32__
	/* Gotta be root */
	if (getuid() != 0) {
		fprintf(stderr,"You must be root to run this program.\n");
		return 1;
	}
#endif
#endif

	/* Get a list of the devices */
	devs = get_xplist();
	if (!devs) return 1;

	mpdevs = list_create();
	if (!mpdevs) {
		perror("list_create");
		return 1;
	}

	if (vols) {
#ifndef __MINGW32__
		/* Get multipath map */
		get_mp();

		/* Get ESX volume names */
		get_esxvols(devs);

		/* Get LVM VG names */
		get_vgnames(devs);
#endif

		/* Get VxVM DG names */
		get_dgnames(devs);
	}

	/* Remove duplicates */
	if (!all) remove_dups(devs);

	/* Sort list */
	if (namesort)
		list_sort(devs, compdev, 0);
	else
		list_sort(devs, compid, 0);

	list_reset(devs);
	if (dump) {
//		/dev/rdsk/c6t4d1,,,CL2B,01:2a,OPEN-V,65536,00022942,,,,,,,,,,,,,,,,,,,,0000599e0000012a
//		/dev/rdsk/c6t4d1,04,21,CL2B,01:2a,OPEN-V,65536,00022942,5001,0005,5,PVOL,SMPL,SMPL,SMPL,3,RAID5,7-13,L100C,L110C,L120C,L130C,XP12000,50060e8004599e11,c9,14,---,0000599e0000012a,dgsan,VXVM,/dev/vx/dsk/dgsan/lvol1:/sanfs ,c6t4d1 c10t4d1 ,50060b000039a373,50060b000039a372,---,---,---,---
//		/dev/sda,00,00,CL2H,09:cc,OPEN-V,16384,00032439,5009,000d,---,SMPL,SMPL,SMPL,SMPL,2,RAID5,5-4,R1013,R1113,R1213,R1313,XP12000,50060e80047eb717,76,3c,---,00007eb7000009cc,---,---,---,---,50060b0000c17fed,50060b0000c17fec,0,---,---,---
//		/dev/sda,00,00,CL2H,09:cc,OPEN-V,16384,00032439,,,,,,,,,,,,,,,,,,,,00007eb7000009cc,
#if 0
Device File : /dev/sda                                 Model : XP12000
       Port : CL2H                                  Serial # : 00032439
Host Target : 00                                    Code Rev : 5009
  Array LUN : 00                                   Subsystem : 000d
    CU:LDev : 09:cc                                 CT Group : ---
       Type : OPEN-V                               CA Volume : SMPL
       Size : 16384 MB                            BC0 (MU#0) : SMPL
       ALPA : 76                                  BC1 (MU#1) : SMPL
    Loop Id : 3c                                  BC2 (MU#2) : SMPL
    SCSI Id : ---
 RAID Level : RAID5                               RAID Type  : ---
 RAID Group : 5-4                                   ACP Pair : 2
 Disk Mechs : R1013   R1113   R1213   R1313
     FC-LUN : 00007eb7000009cc                      Port WWN : 50060e80047eb717
HBA Node WWN: 50060b0000c17fed                   HBA Port WWN: 50060b0000c17fec
 Vol Group  : ---                                Vol Manager : ---
Mount Points: ---
  DMP Paths : ---
  SLPR : 0                                         CLPR : 0
#endif
		while((ent = list_get_next(devs)) != 0)
			printf("%s,,,%s,%02x:%02x,%s,%s,%08d,,,,,,,,,,,,,,,,,,,,%016llx,%s\n",
				ent->dev, ent->port, ent->cu, ent->ldev,
				ent->type, ent->size, ent->serial, ent->id, ent->vol);
	} else {
		if (head) {
			line[0] = 0;
			p = line;
			p += sprintf(p,"%-20s ", "Device");
			if (vols) p += sprintf(p,"%-19s ", "Volume");
			p += sprintf(p,"%-11s ", "Size");
			if (lunid) p += sprintf(p,"%-5s ", "LUN");
			p += sprintf(p,"%-6s %-8s %-10s %s\n", "Port", "CU:LDev", "Type", "Serial #");
			printf(line);
			for(p=line; *p; p++) *p = '=';
			*(p-1) = '\n';
			printf(line);
		}
		ch=1;
		while((ent = list_get_next(devs)) != 0) {
			line[0] = 0;
			p = line;
			if (strncmp(ent->dev,"/dev/disks/naa",14) == 0) sprintf(ent->dev,"Disk %02d", ch++);
			p += sprintf(p,"%-20s ", ent->dev);
			if (vols) p += sprintf(p,"%-19s ", ent->vol);
			sprintf(temp, "%s", ent->size);
			p += sprintf(p,"%-11s ", temp);
			if (lunid) {
				sprintf(temp, "%03d", ent->lunid);
				p += sprintf(p,"%-5s ", temp);
			}
			p += sprintf(p,"%-6s ", ent->port);
			sprintf(temp, "%02x:%02x", ent->cu, ent->ldev);
			p += sprintf(p,"%-8s ", temp);
			p += sprintf(p,"%-10s ", ent->type);
			sprintf(temp, "%08d", ent->serial);
			p += sprintf(p,"%s", temp);
			printf("%s\n",line);
		}
	}
	return 0;
}
