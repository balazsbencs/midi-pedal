# MIDI Controller Pedal Design Specification

**Status:** Approved for implementation planning
**Date:** 2026-09-01
**Reference product:** Morningstar MC4 Pro interaction model

## 1. Product intent

Build an approachable open-source four-switch MIDI controller around a Raspberry Pi Pico 2 development board. The controller should deliver the core live experience of the Morningstar MC4 Pro while remaining deliberately smaller in scope and easier to understand, assemble, repair, and modify.

The product has two operating contexts:

- **On stage:** it runs without a computer, uses a standard pedalboard power supply, and prioritizes deterministic switch-to-output behavior.
- **On a desk:** one USB cable can power it, configure it from a Chromium browser, and carry class-compliant USB-MIDI.

The editor is a static browser application. It communicates directly with the pedal and never requires an account, cloud database, network connection, or server-side device access.

## 2. Goals and non-goals

### Goals

- Four programmable footswitches with an MC4-style bank/page workflow.
- Clear 480×320 live feedback that spatially mirrors the four switches.
- TRS Type-A MIDI OUT and class-compliant USB-MIDI.
- Per-message routing to TRS, USB, or both.
- One calibrated expression-pedal input.
- Two independent normally-open dry-contact relay outputs on one TRS jack.
- Safe USB and 9 V center-negative power operation.
- A documented, human-readable backup format and open device protocol.
- Atomic configuration updates with recovery from interrupted or corrupt uploads.
- Hardware and software that open-source builders can understand without specialized manufacturing equipment.

### Non-goals for v1

- MIDI input or MIDI message conversion.
- Wi-Fi, Bluetooth, or any future-facing wireless abstraction.
- On-device editing, setup menus, or touch input.
- Auxiliary switch inputs or configurable Omniports.
- USB host mode.
- MIDI Note, Clock, SysEx, MMC, keystrokes, or arbitrary delayed macros.
- More than one expression input or one relay TRS port.
- Cloud accounts, synchronization, telemetry, or hosted configuration storage.
- Safari, Firefox, or mobile-browser support.

## 3. Product architecture

The system has a strict boundary between configuration and live execution.

### Browser side

The hosted static editor:

- Reads a device descriptor and active configuration over USB CDC/WebSerial.
- Maintains edits as a local draft.
- Validates the complete draft before synchronization.
- Compiles portable JSON into the versioned device binary format.
- Uploads, verifies, and activates a complete configuration image.
- Imports and exports configuration as local JSON files.
- Exposes no backend API and transmits no configuration off the computer.

### Pedal side

The Pico 2 firmware:

- Uses C++ with the Raspberry Pi Pico SDK and TinyUSB, without an RTOS in v1.
- Runs the live engine and non-blocking services on one core; the second core remains unused unless measurement proves it necessary.
- Owns footswitch timing, chord recognition, toggle state, expression processing, output routing, and display state.
- Executes only a previously validated configuration image.
- Keeps USB configuration parsing, flash writes, and display transfers out of the time-sensitive switch-to-MIDI path.
- Uses bounded queues and non-blocking services for TRS MIDI, USB-MIDI, and display updates.

### Architectural invariant

Editing, serialization, flash writes, and display drawing must not delay or reorder live actions. A USB disconnect or editor failure must not interfere with TRS MIDI or relay behavior.

## 4. Hardware design

### Controller and carrier

- Use a non-wireless Raspberry Pi Pico 2 development board mounted on headers or sockets to a two-layer carrier PCB.
- Keep the Pico USB connector accessible through the enclosure.
- Use the Pico's native USB device controller; USB does not consume general-purpose GPIO.
- Reserve unused pins and SWD access for debugging and future board revisions, without assigning them future product features.

The estimated active GPIO budget is 13 pins: five display, four footswitch, two relay control, one expression ADC, and one UART TX.

The reference mechanical build uses a drilled aluminum stompbox enclosure with the display above the 2×2 switch field. MIDI, relay, expression, 9 V, and USB connections remain accessible without opening the enclosure. Exact enclosure and cutout dimensions are derived from measured production modules during the mechanical prototype; they do not alter the electrical interfaces in this specification.

### Display

- Controller: ST7796S, 480×320, landscape orientation.
- Interface: write-only four-wire SPI using SCL, SDA/MOSI, CS, DC, and RST.
- SDO/MISO remains unconnected.
- Backlight is permanently enabled and has no firmware brightness setting.
- Display logic is always 3.3 V.
- A documented solder selection supplies either 3.3 V or 5 V module power because breakout boards vary; the selected module voltage must be verified before assembly.
- Rendering uses dirty rectangles and small DMA buffers rather than a full-screen RGB565 framebuffer.

### Footswitches

