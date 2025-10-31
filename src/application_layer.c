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
    }
    else if (link.role == LlRx){
        fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);        
    }
    if (fd == -1){
            perror("Error creating file");
            return;
    }

    printf("Opened file %s successfully\n", filename);

    unsigned char data[BUF_SIZE] = {0};

    int startIndex = 0;
    int controlIndex = 0; //for rej
    int dataSize = 0;


    CommunicationStatus communicationStatus = ClosedC;
    MessageType messageRcvd = NO_MSG;
    //variavel controlo 
    while(communicationStatus != EndC){
        printf("Communication Status : %s\n", comStatus[communicationStatus]);
        if(link.role == LlRx){
            receiverProcess(link, &communicationStatus, &messageRcvd, fd);
        }
        else{
            transmitterProcess(link, &communicationStatus, &messageRcvd, fd, &startIndex, &controlIndex);
        }

    }

    close(fd);
    llclose();
}

int appendChunk(int fd, const unsigned char* buffer, int length) {
    if (fd < 0 || buffer == NULL) {
        return -1;
    }

    
    printf("APPENDING CHUNK: \n");
    for(int i = 0 ; i < length ; i++){
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    

    if (lseek(fd, 0, SEEK_END) == -1) {
        perror("Error seeking to end of file");
        return -1;
    }

    ssize_t bytesWritten = write(fd, buffer, length);
    if (bytesWritten < 0) {
        perror("Error writing to file");
        return -1;
    }

    return bytesWritten;
}

int extractChunk(int fd, int startIndex, unsigned char* buffer) {
    if (fd < 0 || buffer == NULL) {
        return -1;
    }
    
    if (lseek(fd, startIndex, SEEK_SET) == -1) {
        perror("Error seeking file");
        return -1;
    }
    
    int bytesRead = read(fd, buffer, BUF_SIZE - 7); //deixar espaço para flag bcc2
    
    if (bytesRead < 0) {
        perror("Error reading file");
        return -1;
    }

    printf("DATA TO SEND: \n");
    for(int i = 0 ; i < bytesRead ; i++){
        printf("%02x ", buffer[i]);
    }
    printf("\n");

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

int transmitterProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageType * messageRcvd, int fd, int * startIndex, int * controlIndex){

    unsigned char packet[BUF_SIZE] = {0};
    unsigned char data[BUF_SIZE] = {0};
    int dataSize = 0;
    int bytesWritten = 0;

    int timedOut = ACTIVE;
    int ignoredBytes = 0;

    MessageType messageCntrl = *messageRcvd;
    CommunicationStatus communicationStatusCtrl = *communicationStatus;
    int bytes = 0;
    do {
        if(timedOut == TRUE){
            printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
            *startIndex = *controlIndex;
        }
        timedOut = ACTIVE;
        
        printf("START INDEX : %d\n", *startIndex);
        printf("CONTROL INDEX : %d\n", *controlIndex);
        
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
                    memset(data, 0, BUF_SIZE);
                    dataSize = extractChunk(fd, *startIndex, data);
                    printf("DATASIZE TO SEND : %d\n", dataSize);
                    
                    if(dataSize == 0){
                        printf("TRANSFERENCE CONCLUDED|\n");
                        llwrite(data, dataSize, DISC_MSG, LlTx, NULL); 
                        communicationStatus = DisconnectingC;
                    }
                    else {
                        bytesWritten = llwrite(data, dataSize, I0_MSG, LlTx, &ignoredBytes); //Enviar nome do ficheiro 
                        *startIndex += dataSize - ignoredBytes;
                        *communicationStatus = OpenC;
                    }
                    
                }
                break;
        
            case RR0_MSG:
                *controlIndex = *startIndex;
                
                if (communicationStatusCtrl == OpenC) {
                    memset(data, 0, BUF_SIZE);
                    dataSize = extractChunk(fd, *startIndex, data);
                    printf("DATASIZE TO SEND : %d\n", dataSize);
                    
                    if(dataSize == 0){
                        printf("TRANSFERENCE CONCLUDED|\n");
                        llwrite(data, dataSize, DISC_MSG, LlTx, NULL); 
                        *communicationStatus = DisconnectingC;
                    }
                    else {
                        bytesWritten = llwrite(data, dataSize, I1_MSG, LlTx, &ignoredBytes);   
                        *startIndex += dataSize - ignoredBytes;

                    }   
                }
                if (communicationStatusCtrl == DisconnectingC){
                    llwrite(data, dataSize, DISC_MSG, LlTx, NULL); 
                    *communicationStatus = DisconnectingC;
                }
                
                break;

            case RR1_MSG:
                *controlIndex = *startIndex;
                
                if (communicationStatusCtrl == OpenC) {
                    memset(data, 0, BUF_SIZE);
                    dataSize = extractChunk(fd, *startIndex, data);
                    printf("DATASIZE TO SEND : %d\n", dataSize);
                    
                    if(dataSize == 0){
                        printf("TRANSFERENCE CONCLUDED|\n");
                        llwrite(data, dataSize, DISC_MSG, LlTx, NULL); 
                        *communicationStatus = DisconnectingC;
                    }
                    else {
                        bytesWritten = llwrite(data, dataSize, I0_MSG, LlTx, &ignoredBytes);                 
                        *startIndex += dataSize - ignoredBytes;
                    }
                }
                if (communicationStatusCtrl == DisconnectingC){
                    llwrite(data, dataSize, DISC_MSG, LlTx, NULL); 
                    *communicationStatus = EndC;
                }
                break;
            case REJ0_MSG:
                if (communicationStatusCtrl == OpenC) {  
                    *startIndex = *controlIndex;        
                    dataSize = extractChunk(fd, *startIndex, data);
                    *startIndex += dataSize - ignoredBytes;
                    bytesWritten = llwrite(data, dataSize, I0_MSG, LlTx, &ignoredBytes);                 
                }
                break;
            case REJ1_MSG:
                if (communicationStatusCtrl == OpenC) {    
                    *startIndex = *controlIndex;        
                    dataSize = extractChunk(fd, *startIndex, data);
                    *startIndex += dataSize - ignoredBytes;
                    bytesWritten = llwrite(data, dataSize, I1_MSG, LlTx, &ignoredBytes);                 
                }
                break;
            case DISC_MSG:
                if (communicationStatusCtrl == DisconnectingC){
                    llwrite(data, dataSize, UA_MSG, LlTx, NULL); 
                    *communicationStatus = EndC;
                }
                return 0;

            case INVALID_MSG: // ambos são invalidos para o transmissor
            default:
                break;
        }
  
    }
    while (llread(packet, messageRcvd, &timedOut, &bytes) || timedOut == TRUE);
        
    return 0; 
}


