
#include <stdio.h>
#include <stdlib.h>

char tab[64];
char dtab[128];

void add2tab(char ch) {
	int i,count;

	count = 64;
	while(1) {
		i = (int)( (float)(count-1) * rand() / ( RAND_MAX + 1.0 ) );
//		printf("i: %d, tab[%d]: %d\n", i, i, tab[i]);
		if (!tab[i]) {
			tab[i] = ch;
			dtab[ch] = i;
			break;
		}
	}
}

#if 0
//32 (space)       1
45 (dash)	1
48-57 0-9       10
65-90 A-Z       26
97-122 a-z      26
95 _             1
this-is-three-words
#endif

int main(void) {
	struct {
		int low;
		int high;
	} ranges[] = {
//		{ 32, 32 },
		{ 45, 45 },
		{ 48, 57 },
		{ 65, 90 },
		{ 95, 95 },
		{ 97, 122 },
		{ 0, 0 }
	};
	int i,j;

	memset(tab,0,sizeof(tab));
	memset(dtab,0,sizeof(dtab));
	for(i=0; ranges[i].low; i++) {
		for(j=ranges[i].low; j <= ranges[i].high; j++) {
			add2tab(j);
		}
	}

	printf("unsigned char tab[64] = {\n\t");
	for(i=0; i < 64; i += 8) {
		for(j=i; j < i+8; j++) {
//			printf("%c ", tab[j]);
			printf("0x%02x,", tab[j]);
			if (!tab[j]) {
				printf("tab[%d] is 0!\n", j);
				return 1;
			}
		}
		printf("\n\t");
	}
	printf("};\n");

	printf("unsigned char dtab[128] = {\n\t");
	for(i=0; i < 128; i += 8) {
		for(j=i; j < i+8; j++) {
			printf("0x%02x,", dtab[j]);
		}
		printf("\n\t");
	}
	printf("};\n");

	return 0;
}
