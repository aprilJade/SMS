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
#include "receiveRoutine.h"
#include "workerRoutine.h"
#include "pgWrapper.h"

#define CONNECTION_COUNT 4
#define WORKER_COUNT 1
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 4242

int OpenSocket(short port)
{
    struct sockaddr_in servAddr;
    int servFd = socket(PF_INET, SOCK_STREAM, 0);
    if (servFd == -1)
    {
        perror("server");
        return -1;
    }
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
    servAddr.sin_family = AF_INET;

    if (bind(servFd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
    {
        printf("Fail to bind\n");
        perror("server");
        close(servFd);
        return -1;
    }
    return servFd;
}

void CreateWorker(Logger* logger, Queue* queue)
{
    SWorkerParam* param;
    pthread_t tid;
    SPgWrapper* db = NewPgWrapper("dbname = postgres");

    for (int i = 0; i < WORKER_COUNT; i++)
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

int main(int argc, char** argv)
{
    short port = (short)atoi(argv[1]);

    int servFd, clientFd;
    struct sockaddr_in clientAddr;
    if ((servFd = OpenSocket(port)) == -1)
    {
        perror("server");
        return 1;
    }

    socklen_t len = sizeof(clientAddr);
    void* (*routine)(void*);
    pthread_t tid;
    SReceiveParam* param;
    Logger* logger = NewLogger("./log/server", LOG_INFO);
    Queue* queue = NewQueue();
    CreateWorker(logger, queue);

    char logMsg[128];
    while (1)
    {
        sprintf(logMsg, "Listen at %d", port);
        Log(logger, LOG_INFO, logMsg);

        if (listen(servFd, CONNECTION_COUNT) == -1)
        {
            Log(logger, LOG_FATAL, "Failed listening");
            exit(1);
        }
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
        param->port = clientAddr.sin_port;
        param->queue = queue;

        if (pthread_create(&tid, NULL, ReceiveRoutine, param) == -1)
        {
            Log(logger, LOG_FATAL, "Failed to create receiver");
            close(clientFd);
            continue;
        }

        sprintf(logMsg, "Run receiver for %s:%d", param->host, param->port);
        Log(logger, LOG_INFO, logMsg);
        pthread_detach(tid);
    }
}   