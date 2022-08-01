#include <stdio.h>
#include <unistd.h>

int main(void)
{
	printf("Clock Tick: %ld\n", sysconf(_SC_CLK_TCK));
	return 0;
}