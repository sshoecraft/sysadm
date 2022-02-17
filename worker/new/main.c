
#include "worker.h"
#include <sys/wait.h>
#include <unistd.h>
#include "version.h"

static void usage(void) {
	printf("usage: worker [options] <input file> <# of threads> <program> [args]\n");
	printf("  where:\n");
	printf("    -V                     print version\n");
	printf("    -t <x>                 kill prog after x seconds\n");
	printf("    -e <file>              error file\n");
	printf("    <input file>           file path or - for stdin\n");
	printf("    <# of threads>         # of <program> to execute @ the same time\n");
	printf("    <program>              program to execute\n");
	printf("    [args]                 optional arguments passed to program\n");
}

enum {
	STATE_NONE,
	STATE_GET_TIMEOUT,
	STATE_GET_ERRFILE,
	STATE_GET_COUNT,
	STATE_GET_PROG,
	STATE_GET_ARGS,
};

struct run_args {
	char line[1024];
	char *error;
};

static char *extra_args[32];
static int num_extra_args;

int do_wait(long pid) {
	long rpid;
	int status, reason;

	dprintf("os_wait: pid: %ld", pid);

	/* Get the exit reason */
	rpid = wait4(pid, &reason, 0, 0);
	if (rpid < 0) {
		perror("wait4");
		exit(1);
	} else if (rpid == 0) return 0;

	dprintf("do_wait: pid: %ld, reason: %x", rpid, reason);

	status = 0;
	if (WIFEXITED(reason)) {
		dprintf("exited.");
		/* Kill the child process (why, I don't know) */
		kill(rpid,SIGTERM);
		status = WEXITSTATUS(reason);
	} else if (WIFSIGNALED(reason)) {
		dprintf("os_wait: signaled.");
		status = WTERMSIG(reason);
	} else if (WIFSTOPPED(reason)) {
		dprintf("os_wait: stopped.");
		status = WSTOPSIG(reason);
	}

	dprintf("os_wait: returning: %d", status);
	return status;
}

int run_prog(void *passed_args) {
	struct run_args *run_args;
	char *argv[32];
	int i,nargv,status;
	pid_t pid;

	/* get args */
	if (!passed_args) return 1;
	run_args = (struct run_args *) passed_args;

	/* Setup argv */
	dprintf("run_prog: line: %s\n", (char *)run_args->line);
	memcpy(argv,extra_args,sizeof(extra_args));
	nargv = num_extra_args;
	argv[nargv++] = run_args->line;
	for(i=0; i < nargv; i++) dprintf("argv[%d]: %s\n", i, argv[i]);

	/* Fork and execute the command */
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	} else if (pid > 0) {
#if 1
		/* Wait for pid */
		waitpid(pid,&status,0);
	
		dprintf("%s WIFEXITED: %d\n", run_args->line,WIFEXITED(status));
		if (WIFEXITED(status)) dprintf("%s WEXITSTATUS: %d\n", run_args->line, WEXITSTATUS(status));
		status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
		kill(pid,SIGTERM);
#else
		status = do_wait(pid);
#endif
		dprintf("%s status: %d\n", run_args->line, status);
		if (status && run_args->error) {
			FILE *fp;

			fp = fopen(run_args->error,"a+");
			if (fp) {
				fprintf(fp,"%s\n",(char *)run_args->line);
				fclose(fp);
			}
		}

		/* Free the args */
		free(run_args);
	} else if (pid == 0) {
		execvp(argv[0],argv);
		perror("exec");
		status = 1;
		_exit(1);
	}

	/* Should never reach here */
	return status;
}

int main(int argc, char **argv) {
	FILE *fp;
	char line[1024], *input, *tc, *prog, *error;
	int using_stdin,count,state,timeout;
	struct run_args *run_args;
	struct worker_pool workers;

	/* Init args */
	memset(extra_args,0,sizeof(extra_args));
	timeout = num_extra_args = 0;

	/* Process command line */
	optind = 1;
	state = STATE_NONE;
	error = input = tc = prog = 0;
	while(optind < argc) {
		dprintf("state: %d, optind: %d, arg: %s\n", state, optind, argv[optind]);
		switch(state) {
		case STATE_NONE:
			if (strcmp(argv[optind],"-t") == 0) {
				state = STATE_GET_TIMEOUT;
			} else if (strcmp(argv[optind],"-e") == 0) {
				state = STATE_GET_ERRFILE;
			} else if (strcmp(argv[optind],"-V") == 0) {
				printf("worker version %s\n", VERSIONSTR);
				printf("CopyLeft(L) 2009-2015 ShadowIT, Inc.\n");
				printf("No rights reserved anywhere\n");
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
		case STATE_GET_ERRFILE:
			error = argv[optind];
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
			extra_args[num_extra_args++] = argv[optind];
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
	dprintf("count: %d\n", count);
	dprintf("timeout: %d\n", timeout);

	for(optind = 0; optind < num_extra_args; optind++) {
		dprintf("args[%d]: %s\n", optind, extra_args[optind]);
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

	/* Create worker pool */
	if (worker_init(&workers, count, timeout)) return 1;

	/* For every line in the input file */
	dprintf("processing input...\n");
	while(fgets(line,sizeof(line),fp)) {
		/* Strip newline, if present */
		while (line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;

		dprintf("line: %s\n", line);
//		run_args = malloc(strlen(line)+1);
		if (!run_args) {
			perror("malloc");
			return 1;
		}
		strcpy(run_args->line,line);
		run_args->error = error;

		/* Run a work item */
		worker_exec(&workers, run_prog, run_args);
	}

	/* Destroy worker pool */
	worker_destroy(&workers);

	/* Close input */
	dprintf("closing input...\n");
	if (!using_stdin) fclose(fp);

	/* Done! */
//	pthread_exit(0);
	return 0;
}
