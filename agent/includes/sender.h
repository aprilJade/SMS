#ifndef SENDER_H
#define SENDER_H

#define RECONNECT_PERIOD 5    // seconds
#define RECONNECT_MAX_TRY 100

void* SendRoutine(void* param);

#endif