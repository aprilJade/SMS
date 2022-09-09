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

#include "packets.h"
#include "confParser.h"

// CHECK: modify below path when deploy SMS
#define UDS_SOCKET_PATH "./bin/.agent.sock"

void PrintUsage()
{
    fprintf(stderr, "Not implemented yet: PrintUsage()\n");
}

int CreateUpdatePacket(SHashTable* options, SUpdatePacket* out)
{
    // Not implemented yet
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
        // TODO: check agent is really not exist. like ps -aux | grep agent
        if (access(UDS_SOCKET_PATH, F_OK) != 0)
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

        SUpdatePacket pkt;
        if (CreateUpdatePacket(options, &pkt) != 0)
        {
            fprintf(stderr, "ERROR: failed to create udpate packet\n");
            exit(EXIT_FAILURE);
        }

        printf("Request to update configuration to agent with the following values\n");
        printf("RUN_CPU_COLLECTOR: %s\n", (pkt.bRunCpuCollector ? "true" : "false"));
        if (pkt.bRunCpuCollector)
            printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt.cpuPeriod);

        printf("RUN_MEM_COLLECTOR: %s\n", (pkt.bRunCpuCollector ? "true" : "false"));
        if (pkt.bRunCpuCollector)
            printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt.cpuPeriod);

        printf("RUN_NET_COLLECTOR: %s\n", (pkt.bRunCpuCollector ? "true" : "false"));
        if (pkt.bRunCpuCollector)
            printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt.cpuPeriod);

        printf("RUN_PROC_COLLECTOR: %s\n", (pkt.bRunCpuCollector ? "true" : "false"));
        if (pkt.bRunCpuCollector)
            printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt.cpuPeriod);

        printf("RUN_DISK_COLLECTOR: %s\n", (pkt.bRunCpuCollector ? "true" : "false"));
        if (pkt.bRunCpuCollector)
            printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt.cpuPeriod);

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
                printf("Request to update...");
                break;
            }
        }

        int uds = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (uds == -1)
        {
            fprintf(stderr, "ERROR: failed to open uds: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        struct sockaddr_un remoteInfo = { 0, };
        remoteInfo.sun_family = AF_UNIX;
        strcpy(remoteInfo.sun_path, UDS_SOCKET_PATH);
        if (sendto(uds, &pkt, sizeof(SUpdatePacket), 0, (struct sockaddr*)&remoteInfo, sizeof(remoteInfo)) == -1)
        {
            fprintf(stderr, "ERROR: failed to send update packet: %s\n", strerror(errno));
            close(uds);
            exit(EXIT_FAILURE);
        }
        char buf[128] = { 0, };
        socklen_t remoteLen = sizeof(remoteInfo);
        ssize_t recvSize = recvfrom(uds, buf, 128, 0, (struct sockaddr*)&remoteInfo, &remoteLen);
        if (recvSize < 0)
        {
            fprintf(stderr, "ERROR: agent is not responding\n");
            exit(EXIT_FAILURE);
        }
        buf[recvSize] = 0;
        if (strcmp(buf, "update complete") == 0)
            printf("Agent configuration is updated.\n");
        else
            fprintf(stderr, "Failed to update agent's configuraion\n");
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        // #00. Send request agent status to agent
        // #01. Receive answer from agent
        // #02. Print agent status using received informations
        // #03. exit
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}