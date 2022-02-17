
/*
	Worker - a utility to help parallelize tasks.

	Steve Shoecraft
	stephen.shoecraft@hp.com
*/

#include <stdio.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <pthread.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define dprintf(m) printf m
#else
#define dprintf(m) /* noop */
#endif

#define VERSIONSTR "1.0"

char *args[32];
int nargs;
unsigned int count;

struct worker_info {
	int slot;
	unsigned short flags;
	pthread_mutex_t busy_mutex;
	pthread_t tid;
	char line[1024];
};
typedef struct worker_info * worker_t;

FILE *error_fp;

#define FLAG_BUSY 1
#define FLAG_DONE 2

worker_t workers;

int get_busy(worker_t wp) {
	int busy;

	pthread_mutex_lock(&wp->busy_mutex);
	busy = ((wp->flags & FLAG_BUSY) != 0);
	pthread_mutex_unlock(&wp->busy_mutex);

//	dprintf(("get_busy: worker[%d]: busy: %d, line: %s\n", wp->slot, busy, wp->line));

	return busy;
}

void set_busy(worker_t wp, int val) {
//	printf("set_busy: val: %d, flags before: %x\n", val, wp->flags);
	pthread_mutex_lock(&wp->busy_mutex);
	if (val)
		wp->flags |= FLAG_BUSY;
	else
		wp->flags &= ~FLAG_BUSY;
	pthread_mutex_unlock(&wp->busy_mutex);
//	printf("flags after: %x\n", wp->flags);

//	dprintf(("set_busy: worker[%d]: busy: %d\n", wp->slot, val));
}

//static int doexec(worker_t wp, const char *cmd);
//static int doexec(worker_t wp, char **);

int do_exec(int slot, char **args) {
	int pid,stat;

	/* Execute the command */
	stat = 77;
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	} else if (pid > 0) {
//		printf("pid: %ld\n", pid);
		waitpid(pid,&stat,0);
	} else if (pid == 0) {
		execvp(args[0],args);
//		dprintf(("worker[%d]: args[0]: %s\n", slot, args[0]));
		perror("exec");
		_exit(1);
	}
	return WEXITSTATUS(stat);
}

/* The actual worker function */
void *worker(void *arg) {
	worker_t wp;
	char *argv[33];
	int x,busy,done,argc,stat;

	wp = (worker_t)arg;
	while(1) {
		/* Get info */
		pthread_mutex_lock(&wp->busy_mutex);
		busy = ((wp->flags & FLAG_BUSY) != 0);
		done = ((wp->flags & FLAG_DONE) != 0);
		pthread_mutex_unlock(&wp->busy_mutex);
		dprintf(("worker[%d]: busy: %d, done: %d\n", wp->slot, busy, done));

		if (!busy) {
			if (done) break;
//			usleep(100);
			sleep(1);
			continue;
		}

		dprintf(("worker[%d]: line: %s\n", wp->slot, wp->line));

		/* Build argv */
		memset(argv, 0, sizeof(argv));
//		dprintf(("worker[%d]: nargs: %d\n", wp->slot, nargs));
		for(x=argc=0; x < nargs; x++) argv[argc++] = args[x];
		argv[argc] = malloc(strlen(wp->line)+1);
		strcpy(argv[argc],wp->line);
		argc++;

#if 0
#if DEBUG
		printf("worker[%d]: *****************************\n", wp->slot);
		printf("worker[%d]: argc: %d\n", wp->slot, argc);
		for(x=0; x < argc; x++) printf("worker[%d]: argv[%d]: %s\n", wp->slot, x, argv[x]);
		printf("worker[%d]: *****************************\n", wp->slot);
#endif
#endif

		dprintf(("worker[%d]: exec'ing...\n", wp->slot));
		stat = do_exec(wp->slot, argv);
		dprintf(("worker[%d]: stat: %d\n", wp->slot, stat));
		if (stat != 0 && error_fp) fprintf(error_fp,"%s\n",wp->line);

		/* Clear busy flag */
		set_busy(wp,0);
	}
//	pthread_exit(0);
	dprintf(("worker[%d]: returning...\n", wp->slot));
	return (void *)0;
}

/* Get an idle worker slot */
worker_t get_worker(void) {
	register int x;

	while(1) {
		for(x=0; x < count; x++) {
			if (!get_busy(&workers[x])) {
				dprintf(("get_worker: returning worker: %d\n", x));
				return &workers[x];
			}
		}
		usleep(100);
	}
}

#if 0
static void _do_exit(int sig) {
	printf("signal %d received, exiting.\n", sig);
	exit(0);
}
#endif

void usage(void) {
	fprintf(stderr,"usage: worker <input file> <# of threads> <program> <arg1> ... <argn>\n");
	exit(1);
}

static struct option options[] = {
	{ "input",	required_argument,	0, 'i' },
	{ "threads",	required_argument,	0, 't' },
	{ "error",	required_argument,	0, 'e' },
	{ "help",	no_argument,		0, 'h' },
	{ "version",	no_argument,		0, 'v' },
	{0, 0, 0, 0}
};

