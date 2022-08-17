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

#define CONNECTION_COUNT 1024
#define WORKER_COUNT 1
#define DEFAULT_HOST "127.0.0.1"
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
    Logger* logger = NewLogger("./log/server", LOG_INFO);
    char logMsg[128];

    sprintf(logMsg, "Server loaded: %d", getpid());
    Log(logger, LOG_INFO, logMsg);
    sprintf(logMsg, "Log level: %d", LOG_INFO);
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
    CreateWorker(logger, queue);

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
        param->port = clientAddr.sin_port;
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