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

#include "packets.h"
#include "confParser.h"

// CHECK: modify below path when deploy SMS
#define UDS_AGENT_PATH "/home/apriljade/repo/SMS/bin/.agent.sock"
#define UDS_CLIENT_PATH "/home/apriljade/repo/SMS/bin/.client.sock"

void PrintUsage()
{
    fprintf(stderr, "Not implemented yet: PrintUsage()\n");
}

void PrintPacketContent(SUpdatePacket* pkt)
{
    printf("client UDS socket path: %s\n", pkt->udsPath);

    printf("RUN_CPU_COLLECTOR: %s\n", (pkt->bRunCollector[0] ? "true" : "false"));
    if (pkt->bRunCollector[0])
        printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->collectPeriod[0]);

    printf("RUN_MEM_COLLECTOR: %s\n", (pkt->bRunCollector[1] ? "true" : "false"));
    if (pkt->bRunCollector[1])
        printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->collectPeriod[1]);

    printf("RUN_NET_COLLECTOR: %s\n", (pkt->bRunCollector[2] ? "true" : "false"));
    if (pkt->bRunCollector[2])
        printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->collectPeriod[2]);

    printf("RUN_PROC_COLLECTOR: %s\n", (pkt->bRunCollector[3] ? "true" : "false"));
    if (pkt->bRunCollector[3])
        printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->collectPeriod[3]);

    printf("RUN_DISK_COLLECTOR: %s\n", (pkt->bRunCollector[4] ? "true" : "false"));
    if (pkt->bRunCollector[4])
        printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->collectPeriod[4]);
}

void PrintAgentStatus(SAgentStatusPacket* pkt)
{
    printf("<Agent status>\n");
    printf("# Basic information\n");
    printf("Running time: %lu s\n", pkt->runningTime);
    printf("PID: %d\n", pkt->pid);
    printf("Process name: %s\n", "not implemented yet");
    putchar('\n');
    printf("# Collector Status\n");
    printf("CPU collector: %s on every %lu ms\n", pkt->bRunCollector[0] ? "ON" : "OFF", pkt->collectPeriod[0]);
    printf("Memory collector: %s on every %lu ms\n", pkt->bRunCollector[1] ? "ON" : "OFF", pkt->collectPeriod[1]);
    printf("Network collector: %s on every %lu ms\n", pkt->bRunCollector[2] ? "ON" : "OFF", pkt->collectPeriod[2]);
    printf("Process collector: %s on every %lu ms\n", pkt->bRunCollector[3] ? "ON" : "OFF", pkt->collectPeriod[3]);
    printf("Disk collector: %s on every %lu ms\n", pkt->bRunCollector[4] ? "ON" : "OFF", pkt->collectPeriod[4]);
    putchar('\n');
    printf("# Network Status\n");
    printf("Connection: %s\n", pkt->bConnectedWithServer ? "Connected" : "Disconnected");
    printf("Connectivity: not implemented yet(Good or Bad)\n");
    printf("Peer IP: %s\n", pkt->peerIP);
    printf("Peer Port: %u\n", pkt->peerPort);
    putchar('\n');
}

static struct sockaddr_un clientInfo;

int CreateUDS()
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


static const char* const runKeys[] = {
	CONF_KEY_RUN_CPU_COLLECTOR,
	CONF_KEY_RUN_MEM_COLLECTOR,
	CONF_KEY_RUN_NET_COLLECTOR,
	CONF_KEY_RUN_PROC_COLLECTOR,
	CONF_KEY_RUN_DISK_COLLECTOR
};

static const char* const periodKeys[] = {
	CONF_KEY_CPU_COLLECTION_PERIOD,
	CONF_KEY_MEM_COLLECTION_PERIOD,
	CONF_KEY_NET_COLLECTION_PERIOD,
	CONF_KEY_PROC_COLLECTION_PERIOD,
	CONF_KEY_DISK_COLLECTION_PERIOD
};

int CreateUpdatePacket(SHashTable* options, SUpdatePacket* out)
{
    char* tmp;

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

int main(int argc, char** argv)
{   
    if (argc == 1)
    {
        PrintUsage();
        exit(EXIT_SUCCESS);
    }

    if (strcmp(argv[1], "update") == 0)
    {
        if (access(UDS_AGENT_PATH, F_OK) != 0)
        {
            printf("Agent is not working!\n");
            exit(EXIT_SUCCESS);
        }

        if (argc != 3)
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        SHashTable* options = NewHashTable();
        if (ParseConf(argv[2], options) != CONF_NO_ERROR)
        {
            fprintf(stderr, "ERROR: failed to parse \'%s\'\n", argv[2]);
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
        printf("wait agnet response\n");
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
    else if (strcmp(argv[1], "status") == 0)
    {
        if (access(UDS_AGENT_PATH, F_OK) != 0)
        {
            printf("Agent is not working!\n");
            exit(EXIT_SUCCESS);
        }

        if (argc != 2)
        {
            PrintUsage();
            exit(EXIT_FAILURE);
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
        printf("wait agent response...\n");
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
        printf("Agent's status has arrived. Print status.\n");
        PrintAgentStatus(&response);

        close(recvFd);
        remove(UDS_CLIENT_PATH);
        exit(EXIT_SUCCESS);
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}