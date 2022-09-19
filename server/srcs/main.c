#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "receiveRoutine.h"
#include "workerRoutine.h"
#include "pgWrapper.h"
#include "confParser.h"
#include "hookerUtils.h"
#include "logger.h"
#include "queue.h"

#define CONNECTION_COUNT 1024
#define MAX_WORKER_COUNT 16
#define DEFAULT_PORT 4242

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

bool g_turnOff = false;
const Logger* g_logger;
pthread_t udpTid;
pthread_t* workerId;
Queue* g_queue;
int g_stderrFd = 2;
pthread_mutex_t g_workerLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_workerCond = PTHREAD_COND_INITIALIZER;
unsigned int g_clientCnt = 0;
unsigned int g_workerCnt;

static void HandleTerminateSignals(int signo)
{
    g_turnOff = true;
    pthread_mutex_lock(&g_workerLock);
    pthread_cond_broadcast(&g_workerCond);
    pthread_mutex_unlock(&g_workerLock);
    for (int i = 0; i < g_workerCnt; i++)
        pthread_join(workerId[i], NULL);
    LOG_INFO(g_logger, "Server is terminated");
    exit(EXIT_SUCCESS);
}

static void HandleFatalSignals(int signo)
{
	char logMsg[512];

    LOG_FATAL(g_logger, "Server is aborted: %s", strSignal[signo]);
    char logPathBuf[128];
    GenLogFileFullPath(g_logger->logPath, logPathBuf);

    sprintf(logMsg, "SMS: Server is aborted. Check below log.\n%s\n", logPathBuf);
    write(g_stderrFd, logMsg, strlen(logMsg));
    exit(signo);
}

static void CreateWorker(int workerCount, SHashTable* options)
{
    SWorkerParam* param;
    SThreshold threshold = GetThresholds(options);
    workerId = (pthread_t*)malloc(sizeof(pthread_t) * workerCount);

    for (int i = 0; i < workerCount; i++)
    {
        param = (SWorkerParam*)malloc(sizeof(SWorkerParam));
        param->workerId = i;
        param->threshold = threshold;
        pthread_create(&workerId[i], NULL, WorkerRoutine, param);
    }
    g_workerCnt = workerCount;
}

static Logger* GenLogger(SHashTable* options)
{
	char* logPath;
	char* logLevel;
	Logger* logger;

	if ((logPath = GetValueByKey(CONF_KEY_LOG_PATH, options)) == NULL)
		logPath = "./server";
	if ((logLevel = GetValueByKey(CONF_KEY_LOG_LEVEL, options)) != NULL)
	{
		if (strcmp(logLevel, "default") == 0)
			return NewLogger(logPath, LOG_LEVEL_INFO);
	}
	return NewLogger(logPath, LOG_LEVEL_DEBUG);
}

