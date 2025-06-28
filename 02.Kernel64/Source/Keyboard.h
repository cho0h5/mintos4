#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"

#define KEY_SKIPCOUNTFORPAUSE       2

#define KEY_FLAGS_UP                0x00
#define KEY_FLAGS_DOWN              0x01
#define KEY_FLAGS_EXTENDEDKEY       0x02

#define KEY_MAPPINGTABLEMAXCOUNT    89

#define KEY_NONE                    0x00
#define KEY_ENTER                   '\n'
#define KEY_TAB                     '\t'
#define KEY_ESC                     0x1b
#define KEY_BACKSPACE               0x08
#define KEY_CTRL                    0x81
#define KEY_LSHIFT                  0x82
#define KEY_RSHIFT                  0x83
#define KEY_PRINTSCREEN             0x84
#define KEY_LALT                    0x85
#define KEY_CAPSLOCK                0x86
#define KEY_F1                      0x87
#define KEY_F2                      0x88
#define KEY_F3                      0x89
#define KEY_F4                      0x8a
#define KEY_F5                      0x8b
#define KEY_F6                      0x8c
#define KEY_F7                      0x8d
#define KEY_F8                      0x8e
#define KEY_F9                      0x8f
#define KEY_F10                     0x90
#define KEY_NUMLOCK                 0x91
#define KEY_SCROLLLOCK              0x92
#define KEY_HOME                    0x93
#define KEY_UP                      0x94
#define KEY_PAGEUP                  0x95
#define KEY_LEFT                    0x96
#define KEY_CENTER                  0x97
#define KEY_RIGHT                   0x98
#define KEY_END                     0x99
#define KEY_DOWN                    0x9a
#define KEY_PAGEDOWN                0x9b
#define KEY_INS                     0x9c
#define KEY_DEL                     0x9d
#define KEY_F11                     0x9e
#define KEY_F12                     0x9f
#define KEY_PAUSE                   0xa0

#pragma pack(push, 1)

typedef struct kKeyMappingEntryStruct {
    BYTE bNormalCode;
    BYTE bCombinedCode;
} KEYMAPPINGENTRY;

#pragma pack(pop)

typedef struct kKeyboardManagerStruct {
    BOOL bShiftdown;
    BOOL bCapsLockOn;
    BOOL bNumLockOn;

    BOOL bExtendedCodeIn;
    int iSkipCountForPause;
} KEYBOARDMANAGER;

BOOL kIsOutputBufferFull();
BOOL kIsInputBufferFull();
BOOL kActivateKeyboard();
BYTE kGetKeyboardScanCode();
void kReboot();
BOOL kIsAlphabetScanCode(const BYTE bScanCode);
BOOL kIsNumberOrSymbolScanCode(const BYTE bScanCode);
BOOL kIsNumberPadScanCode(const BYTE bScanCode);
BOOL kIsUseCombinedCode(const BYTE bScanCode);
void UpdateCombinationKeyStatus(const BYTE bScanCode);
BOOL kConvertScanCodeToASCIICode(const BYTE bScanCode, BYTE *pbASCIICode, BOOL *pbFlags);

#endif
