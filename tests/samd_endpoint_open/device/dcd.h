#pragma once
#include <stdint.h>
#include <stddef.h>
enum { TUSB_DIR_OUT, TUSB_DIR_IN, TUSB_DIR_IN_MASK=128, TUSB_REQ_RCPT_DEVICE=0, TUSB_REQ_TYPE_STANDARD=0, TUSB_REQ_SET_ADDRESS=5, TUSB_SPEED_FULL=0, XFER_RESULT_SUCCESS=0, DCD_EVENT_SUSPEND=0, DCD_EVENT_RESUME=1 };
struct tusb_rhport_init_t {};
struct tusb_control_request_t { struct { uint8_t recipient, type; } bmRequestType_bit; uint8_t bRequest; uint16_t wValue; };
struct tusb_desc_endpoint_t { uint8_t bEndpointAddress; struct { uint8_t xfer; } bmAttributes; uint16_t wMaxPacketSize; };
inline uint8_t tu_edpt_number(uint8_t ep) { return ep & 15; }
inline uint8_t tu_edpt_dir(uint8_t ep) { return ep >> 7; }
inline uint16_t tu_edpt_packet_size(const tusb_desc_endpoint_t *ep) { return ep->wMaxPacketSize; }
bool dcd_edpt_xfer(uint8_t, uint8_t, uint8_t*, uint16_t, bool);
inline void dcd_event_xfer_complete(uint8_t, uint8_t, uint16_t, int, bool) {}
inline void dcd_event_sof(uint8_t, uint32_t, bool) {}
inline void dcd_event_bus_signal(uint8_t, int, bool) {}
inline void dcd_event_bus_reset(uint8_t, int, bool) {}
inline void dcd_event_setup_received(uint8_t, uint8_t*, bool) {}
