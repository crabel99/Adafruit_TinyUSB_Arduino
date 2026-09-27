# SAMD endpoint reopen ordering

Run `python3 tests/samd_endpoint_open/run.py` with Clang available as `c++` or through `CXX`.

The test compiles the production `dcd_samd.c` into a C++ register model. It starts each nonzero endpoint direction disabled, then calls the production transfer function with an old buffer. This represents CPU submission finishing after a physical USB reset while interrupt delivery is masked. The production open function must hold that bank before it enables the direction.

Register write observers offer an immediate host token after each EPCFG or bank status write. The assertions cover both OUT and IN, the absence of old-buffer DMA during and after open, normal submission with a new buffer, and unchanged readiness when reopening an already enabled direction. Moving the hold operation below the EPTYPE write fails the test even if the final register values are correct.

The modeled hardware rules come from SAM D21/DA1 DS40001882 section 32.6.2.4 and SAM D5x/E5x DS60001507 section 38.6.2.4, plus each device's OUT/IN transaction sections. A bus reset clears nonzero EPCFG. An OUT bank with BK0RDY set cannot accept payload; an IN bank with BK1RDY clear cannot send payload.

This is an ordering test, not peripheral emulation. Other register fields and interrupts are compile stubs, initialization and IRQ handlers are not executed, and no buffer is dereferenced. The compiler's Microsoft extension permits the production 32-bit address cast on a 64-bit test host. SAMD21 and SAME54 firmware builds validate the real headers and C compilation. Physical USB reset/reconnect acceptance remains separate.
