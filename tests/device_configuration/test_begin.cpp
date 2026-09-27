#include "test_support.h"

#include <assert.h>

#ifndef HOOK_RESULT
#define HOOK_RESULT -1
#endif

class CustomInterface : public Adafruit_USBD_Interface {
public:
  uint16_t getInterfaceDescriptor(uint8_t itfnum_deprecated, uint8_t *buf,
                                  uint16_t bufsize) override {
    (void)itfnum_deprecated;
    assert(buf != nullptr);
    assert(bufsize >= 1);
    record_event(EVENT_CUSTOM_DESCRIPTOR);
    TinyUSBDevice.allocInterface();
    buf[0] = 0xc0;
    return 1;
  }
};

#if HOOK_RESULT >= 0
bool TinyUSB_Device_Configure(Adafruit_USBD_Device &device) {
  record_event(EVENT_CUSTOM_HOOK);

  tusb_desc_configuration_t const *config =
      reinterpret_cast<tusb_desc_configuration_t const *>(
          tud_descriptor_configuration_cb(0));
  assert(config->bNumInterfaces == 0);
  assert(config->wTotalLength == sizeof(tusb_desc_configuration_t));

#if HOOK_RESULT
  static CustomInterface custom_interface;
  assert(device.addInterface(custom_interface));
  return true;
#else
  (void)device;
  return false;
#endif
}
#endif

int main() {
  assert(TinyUSBDevice.begin(7) == (HOOK_RESULT != 0));

  tusb_desc_device_t const *device =
      reinterpret_cast<tusb_desc_device_t const *>(tud_descriptor_device_cb());
  tusb_desc_configuration_t const *config =
      reinterpret_cast<tusb_desc_configuration_t const *>(
          tud_descriptor_configuration_cb(0));

#if HOOK_RESULT < 0
  TestEvent const expected[] = {EVENT_DEFAULT_CDC_BEGIN,
                                EVENT_DEFAULT_CDC_DESCRIPTOR,
                                EVENT_HARDWARE_INIT};
  assert(config->bNumInterfaces == 2);
  assert(device->bDeviceClass == TUSB_CLASS_MISC);
  assert(initialized_rhport == 7);
#elif HOOK_RESULT
  TestEvent const expected[] = {EVENT_CUSTOM_HOOK, EVENT_CUSTOM_DESCRIPTOR,
                                EVENT_HARDWARE_INIT};
  assert(config->bNumInterfaces == 1);
  assert(device->bDeviceClass == 0);
  assert(initialized_rhport == 7);
#else
  TestEvent const expected[] = {EVENT_CUSTOM_HOOK};
  assert(config->bNumInterfaces == 0);
  assert(device->bDeviceClass == 0);
  assert(initialized_rhport == 0xff);
#endif

  assert(test_event_count == sizeof(expected) / sizeof(expected[0]));
  for (uint8_t i = 0; i < test_event_count; ++i) {
    assert(test_events[i] == expected[i]);
  }
}

