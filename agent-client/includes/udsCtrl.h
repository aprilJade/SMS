#ifndef UDS_CTRL_H
#define UDS_CTRL_H

#include "confParser.h"

// CHECK: modify below path when deploy SMS
#define UDS_AGENT_PATH "/home/apriljade/repo/SMS/bin/.agent.sock"
#define UDS_CLIENT_PATH "/home/apriljade/repo/SMS/bin/.client.sock"

static const char* const runKeys[] = {
	CONF_KEY_RUN_CPU_COLLECTOR,
	CONF_KEY_RUN_MEM_COLLECTOR,
	CONF_KEY_RUN_NET_COLLECTOR,
	CONF_KEY_RUN_PROC_COLLECTOR,
	CONF_KEY_RUN_DISK_COLLECTOR
};

static const char* const periodKeys[] = {
	CONF_KEY_CPU_COLLECTION_PERIOD,
	CONF_KEY_MEM_COLLECTION_PERIOD,
	CONF_KEY_NET_COLLECTION_PERIOD,
	CONF_KEY_PROC_COLLECTION_PERIOD,
	CONF_KEY_DISK_COLLECTION_PERIOD
};


void RequestStatusToAgent(void);
void RequestUpdateConfigToAgent(const char* confPath);

#endif