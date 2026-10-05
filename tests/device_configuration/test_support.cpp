#include "test_support.h"

#include <assert.h>

TestEvent test_events[8];
uint8_t test_event_count;
uint8_t initialized_rhport = 0xff;

Adafruit_USBD_CDC SerialTinyUSB;

void record_event(TestEvent event) {
  assert(test_event_count < sizeof(test_events) / sizeof(test_events[0]));
  test_events[test_event_count++] = event;
}

void Adafruit_USBD_CDC::begin(uint32_t baud) {
  assert(baud == 115200);
  record_event(EVENT_DEFAULT_CDC_BEGIN);
  assert(TinyUSBDevice.addInterface(*this));
}

uint16_t Adafruit_USBD_CDC::getInterfaceDescriptor(
    uint8_t itfnum_deprecated, uint8_t *buf, uint16_t bufsize) {
  (void)itfnum_deprecated;
  assert(buf != nullptr);
  assert(bufsize >= 1);
  record_event(EVENT_DEFAULT_CDC_DESCRIPTOR);
  TinyUSBDevice.allocInterface(2);
  buf[0] = 0xcd;
  return 1;
}

void TinyUSB_Port_InitDevice(uint8_t rhport) {
  record_event(EVENT_HARDWARE_INIT);
  initialized_rhport = rhport;
}

