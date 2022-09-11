#ifndef PRINTER_H
#define PRINTER_H
#include "packets.h"


void PrintUsage();
void PrintPacketContent(SUpdatePacket* pkt);
void PrintAgentStatus(SAgentStatusPacket* pkt);

#endif