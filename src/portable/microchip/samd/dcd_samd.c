/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#if CFG_TUD_ENABLED && TU_CHECK_MCU(OPT_MCU_SAMD11, OPT_MCU_SAMD21, OPT_MCU_SAML2X, OPT_MCU_SAMD51, OPT_MCU_SAME5X)

#include "sam.h"
#include "device/dcd.h"

/*------------------------------------------------------------------*/
/* MACRO TYPEDEF CONSTANT ENUM
 *------------------------------------------------------------------*/
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
static TU_ATTR_ALIGNED(4) usb_device_desc_bank_registers_t sram_registers[8][2];
#else
static TU_ATTR_ALIGNED(4) UsbDeviceDescBank sram_registers[8][2];
#endif

// Setup packet is only 8 bytes in length. However under certain scenario,
// USB DMA controller may decide to overwrite/overflow the buffer  with
// 2 extra bytes of CRC. From datasheet's "Management of SETUP Transactions" section
//    If the number of received data bytes is the maximum data payload specified by
//    PCKSIZE.SIZE minus one, only the first CRC data is written to the data buffer.
//    If the number of received data is equal or less than the data payload specified
//    by PCKSIZE.SIZE minus two, both CRC data bytes are written to the data buffer.
// Therefore we will need to increase it to 10 bytes here.
static TU_ATTR_ALIGNED(4) uint8_t _setup_packet[8+2];

// ready for receiving SETUP packet
static inline void prepare_setup(void)
{
  // Only make sure the EP0 OUT buffer is ready
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  sram_registers[0][0].USB_ADDR = (uint32_t) _setup_packet;
  sram_registers[0][0].USB_PCKSIZE =
      (sram_registers[0][0].USB_PCKSIZE & ~USB_DEVICE_PCKSIZE_MULTI_PACKET_SIZE_Msk) |
      USB_DEVICE_PCKSIZE_MULTI_PACKET_SIZE(sizeof(tusb_control_request_t));
  sram_registers[0][0].USB_PCKSIZE &= ~USB_DEVICE_PCKSIZE_BYTE_COUNT_Msk;
#else
  sram_registers[0][0].ADDR.reg = (uint32_t) _setup_packet;
  sram_registers[0][0].PCKSIZE.bit.MULTI_PACKET_SIZE = sizeof(tusb_control_request_t);
  sram_registers[0][0].PCKSIZE.bit.BYTE_COUNT = 0;
#endif
}

// Setup the control endpoint 0.
static void bus_reset(void)
{
  // Max size of packets is 64 bytes.
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  usb_device_desc_bank_registers_t* bank_out = &sram_registers[0][TUSB_DIR_OUT];
  bank_out->USB_PCKSIZE = (bank_out->USB_PCKSIZE & ~USB_DEVICE_PCKSIZE_SIZE_Msk) |
                          USB_DEVICE_PCKSIZE_SIZE(0x3);
  usb_device_desc_bank_registers_t* bank_in = &sram_registers[0][TUSB_DIR_IN];
  bank_in->USB_PCKSIZE = (bank_in->USB_PCKSIZE & ~USB_DEVICE_PCKSIZE_SIZE_Msk) |
                         USB_DEVICE_PCKSIZE_SIZE(0x3);

  usb_device_endpoint_registers_t* ep = &USB_REGS->DEVICE.DEVICE_ENDPOINT[0];
  ep->USB_EPCFG = USB_DEVICE_EPCFG_EPTYPE0(0x1) | USB_DEVICE_EPCFG_EPTYPE1(0x1);
  ep->USB_EPINTENSET = USB_DEVICE_EPINTENSET_TRCPT0_Msk |
                       USB_DEVICE_EPINTENSET_TRCPT1_Msk |
                       USB_DEVICE_EPINTENSET_RXSTP_Msk;
#else
  UsbDeviceDescBank* bank_out = &sram_registers[0][TUSB_DIR_OUT];
  bank_out->PCKSIZE.bit.SIZE = 0x3;
  UsbDeviceDescBank* bank_in = &sram_registers[0][TUSB_DIR_IN];
  bank_in->PCKSIZE.bit.SIZE = 0x3;

  UsbDeviceEndpoint* ep = &USB->DEVICE.DeviceEndpoint[0];
  ep->EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE0(0x1) | USB_DEVICE_EPCFG_EPTYPE1(0x1);
  ep->EPINTENSET.reg = USB_DEVICE_EPINTENSET_TRCPT0 | USB_DEVICE_EPINTENSET_TRCPT1 | USB_DEVICE_EPINTENSET_RXSTP;
#endif

  // Prepare for setup packet
  prepare_setup();
}