- Four normally-open momentary footswitches named A, B, C, and D.
- Each input uses a protected GPIO with a defined pull state.
- Firmware performs debounce and all higher-level gesture recognition.
- There are no encoders, menu buttons, or setup controls.

### Expression input

- One 6.35 mm TRS jack supporting voltage-divider expression pedals only.
- Target pedal potentiometer range: 10–25 kΩ linear.
- Ring provides a protected 3.3 V reference, tip is the wiper/ADC input, and sleeve is ground.
- The ADC path includes input protection and an RC low-pass filter.
- Heel and toe ADC readings are calibrated once for the physical input and stored as controller-level settings, not repeated per bank.
- TS rheostat wiring is not supported.

### Relay output

- One 3.5 mm TRS relay jack.
- Contact 1 switches tip to sleeve; contact 2 switches ring to sleeve.
- Use two independently driven mechanical dry-contact relays.
- Both contacts are normally open and electrically isolated from controller logic.
- Reset, brownout, watchdog, and unpowered states leave both contacts open.
- Firmware supports explicit open, close, and toggle commands. Momentary behavior is composed from close-on-press and open-on-release actions.

### MIDI output

- One 3.5 mm TRS Type-A MIDI OUT.
- Drive it from a hardware UART through a standards-compliant, current-limited output stage; do not connect the jack directly to a GPIO.
- USB enumerates as a class-compliant USB-MIDI output in addition to the configuration interface.
- Each PC or CC message selects `TRS`, `USB`, or `BOTH`; the editor defaults new messages to `BOTH`.

### Power

- Accept USB 5 V or 9 V center-negative DC.
- Protect the 9 V input against reverse polarity, overcurrent, and transients.
- Convert the protected 9 V rail to the board's 5 V system rail with an efficient regulator.
- Combine USB and regulated pedal power without backfeeding either source.
- Removing either source while the other is present must not reset the controller.

## 5. Live interaction model

### Navigation

Use the MC4 Pro chord map exactly:

- A+C: bank down.
- B+D: bank up.
- A+B: page down.
- C+D: page up.
- Holding a chord repeats its navigation action.

Recognized chords suppress all constituent preset events. There is no four-switch setup chord.

### Timing constants

The following are fixed firmware constants in v1 and are not editor settings:

- Debounce stability: 20 ms.
- Chord-recognition window: 35 ms from the first physical edge; debounce occurs within this window.
- Double-tap window: 300 ms from the first recognized press.
- Long-press threshold: 500 ms from the recognized press.
- Navigation repeat delay: 600 ms.
- Navigation repeat interval: 200 ms.

A normal press fires after the chord window. A double tap also produces the ordinary press events for its two constituent taps; the double-tap event is additional. A long press also follows the initial press event. These semantics avoid delaying normal press actions for 300–500 ms.

### Toggle semantics

- Each preset has Position 1 and Position 2.
- A preset may toggle after press, long press, double tap, or not toggle.
- Messages matching the trigger execute against the current position in their configured order.
- The position transition occurs only after all matching messages have been queued.
- Toggle positions survive page and bank changes during the power session.
- All positions reset to Position 1 on boot.

## 6. Configuration model

### Capacity

- 128 banks.
- Four pages per bank.
- Four switch presets per page.
- Up to eight ordered message slots per preset.
- One expression assignment per bank.

### Bank and preset presentation

- Bank names contain at most 20 printable ASCII characters in v1.
- Each toggle position has a label of at most 12 printable ASCII characters and one RGB565 accent color.
- The device uses a fixed high-contrast dark screen theme; browser Light/Dark mode does not affect it.
- Unsupported or corrupt strings are rejected during validation rather than silently truncated.

### Message slot

Each slot contains:

- Trigger: `PRESS`, `RELEASE`, `LONG_PRESS`, or `DOUBLE_TAP`.
- Position filter: `POSITION_1`, `POSITION_2`, or `BOTH`.
- Message type and its validated parameters.

Supported v1 message types are:

- Program Change: channel 1–16, program 0–127, destination TRS/USB/both.
- Control Change: channel 1–16, controller 0–127, value 0–127, destination TRS/USB/both.
- Relay command: contact 1 or 2, operation open/close/toggle.
- Internal navigation: bank up/down/set 1–128 or page up/down/set 1–4.

All matching messages are queued in slot order without programmable delays.

### Expression assignment

Each bank defines:

- Enabled/disabled.
- Display label of at most 12 printable ASCII characters.
- MIDI channel and CC number.
- Destination TRS, USB, or both.
- Output minimum and maximum, each 0–127.
- Normal or inverted direction.

Firmware samples the ADC at 1 kHz, applies a five-sample median followed by a first-order IIR filter with coefficient 0.2, maps the result through the global calibration, and applies one MIDI-value step of hysteresis. It sends only changed mapped values and sends no faster than once every 20 ms. An absent pedal or calibration span below 10% of ADC range produces no MIDI and displays expression as unavailable.

