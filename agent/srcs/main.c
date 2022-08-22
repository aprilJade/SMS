#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "routines.h"
#include "collector.h"
#include "confParser.h"

void HandleSignal(int );
pthread_t RunCollector(void* (*)(void*), const char*, const char*, Logger*, Queue*, SHashTable*);
Logger* GenLogger(SHashTable*);

/****************************** Start Main ******************************/
int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "ERROR: you must input conf file path\n");
		return EXIT_FAILURE;
	}
	SHashTable* options = NewHashTable();
	if (ParseConf(argv[1], options) != CONF_NO_ERROR)
	{
		// TODO: handle error
		fprintf(stderr, "conf error\n");
		exit(1);
	}
	
	char logmsgBuf[128] = { 0, };
	Logger* logger = GenLogger(options);
	Queue* queue = NewQueue();

	pthread_t cpuTid = RunCollector(CpuInfoRoutine, CONF_KEY_RUN_CPU_COLLECTOR, CONF_KEY_CPU_COLLECTION_PERIOD, logger, queue, options);
	pthread_t memTid = RunCollector(MemInfoRoutine, CONF_KEY_RUN_MEM_COLLECTOR, CONF_KEY_MEM_COLLECTION_PERIOD, logger, queue, options);
	pthread_t netTid = RunCollector(NetInfoRoutine, CONF_KEY_RUN_NET_COLLECTOR, CONF_KEY_NET_COLLECTION_PERIOD, logger, queue, options);
	pthread_t procTid = RunCollector(ProcInfoRoutine, CONF_KEY_RUN_PROC_COLLECTOR, CONF_KEY_PROC_COLLECTION_PERIOD, logger, queue, options);
	pthread_t diskTid = RunCollector(DiskInfoRoutine, CONF_KEY_RUN_DISK_COLLECTOR, CONF_KEY_DISK_COLLECTION_PERIOD, logger, queue, options);
	
	signal(SIGPIPE, SIG_IGN);
	sprintf(logmsgBuf, "Ignored SIGPIPE");
	Log(logger, LOG_INFO, logmsgBuf);
	
	signal(SIGHUP, SIG_IGN);	// for deamon...
	// handle below signal
	//signal(SIGBUS, SIG_IGN);	// bus error
	//signal(SIGABRT, SIG_IGN);	// abort signal
	//signal(SIGFPE, SIG_IGN);	// floating point error
	//signal(SIGQUIT, SIG_IGN);	// quit signal
	//signal(SIGSEGV, SIG_IGN); // segmentation fault
	signal(SIGINT, HandleSignal);


	char* value;
	pthread_t senderTid;
	SSenderParam senderParam;
	senderParam.logger = logger;
	senderParam.queue = queue;

	if ((value = GetValueByKey(CONF_KEY_HOST_ADDRESS, options)) != NULL)
		strcpy(senderParam.host, value);
	else
		strcpy(senderParam.host, "127.0.0.1");
	value = GetValueByKey(CONF_KEY_HOST_PORT, options);
	senderParam.port = value != NULL ? atoi(value) : 4242;

	if (pthread_create(&senderTid, NULL, SendRoutine, &senderParam) == -1)
	{
		sprintf(logmsgBuf, "Fail to start sender");
		Log(logger, LOG_FATAL, logmsgBuf);
		exit(EXIT_FAILURE);
	}

	pthread_join(senderTid, NULL);
		
	exit(EXIT_SUCCESS);
}
/****************************** End Main ******************************/

void HandleSignal(int signo)
{
	switch (signo)
	{
	case SIGINT:
		printf("killed by SIGINT\n");
		break;
	case SIGABRT:
		printf("killed by SIGIABRT\n");
		break;
	case SIGSEGV:
		printf("killed by SIGSEGV\n");
		break;
	}
	exit(signo);
}

pthread_t RunCollector(void* (*collectRoutine)(void*), const char* keyRunOrNot,
						const char* keyPeriod, Logger* logger , Queue* queue,
						SHashTable* options)
{
	char* tmp;
	char logmsgBuf[128];
	pthread_t tid = -1;
	SRoutineParam* param = (SRoutineParam*)malloc(sizeof(SRoutineParam));
	param->logger = logger;
	param->queue = queue;

	if ((tmp = GetValueByKey(keyRunOrNot, options)) != NULL)
	{
		if (strcmp(tmp, "true") == 0)
		{
			if ((tmp = GetValueByKey(keyPeriod, options)) != NULL)
				param->collectPeriod = atoi(tmp);
			else
				param->collectPeriod = 1000;
					
			if (param->collectPeriod < MIN_SLEEP_MS)
				param->collectPeriod = MIN_SLEEP_MS;
			if (pthread_create(&tid, NULL, collectRoutine, param) == -1)
			{
				sprintf(logmsgBuf, "Failed to start collector");
				Log(param->logger, LOG_FATAL, logmsgBuf);
				exit(EXIT_FAILURE);
			}
		}
	}
	return tid;
}

Logger* GenLogger(SHashTable* options)
{
	char* logPath;
	char* logLevel;
	Logger* logger;
	if ((logPath = GetValueByKey(CONF_KEY_LOG_PATH, options)) == NULL)
		logPath = "./agent";
	if ((logLevel = GetValueByKey(CONF_KEY_LOG_LEVEL, options)) != NULL)
	{
		if (strcmp(logLevel, "default") == 0)
			logger = NewLogger(logPath, LOG_INFO);
		else if (strcmp(logLevel, "debug") == 0)	// doesn't necessary..
			logger = NewLogger(logPath, LOG_DEBUG);
		return logger;
	}
	logger = NewLogger(logPath, LOG_DEBUG);
	return logger;
}