#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "hookRoutine.h"


ssize_t write(int fd, const void* buf, size_t count)
{
    static pthread_t tid = 0;
    static ssize_t (*orgWrite)(int, const void*, size_t) = 0;

    if (orgWrite == NULL)
        orgWrite = (ssize_t (*)(int, const void*, size_t))dlsym(RTLD_NEXT, "write");

    if (tid == 0)
    {   
        // 1. create thread...?
        if (pthread_create(&tid, NULL, hookRoutine, NULL) == -1)
        {
            // TODO: Handle err
            return -1;
        }
        // 2. connection check...?
        // 3. queuing...?
        // 4. tcp sending...?
    }
    return orgWrite(fd, buf, count);
}