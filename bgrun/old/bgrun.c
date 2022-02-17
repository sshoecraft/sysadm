
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

#if DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif

char *trim(char *string) {
        register char *src,*dest;

        /* If string is empty, just return it */
        if (*string == '\0') return string;

        /* Trim the front */
        src = string;
        while(isspace((int)*src) || (*src > 0 && *src < 32)) src++;
        dest = string;
        while(*src != '\0') *dest++ = *src++;

        /* Trim the back */
        *dest-- = '\0';
        while((dest >= string) && isspace((int)*dest)) dest--;
        *(dest+1) = '\0';

        return string;
}

void usage(void) {
	printf("usage: bgrun [options] command args...\n");
	printf("  options:\n");
	printf("    -l <logfile>            redirect stdout+stderr to logfile (must be specified for emailing)\n");
	printf("    -m <mark>               only include lines from log after this mark\n");
	printf("    -t <mark>               only include lines from log before this mark\n");
	printf("    -f <from>               specify from field in email\n");
	printf("    -e <email>              email logfile to these recipients\n");
	printf("    -c <cclist>             cc these recipients when emailing logfile\n");
	printf("    -b <bcclist>            bcc these recipients when emailing logfile\n");
	printf("    -s <subject>            specify subject in email\n");
	printf("    -r                      don't autogenerate a subject\n");
	printf("    -u                      append status to subject\n");
	printf("    -x                      only mail logfile when command returns non-zero exit status\n");
	printf("    -n <noheader>           dont generate email header - will be read from logfile\n");
	printf("    -h                      this listing\n");
	printf("\n");
	printf("  NOTE: Options must be specified seperately.  For example: -u -x, not -ux\n");
}

enum STATES {
	NONE,
	GET_LOG,
	GET_BMARK,
	GET_EMARK,
	GET_EMAIL,
	GET_CC,
	GET_BCC,
	GET_FROM,
	GET_SUB,
	GET_ARGS
};

