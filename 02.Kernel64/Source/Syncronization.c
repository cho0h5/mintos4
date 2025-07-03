#include "Syncronization.h"
#include "Utility.h"

BOOL kLockForSystemData() {
    return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(const BOOL bInterruptFlag) {
    kSetInterruptFlag(bInterruptFlag);
}
