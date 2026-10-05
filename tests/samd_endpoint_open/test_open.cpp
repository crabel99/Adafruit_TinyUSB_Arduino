#include <cstdio>
#include <cstdlib>
#include <sam.h>
#include "../../src/portable/microchip/samd/dcd_samd.c"
UsbRegisters usb_registers;
static uint32_t status;
static unsigned staleDma;
static uint32_t retiredAddress;
static void token(unsigned dir) {
    auto &ep = USB->DEVICE.DeviceEndpoint[1];
    const unsigned type = dir ? ep.EPCFG.bit.EPTYPE1 : ep.EPCFG.bit.EPTYPE0;
    const bool ready = dir ? (status & 128) : !(status & 64);
    if (type && ready && sram_registers[1][dir].ADDR.reg == retiredAddress) ++staleDma;
}
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main() {
    auto &ep = USB->DEVICE.DeviceEndpoint[1];
    ep.EPCFG.bit.EPTYPE0.written = [](uint32_t) { token(0); };
    ep.EPCFG.bit.EPTYPE1.written = [](uint32_t) { token(1); };
    ep.EPSTATUSCLR.reg.written = [](uint32_t value) { status &= ~value; token(0); token(1); };
    ep.EPSTATUSSET.reg.written = [](uint32_t value) { status |= value; token(0); token(1); };
    for (unsigned dir = 0; dir < 2; ++dir) {
        ep.EPCFG.bit.EPTYPE0 = 0;
        ep.EPCFG.bit.EPTYPE1 = 0;
        status = 64;
        retiredAddress = 0x12340000;
        auto *retired = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(retiredAddress));
        const uint8_t address = static_cast<uint8_t>(1 | (dir << 7));
        // Physical EORST disables EPTYPE while masked CPU code finishes arming the old bank.
        require(dcd_edpt_xfer(0, address, retired, 64, false), "post-reset submission accepted");
        staleDma = 0;
        tusb_desc_endpoint_t descriptor = {address, {2}, 64};
        require(dcd_edpt_open(0, &descriptor), "fresh endpoint open");
        require(staleDma == 0, dir ? "IN old bank became live during open" : "OUT old bank became live during open");
        token(dir);
        require(staleDma == 0, "old bank became live after fresh mount");
        require(dir ? !(status & 128) : (status & 64), "fresh bank remains held until submission");
        auto *fresh = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(0x56780000));
        require(dcd_edpt_xfer(0, address, fresh, 32, false), "fresh transfer accepted");
        require(dir ? (status & 128) : !(status & 64), "fresh transfer ready");
        require(sram_registers[1][dir].ADDR.reg == 0x56780000, "fresh transfer owns new buffer");
        const uint32_t before = status;
        require(dcd_edpt_open(0, &descriptor), "existing enabled-direction reopen remains supported");
        require((status & 192) == (before & 192), "enabled-direction bank ownership unchanged");
    }
    std::puts("PASS: production SAMD DCD holds stale OUT/IN banks before enabling; fresh transfer and enabled reopen preserved");
}