int main(int argc, char **argv) {
	char *prog, *log, *bmark, *emark, *email, *from, *sub, *cc, *bcc;
	char *args[32];
	pid_t pid;
	int state,count,fdout,optind,have_bmark,len,nohead,nosub,add_status,onerr;

	prog = log = bmark = emark = email = from = sub = cc = bcc = 0;
	nosub = nohead = add_status = onerr = 0;
	fdout = 0;
	memset(args,0,sizeof(args));
	count = 1;

	optind = 1;
	state = NONE;
	while(optind < argc) {
		dprintf("state: %d, optind: %d, arg: %s\n", state, optind, argv[optind]);
		switch(state) {
		case NONE:
			if (strcmp(argv[optind],"-l") == 0) {
				state = GET_LOG;
			} else if (strcmp(argv[optind],"-m") == 0) {
				state = GET_BMARK;
			} else if (strcmp(argv[optind],"-t") == 0) {
				state = GET_EMARK;
			} else if (strcmp(argv[optind],"-e") == 0) {
				state = GET_EMAIL;
			} else if (strcmp(argv[optind],"-c") == 0) {
				state = GET_CC;
			} else if (strcmp(argv[optind],"-b") == 0) {
				state = GET_BCC;
			} else if (strcmp(argv[optind],"-f") == 0) {
				state = GET_FROM;
			} else if (strcmp(argv[optind],"-s") == 0) {
				state = GET_SUB;
			} else if (strcmp(argv[optind],"-r") == 0) {
				nosub = 1;
			} else if (strcmp(argv[optind],"-u") == 0) {
				add_status = 1;
			} else if (strcmp(argv[optind],"-n") == 0) {
				nohead = 1;
			} else if (strcmp(argv[optind],"-x") == 0) {
				onerr = 1;
			} else if (strcmp(argv[optind],"-h") == 0) {
				usage();
				return 0;
			} else if (strncmp(argv[optind],"-",1) == 0) {
				usage();
				return 1;
			} else {
				prog = argv[optind];
				state = GET_ARGS;
			}
			break;
		case GET_LOG:
			log = argv[optind];
			state = NONE;
			break;
		case GET_BMARK:
			bmark = argv[optind];
			state = NONE;
			break;
		case GET_EMARK:
			emark = argv[optind];
			state = NONE;
			break;
		case GET_EMAIL:
			email = argv[optind];
			state = NONE;
			break;
		case GET_CC:
			cc = argv[optind];
			state = NONE;
			break;
		case GET_BCC:
			bcc = argv[optind];
			state = NONE;
			break;
		case GET_FROM:
			from = argv[optind];
			state = NONE;
			break;
		case GET_SUB:
			sub = argv[optind];
			state = NONE;
			break;
		case GET_ARGS:
			args[count++] = argv[optind];
			break;
		}
		optind++;
	}
	dprintf("END: state: %d, optind: %d, arg: %s\n", state, optind, argv[optind]);
	if (state != GET_ARGS) {
		usage();
		return 1;
	}
	args[0] = prog;
	args[count] = 0;

	dprintf("prog: %s, log: %s, email: %s\n", prog, (log ? log : "(none)"), (email ? email : "(none)"));
	dprintf("bmask: %s, emark: %s\n", (bmark ? bmark : ""), (emark ? emark : ""));
	for(optind = 0; optind < count; optind++) dprintf("arg[%d]: %s\n", optind, args[optind]);

	/* Check to make sure prog is executable */
	if (access(prog,X_OK) < 0) {
		char msg[256];

		sprintf(msg, "unable to access %s", prog);
		perror(msg);
		return 1;
	}

	/* Open log */
	if (log) {
		fdout = open(log,O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
		if (fdout < 0) {
			perror("open log");
			return 1;
		}
	}

#if !DEBUG
	/* Fork the process */
	pid = fork();

	/* If pid < 0, error */
	if (pid < 0) {
		perror("fork");
		return 1;
	}

	/* If pid > 0, parent */
	else if (pid > 0)
		_exit(0);

	/* Set the session ID */
	setsid();
#endif

	/* Fork again */
	pid = fork();
	if (pid < 0) {
		perror("fork");
		return 1;
	}

	/* Parent */
	else if (pid != 0) {
		int status, doit;

		/* Wait for child to finish */
		dprintf("pid: %d\n", pid);
		waitpid(pid, &status, 0);

		/* Get exit status */
		dprintf("WIFEXITED: %d\n", WIFEXITED(status));
		if (WIFEXITED(status)) dprintf("WEXITSTATUS: %d\n", WEXITSTATUS(status));
		status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
		dprintf("status: %d\n", status);

		doit = (status == 0 && onerr == 1 ? 0 : 1);

		/* If both log & email specified, email logfile output */
		if (log && email && doit) {
			FILE *fp, *fp2;
			char msg[32], line[1024];

			sprintf(msg,"/tmp/bgrunmsg%d",pid);
			fp = fopen(msg,"w+");
			if (fp) {
				if (nohead == 0) {
					if (from) fprintf(fp,"From: %s\n", from);
					fprintf(fp,"To: %s\n", email);
					if (cc) fprintf(fp,"CC: %s\n", cc);
					if (bcc) fprintf(fp,"BCC: %s\n", bcc);
					if (sub) fprintf(fp,"Subject: %s", sub);
					else if (nosub == 0)
						fprintf(fp,"Subject: logfile %s from command %s", log, prog);
					if (sub) {
						if (nosub == 0 && add_status) {
							if (status == 0)
								fprintf(fp," - SUCCESS\n");
							else if (status == 2)
								fprintf(fp," - WARNING\n");
							else
								fprintf(fp," - FAILED\n");
						} else {
							fprintf(fp,"\n");
						}
					}
					fprintf(fp,"\n");
				}
				fp2 = fopen(log,"r");
				if (fp2) {
					have_bmark = 0;
					while(fgets(line,sizeof(line),fp2)) {
						len = strlen(line);
						if (len) {
							if (line[len-1] == '\n') line[len-1] = 0;
							if (bmark) dprintf("have_bmark: %d, bmark: %s, line: %s\n",
									have_bmark, bmark, line);
							if (bmark && !have_bmark) {
								if (strcmp(bmark,line) == 0) have_bmark = 1;
								continue;
							}
							if (emark && strcmp(line,emark) == 0) break;
						}
						fprintf(fp,"%s\n",line);
					}
					/* If we didn't get the bmark, ... copy entire log */
					dprintf("have_bmark: %d\n", have_bmark);
					if (bmark && !have_bmark) {
						rewind(fp2);
						while(fgets(line,sizeof(line),fp2)) {
							len = strlen(line);
							if (len) {
								if (line[len-1] == '\n') line[len-1] = 0;
								dprintf("line: %s, emark: %s\n", line, (emark ? emark : ""));
								if (emark && strcmp(line,emark) == 0) break;
							}
							fprintf(fp,"%s\n",line);
						}
					}
					fclose(fp2);
				} else {
					perror("fopen logfile");
				}
			} else {
				perror("fopen msg");
			}
			fclose(fp);
#if DEBUG
			sprintf(line,"cat %s", msg);
			system(line);
#endif
			sprintf(line,"cat %s | /usr/sbin/sendmail -t", msg);
			system(line);
			unlink(msg);
		}
		_exit(0);
	}

	/* Close stdin */
	close(0);

	/* If we have a log, make that stdout */
	if (log) {
		dup2(fdout, STDOUT_FILENO);
		close(fdout);
	} else {
		close(1);
	}

	/* Redirect stderr to stdout */
	dup2(1, 2);

	/* Exec prog */
	execvp(prog,args);
	exit(1);
}
