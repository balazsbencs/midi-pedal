# Pico 2 Firmware Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and verify deterministic Pico 2 firmware for four switches, expression, TRS/USB MIDI, two fail-open relays, atomic configuration storage, and the ST7796S display.

**Architecture:** Hardware-independent C++ core modules consume timestamped inputs and produce bounded commands through abstract ports. Pico-specific HAL code owns GPIO, ADC, UART, SPI/DMA, flash, watchdog, and TinyUSB; the main loop polls non-blocking services on one core without an RTOS.

**Tech Stack:** C++20, Raspberry Pi Pico SDK 2.2.0, TinyUSB from that SDK, CMake/Ninja, GoogleTest/CTest, Arm GNU Toolchain, picotool, OpenOCD.

**Spec:** `docs/superpowers/specs/2026-09-01-midi-controller-pedal-design.md`

## Global Constraints

- Pin map: switches GP2–GP5; UART1 TX GP8; relays GP10–GP11; display CS GP17, SPI0 SCK GP18, SPI0 MOSI GP19, DC GP20, RST GP21; expression ADC0 GP26.
- Display backlight is hardwired on and SDO is not connected.
- Fixed timing: debounce 20 ms, chord 35 ms from first edge, double tap 300 ms, long press 500 ms, repeat delay 600 ms, repeat interval 200 ms.
- TRS and USB each use a 64-message queue; normal press-to-MIDI is under 50 ms and display response begins under 100 ms.
- Relays initialize open in hardware and software before configuration is read.
- Flash layout on 4 MiB Pico 2: firmware below `0x200000`; slot A `0x200000..0x2EFFFF`; slot B `0x2F0000..0x3DFFFF`; metadata `0x3E0000..0x3FFFFF`; maximum image 768 KiB.

### Current WIP status (2026-09-01)

The host-tested core, A/B configuration store, composite CDC + USB-MIDI
protocol path, Pico flash/USB adapters, embedded factory image, and the
switch/action/expression runtime integration are present on `main`. Physical
relay, MIDI, expression, display, power, and USB enumeration qualification is
still required before a stage-use release; watchdog cadence and persistent
expression settings remain follow-up hardening work.

---

### Task 1: Firmware build, board contract, and safe boot shell

**Files:**
- Add submodule: `third_party/pico-sdk` at tag `2.2.0`
- Create: `firmware/pico_sdk_import.cmake`
- Create: `firmware/src/board/pico2_pins.hpp`
- Create: `firmware/src/hal/ports.hpp`
- Create: `firmware/src/app/controller.hpp`
- Create: `firmware/src/app/controller.cpp`
- Create: `firmware/src/main.cpp`
- Create: `firmware/tests/controller_boot_test.cpp`
- Modify: `firmware/CMakeLists.txt`
- Modify: `CMakePresets.json`

**Interfaces:**
- Consumes: `ImageReader` from Plan 1.
- Produces: `Controller::initialize()` and abstract `Clock`, `MidiPort`, `RelayPort`, `DisplayPort`, `ConfigStore`, and `DiagnosticPort` interfaces.

- [ ] **Step 1: Write the failing safe-boot test**

```cpp
TEST(ControllerBoot, OpensRelaysBeforeReadingConfiguration) {
  FakeHal hal;
  midi::Controller controller(hal);
  controller.initialize();
  EXPECT_EQ(hal.calls[0], "relay1:open");
  EXPECT_EQ(hal.calls[1], "relay2:open");
  EXPECT_LT(hal.index_of("relay2:open"), hal.index_of("config:read"));
}
```

- [ ] **Step 2: Run and observe failure**

Run: `cmake --preset host-debug && cmake --build --preset host-debug && ctest --preset host-debug -R ControllerBoot`

Expected: compile failure because controller and ports do not exist.

- [ ] **Step 3: Define ports and board pins**

```cpp
struct MidiMessage { std::array<uint8_t, 3> bytes{}; uint8_t length{}; };
enum class Destination : uint8_t { Trs, Usb, Both };
struct RelayPort { virtual void set(uint8_t contact, bool closed) = 0; };
struct MidiPort { virtual bool enqueue(Destination, MidiMessage) = 0; };
struct DisplayPort { virtual void present(const LiveView&) = 0; };
```

