#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#define SYS_INFO_BUF_SIZE 512

#define SYSTEM_INFO		"/proc/stat"
#define PROCESS_INFO	"/proc/%d/stat"
#define MEMORY_INFO		"/proc/meminfo"
#define DISK_INFO		"/proc/diskstats"
#define NET_INFO		"/proc/net/dev"

int main(void)
{
	int	fd;
	char buf[SYS_INFO_BUF_SIZE + 1] = { 0, };
	long logicalCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
	long oneTick = sysconf(_SC_CLK_TCK);
	long toMs = 1000 / oneTick;
	int readSize = 0;
	while (1)
	{
		fd = open("/proc/stat", O_RDONLY);
		if (fd == -1)
			return 1;
		readSize = read(fd, buf, SYS_INFO_BUF_SIZE);
		if (readSize != SYS_INFO_BUF_SIZE)
			return 1;
		buf[SYS_INFO_BUF_SIZE] = 0;
		char* pb = buf;
		for (int i = 0; i < logicalCoreCount; i++)
		{
			printf("cpu: %d\t", i);
			while (*pb++ != ' ');
			size_t userModeRunningTime = atoll(pb) * toMs;
			while (*pb++ != ' ');
			size_t sysModeRunningTime = atoll(pb) * toMs;
			while (*pb++ != ' ');
			while (*pb++ != ' ');
			size_t idleTime = atoll(pb) * toMs;
			while (*pb++ != ' ');
			size_t waitTime = atoll(pb) * toMs;
			printf("user: %8ld ms\tsys: %8ld ms\tidle: %8ld ms\twait: %10ld ms\n",
				userModeRunningTime, sysModeRunningTime, idleTime, waitTime);
			while (*pb++ != '\n');
		}
		usleep(3000);
		close(fd);
	}
}