#include "udsCtrl.h"
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/un.h>
#include <fcntl.h>
#include "printer.h"
#include "logger.h"

static struct sockaddr_un clientInfo;

static int CreateUpdatePacket(SHashTable* options, SUpdatePacket* out)
{
    char* tmp;
    
    out->logLevel = LOG_INFO;
    if ((tmp = GetValueByKey(CONF_KEY_LOG_LEVEL, options)) != NULL)
    {
        if (strcmp(tmp, "debug") == 0)
            out->logLevel = LOG_DEBUG;
    }

    for (int i = 0; i < 5; i++)
    {
        if ((tmp = GetValueByKey(runKeys[i], options)) != NULL)
        {
            if (strcmp(tmp, "true") == 0)
            {
                out->bRunCollector[i] = true;

                if ((tmp = GetValueByKey(periodKeys[i], options)) != NULL)
                    out->collectPeriod[i] = atoi(tmp);
                else
                    out->collectPeriod[i] = 1000;
                        
                if (out->collectPeriod[i] < 500)
                    out->collectPeriod[i] = 500;
            }
        }
    }
    strcpy(out->udsPath, UDS_CLIENT_PATH);
    return 0;
}

static int CreateUDS()
{
    int fd = open(UDS_CLIENT_PATH, O_CREAT | O_RDWR, 0777);
	if (fd == -1)
	{
		fprintf(stderr, "failed to create uds file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(fd);

	int uds;
	if ((uds = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
	{
		fprintf(stderr, "failed to uds socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	clientInfo.sun_family = AF_UNIX;
	strcpy(clientInfo.sun_path, UDS_CLIENT_PATH);
	unlink(UDS_CLIENT_PATH);

	if (bind(uds, (struct sockaddr*)&clientInfo, sizeof(clientInfo)) == -1)
	{
		close(uds);
        remove(UDS_CLIENT_PATH);
		fprintf(stderr, "failed to bind uds socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
    return uds;
}

void RequestUpdateConfigToAgent(const char* confPath)
{
    if (access(UDS_AGENT_PATH, F_OK) != 0)
    {
        printf("Agent is not working!\n");
        exit(EXIT_SUCCESS);
    }

    SHashTable* options = NewHashTable();
    if (ParseConf(confPath, options) != CONF_NO_ERROR)
    {
        fprintf(stderr, "ERROR: failed to parse \'%s\'\n", confPath);
        exit(EXIT_FAILURE);
    }

    SUpdatePacket pkt = { 0, };
    if (CreateUpdatePacket(options, &pkt) != 0)
    {
        fprintf(stderr, "ERROR: failed to create udpate packet\n");
        exit(EXIT_FAILURE);
    }

    printf("Request to update configuration to agent with the following values\n");
    PrintPacketContent(&pkt);

    char c;
    while (1)
    {
        printf("Are you sure you want to request it?(Y/N): ");
        c = getchar();
        if (c == 'n' || c == 'N')
        {
            printf("Update agent's configuration has been cancled.\n");
            exit(EXIT_SUCCESS);
        }
        else if (c == 'y' || c == 'Y')
        {
            printf("Request to update...\n");
            break;
        }
    }
    int recvFd = CreateUDS();

    int uds = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (uds == -1)
    {
        fprintf(stderr, "ERROR: failed to open uds: %s\n", strerror(errno));
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un remoteInfo = { 0, };
    remoteInfo.sun_family = AF_UNIX;
    strcpy(remoteInfo.sun_path, UDS_AGENT_PATH);
    uint helloPkt = UDS_UPDATE_PACKET;
    if (sendto(uds, &helloPkt, sizeof(uint), 0, (struct sockaddr*)&remoteInfo, sizeof(remoteInfo)) == -1)
    {
        fprintf(stderr, "ERROR: failed to send update packet: %s\n", strerror(errno));
        close(uds);
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }

    if (sendto(uds, &pkt, sizeof(SUpdatePacket), 0, (struct sockaddr*)&remoteInfo, sizeof(remoteInfo)) == -1)
    {
        fprintf(stderr, "ERROR: failed to send update packet: %s\n", strerror(errno));
        close(uds);
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }
    close(uds);
    
    char buf[128] = { 0, };
    socklen_t remoteLen = sizeof(clientInfo);
    ssize_t recvSize = recvfrom(recvFd, buf, 128, 0, (struct sockaddr*)&clientInfo, &remoteLen);
    if (recvSize < 0)
    {
        fprintf(stderr, "ERROR: agent is not responding\n");
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }
    buf[recvSize] = 0;
    if (strcmp(buf, "update complete") == 0)
        printf("Agent configuration is updated.\n");
    else
        fprintf(stderr, "Failed to update agent's configuraion: %s\n", buf);
    
    close(recvFd);
    remove(UDS_CLIENT_PATH);
    exit(EXIT_SUCCESS);
}

void RequestStatusToAgent(void)
{
    if (access(UDS_AGENT_PATH, F_OK) != 0)
    {
        printf("Agent is not working!\n");
        exit(EXIT_SUCCESS);
    }

    int recvFd = CreateUDS();

    int uds = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (uds == -1)
    {
        fprintf(stderr, "ERROR: failed to open uds: %s\n", strerror(errno));
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un remoteInfo = { 0, };
    remoteInfo.sun_family = AF_UNIX;
    strcpy(remoteInfo.sun_path, UDS_AGENT_PATH);
    uint helloPkt = UDS_STATUS_PACKET;
    if (sendto(uds, &helloPkt, sizeof(uint), 0, (struct sockaddr*)&remoteInfo, sizeof(remoteInfo)) == -1)
    {
        fprintf(stderr, "ERROR: failed to send update packet: %s\n", strerror(errno));
        close(uds);
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }

    if (sendto(uds, UDS_CLIENT_PATH, strlen(UDS_CLIENT_PATH), 0, (struct sockaddr*)&remoteInfo, sizeof(remoteInfo)) == -1)
    {
        fprintf(stderr, "ERROR: failed to send update packet: %s\n", strerror(errno));
        close(uds);
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }
    close(uds);

    SAgentStatusPacket response;
    socklen_t remoteLen = sizeof(clientInfo);
    ssize_t recvSize = recvfrom(recvFd, &response, sizeof(SAgentStatusPacket), 0, (struct sockaddr*)&clientInfo, &remoteLen);
    if (recvSize < 0)
    {
        fprintf(stderr, "ERROR: agent is not responding\n");
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }
    if (recvSize != sizeof(SAgentStatusPacket))
    {
        fprintf(stderr, "ERROR: agent response???\n");
        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_FAILURE);
    }
    PrintAgentStatus(&response);

    close(recvFd);
    remove(UDS_CLIENT_PATH);
    exit(EXIT_SUCCESS);
}