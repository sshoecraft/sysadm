
#ifndef __UTIL_ENCODE_H
#define __UTIL_ENCODE_H

typedef int (*encode_outfunc_t)(void *, int);
struct encoder_context {
	unsigned char och,omask;
	encode_outfunc_t func;
	void *arg;
};

int encoder_init(struct encoder_context *ctx, encode_outfunc_t func, void *arg);
int encode_byte(void *arg, int ch);
int decode_byte(void *arg, int ch);
void encode_end(struct encoder_context *);
void decode_end(struct encoder_context *);

#endif
