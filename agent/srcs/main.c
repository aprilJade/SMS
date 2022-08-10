#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "routines.h"
#include "collector.h"

void PrintHelp()
{
	printf("PrintHelp(): Not implemented Yet\n");
}

void PrintUsage()
{
	fprintf(stderr, "Try \"./agent -h\" or \"./agent --help\" for more information.\n");
}

int main(int argc, char** argv)
{
	static struct option longOptions[] =
	{
		{"CPU", required_argument, 0, 'c'},
		{"memory", required_argument, 0, 'm'},
		{"network", required_argument, 0, 'n'},
		{"process", required_argument, 0, 'p'},
		{"host", required_argument, 0, 'H'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0} 
	};

	if (argc == 1)
	{
		PrintUsage();
		return 1;
	}

	char c;
	int i = 0;
	pthread_t collectorTids[ROUTINE_COUNT] = { 0, };
	pthread_t senderTid;
	SRoutineParam* param[ROUTINE_COUNT] = { 0, };
	SSenderParam senderParam;
	void* (*collector[ROUTINE_COUNT + 1])(void*) = { 0, };
	char logmsgBuf[128];
	sprintf(logmsgBuf, "./log/agent");
	Logger* logger = NewLogger(logmsgBuf);
	Queue* queue = NewQueue();
	senderParam.logger = logger;
	senderParam.queue = queue;
	senderParam.port = 0;
	while ((c = getopt_long(argc, argv, "c:m:n:p:H:h", longOptions, 0)) > 0)
	{
		if (c == '?')
		{
			PrintUsage();
			exit(1);
		}
		if (optarg == NULL)
		{
			PrintHelp();
			exit(1);
		}
		switch (c)
		{
		case 'c':
			collector[i] = CpuInfoRoutine;
			senderParam.cpuPeriod = atoi(optarg);
			param[i] = GenRoutineParam(atoi(optarg), CPU, queue);
			param[i]->logger = logger;
			param[i]->queue = queue;
			sprintf(logmsgBuf, "c collect every %dms", senderParam.cpuPeriod);
			Log(logger, logmsgBuf);
			break;
		case 'm':
			collector[i] = MemInfoRoutine;
			senderParam.memPeriod = atoi(optarg);
			param[i] = GenRoutineParam(atoi(optarg), MEMORY, queue);
			param[i]->logger = logger;
			param[i]->queue = queue;
			sprintf(logmsgBuf, "m collect every %dms", senderParam.cpuPeriod);
			Log(logger, logmsgBuf);
			break;
		case 'n':
			collector[i] = NetInfoRoutine;
			senderParam.netPeriod = atoi(optarg);
			param[i] = GenRoutineParam(atoi(optarg), NETWORK, queue);
			param[i]->logger = logger;
			param[i]->queue = queue;
			sprintf(logmsgBuf, "n collect every %dms", senderParam.cpuPeriod);
			Log(logger, logmsgBuf);
			break;
		case 'p':
			collector[i] = ProcInfoRoutine;
			senderParam.procPeriod = atoi(optarg);
			param[i] = GenRoutineParam(atoi(optarg), PROCESS, queue);
			param[i]->logger = logger;
			param[i]->queue = queue;
			sprintf(logmsgBuf, "p collect every %dms", senderParam.cpuPeriod);
			Log(logger, logmsgBuf);
			break;
		case 'H':
			if (ParseHost(optarg, senderParam.host, &senderParam.port))
				exit(1);
			break;
		case 'h':
			PrintHelp();
			exit(0);
		default:
			abort();
		}
		i++;
	}
	
	signal(SIGPIPE, SIG_IGN);
    // struct timeval timeVal;
	// ulong collectorCreateTime;
	for (i = 0; collector[i]; i++)
	{
		// gettimeofday(&timeVal, NULL);
		// param[i]->collectorCreateTime = timeVal.tv_sec * 1000 + timeVal.tv_usec / 1000;
		pthread_create(&collectorTids[i], NULL, collector[i], param[i]);
	}

	if (senderParam.port == 0)
	{
		strcpy(senderParam.host, DEFAULT_HOST);
		senderParam.port = DEFAULT_PORT;
	}
	pthread_create(&senderTid, NULL, SendRoutine, &senderParam);

	for (i = 0; collector[i]; i++)
		pthread_join(collectorTids[i], NULL);
	pthread_join(senderTid, NULL);
		
	exit(0);
}