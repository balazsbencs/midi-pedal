# Building

The project has two build surfaces: native contract tests (always available)
and the Pico 2 firmware (requires the Arm compiler and the pinned Pico SDK).
The browser editor is a separate static Vite package. The commands below are
the source of truth for CI and are intended for Linux, macOS, and Windows
(PowerShell or Git Bash).

## Prerequisites

- Git 2.40 or newer, with submodule support
- Node.js 24 LTS and Corepack
- pnpm 10.15.0
- CMake 3.25 or newer and Ninja
- Arm GNU Toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-g++`, `arm-none-eabi-newlib`) for firmware

The Pico SDK is pinned as `third_party/pico-sdk` at 2.2.0; do not replace it
with an unpinned system copy when reproducing a release. On Debian/Ubuntu,
the common host packages are:

```bash
sudo apt-get install cmake ninja-build gcc-arm-none-eabi libstdc++-arm-none-eabi-dev libstdc++-arm-none-eabi-newlib libnewlib-arm-none-eabi
```

On macOS install the equivalent packages with Homebrew. On Windows, install
the Arm GNU Toolchain and CMake/Ninja, then put their `bin` directories on
`PATH`. Verify the tools before continuing:

```bash
node --version          # v24.x
corepack enable
corepack prepare pnpm@10.15.0 --activate
pnpm --version          # 10.15.0
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

## Clone and install

```bash
git clone --recurse-submodules git@github.com:balazsbencs/midi-pedal.git
cd midi-pedal
git submodule update --init --recursive
corepack enable
pnpm install --frozen-lockfile
```

## Native checks

```bash
pnpm check:toolchain
pnpm --filter @midi-pedal/protocol fixtures
git diff --exit-code protocol/fixtures
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug --output-on-failure
```

`pnpm test:ts` runs every workspace TypeScript test and `pnpm test:cpp` runs
the commands above. `pnpm check` combines the toolchain, TypeScript, and C++
checks. Native build output is under `build/host-debug/` and is not part of a
release.

## Pico 2 firmware

Configure and build the release target from a clean checkout:

```bash
cmake --preset pico2-release
cmake --build --preset pico2-release
```

The release outputs are:

```text
build/pico2-release/firmware/midi_pedal.uf2  # BOOTSEL drag-and-drop image
build/pico2-release/firmware/midi_pedal.elf  # debugger image
build/pico2-release/firmware/midi_pedal.hex  # probe/programmer image
build/pico2-release/firmware/midi_pedal.elf.map
```

The linker script keeps firmware below the 2 MiB configuration boundary. To
inspect the image before flashing:

```bash
arm-none-eabi-size build/pico2-release/firmware/midi_pedal.elf
stat -c '%s bytes' build/pico2-release/firmware/midi_pedal.bin   # Linux
```

Use `Get-Item ... .Length` for the equivalent PowerShell size check. Follow
[flashing.md](flashing.md) for BOOTSEL, picotool, debug-probe, and recovery
procedures. The checked-in factory image can be regenerated with:

```bash
pnpm factory:empty
git diff --exit-code firmware/defaults/factory-empty.bin
```

## Editor package

Once the editor package is present, build and test the static site with:

```bash
pnpm --filter @midi-pedal/editor typecheck
pnpm --filter @midi-pedal/editor test
pnpm --filter @midi-pedal/editor build
```

The deployable files are in `editor/dist/`; no server-side runtime is
required. See [editor-hosting.md](editor-hosting.md) for HTTPS/WebSerial
requirements.

## Reproducibility notes

- Do not commit `build/`, `editor/dist/`, or dependency caches.
- Keep `pnpm-lock.yaml` and the Pico SDK submodule pointer in commits that
  change tool versions.
- A physical display, MIDI monitor, relays, and expression pedal are required
  for the electrical checks in `hardware/validation/`; native tests cannot
  prove those measurements.
