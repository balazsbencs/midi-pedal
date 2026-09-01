# Building

This guide will track commands verified from a clean clone. The supported host is a desktop Linux, macOS, or Windows machine with Node.js 24, Corepack/pnpm 10, CMake 3.25 or newer, Ninja, Git, an Arm GNU Toolchain, and the Pico SDK 2.2.0.

## Host setup

Install Node.js 24 from the official Node.js distribution, enable Corepack, and select the repository version:

```bash
corepack enable
corepack prepare pnpm@10.15.0 --activate
pnpm --version
```

Install CMake and Ninja from your operating system package manager, install the Arm GNU Toolchain, and make sure `cmake`, `ninja`, `arm-none-eabi-gcc`, and `git` are on `PATH`. Windows users should run these commands in PowerShell or Git Bash with the tools' `bin` directories on `PATH`.

## Clean clone and checks

```bash
git clone --recurse-submodules git@github.com:balazsbencs/midi-pedal.git
cd midi-pedal
corepack enable
pnpm install --frozen-lockfile
pnpm check
```

The complete command builds host tests and runs the TypeScript and C++ suites. Build directories are generated under `build/` and must not be committed.

To regenerate the language-neutral configuration images after editing a JSON fixture:

```bash
pnpm --filter @midi-pedal/protocol fixtures
git diff --exit-code protocol/fixtures
```

The binary layout and USB frame contract are documented in [protocol.md](protocol.md).

## Current implementation status

Firmware, editor, protocol fixtures, and fabrication commands are added phase by phase. Once those phases land, this guide will contain the exact Debug and Release artifact paths, Pico SDK environment variables, editor typecheck/test/build commands, HIL simulation command, and one all-check command. If a command here differs from the phase plan, use the phase plan until this guide is updated and verified.
