#include "Types.h"
#include "Descriptor.h"
#include "Utility.h"

// GDT

void kInitializeGDTTableAndTSS() {
    // GDTR
    GDTR *pstGDTR = (GDTR *)0x142000;
    GDTENTRY8 *pstEntry = (GDTENTRY8 *)(0x142000 + sizeof(GDTR));
    pstGDTR->wLimit = GDT_TABLESIZE - 1;
    pstGDTR->qwBaseAddress = (QWORD)pstEntry;
    TSSSEGMENT *pstTSS = (TSSSEGMENT *)((QWORD)pstEntry + GDT_TABLESIZE);

    // NULL Descriptor
    kSetGDTEntry8(&pstEntry[0], 0, 0, 0, 0, 0);

    // CODE Descriptor
    kSetGDTEntry8(&pstEntry[1], 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);

    // DATA Descriptor
    kSetGDTEntry8(&pstEntry[2], 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);

    // TSS Descriptor
    kSetGDTEntry16((GDTENTRY16 *)&pstEntry[3], (QWORD)pstTSS, sizeof(TSSSEGMENT) - 1,
            GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);

    kInitializeTSSSegment(pstTSS);
}

void kSetGDTEntry8(GDTENTRY8 *pstEntry, DWORD dwBaseAddress, DWORD dwLimit,
        BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType) {
    pstEntry->wLowerLimit = dwLimit & 0xffff;
    pstEntry->wLowerBaseAddress = dwBaseAddress & 0xffff;
    pstEntry->bUpperBaseAddress1 = (dwBaseAddress >> 16) & 0xff;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xff) | bUpperFlags;
    pstEntry->bUpperBaseAddress2 = (dwBaseAddress >> 24) & 0xff;
}

void kSetGDTEntry16(GDTENTRY16 *pstEntry, QWORD qwBaseAddress, DWORD dwLimit,
        BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType) {
    pstEntry->wLowerLimit = dwLimit & 0xffff;
    pstEntry->wLowerBaseAddress = qwBaseAddress & 0xffff;
    pstEntry->bMiddleBaseAddress1 = (qwBaseAddress >> 16) & 0xff;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xff) | bUpperFlags;
    pstEntry->bMiddleBaseAddress2 = (qwBaseAddress >> 24) & 0xff;
    pstEntry->dwUpperBaseAddress = qwBaseAddress >> 32;
    pstEntry->dwReserved = 0;
}

void kInitializeTSSSegment(TSSSEGMENT *pstTSS) {
    kMemSet(pstTSS, 0, sizeof(TSSSEGMENT));
    pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE;
    pstTSS->wIOMapBaseAddress = 0xffff;
}

// IDT

void kInitializeIDTTables() {
    IDTR *pstIDTR = (IDTR *)0x1420a0;
    IDTENTRY *pstEntry = (IDTENTRY *)(0x1420a0 + sizeof(IDTR));
    pstIDTR->qwBaseAddress = (QWORD)pstEntry;
    pstIDTR->wLimit = 100 * sizeof(IDTENTRY) - 1;

    for (int i = 0; i < 100; i++) {
        kSetIDTEntry(&pstEntry[i], kDummyHandler, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    }
}

void kSetIDTEntry(IDTENTRY *pstEntry, void *pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType) {
    pstEntry->wLowerBaseAddress = (QWORD)pvHandler & 0xffff;
    pstEntry->wSegmentSelector = wSelector;
    pstEntry->bIST = bIST & 0x3;
    pstEntry->bTypeAndFlags = bType | bFlags;
    pstEntry->wMiddleBaseAddress = ((QWORD)pvHandler >> 16) & 0xffff;
    pstEntry->dwUpperBaseAddress = (QWORD)pvHandler >> 32;
    pstEntry->dwReserved = 0;
}

static void kPrintString(const int iX, const int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)0xb8000;

    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != '\0'; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}

void kDummyHandler() {
    kPrintString(0, 0, "========================================");
    kPrintString(0, 1, "     Dummy Interrupt Handler Called     ");
    kPrintString(0, 2, "            Interrupt Occur             ");
    kPrintString(0, 3, "========================================");

    while (1) ;
}