/*------------------------------------------------------------------*/
/* Controller API
 *------------------------------------------------------------------*/
bool dcd_init(uint8_t rhport, const tusb_rhport_init_t* rh_init) {
  (void) rhport;
  (void) rh_init;

#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  USB_REGS->DEVICE.USB_CTRLA = USB_CTRLA_SWRST_Msk;
  while ((USB_REGS->DEVICE.USB_SYNCBUSY & USB_SYNCBUSY_SWRST_Msk) == 0) {}
  while ((USB_REGS->DEVICE.USB_SYNCBUSY & USB_SYNCBUSY_SWRST_Msk) != 0) {}

  uint32_t const usb_fuses = *((uint32_t*) (SW0_ADDR + 4u));
  USB_REGS->DEVICE.USB_PADCAL =
      USB_PADCAL_TRANSP((usb_fuses & FUSES_SW0_WORD_1_USB_TRANSP_Msk) >>
                        FUSES_SW0_WORD_1_USB_TRANSP_Pos) |
      USB_PADCAL_TRANSN((usb_fuses & FUSES_SW0_WORD_1_USB_TRANSN_Msk) >>
                        FUSES_SW0_WORD_1_USB_TRANSN_Pos) |
      USB_PADCAL_TRIM((usb_fuses & FUSES_SW0_WORD_1_USB_TRIM_Msk) >>
                      FUSES_SW0_WORD_1_USB_TRIM_Pos);

  USB_REGS->DEVICE.USB_QOSCTRL = USB_QOSCTRL_CQOS(3) | USB_QOSCTRL_DQOS(3);
  USB_REGS->DEVICE.USB_DESCADD = (uint32_t) &sram_registers;
  USB_REGS->DEVICE.USB_CTRLB = USB_DEVICE_CTRLB_SPDCONF_FS;
  USB_REGS->DEVICE.USB_CTRLA = USB_CTRLA_MODE_DEVICE |
                                USB_CTRLA_ENABLE_Msk |
                                USB_CTRLA_RUNSTDBY_Msk;
  while ((USB_REGS->DEVICE.USB_SYNCBUSY & USB_SYNCBUSY_ENABLE_Msk) != 0) {}

  USB_REGS->DEVICE.USB_INTFLAG = USB_REGS->DEVICE.USB_INTFLAG;
  USB_REGS->DEVICE.USB_INTENSET = USB_DEVICE_INTENSET_EORST_Msk;
#else
  // Reset to get in a clean state.
  USB->DEVICE.CTRLA.bit.SWRST = true;
  while (USB->DEVICE.SYNCBUSY.bit.SWRST == 0) {}
  while (USB->DEVICE.SYNCBUSY.bit.SWRST == 1) {}

  USB->DEVICE.PADCAL.bit.TRANSP = (*((uint32_t*) USB_FUSES_TRANSP_ADDR) & USB_FUSES_TRANSP_Msk) >> USB_FUSES_TRANSP_Pos;
  USB->DEVICE.PADCAL.bit.TRANSN = (*((uint32_t*) USB_FUSES_TRANSN_ADDR) & USB_FUSES_TRANSN_Msk) >> USB_FUSES_TRANSN_Pos;
  USB->DEVICE.PADCAL.bit.TRIM   = (*((uint32_t*) USB_FUSES_TRIM_ADDR) & USB_FUSES_TRIM_Msk) >> USB_FUSES_TRIM_Pos;

  USB->DEVICE.QOSCTRL.bit.CQOS = 3; // High Quality
  USB->DEVICE.QOSCTRL.bit.DQOS = 3; // High Quality

  // Configure registers
  USB->DEVICE.DESCADD.reg = (uint32_t) &sram_registers;
  USB->DEVICE.CTRLB.reg = USB_DEVICE_CTRLB_SPDCONF_FS;
  USB->DEVICE.CTRLA.reg = USB_CTRLA_MODE_DEVICE | USB_CTRLA_ENABLE | USB_CTRLA_RUNSTDBY;
  while (USB->DEVICE.SYNCBUSY.bit.ENABLE == 1) {}

  USB->DEVICE.INTFLAG.reg |= USB->DEVICE.INTFLAG.reg; // clear pending
  USB->DEVICE.INTENSET.reg = /* USB_DEVICE_INTENSET_SOF | */ USB_DEVICE_INTENSET_EORST;
#endif

  return true;
}

