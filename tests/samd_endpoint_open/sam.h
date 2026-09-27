#pragma once
#include <stdint.h>
#include <functional>
struct Value {
  uint32_t value = 0;
  std::function<void(uint32_t)> written;
  operator uint32_t() const { return value; }
  Value &operator=(uint32_t v) { value = v; if (written) written(v); return *this; }
  Value &operator|=(uint32_t v) { return *this = value | v; }
  Value &operator&=(uint32_t v) { return *this = value & v; }
};
struct Bits {
  Value BYTE_COUNT;
  Value CQOS;
  Value DQOS;
  Value ENABLE;
  Value EPTYPE0;
  Value EPTYPE1;
  Value FNUM;
  Value MULTI_PACKET_SIZE;
  Value RXSTP;
  Value SIZE;
  Value SOF;
  Value SWRST;
  Value TRANSN;
  Value TRANSP;
  Value TRCPT0;
  Value TRCPT1;
  Value TRIM;
  Value UPRSM;
};
struct Register { Value reg; Bits bit; };
struct UsbDeviceDescBank { Register ADDR, PCKSIZE; };
struct UsbDeviceEndpoint { Register EPCFG, EPSTATUSCLR, EPSTATUSSET, EPINTENSET, EPINTFLAG; };
struct DeviceRegisters {
  Register CTRLA, SYNCBUSY, PADCAL, QOSCTRL, DESCADD, CTRLB, INTFLAG, INTENSET, INTENCLR, DADD, EPINTSMRY, FNUM;
  UsbDeviceEndpoint DeviceEndpoint[8];
};
struct UsbRegisters { DeviceRegisters DEVICE; };
extern UsbRegisters usb_registers;
static uint32_t mock_fuses;
#define USB (&usb_registers)
inline void NVIC_EnableIRQ(int) {}
inline void NVIC_DisableIRQ(int) {}
enum { USB_IRQn, USB_0_IRQn, USB_1_IRQn, USB_2_IRQn, USB_3_IRQn, USB_OTHER_IRQn, USB_SOF_HSOF_IRQn, USB_TRCPT0_IRQn, USB_TRCPT1_IRQn };
#define USB_CTRLA_ENABLE 0
#define USB_CTRLA_MODE_DEVICE 0
#define USB_CTRLA_RUNSTDBY 0
#define USB_DEVICE_CTRLB_DETACH 0
#define USB_DEVICE_CTRLB_SPDCONF_FS 0
#define USB_DEVICE_DADD_ADDEN 0
#define USB_DEVICE_DADD_DADD(x) (x)
#define USB_DEVICE_EPCFG_EPTYPE0(x) (x)
#define USB_DEVICE_EPCFG_EPTYPE1(x) (x)
#define USB_DEVICE_EPINTENSET_RXSTP 0
#define USB_DEVICE_EPINTENSET_TRCPT0 0
#define USB_DEVICE_EPINTENSET_TRCPT1 0
#define USB_DEVICE_EPINTFLAG_RXSTP 0
#define USB_DEVICE_EPINTFLAG_TRCPT0 0
#define USB_DEVICE_EPINTFLAG_TRCPT1 0
#define USB_DEVICE_EPINTFLAG_TRFAIL0 0
#define USB_DEVICE_EPINTFLAG_TRFAIL1 0
#define USB_DEVICE_EPSTATUSCLR_BK0RDY 64
#define USB_DEVICE_EPSTATUSCLR_BK1RDY 128
#define USB_DEVICE_EPSTATUSCLR_DTGLIN 2
#define USB_DEVICE_EPSTATUSCLR_DTGLOUT 1
#define USB_DEVICE_EPSTATUSCLR_STALLRQ0 16
#define USB_DEVICE_EPSTATUSCLR_STALLRQ1 32
#define USB_DEVICE_EPSTATUSSET_BK0RDY 64
#define USB_DEVICE_EPSTATUSSET_BK1RDY 128
#define USB_DEVICE_EPSTATUSSET_STALLRQ0 0
#define USB_DEVICE_EPSTATUSSET_STALLRQ1 0
#define USB_DEVICE_INTENCLR_SUSPEND 0
#define USB_DEVICE_INTENSET_EORST 0
#define USB_DEVICE_INTENSET_SOF 0
#define USB_DEVICE_INTENSET_SUSPEND 0
#define USB_DEVICE_INTFLAG_EORST 0
#define USB_DEVICE_INTFLAG_SOF 0
#define USB_DEVICE_INTFLAG_SUSPEND 0
#define USB_DEVICE_INTFLAG_WAKEUP 0
#define USB_EPT_NUM 8
#define USB_FUSES_TRANSN_ADDR (&mock_fuses)
#define USB_FUSES_TRANSP_ADDR (&mock_fuses)
#define USB_FUSES_TRIM_ADDR (&mock_fuses)
#define USB_FUSES_TRANSP_Msk 0
#define USB_FUSES_TRANSP_Pos 0
#define USB_FUSES_TRANSN_Msk 0
#define USB_FUSES_TRANSN_Pos 0
#define USB_FUSES_TRIM_Msk 0
#define USB_FUSES_TRIM_Pos 0
