
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef int (*compress_outfunc_t)(void *, int);
struct compressor_context {
	
	compress_outfunc_t func;
	void *arg;
};

int compressor_init(struct compressor_context *ctx, compress_outfunc_t func, void *arg) {
//	ctx->omask = 1;
//	ctx->och = 0;
	ctx->func = func;
	ctx->arg = arg;
}

int compress_byte(void *arg, int ch) {
	struct compressor_context *ctx = arg;

//	printf("compress: ch: %d\n", ch);

	/* no-op */
	ctx->func(ctx->arg,ch);
}

int compress_block(void *arg, unsigned char *buffer,  int buflen) {
	struct compressor_context *ctx = arg;
	char *end;
	register char *p;
	int r;

	end = buffer + buflen;
	r = 0;
	for(p=buffer; p < end; p++) {
		r = compress_byte(ctx,*p);
		if (r != 0) break;
	}

	return r;
}

int compress_file(void *arg, FILE *fp) {
	struct compressor_context *ctx = arg;
	unsigned char buffer[4096];
	int r,bytes;

	r = 0;
	while(1) {
		bytes = fread(buffer,1,sizeof(buffer),fp);
		if (bytes < 1) break;
		r = compress_block(ctx,buffer,bytes);
		if (r != 0) break;
	}
	compress_byte(ctx,-1);

	return r;
}

int decompress_byte(void *arg, int ch) {
	struct compressor_context *ctx = arg;

//	printf("decompress: ch: %d\n", ch);

	/* no-op */
	ctx->func(ctx->arg,ch);
}
