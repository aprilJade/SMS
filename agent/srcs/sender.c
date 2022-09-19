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
    LOG_FATAL(globResource.logger, "Disconnected to server: %s:%d", globResource.peerIP, globResource.peerPort);
    LOG_DEBUG(globResource.logger, "Try to reconnect: %s:%d", globResource.peerIP, globResource.peerPort);
    
    while (1)
    {
        if (globResource.turnOff)
            return;
        close(g_servSockFd);
        if ((g_servSockFd = ConnectToServer(globResource.peerIP, globResource.peerPort)) != -1)
            break;
        connFailCount++;
        LOG_ERROR(globResource.logger, "Failed to connect %s:%d...%d", globResource.peerIP, globResource.peerPort, connFailCount);
        sleep(RECONNECT_PERIOD);
    }
    
    LOG_INFO(globResource.logger, "Re-connected to %s:%d", globResource.peerIP, globResource.peerPort);
    globResource.bIsConnected = true;
}

void* SendRoutine(void* param)
{
    SCData* collectedData = NULL;
    int sendBytes;
    int connFailCount = 0;

	signal(SIGPIPE, RecoverTcpConnection);
    LOG_INFO(globResource.logger, "Start sender routine");

    globResource.bIsConnected = false;

    while (1)
    {
        if (globResource.turnOff)
        {
            LOG_INFO(globResource.logger, "Terminate sender");
            return NULL;
        }
        if ((g_servSockFd = ConnectToServer(globResource.peerIP, globResource.peerPort)) != -1)
            break;
        connFailCount++;
        LOG_ERROR(globResource.logger, "Failed to connect %s:%d (%d)", globResource.peerIP, globResource.peerPort, connFailCount);
        sleep(RECONNECT_PERIOD);
    }

    LOG_INFO(globResource.logger, "Connected to %s:%d", globResource.peerIP, globResource.peerPort);
    
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

        LOG_DEBUG(globResource.logger, "Send %d bytes to %s:%d ", sendBytes, globResource.peerIP, globResource.peerPort);

        DestorySCData(&collectedData);
    }
    
    LOG_INFO(globResource.logger, "Terminate sender");
}