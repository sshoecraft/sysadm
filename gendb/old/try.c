
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "list.h"

struct _parse_info {
	int level;
	char *string;
	list opts;
};
typedef struct _parse_info PARSE_INFO;
#define PARSE_INFO_SIZE sizeof(struct _parse_info)

list parse_info(char *);

main()
{
	FILE *fp;
	char line[1024];
	list parse_list;

	fp = fopen("dbaf.inf","r");
	if (!fp) {
		perror("fopen");
		return 1;
	}
	while(fgets(line,sizeof(line)-1,fp)) {
		if (line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = 0;
		parse_list = parse_info(line);
		if (parse_list) {
			printf("original string: %s\n",line);
			printf("  parsed string: ");
			disp_info(parse_list);
			putchar('\n');
		}
	}
	fclose(fp);
	return 0;
}

disp_info(list parse_list) {
	register PARSE_INFO *info;

	list_reset(parse_list);
	while( (info = list_get_next(parse_list)) != 0) {
		printf("%s",info->string);
		if (info->opts) {
			putchar('(');
			disp_info(info->opts);
			putchar(')');
		}
		if (list_is_next(parse_list)) putchar(',');
	}
}

struct _parse_control_block {
	int level;
	char *string;
};

list _parse_info(struct _parse_control_block *);

list parse_info(char *string) {
	struct _parse_control_block pcb = { 0,string };
	return(_parse_info(&pcb));
}

list _parse_info(struct _parse_control_block *pcb) {
	list parse_list;
	PARSE_INFO info = { 0,0,0 };
	char temp[1024];
	register char *wptr,*ptr;

	/* The pcb is kinda important */
	if (!pcb) return 0;

	/* Create the parse_list */
	parse_list = list_create();
	if (!parse_list) return 0;

	wptr = temp;
	for(ptr=pcb->string; *ptr; ptr++) {
		printf("level: %d, ptr(%p): %c\n",pcb->level,ptr,*ptr);
		if (*ptr == '(') {
			*wptr = 0;
			info.level = pcb->level++;
			info.string = (char *) malloc(strlen(temp)+1);
			if (!info.string) {
				perror("malloc string");
				return 0;
			}
			strcpy(info.string,temp);
			pcb->string = ptr + 1;
			info.opts = _parse_info(pcb);
			ptr = pcb->string;
			printf("open: %d, %s, %p\n",
				info.level,info.string,info.opts);
			list_add(parse_list,&info,PARSE_INFO_SIZE);
			info.level = 0;
			info.string = 0;
			info.opts = 0;
			wptr = temp;
		} else if (*ptr == ')') {
			*wptr = 0;
			if (strlen(temp)) {
				info.level = pcb->level;
				info.string = (char *) malloc(strlen(temp)+1);
				if (!info.string) return 0;
				strcpy(info.string,temp);
				printf("close: %d, %s, %p\n",
					info.level,info.string,info.opts);
				list_add(parse_list,&info,PARSE_INFO_SIZE);
				info.level = 0;
				info.string = 0;
				info.opts = 0;
			}
			wptr = temp;
			pcb->level--;
			pcb->string = ptr;
			return parse_list;
		} else if (*ptr == ',') {
			*wptr = 0;
			if (strlen(temp)) {
				info.level = pcb->level;
				info.string = (char *) malloc(strlen(temp)+1);
				if (!info.string) return 0;
				strcpy(info.string,temp);
				printf("comma: %d, %s, %p\n",
					info.level,info.string,info.opts);
				list_add(parse_list,&info,PARSE_INFO_SIZE);
				info.level = 0;
				info.string = 0;
				info.opts = 0;
			}
			wptr = temp;
		} else
			*wptr++ = *ptr;
	}
	*wptr = 0;
	if (strlen(temp)) {
		info.level = pcb->level;
		info.string = (char *) malloc(strlen(temp)+1);
		if (!info.string) return 0;
		strcpy(info.string,temp);
		printf("done: %d, %s, %p\n",
			info.level,info.string,info.opts);
		list_add(parse_list,&info,PARSE_INFO_SIZE);
	}
	pcb->string = ptr;
	return parse_list;
}
