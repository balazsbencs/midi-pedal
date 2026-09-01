# Architecture overview

The product has four independently testable boundaries:

1. `packages/protocol` owns the JSON schema, deterministic binary image, CRC,
   and USB frame contracts shared by TypeScript and C++.
2. `firmware/src/core` is hardware-independent C++20. It recognizes gestures,
   executes ordered actions, filters expression, and atomically selects flash
   images without dynamic allocation in the live path.
3. `firmware/src/hal`, `firmware/src/usb`, and `firmware/src/display` adapt Pico
   GPIO/UART/ADC/flash, TinyUSB, and the write-only ST7796S SPI panel.
4. `editor` is a static Chromium application. Its reducer keeps drafts local;
   `DeviceSession` sends framed whole-image synchronization over WebSerial.

The carrier hardware is deliberately a separate measured layer. Relay contact
isolation, power source handover, and display-module voltage are not inferred
from software tests. See the approved [design specification](superpowers/specs/2026-09-01-midi-controller-pedal-design.md)
for fixed pin/timing/safety decisions.

