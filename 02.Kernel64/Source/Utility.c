#include "Utility.h"
#include "AssemblyUtility.h"

volatile QWORD g_qwTickCount = 0;

void kMemSet(void *pvDestination, BYTE bData, int iSize) {
    // for (int i = 0; i < iSize; i++) {
    //     ((char *)pvDestination)[i] = bData;
    // }

    QWORD qwData = 0;
    for (int i = 0; i < 8; i++) {
        qwData = (qwData << 8) | bData;
    }

    int i = 0;
    for (; i < iSize / 8; i++) {
        ((QWORD *)pvDestination)[i] = qwData;
    }

    int iRemainByteStartOffset = i * 8;
    for (int i = 0; i < iSize % 8; i++) {
        ((char *)pvDestination)[iRemainByteStartOffset++] = bData;
    }
}

int kMemCpy(void *pvDestination, const void *pvSource, int iSize) {
    // for (int i = 0; i < iSize; i++) {
    //     ((char *)pvDestination)[i] = ((char *)pvSource)[i];
    // }

    // return iSize;

    int i = 0;
    for (; i < iSize / 8; i++) {
        ((QWORD *)pvDestination)[i] = ((QWORD *)pvSource)[i];
    }

    int iRemainByteStartOffset = i * 8;
    for (int i = 0; i < iSize % 8; i++) {
        ((char *)pvDestination)[iRemainByteStartOffset] = ((char *)pvSource)[iRemainByteStartOffset];
        iRemainByteStartOffset++;
    }
}

int kMemCmp(const void *pvDestination, const void *pvSource, int iSize) {
    // for (int i = 0; i < iSize; i++) {
    //     const char cTemp = ((char *)pvDestination)[i] - ((char *)pvSource)[i];
    //     if (cTemp != 0) {
    //         return (int)cTemp;
    //     }
    // }

    // return 0;

    int i = 0;
    for (; i < iSize / 8; i++) {
        const QWORD qwValue = ((QWORD *)pvDestination)[i] - ((QWORD *)pvSource)[i];
        if (qwValue != 0) {
            for (int i = 0; i < 8; i++) {
                if (((qwValue >> (i * 8)) & 0xff) != 0) {
                    return (qwValue >> (i * 8)) &0xff;
                }
            }
        }
    }

    int iRemainByteStartOffset = i * 8;
    for (int i = 0; i < iSize % 8; i++) {
        char cValue = ((char *)pvDestination)[iRemainByteStartOffset] - ((char *)pvSource)[iRemainByteStartOffset];
        if (cValue != 0) {
            return cValue;
        }
        iRemainByteStartOffset++;
    }

    return 0;
}

int kStrLen(const char *pcBuffer) {
    int i = 0;
    for (; ; i++) {
        if (pcBuffer[i] == '\0') {
            break;
        }
    }

    return i;
}

BOOL kSetInterruptFlag(BOOL bEnableInterrupt) {
    QWORD qwRFLAGS;

    qwRFLAGS = kReadRFLAGS();
    if (bEnableInterrupt) {
        kEnableInterrupt();
    } else {
        kDisableInterrupt();
    }

    if (qwRFLAGS & 0x0200) {
        return TRUE;
    }
    return FALSE;
}

static QWORD gs_qwTotalRAMMBSize = 0;

void kCheckTotalRAMSize() {
    DWORD *pdwCurrentAddress = (DWORD *)0x4000000;
    DWORD dwPreviousValue;

    while (1) {
        dwPreviousValue = *pdwCurrentAddress;

        *pdwCurrentAddress = 0x12345678;
        if (*pdwCurrentAddress != 0x12345678) {
            break;
        }

        *pdwCurrentAddress = dwPreviousValue;
        pdwCurrentAddress += 0x400000 / 4;
    }

    gs_qwTotalRAMMBSize = (QWORD)pdwCurrentAddress / 0x100000;
}

QWORD kGetTotalRAMSize() {
    return gs_qwTotalRAMMBSize;
}

long kAToI(const char *pcBuffer, int iRadix) {
    long lReturn;

    switch (iRadix) {
        case 16:
            lReturn = kHexStringToQword(pcBuffer);
            break;

        case 10:
        default:
            lReturn = kDecimalStringToLong(pcBuffer);
            break;
    }

    return lReturn;
}

QWORD kHexStringToQword(const char *pcBuffer) {
    QWORD qwValue = 0;

    for (int i = 0; pcBuffer[i] != '\0'; i++) {
        qwValue *= 16;
        if ('A' <= pcBuffer[i] && pcBuffer[i] <= 'Z') {
            qwValue += pcBuffer[i] - 'A' + 10;
        } else if ('a' <= pcBuffer[i] && pcBuffer[i] <= 'z') {
            qwValue += pcBuffer[i] - 'a' + 10;
        } else {
            qwValue += pcBuffer[i] - '0';
        }
    }

    return qwValue;
}

