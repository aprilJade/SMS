#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>

// CHECK: modify below path when deploy SMS
#define UDS_SOCKET_PATH "./bin/agent.sock"

void PrintUsage()
{
    fprintf(stderr, "Not implemented yet: PrintUsage()\n");
}

int main(int argc, char** argv)
{   
    if (argc == 1)
    {
        PrintUsage();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "update") == 0)
    {
        if (argc != 3)
        {
            PrintUsage();
            return EXIT_FAILURE;
        }
        // #00. Send request to update configuration to agent
        // #01. Wait agent's answer that whether update is success or not
        // #02. Print announcement message about update
        // #03. Exit  
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
        return EXIT_FAILURE;
    }

    return 0;
}