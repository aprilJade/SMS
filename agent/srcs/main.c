#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
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
	int tmp;
	pthread_t tids[ROUTINE_COUNT] = { 0, };
	SRoutineParam* param[ROUTINE_COUNT] = { 0, };
	void* (*routine[ROUTINE_COUNT + 1])(void*) = { 0, };
	
	while ((c = getopt_long(argc, argv, "C:m:n:p:h", longOptions, 0)) > 0)
	{
		if (c == '?')
		{
			PrintUsage();
			exit(1);
		}
		if ((tmp = atoi(optarg)) == 0)
		{
			PrintHelp();
			exit(1);
		}
		switch (c)
		{
		case 'C':
			routine[i] = CpuInfoRoutine;
			param[i] = GenRoutineParam(tmp);
			break;
		case 'm':
			routine[i] = MemInfoRoutine;
			param[i] = GenRoutineParam(tmp);
			break;
		case 'n':
			routine[i] = NetInfoRoutine;
			param[i] = GenRoutineParam(tmp);
			break;
		case 'p':
			routine[i] = ProcInfoRoutine;
			param[i] = GenRoutineParam(tmp);
			break;
		case 'h':
			PrintHelp();
			exit(0);
		default:
			abort();
		}
		i++;
	}
	printf("over parse.\n");
	for (i = 0; routine[i] != 0; i++)
		pthread_create(&tids[i], NULL, routine[i], param[i]);

	for (i = 0; routine[i]; i++)
		pthread_join(tids[i], NULL);
		
	//printf("Create thread collect CPU information every %d ms.\n", param[i]->collectPeriod);
	//printf("Create thread collect memory information every %d ms.\n", param[i]->collectPeriod);
	//printf("Create thread collect network information every %d ms.\n", param[i]->collectPeriod);
	//printf("Create thread collect all process information every %d ms.\n", param[i]->collectPeriod);

	return 0; 
}