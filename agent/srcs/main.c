#include <unistd.h>
#include <pthread.h>
#include "collector.h"
#include "collectRoutine.h"
#include <stdio.h>

int main(void)
{
	pthread_t tid;
	// TODO: set routine parameter using argv!
	SRoutineParam param;
	param.collectPeriod = 2500;

	pthread_create(&tid, NULL, &cpuInfoRoutine, (void*)&param);
	pthread_join(tid, NULL);
	return 0; 
}