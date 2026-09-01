# Architecture overview

The product has four independently testable boundaries:

1. `packages/protocol` owns the JSON schema, deterministic binary image, CRC,
   and USB frame contracts shared by TypeScript and C++.
2. `firmware/src/core` is hardware-independent C++20. It recognizes gestures,
   executes ordered actions, filters expression, and atomically selects flash
   images without dynamic allocation in the live path.
3. `firmware/src/hal`, `firmware/src/usb`, and `firmware/src/display` adapt Pico
   GPIO/UART/ADC/flash, TinyUSB, and the write-only ST7796S SPI panel. The
   `PicoUsbPort` keeps CDC responses bounded and non-blocking, while the
   `ProtocolDispatcher` routes every framed editor command to the flash-backed
   `ControllerProtocolService`.
4. `firmware/src/app/pedal_runtime.cpp` is the live-loop coordinator. It loads
   the selected bank, feeds switches and expression samples into the core,
   routes TRS/USB MIDI and relay commands, applies navigation, and submits
   dirty-only `LiveView` updates to the display.
5. `editor` is a static Chromium application. Its reducer keeps drafts local;
   `DeviceSession` sends framed whole-image synchronization over WebSerial.

The carrier hardware is deliberately a separate measured layer. Relay contact
isolation, power source handover, and display-module voltage are not inferred
from software tests. See the approved [design specification](superpowers/specs/2026-09-01-midi-controller-pedal-design.md)
for fixed pin/timing/safety decisions.
