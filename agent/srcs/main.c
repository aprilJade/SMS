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
const Logger* g_logger;
Queue* g_queue;
const char g_serverIp[16];
unsigned short g_serverPort;
int g_stderrFd;

void HandleSignal(int);
pthread_t RunCollector(void* (*)(void*), const char*, const char*, SHashTable*);
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
	g_queue = NewQueue();
	
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

	pthread_t cpuTid = RunCollector(CpuInfoRoutine, CONF_KEY_RUN_CPU_COLLECTOR, CONF_KEY_CPU_COLLECTION_PERIOD, options);
	pthread_t memTid = RunCollector(MemInfoRoutine, CONF_KEY_RUN_MEM_COLLECTOR, CONF_KEY_MEM_COLLECTION_PERIOD, options);
	pthread_t netTid = RunCollector(NetInfoRoutine, CONF_KEY_RUN_NET_COLLECTOR, CONF_KEY_NET_COLLECTION_PERIOD, options);
	pthread_t procTid = RunCollector(ProcInfoRoutine, CONF_KEY_RUN_PROC_COLLECTOR, CONF_KEY_PROC_COLLECTION_PERIOD, options);
	pthread_t diskTid = RunCollector(DiskInfoRoutine, CONF_KEY_RUN_DISK_COLLECTOR, CONF_KEY_DISK_COLLECTION_PERIOD, options);
	
	
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

	if ((value = GetValueByKey(CONF_KEY_HOST_ADDRESS, options)) != NULL)
		strcpy((char*)g_serverIp, value);
	else
		strcpy((char*)g_serverIp, "127.0.0.1");
	value = GetValueByKey(CONF_KEY_HOST_PORT, options);
	g_serverPort = value != NULL ? atoi(value) : 4242;

	// why did you make sender thread...?
	if (pthread_create(&senderTid, NULL, SendRoutine, NULL) == -1)
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
	sprintf(logMsg, "Killed by %s", strSignal[signo]);

	if (signo == SIGQUIT || signo == SIGKILL)
		Log(g_logger, LOG_INFO, logMsg);
	else
	{
		Log(g_logger, LOG_FATAL, logMsg);
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
						const char* keyPeriod, SHashTable* options)
{
	char* tmp;
	char logmsgBuf[128];
	pthread_t tid = -1;
	SRoutineParam* param = (SRoutineParam*)malloc(sizeof(SRoutineParam));

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
				Log(g_logger, LOG_FATAL, logmsgBuf);
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