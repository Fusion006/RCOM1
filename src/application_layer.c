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
    MessageType messageRcvd = NO_MSG;
    while(communicationStatus != EndC){
        if(link.role == LlRx){
            receiverProcess(link, &communicationStatus, &messageRcvd);
        }
        else{
            transmitterProcess(link, &communicationStatus, &messageRcvd);
        }
    }

    llclose();
}


int transmitterProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageType * messageRcvd){

    unsigned char data[BUF_SIZE] = {0};
    unsigned char packet[BUF_SIZE] = {0};
    int dataSize = 0;


    switch (*messageRcvd)
    {
        case NO_MSG:
            if(*communicationStatus == ClosedC){
                llwrite(data, dataSize, SET_MSG); //Atencao que isto não é definivito
                *communicationStatus = ConnectingC;
            }
            break;
        case UA_MSG:
            if (*communicationStatus == ConnectingC) {
                llwrite(data, dataSize, DISC_MSG); //Temporario obviamente
                *communicationStatus = DisconnectingC;
            }
            break;
    
        case DISC_MSG:
            if (*communicationStatus == DisconnectingC){
                *communicationStatus = EndC;
            }
            return 0;

        case INVALID_MSG: // ambos são invalidos para o transmissor
        case DATA_MSG:
            break;
        default:
            break;
    }


    *messageRcvd = llread(packet);
    
    return 0; 
}


int receiverProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageType * messaageRcvd){
    unsigned char data[BUF_SIZE] = {0};
    unsigned char packet[BUF_SIZE] = {0};
    int dataSize = 0;

    printf("Receiver Process Entered\n");        


    *messaageRcvd = llread(packet);
    
    sleep(1);

    switch (*messaageRcvd)
    {
        case NO_MSG: 
            break;
        case SET_MSG:
            if (*communicationStatus == ClosedC) {
                llwrite(data, dataSize, UA_MSG); 
                *communicationStatus = OpenC;
            }
            break;
        
        case DISC_MSG:
            if (*communicationStatus == OpenC){
                llwrite(data, dataSize, DISC_MSG); 
                *communicationStatus = EndC;
            }
        default:
            break;
    }
    return 0;
}

