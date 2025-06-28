#include "Utility.h"

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