Add compile-time assertions that all assigned GPIOs are unique and GP26 is ADC capable.

- [ ] **Step 4: Implement minimal safe initialization and empty-config fallback**

`main()` initializes relay GPIOs to inactive before creating other services, then initializes watchdog, config store, controller, USB, UART, ADC, and display. If neither slot is valid, load an in-memory empty configuration and expose CDC immediately.

- [ ] **Step 5: Build host and Pico targets**

Run: `cmake --preset host-debug && cmake --build --preset host-debug && ctest --preset host-debug && cmake --preset pico2-release && cmake --build --preset pico2-release`

Expected: tests pass and `build/pico2-release/firmware/midi_pedal.uf2` exists.

- [ ] **Step 6: Commit**

```bash
git add .gitmodules third_party/pico-sdk firmware CMakePresets.json
git commit -m "build(firmware): boot Pico 2 into a safe controller shell"
```

### Task 2: Deterministic footswitch and chord engine

**Files:**
- Create: `firmware/src/core/switch_engine.hpp`
- Create: `firmware/src/core/switch_engine.cpp`
- Create: `firmware/tests/switch_engine_test.cpp`

**Interfaces:**
- Consumes: raw switch bitmask and monotonic milliseconds.
- Produces: `SwitchEvent { EventKind kind; SwitchId switch_id; Chord chord; uint32_t at_ms; }` through `SwitchEngine::update(uint8_t raw_mask, uint32_t now_ms)`.

- [ ] **Step 1: Write a table-driven failing timing test**

```cpp
TEST(SwitchEngine, SuppressesAAndCWhenBankDownChordStabilizes) {
  SwitchEngine engine;
  engine.update(mask(A), 0);
  engine.update(mask(A) | mask(C), 12);
  auto events = engine.update(mask(A) | mask(C), 35);
  EXPECT_THAT(events, ElementsAre(ChordEvent(Chord::BankDown)));
}

TEST(SwitchEngine, EmitsSinglePressAtChordDeadline) {
  SwitchEngine engine;
  engine.update(mask(B), 0);
  EXPECT_TRUE(engine.update(mask(B), 34).empty());
  EXPECT_THAT(engine.update(mask(B), 35), ElementsAre(PressEvent(B)));
}
```

- [ ] **Step 2: Run and observe failure**

Run: `ctest --preset host-debug -R SwitchEngine --output-on-failure`

Expected: test executable is absent or fails to compile.

- [ ] **Step 3: Implement debounce and chord recognition as explicit states**

Use `Idle`, `ChordCandidate`, `SingleActive`, and `ChordActive`. Start the 35 ms chord deadline at the first physical edge; accept only switch levels stable for 20 ms; map only the four approved chords; suppress press/release/long/double events while `ChordActive`.

- [ ] **Step 4: Add gesture and wraparound tests**

Test release, press-plus-long, press/press-plus-double, 600/200 ms hold repeat, bank 1↔128 wrap, page 1↔4 wrap, near-simultaneous invalid combinations, contact bounce, and 32-bit millisecond wraparound.

- [ ] **Step 5: Run all switch tests and measure runtime**

Run: `ctest --preset host-debug -R SwitchEngine --output-on-failure`

