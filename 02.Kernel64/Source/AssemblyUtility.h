#ifndef __ASSEMBLYUTILITY_C__
#define __ASSEMBLYUTILITY_C__

#include "Types.h"
#include "Task.h"

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
QWORD kReadTSC();

// Task
void kSwitchContext(CONTEXT *pstCurrentContext, CONTEXT *pstNextContext);
void kHlt();
BOOL kTestAndSet(volatile BYTE *pbDestination, BYTE bCompare, BYTE bSource);

// FPU
void kInitializeFPU();
void kSaveFPUContext(void *pvFPUContext);
void kLoadFPUContext(void *pvFPUContext);
void kSetTS();
void kClearTS();

// HDD
WORD kInPortWord(WORD wPort);
void kOutPortWord(WORD wPort, WORD wData);

#endif
