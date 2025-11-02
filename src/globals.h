#ifndef ___GLOBALS_H___
#define ___GLOBALS_H___

#define _POSIX_SOURCE 1 // Enable POSIX compliant functions (for termios, alarms, etc.)

// Boolean and status flags
#define FALSE 0
#define TRUE 1
#define ACTIVE 2

// Default baudrate for serial communication
#define BAUDRATE 38400

// Maximum buffer size for frames
#define BUF_SIZE 256

// Maximum payload size for application data
#define MAX_PAYLOAD_SIZE 256

// Frame delimiter flag
#define F 0x7E // 01111110 – marks start and end of a frame

// Address field (A) values
#define A_TX 0x03 // Transmitter commands, Receiver replies
#define A_RX 0x01 // Receiver commands, Transmitter replies

// Control field (C) values for information frames (I)
#define C_I_0 0x00 // Information frame number 0
#define C_I_1 0x40 // Information frame number 1

// Control field (C) values for supervision frames
#define C_SET   0x03 // SET (Set up connection)
#define C_DISC  0x0B // DISC (Disconnect)
#define C_UA    0x07 // UA (Unnumbered acknowledgment)
#define C_RR_0  0xAA // RR (Receiver Ready / ACK) N(r)=0
#define C_RR_1  0xAB // RR (Receiver Ready / ACK) N(r)=1
#define C_REJ_0 0x54 // REJ (Reject / NACK) N(r)=0
#define C_REJ_1 0x55 // REJ (Reject / NACK) N(r)=1

// Byte used for stuffing (escape character)
#define STUFFING_BYTE 0x7D // 01111101 – used to escape FLAG and ESC bytes

// Link layer role (transmitter or receiver)
typedef enum
{
    LlTx, // Link layer transmitter
    LlRx  // Link layer receiver
} LinkLayerRole;

// Link layer configuration parameters
typedef struct
{
    char serialPort[50]; // Serial port device (e.g., "/dev/ttyS0")
    LinkLayerRole role;  // Role of the link (transmitter or receiver)
    int baudRate;        // Communication speed
    int nRetransmissions;// Maximum number of retransmissions
    int timeout;         // Timeout in seconds for retransmissions
} LinkLayer;

// State machine for frame parsing
enum State {
    Start,   // Initial state
    FLAG,    // FLAG byte received
    A,       // Address byte received
    C,       // Control byte received
    BCC1,    // Header checksum (A XOR C)
    DATA,    // Receiving data bytes
    BCC2,    // Data checksum (XOR of all data)
    Final    // Final state (frame complete)
};

// Message types exchanged between transmitter and receiver
typedef enum {
    SET_MSG,   // Set up connection
    RR0_MSG,   // Receiver Ready, expecting frame 0
    RR1_MSG,   // Receiver Ready, expecting frame 1
    UA_MSG,    // Unnumbered acknowledgment
    I0_MSG,    // Information frame 0
    I1_MSG,    // Information frame 1
    REJ0_MSG,  // Reject frame 0
    REJ1_MSG,  // Reject frame 1
    DISC_MSG,  // Disconnect
    INVALID_MSG, // Invalid message
    NO_MSG,      // No message detected
    INVALID_I0,  // Invalid information frame 0
    INVALID_I1   // Invalid information frame 1
} MessageType;

// Communication state between transmitter and receiver
typedef enum 
{
    ConnectingC,     // Establishing connection
    DisconnectingC,  // Closing connection
    OpenC,           // Connection open
    ClosedC,         // Connection closed
    EndC             // Communication finished
} CommunicationStatus;

// External debug arrays for readable state, message, and status names
extern const char* states[];
extern const char* msgs_[];
extern const char* comStatus[];

// Global control variables
extern int expectedFrameNumber; // Expected sequence number (0 or 1)
extern int alarmEnabled;        // Alarm enabled flag
extern int alarmCount;          // Number of alarms triggered
extern volatile int STOP;       // Stop flag for receiver loop

#endif
