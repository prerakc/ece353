#include "common.h"
#include <ctype.h>
#include <string.h>

int fact (int num);

int
main(int argc, char *argv[])
{
	if (argc == 1) {
		printf("Huh?\n");
		return 0;
	}

	for (int i = 0; i < strlen(argv[1]); i++) {
		if (!isdigit(argv[1][i])) {
			printf("Huh?\n");
			return 0;
		}
	}

	int num = atoi(argv[1]);

	if (num == 0) {
		printf("Huh?\n");
		return 0;
	}

	if (num > 12) {
		printf("Overflow\n");
		return 0;
	}
	
	int ret = fact (num);
	printf("%d\n", ret);
	return 0;
}

int fact (int num) {
	if (num == 1) {
		return 1;
	}

	return num * fact(num - 1);
}
