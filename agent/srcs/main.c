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
	param.collectPeriod = 1800;

	pthread_create(&cpuTid, NULL, &cpuInfoRoutine, (void*)&param);
	pthread_create(&memTid, NULL, &memInfoRoutine, (void*)&param);
	pthread_create(&netTid, NULL, &netInfoRoutine, (void*)&param);
	//pthread_create(&procTid, NULL, &procInfoRoutine, (void*)&param);
	pthread_join(cpuTid, NULL);
	pthread_join(memTid, NULL);
	pthread_join(netTid, NULL);
	//pthread_join(procTid, NULL);
	return 0; 
}