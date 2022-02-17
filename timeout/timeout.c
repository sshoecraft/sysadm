
#ifndef __MINGW32__
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef __MINGW32__
#include <signal.h>
#include <wait.h>
#else
#include <sys/signal.h>
#include <sys/wait.h>
#endif

#ifndef dprintf
#ifdef DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif /* !DEBUG */
#endif

void usage(void) {
	fprintf(stderr, "usage: timeout <seconds> <command>\n");
	_exit(1);
}

int killed;
long pid;

static void _do_exit(int sig) {
	printf("timeout: timeout exceeded, killing process\n");
	dprintf("sending TERM...\n");
	kill(pid, SIGTERM);
	dprintf("sending INT...\n");
	kill(pid, SIGINT);
//	dprintf("sending ABRT...\n");
//	kill(pid, SIGABRT);
	killed = 1;
}

int main(int argc, char **argv) {
	char *args[32];
	int timeout,stat;
	register int x,y;

//dprintf("argc: %d\n", argc);
	if (argc < 3) usage();

	timeout = atoi(argv[1]);
	if (timeout < 1) usage();
//dprintf("timeout: %d\n", timeout);

	/* Setup the args */
	args[0] = argv[2];
	x=3;
	y=1;
	while(x < argc) args[y++] = argv[x++];
	args[y++] = 0;

	for(x=0; x < y && args[x]; x++) dprintf("args[%d]: %s\n", x, args[x]);

	killed = 0;

	/* Execute the command */
	pid = fork();
	dprintf("pid: %ld\n", pid);
	if (pid < 0) {
		perror("timeout: fork");
		exit(1);
	} else if (pid > 0) {
		/* XXX Do the alarm in THIS process context */
		signal(SIGALRM, _do_exit);
		alarm(timeout);
		waitpid(pid,&stat,0);
	} else if (pid == 0) {
		dprintf("timeout: executing: %s\n", argv[2]);
		execvp(argv[2],args);
		_exit(1);
	}

	return (killed ? 1 : WEXITSTATUS(stat));
}
#else
int main(void) { return 0; }
#endif
