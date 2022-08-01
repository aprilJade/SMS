#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#define SYSTEM_INFO "/proc/stat"
#define PROCESS_INFO "/proc/%d/stat"
#define MEMORY_INFO "/proc/meminfo"
#define DISK_INFO "/proc/diskstats"
#define NET_INFO "/proc/net/dev"

int main(void)
{
	int fd;

	if ((fd = open(SYSTEM_INFO, O_RDONLY)) == -1)
	{
		perror("agent: ");
		exit(errno);
	}
	char buf[8192] = { 0, };
	int size = read(fd, buf, 8192);
	buf[size] = 0;
	printf("%s\n", buf);
	close(fd);
	return 0;
}