Expected: all cases pass; a 100,000-update benchmark completes without allocation.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/core/switch_engine.* firmware/tests/switch_engine_test.cpp
git commit -m "feat(firmware): recognize switch gestures and navigation chords"
```

### Task 3: Action engine, toggle state, and output queues

**Files:**
- Create: `firmware/src/core/action_engine.hpp`
- Create: `firmware/src/core/action_engine.cpp`
- Create: `firmware/src/core/midi_queue.hpp`
- Create: `firmware/tests/action_engine_test.cpp`
- Create: `firmware/tests/midi_queue_test.cpp`
- Modify: `firmware/src/app/controller.cpp`

**Interfaces:**
- Consumes: `SwitchEvent`, current `BankConfig`, and the compact 2048-preset toggle bitset.
- Produces: ordered `MidiMessage`, `RelayCommand`, `NavigationCommand`, dirty `LiveView`, and diagnostics.

- [ ] **Step 1: Write failing ordering and state tests**

```cpp
TEST(ActionEngine, QueuesMatchingSlotsBeforeToggling) {
  auto preset = preset_with_cc_then_relay_and_toggle_on(Trigger::Press);
  EngineState state{};
  auto result = run_press(preset, state);
  EXPECT_THAT(result.commands, ElementsAre(Cc(1, 17, 127, Destination::Both), RelayClose(1)));
  EXPECT_EQ(state.position(preset_id), Position::Two);
}
```

- [ ] **Step 2: Run and observe failure**

Run: `ctest --preset host-debug -R 'ActionEngine|MidiQueue' --output-on-failure`

Expected: compile failure because engines are absent.

- [ ] **Step 3: Implement exact filtering and MIDI encoding**

Filter slots by trigger and pre-transition position; encode PC as `0xC0 | channel-1, program` and CC as `0xB0 | channel-1, controller, value`; append commands in slot order; transition only after every command is accepted or diagnosed.

- [ ] **Step 4: Implement separate fixed 64-entry TRS and USB queues**

`Destination::Both` attempts both queues independently. If one queue is full or USB unavailable, preserve the other output and increment `Diagnostic::TrsQueueOverflow`, `UsbQueueOverflow`, or `UsbUnavailable`; never overwrite an older entry.

- [ ] **Step 5: Test full capacity and every message variant**

Run: `ctest --preset host-debug -R 'ActionEngine|MidiQueue' --output-on-failure`

Expected: exact bytes/order, navigation, relay, session state retention, boot reset, and overflow policies pass.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/core firmware/tests firmware/src/app/controller.cpp
git commit -m "feat(firmware): execute ordered preset actions"
```

### Task 4: Expression calibration and filtering

**Files:**
- Create: `firmware/src/core/expression.hpp`
- Create: `firmware/src/core/expression.cpp`
- Create: `firmware/tests/expression_test.cpp`
- Modify: `firmware/src/app/controller.cpp`

**Interfaces:**
- Consumes: 12-bit ADC samples at 1 kHz, global heel/toe calibration, and current bank assignment.
- Produces: optional CC command and `ExpressionView { available, value }`.

- [ ] **Step 1: Write failing filter/rate tests**

```cpp
TEST(Expression, FiltersSpikeAndRateLimitsChanges) {
  ExpressionProcessor p(Calibration{400, 3600});
  feed(p, {400, 402, 4095, 401, 403}, 0);
  EXPECT_EQ(p.view().value, 0);
  EXPECT_FALSE(p.sample(2000, 10).has_value());
  EXPECT_TRUE(p.sample(2000, 20).has_value());
}
```

- [ ] **Step 2: Run and observe failure**

Run: `ctest --preset host-debug -R Expression --output-on-failure`

Expected: compile failure because processor is absent.

- [ ] **Step 3: Implement specified signal path**

Maintain a rolling five-sample median, IIR `filtered += 0.2f * (median-filtered)`, calibrated clamp/map, inversion, output range, one-value hysteresis, and a 20 ms minimum MIDI interval. Treat calibration spans below 410 ADC counts as invalid.

- [ ] **Step 4: Add endpoint/noise/disconnect tests**

Cover exact heel/toe, reversed calibration rejection, inversion, minimum/maximum swap, jitter around a MIDI boundary, unplug/open-circuit samples, bank change, disabled assignment, and TRS/USB/both destinations.

- [ ] **Step 5: Run expression and action tests**

Run: `ctest --preset host-debug -R 'Expression|ActionEngine' --output-on-failure`

