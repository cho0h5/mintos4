#ifndef __ASSEMBLYUTILITY_C__
#define __ASSEMBLYUTILITY_C__

#include "Types.h"

// Keyboard
BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);

// GDT, IDT
void kLoadGDTR(QWORD qwGDTRAddress);
void kLoadTR(WORD wTSSSegmentOffset);
void kLoadIDTR(QWORD qwIDTRAddress);

// Interrupt
void kEnableInterrupt();
void kDisableInterrupt();
QWORD kReadRFLAGS();

#endif
