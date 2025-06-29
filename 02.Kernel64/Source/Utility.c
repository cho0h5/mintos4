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