Expected: all tests pass and no unchanged CC is emitted.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/core/expression.* firmware/tests/expression_test.cpp firmware/src/app/controller.cpp
git commit -m "feat(firmware): process calibrated expression input"
```

### Task 5: Atomic A/B flash configuration store

**Files:**
- Create: `firmware/src/core/config_store.hpp`
- Create: `firmware/src/core/config_store.cpp`
- Create: `firmware/src/hal/pico_flash_port.hpp`
- Create: `firmware/src/hal/pico_flash_port.cpp`
- Create: `firmware/tests/config_store_test.cpp`
- Modify: `firmware/src/app/controller.cpp`

**Interfaces:**
- Produces: `begin_upload(size, sequence)`, `write_chunk(offset, bytes)`, `verify_upload()`, `activate_upload()`, `active_info()`, and `load_bank(index)`.
- Consumes: `FlashPort` with erase/program/read and Plan 1 `ImageReader`.

- [ ] **Step 1: Write failing power-cut sequence tests**

Create a `FakeFlash` that can stop after each erase/program operation. For every cut point, reconstruct `ConfigStore` and assert it selects the previous slot until new image readback and metadata activation both finish.

- [ ] **Step 2: Run and observe failure**

Run: `ctest --preset host-debug -R ConfigStore --output-on-failure`

Expected: compile failure because store is absent.

- [ ] **Step 3: Implement slot and metadata records**

Use two redundant metadata sectors with magic, metadata version, generation, active slot, active image sequence/length/CRC, and metadata CRC. Write image chunks only to the inactive slot. Verify using flash readback and `ImageReader`; activate by writing the next metadata generation to the older metadata sector.

- [ ] **Step 4: Implement Pico flash operations safely**

Use SDK flash-safe execution, 4096-byte erase alignment, 256-byte program alignment, and interrupts disabled only inside bounded erase/program calls. Refuse uploads during a live action and expose busy status over CDC.

- [ ] **Step 5: Run exhaustive interruption tests**

Run: `ctest --preset host-debug -R ConfigStore --output-on-failure`

Expected: valid newest slot, corrupt newest fallback, both invalid empty fallback, duplicate activation idempotency, misaligned write rejection, and every simulated cut point pass.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/core/config_store.* firmware/src/hal/pico_flash_store.* firmware/tests/config_store_test.cpp firmware/src/app/controller.cpp
git commit -m "feat(firmware): activate configurations atomically"
```

### Task 6: Composite CDC and USB-MIDI device

**Files:**
- Create: `firmware/src/usb/usb_descriptors.c`
- Create: `firmware/src/usb/usb_transport.hpp`
- Create: `firmware/src/usb/usb_transport.cpp`
- Create: `firmware/src/core/frame_decoder.hpp`
- Create: `firmware/src/core/frame_decoder.cpp`
- Create: `firmware/tests/frame_decoder_test.cpp`
- Create: `firmware/tests/usb_descriptor_test.cpp`
- Modify: `firmware/CMakeLists.txt`

**Interfaces:**
- Consumes: frame contract from Plan 1, `ConfigStore`, expression calibration, and USB MIDI queue.
- Produces: TinyUSB composite device with one MIDI streaming interface and one CDC ACM interface.

- [ ] **Step 1: Write failing descriptor and frame-fixture tests**

Assert one configuration exposes CDC control/data plus Audio/MIDI interfaces, endpoint numbers fit RP2350 limits, strings include product and serial, and C++ frame decoding matches every `protocol/fixtures/frames` vector.

- [ ] **Step 2: Run and observe failure**

Run: `ctest --preset host-debug -R 'UsbDescriptor|FrameDecoder' --output-on-failure`

Expected: compile failure because descriptors and decoder are absent.

- [ ] **Step 3: Implement TinyUSB descriptors and non-blocking tasks**

Assign CDC bulk endpoints and one MIDI IN endpoint, service `tud_task()` every main-loop iteration, drain USB MIDI only when mounted/ready, and parse CDC bytes incrementally without waiting. Use Pico unique board id for USB serial text.

- [ ] **Step 4: Dispatch all protocol commands idempotently**

Connect capability, metadata, read, staged upload, verify, activate, calibration, and factory-empty commands to controller services. Cache the final encoded response for the most recent 16 request ids.

- [ ] **Step 5: Verify on host and Pico 2**

