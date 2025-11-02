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
#include "globals.h"


// Open a connection using the "port" parameters defined in struct LinkLayer.
// Returns 0 on success or -1 on error.
int llopen(LinkLayer connectionParameters);

// Send data with size dataSize through the link layer.
// Arguments:
//   data: Pointer to the data buffer to send.
//   dataSize: Size of the data buffer in bytes.
//   messageType: Type of message to send (e.g., DATA, SET, UA).
//   linkLayerRole: Role of the device (TRANSMITTER or RECEIVER).
//   ignoredBytes: Pointer to an integer that stores how many bytes were ignored due to buffer size limits.
// Returns:
//   Number of bytes written, or -1 on error.
int llwrite(const unsigned char *data, int dataSize, const MessageType messageType,
            LinkLayerRole linkLayerRole, int *ignoredBytes);

// Receive data into packet buffer.
// Arguments:
//   packet: Buffer to store the received data.
//   messageRcvd: Pointer to store the type of message received.
//   timedOut: Pointer to flag if a timeout occurred during reception.
//   bytes: Pointer to store the number of bytes received.
// Returns:
//   Number of bytes read, or -1 on error.
int llread(unsigned char *packet, MessageType *messageRcvd, int *timedOut, int *bytes);

// Close the previously opened connection and display transmission statistics.
// Returns 0 on success or -1 on error.
int llclose();

// Build a link-layer packet based on message type and data content.
// Arguments:
//   messageType: Type of message to construct.
//   buf: Buffer where the final packet will be stored.
//   bufSize: Size of the buffer.
//   data: Pointer to the data payload.
//   dataSize: Size of the data payload.
//   role: Role of the device (TRANSMITTER or RECEIVER).
//   numStuffs: Pointer to track the number of stuffed bytes added.
//   ignoredBytes: Pointer to track how many bytes were ignored due to buffer size limits.
// Returns:
//   Total packet size, or -1 on error.
int packetBuilder(const MessageType messageType, unsigned char *buf, int bufSize,
                  const unsigned char *data, int dataSize, LinkLayerRole role,
                  int *numStuffs, int *ignoredBytes);

// Alarm signal handler for timeout control during transmission/reception.
// Arguments:
//   signal: Signal number triggering the handler.
void alarmHandler(int signal);

// Perform byte stuffing on a data buffer to escape FLAG and ESC bytes.
// Arguments:
//   data: Pointer to the original data buffer.
//   buffer: Destination buffer for the stuffed data.
//   dataSize: Number of bytes in the original data.
//   numStuffs: Pointer to store how many stuffing bytes were added.
//   ignoredBytes: Pointer to store how many bytes were ignored due to buffer size limits.
// Returns:
//   Size of the stuffed buffer.
int stuffing(const unsigned char *data, unsigned char *buffer, const int dataSize,
             int *numStuffs, int *ignoredBytes);


#endif // _LINK_LAYER_H_
