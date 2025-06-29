#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"

BOOL kIsOutputBufferFull() {
    if (kInPortByte(0x64) & 0x01) {
        return TRUE;
    }
    return FALSE;
}

BOOL kIsInputBufferFull() {
    if (kInPortByte(0x64) & 0x02) {
        return TRUE;
    }
    return FALSE;
}

BOOL kActivateKeyboard() {
    // Activate a keyboard controller
    kOutPortByte(0x64, 0xae);
    for (int i = 0; i < 0xffff; i++) {
        if (!kIsInputBufferFull()) {
            break;
        }
    }

    // Activate a keyboard
    kOutPortByte(0x60, 0xf4);
    for (int j = 0; j < 100; j++) {
        for (int i = 0; i < 0xffff; i++) {
            if (kIsOutputBufferFull() == TRUE) {
                break;
            }
        }

        if (kInPortByte(0x60) == 0xfa) {
            return TRUE;
        }
    }

    return FALSE;
}

BYTE kGetKeyboardScanCode() {
    while (!kIsOutputBufferFull()) ;

    return kInPortByte(0x60);
}

void kReboot() {
    for (int i = 0; i < 0xffff; i++) {
        if (!kIsInputBufferFull()) {
            break;
        }
    }

    kOutPortByte(0x64, 0xd1);
    kOutPortByte(0x60, 0x00);

    while (1) ;
}

static KEYBOARDMANAGER gs_stKeyboardManager = { 0, };

static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] = {
    {KEY_NONE,          KEY_NONE      },    /* 00 */
    {KEY_ESC,           KEY_ESC       },    /* 01 */
    {'1',               '!'           },    /* 02 */
    {'2',               '@'           },    /* 03 */
    {'3',               '#'           },    /* 04 */
    {'4',               '$'           },    /* 05 */
    {'5',               '%'           },    /* 06 */
    {'6',               '^'           },    /* 07 */
    {'7',               '&'           },    /* 08 */
    {'8',               '*'           },    /* 09 */

    {'9',               '('           },    /* 10 */
    {'0',               ')'           },    /* 11 */
    {'-',               '_'           },    /* 12 */
    {'=',               '+'           },    /* 13 */
    {KEY_BACKSPACE,     KEY_BACKSPACE },    /* 14 */
    {KEY_TAB,           KEY_TAB       },    /* 15 */
    {'q',               'Q'           },    /* 16 */
    {'w',               'W'           },    /* 17 */
    {'e',               'E'           },    /* 18 */
    {'r',               'R'           },    /* 19 */

    {'t',               'T'           },    /* 20 */
    {'y',               'Y'           },    /* 21 */
    {'u',               'U'           },    /* 22 */
    {'i',               'I'           },    /* 23 */
    {'o',               'O'           },    /* 24 */
    {'p',               'P'           },    /* 25 */
    {'[',               '{'           },    /* 26 */
    {']',               '}'           },    /* 27 */
    {'\n',              '\n'          },    /* 28 */
    {KEY_CTRL,          KEY_CTRL      },    /* 29 */

    {'a',               'A'           },    /* 30 */
    {'s',               'S'           },    /* 31 */
    {'d',               'D'           },    /* 32 */
    {'f',               'F'           },    /* 33 */
    {'g',               'G'           },    /* 34 */
    {'h',               'H'           },    /* 35 */
    {'j',               'J'           },    /* 36 */
    {'k',               'K'           },    /* 37 */
    {'l',               'L'           },    /* 38 */
    {';',               ':'           },    /* 39 */

    {'\'',              '\"'          },    /* 40 */
    {'`',               '~'           },    /* 41 */
    {KEY_LSHIFT,        KEY_LSHIFT    },    /* 42 */
    {'\\',              '|'           },    /* 43 */
    {'z',               'Z'           },    /* 44 */
    {'x',               'X'           },    /* 45 */
    {'c',               'C'           },    /* 46 */
    {'v',               'V'           },    /* 47 */
    {'b',               'B'           },    /* 48 */
    {'n',               'N'           },    /* 49 */

    {'m',               'M'           },    /* 50 */
    {',',               '<'           },    /* 51 */
    {'.',               '>'           },    /* 52 */
    {'/',               '?'           },    /* 53 */
    {KEY_RSHIFT,        KEY_RSHIFT    },    /* 54 */
    {'*',               '*'           },    /* 55 */
    {KEY_LALT,          KEY_LALT      },    /* 56 */
    {' ',               ' '           },    /* 57 */
    {KEY_CAPSLOCK,      KEY_CAPSLOCK  },    /* 58 */
    {KEY_F1,            KEY_F1        },    /* 59 */

    {KEY_F2,            KEY_F2        },    /* 60 */
    {KEY_F3,            KEY_F3        },    /* 61 */
    {KEY_F4,            KEY_F4        },    /* 62 */
    {KEY_F5,            KEY_F5        },    /* 63 */
    {KEY_F6,            KEY_F6        },    /* 64 */
    {KEY_F7,            KEY_F7        },    /* 65 */
    {KEY_F8,            KEY_F8        },    /* 66 */
    {KEY_F9,            KEY_F9        },    /* 67 */
    {KEY_F10,           KEY_F10       },    /* 68 */
    {KEY_NUMLOCK,       KEY_NUMLOCK   },    /* 69 */

    {KEY_SCROLLLOCK,    KEY_SCROLLLOCK},    /* 70 */
    {KEY_HOME,          '7'           },    /* 71 */
    {KEY_UP,            '8'           },    /* 72 */
    {KEY_PAGEUP,        '9'           },    /* 73 */
    {'-',               '-'           },    /* 74 */
    {KEY_LEFT,          '4'           },    /* 75 */
    {KEY_CENTER,        '5'           },    /* 76 */
    {KEY_RIGHT,         '6'           },    /* 77 */
    {'+',               '+'           },    /* 78 */
    {KEY_END,           '1'           },    /* 79 */

    {KEY_DOWN,          '2'           },    /* 80 */
    {KEY_PAGEDOWN,      '3'           },    /* 81 */
    {KEY_INS,           '0'           },    /* 82 */
    {KEY_DEL,           '.'           },    /* 83 */
    {KEY_NONE,          KEY_NONE      },    /* 84 */
    {KEY_NONE,          KEY_NONE      },    /* 85 */
    {KEY_NONE,          KEY_NONE      },    /* 86 */
    {KEY_F11,           KEY_F11       },    /* 87 */
    {KEY_F12,           KEY_F12       },    /* 88 */
};

