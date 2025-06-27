#include "Types.h"

void kPrintString(const int iX, const int iY, const char *pcString);

void Main() {
    kPrintString(0, 3, "C Lang Kernel Started");

    while (1) ;
}

void kPrintString(const int iX, const int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *)0xb8000;

    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != '\0'; i++) {
        pstScreen[i].bCharactor = pcString[i];
    }
}
