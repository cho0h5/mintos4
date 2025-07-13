#include "IOAPIC.h"
#include "MPConfigurationTable.h"
#include "Utility.h"
#include "Console.h"
#include "PIC.h"

static IOAPICMANAGER gs_stIOAPICManager;

QWORD kGetIOAPICBaseAddressOfISA() {
    MPCONFIGURATIONMANAGER *pstMPManager;

    if (gs_stIOAPICManager.qwIOAPICBaseAddressOfISA == NULL) {
        const IOAPICENTRY *pstIOAPICEntry = kFindIOAPICEntryForISA();
        if (pstIOAPICEntry == NULL) {
            gs_stIOAPICManager.qwIOAPICBaseAddressOfISA = pstIOAPICEntry->dwMemoryMapAddress & 0xffffffff;
        }
    }

    return gs_stIOAPICManager.qwIOAPICBaseAddressOfISA;
}

void kSetIOAPICRedirectionEntry(IOREDIRECTIONTABLE *pstEntry, const BYTE bAPICID,
        const BYTE bInterruptMask, const BYTE bFlagsAndDeliveryMode, const BYTE bVector) {
    kMemSet(pstEntry, 0, sizeof(IOREDIRECTIONTABLE));

    pstEntry->bDestination = bAPICID;
    pstEntry->bFlagsAndDeliveryMode = bFlagsAndDeliveryMode;
    pstEntry->bInterruptMask = bInterruptMask;
    pstEntry->bVector = bVector;
}

void kReadIOAPICRedirectionTable(const int iINTIN, IOREDIRECTIONTABLE *pstEntry) {
    const QWORD qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();
    QWORD *pqwData = (QWORD *)pstEntry;

    // Read Upper 4 Bytes
    *(DWORD *)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
        IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;
    *pqwData = *(DWORD *)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW);
    *pqwData = *pqwData << 32;

    // Read Lower 4 Bytes
    *(DWORD *)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
        IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2;
    *pqwData |= *(DWORD *)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW);
}

void kWriteIOAPICRedirectionTable(const int iINTIN, const IOREDIRECTIONTABLE *pstEntry) {
    const QWORD qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();
    const QWORD *pqwData = (QWORD *)pstEntry;

    // Write Upper 4 Bytes
    *(DWORD *)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
        IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;
    *(DWORD *)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW) = *pqwData >> 32;

    // Read Lower 4 Bytes
    *(DWORD *)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
        IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE+ iINTIN * 2;
    *(DWORD *)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW) = *pqwData;
}

void kMaskAllInterruptInIOAPIC() {
    for (int i = 0; i < IOAPIC_MAXIOREDIRECTIONTABLECOUNT; i++) {
        IOREDIRECTIONTABLE stEntry;
        kReadIOAPICRedirectionTable(i, &stEntry);
        stEntry.bInterruptMask = IOAPIC_INTERRUPT_MASK;
        kWriteIOAPICRedirectionTable(i, &stEntry);
    }
}

void kInitializeIORedirectionTable() {
    // Init gs_stIOAPICManager
    kMemSet(&gs_stIOAPICManager, 0, sizeof(gs_stIOAPICManager));
    kGetIOAPICBaseAddressOfISA();

    for (int i = 0; i < IOAPIC_MAXIRQTOINTINMAPCOUNT; i++) {
        gs_stIOAPICManager.vbIRQToINTINMap[i] = 0xff;
    }

    // Mask interrupts
    kMaskAllInterruptInIOAPIC();

    MPCONFIGURATIONMANAGER *pstMPManager = kGetMPConfigurationManager();
    MPCONFIGURATIONTABLEHEADER *pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
    QWORD qwEntryAddress = pstMPManager->qwBaseEntryStartAddress;

    for (int i = 0; i < pstMPHeader->wEntryCount; i++) {
        const BYTE bEntryType = *(BYTE *)qwEntryAddress;
        switch (bEntryType) {
            case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
                const IOINTERRUPTASSIGNMENTENTRY *pstIOAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY *)qwEntryAddress;
                if (pstIOAssignmentEntry->bSourceBUSID == pstMPManager->bISABusID &&
                        pstIOAssignmentEntry->bInterruptType == MP_INTERRUPTTYPE_INT) {
                    BYTE bDestination;
                    if (pstIOAssignmentEntry->bSourceBUSIRQ == 0) {
                        bDestination = 0xff;
                    } else {
                        bDestination = 0x00;
                    }

                    IOREDIRECTIONTABLE stIORedirectionEntry;
                    kSetIOAPICRedirectionEntry(&stIORedirectionEntry, bDestination,
                            0x00, IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH |
                            IOAPIC_DESTINATIONMODE_PHYSICALMODE | IOAPIC_DELIVERYMODE_FIXED,
                            PIC_IRQSTARTVECTOR + pstIOAssignmentEntry->bSourceBUSIRQ);
                    kWriteIOAPICRedirectionTable(pstIOAssignmentEntry->bDestinationIOAPICINTIN,
                            &stIORedirectionEntry);
                    gs_stIOAPICManager.vbIRQToINTINMap[pstIOAssignmentEntry->bSourceBUSIRQ] =
                        pstIOAssignmentEntry->bDestinationIOAPICINTIN;
                }
                qwEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
                break;

            case MP_ENTRYTYPE_PROCESSOR:
                qwEntryAddress += sizeof(PROCESSORENTRY);
                break;

            case MP_ENTRYTYPE_BUS:
            case MP_ENTRYTYPE_IOAPIC:
            case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
                qwEntryAddress += 8;
                break;
        }
    }
}

void kPrintIRQToINTINMap() {
    kPrintf("= IRQ To I/O APIC INT In Mapping Table =\n");
    for (int i = 0; i < IOAPIC_MAXIRQTOINTINMAPCOUNT; i++) {
        kPrintf("IRQ [%d] -> INTIN [%d]\n", i, gs_stIOAPICManager.vbIRQToINTINMap[i]);
    }
}
