#include <FT232.h>

uint16_t FT232::getDescriptorLength() {
    return 9 + 7 + 7;
}

uint8_t FT232::getInterfaceCount() {
    return 1;
}

bool FT232::getStringDescriptor(uint8_t __attribute__((unused)) idx, uint16_t __attribute__((unused)) maxlen) {
    return false;
}

uint32_t FT232::populateConfigurationDescriptor(uint8_t *buf) {
    uint8_t i = 0;

    // Interface Descriptor
    buf[i++] = 0x09;            // bLength
    buf[i++] = 0x04;            // bDescriptorType
    buf[i++] = _ifBulk;         // bInterfaceNumber
    buf[i++] = 0x00;            // bAlternateSetting
    buf[i++] = 0x02;            // bNumEndpoints
    buf[i++] = 0xff;            // bInterfaceClass
    buf[i++] = 0xff;            // bInterafceSubClass
    buf[i++] = 0xff;            // bInterfaceProtocol
    buf[i++] = 0x02;            // iInterface

    // Bulk-In Endpoint
    buf[i++] = 0x07;            // bLength
    buf[i++] = 0x05;            // bDescriptorType
    buf[i++] = 0x80 | _epBulk;  // bEndpointAddress
    buf[i++] = 0x02;            // bmAttributes
    if (_manager->isHighSpeed()) {
        buf[i++] = 0x00;
        buf[i++] = 0x02;        // packet size
    } else {
        buf[i++] = 0x40;
        buf[i++] = 0x00;        // packet size
    }
    buf[i++] = 0x00;            // bInterval

    // Bulk-Out Endpoint
    buf[i++] = 0x07;            // bLength
    buf[i++] = 0x05;            // bDescriptorType
    buf[i++] = _epBulk;         // bEndpointAddress
    buf[i++] = 0x02;            // bmAttributes
    if (_manager->isHighSpeed()) {
        buf[i++] = 0x00;
        buf[i++] = 0x02;        // packet size
    } else {
        buf[i++] = 0x40;
        buf[i++] = 0x00;        // packet size
    }
    buf[i++] = 0x00;            // bInterval

    return i;
}

void FT232::initDevice(USBManager *manager) {
    _manager = manager;
    _ifBulk = _manager->allocateInterface();
    _epBulk = _manager->allocateEndpoint();
}

bool FT232::getDescriptor(uint8_t __attribute__((unused)) ep, uint8_t __attribute__((unused)) target, uint8_t __attribute__((unused)) id, uint8_t __attribute__((unused)) maxlen) {
    return false;
}

bool FT232::getReportDescriptor(uint8_t __attribute__((unused)) ep, uint8_t __attribute__((unused)) target, uint8_t __attribute__((unused)) id, uint8_t __attribute__((unused)) maxlen) {
    return false;
}

void FT232::configureEndpoints() {
    if (_manager->isHighSpeed()) {
        _manager->addEndpoint(_epBulk, EP_IN, EP_BLK, 512, _bulkRxA, _bulkRxB);
        _manager->addEndpoint(_epBulk, EP_OUT, EP_BLK, 512, _bulkTxA, _bulkTxB);
    } else {
        _manager->addEndpoint(_epBulk, EP_IN, EP_BLK, 64, _bulkRxA, _bulkRxB);
        _manager->addEndpoint(_epBulk, EP_OUT, EP_BLK, 64, _bulkTxA, _bulkTxB);
    }
}

bool FT232::onSetupPacket(uint8_t ep __attribute__((unused)), uint8_t __attribute__((unused)) target, uint8_t __attribute__((unused)) *data, uint32_t __attribute__((unused)) l) {

    uint16_t wRequest = (data[0] << 8) | data[1];
    uint16_t wValue = (data[2] << 8) | data[3];

    switch (wRequest) {
        case FTDI_SIO_RESET:
            if ((wValue == 0) || (wValue == 1)) {
                _rxHead = _rxTail = 0;
            }
            if ((wValue == 0) || (wValue == 2)) {
                _txHead = _txTail = 0;
            }
            _manager->sendBuffer(0, NULL, 0);
            return true;
            break;
        case FTDI_SIO_MODEM_CTRL:
            _lineState = wValue >> 8;
            _manager->sendBuffer(0, NULL, 0);
            return true;
            break;
        case FTDI_SIO_SET_FLOW_CTRL:
            _manager->sendBuffer(0, NULL, 0);
            return true;
            break;
        case FTDI_SIO_SET_BAUD_RATE:
            _manager->sendBuffer(0, NULL, 0);
            return true;
            break;
        case FTDI_SIO_SET_DATA:
            _manager->sendBuffer(0, NULL, 0);
            return true;
            break;
        case FTDI_SIO_GET_MODEM_STATUS: {
                uint8_t stat = 0xF0;
                _manager->sendBuffer(0, &stat, 1);
                return true;
            }
            break;
        case FTDI_SIO_SET_EVENT_CHAR:
            _manager->sendBuffer(0, NULL, 0);
            return true;
            break;
        case FTDI_SIO_SET_ERROR_CHAR:
            _manager->sendBuffer(0, NULL, 0);
            return true;
            break;
        case FTDI_SIO_SET_LATENCY_TIMER:
            _manager->sendBuffer(0, NULL, 0);
            return true;
            break;
        case FTDI_SIO_GET_LATENCY_TIMER: {
                uint8_t latency = 5;
                _manager->sendBuffer(0, &latency, 1);
                return true;
            }
            break;
    }
    return false;
}

