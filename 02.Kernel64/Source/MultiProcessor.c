#include "MultiProcessor.h"
#include "LocalAPIC.h"
#include "PIT.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "MPConfigurationTable.h"

volatile int g_iWakeUpApplicationProcessorCount = 0;

volatile QWORD g_qwAPICIDAddress = 0;

BOOL kStartUpApplicationProcessor() {
    if (!kAnalysisMPConfigurationTable()) {
        return FALSE;
    }

    kEnableGlobalLocalAPIC();

    kEnableSoftwareLocalAPIC();

    if (!kWakeUpApplicationProcessor()) {
        return FALSE;
    }

    return TRUE;
}

static BOOL kWakeUpApplicationProcessor() {
    const BOOL bInterruptFlag = kSetInterruptFlag(FALSE);

    const MPCONFIGURATIONMANAGER *pstMPManager = kGetMPConfigurationManager();
    const MPCONFIGURATIONTABLEHEADER *pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
    const QWORD qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;

    g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;

    *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) =
        APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
        APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;

    kWaitUsingDirectPIT(MSTOCOUNT(10));

    if (*(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING) {
        kInitializePIT(MSTOCOUNT(1), TRUE);

        kSetInterruptFlag(bInterruptFlag);
        return FALSE;
    }

    for (int i = 0; i < 2; i++) {
        *(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) =
            APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
            APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_STARTUP | 0x10;

        kWaitUsingDirectPIT(USTOCOUNT(200));

        if (*(DWORD *)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING) {
            kInitializePIT(MSTOCOUNT(1), TRUE);

            kSetInterruptFlag(bInterruptFlag);
            return FALSE;
        }
    }

    kInitializePIT(MSTOCOUNT(1), TRUE);

    kSetInterruptFlag(bInterruptFlag);

    while (g_iWakeUpApplicationProcessorCount < pstMPManager->iProcessorCount - 1) {
        kSleep(50);
    }

    return TRUE;
}

BYTE kGetAPICID() {
    if (g_qwAPICIDAddress == 0) {
        const MPCONFIGURATIONTABLEHEADER *pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
        if (pstMPHeader == NULL) {
            return 0;
        }

        const QWORD qwLocalAPICBaseAddress =pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
        g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;
    }

    return *((DWORD *)g_qwAPICIDAddress) >> 24;
}
