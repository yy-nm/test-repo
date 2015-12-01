#include <stdio.h>

int main(void)
{
	char *p = (char *) malloc(4 * 1024 * 1024 * 10);
	if (NULL != p)
		printf("malloc success");
	else
		printf("malloc fail");
	free(p);
	return 0;
}
