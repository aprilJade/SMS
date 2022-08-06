#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include "collectRoutine.h"


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
	void* (*sender[ROUTINE_COUNT + 1])(void*) = { 0, };
	
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
			sender[i] = CpuInfoSendRoutine;
			param[i] = GenRoutineParam(atoi(optarg));
			break;
		case 'm':
			collector[i] = MemInfoRoutine;
			sender[i] = MemInfoSendRoutine;
			param[i] = GenRoutineParam(atoi(optarg));
			break;
		case 'n':
			collector[i] = NetInfoRoutine;
			sender[i] = NetInfoSendRoutine;
			param[i] = GenRoutineParam(atoi(optarg));
			break;
		case 'p':
			collector[i] = ProcInfoRoutine;
			sender[i] = ProcInfoSendRoutine;
			param[i] = GenRoutineParam(atoi(optarg));
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
	
	for (i = 0; collector[i]; i++)
	{
		pthread_create(&collectorTids[i], NULL, collector[i], param[i]);
		pthread_create(&senderTids[i], NULL, sender[i], param[i]);
	}

	for (i = 0; collector[i]; i++)
	{
		pthread_join(collectorTids[i], NULL);
		pthread_join(senderTids[i], NULL);
	}
		
	exit(0);
}