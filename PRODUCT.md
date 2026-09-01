# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Stack

Static React 19.2 application built with TypeScript 5.9 and Vite 8, tested with Vitest and Playwright Chromium, and deployable to an ordinary HTTPS host without a backend. Firmware uses C++20 with Raspberry Pi Pico SDK 2.2.0 and TinyUSB; carrier hardware is authored in KiCad 10.0.6.

## Users

Musicians and open-source hardware builders who want an approachable, programmable four-switch MIDI foot controller for pedalboards, desktop rigs, DAWs, and plugins. Builders should be able to understand, assemble, repair, and modify the reference design around a readily available Raspberry Pi Pico 2 development board.

## Product Purpose

Provide the core live experience of a Morningstar MC4 Pro-style controller without its broader feature set: four programmable switches, bank and page navigation, clear live feedback, physical and USB MIDI output, expression control, and two dry-contact relay outputs. Configuration happens off-stage through a directly connected browser editor.

Success means reliable, low-latency live operation; configuration that a musician can understand without reading firmware code; and hardware that open-source builders can reproduce without specialized assembly.

## Positioning

The project pairs a deliberately focused live controller with a transparent, documented configuration model and a module-based reference build. It favors understandable behavior and repairability over feature-count competition.

## Operating Context

- On stage, the controller runs independently from a computer and is powered by a standard 9 V center-negative pedal supply.
- On a desk, USB can power the controller, configure it from a Chromium browser, and carry class-compliant USB-MIDI.
- The editor is hosted as static files and communicates directly with the controller. It has no accounts, cloud storage, analytics, or server-side device access.
- Portable configuration backups are local, human-readable JSON files.

## Capabilities and Constraints

- Raspberry Pi Pico 2 reference controller; wireless hardware and software are out of scope.
- Four footswitches and no additional physical controls or touch input.
- ST7796S 480×320 SPI display with an always-on backlight and a live-performance-only interface.
- Exact MC4 Pro navigation chords: A+C bank down, B+D bank up, A+B page down, and C+D page up, with hold-to-repeat.
- 128 banks, four pages per bank, four presets per page, two toggle states, and up to eight ordered messages per preset.
- Program Change, Control Change, relay control, internal navigation, press, release, long-press, and double-tap are the v1 action scope.
- One TRS voltage-divider expression input with calibration and filtering.
- One 3.5 mm TRS Type-A MIDI output and class-compliant USB-MIDI, selectable per message.
- One 3.5 mm TRS relay connector with two independent, normally-open dry contacts.
- USB or protected 9 V center-negative power.
- Configuration is editor-only. The controller has no setup menu.
- Desktop Chromium-family browsers are the supported editor environment for v1.
- The editor provides explicit Light and Dark modes. The first visit follows the operating-system preference; an explicit user choice is stored locally and takes precedence afterward.

## Evidence on Hand

- Morningstar MC4 Pro user manual is the interaction reference: https://help.morningstar.io/en/article/mc4-pro-user-manual-1p5o2cp/
- The existing display module exposes GND, VCC, SCL, SDA, RST, DC, CS, BL, and SDO-compatible pins and has no touch interface.
- There are no existing brand assets, product claims, benchmarks, or implementation files. Future work must not fabricate them.

## Product Principles

- Live behavior is deterministic; editing, display work, and USB transfers never obstruct switch-to-MIDI execution.
- Keep the performance surface small and predictable; configuration complexity stays off-stage.
- Prefer inspectable protocols, documented formats, replaceable modules, and common components.
- Defaults must be safe: relay contacts fail open, configuration updates are atomic, and reboot state is predictable.
- Add a feature only when it improves the core four-switch controller experience.

## Accessibility & Inclusion

The editor must be keyboard-operable, provide visible focus, use clear language, and never rely on color alone to communicate state. Layout must remain usable at browser zoom and on typical laptop displays.
Both editor themes must maintain WCAG AA text, control, state, and focus contrast.
