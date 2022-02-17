
#include <stdio.h>
#include <string.h>
#include "util.h"

void _bindump(long offset,unsigned char *buf,int len) {
	char line[128];
	int end;
	register char *ptr;
	register int x,y;

	printf("buf: %p, len: %d\n", buf, len);
#ifdef __WIN32__
	if (buf == (void *)0xBAADF00D) return;
#endif

	for(x=y=0; x < len; x += 16) {
		sprintf(line,"%04lX: ",offset);
		ptr = line + strlen(line);
		end=(x+16 >= len ? len : x+16);
		for(y=x; y < end; y++) {
			sprintf(ptr,"%02X ",buf[y]);
			ptr += 3;
		}
		for(y=end; y < x+17; y++) {
			sprintf(ptr,"   ");
			ptr += 3;
		}
		for(y=x; y < end; y++) {
			if (buf[y] > 31 && buf[y] < 127)
				*ptr++ = buf[y];
			else
				*ptr++ = '.';
		}
		for(y=end; y < x+16; y++) *ptr++ = ' ';
		*ptr = 0;
		printf("%s",line);
		offset += 16;
	}
}

int bindump(char *filename) {
	unsigned char data[1024];
	FILE *fp;
	int bytes,offset;

	if (strcmp(filename,"-") == 0) {
		fp = stdin;
	} else {
		fp = fopen(filename,"r");
		if (!fp) {
			sprintf((char *)data,"bindump: fopen %s", filename);
			perror((char *)data);
			return 1;
		}
	}

	offset = 0;
	while( (bytes = fread(data,1,sizeof(data),fp)) > 0) {
		_bindump(offset,data,bytes);
		offset += bytes;
	}

	fclose(fp);
	return 0;
}
