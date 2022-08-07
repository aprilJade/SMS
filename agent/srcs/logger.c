#include "logger.h"

static char* logMsgs[KIND_OF_LOG] = {
    "try-connection",   // TRY_CONN
    "fail-connection",  // FAIL_CONN
    "connected",        // CONN
    "disconnected",     // DISCONN
    "received",         // RCV
    "send",             // SND
    "query",            // QRY
    "thread-created"    // THRD_CRT
};

static char* logProtocolStr[3] = {
    "TCP",
    "UDP",
    "DB"
};

int Log(Logger* handle, unsigned char signature, int msg, int protocol, int optionalFlag, void* optionValue)
{
    // 1. generate log message.
    // 2. insert that message to queue of handle.
    // 3. and.....
}