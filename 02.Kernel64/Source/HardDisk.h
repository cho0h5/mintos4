#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "Types.h"
#include "Syncronization.h"

#define HDD_PORT_PRIMARYBASE                0x1f0
#define HDD_PORT_SECONDARYBASE              0x170

#define HDD_PORT_INDEX_DATA                 0x00
#define HDD_PORT_INDEX_SECTORCOUNT          0x02
#define HDD_PORT_INDEX_SECTORNUMBER         0x03
#define HDD_PORT_INDEX_CYLINDERLSB          0x04
#define HDD_PORT_INDEX_CYLINDERMSB          0x05
#define HDD_PORT_INDEX_DRIVEANDHEAD         0x06
#define HDD_PORT_INDEX_STATUS               0x07
#define HDD_PORT_INDEX_COMMAND              0x07
#define HDD_PORT_INDEX_DIGITALOUTPUT        0x206

#define HDD_COMMAND_READ                    0x20
#define HDD_COMMAND_WRITE                   0x30
#define HDD_COMMAND_IDENTIFY                0xec

#define HDD_STATUS_ERROR                    0x01
#define HDD_STATUS_INDEX                    0x02
#define HDD_STATUS_CORRECTEDDATA            0x04
#define HDD_STATUS_DATAREQUEST              0x08
#define HDD_STATUS_SEEKCOMPLETE             0x10
#define HDD_STATUS_WRITEFAULT               0x20
#define HDD_STATUS_READY                    0x40
#define HDD_STATUS_BUSY                     0x80

#define HDD_DRIVEANDHEAD_LBA                0xe0
#define HDD_DRIVEANDHEAD_SLAVE              0x10

#define HDD_DIGITALOUTPUT_RESET             0x04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT  0x01

#define HDD_WAITTIME                        500
#define HDD_MAXBULKSECTORCOUNT              256

#pragma pack(push, 1)

typedef struct kHDDInformationStruct {
    WORD wConfiguration;

    WORD wNumberOfCylinder;
    WORD wReserved1;

    WORD wNumberOfHead;
    WORD wUnformattedBytesPerTrack;
    WORD wUnformattedBytesPerSector;

    WORD wNumberOfSectorPerCylinder;
    WORD wInterSectorGap;
    WORD wBytesInPhaseLock;
    WORD wNumberOfVendorUniqueStatusWord;

    WORD vwSerialNumber[10];
    WORD wControllerType;
    WORD wBufferSize;
    WORD wNumberOfECCBytes;
    WORD vwFirmwareRevision[4];

    WORD vwModelNumber[20]; // Important
    WORD vwReserved2[13];

    DWORD dwTotalSectors;   // Important
    WORD vwReserved3[196];
} HDDINFORMATION;

#pragma pack(pop)

typedef struct kHDDManagerStruct {
    BOOL bHDDDetected;
    BOOL bCanWrite;

    volatile BOOL bPrimaryInterruptOccur;
    volatile BOOL bSecondaryInterruptOccur;
    MUTEX stMutex;

    HDDINFORMATION stHDDInformation;
} HDDMANAGER;

BOOL kInitializeHDD();
static BYTE kReadHDDStatus(const BOOL bPrimary);
static BOOL kWaitForHDDNoBusy(const BOOL bPrimary);
static BOOL kWaitForHDDReady(const BOOL bPrimary);
void kSetHDDInterruptFlag(const BOOL bPrimary, const BOOL bFlag);
static BOOL kWaitForHDDInterrupt(const BOOL bPrimary);
BOOL kReadHDDInformation(const BOOL bPrimary, const BOOL bMaster, HDDINFORMATION *pstHDDInformation);
static void kSwapByteInWord(WORD *pwData, const int iWordCount);

// Read
int kReadHDDSector(const BOOL bPrimary, const BOOL bMaster, const DWORD dwLBA,
        const int iSectorCount, char *pcBuffer);

// Write
int kWriteHDDSector(const BOOL bPrimary, const BOOL bMaster, const DWORD dwLBA,
        const int iSectorCount, const char *pcBuffer);

#endif
