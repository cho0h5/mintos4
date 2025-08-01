#ifndef __LOCALAPIC_H__
#define __LOCALAPIC_H__

#include "Types.h"

#define APIC_REGISTER_EOI                           0x0000b0
#define APIC_REGISTER_SVR                           0x0000f0
#define APIC_REGISTER_APICID                        0x000020
#define APIC_REGISTER_TASKPRIORITY                  0x000080
#define APIC_REGISTER_TIMER                         0x000320
#define APIC_REGISTER_THERMALSENSOR                 0x000330
#define APIC_REGISTER_PERFORMANCEMONITOIRINGCOUNTER 0x000340
#define APIC_REGISTER_LINT0                         0x000350
#define APIC_REGISTER_LINT1                         0x000360
#define APIC_REGISTER_ERROR                         0x000370
#define APIC_REGISTER_ICR_LOWER                     0x000300
#define APIC_REGISTER_ICR_UPPER                     0x000310

#define APIC_DELIVERYMODE_FIXED                     0x000000
#define APIC_DELIVERYMODE_LOWESTPRIORITY            0x000100
#define APIC_DELIVERYMODE_SMI                       0x000200
#define APIC_DELIVERYMODE_NMI                       0x000400
#define APIC_DELIVERYMODE_INIT                      0x000500
#define APIC_DELIVERYMODE_STARTUP                   0x000600
#define APIC_DELIVERYMODE_EXTINT                    0x000700

#define APIC_DESTINATIONMODE_PHYSICAL               0x000000
#define APIC_DESTINATIONMODE_LOGICAL                0x000800

#define APIC_DELIVERYSTATUS_IDLE                    0x000000
#define APIC_DELIVERYSTATUS_PENDING                 0x001000

#define APIC_LEVEL_DEASSERT                         0x000000
#define APIC_LEVEL_ASSERT                           0x004000

#define APIC_TRIGGERMODE_EDGE                       0x000000
#define APIC_TRIGGERMODE_LEVEL                      0x008000

#define APIC_DESTINATIONSHORTHAND_NOSHORTHAND       0x000000
#define APIC_DESTINATIONSHORTHAND_SELF              0x040000
#define APIC_DESTINATIONSHORTHAND_ALLINCLUDINGSELF  0x080000
#define APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF  0x0c0000

#define APIC_INTERRUPT_MASK                         0x010000

#define APIC_TIMERMODE_PERIODIC                     0x020000
#define APIC_TIMERMODE_ONESHOT                      0x000000

#define APIC_POLARITY_ACTIVELOW                     0x002000
#define APIC_POLARITY_ACTIVEHIGH                    0x000000

QWORD kGetLocalAPICBaseAddress();
void kEnableSoftwareLocalAPIC();
void kSendEOIToLocalAPIC();
void kSetTaskPriority(const BYTE bPriority);
void kInitializeLocalVectorTable();

#endif
