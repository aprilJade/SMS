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
#include <sys/vfs.h>
#include <fstab.h>
#include "collector.h"
#include "packets.h"

static float RoundingOff(float x)
{
    x += 0.005;
    int tmp = (int)(x * 100);
    x = tmp / 100.0;
    return x;
}

SCData* CollectEachCpuInfo(ushort cpuCnt, long timeConversion, char* rdBuf, int collectPeriod, char* agentId)
{
	int	fd;
	int readSize = 0;

	fd = open("/proc/stat", O_RDONLY);
	if (fd == -1)
		return NULL;

	readSize = read(fd, rdBuf, BUFFER_SIZE);
	if (readSize == -1)
		return NULL;
	rdBuf[readSize] = 0;
	close(fd);


	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = cpuCnt * sizeof(SBodyc) + sizeof(SHeader);
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
		return NULL;

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
		handle->usrCpuRunTime = atol(rdBuf);

		while (*rdBuf++ != ' ');
		while (*rdBuf++ != ' ');
		handle->sysCpuRunTime = atol(rdBuf);

		while (*rdBuf++ != ' ');
		handle->idleTime = atol(rdBuf);

		while (*rdBuf++ != ' ');
		handle->waitTime = atol(rdBuf);
		
		while (*rdBuf++ != '\n');
		handle++;
	}
	return result;
}

SCData* CollectMemInfo(char* buf, int collectPeriod, char* agentId)
{
	int fd = open("/proc/meminfo", O_RDONLY);
	int readSize;

	if (fd == -1)
		return NULL;

	readSize = read(fd, buf, BUFFER_SIZE);
	if (readSize == -1)
		return NULL;
	buf[readSize] = 0;
	close(fd);
	
	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = sizeof(SHeader) + sizeof(SBodym);
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
		return NULL;

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

	return result;
}

