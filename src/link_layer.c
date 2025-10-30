// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>



////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // Open serial port device for reading and writing
    if (openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate) < 0)
    {
        perror("openSerialPort");
        return -1;
    }

    printf("Serial port %s opened\n", connectionParameters.serialPort);

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *data, int dataSize, const MessageType messageType, LinkLayerRole linkLayerRole, int * ignoredBytes)
{
    int numStuffs = 0;
    unsigned char buf[BUF_SIZE] = {0};
    int bytes = 0;

    printf("\nSending message: %s\n\n", msgs_[messageType]);

    if(messageType == I0_MSG || messageType == I1_MSG){
        if(!packetBuilder(messageType, buf, BUF_SIZE, data, dataSize, linkLayerRole, &numStuffs, ignoredBytes)){
            bytes = writeBytesSerialPort(buf, dataSize + 6);
        }
    }
    else {
        if(!packetBuilder(messageType, buf, 5, NULL, 0, linkLayerRole, &numStuffs, ignoredBytes)){
            bytes = writeBytesSerialPort(buf, 5);
        }
    }
    

    return bytes;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet, MessageType * messageRcvd, int * timedOut, int * bytes)
{

    int alarmActive = FALSE;
    if (*timedOut == ACTIVE){
        struct sigaction act = {0};
        act.sa_handler = &alarmHandler;
        if (sigaction(SIGALRM, &act, NULL) == -1)
        {
            perror("sigaction");
            exit(1);
        }

        alarm(3);
        alarmEnabled = TRUE;
        alarmActive = TRUE;

        //printf("Alarm configured\n");

        *timedOut = FALSE; 
    }

    //printf("llread entered \n");
    int numOfStuffingOps = 0;
    int nBytesBuf = 0;
    int counter = 0;
    int msg_counter = 0;
    enum State currentState = Start;
    unsigned char receivedA = 0;
    unsigned char receivedC = 0;
    unsigned char dataBuffer[BUF_SIZE + 1] = {0};
    int dataIndex = 0;
    unsigned char calculatedBCC2 = 0;
    int retransmissionCount = 0;
    STOP = FALSE;    
    int stuffingActive = FALSE;
    

    unsigned char beforeRcvdByte = 0x0;

    while(STOP == FALSE && (alarmActive == alarmEnabled)){
        unsigned char byte;
        int nBytes = readByteSerialPort(&byte);            
        nBytesBuf += nBytes;

        //printf("Current State : %s\n", states[currentState]);
        //printf("Byte received: 0x%02X\n", byte);
        

        switch(currentState){
            case Start:
                if (byte == F){
                    currentState = FLAG;
                }
                break;
                
            case FLAG:
                if (byte == F) break;
                else if (byte == A_TX || byte == A_RX){
                    receivedA = byte;
                    currentState = A;
                }
                else {
                    currentState = Start;
                }
                break;
                
            case A:
                if (byte == F) {
                    currentState = FLAG;
                }
                // Accept any valid control field value
                else if (byte == C_SET || byte == C_DISC || byte == C_UA || 
                            byte == C_RR_0 || byte == C_RR_1 || 
                            byte == C_REJ_0 || byte == C_REJ_1 ||
                            byte == C_I_0 || byte == C_I_1) {
                    receivedC = byte;
                    //printf("Control field received: 0x%02X\n", byte);
                    currentState = C;
                }
                else {
                    //printf("Unknown control field: 0x%02X, ignoring frame\n", byte);
                    currentState = Start;
                }
                break;

            case C:
                if (byte == F) {
                    currentState = FLAG;
                }
                else if (byte == (receivedA ^ receivedC)) {
                    //printf("BCC1 correct!\n");
                    
                    // Determine frame type and expected structure
                    if (receivedC == C_I_0 || receivedC == C_I_1) {
                        // Information frame - expect data field
                      //  printf("I-frame detected, expecting data field\n");
                        currentState = BCC1;
                    }
                    else if (receivedC == C_SET || receivedC == C_DISC || receivedC == C_UA ||
                                receivedC == C_RR_0 || receivedC == C_RR_1 ||
                                receivedC == C_REJ_0 || receivedC == C_REJ_1) {
                        // Supervision frame - no data field expected
                        //printf("Supervision frame detected, no data field\n");
                        currentState = BCC1;
                    }
                    else {
                        currentState = BCC1;
                    }
                }
                else {
                    //printf("BCC1 error! Expected 0x%02X, got 0x%02X\n", (receivedA ^ receivedC), byte);
                    //printf("Header error detected - ignoring frame without action\n");
                    // According to procedures: frames with wrong header are ignored
                    currentState = Start;
                }
                break;
                
            case BCC1:
                if (byte == F) {
                    // Frame complete - this is a supervision frame (no data field)
                    //printf("Supervision frame complete\n");
                    
                    // Process supervision frames
                    if (receivedC == C_SET) {
                        //printf("SET received - sending UA response\n");
                        *messageRcvd = SET_MSG;
                    }
                    else if (receivedC == C_DISC) {
                        //printf("DISC received - sending DISC then UA response\n");
                        *messageRcvd = DISC_MSG;
                    }
                    else if (receivedC == C_UA) {
                        *messageRcvd = UA_MSG;
                        //printf("UA received - acknowledgment confirmed\n");
                    }
                    else if (receivedC == C_RR_0 || receivedC == C_RR_1) {
                        int ackFrame = (receivedC == C_RR_1) ? 1 : 0;
                        if(receivedC == C_RR_0){
                            *messageRcvd = RR0_MSG;
                        }
                        else {
                            *messageRcvd = RR1_MSG;
                        }
                        //printf("RR(%d) received - positive acknowledgment\n", ackFrame);
                    }
                    else if (receivedC == C_REJ_0 || receivedC == C_REJ_1) {
                        int rejFrame = (receivedC == C_REJ_1) ? 1 : 0;
                        //printf("REJ(%d) received - retransmission requested\n", rejFrame);
                    }
                    
                    STOP = TRUE;
                }
                else {
                    // Information frame - start receiving data
                    if (receivedC == C_I_0 || receivedC == C_I_1) {
                        
                        if(receivedC == C_I_0){
                            *messageRcvd = I0_MSG;
                        }
                        else if(receivedC == C_I_1){
                            *messageRcvd =  I1_MSG;
                        }
                        //printf("Starting data reception for I-frame\n");
                        dataBuffer[dataIndex++] = byte;
                        calculatedBCC2 ^= byte;
                        printf("CALCULATED BCC2 : %02x BY XORING : %02x\n", calculatedBCC2, byte);
                        currentState = DATA;
                    }
                    else {
                        // Supervision frame shouldn't have data - error
                        //printf("Error: Supervision frame has data field!\n");
                        currentState = Start; 
                        // STOP = TRUE;
                        // *messageRcvd = INVALID_MSG;
                        // printf("Frame processing terminated due to error\n");
                    }
                }
                break;
                
            case DATA:
                if (byte == F) {
                    calculatedBCC2 ^= beforeRcvdByte;
                    printf("CALCULATED BCC2 : %02x BY XORING : %02x\n", calculatedBCC2, beforeRcvdByte);


                    printf("\nReceived BCC2 : %#08x\n",beforeRcvdByte);
                    printf("Calculated BCC2 : %#08x\n",calculatedBCC2);


                    dataIndex--;
                    printf("SETTING %02x TO 0\n", dataBuffer[dataIndex]);
                    dataBuffer[dataIndex] = 0;

                    if (beforeRcvdByte == calculatedBCC2) {
                        printf("Correct BCC2\n");                        
                        memcpy(packet, dataBuffer, dataIndex);
                        STOP = TRUE;
                    }
                    else {
                        printf("Incorrect BCC2\n");  
                        memcpy(packet, dataBuffer, dataIndex);
                        STOP = TRUE;
                    }
                }
                else if (byte == STUFFING_BYTE) {
                    stuffingActive = TRUE;
                }
                else if (byte == 0x5e && stuffingActive){
                    dataBuffer[dataIndex] = F;
                    // Store byte and update running XOR
                    //printf("XORING %02x\n", F);
                    calculatedBCC2 ^= F; 
                    printf("CALCULATED BCC2 : %02x BY XORING : %02x\n", calculatedBCC2, F);
                    stuffingActive = FALSE;
                    dataIndex++;
                }
                else if (byte == 0x5d && stuffingActive){
                    dataBuffer[dataIndex] = STUFFING_BYTE;
                    // Store byte and update running XOR
                    calculatedBCC2 ^= STUFFING_BYTE; 
                    printf("CALCULATED BCC2 : %02x BY XORING : %02x\n", calculatedBCC2, STUFFING_BYTE);
                    stuffingActive = FALSE;
                    dataIndex++;
                }
                else {
                    dataBuffer[dataIndex] = byte; 
                    // Store byte and update running XOR
                    calculatedBCC2 ^= byte;
                    printf("CALCULATED BCC2 : %02x BY XORING : %02x\n", calculatedBCC2, byte);
                    dataIndex++;
                }
                break;
        }

        beforeRcvdByte = byte;
    }

    if (alarmActive && !alarmEnabled){
        *timedOut = TRUE;
        printf("TIME OUT!\n\n");
        return 0;
    }

    //printf("Frame processing complete\n");



    printf("Total bytes received: %d\n", nBytesBuf);

    printf("\n\nReceived Message : %s\n\n", msgs_[*messageRcvd]);
    
    if (bytes != NULL){
        *bytes = nBytesBuf - 6;
    }


    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{
       // Close serial port
    if (closeSerialPort() < 0)
    {
        perror("closeSerialPort");
        exit(-1);
    }

    return 0;
}

////////////////////////////////////////////////
// PACKET BUILDER
////////////////////////////////////////////////

int packetBuilder(const MessageType messageType, unsigned char * buf, int bufSize, const unsigned char * data, int dataSize, LinkLayerRole role, int * numStuffs, int * ignoredBytes){
    

    unsigned char bcc2 = 0;

    unsigned char stuffedBuf[BUF_SIZE] = {0};

    int stuffedBufSize = stuffing(data, stuffedBuf, dataSize, numStuffs, ignoredBytes);
    
    switch (messageType)
    {
    case SET_MSG:
        if(bufSize < 5){
            return -1;
        }

        buf[0] = F;
        if(role == LlTx) buf[1] = A_TX;
        else return -1;
        buf[2] = C_SET;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    case RR0_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        if(role == LlRx) buf[1] = A_RX;
        else return -1;
        buf[2] = C_RR_0;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    case RR1_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        if(role == LlRx) buf[1] = A_RX;
        else return -1;
        buf[2] = C_RR_1;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    case UA_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        if(role == LlRx) buf[1] = A_RX;
        else buf[1] = A_TX;
        buf[2] = C_UA;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    case I0_MSG:
        if(dataSize > BUF_SIZE - 6 && bufSize < dataSize + 6){
            return -1;
        }
        buf[0] = F;
        if(role == LlTx) buf[1] = A_TX;
        else return -1;
        buf[2] = C_I_0;
        buf[3] = buf[1] ^ buf[2];
        
        // Copy data and calculate BCC2
        for(int i = 0 ; i < stuffedBufSize ; i++){
            buf[i + 4] = stuffedBuf[i];
        }
        buf[4 + stuffedBufSize] = 0; // Initialize BCC2 position
        for(int i = 0 ; i < dataSize - *ignoredBytes ; i++){
             bcc2 ^= data[i]; // BCC2 is at position 4 + dataSize
            printf("TRANSMITTER CALCULATED BCC2 : %02x BY XORING : %02x\n", bcc2, data[i]);

        }
        if(bcc2 == F){
            buf[4 + stuffedBufSize] = STUFFING_BYTE;
            buf[4 + stuffedBufSize + 1] = 0x5e;
            buf[4 + stuffedBufSize + 2] = F; // Final FLAG after BCC2
        }
        else if(bcc2 == STUFFING_BYTE){
            buf[4 + stuffedBufSize] = STUFFING_BYTE;
            buf[4 + stuffedBufSize + 1] = 0x5d;
            buf[4 + stuffedBufSize + 2] = F; // Final FLAG after BCC2
        }
        else {
            buf[4 + stuffedBufSize] = bcc2;
            buf[4 + stuffedBufSize + 1] = F; // Final FLAG after BCC2

        }

        break;
    case I1_MSG:
        if(dataSize > BUF_SIZE - 6 && bufSize < dataSize + 6){
            return -1;
        }
        buf[0] = F;
        if(role == LlTx) buf[1] = A_TX;
        else return -1;
        buf[2] = C_I_1; 
        buf[3] = buf[1] ^ buf[2];
        
        // Copy data and calculate BCC2
        for(int i = 0 ; i < stuffedBufSize ; i++){
            buf[i + 4] = stuffedBuf[i];
        }
        buf[4 + stuffedBufSize] = 0; // Initialize BCC2 position
        for(int i = 0 ; i < dataSize - *ignoredBytes ; i++){
             bcc2 ^= data[i]; // BCC2 is at position 4 + dataSize
            printf("TRANSMITTER CALCULATED BCC2 : %02x BY XORING : %02x\n", bcc2, data[i]);

        }
        if(bcc2 == F){
            buf[4 + stuffedBufSize] = STUFFING_BYTE;
            buf[4 + stuffedBufSize + 1] = 0x5e;
            buf[4 + stuffedBufSize + 2] = F; // Final FLAG after BCC2
        }
        else if(bcc2 == STUFFING_BYTE){
            buf[4 + stuffedBufSize] = STUFFING_BYTE;
            buf[4 + stuffedBufSize + 1] = 0x5d;
            buf[4 + stuffedBufSize + 2] = F; // Final FLAG after BCC2
        }
        else {
            buf[4 + stuffedBufSize] = bcc2;
            buf[4 + stuffedBufSize + 1] = F; // Final FLAG after BCC2

        }

        break;
    case REJ0_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        if(role == LlRx) buf[1] = A_RX;
        else return -1;
        buf[2] = C_REJ_0;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    case REJ1_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        if(role == LlRx) buf[1] = A_RX;
        else return -1;
        buf[2] = C_REJ_1;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    case DISC_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        if(role == LlRx) buf[1] = A_RX;
        else buf[1] = A_TX;
        buf[2] = C_DISC;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    default:
        break;
    }
    
   printf("STUFFED BUFFER\n");
    for (int i = 0 ; i < bufSize ; i++){
        printf("%02x ", buf[i]);
    }
       

    /* printf("STUFFED BUFFER\n");
    for (int i = 0 ; i < stuffedBufSize ; i++){
        printf("%02x ", stuffedBuf[i]);
    } */
    
    return 0;
}

void alarmHandler(int signal){
    alarmEnabled = FALSE;
}


int stuffing(const unsigned char * data, unsigned char * buffer, const int dataSize, int * numStuffs, int * ignoredBytes){ //returns number of stuffs
    
    int buffSize = 0;

    for(int i = 0, j = 0 ; i < dataSize && j < dataSize ; i++, j++){

        if(data[j] == F){
            buffer[i] = STUFFING_BYTE;
            buffer[++i] = 0x5e;
            (*numStuffs)++;
            buffSize++;
        }
        else if(data[j] == STUFFING_BYTE){
            buffer[i] = STUFFING_BYTE;
            buffer[++i] = 0x5d;
            (*numStuffs)++;
            buffSize++;
        }
        else {
            buffer[i] = data[j];
        }
        buffSize++;
    }

    if(ignoredBytes != NULL){
        if (dataSize + *numStuffs > BUF_SIZE){
            *ignoredBytes = dataSize + *numStuffs - BUF_SIZE;
        }
        else {
            *ignoredBytes = 0;
        }
    }
    
    //printf("Stuffed Buf Size : %d\n", buffSize);

    //printf("STUFFED BUF : %s\n", buffer);

    printf("NUM STUFFS : %d\n", *numStuffs);

    return buffSize;

}