#if TU_CHECK_MCU(OPT_MCU_SAMD51)
void dcd_int_enable(uint8_t rhport) {
  (void) rhport;
  NVIC_EnableIRQ(USB_0_IRQn);
  NVIC_EnableIRQ(USB_1_IRQn);
  NVIC_EnableIRQ(USB_2_IRQn);
  NVIC_EnableIRQ(USB_3_IRQn);
}

void dcd_int_disable(uint8_t rhport) {
  (void) rhport;
  NVIC_DisableIRQ(USB_3_IRQn);
  NVIC_DisableIRQ(USB_2_IRQn);
  NVIC_DisableIRQ(USB_1_IRQn);
  NVIC_DisableIRQ(USB_0_IRQn);
}

#elif TU_CHECK_MCU(OPT_MCU_SAME5X)
void dcd_int_enable(uint8_t rhport) {
  (void) rhport;
  NVIC_EnableIRQ(USB_OTHER_IRQn);
  NVIC_EnableIRQ(USB_SOF_HSOF_IRQn);
  NVIC_EnableIRQ(USB_TRCPT0_IRQn);
  NVIC_EnableIRQ(USB_TRCPT1_IRQn);
}

void dcd_int_disable(uint8_t rhport) {
  (void) rhport;
  NVIC_DisableIRQ(USB_TRCPT1_IRQn);
  NVIC_DisableIRQ(USB_TRCPT0_IRQn);
  NVIC_DisableIRQ(USB_SOF_HSOF_IRQn);
  NVIC_DisableIRQ(USB_OTHER_IRQn);
}

#elif TU_CHECK_MCU(OPT_MCU_SAMD11, OPT_MCU_SAMD21, OPT_MCU_SAML2X)
void dcd_int_enable(uint8_t rhport) {
  (void) rhport;
  NVIC_EnableIRQ(USB_IRQn);
}

void dcd_int_disable(uint8_t rhport) {
  (void) rhport;
  NVIC_DisableIRQ(USB_IRQn);
}

#else

#error "No implementation available for dcd_int_enable / dcd_int_disable"

#endif

void dcd_set_address (uint8_t rhport, uint8_t dev_addr)
{
  (void) dev_addr;

  // Response with zlp status
  dcd_edpt_xfer(rhport, 0x80, NULL, 0, false);

  // DCD can only set address after status for this request is complete
  // do it at dcd_edpt0_status_complete()

  // Enable SUSPEND interrupt since the bus signal D+/D- are stable now.
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  USB_REGS->DEVICE.USB_INTFLAG = USB_DEVICE_INTENCLR_SUSPEND_Msk;
  USB_REGS->DEVICE.USB_INTENSET = USB_DEVICE_INTENSET_SUSPEND_Msk;
#else
  USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTENCLR_SUSPEND; // clear pending
  USB->DEVICE.INTENSET.reg = USB_DEVICE_INTENSET_SUSPEND;
#endif
}

void dcd_remote_wakeup(uint8_t rhport)
{
  (void) rhport;
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  USB_REGS->DEVICE.USB_CTRLB |= USB_DEVICE_CTRLB_UPRSM_Msk;
#else
  USB->DEVICE.CTRLB.bit.UPRSM = 1;
#endif
}

