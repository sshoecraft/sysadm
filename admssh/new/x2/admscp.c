
#include <stdio.h>
#define __USE_XOPEN
#define __USE_GNU
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
//#include <stropts.h>
#include <sys/ioctl.h>
#include <termio.h>
#include <errno.h>
#include <sys/wait.h>
#include <ctype.h>
#include "version.h"
#include "key.h"
#include "encode.h"
#include "encrypt.h"

extern int fd_printf(int fd,char *fmt,...);
extern int fd_telgets(int fd,char *buffer,unsigned short buflen,int wait);

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

#if DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif /* !DEBUG */

char proxy[64];
int verbose;

static pid_t pid;
static int term = 0;
void catch_alarm(int sig) {
//	printf("timeout expired, killing pid %d\n", (int)pid);
	printf("syscmd: timeout exceeded, aborting!\n");
	if (term)
		kill(pid,SIGKILL);
	else {
		kill(pid,SIGTERM);
		term++;
		alarm(3);
	}
}

enum {
	NONE,
	GET_TIMEOUT,
	GET_PROXY,
	GET_HOST,
	GET_CMD,
	GET_ARGS,
};

void usage(void) {
	printf("usage: syscmd [-v] [-V] [-t timeout] hostname command [args...]\n");
}

char out[256];
int idx;

int addit(void *arg, int ch) {
	if (ch < 0) {
		idx = 0;
		return ch;
	} else {
		out[idx++] = ch;
		return 0;
	}
}

char *ignore[] = {
	"The authenticity of host",
	"RSA key fingerprint",
	"Warning: Permanently added",
	0
};

int runargs(char *host, char **args, int timeout, char *key) {
#ifdef KEY_PHRASE
        struct encryptor_context decrypt_ctx;
        struct encoder_context decode_ctx;
	char *phrase,*p;
#endif
	char line[1024];
	struct winsize ttysize;
	int pty,tty,eatnext;

#ifdef KEY_PHRASE
	/* decode -> decrypt -> addit */
	idx = 0;
	encoder_init(&decode_ctx, decrypt_byte, &decrypt_ctx);
	encryptor_init(&decrypt_ctx, addit, 0, (unsigned char *)key, strlen(key));
	for(p=KEY_PHRASE; *p; p++) decode_byte(&decode_ctx, *p);
	decode_end(&decode_ctx);
	out[idx] = 0;
	phrase = out;
	printf("phrase: %s\n", phrase);
#endif

	if ((pty = getpt()) < 0) {
		perror("openpt");
		return -1;
	}
	if (grantpt(pty)) {
		perror("granpt");
		return -1;
	}
	if (unlockpt(pty)) {
		perror("unlockpt");
		return -1;
	}

	/* Open our tty (only if we have one) */
	tty = open("/dev/tty", 0);
	if (tty >= 0) {
		if (ioctl(tty, TIOCGWINSZ, &ttysize) < 0) {
			perror("ioctl");
			return -1;
		}

		/* Catch resizes */
//		signal(SIGWINCH, window_resize_handler);

		/* Set ttysize on pty */
		ioctl(pty, TIOCSWINSZ, &ttysize);
	}

	pid = fork();
	if (pid < 0) {
		/* Error */
		perror("fork");
		return -1;
	} else if (pid == 0) {
		/* Child */
		int pts;
		struct termios old_tcattr,tcattr;

		/* Detach from current tty */
		setsid();

		/* Open pty slave */
		pts = open(ptsname(pty), O_RDWR );
		if (pts < 0) perror("open pts");

		/* Close master */
		close(pty);

		/* Set raw mode term attribs */
		tcgetattr(pts, &old_tcattr); 
		tcattr = old_tcattr;
		cfmakeraw (&tcattr);
		tcsetattr (pts, TCSANOW, &tcattr); 

		/* Set stdin/stdout/stderr with pts */
		dup2(pts, STDIN_FILENO);
		dup2(pts, STDOUT_FILENO);
		dup2(pts, STDERR_FILENO);

		/* Exec prog */
		execvp(args[0],args);
		perror("exec");
		_exit(1);
	} else {
		/* Parent */
		struct sigaction act1, oact1;
		int status,i;

		/* Set up timeout alarm */
		if (timeout) {
			act1.sa_handler = catch_alarm;
			sigemptyset(&act1.sa_mask);
			act1.sa_flags = 0;
#ifdef SA_INTERRUPT
			act1.sa_flags |= SA_INTERRUPT;
#endif
			if( sigaction(SIGALRM, &act1, &oact1) < 0 ){
				perror("sigaction");
				exit(1);
			}
			alarm(timeout);
		}

		eatnext = 0;
		while(fd_telgets(pty,line,sizeof(line),0)) {
			for(i=0; ignore[i]; i++) {
				if (strncmp(line,ignore[i],strlen(ignore[i])) == 0) {
					eatnext = 1;
					break;
				}
			}
			if (eatnext) {
				eatnext = 0;
				continue;
			}
#define SURE "Are you sure"
			if (strncmp(line,SURE,strlen(SURE)) == 0) {
				dprintf("answering question...\n");
				fd_printf(pty, "yes\n");
				eatnext = 1;
#ifdef KEY_PHRASE
#define ASKPH "Enter passphrase for key"
			} else if (strncmp(line,ASKPH,strlen(ASKPH)) == 0) {
				dprintf("writing phrase...\n");
				fd_printf(pty, "%s\n", phrase);
				eatnext = 1;
#endif
#define ASKPW "assword:"
			} else if (strstr(line,ASKPW) || strstr(line,"expired")) {
				printf("problem with %s account on %s\n", KEY_USER, host);
				kill(pid,SIGKILL);
				return -1;
			} else {
				printf("%s", line);
			}
		}
		dprintf("waiting on pid...\n");
		waitpid(pid,&status,0);
		dprintf("WIFEXITED: %d\n", WIFEXITED(status));
		if (WIFEXITED(status)) dprintf("WEXITSTATUS: %d\n", WEXITSTATUS(status));
		status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
		dprintf("status: %d\n", status);
		return status;
	}
	return 0;
}

