#include "Utility.h"
#include "AssemblyUtility.h"

void kMemSet(void *pvDestination, BYTE bData, int iSize) {
    for (int i = 0; i < iSize; i++) {
        ((char *)pvDestination)[i] = bData;
    }
}

int kMemCpy(void *pvDestination, const void *pvSource, int iSize) {
    for (int i = 0; i < iSize; i++) {
        ((char *)pvDestination)[i] = ((char *)pvSource)[i];
    }

    return iSize;
}

int kMemCmp(const void *pvDestination, const void *pvSource, int iSize) {
    for (int i = 0; i < iSize; i++) {
        const char cTemp = ((char *)pvDestination)[i] - ((char *)pvSource)[i];
        if (cTemp != 0) {
            return (int)cTemp;
        }
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

void kPrintString(const int iX, const int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)0xb8000;

    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != '\0'; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
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
        if (qwValue >= 10) {
            pcBuffer[i] = 'a' + (qwCurrentValue - 10);
        } else {
            pcBuffer[i] = '0' + qwCurrentValue;
        }

        qwValue /= 16;
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
