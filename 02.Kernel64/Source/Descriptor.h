#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

#define GDT_TYPE_CODE               0x0a
#define GDT_TYPE_DATA               0x02
#define GDT_TYPE_TSS                0x09
#define GDT_FLAGS_LOWER_S           0x10
#define GDT_FLAGS_LOWER_DPL0        0x00
#define GDT_FLAGS_LOWER_DPL1        0x20
#define GDT_FLAGS_LOWER_DPL2        0x40
#define GDT_FLAGS_LOWER_DPL3        0x60
#define GDT_FLAGS_LOWER_P           0x80
#define GDT_FLAGS_UPPER_L           0x20
#define GDT_FLAGS_UPPER_DB          0x40
#define GDT_FLAGS_UPPER_G           0x80

#define GDT_FLAGS_LOWER_KERNELCODE  (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA  (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS         (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE    (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA    (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_UPPER_CODE        (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA        (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS         (GDT_FLAGS_UPPER_G)

#define GDT_MAXENTRY8COUNT          3
#define GDT_MAXENTRY16COUNT         1
#define GDT_TABLESIZE               sizeof(GDTENTRY8) * GDT_MAXENTRY8COUNT + sizeof(GDTENTRY16) * GDT_MAXENTRY16COUNT

#define IST_STARTADDRESS            0x700000
#define IST_SIZE                    0x100000

#pragma pack(push, 1)

typedef struct kGDTRStruct {
    WORD wLimit;
    QWORD qwBaseAddress;
    WORD wPading;
    DWORD dwPading;
} GDTR, IDTR;

typedef struct kGDTEntry8Struct {
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bUpperBaseAddress1;
    BYTE bTypeAndLowerFlag;
    BYTE bUpperLimitAndUpperFlag;
    BYTE bUpperBaseAddress2;
} GDTENTRY8;

typedef struct kGDTEntry16Struct {
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bMiddleBaseAddress1;
    BYTE bTypeAndLowerFlag;
    BYTE bUpperLimitAndUpperFlag;
    BYTE bMiddleBaseAddress2;
    DWORD dwUpperBaseAddress;
    DWORD dwReserved;
} GDTENTRY16;

typedef struct kTSSDataStruct {
    DWORD dwReserved1;
    QWORD qwRsp[3];
    QWORD qwReserved2;
    QWORD qwIST[7];
    QWORD qwReserved3;
    WORD wReserved;
    WORD wIOMapBaseAddress;
} TSSSEGMENT;

#pragma pack(pop)

void kInitializeGDTTableAndTSS();
void kSetGDTEntry8(GDTENTRY8 *pstEntry, DWORD dwBaseAddress, DWORD dwLimit,
        BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kSetGDTEntry16(GDTENTRY16 *pstEntry, QWORD qwBaseAddress, DWORD dwLimit,
        BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kInitializeTSSSegment(TSSSEGMENT *pstTSS);

#endif
