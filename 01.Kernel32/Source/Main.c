#include "Types.h"

void kPrintString(const int iX, const int iY, const char *pcString);
BOOL kInitializeKernel64Area();

void Main() {
    kPrintString(0, 3, "C Lang Kernel Started");

    if(kInitializeKernel64Area()) {
        kPrintString(0, 4, "IA-32e Kernel Area Initialization Complete");
    } else {
        kPrintString(0, 4, "IA-32e Kernel Area Initialization Fail");
        while (1) ;
    }

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

    while (pdwCurrentAddress < 0x600000) {
        *pdwCurrentAddress = 0x00;

        if (*pdwCurrentAddress != 0) {
            return FALSE;
        }

        pdwCurrentAddress++;
    }

    return TRUE;
}