int receiverProcess(LinkLayer link, CommunicationStatus * communicationStatus, MessageType * messaageRcvd, int fd){
    unsigned char data[BUF_SIZE] = {0};
    unsigned char packet[BUF_SIZE] = {0};
    int dataSize = 0;

    //printf("Receiver Process Entered\n");        


    int timedOut = FALSE;
    int bytes = 0;
    llread(packet,messaageRcvd,&timedOut, &bytes); // checkar para o valor de retorno

    switch (*messaageRcvd)
    {
        case NO_MSG: 
            break;
        case INVALID_I0:
            llwrite(data, dataSize, REJ0_MSG, LlRx, NULL);
            break;
        case INVALID_I1:
            llwrite(data, dataSize, REJ1_MSG , LlRx, NULL);
            break;
        case SET_MSG:
            if (*communicationStatus == ClosedC) {
                llwrite(data, dataSize, UA_MSG, LlRx, NULL); 
                *communicationStatus = OpenC;
            }
            break;
        case I0_MSG:
            if (*communicationStatus == OpenC) {
                appendChunk(fd, packet, bytes);
                printf("APPENDING %d BYTES\n", bytes);
                llwrite(data, dataSize, RR0_MSG, LlRx, NULL); 
            }
            break;
        case I1_MSG:
            if (*communicationStatus == OpenC) {
                appendChunk(fd, packet, bytes);
                printf("APPENDING %d BYTES\n", bytes);
                llwrite(data, dataSize, RR1_MSG, LlRx, NULL); 
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

