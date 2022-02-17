
#ifndef __MINGW32__
#include "worker.h"
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#define VERSIONSTR "1.1"

static void usage(void) {
	printf("usage: worker [options] <input file> <# of threads> <program> [args]\n");
	printf("  where:\n");
	printf("    -V                     print version\n");
	printf("    -d                     increase debug level\n");
	printf("    -t <x>                 kill prog after x seconds\n");
	printf("    -i <x>                 interval between starting another worker\n");
	printf("    -e <file>              error file\n");
	printf("    <input file>           file path or - for stdin\n");
	printf("    <# of threads>         # of <program> to execute @ the same time\n");
	printf("    <program>              program to execute\n");
	printf("    [args]                 optional arguments passed to program\n");
}

enum {
	STATE_NONE,
	STATE_GET_TIMEOUT,
	STATE_GET_INTERVAL,
	STATE_GET_DEBUG,
	STATE_GET_ERRFILE,
	STATE_GET_COUNT,
	STATE_GET_PROG,
	STATE_GET_ARGS,
};

static char *args[32];
static int nargs;

//FILE *error_fp = 0;
char *error_file = 0;
int debug;

#if 0
static int run_prog(void *arg) {
	char *argv[32];
	int i,nargv,status;
	pid_t pid;

	/* Setup argv */
	dprintf("run_prog: arg: %s\n", (char *)arg);
	memcpy(argv,args,sizeof(args));
	nargv = nargs;
	argv[nargv++] = arg;
	for(i=0; i < nargv; i++) dprintf("argv[%d]: %s\n", i, argv[i]);

	/* Execute the command */
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	} else if (pid > 0) {
		/* Wait for pid */
		waitpid(pid,&status,0);
	
                dprintf("WIFEXITED: %d\n", WIFEXITED(status));
                if (WIFEXITED(status)) dprintf("WEXITSTATUS: %d\n", WEXITSTATUS(status));
                status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
                dprintf("status: %d\n", status);
		if (status && error) {
			FILE *fp;

			fp = fopen(error,"a+");
			if (fp) {
				fprintf(fp,"%s\n",(char *)arg);
				fclose(fp);
			}
		}
                return status;
	} else if (pid == 0) {
		execvp(argv[0],argv);
		perror("exec");
		_exit(1);
	}

	/* Should never reach here */
	return 1;
}
#endif

int main(int argc, char **argv) {
	FILE *fp;
	char line[1024], *input, *tc, *prog;
	int using_stdin,count,state,timeout,interval;
	worker_t wp;

	/* Init args */
	memset(args,0,sizeof(args));
	nargs = 0;
	timeout = interval = 0;

	/* Process command line */
	optind = 1;
	state = STATE_NONE;
	error_file = input = tc = prog = 0;
	debug = 0;
	while(optind < argc) {
		dprintf("state: %d, optind: %d, arg: %s\n", state, optind, argv[optind]);
		switch(state) {
		case STATE_NONE:
			if (strcmp(argv[optind],"-t") == 0) {
				state = STATE_GET_TIMEOUT;
			} else if (strcmp(argv[optind],"-i") == 0) {
				state = STATE_GET_INTERVAL;
			} else if (strcmp(argv[optind],"-d") == 0) {
				debug++;
			} else if (strcmp(argv[optind],"-e") == 0) {
				state = STATE_GET_ERRFILE;
			} else if (strcmp(argv[optind],"-V") == 0) {
				printf("worker version %s\n", VERSIONSTR);
				printf("CopyLeft(L) 2009-2011 ShadowIT, Inc.\n");
				printf("No rights reserved anywhere.\n");
				return 0;
			} else if (strcmp(argv[optind],"-h") == 0) {
				usage();
				return 0;
			} else if (strcmp(argv[optind],"-") == 0) {
				input = argv[optind];
				state = STATE_GET_COUNT;
			} else if (strncmp(argv[optind],"-",1) == 0) {
				usage();
				return 1;
			} else if (!input) {
				input = argv[optind];
				state = STATE_GET_COUNT;
			}
			break;
		case STATE_GET_TIMEOUT:
			timeout = atoi(argv[optind]);
			state = STATE_NONE;
			break;
		case STATE_GET_INTERVAL:
			timeout = atoi(argv[optind]);
			state = STATE_NONE;
			break;
		case STATE_GET_ERRFILE:
			error_file = malloc(strlen(argv[optind])+1);
			strcpy(error_file,argv[optind]);
			state = STATE_NONE;
			break;
		case STATE_GET_COUNT:
			tc = argv[optind];
			state = STATE_GET_PROG;
			break;
		case STATE_GET_PROG:
			prog = argv[optind];
			state = STATE_GET_ARGS;
			/* Fallthrough */
		case STATE_GET_ARGS:
			args[nargs++] = argv[optind];
			break;
		}
		optind++;
	}
	dprintf("input: %p, tc: %p, prog: %p\n", input, tc, prog);
	if (!input || !tc || !prog) {
		usage();
		return 1;
	}
	count = atoi(tc);
	dprintf("debug: %d\n", debug);
	if (error_file) dprintf("error_file: %s\n",error_file);
	dprintf("count: %d\n", count);
	dprintf("timeout: %d\n", timeout);

//	add_arg(prog);
	for(optind = 0; optind < nargs; optind++) {
		dprintf("args[%d]: %s\n", optind, args[optind]);
		add_arg(args[optind]);
	}
//	return 0;

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

	/* Create worker threads */
	if (create_workers(count)) return 1;

	/* For every line in the input file */
	dprintf("processing input...\n");
	while(fgets(line,sizeof(line),fp)) {
		/* Strip newline, if present */
		if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;

		dprintf("line: %s\n", line);

		/* Get an idle worker */
		wp = get_idle_worker(count);

		/* Crack the whip */
		strcpy(wp->line,line);
		set_busy(wp, 1);
		if (interval) { sleep(interval); }
	}

	worker_finish(count);

	/* Close input */
	dprintf("closing input...\n");
	if (!using_stdin) fclose(fp);

	system("stty sane icrnl ixon opost isig icanon iexten echo");

	/* Done! */
	pthread_exit(0);
}
#else
int main(void) { return 1; }
#endif
