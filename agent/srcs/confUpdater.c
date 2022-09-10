#include "globalResource.h"

#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>

extern SGlobResource globResource;

e_UdsError ResponseToClientUDS(const char* path, char* data)
{
	int uds = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (uds == -1)
	{
		Log(globResource.logger, LOG_FATAL, "Failed to open UDS socket for");
		return UDS_SOCKET_OPEN_ERROR;
	}

	struct sockaddr_un remoteInfo = { 0, };
	remoteInfo.sun_family = AF_UNIX;
	strcpy(remoteInfo.sun_path, path);
	if (sendto(uds, data, strlen(data), 0, (struct sockaddr*)&remoteInfo, sizeof(remoteInfo)) == -1)
	{
		Log(globResource.logger, LOG_ERROR, "Failed to send UDS");
		close(uds);
		return UDS_RESPONSE_ERROR;
	}
	close(uds);
	return UDS_OK;
}

void CreateStatusPacket(SAgentStatusPacket* pkt)
{
	// not implemented yet
	pkt->pid = getpid();
	strcpy(pkt->peerIP, globResource.peerIP);
	pkt->peerPort = globResource.peerPort;
	
}

void ManageAgentConfiguration(void)
{
	int fd = open(UDS_SOCKET_PATH, O_CREAT | O_RDWR, 0777);
	if (fd == -1)
	{
		// TODO: handle error
		//fprintf(stderr, "failed to create uds file: %s\n", strerror(errno));
		return;
	}
	close(fd);

	int uds;
	if ((uds = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
	{
		// TODO: handle error
		//fprintf(stderr, "failed to uds socket: %s\n", strerror(errno));
		return;
	}
	struct sockaddr_un sockInfo = { 0, };
	sockInfo.sun_family = AF_UNIX;
	strcpy(sockInfo.sun_path, UDS_SOCKET_PATH);
	unlink(UDS_SOCKET_PATH);

	if (bind(uds, (struct sockaddr*)&sockInfo, sizeof(sockInfo)) == -1)
	{
		// TODO: handle error
		close(uds);
		//fprintf(stderr, "failed to bind uds socket: %s\n", strerror(errno));
		return;
	}

	ssize_t recvSize;
	socklen_t sockLen = sizeof(sockInfo);
	SUpdatePacket* pkt;
	char buf[256] = { 0, };
	int* npTmp;
	while (1)
	{
		//printf("Wait to receive update request...\n");
		if ((recvSize = recvfrom(uds, buf, 4, 0, (struct sockaddr*)&sockInfo, &sockLen)) == -1)
		{
			Log(globResource.logger, LOG_FATAL, "Failed to receive UDS packet");
			continue;
		}
		npTmp = (int*)buf;
		if (*npTmp == UDS_UPDATE_PACKET)
		{
			Log(globResource.logger, LOG_DEBUG, "UDS: received update configuration request");
			if ((recvSize = recvfrom(uds, buf, 256, 0, (struct sockaddr*)&sockInfo, &sockLen)) == -1)
			{
				Log(globResource.logger, LOG_FATAL, "Failed to receive UDS packet");
				continue;
			}
			if (recvSize == 0)
			{
				Log(globResource.logger, LOG_INFO, "UDS: EOF");
				close(uds);
				return ;
			}
			buf[recvSize] = 0;
			pkt = (SUpdatePacket*)buf;

			// TODO: update configuration using values in pkt. not print
			// printf("Received informations\n");
			// printf("RUN_CPU_COLLECTOR: %s\n", (pkt->bRunCpuCollector ? "true" : "false"));
			// if (pkt->bRunCpuCollector)
			// 	printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->cpuPeriod);
			// printf("RUN_MEM_COLLECTOR: %s\n", (pkt->bRunCpuCollector ? "true" : "false"));
			// if (pkt->bRunCpuCollector)
			// 	printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->cpuPeriod);
			// printf("RUN_NET_COLLECTOR: %s\n", (pkt->bRunCpuCollector ? "true" : "false"));
			// if (pkt->bRunCpuCollector)
			// 	printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->cpuPeriod);
			// printf("RUN_PROC_COLLECTOR: %s\n", (pkt->bRunCpuCollector ? "true" : "false"));
			// if (pkt->bRunCpuCollector)
			// 	printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->cpuPeriod);
			// printf("RUN_DISK_COLLECTOR: %s\n", (pkt->bRunCpuCollector ? "true" : "false"));
			// if (pkt->bRunCpuCollector)
			// 	printf("CPU_COLLECTION_PERIOD: %lu ms\n", pkt->cpuPeriod);

			if (ResponseToClientUDS(pkt->udsPath, "update complete") != UDS_OK)
				Log(globResource.logger, LOG_FATAL, "Failed to UDS response");
		}
		else if (*npTmp == UDS_STATUS_PACKET)
		{
			Log(globResource.logger, LOG_DEBUG, "UDS: received update configuration request");
			if ((recvSize = recvfrom(uds, buf, 108, 0, (struct sockaddr*)&sockInfo, &sockLen)) == -1)
			{
				Log(globResource.logger, LOG_FATAL, "Failed to receive UDS packet");
				continue;
			}
			if (recvSize == 0)
			{
				Log(globResource.logger, LOG_INFO, "UDS: EOF");
				close(uds);
				return ;
			}
			buf[recvSize] = 0;
			SAgentStatusPacket pkt = { 0, };
			CreateStatusPacket(&pkt);
			if (ResponseToClientUDS(buf, (char*)&pkt) != UDS_OK)
				Log(globResource.logger, LOG_FATAL, "Failed to UDS response");
		}
		else
		{
			Log(globResource.logger, LOG_ERROR, "UDS: undefined request");
			continue;
		}
	}
}