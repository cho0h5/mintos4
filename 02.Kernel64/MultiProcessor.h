#ifndef __MULTIPROCESSOR_H__
#define __MULTIPROCESSOR_H__

#include "Types.h"

#define BOOTSTRAPPROCESSOR_FLAGADDRESS 0x7c09
#define MAXPROCESSORCOUNT 16

BOOL kStartUpApplicationProcessor();
static BOOL kWakeUpApplicationProcessor();
BYTE kGetAPICID();

#endif
