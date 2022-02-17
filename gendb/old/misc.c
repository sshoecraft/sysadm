
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "gendb.h"

#ifdef DEBUG
#undef DEBUG
#endif

#if 0
extern char sql_error_text[];

char *typeinfo[4][FT_MAX] = {
	{ "unknown", "char", "string", "short", "int", "long", "quad",
		"float", "double", "long double", "date" },
	{ "void", "char", "char", "short", "int", "long", "long long",
		"float", "double", "long double", "char" },
	{ "void *","char *", "char *", "short *", "int *", "long *", "long long *",
		"float *", "double *", "long double *", "char *" },
};

char *typestr(int f,int t) {
	return(typeinfo[f][t]);
}

char *dstrtype(int type) {
	return(typestr(0,type));
}
#endif

char *trim(char *string) {
	register char *src,*dest;

#if DEBUG
	printf("trim: before: %s\n",string);
#endif
	/* Trim the front */
	src = string;
	while(isspace(*src) && *src != '\t') src++;
	dest = string;
	while(*src != '\0') *dest++ = *src++;

	/* Trim the back */
	*dest-- = '\0';
	while(isspace(*dest)) dest--;
	*(dest+1) = '\0';
#if DEBUG
	printf("trim: after: %s\n",string);
#endif
	return string;
}

extern int sys_nerr;

void oserr(char *prefix) {
	register char *ptr;

#if 0
	if (errno <= sys_nerr)
		ptr = strerror(errno);
	else
		ptr = "unknown error";
#endif
	ptr = strerror(errno);
	if (prefix)
		fprintf(stderr,"error: %s: %s\n",prefix,ptr);
	else
		fprintf(stderr,"error: %s\n",ptr);
	return;
}

char *strele(int num,char *delimiter,char *string) {
	static char return_info[1024];
	register char *src,*dptr,*eptr,*cptr;
	register char *dest;
	register int count;

#if DEBUG
	printf("Element: %d,delimiter: %s,string: %s\n",num,delimiter,string);
#endif
	eptr = string;
	dptr = delimiter;
	dest = return_info;
	count = 0;
	for(src = string; *src != '\0'; src++) {
#if DEBUG
		if (*src)
			printf("src: %c == ",*src);
		else
			printf("src: (null) == ");
		if (*dptr)
			printf("dptr: %c\n",*dptr);
		else
			printf("dptr: (null)\n");
#endif
		if (*src == *dptr) {
			cptr = src+1;
			dptr++;
#if DEBUG
			if (*cptr != '\0')
				printf("cptr: %c == ",*cptr);
			else
				printf("cptr: (null) == ");
			if (*dptr != '\0')
				printf("dptr: %c\n",*dptr);
			else
				printf("dptr: (null)\n");
#endif
			while(*cptr == *dptr && *cptr != '\0' && *dptr != '\0') {
				cptr++;
				dptr++;
#if DEBUG
				if (*cptr != '\0')
					printf("cptr: %c == ",*cptr);
				else
					printf("cptr: (null) == ");
				if (*dptr != '\0')
					printf("dptr: %c\n",*dptr);
				else
					printf("dptr: (null)\n");
#endif
				if (*dptr == '\0' || *cptr == '\0') {
#if DEBUG
					printf("Breaking...\n");
#endif
					break;
				}
/*
				dptr++;
				if (*dptr == '\0') break;
				cptr++;
				if (*cptr == '\0') break;
*/
			}
#if DEBUG
			if (*cptr != '\0')
				printf("cptr: %c == ",*cptr);
			else
				printf("cptr: (null) == ");
			if (*dptr != '\0')
				printf("dptr: %c\n",*dptr);
			else
				printf("dptr: (null)\n");
#endif
			if (*dptr == '\0') {
#if DEBUG
				printf("Count: %d, num: %d\n",count,num);
#endif
				if (count == num) break;
				if (cptr > src+1) src = cptr-1;
				eptr = src+1;
				count++;
#if DEBUG
				printf("eptr: %s, src: %s\n",eptr,src+1);
#endif
			}
			dptr = delimiter;
		}
	}
#if DEBUG
	printf("Count: %d, num: %d\n",count,num);
#endif
	if (count == num) {
#if DEBUG
		printf("eptr: %s\n",eptr);
		printf("src: %s\n",src);
#endif
		while(eptr < src)
			*dest++ = *eptr++;
	}
	*dest = '\0';
#if DEBUG
	printf("Returning: %s\n",return_info);
#endif
	return(return_info);
}

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

	func = (char *)&edit_funcs;
	func[0] = 0;
	ele = 0;
	dest = (char *)&edit_list;
	for(src = list; *src != '\0'; src++) {
		if (!isspace(*src))
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
			if (!isspace(*src))
				*dest++ = *src;
		}
		*dest = '\0';
	} else if (edit_funcs.compress) {
		found = 0;
		for(src = dest = (char *)&return_info; *src != '\0'; src++) {
			if (isspace(*src)) {
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
		while(isspace(*src) && *src != '\t' && *src) src++;
		while(*src != '\0') *dest++ = *src++;
		*dest-- = '\0';
		while(isspace(*dest)) dest--;
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
