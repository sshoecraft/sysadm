
#include "worker.h"
#define _GNU_SOURCE
#include <getopt.h>
#include <pthread.h>

#define VERSIONSTR "1.0"

int debug;
FILE *error_fp;

static struct option options[] = {
	{ "debug",	no_argument,		0, 'd' },
	{ "error",	required_argument,	0, 'e' },
	{ "help",	no_argument,		0, 'h' },
	{ "version",	no_argument,		0, 'v' },
	{0, 0, 0, 0}
};

static void usage(void) {
	printf("usage: worker [options] <input file> <# of threads> <program> [args]\n");
	printf("  where:\n");
	printf("    -d,--debug             enable debug output\n");
	printf("    -e,--error             path to error file\n");
	printf("    -h,--help              display usage\n");
	printf("    -v,--version           display version\n");
	printf("    <input file>           file path or - for stdin\n");
	printf("    <# of threads>         # of <program> to execute @ the same time\n");
	printf("    <program>              program to execute\n");
	printf("    [args]                 optional arguments passed to program\n");
}

int main(int argc, char **argv) {
	FILE *fp;
	char line[1024], *input, *prog, *error;
	int c,using_stdin,count;
	char optstr[32];
	struct option *popt;
	worker_t wp;

	/* Build optstr */
	c = 0;
	for(popt = options; popt->name; popt++) {
		if (!popt->flag) {
			optstr[c++] = popt->val;
			if (popt->has_arg == required_argument)
				optstr[c++] = ':';
		}
	}
	dprintf("optstr: %s\n", optstr);

	error = 0;
	while ((c = getopt_long(argc, argv, optstr, options, NULL)) != -1) {
		case 0:
			if (options[optind].flag != 0) break;
			printf ("option %s", options[optind].name);
			if (optarg) printf (" with arg %s", optarg);
			printf ("\n");
			break;
		case 'd':
			debug = 1;
			break;
		case 'e':
			error = optarg;
			break;
		case 'v':
			printf("worker version %s\n", VERSIONSTR);
			printf("Copyright(C) 2009 ShadowIT, Inc.\n");
			printf("No rights reserved anywhere.\n");
			return 0;
			break;
		case 'h':
			usage();
			exit(0);
		case '?':
			usage();
			exit(1);
		default:
			break;
		}
	}

	/* Must have at least 3 args (input/count/prog) */
	if (argc - optind < 3) {
		usage();
		exit(1);
	}

	input = argv[optind++];
	dprintf("input: %s\n", input);
	count = atoi(argv[optind++]);
	if (count < 0) {
		usage();
		exit(1);
	}
	dprintf("count: %d\n", count);
	add_arg(argv[optind++]);

	dprintf("optind: %d, argc: %d\n", optind, argc);
	while(optind < argc) {
		dprintf("argv[%d]: %s\n", optind, argv[optind]);
		add_arg(argv[optind]);
		optind++;
	}

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

	error_fp = (FILE *) 0;
	if (error) {
		error_fp = fopen(error,"w+");
		if (!error_fp) {
			perror("unable to open error file");
			return 1;
		}
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
	}

	worker_finish(count);

	/* Close input */
	dprintf("closing input...\n");
	if (!using_stdin) fclose(fp);

	/* Done! */
	pthread_exit(0);
}
