#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
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

uchar* CollectEachCpuInfo(ushort cpuCnt, long timeConversion, char* rdBuf)
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
	uchar* result = (uchar*)malloc(cpuCnt * sizeof(SBodyc) + sizeof(SHeader));
	if (result == NULL)
	{
		// TODO: Handling malloc error
		return NULL;
	}
	// gen header..

	SHeader* hh = (SHeader*)result;
	//memcpy(result, SIGNATURE_CPU, 4);
	hh->signature = SIGNATURE_CPU;
	hh->bodyCount = cpuCnt;
	hh->bodySize = sizeof(SBodyc);

	SBodyc* handle = (SBodyc*)(result + sizeof(SHeader));
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

uchar* CollectMemInfo(char* buf)
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
	
	uchar* result = (uchar*)malloc(sizeof(SHeader) + sizeof(SBodym));
	if (result == NULL)
	{
		// TODO: handling malloc error
		return NULL;
	}

	SHeader* hh = (SHeader*)result;
	//memcpy(result, SIGNATURE_MEM, 4);
	hh->signature = SIGNATURE_MEM;
	hh->bodyCount = 1;
	hh->bodySize = sizeof(SBodym);
	SBodym* handle = (SBodym*)(result + sizeof(SHeader));

	while (*buf++ != ' ');
	ulong memTotal = atol(buf);

	while (*buf++ != '\n');
	while (*buf++ != ' ');
	handle->memFree = atol(buf);
	
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	handle->memAvail = atol(buf);

	while (*buf++ != '\n');
	while (*buf++ != ' ');
	ulong memBuffers = atol(buf);
	
	while (*buf++ != '\n');
	while (*buf++ != ' ');
	ulong memCached = atol(buf);

	handle->memUsed = memTotal - handle->memFree - memBuffers - memCached;
	for (int i = 0; i < 11; i++)
		while (*buf++ != '\n');
	while (*buf++ != ' ');
	handle->swapFree = atol(buf);
	close(fd);
	return result;
}

uchar* CollectNetInfo(char* buf, int nicCount)
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

	uchar* result = (uchar*)malloc(sizeof(SHeader) + sizeof(SBodyn) * nicCount);
	if (result == NULL)
	{
		// TODO: handle malloc error
		return NULL;
	}
	SHeader* hh = (SHeader*)result;
	//memcpy(result, SIGNATURE_NET, 4);
	hh->signature = SIGNATURE_NET;
	hh->bodyCount = nicCount;
	hh->bodySize = sizeof(SBodyn);
	SBodyn* handle = (SBodyn*)(result + sizeof(SHeader));

	while (*buf++ != '\n');
	while (*buf++ != '\n');
	// while (*buf++ != '\n'); // if this code line is run, then couldn't collect loopback data
	for (int i = 0; i < nicCount; i++)
	{
		while (*buf++ != ':');
		handle->recvBytes = atol(buf) / 1024;

		while (*buf++ == ' ');
		while (*buf++ != ' ');
		handle->recvPackets = atol(buf);

		for (int i = 0; i < 7; i++)
		{
			while (*buf++ == ' ');
			while (*buf++ != ' ');
		}
		handle->sendBytes = atol(buf) / 1024;

		while (*buf++ == ' ');
		while (*buf++ != ' ');
		handle->sendPackets = atol(buf);
		handle++;
	}
	return result;
}

uchar* CollectProcInfo(char *buf, uchar* dataBuf)
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
	uchar* result = (uchar*)malloc(tmp - dataBuf);
	if (result == NULL)
	{
		// TODO: handle malloc error
		return NULL;
	}
	memcpy(result, tmp, tmp - dataBuf);
	SHeader* hh = (SHeader*)result;
	hh->bodyCount = 0;
	hh->bodySize = tmp - dataBuf;
	//memcpy(result, SIGNATURE_PROC, 4);
	hh->signature = SIGNATURE_PROC;
	return result;
}