static int OpenServerSocket(short port)
{
    struct sockaddr_in servAddr;
    int servFd = socket(PF_INET, SOCK_STREAM, 0);
    if (servFd == -1)
    {
        LOG_FATAL(g_logger, "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
    servAddr.sin_family = AF_INET;

    if (bind(servFd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
    {
        LOG_FATAL(g_logger, "Failed to bind: %s", strerror(errno));
        close(servFd);
        return -1;
    }
    return servFd;
}

static bool GetMacAddr(int targetSock, uchar* out, int outSize)
{
    struct ifconf ifc = { 0, };
    struct ifreq ifr[10];

    ifc.ifc_len = 10 * sizeof(struct ifreq);
    ifc.ifc_buf = (char *)ifr;

    if (ioctl(targetSock, SIOCGIFCONF, (char *)&ifc) < 0)
    {
        // TODO: handle error: couldn't get a MAC address
        fprintf(stderr, "Failed to get MAC address\n");
        return false;
    }
    else
    {
        int nicCnt = ifc.ifc_len / (sizeof(struct ifreq));
        for (int i = 0; i < nicCnt; i++)
        {        
            if (ioctl(targetSock, SIOCGIFHWADDR, &ifr[i]) == 0) 
            {  
                if (strncmp(ifr[i].ifr_name, "lo", 2) == 0)
                    continue;
                memcpy(out, ifr[i].ifr_hwaddr.sa_data, 6);
                out[6] = 0;
                return true;
            } 
        }
    }
}

void ListenAndCreateSession();

int main(int argc, char** argv)
{
    if (argc != 2)
	{
		fprintf(stderr, "ERROR: you must input conf file path\n");
		return EXIT_FAILURE;
	}

	SHashTable* options = NewHashTable();
    int ret;
    
	if ((ret = ParseConf(argv[1], options)) != CONF_NO_ERROR)
	{
		fprintf(stderr, "conf error: %d\n", ret);
		exit(EXIT_FAILURE);
	}

    char* tmp;
    if ((tmp = GetValueByKey(CONF_KEY_RUN_AS_DAEMON, options)) != NULL)
	{
		if (strcmp(tmp, "true") == 0)
		{
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
            dup2(STDERR_FILENO, g_stderrFd);
			close(STDERR_FILENO);
			chdir("/");
			setsid();
		}
	}

    signal(SIGBUS, HandleFatalSignals);	        // bus error
	signal(SIGABRT, HandleFatalSignals);	    // abort signal
	signal(SIGFPE, HandleFatalSignals);	        // floating point error
	signal(SIGQUIT, HandleFatalSignals);	    // quit signal
	signal(SIGSEGV, HandleFatalSignals);        // segmentation fault
	signal(SIGINT, HandleFatalSignals);	        // interrupted
	signal(SIGILL, HandleFatalSignals);	        // illegal instruction
	signal(SIGSYS, HandleFatalSignals);	        // system call error
	signal(SIGTERM, HandleTerminateSignals);	// terminate signalr
	signal(SIGKILL, HandleTerminateSignals);	// terminate signal

    tmp = GetValueByKey(CONF_KEY_LISTEN_PORT, options);
    unsigned short port = DEFAULT_PORT;
    if (tmp != NULL)
        port = (unsigned short)atoi(tmp);
	g_logger = GenLogger(options);

    LOG_INFO(g_logger, "Server loaded: %d", getpid());
    
    pthread_create(&udpTid, NULL, UdpReceiveRoutine, NULL);
    
    int servFd;
    if ((servFd = OpenServerSocket(port)) == -1)
    {
        char logMsg[256];
        char logPath[128];
		GenLogFileFullPath(g_logger->logPath, logPath);
		sprintf(logMsg, "SMS: Socket is not opened. Check below log.\n%s\n", logPath);
		write(g_stderrFd, logMsg, strlen(logMsg));
        exit(EXIT_FAILURE);
    }

    g_queue = NewQueue();

    int workerCount = 4;
    tmp = GetValueByKey(CONF_KEY_WORKER_COUNT, options);
    if (tmp != NULL)
        workerCount = atoi(tmp);
    
    CreateWorker(workerCount, options);
    ListenAndCreateSession(servFd, port);

    exit(EXIT_SUCCESS);
}

void ListenAndCreateSession(int sock, int serverPort)
{
    int clientFd;
    struct sockaddr_in clientAddr;
    socklen_t sockLen = sizeof(clientAddr);
    SReceiveParam* param;
    pthread_t tid;

    LOG_INFO(g_logger, "Listen at %d", serverPort);

    while (1)
    {
        if (listen(sock, CONNECTION_COUNT) == -1)
        {
            LOG_FATAL(g_logger, "Failed to listen");
            exit(EXIT_FAILURE);
        }

        clientFd = accept(sock, (struct sockaddr*)&clientAddr, &sockLen);
        if (clientFd == -1)
        {
            LOG_FATAL(g_logger, "Failed to accept connection");
            exit(EXIT_FAILURE);
        }
        
        param = (SReceiveParam*)malloc(sizeof(SReceiveParam));
        param->clientSock = clientFd;
        param->host = inet_ntoa(clientAddr.sin_addr);
        param->port = ntohs(clientAddr.sin_port);

        if (pthread_create(&tid, NULL, TcpReceiveRoutine, param) == -1)
        {
            LOG_FATAL(g_logger, "Failed to create receiver");
            close(clientFd);
            continue;
        }
        
        pthread_mutex_lock(&g_workerLock);
        g_clientCnt++;
        pthread_cond_broadcast(&g_workerCond);
        pthread_mutex_unlock(&g_workerLock);

        pthread_detach(tid);
    }
}