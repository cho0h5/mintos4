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
