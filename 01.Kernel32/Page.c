#include "Page.h"

void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress,
        DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags) {
    pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
    pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}

void kInitializePageTables() {
    // PML4
    PML4ENTRY *pstPML4Entry = (PML4ENTRY *)0x100000;
    kSetPageEntryData(&pstPML4Entry[0], 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
    for (int i = 1; i < PAGE_MAXENTRYCOUNT; i++) {
        kSetPageEntryData(&pstPML4Entry[i], 0, 0, 0, 0);
    }

    // PDP
    PDPTENTRY *pstPDPTEntry = (PDPTENTRY *)0x101000;
    for (int i = 0; i < 64; i++) {
        kSetPageEntryData(&pstPDPTEntry[i], 0, 0x102000 + (i * PAGE_TABLESIZE),
                PAGE_FLAGS_DEFAULT, 0);
    }
    for (int i = 64; i < PAGE_MAXENTRYCOUNT; i++) {
        kSetPageEntryData(&pstPDPTEntry[i], 0, 0, 0, 0);
    }

    // PD
    PDENTRY *pstPDEntry = (PDENTRY *)0x102000;
    DWORD dwMappingAddress = 0;
    for (int i = 0; i < PAGE_MAXENTRYCOUNT * 64; i++) {
        kSetPageEntryData(&pstPDEntry[i], (i * (0x200000 >> 20)) >> 12,
                dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
        dwMappingAddress += PAGE_DEFAULTSIZE;
    }
}

