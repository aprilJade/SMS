#include <unistd.h>
#include <pthread.h>
#include "collector.h"
#include "collectRoutine.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#define MIN_SLEEP_MS 1000

void PrintHelp()
{
	printf("Not implemented Yet\n");
}

int main(int argc, char** argv)
{
	pthread_t cpuTid = 0, memTid = 0, netTid = 0, procTid = 0;
	static struct option longOptions[] =
	{
		{"CPU", required_argument, 0, 'C'},
		{"memory", required_argument, 0, 'm'},
		{"network", required_argument, 0, 'n'},
		{"process", required_argument, 0, 'p'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0} 
	};
	char c;
	SRoutineParam param;
	param.collectPeriod = 0;
	
	while ((c = getopt_long(argc, argv, "C:m:n:p:", longOptions, 0)) > 0)
	{
		if (c == 'h')
		{
			PrintHelp();
			return 0;
		}

		if ((param.collectPeriod = atoi(optarg)) < MIN_SLEEP_MS)
		{
			// TODO: handling error
			fputs("Option value is must be more than 1000\n", stderr);
			return 1;
		}

		switch (c)
		{
		case 'C':
			printf("Create thread collect CPU information every %d ms.\n", param.collectPeriod);
			pthread_create(&cpuTid, NULL, &CpuInfoRoutine, (void*)&param);
			continue;
		case 'm':
			printf("Create thread collect memory information every %d ms.\n", param.collectPeriod);
			pthread_create(&memTid, NULL, &MemInfoRoutine, (void*)&param);
			continue;
		case 'n':
			printf("Create thread collect network information every %d ms.\n", param.collectPeriod);
			pthread_create(&netTid, NULL, &NetInfoRoutine, (void*)&param);
			continue;
		case 'p':
			printf("Create thread collect all process information every %d ms.\n", param.collectPeriod);
			pthread_create(&procTid, NULL, &ProcInfoRoutine, (void*)&param);
			continue;
		default:
			printf("Error: undefined optargs: %c\n", c);
			PrintHelp();
			exit(1);
		}
	}
	if (cpuTid != 0)
		pthread_join(cpuTid, NULL);
	if (memTid != 0)
		pthread_join(memTid, NULL);
	if (netTid != 0)
		pthread_join(netTid, NULL);
	if (procTid != 0)
		pthread_join(procTid, NULL);
	return 0; 
}