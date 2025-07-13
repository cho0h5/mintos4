#include "LocalAPIC.h"
#include "MPConfigurationTable.h"

QWORD kGetLocalAPICBaseAddress() {
    MPCONFIGURATIONTABLEHEADER *pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
    return pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
}

void kEnableSoftwareLocalAPIC() {
    QWORD qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_SVR) |= 0x100;
}

void kSendEOIToLocalAPIC() {
    const QWORD qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_EOI) = 0;
}

void kSetTaskPriority(const BYTE bPriority) {
    const QWORD qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_TASKPRIORITY) = bPriority;
}

void kInitializeLocalVectorTable() {
    const QWORD qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();

    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_TIMER) |= APIC_INTERRUPT_MASK;
    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_LINT0) |= APIC_INTERRUPT_MASK;
    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_LINT1) |= APIC_TRIGGERMODE_EDGE |
        APIC_POLARITY_ACTIVEHIGH | APIC_DELIVERYMODE_NMI;
    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_ERROR) |= APIC_INTERRUPT_MASK;
    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_PERFORMANCEMONITOIRINGCOUNTER) |= APIC_INTERRUPT_MASK;
    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_THERMALSENSOR) |= APIC_INTERRUPT_MASK;
}
