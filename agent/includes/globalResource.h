#ifndef AGENT_UTILS_H
#define AGENT_UTILS_H
#include <pthread.h>
#include "confParser.h"
#include "routines.h"
#include "collector.h"

#define COLLECTOR_COUNT 5

// A collection of variables used across the agent
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
	SHashTable* configurations;
	bool turnOff;
	bool bIsConnected;
	ulong loadTime;
} SGlobResource;


#endif