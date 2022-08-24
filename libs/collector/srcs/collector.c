#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include "collector.h"
#include "packets.h"

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

SCData* CollectEachCpuInfo(ushort cpuCnt, long timeConversion, char* rdBuf, int collectPeriod, char* agentId)
{
	int	fd;
	int readSize = 0;

	fd = open("/proc/stat", O_RDONLY);
	if (fd == -1)
	{
		// TODO: Handling open error
		perror("agent");
		return NULL;
	}
	readSize = read(fd, rdBuf, BUFFER_SIZE);
	if (readSize == -1)
	{
		// TODO: Handling read error
		perror("agent");
		return NULL;
	}
	rdBuf[readSize] = 0;


	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = cpuCnt * sizeof(SBodyc) + sizeof(SHeader);
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
	{
		// TODO: Handling malloc error
		return NULL;
	}

	SHeader* hh = (SHeader*)result->data;
	hh->signature = SIGNATURE_CPU;
	hh->collectPeriod = collectPeriod;
	hh->bodyCount = cpuCnt;
	hh->bodySize = sizeof(SBodyc);
	hh->collectTime = time(NULL);
	memset(hh->agentId, 0, 16);
	strncpy(hh->agentId, agentId, 15);
	hh->agentId[15] = 0;

	SBodyc* handle = (SBodyc*)(result->data + sizeof(SHeader));
    while (*rdBuf++ != '\n');
	for (int i = 0; i < cpuCnt; i++)
	{
		while (*rdBuf++ != ' ');
		handle->usrCpuRunTime = atol(rdBuf) * timeConversion;

		while (*rdBuf++ != ' ');
		handle->sysCpuRunTime = atol(rdBuf) * timeConversion;

		while (*rdBuf++ != ' ');
		while (*rdBuf++ != ' ');
		handle->idleTime = atol(rdBuf) * timeConversion;

		while (*rdBuf++ != ' ');
		handle->waitTime = atol(rdBuf) * timeConversion;
		
		while (*rdBuf++ != '\n');
		handle++;
	}
	close(fd);
	return result;
}

SCData* CollectMemInfo(char* buf, int collectPeriod, char* agentId)
{
	int fd = open("/proc/meminfo", O_RDONLY);
	int readSize;

	if (fd == -1)
	{
		// TODO: handling open error
		perror("agent");
		return NULL;
	}
	readSize = read(fd, buf, BUFFER_SIZE);
	if (readSize == -1)
	{
		// TODO: handling read error
		perror("agent");
		return NULL;
	}
	buf[readSize] = 0;
	
	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = sizeof(SHeader) + sizeof(SBodym);
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
	{
		// TODO: handling malloc error
		return NULL;
	}

	SHeader* hh = (SHeader*)result->data;
	hh->signature = SIGNATURE_MEM;
	hh->bodyCount = 1;
	hh->bodySize = sizeof(SBodym);
	hh->collectPeriod = collectPeriod;
	hh->collectTime = time(NULL);
	memset(hh->agentId, 0, 16);
	strncpy(hh->agentId, agentId, 15);
	hh->agentId[15] = 0;

	SBodym* handle = (SBodym*)(result->data + sizeof(SHeader));

	while (*buf++ != ' ');
	handle->memTotal = atoi(buf);

	while (*buf++ != '\n');
	while (*buf++ != ' ');
	handle->memFree = atoi(buf);
	
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	handle->memAvail = atoi(buf);

	while (*buf++ != '\n');
	while (*buf++ != ' ');
	uint memBuffers = atoi(buf);
	
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	uint memCached = atoi(buf);

	handle->memUsed = handle->memTotal - handle->memFree - memBuffers - memCached;
	for (int i = 0; i < 10; i++)
		while (*buf++ != '\n');
	while (*buf++ != ' ');
	handle->swapTotal = atoi(buf);

	while (*buf++ != '\n');
	while (*buf++ != ' ');
	handle->swapFree = atoi(buf);

	close(fd);
	return result;
}