char tempfile[64];
void deltmp(void) {
	unlink(tempfile);
}

int sendbyte(void *arg, int ch) {
//	putchar(ch);
	return fputc(ch, (FILE *)arg);
}

int dohost(int one, char **argv, int nargv, int timeout) {
	char *args[64],key[64],pcmd[4096];
	char *tmpdir;
	int count,i,pid;

	/* Init args */
	memset(args,0,sizeof(args));
	count = 0;

	if (access("/dev/shm",W_OK) == 0) {
		tmpdir = "/dev/shm";
	} else if (access("/var/tmp",W_OK) == 0) {
		tmpdir = "/var/tmp";
	} else {
		tmpdir = "/tmp";
	}
	sprintf(tempfile,"%s/sc%d",tmpdir,getpid());
//	sprintf(tempfile,"%s/scfifo",tmpdir);
//	dprintf("tempfile: %s\n", tempfile);

	atexit(deltmp);
//	unlink(tempfile);
	if (mkfifo(tempfile,0600) < 0) {
		perror("mkfifo");
		exit(1);
	}

	gethostname(key,sizeof(key));
	strcat(key,"+");
	strcat(key,KEY_USER);

	/* open a pipe and feed the key through the pipe */
	pid = fork();
//	dprintf("pid: %d\n", pid);
	if (pid < 0) {
		/* Error */
		perror("fork");
		return -1;
	} else if (pid == 0) {
        	struct encryptor_context decrypt_ctx;
	        struct encoder_context decode_ctx;
		FILE *cfp;
		register char *p;

		/* Child */
		for(i=0; i < 20; i++) {
//			dprintf("opening...\n");
			cfp = fopen(tempfile,"wb");
//			dprintf("cfp: %p\n", cfp);
			if (!cfp) {
				perror("child fopen");
				exit(1);
			}
//			dprintf("sending...\n");
			encoder_init(&decode_ctx, decrypt_byte, &decrypt_ctx);
			encryptor_init(&decrypt_ctx, sendbyte, cfp, (unsigned char *)key, strlen(key));
			for(p=KEY_DATA; *p; p++) decode_byte(&decode_ctx, *p);
			decode_end(&decode_ctx);
			fprintf(cfp,"\n");
			fclose(cfp);
			usleep(500);
//			sleep(1);
//			dprintf("sent!\n");
		}
		dprintf("exiting...\n");
		_exit(0);
	} else {
#if 0
		int status;

		usleep(500);
                dprintf("waiting on pid...\n");
                waitpid(pid,&status,0);
                dprintf("WIFEXITED: %d\n", WIFEXITED(status));
                if (WIFEXITED(status)) dprintf("WEXITSTATUS: %d\n", WEXITSTATUS(status));
                status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
                dprintf("status: %d\n", status);
#endif

//		args[count++] = "/usr/bin/strace";
		args[count++] = "/usr/bin/scp";
		args[count++] = "-2";
		args[count++] = "-x";
		args[count++] = "-n";
		args[count++] = "-q";
		args[count++] = "-tt";
		args[count++] = "-l";
		args[count++] = KEY_USER; 
		args[count++] = "-i";
		args[count++] = tempfile;
//		args[count++] = "sysadm";
		args[count++] = "-F";
		args[count++] = "/dev/null";
		args[count++] = "-o";
		args[count++] = "UserKnownHostsFile=/dev/null";
		args[count++] = "-o";
		args[count++] = "StrictHostKeyChecking=no";
		args[count++] = "-o";
		args[count++] = "ForwardAgent=no";
		args[count++] = "-o";
		args[count++]=  "ClearAllForwardings=yes";
		args[count++] = "-o";
		args[count++] = "BatchMode=no";
		args[count++] = "-o";
		args[count++] = "ConnectTimeout=5";
#if 0
		args[count++] = "-o";
		args[count++] = "ProxyCommand=\'\'/usr/bin/ssh -tt usow-coe\'\'";
#else
		if (strlen(proxy)) {
			sprintf(pcmd,"ProxyCommand=\'\'");
			for(i=0; i < count; i++) {
#if 0
				if (strcmp(args[i],"-i") == 0) {
//					strcat(pcmd,"-i sysadm ");
					i++;
					continue;
				}
#endif
				strcat(pcmd,args[i]);
				strcat(pcmd," ");
			}
			strcat(pcmd,"-W %h:%p ");
			strcat(pcmd,proxy);
			strcat(pcmd,"\'\'");
//printf("pcmd: %s\n", pcmd);
			args[count++] = "-o";
			args[count++] = pcmd;
		}
#endif
		args[count++] = "--";

		/* Add argv to args */
		dprintf("nargv: %d\n", nargv);
		for(i=0; i < nargv; i++) args[count++] = argv[i];
		for(i = 0; i < count; i++) dprintf("args[%d]: %s\n", i, args[i]);

#if DEBUG
		{
			char cmd[1024];
			cmd[0] = 0;
			for(i = 0; i < count; i++) {
				strcat(cmd," ");
				strcat(cmd,args[i]);
			}
			printf("cmd: %s\n", cmd);
		}
#endif

		i = runargs(argv[0],args,timeout,key);
		return i;
	}
}

