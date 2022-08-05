#include <unistd.h>
#include <pthread.h>
#include "collector.h"
#include "collectRoutine.h"
#include <stdio.h>

int main(void)
{
	pthread_t cpuTid, memTid, netTid, procTid;
	// TODO: set routine parameter using argv!
	SRoutineParam param;
	param.collectPeriod = 3000;

	pthread_create(&cpuTid, NULL, &CpuInfoRoutine, (void*)&param);
	pthread_create(&memTid, NULL, &MemInfoRoutine, (void*)&param);
	pthread_create(&netTid, NULL, &NetInfoRoutine, (void*)&param);
	//pthread_create(&procTid, NULL, &ProcInfoRoutine, (void*)&param);
	pthread_join(cpuTid, NULL);
	pthread_join(memTid, NULL);
	pthread_join(netTid, NULL);
	//pthread_join(procTid, NULL);
	return 0; 
}