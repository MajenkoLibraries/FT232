// Creates a CDC/ACM port and an FT232 port. Data to one is
// forwarded to the other.

// Set the USB profile to "Custom"

#include <FT232.h>

USBFS usbDevice;
USBManager USB(usbDevice, 0x0403, 0x6001); // These idenify it as FT232.
FT232 ft232Serial;
CDCACM cdcSerial;

void setup() {
    USB.addDevice(ft232Serial);
    USB.addDevice(cdcSerial);
    USB.begin();
}

void loop() {
    if (ft232Serial.available()) {
        cdcSerial.write(ft232Serial.read());
    }
    if (cdcSerial.available()) {
        ft232Serial.write(cdcSerial.read());
    }
}

