
#include "xplist.h"

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

char *strele(int num,char *delimiter,char *string) {
	static char return_info[1024];
	register char *src,*dptr,*eptr,*cptr;
	register char *dest;
	register int count;

	eptr = string;
	dptr = delimiter;
	dest = return_info;
	count = 0;
	for(src = string; *src != '\0'; src++) {
		if (*src == *dptr) {
			cptr = src+1;
			dptr++;
			while(*cptr == *dptr && *cptr != '\0' && *dptr != '\0') {
				cptr++;
				dptr++;
				if (*dptr == '\0' || *cptr == '\0') break;
			}
			if (*dptr == '\0') {
				if (count == num) break;
				if (cptr > src+1) src = cptr-1;
				eptr = src+1;
				count++;
			}
			dptr = delimiter;
		}
	}
	if (count == num) {
		while(eptr < src)
			*dest++ = *eptr++;
	}
	*dest = '\0';
	return(return_info);
}

#include <string.h>
#include <ctype.h>

enum EDIT_FUNCS { COLLAPSE, COMPRESS, LOWERCASE, TRIM, UNCOMMENT, UPCASE };

struct {
	char *item;
	short func;
} EDIT_ITEM_INFO[] = {
	{ "COLLAPSE", COLLAPSE },
	{ "COMPRESS", COMPRESS },
	{ "LOWERCASE", LOWERCASE },
	{ "TRIM", TRIM, },
	{ "UNCOMMENT", UNCOMMENT },
	{ "UPCASE", UPCASE },
	{ 0, 0 },
	{ 0, 0 }
};

static struct {
	unsigned collapse: 1;
	unsigned compress: 1;
	unsigned lowercase: 1;
	unsigned trim: 1;
	unsigned uncomment: 1;
	unsigned upcase: 1;
	unsigned fill: 2;
} edit_funcs;

char *stredit(char *string, char *list) {
	static char return_info[1024];
	char edit_list[255], *func, *src, *dest;
	register int x, ele, found;

	/* If string is empty, just return it */
	if (*string == '\0') return string;

	func = (char *)&edit_funcs;
	func[0] = 0;
	ele = 0;
	dest = (char *)&edit_list;
	for(src = list; *src != '\0'; src++) {
		if (!isspace((int)*src))
			*dest++ = toupper(*src);
	}
	*dest = '\0';
	return_info[0] = 0;
	strncat(return_info,string,sizeof(return_info));
	while(1) {
		func = strele(ele,",",edit_list);
		if (!strlen(func)) break;
		found = 0;
		for(x=0; EDIT_ITEM_INFO[x].item; x++) {
			if (strcmp(EDIT_ITEM_INFO[x].item,func)==0) {
				found = 1;
				break;
			}
		}
		if (found) {
			switch(EDIT_ITEM_INFO[x].func) {
				case COLLAPSE:
					edit_funcs.collapse = 1;
					break;
				case COMPRESS:
					edit_funcs.compress = 1;
					break;
				case LOWERCASE:
					edit_funcs.lowercase = 1;
					break;
				case TRIM:
					edit_funcs.trim = 1;
					break;
				case UNCOMMENT:
					edit_funcs.uncomment = 1;
					break;
				case UPCASE:
					edit_funcs.upcase = 1;
					break;
			}
		}
		ele++;
	}
	if (edit_funcs.collapse) {
		for(src = dest = (char *)&return_info; *src != '\0'; src++) {
			if (!isspace((int)*src))
				*dest++ = *src;
		}
		*dest = '\0';
	} else if (edit_funcs.compress) {
		found = 0;
		for(src = dest = (char *)&return_info; *src != '\0'; src++) {
			if (isspace((int)*src)) {
				if (found == 0) {
					*dest++ = ' ';
					found = 1;
				}
			}
			else {
				*dest++ = *src;
				found = 0;

			}
		}
		*dest = '\0';
	}
	if (edit_funcs.trim) {
		src = dest = (char *)&return_info;
		while(isspace((int)*src) && *src != '\t' && *src) src++;
		while(*src != '\0') *dest++ = *src++;
		*dest-- = '\0';
		while((dest >= return_info) && isspace((int)*dest)) dest--;
		*(dest+1) = '\0';
	}
	if (edit_funcs.uncomment) {
		for(src = (char *)&return_info; src != '\0'; src++) {
			if (*src == '!') {
				*src = '\0';
				break;
			}
		}
	}
	if (edit_funcs.upcase) {
		for(src = (char *)&return_info; *src != '\0'; src++)
			*src = toupper(*src);
	} else if (edit_funcs.lowercase) {
		for(src = (char *)&return_info; *src != '\0'; src++)
			*src = tolower(*src);
	}
	return(return_info);
}

void bindump(char *msg, void *bdata, int bytes) {
        unsigned char *data = bdata;
        int offset,end,x,y;
        char line[128],*p;

        printf("%s:\n",msg);
        p = line;
        offset = 0;
        for(x=0; x < bytes; x += 16) {
                p += sprintf(p, "%04X: ",offset);
                end=(x+16 > bytes ? bytes : x+16);
                for(y=x; y < end; y++)  p += sprintf(p,"%02X ",data[y]);
                for(y=end; y < x+16; y++) p += sprintf(p,"   ");
                p += sprintf(p,"  ");
                for(y=x; y < end; y++) {
                        if (data[y] > 32 && data[y] < 127)
                                p += sprintf(p,"%c",data[y]);
                        else
                                p += sprintf(p,".");
                }
                p += sprintf(p,"\n");
                printf(line);
                p = line;
                offset += 16;
        }
}

int get_path(char *var, char **paths) {
	int i,found;

	*var = 0;
	found = 0;
	for(i=0; paths[i]; i++) {
        	if (access(paths[i],X_OK) == 0) {
			strcpy(var, paths[i]);
			found = 1;
			break;
		}
	}
	return (found == 0);
}

char *concat_path(char *d,char *a, char *b) {
       if (a[0]) {
                strcpy(d,a);
#ifdef __MINGW32__
                if (d[strlen(d)-1] != '\\') strcat(d,"\\");
#else
                if (d[strlen(d)-1] != '/') strcat(d,"/");
#endif
                strcat(d,b);
        } else
                strcpy(d,b);

        return d;
}
