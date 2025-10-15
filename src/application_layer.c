// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer link;
    
    strncpy(link.serialPort, serialPort, sizeof(link.serialPort) - 1);
    link.serialPort[sizeof(link.serialPort) - 1] = '\0';
    
    if (strcmp(role, "tx") == 0) {
        link.role = LlTx;
    } else if (strcmp(role, "rx") == 0) {
        link.role = LlRx;
    } else {
        printf("ERROR: Invalid role\n");
        return;
    }
    
    link.baudRate = baudRate;
    link.nRetransmissions = nTries;
    link.timeout = timeout;

    llopen(link);
}