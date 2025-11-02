// Application layer protocol header.
// DO NOT CHANGE THIS FILE

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include "globals.h"

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

// Appends a received data chunk to the destination file.
// Arguments:
//   fd: File descriptor of the file being written.
//   buffer: Pointer to the received data buffer.
//   length: Number of bytes to append from the buffer.
// Returns:
//   Number of bytes successfully written, or -1 on error.
int appendChunk(int fd, const unsigned char* buffer, int length);

// Extracts a data chunk from the source file to be transmitted.
// Arguments:
//   fd: File descriptor of the file being read.
//   startIndex: Byte offset from which to start reading.
//   buffer: Destination buffer to store the read data.
// Returns:
//   Number of bytes read into the buffer, or 0 if EOF is reached.
int extractChunk(int fd, int startIndex, unsigned char* buffer);

// Handles the transmitter-side operations of the application layer.
// Arguments:
//   link: Link layer configuration structure.
//   communicationStatus: Pointer to the current communication state tracker.
//   messageRcvd: Pointer to store any message received from the receiver.
//   fd: File descriptor of the file being transmitted.
//   startIndex: Pointer to the index indicating where to continue reading data.
//   controlIndex: Pointer to the control frame index for retransmission handling.
// Returns:
//   0 on success, nonzero on error or transmission failure.
int transmitterProcess(LinkLayer link, CommunicationStatus * communicationStatus,
                       MessageType * messageRcvd, int fd, int * startIndex, int * controlIndex);

// Handles the receiver-side operations of the application layer.
// Arguments:
//   link: Link layer configuration structure.
//   communicationStatus: Pointer to the current communication state tracker.
//   messageRcvd: Pointer to store the type of message received.
//   fd: File descriptor of the file being written to.
//   sequenceNumber: Pointer to the expected frame sequence number.
// Returns:
//   0 on success, nonzero on error or reception failure.
int receiverProcess(LinkLayer link, CommunicationStatus * communicationStatus,
                    MessageType * messaageRcvd, int fd, int *sequenceNumber);



#endif // _APPLICATION_LAYER_H_