Run host tests, flash the UF2, then verify `GET_CAPABILITIES`, MIDI enumeration, and a three-byte note-free CC packet using `lsusb`/Device Manager/System Information and a MIDI monitor. Record USB VID/PID selection procedure in `docs/protocol.md` before public release.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/usb firmware/src/core/frame_decoder.* firmware/tests firmware/CMakeLists.txt docs/protocol.md
git commit -m "feat(firmware): expose composite USB MIDI and configuration"
```

### Task 7: Pico GPIO/UART/ADC/relay HAL and fail-open behavior

**Files:**
- Create: `firmware/src/hal/pico_switches.hpp`
- Create: `firmware/src/hal/pico_switches.cpp`
- Create: `firmware/src/hal/pico_uart_midi.hpp`
- Create: `firmware/src/hal/pico_uart_midi.cpp`
- Create: `firmware/src/hal/pico_expression_adc.hpp`
- Create: `firmware/src/hal/pico_expression_adc.cpp`
- Create: `firmware/src/hal/pico_relays.hpp`
- Create: `firmware/src/hal/pico_relays.cpp`
- Create: `firmware/src/app/pedal_runtime.hpp`
- Create: `firmware/src/app/pedal_runtime.cpp`
- Create: `firmware/tests/pico_hal_contract_test.cpp`
- Create: `firmware/tests/pedal_runtime_test.cpp`
- Modify: `firmware/src/main.cpp`

**Interfaces:**
- Implements the ports from Task 1 and feeds the core at fixed cadence.

- [ ] **Step 1: Add failing HAL contract tests**

Compile Pico HAL against fake SDK shims and assert relay GPIO direction/value order, UART 31,250 baud 8N1, ADC0/GP26 selection, switch pull states, and MIDI queue draining without blocking.

- [ ] **Step 2: Run and observe failure**

Run: `ctest --preset host-debug -R PicoHalContract --output-on-failure`

Expected: compile failure because HAL modules are absent.

- [ ] **Step 3: Implement hardware services**

Initialize relays inactive before setting output direction; use UART1 at 31,250 baud; sample ADC from a repeating timer/FIFO at 1 kHz; read switch GPIO as one bitmask; drain at most one MIDI message per ready UART opportunity to keep the loop non-blocking.

- [ ] **Step 4: Bench-test electrical behavior**

Use a logic analyzer to confirm MIDI baud/bytes and press latency; use continuity mode to verify both relay contacts remain open during reset button hold, BOOTSEL boot, watchdog reset, power ramp, and unplug; record results under `hardware/validation/firmware-bench.md`.

- [ ] **Step 5: Run tests and Pico release build**

Run: `pnpm check && cmake --preset pico2-release && cmake --build --preset pico2-release`

Expected: all host tests pass and release UF2/ELF/map files build.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/hal firmware/src/main.cpp firmware/tests hardware/validation/firmware-bench.md
git commit -m "feat(firmware): drive Pico 2 pedal interfaces"
```

### Task 8: ST7796S driver and fixed live renderer

**Files:**
- Create: `firmware/src/display/st7796s.hpp`
- Create: `firmware/src/display/st7796s.cpp`
- Create: `firmware/src/display/live_renderer.hpp`
- Create: `firmware/src/display/live_renderer.cpp`
- Create: `firmware/src/display/font_ascii.hpp`
- Create: `firmware/src/display/font_ascii.cpp`
- Create: `firmware/tests/live_renderer_test.cpp`
- Create: `firmware/tests/golden/display/factory-empty.rgb565`
- Create: `firmware/tests/golden/display/toggle-position-1.rgb565`
- Create: `firmware/tests/golden/display/toggle-position-2.rgb565`
- Create: `firmware/tests/golden/display/expression-unavailable.rgb565`
- Create: `firmware/tests/golden/display/recoverable-error.rgb565`
- Modify: `firmware/src/main.cpp`

**Interfaces:**
- Consumes: `LiveView` containing bank/page, four position views, expression, USB/config diagnostic indicators.
- Produces: clipped dirty rectangles sent over SPI0/DMA; no full framebuffer.

