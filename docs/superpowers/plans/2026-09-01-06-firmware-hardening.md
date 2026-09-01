# Pico 2 firmware hardening follow-up

> This follow-up executes the five reliability items requested after the WIP firmware baseline. Use Superpowers subagent-driven development and test-driven development for each task.

**Goal:** Make the current Pico 2 firmware safer to run unattended by adding watchdog/reset diagnostics, deterministic expression sampling cadence, persistent expression calibration, live-action upload protection, and useful labeled/diagnostic display output.

**Spec authority:** `docs/superpowers/specs/2026-09-01-midi-controller-pedal-design.md` and the existing firmware interfaces in `docs/superpowers/plans/2026-09-01-02-pico2-firmware.md`.

## Global constraints

- Keep the existing four-switch, MIDI-out-only product scope and pin map unchanged.
- Preserve the non-blocking single-loop architecture; do not add an RTOS, wireless stack, MIDI input, touch handling, or new user controls.
- Watchdog timeout is 2000 ms. GPIO relays must be initialized open before configuration is read, including after a watchdog-caused reboot.
- Expression ADC sampling is scheduled at a 1 ms cadence (1 kHz) and the expression processor's existing 20 ms MIDI rate limit remains authoritative.
- Expression calibration remains global/controller-level state, is accepted only when toe > heel and the span is at least 410 ADC counts, and must survive a `ConfigStore` reconstruction.
- Configuration mutations that can erase/program flash return protocol `BUSY` while a live switch/chord action is executing. The existing idempotent frame behavior and error names remain unchanged.
- Display remains 480x320 landscape, write-only ST7796S, dirty-region only, and no full framebuffer. Labels are printable ASCII and must be clipped to their existing 12-character/20-character limits.
- Existing protocol/image versions, flash slot offsets, queue sizes, and USB identities do not change.
- No physical hardware is available during this pass; hardware-specific behavior must remain behind existing HAL boundaries and be covered by host tests where possible.

## Task 1: Watchdog/reset handling and fixed-rate expression scheduling

**Files:**
- Add `firmware/src/hal/pico_watchdog.hpp` and `.cpp`.
- Modify `firmware/src/main.cpp`, `firmware/src/app/pedal_runtime.hpp`, `firmware/src/app/pedal_runtime.cpp`, `firmware/tests/pico_hal_contract_test.cpp`, `firmware/tests/pedal_runtime_test.cpp`, and `firmware/CMakeLists.txt` as needed.

Implement `PicoWatchdog` with `TimeoutMs == 2000`, a safe `initialize()` that samples `watchdog_caused_reboot()` before enabling the watchdog, and an idempotent `feed()` used once per main-loop iteration. Feed the watchdog after USB/runtime/MIDI services. Pass the reset-cause bit into `PedalRuntime` so it can expose a diagnostic flag in its `LiveView`.

Move expression reads behind a runtime scheduler that requests at most one ADC sample per due 1 ms period, catches up safely after a delayed loop without a burst, and preserves millisecond wraparound behavior. Add tests that prove repeated `tick()` calls at the same timestamp do not read the ADC repeatedly and that a later timestamp resumes the 1 kHz schedule.

Use TDD: add failing tests first, observe the expected failure, implement the smallest change, then run focused tests and the full native suite before committing.

## Task 2: Persistent calibration and upload/live-action busy protection

**Files:**
- Modify `firmware/src/core/config_store.hpp`, `firmware/src/core/config_store.cpp`, `firmware/src/app/controller_protocol_service.hpp`, `firmware/src/app/controller_protocol_service.cpp`, `firmware/src/app/pedal_runtime.hpp`, `firmware/src/app/pedal_runtime.cpp`, `firmware/src/main.cpp`, `firmware/tests/config_store_test.cpp`, `firmware/tests/controller_protocol_service_test.cpp`, and `firmware/tests/pedal_runtime_test.cpp`.

Add a redundant settings record inside each existing metadata sector without changing the image slot layout or metadata image record. Store a generation, heel ADC, toe ADC, and CRC; scan the newest valid settings independently of the active image. Preserve settings when activating a new image, support calibration persistence even before the first image exists, and default to `{0,4095}` when no valid record exists. Expose `ConfigStore::expression_calibration()` and `set_expression_calibration()`.

Add a `LiveActionState`/probe interface implemented by `PedalRuntime`. Mark the state around switch/chord action execution. `ControllerProtocolService` must reject `BEGIN_UPLOAD`, `WRITE_CHUNK`, `VERIFY_UPLOAD`, `ACTIVATE_UPLOAD`, `FACTORY_EMPTY_RESET`, and calibration writes with `ServiceError::Busy` while the probe reports active; otherwise retain the existing error mapping. Wire the probe in `main()` after constructing the runtime. Ensure failed persistence does not leave the in-memory processor on an uncommitted calibration.

Use TDD with reconstruction/power-cut-style settings tests, service busy tests, and runtime state tests. Keep settings writes bounded and non-allocating.

## Task 3: Rich labeled display and diagnostics

**Files:**
- Modify `firmware/src/hal/ports.hpp`, `firmware/src/app/pedal_runtime.hpp`, `firmware/src/app/pedal_runtime.cpp`, `firmware/src/display/live_renderer.hpp`, `firmware/src/display/live_renderer.cpp`, `firmware/tests/live_renderer_test.cpp`, `firmware/tests/display_golden_generator.cpp`, and committed display goldens as needed.

Extend `LiveView` with the bank name, each switch's currently selected position label/accent, the expression assignment label, and a watchdog-reset diagnostic bit. Populate those fields from the active `BankConfig` and `ExpressionProcessor`; use bounded empty/fallback labels when no image is active. Include the new fields in dirty-view comparison.

Update the renderer while preserving the approved geometry: show the bank name and bank/page numbers in the header, render the selected A/B/C/D label and position marker with its configured accent, show the expression assignment label and value/bar or unavailable marker in the footer, and make USB/configuration/queue/watchdog diagnostics legible without changing controls or adding touch. Keep all writes clipped and dirty-only. Update the golden generator/tests to assert label pixels, diagnostics, and unchanged-view zero-transfer behavior.

Use TDD and regenerate goldens only after focused renderer tests demonstrate the intended behavior.

## Verification gate

Run:

```text
pnpm check
cmake --preset pico2-release
cmake --build --preset pico2-release -j2
test -f build/pico2-release/firmware/midi_pedal.uf2
```

Record that physical relay/MIDI/display/power qualification still requires the user's Pico and carrier hardware.
