
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef int (*encrypt_outfunc_t)(void *, int);
struct encryptor_context {
	unsigned char sbox[256];
	int i, j;
	encrypt_outfunc_t func;
	void *arg;
};

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

int encrypt_byte(void *arg, int ch) {
	struct encryptor_context *ctx = arg;
	int i = ctx->i;
	int j = ctx->j;
	int tmp;
	int t;
	int K;

//	printf("encrypt: ch: %d\n", ch);

	if (ch < 0) {
		/* no-op */
		ctx->func(ctx->arg,ch);
	} else {
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
	}

	return 0;
}

encrypt_outfunc_t decrypt_byte = encrypt_byte;

#if 0
int decrypt(void *arg, int ch) {
	struct encryptor_context *ctx = arg;
	int tmp;
	int i = ctx->i;
	int j = ctx->j;
	int t;
	int K;

	printf("decrypt: ch: %d\n", ch);

	if (ch < 0) {
		/* no-op */
		ctx->func(ctx->arg,ch);
	} else {
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
	}
}
#endif
