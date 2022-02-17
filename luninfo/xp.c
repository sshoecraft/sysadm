
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include "xp.h"

int compid(list_item i1, list_item i2) {
#if 0
	register const struct luninfo *r1 = i1->item;
	register const struct luninfo *r2 = i2->item;
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
#endif
		return 1;
}

int get_xpinfo(filehandle_t fd, struct luninfo *info) {
	char temp[128];
	struct xpinfo xp;

	memcpy(temp,&info->inqdata[39],5);
	temp[5] = 0;
	dprintf("serial string: %s\n", temp);
	xp.serial = strtol(temp, (char **)NULL, 16);
	dprintf("serial: %d\n", xp.serial);
	temp[0] = info->inqdata[36+8];
	temp[1] = info->inqdata[36+9];
	temp[2] = 0;
	xp.cu = strtol(temp, 0, 16);
	dprintf("cu: %02x\n", xp.cu);
	temp[0] = info->inqdata[36+10];
	temp[1] = info->inqdata[36+11];
	temp[2] = 0;
	xp.ldev = strtol(temp, 0, 16);
	dprintf("ldev: %02x\n", xp.ldev);
	xp.port[0] = 'C';
	xp.port[1] = 'L';
	xp.port[2] = info->inqdata[36+13];
	xp.port[3] = info->inqdata[36+14];
	xp.port[4] = 0;
	dprintf("port: %s\n", xp.port);

	/* Save a copy for dump if requested */
	info->extra = malloc(sizeof(struct xpinfo));
	if (info->extra) memcpy(info->extra,&xp,sizeof(xp));

#if 0
                               p += sprintf(p,"%-6s ", info->port);
                                sprintf(temp, "%02x:%02x", info->cu, info->ldev);
                                p += sprintf(p,"%-8s ", temp);
                                p += sprintf(p,"%-10s ", info->type);
                                sprintf(temp, "%08d", info->serial);
                                p += sprintf(p,"%s", temp);
#endif

	sprintf(info->spec,"%s/%02x:%02x/%08d", xp.port, xp.cu, xp.ldev, xp.serial);

	return 0;
}
