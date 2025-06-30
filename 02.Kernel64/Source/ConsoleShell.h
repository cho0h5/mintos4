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
void kHelp(const char *pcParameterBuffer);
void kCls(const char *pcParameterBuffer);
void kShowTotalRAMSize(const char *pcParameterBuffer);
void kStringToDecimalHexTest(const char *pcParameterBuffer);
void kShutdown(const char *pcParameterBuffer);
void kSetTimer(const char *pcParameterBuffer);
void kWaitUsingPIT(const char *pcParameterBuffer);
void kReadTimeStampCounter(const char * pcParameterBuffer);
void kMeasureProcessorSpeed(const char *pcParameterBuffer);
void kShowDateAndTime(const char *pcParameterBuffer);
void kCreateTestTask(const char *pcParameterBuffer);

#endif
