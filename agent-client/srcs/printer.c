#include "printer.h"
#include <stdio.h>

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
    putchar('\n');
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