bool FT232::onOutPacket(uint8_t ep, uint8_t __attribute__((unused)) target, uint8_t *data, uint32_t l) {
    if (ep == _epBulk) {
        for (uint32_t i = 0; i < l; i++) {
            uint32_t bufIndex = (_rxHead + 1) % FT232_BUFFER_SIZE;
            if (bufIndex != _rxTail) {
                _rxBuffer[_rxHead] = data[i];
                _rxHead = bufIndex;
            }
        }

        int remaining = (_rxTail - _rxHead + FT232_BUFFER_SIZE) % FT232_BUFFER_SIZE;

        if ((remaining < FT232_BUFFER_HIGH) && (_rxHead != _rxTail)) {
            _manager->haltEndpoint(_epBulk);
        }
        return true;
    }
    return false;
}

bool FT232::onInPacket(uint8_t ep, uint8_t __attribute__((unused)) target, uint8_t __attribute__((unused)) *data, uint32_t __attribute__((unused)) l) {

    if (ep == _epBulk) {
        uint32_t avail = (FT232_BUFFER_SIZE + _txHead - _txTail) % FT232_BUFFER_SIZE;
        if (avail == 0) {
            if (millis() - _lastPacket >= 40) {
                _lastPacket = millis();
                uint8_t tbuf[2];
                tbuf[0] = 0xFF;
                tbuf[1] = 0x00;
                _manager->enqueuePacket(_epBulk, tbuf, 2);
                return true;
            }
            return false;
        }

        if (avail > FT232_BULKEP_SIZE) {
            avail = FT232_BULKEP_SIZE;
        }

        uint8_t tbuf[avail + 2];

        tbuf[0] = 0xFF;
        tbuf[1] = 0x00;

        for (uint32_t i = 0; i < avail; i++) {
            _txTail = (_txTail + 1) % FT232_BUFFER_SIZE;
            tbuf[i + 2] = _txBuffer[_txTail];
        }
        _manager->enqueuePacket(_epBulk, tbuf, avail + 2);
        _lastPacket = millis();
        return true;
    }

    return false;
}

void FT232::onEnumerated() {
}

size_t FT232::write(uint8_t b) {

    if (_lineState == 0) return 0;

    volatile uint32_t h = _txHead;
    volatile uint32_t newhead = (h + 1) % FT232_BUFFER_SIZE;

    // Wait for room
    while (newhead == _txTail) {
        h = _txHead;
        newhead = (h + 1) % FT232_BUFFER_SIZE;
    }

    _txBuffer[newhead] = b;
    _txHead = newhead;

    if (_manager->canEnqueuePacket(_epBulk)) {
        uint32_t avail = (FT232_BUFFER_SIZE + _txHead - _txTail) % FT232_BUFFER_SIZE;
        if (avail > FT232_BULKEP_SIZE) {
            avail = FT232_BULKEP_SIZE;
        }

        uint8_t tbuf[avail + 2];

        tbuf[0] = 0xFF;
        tbuf[1] = 0x0;

        for (uint32_t i = 0; i < avail; i++) {
            _txTail = (_txTail + 1) % FT232_BUFFER_SIZE;
            tbuf[i + 2] = _txBuffer[_txTail];
        }
        _manager->enqueuePacket(_epBulk, tbuf, avail + 2);
        _lastPacket = millis();
    }

//    _manager->sendBuffer(_epBulk, &b, 1);
    return 1;
}

size_t FT232::write(const uint8_t *b, size_t len) {

    if (_lineState == 0) return 0;
    for (uint32_t i = 0; i < len; i++) {
        write(b[i]);
    }
/*

    size_t pos = 0;
    int32_t slen = len;
    uint32_t packetSize = _manager->isHighSpeed() ? 512 : 64;
    while (pos < len) {
        int32_t toSend = min((int32_t)packetSize, slen);
        while (!_manager->canEnqueuePacket(_epBulk)) {
        }
        _manager->enqueuePacket(_epBulk, &b[pos], toSend);
        pos += toSend;
        slen -= toSend;
    }
*/
    return len;
}

int FT232::available() {
    return (FT232_BUFFER_SIZE + _rxHead - _rxTail) % FT232_BUFFER_SIZE;
}

int FT232::availableForWrite() {
    return (FT232_BUFFER_SIZE + _txTail - _txHead) % FT232_BUFFER_SIZE;
}

int FT232::read() {
    if (_rxHead == _rxTail) return -1;
    int prevremaining = (_rxTail - _rxHead + FT232_BUFFER_SIZE) % FT232_BUFFER_SIZE;

    uint8_t ch = _rxBuffer[_rxTail];
    _rxTail = (_rxTail + 1) % FT232_BUFFER_SIZE;

    int remaining = (_rxTail - _rxHead + FT232_BUFFER_SIZE) % FT232_BUFFER_SIZE;

    if (prevremaining < FT232_BUFFER_HIGH) {
        if ((remaining >= FT232_BUFFER_HIGH) || (_rxHead == _rxTail)) {
            _manager->resumeEndpoint(_epBulk);
        }
    }

    return ch;
}

void FT232::flush() {
}

FT232::operator int() {
    return _lineState > 0;
}

int FT232::peek() {
    if (_rxHead == _rxTail) return -1;
    return _rxBuffer[_rxTail];
}

