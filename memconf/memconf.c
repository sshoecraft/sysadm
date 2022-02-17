
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define DEBUG 0

#if DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif

#define FileName "/tmp/dmidecode.txt"

struct info {
	char size[32];
	char loc[32];
	char type[64];
	struct info *next;
};

struct info *list, *last;

char *trim(char *string) {
	register char *src,*dest;

	/* If string is empty, just return it */
	if (*string == '\0') return string;

	/* Trim the front */
	src = string;
	while(isspace((int)*src) && *src != '\t') src++;
	dest = string;
	while(*src != '\0') *dest++ = *src++;

	/* Trim the back */
	*dest-- = '\0';
	while((dest >= string) && isspace((int)*dest)) dest--;
	*(dest+1) = '\0';

	return string;
}

int main(void) {
	FILE *fp;
	char line[128], *p;
	char ff[32], mt[32], speed[32];
	char mfg[32], prod[64], serial[32];
	struct info *new;
	int got_sys, have_sys, have_mem;

	sprintf(line, "dmidecode > %s", FileName);
	if (system(line)) exit(1);
	fp = fopen(FileName, "r");
	if (!fp) {
		perror("unable to open output");
		exit(1);
	}

	got_sys = have_sys = have_mem = 0;
	list = last = new = 0;
	ff[0] = mt[0] = speed[0] = 0;
	while(fgets(line,sizeof(line),fp)) {
		line[strlen(line)-1] = 0;
		dprintf("line: %s\n", line);
#if 0
		if (strncmp(line,"Handle",6) != 0) {
			if (new) {
				if (strlen(mt))
					sprintf(new->type, "%s %s %s", mt, speed, ff);
				else
					sprintf(new->type, "%s %s", speed, ff);
				if (last) {
					last->next = new;
					last = new;
				} else {
					list = last = new;
				}
			}
			have_sys = have_mem = 0;
			new = 0;
#endif
		if (have_sys) {
			dprintf("*** SYS\n");
			if (strlen(line) == 0 || strncmp(line,"Handle",6) == 0) {
				have_sys = 0;
				got_sys = 1;
			} else {
				if ((p = strstr(line,"Manufacturer: ")) != 0) trim(strcpy(mfg, p+14));
				if ((p = strstr(line,"Product Name: ")) != 0) trim(strcpy(prod, p+14));
				if ((p = strstr(line,"Serial Number: ")) != 0) trim(strcpy(serial, p+15));
			}
		} else if (have_mem) {
			dprintf("*** MEM\n");
			if (strlen(line) == 0 || strncmp(line,"Handle",6) == 0) {
				have_mem = 0;
				if (strlen(mt))
					sprintf(new->type, "%s %s %s", mt, speed, ff);
				else
					sprintf(new->type, "%s %s", speed, ff);
				if (last) {
					last->next = new;
					last = new;
				} else {
					list = last = new;
				}
			} else {
				if ((p = strstr(line,"Size: ")) != 0) trim(strcpy(new->size, p+6));
				p = strstr(line, "Locator: ");
				if (p) {
					if (strstr(line, "Bank") == 0)
						trim(strcpy(new->loc, p+9));
				}
				if ((p = strstr(line, "Type: ")) != 0) {
					trim(strcpy(mt, p+6));
					if (strcmp(mt, "<OUT OF SPEC>")==0) mt[0] = 0;
				}
				if ((p = strstr(line, "Form Factor: ")) != 0) {
					trim(strcpy(ff, p+13));
				}
				if ((p = strstr(line, "Speed: ")) != 0) {
					register char *p2 = strchr(p+7,'(');
					if (p2) *p2 = 0;

					trim(strcpy(speed, p+7));
				}
			}
		}
		if (!have_sys || !have_mem) {
#define TYPE1 "Handle 0x01"
			if (strncmp(line,TYPE1,strlen(TYPE1)) == 0) {
				mfg[0] = prod[0] = serial[0] = 0;
				dprintf("have_sys\n");
				have_sys = 1;
#define TYPE17 "Handle 0x11"
			} else if (strncmp(line,TYPE17,strlen(TYPE17)) == 0) {
				new = malloc(sizeof(*new));
				new->next = 0;
				ff[0] = mt[0] = speed[0] = 0;
				dprintf("have_mem\n");
				have_mem= 1;
			}
		}
	}

	if (!got_sys) {
		printf("unable get system info, aborting.\n");
		return 1;
	}
	printf("Product: %s %s, Serial: %s\n\n", mfg, prod, serial);
	for(new = list; new; new = new->next) {
		printf("%-20s %s ", new->loc, new->size);
		if (strstr(new->size, "No Module") == 0)
			printf("%s\n", new->type);
		else
			printf("\n");
	}

	return 0;
}
