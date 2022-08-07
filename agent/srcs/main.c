#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
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
		{"CPU", required_argument, 0, 'C'},
		{"memory", required_argument, 0, 'm'},
		{"network", required_argument, 0, 'n'},
		{"process", required_argument, 0, 'p'},
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
	pthread_t senderTids[ROUTINE_COUNT] = { 0, };
	SRoutineParam* param[ROUTINE_COUNT] = { 0, };
	void* (*collector[ROUTINE_COUNT + 1])(void*) = { 0, };
	Logger* logger = NewLogger(HOST, PORT);
	while ((c = getopt_long(argc, argv, "C:m:n:p:h", longOptions, 0)) > 0)
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
		case 'C':
			collector[i] = CpuInfoRoutine;
			param[i] = GenRoutineParam(atoi(optarg), CPU);
			param[i]->logger = logger;
			break;
		case 'm':
			collector[i] = MemInfoRoutine;
			param[i] = GenRoutineParam(atoi(optarg), MEMORY);
			param[i]->logger = logger;
			break;
		case 'n':
			collector[i] = NetInfoRoutine;
			param[i] = GenRoutineParam(atoi(optarg), NETWORK);
			param[i]->logger = logger;
			break;
		case 'p':
			collector[i] = ProcInfoRoutine;
			param[i] = GenRoutineParam(atoi(optarg), PROCESS);
			param[i]->logger = logger;
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
    struct timeval timeVal;
	ulong collectorCreateTime;
	for (i = 0; collector[i]; i++)
	{
		gettimeofday(&timeVal, NULL);
		param[i]->collectorCreateTime = timeVal.tv_sec * 1000 + timeVal.tv_usec / 1000;
		pthread_create(&collectorTids[i], NULL, collector[i], param[i]);
		pthread_create(&senderTids[i], NULL, SendRoutine, param[i]);
	}

	for (i = 0; collector[i]; i++)
	{
		pthread_join(collectorTids[i], NULL);
		pthread_join(senderTids[i], NULL);
	}
		
	exit(0);
}