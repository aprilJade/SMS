#ifndef TCP_CTRL_H
#define TCP_CTRL_H

int ConnectToServer(const char* host, short port);
int ParseHost(char* input, char* host, short* port);

#endif