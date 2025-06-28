#include "Utility.h"

void kMemSet(void *pvDestination, BYTE bData, int iSize) {
    for (int i = 0; i < iSize; i++) {
        ((char *)pvDestination)[i] = bData;
    }
}
