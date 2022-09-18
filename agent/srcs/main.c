#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include "globalResource.h"
#include "sender.h"

static const char* const strSignal[] = {
	[SIGBUS] = "SIGBUS",
	[SIGABRT] = "SIGABRT",
	[SIGFPE] = "SIGFPE",
	[SIGQUIT] = "SIGQUIT",
	[SIGSEGV] = "SIGSEGV",
	[SIGINT] = "SIGINT",
	[SIGILL] = "SIGILL",
	[SIGSYS] = "SIGSYS",
	[SIGTERM] = "SIGTERM",
	[SIGKILL] = "SIGKILL",
};

static const char* const runKeys[COLLECTOR_COUNT] = {
	[CPU_COLLECTOR_ID] = CONF_KEY_RUN_CPU_COLLECTOR,
	[MEM_COLLECTOR_ID] = CONF_KEY_RUN_MEM_COLLECTOR,
	[NET_COLLECTOR_ID] = CONF_KEY_RUN_NET_COLLECTOR,
	[PROC_COLLECTOR_ID] = CONF_KEY_RUN_PROC_COLLECTOR,
	[DISK_COLLECTOR_ID] = CONF_KEY_RUN_DISK_COLLECTOR
};

static const char* const periodKeys[COLLECTOR_COUNT] = {
	[CPU_COLLECTOR_ID] = CONF_KEY_CPU_COLLECTION_PERIOD,
	[MEM_COLLECTOR_ID] = CONF_KEY_MEM_COLLECTION_PERIOD,
	[NET_COLLECTOR_ID] = CONF_KEY_NET_COLLECTION_PERIOD,
	[PROC_COLLECTOR_ID] = CONF_KEY_PROC_COLLECTION_PERIOD,
	[DISK_COLLECTOR_ID] = CONF_KEY_DISK_COLLECTION_PERIOD
};

void* (*collectRoutines[COLLECTOR_COUNT])(void*) = {
	[CPU_COLLECTOR_ID] = CpuInfoRoutine,
	[MEM_COLLECTOR_ID] = MemInfoRoutine,
	[NET_COLLECTOR_ID] = NetInfoRoutine,
	[PROC_COLLECTOR_ID] = ProcInfoRoutine,
	[DISK_COLLECTOR_ID] = DiskInfoRoutine
};

SGlobResource globResource = { 0, };

void HandleSignal(int signo)
{
	char logMsg[512];

	if (signo == SIGQUIT || signo == SIGTERM)
	{
		globResource.turnOff = true;
		for (int i = 0; i < COLLECTOR_COUNT; i++)
		{
			if (globResource.collectors[i] == 0)
				continue;
			pthread_join(globResource.collectors[i], NULL);
		}
		sprintf(logMsg, "Agent is terminated");
		Log(globResource.logger, LOG_INFO, logMsg);
	}
	else
	{
		sprintf(logMsg, "Agent is aborted: %s", strSignal[signo]);
		Log(globResource.logger, LOG_FATAL, logMsg);
		char logPathBuf[128];
		GenLogFileFullPath(globResource.logger->logPath, logPathBuf);

		sprintf(logMsg, "SMS: Agent is aborted. Check below log.\n%s\n", logPathBuf);
		write(globResource.stderrFd, logMsg, strlen(logMsg));
	}
	exit(signo);
}

int RunCollectors()
{
	char logmsgBuf[128];
	char* tmp;

	memset(globResource.agentID, 0, 16);
	// TODO: change agent ID processing
	if ((tmp = GetValueByKey(CONF_KEY_ID, globResource.configurations)) == NULL)
		strcpy(globResource.agentID, "debug");
	else
		strncpy(globResource.agentID, tmp, 15);
		
	for (int i = 0; i < COLLECTOR_COUNT; i++)
	{
		if ((tmp = GetValueByKey(runKeys[i], globResource.configurations)) != NULL)
		{
			if (strcmp(tmp, "true") == 0)
			{
				if ((tmp = GetValueByKey(periodKeys[i], globResource.configurations)) != NULL)
					globResource.collectPeriods[i] = atoi(tmp);
						
				if (globResource.collectPeriods[i] < MIN_SLEEP_MS)
					globResource.collectPeriods[i] = MIN_SLEEP_MS;
				
				if (pthread_create(&globResource.collectors[i], NULL, collectRoutines[i], NULL) == -1)
				{
					sprintf(logmsgBuf, "Failed to start collector");
					Log(globResource.logger, LOG_FATAL, logmsgBuf);
					exit(EXIT_FAILURE);
				}
			}
		}
	}
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

__attribute__((constructor))
static void InitializeAgent(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "ERROR: you must input conf file path\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGBUS, HandleSignal);
	signal(SIGABRT, HandleSignal);
	signal(SIGFPE, HandleSignal);
	signal(SIGQUIT, HandleSignal);
	signal(SIGSEGV, HandleSignal); 
	signal(SIGINT, HandleSignal);
	signal(SIGILL, HandleSignal);
	signal(SIGSYS, HandleSignal);
	signal(SIGTERM, HandleSignal);
	signal(SIGKILL, HandleSignal);

	globResource.loadTime = time(NULL);
	globResource.configurations = NewHashTable();
	if (ParseConf(argv[1], globResource.configurations) != CONF_NO_ERROR)
	{
		fprintf(stderr, "ERROR: ParseConf failed\n");
		exit(EXIT_FAILURE);
	}
	globResource.logger = GenLogger(globResource.configurations);
	globResource.queue = NewQueue();
	char* value;

	if ((value = GetValueByKey(CONF_KEY_RUN_AS_DAEMON, globResource.configurations)) != NULL)
	{
		if (strcmp(value, "true") == 0)
		{
			Log(globResource.logger, LOG_INFO, "Agent run as daemon");
			pid_t pid = fork();

			if (pid == -1)
			{
				fprintf(stderr, "ERROR: failed to fork: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			if (pid != 0)
				exit(EXIT_SUCCESS);
			
			signal(SIGHUP, SIG_IGN);
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			globResource.stderrFd = dup(STDERR_FILENO);
			close(STDERR_FILENO);
			chdir("/");
			setsid();
		}
	}

	if ((value = GetValueByKey(CONF_KEY_HOST_ADDRESS, globResource.configurations)) != NULL)
		strcpy(globResource.peerIP, value);
	else
		strcpy(globResource.peerIP, "127.0.0.1");
	value = GetValueByKey(CONF_KEY_HOST_PORT, globResource.configurations);
	globResource.peerPort = value != NULL ? atoi(value) : 4242;
}

int main(int argc, char** argv)
{
	char logmsgBuf[128] = { 0, };

	RunCollectors();
	pthread_t senderTid;
	if (pthread_create(&senderTid, NULL, SendRoutine, NULL) != 0)
	{
		sprintf(logmsgBuf, "Fail to start sender: %s", strerror(errno));
		Log(globResource.logger, LOG_FATAL, logmsgBuf);
		exit(EXIT_FAILURE);
	}
	pthread_join(senderTid, NULL);
	
	exit(EXIT_SUCCESS);
}