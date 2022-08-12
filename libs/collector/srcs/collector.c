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

SCData* CollectEachCpuInfo(ushort cpuCnt, long timeConversion, char* rdBuf, int collectPeriod)
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

SCData* CollectMemInfo(char* buf, int collectPeriod)
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
	//memcpy(result, SIGNATURE_MEM, 4);
	hh->signature = SIGNATURE_MEM;
	hh->bodyCount = 1;
	hh->bodySize = sizeof(SBodym);
	hh->collectPeriod = collectPeriod;
	hh->collectTime = time(NULL);

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

SCData* CollectNetInfo(char* buf, int nicCount, int collectPeriod)
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
	//memcpy(result, SIGNATURE_NET, 4);
	hh->signature = SIGNATURE_NET;
	hh->bodyCount = nicCount;
	hh->bodySize = sizeof(SBodyn);
	hh->collectPeriod = collectPeriod;
	hh->collectTime = time(NULL);
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

SCData* CollectProcInfo(char *buf, uchar* dataBuf, int collectPeriod)
{
	char filePath[260] = { 0, };
	int fd = 0;
	int readSize = 0;
	char *pbuf;
	struct passwd *pwd;

	DIR* dir;
    struct dirent* entry;
    char path[260];

	uchar* tmp = dataBuf;
	SBodyp* handle;
	tmp += sizeof(SHeader);
	int procCnt = 0;

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
			snprintf(path, 262, "/proc/%s", entry->d_name);
			if (access(path, F_OK) != 0)
				continue;
			
			// start parsing
			procCnt++;
			handle = (SBodyp*)tmp;

			sprintf(filePath, "%s/stat", path);
			fd = open(filePath, O_RDONLY);
			if (fd == -1)
			{
				// TODO: handling open error
				perror("agent");
				return NULL;
			}
			pbuf = buf;
			readSize = read(fd, buf, 512);
			if (readSize == -1)
			{
				// TODO: handling read error
				perror("agent");
				return NULL;
			}
			pbuf[readSize] = 0;

			handle->pid = atoi(pbuf);
			while (*pbuf++ != '(');
			for (int j = 0; *pbuf != ')' && j < 16; j++)
				handle->procName[j] = *pbuf++;
			while (*pbuf++ != ')');
			pbuf++;
			handle->state = *pbuf++;
			handle->ppid = atoi(pbuf);

			for (int cnt = 0; cnt < 11; cnt++)
				while (*pbuf++ != ' ');

			handle->utime = atol(pbuf) * 1000;

			while (*pbuf++ != ' ');
			handle->stime = atol(pbuf) * 1000;
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
			readSize = read(fd, buf, 512);
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
			strcpy(handle->userName, pwd->pw_name);
			close(fd);

			sprintf(filePath, "%s/cmdline", path);
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

			handle->cmdlineLen = strlen(buf);
			tmp += sizeof(SBodyp);

			if (readSize > 0)
			{
				buf[readSize] = 0;
				handle->cmdlineLen = strlen(buf);
				strncpy(tmp, buf, handle->cmdlineLen);
				tmp += handle->cmdlineLen;
			}
			close(fd);
		}
	}
	
	// complete collection
	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = tmp - dataBuf;
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
	{
		// TODO: handle malloc error
		return NULL;
	}
	memcpy(result->data, tmp, tmp - dataBuf);
	SHeader* hh = (SHeader*)result->data;
	hh->bodyCount = procCnt;
	hh->bodySize = tmp - dataBuf - sizeof(SHeader);
	hh->collectPeriod = collectPeriod;
	hh->signature = SIGNATURE_PROC;
	hh->collectTime = time(NULL);
	return result;
}