# USB configuration and control completion

Run `python3 tests/device_configuration/run.py` with GNU C++ installed. Set `CXX`
if it is not named `g++` or `g++-15`.

The test compiles the real Arduino device implementation with a fake CDC
interface and hardware initializer. Three independently linked programs verify
the absent hook, successful custom configuration, and failed custom configuration.
The C test compiles the real device core and verifies actual DATA lengths, short
OUT transfers, failed DATA/ACK completion, and successful ACK completion.

The native GNU build uses `-fpermissive` for the Arduino adapter's existing
32-bit pointer cast. On macOS, the absent-hook case also permits an unresolved
weak symbol. These native checks do not replace MCU builds. Both Metro M0 and
Metro M4 TinyUSB reference sketches must compile with the target toolchains.