int main(int argc, char **argv) {
	FILE *fp;
	char line[1024], *input, *prog;
	worker_t wp;
	int c,x,using_stdin,busy;
	char optstr[32];
	int option_index = 0;
	struct option *popt;

	/* Build optstr */
	x = 0;
	for(popt = options; popt->name; popt++) {
		if (!popt->flag) {
			optstr[x++] = popt->val;
			if (popt->has_arg == required_argument)
				optstr[x++] = ':';
		}
	}
	printf("optstr: %s\n", optstr);

	while(1) {
		c = getopt_long (argc, argv, "abc:d:f:", options, &option_index);
		if (c == -1) break;
		if (c >= 32)
			printf("c(%d): %c\n", c, c);
		else
			printf("c: %d\n", c);
		switch(c) {
		case 0:
			if (options[option_index].flag != 0) break;
			printf ("option %s", options[option_index].name);
			if (optarg) printf (" with arg %s", optarg);
			printf ("\n");
			break;
		case 'v':
			printf("worker version %s\n", VERSIONSTR);
			printf("Copyright(C) 2009 ShadowIT, Inc.\n");
			printf("No rights reserved anywhere.\n");
			break;
		case '?':
			break;
		}
	}
	return 1;

#if 0
	option_index = 0;
	c = getopt_long (argc, argv, "abc:d:f:", long_options, &option_index);

	help = debug = 0;
	while(1) {
		c = getopt_long(argc, argv, optstring, options, &option_index);
		if (c == -1) break;
		i = 0;
		for(x=0; x < fcount; x++) {
			if (fields[x].copt[0] == c) {
				/* Handle special case if no query string */
				if (!fields[x].query) {
					switch(c) {
					case 'h':
						usage();
						return 0;
					case 'd':
						debug = 1;
						break;
					default:
						printf("unhandled opt: %c\n", c);
						usage();
						return 1;
					}
				} else if (count < MAX_QUERIES) {
					strcat(query,",");
					strcat(query,fields[x].query);
					format[count] = fields[x].format;
					header[count] = fields[x].header;
					count++;
				}
				i = 1;
			}
		}
		if (!i) {
			usage();
			return 1;
		}
	}
	strcat(query," FROM ci_vw AS T1, srvr_dtl_vw AS T2");
	strcat(query," WHERE (T1.CI_NM = T2.SRVR_NM)");

	/* Specify only servers or virtual machines */
//	strcat(query," AND (T1.type = 'server' OR T1.type = 'virtualserverinstance')");

	/* non-switch args are hostnames */
	if (optind < argc) {
		strcat(query," AND (T1.CI_NM REGEXP '");
		x=0;
		while (optind < argc) {
			if (x) strcat(query,"|");
			else x++;
			strcat(query,argv[optind++]);
		}
		strcat(query,"')");
	}
#endif

	if (argc < 3) usage();
	input = argv[1];
	count=(unsigned int) atoi(argv[2]);
	prog = argv[3];
	dprintf(("main: input: %s, count: %d, prog: %s\n", input, count, prog));
	if (count < 1) usage();

	nargs = 0;
	for(x=3; x < argc; x++) args[nargs++] = argv[x];

#if DEBUG
	for(x=0; x < nargs; x++) printf("main: arg[%d]: %s\n", x, args[x]);
#endif

	if (strcmp(input,"-") == 0) {
		fp = stdin;
		using_stdin = 1;
	} else {
		fp = fopen(input,"r");
		if (!fp) {
			sprintf(line,"worker: unable to open %s", input);
			perror(line);
			exit(1);
		}
		using_stdin=0;
	}

	workers = malloc(count * sizeof(struct worker_info));
	if (!workers) {
		perror("malloc workers");
		exit(1);
	}

	/* Init worker info */
	dprintf(("creating workers...\n"));
	for(x=0; x < count; x++) {
		wp = (worker_t) &workers[x];
		memset(wp,0,sizeof(*wp));
		wp->slot = x;
		pthread_mutex_init(&wp->busy_mutex, 0);
		if (pthread_create(&wp->tid, NULL, worker, wp)) {
			perror("pthread_create");
			exit(1);
		}
	}

//	signal(SIGINT,_do_exit);
//	signal(SIGTERM,_do_exit);

	if (using_stdin)
		sprintf(line,"stdin.error");
	else
		sprintf(line,"%s.error",input);
	error_fp = fopen(line,"w+");

	/* For every line in the input file */
	dprintf(("processing input...\n"));
	while(fgets(line,sizeof(line),fp)) {
		/* Strip newline, if present */
		if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;

		dprintf(("main: line: %s\n", line));

		/* Get an idle worker */
		wp = get_worker();

		/* Crack the whip */
		strcpy(wp->line,line);
		set_busy(wp, 1);
	}

	/* Mark done */
	for(x=0; x < count; x++) {
		wp = &workers[x];
		pthread_mutex_lock(&wp->busy_mutex);
		wp->flags |= FLAG_DONE;
		pthread_mutex_unlock(&wp->busy_mutex);
	}

	dprintf(("*** WAITING ON FINISH ****\n"));
	/* Wait for any remaining threads */
	do {
		busy = 0;
		for(x=0; x < count; x++) {
			wp = &workers[x];
			pthread_mutex_lock(&wp->busy_mutex);
			if (wp->flags & FLAG_BUSY) {
				dprintf(("wp[%d]: %s\n", wp->slot, wp->line));
				busy++;
			}
			pthread_mutex_unlock(&wp->busy_mutex);
		}
		dprintf(("busy: %d\n", busy));
		if (busy) {
			sleep(1);
			continue;
		}
	} while(busy);

	/* Kill the workers - like all good pharoahs */
	dprintf(("killing workers...\n"));
	for(x=0; x < count; x++) pthread_cancel(workers[x].tid);

	/* Close input */
	dprintf(("closing input...\n"));
	if (!using_stdin) fclose(fp);

	/* Done! */
	pthread_exit(0);
}
