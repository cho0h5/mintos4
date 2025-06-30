#include "RTC.h"
#include "AssemblyUtility.h"

void kReadRTCTime(BYTE *pbHour, BYTE *pbMinute, BYTE *pbSecond) {
    // Hour
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
    BYTE bData = kInPortByte(RTC_CMOSDATA);
    *pbHour = RTC_BCDTOBINARY(bData);

    // Minute
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMinute = RTC_BCDTOBINARY(bData);
    
    // Second
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbSecond = RTC_BCDTOBINARY(bData);
}

void kReadRTCDate(WORD *pbYear, BYTE *pbMonth, BYTE *pbDayOfMonth, BYTE *pbDayOfWeek) {
    // Year
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
    BYTE bData = kInPortByte(RTC_CMOSDATA);
    *pbYear = RTC_BCDTOBINARY(bData);

    // Month
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMonth = RTC_BCDTOBINARY(bData);
    
    // Day of month
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbDayOfMonth = RTC_BCDTOBINARY(bData);

    // Day of week
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbDayOfWeek = RTC_BCDTOBINARY(bData);
}

char *kConvertDayOfWeekToString(const BYTE bDayOfWeek) {
    static const char *vpcDayOfWeekString[8] = {"Error", "Sunday", "Monday",
        "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    if (bDayOfWeek >= 8) {
        return vpcDayOfWeekString[0];
    }

    return vpcDayOfWeekString[bDayOfWeek];
}
