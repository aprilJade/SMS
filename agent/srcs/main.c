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
	char logmsgBuf[128] = { 0, };
	sprintf(logmsgBuf, "./log/agent");
	Logger* logger = NewLogger(logmsgBuf);

	pid_t agentPid = getpid();
	sprintf(logmsgBuf, "agent loaded: %d", agentPid);
	Log(logger, logmsgBuf);

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
		return EXIT_SUCCESS;
	}

	char c;
	int i = 0;
	pthread_t collectorTids[ROUTINE_COUNT] = { 0, };
	pthread_t senderTid;
	SRoutineParam* param[ROUTINE_COUNT] = { 0, };
	SSenderParam senderParam;
	void* (*collector[ROUTINE_COUNT + 1])(void*) = { 0, };
	Queue* queue = NewQueue();
	senderParam.logger = logger;
	senderParam.queue = queue;
	senderParam.port = 0;
	while ((c = getopt_long(argc, argv, "c:m:n:p:H:h", longOptions, 0)) > 0)
	{
		if (c == '?')
		{
			PrintUsage();
			exit(EXIT_FAILURE);
		}
		if (optarg == NULL)
		{
			PrintHelp();
			exit(EXIT_FAILURE);
		}
		switch (c)
		{
		case 'c':
			collector[i] = CpuInfoRoutine;
			param[i] = GenRoutineParam(atoi(optarg), CPU, queue);
			param[i]->logger = logger;
			param[i]->queue = queue;
			sprintf(logmsgBuf, "INFO: Collect every %dms: CPU", param[i]->collectPeriod);
			Log(logger, logmsgBuf);
			break;
		case 'm':
			collector[i] = MemInfoRoutine;
			param[i] = GenRoutineParam(atoi(optarg), MEMORY, queue);
			param[i]->logger = logger;
			param[i]->queue = queue;
			sprintf(logmsgBuf, "INFO: Collect every %dms: Memory", param[i]->collectPeriod);
			Log(logger, logmsgBuf);
			break;
		case 'n':
			collector[i] = NetInfoRoutine;
			param[i] = GenRoutineParam(atoi(optarg), NETWORK, queue);
			param[i]->logger = logger;
			param[i]->queue = queue;
			sprintf(logmsgBuf, "INFO: Collect every %dms: Network", param[i]->collectPeriod);
			Log(logger, logmsgBuf);
			break;
		case 'p':
			collector[i] = ProcInfoRoutine;
			param[i] = GenRoutineParam(atoi(optarg), PROCESS, queue);
			param[i]->logger = logger;
			param[i]->queue = queue;
			sprintf(logmsgBuf, "INFO: Collect every %dms: Process", param[i]->collectPeriod);
			Log(logger, logmsgBuf);
			break;
		case 'H':
			if (ParseHost(optarg, senderParam.host, &senderParam.port))
				exit(EXIT_FAILURE);
			break;
		case 'h':
			PrintHelp();
			exit(EXIT_SUCCESS);
		default:
			abort();
		}
		i++;
	}
	
	signal(SIGPIPE, SIG_IGN);
	for (i = 0; collector[i]; i++)
	{
		if (pthread_create(&collectorTids[i], NULL, collector[i], param[i]) == -1)
		{
			sprintf(logmsgBuf, "ERR: FATAL: Failed to start collector");
			Log(logger, logmsgBuf);
			exit(EXIT_FAILURE);
		}
	}

	if (senderParam.port == 0)
	{
		strcpy(senderParam.host, DEFAULT_HOST);
		senderParam.port = DEFAULT_PORT;
		sprintf(logmsgBuf, "INFO: Set server host by default %s:%d", DEFAULT_HOST, DEFAULT_PORT);
		Log(logger, logmsgBuf);
	}

	if (pthread_create(&senderTid, NULL, SendRoutine, &senderParam) == -1)
	{
		sprintf(logmsgBuf, "ERR: FATAL: Fail to start sender");
		Log(logger, logmsgBuf);
		exit(EXIT_FAILURE);
	}

	for (i = 0; collector[i]; i++)
		pthread_join(collectorTids[i], NULL);
	pthread_join(senderTid, NULL);
		
	exit(EXIT_SUCCESS);
}