// disconnect by disabling internal pull-up resistor on D+/D-
void dcd_disconnect(uint8_t rhport)
{
  (void) rhport;
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  USB_REGS->DEVICE.USB_CTRLB |= USB_DEVICE_CTRLB_DETACH_Msk;
#else
  USB->DEVICE.CTRLB.reg |= USB_DEVICE_CTRLB_DETACH;
#endif
}

// connect by enabling internal pull-up resistor on D+/D-
void dcd_connect(uint8_t rhport)
{
  (void) rhport;
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  USB_REGS->DEVICE.USB_CTRLB &= ~USB_DEVICE_CTRLB_DETACH_Msk;
#else
  USB->DEVICE.CTRLB.reg &= ~USB_DEVICE_CTRLB_DETACH;
#endif
}

void dcd_sof_enable(uint8_t rhport, bool en)
{
  (void) rhport;

  if (en) {
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
    USB_REGS->DEVICE.USB_INTENSET = USB_DEVICE_INTENSET_SOF_Msk;
#else
    USB->DEVICE.INTENSET.bit.SOF = 1;
#endif
  } else {
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
    USB_REGS->DEVICE.USB_INTENCLR = USB_DEVICE_INTENCLR_SOF_Msk;
#else
    USB->DEVICE.INTENCLR.bit.SOF = 1;
#endif
  }
}

/*------------------------------------------------------------------*/
/* DCD Endpoint port
 *------------------------------------------------------------------*/

// Invoked when a control transfer's status stage is complete.
// May help DCD to prepare for next control transfer, this API is optional.
void dcd_edpt0_status_complete(uint8_t rhport, tusb_control_request_t const * request)
{
  (void) rhport;

  if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
      request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD &&
      request->bRequest == TUSB_REQ_SET_ADDRESS )
  {
    uint8_t const dev_addr = (uint8_t) request->wValue;
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
    USB_REGS->DEVICE.USB_DADD = USB_DEVICE_DADD_DADD(dev_addr) |
                                USB_DEVICE_DADD_ADDEN_Msk;
#else
    USB->DEVICE.DADD.reg = USB_DEVICE_DADD_DADD(dev_addr) | USB_DEVICE_DADD_ADDEN;
#endif
  }

  // Just finished status stage, prepare for next setup packet
  // Note: we may already prepare setup when queueing the control status.
  // but it has no harm to do it again here
  prepare_setup();
}

bool dcd_edpt_open (uint8_t rhport, tusb_desc_endpoint_t const * desc_edpt)
{
  (void) rhport;

  uint8_t const epnum = tu_edpt_number(desc_edpt->bEndpointAddress);
  uint8_t const dir   = tu_edpt_dir(desc_edpt->bEndpointAddress);

#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  usb_device_desc_bank_registers_t* bank = &sram_registers[epnum][dir];
#else
  UsbDeviceDescBank* bank = &sram_registers[epnum][dir];
#endif
  uint32_t size_value = 0;
  while (size_value < 7) {
    if (1 << (size_value + 3) >= tu_edpt_packet_size(desc_edpt)) {
      break;
    }
    size_value++;
  }

  // unsupported endpoint size
  if ( size_value == 7 && tu_edpt_packet_size(desc_edpt) > 1023 ) return false;

#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  bank->USB_PCKSIZE = (bank->USB_PCKSIZE & ~USB_DEVICE_PCKSIZE_SIZE_Msk) |
                      USB_DEVICE_PCKSIZE_SIZE(size_value);
  usb_device_endpoint_registers_t* ep = &USB_REGS->DEVICE.DEVICE_ENDPOINT[epnum];

  if ( dir == TUSB_DIR_OUT )
  {
    ep->USB_EPCFG = (ep->USB_EPCFG & ~USB_DEVICE_EPCFG_EPTYPE0_Msk) |
                    USB_DEVICE_EPCFG_EPTYPE0(desc_edpt->bmAttributes.xfer + 1);
    ep->USB_EPSTATUSCLR = USB_DEVICE_EPSTATUSCLR_STALLRQ0_Msk |
                          USB_DEVICE_EPSTATUSCLR_DTGLOUT_Msk;
    ep->USB_EPINTENSET = USB_DEVICE_EPINTENSET_TRCPT0_Msk;
  }else
  {
    ep->USB_EPCFG = (ep->USB_EPCFG & ~USB_DEVICE_EPCFG_EPTYPE1_Msk) |
                    USB_DEVICE_EPCFG_EPTYPE1(desc_edpt->bmAttributes.xfer + 1);
    ep->USB_EPSTATUSCLR = USB_DEVICE_EPSTATUSCLR_STALLRQ1_Msk |
                          USB_DEVICE_EPSTATUSCLR_DTGLIN_Msk;
    ep->USB_EPINTENSET = USB_DEVICE_EPINTENSET_TRCPT1_Msk;
  }
#else
  bank->PCKSIZE.bit.SIZE = size_value;

  UsbDeviceEndpoint* ep = &USB->DEVICE.DeviceEndpoint[epnum];

  if ( dir == TUSB_DIR_OUT )
  {
    ep->EPCFG.bit.EPTYPE0 = desc_edpt->bmAttributes.xfer + 1;
    ep->EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_STALLRQ0 | USB_DEVICE_EPSTATUSCLR_DTGLOUT; // clear stall & dtoggle
    ep->EPINTENSET.bit.TRCPT0 = true;
  }else
  {
    ep->EPCFG.bit.EPTYPE1 = desc_edpt->bmAttributes.xfer + 1;
    ep->EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_STALLRQ1 | USB_DEVICE_EPSTATUSCLR_DTGLIN; // clear stall & dtoggle
    ep->EPINTENSET.bit.TRCPT1 = true;
  }
#endif

  return true;
}

