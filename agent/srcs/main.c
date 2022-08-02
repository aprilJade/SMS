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


void	collectCpuInfo(int us, long cpuCnt, long timeConversion, char* rdBuf)
{
	int	fd;
	int readSize = 0;

	fd = open("/proc/stat", O_RDONLY);
	if (fd == -1)
	{
		// TODO: Handling open error
		perror("agent");
		return ;
	}
	readSize = read(fd, rdBuf, SYS_INFO_BUF_SIZE);
	if (readSize != SYS_INFO_BUF_SIZE)
	{
		// TODO: Handling read error
		perror("agent");
		return ;
	}
	rdBuf[SYS_INFO_BUF_SIZE] = 0;
	for (int i = 0; i < cpuCnt; i++)
	{
		printf("cpu: %d\t", i);
		while (*rdBuf++ != ' ');
		size_t userModeRunningTime = atoll(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		size_t sysModeRunningTime = atoll(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		while (*rdBuf++ != ' ');
		size_t idleTime = atoll(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		size_t waitTime = atoll(rdBuf) * timeConversion;
		printf("user: %8ld ms\tsys: %8ld ms\tidle: %8ld ms\twait: %10ld ms\n",
			userModeRunningTime, sysModeRunningTime, idleTime, waitTime);
		while (*rdBuf++ != '\n');
	}
	usleep(us);
	close(fd);
}

int main(void)
{
	char	buf[SYS_INFO_BUF_SIZE] = { 0, };
	long logicalCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
	long oneTick = sysconf(_SC_CLK_TCK);
	long toMs = 1000 / oneTick;

	while(1)
	{
		collectCpuInfo(2000, logicalCoreCount, toMs, buf);
	}
}