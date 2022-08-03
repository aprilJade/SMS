#include <unistd.h>
#include <pthread.h>
#include "collector.h"

int main(void)
{
	char sysbuf[BUFFER_SIZE + 1] = { 0, };
	long logicalCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
	long oneTick = sysconf(_SC_CLK_TCK);
	long toMs = 1000 / oneTick;

	while(1)
	{
		collectCpuInfo(toMs, sysbuf);
		//scollectMemInfo(sysbuf);
		//scollectNetInfo(sysbuf);
		//scollectProcInfo(sysbuf, getMaxPid());
		sleep(1);
	}

	pthread_t tid;
	// send basic info
	// logical core count, oneTick, toMs, Total memory, Total swap space, 
}