#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdarg.h>
#include "Types.h"

void kMemSet(void *pvDestination, BYTE bData, int iSize);
int kMemCpy(void *pvDestination, const void *pvSource, int iSize);
int kMemCmp(const void *pvDestination, const void *pvSource, int iSize);
int kStrLen(const char *pcBuffer);

BOOL kSetInterruptFlag(BOOL bEnableInterrupt);

void kCheckTotalRAMSize();
QWORD kGetTotalRAMSize();

long kAToI(const char *pcBuffer, int iRadix);
QWORD kHexStringToQword(const char *pcBuffer);
long kDecimalStringToLong(const char *pcBuffer);
int kIToA(long lValue, char *pcBuffer, int iRadix);
int kDecimalToString(long lValue, char *pcBuffer);
int kHexToString(QWORD qwValue, char *pcBuffer);
int kReverseString(char *pcBuffer);
int kVSPrintf(char *pcBuffer, const char *pcFormatString, va_list ap);

extern volatile QWORD g_qwTickCount;

#endif