int main(int argc, char **argv) {
	char *args[32], *host, *cmd;
	int r,optind,count,state,timeout;

	printf("argv[0]: %s\n", argv[0]);

	/* Setup args */
	memset(args,0,sizeof(args));
	count = 0;

	/* Process command line */
	optind = 1;
	host = cmd = 0;
	verbose = 0;
	timeout = 0;
	state = NONE;
	while(optind < argc) {
		dprintf("state: %d, optind: %d, arg: %s\n", state, optind, argv[optind]);
		switch(state) {
		case NONE:
			if (strcmp(argv[optind],"-t") == 0) {
				state = GET_TIMEOUT;
			} else if (strcmp(argv[optind],"-p") == 0) {
				state = GET_PROXY;
			} else if (strcmp(argv[optind],"-v") == 0) {
				verbose = 1;
			} else if (strcmp(argv[optind],"-V") == 0) {
				printf("syscmd version %s\n", VERSIONSTR);
				printf("Copyright(C) 2010 ShadowIT, Inc.\n");
				printf("No rights reserved anywhere.\n");
				return 0;
			} else if (strcmp(argv[optind],"-h") == 0) {
				usage();
				return 0;
			} else if (strncmp(argv[optind],"-",1) == 0) {
				usage();
				return 1;
			} else {
				host = argv[optind];
				args[count++] = argv[optind];
				state = GET_CMD;
			}
			break;
		case GET_TIMEOUT:
			timeout = atoi(argv[optind]);
			state = NONE;
			break;
		case GET_PROXY:
			strcpy(proxy,argv[optind]);
			state = NONE;
			break;
		case GET_CMD:
			cmd = argv[optind];
			state = GET_ARGS;
			/* Fall through */
		case GET_ARGS:
			args[count++] = argv[optind];
			break;
		}
		optind++;
	}
	dprintf("host: %p, cmd: %p\n", host, cmd);
	if (!host || !cmd) {
		usage();
		return 1;
	}
	dprintf("timeout: %d\n", timeout);

#if DEBUG
	printf("Executing command:");
	for(optind=1; optind < count; optind++) printf(" %s", args[optind]);
	printf("\n");
#endif

	r = dohost(1,args,count,timeout);
#if 0
	dprintf("r: %d\n", r);
	/* If error, fallback to 2 */
	if (r < 0) r = dohost(0,args,count,timeout);
#endif
	return r;
}