bool dcd_edpt_iso_alloc(uint8_t rhport, uint8_t ep_addr, uint16_t largest_packet_size) {
  (void) rhport;
  (void) ep_addr;
  (void)largest_packet_size;
  return false;
}

bool dcd_edpt_iso_activate(uint8_t rhport, const tusb_desc_endpoint_t *desc_ep) {
  (void)rhport;
  (void)desc_ep;
  return false;
}

void dcd_edpt_close_all (uint8_t rhport)
{
  (void) rhport;
  // TODO implement dcd_edpt_close_all()
}

bool dcd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t * buffer, uint16_t total_bytes, bool is_isr)
{
  (void) is_isr;
  (void) rhport;

  uint8_t const epnum = tu_edpt_number(ep_addr);
  uint8_t const dir   = tu_edpt_dir(ep_addr);

#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  usb_device_desc_bank_registers_t* bank = &sram_registers[epnum][dir];
  usb_device_endpoint_registers_t* ep = &USB_REGS->DEVICE.DEVICE_ENDPOINT[epnum];
  bank->USB_ADDR = (uint32_t) buffer;
#else
  UsbDeviceDescBank* bank = &sram_registers[epnum][dir];
  UsbDeviceEndpoint* ep = &USB->DEVICE.DeviceEndpoint[epnum];
  bank->ADDR.reg = (uint32_t) buffer;
#endif

  // A SETUP token can occur immediately after an ZLP Status.
  // So make sure we have a valid buffer for setup packet.
  //   Status = ZLP EP0 with direction opposite to one in the dir bit of current setup
  if ( (epnum == 0) && (buffer == NULL) && (total_bytes == 0) && (dir != tu_edpt_dir(_setup_packet[0])) ) {
    prepare_setup();
  }

  if ( dir == TUSB_DIR_OUT )
  {
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
    bank->USB_PCKSIZE = (bank->USB_PCKSIZE &
                         ~(USB_DEVICE_PCKSIZE_MULTI_PACKET_SIZE_Msk |
                           USB_DEVICE_PCKSIZE_BYTE_COUNT_Msk)) |
                        USB_DEVICE_PCKSIZE_MULTI_PACKET_SIZE(total_bytes);
    ep->USB_EPSTATUSCLR = USB_DEVICE_EPSTATUSCLR_BK0RDY_Msk;
    ep->USB_EPINTFLAG = USB_DEVICE_EPINTFLAG_TRFAIL0_Msk;
#else
    bank->PCKSIZE.bit.MULTI_PACKET_SIZE = total_bytes;
    bank->PCKSIZE.bit.BYTE_COUNT = 0;
    ep->EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_BK0RDY;
    ep->EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRFAIL0;
#endif
  } else
  {
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
    bank->USB_PCKSIZE = (bank->USB_PCKSIZE &
                         ~(USB_DEVICE_PCKSIZE_MULTI_PACKET_SIZE_Msk |
                           USB_DEVICE_PCKSIZE_BYTE_COUNT_Msk)) |
                        USB_DEVICE_PCKSIZE_BYTE_COUNT(total_bytes);
    ep->USB_EPSTATUSSET = USB_DEVICE_EPSTATUSSET_BK1RDY_Msk;
    ep->USB_EPINTFLAG = USB_DEVICE_EPINTFLAG_TRFAIL1_Msk;
#else
    bank->PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    bank->PCKSIZE.bit.BYTE_COUNT = total_bytes;
    ep->EPSTATUSSET.reg = USB_DEVICE_EPSTATUSSET_BK1RDY;
    ep->EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRFAIL1;
#endif
  }

  return true;
}

