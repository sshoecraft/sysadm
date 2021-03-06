
#ifndef __MINGW32__
#include "worker.h"
#include <sys/wait.h>

static char *args[32];
static int nargs = 0;

void add_arg(char *arg) {
	if (*arg == '+') *arg = '-';
	dprintf("arg: %s\n", arg);
	args[nargs++] = arg;
}

static int do_exec(int slot, char **args, int serial) {
	int pid,stat;
	char var[32];

	/* Execute the command */
	stat = 77;
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	} else if (pid > 0) {
//		printf("pid: %ld\n", pid);
		usleep(10);
		waitpid(pid,&stat,0);
	} else if (pid == 0) {
		/* Inject WORKER vars into env */
		sprintf(var,"%d",slot);
		setenv("WORKER_SLOT",var,1);
		sprintf(var,"%d",serial);
		setenv("WORKER_SERIAL",var,1);
		execvp(args[0],args);
//		dprintf("worker[%d]: args[0]: %s\n", slot, args[0]);
		perror("exec");
		_exit(1);
	}
	return WEXITSTATUS(stat);
}

int exec_prog(worker_t wp) {
	char *argv[33];
	int x,argc,stat;

	/* Build argv */
	memset(argv, 0, sizeof(argv));
	dprintf("worker[%d]: nargs: %d\n", wp->slot, nargs);
	for(x=argc=0; x < nargs; x++) argv[argc++] = args[x];
	argv[argc] = malloc(strlen(wp->line)+1);
	strcpy(argv[argc],wp->line);
	argc++;

	dprintf("worker[%d]: *****************************\n", wp->slot);
	dprintf("worker[%d]: argc: %d\n", wp->slot, argc);
	for(x=0; x < argc; x++) dprintf("worker[%d]: argv[%d]: %s\n", wp->slot, x, argv[x]);
	dprintf("worker[%d]: *****************************\n", wp->slot);

	dprintf("worker[%d]: exec'ing...\n", wp->slot);
	stat = do_exec(wp->slot, argv, wp->serial);
	dprintf("worker[%d]: stat: %d\n", wp->slot, stat);

//	if (stat != 0 && error_fp) fprintf(error_fp,"%s\n",wp->line);

	return stat;
}
#endif
