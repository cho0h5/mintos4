#include "Types.h"
#include "PIC.h"
#include "AssemblyUtility.h"

void kInitializePIC() {
    // Init master
    kOutPortByte(PIC_MASTER_PORT1, 0x11);
    kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);
    kOutPortByte(PIC_MASTER_PORT2, 0x04);
    kOutPortByte(PIC_MASTER_PORT2, 0x01);

    // Init slave
    kOutPortByte(PIC_SLAVE_PORT1, 0x11);
    kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);
    kOutPortByte(PIC_SLAVE_PORT2, 0x02);
    kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}

void kMaskPICInterrupt(WORD wIRQBitmask) {
    // To master
    kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitmask);

    // To slave
    kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(wIRQBitmask >> 8));
}

void kSendEOIToPIC(int iIRQNumber) {
    // To master
    kOutPortByte(PIC_MASTER_PORT1, 0x20);

    // To slave
    if (iIRQNumber >= 8) {
        kOutPortByte(PIC_SLAVE_PORT1, 0x20);
    }
}
