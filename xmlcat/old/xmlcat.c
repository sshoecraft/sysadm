
#include <stdio.h>
#include <string.h>
#include <ctype.h>

char *trim(char *string) {
        register char *src,*dest;

        /* If string is empty, just return it */
        if (*string == '\0') return string;

        /* Trim the front */
        src = string;
        while(isspace((int)*src) || (*src > 0 && *src < 32)) src++;
        dest = string;
        while(*src != '\0') *dest++ = *src++;

        /* Trim the back */
        *dest-- = '\0';
        while((dest >= string) && isspace((int)*dest)) dest--;
        *(dest+1) = '\0';

        return string;
}

void disp(int level, char *data) {
	int x;

	for(x=0; x < level; x++) printf("  ");
	printf("%s\n", data);
}

int main(int argc, char **argv) {
	FILE *fp;
	char temp[4096], f2[3], l2[3];
	int a, i, level, ch;

	if (argc < 2) {
		printf("usage: xmlcat <file>\n");
		return 1;
	}

	for(a=1; a < argc; a++) {
		fp = fopen(argv[a],"r");
		if (!fp) {
			sprintf(temp, "fopen %s", argv[a]);
			perror(temp);
			return 1;
		}

		i = level = 0;
		while((ch = fgetc(fp)) != EOF) {
//			if (ch == '>') {
			if (ch == '<' || ch == '>') {
				if (ch == '>') temp[i++] = ch;
				temp[i] = 0;
				trim(temp);
				i = strlen(temp);
				if (i) {
//					printf("level: %d, temp: ==> %s <==\n", level, temp);
					strncpy(f2,temp,2);
					strncpy(l2,temp+(i-2),2);
//					printf("f2: %s, l2: %s\n", f2, l2);
					if (strcmp(f2,"</") == 0) {
						disp(--level,temp);
//						printf("dec\n");
					} else if (strcmp(f2,"<?") == 0 || strcmp(l2,"/>") == 0) {
						disp(level,temp);
//						printf("no change\n");
					} else if (temp[0] == '<') {
						disp(level++,temp);
//						printf("inc\n");
					}  else {
						disp(level,temp);
//						printf("no change\n");
					}
				}
				i = 0;
				if (ch == '<') temp[i++] = ch;
			} else {
				temp[i++] = ch;
			}
		}
		fclose(fp);
	}

	return 0;
}
