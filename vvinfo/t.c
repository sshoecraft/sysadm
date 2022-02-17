#include <stdio.h>
int main(void) {
	long long v = 8796093021696LL;
	long double f;

	printf("v: %lld\n", v);
	f = v;
	printf("f: %Lf\n", f);
	f /= 1048576;
	printf("f: %.2Lf\n", f);
}
