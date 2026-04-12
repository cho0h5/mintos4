#include "SerialPort.h"
#include "AssemblyUtility.h"
#include "Utility.h"

static SERIALMANAGER gs_stSerialManager;

void kInitializeSerialPort() {
    const WORD wPortBaseAddress = SERIAL_PORT_COM1;

    kInitializeMutex(&gs_stSerialManager.stLock);

    // Disable all interrupts
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_INTERRUPTENABLE, 0);

    // Set Baudrate
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL, SERIAL_LINECONTROL_DLAB);
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHLSB, SERIAL_DIVISORLATCH_115200);
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHMSB, SERIAL_DIVISORLATCH_115200 >> 8);

    // Set line
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL,
            SERIAL_LINECONTROL_8BIT | SERIAL_LINECONTROL_NOPARITY | SERIAL_LINECONTROL_1BITSTOP);

    // Set FIFO
    kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_FIFOCONTROL,
            SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO);
}

static BOOL kIsSerialTransmitterBufferEmpty() {
    const BYTE bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
    if ((bData & SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) == SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) {
        return TRUE;
    }
    return FALSE;
}

void kSendSerialData(const BYTE *pbBuffer, const int iSize) {
    kLock(&gs_stSerialManager.stLock);

    int iSentByte = 0;
    while (iSentByte < iSize) {
        while (!kIsSerialTransmitterBufferEmpty()) {
            kSleep(0);
        }

        const int iTempSize = MIN(iSize - iSentByte, SERIAL_FIFOMAXSIZE);
        for (int j = 0; j < iTempSize; j++) {
            kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_TRANSMITBUFFER, pbBuffer[iSentByte + j]);
        }
        iSentByte += iTempSize;
    }

    kUnlock(&gs_stSerialManager.stLock);
}

static BOOL kIsSerialReceiveBufferFull() {
    const BYTE bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
    if ((bData & SERIAL_LINESTATUS_RECEIVEDATAREADY) == SERIAL_LINESTATUS_RECEIVEDATAREADY) {
        return TRUE;
    }
    return FALSE;
}

int kReceiveSerialData(BYTE *pbBuffer, const int iSize) {
    kLock(&gs_stSerialManager.stLock);

    int i = 0;
    for (; i < iSize; i++) {
        if (!kIsSerialReceiveBufferFull()) {
            break;
        }

        pbBuffer[i] = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_RECEIVEBUFFER);
    }

    kUnlock(&gs_stSerialManager.stLock);

    return i;
}

void kClearSerialFIFO() {
    kLock(&gs_stSerialManager.stLock);

    kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_FIFOCONTROL,
            SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO |
            SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO | SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO);

    kUnlock(&gs_stSerialManager.stLock);
}
