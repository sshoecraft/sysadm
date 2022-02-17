
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gendb.h"

#ifdef VMS
#define OS_STATUS_SUCCESS	1
#define OS_STATUS_ERROR		2
#else
#define OS_STATUS_SUCCESS	0
#define OS_STATUS_ERROR		1
#endif

/* Global vars & funcs */
char *descrip_string = "GENerate DataBase Access Files";
char *version_string = "version 1.00";
char *user_name = "dtadmin";
int opts = 0;
list tables;
list syns;
list funcs;

/* Local vars & funcs */
static char userid[80],table_info[1024];
static void usage(void);
struct _parse_info {
	int level;
	char *string;
	list opts;
};
typedef struct _parse_info PARSE_INFO;
#define PARSE_INFO_SIZE sizeof(struct _parse_info)
list parse_info(char *);
void disp_info(list);

int main(int argc,char **argv) {
	FILE *fp;
	char line[1024];
	list parse_list;
	register char *ptr;
	int arg_ind;

	/* Init some vars */
	userid[0] = table_info[0] = 0;

	/* Get the command-line args */
	if (argc < 2) {
		usage();
		exit(1);
	}

	opts = 0;
	arg_ind = 1;
	ptr = argv[arg_ind];
	if (*ptr == '-') {
		ptr++;
		while(*ptr != 0) {
			switch(*ptr) {
				case 'd':
					opts |= OPT_DEBUG;
					break;
				case 'h':
					usage();
					return OS_STATUS_SUCCESS;
				case 'n':
					opts |= OPT_NET;
					break;
				case 'p':
					opts |= OPT_PORT;
					break;
				case 'q':
					opts |= OPT_QUIET;
					break;
				case 'r':
					opts |= OPT_RDT;
					break;
				case 'u':
					strcpy(userid,argv[arg_ind++]);
					while(*ptr != 0) ptr++;
					ptr--;
					argc--;
					break;
				case 'v':
					opts |= OPT_VERBOSE;
					break;
				default:
					fprintf(stderr,"unknown option: %s\n",
						ptr);
			}
			ptr++;
		}
		arg_ind++;
		argc--;
	}

	/* If an argument is given, it's the table info */
	if (argc > 1)
		strcpy(table_info,argv[arg_ind++]);	/* Arg 1 -- userid */
	if (opts & OPT_DEBUG) printf("table_info: %s\n",table_info);

	/* If the userid wasn't specified, use the username */
	if (opts & OPT_DEBUG) printf("userid: %s\n",userid);
	if (!strlen(userid)) {
		strcpy(userid,user_name);
		strcat(userid,"/");
		strcat(userid,user_name);
	}
	if (opts & OPT_DEBUG) printf("userid: %s\n",userid);

	/* If table_info is empty, try to open dbaf.inf */
	if (!strlen(table_info)) {
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
	}
	/* Otherwise, parse table_info */
	return OS_STATUS_SUCCESS;
}

static void usage(void) {
	fprintf(stderr,"usage: gendbaf [-dqnpqrv] [-u <userid>] [gen info]\n");
	return;
}

void disp_info(list parse_list) {
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
//	list info;
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
		if (*ptr == '(') {
			*wptr = 0;
			info.level = pcb->level++;
			info.string = (char *) malloc(strlen(temp)+1);
			if (!info.string) {
				oserr("unable to allocate memory");
				return 0;
			}
			strcpy(info.string,temp);
			pcb->string = ptr + 1;
			info.opts = _parse_info(pcb);
			ptr = pcb->string;
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
				if (!info.string) {
					oserr("unable to allocate memory");
					return 0;
				}
				strcpy(info.string,temp);
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
				if (!info.string) {
					oserr("unable to allocate memory");
					return 0;
				}
				strcpy(info.string,temp);
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
		if (!info.string) {
			oserr("unable to allocate memory");
			return 0;
		}
		strcpy(info.string,temp);
		list_add(parse_list,&info,PARSE_INFO_SIZE);
	}
	pcb->string = ptr;
	return parse_list;
}
