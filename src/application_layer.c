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

    int fd = -1;
    if (link.role == LlTx){
        fd = open(filename, O_RDONLY);
        if (fd == -1){
            perror("Error creating file");
            return;
        }
        printf("Opened file %s successfully\n");
    }

    unsigned char data[BUF_SIZE] = {0};

    int dataSize = extractChunk(fd, 0, data);

    CommunicationStatus communicationStatus = ClosedC;
    MessageType messageRcvd = NO_MSG;
    //variavel controlo 
    int startIndex = 0;
    while(communicationStatus != EndC){
        if(link.role == LlRx){
            receiverProcess(link, &communicationStatus, &messageRcvd);
        }
        else{ 
            //transmitterProcess(link, &communicationStatus, &messageRcvd, fd, &startIndex);
        }
    }

    llclose();
}


int extractChunk(int fd, int startIndex, unsigned char* buffer) {
    if (fd < 0 || buffer == NULL) {
        return -1;
    }
    
    // Seek to startIndex position in file
    if (lseek(fd, startIndex, SEEK_SET) == -1) {
        perror("Error seeking file");
        return -1;
    }
    
    // Read up to BUF_SIZE bytes
    int bytesRead = read(fd, buffer, BUF_SIZE);
    
    if (bytesRead < 0) {
        perror("Error reading file");
        return -1;
    }
    
    // Null-terminate if treating as string (optional, depends on use case)
    if (bytesRead < BUF_SIZE) {
        buffer[bytesRead] = '\0';
    }
    
    return bytesRead;
}



int createFile(const char *filename) {
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    
    if (fd == -1) {
        perror("Error creating file");
        return -1;
    }

    return fd;
}

int transmitterProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageType * messageRcvd, int fd, int * startIndex){

    unsigned char packet[BUF_SIZE] = {0};
    unsigned char data[BUF_SIZE] = {0};
    int dataSize = 0;
    int bytesWritten = 0;

    int timedOut = ACTIVE;
    int ignoredBytes = 0;

    MessageType messageCntrl = *messageRcvd;
    CommunicationStatus communicationStatusCtrl = *communicationStatus;
    do {
        timedOut = ACTIVE;
        switch (messageCntrl)
        {
            case NO_MSG:
                if(communicationStatusCtrl == ClosedC){
                    llwrite(data, dataSize, SET_MSG, LlTx, NULL); 
                    *communicationStatus = ConnectingC;
                }
                break;
            case UA_MSG:
                if (communicationStatusCtrl == ConnectingC) {
                    dataSize = extractChunk(fd, *startIndex, data);
                    bytesWritten = llwrite(data, dataSize, I0_MSG, LlTx, &ignoredBytes); //Enviar nome do ficheiro 
                    *communicationStatus = OpenC;
                }
                break;
        
            case RR0_MSG:
                if (communicationStatusCtrl == OpenC) {
                    bytesWritten = llwrite(data, dataSize, DISC_MSG /*I1_MSG */, LlTx, &ignoredBytes); //Temporario obviamente
                    *communicationStatus = DisconnectingC;
                }
                break;

            case RR1_MSG:
                if (communicationStatusCtrl == OpenC) {
                    bytesWritten = llwrite(data, dataSize, DISC_MSG /*I0_MSG */, LlRx, &ignoredBytes); //Temporario obviamente
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

    *startIndex += dataSize - ignoredBytes;
        
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
                llwrite(data, dataSize, UA_MSG, LlRx, NULL); 
                *communicationStatus = OpenC;
            }
            break;
        case I0_MSG:
            if (*communicationStatus == OpenC) {
                llwrite(data, dataSize, RR0_MSG, LlRx, NULL); 
                *communicationStatus = OpenC;
            }
            break;
        case I1_MSG:
            if (*communicationStatus == OpenC) {
                llwrite(data, dataSize, RR1_MSG, LlRx, NULL); 
                *communicationStatus = OpenC;
            }
            break;
        case DISC_MSG:
            if (*communicationStatus == OpenC){
                llwrite(data, dataSize, DISC_MSG, LlRx, NULL); 
                *communicationStatus = EndC;
            }
        default:
            break;
    }
    return 0;
}

