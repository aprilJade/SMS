#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

int main(void)
{
	int fd;

	if ((fd = open("asdf", O_RDONLY)) == -1)
	{
		perror("agent");
		exit(errno);
	}
	char buf[8192] = { 0, };
	int size = read(fd, buf, 8192);
	buf[size] = 0;
	printf("%s\n", buf);
	close(fd);
	return 0;
}