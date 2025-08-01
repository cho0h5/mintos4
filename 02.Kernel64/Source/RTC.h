#ifndef __RTC_H__
#define __RTC_H__

#include "Types.h"

#define RTC_CMOSADDRESS         0x70
#define RTC_CMOSDATA            0x71

#define RTC_ADDRESS_SECOND      0x00
#define RTC_ADDRESS_MINUTE      0x02
#define RTC_ADDRESS_HOUR        0x04
#define RTC_ADDRESS_DAYOFWEEK   0x06
#define RTC_ADDRESS_DAYOFMONTH  0x07
#define RTC_ADDRESS_MONTH       0x08
#define RTC_ADDRESS_YEAR        0x09

#define RTC_BCDTOBINARY(x)      ((((x) >> 4) * 10) + ((x) & 0x0f))

void kReadRTCTime(BYTE *pbHour, BYTE *pbMinute, BYTE *pbSecond);
void kReadRTCDate(WORD *pbYear, BYTE *pbMonth, BYTE *pbDayOfMonth, BYTE *pbDayOfWeek);
const char *kConvertDayOfWeekToString(const BYTE bDayOfWeek);

#endif
