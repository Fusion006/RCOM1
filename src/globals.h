#ifndef ___GLOBALS_H___
#define ___GLOBALS_H___


#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1
#define ACTIVE 2

#define BAUDRATE 38400
#define BUF_SIZE 256

// Size of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer.
#define MAX_PAYLOAD_SIZE 256

// Flag
#define F    0x7E  // 0 1 1 1 1 1 1 0 - Frame delimiter

// Address Field (A) Values
#define A_TX    0x03  // 0 0 0 0 0 0 1 1 - Commands sent by Transmitter, replies sent by Receiver
#define A_RX    0x01  // 0 0 0 0 0 0 0 1 - Commands sent by Receiver, replies sent by Transmitter

// Control Field (C) Values for Information Frames (I)
#define C_I_0   0x00  // 0 0 0 0 0 0 0 0 - Information frame number 0
#define C_I_1   0x40  // 0 1 0 0 0 0 0 0 - Information frame number 1

// Control Field (C) Values for Supervision Frames
#define C_SET   0x03  // 0 0 0 0 0 0 1 1 - SET (set up)
#define C_DISC  0x0B  // 0 0 0 0 1 0 1 1 - DISC (disconnect)
#define C_UA    0x07  // 0 0 0 0 0 1 1 1 - UA (unnumbered acknowledgment)
#define C_RR_0  0xAA  // R 0 0 0 0 1 0 1 - RR (receiver ready / positive ACK) N(r)=0
#define C_RR_1  0xAB  // R 0 0 0 0 1 0 1 - RR (receiver ready / positive ACK) N(r)=1
#define C_REJ_0 0x54  // R 0 0 0 0 0 0 1 - REJ (reject / negative ACK) N(r)=0
#define C_REJ_1 0x55  // R 0 0 0 0 0 0 1 - REJ (reject / negative ACK) N(r)=1

enum State {
    Start,
    FLAG,
    A,
    C,
    BCC1,      // XOR of A and C (header checksum)
    DATA,      // Data bytes
    BCC2,      // XOR of all data bytes (data checksum)
    Final
};

typedef enum {
    SET_MSG,
    RR0_MSG,
    RR1_MSG,
    UA_MSG,
    I0_MSG,
    I1_MSG,
    REJ0_MSG,
    REJ1_MSG,
    DISC_MSG,
    INVALID_MSG,
    NO_MSG
} MessageType;

typedef enum 
{
    ConnectingC,
    DisconnectingC,
    OpenC,
    ClosedC,
    EndC
} CommunicationStatus;


extern const char* states[];
extern const char* msgs_[];

extern int expectedFrameNumber;

extern int alarmEnabled;
extern int alarmCount;
extern volatile int STOP;


#endif