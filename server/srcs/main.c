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
#include "receiveRoutine.h"
#include "workerRoutine.h"
#include "pgWrapper.h"
#include "confParser.h"
#include "udpPacket.h"

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
int udpSockFd;
pthread_t udpTid;
pthread_t* workerId;
Queue* g_queue;
int g_stderrFd;

void HandleSignal(int signo)
{
	char logMsg[512];

	if (signo == SIGQUIT || signo == SIGTERM)
	{
        g_turnOff = true;
        close(udpSockFd);
        pthread_kill(udpTid, SIGTERM);
        pthread_join(udpTid, NULL);
        for (int i = 0; i < 4; i++)
            pthread_join(workerId[i], NULL);

		sprintf(logMsg, "Server is terminated");
		Log(g_logger, LOG_INFO, logMsg);
	}
	else
	{
		sprintf(logMsg, "Server is aborted: %s", strSignal[signo]);
		Log(g_logger, LOG_FATAL, logMsg);
		char logPathBuf[128];
		GenLogFileFullPath(g_logger->logPath, logPathBuf);

		sprintf(logMsg, "SMS: Server is aborted. Check below log.\n%s\n", logPathBuf);
		write(g_stderrFd, logMsg, strlen(logMsg));
	}

	exit(signo);
}

int OpenSocket(short port)
{
    struct sockaddr_in servAddr;
    int servFd = socket(PF_INET, SOCK_STREAM, 0);
    if (servFd == -1)
        return -1;

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
    servAddr.sin_family = AF_INET;

    if (bind(servFd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
    {
        close(servFd);
        return -1;
    }
    return servFd;
}

void CreateWorker(int workerCount)
{
    SWorkerParam* param;
    workerId = (pthread_t*)malloc(sizeof(pthread_t) * workerCount);

    SPgWrapper* db = NewPgWrapper("dbname = postgres");

    if (db->connected == false)
        Log(g_logger, LOG_FATAL, "PostgreSQL connection failed");

    for (int i = 0; i < workerCount; i++)
    {
        param = (SWorkerParam*)malloc(sizeof(SWorkerParam));
        param->workerId = i;
        param->db = db;
        pthread_create(&workerId[i], NULL, WorkerRoutine, param);
    }
}

Logger* GenLogger(SHashTable* options)
{
	char* logPath;
	char* logLevel;
	Logger* logger;
	if ((logPath = GetValueByKey(CONF_KEY_LOG_PATH, options)) == NULL)
		logPath = "./server";
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

void* UdpRoutine(void* param)
{
    udpSockFd = socket(PF_INET, SOCK_DGRAM, 0);
    if (udpSockFd < 0)
        return 0;
    struct sockaddr_in udpAddr;
    memset(&udpAddr, 0, sizeof(udpAddr));

    udpAddr.sin_family = AF_INET;
    udpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpAddr.sin_port = htons(4343);

    if (bind(udpSockFd, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) < 0)
        return 0;
    
    int readSize;
    char buf[1024];
    char logMsg[256];
    struct sockaddr_in udpClientAddr;
    socklen_t len;
    SPrefixPkt* prevPkt;
    SPostfixPkt* postPkt;

    sprintf(logMsg, "Start UDP receiver");
    Log(g_logger, LOG_INFO, logMsg);

    while (1)
    {
        if ((readSize = recvfrom(udpSockFd, buf, sizeof(SPrefixPkt), 0, (struct sockaddr*)&udpClientAddr, &len)) < 0)
        {
            sprintf(logMsg, "Fail to receive UDP packet");
            Log(g_logger, LOG_ERROR, logMsg);
            continue;
        }
        buf[readSize];

        if (readSize != sizeof(SPrefixPkt))
        {
            // packet loss..
            continue;
        }
        
        prevPkt = (SPrefixPkt*)buf;
        //printf("<Prefix Packet Info>\nno.%lu: %lu: %s: %d (%s) addr: %s:%u\n",
        //    prevPkt->packetNo ,prevPkt->beginTime, prevPkt->agentId, prevPkt->pid, prevPkt->processName,
        //    prevPkt->hostIp, prevPkt->hostPort);


        if ((readSize = recvfrom(udpSockFd, buf, sizeof(SPostfixPkt), 0, (struct sockaddr*)&udpClientAddr, &len)) < 0)
        {
            sprintf(logMsg, "Fail to receive UDP packet");
            Log(g_logger, LOG_ERROR, logMsg);  
            continue;
        }
        buf[readSize];

        if (readSize != sizeof(SPostfixPkt))
        {
            // packet loss...
            continue;
        }

        postPkt = (SPostfixPkt*)buf;
        //printf("<Postfix Packet Info>\n%lu: %s: %d (%s) sended: %d\n\n",
        //    postPkt->elapseTime, postPkt->agentId, postPkt->pid, postPkt->processName, postPkt->sendBytes);
    }

    sprintf(logMsg, "End UDP receiver");
    Log(g_logger, LOG_INFO, logMsg);
}

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
				// TODO: handle error
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

    tmp = GetValueByKey(CONF_KEY_LISTEN_PORT, options);
    unsigned short port = 4242;
    if (tmp != NULL)
        port = (unsigned short)atoi(tmp);
	g_logger = GenLogger(options);
    char logMsg[256];

    sprintf(logMsg, "Server loaded: %d", getpid());
    Log(g_logger, LOG_INFO, logMsg);
    
    pthread_create(&udpTid, NULL, UdpRoutine, NULL);
    
    int servFd, clientFd;
    struct sockaddr_in clientAddr;
    if ((servFd = OpenSocket(port)) == -1)
    {
        sprintf(logMsg, "Socket is not opened: %s", strerror(errno));
        Log(g_logger, LOG_FATAL, "Socket is not opened.");

        char logPath[128];
		GenLogFileFullPath(g_logger->logPath, logPath);
		sprintf(logMsg, "SMS: Socket is not opened. Check below log.\n%s\n", logPath);
		write(g_stderrFd, logMsg, strlen(logMsg));
        exit(1);
    }

    socklen_t len = sizeof(clientAddr);
    pthread_t tid;
    SReceiveParam* param;
    g_queue = NewQueue();
    tmp = GetValueByKey(CONF_KEY_WORKER_COUNT, options);
    int workerCount = 2;
    if (tmp != NULL)
        workerCount = atoi(tmp);
    CreateWorker(workerCount);

    while (1)
    {
        if (listen(servFd, CONNECTION_COUNT) == -1)
        {
            Log(g_logger, LOG_FATAL, "Failed listening");
            exit(1);
        }

        sprintf(logMsg, "Wait for connection from client at %d", port);
        Log(g_logger, LOG_INFO, logMsg);

        clientFd = accept(servFd, (struct sockaddr*)&clientAddr, &len);
        if (clientFd == -1)
        {
            Log(g_logger, LOG_FATAL, "Failed to accept connection");
            exit(1);
        }

        param = (SReceiveParam*)malloc(sizeof(SReceiveParam));
        param->clientSock = clientFd;
        param->host = inet_ntoa(clientAddr.sin_addr);
        param->port = ntohs(clientAddr.sin_port);

        if (pthread_create(&tid, NULL, ReceiveRoutine, param) == -1)
        {
            Log(g_logger, LOG_FATAL, "Failed to create receiver");
            close(clientFd);
            continue;
        }

        sprintf(logMsg, "Start TCP receiver for %s:%d", param->host, param->port);
        Log(g_logger, LOG_INFO, logMsg);
        pthread_detach(tid);
    }
}   