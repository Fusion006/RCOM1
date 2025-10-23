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
int llwrite(const unsigned char *data, int dataSize, const MessageType messageType)
{
    
    unsigned char buf[BUF_SIZE] = {0};

    printf("\nSending message: %s\n\n", msgs_[messageType]);

    if(!packetBuilder(messageType, buf, BUF_SIZE)){
        int bytes = writeBytesSerialPort(buf, BUF_SIZE);
    }

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet, MessageType * messageRcvd, int * timedOut)
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

        printf("Alarm configured\n");

        *timedOut = FALSE; 
    }

    printf("llread entered \n");
    int nBytesBuf = 0;
    int counter = 0;
    int msg_counter = 0;
    enum State currentState = Start;
    unsigned char receivedA = 0;
    unsigned char receivedC = 0;
    unsigned char dataBuffer[MAX_PAYLOAD_SIZE];
    int dataIndex = 0;
    unsigned char calculatedBCC2 = 0;
    int retransmissionCount = 0;
    STOP = FALSE;    

    while(STOP == FALSE && (alarmActive == alarmEnabled)){
        unsigned char byte;
        printf("HERE\n");
        int bytes = readByteSerialPort(&byte);            
        nBytesBuf += bytes;

        printf("Current State : %s\n", states[currentState]);
        printf("Byte received: 0x%02X\n", byte);

        switch(currentState){
            case Start:
                if (byte == F){
                    currentState = FLAG;
                }
                break;
                
            case FLAG:
                if (byte == F) break;
                else if (byte == A_TX){
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
                    printf("Control field received: 0x%02X\n", byte);
                    currentState = C;
                }
                else {
                    printf("Unknown control field: 0x%02X, ignoring frame\n", byte);
                    currentState = Start;
                }
                break;

            case C:
                if (byte == F) {
                    currentState = FLAG;
                }
                else if (byte == (receivedA ^ receivedC)) {
                    printf("BCC1 correct!\n");
                    
                    // Determine frame type and expected structure
                    if (receivedC == C_I_0 || receivedC == C_I_1) {
                        // Information frame - expect data field
                        printf("I-frame detected, expecting data field\n");
                        currentState = BCC1;
                    }
                    else if (receivedC == C_SET || receivedC == C_DISC || receivedC == C_UA ||
                                receivedC == C_RR_0 || receivedC == C_RR_1 ||
                                receivedC == C_REJ_0 || receivedC == C_REJ_1) {
                        // Supervision frame - no data field expected
                        printf("Supervision frame detected, no data field\n");
                        currentState = BCC1;
                    }
                    else {
                        currentState = BCC1;
                    }
                }
                else {
                    printf("BCC1 error! Expected 0x%02X, got 0x%02X\n", (receivedA ^ receivedC), byte);
                    printf("Header error detected - ignoring frame without action\n");
                    // According to procedures: frames with wrong header are ignored
                    currentState = Start;
                }
                break;
                
            case BCC1:
                if (byte == F) {
                    // Frame complete - this is a supervision frame (no data field)
                    printf("Supervision frame complete\n");
                    
                    // Process supervision frames
                    if (receivedC == C_SET) {
                        printf("SET received - sending UA response\n");
                        *messageRcvd = SET_MSG;
                    }
                    else if (receivedC == C_DISC) {
                        printf("DISC received - sending DISC then UA response\n");
                        *messageRcvd = DISC_MSG;
                    }
                    else if (receivedC == C_UA) {
                        *messageRcvd = UA_MSG;
                        printf("UA received - acknowledgment confirmed\n");
                    }
                    else if (receivedC == C_RR_0 || receivedC == C_RR_1) {
                        int ackFrame = (receivedC == C_RR_1) ? 1 : 0;
                        printf("RR(%d) received - positive acknowledgment\n", ackFrame);
                    }
                    else if (receivedC == C_REJ_0 || receivedC == C_REJ_1) {
                        int rejFrame = (receivedC == C_REJ_1) ? 1 : 0;
                        printf("REJ(%d) received - retransmission requested\n", rejFrame);
                    }
                    
                    STOP = TRUE;
                }
                else {
                    // Information frame - start receiving data
                    if (receivedC == C_I_0 || receivedC == C_I_1) {
                        printf("Starting data reception for I-frame\n");
                        dataBuffer[dataIndex++] = byte;
                        calculatedBCC2 = byte;
                        currentState = DATA;
                    }
                    else {
                        // Supervision frame shouldn't have data - error
                        printf("Error: Supervision frame has data field!\n");
                        currentState = Start;
                    }
                }
                break;
                
            case DATA:
                if (byte == F) {
                    // Shouldn't happen - missing BCC2
                    printf("Error: Missing BCC2!\n");
                    currentState = Start;
                }
                else {
                    // Check if this could be BCC2 by trying to validate
                    // For now, we need to know when data ends
                    // Simple approach: assume next byte after data is BCC2
                    // We'll validate in BCC2 state
                    dataBuffer[dataIndex] = byte;
                    
                    // Check if this byte XORed with previous equals 0 (potential BCC2)
                    unsigned char testBCC2 = calculatedBCC2;
                    for (int i = dataIndex; i < dataIndex + 1; i++) {
                        testBCC2 ^= dataBuffer[i];
                    }
                    
                    // Store byte and update running XOR
                    calculatedBCC2 ^= byte;
                    dataIndex++;
                    
                    // Move to BCC2 state to check next byte
                    currentState = BCC2;
                }
                break;
                
            case BCC2:
                if (byte == F) {
                    // Last data byte was actually BCC2
                    dataIndex--; // Remove BCC2 from data
                    unsigned char receivedBCC2 = dataBuffer[dataIndex];
                    
                    // Recalculate BCC2 without the last byte
                    calculatedBCC2 = 0;
                    for (int i = 0; i < dataIndex; i++) {
                        calculatedBCC2 ^= dataBuffer[i];
                    }
                    
                    // Determine the received frame sequence number
                    int receivedFrameNumber = (receivedC == C_I_1) ? 1 : 0;
                    
                    if (receivedBCC2 == calculatedBCC2) {
                        // No errors in header or data field
                        printf("BCC2 correct! Data received successfully.\n");
                        printf("Received %d data bytes\n", dataIndex);
                        
                        // Check if this is a new frame or duplicate
                        if (receivedFrameNumber == expectedFrameNumber) {
                            // New frame - accept data and pass to Application
                            printf("New frame I(%d) accepted - passing to Application\n", receivedFrameNumber);
                            // Copy data to packet for application layer
                            memcpy(packet, dataBuffer, dataIndex);
                            
                            // Toggle expected frame number for next transmission
                            expectedFrameNumber = 1 - expectedFrameNumber;
                            
                            // Send RR with next expected frame number
                            unsigned char rrControl = (expectedFrameNumber == 0) ? C_RR_0 : C_RR_1;
                            unsigned char rrBuf[5] = {FLAG, A_RX, rrControl, A_RX ^ rrControl, FLAG};
                            //writeBytesSerialPort(rrBuf, 5);
                            printf("Sent RR(%d) - ready for next frame\n", expectedFrameNumber);
                        }
                        else {
                            // Duplicate frame - discard data but confirm with RR
                            printf("Duplicate frame I(%d) detected - discarding data\n", receivedFrameNumber);
                            
                            // Send RR with next expected frame number (same as before)
                            unsigned char rrControl = (expectedFrameNumber == 0) ? C_RR_0 : C_RR_1;
                            unsigned char rrBuf[5] = {FLAG, A_RX, rrControl, A_RX ^ rrControl, FLAG};
                            //writeBytesSerialPort(rrBuf, 5);
                            printf("Sent RR(%d) - confirming duplicate\n", expectedFrameNumber);
                        }
                        
                        STOP = TRUE;
                        alarm(0); // Cancel alarm
                    }
                    else {
                        // BCC2 error - no header error but data field error
                        printf("BCC2 error! Expected 0x%02X, got 0x%02X\n", calculatedBCC2, receivedBCC2);
                        
                        // Check if new frame or duplicate
                        if (receivedFrameNumber == expectedFrameNumber) {
                            // New frame with error - send REJ to request retransmission
                            printf("New frame I(%d) with data error - sending REJ\n", receivedFrameNumber);
                            unsigned char rejControl = (expectedFrameNumber == 0) ? C_REJ_0 : C_REJ_1;
                            unsigned char rejBuf[5] = {FLAG, A_RX, rejControl, A_RX ^ rejControl, FLAG};
                            //writeBytesSerialPort(rejBuf, 5);
                            printf("Sent REJ(%d) - requesting retransmission\n", expectedFrameNumber);
                        }
                        else {
                            // Duplicate with error - confirm with RR (data already discarded)
                            printf("Duplicate frame I(%d) with data error - confirming with RR\n", receivedFrameNumber);
                            unsigned char rrControl = (expectedFrameNumber == 0) ? C_RR_0 : C_RR_1;
                            unsigned char rrBuf[5] = {FLAG, A_RX, rrControl, A_RX ^ rrControl, FLAG};
                            //writeBytesSerialPort(rrBuf, 5);
                            printf("Sent RR(%d) - confirming duplicate\n", expectedFrameNumber);
                        }
                        
                        currentState = Start;
                        // Reset for next frame
                        dataIndex = 0;
                        calculatedBCC2 = 0;
                    }
                }
                else {
                    // Continue receiving data
                    dataBuffer[dataIndex++] = byte;
                    calculatedBCC2 ^= byte;
                    // Stay in DATA state
                    currentState = DATA;
                }
                break;
        }
    }

    if (alarmActive && !alarmEnabled){
        *timedOut = TRUE;
        printf("TIME OUT!\n\n");
        return 0;
    }

    printf("Frame processing complete\n");



    printf("Total bytes received: %d\n", nBytesBuf);

    printf("\n\nReceived Message : %s\n\n", msgs_[*messageRcvd]);
    


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

int packetBuilder(const MessageType messageType, unsigned char * buf, int bufSize){
    switch (messageType)
    {
    case SET_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        buf[1] = A_TX;
        buf[2] = C_SET;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;

    case UA_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        buf[1] = A_TX;
        buf[2] = C_UA;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    case DATA_MSG:
        break;
    case REJ_MSG:
        break;
    case DISC_MSG:
        if(bufSize < 5){
            return -1;
        }
        buf[0] = F;
        buf[1] = A_TX;
        buf[2] = C_DISC;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = F;
        break;
    default:
        break;
    }

    return 0;
}

void alarmHandler(int signal){
    alarmEnabled = FALSE;
}