- [ ] **Step 1: Write failing geometry and snapshot tests**

Assert header/footer heights, A/B/C/D quadrant coordinates, 12-character labels, Position 1/2 text markers, unavailable expression state, and unchanged-view zero-transfer behavior against RGB565 golden regions.

- [ ] **Step 2: Run and observe failure**

Run: `ctest --preset host-debug -R LiveRenderer --output-on-failure`

Expected: compile failure because renderer is absent.

- [ ] **Step 3: Implement ST7796S write-only SPI/DMA**

Initialize 480×320 landscape orientation, RGB565 color order, CS/DC/RST control, 32 MHz SPI, address-window writes, and one 4 KiB line/tile buffer. Queue DMA only after the previous transfer completes.

- [ ] **Step 4: Implement the approved 2×2 layout**

Render bank header, B/P numbers, A–D quadrants with fixed white text plus accent/state indicator, and expression footer with label/value/bar or unavailable marker. Add brief non-modal text/icon indicators for USB connection, configuration activation, queue overflow, and recoverable configuration errors. Update only regions whose view model changed.

- [ ] **Step 5: Verify snapshots and physical display**

Run host snapshot tests, flash Pico 2, photograph all required states, and record measured full-screen/dirty update times in `hardware/validation/display.md`. Dirty action feedback must begin within 100 ms.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/display firmware/tests firmware/src/main.cpp hardware/validation/display.md
git commit -m "feat(firmware): render the live pedal display"
```

### Task 9: Verified firmware build and flashing documentation

**Files:**
- Create: `docs/flashing.md`
- Create: `firmware/defaults/factory-empty.json`
- Create: `firmware/defaults/factory-empty.bin`
- Modify: `docs/building.md`
- Modify: `README.md`
- Create: `.github/workflows/firmware.yml`

**Interfaces:**
- Produces: clean-clone UF2 build, BOOTSEL and debug-probe procedures, recovery flow, and CI artifact.

- [ ] **Step 1: Test the documented clean-clone commands in a temporary clone**

```bash
git clone --recurse-submodules . /tmp/midi-pedal-doc-test
cmake --preset pico2-release -S /tmp/midi-pedal-doc-test
cmake --build --preset pico2-release
```

Expected before documentation changes: a reader cannot discover all prerequisites or the UF2 path from README.

- [ ] **Step 2: Write `docs/flashing.md`**

Document:

```text
BOOTSEL/UF2: unplug, hold BOOTSEL, connect USB, release, copy midi_pedal.uf2 to RP2350, wait for reboot.
picotool: picotool load -f build/pico2-release/firmware/midi_pedal.uf2 && picotool reboot.
Debug probe: SWD wiring, OpenOCD command, GDB load/reset command.
Recovery: reflash firmware; reconnect editor; issue factory-empty reset; resync JSON backup.
Verification: USB CDC + MIDI enumerate, empty 2×2 screen appears, both relays measure open.
```

Include Linux, macOS, and Windows device-volume notes and warnings about 9 V polarity and relay/expression/MIDI jack identity.

- [ ] **Step 3: Append exact firmware commands to `docs/building.md` and README**

List tool versions, submodule initialization, Debug/Release preset commands, native tests, output file locations, map-size inspection, and direct links to flashing and troubleshooting.

- [ ] **Step 4: Add firmware CI**

Build native tests and `pico2-release`, upload UF2/ELF/map as CI artifacts, verify factory-empty JSON regenerates the committed binary without diff, and fail when firmware exceeds the 2 MiB boundary.

- [ ] **Step 5: Run the firmware phase gate**

Run: `pnpm check && cmake --preset pico2-release && cmake --build --preset pico2-release && test -f build/pico2-release/firmware/midi_pedal.uf2`

Expected: all tests pass and UF2 exists at the documented path.

- [ ] **Step 6: Commit**

```bash
git add README.md docs/building.md docs/flashing.md firmware/defaults .github/workflows/firmware.yml
git commit -m "docs: add verified firmware build and flashing guide"
```
