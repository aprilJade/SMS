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

#include "receiveRoutine.h"
#include "workerRoutine.h"
#include "pgWrapper.h"
#include "confParser.h"

#define CONNECTION_COUNT 1024
#define MAX_WORKER_COUNT 16
#define DEFAULT_PORT 4242

int OpenSocket(short port)
{
    struct sockaddr_in servAddr;
    int servFd = socket(PF_INET, SOCK_STREAM, 0);
    if (servFd == -1)
    {
        // TODO: handle error
        return -1;
    }
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
    servAddr.sin_family = AF_INET;

    if (bind(servFd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
    {
        // TODO: handle error
        close(servFd);
        return -1;
    }
    return servFd;
}

void CreateWorker(int workerCount, Logger* logger, Queue* queue)
{
    SWorkerParam* param;
    pthread_t tid;
    SPgWrapper* db = NewPgWrapper("dbname = postgres");
    if (db == NULL)
        exit(1);
    for (int i = 0; i < workerCount; i++)
    {
        param = (SWorkerParam*)malloc(sizeof(SWorkerParam));
        param->workerId = i;
        param->logger = logger;
        param->queue = queue;
        param->db = db;
        pthread_create(&tid, NULL, WorkerRoutine, param);
        pthread_detach(tid);
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
		// TODO: handle error
		fprintf(stderr, "conf error: %d\n", ret);
		exit(1);
	}
    char* tmp = GetValueByKey(CONF_KEY_LISTEN_PORT, options);
    unsigned short port = 4242;
    if (tmp != NULL)
        port = (unsigned short)atoi(tmp);
	Logger* logger = GenLogger(options);
    char logMsg[128];

    sprintf(logMsg, "Server loaded: %d", getpid());
    Log(logger, LOG_INFO, logMsg);
    

    int servFd, clientFd;
    struct sockaddr_in clientAddr;
    if ((servFd = OpenSocket(port)) == -1)
    {
        // TODO: handle error
        exit(1);
    }

    socklen_t len = sizeof(clientAddr);
    pthread_t tid;
    SReceiveParam* param;
    Queue* queue = NewQueue();
    tmp = GetValueByKey(CONF_KEY_WORKER_COUNT, options);
    int workerCount = 2;
    if (tmp != NULL)
        workerCount = atoi(tmp);
    CreateWorker(workerCount, logger, queue);

    while (1)
    {
        if (listen(servFd, CONNECTION_COUNT) == -1)
        {
            Log(logger, LOG_FATAL, "Failed listening");
            exit(1);
        }

        sprintf(logMsg, "Wait for connection from client at %d", port);
        Log(logger, LOG_INFO, logMsg);

        clientFd = accept(servFd, (struct sockaddr*)&clientAddr, &len);
        if (clientFd == -1)
        {
            Log(logger, LOG_FATAL, "Failed to accept connection");
            exit(1);
        }

        param = (SReceiveParam*)malloc(sizeof(SReceiveParam));
        param->clientSock = clientFd;
        param->logger = logger;
        param->host = inet_ntoa(clientAddr.sin_addr);
        param->port = ntohs(clientAddr.sin_port);
        param->queue = queue;

        if (pthread_create(&tid, NULL, ReceiveRoutine, param) == -1)
        {
            Log(logger, LOG_FATAL, "Failed to create receiver");
            close(clientFd);
            continue;
        }

        sprintf(logMsg, "Start receiver for %s:%d", param->host, param->port);
        Log(logger, LOG_INFO, logMsg);
        pthread_detach(tid);
    }
}   