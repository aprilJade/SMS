#include "udsCtrl.h"
#include "printer.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{   
    if (argc == 3)
    {
        if (strcmp(argv[1], "update") == 0)
            RequestUpdateConfigToAgent(argv[2]);
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "status") == 0)
            RequestStatusToAgent();
    }
    else
        PrintUsage();
    exit(EXIT_SUCCESS);
}