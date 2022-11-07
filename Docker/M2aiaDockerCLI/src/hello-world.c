#include <stdio.h>

int main(int argc, char* argv[])
{
	printf("Hello, ");
	if (argc > 1)
		printf("%s!\n", argv[1]);
	else 
		printf("World!\n");
	return 0;
}

