#include <unistd.h>
#include "collector.h"

int main(void)
{
	char sysbuf[BUFFER_SIZE + 1] = { 0, };
	long logicalCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
	long oneTick = sysconf(_SC_CLK_TCK);
	long toMs = 1000 / oneTick;

	while(1)
	{
		collectCpuInfo(logicalCoreCount, toMs, sysbuf);
		collectMemInfo(sysbuf);
		collectNetInfo(sysbuf);
		collectProcInfo(sysbuf, getMaxPid());
		sleep(1);
	}
}