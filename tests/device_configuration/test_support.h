#pragma once

#include <stdint.h>

#define ADAFRUIT_USBD_CDC_H_
#include "Adafruit_USBD_Device.h"

enum TestEvent : uint8_t {
  EVENT_DEFAULT_CDC_BEGIN,
  EVENT_DEFAULT_CDC_DESCRIPTOR,
  EVENT_CUSTOM_HOOK,
  EVENT_CUSTOM_DESCRIPTOR,
  EVENT_HARDWARE_INIT,
};

extern TestEvent test_events[8];
extern uint8_t test_event_count;
extern uint8_t initialized_rhport;

void record_event(TestEvent event);

class Adafruit_USBD_CDC : public Adafruit_USBD_Interface {
public:
  void begin(uint32_t baud);
  uint16_t getInterfaceDescriptor(uint8_t itfnum_deprecated, uint8_t *buf,
                                  uint16_t bufsize) override;
};

extern Adafruit_USBD_CDC SerialTinyUSB;