## 7. Storage and transport

### Portable JSON

- JSON is the documented backup, interchange, and source-control format.
- It contains an explicit schema version and stable identifiers for banks, pages, presets, and messages.
- The editor preserves unknown top-level metadata when it can do so safely, but rejects unsupported behavioral fields.

### Device image

- The editor compiles JSON into a compact, documented binary image.
- The image has a magic value, format version, total length, sequence number, bank-offset index, payload, and checksum.
- Firmware reads only the current bank into working RAM; toggle state for all presets remains a compact RAM bitset.

### Atomic synchronization

- Flash reserves active and inactive configuration slots.
- The current active slot remains readable throughout upload.
- Firmware writes the inactive slot, validates its complete structure and checksum, reads it back, and only then updates the active-slot marker.
- Power loss, cable removal, malformed input, or checksum failure leaves the active configuration unchanged.
- Boot chooses the newest valid active slot, falls back to the other valid slot, then falls back to a safe empty factory configuration.

### USB protocol

- USB is a composite device with class-compliant MIDI and CDC configuration interfaces.
- Configuration traffic uses framed requests and responses with protocol version, request identifier, command, payload length, payload, and checksum.
- Commands cover capability/version query, configuration metadata, download, staged upload, verification, activation, expression calibration, and factory-empty reset.
- Every mutating command returns a structured success or error response; retrying a request identifier is idempotent.

## 8. Device display

The 480×320 screen is landscape and live-performance-only:

- Header: bank name on the left; bank number and page number on the right.
- Center: four equal quadrants laid out A top-left, B top-right, C bottom-left, and D bottom-right.
- Each quadrant shows the current-position label, switch letter, toggle position, and accent color. Text or symbols also communicate state so color is never the only indicator.
- Footer: expression assignment name/CC, value bar, and numeric value; show an unavailable marker when disconnected or invalid.

The screen displays brief non-modal indicators for USB connection, configuration activation, and recoverable errors. It has no settings or editing screens.

## 9. Browser editor experience

### Supported environment

- Static web application hosted on any ordinary HTTPS server.
- Desktop Chrome, Edge, and Brave are supported in v1.
- Direct connection uses WebSerial with an explicit browser permission prompt.

### Workspace structure

- Top bar: device connection/status, theme, unsynced-change status, Export JSON, Import JSON, and Sync to pedal.
- Left pane: searchable 128-bank list.
- Center pane: page tabs, a 2×2 pedal-shaped preset map, and the bank expression assignment.
- Right pane: selected preset metadata, Position 1/2 presentation, toggle trigger, and ordered message editor.
- Bottom status: validation outcome and whether the draft matches the pedal.

The four center cards mirror the physical screen and switch positions. Selecting a bank, page, preset, or position changes editor context; it does not write to the pedal.

### Synchronization model

- Connecting reads device capabilities, versions, active-image metadata, and configuration.
- Edits create an explicit dirty local draft.
- Sync is disabled while validation fails.
- Sync uploads the whole compiled image atomically and verifies the activated checksum before reporting success.
- Navigating away with unsynced changes requires confirmation.
- Import previews schema/version and validation results before replacing the draft.
- Export always writes the current local draft, including unsynced changes, to JSON.

### Light and Dark modes

- Provide an explicit, keyboard-accessible Light/Dark switch in the top bar.
- On first visit, initialize from `prefers-color-scheme`.
- After the user chooses a mode, persist it locally and give it precedence over later operating-system changes.
- Implement both modes from shared semantic tokens.
- Both modes must maintain WCAG AA contrast for text, controls, validation, connection state, and visible focus.

### Required editor states

- No device selected.
- Browser permission denied, with recovery instructions.
- Connected and synchronized.
- Connected with unsynced draft changes.
- Draft validation errors linked to their fields.
- Uploading and verifying, with controls that would invalidate the operation disabled.
- Verification or activation failure, with the previous pedal configuration explicitly reported as intact.
- Firmware/protocol or configuration-schema incompatibility.
- Unexpected USB disconnect, preserving the local draft.

## 10. Fault handling and safety

- Invalid or missing flash configuration never prevents USB configuration access.
- TRS MIDI continues if the USB host is absent or disconnects.
- USB-destined messages are dropped when unavailable; they never block other destinations.
- TRS and USB each have a bounded 64-message MIDI queue. Overflow sets a visible diagnostic and drops the newest message for the affected output without corrupting messages already queued.
- Invalid expression calibration or a disconnected pedal sends no expression MIDI.
- Relay drivers use electrical defaults that keep both contacts open before firmware initialization.
- Watchdog recovery returns to the last valid configuration and Position 1 states.
- Flash writes happen only during explicit editor synchronization or reset, never during live switch, relay, or expression activity.

