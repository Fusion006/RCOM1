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
    char data[BUF_SIZE] = {0};
    while(communicationStatus != EndC){
        if(link.role == LlRx){
            receiverProcess(link, &communicationStatus, &messageRcvd);
        }
        else{
            transmitterProcess(link, &communicationStatus, &messageRcvd, ""); // alterar
        }
    }

    llclose();
}


int transmitterProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageType * messageRcvd, const char * data){

    unsigned char packet[BUF_SIZE] = {0};
    int dataSize = 0;

    int timedOut = ACTIVE;

    MessageType messageCntrl = *messageRcvd;
    CommunicationStatus communicationStatusCtrl = *communicationStatus;
    do {
        timedOut = ACTIVE;
        switch (messageCntrl)
        {
            case NO_MSG:
                if(communicationStatusCtrl == ClosedC){
                    llwrite(data, dataSize, SET_MSG, LlTx); 
                    //llwrite(data, dataSize, I0_MSG, LlTx); //Atencao que isto não é definivito
                    *communicationStatus = ConnectingC;
                }
                break;
            case UA_MSG:
                if (communicationStatusCtrl == ConnectingC) {
                    llwrite(data, dataSize, I0_MSG, LlTx); //Atencao que isto não é definivito
                    *communicationStatus = OpenC;
                }
                break;
        
            case RR0_MSG:
                if (communicationStatusCtrl == OpenC) {
                    llwrite(data, dataSize, DISC_MSG /*I1_MSG */, LlTx); //Temporario obviamente
                    *communicationStatus = DisconnectingC;
                }
                break;

            case RR1_MSG:
                if (communicationStatusCtrl == OpenC) {
                    llwrite(data, dataSize, DISC_MSG /*I0_MSG */, LlRx); //Temporario obviamente
                    *communicationStatus = DisconnectingC;
                }
                break;
            case DISC_MSG:
                if (communicationStatusCtrl == DisconnectingC){
                    *communicationStatus = EndC;
                }
                return 0;

            case INVALID_MSG: // ambos são invalidos para o transmissor
            default:
                break;
        }
  
    }
    while (llread(packet, messageRcvd, &timedOut) || timedOut == TRUE);
        
    return 0; 
}


int receiverProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageType * messaageRcvd){
    unsigned char data[BUF_SIZE] = {0};
    unsigned char packet[BUF_SIZE] = {0};
    int dataSize = 0;

    printf("Receiver Process Entered\n");        


    int timedOut = FALSE;
    llread(packet,messaageRcvd,&timedOut); // checkar para o valor de retorno
    
    sleep(1);

    switch (*messaageRcvd)
    {
        case NO_MSG: 
            break;
        case SET_MSG:
            if (*communicationStatus == ClosedC) {
                llwrite(data, dataSize, UA_MSG, LlRx); 
                *communicationStatus = OpenC;
            }
            break;
        case I0_MSG:
            if (*communicationStatus == OpenC) {
                llwrite(data, dataSize, RR0_MSG, LlRx); 
                *communicationStatus = OpenC;
            }
            break;
        case I1_MSG:
            if (*communicationStatus == OpenC) {
                llwrite(data, dataSize, RR1_MSG, LlRx); 
                *communicationStatus = OpenC;
            }
            break;
        case DISC_MSG:
            if (*communicationStatus == OpenC){
                llwrite(data, dataSize, DISC_MSG, LlRx); 
                *communicationStatus = EndC;
            }
        default:
            break;
    }
    return 0;
}