void dcd_edpt_stall (uint8_t rhport, uint8_t ep_addr)
{
  (void) rhport;

  uint8_t const epnum = tu_edpt_number(ep_addr);
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  usb_device_endpoint_registers_t* ep = &USB_REGS->DEVICE.DEVICE_ENDPOINT[epnum];
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN) {
    ep->USB_EPSTATUSSET = USB_DEVICE_EPSTATUSSET_STALLRQ1_Msk;
  } else {
    ep->USB_EPSTATUSSET = USB_DEVICE_EPSTATUSSET_STALLRQ0_Msk;
  }
#else
  UsbDeviceEndpoint* ep = &USB->DEVICE.DeviceEndpoint[epnum];
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN) {
    ep->EPSTATUSSET.reg = USB_DEVICE_EPSTATUSSET_STALLRQ1;
  } else {
    ep->EPSTATUSSET.reg = USB_DEVICE_EPSTATUSSET_STALLRQ0;
  }
#endif
}

void dcd_edpt_clear_stall (uint8_t rhport, uint8_t ep_addr)
{
  (void) rhport;

  uint8_t const epnum = tu_edpt_number(ep_addr);
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  usb_device_endpoint_registers_t* ep = &USB_REGS->DEVICE.DEVICE_ENDPOINT[epnum];
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN) {
    ep->USB_EPSTATUSCLR = USB_DEVICE_EPSTATUSCLR_STALLRQ1_Msk |
                          USB_DEVICE_EPSTATUSCLR_DTGLIN_Msk;
  } else {
    ep->USB_EPSTATUSCLR = USB_DEVICE_EPSTATUSCLR_STALLRQ0_Msk |
                          USB_DEVICE_EPSTATUSCLR_DTGLOUT_Msk;
  }
#else
  UsbDeviceEndpoint* ep = &USB->DEVICE.DeviceEndpoint[epnum];
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN) {
    ep->EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_STALLRQ1 | USB_DEVICE_EPSTATUSCLR_DTGLIN;
  } else {
    ep->EPSTATUSCLR.reg = USB_DEVICE_EPSTATUSCLR_STALLRQ0 | USB_DEVICE_EPSTATUSCLR_DTGLOUT;
  }
#endif
}

