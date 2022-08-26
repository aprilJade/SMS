#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "routines.h"
#include "collector.h"
#include "confParser.h"

const Logger* g_logger;
int g_stderrFd;

void HandleSignal(int);
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
		fprintf(stderr, "ERROR: conf error\n");
		exit(EXIT_FAILURE);
	}
	char* value;
	char logmsgBuf[128] = { 0, };
	g_logger = GenLogger(options);
	
	if ((value = GetValueByKey(CONF_KEY_RUN_AS_DAEMON, options)) != NULL)
	{
		if (strcmp(value, "true") == 0)
		{
			pid_t pid = fork();

			if (pid == -1)
			{
				// TODO: handle error
				fprintf(stderr, "ERROR: failed to fork: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			if (pid != 0)
				exit(EXIT_SUCCESS);
			
			signal(SIGHUP, SIG_IGN);
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			g_stderrFd = dup(STDERR_FILENO);
			close(STDERR_FILENO);
			chdir("/");
			setsid();
		}
		
	}

	
	Queue* queue = NewQueue();

	pthread_t cpuTid = RunCollector(CpuInfoRoutine, CONF_KEY_RUN_CPU_COLLECTOR, CONF_KEY_CPU_COLLECTION_PERIOD, g_logger, queue, options);
	pthread_t memTid = RunCollector(MemInfoRoutine, CONF_KEY_RUN_MEM_COLLECTOR, CONF_KEY_MEM_COLLECTION_PERIOD, g_logger, queue, options);
	pthread_t netTid = RunCollector(NetInfoRoutine, CONF_KEY_RUN_NET_COLLECTOR, CONF_KEY_NET_COLLECTION_PERIOD, g_logger, queue, options);
	pthread_t procTid = RunCollector(ProcInfoRoutine, CONF_KEY_RUN_PROC_COLLECTOR, CONF_KEY_PROC_COLLECTION_PERIOD, g_logger, queue, options);
	pthread_t diskTid = RunCollector(DiskInfoRoutine, CONF_KEY_RUN_DISK_COLLECTOR, CONF_KEY_DISK_COLLECTION_PERIOD, g_logger, queue, options);
	
	signal(SIGPIPE, SIG_IGN);
	Log(g_logger, LOG_INFO, "Ignored SIGPIPE");
	
	// handle below signal
	signal(SIGBUS, HandleSignal);	// bus error
	signal(SIGABRT, HandleSignal);	// abort signal
	signal(SIGFPE, HandleSignal);	// floating point error
	signal(SIGQUIT, HandleSignal);	// quit signal
	signal(SIGSEGV, HandleSignal);  // segmentation fault
	signal(SIGINT, HandleSignal);	// interrupted
	signal(SIGILL, HandleSignal);	// illegal instruction
	signal(SIGSYS, HandleSignal);	// system call error
	signal(SIGTERM, HandleSignal);	// terminate signalr
	signal(SIGKILL, HandleSignal);	// terminate signal


	pthread_t senderTid;
	SSenderParam senderParam;
	senderParam.logger = g_logger;
	senderParam.queue = queue;

	if ((value = GetValueByKey(CONF_KEY_HOST_ADDRESS, options)) != NULL)
		strcpy(senderParam.host, value);
	else
		strcpy(senderParam.host, "127.0.0.1");
	value = GetValueByKey(CONF_KEY_HOST_PORT, options);
	senderParam.port = value != NULL ? atoi(value) : 4242;

	// why did you make sender thread...?
	if (pthread_create(&senderTid, NULL, SendRoutine, &senderParam) == -1)
	{
		sprintf(logmsgBuf, "Fail to start sender");
		Log(g_logger, LOG_FATAL, logmsgBuf);
		exit(EXIT_FAILURE);
	}

	pthread_join(senderTid, NULL);
		
	exit(EXIT_SUCCESS);
}
/****************************** End Main ******************************/

void HandleSignal(int signo)
{
	char logMsg[512];
	switch (signo)
	{
	case SIGINT:
		sprintf(logMsg, "Killed by SIGINT");
		Log(g_logger, LOG_ERROR, logMsg);
		break;
	case SIGABRT:
		sprintf(logMsg, "Killed by SIGIABRT");
		Log(g_logger, LOG_FATAL, logMsg);
		break;
	case SIGSEGV:
		sprintf(logMsg, "Killed by SIGSEGV");
		Log(g_logger, LOG_FATAL, logMsg);
		break;
	case SIGBUS:
		sprintf(logMsg, "Killed by SIGBUS");
		Log(g_logger, LOG_FATAL, logMsg);
		break;
	case SIGFPE:
		sprintf(logMsg, "Killed by SIGFPE");
		Log(g_logger, LOG_FATAL, logMsg);
		break;
	case SIGTERM:
		sprintf(logMsg, "Killed by SIGTERM");
		Log(g_logger, LOG_FATAL, logMsg);
		break;
	case SIGSYS:
		sprintf(logMsg, "Killed by SIGSYS");
		Log(g_logger, LOG_FATAL, logMsg);
		break;
	case SIGILL:
		sprintf(logMsg, "Killed by SIGILL");
		Log(g_logger, LOG_FATAL, logMsg);
		break;
	case SIGQUIT:
		sprintf(logMsg, "Killed by SIGQUIT");
		Log(g_logger, LOG_INFO, logMsg);
		break;
	case SIGKILL:
		sprintf(logMsg, "Killed by user");
		Log(g_logger, LOG_INFO, logMsg);
		break;
	}

	if (signo != SIGQUIT && signo != SIGKILL)
	{
		// TODO: handle log path in agent.conf is absolute path...
		char buf[128];
		getcwd(buf, 128);
		char logPathBuf[128];
		GenLogFileFullPath(g_logger->logPath, logPathBuf);
		int len = strlen(logPathBuf);
		memmove(logPathBuf, logPathBuf + 1, len - 1);
		logPathBuf[len - 1] = 0;

		sprintf(logMsg, "SMS: Agent is aborted. Check below log.\n%s%s\n", buf, logPathBuf);
		write(g_stderrFd, logMsg, strlen(logMsg));
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

			memset(param->agentId, 0, 16);
			if ((tmp = GetValueByKey(CONF_KEY_ID, options)) == NULL)
				strcpy(param->agentId, "debug");
			else
				strncpy(param->agentId, tmp, 15);
			
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
			return NewLogger(logPath, LOG_INFO);
	}
	return NewLogger(logPath, LOG_DEBUG);
}