#include "globalResource.h"
#include "sender.h"

#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

static int g_servSockFd;
extern SGlobResource globResource;

static void RecoverTcpConnection(int signo)
{
    assert(signo == SIGPIPE);
    globResource.bIsConnected = false;

    int connFailCount = 0;
    char logmsgBuf[128];
    sprintf(logmsgBuf, "Disconnected to server: %s:%d", globResource.peerIP, globResource.peerPort);
    Log(globResource.logger, LOG_FATAL, logmsgBuf);

    sprintf(logmsgBuf, "Try to reconnect: %s:%d", globResource.peerIP, globResource.peerPort);
    Log(globResource.logger, LOG_DEBUG, logmsgBuf);
    while (1)
    {
        if (globResource.turnOff)
            return;
        close(g_servSockFd);
        if ((g_servSockFd = ConnectToServer(globResource.peerIP, globResource.peerPort)) != -1)
            break;
        connFailCount++;
        sprintf(logmsgBuf, "Failed to connect %s:%d...%d", globResource.peerIP, globResource.peerPort, connFailCount);
        Log(globResource.logger, LOG_ERROR, logmsgBuf);
        sleep(RECONNECT_PERIOD);
    }
    
    sprintf(logmsgBuf, "Re-connected to %s:%d", globResource.peerIP, globResource.peerPort);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    globResource.bIsConnected = true;
}

void* SendRoutine(void* param)
{
    SCData* collectedData = NULL;
    int sendBytes;
    char logmsgBuf[128];
    int connFailCount = 0;

	signal(SIGPIPE, RecoverTcpConnection);
    Log(globResource.logger, LOG_INFO, "Start sender routine");

    globResource.bIsConnected = false;

    while (1)
    {
        if (globResource.turnOff)
        {
            sprintf(logmsgBuf, "Terminate sender");
            Log(globResource.logger, LOG_INFO, logmsgBuf);
            return NULL;
        }
        if ((g_servSockFd = ConnectToServer(globResource.peerIP, globResource.peerPort)) != -1)
            break;
        sprintf(logmsgBuf, "Failed to connect %s:%d (%d)", globResource.peerIP, globResource.peerPort, connFailCount);
        connFailCount++;
        Log(globResource.logger, LOG_ERROR, logmsgBuf);
        sprintf(logmsgBuf, "Try to reconnect: %s:%d", globResource.peerIP, globResource.peerPort);
        sleep(RECONNECT_PERIOD);
    }

    sprintf(logmsgBuf, "Connected to %s:%d", globResource.peerIP, globResource.peerPort);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    globResource.bIsConnected = true;

    while(globResource.turnOff == false)
    {
        if (collectedData == NULL)
        {
            if (IsEmpty(globResource.queue))
            {
                usleep(500000);
                continue;
            }
            
            pthread_mutex_lock(&globResource.queue->lock);
            collectedData = Pop(globResource.queue);
            pthread_mutex_unlock(&globResource.queue->lock);
        }

        if ((sendBytes = send(g_servSockFd, collectedData->data, collectedData->dataSize, 0)) == -1)
            continue;

        sprintf(logmsgBuf, "Send %d bytes to %s:%d ", sendBytes, globResource.peerIP, globResource.peerPort);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);

        DestorySCData(&collectedData);
    }
    
    sprintf(logmsgBuf, "Terminate sender");
    Log(globResource.logger, LOG_INFO, logmsgBuf);
}