# MIDI Pedal

An open-source four-switch MIDI foot controller built around a Raspberry Pi Pico 2. It follows the focused live interaction model of the Morningstar MC4 Pro while keeping the hardware, protocol, and editor approachable for builders.

## Project status

This repository is work in progress. The approved design and implementation
roadmap are complete; firmware, editor, carrier PCB, enclosure files, and
qualification artifacts are being built in phases. Do not use an unqualified
revision on stage.

The v1 boundary is four normally-open footswitches, an ST7796S 480×320 display, TRS Type-A MIDI OUT, class-compliant USB-MIDI, one expression input, and two isolated normally-open relay contacts. There is no MIDI input, wireless feature, touch input, auxiliary control, USB host mode, or on-device setup menu.

## Start here

```bash
git clone --recurse-submodules git@github.com:balazsbencs/midi-pedal.git
cd midi-pedal
corepack enable
pnpm install --frozen-lockfile
pnpm check
```

Read the [approved design](docs/superpowers/specs/2026-09-01-midi-controller-pedal-design.md) and [implementation roadmap](docs/superpowers/plans/2026-09-01-midi-controller-pedal-implementation-plan.md) before building hardware.

The portable configuration and USB frame contracts are documented in [docs/protocol.md](docs/protocol.md). The phase plans define the exact firmware, editor, hardware, and release gates while implementation is in progress.

## Repository map

- `packages/protocol/` — shared JSON schema, binary image, and USB frame contracts.
- `firmware/` — Pico 2 C++20 firmware and native tests.
- `editor/` — static React editor for desktop Chromium browsers.
- `hardware/` — KiCad carrier PCB, enclosure drawings, and fabrication outputs.
- `docs/` — build, flashing, hosting, assembly, protocol, troubleshooting, and acceptance guides.
- `tools/` — validation, HIL, and release tooling.

Detailed guides: [building](docs/building.md), [flashing](docs/flashing.md), [editor hosting](docs/editor-hosting.md), [hardware assembly](docs/hardware-assembly.md), [protocol](docs/protocol.md), and [troubleshooting](docs/troubleshooting.md).

## Build and flash

Use the verified [build guide](docs/building.md) and [flashing guide](docs/flashing.md). Power hardware only after reading the 9 V center-negative polarity and relay-contact warnings in the [assembly guide](docs/hardware-assembly.md).

The editor is hosted as static files; see [editor hosting](docs/editor-hosting.md).
The browser connects directly to the pedal over WebSerial and never requires a
project account or backend. Configuration is intentionally prepared off-stage;
the pedal has no touch setup or wireless feature.

## License

The repository uses MIT for software, CERN-OHL-S-2.0 for hardware design, and
CC-BY-SA-4.0 for documentation. See [LICENSE](LICENSE) and the notices under
`LICENSES/` as they land.
