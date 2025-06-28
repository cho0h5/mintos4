#include "Types.h"

void kPrintString(const int iX, const int iY, const char *pcString);

void Main() {
    kPrintString(1, 9, "Pass");
    kPrintString(0, 10, "IA-32e C Language Kernel Start");
}

void kPrintString(const int iX, const int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)0xb8000;

    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != '\0'; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}

