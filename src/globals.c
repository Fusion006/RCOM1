#include "globals.h"

const char* states[] = {"START","FLAG","A","C","BCC1","DATA","BCC2","FINAL"};
const char * msgs_[] = {"SET_MSG", "RR0_MSG", "RR1_MSG", "UA_MSG","I0_MSG",
    "I1_MSG", "REJ0_MSG", "REJ1_MSG", "DISC_MSG","INVALID_MSG","NO_MSG"};

// Sequence number tracking for duplicate detection
int expectedFrameNumber = 0;  // Expected I-frame sequence number (0 or 1)

int alarmEnabled = FALSE;
int alarmCount = 0;
volatile int STOP = FALSE;
