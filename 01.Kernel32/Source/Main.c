#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void kPrintString(const int iX, const int iY, const char *pcString);
BOOL kInitializeKernel64Area();
BOOL kIsMemoryEnough();

void Main() {
    kPrintString(0, 3, "[Pass] C Lang Kernel Started");

    kPrintString(0, 4, "[    ] IA-32e Kernel Area Initialization");
    if(kInitializeKernel64Area()) {
        kPrintString(1, 4, "Pass");
    } else {
        kPrintString(1, 4, "Fail");
        while (1) ;
    }

    kPrintString(0, 5, "[    ] Minimum Memory Size Check");
    if(kIsMemoryEnough()) {
        kPrintString(1, 5, "Pass");
    } else {
        kPrintString(1, 5, "Fail");
        while (1) ;
    }

    kPrintString(0, 6, "[    ] IA-32e Page Tables Initialize");
    kInitializePageTables();
    kPrintString(1, 6, "Pass");

    DWORD dwEAX, dwEBX, dwECX, dwEDX;
    char vcVendorString[13] = { 0, };
    kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    *(DWORD *)vcVendorString = dwEBX;
    *((DWORD *)vcVendorString + 1) = dwEDX;
    *((DWORD *)vcVendorString + 2) = dwECX;
    kPrintString(0, 7, "[            ] Processor Vendor String");
    kPrintString(1, 7, vcVendorString);

    kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    kPrintString(0, 8, "[    ] 64bits Mode Support Check");
    if (dwEDX & (1 << 29)) {
        kPrintString(1, 8, "Pass");
    } else {
        kPrintString(1, 8, "Fail");
        while (1) ;
    }

    kPrintString(0, 9, "[   ] Switch To IA-32e Mode");
    // kSwitchAndExecute64bitKernel();

    while (1) ;
}

void kPrintString(const int iX, const int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)0xb8000;

    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != '\0'; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}

BOOL kInitializeKernel64Area() {
    DWORD *pdwCurrentAddress = (DWORD *)0x100000;

    while (pdwCurrentAddress < (DWORD *)0x600000) {
        *pdwCurrentAddress = 0x00;

        if (*pdwCurrentAddress != 0) {
            return FALSE;
        }

        pdwCurrentAddress++;
    }

    return TRUE;
}

BOOL kIsMemoryEnough() {
    DWORD *pdwCurrentAddress = (DWORD *)0x100000;

    while (pdwCurrentAddress < (DWORD *)0x4000000) {
        *pdwCurrentAddress = 0x12345678;
        if (*pdwCurrentAddress != 0x12345678) {
            return FALSE;
        }

        pdwCurrentAddress += (0x100000 / 4);
    }

    return TRUE;
}
