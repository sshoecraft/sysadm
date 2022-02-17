
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#define _GNU_SOURCE
#include <getopt.h>

#include "encrypt.c"
#include "encode.c"

#define VERSIONSTR "1.0"

static struct option options[] = {
	{ "help",	no_argument,		0, 'h' },
	{ "version",	no_argument,		0, 'v' },
	{0, 0, 0, 0}
};

static void usage(int code) {
	struct option *popt;

	printf("usage: encfile [options] <key> <ifile> <ofile>>\n");
	for(popt = options; popt->name; popt++) {
		if (popt->flag) continue;
		printf("\t-%c,--%s\n", popt->val, popt->name);
	}
	exit(code);
}

int out(void *arg, int ch) {
	FILE *fp = (FILE *) arg;
//	if (ch >= 0) putc(ch,fp);
	putc(ch,fp);
	return 0;
}

int main(int argc, char **argv) {
	char optstr[32],*key,*ifile,*ofile;
	unsigned char buf[4096];
	int c,r;
	struct option *popt;
	struct encryptor_context encrypt_ctx;
	struct encoder_context encode_ctx;
	FILE *fp,*fp2;
	register unsigned int ch;

	/* Build optstr */
	c = 0;
	for(popt = options; popt->name; popt++) {
		if (!popt->flag) {
			optstr[c++] = popt->val;
			if (popt->has_arg == required_argument)
				optstr[c++] = ':';
		}
	}
//	printf("optstr: %s\n", optstr);

	while ((c = getopt_long(argc, argv, optstr, options, NULL)) != -1) {
		if (c >= 32)
			printf("c(%d): %c\n", c, c);
		else
			printf("c: %d\n", c);
		switch(c) {
		case 'v':
			printf("encfile version %s\n", VERSIONSTR);
			printf("by Steve Shoecratft (sshoecraft@hp.com)\n");
			return 0;
			break;
		case 'h':
			usage(0);
			return 1;
			break;
		case '?':
			usage(1);
			r = 1;
			break;
		}
	}

//	printf("optind: %d, argc: %d\n", optind, argc);
	if (optind >= argc) usage(1);
	key = argv[optind++];
//	printf("key: %s\n", key);

	/* Input file */
	if (optind >= argc) usage(1);
	ifile = argv[optind++];
//	printf("ifile: %s\n", ifile);
	if (strcmp(ifile,"-") == 0) {
		fp = stdin;
	} else {
		fp = fopen(ifile,"rb");
		if (!fp) {
			sprintf((char *)buf,"fopen(%s,r)",ifile);
			perror((char *)buf);
			return 1;
		}
	}

	/* Output file */
	if (optind >= argc) usage(1);
	ofile = argv[optind++];
//	printf("ofile: %s\n", ofile);
	if (strcmp(ofile,"-") == 0) {
		fp2 = stdout;
	} else {
		fp2 = fopen(ofile,"wb+");
		if (!fp2) {
			sprintf((char *)buf,"fopen(%s,w)",ofile);
			perror((char *)buf);
			return 1;
		}
	}

	/* encrypt -> encode -> display */
	encryptor_init(&encrypt_ctx, encode_byte, &encode_ctx, (unsigned char *)key, strlen(key));
	encoder_init(&encode_ctx, out, fp2);
	while((ch = getc(fp)) != EOF) encrypt_byte(&encrypt_ctx, ch);
	encode_end(&encode_ctx);

	return 0;
}
