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
    Logger* logger = NewLogger("./log/server");
    Queue* queue = NewQueue();
    CreateWorker(logger, queue);

    char logMsg[128];
    while (1)
    {
        sprintf(logMsg, "listen at %d", port);
        Log(logger, logMsg);

        if (listen(servFd, CONNECTION_COUNT) == -1)
        {
            Log(logger, "fail listen");
            exit(1);
        }
        clientFd = accept(servFd, (struct sockaddr*)&clientAddr, &len);
        if (clientFd == -1)
        {
            Log(logger, "fail accept");
            exit(1);
        }

        param = (SReceiveParam*)malloc(sizeof(SReceiveParam));
        param->clientSock = clientFd;
        param->logger = logger;
        param->host = inet_ntoa(clientAddr.sin_addr);
        param->port = ntohs(clientAddr.sin_port);
        param->queue = queue;
        sprintf(logMsg, "connected with %s:%d", param->host, param->port);
        Log(logger, logMsg);

        if (pthread_create(&tid, NULL, ReceiveRoutine, param) == -1)
        {
            Log(logger, "fail create receiver");
            close(clientFd);
            continue;
        }

        sprintf(logMsg, "run-receiver for %s:%d", param->host, param->port);
        Log(logger, logMsg);
        pthread_detach(tid);
    }
}   