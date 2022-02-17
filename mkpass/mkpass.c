
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

	printf("usage: mkpass [options] <key> <string>\n");
	for(popt = options; popt->name; popt++) {
		if (popt->flag) continue;
		printf("\t-%c,--%s\n", popt->val, popt->name);
	}
	exit(code);
}

int display(void *arg, int ch) {
	if (ch >= 0) putc(ch,stdout);
	return 0;
}

int main(int argc, char **argv) {
	char optstr[32],*key,*string,*p;
	int c,r;
	struct option *popt;
	struct encryptor_context encrypt_ctx;
	struct encoder_context encode_ctx;

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
			printf("esxpost version %s\n", VERSIONSTR);
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

	printf("optind: %d, argc: %d\n", optind, argc);
	if (optind >= argc) usage(1);
	key = argv[optind++];
	printf("key: %s\n", key);

	if (optind >= argc) usage(1);
	string = argv[optind++];
	printf("string: %s\n", string);

	/* encrypt -> encode -> display */
	encryptor_init(&encrypt_ctx, encode_byte, &encode_ctx, (unsigned char *)key, strlen(key));
	encoder_init(&encode_ctx, display, 0);

	for(p=string; *p; p++) encrypt_byte(&encrypt_ctx, *p);
	encrypt_byte(&encrypt_ctx, -1);

	return 0;
}