//--------------------------------------------------------------------+
// Interrupt Handler
//--------------------------------------------------------------------+
static void maybe_transfer_complete(void) {
#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  uint32_t epints = USB_REGS->DEVICE.USB_EPINTSMRY;

  for (uint8_t epnum = 0; epnum < USB_DEVICE_ENDPOINT_NUMBER; epnum++) {
    if ((epints & (1u << epnum)) == 0) {
      continue;
    }

    usb_device_endpoint_registers_t* ep = &USB_REGS->DEVICE.DEVICE_ENDPOINT[epnum];
    uint32_t epintflag = ep->USB_EPINTFLAG;

    if ((epintflag & USB_DEVICE_EPINTFLAG_TRCPT1_Msk) != 0) {
      usb_device_desc_bank_registers_t* bank = &sram_registers[epnum][TUSB_DIR_IN];
      uint16_t const total_transfer_size =
          (uint16_t) ((bank->USB_PCKSIZE & USB_DEVICE_PCKSIZE_BYTE_COUNT_Msk) >>
                      USB_DEVICE_PCKSIZE_BYTE_COUNT_Pos);

      ep->USB_EPINTFLAG = USB_DEVICE_EPINTFLAG_TRCPT1_Msk;
      dcd_event_xfer_complete(0, epnum | TUSB_DIR_IN_MASK, total_transfer_size,
                              XFER_RESULT_SUCCESS, true);
    }

    if ((epintflag & USB_DEVICE_EPINTFLAG_TRCPT0_Msk) != 0) {
      usb_device_desc_bank_registers_t* bank = &sram_registers[epnum][TUSB_DIR_OUT];
      uint16_t const total_transfer_size =
          (uint16_t) ((bank->USB_PCKSIZE & USB_DEVICE_PCKSIZE_BYTE_COUNT_Msk) >>
                      USB_DEVICE_PCKSIZE_BYTE_COUNT_Pos);

      ep->USB_EPINTFLAG = USB_DEVICE_EPINTFLAG_TRCPT0_Msk;
      dcd_event_xfer_complete(0, epnum, total_transfer_size, XFER_RESULT_SUCCESS, true);
    }
  }
#else
  uint32_t epints = USB->DEVICE.EPINTSMRY.reg;

  for (uint8_t epnum = 0; epnum < USB_EPT_NUM; epnum++) {
    if ((epints & (1 << epnum)) == 0) {
      continue;
    }

    UsbDeviceEndpoint* ep = &USB->DEVICE.DeviceEndpoint[epnum];
    uint32_t epintflag = ep->EPINTFLAG.reg;

    // Handle IN completions
    if ((epintflag & USB_DEVICE_EPINTFLAG_TRCPT1) != 0) {
      UsbDeviceDescBank* bank = &sram_registers[epnum][TUSB_DIR_IN];
      uint16_t const total_transfer_size = bank->PCKSIZE.bit.BYTE_COUNT;

      dcd_event_xfer_complete(0, epnum | TUSB_DIR_IN_MASK, total_transfer_size, XFER_RESULT_SUCCESS, true);

      ep->EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT1;
    }

    // Handle OUT completions
    if ((epintflag & USB_DEVICE_EPINTFLAG_TRCPT0) != 0) {
      UsbDeviceDescBank* bank = &sram_registers[epnum][TUSB_DIR_OUT];
      uint16_t const total_transfer_size = bank->PCKSIZE.bit.BYTE_COUNT;

      dcd_event_xfer_complete(0, epnum, total_transfer_size, XFER_RESULT_SUCCESS, true);

      ep->EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT0;
    }
  }
#endif
}


