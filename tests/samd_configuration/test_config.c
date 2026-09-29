#include "tusb_option.h"

_Static_assert(CFG_TUSB_MCU == EXPECTED_MCU, "SAMD family selection changed");
_Static_assert(CFG_TUD_ENABLED == 1, "Device support must remain enabled");
_Static_assert(CFG_TUD_CDC == 1, "CDC support must remain enabled");
_Static_assert(CFG_TUD_VENDOR == 1, "Vendor support must remain enabled");

#ifdef DEVICE_ONLY
_Static_assert(CFG_TUH_ENABLED == 0, "Host override was ignored");
_Static_assert(CFG_TUH_MAX3421 == 0, "MAX3421 override was ignored");
_Static_assert(CFG_TUD_MSC == 0, "MSC override was ignored");
_Static_assert(CFG_TUD_HID == 0, "HID override was ignored");
_Static_assert(CFG_TUD_MIDI == 0, "MIDI override was ignored");
_Static_assert(CFG_TUD_VIDEO == 0, "Video override was ignored");
_Static_assert(CFG_TUD_VIDEO_STREAMING == 0,
               "Video streaming override was ignored");
#else
_Static_assert(CFG_TUH_ENABLED == 1, "Default host support changed");
_Static_assert(CFG_TUH_MAX3421 == 1, "Default MAX3421 support changed");
_Static_assert(CFG_TUD_MSC == 1, "Default MSC support changed");
_Static_assert(CFG_TUD_HID == 2, "Default HID support changed");
_Static_assert(CFG_TUD_MIDI == 1, "Default MIDI support changed");
_Static_assert(CFG_TUD_VIDEO == 1, "Default video support changed");
_Static_assert(CFG_TUD_VIDEO_STREAMING == 1,
               "Default video streaming support changed");
#endif

_Static_assert(CFG_TUD_ENDPOINT0_SIZE == 64, "Endpoint zero size changed");
_Static_assert(CFG_TUD_CDC_RX_BUFSIZE == 256, "CDC RX capacity changed");
_Static_assert(CFG_TUD_CDC_TX_BUFSIZE == 256, "CDC TX capacity changed");
_Static_assert(CFG_TUD_MSC_EP_BUFSIZE == 512, "MSC capacity changed");
_Static_assert(CFG_TUD_HID_EP_BUFSIZE == 64, "HID capacity changed");
_Static_assert(CFG_TUD_MIDI_RX_BUFSIZE == 128, "MIDI RX capacity changed");
_Static_assert(CFG_TUD_MIDI_TX_BUFSIZE == 128, "MIDI TX capacity changed");
_Static_assert(CFG_TUD_VENDOR_RX_BUFSIZE == 64, "Vendor RX capacity changed");
_Static_assert(CFG_TUD_VENDOR_TX_BUFSIZE == 64, "Vendor TX capacity changed");
_Static_assert(CFG_TUD_VIDEO_STREAMING_EP_BUFSIZE == 256,
               "Video capacity changed");
_Static_assert(CFG_TUH_ENUMERATION_BUFSIZE == 256,
               "Host enumeration capacity changed");
_Static_assert(CFG_TUH_CDC_RX_BUFSIZE == 64, "Host CDC RX capacity changed");
_Static_assert(CFG_TUH_CDC_TX_BUFSIZE == 64, "Host CDC TX capacity changed");
