#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "collector.h"
#include "packets.h"
#include "collectRoutine.h"

// TODO: Calculate CPU usage about OS....
// Formula: Cpu usage = (CPU running time) / (Logical CPU core count) / (Wall time)
// CPU running time: get from /proc/stat
// Logical CPU: get using sysconf()
// Wall time: get using gettimeofday()
//
// TODO: Calculate CPU usage per process...
// Formula: same as upper one
// CPU running time: get from /proc/[pid]/stat utime + stime
// Logical CPU: get using sysconf()
// Wall time: get using gettimeofday()

void CollectEachCpuInfo(long cpuCnt, long timeConversion, char* rdBuf)
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
    while (*rdBuf++ != '\n');
	for (int i = 0; i < cpuCnt; i++)
	{
		while (*rdBuf++ != ' ');
		size_t userModeRunningTime = atol(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		size_t sysModeRunningTime = atol(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		while (*rdBuf++ != ' ');
		size_t idleTime = atol(rdBuf) * timeConversion;
		while (*rdBuf++ != ' ');
		size_t waitTime = atol(rdBuf) * timeConversion;
		while (*rdBuf++ != '\n');
        printf("cpu%d\tumrt: %10ld\tsmrt: %10ld\tidle: %10ld\twait: %10ld\n",
         i, userModeRunningTime, sysModeRunningTime, idleTime, waitTime);
	}
	close(fd);
}

void CollectCpuInfo(long timeConversion, char* buf, SCpuInfoPacket* packet)
{
	assert(packet != NULL);
    int	fd;
	int readSize = 0;
	fd = open("/proc/stat", O_RDONLY);
	if (fd == -1)
	{
		// TODO: Handling open error
		perror("agent");
		return ;
	}
	readSize = read(fd, buf, BUFFER_SIZE);
	if (readSize == -1)
	{
		// TODO: Handling read error
		perror("agent");
		return ;
	}
	buf[readSize] = 0;
    while (*buf++ != ' ');
    packet->usrCpuRunTime = atol(buf) * timeConversion;
    while (*buf++ != ' ');
    packet->sysCpuRunTime = atol(buf) * timeConversion;
    while (*buf++ != ' ');
    while (*buf++ != ' ');
    packet->idleTime = atol(buf) * timeConversion;
    while (*buf++ != ' ');
    packet->waitTime = atol(buf) * timeConversion;
    while (*buf++ != '\n');
	close(fd);	
}

void CollectMemInfo(char* buf, SMemInfoPacket* packet)
{
	int fd = open("/proc/meminfo", O_RDONLY);
	int readSize;
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
	
	while (*buf++ != ' ');
	ulong memTotal = atol(buf);

	while (*buf++ != '\n');
	while (*buf++ != ' ');
	packet->memFree = atol(buf);
	
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	packet->memAvail = atol(buf);

	while (*buf++ != '\n');
	while (*buf++ != ' ');
	ulong memBuffers = atol(buf);
	
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	ulong memCached = atol(buf);

	packet->memUsed = memTotal - packet->memFree - memBuffers - memCached;
	for (int i = 0; i < 11; i++)
		while (*buf++ != '\n');
	while (*buf++ != ' ');
	packet->swapFree = atol(buf);
	close(fd);
}

void CollectNetInfo(char* buf, SNetInfoPacket* packet)
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
	while (*buf++ != '\n');
	while (*buf++ != ':');
	packet->recvBytes = atol(buf) / 1024;

	while (*buf++ == ' ');
	while (*buf++ != ' ');
	packet->recvPackets = atol(buf);

	for (int i = 0; i < 7; i++)
	{
		while (*buf++ == ' ');
		while (*buf++ != ' ');
	}
	packet->sendBytes = atol(buf) / 1024;

	while (*buf++ == ' ');
	while (*buf++ != ' ');
	packet->sendPackets = atol(buf);
}

void CollectProcInfo(char* path, char *buf, SProcInfoPacket* packet)
{
	char fileName[32] = { 0, };
	int fd = 0;
	int readSize = 0;
	int cnt;
	char *pbuf;
	char* cmdLine;
	struct passwd *pwd;
	
	sprintf(fileName, "%s/stat", path);
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

	packet->pid = atoi(pbuf);
	while (*pbuf++ != '(');
	for (int j = 0; *pbuf != ')' && j < 16; j++)
		packet->procName[j] = *pbuf++;
	while (*pbuf++ != ')');
	pbuf++;
	packet->state = *pbuf++;
	packet->ppid = atoi(pbuf);

	cnt = 0;
	while (cnt < 11)
	{
		while (*pbuf++ != ' ');
		cnt++;
	}
	packet->utime = atol(pbuf) * 1000;

	while (*pbuf++ != ' ');
	packet->stime = atol(pbuf) * 1000;
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
	size_t uid = atoi(pbuf);
	pwd = getpwuid(uid);
	strcpy(packet->userName, pwd->pw_name);
	close(fd);

	sprintf(fileName, "%s/cmdline", path);
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
	if (readSize > 0)
	{
		buf[readSize] = 0;
		cmdLine = strdup(buf);
	}
	else
	{
		packet->cmdlineLen = 0;
	}
	close(fd);
}

void GenerateInitialCpuPacket(SInitialPacket* packet, SRoutineParam* param)
{
	packet->logicalCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
	memcpy(&packet->signature, SIGNATURE_CPU, 4);
	packet->collectPeriod = param->collectPeriod;
}

void GenerateInitialMemPacket(SInitialPacket* packet, SRoutineParam* param)
{
	memcpy(&packet->signature, SIGNATURE_MEM, 4);
	char buf[BUFFER_SIZE + 1] = { 0, };
	int fd = open("/proc/meminfo", O_RDONLY);
	int readSize;
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
	char* pbuf = buf;
	while (*pbuf++ != ' ');
	packet->memTotal = atol(buf);

	for (int i = 0; i < 14; i++)
		while (*pbuf++ != '\n');
	while (*pbuf++ != ' ');
	packet->swapTotal = atol(buf);
	close(fd);
	packet->collectPeriod = param->collectPeriod;
}

void GenerateInitialNetPacket(SInitialPacket* packet, SRoutineParam* param)
{
	memcpy(&packet->signature, SIGNATURE_NET, 4);
	char buf[BUFFER_SIZE + 1] = { 0, };
	char *pbuf = buf;
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
	while (*pbuf++ != '\n');
	while (*pbuf++ != '\n');
	while (*pbuf++ != '\n');
	int i = 0;
	memset(packet->netIfName, 0, 16);
	for (i = 0; *pbuf != ':' && i < 15; i++)
		packet->netIfName[i] = *pbuf++;
	packet->collectPeriod = param->collectPeriod;
}

void GenerateInitialProcPacket(SInitialPacket* packet, SRoutineParam* param)
{
	memcpy(&packet->signature, SIGNATURE_PROC, 4);
	packet->collectPeriod = param->collectPeriod;
}