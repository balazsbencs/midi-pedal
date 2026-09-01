# MIDI Controller Pedal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver a reproducible open-source Pico 2 MIDI foot controller, static Chromium editor, carrier PCB, enclosure files, and release-quality build/flash/assembly documentation.

**Architecture:** Implement the product as five gated workstreams joined by a versioned configuration image and USB protocol. Shared contracts land first; firmware and editor then progress against the same fixtures; carrier hardware follows proven bench circuits; integration and release qualification finish the project.

**Tech Stack:** C++20, Raspberry Pi Pico SDK 2.2.0, TinyUSB, CMake/Ninja/CTest; Node.js 24 LTS, pnpm 10, TypeScript 5.9, React 19.2, Vite 8, Vitest 4.1, Playwright; KiCad 10.0.6; GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-01-midi-controller-pedal-design.md`

## Global Constraints

- Target the non-wireless Raspberry Pi Pico 2 and the Arm Cortex-M33 build; do not add Wi-Fi or Bluetooth dependencies.
- Do not add MIDI input, touch/on-device setup, auxiliary controls, USB host mode, message types beyond PC/CC/relay/navigation, or programmable delays in v1.
- Keep the live firmware single-core and RTOS-free unless measured evidence triggers a separately approved design change.
- Preserve the exact A+C, B+D, A+B, and C+D chord map and the fixed timing constants from the spec.
- Support only PC, CC, relay, and internal navigation messages in v1.
- Keep both relay contacts electrically normally open through boot, reset, watchdog, brownout, and loss of power.
- Use explicit whole-image synchronization; never write flash during live switch or expression activity.
- Editor support is desktop Chromium over WebSerial; the deployed application must be static and backend-free.
- Both editor themes must meet WCAG AA and remain keyboard-operable at 200% zoom.
- Commit generated lockfiles, protocol fixtures, fabrication outputs used for releases, and build instructions; do not commit local build directories.

---

## Repository map

```text
README.md                         Project overview and shortest successful build/flash path
LICENSES/                         MIT, CERN-OHL-S-2.0, and CC-BY-SA-4.0 license texts
docs/
  building.md                     Host toolchain and firmware/editor build instructions
  flashing.md                     UF2 and debug-probe flashing/recovery instructions
  editor-hosting.md               Static hosting and HTTPS/WebSerial requirements
  hardware-assembly.md            Carrier assembly, wiring, inspection, and first power-up
  protocol.md                     Binary image and USB frame reference
  troubleshooting.md              Recovery paths and observable failure modes
  superpowers/specs/              Approved design specification
  superpowers/plans/              This roadmap and phase plans
packages/protocol/                TypeScript config model, validation, codec, frame protocol
protocol/fixtures/                Language-neutral JSON/binary/frame golden vectors
firmware/
  src/core/                       Hardware-independent event/config/expression logic
  src/hal/                        Abstract ports and Pico 2 implementations
  src/usb/                        TinyUSB descriptors and configuration transport
  src/display/                    ST7796S driver and fixed live-screen renderer
  tests/                          Native host tests and Pico integration tests
editor/
  src/domain/                     Draft state and editor use cases
  src/device/                     WebSerial transport and device-session state machine
  src/ui/                         Accessible React workspace and themes
  tests/                          Vitest and Playwright tests
hardware/
  midi-pedal.kicad_*              KiCad project, schematic, and two-layer PCB
  libraries/                      Project-owned symbols, footprints, and STEP mappings
  mechanical/                     Dimensioned panel drawings and enclosure CAD/export
  fabrication/                    Reviewed Gerbers, drills, BOM, and placement files
tools/hil/                        Hardware-in-the-loop capture and qualification tools
.github/workflows/                Contract, firmware, editor, hardware, and release CI
```

## Execution order

### Plan 1: Shared contracts and toolchain

**Plan:** `docs/superpowers/plans/2026-09-01-01-contracts-and-toolchain.md`

Produces a cloneable repository, root README, pinned toolchain, canonical JSON model, binary image codec, USB frame protocol, C++ decoder, golden fixtures, and CI. Nothing in later plans may invent a second representation of these contracts.

**Gate:** TypeScript encoder and C++ decoder pass the same valid, boundary, corrupt, and unsupported-version fixtures on Linux CI.

### Plan 2: Pico 2 firmware

**Plan:** `docs/superpowers/plans/2026-09-01-02-pico2-firmware.md`

Produces the deterministic switch/action engine, expression processing, A/B flash storage, composite CDC + MIDI USB device, TRS MIDI, fail-open relay control, and ST7796S live display. It also creates the build and flashing instructions while those commands are verified.

**Gate:** Native tests pass, a Pico 2 UF2 builds from a clean clone, and bench tests prove switch-to-MIDI, USB enumeration, relay fail-open, expression, display, and recovery behavior.

### Plan 3: Static browser editor

**Plan:** `docs/superpowers/plans/2026-09-01-03-browser-editor.md`

Produces the three-pane editor, Light/Dark themes, local draft model, import/export, WebSerial device session, atomic sync UX, simulated-device tests, and static deployment documentation.

**Gate:** Unit, accessibility, and Playwright tests pass in Chromium; a production build runs from an ordinary static HTTPS host and synchronizes with real firmware.

### Plan 4: Carrier PCB and enclosure

**Plan:** `docs/superpowers/plans/2026-09-01-04-carrier-pcb-and-enclosure.md`

Turns individually proven bench circuits into a KiCad 10 carrier board, BOM, fabrication outputs, aluminum-enclosure panel files, assembly guide, and first-power checklist.

**Gate:** ERC and DRC are clean, independent schematic review passes, fabricated hardware passes current/voltage, MIDI, relay, expression, USB/9 V handover, and thermal tests.

### Plan 5: Integration, documentation, and release

**Plan:** `docs/superpowers/plans/2026-09-01-05-integration-and-release.md`

Adds HIL automation, complete user and contributor documentation, release packaging, licenses, version compatibility, factory-empty configuration, and a repeatable tagged release workflow.

**Gate:** A new builder can follow the README from clean clone to flashed controller and hosted editor; release artifacts reproduce in CI and all acceptance criteria in the design spec have recorded evidence.

## Commit discipline

Each numbered task in the phase plans is a review boundary and ends with one focused commit. Use conventional subjects such as:

```text
build: establish reproducible host toolchain
feat(protocol): encode versioned configuration images
feat(firmware): recognize switch chords deterministically
feat(editor): synchronize validated drafts over WebSerial
feat(hardware): route protected Pico 2 carrier PCB
docs: add build flashing and assembly guides
```

Do not combine firmware, editor, and PCB changes in one commit merely because they participate in one feature. Update a golden contract fixture first, then make each consumer pass it in its own commit.

## Definition of complete

- All five phase gates pass from a clean clone.
- `README.md` contains a ten-minute orientation and direct links to build, flashing, assembly, editor hosting, protocol, and troubleshooting guides.
- `docs/building.md` gives verified Linux, macOS, and Windows prerequisites plus exact commands for host tests, UF2, editor, and all checks.
- `docs/flashing.md` gives verified BOOTSEL/UF2, debug-probe, update, recovery, and factory-empty procedures.
- The release contains firmware UF2/ELF, editor static bundle, JSON schema, default configuration, Gerbers/drills, BOM/placement, panel drawings, checksums, license notices, and a compatibility manifest.
- The repository records evidence for the latency, display, relay safety, power handover, configuration recovery, accessibility, and 200% zoom acceptance criteria.