void dcd_int_handler (uint8_t rhport)
{
  (void) rhport;

#if TU_CHECK_MCU(OPT_MCU_SAME5X)
  uint32_t int_status = USB_REGS->DEVICE.USB_INTFLAG &
                        USB_REGS->DEVICE.USB_INTENSET;

  if (int_status & USB_DEVICE_INTFLAG_SOF_Msk) {
    USB_REGS->DEVICE.USB_INTFLAG = USB_DEVICE_INTFLAG_SOF_Msk;
    const uint32_t frame =
        (USB_REGS->DEVICE.USB_FNUM & USB_DEVICE_FNUM_FNUM_Msk) >>
        USB_DEVICE_FNUM_FNUM_Pos;
    dcd_event_sof(0, frame, true);
  }

  if (int_status & USB_DEVICE_INTFLAG_SUSPEND_Msk) {
    USB_REGS->DEVICE.USB_INTFLAG = USB_DEVICE_INTFLAG_SUSPEND_Msk;
    USB_REGS->DEVICE.USB_INTFLAG = USB_DEVICE_INTFLAG_WAKEUP_Msk;
    USB_REGS->DEVICE.USB_INTENSET = USB_DEVICE_INTENSET_WAKEUP_Msk;
    dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
  }

  if (int_status & USB_DEVICE_INTFLAG_WAKEUP_Msk) {
    USB_REGS->DEVICE.USB_INTFLAG = USB_DEVICE_INTFLAG_WAKEUP_Msk;
    USB_REGS->DEVICE.USB_INTENCLR = USB_DEVICE_INTENCLR_WAKEUP_Msk;
    dcd_event_bus_signal(0, DCD_EVENT_RESUME, true);
  }

  if (int_status & USB_DEVICE_INTFLAG_EORST_Msk) {
    USB_REGS->DEVICE.USB_INTFLAG = USB_DEVICE_INTFLAG_EORST_Msk;
    USB_REGS->DEVICE.USB_INTENCLR = USB_DEVICE_INTENCLR_WAKEUP_Msk |
                                    USB_DEVICE_INTENCLR_SUSPEND_Msk;
    bus_reset();
    dcd_event_bus_reset(0, TUSB_SPEED_FULL, true);
  }

  usb_device_endpoint_registers_t* ep0 = &USB_REGS->DEVICE.DEVICE_ENDPOINT[0];
  if ((ep0->USB_EPINTFLAG & USB_DEVICE_EPINTFLAG_RXSTP_Msk) != 0) {
    dcd_event_setup_received(0, _setup_packet, true);
    ep0->USB_EPINTFLAG = USB_DEVICE_EPINTFLAG_RXSTP_Msk |
                         USB_DEVICE_EPINTFLAG_TRCPT0_Msk;
  }

  maybe_transfer_complete();
#else
  uint32_t int_status = USB->DEVICE.INTFLAG.reg & USB->DEVICE.INTENSET.reg;

  // Start of Frame
  if ( int_status & USB_DEVICE_INTFLAG_SOF )
  {
    USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_SOF;
    const uint32_t frame = USB->DEVICE.FNUM.bit.FNUM;
    dcd_event_sof(0, frame, true);
    //dcd_event_bus_signal(0, DCD_EVENT_SOF, true);
  }

  // SAMD doesn't distinguish between Suspend and Disconnect state.
  // Both condition will cause SUSPEND interrupt triggered.
  // To prevent being triggered when D+/D- are not stable, SUSPEND interrupt is only
  // enabled when we received SET_ADDRESS request and cleared on Bus Reset
  if ( int_status & USB_DEVICE_INTFLAG_SUSPEND )
  {
    USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_SUSPEND;

    // Enable wakeup interrupt
    USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_WAKEUP; // clear pending
    USB->DEVICE.INTENSET.reg = USB_DEVICE_INTFLAG_WAKEUP;

    dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
  }

  // Wakeup interrupt is only enabled when we got suspended.
  // Wakeup interrupt will disable itself
  if ( int_status & USB_DEVICE_INTFLAG_WAKEUP )
  {
    USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_WAKEUP;

    // disable wakeup interrupt itself
    USB->DEVICE.INTENCLR.reg = USB_DEVICE_INTFLAG_WAKEUP;
    dcd_event_bus_signal(0, DCD_EVENT_RESUME, true);
  }

  // Enable of Reset
  if ( int_status & USB_DEVICE_INTFLAG_EORST )
  {
    USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_EORST;

    // Disable both suspend and wakeup interrupt
    USB->DEVICE.INTENCLR.reg = USB_DEVICE_INTFLAG_WAKEUP | USB_DEVICE_INTFLAG_SUSPEND;

    bus_reset();
    dcd_event_bus_reset(0, TUSB_SPEED_FULL, true);
  }

  // Handle SETUP packet
  if (USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.bit.RXSTP)
  {
    // This copies the data elsewhere so we can reuse the buffer.
    dcd_event_setup_received(0, _setup_packet, true);

    // Although Setup packet only set RXSTP bit,
    // TRCPT0 bit could already be set by previous ZLP OUT Status (not handled until now).
    // Since control status complete event is optional, we can just clear TRCPT0 and skip the status event
    USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_RXSTP | USB_DEVICE_EPINTFLAG_TRCPT0;
  }

  // Handle complete transfer
  maybe_transfer_complete();
#endif
}
#endif