## 11. Verification strategy

### Firmware unit and integration tests

- Debounce and exact timing boundaries.
- All four chords, constituent-event suppression, and held repeat.
- Press, release, long-press, and double-tap interactions.
- Toggle transition ordering and session persistence.
- Ordered PC, CC, relay, and navigation messages.
- Expression calibration, inversion, filtering, hysteresis, and rate limiting.
- Binary-image validation, slot selection, corrupt images, and interrupted activation.
- Queue overflow and unavailable-output behavior.

### Shared contract tests

- Store golden JSON files and their expected binary images in a language-neutral fixture directory.
- Run the same valid, boundary, and invalid fixtures through the editor encoder and firmware decoder.
- Verify protocol framing, request idempotency, error codes, and schema/version negotiation.

### Editor tests

- Draft editing, validation, import/export, dirty-state protection, and full synchronization states.
- Simulated transport for disconnects, timeouts, corrupt reads, unsupported versions, and activation failure.
- Keyboard operation, focus order, zoom, long labels, and Light/Dark contrast.
- Chromium integration tests using a real or loopback WebSerial transport where automation permits.

### Hardware-in-the-loop tests

- Capture and compare TRS MIDI bytes and USB-MIDI packets.
- Measure press-to-MIDI timing and output ordering.
- Exercise all relay open/close/toggle transitions and confirm fail-open behavior during reset and power loss.
- Sweep valid expression pedals and test unplugging, endpoint noise, and invalid calibration spans.
- Run from USB only, 9 V only, and both; remove either source while the other remains.
- Interrupt configuration upload at multiple offsets and verify the old configuration remains active.
- Capture display snapshots for both toggle positions, every quadrant, expression unavailable, long labels, and error indicators.

### Acceptance criteria

- No lost or reordered messages while neither 64-message output queue is full.
- Normal press-to-MIDI output completes within 50 ms of the initial physical edge, including chord recognition.
- Visible display feedback begins within 100 ms of an accepted action.
- The inactive configuration slot cannot become active without full validation and readback.
- Both relay contacts remain open throughout boot, reset, watchdog recovery, brownout, and unpowered states.
- Removing USB or 9 V while the other source is already present does not reset the controller.
- The editor remains fully usable with keyboard input and at 200% browser zoom on a typical laptop viewport.

## 12. Architecture decision record

### Pico 2 rather than ESP32-S3

Pico 2 was selected because it provides sufficient GPIO, RAM, flash, DMA, ADC, UART, SPI, and native USB without adding an unused radio subsystem. Its development-board form also makes the open-source reference build easier to assemble and replace. ESP32-S3 remains technically capable but offers no required v1 advantage that outweighs its extra platform and RTOS complexity.

### Carrier PCB before an integrated RP2350 board

The first hardware revision uses the standard Pico 2 to reduce board bring-up risk and improve reproducibility. Interfaces and firmware boundaries must avoid assumptions that prevent a later integrated RP2350 board, but that later board is not part of v1.

### Compact device image plus JSON interchange

Human-readable JSON makes backup, review, and community sharing approachable. A compiled binary image keeps boot validation and random bank access deterministic without requiring the firmware to parse and retain the complete JSON document.

### Explicit synchronization rather than live writes

The editor maintains a draft and writes a complete atomic image only on request. This makes configuration state understandable, minimizes flash wear, and prevents a partially edited preset from becoming active on stage.

## 13. Implementation-planning boundary

This specification describes one product, but implementation must proceed through independently verifiable stages rather than attempting the complete system at once:

1. Shared configuration schema, binary image, protocol, and golden fixtures.
2. Bench electronics and Pico 2 hardware-interface proofs.
3. Deterministic firmware engine, persistence, USB composite device, and display.
4. Static browser editor against a simulated transport, followed by real-device integration.
5. Carrier PCB, power validation, enclosure, hardware-in-the-loop qualification, and release documentation.

Each stage must meet its own tests before the next stage depends on it. The implementation plan may subdivide these stages, but it must not merge their verification gates.

## 14. Source references

- [Morningstar MC4 Pro User Manual](https://help.morningstar.io/en/article/mc4-pro-user-manual-1p5o2cp/): reference for the four-switch bank/page interaction, presets, actions, toggle positions, expression behavior, and relay concepts.
- [Raspberry Pi Pico 2 documentation](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.htmlPICO): RP2350 board capabilities, memory, GPIO, ADC, and USB support.
- [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf): native SDK and TinyUSB device support.
- [ESP32-S3 USB Device Stack](https://docs.espressif.com/projects/esp-usb/en/latest/esp32s3/usb_device.html): alternative-platform composite USB capabilities considered during selection.
