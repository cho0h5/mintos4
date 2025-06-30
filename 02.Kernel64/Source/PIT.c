#include "PIT.h"
#include "AssemblyUtility.h"

void kInitializePIT(const WORD wCount, const BOOL bPeriodic) {
    if (bPeriodic) {
        kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
    } else {
        kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);
    }

    kOutPortByte(PIT_PORT_COUNTER0, wCount);
    kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}

WORD kReadCounter0() {
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

    const BYTE bLowByte = kInPortByte(PIT_PORT_COUNTER0);
    const BYTE bHighByte = kInPortByte(PIT_PORT_COUNTER0);

    return (bHighByte << 8) | bLowByte;
}

void kWaitUsingDirectPIT(const WORD wCount) {
    kInitializePIT(0, TRUE);

    WORD wLastCounter0 = kReadCounter0();
    while (1) {
        const WORD wCurrentCounter0 = kReadCounter0();
        if (((wLastCounter0 - wCurrentCounter0) & 0xffff) >= wCount) {
            break;
        }
    }
}
