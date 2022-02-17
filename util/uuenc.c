
/* XXX only want DEBUG defined if debugging this module */
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#if 0
32 (space)	 1
48-57 0-9	10
65-90 A-Z	26
97-122 a-z	26
95 _		 1
--------------	--
total		64
#endif

/* Encoder table */
static unsigned char uutable[64] = {
	0x60,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
	0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
	0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F
};

static unsigned char tab[64] = {
}

#define ENC(c) uutable[(c) & 0x3F]
#define DEC(c) (((c) - 0x20) & 0x3F)

static int encode(char *out, unsigned char *in, int in_len) {
	unsigned char imask, omask;
	unsigned char ich,och;
	int i,j;

	imask = omask = 0x01;
	i = och = 0;
	for(j=0; j < in_len; j++) {
		ich = in[j];
		for(imask=1; imask; imask <<= 1) {
			if (ich & imask) och |= omask;
			omask <<= 1;
			if (omask == 0x40) {
				out[i++] = ENC(och);
				och = 0;
				omask = 1;
			}
		}
	}
	if (och) out[i++] = ENC(och);
	out[i++] = 0;

	return i;
}

static int decode(unsigned char *out, char *in) {
	unsigned char imask, omask;
	unsigned char ich,och;
	char *p;
	int i;

	i = 0;
	imask = omask = 0x01;
	ich = och = 0;
	for (p=in; *p; p++) {
		ich = DEC(*p);
		for(imask=1; imask < 0x40; imask <<= 1) {
			if (ich & imask) och |= omask;
			omask <<= 1;
			if (!omask) {
				out[i++] = och;
				och = 0;
				omask = 1;
			}
		}
	}
	if (och) out[i++] = och;
	out[i] = 0;

	return i;
}
