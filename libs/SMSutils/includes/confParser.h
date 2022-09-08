#ifndef CONF_PARSER_H
#define CONF_PARSER_H

#include "simpleHashTable.h"

#define CONF_OPTION_COUNT 14

#define CONF_DEFAULT_ID "debug"
#define CONF_DEFAULT_IP "127.0.0.1"
#define CONF_DEFAULT_PORT 4242
#define CONF_DEAFULT_CPU_PERIOD 3000
#define CONF_DEFAULT_MEM_PREIOD 3000
#define CONF_DEFAULT_NET_PREIOD 3000
#define CONF_DEFAULT_PROC_PREIOD 3000
#define CONF_DEFAULT_DISK_PREIOD 3000
#define CONF_DEFAULT_RUN_DAEMON false

#define CONF_KEY_ID                     "ID"
#define CONF_KEY_HOST_ADDRESS           "HOST_ADDRESS"
#define CONF_KEY_HOST_PORT              "HOST_PORT"
#define CONF_KEY_RUN_AS_DAEMON          "RUN_AS_DAEMON"

#define CONF_KEY_LOG_LEVEL              "LOG_LEVEL"
#define CONF_KEY_LOG_PATH               "LOG_PATH"

#define CONF_KEY_RUN_CPU_COLLECTOR      "RUN_CPU_COLLECTOR"
#define CONF_KEY_CPU_COLLECTION_PERIOD  "CPU_COLLECTION_PERIOD"

#define CONF_KEY_RUN_MEM_COLLECTOR      "RUN_MEM_COLLECTOR"
#define CONF_KEY_MEM_COLLECTION_PERIOD  "MEM_COLLECTION_PERIOD"

#define CONF_KEY_RUN_NET_COLLECTOR      "RUN_NET_COLLECTOR"
#define CONF_KEY_NET_COLLECTION_PERIOD  "NET_COLLECTION_PERIOD"

#define CONF_KEY_RUN_PROC_COLLECTOR     "RUN_PROC_COLLECTOR"
#define CONF_KEY_PROC_COLLECTION_PERIOD "PROC_COLLECTION_PERIOD"

#define CONF_KEY_RUN_DISK_COLLECTOR     "RUN_DISK_COLLECTOR"
#define CONF_KEY_DISK_COLLECTION_PERIOD "DISK_COLLECTION_PERIOD"

#define CONF_KEY_LISTEN_PORT            "LISTEN_PORT"
#define CONF_KEY_MAX_CONNECTION         "MAX_CONN"
#define CONF_KEY_WORKER_COUNT           "WORKER_COUNT"

#define CONF_KEY_CPU_UTILIAZATION_LIMIT "CPU_UTILIZATION_LIMIT"
#define CONF_KEY_NET_THROUGHPUT_LIMIT   "NETWORK_THROUGHPUT_LIMIT"
#define CONF_KEY_MEM_USAGE_LIMIT        "MEMORY_USAGE_LIMIT"
#define CONF_KET_SWAP_USAGE_LIMIT       "SWAP_USAGE_LIMIT"

enum eConfError // If you want see specific error message, use functions handling errno variable such as "perror"
{
    CONF_NO_ERROR = 0,
    CONF_ERROR_INVALID_PATH,
    CONF_ERROR_FAIL_READ,
    CONF_ERROR_INVALID_FORMAT,
    CONF_ERROR_MAKE_PAIR
};

int ParseConf(const char* confPath, SHashTable* table);

#endif