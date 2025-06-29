#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "Types.h"

#define CONSOLE_BACKGROUND_BLACK            0x00
#define CONSOLE_BACKGROUND_BLUE             0x10
#define CONSOLE_BACKGROUND_GREEN            0x20
#define CONSOLE_BACKGROUND_CYAN             0x30
#define CONSOLE_BACKGROUND_RED              0x40
#define CONSOLE_BACKGROUND_MAGENTA          0x50
#define CONSOLE_BACKGROUND_BROWN            0x60
#define CONSOLE_BACKGROUND_WHITE            0x70
#define CONSOLE_BACKGROUND_BLINK            0x80

#define CONSOLE_FOREGROUND_DARKBLACK        0x00
#define CONSOLE_FOREGROUND_DARKBLUE         0x01
#define CONSOLE_FOREGROUND_DARKGREEN        0x02
#define CONSOLE_FOREGROUND_DARKCYAN         0x03
#define CONSOLE_FOREGROUND_DARKRED          0x04
#define CONSOLE_FOREGROUND_DARKMAGENTA      0x05
#define CONSOLE_FOREGROUND_DARKBROWN        0x06
#define CONSOLE_FOREGROUND_DARKWHITE        0x07

#define CONSOLE_FOREGROUND_BRIGHTBLACK      0x08
#define CONSOLE_FOREGROUND_BRIGHTBLUE       0x09
#define CONSOLE_FOREGROUND_BRIGHTGREEN      0x0a
#define CONSOLE_FOREGROUND_BRIGHTCYAN       0x0b
#define CONSOLE_FOREGROUND_BRIGHTRED        0x0c
#define CONSOLE_FOREGROUND_BRIGHTMAGENTA    0x0d
#define CONSOLE_FOREGROUND_BRIGHTBROWN      0x0e
#define CONSOLE_FOREGROUND_BRIGHTWHITE      0x0f

#define CONSOLE_DEFAULTTEXTCOLOR            (CONSOLE_BACKGROUND_BLACK | CONSOLE_FOREGROUND_BRIGHTCYAN)

#define CONSOLE_WIDTH                       80
#define CONSOLE_HEIGHT                      25
#define CONSOLE_VIDEOMEMORYADDRESS          0xb8000

#define VGA_PORT_INDEX                      0x03d4
#define VGA_PORT_DATA                       0x03d5
#define VGA_INDEX_UPPERCURSOR               0x0e
#define VGA_INDEX_LOWERCURSOR               0x0f

#pragma pack(push, 1)

typedef struct kConsoleManagerStruct {
    int iCurrentPrintOffset;
} CONSOLEMANAGER;

#pragma pack(pop)

void kInitializeConsole(int iX, int iY);
void kSetCursor(const int iX, const int iY);
void kGetCursor(int *piX, int *piY);
void kPrintf(const char *pcFormatString, ...);
int kConsolePrintString(const char *pcBuffer);
void kClearScreen();
BYTE kGetCh();
void kPrintStringXY(int iX, int iY, const char *pcString);

#endif
