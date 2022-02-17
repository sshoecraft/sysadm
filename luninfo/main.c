
/*
 * luninfo - list luns on the system
 *
 * Steve Shoecraft (sshoecraft@earthlink.net)
*/

#include "luninfo.h"
#include "version.h"
#ifdef _WIN32
#include "win32.h"
#else
#include "esx.h"
#endif
#include "xp.h"

enum FLAGS {
	HEAD=1,
	ALL,
	DUMP,
	NAMESORT,
	VERS,
	VOLS,
	LUNID,
	HELP,
};

int wwn_type;

int main(int argc, char **argv) {
	char temp[1024];
	list devs;
	struct luninfo *info;
	int ch, dump, help, all, namesort, vols, vers, head, lunid, comma, spec, maxlen, labels, omitdev;
	int maxvol;
	char line[128], *p, devfmt[8];

	all = namesort = dump = vols = vers = help = lunid = comma = spec = labels = omitdev = 0;
//	lunid = 1;
	head = 1;
	wwn_type = 0;
	while((ch = getopt(argc, argv, "adhNvVHlcstno")) != -1) {
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
		case 'c':
			comma = 1;
			break;
		case 'n':
			head = 0;
			break;
		case 'N':
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
		case 'o':
			omitdev = 1;
			break;
		case 's':
			spec = 1;
			break;
		case 't':
			wwn_type = 1;
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
		printf("    -c             comma delimited output\n");
		printf("    -s             display vendor specific info\n");
		printf("    -t             prefix wwn with type\n");
		printf("    -h             this listing\n");
		return 1;
	}

	if (vers) {
		printf("luninfo version %s\n", VERSIONSTR);
		printf("By Steve Shoecraft (sshoecraft@earthlink.net)\n");
		return 0;
	}

#ifdef __win32
//	SecurityIdentifier sidAdmin = new SecurityIdentifier(WellKnownSidType.BuiltinAdministratorsSid, null);
//	if (!user.IsInRole(sidAdmin)) {
//	}
#endif

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
	devs = get_info(all,namesort,vols);
	if (!devs) return 1;

	ch = 1;
	maxlen = 0;
	list_reset(devs);
	maxvol = 12;
	while((info = list_get_next(devs)) != 0) {
		if (strlen(info->label) == 0 && strlen(info->dev) != 0) {
#define MAINDD "/dev/disks/"
#define MAINVD "/vmfs/devices/disks/"
//			printf("info->dev: %s\n", info->dev);
			if (strncmp(info->dev,MAINDD,strlen(MAINDD)) == 0 || strncmp(info->dev,MAINVD,strlen(MAINVD)) == 0)
				info->label[0] = 0;
			else
				strcpy(info->label,info->dev);
		}
		if (strlen(info->label) > maxlen)
		maxlen = strlen(info->label);
		if (vols) {
			trim(info->vol);
			if (strlen(info->vol) > maxvol)
				maxvol = strlen(info->vol);
		}
	}
	dprintf("maxvol: %d\n", maxvol);
	sprintf(devfmt,"%%-%ds ",maxlen+1);
	dprintf("devfmt: %s\n", devfmt);

	if (!omitdev) {
	list_reset(devs);
	while((info = list_get_next(devs)) != 0) {
//		printf("info->label: %s\n", info->label);
		if (strlen(info->label) != 0) {
			labels = 1;
			break;
		}
	}
	}

	list_reset(devs);
	if (dump) {
		struct xpinfo xp_dummy, *xp;
		memset(&xp_dummy,0,sizeof(xp_dummy));
		while((info = list_get_next(devs)) != 0) {
			if (info->extra) {
				xp = (struct xpinfo *)info->extra;
			} else {
				xp = &xp_dummy;
			}
			/* XXX orig xplinfo -d output 
			printf("%s,,,%s,%02x:%02x,%s,%s,%08d,,,,,,,,,,,,,,,,,,,,,\n",
			*/
			printf("%s,%d,%s,%s,%02x:%02x,%s,%s,%08d,%s\n",
				info->dev, info->size, info->wwn, xp->port, xp->cu, xp->ldev,
				info->product, info->ssize, xp->serial, info->vol);
		}
	} else {
		if (head) {
			line[0] = 0;
			p = line;
			if (comma && head) {
				p += sprintf(p,"Device,Volume,Size,LUN,Vendor,Product,ID,Spec\n");
			} else {
				if (labels) p += sprintf(p,devfmt, "Device");
				if (vols) {
					sprintf(temp,"%%-%ds ", maxvol);
					p += sprintf(p, temp, "Volume");
				}
				p += sprintf(p,"%-11s ", "Size");
				if (lunid) p += sprintf(p,"%-5s ", "LUN");
				p += sprintf(p,"%-10s ", "Vendor");
				p += sprintf(p,"%-16s ", "Product");
				p += sprintf(p,"%-32s ", "ID");
				if (spec) p += sprintf(p,"%-20s ", "Spec");
				sprintf(p,"\n");
				printf(line);
				for(p=line; *p; p++) *p = '=';
				*(p-1) = '\n';
			}
			printf(line);
		}
		while((info = list_get_next(devs)) != 0) {
			line[0] = 0;
			p = line;
			if (comma) {
				printf("%s,%s,%d,%d,%s,%s,%s,%s\n", info->label, info->vol, info->size, info->lunid,
					info->vendor, info->product, info->wwn, info->spec);
			} else {
				if (labels) p += sprintf(p,devfmt, info->label);
				if (vols) {
					sprintf(temp,"%%-%ds ", maxvol);
					p += sprintf(p, temp, info->vol);
				}
				sprintf(temp, "%s", info->ssize);
				p += sprintf(p,"%-11s ", temp);
				if (lunid) {
					sprintf(temp, "%03d", info->lunid);
					p += sprintf(p,"%-5s ", temp);
				}
				p += sprintf(p,"%-10s ", info->vendor);
				p += sprintf(p,"%-16s ", info->product);
				p += sprintf(p,"%-32s ", info->wwn);
				if (spec) p += sprintf(p,"%-20s ", info->spec);

				printf("%s\n",line);
			}
		}
	}
	return 0;
}
