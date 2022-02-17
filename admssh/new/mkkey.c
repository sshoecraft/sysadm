
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "encode.h"
#include "encrypt.h"

void usage() {
	exit(0);
}

char out[8192];
int idx;

int addit(void *arg, int ch) {
	out[idx++] = ch;
}

int main(int argc, char **argv) {
        struct encryptor_context encrypt_ctx;
        struct encoder_context encode_ctx;
	char line[1024],key[128],*phrase,*p;
	char *user;
//	struct winsize ttysize;
	int pty,tty,eatnext;
	int ch,verbose,debug;
	FILE *fp,*fp2;

	phrase = 0;
	while((ch = getopt(argc, argv, "hnvt:d")) != -1) {
		switch(ch) {
		case 'h':
			usage();
			break;
		case 'd':
			debug++;
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}
	printf("debug: %d\n", debug);

	if (optind >= argc) usage();
	user = argv[optind++];
	printf("user: %s\n", user);

	printf("optind: %d, argc: %d\n", optind, argc);
	if (optind < argc) {
		phrase = argv[optind++];
		printf("phrase: %s\n", phrase);
	}

	/* Read and encrypt the key file */
	fp = fopen(user,"r");
	if (!fp) {
		printf("error opening key file(%s): %s\n", user, strerror(errno));
		return 1;
	}

	fp2 = fopen("key.h","w+");
	if (!fp2) {
		printf("error opening key.h: %s\n", strerror(errno));
		return 1;
	}
	fprintf(fp2,"\n");
	fprintf(fp2,"#ifndef __KEY_H\n");
	fprintf(fp2,"#define __KEY_H\n\n");

	gethostname(key,sizeof(key)); strcat(key,"+"); strcat(key,user);
	printf("key(%d): %s\n", strlen(key), key);
	fprintf(fp2,"#define KEY_USER \"%s\"\n",user);

	/* Encrypt the key */
        encryptor_init(&encrypt_ctx, encode_byte, &encode_ctx, (unsigned char *)key, strlen(key));
        encoder_init(&encode_ctx, addit, 0);
	idx=0;
        while((ch = getc(fp)) != EOF) encrypt_byte(&encrypt_ctx, ch);
        encode_end(&encode_ctx);
	out[idx] = 0;
	fprintf(fp2,"#define KEY_DATA \"%s\"\n",out);
	fclose(fp);

	/* Encrypt the phrase */
	if (phrase) {
		idx = 0;
		encryptor_init(&encrypt_ctx, encode_byte, &encode_ctx, (unsigned char *)key, strlen(key));
		encoder_init(&encode_ctx, addit, 0);
		idx=0;
		for(p=phrase; *p; p++) encrypt_byte(&encrypt_ctx, *p);
		encode_end(&encode_ctx);
		out[idx] = 0;
		fprintf(fp2,"#define KEY_PHRASE \"%s\"\n",out);
	}

	fprintf(fp2,"\n");
	fprintf(fp2,"#endif\n");

	return 0;
}
