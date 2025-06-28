#ifndef __ASSEMBLYUTILITY_C__
#define __ASSEMBLYUTILITY_C__

#include "Types.h"

BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);
void kLoadGDTR(QWORD qwGDTRAddress);
void kLoadTR(WORD wTSSSegmentOffset);
void kLoadIDTR(QWORD qwIDTRAddress);

#endif
