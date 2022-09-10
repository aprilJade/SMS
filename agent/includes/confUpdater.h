#ifndef CONF_UPDATER_H
#define CONF_UPDATER_H

// CHECK: modify below path when deploy SMS
#define UDS_SOCKET_PATH "/home/apriljade/repo/SMS/bin/.agent.sock"

typedef enum e_UdsError
{
	UDS_OK = 0,
	UDS_SOCKET_OPEN_ERROR,
	UDS_RESPONSE_ERROR,
	UDS_RECEIVE_ERROR,
	UDS_UNDEFINED_PACKET_ERROR
} e_UdsError;

e_UdsError ResponseToClientUDS(const char* path, char* data);
void ManageAgentConfiguration(void);

#endif