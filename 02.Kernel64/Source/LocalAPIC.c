#include "LocalAPIC.h"
#include "MPConfigurationTable.h"

QWORD kGetLocalAPIBaseAddress() {
    MPCONFIGURATIONTABLEHEADER *pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
    return pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
}

void kEnableSoftwareLocalAPIC() {
    QWORD qwLocalAPIBaseAddress = kGetLocalAPIBaseAddress();
    *(DWORD *)(qwLocalAPIBaseAddress + APIC_REGISTER_SVR) |= 0x100;
}
