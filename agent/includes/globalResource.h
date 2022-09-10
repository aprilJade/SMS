#ifndef AGENT_UTILS_H
#define AGENT_UTILS_H
#include <pthread.h>
#include "confParser.h"
#include "routines.h"
#include "collector.h"
#include "confUpdater.h"

#define COLLECTOR_COUNT 5
typedef struct SGlobResource
{
	Logger* logger;
	Queue* queue;
	char peerIP[16];
	ushort peerPort;
	char agentID[16];
	int stderrFd;
	pthread_t collectors[COLLECTOR_COUNT];
	ulong collectPeriods[COLLECTOR_COUNT];
	bool collectorSwitch[COLLECTOR_COUNT];
	SHashTable* configurations;
	bool turnOff;
} SGlobResource;


#endif