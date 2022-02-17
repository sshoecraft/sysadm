
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
	putc(ch,fp);
	return 0;
}

int main(int argc, char **argv) {
	char optstr[32],*key,*ifile,*ofile;
	unsigned char buf[4096], *end, *p;
	int i,r,bytes;
	struct option *popt;
	struct encoder_context decode_ctx;
	struct encryptor_context decrypt_ctx;
	FILE *fp,*fp2;
	register unsigned int ch;

	/* Build optstr */
	i = 0;
	for(popt = options; popt->name; popt++) {
		if (!popt->flag) {
			optstr[i++] = popt->val;
			if (popt->has_arg == required_argument)
				optstr[i++] = ':';
		}
	}
//	printf("optstr: %s\n", optstr);

	while ((i = getopt_long(argc, argv, optstr, options, NULL)) != -1) {
		if (i >= 32)
			printf("c(%d): %c\n", i, i);
		else
			printf("c: %d\n", i);
		switch(i) {
		case 'v':
			printf("decfile version %s\n", VERSIONSTR);
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
//	dprintf("key: %s\n", key);

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

        /* in -> decode -> decrypt -> out */

#if 0
	encoder_init(&decode_ctx, out, fp2);
#else
	encoder_init(&decode_ctx, decrypt_byte, &decrypt_ctx);
	encryptor_init(&decrypt_ctx, out, fp2, (unsigned char *)key, strlen(key));
#endif
	while((ch = getc(fp)) != EOF) decode_byte(&decode_ctx, ch);
	decode_end(&decode_ctx);
#if 0
        encoder_init(&decode_ctx, decrypt_byte, &decrypt_ctx);
        encryptor_init(&decrypt_ctx, out, fp2, (unsigned char *)key, strlen(key));
	while(!feof(fp)) {
		bytes = fread(buf,1,sizeof(buf),fp);
		if (bytes < 0) {
			perror("fread");
			return 1;
		}
		if (bytes < 1) break;
//		printf("bytes: %d\n", bytes);
		end = buf + bytes;
		for(p=buf; p < end; p++) decode_byte(&decode_ctx, *p);
	}
	decode_byte(&decode_ctx, -1);
#endif

	return 0;
}
