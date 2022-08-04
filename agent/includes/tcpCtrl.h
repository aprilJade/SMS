#ifndef TCP_CTRL_H
#define TCP_CTRL_H

/*
 * Connect to server using host, port
 * and return connected socket
 * <return value>
 * Success: some socket that connected to server
 * Fail: return -1
 */
int ConnectToServer(char *host, short port);

#endif