// Link layer header.
// DO NOT CHANGE THIS FILE

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
/* Modify this section
const char set = 0x7e;
const char ua = 
const char * standard;
standard[2] = 0x07;
standard[3] = standard [1] ^ standard[2];   
standard[4] = 0x7e;
*/

// Flag
#define FLAG    0x7E  // 0 1 1 1 1 1 1 0 - Frame delimiter

// Address Field (A) Values
#define A_TX    0x03  // 0 0 0 0 0 0 1 1 - Commands sent by Transmitter, replies sent by Receiver
#define A_RX    0x01  // 0 0 0 0 0 0 0 1 - Commands sent by Receiver, replies sent by Transmitter

// Control Field (C) Values for Information Frames (I)
#define C_I_0   0x00  // 0 0 0 0 0 0 0 0 - Information frame number 0, S = N(s)
#define C_I_1   0x80  // 1 0 0 0 0 0 0 0 - Information frame number 1, S = N(s)

// Control Field (C) Values for Supervision Frames
#define C_SET   0x03  // 0 0 0 0 0 0 1 1 - SET (set up)
#define C_DISC  0x0B  // 0 0 0 0 1 0 1 1 - DISC (disconnect)
#define C_UA    0x07  // 0 0 0 0 0 1 1 1 - UA (unnumbered acknowledgment)
#define C_RR_0  0x05  // R 0 0 0 0 1 0 1 - RR (receiver ready / positive ACK) N(r)=0
#define C_RR_1  0x85  // R 0 0 0 0 1 0 1 - RR (receiver ready / positive ACK) N(r)=1
#define C_REJ_0 0x01  // R 0 0 0 0 0 0 1 - REJ (reject / negative ACK) N(r)=0
#define C_REJ_1 0x81  // R 0 0 0 0 0 0 1 - REJ (reject / negative ACK) N(r)=1

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// Size of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer.
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1



int ll_process(int argc, char *argv[]);
// Open a connection using the "port" parameters defined in struct linkLayer.
// Return 0 on success or -1 on error.
int llopen(LinkLayer connectionParameters);

// Send data in standard with size standardSize.
// Return number of chars written, or -1 on error.
int llwrite(const unsigned char *standard, int standardSize);

// Receive data in packet.
// Return number of chars read, or -1 on error.
int llread(unsigned char *packet);

// Close previously opened connection and print transmission statistics in the console.
// Return 0 on success or -1 on error.
int llclose();

#endif // _LINK_LAYER_H_