SCData* CollectNetInfo(char* buf, int nicCount, int collectPeriod, char* agentId)
{
	int fd = open("/proc/net/dev", O_RDONLY);
	if (fd == -1)
	{
		// TODO: handling open error
		perror("agent");
		return NULL;
	}
	int readSize = read(fd, buf, BUFFER_SIZE);
	if (readSize == -1)
	{
		// TODO: handling read error
		perror("agent");
		return NULL;
	}
	buf[readSize] = 0;

	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = sizeof(SHeader) + sizeof(SBodyn) * nicCount;
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
	{
		// TODO: handle malloc error
		return NULL;
	}
	SHeader* hh = (SHeader*)result->data;
	hh->signature = SIGNATURE_NET;
	hh->bodyCount = nicCount;
	hh->bodySize = sizeof(SBodyn);
	hh->collectPeriod = collectPeriod;
	hh->collectTime = time(NULL);
	memset(hh->agentId, 0, 16);
	strncpy(hh->agentId, agentId, 15);
	hh->agentId[15] = 0;
	SBodyn* handle = (SBodyn*)(result->data + sizeof(SHeader));

	while (*buf++ != '\n');
	while (*buf++ != '\n');
	int j;
	for (int i = 0; i < nicCount; i++)
	{
		while (*buf == ' ') 
			buf++;

		memset(handle->name, 0, 15);
		for (j = 0; *buf != ':' && j < 15; j++)
			handle->name[j] = *buf++;
		handle->nameLength = j;
		
		while (*buf++ != ' ');

		handle->recvBytes = atol(buf);

		while (*buf++ == ' ');
		while (*buf++ != ' ');
		handle->recvPackets = atol(buf);

		for (j = 0; j < 7; j++)
		{
			while (*buf++ == ' ');
			while (*buf++ != ' ');
		}
		handle->sendBytes = atol(buf);

		while (*buf++ == ' ');
		while (*buf++ != ' ');
		handle->sendPackets = atol(buf);

		while (*buf++ != '\n');
		handle++;
	}
	close(fd);
	return result;
}

SCData* CollectProcInfo(char *buf, uchar* dataBuf, int collectPeriod, char* agentId)
{
	char filePath[272] = { 0, };
	int fd = 0;
	int readSize = 0;
	char *pbuf;
	struct passwd *pwd;

	DIR* dir;
    struct dirent* entry;
    char path[264];

	uchar* handle = dataBuf;
	int procCnt = 0;
	SBodyp* hBody;
	SHeader* hHeader;
	ulong collectBeginTime = time(NULL);

	dir = opendir("/proc");
	if (dir == NULL)
	{
		// TODO: handle error
		return NULL;
	}
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_DIR)
		{
			if (atoi(entry->d_name) < 1)
				continue;
			snprintf(path, 264, "/proc/%s", entry->d_name);
			if (access(path, F_OK) != 0)
				continue;
			hBody = (SBodyp*)handle;

			// 2. parse /proc/[pid]/stat
			snprintf(filePath, 272, "%s/stat", path);
			fd = open(filePath, O_RDONLY);
			if (fd == -1)
			{
				// TODO: handling open error
				perror("agent");
				return NULL;
			}
			pbuf = buf;
			readSize = read(fd, buf, BUFFER_SIZE);
			if (readSize == -1)
			{
				// TODO: handling read error
				perror("agent");
				return NULL;
			}
			pbuf[readSize] = 0;

			hBody->pid = atoi(pbuf);
			while (*pbuf++ != '(');
			int j;
			for (j = 0; *pbuf != ')' && j < 15; j++)
				hBody->procName[j] = *pbuf++;
			hBody->procName[j] = 0;
			while (*pbuf++ != ')');
			pbuf++;
			hBody->state = *pbuf++;
			hBody->ppid = atoi(pbuf);

			for (int cnt = 0; cnt < 11; cnt++)
				while (*pbuf++ != ' ');

			hBody->utime = atoi(pbuf);

			while (*pbuf++ != ' ');
			hBody->stime = atoi(pbuf);
			close(fd);
			

			int fileNameLen = strlen(filePath);
			filePath[fileNameLen++] = 'u';
			filePath[fileNameLen++] = 's';
			filePath[fileNameLen] = '\0';
			fd = open(filePath, O_RDONLY);
			if (fd == -1)
			{
				// TODO: handling open error
				perror("agent");
				return NULL;
			}
			readSize = read(fd, buf, BUFFER_SIZE);
			if (readSize == -1)
			{
				// TODO: handling read error
				perror("agent");
				return NULL;
			}
			buf[readSize] = 0;

			pbuf = buf;
			for (int j = 0; j < 8; j++)
				while (*pbuf++ != '\n');
			pbuf += 4;
			size_t uid = atoi(pbuf);
			pwd = getpwuid(uid);
			strcpy(hBody->userName, pwd->pw_name);
			close(fd);

			handle += sizeof(SBodyp);

			// Parse cmdline information
			snprintf(filePath, 272, "%s/cmdline", path);
			if ((fd = open(filePath, O_RDONLY)) == -1)
			{
				// TODO: handling open error
				perror("agent");
				return NULL;
			}

			if ((readSize = read(fd, buf, BUFFER_SIZE)) == -1)
			{
				// TODO: handling read error
				perror("agent");
				return NULL;
			}
			buf[readSize] = 0;
			close(fd);

			hBody->cmdlineLen = readSize;
			if (readSize > 0)
			{
				memcpy(handle, buf, readSize);
				handle += readSize;
			}
			procCnt++;
		}
	}
	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = handle - dataBuf + sizeof(SHeader);
	result->data = (uchar*)malloc(sizeof(uchar) * result->dataSize);
	memcpy(result->data + sizeof(SHeader), dataBuf, result->dataSize - sizeof(SHeader));
	hHeader = (SHeader*)result->data;

	hHeader->bodyCount = procCnt;
	hHeader->bodySize = result->dataSize - sizeof(SHeader);
	hHeader->collectPeriod = collectPeriod;
	hHeader->signature = SIGNATURE_PROC;
	hHeader->collectTime = collectBeginTime;
	memset(hHeader->agentId, 0, 16);
	strncpy(hHeader->agentId, agentId, 15);
	hHeader->agentId[15] = 0;

	return result;
}


