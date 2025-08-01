#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT  300
#define CONSOLESHELL_PROMPTMESSAGE          "sh$ "

typedef void (*CommandFunction)(const char *pcParameter);

#pragma pack(push, 1)

typedef struct kShellCommandEntryStruct {
    char *pcCommand;
    char *pcHelp;
    CommandFunction pfFunction;
} SHELLCOMMANENTRY;

typedef struct kParameterListStruct {
    const char *pcBuffer;
    int iLength;
    int iCurrentPosition;
} PARAMETERLIST;

#pragma pack(pop)

// Shell
void kStartConsoleShell();
void kExecuteCommand(const char *pcParameterBuffer);
void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter);
int kGetNextParameter(PARAMETERLIST *pstList, char *pcParameter);

// Command
static void kHelp(const char *pcParameterBuffer);
static void kCls(const char *pcParameterBuffer);
static void kShowTotalRAMSize(const char *pcParameterBuffer);
static void kStringToDecimalHexTest(const char *pcParameterBuffer);
static void kShutdown(const char *pcParameterBuffer);

static void kSetTimer(const char *pcParameterBuffer);
static void kWaitUsingPIT(const char *pcParameterBuffer);
static void kReadTimeStampCounter(const char * pcParameterBuffer);
static void kMeasureProcessorSpeed(const char *pcParameterBuffer);
static void kShowDateAndTime(const char *pcParameterBuffer);

static void kCreateTestTask(const char *pcParameterBuffer);
static void kChangeTaskPriority(const char *pcParameterBuffer);
static void kShowTaskList(const char *pcParameterBuffer);
static void kKillTask(const char *pcParameterBuffer);
static void kCPULoad(const char *pcParameterBuffer);

static void kTestMutex(const char *pcParameterBuffer);
static void kCreateThreadTask();
static void kTestThread(const char *pcParameterBuffer);
static void kShowMatrix(const char *pcParameterBuffer);
static void kTestPIE(const char *pcParameterBuffer);

// Dynamic Memory
static void kShowDynamicMemoryInformation(const char *pcParameterBuffer);
static void kTestSequentialAllocation(const char *pcParameterBuffer);
static void kTestRandomAllocation(const char *pbCurrentBitmapPosition);
static void kRandomAllocationTask();

// HDD Driver
static void kShowHDDInformation(const char *pcParameterBuffer);
static void kReadSector(const char *pcParameterBuffer);
static void kWriteSector(const char *pcParameterBuffer);

// FileSystem
static void kMountHDD(const char *pcParameterBuffer);
static void kFormatHDD(const char *pcParameterBuffer);
static void kShowFileSystemInformation(const char *pcParameterBuffer);
static void kCreateFileInRootDirectory(const char *pcParameterBuffer);
static void kDeleteFileInRootDirectory(const char *pcParameterBuffer);
static void kShowRootDirectory(const char *pcParameterBuffer);
static void kWriteDataToFile(const char *pcParameterBuffer);
static void kReadDataFromFile(const char *pcParameterBuffer);
static void kTestFileIO(const char *pcParameterBuffer);
static void kTestPerformance(const char *pcParameterBuffer);
static void kFlushCache(const char *pcParameterBuffer);

// SerialPort
static void kDownloadFile(const char *pcParameterBuffer);

// MP
static void kShowMPConfigurationTable(const char *pcParameterBuffer);
static void kStartApplicationProcessor(const char *pcParameterBuffer);
static void kStartSymmetricIOMode(const char *pcParameterBuffer);
static void kShowIRQINTINMappingTable(const char *pcParameterBuffer);

#endif
