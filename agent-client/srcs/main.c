#include "udsCtrl.h"
#include "printer.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{   
    if (strcmp(argv[1], "update") == 0 && argc == 3)
        RequestUpdateConfigToAgent(argv[2]);
    else if (strcmp(argv[1], "status") == 0 && argc == 2)
        RequestStatusToAgent();
    else
        PrintUsage();

    exit(EXIT_SUCCESS);
}