long kDecimalStringToLong(const char *pcBuffer) {
    int i;
    if (pcBuffer[0] == '-') {
        i = 1;
    } else {
        i = 0;
    }

    long lValue = 0;
    for (; pcBuffer[i] != '\0'; i++) {
        lValue *= 10;
        lValue += pcBuffer[i] - '0';
    }

    if (pcBuffer[0] == '-') {
        lValue = -lValue;
    }

    return lValue;
}

int kIToA(long lValue, char *pcBuffer, int iRadix) {
    int iReturn;
    switch (iRadix) {
        case 16:
            iReturn = kHexToString(lValue, pcBuffer);
            break;

        case 10:
        default:
            iReturn = kDecimalToString(lValue, pcBuffer);
            break;
    }

    return iReturn;
}

int kDecimalToString(long lValue, char *pcBuffer) {
    if (lValue == 0) {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    long i;
    if (lValue < 0) {
        i = 1;
        pcBuffer[0] = '-';
        lValue = -lValue;
    } else {
        i = 0;
    }

    for (; lValue > 0; i++) {
        pcBuffer[i] = '0' + lValue % 10;
        lValue = lValue / 10;
    }

    pcBuffer[i] = '\0';
    kReverseString(&pcBuffer[pcBuffer[0] == '-' ? 1 : 0]);
    return i;
}

int kHexToString(QWORD qwValue, char *pcBuffer) {
    if (qwValue == 0) {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    int i = 0;
    for (; qwValue > 0; i++) {
        QWORD qwCurrentValue = qwValue % 16;
        if (qwCurrentValue >= 10) {
            pcBuffer[i] = 'a' + (qwCurrentValue - 10);
        } else {
            pcBuffer[i] = '0' + qwCurrentValue;
        }

        qwValue = qwValue / 16;
    }

    pcBuffer[i] = '\0';
    kReverseString(pcBuffer);
    return i;
}

int kReverseString(char *pcBuffer) {
    int iLength = kStrLen(pcBuffer);
    for (int i = 0; i < iLength / 2; i++) {
        char cTemp = pcBuffer[i];
        pcBuffer[i] = pcBuffer[iLength - 1 - i];
        pcBuffer[iLength - 1 - i] = cTemp;
    }
}

int kSPrintf(char *pcBuffer, const char *pcFormatString, ...) {
    va_list ap;
    int iReturn;

    va_start(ap, pcFormatString);
    iReturn = kVSPrintf(pcBuffer, pcFormatString, ap);
    va_end(ap);

    return iReturn;
}

int kVSPrintf(char *pcBuffer, const char *pcFormatString, va_list ap) {
    int iBufferIndex = 0;
    const int iFormatLength = kStrLen(pcFormatString);

    for (int i = 0; i < iFormatLength; i++) {
        if (pcFormatString[i] == '%') {
            i++;
            switch(pcFormatString[i]) {
                case 's':
                    char *pcCopyString = (char *)va_arg(ap, char *);
                    int iCopyLength = kStrLen(pcCopyString);
                    kMemCpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
                    iBufferIndex += iCopyLength;
                    break;

                case 'c':
                    pcBuffer[iBufferIndex] = (char)va_arg(ap, int);
                    iBufferIndex++;
                    break;

                case 'd':
                case 'i':
                    int iValue = (int)va_arg(ap, int);
                    iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
                    break;

                case 'x':
                case 'X':
                    QWORD qwValue = (DWORD)va_arg(ap, DWORD) & 0xffffffff;
                    iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                    break;

                case 'q':
                case 'Q':
                case 'p':
                    qwValue = (QWORD)va_arg(ap, QWORD);
                    iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                    break;

                case 'f':
                    double dValue = (double)va_arg(ap, double);
                    dValue += 0.005;
                    pcBuffer[iBufferIndex] = '0' + (QWORD)(dValue * 100) % 10;
                    pcBuffer[iBufferIndex + 1] = '0' + (QWORD)(dValue * 10) % 10;
                    pcBuffer[iBufferIndex + 2] = '.';
                    int  k = 0;
                    for (; ; k++) {
                        if ((QWORD)dValue == 0 && k != 0) {
                            break;
                        }
                        pcBuffer[iBufferIndex + 3 + k] = '0' + ((QWORD)dValue % 10);
                        dValue = dValue / 10;
                    }
                    pcBuffer[iBufferIndex + 3 + k] = '\0';
                    kReverseString(pcBuffer + iBufferIndex);
                    iBufferIndex += 3 + k;
                    break;

                default:
                    pcBuffer[iBufferIndex] = pcFormatString[i];
                    iBufferIndex++;
                    break;
            }
        } else {
            pcBuffer[iBufferIndex] = pcFormatString[i];
            iBufferIndex++;
        }
    }

    pcBuffer[iBufferIndex] = '\0';
    return iBufferIndex;
}

QWORD kGetTickCount() {
    return g_qwTickCount;
}

void kSleep(const QWORD qwMillisecond) {
    QWORD qwLastTickCount = g_qwTickCount;

    while (g_qwTickCount - qwLastTickCount <= qwMillisecond) {
        kSchedule();
    }
}