BOOL kIsAlphabetScanCode(const BYTE bScanCode) {
    return 'a' <= gs_vstKeyMappingTable[bScanCode].bNormalCode &&
        gs_vstKeyMappingTable[bScanCode].bNormalCode <= 'z';
}

BOOL kIsNumberOrSymbolScanCode(const BYTE bScanCode) {
    return 2 <= bScanCode && bScanCode <= 53 && !kIsAlphabetScanCode(bScanCode);
}

BOOL kIsNumberPadScanCode(const BYTE bScanCode) {
    return 71 <= bScanCode && bScanCode <= 83;
}

BOOL kIsUseCombinedCode(const BYTE bScanCode) {
    BYTE bDownScanCode = bScanCode & 0x7f;
    if (kIsAlphabetScanCode(bDownScanCode)) {
        if (gs_stKeyboardManager.bShiftdown ^ gs_stKeyboardManager.bCapsLockOn) {
            return TRUE;
        }
    } else if (kIsNumberOrSymbolScanCode(bDownScanCode)) {
        if (gs_stKeyboardManager.bShiftdown) {
            return TRUE;
        }
    } else if (kIsNumberPadScanCode(bDownScanCode) && !gs_stKeyboardManager.bExtendedCodeIn) {
        if (gs_stKeyboardManager.bNumLockOn) {
            return TRUE;
        }
    }

    return FALSE;
}

void UpdateCombinationKeyStatus(const BYTE bScanCode) {
    const BYTE bDownScanCode = bScanCode & 0x7f;

    BOOL bDown;
    if (bScanCode & 0x80) {
        bDown = FALSE;
    } else {
        bDown = TRUE;
    }

    if (bDownScanCode == 42 || bDownScanCode == 54) {
        gs_stKeyboardManager.bShiftdown = bDown;
    } else if (bDownScanCode == 58 && bDown == TRUE) {
        gs_stKeyboardManager.bCapsLockOn ^= TRUE;
    } else if (bDownScanCode == 69 && bDown == TRUE) {
        gs_stKeyboardManager.bNumLockOn ^= TRUE;
    }
}

BOOL kConvertScanCodeToASCIICode(const BYTE bScanCode, BYTE *pbASCIICode, BOOL *pbFlags) {
    if (gs_stKeyboardManager.iSkipCountForPause > 0) {
        gs_stKeyboardManager.iSkipCountForPause--;
        return FALSE;
    }

    if (bScanCode == 0xe1) {
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
        return TRUE;
    } else if (bScanCode == 0xe0) {
        gs_stKeyboardManager.bExtendedCodeIn = TRUE;
        return FALSE;
    }

    BOOL bUseCombinedKey = kIsUseCombinedCode(bScanCode);
    if (bUseCombinedKey) {
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7f].bCombinedCode;
    } else {
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7f].bNormalCode;
    }

    if (gs_stKeyboardManager.bExtendedCodeIn) {
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = FALSE;
    } else {
        *pbFlags = 0;
    }

    if ((bScanCode & 0x80) == 0) {
        *pbFlags |= KEY_FLAGS_DOWN;
    }

    UpdateCombinationKeyStatus(bScanCode);
    return TRUE;
}

BOOL kInitializeKeyboard() {
    kInitializeQueue(&gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));

    return kActivateKeyboard();
}

BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode) {
    KEYDATA stData;
    stData.bScanCode = bScanCode;

    if (kConvertScanCodeToASCIICode(bScanCode, &stData.bASCIICode, &stData.bFlags)) {
        return kPutQueue(&gs_stKeyQueue, &stData);
    }

    return FALSE;
}

BOOL kGetKeyFromKeyQueue(KEYDATA *pstData) {
    if (kIsQueueEmpty(&gs_stKeyQueue)) {
        return FALSE;
    }

    return kGetQueue(&gs_stKeyQueue, pstData);
}
