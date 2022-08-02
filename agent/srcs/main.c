#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>

#define DEBUG 1
#define BUFFER_SIZE 512

enum e_procState
{
	RUNNING = 'R',
	SLEEPING = 'S',			// can interrupt
	DISK_SLEEPING = 'D',	// can't interrupt
	ZOMBIE = 'Z',
	STOPPED = 'T',
	IDLE = 'I',
};

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
	readSize = read(fd, rdBuf, BUFFER_SIZE);
	if (readSize == -1)
	{
		// TODO: Handling read error
		perror("agent");
		return ;
	}
	rdBuf[readSize] = 0;
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
	readSize = read(fd, buf, BUFFER_SIZE);
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
	for (int i = 0; i < 10; i++)
		while (*buf++ != '\n');
	while (*buf++ != ' ');
	size_t swapTotal = atol(buf);

	while (*buf++ != '\n');
	while (*buf++ != ' ');
	size_t swapFree = atol(buf);
#if DEBUG
	printf("Total: %9ld kB\tFree: %9ld kB\tUsed: %9ld kB\tSwap Total: %ld kB\tSwap Free %ld kB\n",
		memTotal, memFree, memTotal - memFree - memBuffers - memCached,
		swapTotal, swapFree);
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
	int readSize = read(fd, buf, BUFFER_SIZE);
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

size_t getMaxPid()
{
	int fd = open("/proc/sys/kernel/pid_max", O_RDONLY);
	if (fd == -1)
	{
		perror("agent");
		return -1;
	}
	char buf[16] = { 0, };
	int readSize = read(fd, buf, 16);
	if (readSize == -1)
	{
		perror("agent");
		close(fd);
		return -1;
	}
	return (atol(buf));
}

void collectProcInfo(char *buf, size_t maxPid)
{
	char fileName[32] = { 0, };
	int fd = 0;
	int readSize = 0;
	int cnt;
	char *pbuf;
	struct passwd *pwd;
	char* userName;
	char* cmdLine;

	// version 1...
	for (int i = 1; i <= maxPid; i++)
	{
		sprintf(fileName, "/proc/%d/stat", i);
		if (access(fileName, F_OK) == 0)
		{
			fd = open(fileName, O_RDONLY);
			if (fd == -1)
			{
				// TODO: handling open error
				perror("agent");
				return ;
			}
			pbuf = buf;
			readSize = read(fd, buf, 512);
			if (readSize == -1)
			{
				// TODO: handling read error
				perror("agent");
				return ;
			}
			pbuf[readSize] = 0;
			char procName[64] = { 0, };
			size_t pid = atol(pbuf);
			while (*pbuf++ != '(');
			for (int j = 0; *pbuf != ')'; j++)
				procName[j] = *pbuf++;
			pbuf += 2;
			u_char state = *pbuf++;
			// TODO: Clarify what every running process means..
			// if (state != RUNNING)
			// {
			// 	close(fd);
			// 	continue;
			// }
			size_t ppid = atol(pbuf);
			cnt = 0;
			while (cnt < 11)
			{
				while (*pbuf++ != ' ');
				cnt++;
			}
			size_t utime = atol(pbuf);
			while (*pbuf++ != ' ');
			size_t stime = atol(pbuf);
			while (*pbuf++ != ' ');
			size_t cutime = atol(pbuf);
			while (*pbuf++ != ' ');
			size_t cstime = atol(pbuf);
			while (*pbuf++ != ' ');
			close(fd);

			int fileNameLen = strlen(fileName);
			fileName[fileNameLen++] = 'u';
			fileName[fileNameLen++] = 's';
			fileName[fileNameLen] = '\0';
			fd = open(fileName, O_RDONLY);
			if (fd == -1)
			{
				// TODO: handling open error
				perror("agent");
				return ;
			}
			readSize = read(fd, buf, 512);
			if (readSize == -1)
			{
				// TODO: handling read error
				perror("agent");
				return ;
			}
			buf[readSize] = 0;
			pbuf = buf;
			for (int j = 0; j < 8; j++)
				while (*pbuf++ != '\n');
			pbuf += 4;
			size_t uid = atol(pbuf);
			pwd = getpwuid(uid);
			userName = strdup(pwd->pw_name);
			close(fd);
#if DEBUG
			//printf("Name: %s\tPID: %ld\tPPID: %ld\tutime: %ld\tstime: %ld\tcutime: %ld\tcstime: %ld\tUID: %ld\t",
			//	procName, pid, ppid, utime, stime, cutime, cstime, uid);
			//printf("user name: %s\n", pwd->pw_name);
			//printf("%c\n", state);
#endif
			free(userName);
			sprintf(fileName, "/proc/%d/cmdline", i);
			fd = open(fileName, O_RDONLY);
			if (fd == -1)
			{
				// TODO: handling open error
				perror("agent");
				return ;
			}
			readSize = read(fd, buf, BUFFER_SIZE);
			if (readSize == -1)
			{
				// TODO: handling read error
				perror("agent");
				return ;
			}
			if (readSize != 0)
			{
				buf[readSize] = 0;
				cmdLine = strdup(buf);
			}
		}
	}
}

int main(void)
{
	char sysbuf[BUFFER_SIZE + 1] = { 0, };
	long logicalCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
	long oneTick = sysconf(_SC_CLK_TCK);
	long toMs = 1000 / oneTick;

	while(1)
	{
		//collectCpuInfo(logicalCoreCount, toMs, sysbuf);
		collectMemInfo(sysbuf);
		//collectNetInfo(sysbuf);
		// collectProcInfo(sysbuf, getMaxPid());
		sleep(1);
	}
}