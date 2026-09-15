#include "device/usbd.c"
#include <assert.h>

static unsigned data_calls, ack_calls, stalls, status_submissions;
static uint16_t observed_length;
static bool on_control(uint8_t rhport, uint8_t stage,
                       tusb_control_request_t const *request) {
  (void)rhport;
  (void)request;
  if (stage == CONTROL_STAGE_DATA) {
    data_calls++;
    observed_length = tud_control_xfer_bytes();
    return observed_length == 16;
  }
  if (stage == CONTROL_STAGE_ACK)
    ack_calls++;
  return true;
}
void dcd_edpt_stall(uint8_t rhport, uint8_t ep) {
  (void)rhport;
  (void)ep;
  stalls++;
}
void dcd_int_disable(uint8_t rhport) { (void)rhport; }
void dcd_int_enable(uint8_t rhport) { (void)rhport; }
bool dcd_edpt_xfer(uint8_t rhport, uint8_t ep, uint8_t *buffer, uint16_t length,
                   bool is_isr) {
  (void)rhport;
  (void)ep;
  (void)buffer;
  (void)length;
  (void)is_isr;
  status_submissions++;
  return true;
}
int main(void) {
  uint8_t token[16] = {0};
  _usbd_dev.ctrl_xfer.request.bmRequestType = 0x41;
  _usbd_dev.ctrl_xfer.request.wLength = 16;
  _usbd_dev.ctrl_xfer.buffer = token;
  _usbd_dev.ctrl_xfer.complete_cb = on_control;
  assert(usbd_control_xfer_cb(0, TU_EP0_OUT, XFER_RESULT_SUCCESS, 7));
  assert(data_calls == 1 && observed_length == 7 && stalls == 2 &&
         status_submissions == 0);
  _usbd_dev.ctrl_xfer.buffer = token;
  _usbd_dev.ctrl_xfer.total_xferred = 0;
  _usbd_dev.ctrl_xfer.complete_cb = on_control;
  assert(usbd_control_xfer_cb(0, TU_EP0_OUT, XFER_RESULT_SUCCESS, 16));
  assert(data_calls == 2 && observed_length == 16 && status_submissions == 1);
  usbd_control_xfer_cb(0, TU_EP0_IN, XFER_RESULT_FAILED, 0);
  assert(ack_calls == 0);
  _usbd_dev.ctrl_xfer.complete_cb = on_control;
  _usbd_dev.ctrl_xfer.buffer = token;
  _usbd_dev.ctrl_xfer.total_xferred = 0;
  assert(!usbd_control_xfer_cb(0, TU_EP0_OUT, XFER_RESULT_FAILED, 16));
  assert(data_calls == 2 && ack_calls == 0 && tud_control_xfer_bytes() == 0);
  _usbd_dev.ctrl_xfer.complete_cb = on_control;
  assert(usbd_control_xfer_cb(0, TU_EP0_IN, XFER_RESULT_SUCCESS, 0));
  assert(ack_calls == 1);
  return 0;
}
