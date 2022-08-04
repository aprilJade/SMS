#include <unistd.h>
#include <pthread.h>
#include "collector.h"
#include "collectRoutine.h"
#include <stdio.h>

int main(void)
{
	pthread_t cpuTid, memTid;
	// TODO: set routine parameter using argv!
	SRoutineParam param;
	param.collectPeriod = 800;

	pthread_create(&cpuTid, NULL, &cpuInfoRoutine, (void*)&param);
	pthread_create(&memTid, NULL, &memInfoRoutine, (void*)&param);
	pthread_join(cpuTid, NULL);
	pthread_join(memTid, NULL);
	return 0; 
}