SCData* CollectNetInfo(char* buf, int nicCount, int collectPeriod, char* agentId)
{
	int fd = open("/proc/net/dev", O_RDONLY);
	if (fd == -1)
		return NULL;

	int readSize = read(fd, buf, BUFFER_SIZE);
	if (readSize == -1)
		return NULL;
	buf[readSize] = 0;
	close(fd);

	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = sizeof(SHeader) + sizeof(SBodyn) * nicCount;
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
		return NULL;

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
		return NULL;
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
				return NULL;
			pbuf = buf;
			readSize = read(fd, buf, BUFFER_SIZE);

			if (readSize == -1)
				return NULL;
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
				return NULL;

			readSize = read(fd, buf, BUFFER_SIZE);
			if (readSize == -1)
				return NULL;
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

			snprintf(filePath, 272, "%s/cmdline", path);
			if ((fd = open(filePath, O_RDONLY)) == -1)
				return NULL;

			if ((readSize = read(fd, buf, BUFFER_SIZE)) == -1)
				return NULL;
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

SCData* CollectDiskInfo(char *buf, int collectPeriod, char* agentId)
{
	struct statfs fs;
	struct fstab* fsent;
	int fsCnt = 0;

	SBodyd* hBody = (SBodyd*)buf;

	while (fsent = getfsent())
	{
		strcpy(hBody->mountPoint, fsent->fs_file);
		strcpy(hBody->fsType, fsent->fs_vfstype);
		statfs(fsent->fs_file, &fs);
		hBody->freeSizeGB = (float)(fs.f_bfree * fs.f_bsize) / 1024.0 / 1024.0 / 1024.0;
		hBody->totalSizeGB = (float)(fs.f_blocks * fs.f_bsize) / 1024.0 / 1024.0 / 1024.0;
		hBody->diskUsage = 100 - hBody->freeSizeGB / hBody->totalSizeGB * 100.0;
		hBody++;
		fsCnt++;
	}
	endfsent();

	SCData* result = (SCData*)malloc(sizeof(SCData));
	result->dataSize = fsCnt * sizeof(SBodyd) + sizeof(SHeader);
	result->data = (uchar*)malloc(result->dataSize);
	if (result == NULL)
		return NULL;

	SHeader* hh = (SHeader*)result->data;
	hh->signature = SIGNATURE_DISK;
	hh->collectPeriod = collectPeriod;
	hh->bodyCount = fsCnt;
	hh->bodySize = sizeof(SBodyd);
	hh->collectTime = time(NULL);
	memset(hh->agentId, 0, 16);
	strncpy(hh->agentId, agentId, 15);
	hh->agentId[15] = 0;

	memcpy(result->data + sizeof(SHeader), buf, result->dataSize - sizeof(SHeader));
	
	return result;
}

SCData* CalcCpuUtilizationAvg(uchar* collectedData, int cpuCnt, int maxCount, float toMs, int collectPeriod)
{
    static ulong* curIdle;
    static ulong* prevIdle;
    static float* deltaIdle;
    static float** cpuUtilizations;
    static int curCount;
    static int idx;
    static bool initialized;

    if (initialized == false)
    {
        curIdle = (ulong*)malloc(sizeof(ulong) * cpuCnt);
        prevIdle = (ulong*)malloc(sizeof(ulong) * cpuCnt);
        deltaIdle = (float*)malloc(sizeof(float) * cpuCnt);
        cpuUtilizations = (float**)malloc(sizeof(float*) * cpuCnt);
        for (int i = 0; i < cpuCnt; i++)
        {
            cpuUtilizations[i] = (float*)malloc(sizeof(float) * maxCount);
            memset(cpuUtilizations[i], 0, sizeof(float) * maxCount);
        }
        initialized = true;
    }
    
    SCData* result = (SCData*)malloc(sizeof(SCData));
    result->dataSize = sizeof(SHeader) + sizeof(SBodyAvgC) * cpuCnt;
    result->data = (uchar*)malloc(result->dataSize);
    memcpy(result->data, collectedData, sizeof(SHeader));
    SHeader* hHeader = (SHeader*)result->data;
    hHeader->signature = SIGNATURE_AVG_CPU;
    hHeader->bodySize = sizeof(SBodyAvgC);

    SBodyc* hBody = (SBodyc*)(collectedData + sizeof(SHeader));
    SBodyAvgC* hAvgBody = (SBodyAvgC*)(result->data + sizeof(SHeader));
    float avg;
    for (int i = 0; i < cpuCnt; i++)
    {
        curIdle[i] = hBody[i].idleTime;
        deltaIdle[i] = (float)(curIdle[i] - prevIdle[i]) * 10;
        prevIdle[i] = curIdle[i];
        cpuUtilizations[i][idx] = RoundingOff(100.0 - ((deltaIdle[i]) / (float)collectPeriod * 100.0));
        avg = 0.0;
        for (int j = 0; j < curCount; j++)
            avg += cpuUtilizations[i][j];
        hAvgBody[i].cpuUtilizationAvg = RoundingOff(avg / (float)curCount);
        hAvgBody[i].cpuUtilization = cpuUtilizations[i][idx];
    }
    if (curCount)
    {
        idx++;
        idx %= maxCount;
    }
    if (curCount < maxCount)
        curCount++;
    
    return result;
}

SCData* CalcMemAvg(uchar* collectedData, int maxCount)
{
    static int curCount;
    static int idx;
    static bool initialized;
    static float* memUsage;
    static float* swapUsage;

    if (initialized == false)
    {
        memUsage = (float*)malloc(sizeof(float) * maxCount);
        swapUsage = (float*)malloc(sizeof(float) * maxCount);
        initialized = true;
    }

    SCData* avgData = (SCData*)malloc(sizeof(SCData));
    avgData->dataSize = sizeof(SHeader) + sizeof(SBodyAvgM);
    avgData->data = (uchar*)malloc(avgData->dataSize);
    
    memcpy(avgData->data, collectedData, sizeof(SHeader));
    SHeader* hHeader = (SHeader*)avgData->data;
    hHeader->signature = SIGNATURE_AVG_MEM;
    hHeader->bodySize = sizeof(SBodyAvgM);

    SBodym* hBody = (SBodym*)(collectedData + sizeof(SHeader));
    SBodyAvgM* hAvgBody = (SBodyAvgM*)(avgData->data + sizeof(SHeader));
    
    memUsage[idx] = RoundingOff((float)(hBody->memTotal - hBody->memAvail) / (float)hBody->memTotal * 100);
    swapUsage[idx] = RoundingOff((float)(hBody->swapTotal - hBody->swapFree) / (float)hBody->swapTotal * 100);
    hAvgBody->memUsage = memUsage[idx];
    hAvgBody->swapUsage = swapUsage[idx];

    float memAvg = 0.0;
    float swapAvg = 0.0;
    for (int i = 0; i < curCount; i++)
    {
        memAvg += memUsage[i];
        swapAvg += swapUsage[i];
    }
    hAvgBody->memUsageAvg = RoundingOff(memAvg / (float)curCount);
    hAvgBody->swapUsageAvg = RoundingOff(swapAvg / (float)curCount);

    if (curCount)
    {
        idx++;
        idx %= maxCount;
    }
    if (curCount < maxCount)
        curCount++;
    
    return avgData;
}

SCData* CalcNetThroughputAvg(uchar* collectedData, int nicCount, int maxCount, int collectPeriod)
{
    static int idx;
    static int curCount;
    static bool initialized;
    static ulong* prevVal[4];
    static ulong* curVal[4];
    static float** deltaVal[4];

    if (initialized == false)
    {
        for (int i = 0; i < 4; i++)
        {
            prevVal[i] = (ulong*)malloc(sizeof(ulong) * nicCount);
            curVal[i] = (ulong*)malloc(sizeof(ulong) * nicCount);
            deltaVal[i] = (float**)malloc(sizeof(float*) * nicCount);
            for (int j = 0; j < nicCount; j++)
            {
                deltaVal[i][j] = (float*)malloc(sizeof(float) * maxCount);
                memset(deltaVal[i][j], 0, sizeof(float) * maxCount);
            }

        }
        initialized = true;
    }
    
    SBodyn* hBody = (SBodyn*)(collectedData + sizeof(SHeader));

    SCData* avgData = (SCData*)malloc(sizeof(SCData));
    avgData->dataSize = sizeof(SHeader) + sizeof(SBodyAvgN) * nicCount;
    avgData->data = (uchar*)malloc(avgData->dataSize);
    memcpy(avgData->data, collectedData, sizeof(SHeader));

    SHeader* hHeader = (SHeader*)avgData->data;
    hHeader->signature = SIGNATURE_AVG_NET;
    hHeader->bodySize = sizeof(SBodyAvgN);

    SBodyAvgN* hAvgBody = (SBodyAvgN*)(avgData->data + sizeof(SHeader));
    float sum = 0.0;
    for (int i = 0; i < nicCount; i++)
    {
        hAvgBody[i].nameLength = hBody[i].nameLength;
        memcpy(hAvgBody[i].name, hBody[i].name, hBody[i].nameLength);

        curVal[RECV_BYTES][i] = hBody[i].recvBytes;
        deltaVal[RECV_BYTES][i][idx] = RoundingOff((float)(curVal[RECV_BYTES][i] - prevVal[RECV_BYTES][i]) / (float)collectPeriod * 1000.0);
        prevVal[RECV_BYTES][i] = curVal[RECV_BYTES][i];
        hAvgBody[i].recvBytesPerSec = deltaVal[RECV_BYTES][i][idx];

        sum = 0.0;
        for (int j = 0; j < curCount; j++)
            sum += deltaVal[RECV_BYTES][i][j];
        hAvgBody[i].recvBytesPerSecAvg = sum / curCount;

        curVal[RECV_PACKETS][i] = hBody[i].recvPackets;
        deltaVal[RECV_PACKETS][i][idx] = RoundingOff((float)(curVal[RECV_PACKETS][i] - prevVal[RECV_PACKETS][i]) / (float)collectPeriod * 1000.0);
        prevVal[RECV_PACKETS][i] = curVal[RECV_PACKETS][i];
        hAvgBody[i].recvPacketsPerSec = deltaVal[RECV_PACKETS][i][idx];

        sum = 0.0;
        for (int j = 0; j < curCount; j++)
            sum += deltaVal[RECV_PACKETS][i][j];
        hAvgBody[i].recvPacketsPerSecAvg = sum / curCount;

        curVal[SEND_BYTES][i] = hBody[i].sendBytes;
        deltaVal[SEND_BYTES][i][idx] = RoundingOff((float)(curVal[SEND_BYTES][i] - prevVal[SEND_BYTES][i]) / (float)collectPeriod * 1000.0);
        prevVal[SEND_BYTES][i] = curVal[SEND_BYTES][i];
        hAvgBody[i].sendBytesPerSec = deltaVal[SEND_BYTES][i][idx];

        sum = 0.0;
        for (int j = 0; j < curCount; j++)
            sum += deltaVal[RECV_BYTES][i][j];
        hAvgBody[i].sendBytesPerSecAvg = sum / curCount;

        curVal[SEND_PACKETS][i] = hBody[i].sendPackets;
        deltaVal[SEND_PACKETS][i][idx] = RoundingOff((float)(curVal[SEND_PACKETS][i] - prevVal[SEND_PACKETS][i]) / (float)collectPeriod * 1000.0);
        prevVal[SEND_PACKETS][i] = curVal[SEND_PACKETS][i];
        hAvgBody[i].sendPacketsPerSec = deltaVal[SEND_PACKETS][i][idx];

        sum = 0.0;
        for (int j = 0; j < curCount; j++)
            sum += deltaVal[SEND_PACKETS][i][j];
        hAvgBody[i].sendPacketsPerSecAvg = sum / curCount;
    }
    
    if (curCount)
    {
        idx++;
        idx %= maxCount;
    }

    if (curCount < maxCount)
        curCount++;
    
    return avgData;
}

void DestorySCData(SCData** target)
{
	free((*target)->data);
	free(*target);
	*target = NULL;
}