#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#define DEBUG 1
#define SYS_INFO_BUF_SIZE 512
#define MEM_INFO_BUF_SIZE 512
#define SYSTEM_INFO		"/proc/stat"
#define PROCESS_INFO	"/proc/%d/stat"
#define MEMORY_INFO		"/proc/meminfo"
#define DISK_INFO		"/proc/diskstats"
#define NET_INFO		"/proc/net/dev"


void collectCpuInfo(long cpuCnt, long timeConversion, char* rdBuf)
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
		while (*rdBuf++ != ' ');
		size_t userModeRunningTime = atoll(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		size_t sysModeRunningTime = atoll(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		while (*rdBuf++ != ' ');
		size_t idleTime = atoll(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		size_t waitTime = atoll(rdBuf) * timeConversion;
#if DEBUG
		printf("cpu%d:\tuser: %8ld ms\tsys: %8ld ms\tidle: %8ld ms\twait: %10ld ms\n",
			i, userModeRunningTime, sysModeRunningTime, idleTime, waitTime);
#endif
		while (*rdBuf++ != '\n');
	}
	close(fd);
}

void collectMemInfo(char* buf)
{
	int fd = open("/proc/meminfo", O_RDONLY);
	int readSize;
	static size_t memTotal;
	if (fd == -1)
	{
		// TODO: handling open error
		perror("agent");
		return ;
	}
	readSize = read(fd, buf, MEM_INFO_BUF_SIZE);
	if (readSize == -1)
	{
		// TODO: handling read error
		perror("agent");
		return ;
	}
	buf[readSize] = 0;
	if (memTotal == 0)
	{
		while (*buf++ != ' ');
		memTotal = atoll(buf);
	}
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	size_t memFree = atoll(buf);

	while (*buf++ != '\n');
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	size_t memBuffers = atoll(buf);
	
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	size_t memCached = atoll(buf);
#if DEBUG
	printf("Total: %9ld kB\tFree: %9ld kB\tUsed: %9ld kB\n",
		memTotal, memFree, memTotal - memFree - memBuffers - memCached);
#endif
	close(fd);
}

void collectNetInfo(char* buf)
{
	int fd = open("/proc/net/dev", O_RDONLY);
	if (fd == -1)
	{
		// TODO: handling open error
		perror("agent");
		return ;
	}
	int readSize = read(fd, buf, 512);
	if (readSize == -1)
	{
		// TODO: handling read error
		perror("agent");
		return ;
	}
	buf[readSize] = 0;
	while (*buf++ != '\n');
	while (*buf++ != '\n');
	char niName[16] = { 0, };

	while (*buf)
	{
		while (*buf == ' ')
			buf++;
		for (int i = 0; *buf != ':'; i++)
			niName[i] = *buf++;
		buf++;
		size_t recvBytes = atol(buf);
		while (*buf++ == ' ');
		while (*buf++ != ' ');
		size_t recvPacketCnt = atol(buf);
		for (int i = 0; i < 7; i++)
		{
			while (*buf++ == ' ');
			while (*buf++ != ' ');
		}
		size_t sendBytes = atol(buf);
		while (*buf++ == ' ');
		while (*buf++ != ' ');
		size_t sendPacketCnt = atol(buf);
		while (*buf != '\n' && *buf != '\0')
			buf++;
		if (*buf == '\0')
			break;
		buf++;
#if DEBUG
		printf("name: %12s\trecv bytes: %12ld B\trecv packet: %8ld\tsend bytes: %12ld B\tsend packet: %8ld\n",
			niName, recvBytes, recvPacketCnt, sendBytes, sendPacketCnt);
#endif
	}
}

int main(void)
{
	char sysbuf[SYS_INFO_BUF_SIZE + 1] = { 0, };
	long logicalCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
	long oneTick = sysconf(_SC_CLK_TCK);
	long toMs = 1000 / oneTick;
	//int netInterfaceCnt = getNetInterfaceCount();
	while(1)
	{
		collectCpuInfo(logicalCoreCount, toMs, sysbuf);
		collectMemInfo(sysbuf);
		collectNetInfo(sysbuf);
		sleep(1);
	}
}