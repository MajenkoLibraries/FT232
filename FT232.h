#ifndef _FT232_H
#define _FT232_H

#include <Arduino.h>

#define FTDI_SIO_RESET              0x4000 /* Reset the port */
#define FTDI_SIO_MODEM_CTRL         0x4001 /* Set the modem control register */
#define FTDI_SIO_SET_FLOW_CTRL      0x4002 /* Set flow control register */
#define FTDI_SIO_SET_BAUD_RATE      0x4003 /* Set baud rate */
#define FTDI_SIO_SET_DATA           0x4004 /* Set the data characteristics of the port */
#define FTDI_SIO_GET_MODEM_STATUS   0xC005 /* Retrieve current value of modem status register */
#define FTDI_SIO_SET_EVENT_CHAR     0x4006 /* Set the event character */
#define FTDI_SIO_SET_ERROR_CHAR     0x4007 /* Set the error character */
#define FTDI_SIO_SET_LATENCY_TIMER  0x4009 /* Set the latency timer */
#define FTDI_SIO_GET_LATENCY_TIMER  0xc00A /* Get the latency timer */

class FT232 : public USBDevice, public Stream {
    private:
        uint8_t _ifBulk;
        uint8_t _epBulk;
        USBManager *_manager;

        uint8_t _lineState;
        uint32_t _lastPacket;

#if defined (__PIC32MX__)
#define FT232_BUFFER_SIZE 256
#define FT232_BULKEP_SIZE 64
        uint8_t _rxBuffer[FT232_BUFFER_SIZE];
        uint8_t _txBuffer[FT232_BUFFER_SIZE];
        uint8_t _bulkRxA[FT232_BULKEP_SIZE];
        uint8_t _bulkRxB[FT232_BULKEP_SIZE];
        uint8_t _bulkTxA[FT232_BULKEP_SIZE];
        uint8_t _bulkTxB[FT232_BULKEP_SIZE];
#define FT232_BUFFER_HIGH 4
#elif defined(__PIC32MZ__)
#define FT232_BUFFER_SIZE 256
#define FT232_BULKEP_SIZE 512
        uint8_t _rxBuffer[FT232_BUFFER_SIZE];
        uint8_t _txBuffer[FT232_BUFFER_SIZE];
        uint8_t _bulkRxA[FT232_BULKEP_SIZE];
        uint8_t _bulkRxB[FT232_BULKEP_SIZE];
        uint8_t _bulkTxA[FT232_BULKEP_SIZE];
        uint8_t _bulkTxB[FT232_BULKEP_SIZE];
#define FT232_BUFFER_HIGH 8
#endif

        volatile uint32_t _txHead;
        volatile uint32_t _txTail;
        volatile uint32_t _rxHead;
        volatile uint32_t _rxTail;

    public:
        FT232() : _lineState(0), _lastPacket(0), _txHead(0), _txTail(0), _rxHead(0), _rxTail(0) {}

        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen);
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        void configureEndpoints();
        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        void onEnumerated();

        // Stream

        size_t write(uint8_t);
        size_t write(const uint8_t *b, size_t len);
        int available();
        int read();
        int peek();
        void flush();
        void begin() {}
        void begin(uint32_t __attribute__((unused)) baud) {}
        void end() {}
        int availableForWrite();
        operator int();
};

#endif
