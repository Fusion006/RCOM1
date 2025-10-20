// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer link;
    int messageToSend = FALSE;
    
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

    CommunicationStatus communicationStatus = ClosedC;
    MessageRcvd messaageRcvd = NO_MSG;
    while(communicationStatus != EndC){
        if(link.role == LlRx){
            receiverProcess(link, &communicationStatus, &messaageRcvd);
        }
        else{
            transmitterProcess(link, &communicationStatus, &messaageRcvd);
        }
    }

    llclose();
}


int transmitterProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageRcvd * messaageRcvd){

    unsigned char buf[BUF_SIZE] = {0};
    unsigned char packet[BUF_SIZE] = {0};
    
    switch (*messaageRcvd)
    {
        case NO_MSG:
            if(*communicationStatus == ClosedC){
                buf[0] = F;
                buf[1] = A_TX;
                buf[2] = C_SET;
                buf[3] = buf[1] ^ buf[2];
                buf[4] = F;
                llwrite(buf, 5);
                *communicationStatus = ConnectingC;
            }
            break;
        case UA_MSG:
            if (*communicationStatus == ConnectingC) {
                buf[0] = F;
                buf[1] = A_TX;
                buf[2] = C_DISC; // teste
                buf[3] = buf[1] ^ buf[2];
                buf[4] = F;
                llwrite(buf, 5);
                *communicationStatus = DisconnectingC;
            }
            break;
        
        case DISC_MSG:
            if (*communicationStatus == DisconnectingC){
                *communicationStatus = EndC;
            }
        default:
            break;
    }

    *messaageRcvd = llread(packet);
    return 0; 
}


int receiverProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageRcvd * messaageRcvd){
    unsigned char buf[BUF_SIZE] = {0};
    unsigned char packet[BUF_SIZE] = {0};

    *messaageRcvd = llread(packet);
    

    switch (*messaageRcvd)
    {
        case NO_MSG: 
            break;
        case SET_MSG:
            if (*communicationStatus == ClosedC) {
                buf[0] = F;
                buf[1] = A_TX;
                buf[2] = C_UA;
                buf[3] = buf[1] ^ buf[2];
                buf[4] = F;
                llwrite(buf, 5);
                *communicationStatus = OpenC;
            }
            break;
        
        case DISC_MSG:
            if (*communicationStatus == OpenC){
                buf[0] = F;
                buf[1] = A_TX;
                buf[2] = C_DISC;
                buf[3] = buf[1] ^ buf[2];
                buf[4] = F;
                llwrite(buf, 5);
                *communicationStatus = EndC;
            }
        default:
            break;
    }
    return 0;
}

