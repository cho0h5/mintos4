#ifndef __LOCALAPIC_H__
#define __LOCALAPIC_H__

#include "Types.h"

#define APIC_REGISTER_SVR                           0x0000f0
#define APIC_REGISTER_APICID                        0x000020
#define APIC_REGISTER_ICR_LOWER                     0x000300

#define APIC_DELIVERYMODE_INIT                      0x000500
#define APIC_DELIVERYMODE_STARTUP                   0x000600

#define APIC_DESTINATIONMODE_PHYSICAL               0x000000

#define APIC_DELIVERYSTATUS_PENDING                 0x001000

#define APIC_LEVEL_ASSERT                           0x004000

#define APIC_TRIGGERMODE_EDGE                      0x000000
#define APIC_TRIGGERMODE_LEVEL                      0x008000

#define APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF  0x0c0000

QWORD kGetLocalAPIBaseAddress();
void kEnableSoftwareLocalAPIC();

#endif