SCData* CollectDiskInfo(char *buf, int diskDevCnt, int collectPeriod, char* agentId)
{
	int	fd;
	int readSize = 0;

	fd = open("/proc/diskstats", O_RDONLY);
	if (fd == -1)
	{
		// TODO: Handling open error
		perror("agent");
		return NULL;
	}
	readSize = read(fd, buf, BUFFER_SIZE);
	if (readSize == -1)
	{
		// TODO: Handling read error
		perror("agent");
		return NULL;
	}
	buf[readSize] = 0;
	close(fd);

	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = diskDevCnt * sizeof(SBodyd) + sizeof(SHeader);
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
	{
		// TODO: Handling malloc error
		return NULL;
	}

	SHeader* hh = (SHeader*)result->data;
	hh->signature = SIGNATURE_DISK;
	hh->collectPeriod = collectPeriod;
	hh->bodyCount = diskDevCnt;
	hh->bodySize = sizeof(SBodyd);
	hh->collectTime = time(NULL);
	memset(hh->agentId, 0, 16);
	strncpy(hh->agentId, agentId, 15);
	hh->agentId[15] = 0;

	SBodyd* hBody;
	hBody = (SBodyd*)(result->data + sizeof(SHeader));

	while (*buf)
	{
		while (*buf == ' ')
			buf++;
		while (*buf != ' ')
			buf++;
		while (*buf == ' ')
			buf++;
		while (*buf != ' ')
			buf++;
		
		buf++;
		if (strncmp(buf, "loop", 4) == 0)
		{
			while(*buf++ != '\n');
			continue;
		}

		memset(hBody->name, 0, 16);
		for (int i = 0; *buf != ' ' && i < 15; i++)
			hBody->name[i] = *buf++;

		buf++;
		hBody->readSuccessCount = atol(buf);
		while (*buf++ != ' ');
		while (*buf++ != ' ');

		hBody->readSectorCount = atol(buf);
		while (*buf++ != ' ');

		hBody->readTime = atol(buf);
		while (*buf++ != ' ');

		hBody->writeSuccessCount = atol(buf);
		while (*buf++ != ' ');
		while (*buf++ != ' ');

		hBody->writeSectorCount = atol(buf);
		while (*buf++ != ' ');
		
		hBody->writeTime = atol(buf);
		while (*buf++ != ' ');

		hBody->currentIoCount = atoi(buf);
		while (*buf++ != ' ');

		hBody->doingIoTime = atol(buf);
		while (*buf++ != ' ');

		hBody->weightedDoingIoTime = atol(buf);
		while(*buf++ != '\n');

		hBody++;
	}
	
	return result;
}