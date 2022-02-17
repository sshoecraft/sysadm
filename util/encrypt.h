
#ifndef __UTIL_ENCRYPT_H
#define __UTIL_ENCRYPT_H

typedef int (*encrypt_outfunc_t)(void *, int);
struct encryptor_context {
	unsigned char sbox[256];
	int i, j;
	encrypt_outfunc_t func;
	void *arg;
};

int encryptor_init(struct encryptor_context *ctx, encrypt_outfunc_t func, void *arg, unsigned char *key, int keylen);
int encrypt_byte(void *arg, int ch);
int decrypt_byte(void *arg, int ch);

#endif
