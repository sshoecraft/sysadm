
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "encrypt.h"

int encryptor_init(struct encryptor_context *ctx, encrypt_outfunc_t func, void *arg, unsigned char *key, int keylen) {
	int i, j;
	unsigned char tmp;

	/* fill in linearly s0=0 s1=1... */
	for (i=0;i<256;i++)
		ctx->sbox[i]=i;

	j=0;
	for (i = 0; i < 256; i++) {
		/* j = (j + Si + Ki) mod 256 */
		j = (j + ctx->sbox[i] + key[i % keylen]) % 256;

		/* swap Si and Sj */
		tmp = ctx->sbox[i];
		ctx->sbox[i] = ctx->sbox[j];
		ctx->sbox[j] = tmp;
	}

	/* counters initialized to 0 */
	ctx->i = 0;
	ctx->j = 0;

	ctx->func = func;
	ctx->arg = arg;

	return 0;
}

static int _dobyte(char *name, struct encryptor_context *ctx, int ch) {
	int i = ctx->i;
	int j = ctx->j;
	int tmp;
	int t;
	int K;
	int r;

	if (!ctx) {
		printf("%s_byte: ctx is null!\n",name);
		return 1;
	}
	if (!ctx->func) {
		printf("%s_byte: ctx->func is null!\n",name);
		return 1;
	}

//	printf("%s: ch: %x\n", name, ch);

	r = 0;
#if 0
	if (ch < 0) {
		/* no-op */
		r = ctx->func(ctx->arg,ch);
	} else {
#endif
		i = (i + 1) % 256;
		j = (j + ctx->sbox[i]) % 256;

		/* swap Si and Sj */
		tmp = ctx->sbox[i];
		ctx->sbox[i] = ctx->sbox[j];
		ctx->sbox[j] = tmp;

		t = (ctx->sbox[i] + ctx->sbox[j]) % 256;
		K = ctx->sbox[t];

		/* byte K is Xor'ed with plainctx */
		ctx->func(ctx->arg,ch ^ K);

		ctx->i = i;
		ctx->j = j;
//	}

	return r;
}

int encrypt_byte(void *arg, int ch) {
	struct encryptor_context *ctx = arg;
	return _dobyte("encrypt",ctx,ch);
}

int decrypt_byte(void *arg, int ch) {
	struct encryptor_context *ctx = arg;
	return _dobyte("decrypt",ctx,ch);
}
